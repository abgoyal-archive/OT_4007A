  
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "sp0718yuv_Sensor.h"
#include "sp0718yuv_Camera_Sensor_para.h"
#include "sp0718yuv_CameraCustomized.h"

//#include <mt6575_gpio.h>

#define SP0718_YUV_DEBUG
#ifdef SP0718_YUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

#define SP0718_NORMAL_Y0ffset  		0x15
#define SP0718_LOWLIGHT_Y0ffset		0x25

//#define DEBUG_SENSOR	//T-card for camera debug on/off
#ifdef DEBUG_SENSOR

kal_uint8 fromsd = 0;
kal_uint16 SP0718_write_cmos_sensor(kal_uint8 addr, kal_uint8 para);

#define SP2528_OP_CODE_INI		0x00		/* Initial value. */
#define SP2528_OP_CODE_REG	0x01		/* Register */
#define SP2528_OP_CODE_DLY	0x02		/* Delay */
#define SP2528_OP_CODE_END	0x03		/* End of initial setting. */


	

	typedef struct
	{
		u16 init_reg;
		u16 init_val;	/* Save the register value and delay tick */
		u8 op_code;		/* 0 - Initial value, 1 - Register, 2 - Delay, 3 - End of setting. */
	} SP2528_initial_set_struct;

	SP2528_initial_set_struct SP2528_Init_Reg[1000];
	
 u32 strtol(const char *nptr, u8 base)
{
	u8 ret;
	if(!nptr || (base!=16 && base!=10 && base!=8))
	{
		printk("%s(): NULL pointer input\n", __FUNCTION__);
		return -1;
	}
	for(ret=0; *nptr; nptr++)
	{
		if((base==16 && *nptr>='A' && *nptr<='F') || 
			(base==16 && *nptr>='a' && *nptr<='f') || 
			(base>=10 && *nptr>='0' && *nptr<='9') ||
			(base>=8 && *nptr>='0' && *nptr<='7') )
		{
			ret *= base;
			if(base==16 && *nptr>='A' && *nptr<='F')
				ret += *nptr-'A'+10;
			else if(base==16 && *nptr>='a' && *nptr<='f')
				ret += *nptr-'a'+10;
			else if(base>=10 && *nptr>='0' && *nptr<='9')
				ret += *nptr-'0';
			else if(base>=8 && *nptr>='0' && *nptr<='7')
				ret += *nptr-'0';
		}
		else
			return ret;
	}
	return ret;
}

 u8 SP2528_Initialize_from_T_Flash()
{
	//FS_HANDLE fp = -1;				/* Default, no file opened. */
	//u8 *data_buff = NULL;
	u8 *curr_ptr = NULL;
	u32 file_size = 0;
	//u32 bytes_read = 0;
	u32 i = 0, j = 0;
	u8 func_ind[4] = {0};	/* REG or DLY */

	
	struct file *fp; 
    mm_segment_t fs; 
    loff_t pos = 0; 
	static u8 data_buff[10*1024] ;
    fp = filp_open("/mnt/sdcard/sp0718_sd.dat", O_RDONLY , 0); 
 
  //  fp = filp_open("/storage/sdcard1/sp0718_sd.dat", O_RDONLY , 0); 
    if (IS_ERR(fp)) { 
		printk("create file error 0\n");
 fp = filp_open("/mnt/sdcard/sp0718_sd.dat", O_RDONLY , 0); 

		//fp = filp_open("/storage/sdcard0/sp0718_sd.dat", O_RDONLY , 0); 
    	if (IS_ERR(fp)) { 
	        printk("create file error 1\n"); 
	        return 2;//-1; 
    	}
    } 
    else
		printk("SP0718_Initialize_from_T_Flash Open File Success\n");
	
    fs = get_fs(); 
    set_fs(KERNEL_DS); 

    file_size = vfs_llseek(fp, 0, SEEK_END);
    vfs_read(fp, data_buff, file_size, &pos); 
    filp_close(fp, NULL); 
    set_fs(fs);
	
	

printk("1\n");

	/* Start parse the setting witch read from t-flash. */
	curr_ptr = data_buff;
	while (curr_ptr < (data_buff + file_size))
	{
		while ((*curr_ptr == ' ') || (*curr_ptr == '\t'))/* Skip the Space & TAB */
			curr_ptr++;				

		if (((*curr_ptr) == '/') && ((*(curr_ptr + 1)) == '*'))
		{
			while (!(((*curr_ptr) == '*') && ((*(curr_ptr + 1)) == '/')))
			{
				curr_ptr++;		/* Skip block comment code. */
			}

			while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
			{
				curr_ptr++;
			}

			curr_ptr += 2;						/* Skip the enter line */
			
			continue ;
		}
		
		if (((*curr_ptr) == '/') || ((*curr_ptr) == '{') || ((*curr_ptr) == '}'))		/* Comment line, skip it. */
		{
			while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
			{
				curr_ptr++;
			}

			curr_ptr += 2;						/* Skip the enter line */

			continue ;
		}
		/* This just content one enter line. */
		if (((*curr_ptr) == 0x0D) && ((*(curr_ptr + 1)) == 0x0A))
		{
			curr_ptr += 2;
			continue ;
		}
		//printk(" curr_ptr1 = %s\n",curr_ptr);
		memcpy(func_ind, curr_ptr, 3);
	
						
		if (strcmp((const char *)func_ind, "REG") == 0)		/* REG */
		{
			curr_ptr += 6;				/* Skip "REG(0x" or "DLY(" */
			SP2528_Init_Reg[i].op_code = SP2528_OP_CODE_REG;
			
			SP2528_Init_Reg[i].init_reg = strtol((const char *)curr_ptr, 16);
			curr_ptr += 5;	/* Skip "00, 0x" */
		
			SP2528_Init_Reg[i].init_val = strtol((const char *)curr_ptr, 16);
			curr_ptr += 4;	/* Skip "00);" */
		
		}
		else									/* DLY */
		{
			/* Need add delay for this setting. */
			curr_ptr += 4;	
			SP2528_Init_Reg[i].op_code = SP2528_OP_CODE_DLY;
			
			SP2528_Init_Reg[i].init_reg = 0xFF;
			SP2528_Init_Reg[i].init_val = strtol((const char *)curr_ptr,  10);	/* Get the delay ticks, the delay should less then 50 */
		}
		i++;
		

		/* Skip to next line directly. */
		while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
		{
			curr_ptr++;
		}
		curr_ptr += 2;
	}
printk("2\n");
	/* (0xFFFF, 0xFFFF) means the end of initial setting. */
	SP2528_Init_Reg[i].op_code = SP2528_OP_CODE_END;
	SP2528_Init_Reg[i].init_reg = 0xFF;
	SP2528_Init_Reg[i].init_val = 0xFF;
	i++;
	//for (j=0; j<i; j++)
		//printk(" %x  ==  %x\n",SP2528_Init_Reg[j].init_reg, SP2528_Init_Reg[j].init_val);

	/* Start apply the initial setting to sensor. */
	#if 1
	for (j=0; j<i; j++)
	{
		if (SP2528_Init_Reg[j].op_code == SP2528_OP_CODE_END)	/* End of the setting. */
		{
			break ;
		}
		else if (SP2528_Init_Reg[j].op_code == SP2528_OP_CODE_DLY)
		{
			msleep(SP2528_Init_Reg[j].init_val);		/* Delay */
		}
		else if (SP2528_Init_Reg[j].op_code == SP2528_OP_CODE_REG)
		{
		
			SP0718_write_cmos_sensor((kal_uint8)SP2528_Init_Reg[j].init_reg, (kal_uint8)SP2528_Init_Reg[j].init_val);
		}
		else
		{
			printk("REG ERROR!\n");
		}
	}
#endif

printk("3\n");
	return 1;	
}

#endif
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

kal_uint16 SP0718_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
    char puSendCmd[2] = {(char)(addr & 0xFF) , (char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 2, SP0718_WRITE_ID);

}
kal_uint16 SP0718_read_cmos_sensor(kal_uint8 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd = { (char)(addr & 0xFF) };
	iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte, 1, SP0718_WRITE_ID);
	
    return get_byte;
}


#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT



kal_bool   SP0718_MPEG4_encode_mode = KAL_FALSE;
kal_uint16 SP0718_dummy_pixels = 0, SP0718_dummy_lines = 0;
kal_bool   SP0718_MODE_CAPTURE = KAL_FALSE;
kal_bool   SP0718_NIGHT_MODE = KAL_FALSE;

kal_uint32 SP0718_isp_master_clock;
static kal_uint32 SP0718_g_fPV_PCLK = 24;

kal_uint8 SP0718_sensor_write_I2C_address = SP0718_WRITE_ID;
kal_uint8 SP0718_sensor_read_I2C_address = SP0718_READ_ID;

UINT8 SP0718_PixelClockDivider=0;

kal_uint8 sp0718_isBanding = 0; // 0: 50hz  1:60hz

MSDK_SENSOR_CONFIG_STRUCT SP0718_SensorConfigData;



void SP0718_Set_Shutter(kal_uint16 iShutter)
{
} /* Set_SP0718_Shutter */


kal_uint16 SP0718_Read_Shutter(void)
{
} /* SP0718_read_shutter */


static void SP0718_awb_enable(kal_bool enalbe)
{	 
	kal_uint16 temp_AWB_reg = 0;
#if 0
	temp_AWB_reg = SP0718_read_cmos_sensor(0x42);
	
	if (enalbe)
	{
		SP0718_write_cmos_sensor(0x42, (temp_AWB_reg |0x02));
	}
	else
	{
		SP0718_write_cmos_sensor(0x42, (temp_AWB_reg & (~0x02)));
	}
#endif
}


void SP0718_config_window(kal_uint16 startx, kal_uint16 starty, kal_uint16 width, kal_uint16 height)
{
} /* SP0718_config_window */


kal_uint16 SP0718_SetGain(kal_uint16 iGain)
{
	return iGain;
}

void SP0718_NightMode(kal_bool bEnable)
{
	SENSORDB("%s   %d\n", __func__, bEnable);
	if(bEnable)//night mode
	{ 
		SP0718_NIGHT_MODE = KAL_TRUE;		
		SP0718_write_cmos_sensor(0xfd,0x01);
		SP0718_write_cmos_sensor(0xcd,SP0718_LOWLIGHT_Y0ffset);
		SP0718_write_cmos_sensor(0xce,0x1f);
		if(SP0718_MPEG4_encode_mode == KAL_TRUE)
		{
			if(sp0718_isBanding== 0)
			{
				//SI15_SP0718 24M 50Hz 10-10fps AE_Parameters_20121210104002.txt
				//ae setting
				SP0718_write_cmos_sensor(0xfd,0x00);
				SP0718_write_cmos_sensor(0x03,0x01);
				SP0718_write_cmos_sensor(0x04,0x32);
				SP0718_write_cmos_sensor(0x06,0x00);
				SP0718_write_cmos_sensor(0x09,0x05);
				SP0718_write_cmos_sensor(0x0a,0xdf);
				SP0718_write_cmos_sensor(0xfd,0x01);
				SP0718_write_cmos_sensor(0xef,0x33);
				SP0718_write_cmos_sensor(0xf0,0x00);
				SP0718_write_cmos_sensor(0x02,0x0a);
				SP0718_write_cmos_sensor(0x03,0x01);
				SP0718_write_cmos_sensor(0x06,0x2d);
				SP0718_write_cmos_sensor(0x07,0x00);
				SP0718_write_cmos_sensor(0x08,0x01);
				SP0718_write_cmos_sensor(0x09,0x00);
				//Status
				SP0718_write_cmos_sensor(0xfd,0x02);
				SP0718_write_cmos_sensor(0xbe,0xfe);
				SP0718_write_cmos_sensor(0xbf,0x01);
				SP0718_write_cmos_sensor(0xd0,0xfe);
				SP0718_write_cmos_sensor(0xd1,0x01);
				SP0718_write_cmos_sensor(0xfd,0x01);
				SP0718_write_cmos_sensor(0x5b,0x01);
				SP0718_write_cmos_sensor(0x5c,0xfe);

				SENSORDB(" video 50Hz night\r\n");
			}
			else if(sp0718_isBanding == 1)
			{
				//SI15_SP0718 24M 60Hz 10.1176-10fps AE_Parameters_20121210104008.txt
				//ae setting
				SP0718_write_cmos_sensor(0xfd,0x00);
				SP0718_write_cmos_sensor(0x03,0x01);
				SP0718_write_cmos_sensor(0x04,0x02);
				SP0718_write_cmos_sensor(0x06,0x00);
				SP0718_write_cmos_sensor(0x09,0x05);
				SP0718_write_cmos_sensor(0x0a,0xc4);
				SP0718_write_cmos_sensor(0xfd,0x01);
				SP0718_write_cmos_sensor(0xef,0x2b);
				SP0718_write_cmos_sensor(0xf0,0x00);
				SP0718_write_cmos_sensor(0x02,0x0c);
				SP0718_write_cmos_sensor(0x03,0x01);
				SP0718_write_cmos_sensor(0x06,0x25);
				SP0718_write_cmos_sensor(0x07,0x00);
				SP0718_write_cmos_sensor(0x08,0x01);
				SP0718_write_cmos_sensor(0x09,0x00);
				//Status
				SP0718_write_cmos_sensor(0xfd,0x02);
				SP0718_write_cmos_sensor(0xbe,0x04);
				SP0718_write_cmos_sensor(0xbf,0x02);
				SP0718_write_cmos_sensor(0xd0,0x04);
				SP0718_write_cmos_sensor(0xd1,0x02);
				SP0718_write_cmos_sensor(0xfd,0x01);
				SP0718_write_cmos_sensor(0x5b,0x02);
				SP0718_write_cmos_sensor(0x5c,0x04);

				SENSORDB(" video 60Hz night\r\n");
			}
		}	
		else 
		{  
			if(sp0718_isBanding== 0)
			{
				//SI15_SP0718 24M 50Hz 20-6fps AE_Parameters_20121210101250.txt
				//ae setting
				SP0718_write_cmos_sensor(0xfd,0x00);
				SP0718_write_cmos_sensor(0x03,0x02);
				SP0718_write_cmos_sensor(0x04,0x64);
				SP0718_write_cmos_sensor(0x06,0x00);
				SP0718_write_cmos_sensor(0x09,0x01);
				SP0718_write_cmos_sensor(0x0a,0x46);
				SP0718_write_cmos_sensor(0xfd,0x01);
				SP0718_write_cmos_sensor(0xef,0x66);
				SP0718_write_cmos_sensor(0xf0,0x00);
				SP0718_write_cmos_sensor(0x02,0x10);
				SP0718_write_cmos_sensor(0x03,0x01);
				SP0718_write_cmos_sensor(0x06,0x60);
				SP0718_write_cmos_sensor(0x07,0x00);
				SP0718_write_cmos_sensor(0x08,0x01);
				SP0718_write_cmos_sensor(0x09,0x00);
				//Status
				SP0718_write_cmos_sensor(0xfd,0x02);
				SP0718_write_cmos_sensor(0xbe,0x60);
				SP0718_write_cmos_sensor(0xbf,0x06);
				SP0718_write_cmos_sensor(0xd0,0x60);
				SP0718_write_cmos_sensor(0xd1,0x06);
				SP0718_write_cmos_sensor(0xfd,0x01);
				SP0718_write_cmos_sensor(0x5b,0x06);
				SP0718_write_cmos_sensor(0x5c,0x60);

				SENSORDB(" priview 50Hz night\r\n");	
			}  
			else if(sp0718_isBanding== 1)
			{
				//SI15_SP0718 24M 60Hz 20-6fps AE_Parameters_20121210101304.txt
				//ae setting
				SP0718_write_cmos_sensor(0xfd,0x00);
				SP0718_write_cmos_sensor(0x03,0x01);
				SP0718_write_cmos_sensor(0x04,0xfe);
				SP0718_write_cmos_sensor(0x06,0x00);
				SP0718_write_cmos_sensor(0x09,0x01);
				SP0718_write_cmos_sensor(0x0a,0x46);
				SP0718_write_cmos_sensor(0xfd,0x01);
				SP0718_write_cmos_sensor(0xef,0x55);
				SP0718_write_cmos_sensor(0xf0,0x00);
				SP0718_write_cmos_sensor(0x02,0x14);
				SP0718_write_cmos_sensor(0x03,0x01);
				SP0718_write_cmos_sensor(0x06,0x4f);
				SP0718_write_cmos_sensor(0x07,0x00);
				SP0718_write_cmos_sensor(0x08,0x01);
				SP0718_write_cmos_sensor(0x09,0x00);
				//Status
				SP0718_write_cmos_sensor(0xfd,0x02);
				SP0718_write_cmos_sensor(0xbe,0xa4);
				SP0718_write_cmos_sensor(0xbf,0x06);
				SP0718_write_cmos_sensor(0xd0,0xa4);
				SP0718_write_cmos_sensor(0xd1,0x06);
				SP0718_write_cmos_sensor(0xfd,0x01);
				SP0718_write_cmos_sensor(0x5b,0x06);
				SP0718_write_cmos_sensor(0x5c,0xa4);

				SENSORDB(" priview 60Hz night\r\n");	
			}
		} 		
	}
	else    // daylight mode
	{
		SP0718_NIGHT_MODE = KAL_FALSE;	
		SP0718_write_cmos_sensor(0xfd,0x01);
		SP0718_write_cmos_sensor(0xcd,SP0718_NORMAL_Y0ffset);
		SP0718_write_cmos_sensor(0xce,0x1f);
		if(SP0718_MPEG4_encode_mode == KAL_TRUE)
		{
			if(sp0718_isBanding== 0)
			{
				//SI15_SP0718 24M 50Hz 20-20fps AE_Parameters_20121210103954.txt
				//ae setting
				SP0718_write_cmos_sensor(0xfd,0x00);
				SP0718_write_cmos_sensor(0x03,0x02);
				SP0718_write_cmos_sensor(0x04,0x64);
				SP0718_write_cmos_sensor(0x06,0x00);
				SP0718_write_cmos_sensor(0x09,0x01);
				SP0718_write_cmos_sensor(0x0a,0x46);
				SP0718_write_cmos_sensor(0xfd,0x01);
				SP0718_write_cmos_sensor(0xef,0x66);
				SP0718_write_cmos_sensor(0xf0,0x00);
				SP0718_write_cmos_sensor(0x02,0x05);
				SP0718_write_cmos_sensor(0x03,0x01);
				SP0718_write_cmos_sensor(0x06,0x60);
				SP0718_write_cmos_sensor(0x07,0x00);
				SP0718_write_cmos_sensor(0x08,0x01);
				SP0718_write_cmos_sensor(0x09,0x00);
				//Status
				SP0718_write_cmos_sensor(0xfd,0x02);
				SP0718_write_cmos_sensor(0xbe,0xfe);
				SP0718_write_cmos_sensor(0xbf,0x01);
				SP0718_write_cmos_sensor(0xd0,0xfe);
				SP0718_write_cmos_sensor(0xd1,0x01);
				SP0718_write_cmos_sensor(0xfd,0x01);
				SP0718_write_cmos_sensor(0x5b,0x01);
				SP0718_write_cmos_sensor(0x5c,0xfe);

				SENSORDB(" video 50Hz normal\r\n");				
			}
			else if(sp0718_isBanding == 1)
			{
				//SI15_SP0718 24M 60Hz 20-20fps AE_Parameters_20121210103936.txt
				//ae setting
				SP0718_write_cmos_sensor(0xfd,0x00);
				SP0718_write_cmos_sensor(0x03,0x01);
				SP0718_write_cmos_sensor(0x04,0xfe);
				SP0718_write_cmos_sensor(0x06,0x00);
				SP0718_write_cmos_sensor(0x09,0x01);
				SP0718_write_cmos_sensor(0x0a,0x46);
				SP0718_write_cmos_sensor(0xfd,0x01);
				SP0718_write_cmos_sensor(0xef,0x55);
				SP0718_write_cmos_sensor(0xf0,0x00);
				SP0718_write_cmos_sensor(0x02,0x06);
				SP0718_write_cmos_sensor(0x03,0x01);
				SP0718_write_cmos_sensor(0x06,0x4f);
				SP0718_write_cmos_sensor(0x07,0x00);
				SP0718_write_cmos_sensor(0x08,0x01);
				SP0718_write_cmos_sensor(0x09,0x00);
				//Status
				SP0718_write_cmos_sensor(0xfd,0x02);
				SP0718_write_cmos_sensor(0xbe,0xfe);
				SP0718_write_cmos_sensor(0xbf,0x01);
				SP0718_write_cmos_sensor(0xd0,0xfe);
				SP0718_write_cmos_sensor(0xd1,0x01);
				SP0718_write_cmos_sensor(0xfd,0x01);
				SP0718_write_cmos_sensor(0x5b,0x01);
				SP0718_write_cmos_sensor(0x5c,0xfe);

				SENSORDB(" video 60Hz normal \n");	
			}
		}
		else 
		{
			if(sp0718_isBanding== 0)
			{
				//SI15_SP0718 24M 50Hz 20-8fps AE_Parameters_20121208114739.txt
				//ae setting
				SP0718_write_cmos_sensor(0xfd,0x00);
				SP0718_write_cmos_sensor(0x03,0x02);
				SP0718_write_cmos_sensor(0x04,0x64);
				SP0718_write_cmos_sensor(0x06,0x00);
				SP0718_write_cmos_sensor(0x09,0x01);
				SP0718_write_cmos_sensor(0x0a,0x46);
				SP0718_write_cmos_sensor(0xfd,0x01);
				SP0718_write_cmos_sensor(0xef,0x66);
				SP0718_write_cmos_sensor(0xf0,0x00);
				SP0718_write_cmos_sensor(0x02,0x0c);
				SP0718_write_cmos_sensor(0x03,0x01);
				SP0718_write_cmos_sensor(0x06,0x60);
				SP0718_write_cmos_sensor(0x07,0x00);
				SP0718_write_cmos_sensor(0x08,0x01);
				SP0718_write_cmos_sensor(0x09,0x00);
				//Status
				SP0718_write_cmos_sensor(0xfd,0x02);
				SP0718_write_cmos_sensor(0xbe,0xc8);
				SP0718_write_cmos_sensor(0xbf,0x04);
				SP0718_write_cmos_sensor(0xd0,0xc8);
				SP0718_write_cmos_sensor(0xd1,0x04);
				SP0718_write_cmos_sensor(0xfd,0x01);
				SP0718_write_cmos_sensor(0x5b,0x04);
				SP0718_write_cmos_sensor(0x5c,0xc8);

				SENSORDB(" priview 50Hz normal\r\n");
			}
			else if(sp0718_isBanding== 1)
			{
				//SI15_SP0718 24M 60Hz 20-8fps AE_Parameters_20121210101304.txt
				//ae setting
				SP0718_write_cmos_sensor(0xfd,0x00);
				SP0718_write_cmos_sensor(0x03,0x01);
				SP0718_write_cmos_sensor(0x04,0xfe);
				SP0718_write_cmos_sensor(0x06,0x00);
				SP0718_write_cmos_sensor(0x09,0x01);
				SP0718_write_cmos_sensor(0x0a,0x46);
				SP0718_write_cmos_sensor(0xfd,0x01);
				SP0718_write_cmos_sensor(0xef,0x55);
				SP0718_write_cmos_sensor(0xf0,0x00);
				SP0718_write_cmos_sensor(0x02,0x0f);
				SP0718_write_cmos_sensor(0x03,0x01);
				SP0718_write_cmos_sensor(0x06,0x4f);
				SP0718_write_cmos_sensor(0x07,0x00);
				SP0718_write_cmos_sensor(0x08,0x01);
				SP0718_write_cmos_sensor(0x09,0x00);
				//Status
				SP0718_write_cmos_sensor(0xfd,0x02);
				SP0718_write_cmos_sensor(0xbe,0xfb);
				SP0718_write_cmos_sensor(0xbf,0x04);
				SP0718_write_cmos_sensor(0xd0,0xfb);
				SP0718_write_cmos_sensor(0xd1,0x04);
				SP0718_write_cmos_sensor(0xfd,0x01);
				SP0718_write_cmos_sensor(0x5b,0x04);
				SP0718_write_cmos_sensor(0x5c,0xfb);

				SENSORDB(" priview 60Hz normal\r\n");
			}
		}
	}  
}

void SP0718_Sensor_Init(void)
{
	SENSORDB("%s", __func__);
	#if 1
	SP0718_write_cmos_sensor(0xfd,0x00);
	SP0718_write_cmos_sensor(0x1C,0x14);
	SP0718_write_cmos_sensor(0x31,0x60);
	SP0718_write_cmos_sensor(0x27,0xb3);
	SP0718_write_cmos_sensor(0x1b,0x17);
	SP0718_write_cmos_sensor(0x26,0xaa);
	SP0718_write_cmos_sensor(0x37,0x02);
	SP0718_write_cmos_sensor(0x28,0x8f);
	SP0718_write_cmos_sensor(0x1a,0x73);
	SP0718_write_cmos_sensor(0x1e,0x1b);
	SP0718_write_cmos_sensor(0x21,0x06);
	SP0718_write_cmos_sensor(0x22,0x2a);
	SP0718_write_cmos_sensor(0x0f,0x3f);
	SP0718_write_cmos_sensor(0x10,0x3e);
	SP0718_write_cmos_sensor(0x11,0x00);
	SP0718_write_cmos_sensor(0x12,0x01);
	SP0718_write_cmos_sensor(0x13,0x3f);
	SP0718_write_cmos_sensor(0x14,0x04);
	SP0718_write_cmos_sensor(0x15,0x30);
	SP0718_write_cmos_sensor(0x16,0x31);
	SP0718_write_cmos_sensor(0x17,0x01);
	SP0718_write_cmos_sensor(0x69,0x31);
	SP0718_write_cmos_sensor(0x6a,0x2a);
	SP0718_write_cmos_sensor(0x6b,0x33);
	SP0718_write_cmos_sensor(0x6c,0x1a);
	SP0718_write_cmos_sensor(0x6d,0x32);
	SP0718_write_cmos_sensor(0x6e,0x28);
	SP0718_write_cmos_sensor(0x6f,0x29);
	SP0718_write_cmos_sensor(0x70,0x34);
	SP0718_write_cmos_sensor(0x71,0x18);
	SP0718_write_cmos_sensor(0x36,0x00);
	SP0718_write_cmos_sensor(0xfd,0x01);
	SP0718_write_cmos_sensor(0x5d,0x51);
	SP0718_write_cmos_sensor(0xf2,0x19);
	           
	SP0718_write_cmos_sensor(0x1f,0x10);
	SP0718_write_cmos_sensor(0x20,0x1f);
	           
	SP0718_write_cmos_sensor(0xfd,0x02);
	SP0718_write_cmos_sensor(0x00,0x88);
	SP0718_write_cmos_sensor(0x01,0x88);


	//SI15_SP0718 24M 50Hz 20-8fps
	//ae setting
	SP0718_write_cmos_sensor(0xfd,0x00);
	SP0718_write_cmos_sensor(0x03,0x02);
	SP0718_write_cmos_sensor(0x04,0x64);
	SP0718_write_cmos_sensor(0x06,0x00);
	SP0718_write_cmos_sensor(0x09,0x01);
	SP0718_write_cmos_sensor(0x0a,0x46);
	SP0718_write_cmos_sensor(0xfd,0x01);
	SP0718_write_cmos_sensor(0xef,0x66);
	SP0718_write_cmos_sensor(0xf0,0x00);
	SP0718_write_cmos_sensor(0x02,0x0c);
	SP0718_write_cmos_sensor(0x03,0x01);
	SP0718_write_cmos_sensor(0x06,0x60);
	SP0718_write_cmos_sensor(0x07,0x00);
	SP0718_write_cmos_sensor(0x08,0x01);
	SP0718_write_cmos_sensor(0x09,0x00);
	//Status
	SP0718_write_cmos_sensor(0xfd,0x02);
	SP0718_write_cmos_sensor(0xbe,0xc8);
	SP0718_write_cmos_sensor(0xbf,0x04);
	SP0718_write_cmos_sensor(0xd0,0xc8);
	SP0718_write_cmos_sensor(0xd1,0x04);
	SP0718_write_cmos_sensor(0xfd,0x01);
	SP0718_write_cmos_sensor(0x5b,0x04);
	SP0718_write_cmos_sensor(0x5c,0xc8);

	           
	SP0718_write_cmos_sensor(0xfd,0x01);
	SP0718_write_cmos_sensor(0xe0,0x40);
	SP0718_write_cmos_sensor(0xe1,0x30);
	SP0718_write_cmos_sensor(0xe2,0x2e);
	SP0718_write_cmos_sensor(0xe3,0x2a);
	SP0718_write_cmos_sensor(0xe4,0x2a);
	SP0718_write_cmos_sensor(0xe5,0x28);
	SP0718_write_cmos_sensor(0xe6,0x28);
	SP0718_write_cmos_sensor(0xe7,0x26);
	SP0718_write_cmos_sensor(0xe8,0x26);
	SP0718_write_cmos_sensor(0xe9,0x26);
	SP0718_write_cmos_sensor(0xea,0x26);
	SP0718_write_cmos_sensor(0xf3,0x26);
	SP0718_write_cmos_sensor(0xf4,0x26);
	           
	SP0718_write_cmos_sensor(0xfd,0x01);
	SP0718_write_cmos_sensor(0x04,0xe0);
	SP0718_write_cmos_sensor(0x05,0x26);
	SP0718_write_cmos_sensor(0x0a,0xa0);
	SP0718_write_cmos_sensor(0x0b,0x26);
	SP0718_write_cmos_sensor(0x5a,0x60);
	SP0718_write_cmos_sensor(0xfd,0x02);
	SP0718_write_cmos_sensor(0xbc,0xa0);
	SP0718_write_cmos_sensor(0xbd,0x80);
	SP0718_write_cmos_sensor(0xb8,0x80);
	SP0718_write_cmos_sensor(0xb9,0x90);
	           
	SP0718_write_cmos_sensor(0xfd,0x01);
	SP0718_write_cmos_sensor(0xeb,0x80);
	SP0718_write_cmos_sensor(0xec,0x78);
	SP0718_write_cmos_sensor(0xed,0x0a);
	SP0718_write_cmos_sensor(0xee,0x10);
	           
	SP0718_write_cmos_sensor(0xfd,0x01);
	SP0718_write_cmos_sensor(0x26,0x30);
	SP0718_write_cmos_sensor(0x27,0x2c);
	SP0718_write_cmos_sensor(0x28,0x07);
	SP0718_write_cmos_sensor(0x29,0x08);

	SP0718_write_cmos_sensor(0x2a,0x54);
	SP0718_write_cmos_sensor(0x2b,0x03);
	SP0718_write_cmos_sensor(0x2c,0x54);
	SP0718_write_cmos_sensor(0x2d,0x00);
	           
	SP0718_write_cmos_sensor(0xfd,0x01);

	SP0718_write_cmos_sensor(0xa1,0x20);
	SP0718_write_cmos_sensor(0xa2,0x15);
	SP0718_write_cmos_sensor(0xa3,0x17);
	SP0718_write_cmos_sensor(0xa4,0x12);
	SP0718_write_cmos_sensor(0xa5,0x13);
	SP0718_write_cmos_sensor(0xa6,0x09);
	SP0718_write_cmos_sensor(0xa7,0x0b);
	SP0718_write_cmos_sensor(0xa8,0x03);
	SP0718_write_cmos_sensor(0xa9,0x17);
	SP0718_write_cmos_sensor(0xaa,0x10);
	SP0718_write_cmos_sensor(0xab,0x14);
	SP0718_write_cmos_sensor(0xac,0x0c);
	SP0718_write_cmos_sensor(0xad,0x04);
	SP0718_write_cmos_sensor(0xae,0x02);
	SP0718_write_cmos_sensor(0xaf,0x03);
	SP0718_write_cmos_sensor(0xb0,0x01);
	SP0718_write_cmos_sensor(0xb1,0x01);
	SP0718_write_cmos_sensor(0xb2,0x01);
	SP0718_write_cmos_sensor(0xb3,0x01);
	SP0718_write_cmos_sensor(0xb4,0x03);
	SP0718_write_cmos_sensor(0xb5,0x07);
	SP0718_write_cmos_sensor(0xb6,0x0a);
	SP0718_write_cmos_sensor(0xb7,0x06);
	SP0718_write_cmos_sensor(0xb8,0x07);
	           
	SP0718_write_cmos_sensor(0xfd,0x01);
	SP0718_write_cmos_sensor(0x48,0x09);
	SP0718_write_cmos_sensor(0x49,0x99);
	           
	SP0718_write_cmos_sensor(0xfd,0x01);
	SP0718_write_cmos_sensor(0x32,0x15);
	SP0718_write_cmos_sensor(0xfd,0x00);
	SP0718_write_cmos_sensor(0xe7,0x03);
	SP0718_write_cmos_sensor(0xfd,0x02);
	SP0718_write_cmos_sensor(0x26,0xb0);
	SP0718_write_cmos_sensor(0x27,0xb6);
	SP0718_write_cmos_sensor(0xfd,0x00);
	SP0718_write_cmos_sensor(0xe7,0x00);
	SP0718_write_cmos_sensor(0xfd,0x02);
	SP0718_write_cmos_sensor(0x1b,0x80);
	SP0718_write_cmos_sensor(0x1a,0x80);
	SP0718_write_cmos_sensor(0x18,0x26);
	SP0718_write_cmos_sensor(0x19,0x28);
	SP0718_write_cmos_sensor(0xfd,0x02);
	SP0718_write_cmos_sensor(0x2a,0x00);
	SP0718_write_cmos_sensor(0x2b,0x00);
	SP0718_write_cmos_sensor(0x28,0xef);
	SP0718_write_cmos_sensor(0x29,0x10);
	           
	SP0718_write_cmos_sensor(0x66,0x40);
	SP0718_write_cmos_sensor(0x67,0x64);
	SP0718_write_cmos_sensor(0x68,0xe6);
	SP0718_write_cmos_sensor(0x69,0x06);
	SP0718_write_cmos_sensor(0x6a,0xa6);
	           
	SP0718_write_cmos_sensor(0x7c,0x58);
	SP0718_write_cmos_sensor(0x7d,0x78);
	SP0718_write_cmos_sensor(0x7e,0x06);
	SP0718_write_cmos_sensor(0x7f,0x2a);
	SP0718_write_cmos_sensor(0x80,0xaa);
	           
	SP0718_write_cmos_sensor(0x70,0x28);
	SP0718_write_cmos_sensor(0x71,0x4f);
	SP0718_write_cmos_sensor(0x72,0x3d);
	SP0718_write_cmos_sensor(0x73,0x5d);
	SP0718_write_cmos_sensor(0x74,0xaa);
	           
	SP0718_write_cmos_sensor(0x6b,0x12);
	SP0718_write_cmos_sensor(0x6c,0x38);
	SP0718_write_cmos_sensor(0x6d,0x3b);
	SP0718_write_cmos_sensor(0x6e,0x5b);
	SP0718_write_cmos_sensor(0x6f,0xaa);
	           
	SP0718_write_cmos_sensor(0x61,0xe4);
	SP0718_write_cmos_sensor(0x62,0x0a);
	SP0718_write_cmos_sensor(0x63,0x70);
	SP0718_write_cmos_sensor(0x64,0x90);
	SP0718_write_cmos_sensor(0x65,0x6a);
	           
	SP0718_write_cmos_sensor(0x75,0x80);
	SP0718_write_cmos_sensor(0x76,0x0a);
	SP0718_write_cmos_sensor(0x77,0x00);
	SP0718_write_cmos_sensor(0x24,0x25);
	           
	SP0718_write_cmos_sensor(0x0e,0x30);
	SP0718_write_cmos_sensor(0x09,0x07);
	           
	SP0718_write_cmos_sensor(0x31,0x60);
	SP0718_write_cmos_sensor(0x32,0x60);
	SP0718_write_cmos_sensor(0x33,0xc0);
	SP0718_write_cmos_sensor(0x35,0x6f);
	SP0718_write_cmos_sensor(0x3b,0x10);
	           
	SP0718_write_cmos_sensor(0xfd,0x02);
	SP0718_write_cmos_sensor(0xde,0x0f);

	SP0718_write_cmos_sensor(0xd2,0x09);
	SP0718_write_cmos_sensor(0xd3,0x09);
	SP0718_write_cmos_sensor(0xd4,0x0a);
	SP0718_write_cmos_sensor(0xd5,0x0a);

	SP0718_write_cmos_sensor(0xd7,0x20);
	SP0718_write_cmos_sensor(0xd8,0x30);
	SP0718_write_cmos_sensor(0xd9,0x38);
	SP0718_write_cmos_sensor(0xda,0x38);
	SP0718_write_cmos_sensor(0xdb,0x08);

	SP0718_write_cmos_sensor(0xe8,0x40);
	SP0718_write_cmos_sensor(0xe9,0x40);
	SP0718_write_cmos_sensor(0xea,0x30);
	SP0718_write_cmos_sensor(0xeb,0x20);

	SP0718_write_cmos_sensor(0xec,0x48);
	SP0718_write_cmos_sensor(0xed,0x48);
	SP0718_write_cmos_sensor(0xee,0x30);
	SP0718_write_cmos_sensor(0xef,0x20);
	           
	SP0718_write_cmos_sensor(0xf3,0x50);
	SP0718_write_cmos_sensor(0xf4,0x10);
	SP0718_write_cmos_sensor(0xf5,0x10);
	SP0718_write_cmos_sensor(0xf6,0x10);
	           
	SP0718_write_cmos_sensor(0xfd,0x01);
	SP0718_write_cmos_sensor(0x64,0x44);
	SP0718_write_cmos_sensor(0x65,0x22);
	SP0718_write_cmos_sensor(0x6d,0x04);
	SP0718_write_cmos_sensor(0x6e,0x06);
	SP0718_write_cmos_sensor(0x6f,0x10);
	SP0718_write_cmos_sensor(0x70,0x10);
	SP0718_write_cmos_sensor(0x71,0x08);
	SP0718_write_cmos_sensor(0x72,0x12);
	SP0718_write_cmos_sensor(0x73,0x1c);
	SP0718_write_cmos_sensor(0x74,0x24);
	SP0718_write_cmos_sensor(0x75,0x44);
	SP0718_write_cmos_sensor(0x76,0x02);
	SP0718_write_cmos_sensor(0x77,0x02);
	SP0718_write_cmos_sensor(0x78,0x02);
	SP0718_write_cmos_sensor(0x81,0x10);
	SP0718_write_cmos_sensor(0x82,0x20);
	SP0718_write_cmos_sensor(0x83,0x30);
	SP0718_write_cmos_sensor(0x84,0x48);
	SP0718_write_cmos_sensor(0x85,0x0c);
	SP0718_write_cmos_sensor(0xfd,0x02);
	SP0718_write_cmos_sensor(0xdc,0x0f);
	           
	SP0718_write_cmos_sensor(0xfd,0x01);
	SP0718_write_cmos_sensor(0x8b,0x00);
	SP0718_write_cmos_sensor(0x8c,0x15);
	SP0718_write_cmos_sensor(0x8d,0x26);
	SP0718_write_cmos_sensor(0x8e,0x34);
	SP0718_write_cmos_sensor(0x8f,0x42);
	SP0718_write_cmos_sensor(0x90,0x57);
	SP0718_write_cmos_sensor(0x91,0x67);
	SP0718_write_cmos_sensor(0x92,0x75);
	SP0718_write_cmos_sensor(0x93,0x81);
	SP0718_write_cmos_sensor(0x94,0x93);
	SP0718_write_cmos_sensor(0x95,0xa1);
	SP0718_write_cmos_sensor(0x96,0xae);
	SP0718_write_cmos_sensor(0x97,0xb9);
	SP0718_write_cmos_sensor(0x98,0xc2);
	SP0718_write_cmos_sensor(0x99,0xcb);
	SP0718_write_cmos_sensor(0x9a,0xd3);
	SP0718_write_cmos_sensor(0x9b,0xdb);
	SP0718_write_cmos_sensor(0x9c,0xe4);
	SP0718_write_cmos_sensor(0x9d,0xea);
	SP0718_write_cmos_sensor(0x9e,0xf2);
	SP0718_write_cmos_sensor(0x9f,0xf9);
	SP0718_write_cmos_sensor(0xa0,0xff);
	           
	SP0718_write_cmos_sensor(0xfd,0x02);
	SP0718_write_cmos_sensor(0x15,0xd5);
	SP0718_write_cmos_sensor(0x16,0x95);
	           
	SP0718_write_cmos_sensor(0xa0,0xa1);
	SP0718_write_cmos_sensor(0xa1,0xe9);
	SP0718_write_cmos_sensor(0xa2,0xf5);
	SP0718_write_cmos_sensor(0xa3,0x06);
	SP0718_write_cmos_sensor(0xa4,0x83);
	SP0718_write_cmos_sensor(0xa5,0xf6);
	SP0718_write_cmos_sensor(0xa6,0x05);
	SP0718_write_cmos_sensor(0xa7,0xf4);
	SP0718_write_cmos_sensor(0xa8,0x87);
	SP0718_write_cmos_sensor(0xa9,0x3c);
	SP0718_write_cmos_sensor(0xaa,0x30);
	SP0718_write_cmos_sensor(0xab,0x0c);
	SP0718_write_cmos_sensor(0xfd,0x00);
	           
	SP0718_write_cmos_sensor(0xfd,0x02);
	SP0718_write_cmos_sensor(0xac,0xa6);
	SP0718_write_cmos_sensor(0xad,0xf4);
	SP0718_write_cmos_sensor(0xae,0xe7);
	SP0718_write_cmos_sensor(0xaf,0xf4);
	SP0718_write_cmos_sensor(0xb0,0x9c);
	SP0718_write_cmos_sensor(0xb1,0xf4);
	SP0718_write_cmos_sensor(0xb2,0xf4);
	SP0718_write_cmos_sensor(0xb3,0xe0);
	SP0718_write_cmos_sensor(0xb4,0xb0);
	SP0718_write_cmos_sensor(0xb5,0x3c);
	SP0718_write_cmos_sensor(0xb6,0x33);
	SP0718_write_cmos_sensor(0xb7,0x0f);
	SP0718_write_cmos_sensor(0xfd,0x00); 
	           
	SP0718_write_cmos_sensor(0xfd,0x01);
	           
	SP0718_write_cmos_sensor(0xd3,0xa4);
	SP0718_write_cmos_sensor(0xd4,0xa4);
	SP0718_write_cmos_sensor(0xd5,0x94);
	SP0718_write_cmos_sensor(0xd6,0x90); 
	           
	SP0718_write_cmos_sensor(0xd7,0xa4);
	SP0718_write_cmos_sensor(0xd8,0xa4);
	SP0718_write_cmos_sensor(0xd9,0x94);
	SP0718_write_cmos_sensor(0xda,0x90);
	           
	SP0718_write_cmos_sensor(0xdd,0x30);
	SP0718_write_cmos_sensor(0xde,0x10);
	SP0718_write_cmos_sensor(0xd2,0x01);
	SP0718_write_cmos_sensor(0xdf,0xff);
	           
	SP0718_write_cmos_sensor(0xfd,0x01);
	SP0718_write_cmos_sensor(0xc2,0xaa);
	SP0718_write_cmos_sensor(0xc3,0xaa);
	SP0718_write_cmos_sensor(0xc4,0x66);
	SP0718_write_cmos_sensor(0xc5,0x66);
	           
	SP0718_write_cmos_sensor(0xfd,0x01);
	SP0718_write_cmos_sensor(0x0f,0xff);

	SP0718_write_cmos_sensor(0x10,0x00);
	SP0718_write_cmos_sensor(0x11,0x00);
	SP0718_write_cmos_sensor(0x12,0x00);
	SP0718_write_cmos_sensor(0x13,0x00); 

	SP0718_write_cmos_sensor(0x14,0x20);
	SP0718_write_cmos_sensor(0x15,0x20);
	SP0718_write_cmos_sensor(0x16,0x18);
	SP0718_write_cmos_sensor(0x17,0x18);

	SP0718_write_cmos_sensor(0xfd,0x01);
	SP0718_write_cmos_sensor(0xcd,0x15);
	SP0718_write_cmos_sensor(0xce,0x1f);
	SP0718_write_cmos_sensor(0xcf,0x20);
	SP0718_write_cmos_sensor(0xd0,0x55);
	           
	SP0718_write_cmos_sensor(0xfd,0x01);
	SP0718_write_cmos_sensor(0xfb,0x33);
	SP0718_write_cmos_sensor(0x32,0x15);
	SP0718_write_cmos_sensor(0x33,0xff);
	SP0718_write_cmos_sensor(0x34,0xe7);
	SP0718_write_cmos_sensor(0x35,0x00);
	#endif
}



UINT32 SP0718_GetSensorID(UINT32 *sensorID)
{
    kal_uint16 sensor_id=0;
    int retry=3;

     SENSORDB("SP0718_GetSensorID \n");	

       // check if sensor ID correct
	do {
		
		SP0718_write_cmos_sensor(0xfd,0x00);
	    sensor_id=SP0718_read_cmos_sensor(0x02);
    	    if (sensor_id == SP0718_SENSOR_ID) {
                 break; 
    	    }
         SENSORDB("Read Sensor ID Fail = 0x%x\n", sensor_id); 
    	    
    	    retry--; 
	}while (retry > 0); 
	
	if (sensor_id != SP0718_SENSOR_ID) {
	    *sensorID = 0xFFFFFFFF;
	    return ERROR_SENSOR_CONNECT_FAIL;
	}

      *sensorID = sensor_id;
       RETAILMSG(1, (TEXT("Sensor Read ID OK \r\n")));
	
    return ERROR_NONE;
}

void SP0718_Write_More_Registers(void)
{
	//Ronlus this function is used for FAE debug
}

UINT32 SP0718_Open(void)
{
    kal_uint16 sensor_id=0;
    int retry = 3; 

     SENSORDB("SP0718_Open \n");

     do {
		
		SP0718_write_cmos_sensor(0xfd,0x00);
	    sensor_id=SP0718_read_cmos_sensor(0x02);
    	    if (sensor_id == SP0718_SENSOR_ID) {
                 break; 
    	    }
         SENSORDB("Read Sensor ID Fail = 0x%x\n", sensor_id); 
    	    
    	    retry--; 
	}while (retry > 0); 
	
	if (sensor_id != SP0718_SENSOR_ID) 
	   {
	      return ERROR_SENSOR_CONNECT_FAIL;
	   }
         SENSORDB("SP0718_ Sensor id read OK, ID = %x\n", sensor_id);

#ifdef DEBUG_SENSOR  //gepeiwei   120903
			//判断手机对应目录下是否有名为sp2528_sd 的文件,没有默认参数
	
			//介于各种原因，本版本初始化参数在_s_fmt中。
			struct file *fp; 
			mm_segment_t fs; 
			loff_t pos = 0; 
			//static char buf[10*1024] ;
	
			printk("SP0718 Open File Start\n");
 fp = filp_open("/mnt/sdcard/sp0718_sd.dat", O_RDONLY , 0); 

			//fp = filp_open("/storage/sdcard1/sp0718_sd.dat", O_RDONLY , 0); 
			if (IS_ERR(fp)) 
			{ 
				printk("open file error 0\n");
 fp = filp_open("/mnt/sdcard/sp0718_sd.dat", O_RDONLY , 0); 

			//	fp = filp_open("/storage/sdcard0/sp0718_sd.dat", O_RDONLY , 0); 
				if (IS_ERR(fp)) { 
					fromsd = 0;   
					printk("open file error 1\n");
				}
				else{
					printk("open file success 1\n");
					fromsd = 1;
					filp_close(fp, NULL); 
				}
			} 
			else 
			{
				printk("open file success 0\n");
				fromsd = 1;
				//SP2528_Initialize_from_T_Flash();
				filp_close(fp, NULL); 
			}
			//set_fs(fs);
			if(fromsd == 1)//是否从SD读取//gepeiwei   120903
				SP2528_Initialize_from_T_Flash();//从SD卡读取的主要函数
			else
				SP0718_Sensor_Init();
#else
    SP0718_Sensor_Init();		
    SENSORDB("SP0718_Open end \n");
#endif
    SP0718_Write_More_Registers();//added for FAE to debut
    return ERROR_NONE;
} /* SP0718_Open */


UINT32 SP0718_Close(void)
{
     SENSORDB("SP0718_Close\n");
    return ERROR_NONE;
} /* SP0718_Close */


UINT32 SP0718_Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
    kal_uint32 iTemp;
    kal_uint16 iStartX = 0, iStartY = 1;

    if(sensor_config_data->SensorOperationMode == MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
    {
        RETAILMSG(1, (TEXT("Camera Video preview\r\n")));
        SP0718_MPEG4_encode_mode = KAL_TRUE;
       
    }
    else
    {
        RETAILMSG(1, (TEXT("Camera preview\r\n")));
        SP0718_MPEG4_encode_mode = KAL_FALSE;
    }

    image_window->GrabStartX= IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY= IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth = IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight =IMAGE_SENSOR_PV_HEIGHT;

    // copy sensor_config_data
    memcpy(&SP0718_SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* SP0718_Preview */


UINT32 SP0718_Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
    SP0718_MODE_CAPTURE=KAL_TRUE;

    image_window->GrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth= IMAGE_SENSOR_FULL_WIDTH;
    image_window->ExposureWindowHeight = IMAGE_SENSOR_FULL_HEIGHT;

    // copy sensor_config_data
    memcpy(&SP0718_SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* SP0718_Capture() */



UINT32 SP0718_GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    pSensorResolution->SensorFullWidth=IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight=IMAGE_SENSOR_FULL_HEIGHT;
    pSensorResolution->SensorPreviewWidth=IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight=IMAGE_SENSOR_PV_HEIGHT;
    return ERROR_NONE;
} /* SP0718_GetResolution() */


UINT32 SP0718_GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
        MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    pSensorInfo->SensorPreviewResolutionX=IMAGE_SENSOR_PV_WIDTH;
    pSensorInfo->SensorPreviewResolutionY=IMAGE_SENSOR_PV_HEIGHT;
    pSensorInfo->SensorFullResolutionX=IMAGE_SENSOR_FULL_WIDTH;
    pSensorInfo->SensorFullResolutionY=IMAGE_SENSOR_FULL_WIDTH;

    pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=10;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=1;
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_HIGH;
    pSensorInfo->SensorInterruptDelayLines = 1;
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxWidth=CAM_SIZE_1M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxWidth=CAM_SIZE_1M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].BinningEnable=FALSE;
    pSensorInfo->CaptureDelayFrame = 1;
    pSensorInfo->PreviewDelayFrame = 0;
    pSensorInfo->VideoDelayFrame = 4;
    pSensorInfo->SensorMasterClockSwitch = 0;
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;

    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
        pSensorInfo->SensorClockFreq=24;
        pSensorInfo->SensorClockDividCount=	3;
        pSensorInfo->SensorClockRisingCount= 0;
        pSensorInfo->SensorClockFallingCount= 2;
        pSensorInfo->SensorPixelClockCount= 3;
        pSensorInfo->SensorDataLatchCount= 2;
        pSensorInfo->SensorGrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
        pSensorInfo->SensorGrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;

        break;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
        pSensorInfo->SensorClockFreq=24;
        pSensorInfo->SensorClockDividCount= 3;
        pSensorInfo->SensorClockRisingCount=0;
        pSensorInfo->SensorClockFallingCount=2;
        pSensorInfo->SensorPixelClockCount=3;
        pSensorInfo->SensorDataLatchCount=2;
        pSensorInfo->SensorGrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
        pSensorInfo->SensorGrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
        break;
    default:
        pSensorInfo->SensorClockFreq=24;
        pSensorInfo->SensorClockDividCount= 3;
        pSensorInfo->SensorClockRisingCount=0;
        pSensorInfo->SensorClockFallingCount=2;
        pSensorInfo->SensorPixelClockCount=3;
        pSensorInfo->SensorDataLatchCount=2;
        pSensorInfo->SensorGrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
        pSensorInfo->SensorGrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
        break;
    }
    SP0718_PixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &SP0718_SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* SP0718_GetInfo() */


UINT32 SP0718_Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
        SP0718_Preview(pImageWindow, pSensorConfigData);
        break;
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
        SP0718_Capture(pImageWindow, pSensorConfigData);
        break;
    }


    return TRUE;
}	/* SP0718_Control() */

BOOL SP0718_set_param_wb(UINT16 para)
{

	switch (para)
	{
		case AWB_MODE_OFF:
			// SP0718_write_cmos_sensor(0xfd,0x00);				   
			// SP0718_write_cmos_sensor(0x32,0x05);	  
		break;
		
		case AWB_MODE_AUTO:
			
			 SP0718_write_cmos_sensor(0xfd,0x02);	
			 SP0718_write_cmos_sensor(0x26,0xb0);	
			 SP0718_write_cmos_sensor(0x27,0xb6);	
			 SP0718_write_cmos_sensor(0xfd,0x01);	
			 SP0718_write_cmos_sensor(0x32,0x15);	
			 SP0718_write_cmos_sensor(0xfd,0x00);	
		break;
		
		case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy
			 SP0718_write_cmos_sensor(0xfd,0x01);	
			 SP0718_write_cmos_sensor(0x32,0x05);	
			 SP0718_write_cmos_sensor(0xfd,0x02);	
			 SP0718_write_cmos_sensor(0x26,0xdc);	
			 SP0718_write_cmos_sensor(0x27,0x75);	
			 SP0718_write_cmos_sensor(0xfd,0x00);	
		break;
		
		case AWB_MODE_DAYLIGHT: //sunny
			 SP0718_write_cmos_sensor(0xfd,0x01);	
			 SP0718_write_cmos_sensor(0x32,0x05);	
			 SP0718_write_cmos_sensor(0xfd,0x02);	
			 SP0718_write_cmos_sensor(0x26,0xc8);	
			 SP0718_write_cmos_sensor(0x27,0x89);	
			 SP0718_write_cmos_sensor(0xfd,0x00);	
		break;
		
		case AWB_MODE_INCANDESCENT: //office
			 SP0718_write_cmos_sensor(0xfd,0x01);	
			 SP0718_write_cmos_sensor(0x32,0x05);	
			 SP0718_write_cmos_sensor(0xfd,0x02);	
			 SP0718_write_cmos_sensor(0x26,0xaa);	
			 SP0718_write_cmos_sensor(0x27,0xce);	
			 SP0718_write_cmos_sensor(0xfd,0x00);	
		break;
		
		case AWB_MODE_TUNGSTEN: //home
			 SP0718_write_cmos_sensor(0xfd,0x01);	
			 SP0718_write_cmos_sensor(0x32,0x05);	
			 SP0718_write_cmos_sensor(0xfd,0x02);	
			 SP0718_write_cmos_sensor(0x26,0x75);	
			 SP0718_write_cmos_sensor(0x27,0xe2);	
			 SP0718_write_cmos_sensor(0xfd,0x00);	
		break;
		
		case AWB_MODE_FLUORESCENT:
			 SP0718_write_cmos_sensor(0xfd,0x01);	
			 SP0718_write_cmos_sensor(0x32,0x05);	
			 SP0718_write_cmos_sensor(0xfd,0x02);	
			 SP0718_write_cmos_sensor(0x26,0x91);	
			 SP0718_write_cmos_sensor(0x27,0xc8);	
			 SP0718_write_cmos_sensor(0xfd,0x00);	
		break;
		
		default:
		return FALSE;
	}

	return TRUE;
} /* SP0718_set_param_wb */


BOOL SP0718_set_param_effect(UINT16 para)
{
	kal_uint32  ret = KAL_TRUE;

	switch (para)
	{
		case MEFFECT_OFF:
			SP0718_write_cmos_sensor(0xfd, 0x01);
			SP0718_write_cmos_sensor(0x66, 0x00);
			SP0718_write_cmos_sensor(0x67, 0x80);
			SP0718_write_cmos_sensor(0x68, 0x80);
			SP0718_write_cmos_sensor(0xfd, 0x00);
		break;
		
		case MEFFECT_SEPIA:
			SP0718_write_cmos_sensor(0xfd, 0x01);
			SP0718_write_cmos_sensor(0x66, 0x10);
			SP0718_write_cmos_sensor(0x67, 0xc0);
			SP0718_write_cmos_sensor(0x68, 0x20);
			SP0718_write_cmos_sensor(0xfd, 0x00);
		break;
		
		case MEFFECT_NEGATIVE:
			SP0718_write_cmos_sensor(0xfd, 0x01);
			SP0718_write_cmos_sensor(0x66, 0x08);
			SP0718_write_cmos_sensor(0x67, 0x80);
			SP0718_write_cmos_sensor(0x68, 0x80);
			SP0718_write_cmos_sensor(0xfd, 0x00);
		break;
		
		case MEFFECT_SEPIAGREEN:
			SP0718_write_cmos_sensor(0xfd, 0x01);
			SP0718_write_cmos_sensor(0x66, 0x10);
			SP0718_write_cmos_sensor(0x67, 0x20);
			SP0718_write_cmos_sensor(0x68, 0x20);
			SP0718_write_cmos_sensor(0xfd, 0x00);
		break;
		
		case MEFFECT_SEPIABLUE:
			SP0718_write_cmos_sensor(0xfd, 0x01);
			SP0718_write_cmos_sensor(0x66, 0x10);
			SP0718_write_cmos_sensor(0x67, 0x20);
			SP0718_write_cmos_sensor(0x68, 0xf0);
			SP0718_write_cmos_sensor(0xfd, 0x00);
		break;

		case MEFFECT_MONO:
			SP0718_write_cmos_sensor(0xfd, 0x01);
			SP0718_write_cmos_sensor(0x66, 0x20);
			SP0718_write_cmos_sensor(0x67, 0x80);
			SP0718_write_cmos_sensor(0x68, 0x80);
			SP0718_write_cmos_sensor(0xfd, 0x00);
		break;
		default:
			ret = FALSE;
	}

	return ret;

} /* SP0718_set_param_effect */


BOOL SP0718_set_param_banding(UINT16 para)
{
	switch (para)
	{
		case AE_FLICKER_MODE_50HZ:
			sp0718_isBanding = 0;
			break;

		case AE_FLICKER_MODE_60HZ:
			sp0718_isBanding = 1;
		break;
		default:
		return FALSE;
	}

	return TRUE;
} /* SP0718_set_param_banding */


BOOL SP0718_set_param_exposure(UINT16 para)
{
	switch (para)
	{
		case AE_EV_COMP_n13:
			SP0718_write_cmos_sensor(0xfd, 0x01);
			SP0718_write_cmos_sensor(0xdb, 0xc0);
		break;
		
		case AE_EV_COMP_n10:
			SP0718_write_cmos_sensor(0xfd, 0x01);
			SP0718_write_cmos_sensor(0xdb, 0xd0);
		break;
		
		case AE_EV_COMP_n07:
			SP0718_write_cmos_sensor(0xfd, 0x01);
			SP0718_write_cmos_sensor(0xdb, 0xe0);
		break;
		
		case AE_EV_COMP_n03:
			SP0718_write_cmos_sensor(0xfd, 0x01);
			SP0718_write_cmos_sensor(0xdb, 0xf0);
		break;				
		
		case AE_EV_COMP_00:
			SP0718_write_cmos_sensor(0xfd, 0x01);
			SP0718_write_cmos_sensor(0xdb, 0x00);
		break;

		case AE_EV_COMP_03:
			SP0718_write_cmos_sensor(0xfd, 0x01);
			SP0718_write_cmos_sensor(0xdb, 0x10);
		break;
		
		case AE_EV_COMP_07:
			SP0718_write_cmos_sensor(0xfd, 0x01);
			SP0718_write_cmos_sensor(0xdb, 0x20);
		break;
		
		case AE_EV_COMP_10:
			SP0718_write_cmos_sensor(0xfd, 0x01);
			SP0718_write_cmos_sensor(0xdb, 0x30);
		break;
		
		case AE_EV_COMP_13:
			SP0718_write_cmos_sensor(0xfd, 0x01);
			SP0718_write_cmos_sensor(0xdb, 0x40);
		break;
		default:
		return FALSE;
	}

	return TRUE;
} /* SP0718_set_param_exposure */


UINT32 SP0718_YUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)
{
#ifdef DEBUG_SENSOR
xxx
	return TRUE;
#endif
    switch (iCmd) {
	case FID_SCENE_MODE:	    
	    if (iPara == SCENE_MODE_OFF)
	    {
			SP0718_NightMode(0); 
	    }
	    else if (iPara == SCENE_MODE_NIGHTSCENE)
	    {
			SP0718_NightMode(1); 
	    }	    
	    break; 	    
    case FID_AWB_MODE:
        SP0718_set_param_wb(iPara);
        break;
    case FID_COLOR_EFFECT:
        SP0718_set_param_effect(iPara);
        break;
    case FID_AE_EV:
        SP0718_set_param_exposure(iPara);
        break;
    case FID_AE_FLICKER:
        SP0718_set_param_banding(iPara);
		break;
	#if 0
	case FID_SCENE_MODE:
        SP0718_set_param_banding(iPara);
		SP0718_NightMode(SP0718_NIGHT_MODE);
        break;
	#endif
    default:
        break;
    }
    return TRUE;
} /* SP0718_YUVSensorSetting */


UINT32 SP0718_FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
        UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    UINT32 SP0718_SensorRegNumber;
    UINT32 i;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

    RETAILMSG(1, (_T("gaiyang SP0718_FeatureControl FeatureId=%d\r\n"), FeatureId));

    switch (FeatureId)
    {
    case SENSOR_FEATURE_GET_RESOLUTION:
        *pFeatureReturnPara16++=IMAGE_SENSOR_FULL_WIDTH;
        *pFeatureReturnPara16=IMAGE_SENSOR_FULL_HEIGHT;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PERIOD:
        *pFeatureReturnPara16++=(VGA_PERIOD_PIXEL_NUMS)+SP0718_dummy_pixels;
        *pFeatureReturnPara16=(VGA_PERIOD_LINE_NUMS)+SP0718_dummy_lines;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        *pFeatureReturnPara32 = SP0718_g_fPV_PCLK;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_ESHUTTER:
        break;
    case SENSOR_FEATURE_SET_NIGHTMODE:
#ifndef DEBUG_SENSOR
        SP0718_NightMode((BOOL) *pFeatureData16);
#endif
        break;
    case SENSOR_FEATURE_SET_GAIN:
    case SENSOR_FEATURE_SET_FLASHLIGHT:
        break;
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
        SP0718_isp_master_clock=*pFeatureData32;
        break;
    case SENSOR_FEATURE_SET_REGISTER:
        SP0718_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
        break;
    case SENSOR_FEATURE_GET_REGISTER:
        pSensorRegData->RegData = SP0718_read_cmos_sensor(pSensorRegData->RegAddr);
        break;
    case SENSOR_FEATURE_GET_CONFIG_PARA:
        memcpy(pSensorConfigData, &SP0718_SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
        *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
        break;
    case SENSOR_FEATURE_SET_CCT_REGISTER:
    case SENSOR_FEATURE_GET_CCT_REGISTER:
    case SENSOR_FEATURE_SET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
    case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
    case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
    case SENSOR_FEATURE_GET_GROUP_COUNT:
    case SENSOR_FEATURE_GET_GROUP_INFO:
    case SENSOR_FEATURE_GET_ITEM_INFO:
    case SENSOR_FEATURE_SET_ITEM_INFO:
    case SENSOR_FEATURE_GET_ENG_INFO:
        break;
    case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
        // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
        // if EEPROM does not exist in camera module.
        *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_YUV_CMD:
        SP0718_YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
        break;
    case SENSOR_FEATURE_CHECK_SENSOR_ID:


	SP0718_GetSensorID(pFeatureData32);
	break;
    default:
        break;
	}
return ERROR_NONE;
}	/* SP0718_FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncSP0718_YUV=
{
	SP0718_Open,
	SP0718_GetInfo,
	SP0718_GetResolution,
	SP0718_FeatureControl,
	SP0718_Control,
	SP0718_Close
};


UINT32 SP0718_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncSP0718_YUV;
	return ERROR_NONE;
} /* SensorInit() */

