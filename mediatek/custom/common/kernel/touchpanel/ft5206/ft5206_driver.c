
 
#include "tpd.h"
#include <linux/interrupt.h>
#include <cust_eint.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>

#include "tpd_custom_ft5206.h"
#ifdef MT6575
#include <mach/mt6575_pm_ldo.h>
#include <mach/mt6575_typedefs.h>
#include <mach/mt6575_boot.h>
#endif
#include <linux/dma-mapping.h>

#include "cust_gpio_usage.h"




#define FTS_PACKET_LENGTH 128
#define I2C_CTPM_ADDRESS 0x70
#define TP_UPGRADE 1
#define MAX_TOUCH_POINT 5					//now support 5 points
static unsigned char CTPM_FW_TRULY[]=
{
	#include "firmware_truly_v10.h"
};
static unsigned char CTPM_FW_JUNDA[]=
{
	#include "firmware_junda_v11.h"
};


extern struct tpd_device *tpd;
struct i2c_client *i2c_client = NULL;
struct task_struct *thread = NULL;
char version[2][10] = {"Truly", "Junda"};

static DECLARE_WAIT_QUEUE_HEAD(waiter);
 
 
static void tpd_eint_interrupt_handler(void);
 
 
 extern void mt65xx_eint_unmask(unsigned int line);
 extern void mt65xx_eint_mask(unsigned int line);
 extern void mt65xx_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
 extern kal_uint32 mt65xx_eint_set_sens(kal_uint8 eintno, kal_bool sens);
 extern void mt65xx_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
									  kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
									  kal_bool auto_umask);

 
static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int __devexit tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);
 
//#define TPD_CLOSE_POWER_IN_SLEEP
static int tpd_flag = 0;
static int point_num = 0;
static int p_point_num = 0;

#define TPD_OK 0
//register define

#define DEVICE_MODE 0x00
#define GEST_ID 0x01
#define TD_STATUS 0x02

#define TOUCH1_XH 0x03
#define TOUCH1_XL 0x04
#define TOUCH1_YH 0x05
#define TOUCH1_YL 0x06

#define TOUCH2_XH 0x09
#define TOUCH2_XL 0x0A
#define TOUCH2_YH 0x0B
#define TOUCH2_YL 0x0C

#define TOUCH3_XH 0x0F
#define TOUCH3_XL 0x10
#define TOUCH3_YH 0x11
#define TOUCH3_YL 0x12
//register define


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

struct touch_info {
    int y[5];
    int x[5];
    int p[5];
    int count;
};
 
 static const struct i2c_device_id tpd_id[] = {{TPD_DEVICE,0},{}};
 static struct i2c_board_info __initdata i2c_tpd={ I2C_BOARD_INFO("mtk-tpd", (0x70>>1))};

 /*add IIC add table for linux 2.6 by wwl*/
 unsigned short force[] = {0,0x70,I2C_CLIENT_END,I2C_CLIENT_END}; 
 static const unsigned short * const forces[] = { force, NULL };
 static struct i2c_client_address_data addr_data = { .forces = forces, };

 
 static struct i2c_driver tpd_i2c_driver = {
  .driver = {
	 .name = TPD_DEVICE,
	 .owner = THIS_MODULE,
  },
  .probe = tpd_probe,
  .remove = __devexit_p(tpd_remove),
  .id_table = tpd_id,
  .detect = tpd_detect,
  .address_data = &addr_data,
 };
 

static  void tpd_down(int x, int y, int p) {
	input_report_key(tpd->dev, BTN_TOUCH, 1);
	input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 1);
	input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
	input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
	input_mt_sync(tpd->dev);
	if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
	{   
		tpd_button(x, y, 1);  
	}	 
	TPD_EM_PRINT(x, y, x, y, 0, 1);
 }
 
static  void tpd_up(int x, int y,int *count) {
	input_report_key(tpd->dev, BTN_TOUCH, 0);
	input_mt_sync(tpd->dev);
	TPD_EM_PRINT(x, y, x, y, 0, 0);
	if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
	{   
		tpd_button(x, y, 0); 
	}   		 

 }

 static int tpd_touchinfo(struct touch_info *cinfo, struct touch_info *pinfo)
 {

	int i = 0;
	
	char data[33] = {0};

    u16 high_byte,low_byte;
	u8 report_rate =0;

	p_point_num = point_num;

	i2c_smbus_read_i2c_block_data(i2c_client, 0x00, 8, &(data[0]));
	i2c_smbus_read_i2c_block_data(i2c_client, 0x08, 8, &(data[8]));
	i2c_smbus_read_i2c_block_data(i2c_client, 0x10, 8, &(data[16]));
	i2c_smbus_read_i2c_block_data(i2c_client, 0x18, 8, &(data[24]));
	i2c_smbus_read_i2c_block_data(i2c_client, 0x20, 1, &(data[32]));

	i2c_smbus_read_i2c_block_data(i2c_client, 0x88, 1, &report_rate);
	
	TPD_DEBUG("received raw data from touch panel as following:\n");
	TPD_DEBUG("[data[0]=%x,data[1]= %x ,data[2]=%x ,data[3]=%x ,data[4]=%x ,data[5]=%x]\n",data[0],data[1],data[2],data[3],data[4],data[5]);
	TPD_DEBUG("[data[9]=%x,data[10]= %x ,data[11]=%x ,data[12]=%x]\n",data[9],data[10],data[11],data[12]);
	TPD_DEBUG("[data[15]=%x,data[16]= %x ,data[17]=%x ,data[18]=%x]\n",data[15],data[16],data[17],data[18]);


	 if(report_rate < 8)
	 {
	   report_rate = 0x8;
	   if((i2c_smbus_write_i2c_block_data(i2c_client, 0x88, 1, &report_rate))< 0)
	   {
		   TPD_DMESG("I2C read report rate error, line: %d\n", __LINE__);
	   }
	 }
	 
	
	/* Device Mode[2:0] == 0 :Normal operating Mode*/
	if(data[0] & 0x70 != 0) return false; 

	/*get the number of the touch points*/
	point_num= data[2] & 0x0f;
	if(point_num > MAX_TOUCH_POINT)
	{
		printk("error: point num > max point\n");
		return false;
	}	
	TPD_DEBUG("point_num =%d\n",point_num);

	TPD_DEBUG("Procss raw data...\n");

		
		for(i = 0; i < point_num; i++)
		{
			cinfo->p[i] = data[3+6*i] >> 6; //event flag 

	       /*get the X coordinate, 2 bytes*/
			high_byte = data[3+6*i];
			high_byte <<= 8;
			high_byte &= 0x0f00;
			low_byte = data[3+6*i + 1];
			cinfo->x[i] = high_byte |low_byte;

				//cinfo->x[i] =  cinfo->x[i] * 480 >> 11; //calibra
		
			/*get the Y coordinate, 2 bytes*/
			
			high_byte = data[3+6*i+2];
			high_byte <<= 8;
			high_byte &= 0x0f00;
			low_byte = data[3+6*i+3];
			cinfo->y[i] = high_byte |low_byte;

			  //cinfo->y[i]=  cinfo->y[i] * 800 >> 11;
		
			cinfo->count++;
			
		}
		TPD_DEBUG(" cinfo->x[0] = %d, cinfo->y[0] = %d, cinfo->p[0] = %d\n", cinfo->x[0], cinfo->y[0], cinfo->p[0]);	
		TPD_DEBUG(" cinfo->x[1] = %d, cinfo->y[1] = %d, cinfo->p[1] = %d\n", cinfo->x[1], cinfo->y[1], cinfo->p[1]);		
		TPD_DEBUG(" cinfo->x[2]= %d, cinfo->y[2]= %d, cinfo->p[2] = %d\n", cinfo->x[2], cinfo->y[2], cinfo->p[2]);	
		  
	 return true;

 };

 static int touch_event_handler(void *unused)
 {
  	struct touch_info cinfo, pinfo;
	int i;

	struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
	sched_setscheduler(current, SCHED_RR, &param);
 
	do
	{
		mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM); 
		set_current_state(TASK_INTERRUPTIBLE); 
		wait_event_interruptible(waiter,tpd_flag!=0);
		 
		tpd_flag = 0;

		set_current_state(TASK_RUNNING);

		if (tpd_touchinfo(&cinfo, &pinfo)) 
		{
			//		    printk("point_num = %d, cinfo.x = %d, cinfo.y = %d\n",point_num, cinfo.x[0], cinfo.y[0]);
			TPD_DEBUG_SET_TIME;

			if(point_num >0) 
			{
				for(i=0; i<point_num; i++)
				{
					tpd_down(cinfo.x[i], cinfo.y[i], 1);
				}
				input_sync(tpd->dev);
				TPD_DEBUG("press --->\n");
			} 
			else  
			{
				tpd_up(cinfo.x[0], cinfo.y[0], 0);
				TPD_DEBUG("release --->\n"); 
				input_sync(tpd->dev);
			}
		}
		else
			continue;

 	}while(!kthread_should_stop());
 
	 return 0;
 }
 
 static int tpd_detect (struct i2c_client *client, int kind, struct i2c_board_info *info) 
 {
	 strcpy(info->type, TPD_DEVICE);	
	  return 0;
 }
 
 static void tpd_eint_interrupt_handler(void)
 {
	 tpd_flag = 1;
	 wake_up_interruptible(&waiter);
	 
 }



#if TP_UPGRADE

static int write_reg(u8 addr, u8 para)
{
	char buf[3];
	int ret = -1;

	buf[0] = addr;
	buf[1] = para;
	ret = i2c_master_send(i2c_client, buf, 2);
	if (ret < 0){
		pr_err("write reg failed! %#x ret: %d", buf[0], ret);
		return -1;
	}
	return 0;
}

static int read_reg(u8 addr, unsigned char *pdata)
{
	int ret;
	unsigned char buf[2];
	struct  i2c_msg msgs[2];

	buf[0] = addr;               //register address

	i2c_master_send(i2c_client, &buf[0], 1);
	ret = i2c_master_recv(i2c_client, &buf[0], 1);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);

	*pdata = buf[0];
	return ret;
}

u8 cmd_write(u8 btcmd,u8 btPara1,u8 btPara2,u8 btPara3,u8 num)
{
	u8 write_cmd[4] = {0};
	i2c_client->addr = i2c_client->addr & I2C_MASK_FLAG;
	write_cmd[0] = btcmd;
	write_cmd[1] = btPara1;
	write_cmd[2] = btPara2;
	write_cmd[3] = btPara3;
	return i2c_master_send(i2c_client, write_cmd, num);
}


int  fts_ctpm_fw_upgrade(unsigned char* pbt_buf, long unsigned int dw_lenth)
{
	unsigned char reg_val[3] = {0};
	int i = 0;

	int  packet_number;
	int  j;
	int  temp;
	int  lenght;
	unsigned char  packet_buf[FTS_PACKET_LENGTH + 6];
	unsigned char  auc_i2c_write_buf[10];
	unsigned char bt_ecc;
	int      i_ret;

	/*********Step 1:Reset  CTPM *****/
	/*write 0xaa to register 0xfc*/
	write_reg(0xfc,0xaa);
	mdelay(50);
	/*write 0x55 to register 0xfc*/
	write_reg(0xfc,0x55);
	printk("Step 1: Reset CTPM test\n");

	mdelay(30);   

	/*********Step 2:Enter upgrade mode *****/
	auc_i2c_write_buf[0] = 0x55;
	auc_i2c_write_buf[1] = 0xaa;
	do
	{
		i ++;
	//	i_ret = ft5306_i2c_txdata(auc_i2c_write_buf, 2);
		i_ret = i2c_master_send(i2c_client, auc_i2c_write_buf, 2);
		mdelay(5);
	}while(i_ret <= 0 && i < 5 );
	printk("Step 2: Enter upgrade mode\n");
	
	/*********Step 3:check READ-ID***********************/        
	cmd_write(0x90,0x00,0x00,0x00,4);
	i2c_client->addr = i2c_client->addr & I2C_MASK_FLAG;
	i2c_master_recv(i2c_client, &reg_val, 2);
	printk("Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
	//byte_read(reg_val,2);
	if (reg_val[0] == 0x79 && reg_val[1] == 0x3)
	{
		printk("Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
	}
	else
	{
		return 2;
		//i_is_new_protocol = 1;
	}

	/*********Step 4:erase app*******************************/
	cmd_write(0x61,0x00,0x00,0x00,1);

	mdelay(1500);
	//cmd_write(0x63, 0x00, 0x00, 0x00, 1);
	//mdelay(100);
	printk("Step 4: erase. \n");

	/*********Step 5:write firmware(FW) to ctpm flash*********/
	i2c_client->addr = i2c_client->addr & I2C_MASK_FLAG;
	bt_ecc = 0;
	printk("Step 5: start upgrade. \n");
	dw_lenth = dw_lenth - 8;
	printk("Packet length = %d\n", dw_lenth);
	packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
	packet_buf[0] = 0xbf;
	packet_buf[1] = 0x00;
	for (j=0;j<packet_number;j++)
	{
		temp = j * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8)(temp>>8);
		packet_buf[3] = (u8)temp;
		lenght = FTS_PACKET_LENGTH;
		packet_buf[4] = (u8)(lenght>>8);
		packet_buf[5] = (u8)lenght;

		for (i=0;i<FTS_PACKET_LENGTH;i++)
		{
			packet_buf[6+i] = pbt_buf[j*FTS_PACKET_LENGTH + i];
			bt_ecc ^= packet_buf[6+i];
		}
		i2c_master_send(i2c_client, packet_buf, FTS_PACKET_LENGTH + 6);
		mdelay(50);
		if ((j * FTS_PACKET_LENGTH % 1024) == 0)
		{
			printk("[FT520X] upgrade the 0x%x th byte.\n", ((unsigned int)j) * FTS_PACKET_LENGTH);
		}
	}

	if ((dw_lenth) % FTS_PACKET_LENGTH > 0)
	{
		temp = packet_number * FTS_PACKET_LENGTH;
		packet_buf[2] = (u8)(temp>>8);
		packet_buf[3] = (u8)temp;

		temp = (dw_lenth) % FTS_PACKET_LENGTH;
		packet_buf[4] = (u8)(temp>>8);
		packet_buf[5] = (u8)temp;

		for (i=0;i<temp;i++)
		{
			packet_buf[6+i] = pbt_buf[ packet_number*FTS_PACKET_LENGTH + i]; 
			bt_ecc ^= packet_buf[6+i];
		}
		i2c_master_send(i2c_client, packet_buf, temp+6);
		mdelay(30);
	}

	//send the last six byte
	for (i = 0; i<6; i++)
	{
		packet_buf[0] = 0xbf;
		packet_buf[1] = 0x00;
		temp = 0x6ffa + i;
		packet_buf[2] = (u8)(temp>>8);
		packet_buf[3] = (u8)temp;
		temp =1;
		packet_buf[4] = (u8)(temp>>8);
		packet_buf[5] = (u8)temp;
		packet_buf[6] = pbt_buf[ dw_lenth + i]; 
		bt_ecc ^= packet_buf[6];

		i2c_master_send(i2c_client, packet_buf, 7);    

		mdelay(40);
	}

	/*********Step 6: read out checksum***********************/
	/*send the operation head*/
	cmd_write(0xcc,0x00,0x00,0x00,1);
	i2c_client->addr = i2c_client->addr & I2C_MASK_FLAG;
	i2c_master_recv(i2c_client, &reg_val, 1);
	printk("[FT520X] Step 6:  ecc read 0x%x, new firmware 0x%x. \n", reg_val[0], bt_ecc);
	if(reg_val[0] != bt_ecc)
	{
		printk("5 check sum error!!\n");
		return 5;
	}

	/*********Step 7: reset the new FW***********************/
	cmd_write(0x07,0x00,0x00,0x00,1);
	printk("[FT520X] Step 7: reset the new FW. \n");
	
	/*********Step 8: calibration TP ***********************/
	mdelay(300);          //��ʱ100ms

	return 0;
}

int fts_ctpm_auto_clb(void)
{
    unsigned char uc_temp[1];
    unsigned char i ;

    printk("[FTS] start auto CLB.\n");
    msleep(200);
    write_reg(0, 0x40);  
    mdelay(100);   //make sure already enter factory mode
    write_reg(2, 0x4);  //write command to start calibration
    mdelay(300);
    for(i=0;i<100;i++)
    {
        read_reg(0,uc_temp);
        if ( ((uc_temp[0]&0x70)>>4) == 0x0)  //return to normal mode, calibration finish
        {
            break;
        }
        mdelay(200);
        printk("[FTS] waiting calibration %d\n",i);
        
    }
    printk("[FTS] calibration OK.\n");
    
    msleep(300);
    write_reg(0, 0x40);  //goto factory mode
    mdelay(100);   //make sure already enter factory mode
    write_reg(2, 0x5);  //store CLB result
    mdelay(300);
    write_reg(0, 0x0); //return to normal mode 
    msleep(300);
    printk("[FTS] store CLB result OK.\n");
    return 0;
}

int fts_ctpm_fw_upgrade_with_i_file(unsigned char *buf, long unsigned int  length)
{
	unsigned char*     pbt_buf = NULL;
	int i_ret;
	//=========FW upgrade========================*/
	pbt_buf = buf;
	/*call the upgrade function*/
	i_ret =  fts_ctpm_fw_upgrade(pbt_buf, length);
	if (i_ret != 0)
	{
		printk("[FTS] upgrade failed i_ret = %d.\n", i_ret);
	}
 	  else
	{
  		printk("[FTS] upgrade successfully.\n");
   		fts_ctpm_auto_clb();  //start auto CLB
	 }

   	return i_ret;
}

unsigned char fts_ctpm_get_upg_ver(unsigned char* buff, unsigned long int length)
{
//	unsigned int ui_sz;
//	ui_sz = sizeof(length);
	if (length > 2)
	{
		return buff[length - 2];
	}
	else
	{
		//TBD, error handling?
		return 0xff; //default value
	}
}

#endif



 
 static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
 {	 
	int retval = TPD_OK;
	char data;
	u8 report_rate=0;
	i2c_client = client;
	unsigned char bufff[8] = {0}, buff[8] = {0};
	unsigned char ii;
	printk("xxxxxxxxxxx%sxxxxxxxxxxxx\n", __FUNCTION__);

//rst pin
	mt_set_gpio_mode(GPIO17, 0);
	mt_set_gpio_dir(GPIO17, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO17, GPIO_OUT_ONE);
//enable
	mt_set_gpio_mode(GPIO12, 0);
	mt_set_gpio_dir(GPIO12, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO12, GPIO_OUT_ONE);
    mdelay(20);
//wake
	mt_set_gpio_mode(GPIO52, 0);
	mt_set_gpio_dir(GPIO52, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO52, GPIO_OUT_ONE);

	mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
	mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);

	mt65xx_eint_set_sens(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_SENSITIVE);
	mt65xx_eint_set_hw_debounce(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);
	mt65xx_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, CUST_EINT_TOUCH_PANEL_POLARITY, tpd_eint_interrupt_handler, 1); 
	mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
 
	msleep(100);
 
	if((i2c_smbus_read_i2c_block_data(i2c_client, 0x00, 1, &data))< 0)
	   {
		   TPD_DMESG("I2C transfer error, line: %d\n", __LINE__);
		   return -1; 
	   }

	//set report rate 80Hz
	report_rate = 0x8; 
	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x88, 1, &report_rate))< 0)
	{
	    if((i2c_smbus_write_i2c_block_data(i2c_client, 0x88, 1, &report_rate))< 0)
	    {
		   TPD_DMESG("I2C read report rate error, line: %d\n", __LINE__);
	    }
		   
	}
	mdelay(200);

	read_reg(0xa8, buff);		//hardware ID(truly or junda)
	read_reg(0xa6, bufff);		//firmware ID(0x??)
	printk("xxxxx%s's firmware version is 0x%2x xxxxxxxxxxxx\n", buff[0]==0x5a?version[0] : version[1], bufff[0]);

#if TP_UPGRADE

	if(buff[0] == 0x5a)				//for truly
	{
		if(fts_ctpm_get_upg_ver(CTPM_FW_TRULY, sizeof(CTPM_FW_TRULY)) != bufff[0]){
			printk("Different version, we need version 0x%2x, while now is 0x%2x\n", 
								fts_ctpm_get_upg_ver(CTPM_FW_TRULY, sizeof(CTPM_FW_TRULY)), bufff[0]);
			printk("updating Truly!!!!\n");
			fts_ctpm_fw_upgrade_with_i_file(CTPM_FW_TRULY, sizeof(CTPM_FW_TRULY));
			if(read_reg(0xa6,bufff)<0){
			printk(KERN_ERR"tp upgrade error!\n");
			}
		printk("xxxxxxxxxTruly: after upgrade : new firmware is %2xxxxxxxxxxx\n", bufff[0]);
		}
	}else if(buff[0] == 0x85){		//for junda
		if(fts_ctpm_get_upg_ver(CTPM_FW_JUNDA, sizeof(CTPM_FW_JUNDA)) != bufff[0]){
			printk("Different version, we need version 0x%2x, while now is 0x%2x\n", 
								fts_ctpm_get_upg_ver(CTPM_FW_JUNDA, sizeof(CTPM_FW_JUNDA)), bufff[0]);
			printk("updating Junda!!!!\n");
			fts_ctpm_fw_upgrade_with_i_file(CTPM_FW_JUNDA, sizeof(CTPM_FW_JUNDA));
			if(read_reg(0xa6,bufff)<0){
			printk(KERN_ERR"tp upgrade error!\n");
			}
		printk("xxxxxxxxxJunda: after upgrade : new firmware is %2xxxxxxxxxxx\n", bufff[0]);
		}
	}
#endif

	tpd_load_status = 1;

	thread = kthread_run(touch_event_handler, 0, TPD_DEVICE);
	 if (IS_ERR(thread))
		 { 
		  retval = PTR_ERR(thread);
		  TPD_DMESG(TPD_DEVICE " failed to create kernel thread: %d\n", retval);
		}

	TPD_DMESG("Touch Panel Device Probe %s\n", (retval < TPD_OK) ? "FAIL" : "PASS");
   return 0;
   
 }

 static int __devexit tpd_remove(struct i2c_client *client)
 
 {
   
	 TPD_DEBUG("TPD removed\n");
 
   return 0;
 }
 
 
 static int tpd_local_init(void)
 {

 
  TPD_DMESG("Focaltech FT5206 I2C Touchscreen Driver (Built %s @ %s)\n", __DATE__, __TIME__);
	if(i2c_add_driver(&tpd_i2c_driver)!=0)
	{
		TPD_DMESG("unable to add i2c driver.\n");
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

 static int tpd_resume(struct i2c_client *client)
 {

	printk("TPD wake up\n");

//rst pin
	mt_set_gpio_mode(GPIO17, 0);
	mt_set_gpio_dir(GPIO17, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO17, GPIO_OUT_ONE);
	mt_set_gpio_out(GPIO17, GPIO_OUT_ZERO);
	mdelay(5);
	mt_set_gpio_out(GPIO17, GPIO_OUT_ONE);
	mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);  \

	return 0;
 }
 
 static int tpd_suspend(struct i2c_client *client, pm_message_t message)
 {
	static char data[1] = {3};	
	mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
	i2c_smbus_write_i2c_block_data(i2c_client, 0xA5, 1, &data);  //TP enter sleep mode
	return 0;
 } 


 static struct tpd_driver_t tpd_device_driver = {
	 .tpd_device_name = "FT5206",
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
	printk("MediaTek FT5206 touch panel driver init\n");

	/*delete I2C register board for linux 2.6 by wwl*/
	//i2c_register_board_info(0, &i2c_tpd, 1);


	/*add I2C register driver for linux 2.6 by wwl*/
	if(i2c_add_driver(&tpd_i2c_driver)!=0)
   	{
  		TPD_DMESG("unable to add i2c driver.\n");
      	return -1;
    }
	
	if(tpd_driver_add(&tpd_device_driver) < 0)
		TPD_DMESG("add FT5206 driver failed\n");
	return 0;
 }
 
 /* should never be called */
 static void __exit tpd_driver_exit(void) {
	TPD_DMESG("MediaTek FT5206 touch panel driver exit\n");
	tpd_driver_remove(&tpd_device_driver);
 }
 
module_init(tpd_driver_init);
module_exit(tpd_driver_exit);


