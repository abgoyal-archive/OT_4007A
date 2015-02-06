
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <linux/aee.h>
#include <linux/xlog.h>

#include "mach/irqs.h"
#include "mach/mt6575_auxadc_hw.h"
#include "mach/mt6575_thermal.h"
#include "mach/mt6575_clock_manager.h"
#include "mach/mt6575_boot.h"

#define THERMAL_NAME    "mtk-thermal"

#define RETRY_TIMES     0x0005

#define thermal_dprintk(fmt, args...)           \
do {                                            \
    if (thermal_debug_mode) {                   \
        xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", fmt, ##args);                    \
    }                                           \
} while(0)

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend thermal_early_suspend_handler = 
{
    .level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN,    
    .suspend = NULL,
    .resume  = NULL,
};
#endif

static struct hrtimer thermal_timer;

struct task_struct *thermal_thread = NULL;

static DECLARE_WAIT_QUEUE_HEAD(thermal_timer_waiter);
static DECLARE_WAIT_QUEUE_HEAD(thermal_interrupt_waiter);

static int thermal_interrupt_flag = 0;
static int thermal_timer_flag = 0;

//static int thermal_period = 30;
static int thermal_period = 3;

#define MS_TO_NS(x) (x * 1000 * 1000)
unsigned long delay_in_ms = 500; //ms, suggest from MH

int T_HIGH_LIMIT = 1250; // 125.0degC

static bool thermal_debug_mode = false;
static bool thermal_period_mode = true;
static bool thermal_keep_on_mode = true;
static bool thermal_keep_on_mode_sleep= false;

enum hrtimer_restart thermal_timer_func(struct hrtimer *timer)
{
    if (thermal_debug_mode)
    {
        thermal_dprintk("[%s]: enter timer function\n", __FUNCTION__);
    }
    
    thermal_timer_flag = 1; wake_up_interruptible(&thermal_timer_waiter);
    
    return HRTIMER_NORESTART;
}

static irqreturn_t thermal_interrupt_handler(void)
{
    kal_uint32 temp = 0, ret = 0;
    ret = DRV_Reg32(TEMPMONINTSTS);
    
    thermal_dprintk("[%s]: interrupt trigger, status = 0x%x\n", __FUNCTION__, ret);
    
    if (ret & THERMAL_MON_CINTSTS0)
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[%s]: thermal sensor point 0 - cold interrupt trigger\n", __FUNCTION__);
    if (ret & THERMAL_MON_HINTSTS0)
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[%s]: thermal sensor point 0 - hot interrupt trigger\n", __FUNCTION__);
    if (ret & THERMAL_MON_LOINTSTS0)
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[%s]: thermal sensor point 0 - low offset interrupt trigger\n", __FUNCTION__);
    if (ret & THERMAL_MON_HOINTSTS0)
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[%s]: thermal sensor point 0 - high offset interrupt trigger\n", __FUNCTION__);
    if (ret & THERMAL_MON_NHINTSTS0)
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[%s]: thermal sensor point 0 - hot to normal interrupt trigger\n", __FUNCTION__);
    
    if (ret & THERMAL_MON_CINTSTS1)
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[%s]: thermal sensor point 1 - cold interrupt trigger\n", __FUNCTION__);
    if (ret & THERMAL_MON_HINTSTS1)
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[%s]: thermal sensor point 1 - hot interrupt trigger\n", __FUNCTION__);
    if (ret & THERMAL_MON_LOINTSTS1)
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[%s]: thermal sensor point 1 - low offset interrupt trigger\n", __FUNCTION__);
    if (ret & THERMAL_MON_HOINTSTS1)
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[%s]: thermal sensor point 1 - high offset interrupt trigger\n", __FUNCTION__);
    if (ret & THERMAL_MON_NHINTSTS1)
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[%s]: thermal sensor point 1 - hot to normal interrupt trigger\n", __FUNCTION__);
        
    if (ret & THERMAL_MON_CINTSTS2)
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[%s]: thermal sensor point 2 - cold interrupt trigger\n", __FUNCTION__);
    if (ret & THERMAL_MON_HINTSTS2)
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[%s]: thermal sensor point 2 - hot interrupt trigger\n", __FUNCTION__);
    if (ret & THERMAL_MON_LOINTSTS2)
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[%s]: thermal sensor point 2 - low offset interrupt trigger\n", __FUNCTION__);
    if (ret & THERMAL_MON_HOINTSTS2)
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[%s]: thermal sensor point 2 - high offset interrupt trigger\n", __FUNCTION__);
    if (ret & THERMAL_MON_NHINTSTS2)
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[%s]: thermal sensor point 2 - hot to normal interrupt trigger\n", __FUNCTION__);   
        
    if (ret & THERMAL_MON_TOINTSTS)
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[%s]: timeout interrupt trigger\n", __FUNCTION__);
       
    #if 0 
    if (ret & THERMAL_MON_IMMDINTSTS0)
    {
        temp = DRV_Reg32(TEMPMONINT);
        temp &= 0xFFFEFFFF;
        DRV_WriteReg32(TEMPMONINT, temp); // disable immediate interrupt for sense point 0
    }
    if (ret & THERMAL_MON_IMMDINTSTS1)
    {
        temp = DRV_Reg32(TEMPMONINT);
        temp &= 0xFFFDFFFF;
        DRV_WriteReg32(TEMPMONINT, temp); // disable immediate interrupt for sense point 1
    }
    if (ret & THERMAL_MON_IMMDINTSTS2)
    {
        temp = DRV_Reg32(TEMPMONINT);
        temp &= 0xFFFBFFFF;
        DRV_WriteReg32(TEMPMONINT, temp); // disable immediate interrupt for sense point 2
    }
    #endif 
    
    #if 0
    if (ret & THERMAL_MON_FILTINTSTS0)
    {
        temp = DRV_Reg32(TEMPMONINTSTS);
        temp &= 0xFFF7FFFF;
        DRV_WriteReg32(TEMPMONINT, temp);   // disable to filter sense point 0
    }    
    if (ret & THERMAL_MON_FILTINTSTS1)
    {
        temp = DRV_Reg32(TEMPMONINTSTS);
        temp &= 0xFFEFFFFF;
        DRV_WriteReg32(TEMPMONINT, temp);   // disable to filter sense point 1
    }
    if (ret & THERMAL_MON_FILTINTSTS2)
    {
        temp = DRV_Reg32(TEMPMONINTSTS);
        temp &= 0xFFDFFFFF;
        DRV_WriteReg32(TEMPMONINT, temp);   // disable to filter sense point 2
    }
    #endif
    
    thermal_interrupt_flag = 1; wake_up_interruptible(&thermal_interrupt_waiter);
    
    return IRQ_HANDLED;
}

void thermal_reset_and_initial(void)
{
    UINT32 temp = 0;
    
    thermal_dprintk("[%s]: reset thermal controller\n", __FUNCTION__);
    
    temp = DRV_Reg32(PERI_GLOBALCON_RST0);
    temp |= 0x00010000;
    DRV_WriteReg32(PERI_GLOBALCON_RST0, temp);
    
    temp = DRV_Reg32(PERI_GLOBALCON_RST0);
    temp &= 0xFFFEFFFF;
    DRV_WriteReg32(PERI_GLOBALCON_RST0, temp);
    
    // AuxADC Initialization
    temp = DRV_Reg32(AUXADC_CON0_V);
    temp &= 0xFFFFFFDF;
    DRV_WriteReg32(AUXADC_CON0_V, temp);        // disable auxadc channel 5 synchronous mode
    
    DRV_WriteReg32(AUXADC_CON1_CLR_V, 0x20);    // disable auxadc channel 5 immediate mode
    
    DRV_WriteReg32(TEMPMONCTL1, 0x00000140);    // counting unit is 320 * bus clock = 10ms
    DRV_WriteReg32(TEMPMONCTL2, 0x000C8000);    // sensing interval is 200 * 10ms = 2000ms
    
    DRV_WriteReg32(TEMPAHBPOLL, 0x0000000F);    // polling interval to check if temperature sense is ready, bus clock
    DRV_WriteReg32(TEMPAHBTO, 0xFFFFFFFF);      // exceed this polling time, IRQ would be inserted 32khz
    
    DRV_WriteReg32(TEMPMONIDET0, 0x00000000);   // times for interrupt occurrance
    DRV_WriteReg32(TEMPMONIDET1, 0x00000000);   // times for interrupt occurrance
    
    DRV_WriteReg32(TEMPHTHRE, 0x0000008FC);     // set hot threshold
    DRV_WriteReg32(TEMPOFFSETH, 0x00000960);    // set high offset threshold
    DRV_WriteReg32(TEMPH2NTHRE, 0x00000A8C);    // set hot to normal threshold
    DRV_WriteReg32(TEMPOFFSETL, 0x00000C80);    // set low offset threshold
    DRV_WriteReg32(TEMPCTHRE, 0x00000CE4);      // set cold threshold
    
    DRV_WriteReg32(TEMPMSRCTL0, 0x0000005);     // temperature sampling control, 18 sampling, drop max and min, then average 16 samples

    DRV_WriteReg32(AUXADC_CON1_SET_V, 0x20);    // enable auxadc channel 5 immediate mode
    
    //DRV_WriteReg32(TEMPADCPNP0, 0x0);                       // this value will be stored to TEMPPNPMUXADDR (TEMPSPARE0) automatically by hw
    //DRV_WriteReg32(TEMPPNPMUXADDR, (UINT32) TEMPSPARE0);    // AHB address for pnp sensor mux selection
    
    DRV_WriteReg32(TEMPADCMUX, 0x20);                            // this value will be stored to TEMPPNPMUXADDR (TEMPSPARE0) automatically by hw
    DRV_WriteReg32(TEMPADCMUXADDR, (UINT32) AUXADC_CON1_CLR_P);  // AHB address for auxadc mux selection
    
    DRV_WriteReg32(TEMPADCEN, 0x20);                            // AHB value for auxadc enable
    DRV_WriteReg32(TEMPADCENADDR, (UINT32) AUXADC_CON1_SET_P);  // AHB address for auxadc enable (channel 0 immediate mode selected)
    
    DRV_WriteReg32(TEMPADCVALIDADDR, (UINT32) AUXADC_DAT5_P);   // AHB address for auxadc valid bit
    DRV_WriteReg32(TEMPADCVOLTADDR, (UINT32) AUXADC_DAT5_P);    // AHB address for auxadc voltage output
    DRV_WriteReg32(TEMPRDCTRL, 0x0);                            // read valid & voltage are at the same register
    DRV_WriteReg32(TEMPADCVALIDMASK, 0x0000002C);               // indicate where the valid bit is (the 12th bit is valid bit and 1 is valid)
    DRV_WriteReg32(TEMPADCVOLTAGESHIFT, 0x0);                   // do not need to shift
    DRV_WriteReg32(TEMPADCWRITECTRL, 0x2);                      // enable auxadc mux write transaction
    
    //DRV_WriteReg32(TEMPMONINT, 0x0000FFFF);                     // enable all interrupt
    //DRV_WriteReg32(TEMPMONCTL0, 0x00000001);                    // enable sensing point 0
}

kal_int32 g_adc_ge=0;
kal_int32 g_adc_oe=0;
kal_int32 g_o_vts=0;
kal_int32 g_o_vbg=0;
kal_int32 g_degc_cali=0;
kal_int32 g_adc_cali_en=0;
kal_int32 g_o_vts_abb=0;

kal_int32 g_ge=0;
kal_int32 g_oe=0;
kal_int32 y_vts_plus=0;
kal_int32 g_gain=0;
kal_int32 g_x_roomt=0;
kal_int32 y_vts_abb_plus=0;
kal_int32 g_x_roomt_abb=0;

void thermal_cal_prepare(void)
{
    kal_uint32 temp = 0;
	
    temp = DRV_Reg32(0xF1019048);
    g_adc_ge = (temp & 0x000000FF);
    xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[thermal] temp=0x%x, g_adc_ge=0x%x\r\n", temp, g_adc_ge);
	
    temp = DRV_Reg32(0xF1019044);
    g_adc_oe = (temp & 0x000000FF);
    xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[thermal] temp=0x%x, g_adc_oe=0x%x\r\n", temp, g_adc_oe);
	
    temp = DRV_Reg32(0xF1019040);
    g_o_vts_abb		= ( (temp & 0xFC000000) >> 26 );		
    g_o_vts 		= ( (temp & 0x03FE0000) >> 17 );
    g_o_vbg 		= ( (temp & 0x0001FF00) >> 8 );
    g_degc_cali 	= ( (temp & 0x000000FE) >> 1 );
    g_adc_cali_en 	= ( (temp & 0x00000001) >> 0 );
	
    xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[thermal] temp=0x%x, g_o_vts=0x%x, g_o_vbg=0x%x, g_degc_cali=0x%x, g_adc_cali_en=0x%x, g_o_vts_abb=0x%x\r\n", 
        temp, g_o_vts, g_o_vbg, g_degc_cali, g_adc_cali_en, g_o_vts_abb);
	
}

void thermal_cal_prepare_2(kal_uint32 ret)
{
    kal_int32 temp = 0;
    kal_int32 y_vbg_plus = ret;
    kal_int32 format_1 = 0;
    kal_int32 format_2 = 0;
    kal_int32 format_3 = 0;
    kal_int32 format_3_abb = 0;
    kal_int32 format_4 = 0;
    kal_int32 format_4_abb = 0;
    
    g_ge = ( (g_adc_ge - 128) * 10000 ) / 4096; //ge*10000
    g_oe = (g_adc_oe - 128);
    
    format_1 = (y_vbg_plus-g_oe);
    format_2 = ((g_o_vbg+1800)-g_oe);
    format_3 = ((g_o_vts+2700)-g_oe);
    format_3_abb = ((g_o_vts+2700+g_o_vts_abb-20)-g_oe);
    
    y_vts_plus = ( (format_1*format_3) / format_2 ) + g_oe;
    y_vts_abb_plus = ( (format_1*format_3_abb) / format_2 ) + g_oe;
    g_gain = (10000 + g_ge) * format_1 / format_2; //gain*10000
    
    format_4 = (y_vts_plus-g_oe);
    format_4_abb = (y_vts_abb_plus-g_oe);
    
    g_x_roomt = (((format_4*10000)/4096)*10000)/g_gain; //x_roomt*10000		
    g_x_roomt_abb = (((format_4_abb*10000)/4096)*10000)/g_gain; //x_roomt*10000		
    
    
    xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[thermal] g_ge=%d, g_oe=%d, y_vts_prime=%d, g_gain=%d, g_x_roomt=%d, y_vts_abb_prime=%d, g_x_roomt_abb=%d\r\n", 
        g_ge, g_oe, y_vts_plus, g_gain, g_x_roomt, y_vts_abb_plus, g_x_roomt_abb);
}

kal_int32 thermal_cal_exec(kal_uint32 ret)
{
    kal_int32 t_current = 0;
    kal_int32 y_curr = ret;
    kal_int32 format_1 = 0;
    kal_int32 format_2 = 0;
    kal_int32 format_3 = 0;
    kal_int32 format_4 = 0;
    kal_int32 format_5 = 0;
    
    format_1 = (g_degc_cali / 2);
    format_2 = (y_curr-g_oe);	
    format_3 = ( ((((format_2)*10000)/4096)*10000)/g_gain ) - g_x_roomt;
    format_4 = ((format_3*100)/139); // uint = 0.1 deg
    format_4 = format_4 - (2*format_4);
    
    t_current = (format_1*10) + format_4; // uint = 0.1 deg
    
    return t_current;
}

kal_int32 thermal_cal_exec_abb(kal_uint32 ret)
{
    kal_int32 t_current = 0;
    kal_int32 y_curr = ret;
    kal_int32 format_1 = 0;
    kal_int32 format_2 = 0;
    kal_int32 format_3 = 0;
    kal_int32 format_4 = 0;
    kal_int32 format_5 = 0;
    
    format_1 = (g_degc_cali / 2);
    format_2 = (y_curr-g_oe);	
    format_3 = ( ((((format_2)*10000)/4096)*10000)/g_gain ) - g_x_roomt_abb;
    format_4 = ((format_3*100)/139); // uint = 0.1 deg
    format_4 = format_4 - (2*format_4);
    
    t_current = (format_1*10) + format_4; // uint = 0.1 deg
    
    return t_current;
}

int thermal_one_shot_handler(int times)
{
    kal_uint32 temp = 0, sum = 0, average_temp = 0;
    kal_int32 i = 0, retry = 0;
    
    DRV_Reg32(TEMPIMMD0);
    	
    DRV_WriteReg32(TEMPMSRCTL1, 0x00000010); // enable immediate mode for sensing point 0    
    //msleep(100);
    
    for (i = 0; i < times; i++)
    {
        retry = RETRY_TIMES;
        
        #if 0
        wait_event_interruptible_timeout(thermal_interrupt_waiter, thermal_interrupt_flag != 0, HZ * 5);
        
        if (!thermal_interrupt_flag)
        {
            thermal_dprintk("[%s]: interrupt timeout\n", __FUNCTION__);
            thermal_reset_and_initial();
            i--;
            continue;
        }
        
        thermal_interrupt_flag = 0;
        thermal_dprintk("[%s]: interrupt trigger\n", __FUNCTION__);
        #endif
        
        temp = DRV_Reg32(TEMPIMMD0);
        while ((temp & 0x8000) == 0 && retry--)
        {
            //msleep(50);
            temp = DRV_Reg32(TEMPIMMD0);
        }
        //thermal_dprintk("[%s]: TEMPIMMD0 = %d, retry = %d\n", __FUNCTION__, (temp & 0x0FFF), retry);
        
        if(retry < 0)
        {
            i--;
        }
        else
        {
            sum += (temp & 0x0FFF);
        }
    }
    
    DRV_WriteReg32(TEMPMSRCTL1, 0x0); // disable immediate mode for sensing point 0
    
    average_temp = sum / times;

    //thermal_dprintk("[%s]: sum = %d, average_temp = %d, times = %d\n", __FUNCTION__, sum, average_temp, times);
        
    return average_temp;
}
EXPORT_SYMBOL(thermal_one_shot_handler);

int y_vbg_plus_repeat_times = 100;
int y_curr_repeat_times = 20;

void thermal_calibration(void)
{
    kal_uint32 ret = 0;
    kal_int32 temp = 0;
    kal_int32 retry = 5;
    
    thermal_cal_prepare();
    
    DRV_WriteReg32(0xF0007804, 0x0002); //TS_CON1
    DRV_WriteReg32(0xF0007808, 0x0200); //TS_CON2
    
    //--------------------------------------------------------------------------------
    DRV_Reg32(TEMPIMMD0);
    DRV_WriteReg32(TEMPMSRCTL1, 0x00000010); // enable immediate mode for sensing point 0
    msleep(100);
    temp = DRV_Reg32(TEMPIMMD0);
    while ((temp & 0x8000) == 0 && retry--)
    {
        temp = DRV_Reg32(TEMPIMMD0);
    }
    DRV_WriteReg32(TEMPMSRCTL1, 0x0); // disable immediate mode for sensing point 0	
    xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[thermal_calibration]: pre fetch TEMPIMMD0 = %d\n", (temp & 0x0FFF) ); 
    //--------------------------------------------------------------------------------
    
    ret = thermal_one_shot_handler(y_vbg_plus_repeat_times); //y_vbg_plus
    xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[thermal_calibration]: average %d times TEMPIMMD0 = %d\n", y_vbg_plus_repeat_times, ret); 
    
    thermal_cal_prepare_2(ret); // Will save DEGC_cali, OE, Gain and X_roomt in memory for future use	
    
    xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[thermal_calibration]: Done\n");
}

int thermal_thread_handler(void *unused)
{
    kal_uint32 ret = 0;    
    kal_int32 t_ret = 0;
    kal_uint32 ret_2 = 0;    
    kal_int32 t_ret_2 = 0;
    kal_uint16 temp = 0;
    
    do
    {
        //ktime_t ktime = ktime_set(thermal_period, 0);		
        ktime_t ktime = ktime_set( 0, MS_TO_NS(delay_in_ms) );
        
        wait_event_interruptible(thermal_timer_waiter, thermal_timer_flag != 0);
        thermal_timer_flag = 0;
        
		//----------------------------------------------------------------------------------
        #if 0
        temp = DRV_Reg16(TS_CON2);
        temp &= 0xFCFF;
        DRV_WriteReg16(TS_CON2, temp);
        
        msleep(100);
        
        ret = thermal_one_shot_handler(5);
        thermal_dprintk("[%s]: average TSMCU to AUXADC TEMPIMMD0 = %d\n", __FUNCTION__, ret);
        #endif
        //----------------------------------------------------------------------------------
        #if 0
        temp = DRV_Reg16(TS_CON2);
        temp |= 0x0100;
        DRV_WriteReg16(TS_CON2, temp);
        
        msleep(100);
        
        ret = thermal_one_shot_handler(5);
        thermal_dprintk("[%s]: average TSABB to AUXADC TEMPIMMD0 = %d\n", __FUNCTION__, ret);
        #endif
        //----------------------------------------------------------------------------------
        //DRV_WriteReg32(0xF0007804, 0x0000); //TS_CON1
        //DRV_WriteReg32(0xF0007804, 0x0001); //TS_CON1
        DRV_WriteReg32(0xF0007804, 0x0002); //TS_CON1		
        
        DRV_WriteReg32(0xF0007808, 0x0000); //TS_CON2
        
        if (thermal_debug_mode)
        {
            xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[Thermal_Result]: -------------------------------------\n");
        }
        ret = thermal_one_shot_handler(y_curr_repeat_times); 
        if (thermal_debug_mode)
        {
            xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[Thermal_Result]: TSMCU average %d times TEMPIMMD0 (0x%x,0x%x) = %d\n", 
                y_curr_repeat_times, DRV_Reg32(0xF0007804), DRV_Reg32(0xF0007808), ret);
        }
        
        t_ret = thermal_cal_exec(ret);
        //xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[Thermal_Result]: T_current = %d\n", t_ret);
        //----------------------------------------------------------------------------------
        temp = DRV_Reg16(TS_CON2);
        temp |= 0x0100;
        DRV_WriteReg16(TS_CON2, temp);		
        
        ret_2 = thermal_one_shot_handler(y_curr_repeat_times); 
        if (thermal_debug_mode)
        {
            xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[Thermal_Result]: TSABB average %d times TEMPIMMD0 (0x%x,0x%x) = %d\n", 
                y_curr_repeat_times, DRV_Reg32(0xF0007804), DRV_Reg32(0xF0007808), ret_2);
        }
        
        //t_ret_2 = thermal_cal_exec(ret_2);
        t_ret_2 = thermal_cal_exec_abb(ret_2);
        //xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "[Thermal_Result]: T_current = %d\n", t_ret_2);
        //----------------------------------------------------------------------------------
        if (thermal_debug_mode)
        {
            xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[Thermal_T_current_TSMCU_TSABB],%d,%d\n", t_ret, t_ret_2);
        }
        
        if(g_adc_cali_en == 1)
        {
            if (thermal_debug_mode)
            {
                xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[Thermal_Result] Check if t_ret(%d) > T_HIGH_LIMIT(%d) \n", t_ret, T_HIGH_LIMIT);
            }
            if(t_ret > T_HIGH_LIMIT)
            {
                aee_kernel_reminding("T_CPU Too Hot",
                    "WARNING : T_CPU Too Hot, may impact CPU age. You can use \"aee -c dal\" to clean screen.");
            }
        }
        
        hrtimer_start(&thermal_timer, ktime, HRTIMER_MODE_REL);

    } while (!kthread_should_stop());
    
    return 0;
}

static int thermal_period_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;
    
    p += sprintf(p, "%d", thermal_period);

    *start = buf + off;
    
    len = p - buf;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len  : count;
}

static ssize_t thermal_period_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = 0, time = 0;
    char desc[32];

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
    {
        return 0;
    }
    desc[len] = '\0';
    
    xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "\n=============== Thermal Controller Period Mode ================\n");
    if (sscanf(desc, "%d", &time) == 1)
    {
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "  config thermal controller period = %d (s)\n", time);
        thermal_period = time;
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "==========================================\n");
        return count;
    }
    else
    {
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "  bad argument!!\n");
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal",   "==========================================\n");
    }
    
    return -EINVAL;
}

static int thermal_period_mode_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;
    
    if (thermal_period_mode)
        p += sprintf(p, "enable");
    else
        p += sprintf(p, "disable");

    *start = buf + off;
    
    len = p - buf;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len  : count;
}

static ssize_t thermal_period_mode_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = 0;
    char mode[20], desc[32];
    //ktime_t ktime = ktime_set(thermal_period, 0);
    ktime_t ktime = ktime_set( 0, MS_TO_NS(delay_in_ms) );
    
    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
    {
        return 0;
    }
    desc[len] = '\0';
    
    xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "\n=============== Thermal Controller Period Mode ================\n");
    if (sscanf(desc, "%s", &mode) == 1)
    {
        if (!strcmp(mode, "enable"))
        {
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "  enable thermal controller period mode\n");
            thermal_period_mode = true;
            hrtimer_start(&thermal_timer, ktime, HRTIMER_MODE_REL);
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "==========================================\n");
            return count;
        }
        else if (!strcmp(mode, "disable"))
        {
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "  disable thermal controller period mode\n");
            thermal_period_mode = false;
            hrtimer_cancel(&thermal_timer);
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "==========================================\n");
            return count;
        }
        else
        {
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "  bad argument!! argument should be \"enable\" or \"disable\"\n");
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal",   "==========================================\n");
        }
    }
    else
    {
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "  bad argument!!\n");
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal",   "==========================================\n");
    }
    
    return -EINVAL;
}

static int thermal_debug_mode_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;
    
    if (thermal_debug_mode)
        p += sprintf(p, "enable");
    else
        p += sprintf(p, "disable");

    *start = buf + off;
    
    len = p - buf;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len  : count;
}

static ssize_t thermal_debug_mode_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = 0;
    char mode[20], desc[32];
    
    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len)) 
    {
        return 0;
    }
    desc[len] = '\0';
    
    xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "\n=============== Thermal Controller Debug Mode ================\n");
    if (sscanf(desc, "%s", &mode) == 1)
    {
        if (!strcmp(mode, "enable"))
        {
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "  enable thermal controller debug mode\n");
            thermal_debug_mode = true;
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "==========================================\n");
            return count;
        }
        else if (!strcmp(mode, "disable"))
        {
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "  disable thermal controller debug mode\n");
            thermal_debug_mode = false;
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "==========================================\n");
            return count;
        }
        else
        {
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "  bad argument!! argument should be \"enable\" or \"disable\"\n");
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal",   "==========================================\n");
        }
    }
    else
    {
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "  bad argument!!\n");
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal",   "==========================================\n");
    }
    
    return -EINVAL;
}

static int thermal_one_shot_mode_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;
    
    p += sprintf(p, "read function not support for now");

    *start = buf + off;
    
    len = p - buf;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len  : count;
}

static ssize_t thermal_one_shot_mode_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = 0;
    kal_uint16 temp = 0;
    char mode[20], desc[32];
    
    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
    {
        return 0;
    }
    desc[len] = '\0';
    
    xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "\n=============== Thermal Controller One Shot Mode ================\n");
    if (sscanf(desc, "%s", &mode) == 1)
    {
        if (!strcmp(mode, "enable"))
        {
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "  enable thermal controller one shot mode\n");
            
            temp = DRV_Reg16(TS_CON2);
            temp &= 0xFCFF;
            DRV_WriteReg16(TS_CON2, temp);
            
            msleep(100);
            
            temp = thermal_one_shot_handler(5);
            thermal_dprintk("[%s]: average TSMCU to AUXADC TEMPIMMD0 = %d\n", __FUNCTION__, temp);
            
            temp = DRV_Reg16(TS_CON2);
            temp |= 0x0100;
            DRV_WriteReg16(TS_CON2, temp);
               
            msleep(100);
            
            temp = thermal_one_shot_handler(5);
            thermal_dprintk("[%s]: average TSABB to AUXADC TEMPIMMD0 = %d\n", __FUNCTION__, temp);
        
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "==========================================\n");
            return count;
        }
        else
        {
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "  bad argument!! argument should be \"enable\"\n");
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal",   "==========================================\n");
        }
    }
    else
    {
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "  bad argument!!\n");
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal",   "==========================================\n");
    }
    
    return -EINVAL;
}

static int thermal_keep_on_mode_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;
    
    if (thermal_keep_on_mode)
        p += sprintf(p, "enable");
    else
        p += sprintf(p, "disable");

    *start = buf + off;
    
    len = p - buf;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len  : count;
}

static ssize_t thermal_keep_on_mode_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = 0;
    char mode[20], desc[32];
    
    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
    {
        return 0;
    }
    desc[len] = '\0';
    
    xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "\n=============== Thermal Controller Keep On Mode ================\n");
    if (sscanf(desc, "%s", &mode) == 1)
    {
        if (!strcmp(mode, "enable"))
        {
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "  enable thermal controller keep on mode\n");
            thermal_keep_on_mode = true;
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "==========================================\n");
            return count;
        }
        else if (!strcmp(mode, "disable"))
        {
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "  disable thermal controller keep on mode\n");
            thermal_keep_on_mode = false;
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "==========================================\n");
            return count;
        }
        else
        {
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "  bad argument!! argument should be \"enable\" or \"disable\"\n");
            xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal",   "==========================================\n");
        }
    }
    else
    {
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal", "  bad argument!!\n");
        xlog_printk(ANDROID_LOG_DEBUG, "Power/Thermal",   "==========================================\n");
    }
    
    return -EINVAL;
}

void thermal_early_suspend(struct early_suspend *h)
{
    kal_uint16 temp;
    
    if (!thermal_keep_on_mode)
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[%s]: enter early suspend\n", __FUNCTION__);
        
        mt6575_irq_mask(MT6575_THERM_CTR_IRQ_ID);
        
        disable_clock(MT65XX_PDN_PERI_THERM, "Thermal");
        disable_clock(MT65XX_PDN_PERI_AUXADC, "Thermal-Auxadc");
        
        temp = DRV_Reg16(TS_CON2);
        temp |= 0x0300;
        DRV_WriteReg16(TS_CON2, temp); // turn off sensor output buffer
        
        if (thermal_period_mode)
            hrtimer_cancel(&thermal_timer);
    }
}

void thermal_late_resume(struct early_suspend *h)
{
    kal_uint16 temp;
    
    if (!thermal_keep_on_mode)
    {
        //ktime_t ktime = ktime_set(thermal_period, 0);
        ktime_t ktime = ktime_set( 0, MS_TO_NS(delay_in_ms) );
        
        xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[%s]: enter late resume\n", __FUNCTION__);
        
        mt6575_irq_unmask(MT6575_THERM_CTR_IRQ_ID);
        
        enable_clock(MT65XX_PDN_PERI_AUXADC, "Thermal-Auxadc");
        enable_clock(MT65XX_PDN_PERI_THERM, "Thermal");
        
        temp = DRV_Reg16(TS_CON2);
        temp &= 0xFCFF;
        DRV_WriteReg16(TS_CON2, temp); // turn on sensor output buffer
        
        if (thermal_period_mode)
            hrtimer_start(&thermal_timer, ktime, HRTIMER_MODE_REL);
    }
}

static int mtk_thermal_probe(struct platform_device *pdev)
{
    int ret = 0;
    ktime_t ktime;
    
    struct proc_dir_entry *entry = NULL;
    struct proc_dir_entry *thermal_dir = NULL;
    
    DRV_WriteReg16(TS_CON3, DRV_Reg16(TS_CON3) | 0x0100); // release turbo mode and enable LPF mode
    
    /* register thermal controller interrupt */
    mt65xx_irq_set_sens(MT6575_THERM_CTR_IRQ_ID, MT65xx_EDGE_SENSITIVE);
    mt65xx_irq_set_polarity(MT6575_THERM_CTR_IRQ_ID, MT65xx_POLARITY_LOW);
    ret = request_irq(MT6575_THERM_CTR_IRQ_ID, thermal_interrupt_handler, 0, THERMAL_NAME, NULL);
    if (ret)
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[%s]: register IRQ failed (%d)\n", __FUNCTION__, ret);
        return ret;
    }
    
    mt6575_irq_unmask(MT6575_THERM_CTR_IRQ_ID);

    #ifdef CONFIG_HAS_EARLYSUSPEND
    thermal_early_suspend_handler.suspend = thermal_early_suspend;
    thermal_early_suspend_handler.resume = thermal_late_resume;
    register_early_suspend(&thermal_early_suspend_handler);
    #endif
    
    //ktime = ktime_set(thermal_period, 0);
    ktime = ktime_set( 0, MS_TO_NS(delay_in_ms) );

    hrtimer_init(&thermal_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    thermal_timer.function = thermal_timer_func;
    
    if (thermal_period_mode)
        hrtimer_start(&thermal_timer, ktime, HRTIMER_MODE_REL);
    
    enable_clock(MT65XX_PDN_PERI_AUXADC, "Thermal-Auxadc");
    enable_clock(MT65XX_PDN_PERI_THERM, "Thermal");
    
    thermal_reset_and_initial();
    thermal_calibration();
        
    thermal_thread = kthread_run(thermal_thread_handler, 0, "thermal controller");
    if (IS_ERR(thermal_thread))
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[%s]: failed to create thermal thread\n", __FUNCTION__);
    }
    
    thermal_dir = proc_mkdir("thermal", NULL);
    if (!thermal_dir)
    {
        pr_err("[%s]: mkdir /proc/thermal failed\n", __FUNCTION__);
    }
    else
    {
        entry = create_proc_entry("thermal_period", S_IRUGO | S_IWUSR, thermal_dir);
        if (entry)
        {
            entry->read_proc = thermal_period_read;
            entry->write_proc = thermal_period_write;
        }
        
        entry = create_proc_entry("thermal_period_mode", S_IRUGO | S_IWUSR, thermal_dir);
        if (entry)
        {
            entry->read_proc = thermal_period_mode_read;
            entry->write_proc = thermal_period_mode_write;
        }
        
        entry = create_proc_entry("thermal_one_shot_mode", S_IRUGO | S_IWUSR, thermal_dir);
        if (entry)
        {
            entry->read_proc = thermal_one_shot_mode_read;
            entry->write_proc = thermal_one_shot_mode_write;
        }
        
        entry = create_proc_entry("thermal_debug_mode", S_IRUGO | S_IWUSR, thermal_dir);
        if (entry)
        {
            entry->read_proc = thermal_debug_mode_read;
            entry->write_proc = thermal_debug_mode_write;
        }
        
        entry = create_proc_entry("thermal_keep_on_mode", S_IRUGO | S_IWUSR, thermal_dir);
        if (entry)
        {
            entry->read_proc = thermal_keep_on_mode_read;
            entry->write_proc = thermal_keep_on_mode_write;
        }
    }
    
    return 0;
}

static int mtk_thermal_suspend(struct platform_device *dev, pm_message_t state)	
{
	kal_uint16 temp;
    
    if (!thermal_keep_on_mode_sleep)
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[%s]: enter suspend\n", __FUNCTION__);
        
        mt6575_irq_mask(MT6575_THERM_CTR_IRQ_ID);
        
        disable_clock(MT65XX_PDN_PERI_THERM, "Thermal");
        disable_clock(MT65XX_PDN_PERI_AUXADC, "Thermal-Auxadc");
        
        temp = DRV_Reg16(TS_CON2);
        temp |= 0x0300;
        DRV_WriteReg16(TS_CON2, temp); // turn off sensor output buffer
        
        if (thermal_period_mode)
            hrtimer_cancel(&thermal_timer);
    }    

    return 0;
}

static int mtk_thermal_resume(struct platform_device *dev)
{
    kal_uint16 temp;
    
    if (!thermal_keep_on_mode_sleep)
    {
        //ktime_t ktime = ktime_set(thermal_period, 0);
        ktime_t ktime = ktime_set( 0, MS_TO_NS(delay_in_ms) );
        
        xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[%s]: enter resume\n", __FUNCTION__);
        
        mt6575_irq_unmask(MT6575_THERM_CTR_IRQ_ID);
        
        enable_clock(MT65XX_PDN_PERI_AUXADC, "Thermal-Auxadc");
        enable_clock(MT65XX_PDN_PERI_THERM, "Thermal");
        
        temp = DRV_Reg16(TS_CON2);
        temp &= 0xFCFF;
        DRV_WriteReg16(TS_CON2, temp); // turn on sensor output buffer
        
        if (thermal_period_mode)
            hrtimer_start(&thermal_timer, ktime, HRTIMER_MODE_REL);
    }

    return 0;
}

static struct platform_driver mtk_thermal_driver = {
    .remove     = NULL,
    .shutdown   = NULL,
    .probe      = mtk_thermal_probe,
    .suspend	= mtk_thermal_suspend,
    .resume		= mtk_thermal_resume,
    .driver     = {
        .name = THERMAL_NAME,
    },
};

static int __init mtk_thermal_init(void)
{
    int ret = 0;
    kal_uint16 temp = 0;
    
    //if (get_chip_eco_ver() != CHIP_E1) // Enable after check at E2 chip
    if(0)
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "MediaTek Thermal Controller driver registration\n");
        
        ret = platform_driver_register(&mtk_thermal_driver);
        
        if (ret != 0)
        {
            xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "unable to register thermal controller driver\n");
            return -1;
        }
    }
    else
    {
        xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "Disable Thermal Controller driver\n");
        disable_clock(MT65XX_PDN_PERI_THERM, "Thermal");
        
        temp = DRV_Reg16(TS_CON2);
        temp |= 0x0300;
        DRV_WriteReg16(TS_CON2, temp); // turn off sensor output buffer
    }
    
    return 0;
}

static void __exit mtk_thermal_exit(void)
{
    kal_uint32 temp = 0, ret = 0;
    
    xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[%s]: thermal controller de-initialized\n", __FUNCTION__);    
    
    ret = hrtimer_cancel(&thermal_timer);
    if (ret) 
        xlog_printk(ANDROID_LOG_INFO, "Power/Thermal", "[%s]: thermal controller timer still in use\n", __FUNCTION__);
}

module_init(mtk_thermal_init);
module_exit(mtk_thermal_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek Thermal Controller Driver");
MODULE_AUTHOR("Chun-Wei Chen <chun-wei.chen@mediatek.com>");
