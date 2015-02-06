#include <cust_leds.h>
#include <mach/mt6575_pwm.h>

#include <linux/kernel.h>
#include <mach/pmic_mt6329_hw_bank1.h> 
#include <mach/pmic_mt6329_sw_bank1.h> 
#include <mach/pmic_mt6329_hw.h>
#include <mach/pmic_mt6329_sw.h>
#include <mach/upmu_common_sw.h>
#include <mach/upmu_hw.h>

extern int mtkfb_set_backlight_level(unsigned int level);
extern int mtkfb_set_backlight_pwm(int div);

unsigned int brightness_mapping(unsigned int level)
{
    unsigned int mapped_level;
    
    mapped_level = level;
       
	return mapped_level;
}

unsigned int Cust_SetBacklight(int level, int div)
{
    mtkfb_set_backlight_pwm(div);
    mtkfb_set_backlight_level(brightness_mapping(level));
    return 0;
}

static struct cust_mt65xx_led cust_led_list[MT65XX_LED_TYPE_TOTAL] = {
	/* < bug: 338591 begin 
	 * added by songlinhua 20121016
	 * reason: 
	 *		fix blink issue by software through GPIO mode when sleep
	 * solution:
	 *		remap gpio led blink control to pwm blink control
	 */	
	//{"red",               MT65XX_LED_MODE_PWM, PWM2},        //20120921 modify by sunjiaojiao for charging led
	//{"green",             MT65XX_LED_MODE_GPIO, 110},//20120921 modify by sunjiaojiao for notify led
	//20130313 modify by sunjiaojiao for pixo start	
	//{"red",               MT65XX_LED_MODE_PWM, PWM2},
	//{"green",             MT65XX_LED_MODE_PWM, PWM2},
	{"red",               MT65XX_LED_MODE_NONE, -1},
	{"green",             MT65XX_LED_MODE_NONE, -1},
	//20130313 modify by sunjiaojiao for pixo end
	/* bug: 338591 end > */
	
	{"blue",              MT65XX_LED_MODE_NONE, -1},
	{"jogball-backlight", MT65XX_LED_MODE_NONE, -1},
	{"keyboard-backlight",MT65XX_LED_MODE_NONE, -1},
	{"button-backlight",  MT65XX_LED_MODE_PMIC, MT65XX_LED_PMIC_BUTTON},
	{"lcd-backlight",     MT65XX_LED_MODE_PMIC, MT65XX_LED_PMIC_LCD_ISINK},
};

struct cust_mt65xx_led *get_cust_led_list(void)
{
	return cust_led_list;
}

