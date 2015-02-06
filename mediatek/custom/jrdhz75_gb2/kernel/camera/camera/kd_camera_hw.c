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
#include "kd_camera_feature.h"

/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PFX "[kd_camera_hw]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    printk(KERN_INFO PFX "%s: " fmt, __FUNCTION__ ,##arg)

#define DEBUG_CAMERA_HW_K
#ifdef DEBUG_CAMERA_HW_K
#define PK_DBG PK_DBG_FUNC
#define PK_ERR(fmt, arg...)         printk(KERN_ERR PFX "%s: " fmt, __FUNCTION__ ,##arg)
#else
#define PK_DBG(a,...)
#define PK_ERR(a,...)
#endif

 



#if defined(MT6575)

int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name)
{
u32 pinSetIdx = 0;//default main sensor
unsigned int (*pinSet)[8];

#define IDX_PS_CMRST 0
#define IDX_PS_CMPDN 4

#define IDX_PS_MODE 1
#define IDX_PS_ON   2
#define IDX_PS_OFF  3
//u32 pinSet[3][8] = {

 u32 gc0328_sp0718[3][8] = {
                    //for main sensor 
                    {GPIO_CAMERA_INVALID, //Reset NC
                        GPIO_CAMERA_CMRST_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,                   /* ON state */
                        GPIO_OUT_ZERO,                  /* OFF state */
                     GPIO_CAMERA_CMPDN_PIN,
                        GPIO_CAMERA_CMPDN_PIN_M_GPIO,
                        GPIO_OUT_ZERO,
                        GPIO_OUT_ONE,
                    },
                    //for sub sensor 
                    {GPIO_CAMERA_CMRST1_PIN,
                     GPIO_CAMERA_CMRST1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     GPIO_CAMERA_CMPDN1_PIN,
                        GPIO_CAMERA_CMPDN1_PIN_M_GPIO,                        
                        GPIO_OUT_ZERO,
                        GPIO_OUT_ONE,					
                    },
					//for main_2 sensor 
                    {GPIO_CAMERA_CMRST_PIN,
                        GPIO_CAMERA_CMRST_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,                   /* ON state */
                        GPIO_OUT_ZERO,                  /* OFF state */
                     GPIO_CAMERA_CMPDN_PIN,
                        GPIO_CAMERA_CMPDN_PIN_M_GPIO,
                        //GPIO_OUT_ONE, //on pdn
                        GPIO_OUT_ZERO,      //on state
                        GPIO_OUT_ONE, //off pdn                        
                    }
                   };

 u32 hi253_s5k5ca[3][8] = {
                    //for main sensor 
                    {GPIO_CAMERA_CMRST_PIN,
                        GPIO_CAMERA_CMRST_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,                   /* ON state */
                        GPIO_OUT_ZERO,                  /* OFF state */
                     GPIO_CAMERA_CMPDN_PIN,
                        GPIO_CAMERA_CMPDN_PIN_M_GPIO,
                        GPIO_OUT_ZERO,
                        GPIO_OUT_ONE,
                    },
                    //for sub sensor 
                    {GPIO_CAMERA_CMRST1_PIN,
                     GPIO_CAMERA_CMRST1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     GPIO_CAMERA_CMPDN1_PIN,
                        GPIO_CAMERA_CMPDN1_PIN_M_GPIO,                        
                        GPIO_OUT_ZERO,
                        GPIO_OUT_ONE,					
                    },
					//for main_2 sensor 
                    {GPIO_CAMERA_CMRST_PIN,
                        GPIO_CAMERA_CMRST_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,                   /* ON state */
                        GPIO_OUT_ZERO,                  /* OFF state */
                     GPIO_CAMERA_CMPDN_PIN,
                        GPIO_CAMERA_CMPDN_PIN_M_GPIO,
                        //GPIO_OUT_ONE, //on pdn
                        GPIO_OUT_ZERO,      //on state
                        GPIO_OUT_ONE, //off pdn                        
                    }
                   };
u32 hi351[3][8] = {
                    //for main sensor 
                    {GPIO_CAMERA_CMRST_PIN,
                        GPIO_CAMERA_CMRST_PIN_M_GPIO,   /* mode */
                        //GPIO_OUT_ZERO,
                        GPIO_OUT_ONE,                   /* ON state */
                        GPIO_OUT_ZERO,                  /* OFF state */
                     	GPIO_CAMERA_CMPDN_PIN,
                        GPIO_CAMERA_CMPDN_PIN_M_GPIO,                        
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                    },
                    //for sub sensor 
                    {GPIO_CAMERA_CMRST1_PIN,
                     GPIO_CAMERA_CMRST1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     GPIO_CAMERA_CMPDN1_PIN,
                        GPIO_CAMERA_CMPDN1_PIN_M_GPIO,                        
                        GPIO_OUT_ZERO,
                        GPIO_OUT_ONE,					
                    },
					//for main_2 sensor 
                    {GPIO_CAMERA_CMRST_PIN,
                        GPIO_CAMERA_CMRST_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,                   /* ON state */
                        GPIO_OUT_ZERO,                  /* OFF state */
                     GPIO_CAMERA_CMPDN_PIN,
                        GPIO_CAMERA_CMPDN_PIN_M_GPIO,
                        //GPIO_OUT_ONE, //on pdn
                        GPIO_OUT_ZERO,      //on state
                        GPIO_OUT_ONE, //off pdn                        
                    }
                   };

   if(0 == strcmp("gc0328yuv",currSensorName) || 0 == strcmp("sp0718yuv",currSensorName)){
		pinSet = gc0328_sp0718;
		printk("XXXXXXXXXXXXXXXXXXXXXXXXXXXXX pinSet == gc0328_sp0718 XXXXXXXXXXXXXXXXXX");
   }else
   if (0 == strcmp("hi351yuv",currSensorName)) {
		pinSet = hi351;
		printk("XXXXXXXXXXXXXXXXXXXXXXXXXXXXX pinSet == hi351 XXXXXXXXXXXXXXXXXX");
   }
   else {
		pinSet = hi253_s5k5ca;
		printk("XXXXXXXXXXXXXXXXXXXXXXXXXXXXX pinSet == hi253 && S5K5CA XXXXXXXXXXXXXXXXXX");
   }

    if (DUAL_CAMERA_MAIN_SENSOR == SensorIdx){
        pinSetIdx = 0;
    }
    else if (DUAL_CAMERA_SUB_SENSOR == SensorIdx) {
        pinSetIdx = 1;
    }
    else if (DUAL_CAMERA_MAIN_SECOND_SENSOR == SensorIdx) {
        pinSetIdx = 2;
    }

    //power ON
    if (On) {
		printk("current open camera is:%s\n\r",currSensorName);
       //if(0 == strcmp("s5k5cagxyuv",currSensorName)){//hi253_yuv s5k5cagx_yuv
	if(0 == strcmp("gc0328yuv",currSensorName)){
	   	printk("current open camera is gc0328 sensor\n\r");
       		if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name))//IO VDD
		{
			PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");		            
			goto _kdCISModulePowerOn_exit_;
		} 
		//mdelay(2);
		if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))//AVDD
		{
		      	PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
		      	//return -EIO;
		      	goto _kdCISModulePowerOn_exit_;
		 } 
		//mdelay(2);
	}
	else if(0 == strcmp("sp0718yuv",currSensorName)){
	   	printk("current open camera is sp0718 sensor\n\r");
		if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))//AVDD
		{
		      	PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
		      	//return -EIO;
		      	goto _kdCISModulePowerOn_exit_;
		 } 
		//mdelay(2);
		
       		if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name))//IO VDD
		{
			PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");		            
			goto _kdCISModulePowerOn_exit_;
		} 
		//mdelay(2);
	}else
       if ((0 == strcmp("hi351yuv",currSensorName))){ 
	   		printk("current open camera is hi351 sensor\n\r");
       		if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name))//IO VDD
			{
				PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");		            
				goto _kdCISModulePowerOn_exit_;
			} 
			//mdelay(2);
			if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))//AVDD
			{
		      	PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
		      	//return -EIO;
		      	goto _kdCISModulePowerOn_exit_;
		 	} 
			//mdelay(2);
	   	    if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1200,mode_name))//DVDD
			{//Hi351 DVDD power is 1.2V
				PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");		      
				goto _kdCISModulePowerOn_exit_;
		     }		
       	}else{ //other DVDD power is 1.8V
       		if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1800,mode_name))//DVDD
			 	{
					PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");		      
					goto _kdCISModulePowerOn_exit_;
				}	       	
			if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))//AVDD
			{
		      	PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
		      	//return -EIO;
		      	goto _kdCISModulePowerOn_exit_;
		 	}            
			if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name))//IO VDD
			{
				PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");		            
				goto _kdCISModulePowerOn_exit_;
			}  
		}
		mdelay(2);					    
		 //disable sub sensor
        if (GPIO_CAMERA_INVALID != pinSet[1-pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[1-pinSetIdx][IDX_PS_CMRST],pinSet[1-pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_mode(pinSet[1-pinSetIdx][IDX_PS_CMPDN],pinSet[1-pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[1-pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_dir(pinSet[1-pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMRST],pinSet[1-pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor
            if(mt_set_gpio_out(pinSet[1-pinSetIdx][IDX_PS_CMPDN],pinSet[1-pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module
        }        

        //enable active sensor
        //RST pin
       if(0 == strcmp("gc0328yuv",currSensorName)){
			     //PDN:1 //sensor disable
			     if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
		            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
		            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
			     mdelay(10);
			     if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
       	}
	else if(0 == strcmp("sp0718yuv",currSensorName))
	{
			     if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
		            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
		            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
			     mdelay(10);
			     if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
				mdelay(10);
			     if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
			     mdelay(10);
			     if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
				 mdelay(10);
       	}else{

		
		        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
			     //PDN:1 //sensor disable
			     if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
		            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
		            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}
			     //mdelay(5);
			     
			     //Reset: 0---->1 //reset sensor
		            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
		            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
			     if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_ON])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");}

			     if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
			     //mdelay(15);//20121215
		            mdelay(2);
		            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_ON])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}
		            //mdelay(1);//20121215
		            mdelay(2);
          
		        }
       		}
    }
    else {//power OFF

#if 0 //TODO: depends on HW layout. Should be notified by SA.
        printk("Set GPIO 94 for power OFF\n"); 
        if (mt_set_gpio_pull_enable(GPIO_CAMERA_LDO_EN_PIN, GPIO_PULL_DISABLE)) {PK_DBG("[CAMERA SENSOR] Set GPIO94 PULL DISABLE ! \n"); }
        if(mt_set_gpio_mode(GPIO_CAMERA_LDO_EN_PIN, GPIO_CAMERA_LDO_EN_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
        if(mt_set_gpio_dir(GPIO_CAMERA_LDO_EN_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
        if(mt_set_gpio_out(GPIO_CAMERA_LDO_EN_PIN,GPIO_OUT_ZERO)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");}    	    
#endif

        //PK_DBG("[OFF]sensorIdx:%d \n",SensorIdx);
	if(0 == strcmp("gc0328yuv",currSensorName) || 0 == strcmp("sp0718yuv",currSensorName))
	{
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
    	     if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module
	}
	else
        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor
    	    if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module
        }
		//turn off IO_VDD 1.8V
		    	if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A,mode_name)) {
		            PK_DBG("[CAMERA SENSOR] Fail to OFF analog power\n");            
		            goto _kdCISModulePowerOn_exit_;
		        }				
				//turn off 1.8V
				 if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name)) {
		            PK_DBG("[CAMERA SENSOR] Fail to OFF digital power\n");
		            //return -EIO;
		            goto _kdCISModulePowerOn_exit_;
		        }
				//turn off IO_VDD 1.8V		
				if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2,mode_name))
		        {
		            PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");           
		            goto _kdCISModulePowerOn_exit_;
		        }
			
    }//

	return 0;

_kdCISModulePowerOn_exit_:
    return -EIO;
}

EXPORT_SYMBOL(kdCISModulePowerOn);
#else 
#error Error !! forget to implement power control for image sensor

#endif 



