//#include <windows.h>
//#include <memory.h>
//#include <nkintr.h>
//#include <ceddk.h>
//#include <ceddk_exp.h>

//#include "kal_release.h"
//#include "i2c_exp.h"
//#include "gpio_exp.h"
//#include "msdk_exp.h"
//#include "msdk_sensor_exp.h"
//#include "msdk_isp_exp.h"
//#include "base_regs.h"
//#include "Sensor.h"
//#include "camera_sensor_para.h"
//#include "CameraCustomized.h"

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
//#include <mach/mt6516_pll.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "mt9t113yuv_Sensor.h"
#include "mt9t113yuv_Camera_Sensor_para.h"
#include "mt9t113yuv_CameraCustomized.h"

#define MT9T113_DEBUG
#ifdef MT9T113_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

//static int sensor_id_fail = 0; 
static kal_uint32 zoom_factor = 0; 
static MT9T113_SENSOR_INFO_ST MT9T113_sensor;
static MT9T113_OPERATION_STATE_ST MT9T113_op_state;

#if 0
kal_uint16 S5K5CAGX_write_cmos_sensor(kal_uint32 addr, kal_uint32 para) 
	{
	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 4,S5K5CAGX_WRITE_ID);
}
kal_uint16 S5K5CAGX_read_cmos_sensor(kal_uint32 addr)
{
   kal_uint16 get_byte=0;
   char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };

   iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,2,S5K5CAGX_READ_ID);
   return get_byte;
}

#else
inline MT9T113_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	/*Lenovo-sw zhangjiano modified 2011-06-13 begin,to match DVT2 camera configuration*/
	iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,2,MT9T113_WRITE_ID_0);
    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}

inline int MT9T113_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{

	char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};
	/*Lenovo-sw zhangjiano modified 2011-06-13 begin,to match DVT2 camera configuration*/
	iWriteRegI2C(puSendCmd , 4,MT9T113_WRITE_ID_0);
}
#endif



#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT




#define	MT9T113_LIMIT_EXPOSURE_LINES				(1253)
#define	MT9T113_VIDEO_NORMALMODE_30FRAME_RATE       (30)
#define	MT9T113_VIDEO_NORMALMODE_FRAME_RATE         (15)
#define	MT9T113_VIDEO_NIGHTMODE_FRAME_RATE          (7.5)
#define BANDING50_30HZ
/* Global Valuable */



static kal_uint8 MT9T113_exposure_line_h = 0, MT9T113_exposure_line_l = 0,MT9T113_extra_exposure_line_h = 0, MT9T113_extra_exposure_line_l = 0;

static kal_bool MT9T113_gPVmode = KAL_TRUE; //PV size or Full size
static kal_bool MT9T113_VEDIO_encode_mode = KAL_FALSE; //Picture(Jpeg) or Video(Mpeg4)
static kal_bool MT9T113_sensor_cap_state = KAL_FALSE; //Preview or Capture

static kal_uint16 MT9T113_dummy_pixels=0, MT9T113_dummy_lines=0;
kal_uint32 MT9T113_FULL_dummy_pixels = 0;
kal_uint32 MT9T113_FULL_dummy_lines = 0;


static kal_uint16 MT9T113_exposure_lines=0, MT9T113_extra_exposure_lines = 0;


static kal_int8 MT9T113_DELAY_AFTER_PREVIEW = -1;

static kal_uint8 MT9T113_Banding_setting = AE_FLICKER_MODE_50HZ;  //Wonder add

/****** OVT 6-18******/
static kal_uint16 MT9T113_Capture_Max_Gain16= 6*16;
static kal_uint16 MT9T113_Capture_Gain16=0 ;    
static kal_uint16 MT9T113_Capture_Shutter=0;
static kal_uint16 MT9T113_Capture_Extra_Lines=0;

static kal_uint16  MT9T113_PV_Dummy_Pixels =0, MT9T113_Capture_Dummy_Pixels =0, MT9T113_Capture_Dummy_Lines =0;
static kal_uint16  MT9T113_PV_Gain16 = 0;
static kal_uint16  MT9T113_PV_Shutter = 0;
static kal_uint16  MT9T113_PV_Extra_Lines = 0;

kal_uint16 MT9T113_sensor_gain_base=0,MT9T113_FAC_SENSOR_REG=0,MT9T113_iMT9T113_Mode=0,MT9T113_max_exposure_lines=0;
kal_uint32 MT9T113_capture_pclk_in_M=520,MT9T113_preview_pclk_in_M=520,MT9T113_PV_dummy_pixels=0,MT9T113_PV_dummy_lines=0,MT9T113_isp_master_clock=0;
static kal_uint32  MT9T113_preview_pclk = 520, MT9T113_capture_pclk = 520;
kal_bool MT9T113_Night_mode = KAL_FALSE;
kal_bool MT9T113_Y_Target_L = 64; 
kal_bool MT9T113_Y_Target_H = 72; 


static kal_uint32  MT9T113_sensor_pclk=520;
kal_bool MT9T113_VEDIO_MPEG4 = KAL_FALSE; //Picture(Jpeg) or Video(Mpeg4);

kal_bool first_enter_preview = KAL_FALSE;

static kal_bool MT9T113_AWB_ENABLE = KAL_TRUE; 
static kal_bool MT9T113_AE_ENABLE = KAL_TRUE; 


//SENSOR_REG_STRUCT MT9T113SensorCCT[FACTORY_END_ADDR]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
//SENSOR_REG_STRUCT MT9T113SensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
//	camera_para.SENSOR.cct	SensorCCT	=> SensorCCT
//	camera_para.SENSOR.reg	SensorReg
MSDK_SENSOR_CONFIG_STRUCT MT9T113SensorConfigData;
#define SENSOR_CORE_PCLK	83200000	//48M PCLK Output 78000000 

void MT9T113_set_dummy(kal_uint16 pixels, kal_uint16 lines)
	{
		kal_uint32 line_length;
		kal_uint32 frame_rate;
		kal_uint32 base_shutter_50Hz,base_shutter_60Hz;
	
		if(KAL_TRUE == MT9T113_op_state.is_PV_mode)
		{
#if 0
			//for preview
			line_length = MT9T113_PV_PERIOD_PIXEL_NUMS + dummy_pixels;
			
			MT9T113_write_cmos_sensor(0x98E, 0x480A);		//Line Length (A)
			MT9T113_write_cmos_sensor(0x990, line_length);	//		
			
			MT9T113_write_cmos_sensor(0x098E, 0x8400);			// MCU_ADDRESS [SEQ_CMD]
			MT9T113_write_cmos_sensor(0x0990, 0x0006);			// MCU_DATA_0
			
			frame_rate = SENSOR_CORE_PCLK*10/line_length/(MT9T113_PV_PERIOD_LINE_NUMS + dummy_lines);
			
			base_shutter_50Hz = (frame_rate*MT9T113_PV_PERIOD_LINE_NUMS/50+5)/2/10;;
			base_shutter_60Hz = (frame_rate*MT9T113_PV_PERIOD_LINE_NUMS/60+5)/2/10;;
			MT9T113_write_cmos_sensor(0x98E, 0x481A);				//fd_period_50Hz (A)
			MT9T113_write_cmos_sensor(0x990, base_shutter_50Hz);	//		
			MT9T113_write_cmos_sensor(0x98E, 0x481C);				//fd_period_60Hz (A)
			MT9T113_write_cmos_sensor(0x990, base_shutter_60Hz);	//	
#endif
		}
		else
		{ 
			//for capture
			
			line_length = MT9T113_FULL_PERIOD_PIXEL_NUMS + pixels;
			
			MT9T113_write_cmos_sensor(0x98E, 0x4837);		//Line Length (B)
			MT9T113_write_cmos_sensor(0x990, line_length);	//		
			
			MT9T113_write_cmos_sensor(0x098E, 0x8400);			// MCU_ADDRESS [SEQ_CMD]
			MT9T113_write_cmos_sensor(0x0990, 0x0006);			// MCU_DATA_0
	
			{
				kal_uint16 temp=0;
				while(temp <60)
				{
					MT9T113_write_cmos_sensor(0x098E, 0x8400);	  // MCU_DATA_0
					if(0==MT9T113_read_cmos_sensor(0x990))
					{
					 break;
					}
					mdelay(4);//DELAY=100
					temp+=1;
				}
			}
			
			frame_rate = SENSOR_CORE_PCLK*10/line_length/(MT9T113_FULL_PERIOD_LINE_NUMS + lines);
			
			base_shutter_50Hz = (frame_rate*MT9T113_FULL_PERIOD_LINE_NUMS/50+5)/2/10;;
			base_shutter_60Hz = (frame_rate*MT9T113_FULL_PERIOD_LINE_NUMS/60+5)/2/10;;
			MT9T113_write_cmos_sensor(0x98E, 0x4847);				//fd_period_50Hz (B)
			MT9T113_write_cmos_sensor(0x990, base_shutter_50Hz);	//		
			MT9T113_write_cmos_sensor(0x98E, 0x4849);				//fd_period_60Hz (B)
			MT9T113_write_cmos_sensor(0x990, base_shutter_60Hz);	//	
		}
		return;
	
	}

 

kal_uint16 MT9T113_read_MT9T113_gain(void)
{
    kal_uint8  temp_reg;
    kal_uint16 sensor_gain;

    temp_reg=MT9T113_read_cmos_sensor(0x3000);  


    sensor_gain=(16+(temp_reg&0x0F));
    if(temp_reg&0x10)
        sensor_gain<<=1;
    if(temp_reg&0x20)
        sensor_gain<<=1;
      
    if(temp_reg&0x40)
        sensor_gain<<=1;
      
    if(temp_reg&0x80)
        sensor_gain<<=1;
      
    return sensor_gain;
}  /* MT9T113_read_MT9T113_gain */


kal_uint32 MT9T113_read_shutter(void)
{
	kal_uint16 temp_reg1;
return 0;	
	temp_reg1 = MT9T113_read_cmos_sensor(0x3012);    // AEC[b15~b8]
	/* Backup the preview mode last shutter & sensor gain. */
	MT9T113_sensor.pv_shutter = temp_reg1;
	
return MT9T113_sensor.pv_shutter;
}    /* MT9T113_read_shutter */

void MT9T113_write_MT9T113_gain(kal_uint16 gain)
{    
    kal_uint16 temp_reg;
   
	RETAILMSG(1, (TEXT("MT9T113 write gain: %d\r\n"), gain));
   
   if(gain > 248)  return ;//ASSERT(0);
   
    temp_reg = 0;
    if (gain > 31)
    {
        temp_reg |= 0x10;
        gain = gain >> 1;
    }
    if (gain > 31)
    {
        temp_reg |= 0x20;
        gain = gain >> 1;
    }

    if (gain > 31)
    {
        temp_reg |= 0x40;
        gain = gain >> 1;
    }
    if (gain > 31)
    {
        temp_reg |= 0x80;
        gain = gain >> 1;
    }
    
    if (gain > 16)
    {
        temp_reg |= ((gain -16) & 0x0f);
    }   
  
   MT9T113_write_cmos_sensor(0x3000,temp_reg);
}  /* MT9T113_write_MT9T113_gain */

static void MT9T113_write_shutter(kal_uint16 shutter)
{    
}    /* MT9T113_write_shutter */


void MT9T113_night_mode(kal_bool enable)
{
	//kal_uint16 night = 0;
	//kal_uint16 temp=MT9T113_read_cmos_sensor(0x3302);
	MT9T113_Night_mode = enable;
		printk("[MT9T113YUV] enable=%d \n",enable);
	if (MT9T113_sensor_cap_state == KAL_TRUE) {
		return ;	//If capture mode, return directely.
	}
   if(MT9T113_VEDIO_encode_mode==KAL_FALSE)
   	{
		if (enable) 
		{	
		#ifdef MT9T113_DEBUG
			printk("[MT9T113YUV] nightmode \n");
		#endif
		    /*MT9T113_write_cmos_sensor(0x98E, 0x480A);		//Line Length (A)
		    MT9T113_write_cmos_sensor(0x990, MT9T113_PV_PERIOD_PIXEL_NUMS);	//*/
			MT9T113_write_cmos_sensor(0x098E, 0x682F);	// MCU_ADDRESS
			MT9T113_write_cmos_sensor(0x0990, 0x0110);	// MCU_DATA_0// gain	
			MT9T113_write_cmos_sensor(0x098E, 0x6820);	// ae_target_zone
			MT9T113_write_cmos_sensor(0x0990, 0x0014);	// 	
		
		    MT9T113_write_cmos_sensor(0x098E, 0x6815);	  // MCU_ADDRESS [PRI_A_CONFIG_FD_MAX_FDZONE_50HZ]
			MT9T113_write_cmos_sensor(0x0990, 0x0014);	  // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6817);	  // MCU_ADDRESS [PRI_A_CONFIG_FD_MAX_FDZONE_60HZ]
			MT9T113_write_cmos_sensor(0x0990, 0x0018);	  // MCU_DATA_0																   	    
		}
		else 
		{
	     //[Normal mode 10-29.1fps] 
		#ifdef MT9T113_DEBUG
	        printk("[MT9T113YUV] automode \n");
		#endif
		    /*MT9T113_write_cmos_sensor(0x98E, 0x480A);		//Line Length (A)
		    MT9T113_write_cmos_sensor(0x990, MT9T113_PV_PERIOD_PIXEL_NUMS);	//*/
			MT9T113_write_cmos_sensor(0x098E, 0x682F);	// MCU_ADDRESS
			MT9T113_write_cmos_sensor(0x0990, 0x00F0);	// MCU_DATA_0// gain	
			MT9T113_write_cmos_sensor(0x098E, 0x6820);	// ae_target_zone
			MT9T113_write_cmos_sensor(0x0990, 0x000C);	// 	
			MT9T113_write_cmos_sensor(0x098E, 0x6815);	  // MCU_ADDRESS [PRI_A_CONFIG_FD_MAX_FDZONE_50HZ]
			MT9T113_write_cmos_sensor(0x0990, 0x000C);	  // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6817);	  // MCU_ADDRESS [PRI_A_CONFIG_FD_MAX_FDZONE_60HZ]
			MT9T113_write_cmos_sensor(0x0990, 0x0010);	  // MCU_DATA_0
		}
   	}
   else
   	{
		if(enable)
		{

		}
		else
		{

		}
   	}
	
}	/* MT9T113_NightMode */


/* Register setting from capture to preview. */
static void MT9T113_set_VGA_mode(void)
{
	//-----------------------------------
	//From capture to preview
	//-----------------------------------

	//-------------------------------------------------------------------------------
    // PLL MY_OUTPUT clock(fclk)
    // fclk = (0x40 - 0x300E[5:0]) x N x Bit8Div x MCLK / M, where
    //      N = 1, 1.5, 2, 3 for 0x300F[7:6] = 0~3, respectively
    //      M = 1, 1.5, 2, 3 for 0x300F[1:0] = 0~3, respectively
    //      Bit8Div = 1, 1, 4, 5 for 0x300F[5:4] = 0~3, respectively
    // Sys Clk = fclk / Bit8Div / SenDiv
    // Sensor MY_OUTPUT clock(DVP PCLK)
    // DVP PCLK = ISP CLK / DVPDiv, where
    //      ISP CLK =  fclk / Bit8Div / SenDiv / CLKDiv / 2, where
    //          Bit8Div = 1, 1, 4, 5 for 0x300F[5:4] = 0~3, respectively
    //          SenDiv = 1, 2 for 0x3010[4] = 0 or 1 repectively
    //          CLKDiv = (0x3011[5:0] + 1)
    //      DVPDiv = 0x304C[3:0] * (2 ^ 0x304C[4]), if 0x304C[3:0] = 0, use 16 instead
    //
    // Base shutter calculation
    //      60Hz: (1/120) * ISP Clk / QXGA_MODE_WITHOUT_DUMMY_PIXELS
    //      50Hz: (1/100) * ISP Clk / QXGA_MODE_WITHOUT_DUMMY_PIXELS
    //-------------------------------------------------------------------------------

	    //26MHz Mclk,22.5Mhz Pclk 30fps	   

	    //gg@MT9T113_write_cmos_sensor(0x300e,0x33); 
	    //gg@MT9T113_write_cmos_sensor(0x300f,0x21); 
	    //gg@MT9T113_write_cmos_sensor(0x3010,0x20); 
	    //gg@MT9T113_write_cmos_sensor(0x3011,0x00); 
	    //gg@MT9T113_write_cmos_sensor(0x304c,0x85);      //reserve

	    //size:640x480	

#if 0
	    MT9T113_write_cmos_sensor(0x3012,0x10);					//soft reset,  sensor array resolution:XGA(1024X768) 	
	    
		//MT9T113_write_cmos_sensor(0x3023,0x07);					// ;05 VS[7:0]:Vertical start point of array																										
              MT9T113_write_cmos_sensor(0x3023,0x06);					// ;05 VS[7:0]:Vertical start point of array																												
		MT9T113_write_cmos_sensor(0x3026,0x03);					//VH[15:8]:Vertical height,  																									
		MT9T113_write_cmos_sensor(0x3027,0x04);					//VH[7:0]	VH=772																										
		MT9T113_write_cmos_sensor(0x302a,0x03);					//VTS[15:8]:Vertical total size for 1 frame 																										
		MT9T113_write_cmos_sensor(0x302b,0x10);					//VTS[7:0]	VTS=784																									
		MT9T113_write_cmos_sensor(0x3075,0x24);				  //Vsync start point and width 																										
		MT9T113_write_cmos_sensor(0x300d,0x01);				  // pclk always output,																											
		//MT9T113_write_cmos_sensor(0x30d7,0x90);				  //reserve 																									
		MT9T113_write_cmos_sensor(0x30d7,0x80);				  //reserve 																									
		MT9T113_write_cmos_sensor(0x3069,0x04);				  //;44 ;BLC																									
		MT9T113_write_cmos_sensor(0x303e,0x00);				  //AVH[11:8]:Average window vertical height																											
		MT9T113_write_cmos_sensor(0x303f,0xc0);				  //AVH[7:0]	,AVH=192(192*4=768)
		//need check if need write this register 0x304c
		MT9T113_write_cmos_sensor(0x304c,0x85);				  //reserve 			
		
		MT9T113_write_cmos_sensor(0x3302,0xef);				  //  sde, uv_adj, gam, awb,scale_en																				
		MT9T113_write_cmos_sensor(0x335f,0x34);				  // SIZE_IN_MISC:Vsize_in[10:8],Hsize_in[11:8] 																										
		MT9T113_write_cmos_sensor(0x3360,0x0c);				  // Hsize_in[7:0]	,Hsize_in=1036																									
		MT9T113_write_cmos_sensor(0x3361,0x04);				  // Vsize_in[7:0]	,Vsize_in=772																									
		MT9T113_write_cmos_sensor(0x3362,0x12);				  // SIZE_OUT_MISC:Zoom_out output size:Vsize_out[10:8],Hsize_out[11:8] 																				
		MT9T113_write_cmos_sensor(0x3363,0x88);				  // Hsize_out[7:0] for zoom_out	Hsize_out=648																										
		MT9T113_write_cmos_sensor(0x3364,0xE4);				  // Vsize_out[7:0] for zoom_out	Vsize_out=484																									
		MT9T113_write_cmos_sensor(0x3403,0x42);				  //bit[7:4]:x start=4,	 bit[3:0]:y start	=2																										
		MT9T113_write_cmos_sensor(0x3088,0x12);				  //x_output_size, isp_xout[15:8]:																							
		MT9T113_write_cmos_sensor(0x3089,0x80);				  // isp_xout[7:0]	:640																									
		MT9T113_write_cmos_sensor(0x308a,0x01);				  // y_output_size,isp_yout[15:8]																						
		MT9T113_write_cmos_sensor(0x308b,0xe0);				  // isp_yout[7:0]	:480	
		MT9T113_write_cmos_sensor(0x3366,0x15);		//reserve
#else
              //gg@MT9T113_write_cmos_sensor(0x3012, 0x10);
              //gg@MT9T113_write_cmos_sensor(0x3023, 0x06);
              //gg@MT9T113_write_cmos_sensor(0x3026, 0x03);
              //gg@MT9T113_write_cmos_sensor(0x3027, 0x04);
              //gg@MT9T113_write_cmos_sensor(0x302a, 0x03);
              //gg@MT9T113_write_cmos_sensor(0x302b, 0x10);
              //gg@MT9T113_write_cmos_sensor(0x3075, 0x24);
              //gg@MT9T113_write_cmos_sensor(0x300d, 0x01);
              //gg@MT9T113_write_cmos_sensor(0x30d7, 0x90);
              //gg@MT9T113_write_cmos_sensor(0x3069, 0x04);
              //gg@MT9T113_write_cmos_sensor(0x303e, 0x00);
              //gg@MT9T113_write_cmos_sensor(0x303f, 0xc0);
              //gg@MT9T113_write_cmos_sensor(0x3302, 0xef);
              //gg@MT9T113_write_cmos_sensor(0x335f, 0x34);
              //gg@MT9T113_write_cmos_sensor(0x3360, 0x0c);
              //gg@MT9T113_write_cmos_sensor(0x3361, 0x04);
              //gg@MT9T113_write_cmos_sensor(0x3362, 0x12);
              //gg@MT9T113_write_cmos_sensor(0x3363, 0x88);
              //gg@MT9T113_write_cmos_sensor(0x3364, 0xe4);
              //gg@MT9T113_write_cmos_sensor(0x3403, 0x42);
              //gg@MT9T113_write_cmos_sensor(0x3088, 0x12);
              //gg@MT9T113_write_cmos_sensor(0x3089, 0x80);
              //gg@MT9T113_write_cmos_sensor(0x308a, 0x01);
              //gg@MT9T113_write_cmos_sensor(0x308b, 0xe0);
		
#endif 

    //    MT9T113_write_cmos_sensor(0x308d,0x04);				  //reset block sleep enable																											
	//    MT9T113_write_cmos_sensor(0x3086,0x03);				  //sleep on																											
	 //   MT9T113_write_cmos_sensor(0x3086,0x00);				  // sleep off	
}
void test_pclk_52M(void)
{
	kal_uint16 tmp =0;
	//MT9T113_write_cmos_sensor(0x0014, 0x0449);	  //PLL Control: BYPASS PLL = 9541
	MT9T113_write_cmos_sensor(0x0010, 0x0110 ); // PLL_DIVIDERS//MCLK = 26MHz ±£¨PCLK = 52MHz

	MT9T113_write_cmos_sensor(0x0012, 0x0070 ); // PLL_P_DIVIDERS
	//MT9T113_write_cmos_sensor(0x0014, 0x904A);	  //PLL Control: TEST_BYPASS on = 9541

	MT9T113_write_cmos_sensor(0x002A, 0x7474 ); // PLL_P4_P5_P6_DIVIDERS
	MT9T113_write_cmos_sensor(0x001E, 0x0344 ); // PAD_SLEW_PAD_CONFIG
	MT9T113_write_cmos_sensor(0x3B84, 0x019C ); //I2C Master Clock Divider = 412
	MT9T113_write_cmos_sensor(0x0018, 0x402D ); // STANDBY_CONTROL_AND_STATUS
	MT9T113_write_cmos_sensor(0x0018, 0x402F ); // STANDBY_CONTROL_AND_STATUS
	MT9T113_write_cmos_sensor(0x0018, 0x402E ); // STANDBY_CONTROL_AND_STATUS
    MT9T113_write_cmos_sensor(0x0018, 0x002A ); // STANDBY_CONTROL_AND_STATUS
    // wait for FW initialization complete

    //POLL_FIELD=STANDBY_CONTROL_AND_STATUS,STANDBY_DONE,==1,DELAY=10,TIMEOUT=100 
    mdelay(10);//DELAY=30
    //REGWIZARD OUTPUT
    MT9T113_write_cmos_sensor(0x98E, 0x4800);	//Row Start (A)
    MT9T113_write_cmos_sensor(0x990, 0x0010);	//		= 16
    MT9T113_write_cmos_sensor(0x98E, 0x4802);	//Column Start (A)
    MT9T113_write_cmos_sensor(0x990, 0x0010);	//		= 16
    MT9T113_write_cmos_sensor(0x98E, 0x4804);	//Row End (A)
    MT9T113_write_cmos_sensor(0x990, 0x062D);	//		= 1581
	MT9T113_write_cmos_sensor(0x98E, 0x4806);	//Column End (A)
	MT9T113_write_cmos_sensor(0x990, 0x082D);	//		= 2093
	MT9T113_write_cmos_sensor(0x98E, 0x4808);	//Base Frame Lines (A)
	MT9T113_write_cmos_sensor(0x990, 0x0359);	//		= 857
	MT9T113_write_cmos_sensor(0x98E, 0x480A);	//Line Length (A)
	MT9T113_write_cmos_sensor(0x990, 0x0D08);	//		= 3336
	MT9T113_write_cmos_sensor(0x98E, 0x480C);	//Fine Correction (A)
	MT9T113_write_cmos_sensor(0x990, 0x0399);	//		= 921
	MT9T113_write_cmos_sensor(0x98E, 0x480E);	//Row Speed (A)
	MT9T113_write_cmos_sensor(0x990, 0x0111);	//		= 273
	MT9T113_write_cmos_sensor(0x98E, 0x4810);	//Read Mode (A)
	MT9T113_write_cmos_sensor(0x990, 0x046F);	//		= 1132
	MT9T113_write_cmos_sensor(0x98E, 0x4812);	//Fine IT Min (A)
	MT9T113_write_cmos_sensor(0x990, 0x0510);	//		= 1296
	MT9T113_write_cmos_sensor(0x98E, 0x4814);	//Fine IT Max Margin (A)
	MT9T113_write_cmos_sensor(0x990, 0x01BA);	//		= 442
	MT9T113_write_cmos_sensor(0x98E, 0x482D);	//Row Start (B)
	MT9T113_write_cmos_sensor(0x990, 0x0018);	//		= 24
	MT9T113_write_cmos_sensor(0x98E, 0x482F);	//Column Start (B)
	MT9T113_write_cmos_sensor(0x990, 0x0018);	//		= 24
	MT9T113_write_cmos_sensor(0x98E, 0x4831);	//Row End (B)
	MT9T113_write_cmos_sensor(0x990, 0x0627);	//		= 1575
	MT9T113_write_cmos_sensor(0x98E, 0x4833);	//Column End (B)
	MT9T113_write_cmos_sensor(0x990, 0x0827);	//		= 2087
	MT9T113_write_cmos_sensor(0x98E, 0x4835);	//Base Frame Lines (B)
	MT9T113_write_cmos_sensor(0x990, 0x065D);	//		= 1629
	MT9T113_write_cmos_sensor(0x98E, 0x4837);	//Line Length (B)
	MT9T113_write_cmos_sensor(0x990, 0x1A99);	//		= 6809
	MT9T113_write_cmos_sensor(0x98E, 0x4839);	//Fine Correction (B)
	MT9T113_write_cmos_sensor(0x990, 0x019F);	//		= 415
	MT9T113_write_cmos_sensor(0x98E, 0x483B);	//Row Speed (B)
	MT9T113_write_cmos_sensor(0x990, 0x0111);	//		= 273
	MT9T113_write_cmos_sensor(0x98E, 0x483D);	//Read Mode (B)
	MT9T113_write_cmos_sensor(0x990, 0x0027);	//		= 36
	MT9T113_write_cmos_sensor(0x98E, 0x483F);	//Fine IT Min (B)
	MT9T113_write_cmos_sensor(0x990, 0x0266);	//		= 614
	MT9T113_write_cmos_sensor(0x98E, 0x4841);	//Fine IT Max Margin (B)
	MT9T113_write_cmos_sensor(0x990, 0x010A);	//		= 266
	MT9T113_write_cmos_sensor(0x98E, 0xB81A);	//fd_zone_height

	MT9T113_write_cmos_sensor(0x990, 0x06); //		= 6
	MT9T113_write_cmos_sensor(0x98E, 0x481A);	//fd_period_50Hz (A)
	MT9T113_write_cmos_sensor(0x990, 0x00F9);	//		= 249
	MT9T113_write_cmos_sensor(0x98E, 0x481C);	//fd_period_60Hz (A)
	MT9T113_write_cmos_sensor(0x990, 0x00D0);	//		= 208
	MT9T113_write_cmos_sensor(0x98E, 0xC81E);	//fd_search_f1_50hz (A)
	MT9T113_write_cmos_sensor(0x990, 0x21); //		= 33
	MT9T113_write_cmos_sensor(0x98E, 0xC81F);	//fd_search_f2_50hz (A)
	MT9T113_write_cmos_sensor(0x990, 0x23); //		= 35
	MT9T113_write_cmos_sensor(0x98E, 0xC820);	//fd_search_f1_60hz (A)
	MT9T113_write_cmos_sensor(0x990, 0x28); //		= 40
	MT9T113_write_cmos_sensor(0x98E, 0xC821);	//fd_search_f2_60hz (A)
	MT9T113_write_cmos_sensor(0x990, 0x2A); //		= 42
	MT9T113_write_cmos_sensor(0x98E, 0x4847);	//fd_period_50Hz (B)
	MT9T113_write_cmos_sensor(0x990, 0x007A);	//		= 122
	MT9T113_write_cmos_sensor(0x98E, 0x4849);	//fd_period_60Hz (B)
	MT9T113_write_cmos_sensor(0x990, 0x0069);	//		= 102
	MT9T113_write_cmos_sensor(0x98E, 0xC84B);	//fd_search_f1_50hz (B)
	MT9T113_write_cmos_sensor(0x990, 0x10); //		= 16
	MT9T113_write_cmos_sensor(0x98E, 0xC84C);	//fd_search_f2_50hz (B)
	MT9T113_write_cmos_sensor(0x990, 0x12); //		= 18
	MT9T113_write_cmos_sensor(0x98E, 0xC84D);	//fd_search_f1_60hz (B)
	MT9T113_write_cmos_sensor(0x990, 0x13); //		= 19
	MT9T113_write_cmos_sensor(0x98E, 0xC84E);	//fd_search_f2_60hz (B)
	MT9T113_write_cmos_sensor(0x990, 0x15); //		= 21
	MT9T113_write_cmos_sensor(0x98E, 0x6800);	//Output Width (A)
	MT9T113_write_cmos_sensor(0x990, 0x0400);	//		= 640
	MT9T113_write_cmos_sensor(0x98E, 0x6802);	//Output Height (A)
	MT9T113_write_cmos_sensor(0x990, 0x0300);	//		= 480
	MT9T113_write_cmos_sensor(0x98E, 0x6804);	//FOV Width (A)
	MT9T113_write_cmos_sensor(0x990, 0x0400);	//		= 1024
	MT9T113_write_cmos_sensor(0x98E, 0x6806);	//FOV Height (A)
	MT9T113_write_cmos_sensor(0x990, 0x0300);	//		= 768
	MT9T113_write_cmos_sensor(0x98E, 0xE892);	//JPEG Mode (A)
	MT9T113_write_cmos_sensor(0x990, 0x00); //		= 0
	MT9T113_write_cmos_sensor(0x98E, 0x6C00);	//Output Width (B)
	MT9T113_write_cmos_sensor(0x990, 0x0800);	//		= 2048
	MT9T113_write_cmos_sensor(0x98E, 0x6C02);	//Output Height (B)
	MT9T113_write_cmos_sensor(0x990, 0x0600);	//		= 1536
	MT9T113_write_cmos_sensor(0x98E, 0x6C04);	//FOV Width (B)
	MT9T113_write_cmos_sensor(0x990, 0x0800);	//		= 2048
	MT9T113_write_cmos_sensor(0x98E, 0x6C06);	//FOV Height (B)
	MT9T113_write_cmos_sensor(0x990, 0x0600);	//		= 1536
	MT9T113_write_cmos_sensor(0x98E, 0xEC92);	//JPEG Mode (B)
	MT9T113_write_cmos_sensor(0x990, 0x00); //		= 0
}

void test_pclk_48M(void)
{
	
	//[MT9T113 (SOC3140) Register Wizard Defaults]
	MT9T113_write_cmos_sensor(0x0010, 0x0C60);	//PLL Dividers = 3168
	MT9T113_write_cmos_sensor(0x0012, 0x0070);	//PLL P Dividers = 112
	MT9T113_write_cmos_sensor(0x002A, 0x7464);	//PLL P Dividers 4-5-6 = 29796
	MT9T113_write_cmos_sensor(0x0018, 0x402E);	//Standby:Default = 16430
	//POLL_REG=0x0018,0x4000,==1,DELAY=10,TIME);OUT=100   // Wait for FW initialization complete
	MT9T113_write_cmos_sensor(0x0022, 0x0140);	//Reference clock count for 20 us = 320
	MT9T113_write_cmos_sensor(0x001E, 0x777 );//Pad Slew Rate = 1911
	MT9T113_write_cmos_sensor(0x3B84, 0x017C);	//I2C Master Clock Divider = 380
											
	//LOAD the FW patches and recommended se);ttings here!
										   
	MT9T113_write_cmos_sensor(0x0018, 0x4028);	//Out of Standby = 16424\
	 mdelay(30);//DELAY=3
	MT9T113_write_cmos_sensor(0x98E, 0x4800 );//Row Start (A)
	MT9T113_write_cmos_sensor(0x990, 0x0010 );//	  = 16
	MT9T113_write_cmos_sensor(0x98E, 0x4802 );//Column Start (A)
	MT9T113_write_cmos_sensor(0x990, 0x0010 );//	  = 16
	MT9T113_write_cmos_sensor(0x98E, 0x4804 );//Row End (A)
	MT9T113_write_cmos_sensor(0x990, 0x062D );//	  = 1581
	MT9T113_write_cmos_sensor(0x98E, 0x4806 );//Column End (A)
	MT9T113_write_cmos_sensor(0x990, 0x082D );//	  = 2093
	MT9T113_write_cmos_sensor(0x98E, 0x4808 );//Base Frame Lines (A)
	MT9T113_write_cmos_sensor(0x990, 0x0364 );//	  = 868
	MT9T113_write_cmos_sensor(0x98E, 0x480A );//Line Length (A)
	MT9T113_write_cmos_sensor(0x990, 0x0D07 );//	  = 3335
	MT9T113_write_cmos_sensor(0x98E, 0x480C );//Fine Correction (A)
	MT9T113_write_cmos_sensor(0x990, 0x0399 );//	  = 921
	MT9T113_write_cmos_sensor(0x98E, 0x480E );//Row Speed (A)
	MT9T113_write_cmos_sensor(0x990, 0x0111 );//	  = 273
	MT9T113_write_cmos_sensor(0x98E, 0x4810 );//Read Mode (A)
	MT9T113_write_cmos_sensor(0x990, 0x046C );//	  = 1132
	MT9T113_write_cmos_sensor(0x98E, 0x4812 );//Fine IT Min (A)
	MT9T113_write_cmos_sensor(0x990, 0x0510 );//	  = 1296
	MT9T113_write_cmos_sensor(0x98E, 0x4814 );//Fine IT Max Margin (A)
	MT9T113_write_cmos_sensor(0x990, 0x01BA );//	  = 442
	MT9T113_write_cmos_sensor(0x98E, 0x482D );//Row Start (B)
	MT9T113_write_cmos_sensor(0x990, 0x0018 );//	  = 24
	MT9T113_write_cmos_sensor(0x98E, 0x482F );//Column Start (B)
	MT9T113_write_cmos_sensor(0x990, 0x0018 );//	  = 24
	MT9T113_write_cmos_sensor(0x98E, 0x4831 );//Row End (B)
	MT9T113_write_cmos_sensor(0x990, 0x0627 );//	  = 1575
	MT9T113_write_cmos_sensor(0x98E, 0x4833 );//Column End (B)
	MT9T113_write_cmos_sensor(0x990, 0x0827 );//	  = 2087
	MT9T113_write_cmos_sensor(0x98E, 0x4835 );//Base Frame Lines (B)
	MT9T113_write_cmos_sensor(0x990, 0x065D );//	  = 1629
	MT9T113_write_cmos_sensor(0x98E, 0x4837 );//Line Length (B)
	MT9T113_write_cmos_sensor(0x990, 0x1A4F );//	  = 6735
	MT9T113_write_cmos_sensor(0x98E, 0x4839 );//Fine Correction (B)
	MT9T113_write_cmos_sensor(0x990, 0x019F );//	  = 415
	MT9T113_write_cmos_sensor(0x98E, 0x483B );//Row Speed (B)
	MT9T113_write_cmos_sensor(0x990, 0x0111 );//	  = 273
	MT9T113_write_cmos_sensor(0x98E, 0x483D );//Read Mode (B)
	MT9T113_write_cmos_sensor(0x990, 0x0024 );//	  = 36
	MT9T113_write_cmos_sensor(0x98E, 0x483F );//Fine IT Min (B)
	MT9T113_write_cmos_sensor(0x990, 0x0266 );//	  = 614
	MT9T113_write_cmos_sensor(0x98E, 0x4841 );//Fine IT Max Margin (B)
	MT9T113_write_cmos_sensor(0x990, 0x010A );//	  = 266
	MT9T113_write_cmos_sensor(0x98E, 0xB81A );//fd_zone_height
	MT9T113_write_cmos_sensor(0x990, 0x05	);		//= 5
	MT9T113_write_cmos_sensor(0x98E, 0x481A );//fd_period_50Hz (A)
	MT9T113_write_cmos_sensor(0x990, 0x00E6 );//	  = 230
	MT9T113_write_cmos_sensor(0x98E, 0x481C );//fd_period_60Hz (A)
	MT9T113_write_cmos_sensor(0x990, 0x00C0 );//	  = 192
	MT9T113_write_cmos_sensor(0x98E, 0xC81E );//fd_search_f1_50hz (A)
	MT9T113_write_cmos_sensor(0x990, 0x25	);		//= 37
	MT9T113_write_cmos_sensor(0x98E, 0xC81F );//fd_search_f2_50hz (A)
	MT9T113_write_cmos_sensor(0x990, 0x27	);		//= 39
	MT9T113_write_cmos_sensor(0x98E, 0xC820 );//fd_search_f1_60hz (A)
	MT9T113_write_cmos_sensor(0x990, 0x2D	);		//= 45
	MT9T113_write_cmos_sensor(0x98E, 0xC821 );//fd_search_f2_60hz (A)
	MT9T113_write_cmos_sensor(0x990, 0x2F	);		//= 47
	MT9T113_write_cmos_sensor(0x98E, 0x4847 );//fd_period_50Hz (B)
	MT9T113_write_cmos_sensor(0x990, 0x0072 );//	  = 114
	MT9T113_write_cmos_sensor(0x98E, 0x4849 );//fd_period_60Hz (B)
	MT9T113_write_cmos_sensor(0x990, 0x005F );//	  = 95
	MT9T113_write_cmos_sensor(0x98E, 0xC84B );//fd_search_f1_50hz (B)
	MT9T113_write_cmos_sensor(0x990, 0x12	);		//= 18
	MT9T113_write_cmos_sensor(0x98E, 0xC84C );//fd_search_f2_50hz (B)
	MT9T113_write_cmos_sensor(0x990, 0x14	);		//= 20
	MT9T113_write_cmos_sensor(0x98E, 0xC84D );//fd_search_f1_60hz (B)
	MT9T113_write_cmos_sensor(0x990, 0x15	);	   // = 21
	MT9T113_write_cmos_sensor(0x98E, 0xC84E );//fd_search_f2_60hz (B)
	MT9T113_write_cmos_sensor(0x990, 0x17	);		//= 23
	MT9T113_write_cmos_sensor(0x98E, 0x6800 );//Output Width (A)
	MT9T113_write_cmos_sensor(0x990, 0x0400 );//	  = 1024
	MT9T113_write_cmos_sensor(0x98E, 0x6802 );//Output Height (A)
	MT9T113_write_cmos_sensor(0x990, 0x0300 );//	  = 768
	MT9T113_write_cmos_sensor(0x98E, 0x6804 );//FOV Width (A)
	MT9T113_write_cmos_sensor(0x990, 0x0400 );//	  = 1024
	MT9T113_write_cmos_sensor(0x98E, 0x6806 );//FOV Height (A)
	MT9T113_write_cmos_sensor(0x990, 0x0300 );//	  = 768
	MT9T113_write_cmos_sensor(0x98E, 0xE892 );//JPEG Mode (A)
	MT9T113_write_cmos_sensor(0x990, 0x00	);		//= 0
	MT9T113_write_cmos_sensor(0x98E, 0x6C00 );//Output Width (B)
	MT9T113_write_cmos_sensor(0x990, 0x0800 );//	  = 2048
	MT9T113_write_cmos_sensor(0x98E, 0x6C02 );//Output Height (B)
	MT9T113_write_cmos_sensor(0x990, 0x0600 );//	  = 1536
	MT9T113_write_cmos_sensor(0x98E, 0x6C04 );//FOV Width (B)
	MT9T113_write_cmos_sensor(0x990, 0x0800 );//	  = 2048
	MT9T113_write_cmos_sensor(0x98E, 0x6C06 );//FOV Height (B)
	MT9T113_write_cmos_sensor(0x990, 0x0600 );//	  = 1536
	MT9T113_write_cmos_sensor(0x98E, 0xEC92 );//JPEG Mode (B)
	MT9T113_write_cmos_sensor(0x990, 0x00	);	   // = 0
	MT9T113_write_cmos_sensor(0x98E, 0x8400 );//Refresh Sequencer Mode
	MT9T113_write_cmos_sensor(0x990, 0x06	);	   // = 6
	

}

static void MT9T113_YUV_sensor_initial_setting(void)
{
	kal_uint16 tmp=0;

	//PLL_Timing
	test_pclk_52M();

	//mdelay(10);//DELAY=100
	MT9T113_write_cmos_sensor(0x098E, 0x6CA6);
	MT9T113_write_cmos_sensor(0x0990, 0x082D);
	MT9T113_write_cmos_sensor(0x098E, 0xECA5);
	MT9T113_write_cmos_sensor(0x0990, 0x0000);
	MT9T113_write_cmos_sensor(0x098E, 0x6C94);
	MT9T113_write_cmos_sensor(0x0990, 0x0C34);
	
	MT9T113_write_cmos_sensor(0x3172, 0x0033);	// ANALOG_CONTROL2
	
	//TX
	MT9T113_write_cmos_sensor(0x3C86, 0x00E1);	// OB_PCLK1_CONFIG
	MT9T113_write_cmos_sensor(0x3C20, 0x0000);	// TX_SS_CONTROL
	
	//[CCM and AWB]
	MT9T113_write_cmos_sensor(0x098E, 0x4873);	// MCU_ADDRESS [CAM1_AWB_CCM_L_0]
	MT9T113_write_cmos_sensor(0x0990, 0x0206);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4875);	// MCU_ADDRESS [CAM1_AWB_CCM_L_1]
	MT9T113_write_cmos_sensor(0x0990, 0xFEDA);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4877);	// MCU_ADDRESS [CAM1_AWB_CCM_L_2]
	MT9T113_write_cmos_sensor(0x0990, 0x001F);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4879);	// MCU_ADDRESS [CAM1_AWB_CCM_L_3]
	MT9T113_write_cmos_sensor(0x0990, 0xFFC7);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x487B);	// MCU_ADDRESS [CAM1_AWB_CCM_L_4]
	MT9T113_write_cmos_sensor(0x0990, 0x014E);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x487D);	// MCU_ADDRESS [CAM1_AWB_CCM_L_5]
	MT9T113_write_cmos_sensor(0x0990, 0xFFE9);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x487F);	// MCU_ADDRESS [CAM1_AWB_CCM_L_6]
	MT9T113_write_cmos_sensor(0x0990, 0xFFAD);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4881);	// MCU_ADDRESS [CAM1_AWB_CCM_L_7]
	MT9T113_write_cmos_sensor(0x0990, 0xFF12);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4883);	// MCU_ADDRESS [CAM1_AWB_CCM_L_8]
	MT9T113_write_cmos_sensor(0x0990, 0x023F);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4885);	// MCU_ADDRESS [CAM1_AWB_CCM_L_9]
	MT9T113_write_cmos_sensor(0x0990, 0x001D);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4887);	// MCU_ADDRESS [CAM1_AWB_CCM_L_10]
	MT9T113_write_cmos_sensor(0x0990, 0x005E);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4889);	// MCU_ADDRESS [CAM1_AWB_CCM_RL_0]
	MT9T113_write_cmos_sensor(0x0990, 0xFFA2);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x488B);	// MCU_ADDRESS [CAM1_AWB_CCM_RL_1]
	MT9T113_write_cmos_sensor(0x0990, 0x00A5);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x488D);	// MCU_ADDRESS [CAM1_AWB_CCM_RL_2]
	MT9T113_write_cmos_sensor(0x0990, 0xFFB6);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x488F);	// MCU_ADDRESS [CAM1_AWB_CCM_RL_3]
	MT9T113_write_cmos_sensor(0x0990, 0x0002);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4891);	// MCU_ADDRESS [CAM1_AWB_CCM_RL_4]
	MT9T113_write_cmos_sensor(0x0990, 0xFFE5);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4893);	// MCU_ADDRESS [CAM1_AWB_CCM_RL_5]
	MT9T113_write_cmos_sensor(0x0990, 0x0017);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4895);	// MCU_ADDRESS [CAM1_AWB_CCM_RL_6]
	MT9T113_write_cmos_sensor(0x0990, 0x004C);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4897);	// MCU_ADDRESS [CAM1_AWB_CCM_RL_7]
	MT9T113_write_cmos_sensor(0x0990, 0x007F);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4899);	// MCU_ADDRESS [CAM1_AWB_CCM_RL_8]
	MT9T113_write_cmos_sensor(0x0990, 0xFF34);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x489B);	// MCU_ADDRESS [CAM1_AWB_CCM_RL_9]
	MT9T113_write_cmos_sensor(0x0990, 0x0014);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x489D);	// MCU_ADDRESS [CAM1_AWB_CCM_RL_10]
	MT9T113_write_cmos_sensor(0x0990, 0xFFCF);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48B8);	// MCU_ADDRESS [CAM1_AWB_X_SHIFT]
	MT9T113_write_cmos_sensor(0x0990, 0x0021);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48BA);	// MCU_ADDRESS [CAM1_AWB_Y_SHIFT]
	MT9T113_write_cmos_sensor(0x0990, 0x0015);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48BC);	// MCU_ADDRESS [CAM1_AWB_RECIP_XSCALE]
	MT9T113_write_cmos_sensor(0x0990, 0x0080);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48BE);	// MCU_ADDRESS [CAM1_AWB_RECIP_YSCALE]
	MT9T113_write_cmos_sensor(0x0990, 0x00AB);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48C0);	// MCU_ADDRESS [CAM1_AWB_ROT_CENTER_X]
	MT9T113_write_cmos_sensor(0x0990, 0x03FC);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48C2);	// MCU_ADDRESS [CAM1_AWB_ROT_CENTER_Y]
	MT9T113_write_cmos_sensor(0x0990, 0x03E1);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC8C4);	// MCU_ADDRESS [CAM1_AWB_ROT_SIN]
	MT9T113_write_cmos_sensor(0x0990, 0x0036);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC8C5);	// MCU_ADDRESS [CAM1_AWB_ROT_COS]
	MT9T113_write_cmos_sensor(0x0990, 0x0023);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48C6);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_0]
	MT9T113_write_cmos_sensor(0x0990, 0x0000);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48C8);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_1]
	MT9T113_write_cmos_sensor(0x0990, 0x0011);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48CA);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_2]
	MT9T113_write_cmos_sensor(0x0990, 0x1110);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48CC);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_3]
	MT9T113_write_cmos_sensor(0x0990, 0x0000);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48CE);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_4]
	MT9T113_write_cmos_sensor(0x0990, 0x0000);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48D0);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_5]
	MT9T113_write_cmos_sensor(0x0990, 0x0011);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48D2);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_6]
	MT9T113_write_cmos_sensor(0x0990, 0x1111);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48D4);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_7]
	MT9T113_write_cmos_sensor(0x0990, 0x1000);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48D6);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_8]
	MT9T113_write_cmos_sensor(0x0990, 0x0000);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48D8);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_9]
	MT9T113_write_cmos_sensor(0x0990, 0x1111);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48DA);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_10]
	MT9T113_write_cmos_sensor(0x0990, 0x2222);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48DC);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_11]
	MT9T113_write_cmos_sensor(0x0990, 0x1100);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48DE);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_12]
	MT9T113_write_cmos_sensor(0x0990, 0x0011);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48E0);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_13]
	MT9T113_write_cmos_sensor(0x0990, 0x2222);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48E2);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_14]
	MT9T113_write_cmos_sensor(0x0990, 0x2222);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48E4);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_15]
	MT9T113_write_cmos_sensor(0x0990, 0x2100);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48E6);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_16]
	MT9T113_write_cmos_sensor(0x0990, 0x0113);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48E8);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_17]
	MT9T113_write_cmos_sensor(0x0990, 0x4543);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48EA);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_18]
	MT9T113_write_cmos_sensor(0x0990, 0x2112);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48EC);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_19]
	MT9T113_write_cmos_sensor(0x0990, 0x2110);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48EE);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_20]
	MT9T113_write_cmos_sensor(0x0990, 0x0123);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48F0);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_21]
	MT9T113_write_cmos_sensor(0x0990, 0x5543);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48F2);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_22]
	MT9T113_write_cmos_sensor(0x0990, 0x2212);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48F4);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_23]
	MT9T113_write_cmos_sensor(0x0990, 0x2210);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48F6);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_24]
	MT9T113_write_cmos_sensor(0x0990, 0x0112);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48F8);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_25]
	MT9T113_write_cmos_sensor(0x0990, 0x3333);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48FA);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_26]
	MT9T113_write_cmos_sensor(0x0990, 0x2111);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48FC);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_27]
	MT9T113_write_cmos_sensor(0x0990, 0x2211);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x48FE);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_28]
	MT9T113_write_cmos_sensor(0x0990, 0x0011);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4900);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_29]
	MT9T113_write_cmos_sensor(0x0990, 0x1111);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4902);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_30]
	MT9T113_write_cmos_sensor(0x0990, 0x1111);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4904);	// MCU_ADDRESS [CAM1_AWB_WEIGHT_TABLE_31]
	MT9T113_write_cmos_sensor(0x0990, 0x1110);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xAC3B);	// MCU_ADDRESS [AWB_R_RATIO_PRE_AWB]
	MT9T113_write_cmos_sensor(0x0990, 0x0055);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xAC3C);	// MCU_ADDRESS [AWB_B_RATIO_PRE_AWB]
	MT9T113_write_cmos_sensor(0x0990, 0x0025);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xAC37);	// MCU_ADDRESS [AWB_R_SCENE_RATIO_LOWER]
	MT9T113_write_cmos_sensor(0x0990, 0x0036);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xAC38);	// MCU_ADDRESS [AWB_R_SCENE_RATIO_UPPER]
	MT9T113_write_cmos_sensor(0x0990, 0x005E);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xAC39);	// MCU_ADDRESS [AWB_B_SCENE_RATIO_LOWER]
	MT9T113_write_cmos_sensor(0x0990, 0x0026);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xAC3A);	// MCU_ADDRESS [AWB_B_SCENE_RATIO_UPPER]
	MT9T113_write_cmos_sensor(0x0990, 0x004B);	// MCU_DATA_0


	MT9T113_write_cmos_sensor(0x098E, 0x4887);
	MT9T113_write_cmos_sensor(0x0990, 0x0051);
	MT9T113_write_cmos_sensor(0x098E, 0x489D);
	MT9T113_write_cmos_sensor(0x0990, 0xFFE2);
	MT9T113_write_cmos_sensor(0x098E, 0x48B8);
	MT9T113_write_cmos_sensor(0x0990, 0x0027);
	MT9T113_write_cmos_sensor(0x098E, 0x48BA);
	MT9T113_write_cmos_sensor(0x0990, 0x001D);
	//turn off low ccm
	MT9T113_write_cmos_sensor(0x098E, 0xAC02);
	MT9T113_write_cmos_sensor(0x0990, 0x0004);
	//reduce blue
	MT9T113_write_cmos_sensor(0x098E, 0xE856);
	MT9T113_write_cmos_sensor(0x0990, 0x0070);
	MT9T113_write_cmos_sensor(0x098E, 0xE853);
	MT9T113_write_cmos_sensor(0x0990, 0x0070);
	
	//Char_settings
	MT9T113_write_cmos_sensor(0x3ED6, 0x0F00);	// DAC_LD_10_11
	MT9T113_write_cmos_sensor(0x3EF2, 0xD965);	// DAC_LP_6_7
	MT9T113_write_cmos_sensor(0x3FD2, 0xD965);
	MT9T113_write_cmos_sensor(0x3EF8, 0x7F7F);	// DAC_LD_TXHI
	MT9T113_write_cmos_sensor(0x3ED8, 0x7F1D);	// DAC_LD_12_13
	MT9T113_write_cmos_sensor(0x3172, 0x0033);	// ANALOG_CONTROL2
	MT9T113_write_cmos_sensor(0x3EEA, 0x0200);	// DAC_LD_30_31
	MT9T113_write_cmos_sensor(0x3EE2, 0x0050);	// DAC_LD_22_23
	MT9T113_write_cmos_sensor(0x316A, 0x8200);	// DAC_FBIAS
	MT9T113_write_cmos_sensor(0x316C, 0x8200);	// DAC_TXLO
	MT9T113_write_cmos_sensor(0x3EFC, 0xA8E8);	// DAC_LD_FBIAS
	MT9T113_write_cmos_sensor(0x3EFE, 0x130D);	// DAC_LD_TXLO

	
	// Additional Optimized Settings
	MT9T113_write_cmos_sensor(0x3180, 0xB3FF);	// 
	MT9T113_write_cmos_sensor(0x30B2, 0xC000);	// 
	
	MT9T113_write_cmos_sensor(0x30BC, 0x0384);	// CALIB_GLOBAL
	MT9T113_write_cmos_sensor(0x30C0, 0x1220);	// CALIB_CONTROL
	
		// Low_Power_Mode
	MT9T113_write_cmos_sensor(0x3170, 0x000A);     //Dynamic pwr setting
	MT9T113_write_cmos_sensor(0x3174, 0x8060);     //Dynamic pwr setting
	MT9T113_write_cmos_sensor(0x3ECC, 0x22B0);     //Dynamic pwr setting
	MT9T113_write_cmos_sensor(0x098E, 0x482B);     //LP Mode (A)
	MT9T113_write_cmos_sensor(0x0990, 0x22B0);     //
	MT9T113_write_cmos_sensor(0x098E, 0x4858);     //LP Mode (B)
	MT9T113_write_cmos_sensor(0x0990, 0x22B0);     //
	MT9T113_write_cmos_sensor(0x317A, 0x000A); 	// 
	MT9T113_write_cmos_sensor(0x098E, 0x4822); 	// 
	MT9T113_write_cmos_sensor(0x0990, 0x000A); 	// 
	MT9T113_write_cmos_sensor(0x098E, 0x4824); 	// 
	MT9T113_write_cmos_sensor(0x0990, 0x000A); 	// 
	MT9T113_write_cmos_sensor(0x098E, 0x484F); 	// 
	MT9T113_write_cmos_sensor(0x0990, 0x000A); 	// 
	MT9T113_write_cmos_sensor(0x098E, 0x4851); 	// 
	MT9T113_write_cmos_sensor(0x0990, 0x000A); 	//

	
	//LSC
	//85%
	MT9T113_write_cmos_sensor(0x3210, 0x01B0);
	#if 1
	MT9T113_write_cmos_sensor(0x3640, 0x0490);
	MT9T113_write_cmos_sensor(0x3642, 0x216E);
	MT9T113_write_cmos_sensor(0x3644, 0x1371);
	MT9T113_write_cmos_sensor(0x3646, 0xFD8E);
	MT9T113_write_cmos_sensor(0x3648, 0x8210);
	MT9T113_write_cmos_sensor(0x364A, 0x01D0);
	MT9T113_write_cmos_sensor(0x364C, 0xEB6D);
	MT9T113_write_cmos_sensor(0x364E, 0x6C10);
	MT9T113_write_cmos_sensor(0x3650, 0x616E);
	MT9T113_write_cmos_sensor(0x3652, 0xA38E);
	MT9T113_write_cmos_sensor(0x3654, 0x01B0);
	MT9T113_write_cmos_sensor(0x3656, 0x4CCE);
	MT9T113_write_cmos_sensor(0x3658, 0x1DF0);
	MT9T113_write_cmos_sensor(0x365A, 0xCEEF);
	MT9T113_write_cmos_sensor(0x365C, 0x6BCF);
	MT9T113_write_cmos_sensor(0x365E, 0x0110);
	MT9T113_write_cmos_sensor(0x3660, 0xF9CD);
	MT9T113_write_cmos_sensor(0x3662, 0x1911);
	MT9T113_write_cmos_sensor(0x3664, 0xB1CC);
	MT9T113_write_cmos_sensor(0x3666, 0xF7CF);
	MT9T113_write_cmos_sensor(0x3680, 0x92AD);
	MT9T113_write_cmos_sensor(0x3682, 0xE4EE);
	MT9T113_write_cmos_sensor(0x3684, 0x8FF0);
	MT9T113_write_cmos_sensor(0x3686, 0x0770);
	MT9T113_write_cmos_sensor(0x3688, 0x2C92);
	MT9T113_write_cmos_sensor(0x368A, 0xE08D);
	MT9T113_write_cmos_sensor(0x368C, 0x0DEE);
	MT9T113_write_cmos_sensor(0x368E, 0x9FEF);
	MT9T113_write_cmos_sensor(0x3690, 0xA62E);
	MT9T113_write_cmos_sensor(0x3692, 0x0252);
	MT9T113_write_cmos_sensor(0x3694, 0x6F0D);
	MT9T113_write_cmos_sensor(0x3696, 0x0B8F);
	MT9T113_write_cmos_sensor(0x3698, 0x946E);
	MT9T113_write_cmos_sensor(0x369A, 0xEA0F);
	MT9T113_write_cmos_sensor(0x369C, 0x3071);
	MT9T113_write_cmos_sensor(0x369E, 0x76EC);
	MT9T113_write_cmos_sensor(0x36A0, 0xD32E);
	MT9T113_write_cmos_sensor(0x36A2, 0x282F);
	MT9T113_write_cmos_sensor(0x36A4, 0x1C8F);
	MT9T113_write_cmos_sensor(0x36A6, 0x098F);
	MT9T113_write_cmos_sensor(0x36C0, 0x3451);
	MT9T113_write_cmos_sensor(0x36C2, 0x53AF);
	MT9T113_write_cmos_sensor(0x36C4, 0x21D2);
	MT9T113_write_cmos_sensor(0x36C6, 0xA151);
	MT9T113_write_cmos_sensor(0x36C8, 0xDE14);
	MT9T113_write_cmos_sensor(0x36CA, 0x42B1);
	MT9T113_write_cmos_sensor(0x36CC, 0x832F);
	MT9T113_write_cmos_sensor(0x36CE, 0x5892);
	MT9T113_write_cmos_sensor(0x36D0, 0x288D);
	MT9T113_write_cmos_sensor(0x36D2, 0xF254);
	MT9T113_write_cmos_sensor(0x36D4, 0x12F1);
	MT9T113_write_cmos_sensor(0x36D6, 0x1D10);
	MT9T113_write_cmos_sensor(0x36D8, 0x10F3);
	MT9T113_write_cmos_sensor(0x36DA, 0x9451);
	MT9T113_write_cmos_sensor(0x36DC, 0x84D5);
	MT9T113_write_cmos_sensor(0x36DE, 0x2B11);
	MT9T113_write_cmos_sensor(0x36E0, 0xC1CF);
	MT9T113_write_cmos_sensor(0x36E2, 0x4B32);
	MT9T113_write_cmos_sensor(0x36E4, 0xC20F);
	MT9T113_write_cmos_sensor(0x36E6, 0xCF94);
	MT9T113_write_cmos_sensor(0x3700, 0x2770);
	MT9T113_write_cmos_sensor(0x3702, 0x2371);
	MT9T113_write_cmos_sensor(0x3704, 0x3AD3);
	MT9T113_write_cmos_sensor(0x3706, 0xC152);
	MT9T113_write_cmos_sensor(0x3708, 0xF7F4);
	MT9T113_write_cmos_sensor(0x370A, 0x0BF1);
	MT9T113_write_cmos_sensor(0x370C, 0x5D2E);
	MT9T113_write_cmos_sensor(0x370E, 0x6D32);
	MT9T113_write_cmos_sensor(0x3710, 0x174F);
	MT9T113_write_cmos_sensor(0x3712, 0xF4D4);
	MT9T113_write_cmos_sensor(0x3714, 0x1690);
	MT9T113_write_cmos_sensor(0x3716, 0x566C);
	MT9T113_write_cmos_sensor(0x3718, 0x4B13);
	MT9T113_write_cmos_sensor(0x371A, 0x9AD0);
	MT9T113_write_cmos_sensor(0x371C, 0xAED5);
	MT9T113_write_cmos_sensor(0x371E, 0x3290);
	MT9T113_write_cmos_sensor(0x3720, 0x470F);
	MT9T113_write_cmos_sensor(0x3722, 0x55D2);
	MT9T113_write_cmos_sensor(0x3724, 0xEE11);
	MT9T113_write_cmos_sensor(0x3726, 0xEE93);
	MT9T113_write_cmos_sensor(0x3740, 0x89D2);
	MT9T113_write_cmos_sensor(0x3742, 0xF7F2);
	MT9T113_write_cmos_sensor(0x3744, 0xBA55);
	MT9T113_write_cmos_sensor(0x3746, 0x5094);
	MT9T113_write_cmos_sensor(0x3748, 0x2297);
	MT9T113_write_cmos_sensor(0x374A, 0xA392);
	MT9T113_write_cmos_sensor(0x374C, 0xEDB0);
	MT9T113_write_cmos_sensor(0x374E, 0xB995);
	MT9T113_write_cmos_sensor(0x3750, 0x8691);
	MT9T113_write_cmos_sensor(0x3752, 0x4017);
	MT9T113_write_cmos_sensor(0x3754, 0xC5B1);
	MT9T113_write_cmos_sensor(0x3756, 0x98D3);
	MT9T113_write_cmos_sensor(0x3758, 0xDA35);
	MT9T113_write_cmos_sensor(0x375A, 0x57F4);
	MT9T113_write_cmos_sensor(0x375C, 0x2437);
	MT9T113_write_cmos_sensor(0x375E, 0xD6B1);
	MT9T113_write_cmos_sensor(0x3760, 0xE86F);
	MT9T113_write_cmos_sensor(0x3762, 0xB6D5);
	MT9T113_write_cmos_sensor(0x3764, 0x4CF3);
	MT9T113_write_cmos_sensor(0x3766, 0x0C17);
	MT9T113_write_cmos_sensor(0x3782, 0x02A0);
	MT9T113_write_cmos_sensor(0x3784, 0x03C0);
	#else
	MT9T113_write_cmos_sensor(0x3640, 0x0150);
	MT9T113_write_cmos_sensor(0x3642, 0x2FAE);
	MT9T113_write_cmos_sensor(0x3644, 0x54D1);
	MT9T113_write_cmos_sensor(0x3646, 0x4F4D);
	MT9T113_write_cmos_sensor(0x3648, 0xA391);
	MT9T113_write_cmos_sensor(0x364A, 0x0310);
	MT9T113_write_cmos_sensor(0x364C, 0xC6CD);
	MT9T113_write_cmos_sensor(0x364E, 0x1ED1);
	MT9T113_write_cmos_sensor(0x3650, 0x7BAF);
	MT9T113_write_cmos_sensor(0x3652, 0xC0F0);
	MT9T113_write_cmos_sensor(0x3654, 0x02D0);
	MT9T113_write_cmos_sensor(0x3656, 0x5AEE);
	MT9T113_write_cmos_sensor(0x3658, 0x6CF0);
	MT9T113_write_cmos_sensor(0x365A, 0xAEEE);
	MT9T113_write_cmos_sensor(0x365C, 0x87EE);
	MT9T113_write_cmos_sensor(0x365E, 0x0350);
	MT9T113_write_cmos_sensor(0x3660, 0xFBCD);
	MT9T113_write_cmos_sensor(0x3662, 0x5E31);
	MT9T113_write_cmos_sensor(0x3664, 0x7ACE);
	MT9T113_write_cmos_sensor(0x3666, 0xB8F1);
	MT9T113_write_cmos_sensor(0x3680, 0xF00D);
	MT9T113_write_cmos_sensor(0x3682, 0xC26E);
	MT9T113_write_cmos_sensor(0x3684, 0xD1AF);
	MT9T113_write_cmos_sensor(0x3686, 0x44CF);
	MT9T113_write_cmos_sensor(0x3688, 0x18D1);
	MT9T113_write_cmos_sensor(0x368A, 0x8A6E);
	MT9T113_write_cmos_sensor(0x368C, 0x1AEE);
	MT9T113_write_cmos_sensor(0x368E, 0x5A0C);
	MT9T113_write_cmos_sensor(0x3690, 0xE5CE);
	MT9T113_write_cmos_sensor(0x3692, 0x11EF);
	MT9T113_write_cmos_sensor(0x3694, 0x242B);
	MT9T113_write_cmos_sensor(0x3696, 0x4BEE);
	MT9T113_write_cmos_sensor(0x3698, 0x0A70);
	MT9T113_write_cmos_sensor(0x369A, 0x918F);
	MT9T113_write_cmos_sensor(0x369C, 0x8DB1);
	MT9T113_write_cmos_sensor(0x369E, 0x2FEB);
	MT9T113_write_cmos_sensor(0x36A0, 0xBF8E);
	MT9T113_write_cmos_sensor(0x36A2, 0x1310);
	MT9T113_write_cmos_sensor(0x36A4, 0x624F);
	MT9T113_write_cmos_sensor(0x36A6, 0xBA31);
	MT9T113_write_cmos_sensor(0x36C0, 0x6411);
	MT9T113_write_cmos_sensor(0x36C2, 0x2410);
	MT9T113_write_cmos_sensor(0x36C4, 0x6130);
	MT9T113_write_cmos_sensor(0x36C6, 0x91D0);
	MT9T113_write_cmos_sensor(0x36C8, 0xA114);
	MT9T113_write_cmos_sensor(0x36CA, 0x7451);
	MT9T113_write_cmos_sensor(0x36CC, 0xCDCF);
	MT9T113_write_cmos_sensor(0x36CE, 0x2D51);
	MT9T113_write_cmos_sensor(0x36D0, 0x6890);
	MT9T113_write_cmos_sensor(0x36D2, 0xB474);
	MT9T113_write_cmos_sensor(0x36D4, 0x2911);
	MT9T113_write_cmos_sensor(0x36D6, 0x582F);
	MT9T113_write_cmos_sensor(0x36D8, 0x0E92);
	MT9T113_write_cmos_sensor(0x36DA, 0x4A6E);
	MT9T113_write_cmos_sensor(0x36DC, 0xB114);
	MT9T113_write_cmos_sensor(0x36DE, 0x5F31);
	MT9T113_write_cmos_sensor(0x36E0, 0xB5CF);
	MT9T113_write_cmos_sensor(0x36E2, 0x2C10);
	MT9T113_write_cmos_sensor(0x36E4, 0x4831);
	MT9T113_write_cmos_sensor(0x36E6, 0x8AB4);
	MT9T113_write_cmos_sensor(0x3700, 0xB66D);
	MT9T113_write_cmos_sensor(0x3702, 0x528F);
	MT9T113_write_cmos_sensor(0x3704, 0x37CE);
	MT9T113_write_cmos_sensor(0x3706, 0xEB51);
	MT9T113_write_cmos_sensor(0x3708, 0x2EF0);
	MT9T113_write_cmos_sensor(0x370A, 0x1A0F);
	MT9T113_write_cmos_sensor(0x370C, 0x3FEF);
	MT9T113_write_cmos_sensor(0x370E, 0x8312);
	MT9T113_write_cmos_sensor(0x3710, 0x96F1);
	MT9T113_write_cmos_sensor(0x3712, 0x2A13);
	MT9T113_write_cmos_sensor(0x3714, 0x4847);
	MT9T113_write_cmos_sensor(0x3716, 0x96CF);
	MT9T113_write_cmos_sensor(0x3718, 0xEE51);
	MT9T113_write_cmos_sensor(0x371A, 0x7F10);
	MT9T113_write_cmos_sensor(0x371C, 0x1E53);
	MT9T113_write_cmos_sensor(0x371E, 0x05ED);
	MT9T113_write_cmos_sensor(0x3720, 0x3FCF);
	MT9T113_write_cmos_sensor(0x3722, 0xD4F1);
	MT9T113_write_cmos_sensor(0x3724, 0x9B6F);
	MT9T113_write_cmos_sensor(0x3726, 0x3492);
	MT9T113_write_cmos_sensor(0x3740, 0x9992);
	MT9T113_write_cmos_sensor(0x3742, 0x8F52);
	MT9T113_write_cmos_sensor(0x3744, 0x9C94);
	MT9T113_write_cmos_sensor(0x3746, 0x5AB3);
	MT9T113_write_cmos_sensor(0x3748, 0x79B5);
	MT9T113_write_cmos_sensor(0x374A, 0xACD2);
	MT9T113_write_cmos_sensor(0x374C, 0x05F1);
	MT9T113_write_cmos_sensor(0x374E, 0xFE33);
	MT9T113_write_cmos_sensor(0x3750, 0x9A11);
	MT9T113_write_cmos_sensor(0x3752, 0x3036);
	MT9T113_write_cmos_sensor(0x3754, 0xB311);
	MT9T113_write_cmos_sensor(0x3756, 0x9652);
	MT9T113_write_cmos_sensor(0x3758, 0xAE94);
	MT9T113_write_cmos_sensor(0x375A, 0x1654);
	MT9T113_write_cmos_sensor(0x375C, 0x1DF6);
	MT9T113_write_cmos_sensor(0x375E, 0x8872);
	MT9T113_write_cmos_sensor(0x3760, 0x3051);
	MT9T113_write_cmos_sensor(0x3762, 0xB734);
	MT9T113_write_cmos_sensor(0x3764, 0xA9F3);
	MT9T113_write_cmos_sensor(0x3766, 0x1796);
	MT9T113_write_cmos_sensor(0x3782, 0x02E0);
	MT9T113_write_cmos_sensor(0x3784, 0x03C0);

	#endif
	MT9T113_write_cmos_sensor(0x3210, 0x01B8);
	
	//AE
	  MT9T113_write_cmos_sensor(0x098E, 0x6820); 	// MCU_ADDRESS [PRI_A_CONFIG_AE_TRACK_TARGET_FDZONE]
	  MT9T113_write_cmos_sensor(0x0990, 0x0007); 	// MCU_DATA_0
	  MT9T113_write_cmos_sensor(0x098E, 0x6822); 	// MCU_ADDRESS [PRI_A_CONFIG_AE_TRACK_TARGET_AGAIN]
	  MT9T113_write_cmos_sensor(0x0990, 0x0064); 	// MCU_DATA_0
	  MT9T113_write_cmos_sensor(0x098E, 0x6824); 	// MCU_ADDRESS [PRI_A_CONFIG_AE_TRACK_TARGET_DGAIN]
	  MT9T113_write_cmos_sensor(0x0990, 0x0080); 	// MCU_DATA_0
	  MT9T113_write_cmos_sensor(0x098E, 0xE826); 	// MCU_ADDRESS [PRI_A_CONFIG_AE_TRACK_BASE_TARGET]
	  MT9T113_write_cmos_sensor(0x0990, 0x003b); 	// MCU_DATA_0
	  MT9T113_write_cmos_sensor(0x098E, 0x6829); 	// MCU_ADDRESS [PRI_A_CONFIG_AE_TRACK_AE_MIN_VIRT_DGAIN]
	  MT9T113_write_cmos_sensor(0x0990, 0x0080); 	// MCU_DATA_0
	  MT9T113_write_cmos_sensor(0x098E, 0x682B); 	// MCU_ADDRESS [PRI_A_CONFIG_AE_TRACK_AE_MAX_VIRT_DGAIN]
	  MT9T113_write_cmos_sensor(0x0990, 0x0080); 	// MCU_DATA_0
	  MT9T113_write_cmos_sensor(0x098E, 0x682D); 	// MCU_ADDRESS [PRI_A_CONFIG_AE_TRACK_AE_MIN_VIRT_AGAIN]
	  MT9T113_write_cmos_sensor(0x0990, 0x0038); 	// MCU_DATA_0
	  MT9T113_write_cmos_sensor(0x098E, 0x486F); 	// MCU_ADDRESS [CAM1_CTL_MAX_ANALOG_GAIN]
	  MT9T113_write_cmos_sensor(0x0990, 0x0120); 	// MCU_DATA_0
	  MT9T113_write_cmos_sensor(0x098E, 0x4871); 	// MCU_ADDRESS [CAM1_CTL_MIN_ANALOG_GAIN]
	  MT9T113_write_cmos_sensor(0x0990, 0x0038); 	// MCU_DATA_0
	  MT9T113_write_cmos_sensor(0x098E, 0x682F); 	// MCU_ADDRESS [PRI_A_CONFIG_AE_TRACK_AE_MAX_VIRT_AGAIN]
	  MT9T113_write_cmos_sensor(0x0990, 0x00F0);	// MCU_DATA_0 reduce gain
	
	





	
	//Step6-CPIPE_Calibration
	
	//low_light
	MT9T113_write_cmos_sensor(0x098E, 0x4918); 	// MCU_ADDRESS [CAM1_LL_START_GAIN_METRIC]
	MT9T113_write_cmos_sensor(0x0990, 0x0039); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x491A); 	// MCU_ADDRESS [CAM1_LL_STOP_GAIN_METRIC]
	MT9T113_write_cmos_sensor(0x0990, 0x0100);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x6872); 	// MCU_ADDRESS [PRI_A_CONFIG_LL_START_BRIGHTNESS]
	MT9T113_write_cmos_sensor(0x0990, 0x0005); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x6874); 	// MCU_ADDRESS [PRI_A_CONFIG_LL_STOP_BRIGHTNESS]
	MT9T113_write_cmos_sensor(0x0990, 0x008C); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4956); 	// MCU_ADDRESS [CAM1_LL_DC_START_GAIN_METRIC]
	MT9T113_write_cmos_sensor(0x0990, 0x0040); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4958); 	// MCU_ADDRESS [CAM1_LL_DC_STOP_GAIN_METRIC]
	MT9T113_write_cmos_sensor(0x0990, 0x0100);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x495A); 	// MCU_ADDRESS [CAM1_LL_DC_START]
	MT9T113_write_cmos_sensor(0x0990, 0x0000); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x495C); 	// MCU_ADDRESS [CAM1_LL_DC_STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x0000); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x495E); 	// MCU_ADDRESS [CAM1_LL_CDC_AGG_START_GAIN_METRIC]
	MT9T113_write_cmos_sensor(0x0990, 0x0040); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4960); 	// MCU_ADDRESS [CAM1_LL_CDC_AGG_STOP_GAIN_METRIC]
	MT9T113_write_cmos_sensor(0x0990, 0x0100);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC962); 	// MCU_ADDRESS [CAM1_LL_CDC_AGG_START]
	MT9T113_write_cmos_sensor(0x0990, 0x0000); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC963); 	// MCU_ADDRESS [CAM1_LL_CDC_AGG_STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x0003); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4964); 	// MCU_ADDRESS [CAM1_LL_CDC_BRIGHT_START_GAIN_METRIC]
	MT9T113_write_cmos_sensor(0x0990, 0x0040); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4966); 	// MCU_ADDRESS [CAM1_LL_CDC_BRIGHT_STOP_GAIN_METRIC]
	MT9T113_write_cmos_sensor(0x0990, 0x0100);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4968); 	// MCU_ADDRESS [CAM1_LL_CDC_BRIGHT_T3START]
	MT9T113_write_cmos_sensor(0x0990, 0x0001); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x496A); 	// MCU_ADDRESS [CAM1_LL_CDC_BRIGHT_T3STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x0001); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x496C); 	// MCU_ADDRESS [CAM1_LL_CDC_BRIGHT_T4START]
	MT9T113_write_cmos_sensor(0x0990, 0x0014); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x496E); 	// MCU_ADDRESS [CAM1_LL_CDC_BRIGHT_T4STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x000C); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC970); 	// MCU_ADDRESS [CAM1_LL_CDC_BRIGHT_TO_START]
	MT9T113_write_cmos_sensor(0x0990, 0x0004); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC971); 	// MCU_ADDRESS [CAM1_LL_CDC_BRIGHT_TO_STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x000F); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4972); 	// MCU_ADDRESS [CAM1_LL_CDC_DARK_START_GAIN_METRIC]
	MT9T113_write_cmos_sensor(0x0990, 0x0040); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4974); 	// MCU_ADDRESS [CAM1_LL_CDC_DARK_STOP_GAIN_METRIC]
	MT9T113_write_cmos_sensor(0x0990, 0x0100);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4976); 	// MCU_ADDRESS [CAM1_LL_CDC_DARK_T3START]
	MT9T113_write_cmos_sensor(0x0990, 0x0001); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4978); 	// MCU_ADDRESS [CAM1_LL_CDC_DARK_T3STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x0001); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x497A); 	// MCU_ADDRESS [CAM1_LL_CDC_DARK_T4START]
	MT9T113_write_cmos_sensor(0x0990, 0x00C8); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x497C); 	// MCU_ADDRESS [CAM1_LL_CDC_DARK_T4STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x003C); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC97E); 	// MCU_ADDRESS [CAM1_LL_CDC_DARK_TO_START]
	MT9T113_write_cmos_sensor(0x0990, 0x0004); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC97F); 	// MCU_ADDRESS [CAM1_LL_CDC_DARK_TO_STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x000F); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x491C); 	// MCU_ADDRESS [CAM1_LL_GRB_START_GAIN_METRIC]
	MT9T113_write_cmos_sensor(0x0990, 0x0040); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x491E); 	// MCU_ADDRESS [CAM1_LL_GRB_STOP_GAIN_METRIC]
	MT9T113_write_cmos_sensor(0x0990, 0x0100);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC920); 	// MCU_ADDRESS [CAM1_LL_GRB_SLOPE_START]
	MT9T113_write_cmos_sensor(0x0990, 0x000B); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC921); 	// MCU_ADDRESS [CAM1_LL_GRB_SLOPE_STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x002C); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC922); 	// MCU_ADDRESS [CAM1_LL_GRB_OFFSET_START]
	MT9T113_write_cmos_sensor(0x0990, 0x0007); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC923); 	// MCU_ADDRESS [CAM1_LL_GRB_OFFSET_STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x001D); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4926); 	// MCU_ADDRESS [CAM1_LL_SFFB_START_ANALOG_GAIN]
	MT9T113_write_cmos_sensor(0x0990, 0x0039); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4928); 	// MCU_ADDRESS [CAM1_LL_SFFB_END_ANALOG_GAIN]
	MT9T113_write_cmos_sensor(0x0990, 0x00A0); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x492A); 	// MCU_ADDRESS [CAM1_LL_SFFB_RAMP_START]
	MT9T113_write_cmos_sensor(0x0990, 0x0082); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x492C); 	// MCU_ADDRESS [CAM1_LL_SFFB_RAMP_STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x0040); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x492E); 	// MCU_ADDRESS [CAM1_LL_SFFB_SLOPE_START]
	MT9T113_write_cmos_sensor(0x0990, 0x0015); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4930); 	// MCU_ADDRESS [CAM1_LL_SFFB_SLOPE_STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x0015); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4932); 	// MCU_ADDRESS [CAM1_LL_SFFB_LOW_THRESH1START]
	MT9T113_write_cmos_sensor(0x0990, 0x0002); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4934); 	// MCU_ADDRESS [CAM1_LL_SFFB_LOW_THRESH1STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x0004); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4936); 	// MCU_ADDRESS [CAM1_LL_SFFB_LOW_THRESH2START]
	MT9T113_write_cmos_sensor(0x0990, 0x0008); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4938); 	// MCU_ADDRESS [CAM1_LL_SFFB_LOW_THRESH2STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x0009); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x493A); 	// MCU_ADDRESS [CAM1_LL_SFFB_LOW_THRESH3START]
	MT9T113_write_cmos_sensor(0x0990, 0x000C); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x493C); 	// MCU_ADDRESS [CAM1_LL_SFFB_LOW_THRESH3STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x000D); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x493E); 	// MCU_ADDRESS [CAM1_LL_SFFB_MAX_THRESH_START]
	MT9T113_write_cmos_sensor(0x0990, 0x0015); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4940); 	// MCU_ADDRESS [CAM1_LL_SFFB_MAX_THRESH_STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x0013); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC944); 	// MCU_ADDRESS [CAM1_LL_SFFB_FLATNESS_START]
	MT9T113_write_cmos_sensor(0x0990, 0x0023); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC945); 	// MCU_ADDRESS [CAM1_LL_SFFB_FLATNESS_STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x007F); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC946); 	// MCU_ADDRESS [CAM1_LL_SFFB_TRANSITION_START]
	MT9T113_write_cmos_sensor(0x0990, 0x0007); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC947); 	// MCU_ADDRESS [CAM1_LL_SFFB_TRANSITION_STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x0001); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC948); 	// MCU_ADDRESS [CAM1_LL_SFFB_SOBEL_FLAT_START]
	MT9T113_write_cmos_sensor(0x0990, 0x0002); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC949); 	// MCU_ADDRESS [CAM1_LL_SFFB_SOBEL_FLAT_STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x0002);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC94A); 	// MCU_ADDRESS [CAM1_LL_SFFB_SOBEL_SHARP_START]
	MT9T113_write_cmos_sensor(0x0990, 0x00FF); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC94B); 	// MCU_ADDRESS [CAM1_LL_SFFB_SOBEL_SHARP_STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x00FF); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC906); 	// MCU_ADDRESS [CAM1_LL_DM_EDGE_TH_START]
	MT9T113_write_cmos_sensor(0x0990, 0x0006); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC907); 	// MCU_ADDRESS [CAM1_LL_DM_EDGE_TH_STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x0028); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC02); 	// MCU_ADDRESS [LL_MODE]
	MT9T113_write_cmos_sensor(0x0990, 0x0005); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC908); 	// MCU_ADDRESS [CAM1_LL_AP_KNEE_START]
	MT9T113_write_cmos_sensor(0x0990, 0x0006); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC909); 	// MCU_ADDRESS [CAM1_LL_AP_KNEE_STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x0028); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC90A); 	// MCU_ADDRESS [CAM1_LL_AP_MANTISSA_START]
	MT9T113_write_cmos_sensor(0x0990, 0x0007); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x326C, 0x0F0A); 	// APERTURE_PARAMETERS_2D
	MT9T113_write_cmos_sensor(0x098E, 0xC94C); 	// MCU_ADDRESS [CAM1_LL_DELTA_GAIN]
	MT9T113_write_cmos_sensor(0x0990, 0x0003); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC94E); 	// MCU_ADDRESS [CAM1_LL_DELTA_THRESHOLD_START]
	MT9T113_write_cmos_sensor(0x0990, 0x003C); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xC94F); 	// MCU_ADDRESS [CAM1_LL_DELTA_THRESHOLD_STOP]
	MT9T113_write_cmos_sensor(0x0990, 0x0064); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xE876); 	// MCU_ADDRESS [PRI_A_CONFIG_LL_END_SATURATION]
	MT9T113_write_cmos_sensor(0x0990, 0x0080); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xE877); 	// MCU_ADDRESS [PRI_A_CONFIG_LL_END_SATURATION]
	MT9T113_write_cmos_sensor(0x0990, 0x0070); 	// MCU_DATA_0
	//gamma
	MT9T113_write_cmos_sensor(0x098E, 0x3C42); 	// MCU_ADDRESS [LL_START_GAMMA_FTB]
	MT9T113_write_cmos_sensor(0x0990, 0x0000); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x3C44); 	// MCU_ADDRESS [LL_STOP_GAMMA_FTB]
	MT9T113_write_cmos_sensor(0x0990, 0x0000); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4912); 	// MCU_ADDRESS [CAM1_LL_START_GAMMA_BM]
	MT9T113_write_cmos_sensor(0x0990, 0x0000); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4914); 	// MCU_ADDRESS [CAM1_LL_MID_GAMMA_BM]
	MT9T113_write_cmos_sensor(0x0990, 0x0001); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x4916); 	// MCU_ADDRESS [CAM1_LL_STOP_GAMMA_BM]
	MT9T113_write_cmos_sensor(0x0990, 0x0037); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC09); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_0]
	MT9T113_write_cmos_sensor(0x0990, 0x0000); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC0A); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_1]
	MT9T113_write_cmos_sensor(0x0990, 0x0011); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC0B); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_2]
	MT9T113_write_cmos_sensor(0x0990, 0x0023); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC0C); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_3]
	MT9T113_write_cmos_sensor(0x0990, 0x003F); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC0D); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_4]
	MT9T113_write_cmos_sensor(0x0990, 0x0067); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC0E); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_5]
	MT9T113_write_cmos_sensor(0x0990, 0x0085); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC0F); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_6]
	MT9T113_write_cmos_sensor(0x0990, 0x009B); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC10); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_7]
	MT9T113_write_cmos_sensor(0x0990, 0x00AD); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC11); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_8]
	MT9T113_write_cmos_sensor(0x0990, 0x00BB); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC12); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_9]
	MT9T113_write_cmos_sensor(0x0990, 0x00C7); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC13); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_10]
	MT9T113_write_cmos_sensor(0x0990, 0x00D1); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC14); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_11]
	MT9T113_write_cmos_sensor(0x0990, 0x00DA); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC15); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_12]
	MT9T113_write_cmos_sensor(0x0990, 0x00E1); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC16); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_13]
	MT9T113_write_cmos_sensor(0x0990, 0x00E8); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC17); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_14]
	MT9T113_write_cmos_sensor(0x0990, 0x00EE); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC18); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_15]
	MT9T113_write_cmos_sensor(0x0990, 0x00F3); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC19); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_16]
	MT9T113_write_cmos_sensor(0x0990, 0x00F7); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC1A); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_17]
	MT9T113_write_cmos_sensor(0x0990, 0x00FB); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC1B); 	// MCU_ADDRESS [LL_GAMMA_CONTRAST_CURVE_18]
	MT9T113_write_cmos_sensor(0x0990, 0x00FF); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC1C); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_0]
	MT9T113_write_cmos_sensor(0x0990, 0x0000); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC1D); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_1]
	MT9T113_write_cmos_sensor(0x0990, 0x0011); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC1E); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_2]
	MT9T113_write_cmos_sensor(0x0990, 0x0023); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC1F); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_3]
	MT9T113_write_cmos_sensor(0x0990, 0x003F); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC20); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_4]
	MT9T113_write_cmos_sensor(0x0990, 0x0067); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC21); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_5]
	MT9T113_write_cmos_sensor(0x0990, 0x0085); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC22); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_6]
	MT9T113_write_cmos_sensor(0x0990, 0x009B); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC23); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_7]
	MT9T113_write_cmos_sensor(0x0990, 0x00AD); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC24); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_8]
	MT9T113_write_cmos_sensor(0x0990, 0x00BB); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC25); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_9]
	MT9T113_write_cmos_sensor(0x0990, 0x00C7); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC26); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_10]
	MT9T113_write_cmos_sensor(0x0990, 0x00D1); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC27); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_11]
	MT9T113_write_cmos_sensor(0x0990, 0x00DA); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC28); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_12]
	MT9T113_write_cmos_sensor(0x0990, 0x00E1); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC29); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_13]
	MT9T113_write_cmos_sensor(0x0990, 0x00E8); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC2A); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_14]
	MT9T113_write_cmos_sensor(0x0990, 0x00EE); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC2B); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_15]
	MT9T113_write_cmos_sensor(0x0990, 0x00F3); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC2C); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_16]
	MT9T113_write_cmos_sensor(0x0990, 0x00F7); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC2D); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_17]
	MT9T113_write_cmos_sensor(0x0990, 0x00FB); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC2E); 	// MCU_ADDRESS [LL_GAMMA_NEUTRAL_CURVE_18]
	MT9T113_write_cmos_sensor(0x0990, 0x00FF); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC2F); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_0]
	MT9T113_write_cmos_sensor(0x0990, 0x0000); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC30); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_1]
	MT9T113_write_cmos_sensor(0x0990, 0x0017); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC31); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_2]
	MT9T113_write_cmos_sensor(0x0990, 0x0020); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC32); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_3]
	MT9T113_write_cmos_sensor(0x0990, 0x0032); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC33); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_4]
	MT9T113_write_cmos_sensor(0x0990, 0x005A); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC34); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_5]
	MT9T113_write_cmos_sensor(0x0990, 0x0078); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC35); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_6]
	MT9T113_write_cmos_sensor(0x0990, 0x0089); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC36); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_7]
	MT9T113_write_cmos_sensor(0x0990, 0x0098); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC37); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_8]
	MT9T113_write_cmos_sensor(0x0990, 0x00A6); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC38); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_9]
	MT9T113_write_cmos_sensor(0x0990, 0x00B4); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC39); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_10]
	MT9T113_write_cmos_sensor(0x0990, 0x00C3); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC3A); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_11]
	MT9T113_write_cmos_sensor(0x0990, 0x00CB); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC3B); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_12]
	MT9T113_write_cmos_sensor(0x0990, 0x00D0); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC3C); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_13]
	MT9T113_write_cmos_sensor(0x0990, 0x00D4); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC3D); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_14]
	MT9T113_write_cmos_sensor(0x0990, 0x00DC); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC3E); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_15]
	MT9T113_write_cmos_sensor(0x0990, 0x00E4); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC3F); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_16]
	MT9T113_write_cmos_sensor(0x0990, 0x00EA); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC40); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_17]
	MT9T113_write_cmos_sensor(0x0990, 0x00F5); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xBC41); 	// MCU_ADDRESS [LL_GAMMA_NRCURVE_18]
	MT9T113_write_cmos_sensor(0x0990, 0x00FF); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x3C42); 	// MCU_ADDRESS [LL_START_GAMMA_FTB]
	MT9T113_write_cmos_sensor(0x0990, 0x0032); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x3C44); 	// MCU_ADDRESS [LL_STOP_GAMMA_FTB]
	MT9T113_write_cmos_sensor(0x0990, 0x0000); 	// MCU_DATA_0
	
	//Step8-Features
	MT9T113_write_cmos_sensor(0x098E, 0x8002);	// MCU_ADDRESS [MON_MODE]
	MT9T113_write_cmos_sensor(0x0990, 0x0000);	// MCU_DATA_0
	
	MT9T113_write_cmos_sensor(0x098E, 0x488B); 	// MCU_ADDRESS [CAM1_AWB_CCM_RL_1]
	MT9T113_write_cmos_sensor(0x0990, 0x003F); 	// MCU_DATA_0
	MT9T113_write_cmos_sensor( 0x098E, 0x4897); 	// MCU_ADDRESS [CAM1_AWB_CCM_RL_7]
	MT9T113_write_cmos_sensor(0x0990, 0x0032); 	// MCU_DATA_0

	MT9T113_write_cmos_sensor(0x098E, 0xE854); 	// MCU_ADDRESS [PRI_A_CONFIG_AWB_K_R_R]
	MT9T113_write_cmos_sensor(0x0990, 0x0077); 	// MCU_DATA_0	//Optimized
	//gamma
	MT9T113_write_cmos_sensor(0x098E, 0xA807); 	
	MT9T113_write_cmos_sensor(0x0990, 0x0013); 	

	//50Hz
	MT9T113_write_cmos_sensor(0x098E, 0x2003);	// MCU_ADDRESS [FD_ALGO]
	MT9T113_write_cmos_sensor(0x0990, 0x0002);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0xA005);	// MCU_ADDRESS [FD_FDPERIOD_SELECT]
	MT9T113_write_cmos_sensor(0x0990, 0x0001);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x98E, 0x8400);	//Refresh Sequencer Mode
	MT9T113_write_cmos_sensor(0x990, 0x06); //		= 6
	tmp =0;
	while(tmp<50)
	{
		mdelay(10);//DELAY=100
		MT9T113_write_cmos_sensor(0x98E, 0x8400);	//Refresh Sequencer Mode
		if(0x00==MT9T113_read_cmos_sensor(0x990))
		{
			 break;
		}
		tmp+=1;
	}
	//STATE= Detect Master Clock, 1
	// K46A_REV03_PATCH01_REV1
	MT9T113_write_cmos_sensor(0x0982, 0x0000);	// ACCESS_CTL_STAT
	MT9T113_write_cmos_sensor(0x098A, 0x0A80);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x3C3C);
	MT9T113_write_cmos_sensor(0x0992, 0xCE05);
	MT9T113_write_cmos_sensor(0x0994, 0x1F1F);
	MT9T113_write_cmos_sensor(0x0996, 0x0204);
	MT9T113_write_cmos_sensor(0x0998, 0x0CCC);
	MT9T113_write_cmos_sensor(0x099A, 0x33D4);
	MT9T113_write_cmos_sensor(0x099C, 0x30ED);
	MT9T113_write_cmos_sensor(0x099E, 0x00FC);
	MT9T113_write_cmos_sensor(0x098A, 0x0A90);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x0590);
	MT9T113_write_cmos_sensor(0x0992, 0xBDA8);
	MT9T113_write_cmos_sensor(0x0994, 0x93CE);
	MT9T113_write_cmos_sensor(0x0996, 0x051F);
	MT9T113_write_cmos_sensor(0x0998, 0x1F02);
	MT9T113_write_cmos_sensor(0x099A, 0x0110);
	MT9T113_write_cmos_sensor(0x099C, 0xCC33);
	MT9T113_write_cmos_sensor(0x099E, 0xD830);
	MT9T113_write_cmos_sensor(0x098A, 0x0AA0);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0xED02);
	MT9T113_write_cmos_sensor(0x0992, 0xCC05);
	MT9T113_write_cmos_sensor(0x0994, 0xB8ED);
	MT9T113_write_cmos_sensor(0x0996, 0x00C6);
	MT9T113_write_cmos_sensor(0x0998, 0x06BD);
	MT9T113_write_cmos_sensor(0x099A, 0xA8B1);
	MT9T113_write_cmos_sensor(0x099C, 0xCE05);
	MT9T113_write_cmos_sensor(0x099E, 0x1F1F);
	MT9T113_write_cmos_sensor(0x098A, 0x0AB0);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x0208);
	MT9T113_write_cmos_sensor(0x0992, 0x0CCC);
	MT9T113_write_cmos_sensor(0x0994, 0x33D6);
	MT9T113_write_cmos_sensor(0x0996, 0x30ED);
	MT9T113_write_cmos_sensor(0x0998, 0x00FC);
	MT9T113_write_cmos_sensor(0x099A, 0x0592);
	MT9T113_write_cmos_sensor(0x099C, 0xBDA8);
	MT9T113_write_cmos_sensor(0x099E, 0x93CC);
	MT9T113_write_cmos_sensor(0x098A, 0x0AC0);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x33F4);
	MT9T113_write_cmos_sensor(0x0992, 0x30ED);
	MT9T113_write_cmos_sensor(0x0994, 0x02CC);
	MT9T113_write_cmos_sensor(0x0996, 0xFFE9);
	MT9T113_write_cmos_sensor(0x0998, 0xED00);
	MT9T113_write_cmos_sensor(0x099A, 0xFC05);
	MT9T113_write_cmos_sensor(0x099C, 0x94C4);
	MT9T113_write_cmos_sensor(0x099E, 0x164F);
	MT9T113_write_cmos_sensor(0x098A, 0x0AD0);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0xBDA9);
	MT9T113_write_cmos_sensor(0x0992, 0x0ACE);
	MT9T113_write_cmos_sensor(0x0994, 0x051F);
	MT9T113_write_cmos_sensor(0x0996, 0x1F02);
	MT9T113_write_cmos_sensor(0x0998, 0x020A);
	MT9T113_write_cmos_sensor(0x099A, 0xCC32);
	MT9T113_write_cmos_sensor(0x099C, 0x1030);
	MT9T113_write_cmos_sensor(0x099E, 0xED00);
	MT9T113_write_cmos_sensor(0x098A, 0x0AE0);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x4FBD);
	MT9T113_write_cmos_sensor(0x0992, 0xA8E4);
	MT9T113_write_cmos_sensor(0x0994, 0x3838);
	MT9T113_write_cmos_sensor(0x0996, 0x393C);
	MT9T113_write_cmos_sensor(0x0998, 0x3CFC);
	MT9T113_write_cmos_sensor(0x099A, 0x0322);
	MT9T113_write_cmos_sensor(0x099C, 0xB303);
	MT9T113_write_cmos_sensor(0x099E, 0x2030);
	MT9T113_write_cmos_sensor(0x098A, 0x0AF0);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0xED02);
	MT9T113_write_cmos_sensor(0x0992, 0xCE03);
	MT9T113_write_cmos_sensor(0x0994, 0x141F);
	MT9T113_write_cmos_sensor(0x0996, 0x0408);
	MT9T113_write_cmos_sensor(0x0998, 0x3ECE);
	MT9T113_write_cmos_sensor(0x099A, 0x0314);
	MT9T113_write_cmos_sensor(0x099C, 0x1F0B);
	MT9T113_write_cmos_sensor(0x099E, 0x0134);
	MT9T113_write_cmos_sensor(0x098A, 0x0B00);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x30EC);
	MT9T113_write_cmos_sensor(0x0992, 0x0227);
	MT9T113_write_cmos_sensor(0x0994, 0x2F83);
	MT9T113_write_cmos_sensor(0x0996, 0x0000);
	MT9T113_write_cmos_sensor(0x0998, 0x2C18);
	MT9T113_write_cmos_sensor(0x099A, 0xF603);
	MT9T113_write_cmos_sensor(0x099C, 0x244F);
	MT9T113_write_cmos_sensor(0x099E, 0xED00);
	MT9T113_write_cmos_sensor(0x098A, 0x0B10);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0xFC03);
	MT9T113_write_cmos_sensor(0x0992, 0x20A3);
	MT9T113_write_cmos_sensor(0x0994, 0x00B3);
	MT9T113_write_cmos_sensor(0x0996, 0x0322);
	MT9T113_write_cmos_sensor(0x0998, 0x241A);
	MT9T113_write_cmos_sensor(0x099A, 0xFC03);
	MT9T113_write_cmos_sensor(0x099C, 0x22FD);
	MT9T113_write_cmos_sensor(0x099E, 0x0320);
	MT9T113_write_cmos_sensor(0x098A, 0x0B20);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x2012);
	MT9T113_write_cmos_sensor(0x0992, 0xF603);
	MT9T113_write_cmos_sensor(0x0994, 0x244F);
	MT9T113_write_cmos_sensor(0x0996, 0xF303);
	MT9T113_write_cmos_sensor(0x0998, 0x20B3);
	MT9T113_write_cmos_sensor(0x099A, 0x0322);
	MT9T113_write_cmos_sensor(0x099C, 0x2306);
	MT9T113_write_cmos_sensor(0x099E, 0xFC03);
	MT9T113_write_cmos_sensor(0x098A, 0x0B30);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x22FD);
	MT9T113_write_cmos_sensor(0x0992, 0x0320);
	MT9T113_write_cmos_sensor(0x0994, 0xBD7D);
	MT9T113_write_cmos_sensor(0x0996, 0x9038);
	MT9T113_write_cmos_sensor(0x0998, 0x3839);
	MT9T113_write_cmos_sensor(0x099A, 0x3C3C);
	MT9T113_write_cmos_sensor(0x099C, 0xFC07);
	MT9T113_write_cmos_sensor(0x099E, 0x4327);
	MT9T113_write_cmos_sensor(0x098A, 0x0B40);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x5FDE);
	MT9T113_write_cmos_sensor(0x0992, 0x431F);
	MT9T113_write_cmos_sensor(0x0994, 0xB410);
	MT9T113_write_cmos_sensor(0x0996, 0x563C);
	MT9T113_write_cmos_sensor(0x0998, 0xFC07);
	MT9T113_write_cmos_sensor(0x099A, 0x4130);
	MT9T113_write_cmos_sensor(0x099C, 0xED00);
	MT9T113_write_cmos_sensor(0x099E, 0x3CCC);
	MT9T113_write_cmos_sensor(0x098A, 0x0B50);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x0008);
	MT9T113_write_cmos_sensor(0x0992, 0x30ED);
	MT9T113_write_cmos_sensor(0x0994, 0x00FC);
	MT9T113_write_cmos_sensor(0x0996, 0x0743);
	MT9T113_write_cmos_sensor(0x0998, 0xBDAA);
	MT9T113_write_cmos_sensor(0x099A, 0x7C38);
	MT9T113_write_cmos_sensor(0x099C, 0x38BD);
	MT9T113_write_cmos_sensor(0x099E, 0xE9E4);
	MT9T113_write_cmos_sensor(0x098A, 0x0B60);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x30ED);
	MT9T113_write_cmos_sensor(0x0992, 0x02CC);
	MT9T113_write_cmos_sensor(0x0994, 0x0064);
	MT9T113_write_cmos_sensor(0x0996, 0xED00);
	MT9T113_write_cmos_sensor(0x0998, 0xCC01);
	MT9T113_write_cmos_sensor(0x099A, 0x00BD);
	MT9T113_write_cmos_sensor(0x099C, 0xAA7C);
	MT9T113_write_cmos_sensor(0x099E, 0xFD03);
	MT9T113_write_cmos_sensor(0x098A, 0x0B70);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x103C);
	MT9T113_write_cmos_sensor(0x0992, 0xFC07);
	MT9T113_write_cmos_sensor(0x0994, 0x4530);
	MT9T113_write_cmos_sensor(0x0996, 0xED00);
	MT9T113_write_cmos_sensor(0x0998, 0x3CCC);
	MT9T113_write_cmos_sensor(0x099A, 0x0008);
	MT9T113_write_cmos_sensor(0x099C, 0x30ED);
	MT9T113_write_cmos_sensor(0x099E, 0x00FC);
	MT9T113_write_cmos_sensor(0x098A, 0x0B80);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x0743);
	MT9T113_write_cmos_sensor(0x0992, 0xBDAA);
	MT9T113_write_cmos_sensor(0x0994, 0x7C38);
	MT9T113_write_cmos_sensor(0x0996, 0x38BD);
	MT9T113_write_cmos_sensor(0x0998, 0xE9E4);
	MT9T113_write_cmos_sensor(0x099A, 0x30ED);
	MT9T113_write_cmos_sensor(0x099C, 0x02CC);
	MT9T113_write_cmos_sensor(0x099E, 0x0064);
	MT9T113_write_cmos_sensor(0x098A, 0x0B90);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0xED00);
	MT9T113_write_cmos_sensor(0x0992, 0xCC01);
	MT9T113_write_cmos_sensor(0x0994, 0x00BD);
	MT9T113_write_cmos_sensor(0x0996, 0xAA7C);
	MT9T113_write_cmos_sensor(0x0998, 0xFD03);
	MT9T113_write_cmos_sensor(0x099A, 0x1220);
	MT9T113_write_cmos_sensor(0x099C, 0x03BD);
	MT9T113_write_cmos_sensor(0x099E, 0x7993);
	MT9T113_write_cmos_sensor(0x098A, 0x0BA0);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x3838);
	MT9T113_write_cmos_sensor(0x0992, 0x390F);
	MT9T113_write_cmos_sensor(0x0994, 0xF601);
	MT9T113_write_cmos_sensor(0x0996, 0x05C1);
	MT9T113_write_cmos_sensor(0x0998, 0x0326);
	MT9T113_write_cmos_sensor(0x099A, 0x14F6);
	MT9T113_write_cmos_sensor(0x099C, 0x0106);
	MT9T113_write_cmos_sensor(0x099E, 0xC106);
	MT9T113_write_cmos_sensor(0x098A, 0x0BB0);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x260D);
	MT9T113_write_cmos_sensor(0x0992, 0xF630);
	MT9T113_write_cmos_sensor(0x0994, 0x4DC4);
	MT9T113_write_cmos_sensor(0x0996, 0xF0CA);
	MT9T113_write_cmos_sensor(0x0998, 0x08F7);
	MT9T113_write_cmos_sensor(0x099A, 0x304D);
	MT9T113_write_cmos_sensor(0x099C, 0xBD0B);
	MT9T113_write_cmos_sensor(0x099E, 0xC10E);
	MT9T113_write_cmos_sensor(0x098A, 0x0BC0);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x39F6);
	MT9T113_write_cmos_sensor(0x0992, 0x304D);
	MT9T113_write_cmos_sensor(0x0994, 0xC4F0);
	MT9T113_write_cmos_sensor(0x0996, 0xCA09);
	MT9T113_write_cmos_sensor(0x0998, 0xF730);
	MT9T113_write_cmos_sensor(0x099A, 0x4DDE);
	MT9T113_write_cmos_sensor(0x099C, 0xF218);
	MT9T113_write_cmos_sensor(0x099E, 0xCE0A);
	MT9T113_write_cmos_sensor(0x098A, 0x0BD0);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x00CC);
	MT9T113_write_cmos_sensor(0x0992, 0x001D);
	MT9T113_write_cmos_sensor(0x0994, 0xBDB5);
	MT9T113_write_cmos_sensor(0x0996, 0x31DE);
	MT9T113_write_cmos_sensor(0x0998, 0xA818);
	MT9T113_write_cmos_sensor(0x099A, 0xCE0A);
	MT9T113_write_cmos_sensor(0x099C, 0x1ECC);
	MT9T113_write_cmos_sensor(0x099E, 0x001D);
	MT9T113_write_cmos_sensor(0x098A, 0x0BE0);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0xBDB5);
	MT9T113_write_cmos_sensor(0x0992, 0x31DE);
	MT9T113_write_cmos_sensor(0x0994, 0xA618);
	MT9T113_write_cmos_sensor(0x0996, 0xCE0A);
	MT9T113_write_cmos_sensor(0x0998, 0x3CCC);
	MT9T113_write_cmos_sensor(0x099A, 0x0013);
	MT9T113_write_cmos_sensor(0x099C, 0xBDB5);
	MT9T113_write_cmos_sensor(0x099E, 0x31CC);
	MT9T113_write_cmos_sensor(0x098A, 0x0BF0);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x0A80);
	MT9T113_write_cmos_sensor(0x0992, 0xFD0A);
	MT9T113_write_cmos_sensor(0x0994, 0x0ECC);
	MT9T113_write_cmos_sensor(0x0996, 0x0AE7);
	MT9T113_write_cmos_sensor(0x0998, 0xFD0A);
	MT9T113_write_cmos_sensor(0x099A, 0x30CC);
	MT9T113_write_cmos_sensor(0x099C, 0x0B3A);
	MT9T113_write_cmos_sensor(0x099E, 0xFD0A);
	MT9T113_write_cmos_sensor(0x098A, 0x0C00);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x4CCC);
	MT9T113_write_cmos_sensor(0x0992, 0x0A00);
	MT9T113_write_cmos_sensor(0x0994, 0xDDF2);
	MT9T113_write_cmos_sensor(0x0996, 0xCC0A);
	MT9T113_write_cmos_sensor(0x0998, 0x1EDD);
	MT9T113_write_cmos_sensor(0x099A, 0xA8CC);
	MT9T113_write_cmos_sensor(0x099C, 0x0A3C);
	MT9T113_write_cmos_sensor(0x099E, 0xDDA6);
	MT9T113_write_cmos_sensor(0x098A, 0x0C10);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0xC601);
	MT9T113_write_cmos_sensor(0x0992, 0xF701);
	MT9T113_write_cmos_sensor(0x0994, 0x0CF7);
	MT9T113_write_cmos_sensor(0x0996, 0x010D);
	MT9T113_write_cmos_sensor(0x098A, 0x8C18);	// PHYSICAL_ADDR_ACCESS
	MT9T113_write_cmos_sensor(0x0990, 0x0039);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x0012);	// MCU_ADDRESS [MON_ADDR]
	MT9T113_write_cmos_sensor(0x0990, 0x0BA3);	// MCU_DATA_0
	MT9T113_write_cmos_sensor(0x098E, 0x0003);	// MCU_ADDRESS [MON_ALGO]
	MT9T113_write_cmos_sensor(0x0990, 0x0004);	// MCU_DATA_0
	//POLL_FIELD=MON_RAM_PATCH_ID,==0,DELAY=10,TIMEOUT=100		   // wait for the patch to complete initialization 
	mdelay(50);//DELAY=100
    tmp = 50; 
    do {
        mdelay(2);//DELAY=100
        MT9T113_write_cmos_sensor(0x0018, 0x002A);	// STANDBY_CONTROL_AND_STATUS
        if(0x2a == MT9T113_read_cmos_sensor(0x0018)) {
            printk("[MT9T113]Exit stable done \n"); 
            break;
        }
    }while (tmp-- > 0); 
    
    //mdelay(50);//DELAY=100

	MT9T113_sensor.preview_pclk = 520;

} /* MT9T113_YUV_sensor_initial_setting */
static void MT9T113_CAP_setting(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	MT9T113_sensor.capture_pclk = 520;
	MT9T113_op_state.is_PV_mode = KAL_FALSE;

	return;	

} /* MT9T113_CAP_setting */

static void MT9T113_set_AE_mode(kal_bool AE_enable)
{
	kal_uint8 temp_AE_reg = 0;
	return;
	if (AE_enable == KAL_TRUE)
	{
		// turn on AEC/AGC
	}
	else
	{
		// turn off AEC/AGC

	}
}


static void MT9T113_set_AWB_mode(kal_bool AWB_enable)
{
	kal_uint8 temp_AWB_reg = 0;

	return ;

	if (AWB_enable == KAL_TRUE)
	{
		//enable Auto WB
		
	}
	else
	{
		//turn off AWB

	}
}

static u32 mt9t113_calc_mipiclk(void)
{
       u32 rxpll, val, n, m, bit8div;
       u32 sdiv_inv, mipidiv;
       u32 fclk, mipiclk, mclk = 26000000;
       u8 lut1[4] = {2, 3, 4, 6};
       u8 lut2[4] = {1, 1, 4, 5};

       val = MT9T113_read_cmos_sensor(0x3011); 

       mclk /= (val + 1); 

       /* Calculate fclk */
       val = MT9T113_read_cmos_sensor(0x300E);
       rxpll = val & 0x3F;

      val = MT9T113_read_cmos_sensor(0x300F);
      n = lut1[(val >> 6) & 0x3];
      m = lut1[val & 0x3];
      bit8div = lut2[(val >> 4) & 0x3];
      fclk = (64 - rxpll) * n * bit8div * mclk / m;

      val = MT9T113_read_cmos_sensor(0x3010);
      mipidiv = ((val >> 5) & 1) + 1;
      sdiv_inv = (val & 0xF) * 2;

      if ((val & 0xF) >= 1)
              mipiclk = fclk / sdiv_inv / mipidiv;
      else
              mipiclk = fclk / mipidiv;
     return mipiclk;
}

static void MT9T113_set_QXGA_mode(void)
{
	
	//@@ Capture QXGA Key setting								 							 
	//99 2048 1536												 
	//98 0 0													 														 
	//gg@MT9T113_write_cmos_sensor(0x3012,0x00);		//soft reset, sensor array resolution:QXGA	
	
   /* 
        fclk=(0x40-0x300E[5:0])*N*Bit8Div*MCLK/M
              N=1/1.5/2/3 for ox300F[7:6]=0~3,  M=1/1.5/2/3 for 0x300F[1:0],   Bit8Div=1/1/4/5 for 0x300F[5:4]
         DVP PClk=fclk/ScaleDiv/DVPDiv;  
              ScaleDiv=1 and 0x3010[3:0]*2 for 0x3010[3:0]=0 and other values.
              DVPDiv=1/2/8/16 for 0x3010[7:6]
    */

    //size:2048x1536	
	//gg@MT9T113_write_cmos_sensor(0x3020,0x01);	   //HS[15:8]					 
	//gg@MT9T113_write_cmos_sensor(0x3021,0x1d);	   //HS[7:0]				 
	//gg@MT9T113_write_cmos_sensor(0x3022,0x00);	   //VS[15:8]				 
	//gg@MT9T113_write_cmos_sensor(0x3023,0x0a);	   //VS[7:0]					 
	//gg@MT9T113_write_cmos_sensor(0x3024,0x08);	   //HW[15:8],  HW=2072				 
	//gg@MT9T113_write_cmos_sensor(0x3025,0x18);	   //HW[7:0]				 
	//gg@MT9T113_write_cmos_sensor(0x3026,0x06);	   //VH[15:8],   VH=1548			 
	//gg@MT9T113_write_cmos_sensor(0x3027,0x0c);		//VH[7:0]				 
	//gg@MT9T113_write_cmos_sensor(0x302a,0x06);	   //VTS[15:8]:1568				 
	//gg@MT9T113_write_cmos_sensor(0x302b,0x20);		//VTS[7:0]				 
	//gg@MT9T113_write_cmos_sensor(0x3075,0x44);		//Vsync pulse width and start point				 
	//gg@MT9T113_write_cmos_sensor(0x300d,0x00);		//pclk output control				 
	//gg@MT9T113_write_cmos_sensor(0x30d7,0x10);		//reserve			 
	//gg@MT9T113_write_cmos_sensor(0x3069,0x44);		//BLC control				 
	//gg@MT9T113_write_cmos_sensor(0x303e,0x01);		//AVH[11:8],  AVH=384(384*4=1536)			 
	//gg@MT9T113_write_cmos_sensor(0x303f,0x80);		//AVH[7:0]	
	//gg@//if(MT9T113_Night_mode == KAL_TRUE)
	//gg@//MT9T113_write_cmos_sensor(0x3302,0xeb);      //turn off gamma
	//gg@//else
	//gg@MT9T113_write_cmos_sensor(0x3302,0xef);		//UV_adj, AWB, GMA				 
	//gg@MT9T113_write_cmos_sensor(0x335f,0x68);		//SIZE_IN_MISC,Vsize_in[10:8],Hsize_in[11:8]	for zoom_out			 
	//gg@MT9T113_write_cmos_sensor(0x3360,0x18);		//Hsize_in[7:0],Hsize=2072			 
	//gg@MT9T113_write_cmos_sensor(0x3361,0x0C);		//Vsize_in[7:0],Vsize=1548			 
	//gg@MT9T113_write_cmos_sensor(0x3362,0x68);		//SIZE_OUT_MISC,Vsize_in[10:8],Hsize_in[11:8]	for zoom_out		 
	//gg@MT9T113_write_cmos_sensor(0x3363,0x08);		//Hsize_out[7:0],Hsize=2056				 
	//gg@MT9T113_write_cmos_sensor(0x3364,0x04);		//Vsize_out[7:0],Vsize=1540			 
	//gg@															 
	//gg@MT9T113_write_cmos_sensor(0x3366,0x10);		//reserve				 
	//gg@															 
	//gg@MT9T113_write_cmos_sensor(0x3403,0x42);		//bit[7:4]:x start=4,  bit[3:0]:y start=2				 
	//gg@MT9T113_write_cmos_sensor(0x3088,0x08);		// isp output size, x_out[15:8]	,  x_out=2048			 
	//gg@MT9T113_write_cmos_sensor(0x3089,0x00);		//x_out[7:0]				 
	//gg@MT9T113_write_cmos_sensor(0x308a,0x06);		//y_out[15:8],   y_out=1536			 
	//gg@MT9T113_write_cmos_sensor(0x308b,0x00);		//y_out[7:0]				 
	if(MT9T113_Night_mode == KAL_TRUE)        //add dummy to decrease low light frame for low light noise pass
	{//gg@MT9T113_write_cmos_sensor(0x302a,0x07);
	//gg@MT9T113_write_cmos_sensor(0x302b,0xe4);
	}
	else
	{//gg@MT9T113_write_cmos_sensor(0x302a,0x06);
	 //gg@MT9T113_write_cmos_sensor(0x302b,0x20);
	 }

	 MT9T113_set_AWB_mode(KAL_FALSE); 
        MT9T113_set_AE_mode(KAL_FALSE);

  //MT9T113_write_cmos_sensor(0x3013,0xf2);	 // ; disable AEC/AGC
	
}
static void MT9T113_Set_Video_Frame_Rate(kal_uint32 frame_rate)
{
	kal_uint32 line_length;
	kal_uint32 frame_rate_50Hz,frame_rate_60Hz;
	kal_uint32 base_shutter_50Hz,base_shutter_60Hz;

	line_length = SENSOR_CORE_PCLK*10/frame_rate/(MT9T113_PV_PERIOD_LINE_NUMS + MT9T113_sensor.pv_dummy_lines);
	frame_rate_50Hz		= 50*2*10/frame_rate;
	frame_rate_60Hz		= 60*2*10/frame_rate;


	base_shutter_50Hz = (frame_rate*MT9T113_PV_PERIOD_LINE_NUMS/50+5)/2/10;;
	base_shutter_60Hz = (frame_rate*MT9T113_PV_PERIOD_LINE_NUMS/60+5)/2/10;;
	//gg@MT9T113_write_cmos_sensor(0x98E, 0x481A); 				//fd_period_50Hz (A)
	//gg@MT9T113_write_cmos_sensor(0x990, base_shutter_50Hz); 	//		
	//gg@MT9T113_write_cmos_sensor(0x98E, 0x481C); 				//fd_period_60Hz (A)
	//gg@MT9T113_write_cmos_sensor(0x990, base_shutter_60Hz); 	//	

	//gg@MT9T113_write_cmos_sensor(0x098E, 0x8400);			// MCU_ADDRESS [SEQ_CMD]
	//gg@MT9T113_write_cmos_sensor(0x0990, 0x0006);			// MCU_DATA_0
	{
		kal_uint16 temp=0;
		while(temp <60)
		{
			MT9T113_write_cmos_sensor(0x098E, 0x8400);	  // MCU_DATA_0
			if(0==MT9T113_read_cmos_sensor(0x990))
			{
			 break;
			}
			mdelay(4);//DELAY=100
			temp+=1;
		}
	}

#if 1
	//mdelay(400);

	//gg@MT9T113_write_cmos_sensor(0x098E, 0x6815);			// MCU_ADDRESS [PRI_A_CONFIG_FD_MAX_FDZONE_50HZ]
	//gg@MT9T113_write_cmos_sensor(0x0990, frame_rate_50Hz);	// MCU_DATA_0
	//gg@MT9T113_write_cmos_sensor(0x098E, 0x6817);			// MCU_ADDRESS [PRI_A_CONFIG_FD_MAX_FDZONE_60HZ]
	//gg@MT9T113_write_cmos_sensor(0x0990, frame_rate_60Hz);	// MCU_DATA_0
  //gg@
	//gg@MT9T113_write_cmos_sensor(0x98E, 0x480A);		//Line Length (A)
	//gg@MT9T113_write_cmos_sensor(0x990, line_length);	//		
  //gg@
	//gg@MT9T113_write_cmos_sensor(0x098E, 0x8400);			// MCU_ADDRESS [SEQ_CMD]
	//gg@MT9T113_write_cmos_sensor(0x0990, 0x0006);			// MCU_DATA_0
	{
		kal_uint16 temp=0;
		while(temp <60)
		{
			MT9T113_write_cmos_sensor(0x098E, 0x8400);	  // MCU_DATA_0
			if(0==MT9T113_read_cmos_sensor(0x990))
			{
			 break;
			}
			mdelay(20);//DELAY=100
			temp+=1;
		}
	}
#endif	

	MT9T113_sensor.video_frame_rate = frame_rate;

	return;

}


/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
UINT32 MT9T113Open(void)
{
	volatile signed char i;
	kal_uint32 sensor_id=0;
	kal_uint8 temp_sccb_addr = 0;
    //s_move to here from CISModulePowerOn()
    for (i=0; i<(sizeof(MT9T113_sccb_addr)/sizeof(MT9T113_sccb_addr[0])); i++)
	{
		/* 0xFF is not a valid sensor write ID. */
		
		MT9T113_sensor.sccb_write_id	= MT9T113_sccb_addr[i];
		MT9T113_sensor.sccb_read_id		= (MT9T113_sensor.sccb_write_id | 1);

		//4 <9>software reset sensor and wait (to sensor)
		//Reset
		MT9T113_write_cmos_sensor(0x0018, 0x4129 ); // STANDBY_CONTROL_AND_STATUS
		mdelay(1);//DELAY=1
		MT9T113_write_cmos_sensor(0x0018, 0x4029 ); // STANDBY_CONTROL_AND_STATUS
		mdelay(5);//DELAY=5

		sensor_id = MT9T113_read_cmos_sensor(0x0000);
		if (sensor_id == MT9T113_SENSOR_ID)
		{
			/* Swap the correct i2c address to first one, it will speed up next time read sensor ID */
			temp_sccb_addr = MT9T113_sccb_addr[0];
			MT9T113_sccb_addr[0] = MT9T113_sccb_addr[i];
			MT9T113_sccb_addr[i] = temp_sccb_addr;		
			break;
		}
	}

	//  Read sensor ID to adjust I2C is OK?
    if (sensor_id != MT9T113_SENSOR_ID)
	{
	    printk("[MT9T113YUV]:Read Sensor ID fail:0x%x\n", sensor_id);  
		return ERROR_SENSOR_CONNECT_FAIL;
	}
	MT9T113_sensor.pv_shutter = 0x0265;
	MT9T113_sensor.pv_extra_shutter = 0;

    /*9. Apply sensor initail setting*/
	MT9T113_YUV_sensor_initial_setting();
	MT9T113_sensor.preview_pclk = 520;
	first_enter_preview = KAL_TRUE;
     
	return ERROR_NONE;
}	/* MT9T113Open() */

static kal_uint32 MT9T113_GetSensorID(kal_uint32 *sensorID)

{

   volatile signed char i;
	kal_uint32 sensor_id=0;
	kal_uint8 temp_sccb_addr = 0;
    //s_move to here from CISModulePowerOn()
    
	SENSORDB("[Enter]:MT9T113 Open func ");
    for (i=0; i<(sizeof(MT9T113_sccb_addr)/sizeof(MT9T113_sccb_addr[0])); i++)
	{
		/* 0xFF is not a valid sensor write ID. */
		
		MT9T113_sensor.sccb_write_id	= MT9T113_sccb_addr[i];
		MT9T113_sensor.sccb_read_id		= (MT9T113_sensor.sccb_write_id | 1);

		//4 <9>software reset sensor and wait (to sensor)
		//Reset
		MT9T113_write_cmos_sensor(0x0018, 0x4129 ); // STANDBY_CONTROL_AND_STATUS
		mdelay(1);//DELAY=1
		MT9T113_write_cmos_sensor(0x0018, 0x4029 ); // STANDBY_CONTROL_AND_STATUS
		mdelay(5);//DELAY=5

		sensor_id = MT9T113_read_cmos_sensor(0x0000);
		if (sensor_id == MT9T113_SENSOR_ID)
		{
			/* Swap the correct i2c address to first one, it will speed up next time read sensor ID */
			temp_sccb_addr = MT9T113_sccb_addr[0];
			MT9T113_sccb_addr[0] = MT9T113_sccb_addr[i];
			MT9T113_sccb_addr[i] = temp_sccb_addr;		
			break;
		}
	}
	*sensorID = sensor_id;

	//  Read sensor ID to adjust I2C is OK?
    if (sensor_id != MT9T113_SENSOR_ID)
	{
        *sensorID=0xFFFFFFFF;
	    printk("[MT9T113YUV]:Read Sensor ID fail:0x%x\n", sensor_id);  
		return ERROR_SENSOR_CONNECT_FAIL;
	}
    return ERROR_NONE;    
}   /* MT9T113Open  */

UINT32 MT9T113Close(void)
{
//	CISModulePowerOn(FALSE);
	
	return ERROR_NONE;
}	/* MT9T113Close() */


kal_uint16 MT9T113_write_gain(kal_uint16 gain)
{
//Not support

}

static kal_uint32 MT9T113_read_sensor_gain(void)
{
	kal_uint16 temp_reg = 0;
	kal_uint32 sensor_gain = 0;
return 0;	
	temp_reg = MT9T113_read_cmos_sensor(0x3000);  
	
	sensor_gain = (16 + (temp_reg & 0x0F));
	
	if (temp_reg & 0x10)
	{
		sensor_gain <<= 1;
	}
	if (temp_reg & 0x20)
	{
		sensor_gain <<= 1;
	}
	if (temp_reg & 0x40)
	{
		sensor_gain <<= 1;
	}
	if (temp_reg & 0x80)
	{
		sensor_gain <<= 1;
	}
	
	return sensor_gain;
}  /* MT9T113_read_sensor_gain */




static void MT9T113_set_mirror_flip(kal_uint8 image_mirror)
{
    switch (image_mirror) {
    case IMAGE_NORMAL:
		//[Default Orientation]
		MT9T113_write_cmos_sensor(0x098E, 0x4810);	// MCU_ADDRESS [CAM1_CTX_A_READ_MODE]
		MT9T113_write_cmos_sensor(0x0990, 0x046C);	// MCU_DATA_0
		MT9T113_write_cmos_sensor(0x098E, 0x483D);	// MCU_ADDRESS [CAM1_CTX_B_READ_MODE]
		MT9T113_write_cmos_sensor(0x0990, 0x0024);	// MCU_DATA_0
		MT9T113_write_cmos_sensor(0x098E, 0x8400);	// MCU_ADDRESS [SEQ_CMD]
		MT9T113_write_cmos_sensor(0x0990, 0x0006);	// MCU_DATA_0
        break;
	case IMAGE_H_MIRROR:
		//[Horizontal Mirror]
		MT9T113_write_cmos_sensor(0x098E, 0x4810);	// MCU_ADDRESS [CAM1_CTX_A_READ_MODE]
		MT9T113_write_cmos_sensor(0x0990, 0x046D);	// MCU_DATA_0
		MT9T113_write_cmos_sensor(0x098E, 0x483D);	// MCU_ADDRESS [CAM1_CTX_B_READ_MODE]
		MT9T113_write_cmos_sensor(0x0990, 0x0025);	// MCU_DATA_0
		MT9T113_write_cmos_sensor(0x098E, 0x8400);	// MCU_ADDRESS [SEQ_CMD]
		MT9T113_write_cmos_sensor(0x0990, 0x0006);	// MCU_DATA_0
        break;
	case IMAGE_V_MIRROR:	//Flip Register 0x04[6] and 0x04[4] (FF = 01)
        //[Vertical Flip]
		MT9T113_write_cmos_sensor(0x098E, 0x4810);	// MCU_ADDRESS [CAM1_CTX_A_READ_MODE]
		MT9T113_write_cmos_sensor(0x0990, 0x046E);	// MCU_DATA_0
		MT9T113_write_cmos_sensor(0x098E, 0x483D);	// MCU_ADDRESS [CAM1_CTX_B_READ_MODE]
		MT9T113_write_cmos_sensor(0x0990, 0x0026);	// MCU_DATA_0
		MT9T113_write_cmos_sensor(0x098E, 0x8400);	// MCU_ADDRESS [SEQ_CMD]
		MT9T113_write_cmos_sensor(0x0990, 0x0006);	// MCU_DATA_0
        break;

    case IMAGE_HV_MIRROR:
		//[Flip and Mirror]
		MT9T113_write_cmos_sensor(0x098E, 0x4810);	// MCU_ADDRESS [CAM1_CTX_A_READ_MODE]
		MT9T113_write_cmos_sensor(0x0990, 0x046F);	// MCU_DATA_0
		MT9T113_write_cmos_sensor(0x098E, 0x483D);	// MCU_ADDRESS [CAM1_CTX_B_READ_MODE]
		MT9T113_write_cmos_sensor(0x0990, 0x0027);	// MCU_DATA_0
		MT9T113_write_cmos_sensor(0x098E, 0x8400);	// MCU_ADDRESS [SEQ_CMD]
		MT9T113_write_cmos_sensor(0x0990, 0x0006);	// MCU_DATA_0
        break;

    default:
        ASSERT(0);
    }
}

static kal_bool MT9T113_awb_enable(kal_bool enalbe)
{	 
	kal_uint16 temp_AWB_reg = 0;

	return KAL_TRUE;
}

static kal_bool MT9T113_ae_enable(kal_bool enalbe)
{	 
	kal_uint16 temp_AE_reg = 0;
	
	return KAL_TRUE;
}

UINT32 MT9T113Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{	
	MT9T113_op_state.sensor_cap_state = KAL_FALSE;
	#ifdef MT9T113_DEBUG
	 printk("[MT9T113YUV]:preview\n");  
	#endif
	//MT9T113_PV_setting(image_window, sensor_config_data);
	/* After set exposure line, there should be delay for 2~4 frame time, then enable AEC */
	mdelay(5);
	MT9T113_VEDIO_encode_mode=KAL_FALSE;

	//MT9T113_ae_enable(KAL_TRUE);
	//MT9T113_awb_enable(KAL_TRUE);
																
	MT9T113_sensor.pv_dummy_pixels = 0;
    MT9T113_sensor.pv_dummy_lines = 0;
	
	MT9T113_sensor.preview_pclk = 520;
	MT9T113_op_state.is_PV_mode = KAL_TRUE;
	    printk("[MT9T113YUV]sensor_config_data->SensorImageMirror 0x%x\n", sensor_config_data->SensorImageMirror);  
	//MT9T113_set_dummy(MT9T113_sensor.pv_dummy_pixels, MT9T113_sensor.pv_dummy_lines);

  	//[Go to preview]
  	MT9T113_write_cmos_sensor(0x098E, 0xEC09);	  // MCU_ADDRESS [PRI_B_NUM_OF_FRAMES_RUN]
  	MT9T113_write_cmos_sensor(0x0990, 0x0005);	  // MCU_DATA_0
  	MT9T113_write_cmos_sensor(0x098E, 0x8400);	  // MCU_ADDRESS [SEQ_CMD]
  	MT9T113_write_cmos_sensor(0x0990, 0x0001);	  // MCU_DATA_0
  	{	
		kal_uint16 temp=0;
		while(temp <60)
		{
		  	MT9T113_write_cmos_sensor(0x098E, 0x8401);	  // MCU_DATA_0
			if(3==MT9T113_read_cmos_sensor(0x990))
			{
			 break;
			}
			mdelay(10);//DELAY=20
			temp+=1;
		}
	}
	
	image_window->GrabStartX = MT9T113_PV_GRAB_START_X;
	image_window->GrabStartY = MT9T113_PV_GRAB_START_Y;
	image_window->ExposureWindowWidth = MT9T113_PV_GRAB_WIDTH;
	image_window->ExposureWindowHeight = MT9T113_PV_GRAB_HEIGHT;
    // copy sensor_config_data
	memcpy(&MT9T113SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
  	return ERROR_NONE;
}	/* MT9T113_Preview */

UINT32 MT9T113Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window, MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	volatile kal_uint32 shutter = 0, temp_reg = 0;
	kal_uint32 prev_line_len = 0;
	kal_uint32 cap_line_len = 0;
	
	MT9T113_op_state.sensor_cap_state = KAL_TRUE;	
	MT9T113_ae_enable(KAL_FALSE);
	MT9T113_awb_enable(KAL_FALSE);	
	shutter = MT9T113_read_shutter();
	MT9T113_sensor.pv_sensor_gain = MT9T113_read_sensor_gain();
	
	
	if ((image_window->ImageTargetWidth <= MT9T113_IMAGE_SENSOR_PV_WIDTH)
		&& (image_window->ImageTargetHeight <= MT9T113_IMAGE_SENSOR_PV_HEIGHT))
	{		/* Capture Size Less than PV Size */	
			if (zoom_factor >= 3) // DZ >= 3x
			{
			#ifdef MT9T113_DEBUG			
			    printk("[MT9T113YUV]:capture preview size zoom >=3 \n"); 			
			#endif
				MT9T113_sensor.cap_dummy_pixels =0;//MT9T113_PV_PERIOD_PIXEL_NUMS/4;
				MT9T113_sensor.cap_dummy_lines = 0;
			}
			else if (zoom_factor >= 2) // DZ >= 2x
			{
            #ifdef MT9T113_DEBUG
			    printk("[MT9T113YUV]:capture preview size >=2 \n");
			#endif			
				MT9T113_sensor.cap_dummy_pixels = 0;
				MT9T113_sensor.cap_dummy_lines = 0;
			}
			else
			{
			
			#ifdef MT9T113_DEBUG
			    printk("[MT9T113YUV]:capture preview size \n");
			#endif
				MT9T113_sensor.cap_dummy_pixels = 0;
				MT9T113_sensor.cap_dummy_lines = 0;
			}

			MT9T113_CAP_setting(image_window, sensor_config_data);			
			MT9T113_sensor.capture_pclk = MT9T113_sensor.preview_pclk;   //Don't need change the clk for pv capture
			//MT9T113_set_dummy(MT9T113_sensor.cap_dummy_pixels, MT9T113_sensor.cap_dummy_lines);

			//[Go to capture]
			MT9T113_write_cmos_sensor(0x098E, 0xEC09);	// MCU_ADDRESS [PRI_B_NUM_OF_FRAMES_RUN]
			/*Lenovo-sw zhangjiano modified 2011-06-13 modify after capture,preview zoom 2x */
			MT9T113_write_cmos_sensor(0x0990, 0x0005);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x8400);	// MCU_ADDRESS [SEQ_CMD]
			MT9T113_write_cmos_sensor(0x0990, 0x0002);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x3400, 0x7A24);	// MIPI_CONTROL

			{
				kal_uint16 temp=0;
				while(temp <60)
				{
					MT9T113_write_cmos_sensor(0x098E, 0x8401);	  // MCU_DATA_0
					if(7==MT9T113_read_cmos_sensor(0x990))
					{
					 break;
					}
					mdelay(20);//DELAY=100
					temp+=1;
				}
			}
		
			image_window->GrabStartX = MT9T113_FULL_GRAB_START_X;
			image_window->GrabStartY = MT9T113_FULL_GRAB_START_Y;
			image_window->ExposureWindowWidth = MT9T113_FULL_GRAB_WIDTH;
			image_window->ExposureWindowHeight = MT9T113_FULL_GRAB_HEIGHT;
		
	}
	else 
	{    /* FULL Size Capture Mode */	
		MT9T113_CAP_setting(image_window, sensor_config_data);
		
		/* Capture Size <= 3M */
		//if ((image_window->image_target_width <= MT9T113_IMAGE_SENSOR_FULL_WIDTH)
		//	&& (image_window->image_target_height <= MT9T113_IMAGE_SENSOR_FULL_HEIGHT))
		{
		
			if (zoom_factor >= 7)
			{
			#ifdef MT9T113_DEBUG
			printk("[MT9T113YUV]:capture full size >=7 \n");						
			#endif
				MT9T113_sensor.cap_dummy_pixels = 0x3800;//0x80;
				MT9T113_sensor.cap_dummy_lines = 0;
			}
			else if (zoom_factor >= 5)
			{
            #ifdef MT9T113_DEBUG
			printk("[MT9T113YUV]:capture full size >=5 \n");			
			#endif
				MT9T113_sensor.cap_dummy_pixels = 100;  /*If Capture fail, you can add this dummy*/
				MT9T113_sensor.cap_dummy_lines = 0;
			}
			else if(zoom_factor >= 3)
			{
			#ifdef MT9T113_DEBUG			
			printk("[MT9T113YUV]:capture full size >=3 \n");		
			#endif
				MT9T113_sensor.cap_dummy_pixels =0x0800;  /*If Capture fail, you can add this dummy*/
				MT9T113_sensor.cap_dummy_lines = 0;
			}
			else
			{
			#ifdef MT9T113_DEBUG
				printk("[MT9T113YUV]:capture full size \n");
			#endif
				MT9T113_sensor.cap_dummy_pixels =800;  /*If Capture fail, you can add this dummy*/
				MT9T113_sensor.cap_dummy_lines = 0;
			}
		}
	
		MT9T113_set_dummy(MT9T113_sensor.cap_dummy_pixels, MT9T113_sensor.cap_dummy_lines);
		
		prev_line_len = MT9T113_PV_PERIOD_PIXEL_NUMS + MT9T113_sensor.pv_dummy_pixels;
		cap_line_len = MT9T113_FULL_PERIOD_PIXEL_NUMS + MT9T113_sensor.cap_dummy_pixels;
		shutter = (shutter * MT9T113_sensor.capture_pclk) / MT9T113_sensor.preview_pclk;
		shutter = (shutter * prev_line_len) / cap_line_len;
		shutter *= 2;			/* By sensor design */
		MT9T113_write_shutter(shutter);
		//[Go to capture]
		MT9T113_write_cmos_sensor(0x098E, 0xEC09);	// MCU_ADDRESS [PRI_B_NUM_OF_FRAMES_RUN]
		/*Lenovo-sw zhangjiano modified 2011-06-13 modify after capture,preview zoom 2x */
		MT9T113_write_cmos_sensor(0x0990, 0x000C);	// MCU_DATA_0
		MT9T113_write_cmos_sensor(0x098E, 0x8400);	// MCU_ADDRESS [SEQ_CMD]
		MT9T113_write_cmos_sensor(0x0990, 0x0002);	// MCU_DATA_0
		MT9T113_write_cmos_sensor(0x3400, 0x7A24);	// MIPI_CONTROL
		{
			kal_uint16 temp=0;
			while(temp <60)
			{
				MT9T113_write_cmos_sensor(0x098E, 0x8401);	  // MCU_DATA_0
				if(7==MT9T113_read_cmos_sensor(0x990))
				{
				 break;
				}
				mdelay(20);//DELAY=100
				temp+=1;
			}
		}
		image_window->GrabStartX = MT9T113_FULL_GRAB_START_X;
		image_window->GrabStartY = MT9T113_FULL_GRAB_START_Y;
		image_window->ExposureWindowWidth = MT9T113_FULL_GRAB_WIDTH;
		image_window->ExposureWindowHeight = MT9T113_FULL_GRAB_HEIGHT;
	}
	memcpy(&MT9T113SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;
}



UINT32 MT9T113GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
	pSensorResolution->SensorFullWidth=MT9T113_FULL_GRAB_WIDTH;  
	pSensorResolution->SensorFullHeight=MT9T113_FULL_GRAB_HEIGHT;
	pSensorResolution->SensorPreviewWidth=MT9T113_PV_GRAB_WIDTH;
	pSensorResolution->SensorPreviewHeight=MT9T113_PV_GRAB_HEIGHT;

	return ERROR_NONE;
}	/* MT9T113GetResolution() */

UINT32 MT9T113GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	pSensorInfo->SensorPreviewResolutionX=MT9T113_PV_GRAB_WIDTH;
	pSensorInfo->SensorPreviewResolutionY=MT9T113_PV_GRAB_HEIGHT;
	pSensorInfo->SensorFullResolutionX=MT9T113_FULL_GRAB_WIDTH;
	pSensorInfo->SensorFullResolutionY=MT9T113_FULL_GRAB_HEIGHT;

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

	

	pSensorInfo->CaptureDelayFrame = 3; 
	pSensorInfo->PreviewDelayFrame = 5; 
	pSensorInfo->VideoDelayFrame = 5; 
	pSensorInfo->SensorMasterClockSwitch = 0; 
       pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;   		
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
                     pSensorInfo->SensorGrabStartX = MT9T113_PV_GRAB_START_X; 
                     pSensorInfo->SensorGrabStartY = MT9T113_PV_GRAB_START_Y;     			
			
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
                     pSensorInfo->SensorGrabStartX = MT9T113_FULL_GRAB_START_X; 
                     pSensorInfo->SensorGrabStartY = MT9T113_FULL_GRAB_START_Y;     			
		break;
		default:
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount=3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
                     pSensorInfo->SensorGrabStartX = MT9T113_PV_GRAB_START_X; 
                     pSensorInfo->SensorGrabStartY = MT9T113_PV_GRAB_START_Y;     			
		break;
	}
	//MT9T113_PixelClockDivider=pSensorInfo->SensorPixelClockCount;
	memcpy(pSensorConfigData, &MT9T113SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	return ERROR_NONE;
}	/* MT9T113GetInfo() */


UINT32 MT9T113Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			MT9T113Preview(pImageWindow, pSensorConfigData);
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			MT9T113Capture(pImageWindow, pSensorConfigData);
		break;
		default:
            return ERROR_INVALID_SCENARIO_ID;
	}
	return TRUE;
}	/* MT9T113Control() */



BOOL MT9T113_set_param_wb(UINT16 para)
{
#if 1
	switch (para)
	{
	    case AWB_MODE_OFF:
		   #ifdef MT9T113_DEBUG
				printk("[MT9T113YUV]:AWB off \n");
			#endif	
	        MT9T113_AWB_ENABLE = KAL_FALSE; 
	        MT9T113_set_AWB_mode(MT9T113_AWB_ENABLE);
	        break;             
		case AWB_MODE_AUTO:
			#ifdef MT9T113_DEBUG
				printk("[MT9T113YUV]:AWB auto \n");
			#endif
            MT9T113_AWB_ENABLE = KAL_TRUE; 
            MT9T113_set_AWB_mode(MT9T113_AWB_ENABLE);    
		    MT9T113_write_cmos_sensor(0x098E, 0x6848 ); // MCU_ADDRESS [PRI_A_CONFIG_AWB_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x003F ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6865 ); // MCU_ADDRESS [PRI_A_CONFIG_STAT_ALGO_ENTER]
			MT9T113_write_cmos_sensor(0x0990, 0x801F ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6867 ); // MCU_ADDRESS [PRI_A_CONFIG_STAT_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x12F7 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6881 ); // MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_ALGO_ENTER]
			MT9T113_write_cmos_sensor(0x0990, 0x0008 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6883 ); // MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x000B ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x8400 ); // MCU_ADDRESS [SEQ_CMD]
			MT9T113_write_cmos_sensor(0x0990, 0x0006 ); // MCU_DATA_0
			{
			kal_uint16 temp=0;
			while(temp <60)
			{
				MT9T113_write_cmos_sensor(0x098E, 0x8400);	  // MCU_DATA_0
				if(0==MT9T113_read_cmos_sensor(0x990))
				{
				 break;
				}
				mdelay(28);//DELAY=100
				temp+=1;
			}
		}
		    break;
		case AWB_MODE_CLOUDY_DAYLIGHT:
			#ifdef MT9T113_DEBUG
				printk("[MT9T113YUV]:AWB Cloudy \n");
			#endif
		    MT9T113_write_cmos_sensor(0x098E, 0x6848 ); // MCU_ADDRESS [PRI_A_CONFIG_AWB_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x0000 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6865 ); // MCU_ADDRESS [PRI_A_CONFIG_STAT_ALGO_ENTER]
			MT9T113_write_cmos_sensor(0x0990, 0x0000 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6867 ); // MCU_ADDRESS [PRI_A_CONFIG_STAT_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x0000 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6881 ); // MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_ALGO_ENTER]
			MT9T113_write_cmos_sensor(0x0990, 0x0008 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6883 ); // MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x0008 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x8400 ); // MCU_ADDRESS [SEQ_CMD]
			MT9T113_write_cmos_sensor(0x0990, 0x0006 ); // MCU_DATA_0
			
			MT9T113_write_cmos_sensor(0x098E, 0xAC3B ); // MCU_ADDRESS [AWB_R_RATIO_PRE_AWB]
			MT9T113_write_cmos_sensor(0x0990, 0x0034 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xAC3C ); // MCU_ADDRESS [AWB_B_RATIO_PRE_AWB]
			MT9T113_write_cmos_sensor(0x0990, 0x0054 ); // MCU_DATA_0
		    break;
		case AWB_MODE_DAYLIGHT:
			#ifdef MT9T113_DEBUG
				printk("[MT9T113YUV]:AWB Daylight \n");
			#endif
		    MT9T113_write_cmos_sensor(0x098E, 0x6848 ); // MCU_ADDRESS [PRI_A_CONFIG_AWB_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x0000 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6865 ); // MCU_ADDRESS [PRI_A_CONFIG_STAT_ALGO_ENTER]
			MT9T113_write_cmos_sensor(0x0990, 0x0000 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6867 ); // MCU_ADDRESS [PRI_A_CONFIG_STAT_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x0000 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6881 ); // MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_ALGO_ENTER]
			MT9T113_write_cmos_sensor(0x0990, 0x0008 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6883 ); // MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x0008 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x8400 ); // MCU_ADDRESS [SEQ_CMD]
			MT9T113_write_cmos_sensor(0x0990, 0x0006 ); // MCU_DATA_0
			
			MT9T113_write_cmos_sensor(0x098E, 0xAC3B ); // MCU_ADDRESS [AWB_R_RATIO_PRE_AWB]
			MT9T113_write_cmos_sensor(0x0990, 0x0039 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xAC3C ); // MCU_ADDRESS [AWB_B_RATIO_PRE_AWB]
			MT9T113_write_cmos_sensor(0x0990, 0x0054 ); // MCU_DATA_0
		    break;
		case AWB_MODE_INCANDESCENT:	
			#ifdef MT9T113_DEBUG
				printk("[MT9T113YUV]:AWB INCANDESCENT \n");
			#endif
		    MT9T113_write_cmos_sensor(0x098E, 0x6848 ); // MCU_ADDRESS [PRI_A_CONFIG_AWB_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x0000 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6865 ); // MCU_ADDRESS [PRI_A_CONFIG_STAT_ALGO_ENTER]
			MT9T113_write_cmos_sensor(0x0990, 0x0000 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6867 ); // MCU_ADDRESS [PRI_A_CONFIG_STAT_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x0000 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6881 ); // MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_ALGO_ENTER]
			MT9T113_write_cmos_sensor(0x0990, 0x0008 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6883 ); // MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x0008 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x8400 ); // MCU_ADDRESS [SEQ_CMD]
			MT9T113_write_cmos_sensor(0x0990, 0x0006 ); // MCU_DATA_0
			
			MT9T113_write_cmos_sensor(0x098E, 0xAC3B ); // MCU_ADDRESS [AWB_R_RATIO_PRE_AWB]
			MT9T113_write_cmos_sensor(0x0990, 0x0058 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xAC3C ); // MCU_ADDRESS [AWB_B_RATIO_PRE_AWB]
			MT9T113_write_cmos_sensor(0x0990, 0x0028 ); // MCU_DATA_0	
		    break;  
		case AWB_MODE_FLUORESCENT:
			#ifdef MT9T113_DEBUG
				printk("[MT9T113YUV]:AWB FLUORESCENT \n");
			#endif
		    MT9T113_write_cmos_sensor(0x098E, 0x6848 ); // MCU_ADDRESS [PRI_A_CONFIG_AWB_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x0000 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6865 ); // MCU_ADDRESS [PRI_A_CONFIG_STAT_ALGO_ENTER]
			MT9T113_write_cmos_sensor(0x0990, 0x0000 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6867 ); // MCU_ADDRESS [PRI_A_CONFIG_STAT_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x0000 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6881 ); // MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_ALGO_ENTER]
			MT9T113_write_cmos_sensor(0x0990, 0x0008 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6883 ); // MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x0008 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x8400 ); // MCU_ADDRESS [SEQ_CMD]
			MT9T113_write_cmos_sensor(0x0990, 0x0006 ); // MCU_DATA_0
			
			MT9T113_write_cmos_sensor(0x098E, 0xAC3B ); // MCU_ADDRESS [AWB_R_RATIO_PRE_AWB]
			MT9T113_write_cmos_sensor(0x0990, 0x0044 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xAC3C ); // MCU_ADDRESS [AWB_B_RATIO_PRE_AWB]
			MT9T113_write_cmos_sensor(0x0990, 0x0031 ); // MCU_DATA_0
		    break;  
		case AWB_MODE_TUNGSTEN:
			#ifdef MT9T113_DEBUG
				printk("[MT9T113YUV]:AWB TUNGSTEN \n");
			#endif
		    MT9T113_write_cmos_sensor(0x098E, 0x6848 ); // MCU_ADDRESS [PRI_A_CONFIG_AWB_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x0000 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6865 ); // MCU_ADDRESS [PRI_A_CONFIG_STAT_ALGO_ENTER]
			MT9T113_write_cmos_sensor(0x0990, 0x0000 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6867 ); // MCU_ADDRESS [PRI_A_CONFIG_STAT_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x0000 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6881 ); // MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_ALGO_ENTER]
			MT9T113_write_cmos_sensor(0x0990, 0x0008 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x6883 ); // MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_ALGO_RUN]
			MT9T113_write_cmos_sensor(0x0990, 0x0008 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x8400 ); // MCU_ADDRESS [SEQ_CMD]
			MT9T113_write_cmos_sensor(0x0990, 0x0006 ); // MCU_DATA_0
			
			MT9T113_write_cmos_sensor(0x098E, 0xAC3B ); // MCU_ADDRESS [AWB_R_RATIO_PRE_AWB]
			MT9T113_write_cmos_sensor(0x0990, 0x0058 ); // MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xAC3C ); // MCU_ADDRESS [AWB_B_RATIO_PRE_AWB]
			MT9T113_write_cmos_sensor(0x0990, 0x0023 ); // MCU_DATA_0
		    break;
		default:
			return FALSE;
	}
#endif
	return TRUE;
	
} /* MT9T113_set_param_wb */

BOOL MT9T113_set_param_effect(UINT16 para)
{
  kal_uint32 ret = KAL_TRUE;
  #if 1
	switch (para)
	{
		case MEFFECT_OFF:
			#ifdef MT9T113_DEBUG
				printk("[MT9T113YUV]:effect off \n");
			#endif
			MT9T113_write_cmos_sensor(0x098E, 0xE887);	// MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_SELECT_FX]
			MT9T113_write_cmos_sensor(0x0990, 0x0000);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xEC87);	// MCU_ADDRESS [PRI_B_CONFIG_SYSCTRL_SELECT_FX]
			MT9T113_write_cmos_sensor(0x0990, 0x0000);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x8400);	// MCU_ADDRESS [SEQ_CMD]
			MT9T113_write_cmos_sensor(0x0990, 0x0006);	// MCU_DATA_0
			{
			kal_uint16 temp=0;
			while(temp <60)
			{
				MT9T113_write_cmos_sensor(0x098E, 0x8400);	  // MCU_DATA_0
				if(0==MT9T113_read_cmos_sensor(0x990))
				{
				 break;
				}
				mdelay(10);//DELAY=100
				temp+=1;
			}
		}
	              break;
		case MEFFECT_SEPIA:
			#ifdef MT9T113_DEBUG
				printk("[MT9T113YUV]:effect SEPIA \n");
			#endif
			//[Special Effect-Sepia]
			MT9T113_write_cmos_sensor(0x098E, 0xE887);	// MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_SELECT_FX]
			MT9T113_write_cmos_sensor(0x0990, 0x0002);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xEC87);	// MCU_ADDRESS [PRI_B_CONFIG_SYSCTRL_SELECT_FX]
			MT9T113_write_cmos_sensor(0x0990, 0x0002);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xE889);	// MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_SEPIA_CR]
			MT9T113_write_cmos_sensor(0x0990, 0x001D);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xEC89); 	// MCU_ADDRESS [PRI_B_CONFIG_SYSCTRL_SEPIA_CR]
		    MT9T113_write_cmos_sensor(0x0990, 0x001D); 	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xE88A);	// MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_SEPIA_CB]
			MT9T113_write_cmos_sensor(0x0990, 0x00D8);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xEC8A);	// MCU_ADDRESS [PRI_B_CONFIG_SYSCTRL_SEPIA_CR]
			MT9T113_write_cmos_sensor(0x0990, 0x00D8);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x8400);	// MCU_ADDRESS [SEQ_CMD]
			MT9T113_write_cmos_sensor(0x0990, 0x0006);	// MCU_DATA_0	
			{
				kal_uint16 temp=0;
				while(temp <60)
				{
					MT9T113_write_cmos_sensor(0x098E, 0x8400);	  // MCU_DATA_0
					if(0==MT9T113_read_cmos_sensor(0x990))
					{
					 break;
					}
					mdelay(10);//DELAY=100
					temp+=1;
				}
			}
			break;  
		case MEFFECT_NEGATIVE:		
			#ifdef MT9T113_DEBUG
				printk("[MT9T113YUV]:effect negative \n");
			#endif
			//[Special Effect-Negative]
			MT9T113_write_cmos_sensor(0x098E, 0xE887);	// MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_SELECT_FX]
			MT9T113_write_cmos_sensor(0x0990, 0x0003);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xEC87);	// MCU_ADDRESS [PRI_B_CONFIG_SYSCTRL_SELECT_FX]
			MT9T113_write_cmos_sensor(0x0990, 0x0003);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x8400);	// MCU_ADDRESS [SEQ_CMD]
			MT9T113_write_cmos_sensor(0x0990, 0x0006);	// MCU_DATA_0
			{
				kal_uint16 temp=0;
				while(temp <60)
				{
					MT9T113_write_cmos_sensor(0x098E, 0x8400);	  // MCU_DATA_0
					if(0==MT9T113_read_cmos_sensor(0x990))
					{
					 break;
					}
					mdelay(10);//DELAY=100
					temp+=1;
				}
			}
			break; 
		case MEFFECT_SEPIAGREEN:		
			#ifdef MT9T113_DEBUG
				printk("[MT9T113YUV]:effect sepiaGreen \n");
			#endif
			//[Special Effect-green]
			MT9T113_write_cmos_sensor(0x098E, 0xE887);	// MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_SELECT_FX]
			MT9T113_write_cmos_sensor(0x0990, 0x0002);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xEC87);	// MCU_ADDRESS [PRI_B_CONFIG_SYSCTRL_SELECT_FX]
			MT9T113_write_cmos_sensor(0x0990, 0x0002);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xE889);	// MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_SEPIA_CR]
			MT9T113_write_cmos_sensor(0x0990, 0x00D8);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xE88A);	// MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_SEPIA_CB]
			MT9T113_write_cmos_sensor(0x0990, 0x00BA);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xEC89);	// MCU_ADDRESS [PRI_B_CONFIG_SYSCTRL_SEPIA_CR]
			MT9T113_write_cmos_sensor(0x0990, 0x00D8);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xEC8A);	// MCU_ADDRESS [PRI_B_CONFIG_SYSCTRL_SEPIA_CB]
			MT9T113_write_cmos_sensor(0x0990, 0x00BA);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x8400);	// MCU_ADDRESS [SEQ_CMD]
			MT9T113_write_cmos_sensor(0x0990, 0x0006);	// MCU_DATA_0	
			{
				kal_uint16 temp=0;
				while(temp <60)
				{
					MT9T113_write_cmos_sensor(0x098E, 0x8400);	  // MCU_DATA_0
					if(0==MT9T113_read_cmos_sensor(0x990))
					{
					 break;
					}
					mdelay(10);//DELAY=100
					temp+=1;
				}
			}
			break;
		case MEFFECT_SEPIABLUE:	
			#ifdef MT9T113_DEBUG
				printk("[MT9T113YUV]:effect sepiablue \n");
			#endif
			//[Special Effect-Aqua]
			MT9T113_write_cmos_sensor(0x098E, 0xE887);	// MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_SELECT_FX]
			MT9T113_write_cmos_sensor(0x0990, 0x0002);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xEC87);	// MCU_ADDRESS [PRI_B_CONFIG_SYSCTRL_SELECT_FX]
			MT9T113_write_cmos_sensor(0x0990, 0x0002);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xE889);	// MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_SEPIA_CR]
			MT9T113_write_cmos_sensor(0x0990, 0x00E2);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xE88A);	// MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_SEPIA_CB]
			MT9T113_write_cmos_sensor(0x0990, 0x0030);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xEC89);	// MCU_ADDRESS [PRI_B_CONFIG_SYSCTRL_SEPIA_CR]
			MT9T113_write_cmos_sensor(0x0990, 0x00E2);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xEC8A);	// MCU_ADDRESS [PRI_B_CONFIG_SYSCTRL_SEPIA_CB]
			MT9T113_write_cmos_sensor(0x0990, 0x0030);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x8400);	// MCU_ADDRESS [SEQ_CMD]
			MT9T113_write_cmos_sensor(0x0990, 0x0006);	// MCU_DATA_0	
			{
				kal_uint16 temp=0;
				while(temp <60)
				{
					MT9T113_write_cmos_sensor(0x098E, 0x8400);	  // MCU_DATA_0
					if(0==MT9T113_read_cmos_sensor(0x990))
					{
					 break;
					}
					mdelay(10);//DELAY=100
					temp+=1;
				}
			}
			break;        
		case MEFFECT_MONO:				
		#ifdef MT9T113_DEBUG
				printk("[MT9T113YUV]:effect mono \n");
		#endif
			//[Special Effect-Black/White]
			MT9T113_write_cmos_sensor(0x098E, 0xE887);	// MCU_ADDRESS [PRI_A_CONFIG_SYSCTRL_SELECT_FX]
			MT9T113_write_cmos_sensor(0x0990, 0x0001);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0xEC87);	// MCU_ADDRESS [PRI_B_CONFIG_SYSCTRL_SELECT_FX]
			MT9T113_write_cmos_sensor(0x0990, 0x0001);	// MCU_DATA_0
			MT9T113_write_cmos_sensor(0x098E, 0x8400);	// MCU_ADDRESS [SEQ_CMD]
			MT9T113_write_cmos_sensor(0x0990, 0x0006);	// MCU_DATA_0			  
			{
				kal_uint16 temp=0;
				while(temp <60)
				{
					MT9T113_write_cmos_sensor(0x098E, 0x8400);	  // MCU_DATA_0
					if(0==MT9T113_read_cmos_sensor(0x990))
					{
					 break;
					}
					mdelay(10);//DELAY=100
					temp+=1;
				}
			}
			break;

		default:
			ret = FALSE;
	}
#endif
	return ret;

} /* MT9T113_set_param_effect */

BOOL MT9T113_set_param_banding(UINT16 para)
{
	kal_uint16 temp_reg = 0;
	/* Some sensor call base shutter as banding filter. */
	kal_uint32 base_shutter = 0, max_shutter_step = 0, exposure_limitation = 0;
	kal_uint32 line_length = 0, sensor_pixel_clock = 0;
	#ifdef MT9T113_DEBUG
		printk("[MT9T113YUV]:banding\n");  
	#endif
  #if 1
	if (MT9T113_op_state.is_PV_mode == KAL_TRUE)
		{
			line_length = MT9T113_PV_PERIOD_PIXEL_NUMS + MT9T113_sensor.pv_dummy_pixels;
			exposure_limitation = MT9T113_PV_PERIOD_LINE_NUMS + MT9T113_sensor.pv_dummy_lines;
			sensor_pixel_clock = MT9T113_sensor.preview_pclk * 100 * 1000;
		}
		else
		{
			line_length = MT9T113_FULL_PERIOD_PIXEL_NUMS + MT9T113_sensor.cap_dummy_pixels;
			exposure_limitation = MT9T113_FULL_PERIOD_LINE_NUMS + MT9T113_sensor.cap_dummy_lines;
			sensor_pixel_clock = MT9T113_sensor.capture_pclk * 100 * 1000;
		}
	
		line_length = line_length * 2;		/* Multiple 2 is because one YUV422 pixels need two clock. */
	
		MT9T113_op_state.curr_banding = para;	/* Record current banding setting. */
		

	switch (para)
	{
		case AE_FLICKER_MODE_50HZ:
		#ifdef MT9T113_DEBUG
	       printk("[MT9T113YUV]:banding 50Hz\n");  
	    #endif	
		    /* + (line_length/2) is used fot base_shutter + 0.5 */
		base_shutter = ((sensor_pixel_clock/100) + (line_length/2)) / line_length;
		max_shutter_step = (exposure_limitation / base_shutter) - 1;

		//[50Hz]
		MT9T113_write_cmos_sensor(0x098E, 0x2003);	// MCU_ADDRESS [FD_ALGO]
		MT9T113_write_cmos_sensor(0x0990, 0x0002);	// MCU_DATA_0
		MT9T113_write_cmos_sensor(0x098E, 0xA005);	// MCU_ADDRESS [FD_FDPERIOD_SELECT]
		MT9T113_write_cmos_sensor(0x0990, 0x0001);	// MCU_DATA_0

			break;

		case AE_FLICKER_MODE_60HZ:
		#ifdef MT9T113_DEBUG
	       printk("[MT9T113YUV]:banding 60Hz\n");  
	    #endif	
			 /* + (line_length/2) is used fot base_shutter + 0.5 */
		base_shutter = ((sensor_pixel_clock/120) + (line_length/2)) / line_length;
		max_shutter_step = (exposure_limitation / base_shutter) - 1;

		//[60Hz]
		MT9T113_write_cmos_sensor(0x098E, 0x2003);	// MCU_ADDRESS [FD_ALGO]
		MT9T113_write_cmos_sensor(0x0990, 0x0002);	// MCU_DATA_0
		MT9T113_write_cmos_sensor(0x098E, 0xA005);	// MCU_ADDRESS [FD_FDPERIOD_SELECT]
		MT9T113_write_cmos_sensor(0x0990, 0x0000);	// MCU_DATA_0
			break;
	     default:
	          return KAL_FALSE;
	}
#endif
	return KAL_TRUE;
} /* MT9T113_set_param_banding */




BOOL MT9T113_set_param_exposure(UINT16 para)
{
	/*Lenovo-sw zhangjiano modified 2011-06-13 modify EV setting from brightness to AE target */
	printk("[MT9T113YUV]:ev 0\n");  
  #if 1
	switch (para)
	{
		case AE_EV_COMP_13:
			printk("[MT9T113YUV]:AE_EV_COMP_13 0\n");  
			MT9T113_write_cmos_sensor(0x098E, 0xe826);	// Y_RGB_OFFSET
			MT9T113_write_cmos_sensor(0x0990, 0x0076);	// Y_RGB_OFFSET			break;  
			break;    
		case AE_EV_COMP_10:
			printk("[MT9T113YUV]:AE_EV_COMP_10 0\n");  
			MT9T113_write_cmos_sensor(0x098E, 0xe826);	// Y_RGB_OFFSET
			MT9T113_write_cmos_sensor(0x0990, 0x0066);	// Y_RGB_OFFSET			break;    
			break;    
		case AE_EV_COMP_07:
			printk("[MT9T113YUV]:AE_EV_COMP_07 0\n");  
			MT9T113_write_cmos_sensor(0x098E, 0xe826);	// Y_RGB_OFFSET
			MT9T113_write_cmos_sensor(0x0990, 0x0056);	// Y_RGB_OFFSET			break;    
			break;    
		case AE_EV_COMP_03:	
			printk("[MT9T113YUV]:AE_EV_COMP_03 0\n");  
			MT9T113_write_cmos_sensor(0x098E, 0xe826);	// Y_RGB_OFFSET
			MT9T113_write_cmos_sensor(0x0990, 0x0050);	// Y_RGB_OFFSET
			break;    
		case AE_EV_COMP_00:
			#ifdef MT9T113_DEBUG
				printk("[MT9T113YUV]:ev 0\n");  
			#endif
			MT9T113_write_cmos_sensor(0x098E, 0xe826);	// Y_RGB_OFFSET
			MT9T113_write_cmos_sensor(0x0990, 0x003b);	// Y_RGB_OFFSET			break;    
			break;    
		case AE_EV_COMP_n03:
			MT9T113_write_cmos_sensor(0x098E, 0xe826);	// Y_RGB_OFFSET
			MT9T113_write_cmos_sensor(0x0990, 0x0038);	// Y_RGB_OFFSET
			break;    
		case AE_EV_COMP_n07:			
			MT9T113_write_cmos_sensor(0x098E, 0xe826);	// Y_RGB_OFFSET
			MT9T113_write_cmos_sensor(0x0990, 0x0030);	// Y_RGB_OFFSET
			break;    
		case AE_EV_COMP_n10:
			MT9T113_write_cmos_sensor(0x098E, 0xe826);	// Y_RGB_OFFSET
			MT9T113_write_cmos_sensor(0x0990, 0x0028);	// Y_RGB_OFFSET
			break;
		case AE_EV_COMP_n13:
			MT9T113_write_cmos_sensor(0x098E, 0xe826);	// Y_RGB_OFFSET
			MT9T113_write_cmos_sensor(0x0990, 0x0017);	// Y_RGB_OFFSET
			break;
		default:
			return FALSE;
	}
#endif
	return TRUE;
	
} /* MT9T113_set_param_exposure */


UINT32 MT9T113YUVSensorSetting(FEATURE_ID iCmd, UINT32 iPara)
{
//   if( MT9T113_sensor_cap_state == KAL_TRUE)
//	   return TRUE;
	    printk("MT9T113YUVSensorSetting iCmd:%d\n", iCmd); 
	switch (iCmd) {
	case FID_SCENE_MODE:	    
	    printk("Set Scene Mode:%d\n", iPara); 
	    if (iPara == SCENE_MODE_OFF)
	    {
	        MT9T113_night_mode(0); 
	    }
	    else if (iPara == SCENE_MODE_NIGHTSCENE)
	    {
               MT9T113_night_mode(1); 
	    }	    
	    break; 	    
	case FID_AWB_MODE:
	    printk("Set AWB Mode:%d\n", iPara); 	    
           MT9T113_set_param_wb(iPara);
	break;
	case FID_COLOR_EFFECT:
	    printk("Set Color Effect:%d\n", iPara); 	    	    
           MT9T113_set_param_effect(iPara);
	break;
	case FID_AE_EV:	    
           printk("Set EV:%d\n", iPara); 	    	    
           MT9T113_set_param_exposure(iPara);
	break;
	case FID_AE_FLICKER:
//           printk("Set Flicker:%d\n", iPara); 	    	    	    
           MT9T113_set_param_banding(iPara);
	break;
	case FID_AE_SCENE_MODE: 
	    if (iPara == AE_MODE_OFF) {
                MT9T113_AE_ENABLE = KAL_FALSE; 
            }
            else {
                MT9T113_AE_ENABLE = KAL_TRUE; 
	    }
            MT9T113_set_AE_mode(MT9T113_AE_ENABLE);
            break; 

	case FID_ZOOM_FACTOR:
	    zoom_factor = iPara; 		
	break; 
	default:
	break;
	}
	return TRUE;
}   /* MT9T113YUVSensorSetting */

UINT32 MT9T113YUVSetVideoMode(UINT16 u2FrameRate)
{
    kal_uint8 iTemp,tmp;
    printk("[MT9T113YUV] Set Video Mode \n"); 
    MT9T113_VEDIO_encode_mode = KAL_TRUE; 
    //iTemp = MT9T113_read_cmos_sensor(0x3014);
    //MT9T113_write_cmos_sensor(0x3014, iTemp & 0xf7); //Disable night mode

    if (u2FrameRate == 30)
    {
    #if 0
      #ifdef MT9T113_DEBUG
        printk("[MT9T113YUV] video 30 \n");
	   #endif
	   MT9T113_write_cmos_sensor(0x098E, 0x682F);  // MCU_ADDRESS
	   MT9T113_write_cmos_sensor(0x0990, 0x0110);  // MCU_DATA_0// gain
       u2FrameRate=29;
       MT9T113_Set_Video_Frame_Rate(u2FrameRate*10);
	   #else
	   MT9T113_write_cmos_sensor(0x098E, 0x4808);  
	   MT9T113_write_cmos_sensor(0x0990, 0x04DF);  //04DF 20fps;25fps,3E5;22fps,46d
		MT9T113_write_cmos_sensor(0x098E, 0x6820);	// ae_target_zone
		MT9T113_write_cmos_sensor(0x0990, 0x0005);	//
		
	    MT9T113_write_cmos_sensor(0x098E, 0x6815);	  // MCU_ADDRESS [PRI_A_CONFIG_FD_MAX_FDZONE_50HZ]
		MT9T113_write_cmos_sensor(0x0990, 0x0005);	  // MCU_DATA_0
		MT9T113_write_cmos_sensor(0x098E, 0x6817);	  // MCU_ADDRESS [PRI_A_CONFIG_FD_MAX_FDZONE_60HZ]
		MT9T113_write_cmos_sensor(0x0990, 0x0006);	  // MCU_DATA_0																   	    
	   #endif
    }
    else if (u2FrameRate == 15)
    {   
	#ifdef MT9T113_DEBUG
        printk("[MT9T113YUV] video 15 \n");
	#endif
		#if 0
	   MT9T113_write_cmos_sensor(0x098E, 0x682F);  // MCU_ADDRESS
	   MT9T113_write_cmos_sensor(0x0990, 0x0090);  // MCU_DATA_0// gain
       MT9T113_Set_Video_Frame_Rate(u2FrameRate*10);
	   #else
	   MT9T113_write_cmos_sensor(0x098E, 0x4808);  // MCU_ADDRESS
	   MT9T113_write_cmos_sensor(0x0990, 0x067E);  // MCU_DATA_0// gain
		MT9T113_write_cmos_sensor(0x098E, 0x6820);	// ae_target_zone
		MT9T113_write_cmos_sensor(0x0990, 0x0006);	// 	

	    MT9T113_write_cmos_sensor(0x098E, 0x6815);	  // MCU_ADDRESS [PRI_A_CONFIG_FD_MAX_FDZONE_50HZ]
		MT9T113_write_cmos_sensor(0x0990, 0x0006);	  // MCU_DATA_0
		MT9T113_write_cmos_sensor(0x098E, 0x6817);	  // MCU_ADDRESS [PRI_A_CONFIG_FD_MAX_FDZONE_60HZ]
		MT9T113_write_cmos_sensor(0x0990, 0x0008);	  // MCU_DATA_0																   	    
	   #endif

	   
    }
    else 
    {
    #ifdef MT9T113_DEBUG
        printk("Wrong Frame Rate \n"); 
	#endif
    }
	
    MT9T113_write_cmos_sensor(0x98E, 0x8400);	//Refresh Sequencer Mode
	MT9T113_write_cmos_sensor(0x990, 0x06); //		= 6
	tmp =0;
	while(tmp<50)
	{
		mdelay(10);//DELAY=100
		MT9T113_write_cmos_sensor(0x98E, 0x8400);	//Refresh Sequencer Mode
		if(0x00==MT9T113_read_cmos_sensor(0x990))
		{
			 break;
		}
		tmp+=1;
	}

    
    return TRUE;
}

UINT32 MT9T113FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
							 UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
       UINT16 u2Temp = 0; 
	UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
	UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
	UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
	UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
	PNVRAM_SENSOR_DATA_STRUCT pSensorDefaultData=(PNVRAM_SENSOR_DATA_STRUCT) pFeaturePara;
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
	MSDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo=(MSDK_SENSOR_GROUP_INFO_STRUCT *) pFeaturePara;
	MSDK_SENSOR_ITEM_INFO_STRUCT *pSensorItemInfo=(MSDK_SENSOR_ITEM_INFO_STRUCT *) pFeaturePara;
	MSDK_SENSOR_ENG_INFO_STRUCT	*pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;


	switch (FeatureId)
	{
		case SENSOR_FEATURE_GET_RESOLUTION:
			*pFeatureReturnPara16++=MT9T113_FULL_GRAB_WIDTH;
			*pFeatureReturnPara16=MT9T113_FULL_GRAB_HEIGHT;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PERIOD:
			//*pFeatureReturnPara16++=MT9T113_PV_PERIOD_PIXEL_NUMS+MT9T113_PV_dummy_pixels;
			//*pFeatureReturnPara16=MT9T113_PV_PERIOD_LINE_NUMS+MT9T113_PV_dummy_lines;
			//*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			*pFeatureReturnPara32 = MT9T113_sensor_pclk/10;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_SET_ESHUTTER:
//			u2Temp = MT9T113_read_shutter(); 
//			printk("Shutter:%d\n", u2Temp); 			
		break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			MT9T113_night_mode((BOOL) *pFeatureData16);
		break;
		case SENSOR_FEATURE_SET_GAIN:
//			u2Temp = MT9T113_read_gain(); 
//			printk("Gain:%d\n", u2Temp); 
//			printk("y_val:%d\n", MT9T113_read_cmos_sensor(0x301B));
			break; 
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			MT9T113_isp_master_clock=*pFeatureData32;
		break;
		case SENSOR_FEATURE_SET_REGISTER:
			MT9T113_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
		case SENSOR_FEATURE_GET_REGISTER:
			pSensorRegData->RegData = MT9T113_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			memcpy(pSensorConfigData, &MT9T113SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
			*pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
		break;
		case SENSOR_FEATURE_SET_CCT_REGISTER:
		case SENSOR_FEATURE_GET_CCT_REGISTER:
		case SENSOR_FEATURE_SET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_REGISTER_DEFAULT:

		case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
		case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
		case SENSOR_FEATURE_GET_GROUP_INFO:
		case SENSOR_FEATURE_GET_ITEM_INFO:
		case SENSOR_FEATURE_SET_ITEM_INFO:
		case SENSOR_FEATURE_GET_ENG_INFO:
		break;
		case SENSOR_FEATURE_GET_GROUP_COUNT:
	                *pFeatureReturnPara32++=0;
			*pFeatureParaLen=4;
		    break; 
		
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_SET_YUV_CMD:
		       printk("MT9T113 YUV sensor Setting:%d, %d \n", *pFeatureData32,  *(pFeatureData32+1));
			MT9T113YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
		break;		
		case SENSOR_FEATURE_SET_VIDEO_MODE:
		    MT9T113YUVSetVideoMode(*pFeatureData16);
		    break; 
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
			MT9T113_GetSensorID(pFeatureData32); 
			break; 	
		default:
			break;			
	}
	return ERROR_NONE;
}	/* MT9T113FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncMT9T113=
{
	MT9T113Open,
	MT9T113GetInfo,
	MT9T113GetResolution,
	MT9T113FeatureControl,
	MT9T113Control,
	MT9T113Close
};

UINT32 MT9T113_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncMT9T113;

	return ERROR_NONE;
}	/* SensorInit() */


