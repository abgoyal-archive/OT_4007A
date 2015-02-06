#include <asm/arch/custom/inc/cust_leds.h>
#include <asm/arch/mt6575_pwm.h>

extern int DISP_SetBacklight(int level);

static struct cust_mt65xx_led cust_led_list[MT65XX_LED_TYPE_TOTAL] = {
	//20130313 modify by sunjiaojiao for pixo start	
 	//{"red",               MT65XX_LED_MODE_PWM, PWM2},//20120921 modify by sunjiaojiao for charge led
	{"red",               MT65XX_LED_MODE_NONE, -1},
	//20130313 modify by sunjiaojiao for pixo end	
	{"green",             MT65XX_LED_MODE_NONE, -1},
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
