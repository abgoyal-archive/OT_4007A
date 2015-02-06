
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

#include "gc0328yuv_Sensor.h"
#include "gc0328yuv_Camera_Sensor_para.h"
#include "gc0328yuv_CameraCustomized.h"

#define GC0328YUV_DEBUG
#ifdef GC0328YUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

kal_uint16 GC0328_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
    char puSendCmd[2] = {(char)(addr & 0xFF) , (char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 2, GC0328_WRITE_ID);

}
kal_uint16 GC0328_read_cmos_sensor(kal_uint8 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd = { (char)(addr & 0xFF) };
	iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte, 1, GC0328_WRITE_ID);
	
    return get_byte;
}

BOOL GC0328_set_param_exposure(UINT16 para);
#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT

kal_bool   GC0328_MPEG4_encode_mode = KAL_FALSE;
kal_uint16 GC0328_dummy_pixels = 0, GC0328_dummy_lines = 0;
kal_bool   GC0328_MODE_CAPTURE = KAL_FALSE;
kal_bool   GC0328_NIGHT_MODE = KAL_FALSE;

UINT16 AE_EV_curr = AE_EV_COMP_00;

kal_uint32 GC0328_isp_master_clock;
static kal_uint32 GC0328_g_fPV_PCLK = 26;

kal_uint8 GC0328_sensor_write_I2C_address = GC0328_WRITE_ID;
kal_uint8 GC0328_sensor_read_I2C_address = GC0328_READ_ID;

UINT8 GC0328PixelClockDivider=0;

MSDK_SENSOR_CONFIG_STRUCT GC0328SensorConfigData;

#define GC0328_SET_PAGE0 	GC0328_write_cmos_sensor(0xfe, 0x00)
#define GC0328_SET_PAGE1 	GC0328_write_cmos_sensor(0xfe, 0x01)


void GC0328_Set_Shutter(kal_uint16 iShutter)
{
} /* Set_GC0328_Shutter */


kal_uint16 GC0328_Read_Shutter(void)
{
    	kal_uint8 temp_reg1, temp_reg2;
	kal_uint16 shutter;

	temp_reg1 = GC0328_read_cmos_sensor(0x04);
	temp_reg2 = GC0328_read_cmos_sensor(0x03);

	shutter = (temp_reg1 & 0xFF) | (temp_reg2 << 8);

	return shutter;
} /* GC0328_read_shutter */


void GC0328_write_reg(kal_uint32 addr, kal_uint32 para)
{
	GC0328_write_cmos_sensor(addr, para);
} /* GC0328_write_reg() */


kal_uint32 GC0328_read_reg(kal_uint32 addr)
{
	return GC0328_read_cmos_sensor(addr);
} /* OV7670_read_reg() */


static void GC0328_awb_enable(kal_bool enalbe)
{	 
	kal_uint16 temp_AWB_reg = 0;

	temp_AWB_reg = GC0328_read_cmos_sensor(0x42);
	
	if (enalbe)
	{
		GC0328_write_cmos_sensor(0x42, (temp_AWB_reg |0x02));
	}
	else
	{
		GC0328_write_cmos_sensor(0x42, (temp_AWB_reg & (~0x02)));
	}

}


void GC0328_config_window(kal_uint16 startx, kal_uint16 starty, kal_uint16 width, kal_uint16 height)
{
} /* GC0328_config_window */


kal_uint16 GC0328_SetGain(kal_uint16 iGain)
{
	return iGain;
}




void GC0328GammaSelect(kal_uint32 GammaLvl)
{
	switch(GammaLvl)
	{
		case GC0328_RGB_Gamma_m1:						//smallest gamma curve
			GC0328_write_cmos_sensor(0xfe, 0x00);
			GC0328_write_cmos_sensor(0xbf, 0x06);
			GC0328_write_cmos_sensor(0xc0, 0x12);
			GC0328_write_cmos_sensor(0xc1, 0x22);
			GC0328_write_cmos_sensor(0xc2, 0x35);
			GC0328_write_cmos_sensor(0xc3, 0x4b);
			GC0328_write_cmos_sensor(0xc4, 0x5f);
			GC0328_write_cmos_sensor(0xc5, 0x72);
			GC0328_write_cmos_sensor(0xc6, 0x8d);
			GC0328_write_cmos_sensor(0xc7, 0xa4);
			GC0328_write_cmos_sensor(0xc8, 0xb8);
			GC0328_write_cmos_sensor(0xc9, 0xc8);
			GC0328_write_cmos_sensor(0xca, 0xd4);
			GC0328_write_cmos_sensor(0xcb, 0xde);
			GC0328_write_cmos_sensor(0xcc, 0xe6);
			GC0328_write_cmos_sensor(0xcd, 0xf1);
			GC0328_write_cmos_sensor(0xce, 0xf8);
			GC0328_write_cmos_sensor(0xcf, 0xfd);
			break;
		case GC0328_RGB_Gamma_m2:
			GC0328_write_cmos_sensor(0xBF, 0x08);
			GC0328_write_cmos_sensor(0xc0, 0x0F);
			GC0328_write_cmos_sensor(0xc1, 0x21);
			GC0328_write_cmos_sensor(0xc2, 0x32);
			GC0328_write_cmos_sensor(0xc3, 0x43);
			GC0328_write_cmos_sensor(0xc4, 0x50);
			GC0328_write_cmos_sensor(0xc5, 0x5E);
			GC0328_write_cmos_sensor(0xc6, 0x78);
			GC0328_write_cmos_sensor(0xc7, 0x90);
			GC0328_write_cmos_sensor(0xc8, 0xA6);
			GC0328_write_cmos_sensor(0xc9, 0xB9);
			GC0328_write_cmos_sensor(0xcA, 0xC9);
			GC0328_write_cmos_sensor(0xcB, 0xD6);
			GC0328_write_cmos_sensor(0xcC, 0xE0);
			GC0328_write_cmos_sensor(0xcD, 0xEE);
			GC0328_write_cmos_sensor(0xcE, 0xF8);
			GC0328_write_cmos_sensor(0xcF, 0xFF);
			break;
			
		case GC0328_RGB_Gamma_m3:			
			GC0328_write_cmos_sensor(0xBF, 0x0B);
			GC0328_write_cmos_sensor(0xc0, 0x16);
			GC0328_write_cmos_sensor(0xc1, 0x29);
			GC0328_write_cmos_sensor(0xc2, 0x3C);
			GC0328_write_cmos_sensor(0xc3, 0x4F);
			GC0328_write_cmos_sensor(0xc4, 0x5F);
			GC0328_write_cmos_sensor(0xc5, 0x6F);
			GC0328_write_cmos_sensor(0xc6, 0x8A);
			GC0328_write_cmos_sensor(0xc7, 0x9F);
			GC0328_write_cmos_sensor(0xc8, 0xB4);
			GC0328_write_cmos_sensor(0xc9, 0xC6);
			GC0328_write_cmos_sensor(0xcA, 0xD3);
			GC0328_write_cmos_sensor(0xcB, 0xDD);
			GC0328_write_cmos_sensor(0xcC, 0xE5);
			GC0328_write_cmos_sensor(0xcD, 0xF1);
			GC0328_write_cmos_sensor(0xcE, 0xFA);
			GC0328_write_cmos_sensor(0xcF, 0xFF);
			break;
			
		case GC0328_RGB_Gamma_m4:
			GC0328_write_cmos_sensor(0xBF, 0x0E);
			GC0328_write_cmos_sensor(0xc0, 0x1C);
			GC0328_write_cmos_sensor(0xc1, 0x34);
			GC0328_write_cmos_sensor(0xc2, 0x48);
			GC0328_write_cmos_sensor(0xc3, 0x5A);
			GC0328_write_cmos_sensor(0xc4, 0x6B);
			GC0328_write_cmos_sensor(0xc5, 0x7B);
			GC0328_write_cmos_sensor(0xc6, 0x95);
			GC0328_write_cmos_sensor(0xc7, 0xAB);
			GC0328_write_cmos_sensor(0xc8, 0xBF);
			GC0328_write_cmos_sensor(0xc9, 0xCE);
			GC0328_write_cmos_sensor(0xcA, 0xD9);
			GC0328_write_cmos_sensor(0xcB, 0xE4);
			GC0328_write_cmos_sensor(0xcC, 0xEC);
			GC0328_write_cmos_sensor(0xcD, 0xF7);
			GC0328_write_cmos_sensor(0xcE, 0xFD);
			GC0328_write_cmos_sensor(0xcF, 0xFF);
			break;
			
		case GC0328_RGB_Gamma_m5:
			GC0328_write_cmos_sensor(0xBF, 0x10);
			GC0328_write_cmos_sensor(0xc0, 0x20);
			GC0328_write_cmos_sensor(0xc1, 0x38);
			GC0328_write_cmos_sensor(0xc2, 0x4E);
			GC0328_write_cmos_sensor(0xc3, 0x63);
			GC0328_write_cmos_sensor(0xc4, 0x76);
			GC0328_write_cmos_sensor(0xc5, 0x87);
			GC0328_write_cmos_sensor(0xc6, 0xA2);
			GC0328_write_cmos_sensor(0xc7, 0xB8);
			GC0328_write_cmos_sensor(0xc8, 0xCA);
			GC0328_write_cmos_sensor(0xc9, 0xD8);
			GC0328_write_cmos_sensor(0xcA, 0xE3);
			GC0328_write_cmos_sensor(0xcB, 0xEB);
			GC0328_write_cmos_sensor(0xcC, 0xF0);
			GC0328_write_cmos_sensor(0xcD, 0xF8);
			GC0328_write_cmos_sensor(0xcE, 0xFD);
			GC0328_write_cmos_sensor(0xcF, 0xFF);
			break;
			
		case GC0328_RGB_Gamma_m6:										// largest gamma curve
			GC0328_write_cmos_sensor(0xBF, 0x14);
			GC0328_write_cmos_sensor(0xc0, 0x28);
			GC0328_write_cmos_sensor(0xc1, 0x44);
			GC0328_write_cmos_sensor(0xc2, 0x5D);
			GC0328_write_cmos_sensor(0xc3, 0x72);
			GC0328_write_cmos_sensor(0xc4, 0x86);
			GC0328_write_cmos_sensor(0xc5, 0x95);
			GC0328_write_cmos_sensor(0xc6, 0xB1);
			GC0328_write_cmos_sensor(0xc7, 0xC6);
			GC0328_write_cmos_sensor(0xc8, 0xD5);
			GC0328_write_cmos_sensor(0xc9, 0xE1);
			GC0328_write_cmos_sensor(0xcA, 0xEA);
			GC0328_write_cmos_sensor(0xcB, 0xF1);
			GC0328_write_cmos_sensor(0xcC, 0xF5);
			GC0328_write_cmos_sensor(0xcD, 0xFB);
			GC0328_write_cmos_sensor(0xcE, 0xFE);
			GC0328_write_cmos_sensor(0xcF, 0xFF);
			break;
		case GC0328_RGB_Gamma_night:									//Gamma for night mode
			GC0328_write_cmos_sensor(0xBF, 0x0B);
			GC0328_write_cmos_sensor(0xc0, 0x16);
			GC0328_write_cmos_sensor(0xc1, 0x29);
			GC0328_write_cmos_sensor(0xc2, 0x3C);
			GC0328_write_cmos_sensor(0xc3, 0x4F);
			GC0328_write_cmos_sensor(0xc4, 0x5F);
			GC0328_write_cmos_sensor(0xc5, 0x6F);
			GC0328_write_cmos_sensor(0xc6, 0x8A);
			GC0328_write_cmos_sensor(0xc7, 0x9F);
			GC0328_write_cmos_sensor(0xc8, 0xB4);
			GC0328_write_cmos_sensor(0xc9, 0xC6);
			GC0328_write_cmos_sensor(0xcA, 0xD3);
			GC0328_write_cmos_sensor(0xcB, 0xDD);
			GC0328_write_cmos_sensor(0xcC, 0xE5);
			GC0328_write_cmos_sensor(0xcD, 0xF1);
			GC0328_write_cmos_sensor(0xcE, 0xFA);
			GC0328_write_cmos_sensor(0xcF, 0xFF);
			break;
		default:
			//GC0328_RGB_Gamma_m1
			GC0328_write_cmos_sensor(0xfe, 0x00);
			GC0328_write_cmos_sensor(0xbf, 0x06);
			GC0328_write_cmos_sensor(0xc0, 0x12);
			GC0328_write_cmos_sensor(0xc1, 0x22);
			GC0328_write_cmos_sensor(0xc2, 0x35);
			GC0328_write_cmos_sensor(0xc3, 0x4b);
			GC0328_write_cmos_sensor(0xc4, 0x5f);
			GC0328_write_cmos_sensor(0xc5, 0x72);
			GC0328_write_cmos_sensor(0xc6, 0x8d);
			GC0328_write_cmos_sensor(0xc7, 0xa4);
			GC0328_write_cmos_sensor(0xc8, 0xb8);
			GC0328_write_cmos_sensor(0xc9, 0xc8);
			GC0328_write_cmos_sensor(0xca, 0xd4);
			GC0328_write_cmos_sensor(0xcb, 0xde);
			GC0328_write_cmos_sensor(0xcc, 0xe6);
			GC0328_write_cmos_sensor(0xcd, 0xf1);
			GC0328_write_cmos_sensor(0xce, 0xf8);
			GC0328_write_cmos_sensor(0xcf, 0xfd);
			break;
	}
}


void GC0328NightMode(kal_bool bEnable)
{
	SENSORDB("%s  enable=%d\n", __func__, bEnable);
	if (bEnable)
	{	
		GC0328_write_cmos_sensor(0xfe, 0x01);
		if(GC0328_MPEG4_encode_mode == KAL_TRUE)
			GC0328_write_cmos_sensor(0x33, 0x00);
		else
			GC0328_write_cmos_sensor(0x33, 0x30);
		GC0328_write_cmos_sensor(0xfe, 0x00);
		GC0328GammaSelect(GC0328_RGB_Gamma_night);
		GC0328_NIGHT_MODE = KAL_TRUE;
	}
	else 
	{
		GC0328_write_cmos_sensor(0xfe, 0x01);
		if(GC0328_MPEG4_encode_mode == KAL_TRUE)
			GC0328_write_cmos_sensor(0x33, 0x00);
		else
			GC0328_write_cmos_sensor(0x33, 0x20);
		GC0328_write_cmos_sensor(0xfe, 0x00);
		GC0328GammaSelect(GC0328_RGB_Gamma_m4);
		GC0328_NIGHT_MODE = KAL_FALSE;
	}

	GC0328_set_param_exposure(AE_EV_curr);
} /* GC0328_NightMode */

void GC0328_Sensor_Init(void)
	{
		GC0328_write_cmos_sensor(0xfe , 0x80);
		GC0328_write_cmos_sensor(0xfe , 0x80);
		GC0328_write_cmos_sensor(0xfc , 0x16);
		GC0328_write_cmos_sensor(0xfc , 0x16);
		GC0328_write_cmos_sensor(0xfc , 0x16);
		GC0328_write_cmos_sensor(0xfc , 0x16);
		
		//GC0328_write_cmos_sensor(0x42 , 0x02);
	  GC0328_write_cmos_sensor(0xfe , 0x00); 
	  GC0328_write_cmos_sensor(0x4f , 0x00);
	  GC0328_write_cmos_sensor(0x42 , 0x00);
	  GC0328_write_cmos_sensor(0x03 , 0x00);
	  GC0328_write_cmos_sensor(0x04 , 0xc0);
	  GC0328_write_cmos_sensor(0x77 , 0x62);
	  GC0328_write_cmos_sensor(0x78 , 0x40);
	  GC0328_write_cmos_sensor(0x79 , 0x4d);

		GC0328_write_cmos_sensor(0xfe , 0x01);
		GC0328_write_cmos_sensor(0x4f , 0x00);
		GC0328_write_cmos_sensor(0x4c , 0x01);
		GC0328_write_cmos_sensor(0xfe , 0x00);
		//////////////////////////////
		//AWB
	////////////////////////////////
	/*
	GC0328_write_cmos_sensor(0xfe , 0x01);  
	GC0328_write_cmos_sensor(0x51 , 0x80);   //20
	GC0328_write_cmos_sensor(0x52 , 0x12);   // 16 //1f
	GC0328_write_cmos_sensor(0x53 , 0x80);   //40
	GC0328_write_cmos_sensor(0x54 , 0x60);   //9f
	GC0328_write_cmos_sensor(0x55 , 0x04);  
	GC0328_write_cmos_sensor(0x56 , 0x0e);  
	GC0328_write_cmos_sensor(0x5b , 0x02);  
	GC0328_write_cmos_sensor(0x61 , 0xdc);  
	GC0328_write_cmos_sensor(0x62 , 0xca);   //R2G STAND
	GC0328_write_cmos_sensor(0x7c , 0x71);   //AWB speed,AWB margin
	GC0328_write_cmos_sensor(0x7d , 0x00);   //AWB every N
	GC0328_write_cmos_sensor(0x76 , 0x8f);   //move mode en,Move mode TH
	GC0328_write_cmos_sensor(0x79 , 0x00);    //use old
  GC0328_write_cmos_sensor(0x80 , 0x70);   																				 //
  GC0328_write_cmos_sensor(0x81 , 0x58); 																				
  GC0328_write_cmos_sensor(0x82 , 0xc0); 																				
  GC0328_write_cmos_sensor(0x70 , 0xf5);    // 
  GC0328_write_cmos_sensor(0x71 , 0x0a);    //
  GC0328_write_cmos_sensor(0x72 , 0x18);    //
  GC0328_write_cmos_sensor(0x73 , 0x30);    //
  GC0328_write_cmos_sensor(0x74 , 0x58);    //
  */
		////AWB////
	/*
	GC0328_write_cmos_sensor(0x50 , 0x00);  
	GC0328_write_cmos_sensor(0xfe , 0x01);  
	GC0328_write_cmos_sensor(0x4f , 0x00);   
	GC0328_write_cmos_sensor(0x4c , 0x01); 
	GC0328_write_cmos_sensor(0x4f , 0x00); 
	GC0328_write_cmos_sensor(0x4f , 0x00); 
	GC0328_write_cmos_sensor(0x4f , 0x00); //wait clr finish 
	GC0328_write_cmos_sensor(0x4d , 0x45);   ////////////40
		GC0328_write_cmos_sensor(0x4e , 0x04);
		GC0328_write_cmos_sensor(0x4e , 0x02);
		GC0328_write_cmos_sensor(0x4e , 0x02);
		GC0328_write_cmos_sensor(0x4d , 0x53);
		GC0328_write_cmos_sensor(0x4e , 0x08);
	GC0328_write_cmos_sensor(0x4e , 0x08);  //08//cwf&skin
	GC0328_write_cmos_sensor(0x4e , 0x04);    //D50
	GC0328_write_cmos_sensor(0x4e , 0x04);    //D50
	GC0328_write_cmos_sensor(0x4d , 0x62);   /////////////60
	GC0328_write_cmos_sensor(0x4e , 0x08);
	GC0328_write_cmos_sensor(0x4e , 0x08);  //08//10 //SPL  TL84&skin
	GC0328_write_cmos_sensor(0x4d , 0x73);   //////////////70
	GC0328_write_cmos_sensor(0x4e , 0x08);  //SPL   A
	GC0328_write_cmos_sensor(0x4d , 0x83);   //////////////80
	GC0328_write_cmos_sensor(0x4e , 0x20);  //SPL   A
	GC0328_write_cmos_sensor(0x4f , 0x01);    

	GC0328_write_cmos_sensor(0x50 , 0x88);  
	GC0328_write_cmos_sensor(0xfe , 0x00);   //page0
	*/
 /* GC0328_write_cmos_sensor(0x4f , 0x00);
  GC0328_write_cmos_sensor(0x4d , 0x34);
  GC0328_write_cmos_sensor(0x4e , 0x04);
  GC0328_write_cmos_sensor(0x4e , 0x00);
  GC0328_write_cmos_sensor(0x4d , 0x43);
  GC0328_write_cmos_sensor(0x4e , 0x04);
  GC0328_write_cmos_sensor(0x4e , 0x04);//D50
  GC0328_write_cmos_sensor(0x4e , 0x02); // d65 add
  GC0328_write_cmos_sensor(0x4e , 0x02); // d65 add
  GC0328_write_cmos_sensor(0x4d , 0x53);
  GC0328_write_cmos_sensor(0x4e , 0x08); //08 
  GC0328_write_cmos_sensor(0x4e , 0x04);
  GC0328_write_cmos_sensor(0x4e , 0x02);  //D65
  GC0328_write_cmos_sensor(0x4e , 0x02);  //D65
  GC0328_write_cmos_sensor(0x4d , 0x63);
  GC0328_write_cmos_sensor(0x4e , 0x10);  //10 //
  GC0328_write_cmos_sensor(0x4d , 0x82);
  GC0328_write_cmos_sensor(0x4e , 0x20);  //A 
  GC0328_write_cmos_sensor(0x4d , 0x92);
  GC0328_write_cmos_sensor(0x4e , 0x20);  //A
  GC0328_write_cmos_sensor(0x4d , 0xa0);
  GC0328_write_cmos_sensor(0x4e , 0x40);   // H
  GC0328_write_cmos_sensor(0x4e , 0x40);  
  GC0328_write_cmos_sensor(0x4e , 0x40);
  GC0328_write_cmos_sensor(0x4f , 0x01);    
  GC0328_write_cmos_sensor(0x50 , 0x84); //80//
  GC0328_write_cmos_sensor(0xfe , 0x00); //page0
  */
      //##############################
      //#AWB
      //################################
  GC0328_write_cmos_sensor(0xfe ,0x01);
  GC0328_write_cmos_sensor(0x51 ,0x80); //20
  GC0328_write_cmos_sensor(0x52 ,0x12); // 16 //1f
  GC0328_write_cmos_sensor(0x53 ,0x80); //40
  GC0328_write_cmos_sensor(0x54 ,0x60); //9f
  GC0328_write_cmos_sensor(0x55 ,0x04); //01
  GC0328_write_cmos_sensor(0x56 ,0x0e); //00
  GC0328_write_cmos_sensor(0x5b ,0x02);  //02
  GC0328_write_cmos_sensor(0x61 ,0xdc); //R2G_stand0[7:0]
  GC0328_write_cmos_sensor(0x62 ,0xca);  //B2G_stand0[7:0]
  
  GC0328_write_cmos_sensor(0x70 ,0xf5);
  GC0328_write_cmos_sensor(0x71 ,0x0a);
  GC0328_write_cmos_sensor(0x72 ,0x18);   //y2c
  GC0328_write_cmos_sensor(0x73 ,0x30); //28 //20  //AWB_C_inter
  GC0328_write_cmos_sensor(0x74 ,0x58); //40 //20  // AWB_C_max
  GC0328_write_cmos_sensor(0x7c ,0x71); //AWB speed,AWB margin
  GC0328_write_cmos_sensor(0x7d ,0x00); //10 //AWB every N
  GC0328_write_cmos_sensor(0x76 ,0x8f); //move mode en,Move mode TH
  GC0328_write_cmos_sensor(0x79 ,0x00); //00
  
  GC0328_write_cmos_sensor(0x80 ,0x70); //70   
  GC0328_write_cmos_sensor(0x81 ,0x58); //58    
  GC0328_write_cmos_sensor(0x82 ,0xc0); //78
  
  
  ////AWB ±ê¶¨////
  
  GC0328_write_cmos_sensor(0x4f ,0x00);
  GC0328_write_cmos_sensor(0x4d ,0x34);
  GC0328_write_cmos_sensor(0x4e ,0x04);
  GC0328_write_cmos_sensor(0x4e ,0x02);
  GC0328_write_cmos_sensor(0x4e ,0x02);
  GC0328_write_cmos_sensor(0x4e ,0x02);
  GC0328_write_cmos_sensor(0x4d ,0x43);
  GC0328_write_cmos_sensor(0x4e ,0x04);
  GC0328_write_cmos_sensor(0x4e ,0x04);//D50
  GC0328_write_cmos_sensor(0x4e ,0x02); // d65 add
  GC0328_write_cmos_sensor(0x4e ,0x02); // d65 add
  GC0328_write_cmos_sensor(0x4d ,0x53);
  GC0328_write_cmos_sensor(0x4e ,0x08); //08 
  GC0328_write_cmos_sensor(0x4e ,0x04);
  GC0328_write_cmos_sensor(0x4e ,0x02);  //D65
  GC0328_write_cmos_sensor(0x4e ,0x02);  //D65
  GC0328_write_cmos_sensor(0x4d ,0x63);
  GC0328_write_cmos_sensor(0x4e ,0x10);  
  GC0328_write_cmos_sensor(0x4d ,0x72);  
  GC0328_write_cmos_sensor(0x4e ,0x20);
  GC0328_write_cmos_sensor(0x4e ,0x20);
  GC0328_write_cmos_sensor(0x4d ,0x82);
  GC0328_write_cmos_sensor(0x4e ,0x20);  //A 
  GC0328_write_cmos_sensor(0x4e ,0x20);  
  GC0328_write_cmos_sensor(0x4d ,0x92);
  GC0328_write_cmos_sensor(0x4e ,0x20);  //A
  GC0328_write_cmos_sensor(0x4e ,0x20);  
  GC0328_write_cmos_sensor(0x4d ,0xa0);
  GC0328_write_cmos_sensor(0x4e ,0x40);   // H
  GC0328_write_cmos_sensor(0x4e ,0x40);  
  GC0328_write_cmos_sensor(0x4e ,0x40);
  GC0328_write_cmos_sensor(0x4f ,0x01); 
  GC0328_write_cmos_sensor(0x50 ,0x84);     
  GC0328_write_cmos_sensor(0xfe ,0x00);  
		
		////////////////////////////////////////////////
	////////////     BLK      //////////////////////
	////////////////////////////////////////////////
	GC0328_write_cmos_sensor(0x27 , 0x00);   //BLK mode
	GC0328_write_cmos_sensor(0x2a , 0x40);
	GC0328_write_cmos_sensor(0x2b , 0x40);
	GC0328_write_cmos_sensor(0x2c , 0x40);
	GC0328_write_cmos_sensor(0x2d , 0x40);
	/*GC0328_write_cmos_sensor(0x27 , 0xf7);
	GC0328_write_cmos_sensor(0x28 , 0x7F);   //BLK LIMIT
	GC0328_write_cmos_sensor(0x29 , 0x20);   //global offset
	GC0328_write_cmos_sensor(0x33 , 0x20);   //1a //1A//offset_ratio_G1
	GC0328_write_cmos_sensor(0x34 , 0x20);   //1a //1A //offset_ratio_R
	GC0328_write_cmos_sensor(0x35 , 0x20);  //1a //1A
	GC0328_write_cmos_sensor(0x36 , 0x20);   // 1a //1A
	GC0328_write_cmos_sensor(0x32 , 0x08);    //darkc ratio

	GC0328_write_cmos_sensor(0x47 , 0x00);  
	GC0328_write_cmos_sensor(0x48 , 0x00);  
*/
		
		
		
		//////////////////////////////////////////////
		////////// page  0	  ////////////////////////
		//////////////////////////////////////////////
		GC0328_write_cmos_sensor(0xfe , 0x00);
		GC0328_write_cmos_sensor(0x05 , 0x00);
		GC0328_write_cmos_sensor(0x06 , 0xde);
		GC0328_write_cmos_sensor(0x07 , 0x00);
		GC0328_write_cmos_sensor(0x08 , 0xa7);
			 
		GC0328_write_cmos_sensor(0x0d , 0x01);
		GC0328_write_cmos_sensor(0x0e , 0xe8);
		GC0328_write_cmos_sensor(0x0f , 0x02);
		GC0328_write_cmos_sensor(0x10 , 0x88);
		GC0328_write_cmos_sensor(0x09 , 0x00);
		GC0328_write_cmos_sensor(0x0a , 0x00);
		GC0328_write_cmos_sensor(0x0b , 0x00);
		GC0328_write_cmos_sensor(0x0c , 0x00);
		GC0328_write_cmos_sensor(0x16 , 0x00);
		GC0328_write_cmos_sensor(0x17 , 0x17);
		GC0328_write_cmos_sensor(0x18 , 0x0e);
		GC0328_write_cmos_sensor(0x19 , 0x06);
							   
		GC0328_write_cmos_sensor(0x1b , 0x48);
		GC0328_write_cmos_sensor(0x1f , 0xC8);
		GC0328_write_cmos_sensor(0x20 , 0x01);
		GC0328_write_cmos_sensor(0x21 , 0x78);
		GC0328_write_cmos_sensor(0x22 , 0xb0);
		GC0328_write_cmos_sensor(0x23 , 0x06);
		GC0328_write_cmos_sensor(0x24 , 0x16);
		GC0328_write_cmos_sensor(0x26 , 0x00);
								 
		GC0328_write_cmos_sensor(0x50 , 0x01);
								
		//global gain for range 
		GC0328_write_cmos_sensor(0x70 , 0x65);
		
		
		
		
		//////////////////////////////////////
		////////////	 block enable	   ///
		//////////////////////////////////////
		GC0328_write_cmos_sensor(0x40 , 0x7f);
		GC0328_write_cmos_sensor(0x41 , 0x24);
		
		GC0328_write_cmos_sensor(0x42 , 0xff);
	
		GC0328_write_cmos_sensor(0x45 , 0x00);
		GC0328_write_cmos_sensor(0x44 , 0xa0);//YUV mode  
		GC0328_write_cmos_sensor(0x46 , 0x02);//[1]Hsync;[0]Vsync;
		
		GC0328_write_cmos_sensor(0x4b , 0x01);
		GC0328_write_cmos_sensor(0x50 , 0x01);
		
		
		
		//DN & EEINTP
		GC0328_write_cmos_sensor(0x7e , 0x0a);
		GC0328_write_cmos_sensor(0x7f , 0x03);
		GC0328_write_cmos_sensor(0x81 , 0x15);
		GC0328_write_cmos_sensor(0x82 , 0x85);
		GC0328_write_cmos_sensor(0x83 , 0x02);
		GC0328_write_cmos_sensor(0x84 , 0xe5);
		GC0328_write_cmos_sensor(0x90 , 0xac);
		GC0328_write_cmos_sensor(0x92 , 0x02);
		GC0328_write_cmos_sensor(0x94 , 0x02);
	  GC0328_write_cmos_sensor(0x95 , 0x54);   //77 //edge effect //43
		
		
		///////YCP
		GC0328_write_cmos_sensor(0xd1 , 0x29);
		GC0328_write_cmos_sensor(0xd2 , 0x29);
		GC0328_write_cmos_sensor(0xd3 , 0x3d);
		GC0328_write_cmos_sensor(0xdd , 0x36);
		GC0328_write_cmos_sensor(0xde , 0x38);
		GC0328_write_cmos_sensor(0xe4 , 0x88);
		GC0328_write_cmos_sensor(0xe5 , 0x40);
		GC0328_write_cmos_sensor(0xd7 , 0x0e);
		
		//////////////////////////////
		//////////////// GAMMA //////
		//////////////////////////////
		//rgb gamma
		GC0328_write_cmos_sensor(0xBF , 0x0E);			
		GC0328_write_cmos_sensor(0xc0 , 0x1C);	
		GC0328_write_cmos_sensor(0xc1 , 0x34);
		GC0328_write_cmos_sensor(0xc2 , 0x48);
		GC0328_write_cmos_sensor(0xc3 , 0x5A);
		GC0328_write_cmos_sensor(0xc4 , 0x6B);
		GC0328_write_cmos_sensor(0xc5 , 0x7B);
		GC0328_write_cmos_sensor(0xc6 , 0x95);
		GC0328_write_cmos_sensor(0xc7 , 0xAB);
		GC0328_write_cmos_sensor(0xc8 , 0xBF);
		GC0328_write_cmos_sensor(0xc9 , 0xCE);
		GC0328_write_cmos_sensor(0xcA , 0xD9);
		GC0328_write_cmos_sensor(0xcB , 0xE4);
		GC0328_write_cmos_sensor(0xcC , 0xEC);
		GC0328_write_cmos_sensor(0xcD , 0xF7);
		GC0328_write_cmos_sensor(0xcE , 0xFD);
		GC0328_write_cmos_sensor(0xcF , 0xFF);
		
		///Y gamma
		GC0328_write_cmos_sensor(0xfe , 0x00);
		GC0328_write_cmos_sensor(0x63 , 0x00);
		GC0328_write_cmos_sensor(0x64 , 0x05);
		GC0328_write_cmos_sensor(0x65 , 0x0b);
		GC0328_write_cmos_sensor(0x66 , 0x19);
		GC0328_write_cmos_sensor(0x67 , 0x2e);
		GC0328_write_cmos_sensor(0x68 , 0x40);
		GC0328_write_cmos_sensor(0x69 , 0x54);
		GC0328_write_cmos_sensor(0x6a , 0x66);
		GC0328_write_cmos_sensor(0x6b , 0x86);
		GC0328_write_cmos_sensor(0x6c , 0xa7);
		GC0328_write_cmos_sensor(0x6d , 0xc6);
		GC0328_write_cmos_sensor(0x6e , 0xe4);
		GC0328_write_cmos_sensor(0x6f , 0xff);
		
		//////ASDE
		GC0328_write_cmos_sensor(0xfe , 0x01);
		GC0328_write_cmos_sensor(0x18 , 0x02);
		
		GC0328_write_cmos_sensor(0xfe , 0x00);
		
		GC0328_write_cmos_sensor(0x98 , 0x00);
		GC0328_write_cmos_sensor(0x9b , 0x20);
		GC0328_write_cmos_sensor(0x9c , 0x80);
		GC0328_write_cmos_sensor(0xa4 , 0x10);
		GC0328_write_cmos_sensor(0xa8 , 0xB0);
		GC0328_write_cmos_sensor(0xaa , 0x40);
		GC0328_write_cmos_sensor(0xa2 , 0x23);
		GC0328_write_cmos_sensor(0xad , 0x01);
		
		///////////////////////////////////////
		////////// AEC	  /////////////////////
		///////////////////////////////////////
		GC0328_write_cmos_sensor(0xfe , 0x01);
		GC0328_write_cmos_sensor(0x9c , 0x02);
		GC0328_write_cmos_sensor(0x08 , 0xa0);
		GC0328_write_cmos_sensor(0x09 , 0xe8);
		
		GC0328_write_cmos_sensor(0x10 , 0x08);
	
		GC0328_write_cmos_sensor(0x11 , 0x91);
		GC0328_write_cmos_sensor(0x12 , 0x10);
		GC0328_write_cmos_sensor(0x13 , 0x42);
		GC0328_write_cmos_sensor(0x15 , 0xfc);
		GC0328_write_cmos_sensor(0x21 , 0xc0);
		GC0328_write_cmos_sensor(0x22 , 0x60);
		GC0328_write_cmos_sensor(0x23 , 0x30);
		GC0328_write_cmos_sensor(0x25 , 0x00);
		GC0328_write_cmos_sensor(0x24 , 0x14);
		
		
		//GC0328_write_cmos_sensor(0x28 , 0x0a);
		
		GC0328_write_cmos_sensor(0x29 , 0x00);
		GC0328_write_cmos_sensor(0x2a , 0x83);
							   
		GC0328_write_cmos_sensor(0x2b , 0x02);
		GC0328_write_cmos_sensor(0x2c , 0x8f);
		GC0328_write_cmos_sensor(0x2d , 0x03);
		GC0328_write_cmos_sensor(0x2e , 0x12);
		GC0328_write_cmos_sensor(0x2f , 0x06);
		GC0328_write_cmos_sensor(0x30 , 0x24);
		GC0328_write_cmos_sensor(0x31 , 0x08);
		GC0328_write_cmos_sensor(0x32 , 0xb3);
		GC0328_write_cmos_sensor(0x33 , 0x20);
		
		
		
		
		
		
		//////////////////////////////////////
		////////////LSC//////////////////////
		//////////////////////////////////////
		/*GC0328_write_cmos_sensor(0xfe , 0x01);
		GC0328_write_cmos_sensor(0xc0 , 0x0f);
		GC0328_write_cmos_sensor(0xc1 , 0x0b);
		GC0328_write_cmos_sensor(0xc2 , 0x0b);
		GC0328_write_cmos_sensor(0xc6 , 0x04);
		GC0328_write_cmos_sensor(0xc7 , 0x00);
		GC0328_write_cmos_sensor(0xc8 , 0x00);
		GC0328_write_cmos_sensor(0xba , 0x1e);
		GC0328_write_cmos_sensor(0xbb , 0x1a);
		GC0328_write_cmos_sensor(0xbc , 0x1b);
		GC0328_write_cmos_sensor(0xb4 , 0x1a);
		GC0328_write_cmos_sensor(0xb5 , 0x12);
		GC0328_write_cmos_sensor(0xb6 , 0x14);
		GC0328_write_cmos_sensor(0xc3 , 0x00);
		GC0328_write_cmos_sensor(0xc4 , 0x00);
		GC0328_write_cmos_sensor(0xc5 , 0x00);
		GC0328_write_cmos_sensor(0xc9 , 0x00);
		GC0328_write_cmos_sensor(0xca , 0x00);
		GC0328_write_cmos_sensor(0xcb , 0x00);
		GC0328_write_cmos_sensor(0xbd , 0x0a);
		GC0328_write_cmos_sensor(0xbe , 0x00);
		GC0328_write_cmos_sensor(0xbf , 0x00);
		GC0328_write_cmos_sensor(0xb7 , 0x04);
		GC0328_write_cmos_sensor(0xb8 , 0x00);
		GC0328_write_cmos_sensor(0xb9 , 0x00);
		GC0328_write_cmos_sensor(0xa8 , 0x13);
		GC0328_write_cmos_sensor(0xa9 , 0x0b);
		GC0328_write_cmos_sensor(0xaa , 0x0b);
		GC0328_write_cmos_sensor(0xab , 0x20);
		GC0328_write_cmos_sensor(0xac , 0x1c);
		GC0328_write_cmos_sensor(0xad , 0x22);
		GC0328_write_cmos_sensor(0xae , 0x0e);
		GC0328_write_cmos_sensor(0xaf , 0x05);
		GC0328_write_cmos_sensor(0xb0 , 0x06);
		GC0328_write_cmos_sensor(0xb1 , 0x0d);
		GC0328_write_cmos_sensor(0xb2 , 0x0a);
		GC0328_write_cmos_sensor(0xb3 , 0x08);
		GC0328_write_cmos_sensor(0xa4 , 0x00);
		GC0328_write_cmos_sensor(0xa5 , 0x00);
		GC0328_write_cmos_sensor(0xa6 , 0x00);
		GC0328_write_cmos_sensor(0xa7 , 0x00);
		GC0328_write_cmos_sensor(0xa1 , 0x3c);
		GC0328_write_cmos_sensor(0xa2 , 0x50);*/
		GC0328_write_cmos_sensor(0xfe , 0x01);
		GC0328_write_cmos_sensor(0xc0 , 0x0f);
		GC0328_write_cmos_sensor(0xc1 , 0x0b);
		GC0328_write_cmos_sensor(0xc2 , 0x0b);
		GC0328_write_cmos_sensor(0xc6 , 0x04);
		GC0328_write_cmos_sensor(0xc7 , 0x00);
		GC0328_write_cmos_sensor(0xc8 , 0x00);
		GC0328_write_cmos_sensor(0xba , 0x1e);
		GC0328_write_cmos_sensor(0xbb , 0x1a);
		GC0328_write_cmos_sensor(0xbc , 0x1b);
		GC0328_write_cmos_sensor(0xb4 , 0x1a);
		GC0328_write_cmos_sensor(0xb5 , 0x12);
		GC0328_write_cmos_sensor(0xb6 , 0x14);
		GC0328_write_cmos_sensor(0xc3 , 0x00);
		GC0328_write_cmos_sensor(0xc4 , 0x00);
		GC0328_write_cmos_sensor(0xc5 , 0x00);
		GC0328_write_cmos_sensor(0xc9 , 0x00);
		GC0328_write_cmos_sensor(0xca , 0x00);
		GC0328_write_cmos_sensor(0xcb , 0x00);
		GC0328_write_cmos_sensor(0xbd , 0x0a);
		GC0328_write_cmos_sensor(0xbe , 0x00);
		GC0328_write_cmos_sensor(0xbf , 0x00);
		GC0328_write_cmos_sensor(0xb7 , 0x04);
		GC0328_write_cmos_sensor(0xb8 , 0x00);
		GC0328_write_cmos_sensor(0xb9 , 0x00);
		GC0328_write_cmos_sensor(0xa8 , 0x13);
		GC0328_write_cmos_sensor(0xa9 , 0x0b);
		GC0328_write_cmos_sensor(0xaa , 0x0b);
		GC0328_write_cmos_sensor(0xab , 0x20);
		GC0328_write_cmos_sensor(0xac , 0x1c);
		GC0328_write_cmos_sensor(0xad , 0x22);
		GC0328_write_cmos_sensor(0xae , 0x0e);
		GC0328_write_cmos_sensor(0xaf , 0x05);
		GC0328_write_cmos_sensor(0xb0 , 0x06);
		GC0328_write_cmos_sensor(0xb1 , 0x0d);
		GC0328_write_cmos_sensor(0xb2 , 0x0a);
		GC0328_write_cmos_sensor(0xb3 , 0x08);
		GC0328_write_cmos_sensor(0xa4 , 0x00);
		GC0328_write_cmos_sensor(0xa5 , 0x00);
		GC0328_write_cmos_sensor(0xa6 , 0x00);
		GC0328_write_cmos_sensor(0xa7 , 0x00);
		GC0328_write_cmos_sensor(0xa1 , 0x3c);
		GC0328_write_cmos_sensor(0xa2 , 0x50);
		GC0328_write_cmos_sensor(0xfe , 0x00);
		
		
		///cct
		GC0328_write_cmos_sensor(0xB1 , 0x09);
		GC0328_write_cmos_sensor(0xB2 , 0x00);
		GC0328_write_cmos_sensor(0xB3 , 0x05);
		GC0328_write_cmos_sensor(0xB4 , 0xf0);
		GC0328_write_cmos_sensor(0xB5 , 0x03);
		GC0328_write_cmos_sensor(0xB6 , 0xf0);
	  mdelay(200);
	  GC0328_write_cmos_sensor(0xfe , 0x00);   
	  GC0328_write_cmos_sensor(0x27 , 0xf7);
	  GC0328_write_cmos_sensor(0x28 , 0x7F);   //BLK LIMIT
	  GC0328_write_cmos_sensor(0x29 , 0x20);   //global offset
	  GC0328_write_cmos_sensor(0x33 , 0x20);   //1a //1A//offset_ratio_G1
	  GC0328_write_cmos_sensor(0x34 , 0x20);   //1a //1A //offset_ratio_R
	  GC0328_write_cmos_sensor(0x35 , 0x20);  //1a //1A
	  GC0328_write_cmos_sensor(0x36 , 0x20);   // 1a //1A
	  GC0328_write_cmos_sensor(0x32 , 0x08);    //darkc ratio
    
	  GC0328_write_cmos_sensor(0x47 , 0x00);  
	  GC0328_write_cmos_sensor(0x48 , 0x00);  
        
	  GC0328_write_cmos_sensor(0xfe , 0x01);
	  GC0328_write_cmos_sensor(0x79 , 0x00);
	  GC0328_write_cmos_sensor(0x7d , 0x00);   //AWB every N
	  GC0328_write_cmos_sensor(0x50 , 0x80);   //AWB every N
	  GC0328_write_cmos_sensor(0x76 , 0x8f);   //move mode en,Move mode TH
	  GC0328_write_cmos_sensor(0xfe , 0x00);
	  GC0328_write_cmos_sensor(0x70 , 0x50);  //global gain tim
	  GC0328_write_cmos_sensor(0x4f , 0x01);  //aec_en	
	  
	  GC0328_write_cmos_sensor(0xeb , 0x80); 
	  GC0328_write_cmos_sensor(0xec , 0x80); 
	  GC0328_write_cmos_sensor(0xed , 0x80); 
		
		
		GC0328_write_cmos_sensor(0xf1 , 0x07);
		
		GC0328_write_cmos_sensor(0xf2 , 0x01);
	}






UINT32 GC0328GetSensorID(UINT32 *sensorID)
{
    kal_uint16 sensor_id=0;
    int i;

    SENSORDB("%s\n", __func__);
    do
    {
        	// check if sensor ID correct
        	for(i = 0; i < 3; i++)
		{
	            	sensor_id = GC0328_read_cmos_sensor(0xf0);
	            	printk("GC0328 Sensor id = %x\n", sensor_id);
	            	if (sensor_id == GC0328_SENSOR_ID)   ///GC0328_SENSOR_ID = 0x9d
			{
	               	break;
	            	}
        	}
        	mdelay(50);
    }while(0);

    if(sensor_id != GC0328_SENSOR_ID)
    {
        *sensorID = 0xFFFFFFFF;
        SENSORDB("GC0328 Sensor id read failed, ID = %x\n", sensor_id);
        return ERROR_SENSOR_CONNECT_FAIL;
    }

    *sensorID = sensor_id;

    RETAILMSG(1, (TEXT("Sensor Read ID OK \r\n")));
	
    return ERROR_NONE;
}




void GC0328_Write_More_Registers(void)
{
	
	///////////////////////////////////////////////
    	//GC0328GammaSelect(GC0328_RGB_Gamma_m4);//0:use default
}


UINT32 GC0328Open(void)
{
    kal_uint16 sensor_id=0;
    int i;
    SENSORDB("%s\n", __func__);
    do
    {
        	// check if sensor ID correct
        	for(i = 0; i < 3; i++)
		{
	            	sensor_id = GC0328_read_cmos_sensor(0xf0);
	            	printk("GC0328 Sensor id = %x\n", sensor_id);
	            	if (sensor_id == GC0328_SENSOR_ID)  //  GC0328_SENSOR_ID= 0x9d
			{
	               	break;
	            	}
        	}
        	mdelay(50);
    }while(0);

    if(sensor_id != GC0328_SENSOR_ID)   // GC0328_SENSOR_ID: 0x9d
    {
        SENSORDB("GC0328 Sensor id read failed, ID = %x\n", sensor_id);
        return ERROR_SENSOR_CONNECT_FAIL;
    }

    RETAILMSG(1, (TEXT("Sensor Read ID OK \r\n")));
    // initail sequence write in
    GC0328_Sensor_Init();
    GC0328_Write_More_Registers();
	
    return ERROR_NONE;
} /* GC0328Open */


UINT32 GC0328Close(void)
{
    return ERROR_NONE;
} /* GC0328Close */


UINT32 GC0328Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
    kal_uint32 iTemp;
    kal_uint16 iStartX = 0, iStartY = 1;

    if(sensor_config_data->SensorOperationMode == MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
    {
        RETAILMSG(1, (TEXT("Camera Video preview\r\n")));
        GC0328_MPEG4_encode_mode = KAL_TRUE;
       
    }
    else
    {
        RETAILMSG(1, (TEXT("Camera preview\r\n")));
        GC0328_MPEG4_encode_mode = KAL_FALSE;
    }

    image_window->GrabStartX= IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY= IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth = IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight =IMAGE_SENSOR_PV_HEIGHT;

    // copy sensor_config_data
    memcpy(&GC0328SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC0328Preview */


UINT32 GC0328Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
    GC0328_MODE_CAPTURE=KAL_TRUE;

    image_window->GrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth= IMAGE_SENSOR_FULL_WIDTH;
    image_window->ExposureWindowHeight = IMAGE_SENSOR_FULL_HEIGHT;

    // copy sensor_config_data
    memcpy(&GC0328SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC0328_Capture() */



UINT32 GC0328GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    pSensorResolution->SensorFullWidth=IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight=IMAGE_SENSOR_FULL_HEIGHT;
    pSensorResolution->SensorPreviewWidth=IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight=IMAGE_SENSOR_PV_HEIGHT;
    return ERROR_NONE;
} /* GC0328GetResolution() */


UINT32 GC0328GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
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
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
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
    pSensorInfo->PreviewDelayFrame = 4; 
    pSensorInfo->VideoDelayFrame = 4;
    pSensorInfo->SensorMasterClockSwitch = 0;
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_4MA;

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
    GC0328PixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &GC0328SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC0328GetInfo() */


UINT32 GC0328Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
       // GC0328Preview(pImageWindow, pSensorConfigData);
      //  break;

    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
        GC0328Capture(pImageWindow, pSensorConfigData);
        break;
    }


    return TRUE;
}	/* GC0328Control() */

BOOL GC0328_set_param_wb(UINT16 para)
{

	switch (para)
	{
		case AWB_MODE_OFF:

		break;
		
		case AWB_MODE_AUTO:
			GC0328_awb_enable(KAL_TRUE);
		break;
		
		case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy
			GC0328_awb_enable(KAL_FALSE);
			GC0328_write_cmos_sensor(0x77, 0x8c); //WB_manual_gain 
			GC0328_write_cmos_sensor(0x78, 0x50);
			GC0328_write_cmos_sensor(0x79, 0x40);
		break;
		
		case AWB_MODE_DAYLIGHT: //sunny
			GC0328_awb_enable(KAL_FALSE);
			GC0328_write_cmos_sensor(0x77, 0x74); 
			GC0328_write_cmos_sensor(0x78, 0x52);
			GC0328_write_cmos_sensor(0x79, 0x40);			
		break;
		
		case AWB_MODE_INCANDESCENT: //office
			GC0328_awb_enable(KAL_FALSE);
			GC0328_write_cmos_sensor(0x77, 0x48);
			GC0328_write_cmos_sensor(0x78, 0x40);
			GC0328_write_cmos_sensor(0x79, 0x5c);
		break;
		
		case AWB_MODE_TUNGSTEN: //home
			GC0328_awb_enable(KAL_FALSE);
			GC0328_write_cmos_sensor(0x77, 0x40);
			GC0328_write_cmos_sensor(0x78, 0x54);
			GC0328_write_cmos_sensor(0x79, 0x70);
		break;
		
		case AWB_MODE_FLUORESCENT:
			GC0328_awb_enable(KAL_FALSE);
			GC0328_write_cmos_sensor(0x77, 0x40);
			GC0328_write_cmos_sensor(0x78, 0x42);
			GC0328_write_cmos_sensor(0x79, 0x50);
		break;
		
		default:
		return FALSE;
	}

	return TRUE;
} /* GC0328_set_param_wb */


BOOL GC0328_set_param_effect(UINT16 para)
{
	kal_uint32  ret = KAL_TRUE;

	switch (para)
	{
		case MEFFECT_OFF:
			GC0328_write_cmos_sensor(0x43 , 0x00);
		break;
		
		case MEFFECT_SEPIA:
			GC0328_write_cmos_sensor(0x43 , 0x02);
			GC0328_write_cmos_sensor(0xda , 0xd0);
			GC0328_write_cmos_sensor(0xdb , 0x28);
		break;
		
		case MEFFECT_NEGATIVE:
			GC0328_write_cmos_sensor(0x43 , 0x01);
		break;
		
		case MEFFECT_SEPIAGREEN:
			GC0328_write_cmos_sensor(0x43 , 0x02);
			GC0328_write_cmos_sensor(0xda , 0xc0);
			GC0328_write_cmos_sensor(0xdb , 0xc0);
		break;
		
		case MEFFECT_SEPIABLUE:
			GC0328_write_cmos_sensor(0x43 , 0x02);
			GC0328_write_cmos_sensor(0xda , 0x50);
			GC0328_write_cmos_sensor(0xdb , 0xe0);
		break;

		case MEFFECT_MONO:
			GC0328_write_cmos_sensor(0x43 , 0x02);
			GC0328_write_cmos_sensor(0xda , 0x00);
			GC0328_write_cmos_sensor(0xdb , 0x00);
		break;
		default:
			ret = FALSE;
	}

	return ret;

} /* GC0328_set_param_effect */


BOOL GC0328_set_param_banding(UINT16 para)
{
	switch (para)
	{
		case AE_FLICKER_MODE_50HZ:
			GC0328_write_cmos_sensor(0x05, 0x02); 	
			GC0328_write_cmos_sensor(0x06, 0x2c); 
			GC0328_write_cmos_sensor(0x07, 0x00);
			GC0328_write_cmos_sensor(0x08, 0xb8);
			
			GC0328_SET_PAGE1;
			GC0328_write_cmos_sensor(0x29, 0x00);   //anti-flicker step [11:8]
			GC0328_write_cmos_sensor(0x2a, 0x60);   //anti-flicker step [7:0]
			
			GC0328_write_cmos_sensor(0x2b, 0x02);   //exp level 0  14.28fps
			GC0328_write_cmos_sensor(0x2c, 0xa0); 
			GC0328_write_cmos_sensor(0x2d, 0x03);   //exp level 1  12.50fps
			GC0328_write_cmos_sensor(0x2e, 0x00); 
			GC0328_write_cmos_sensor(0x2f, 0x03);   //exp level 2  10.00fps
			GC0328_write_cmos_sensor(0x30, 0xc0); 
			GC0328_write_cmos_sensor(0x31, 0x07);   //exp level 3  5fps
			GC0328_write_cmos_sensor(0x32, 0x80); 
			GC0328_SET_PAGE0;
			break;

		case AE_FLICKER_MODE_60HZ:
			GC0328_write_cmos_sensor(0x05, 0x02); 	
			GC0328_write_cmos_sensor(0x06, 0x4c); 
			GC0328_write_cmos_sensor(0x07, 0x00);
			GC0328_write_cmos_sensor(0x08, 0x88);
			
			GC0328_SET_PAGE1;
			GC0328_write_cmos_sensor(0x29, 0x00);   //anti-flicker step [11:8]
			GC0328_write_cmos_sensor(0x2a, 0x4e);   //anti-flicker step [7:0]
			
			GC0328_write_cmos_sensor(0x2b, 0x02);   //exp level 0  15.00fps
			GC0328_write_cmos_sensor(0x2c, 0x70); 
			GC0328_write_cmos_sensor(0x2d, 0x03);   //exp level 0  12.00fps
			GC0328_write_cmos_sensor(0x2e, 0x0c); 
			GC0328_write_cmos_sensor(0x2f, 0x03);   //exp level 0  10.00fps
			GC0328_write_cmos_sensor(0x30, 0xa8); 
			GC0328_write_cmos_sensor(0x31, 0x05);   //exp level 0  7.05fps
			GC0328_write_cmos_sensor(0x32, 0x2e); 
			GC0328_SET_PAGE0;
		break;
		default:
		return FALSE;
	}

	return TRUE;
} /* GC0328_set_param_banding */


BOOL GC0328_set_param_exposure(UINT16 para)
{
	kal_uint8 value_luma, value_Y;
	value_luma = (GC0328_NIGHT_MODE?0x2b:0x00);
	value_Y = (GC0328_NIGHT_MODE?0x70:0x42);

	SENSORDB("%s  para=%d\n", __func__, para);
	AE_EV_curr = para;
	switch (para)
	{
		case AE_EV_COMP_n13:
			GC0328_write_cmos_sensor(0xd5, value_luma - 0x48);
			GC0328_write_cmos_sensor(0xfe, 0x01);
			GC0328_write_cmos_sensor(0x13, value_Y - 0x28);
			GC0328_write_cmos_sensor(0xfe, 0x00);
		break;
		
		case AE_EV_COMP_n10:
			GC0328_write_cmos_sensor(0xd5, value_luma - 0x30);
			GC0328_write_cmos_sensor(0xfe, 0x01);
			GC0328_write_cmos_sensor(0x13, value_Y - 0x18);
			GC0328_write_cmos_sensor(0xfe, 0x00);
		break;
		
		case AE_EV_COMP_n07:
			GC0328_write_cmos_sensor(0xd5, value_luma - 0x20);
			GC0328_write_cmos_sensor(0xfe, 0x01);
			GC0328_write_cmos_sensor(0x13, value_Y - 0x10);
			GC0328_write_cmos_sensor(0xfe, 0x00);
		break;
		
		case AE_EV_COMP_n03:
			GC0328_write_cmos_sensor(0xd5, value_luma - 0x10);
			GC0328_write_cmos_sensor(0xfe, 0x01);
			GC0328_write_cmos_sensor(0x13, value_Y - 0x08);
			GC0328_write_cmos_sensor(0xfe, 0x00);
		break;				
		
		case AE_EV_COMP_00:
			GC0328_write_cmos_sensor(0xd5, value_luma);
			GC0328_write_cmos_sensor(0xfe, 0x01);
			GC0328_write_cmos_sensor(0x13, value_Y);
			GC0328_write_cmos_sensor(0xfe, 0x00);
		break;

		case AE_EV_COMP_03:
			GC0328_write_cmos_sensor(0xd5, value_luma + 0x10);
			GC0328_write_cmos_sensor(0xfe, 0x01);
			GC0328_write_cmos_sensor(0x13, value_Y + 0x10);
			GC0328_write_cmos_sensor(0xfe, 0x00);
		break;
		
		case AE_EV_COMP_07:
			GC0328_write_cmos_sensor(0xd5, value_luma + 0x20);
			GC0328_write_cmos_sensor(0xfe, 0x01);
			GC0328_write_cmos_sensor(0x13, value_Y + 0x20);
			GC0328_write_cmos_sensor(0xfe, 0x00);
		break;
		
		case AE_EV_COMP_10:
			GC0328_write_cmos_sensor(0xd5, value_luma + 0x30);
			GC0328_write_cmos_sensor(0xfe, 0x01);
			GC0328_write_cmos_sensor(0x13, value_Y + 0x30);
			GC0328_write_cmos_sensor(0xfe, 0x00);
		break;
		
		case AE_EV_COMP_13:
			GC0328_write_cmos_sensor(0xd5, value_luma + 0x48);
			GC0328_write_cmos_sensor(0xfe, 0x01);
			GC0328_write_cmos_sensor(0x13, value_Y + 0x48);
			GC0328_write_cmos_sensor(0xfe, 0x00);
		break;
		default:
		return FALSE;
	}

	return TRUE;
} /* GC0328_set_param_exposure */

UINT32 GC0328YUVSetVideoMode(UINT16 u2FrameRate)    
{
  
        GC0328_MPEG4_encode_mode = KAL_TRUE;
     if (u2FrameRate == 30)
   	{
   	
   	    /*********video frame ************/
		
   	}
    else if (u2FrameRate == 15)       
    	{
    	
   	    /*********video frame ************/
		
    	}
    else
   	{
   	
            SENSORDB("Wrong Frame Rate"); 
			
   	}

      return TRUE;

}


UINT32 GC0328YUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)
{
    switch (iCmd) {
    case FID_AWB_MODE:
        GC0328_set_param_wb(iPara);
        break;
    case FID_COLOR_EFFECT:
        GC0328_set_param_effect(iPara);
        break;
    case FID_AE_EV:
        GC0328_set_param_exposure(iPara);
        break;
    case FID_AE_FLICKER:
        GC0328_set_param_banding(iPara);
		break;
	case FID_SCENE_MODE:
		GC0328NightMode(iPara);
        break;
    default:
        break;
    }
    return TRUE;
} /* GC0328YUVSensorSetting */


UINT32 GC0328FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
        UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    UINT32 GC0328SensorRegNumber;
    UINT32 i;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

    RETAILMSG(1, (_T("gaiyang GC0328FeatureControl FeatureId=%d\r\n"), FeatureId));

    switch (FeatureId)
    {
    case SENSOR_FEATURE_GET_RESOLUTION:
        *pFeatureReturnPara16++=IMAGE_SENSOR_FULL_WIDTH;
        *pFeatureReturnPara16=IMAGE_SENSOR_FULL_HEIGHT;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PERIOD:
        *pFeatureReturnPara16++=(VGA_PERIOD_PIXEL_NUMS)+GC0328_dummy_pixels;
        *pFeatureReturnPara16=(VGA_PERIOD_LINE_NUMS)+GC0328_dummy_lines;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        *pFeatureReturnPara32 = GC0328_g_fPV_PCLK;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_ESHUTTER:
        break;
    case SENSOR_FEATURE_SET_NIGHTMODE:
        //GC0328NightMode((BOOL) *pFeatureData16);
        break;
    case SENSOR_FEATURE_SET_GAIN:
    case SENSOR_FEATURE_SET_FLASHLIGHT:
        break;
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
        GC0328_isp_master_clock=*pFeatureData32;
        break;
    case SENSOR_FEATURE_SET_REGISTER:
        GC0328_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
        break;
    case SENSOR_FEATURE_GET_REGISTER:
        pSensorRegData->RegData = GC0328_read_cmos_sensor(pSensorRegData->RegAddr);
        break;
    case SENSOR_FEATURE_GET_CONFIG_PARA:
        memcpy(pSensorConfigData, &GC0328SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
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
        GC0328YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
        break;
    case SENSOR_FEATURE_SET_VIDEO_MODE:    //  lanking
	 GC0328YUVSetVideoMode(*pFeatureData16);
	 break;
    case SENSOR_FEATURE_CHECK_SENSOR_ID:
	GC0328GetSensorID(pFeatureData32);
	break;
    default:
        break;
	}
return ERROR_NONE;
}	/* GC0328FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncGC0328YUV=
{
	GC0328Open,
	GC0328GetInfo,
	GC0328GetResolution,
	GC0328FeatureControl,
	GC0328Control,
	GC0328Close
};


UINT32 GC0328_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncGC0328YUV;
	return ERROR_NONE;
} /* SensorInit() */

