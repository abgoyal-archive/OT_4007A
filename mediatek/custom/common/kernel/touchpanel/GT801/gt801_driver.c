#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/byteorder/generic.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif 
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/rtpm_prio.h>
#include <linux/dma-mapping.h>

#include <linux/syscalls.h>
#include <linux/reboot.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/completion.h>
#include <asm/uaccess.h>

#include "tpd_custom_gt801.h"
#include "tpd.h"
#include <cust_eint.h>

#ifndef TPD_NO_GPIO 
#include "cust_gpio_usage.h"
#endif

#define ABS(x)                  ((x<0)?-x:x)


extern struct tpd_device *tpd;

static int tpd_flag = 0;
static int tpd_halt=0;
static struct task_struct *thread = NULL;
static DECLARE_WAIT_QUEUE_HEAD(waiter);

#ifdef TPD_HAVE_BUTTON 
static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;
#endif
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
static int tpd_wb_start_local[TPD_WARP_CNT] = TPD_WARP_START;
static int tpd_wb_end_local[TPD_WARP_CNT]   = TPD_WARP_END;
#endif
#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
static int tpd_calmat_local[8]     = TPD_CALIBRATION_MATRIX;
static int tpd_def_calmat_local[8] = TPD_CALIBRATION_MATRIX;
#endif

static void tpd_eint_interrupt_handler(void);
static int touch_event_handler(void *unused);
static int tpd_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int tpd_i2c_remove(struct i2c_client *client);
extern void mt65xx_eint_unmask(unsigned int line);
extern void mt65xx_eint_mask(unsigned int line);
extern void mt65xx_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 mt65xx_eint_set_sens(kal_uint8 eintno, kal_bool sens);
extern void mt65xx_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
                                     kal_bool auto_umask);

static struct i2c_client *i2c_client = NULL;
static const struct i2c_device_id tpd_i2c_id[] = {{"mtk-tpd",0},{}};
static unsigned short force[] = {0, 0xAA, I2C_CLIENT_END,I2C_CLIENT_END};
static const unsigned short * const forces[] = { force, NULL };
static struct i2c_client_address_data addr_data = { .forces = forces,};
struct i2c_driver tpd_i2c_driver = {                       
    .probe = tpd_i2c_probe,                                   
    .remove = tpd_i2c_remove,                           
    .detect = tpd_i2c_detect,                           
    .driver.name = "mtk-tpd", 
    .id_table = tpd_i2c_id,                             
    .address_data = &addr_data,                        
}; 

static int i2c_read_bytes( struct i2c_client *client, u8 addr, u8 *rxbuf, int len );
static int i2c_write_bytes( struct i2c_client *client, u16 addr, u8 *txbuf, int len );

#ifdef CONFIG_TOUCHSCREEN_GOODIX_IAP
struct i2c_client * i2c_connect_client = NULL; 
static struct proc_dir_entry *gt801_config_proc = NULL;
static int gt801_config_write_proc(struct file *filp, const char __user *buff, unsigned long len, void *data);
static int gt801_config_read_proc( char *page, char **start, off_t off, int count, int *eof, void *data );

struct goodix_ts_data {
	uint16_t addr;
	uint8_t bad_data;
	struct i2c_client *client;
	struct input_dev *input_dev;
	int use_reset;		//use RESET flag
	int use_irq;		//use EINT flag
	int read_mode;		//read moudle mode,20110221 by andrew
	struct hrtimer timer;
	struct work_struct  work;
	char phys[32];
	int retry;
	struct early_suspend early_suspend;
	int (*power)(struct goodix_ts_data * ts, int on);
};
#endif

static u8 cfg_data[] =
{
	0x00,0x04,0x00,0x02,0x58,0x0A,0xA1,0x1E,
	0xE7,0x32,0x02,0x08,0x10,0x4C,0x4F,0x4F,
	0x20,0x00,0x01,0x80,0x80,0x40,0x80,0x4F,
	0x80,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
	0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x00,0x01,
	0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
	0x0A,0x0B,0x0C,0x0D,0x0E,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00
};


#ifdef CONFIG_TOUCHSCREEN_GOODIX_IAP
static unsigned int Reflect(unsigned long int ref, char ch)
{
	unsigned int value=0;
	int i;
	for(i = 1; i < (ch + 1); i++)
	{
		if(ref & 1)
			value |= 1 << (ch - i);
		ref >>= 1;
	}
	return value;
}
/*---------------------------------------------------------------------------------------------------------*/
/*  CRC Check Program INIT								                                           		   */
/*---------------------------------------------------------------------------------------------------------*/
static void init_crc32_table(void)
{
	unsigned int temp;
	unsigned int t1,t2;
	unsigned int flag;
	int i,j;
	for(i = 0; i <= 0xFF; i++)
	{
		temp=Reflect(i, 8);
		crc32_table[i]= temp<< 24;
		for (j = 0; j < 8; j++)
		{

			flag=crc32_table[i]&0x80000000;
			t1=(crc32_table[i] << 1);
			if(flag==0)
				t2=0;
			else
				t2=ulPolynomial;
			crc32_table[i] =t1^t2 ;

		}
		crc32_table[i] = Reflect(crc32_table[i], 32);
	}
}
/*---------------------------------------------------------------------------------------------------------*/
/*  CRC main Program									                                           		   */
/*---------------------------------------------------------------------------------------------------------*/
static void GenerateCRC32(unsigned char * buf, unsigned int len)
{
	unsigned int i;
	unsigned int t;

	for (i = 0; i != len; ++i)
	{
		t = (oldcrc32 ^ buf[i]) & 0xFF;
		oldcrc32 = ((oldcrc32 >> 8) & 0xFFFFFF) ^ crc32_table[t];
	}
}

static struct file * update_file_open(char * path, mm_segment_t * old_fs_p)
{
	struct file * filp = NULL;
	int errno = -1;
		
	filp = filp_open(path, O_RDONLY, 0644);
	
	if(!filp || IS_ERR(filp))
	{
		if(!filp)
			errno = -ENOENT;
		else 
			errno = PTR_ERR(filp);					
		printk(KERN_ERR "The update file for Guitar open error.\n");
		return NULL;
	}
	*old_fs_p = get_fs();
	set_fs(get_ds());

	filp->f_op->llseek(filp,0,0);
	return filp ;
}

static void update_file_close(struct file * filp, mm_segment_t old_fs)
{
	set_fs(old_fs);
	if(filp)
		filp_close(filp, NULL);
}
static int update_get_flen(char * path)
{
	struct file * file_ck = NULL;
	mm_segment_t old_fs;
	int length ;
	
	file_ck = update_file_open(path, &old_fs);
	if(file_ck == NULL)
		return 0;

	length = file_ck->f_op->llseek(file_ck, 0, SEEK_END);
	//printk("File length: %d\n", length);
	if(length < 0)
		length = 0;
	update_file_close(file_ck, old_fs);
	return length;	
}
static int update_file_check(char * path)
{
	unsigned char buffer[64] = { 0 } ;
	struct file * file_ck = NULL;
	mm_segment_t old_fs;
	int count, ret, length ;
	
	file_ck = update_file_open(path, &old_fs);
	
	if(path != NULL)
		printk("File Path:%s\n", path);
	
	if(file_ck == NULL)
		return -ERROR_NO_FILE;

	length = file_ck->f_op->llseek(file_ck, 0, SEEK_END);
#ifdef GUITAR_MESSAGE
	printk(KERN_INFO "gt801 update: File length: %d\n",length);
#endif	
	if(length <= 0 || (length%4) != 0)
	{
		update_file_close(file_ck, old_fs);
		return -ERROR_FILE_TYPE;
	}
	
	//set file point to the begining of the file
	file_ck->f_op->llseek(file_ck, 0, SEEK_SET);	
	oldcrc32 = 0xFFFFFFFF;
	init_crc32_table();
	while(length > 0)
	{
		ret = file_ck->f_op->read(file_ck, buffer, sizeof(buffer), &file_ck->f_pos);
		if(ret > 0)
		{
			for(count = 0; count < ret;  count++) 	
				GenerateCRC32(&buffer[count],1);			
		}
		else 
		{
			update_file_close(file_ck, old_fs);
			return -ERROR_FILE_READ;
		}
		length -= ret;
	}
	oldcrc32 = ~oldcrc32;
#ifdef GUITAR_MESSAGE	
	printk("CRC_Check: %u\n", oldcrc32);
#endif	
	update_file_close(file_ck, old_fs);
	return 1;	
}

unsigned char wait_slave_ready(struct goodix_ts_data *ts, unsigned short *timeout)
{
	unsigned char i2c_state_buf[2] = {ADDR_STA, UNKNOWN_ERROR};
	int ret;
	while(*timeout < MAX_TIMEOUT)
	{
		ret = i2c_read_bytes(i2c_client, i2c_state_buf[0], &i2c_state_buf[1], 1);
		if(ret <= 0)
			return ERROR_I2C_TRANSFER;
		if(i2c_state_buf[1] & SLAVE_READY)
		{
			return i2c_state_buf[1];
			//return 1;
		}
		msleep(10);
		*timeout += 5;
	}
	return 0;
}

static int gt801_config_write_proc(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
	unsigned char cmd[120];
	int ret = -1;
	int start_addr = 0;	

	static unsigned char update_path[60];
	static unsigned short time_count = 0;
	static unsigned int file_len = 0;
	
	unsigned char i2c_control_buf[2] = {ADDR_CMD, 0};
	unsigned char i2c_states_buf[2] = {ADDR_STA, 0};
	unsigned char i2c_data_buf[PACK_SIZE+1+8] = {ADDR_DAT,};
	unsigned char retries = 0;
	unsigned int rd_len;
	unsigned char i = 0;

#ifdef UPDATE_NEW_PROTOCOL
	unsigned char checksum_error_times = 0;
	unsigned int frame_checksum = 0;
	unsigned int frame_number = 0;
#else
	unsigned char send_crc = 0;
#endif

	struct file * file_data = NULL;
	mm_segment_t old_fs;
	struct goodix_ts_data *ts;
	
	ts = i2c_get_clientdata(i2c_connect_client);
	if(ts==NULL)
		return 0;
	
	if(copy_from_user(&cmd, buff, len))
	{
		return -EFAULT;
	}
	switch(cmd[0])
	{
		case STEP_SET_PATH:
			printk(KERN_INFO"Write cmd is:%d,cmd arg is:%s,write len is:%ld\n",cmd[0], &cmd[1], len);
			memset(update_path, 0, 60);
			strncpy(update_path, cmd+1, 60);
			if(update_path[0] == 0)
				return 0;
			else
				return 1;
		case STEP_CHECK_FILE:
			printk(KERN_INFO"Begin to firmware update ......\n");
			ret = update_file_check(update_path);
			if(ret <= 0)
			{
				printk(KERN_INFO"fialed to check update file!\n");
				return ret;
			}
			msleep(500);
			printk(KERN_INFO"Update check file success!\n");
			return 1;
		case STEP_WRITE_SYN:
			printk(KERN_INFO"STEP1:Write synchronization signal!\n");
			i2c_control_buf[1] = UPDATE_START;
			ret = i2c_write_bytes(i2c_client, i2c_control_buf, i2c_control_buf+1, 1);
			if(ret <= 0)
			{
				ret = ERROR_I2C_TRANSFER;
				return ret;
			}
			//the time include time(APROM -> LDROM) and time(LDROM init)
			msleep(1000);
			return 1;
		case STEP_WAIT_SYN:
			printk(KERN_INFO"STEP2:Wait synchronization signal!\n");
			while(retries < MAX_I2C_RETRIES)
			{
				i2c_states_buf[1] = UNKNOWN_ERROR;
				ret = i2c_read_bytes(i2c_client, i2c_states_buf[0], &i2c_states_buf[1], 1);
				printk(KERN_INFO"The read byte is:%d\n", i2c_states_buf[1]);
				if(i2c_states_buf[1] & UPDATE_START)
					break;
				msleep(5);
				retries++;
				time_count += 10;
			}
			if((retries >= MAX_I2C_RETRIES) && (!(i2c_states_buf[1] & UPDATE_START)))
			{
				if(ret <= 0)
					return 0;
				else
					return -1;
			}
			return 1;
		case STEP_WRITE_LENGTH:
			printk(KERN_INFO"STEP3:Write total update file length!\n");
			file_len = update_get_flen(update_path);
			if(file_len <= 0)
			{
				printk(KERN_INFO"get update file length failed!\n");
				return -1;
			}
			file_len += 4;
			i2c_data_buf[1] = (file_len>>24) & 0xff;
			i2c_data_buf[2] = (file_len>>16) & 0xff;
			i2c_data_buf[3] = (file_len>>8) & 0xff;
			i2c_data_buf[4] = file_len & 0xff;
			file_len -= 4;
			ret = i2c_write_bytes(i2c_client, i2c_data_buf, i2c_data_buf+1, 4);
			if(ret <= 0)
			{
				ret = ERROR_I2C_TRANSFER;
				return 0;
			}
			return 1;
		case STEP_WAIT_READY:
			printk(KERN_INFO"STEP4:Wait slave ready!\n");
			ret = wait_slave_ready(ts, &time_count);
			if(ret == ERROR_I2C_TRANSFER)
				return 0;
			if(!ret)
			{
				return -1;
			}
			printk(KERN_INFO"Slave ready!\n");
			return 1;
		case STEP_WRITE_DATA:
#ifdef UPDATE_NEW_PROTOCOL
			printk(KERN_INFO"STEP5:Begin to send file data use NEW protocol!\n");
			file_data = update_file_open(update_path, &old_fs);
			if(file_data == NULL)	//file_data has been opened at the last time
			{
				return -1;
			}
			frame_number = 0;
			while((file_len >= 0) /*&& (!send_crc)*/)
			{
				i2c_data_buf[0] = ADDR_DAT;
				rd_len = (file_len >= PACK_SIZE) ? PACK_SIZE : file_len;
				frame_checksum = 0;
				if(file_len)
				{
					ret = file_data->f_op->read(file_data, i2c_data_buf+1+4, rd_len, &file_data->f_pos);
					if(ret <= 0)
					{
						printk("[GOODiX_ISP_NEW]:Read File Data Failed!\n");
						return -1;
					}
					i2c_data_buf[1] = (frame_number>>24)&0xff;
					i2c_data_buf[2] = (frame_number>>16)&0xff;
					i2c_data_buf[3] = (frame_number>>8)&0xff;
					i2c_data_buf[4] = frame_number&0xff;
					frame_number++;
					frame_checksum = 0;
					for(i=0; i<rd_len; i++)
					{
						frame_checksum += i2c_data_buf[5+i];
					}
					frame_checksum = 0 - frame_checksum;
					i2c_data_buf[5+rd_len+0] = frame_checksum&0xff;
					i2c_data_buf[5+rd_len+1] = (frame_checksum>>8)&0xff;
					i2c_data_buf[5+rd_len+2] = (frame_checksum>>16)&0xff;
					i2c_data_buf[5+rd_len+3] = (frame_checksum>>24)&0xff;
				}
rewrite:
				printk(KERN_INFO"[GOODiX_ISP_NEW]:%d\n", file_len);				
				ret = i2c_write_bytes(i2c_client, i2c_data_buf, i2c_data_buf+1, 4+rd_len+4);
					//if(ret <= 0)
				if(ret != 1)
				{
					printk("[GOODiX_ISP_NEW]:Write File Data Failed!Return:%d\n", ret);
					return 0;
				}
				
				//Wait for slave ready signal.and read the checksum
				ret = wait_slave_ready(ts, &time_count);
				if(ret & (CHECKSUM_ERROR))
				{
					printk("[GOODiX_ISP_NEW]:File Data Frame checksum Error!\n");
					checksum_error_times++;
					msleep(20);
					if(checksum_error_times > 20)				//max retry times.
						return 0;
					goto rewrite;
				}
				checksum_error_times = 0;
				if(ret & (FRAME_ERROR))
				{
					printk("[GOODiX_ISP_NEW]:File Data Frame Miss!\n");
					return 0;
				}
				if(ret == ERROR_I2C_TRANSFER)
					return 0;
				if(!ret)
				{
					return -1;
				}
				if(file_len < PACK_SIZE)
				{
					update_file_close(file_data, old_fs);
					break;
				}
				file_len -= rd_len;
			}//end of while((file_len >= 0))
			return 1;
#else
			printk(KERN_INFO"STEP5:Begin to send file data use OLD protocol!\n");
			file_data = update_file_open(update_path, &old_fs);
			if(file_data == NULL)	//file_data has been opened at the last time
			{
				return -1;
			}
			while((file_len >= 0) && (!send_crc))
			{
				printk(KERN_INFO"[GOODiX_ISP_OLD]:%d\n", file_len);
				i2c_data_buf[0] = ADDR_DAT;
				rd_len = (file_len >= PACK_SIZE) ? PACK_SIZE : file_len;
				if(file_len)
				{
					ret = file_data->f_op->read(file_data, i2c_data_buf+1, rd_len, &file_data->f_pos);
					if(ret <= 0)
					{
						return -1;
					}
				}
				if(file_len < PACK_SIZE)
				{
					send_crc = 1;
					update_file_close(file_data, old_fs);
					i2c_data_buf[file_len+1] = oldcrc32&0xff;
					i2c_data_buf[file_len+2] = (oldcrc32>>8)&0xff;
					i2c_data_buf[file_len+3] = (oldcrc32>>16)&0xff;
					i2c_data_buf[file_len+4] = (oldcrc32>>24)&0xff;
					ret = i2c_write_bytes(i2c_client, i2c_data_buf, i2c_data_buf+1, (file_len+4));
					//if(ret <= 0)
					if(ret != 1)
					{
						printk("[GOODiX_ISP_OLD]:Write File Data Failed!Return:%d\n", ret);
						return 0;
					}
					break;
				}
				else
				{
					ret = i2c_write_bytes(i2c_client, i2c_data_buf, i2c_data_buf+1, PACK_SIZE);
					//if(ret <= 0)
					if(ret != 1)
					{
						printk("[GOODiX_ISP_OLD]:Write File Data Failed!Return:%d\n", ret);
						return 0;
					}
				}
				file_len -= rd_len;
			
				//Wait for slave ready signal.
				ret = wait_slave_ready(ts, &time_count);
				if(ret == ERROR_I2C_TRANSFER)
					return 0;
				if(!ret)
				{
					return -1;
				}
				//Slave is ready.
			}//end of while((file_len >= 0) && (!send_crc))
			return 1;
#endif
		case STEP_READ_STATUS:
			printk(KERN_INFO"STEP6:Read update status!\n");
			while(time_count < MAX_TIMEOUT)
			{
				ret = i2c_read_bytes(i2c_client, i2c_states_buf[0], &i2c_states_buf[1], 1);
				if(ret <= 0)
				{
					return 0;
				}
				if(i2c_states_buf[1] & SLAVE_READY)
				{
					if(!(i2c_states_buf[1] &0xf0))
					{
						printk(KERN_INFO"The firmware updating succeed!update state:0x%x\n",i2c_states_buf[1]);
						return 1;
					}
					else
					{
						printk(KERN_INFO"The firmware updating failed!update state:0x%x\n",i2c_states_buf[1]);
						return 0;

					}
				}
				msleep(1);
				time_count += 5;
			}
			return -1;
		case FUN_CLR_VAL:								//clear the static val
			time_count = 0;
			file_len = 0;
			return 1;
		case FUN_CMD:							//functional command
			if(cmd[1] == CMD_DISABLE_TP)
			{
				printk(KERN_INFO"Disable TS int!\n");
				mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
				//if(ts->use_irq)
				//	disable_irq(TS_INT);
			}
			else if(cmd[1] == CMD_ENABLE_TP)
			{
				printk(KERN_INFO"Enable TS int!\n");
				mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
				//if(ts->use_irq)
				//	enable_irq(TS_INT);
			}
			else if(cmd[1] == CMD_READ_VER)
			{
				printk(KERN_INFO"Read version!\n");
				ts->read_mode = MODE_RD_VER;
			}
			else if(cmd[1] == CMD_READ_RAW)
			{
				printk(KERN_INFO"Read raw data!\n");
				ts->read_mode = MODE_RD_RAW;
				i2c_control_buf[1] = 201;
				ret = i2c_write_bytes(i2c_client, i2c_control_buf, i2c_control_buf+1, 1);			//read raw data cmd
				if(ret <= 0)
				{
					printk(KERN_INFO"Write read raw data cmd failed!\n");
					return 0;
				}
				msleep(200);
			}
			else if(cmd[1] == CMD_READ_DIF)
			{
				printk(KERN_INFO"Read diff data!\n");
				ts->read_mode = MODE_RD_DIF;
				i2c_control_buf[1] = 202;
				ret = i2c_write_bytes(i2c_client, i2c_control_buf, i2c_control_buf+1, 1);			//read diff data cmd
				if(ret <= 0)
				{
					printk(KERN_INFO"Write read raw data cmd failed!\n");
					return 0;
				}
				msleep(200);
			}
			else if(cmd[1] == CMD_READ_CFG)
			{
				printk(KERN_INFO"Read config info!\n");
				ts->read_mode = MODE_RD_CFG;
			}
			else if(cmd[1] == CMD_SYS_REBOOT)
			{
				printk(KERN_INFO"System reboot!\n");
				sys_sync();
				msleep(200);
				kernel_restart(NULL);
			}
			return 1;
		case FUN_WRITE_CONFIG:
			//printk(KERN_INFO"Write config info!\n");
			//cmd[1] config length，cmd[2]config start addr
			if(cmd[1]> 84 || cmd[2] > (0x65+84) || cmd[2] < 0x65)
			{	//invalid config info
				printk(KERN_INFO "Config data is invalid.\n");
				return 0;
			}

			#define EFFECT_CONFIG_POS 3		//当发送地址为0x65时固定配置有效配置在cmd中的偏移地址
			if((cmd[2]-0x65) <= 7)
			{
				start_addr = (cmd[2]-0x65) + EFFECT_CONFIG_POS + 1;
				//restore some default config.
				for(i = 0; i < 6; i++)
				{
					if(start_addr > (EFFECT_CONFIG_POS+6))
						break;
				
					switch(i)
					{
					case 0:
						//cmd[start_addr] = TOUCH_MAX_HEIGHT>>8;			//malata no need
						break;
					case 1:
						//cmd[start_addr] = TOUCH_MAX_HEIGHT&0xff;			//malata no need 
						break;
					case 2:
						//cmd[start_addr] = TOUCH_MAX_WIDTH>>8;				//malata no need
						break;
					case 3:
						//cmd[start_addr] = TOUCH_MAX_WIDTH&0xff;			//malata no need
						break;
					case 4:
						cmd[start_addr] = (cmd[start_addr]&0xf0) | MAX_FINGER_NUM;
						break;
					case 5:
						cmd[start_addr] = (cmd[start_addr]&0xf0) | INT_TRIGGER;
						break;
					}
					start_addr++;
				}
			}
			
			printk(KERN_INFO"Write config info!\n");
			for(i=3; i<cmd[1];i++)
			{
				if((i-3)%5 == 0)printk("\n");
				printk("No.%d:0x%x ", i-3, cmd[i]);
			}
			printk("\n");
				
			ret = i2c_write_bytes(i2c_client, cmd+2, cmd+3, cmd[1]-1); 
			if(ret > 0)
				return 1;
			else
				return -1;
		default:
			return -ENOSYS;
	}
	return 0;
}

static int  goodix_read_version(struct goodix_ts_data *ts, char **version)
{
	int ret = -1, count = 0;
	//unsigned char version_data[18];
	char *version_data;
	char *p;
	
	*version = (char *)vmalloc(18);
	version_data = *version;
	if(!version_data)
		return -ENOMEM;
	p = version_data;
	memset(version_data, 0, sizeof(version_data));
	version_data[0]=240;	
	ret=i2c_read_bytes(i2c_client,version_data[0], &version_data[1], 16);
	if (ret < 0) 
		return ret;
	version_data[17]='\0';
	
	if(*p == '\0')
		return 0; 	
	do 					
	{
		if((*p > 122) || (*p < 48 && *p != 32) || (*p >57 && *p  < 65) 
			||(*p > 90 && *p < 97 && *p  != '_'))		//check illeqal character
			count++;
	}while(*++p != '\0' );
	if(count > 2)
		return 0;
	else 
		return 1;	
}

static int gt801_config_read_proc( char *page, char **start, off_t off, int count, int *eof, void *data )
{
	int ret = -1;
	struct goodix_ts_data *ts;
	int len = 0;
	char *version_info = NULL;
	unsigned char read_data[1201] = {80, };
  	/*if (off > 0) 
	{
    	*eof = 1;
    	return 0;
  	}*/
	ts = i2c_get_clientdata(i2c_connect_client);
	if(ts==NULL)
		return 0;

	if(ts->read_mode == MODE_RD_VER)		//read version data
	{
		ret = goodix_read_version(ts, &version_info);
		if(ret <= 0)
		{
			printk(KERN_INFO"Read version data failed!\n");
			vfree(version_info);
			return 0;
		}

		for(len=0;len<100;len++)
		{
			if(*(version_info + len) == '\0')
				break;
		}
		printk(KERN_INFO"Gt801_plus ROM version is:%s,and len is:%d\n", (version_info+1), len);
		strncpy(page, version_info+1, len + 1);
		vfree(version_info);
		*eof = 1;
		return len+1;
	}
	else if((ts->read_mode == MODE_RD_RAW)||(ts->read_mode == MODE_RD_DIF))		//read raw data or diff
	{
		//printk(KERN_INFO"Read raw data\n");
		ret = i2c_read_bytes(i2c_client, read_data[0], &read_data[1], 1200);
		if(ret <= 0)
		{
			if(ts->read_mode == 2)
				printk(KERN_INFO"Read raw data failed!\n");
			if(ts->read_mode == 3)
				printk(KERN_INFO"Read diff data failed!\n");
			return 0;
		}
		memcpy(page, read_data+1, 1200);
		*eof = 1;
		*start = NULL;
		return 1200;
	}
	else if(ts->read_mode == MODE_RD_CFG)
	{
		read_data[0] = 0x65;
		ret = i2c_read_bytes(i2c_client, read_data[0], &read_data[1], 83);
		if(ret <= 0)
		{
			printk(KERN_INFO"Read config info failed!\n");
			return 0;
		}
		memcpy(page, read_data+1, 84);
		return 84;
	}
	return len;
}
#endif


static int tpd_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) {
    strcpy(info->type, "mtk-tpd");
    return 0;
}

static int i2c_read_bytes( struct i2c_client *client, u8 addr, u8 *rxbuf, int len )
{
    u8 retry;
    u16 left = len;
    u16 offset = 0;

    if ( rxbuf == NULL )
        return -1;

    TPD_DEBUG("i2c_read_bytes to device %02X address %04X len %d\n", client->addr, addr, len );

    while ( left > 0 )
    {
        if ( left > MAX_TRANSACTION_LENGTH )
        {
            rxbuf[offset] = ( addr+offset ) & 0xFF;
            i2c_client->addr = i2c_client->addr & I2C_MASK_FLAG | I2C_WR_FLAG | I2C_RS_FLAG;
            retry = 0;
            while ( i2c_master_send(i2c_client, &rxbuf[offset], (MAX_TRANSACTION_LENGTH << 8 | 1)) < 0 )
            {
                retry++;

                if ( retry == 5 )
                {
                    i2c_client->addr = i2c_client->addr & I2C_MASK_FLAG;
                    TPD_DEBUG("I2C read 0x%X length=%d failed\n", addr + offset, MAX_TRANSACTION_LENGTH);
                    return -1;
                }
            }
            left -= MAX_TRANSACTION_LENGTH;
            offset += MAX_TRANSACTION_LENGTH;
        }
        else
        {
            
            rxbuf[0] = addr;
            i2c_client->addr = i2c_client->addr & I2C_MASK_FLAG | I2C_WR_FLAG | I2C_RS_FLAG;
            
            retry = 0;
            while ( i2c_master_send(i2c_client, &rxbuf[0], (left << 8 | 1)) < 0 )
            {
                retry++;

                if ( retry == 5 )
                {
                    i2c_client->addr = i2c_client->addr & I2C_MASK_FLAG;
                    TPD_DEBUG("I2C read 0x%X length=%d failed\n", addr + offset, left);
                    return -1;
                }
            }
            left = 0;
        }
    }
    i2c_client->addr = i2c_client->addr & I2C_MASK_FLAG;
    
    return 0;
}

static int i2c_write_bytes( struct i2c_client *client, u16 addr, u8 *txbuf, int len )
{
    u8 buffer[MAX_TRANSACTION_LENGTH];
    u16 left = len;
    u16 offset = 0;
    u8 retry = 0;

    struct i2c_msg msg = 
    {
        .addr = ((client->addr&I2C_MASK_FLAG )|(I2C_ENEXT_FLAG )),
        .flags = 0,
        .buf = buffer
    };


    if ( txbuf == NULL )
        return -1;

    TPD_DEBUG("i2c_write_bytes to device %02X address %04X len %d\n", client->addr, addr, len );

    while ( left > 0 )
    {
        retry = 0;

        buffer[0] = ( addr+offset ) & 0xFF;

        if ( left > MAX_I2C_TRANSFER_SIZE )
        {
            memcpy( &buffer[I2C_DEVICE_ADDRESS_LEN], &txbuf[offset], MAX_I2C_TRANSFER_SIZE );
            msg.len = MAX_TRANSACTION_LENGTH;
            left -= MAX_I2C_TRANSFER_SIZE;
            offset += MAX_I2C_TRANSFER_SIZE;
        }
        else
        {
            memcpy( &buffer[I2C_DEVICE_ADDRESS_LEN], &txbuf[offset], left );
            msg.len = left + I2C_DEVICE_ADDRESS_LEN;
            left = 0;
        }

        TPD_DEBUG("byte left %d offset %d\n", left, offset );

        while ( i2c_transfer( client->adapter, &msg, 1 ) != 1 )
        {
            retry++;

            if ( retry == 5 )
            {
                TPD_DEBUG("I2C write 0x%X%X length=%d failed\n", buffer[0], buffer[1], len);
                return -1;
            }
            else
                 TPD_DEBUG("I2C write retry %d addr 0x%X%X\n", retry, buffer[0], buffer[1]);

        }
    }

    return 0;
}

static int tpd_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id) {             
    int err = 0, ret = -1;
    struct goodix_ts_data *ts;
    u8 version[17];
    
    #ifdef TPD_NO_GPIO
    u16 temp;
    temp = *(volatile u16 *) TPD_RESET_PIN_ADDR;
    temp = temp | 0x40;
    *(volatile u16 *) TPD_RESET_PIN_ADDR = temp;
    #endif
    i2c_client = client;
    
    printk("MediaTek touch panel i2c probe\n");
    
    client->addr = 0xEA;
    version[0]=0x00;
    i2c_write_bytes(client, 0x03, &version[0], 1);
    version[0]=0x00;
    i2c_write_bytes(client, 0x17, &version[0], 1);
    version[0]=0x10;
    i2c_write_bytes(client, 0x18, &version[0], 1);
    client->addr = 0xAA;
    
    #ifndef TPD_NO_GPIO 

    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);  
    msleep(10);  
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
   
    mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
    mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_DISABLE);
    #endif 

    msleep(50);
    
#ifdef CONFIG_TOUCHSCREEN_GOODIX_IAP
    ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    if (ts == NULL) {
        ret = -ENOMEM;
        printk("mtk-tpd: Allocate ts memory fail\n");
	return ret;
    }
    ts->client = client;
    i2c_set_clientdata(client, ts);
    
    i2c_connect_client = client;	//used by Guitar_Update
    // Create proc file system
    gt801_config_proc = create_proc_entry( GT801_CONFIG_PROC_FILE, 0666, NULL);

    if ( gt801_config_proc == NULL )
    {
        printk("create_proc_entry %s failed\n", GT801_CONFIG_PROC_FILE );
    }
    else 
    {
        gt801_config_proc->read_proc = gt801_config_read_proc;
        gt801_config_proc->write_proc = gt801_config_write_proc;
    }
#endif
    
    //Get firmware version
    #if 0
    memset( &version[0], 0, 17);
    version[16] = '\0';
    err = i2c_read_bytes( i2c_client, TPD_VERSION_INFO_REG, (u8 *)&version[0], 16);

    if ( err )
    {
        printk(TPD_DEVICE " fail to get tpd info %d\n", err );
        return err;
    }
    else
    {
        printk( "Goodix TouchScreen Version:%s\n", (char *)&version[0]);
    }
    #endif
    
    //Load the init table
    // setting resolution, RES_X, RES_Y
    //cfg_data[1] = (TPD_RES_X>>8)&0xff;
    //cfg_data[2] = TPD_RES_X&0xff;
    //cfg_data[3] = (TPD_RES_Y>>8)&0xff;
    //cfg_data[4] = TPD_RES_Y&0xff;
    err = i2c_write_bytes( i2c_client, TPD_CONFIG_REG_BASE, cfg_data, sizeof(cfg_data));

    if (err)
    {
        printk(TPD_DEVICE " fail to write tpd cfg %d\n", err );
        return err;
    }
    msleep(10);
    
    
    //i2c_read_bytes(i2c_client, TPD_TOUCH_INFO_REG_BASE, &version[0], 2);
    //printk(TPD_DEVICE " tpd cfg 0x%x 0x%x 0x%x 0x%x\n", version[0], version[1], version[2], version[3]);
	
    thread = kthread_run(touch_event_handler, 0, TPD_DEVICE);
    if (IS_ERR(thread)) { 
        err = PTR_ERR(thread);
        printk(TPD_DEVICE " failed to create kernel thread: %d\n", err);
    }
    
    tpd_load_status = 1;
    
    mt65xx_eint_set_sens(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_SENSITIVE);
    mt65xx_eint_set_hw_debounce(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);
    mt65xx_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, CUST_EINT_TOUCH_PANEL_POLARITY, tpd_eint_interrupt_handler, 1);
    mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
    
    printk("MediaTek touch panel i2c probe success\n");
    
    return 0;
}

void tpd_eint_interrupt_handler(void) { 
    TPD_DEBUG_PRINT_INT; tpd_flag=1; wake_up_interruptible(&waiter);
} 

static int tpd_i2c_remove(struct i2c_client *client) 
{
#ifdef CONFIG_TOUCHSCREEN_GOODIX_IAP
    remove_proc_entry("goodix-update", NULL);
    i2c_set_clientdata(client, NULL);
    kfree(ts);
#endif
    return 0;
}

void tpd_down(int raw_x, int raw_y, int x, int y, int p) {
    input_report_abs(tpd->dev, ABS_PRESSURE, 128);
    input_report_key(tpd->dev, BTN_TOUCH, 1);
    input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 128);
    input_report_abs(tpd->dev, ABS_MT_WIDTH_MAJOR, 128);
    input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
    input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
    input_mt_sync(tpd->dev);
    TPD_DEBUG("D[%4d %4d %4d]\n", x, y, p);
    TPD_EM_PRINT(raw_x, raw_y, x, y, p, 1);
}

void tpd_up(int raw_x, int raw_y, int x, int y, int p) {
    input_report_abs(tpd->dev, ABS_PRESSURE, 0);
    input_report_key(tpd->dev, BTN_TOUCH, 0);
    input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 0);
    input_report_abs(tpd->dev, ABS_MT_WIDTH_MAJOR, 0);
    input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
    input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
    input_mt_sync(tpd->dev);
    TPD_DEBUG("U[%4d %4d %4d]\n", x, y, 0);
    TPD_EM_PRINT(raw_x, raw_y, x, y, p, 0);
}

static int touch_event_handler(void *unused) {
    struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
    static u8 buffer[ TPD_POINT_INFO_LEN*MAX_FINGER_NUM ];
    int x, y, size, max_finger_id = 0, finger_num = 0;
    static u8 id_mask = 0;
    u16 cur_mask;
    int idx;
    int ret = 0;
    static int x_history[MAX_FINGER_NUM+1];
    static int y_history[MAX_FINGER_NUM+1];
    
    sched_setscheduler(current, SCHED_RR, &param); 
    do{
        set_current_state(TASK_INTERRUPTIBLE);
        while (tpd_halt) {tpd_flag = 0; msleep(20);}
        wait_event_interruptible(waiter, tpd_flag != 0);
        tpd_flag = 0;
        TPD_DEBUG_SET_TIME;
        set_current_state(TASK_RUNNING); 
        
        
        ret = i2c_read_bytes(i2c_client, TPD_TOUCH_INFO_REG_BASE, &buffer[0], 2);
        if (ret < 0)
        {
            TPD_DEBUG("[mtk-tpd] i2c write communcate error during read status\n");
            continue;
        }
        TPD_DEBUG("[mtk-tpd] STATUS : %x %x\n", buffer[0], buffer[1]);
        cur_mask = ((buffer[1] & 0x03) << 8) | (buffer[0] & 0xff);
        
        if ( tpd == NULL || tpd->dev == NULL )
            continue;
        
        if ( cur_mask )
        {
            idx = 0;
            do
            {
                idx++;
            }while((cur_mask >> idx) > 0);
            max_finger_id = idx - 1;
            ret = i2c_read_bytes( i2c_client, TPD_POINT_INFO_REG_BASE, buffer, (max_finger_id + 1)*TPD_POINT_INFO_LEN);
            if (ret < 0)
            {
                TPD_DEBUG("[mtk-tpd] i2c write communcate error during read point\n");
                continue;
            }
        }
        else
        {
            max_finger_id = 0;
        }

        for ( idx = 0 ; idx < (max_finger_id + 1) ; idx++ )
        {
            u8 *ptr = &buffer[ idx*TPD_POINT_INFO_LEN ];

            if (cur_mask & (1 << idx))
            {
                x = ptr[1] + (((int)ptr[0]) << 8);
                y = ptr[3] + (((int)ptr[2]) << 8);
                size = ptr[4];

                tpd_calibrate(&x, &y);
                tpd_down( x, y, x, y, size );

                x_history[idx] = x;
                y_history[idx] = y;
            }
            else
                TPD_DEBUG("Invalid id %d\n", idx );
        }         
               
        if ( cur_mask != id_mask )
        {
            u8 diff = cur_mask^id_mask;
            idx = 0;

            while ( diff )
            {
                if ( ( ( diff & 0x01 ) == 1 ) &&
                     ( ( cur_mask >> idx ) & 0x01 ) == 0 )
                {
                    // check if key release
                    tpd_up( x_history[idx], y_history[idx], x_history[idx], y_history[idx], 0);                    
                }

                diff = ( diff >> 1 );
                idx++;
            }
            id_mask = cur_mask;
        }

        if ( tpd != NULL && tpd->dev != NULL )
            input_sync(tpd->dev);
        
    } while (!kthread_should_stop()); 
    return 0;
}

int tpd_local_init(void) 
{
    if(i2c_add_driver(&tpd_i2c_driver)!=0) {
      TPD_DMESG("unable to add i2c driver.\n");
      return -1;
    }

    if(tpd_load_status == 0)
    {
    	TPD_DMESG("add error touch panel driver.\n");
    	i2c_del_driver(&tpd_i2c_driver);
    	return -1;
    }
    
#ifdef TPD_HAVE_BUTTON     
    tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);// initialize tpd button data
#endif   
  
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))    
    TPD_DO_WARP = 1;
    memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT*4);
    memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT*4);
#endif 

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
    memcpy(tpd_calmat, tpd_def_calmat_local, 8*4);
    memcpy(tpd_def_calmat, tpd_def_calmat_local, 8*4);	
#endif  
    TPD_DMESG("end %s, %d\n", __FUNCTION__, __LINE__);  
    tpd_type_cap = 1;
    return 0;
}

/* Function to manage low power suspend */
void tpd_suspend(struct i2c_client *client, pm_message_t message)
{
    int ret = 0, retry = 0;
    u8 Wrbuf[1] = {1};
    tpd_halt = 1;
    mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
    TPD_DEBUG("GT801 call suspend\n");
    
    ret = i2c_write_bytes(i2c_client, TPD_POWER_MODE_REG, &Wrbuf[0], 1);

    if(ret < 0)
    {
        TPD_DEBUG("[mtk-tpd] i2c write communcate error during suspend: 0x%x\n", ret);
    }

}

/* Function to manage power-on resume */
void tpd_resume(struct i2c_client *client)
{   
    TPD_DEBUG("GT801 call resume\n");  
  
    mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_EINT_PIN, GPIO_OUT_ZERO);
    msleep(20);
    
    mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
    mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_DISABLE);
    mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM); 
    tpd_halt = 0;
}

static struct tpd_driver_t tpd_device_driver = {
                .tpd_device_name = "gt801",
                .tpd_local_init = tpd_local_init,
                .suspend = tpd_suspend,
                .resume = tpd_resume,
#ifdef TPD_HAVE_BUTTON
                .tpd_have_button = 1,
#else
                .tpd_have_button = 0,
#endif
};
/* called when loaded into kernel */
static int __init tpd_driver_init(void) {
    printk("MediaTek gt801 touch panel driver init\n");
                if(tpd_driver_add(&tpd_device_driver) < 0)
                        TPD_DMESG("add generic driver failed\n");
    return 0;
}

/* should never be called */
static void __exit tpd_driver_exit(void) {
    TPD_DMESG("MediaTek gt801 touch panel driver exit\n");
    //input_unregister_device(tpd->dev);
    tpd_driver_remove(&tpd_device_driver);
    
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);

