//20121009  modify by sunjiaojiao for green led can not been lighted

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/leds.h>
#include <linux/leds-mt65xx.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include <linux/slab.h>

#include <cust_leds.h>

#if defined (CONFIG_ARCH_MT6573)
#include <mach/mt6573_pwm.h>
#include <mach/mt6573_gpio.h>
#include <mach/pmu6573_sw.h>

#elif defined (CONFIG_ARCH_MT6516)
#include <mach/mt6516_pwm.h>
#include <mach/mt6516_gpio.h>

#elif defined (CONFIG_ARCH_MT6575)
#include <mach/mt6575_pwm.h>
#include <mach/mt6575_gpio.h>
#include <mach/pmic_mt6329_hw_bank1.h> 
#include <mach/pmic_mt6329_sw_bank1.h> 
#include <mach/pmic_mt6329_hw.h>
#include <mach/pmic_mt6329_sw.h>
#include <mach/upmu_common_sw.h>
#include <mach/upmu_hw.h>
#include <mach/mt6575_pmic_feature_api.h>
#include <mach/mt6575_boot.h>
#endif



#define NLED_OFF 0
#define NLED_ON 1
#define NLED_BLINK 2
#define MIN_FRE_OLD_PWM 32 // the min frequence when use old mode pwm by kHz
#define PWM_DIV_NUM 8
#define ERROR_BL_LEVEL 0xFFFFFFFF
struct nled_setting
{
	u8 nled_mode; //0, off; 1, on; 2, blink;
	u32 blink_on_time ;
	u32 blink_off_time;
};
 
struct cust_mt65xx_led* bl_setting = NULL;
unsigned int bl_brightness = 102;
unsigned int bl_duty = 21;
unsigned int bl_div = CLK_DIV1;
unsigned int bl_frequency = 32000;


int time_array[PWM_DIV_NUM]={256,512,1024,2048,4096,8192,16384,32768};
u8 div_array[PWM_DIV_NUM] = {1,2,4,8,16,32,64,128};

static int debug_enable = 1;
#define LEDS_DEBUG(format, args...) do{ \
	if(debug_enable) \
	{\
		printk(KERN_EMERG format,##args);\
	}\
}while(0)

struct mt65xx_led_data {
	struct led_classdev cdev;
	struct cust_mt65xx_led cust;
	struct work_struct work;
	int level;
	int delay_on;
	int delay_off;
};




extern void mt_power_off (U32 pwm_no);
extern S32 mt_set_pwm_disable ( U32 pwm_no ) ;
extern unsigned int brightness_mapping(unsigned int level);
extern int mtkfb_get_backlight_pwm(unsigned int divider, unsigned int *freq);


#if defined (CONFIG_ARCH_MT6573)
extern void upmu_kpled_mode(upmu_kpled_list_enum kpled, upmu_kpled_mode_enum val);
extern void upmu_kpled_enable(upmu_kpled_list_enum kpled, kal_bool enable);

#elif defined (CONFIG_ARCH_MT6516)
/* import functions */
extern void pmic_bl_dim_duty(kal_uint8 duty);
extern ssize_t  mt6326_bl_Enable(void);
extern ssize_t  mt6326_bl_Disable(void);
extern ssize_t mt6326_kpled_Enable(void);
extern ssize_t mt6326_kpled_Disable(void);
extern ssize_t mt6326_kpled_dim_duty_Full(void);
extern ssize_t mt6326_kpled_dim_duty_0(void);

#endif

extern kal_bool upmu_is_chr_det(void); //added by songlinhua 20121016

/* export functions */
int mt65xx_leds_brightness_set(enum mt65xx_led_type type, enum led_brightness level);

static int brightness_set_pmic(enum mt65xx_led_pmic pmic_type, u32 level, u32 div);
static int brightness_set_gpio(int gpio_num, enum led_brightness level);
static int mt65xx_led_set_cust(struct cust_mt65xx_led *cust, int level);
static void mt65xx_led_work(struct work_struct *work);
static void mt65xx_led_set(struct led_classdev *led_cdev, enum led_brightness level);
static int  mt65xx_blink_set(struct led_classdev *led_cdev,
			     unsigned long *delay_on,
			     unsigned long *delay_off);

static struct mt65xx_led_data *g_leds_data[MT65XX_LED_TYPE_TOTAL];
struct wake_lock leds_suspend_lock;
static u32 indicator_led_gpio = 110; //added by songlinhua 20121016
static u32 indicator_blinking = FALSE; //added by songlinhua 20121016
static int brightness_mapto64(int level)
{
        if (level < 30)
                return (level >> 1) + 7;
        else if (level <= 120)
                return (level >> 2) + 14;
        else if (level <= 160)
                return level / 5 + 20;
        else
                return (level >> 3) + 33;
}

int find_time_index(int time)
{	
	int index = 0;	
	while(index < 8)	
	{		
		if(time<time_array[index])			
			return index;		
		else
			index++;
	}	
	return PWM_DIV_NUM-1;
}


static int led_set_pwm(int pwm_num, struct nled_setting* led)
{
	struct pwm_easy_config pwm_setting;
	int time_index = 0;
	pwm_setting.pwm_no = pwm_num;
    
        LEDS_DEBUG("[LED]led_set_pwm: mode=%d,pwm_no=%d\n", led->nled_mode, pwm_num);  
	if((pwm_num != PWM4 && pwm_num != PWM5 && pwm_num != PWM6))
		pwm_setting.clk_src = PWM_CLK_OLD_MODE_32K;
	else
		pwm_setting.clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
    
	switch (led->nled_mode)
	{
		case NLED_OFF :
			pwm_setting.duty = 100; //0  20120921 modify by sunjiaojiao 
			pwm_setting.clk_div = CLK_DIV1;
			pwm_setting.duration = 100;
			break;
            
		case NLED_ON :
			pwm_setting.duty = 0;  //30 20120921 modify by sunjiaojiao 
			pwm_setting.clk_div = CLK_DIV1;			
			pwm_setting.duration = 100;
			break;
            
		case NLED_BLINK :
			LEDS_DEBUG("[LED]LED blink on time = %d offtime = %d\n",led->blink_on_time,led->blink_off_time);
			time_index = find_time_index(led->blink_on_time + led->blink_off_time);
			LEDS_DEBUG("[LED]LED div is %d\n",time_index);
			pwm_setting.clk_div = time_index;
			pwm_setting.duration = (led->blink_on_time + led->blink_off_time) * MIN_FRE_OLD_PWM / div_array[time_index];
			pwm_setting.duty = (led->blink_on_time*100) / (led->blink_on_time + led->blink_off_time);
	}
	pwm_set_easy_config(&pwm_setting);

#if defined (CONFIG_ARCH_MT6573)
	if(pwm_num == PWM7)
	{
		if(led->nled_mode == NLED_OFF)
		{
			LEDS_DEBUG("[LED]Disable MT6573 keypad led\n");
			upmu_kpled_enable(KPLED,0);
		}else
		{
			LEDS_DEBUG("[LED]Enable MT6573 keypad led\n");
			upmu_kpled_mode(KPLED,0);
			upmu_kpled_enable(KPLED,1);
		}		
	}
#endif	

	return 0;
	
}

#if defined (CONFIG_ARCH_MT6575)
#define PMIC_PERIOD_NUM 15
int pmic_period_array[] = {250,500,1000,1250,1666,2000,2500,3333,4000,5000,6666,8000,10000,13333,16000};
u8 pmic_clksel_array[] = {0,0,0,0,0,0,1,1,1,2,2,2,3,3,3};
u8 pmic_freqsel_array[] = {24,26,28,29,30,31,29,30,31,29,30,31,29,30,31};

int find_time_index_pmic(int time_ms) {
	int i;
	for(i=0;i<PMIC_PERIOD_NUM;i++) {
		if(time_ms<=pmic_period_array[i]) {
			return i;
		} else {
			continue;
		}
	}
	return PMIC_PERIOD_NUM-1;
}

static int led_blink_pmic(enum mt65xx_led_pmic pmic_type, struct nled_setting* led) {
	int time_index = 0;
	int duty = 0;
	LEDS_DEBUG("[LED]led_blink_pmic: pmic_type=%d\n", pmic_type);  
	
	if((pmic_type != MT65XX_LED_PMIC_NLED_ISINK4 && pmic_type!= MT65XX_LED_PMIC_NLED_ISINK5) || led->nled_mode != NLED_BLINK) {
		return -1;
	}
				
	LEDS_DEBUG("[LED]LED blink on time = %d offtime = %d\n",led->blink_on_time,led->blink_off_time);
	time_index = find_time_index_pmic(led->blink_on_time + led->blink_off_time);
	LEDS_DEBUG("[LED]LED index is %d clksel=%d freqsel=%d\n", time_index, pmic_clksel_array[time_index], pmic_freqsel_array[time_index]);
	duty=32*led->blink_on_time/(led->blink_on_time + led->blink_off_time);
	switch(pmic_type){
		case MT65XX_LED_PMIC_NLED_ISINK4:
			upmu_isinks_ch4_mode(PMIC_PWM_1);
			upmu_isinks_ch4_step(0x3);
			upmu_isinks_ch4_cabc_en(0);
			upmu_isinks_dim1_duty(duty);
			upmu_isinks_dim1_fsel(pmic_freqsel_array[time_index]);
			pmic_bank1_config_interface(0x2e, pmic_clksel_array[time_index], 0x3, 0x6);
			upmu_top2_bst_drv_ck_pdn(0x0);
			hwBacklightISINKTurnOn(MT65XX_LED_PMIC_NLED_ISINK4);
		break;
		case MT65XX_LED_PMIC_NLED_ISINK5:
			upmu_isinks_ch5_mode(PMIC_PWM_2);
			upmu_isinks_ch5_step(0x3);
			upmu_isinks_ch5_cabc_en(0);
			upmu_isinks_dim2_duty(duty);
			upmu_isinks_dim2_fsel(pmic_freqsel_array[time_index]);
			pmic_bank1_config_interface(0x30, pmic_clksel_array[time_index], 0x3, 0x6);
			upmu_top2_bst_drv_ck_pdn(0x0);
			hwBacklightISINKTurnOn(MT65XX_LED_PMIC_NLED_ISINK5);
		break;
	}
			
	return 0;
}
#endif

static int backlight_set_pwm(int pwm_num, u32 level, u32 div)
{
	struct pwm_spec_config pwm_setting;
	pwm_setting.pwm_no = pwm_num;
	pwm_setting.mode = PWM_MODE_FIFO; //new mode fifo and periodical mode
	pwm_setting.clk_div = div;
	pwm_setting.clk_src = PWM_CLK_NEW_MODE_BLOCK;
	
	pwm_setting.PWM_MODE_FIFO_REGS.IDLE_VALUE = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.GUARD_VALUE = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 63;
	pwm_setting.PWM_MODE_FIFO_REGS.HDURATION = 4;
	pwm_setting.PWM_MODE_FIFO_REGS.LDURATION = 4;
	pwm_setting.PWM_MODE_FIFO_REGS.GDURATION = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;
	
	LEDS_DEBUG("[LED]backlight_set_pwm:duty is %d\n", level);
    if(level>0 && level <= 32)
	{
		pwm_setting.PWM_MODE_FIFO_REGS.SEND_DATA0 =  (1 << level) - 1 ;
		pwm_setting.PWM_MODE_FIFO_REGS.SEND_DATA1 = 0 ;
		pwm_set_spec_config(&pwm_setting);
	}else if(level>32 && level <=64)
	{
		level -= 32;
		pwm_setting.PWM_MODE_FIFO_REGS.SEND_DATA0 =  0xFFFFFFFF ;
		pwm_setting.PWM_MODE_FIFO_REGS.SEND_DATA1 = (1 << level) - 1;
		pwm_set_spec_config(&pwm_setting);
	}else
	{
		LEDS_DEBUG("[LED]Error level in backlight\n");
		mt_set_pwm_disable(pwm_setting.pwm_no);
		mt_power_off(pwm_setting.pwm_no);
	}
	
	//printk("[LED]PWM con register is %x \n", INREG32(PWM_BASE + 0x0150));
	return 0;

}



#if defined (CONFIG_ARCH_MT6516)
/* import functions */
static int brightness_set_pmic(enum mt65xx_led_pmic pmic_type, u32 level, u32 div)
{
	int tmp_level = level;
	LEDS_DEBUG("[LED]PMIC#%d:%d\n", pmic_type, level);

	if (pmic_type == MT65XX_LED_PMIC_LCD) {
		if (level) 
		{
			level = brightness_mapping(tmp_level);
			if(level == ERROR_BL_LEVEL)
				level = tmp_level<8?1:tmp_level/8;
			mt6326_bl_Enable();
			pmic_bl_dim_duty(level);
		}
		else {
			mt6326_bl_Disable();
		}
		return 0;
	}
	else if (pmic_type == MT65XX_LED_PMIC_BUTTON) {
		if (level) {
			mt6326_kpled_dim_duty_Full();
			mt6326_kpled_Enable();
		}
		else {
			mt6326_kpled_dim_duty_0();
			mt6326_kpled_Disable();
		}
		return 0;
	}

	return -1;
}


#elif  defined (CONFIG_ARCH_MT6573)
static int brightness_set_pmic(enum mt65xx_led_pmic pmic_type, u32 level, u32 div)
{
	return 0;
}


#elif  defined (CONFIG_ARCH_MT6575)
static int brightness_set_pmic(enum mt65xx_led_pmic pmic_type, u32 level, u32 div)
{
	int tmp_level = level;
	static bool backlight_init_flag[2] = {false, false};
	static bool led_init_flag[2] = {false, false};
	
	LEDS_DEBUG("[LED]PMIC#%d:%d\n", pmic_type, level);

	if (pmic_type == MT65XX_LED_PMIC_LCD_ISINK)
	{
		if(backlight_init_flag[0] == false)
		{
			//hwBacklightISINKTuning(1, PMIC_PWM_0, 0x3, 0);
			//hwBacklightISINKTuning(2, PMIC_PWM_0, 0x3, 0);
			//hwBacklightISINKTuning(3, PMIC_PWM_0, 0x3, 0);
         
                    // added by zhuqiang for PR418446 2013.3.13
                     hwBacklightISINKTuning(0, 3, 0x4, 0);
                     hwBacklightISINKTuning(1, 3, 0x4, 0);
                     hwBacklightISINKTuning(2, 3, 0x4, 0);
                     hwBacklightISINKTuning(3, 3, 0x4, 0);
                     hwBacklightISINKTuning(4, 3, 0x4, 0);
                     hwBacklightISINKTuning(5, 3, 0x4, 0);

			backlight_init_flag[0] = true;
		}
		
		if (level) 
		{
			level = brightness_mapping(tmp_level);
			if(level == ERROR_BL_LEVEL)
				level = tmp_level/17;
			hwPWMsetting(PMIC_PWM_0, level, div);
			upmu_top2_bst_drv_ck_pdn(0x0);

                     // added by zhuqiang for PR433867 2013.4.2
                        if(level<50)
				tmp_level=0x00;
			else if(level<100)
				tmp_level=0x01;
			else if(level<150)
				tmp_level=0x02;
			else if(level<200)
				tmp_level=0x03;
			else if(level<256)
				tmp_level=0x04;
                     
                     hwBacklightISINKTuning(0, 3, tmp_level, 0);
                     hwBacklightISINKTuning(1, 3, tmp_level, 0);
                     hwBacklightISINKTuning(2, 3, tmp_level, 0);
                     hwBacklightISINKTuning(3, 3, tmp_level, 0);
                     hwBacklightISINKTuning(4, 3, tmp_level, 0);
                     hwBacklightISINKTuning(5, 3, tmp_level, 0);
                     // added by zhuqiang for PR418446 2013.3.13
                     hwBacklightISINKTurnOn(0);

			hwBacklightISINKTurnOn(1);
			hwBacklightISINKTurnOn(2);
			hwBacklightISINKTurnOn(3);
                       
                    // added by zhuqiang for PR418446 2013.3.13
                     hwBacklightISINKTurnOn(4);
                     hwBacklightISINKTurnOn(5);
			bl_duty = level;	
		}
		else 
		{
                    // added by zhuqiang for PR418446 2013.3.13
                     hwBacklightISINKTurnOff(0);

			hwBacklightISINKTurnOff(1);
			hwBacklightISINKTurnOff(2);
			hwBacklightISINKTurnOff(3);

                    // added by zhuqiang for PR418446 2013.3.13
                     hwBacklightISINKTurnOff(4);
                     hwBacklightISINKTurnOff(5);
			bl_duty = level;	
		}
		return 0;
	}
	else if(pmic_type == MT65XX_LED_PMIC_LCD_BOOST)
	{
		/*
		if(backlight_init_flag[1] == false)
		{
			hwBacklightBoostTuning(PMIC_PWM_0, 0xC, 0);
			backlight_init_flag[1] = true;
		}
		*/		
		if (level) 
		{
			level = brightness_mapping(tmp_level);
			if(level == ERROR_BL_LEVEL)
				level = tmp_level/42;
	
			upmu_boost_isink_hw_sel(0x1);
			upmu_boost_mode(3);
			upmu_boost_cabc_en(0);

			switch(level)
			{
				case 0:				
					upmu_boost_vrsel(0x0);
					//hwPWMsetting(PMIC_PWM_0, 0, 0x15);
					break;
				case 1:
					upmu_boost_vrsel(0x1);
					//hwPWMsetting(PMIC_PWM_0, 4, 0x15);
					break;
				case 2:
					upmu_boost_vrsel(0x2);
					//hwPWMsetting(PMIC_PWM_0, 5, 0x15);
					break;
				case 3:
					upmu_boost_vrsel(0x3);
					//hwPWMsetting(PMIC_PWM_0, 6, 0x15);
					break;
				case 4:
					upmu_boost_vrsel(0x5);
					//hwPWMsetting(PMIC_PWM_0, 7, 0x15);
					break;
				case 5:
					upmu_boost_vrsel(0x8);
					//hwPWMsetting(PMIC_PWM_0, 8, 0x15);
					break;
				case 6:
					upmu_boost_vrsel(0xB);
					//hwPWMsetting(PMIC_PWM_0, 9, 0x15);
					break;
				default:
					printk("[LED] invalid brightness level %d->%d\n", tmp_level, level);
					break;
			}
			
			upmu_top2_bst_drv_ck_pdn(0x0);
			upmu_boost_en(0x1);
			bl_duty = level;	
		}
		else 
		{
			upmu_boost_en(0x0);
			bl_duty = level;	
			//upmu_top2_bst_drv_ck_pdn(0x1);
		}
		return 0;
	}
	else if (pmic_type == MT65XX_LED_PMIC_BUTTON) 
	{
		if (level) 
		{
			hwBacklightKPLEDTuning(0x9, 0x0);
			hwBacklightKPLEDTurnOn();
		}
		else 
		{
			hwBacklightKPLEDTurnOff();
		}
		return 0;
	}
	else if(pmic_type == MT65XX_LED_PMIC_NLED_ISINK4)
	{
		//if(led_init_flag[0] == false)
		{
			hwBacklightISINKTuning(MT65XX_LED_PMIC_NLED_ISINK4, PMIC_PWM_1, 0x3, 0);
			hwPWMsetting(PMIC_PWM_1, 31, 8);
			led_init_flag[0] = true;
		}
		
		if (level) 
		{
			upmu_top2_bst_drv_ck_pdn(0x0);
			hwBacklightISINKTurnOn(MT65XX_LED_PMIC_NLED_ISINK4);
		}
		else 
		{
			hwBacklightISINKTurnOff(MT65XX_LED_PMIC_NLED_ISINK4);
			//upmu_top2_bst_drv_ck_pdn(0x1);
		}
		return 0;
	}
	else if(pmic_type == MT65XX_LED_PMIC_NLED_ISINK5)
	{
		//if(led_init_flag[1] == false)
		{
			hwBacklightISINKTuning(MT65XX_LED_PMIC_NLED_ISINK5, PMIC_PWM_2, 0x3, 0);
			hwPWMsetting(PMIC_PWM_2, 31, 8);
			led_init_flag[1] = true;
		}
		if (level) 
		{
			upmu_top2_bst_drv_ck_pdn(0x0);
			hwBacklightISINKTurnOn(MT65XX_LED_PMIC_NLED_ISINK5);
		}
		else 
		{
			hwBacklightISINKTurnOff(MT65XX_LED_PMIC_NLED_ISINK5);
			//upmu_top2_bst_drv_ck_pdn(0x1);
		}
		return 0;
	}
	
	return -1;
}

#endif



static int brightness_set_gpio(int gpio_num, enum led_brightness level)
{
        struct nled_setting setting = {0,0,0};                           //20121009 add by sunjiaojiao 
        
	LEDS_DEBUG("[LED]GPIO#%d:%d\n", gpio_num, level);
	mt_set_gpio_mode(gpio_num, GPIO_MODE_GPIO);
	mt_set_gpio_dir(gpio_num, GPIO_DIR_OUT);

	if (level) {
		setting.nled_mode = NLED_ON;
		mt_set_gpio_out(gpio_num, GPIO_OUT_ONE);
	}
	else {
		setting.nled_mode =NLED_OFF;
		mt_set_gpio_out(gpio_num, GPIO_OUT_ZERO);
	}
	led_set_pwm(PWM2,&setting);  //20121009 add by sunjiaojiao
	return 0;
}

static int mt65xx_led_set_cust(struct cust_mt65xx_led *cust, int level)
{
	struct nled_setting led_tmp_setting = {0,0,0};
	int tmp_level = level;
	if (level > LED_FULL)
		level = LED_FULL;
	else if (level < 0)
		level = 0;

    printk("mt65xx_leds_set_cust: set brightness, name:%s, mode:%d, level:%d\n", 
		cust->name, cust->mode, level);
	switch (cust->mode) {
		case MT65XX_LED_MODE_PWM:
			if(strcmp(cust->name,"lcd-backlight") == 0)
			{
				bl_brightness = level;
				if(level == 0)
				{
					mt_set_pwm_disable(cust->data);
					mt_power_off (cust->data);
				}else
				{
                    //Removed by ygm for beetle lite PWM3 backlight control. 20121005.
					//level = brightness_mapping(tmp_level);
					//if(level == ERROR_BL_LEVEL)
						level = brightness_mapto64(tmp_level);
						
					backlight_set_pwm(cust->data, level, bl_div);
				}
                bl_duty = level;	
				
			}else
			{
				/* < bug: 338591 begin 
				 * added by songlinhua 20121016
				 * reason: 
				 *		fix blink issue by software through GPIO mode when sleep
				 * solution:
				 *		remap gpio led blink control to pwm blink control
				 */
				if (!strcmp(cust->name, "green")
					||(!strcmp(cust->name, "red") && (upmu_is_chr_det() == KAL_FALSE)))
				{
					 // if green(red) led is blinking, skip shutdown  red(green) led since they 
					 // are supplied by the same power source
					if (indicator_blinking && !level)
						return 1;
					mt_set_gpio_mode(indicator_led_gpio, GPIO_MODE_GPIO);
					mt_set_gpio_dir(indicator_led_gpio, GPIO_DIR_OUT);
					if(level == 0)
						mt_set_gpio_out(indicator_led_gpio, GPIO_OUT_ZERO);
					else
						mt_set_gpio_out(indicator_led_gpio, GPIO_OUT_ONE);
					//brightness_set_gpio(indicator_led_gpio, level);
				}
				/* bug: 338591 begin end > */
				
				if(level == 0)
				{
					led_tmp_setting.nled_mode = NLED_OFF;
				}else
				{
					led_tmp_setting.nled_mode = NLED_ON;
				}
				led_set_pwm(cust->data,&led_tmp_setting);
			}
			return 1;
            
		case MT65XX_LED_MODE_GPIO:
			return brightness_set_gpio(cust->data, level);
            
		case MT65XX_LED_MODE_PMIC:
			return brightness_set_pmic(cust->data, level, bl_div);
            
		case MT65XX_LED_MODE_CUST:
            if(strcmp(cust->name,"lcd-backlight") == 0)
			{
			    bl_brightness = level;
            }
			return ((cust_brightness_set)(cust->data))(level, bl_div);
            
		case MT65XX_LED_MODE_NONE:
		default:
			break;
	}
	return -1;
}

static void mt65xx_led_work(struct work_struct *work)
{
	struct mt65xx_led_data *led_data =
		container_of(work, struct mt65xx_led_data, work);

	LEDS_DEBUG("[LED]%s:%d\n", led_data->cust.name, led_data->level);

	mt65xx_led_set_cust(&led_data->cust, led_data->level);
}

static void mt65xx_led_set(struct led_classdev *led_cdev, enum led_brightness level)
{
	struct mt65xx_led_data *led_data =
		container_of(led_cdev, struct mt65xx_led_data, cdev);

	// do something only when level is changed
	if (led_data->level != level) {
		led_data->level = level;
		if(strcmp(led_data->cust.name,"lcd-backlight"))
		{
				schedule_work(&led_data->work);
		}else
		{
				LEDS_DEBUG("[LED]Set Backlight directly %d at time %lu\n",led_data->level,jiffies);
				mt65xx_led_set_cust(&led_data->cust, led_data->level);	
		}
	}
}

static int  mt65xx_blink_set(struct led_classdev *led_cdev,
			     unsigned long *delay_on,
			     unsigned long *delay_off)
{
	struct mt65xx_led_data *led_data =
		container_of(led_cdev, struct mt65xx_led_data, cdev);
	static int got_wake_lock = 0;
	struct nled_setting nled_tmp_setting = {0,0,0};

	// only allow software blink when delay_on or delay_off changed
	if (*delay_on != led_data->delay_on || *delay_off != led_data->delay_off) {

		/* < bug: 338591 begin 
		 * added by songlinhua 20121016
		 * reason: 
		 *		fix blink issue by software through GPIO mode when sleep 
		 * solution:
		 *		remap gpio led blink control to pwm blink control
		 */
		/* second modified by songlinhua 20121204, fix problem that indicator led
                 * can not be turned off after doing a whole charging process from 
                 * low battery to full capacity
                 */
		led_data->delay_on = *delay_off;
		led_data->delay_off = *delay_on;
		indicator_blinking = (led_data->delay_on && led_data->delay_off) ? TRUE : FALSE; 
		LEDS_DEBUG("[LED]PWM#%d led blink %s\n", led_data ->cust.data + 1, 
			indicator_blinking ? "enabled" : "disabled");
		if (!strcmp(led_data->cust.name, "green")
			||(!strcmp(led_data->cust.name, "red") && (upmu_is_chr_det() == KAL_FALSE)))
		{
			brightness_set_gpio(indicator_led_gpio, indicator_blinking ? LED_FULL: LED_OFF);
		}
		/* bug: 338591 end > */
		
		if (led_data->delay_on && led_data->delay_off) { // enable blink
			if(led_data->cust.mode == MT65XX_LED_MODE_PWM && 
			(led_data->cust.data != PWM4 && led_data->cust.data != PWM5 && led_data->cust.data != PWM6))
			{
				nled_tmp_setting.nled_mode = NLED_BLINK;
				nled_tmp_setting.blink_off_time = led_data->delay_off;
				nled_tmp_setting.blink_on_time = led_data->delay_on;
				led_set_pwm(led_data->cust.data,&nled_tmp_setting);
				return 0;
			}
			
			#if defined (CONFIG_ARCH_MT6575)
			else if((led_data->cust.mode == MT65XX_LED_MODE_PMIC) && (led_data->cust.data == MT65XX_LED_PMIC_NLED_ISINK4
				|| led_data->cust.data == MT65XX_LED_PMIC_NLED_ISINK5))
			{
				if(get_chip_eco_ver() == CHIP_E2) {
					nled_tmp_setting.nled_mode = NLED_BLINK;
					nled_tmp_setting.blink_off_time = led_data->delay_off;
					nled_tmp_setting.blink_on_time = led_data->delay_on;
					led_blink_pmic(led_data->cust.data, &nled_tmp_setting);
					return 0;
				} else {
					wake_lock(&leds_suspend_lock);
				}
			}
			#endif		
			
			else if (!got_wake_lock) {
				wake_lock(&leds_suspend_lock);
				got_wake_lock = 1;
			}
		}
		else if (!led_data->delay_on && !led_data->delay_off) { // disable blink
			if(led_data->cust.mode == MT65XX_LED_MODE_PWM && 
			(led_data->cust.data != PWM4 && led_data->cust.data != PWM5 && led_data->cust.data != PWM6))
			{
				nled_tmp_setting.nled_mode = NLED_OFF;
				led_set_pwm(led_data->cust.data,&nled_tmp_setting);
				return 0;
			}
			
			#if defined (CONFIG_ARCH_MT6575)
			else if((led_data->cust.mode == MT65XX_LED_MODE_PMIC) && (led_data->cust.data == MT65XX_LED_PMIC_NLED_ISINK4
				|| led_data->cust.data == MT65XX_LED_PMIC_NLED_ISINK5))
			{
				if(get_chip_eco_ver() == CHIP_E2) {
					brightness_set_pmic(led_data->cust.data, 0, 0);
					return 0;
				} else {
					wake_unlock(&leds_suspend_lock);
				}
			}
			#endif	
			
			else if (got_wake_lock) {
				wake_unlock(&leds_suspend_lock);
				got_wake_lock = 0;
			}
		}
		return -1;
	}

	// delay_on and delay_off are not changed
	return 0;
}

int mt65xx_leds_brightness_set(enum mt65xx_led_type type, enum led_brightness level)
{
	struct cust_mt65xx_led *cust_led_list = get_cust_led_list();

	LEDS_DEBUG("[LED]#%d:%d\n", type, level);

	if (type < 0 || type >= MT65XX_LED_TYPE_TOTAL)
		return -1;

	if (level > LED_FULL)
		level = LED_FULL;
	else if (level < 0)
		level = 0;

	return mt65xx_led_set_cust(&cust_led_list[type], level);

}

EXPORT_SYMBOL(mt65xx_leds_brightness_set);

/* added by songlinhua 20121016 for retriving led blinking status */
int is_led_blinking(void)
{
	if (indicator_blinking)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(is_led_blinking);

static ssize_t show_duty(struct device *dev,struct device_attribute *attr, char *buf)
{
	LEDS_DEBUG("[LED]get backlight duty value is:%d \n",bl_duty);
	return sprintf(buf, "%u\n", bl_duty);
}
static ssize_t store_duty(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	char *pvalue = NULL;
	unsigned int level = 0;
	size_t count = 0;
	LEDS_DEBUG("set backlight duty start \n");
	level = simple_strtoul(buf,&pvalue,10);
	count = pvalue - buf;
	if (*pvalue && isspace(*pvalue))
		count++;
    
	if(count == size)
	{
		if(bl_setting->mode == MT65XX_LED_MODE_PMIC)
		{
#if defined (CONFIG_ARCH_MT6573)	

#elif defined (CONFIG_ARCH_MT6516)
			if (level) //duty:0-31
			{
				mt6326_bl_Enable();
				pmic_bl_dim_duty(level);
				
			}else 
			{
				mt6326_bl_Disable();
			}
			
#elif defined (CONFIG_ARCH_MT6575)
			//duty:0-16
			if((level >= 0) && (level <= 15))
			{
				brightness_set_pmic(MT65XX_LED_PMIC_LCD_BOOST, (level*17), bl_div);
			}
			else
			{
				LEDS_DEBUG("duty value is error, please select vaule from [0-15]!\n");
			}
#endif			
		}
		else if(bl_setting->mode == MT65XX_LED_MODE_PWM)
		{
			if(level == 0)
			{
				mt_set_pwm_disable(bl_setting->data);
				mt_power_off (bl_setting->data);
			}else if(level <= 64)
			{
				backlight_set_pwm(bl_setting->data,level, bl_div);
			}	
		}
        
		bl_duty = level;

	}
	
	return size;
}


static DEVICE_ATTR(duty, 0664, show_duty, store_duty);


static ssize_t show_div(struct device *dev,struct device_attribute *attr, char *buf)
{
	LEDS_DEBUG("get backlight div value is:%d \n",bl_div);
	return sprintf(buf, "%u\n", bl_div);
}

static ssize_t store_div(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	char *pvalue = NULL;
	unsigned int div = 0;
	size_t count = 0;
	LEDS_DEBUG("set backlight div start \n");
	div = simple_strtoul(buf,&pvalue,10);
	count = pvalue - buf;
	
	if (*pvalue && isspace(*pvalue))
		count++;
		
	if(count == size)
	{
                if(div < 0 || (div > 7))
		{
            LEDS_DEBUG("set backlight div parameter error: %d[div:0~7]\n", div);
            return 0;
		}
        
		if(bl_setting->mode == MT65XX_LED_MODE_PWM)
		{
            LEDS_DEBUG("set PWM backlight div OK: div=%d, duty=%d\n", div, bl_duty);
            backlight_set_pwm(bl_setting->data, bl_duty, div);
		}
        else if(bl_setting->mode == MT65XX_LED_MODE_CUST)
        {
            LEDS_DEBUG("set cust backlight div OK: div=%d, brightness=%d\n", div, bl_brightness);
	        ((cust_brightness_set)(bl_setting->data))(bl_brightness, div);
        }
        
		bl_div = div;
	}
	
	return size;
}

static DEVICE_ATTR(div, 0664, show_div, store_div);


static ssize_t show_frequency(struct device *dev,struct device_attribute *attr, char *buf)
{
    if(bl_setting->mode == MT65XX_LED_MODE_PWM)
    {
        bl_frequency = 32000/div_array[bl_div];
    }
    else if(bl_setting->mode == MT65XX_LED_MODE_CUST)
    {
        mtkfb_get_backlight_pwm(bl_div, &bl_frequency);  		
    }

    LEDS_DEBUG("[LED]get backlight PWM frequency value is:%d \n", bl_frequency);
    
	return sprintf(buf, "%u\n", bl_frequency);
}

static DEVICE_ATTR(frequency, 0664, show_frequency, NULL);



static ssize_t store_pwm_register(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	char *pvalue = NULL;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;
	if(buf != NULL && size != 0)
	{
		LEDS_DEBUG("store_pwm_register: size:%d,address:0x%s\n", size,buf);
		reg_address = simple_strtoul(buf,&pvalue,16);
	
		if(*pvalue && (*pvalue == '#'))
		{
			reg_value = simple_strtoul((pvalue+1),NULL,16);
			LEDS_DEBUG("set pwm register:[0x%x]= 0x%x\n",reg_address,reg_value);
			OUTREG32(reg_address,reg_value);
			
		}else if(*pvalue && (*pvalue == '@'))
		{
			LEDS_DEBUG("get pwm register:[0x%x]=0x%x\n",reg_address,INREG32(reg_address));
		}	
	}

	return size;
}

static ssize_t show_pwm_register(struct device *dev,struct device_attribute *attr, char *buf)
{
	return 0;
}

static DEVICE_ATTR(pwm_register, 0664, show_pwm_register, store_pwm_register);


static int __init mt65xx_leds_probe(struct platform_device *pdev)
{
	int i;
	int ret, rc;
	struct cust_mt65xx_led *cust_led_list = get_cust_led_list();
	LEDS_DEBUG("[LED]%s\n", __func__);

	for (i = 0; i < MT65XX_LED_TYPE_TOTAL; i++) {
		if (cust_led_list[i].mode == MT65XX_LED_MODE_NONE) {
			g_leds_data[i] = NULL;
			continue;
		}

		g_leds_data[i] = kzalloc(sizeof(struct mt65xx_led_data), GFP_KERNEL);
		if (!g_leds_data[i]) {
			ret = -ENOMEM;
			goto err;
		}

		g_leds_data[i]->cust.mode = cust_led_list[i].mode;
		g_leds_data[i]->cust.data = cust_led_list[i].data;
		g_leds_data[i]->cust.name = cust_led_list[i].name;

		g_leds_data[i]->cdev.name = cust_led_list[i].name;

		g_leds_data[i]->cdev.brightness_set = mt65xx_led_set;
		g_leds_data[i]->cdev.blink_set = mt65xx_blink_set;

		INIT_WORK(&g_leds_data[i]->work, mt65xx_led_work);

		ret = led_classdev_register(&pdev->dev, &g_leds_data[i]->cdev);
        
		if(strcmp(g_leds_data[i]->cdev.name,"lcd-backlight") == 0)
		{
			rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_duty);
            if(rc)
            {
                LEDS_DEBUG("[LED]device_create_file duty fail!\n");
            }
            
            rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_div);
            if(rc)
            {
                LEDS_DEBUG("[LED]device_create_file duty fail!\n");
            }
            
            rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_frequency);
            if(rc)
            {
                LEDS_DEBUG("[LED]device_create_file duty fail!\n");
            }
            
	    rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_pwm_register);
            if(rc)
            {
                LEDS_DEBUG("[LED]device_create_file duty fail!\n");
            }
			bl_setting = &g_leds_data[i]->cust;
		}

		if (ret)
			goto err;
		
	}


	return 0;

err:
	if (i) {
		for (i = i-1; i >=0; i--) {
			if (!g_leds_data[i])
				continue;
			led_classdev_unregister(&g_leds_data[i]->cdev);
			cancel_work_sync(&g_leds_data[i]->work);
			kfree(g_leds_data[i]);
			g_leds_data[i] = NULL;
		}
	}

	return ret;
}

static int mt65xx_leds_remove(struct platform_device *pdev)
{
	int i;
	for (i = 0; i < MT65XX_LED_TYPE_TOTAL; i++) {
		if (!g_leds_data[i])
			continue;
		led_classdev_unregister(&g_leds_data[i]->cdev);
		cancel_work_sync(&g_leds_data[i]->work);
		kfree(g_leds_data[i]);
		g_leds_data[i] = NULL;
	}

	return 0;
}


static void mt65xx_leds_shutdown(struct platform_device *pdev)
{
	int i;
    struct nled_setting led_tmp_setting = {NLED_OFF,0,0};
    
    LEDS_DEBUG("[LED]%s\n", __func__);
    printk("[LED]mt65xx_leds_shutdown: turn off backlight\n");
    
	for (i = 0; i < MT65XX_LED_TYPE_TOTAL; i++) {
		if (!g_leds_data[i])
			continue;
		switch (g_leds_data[i]->cust.mode) {
		    case MT65XX_LED_MODE_PWM:
			    if(strcmp(g_leds_data[i]->cust.name,"lcd-backlight") == 0)
			    {
					mt_set_pwm_disable(g_leds_data[i]->cust.data);
					mt_power_off (g_leds_data[i]->cust.data);
			    }else
			    {
				    led_set_pwm(g_leds_data[i]->cust.data,&led_tmp_setting);
			    }
                break;
                
		    case MT65XX_LED_MODE_GPIO:
			    brightness_set_gpio(g_leds_data[i]->cust.data, 0);
                break;
                
		    case MT65XX_LED_MODE_PMIC:
			    brightness_set_pmic(g_leds_data[i]->cust.data, 0, 0);
                break;
                
		    case MT65XX_LED_MODE_CUST:
			    ((cust_brightness_set)(g_leds_data[i]->cust.data))(0, bl_div);
                break;
                
		    case MT65XX_LED_MODE_NONE:
		    default:
			    break;
          }
	}

}


static struct platform_driver mt65xx_leds_driver = {
	.driver		= {
		.name	= "leds-mt65xx",
		.owner	= THIS_MODULE,
	},
	.probe		= mt65xx_leds_probe,
	.remove		= mt65xx_leds_remove,
	//.suspend	= mt65xx_leds_suspend,
	.shutdown   = mt65xx_leds_shutdown,
};

#if 0
static struct platform_device mt65xx_leds_device = {
	.name = "leds-mt65xx",
	.id = -1
};
#endif

static int __init mt65xx_leds_init(void)
{
	int ret;

	LEDS_DEBUG("[LED]%s\n", __func__);

#if 0
	ret = platform_device_register(&mt65xx_leds_device);
	if (ret)
		printk("[LED]mt65xx_leds_init:dev:E%d\n", ret);
#endif
	ret = platform_driver_register(&mt65xx_leds_driver);

	if (ret)
	{
		printk("[LED]mt65xx_leds_init:drv:E%d\n", ret);
//		platform_device_unregister(&mt65xx_leds_device);
		return ret;
	}

	wake_lock_init(&leds_suspend_lock, WAKE_LOCK_SUSPEND, "leds wakelock");

	return ret;
}

static void __exit mt65xx_leds_exit(void)
{
	platform_driver_unregister(&mt65xx_leds_driver);
//	platform_device_unregister(&mt65xx_leds_device);
}

module_param(debug_enable, int,0644);

module_init(mt65xx_leds_init);
module_exit(mt65xx_leds_exit);

MODULE_AUTHOR("MediaTek Inc.");
MODULE_DESCRIPTION("LED driver for MediaTek MT65xx chip");
MODULE_LICENSE("GPL");
MODULE_ALIAS("leds-mt65xx");

