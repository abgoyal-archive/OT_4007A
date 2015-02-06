
#include <linux/kernel.h>
#include <linux/module.h>
#include <mach/mt6575_gpio.h>
#include <mach/mt_combo.h>
#include <cust_gpio_usage.h>
#include <mach/mt6575_boot.h>
//#include <mach/mt6575_pll.h>
#include <linux/xlog.h>
#include <mach/mt6575_dcm.h>
extern void combo_audio_pin_conf(COMBO_AUDIO_STATE state);









static void (*cmb_bgf_eirq_cb)(void) = NULL;
static int (*cmb_audio_ctrl_cb)(u32, u32) = NULL;
static void (*cmb_func_ctrl_cb)(u32, u32) = NULL;
static void (*cmb_binfo_ctrl_cb)(u32, u32) = NULL;
static COMBO_AUDIO_STATE cmb_audio_state = COMBO_AUDIO_STATE_0;

int mt_combo_init (struct combo_ctrl *ctrl)
{
    if (!ctrl || ctrl->size != sizeof(struct combo_ctrl)) {
        WMT_CMB_ALERT("invalid ctrl:0x%p size(%d)\n", ctrl, (ctrl) ? ctrl->size: 0);
        return -1;
    }

    WMT_CMB_INFO("0x%p size(%d)\n", ctrl, ctrl->size);

    cmb_bgf_eirq_cb = ctrl->bgf_eirq_cb;
    cmb_audio_ctrl_cb = ctrl->audio_ctrl_cb;
    cmb_func_ctrl_cb = ctrl->func_ctrl_cb;
    cmb_binfo_ctrl_cb = ctrl->binfo_ctrl_cb;

    return 0;
}
EXPORT_SYMBOL(mt_combo_init);

int mt_combo_deinit (struct combo_ctrl *ctrl)
{
    cmb_bgf_eirq_cb = NULL;
    cmb_audio_ctrl_cb = NULL;
    cmb_func_ctrl_cb = NULL;
    cmb_binfo_ctrl_cb = NULL;
    cmb_audio_state = COMBO_AUDIO_STATE_0;
    return 0;
}
EXPORT_SYMBOL(mt_combo_deinit);

/* stub functions for kernel interrupt handler */
void mt_combo_bgf_eirq_handler (void* par)
{
    //WMT_CMB_INFO("mt_combo_bgf_eirq_handler \n");

        /* avoid the early interrupt before we register the eirq_handler */
    if (cmb_bgf_eirq_cb){
        cmb_bgf_eirq_cb();
    }

    return;
}

#if defined(MT6620E1) || defined(MT6620E3)
/* stub functions for kernel to control audio path pin mux */
int mt_combo_audio_ctrl_ex (COMBO_AUDIO_STATE state, u32 clt_ctrl)
{
    int ret = -1;
    int pin_share = 1;//I2S, PCM Pin sharing ?
    int pin_flag = 0;
    
    { //E1 later
        pin_share = 0;
        pin_flag |= COMBO_AUDIO_PIN_SEPARATE;
        WMT_CMB_INFO( "CHIP_E2 PCM/I2S pin_separate configruation \n");

    }
    
    /* FIXME:host first or chip first? */

    /* set host side first */
    switch (state) {
    case COMBO_AUDIO_STATE_0:
        /* BT_PCM_OFF & FM line in/out */
        pin_flag |= COMBO_AUDIO_BT_PCM_OFF;
        pin_flag |= COMBO_AUDIO_FM_LINEIN;
        //WMT_CMB_WARN(KERN_DEBUG "[COMBO] disable BT-PCM\n");
        break;
    case COMBO_AUDIO_STATE_1:

        pin_flag |= COMBO_AUDIO_BT_PCM_ON;
        pin_flag |= COMBO_AUDIO_FM_LINEIN;       
        break;
    case COMBO_AUDIO_STATE_2:

        pin_flag |= COMBO_AUDIO_BT_PCM_OFF;
        pin_flag |= COMBO_AUDIO_FM_I2S;
		break;        
    case COMBO_AUDIO_STATE_3:
        //WMT_CMB_DBG( "enable FM-I2S and disable BT-PCM\n");
        pin_flag |= COMBO_AUDIO_BT_PCM_ON;
        pin_flag |= COMBO_AUDIO_FM_I2S;
        break;
    default:
        /* FIXME: move to cust folder? */
        WMT_CMB_INFO("audio_ctrl invalid state [%d]\n", cmb_audio_state);
        return -1;
        break;
    }
	combo_audio_pin_conf(state);
    WMT_CMB_INFO("cmb_audio_ctrl_cb = %p, clt_ctrl= %d\n",
        cmb_audio_ctrl_cb, clt_ctrl);

    /* chip side */
    if (cmb_audio_ctrl_cb && clt_ctrl) {
        ret = cmb_audio_ctrl_cb(state, pin_flag);
    }
    else {
        if (cmb_audio_ctrl_cb) {
            /* skip case */
            ret = 0;
        }
    }

    WMT_CMB_INFO( "audio_ctrl [%d]->[%d] ret(%d, %d)\n",
        cmb_audio_state, state, ret, (cmb_audio_ctrl_cb) ? 1 : 0);
    cmb_audio_state = state;

    return ret;
}
EXPORT_SYMBOL(mt_combo_audio_ctrl_ex);
#endif

void mt_combo_func_ctrl (u32 type, u32 on) {
    if (cmb_func_ctrl_cb) {
        cmb_func_ctrl_cb(type, on);
    }
    else {
        WMT_CMB_ALERT(" cmb_func_ctrl_cb null \n");
    }
}
EXPORT_SYMBOL(mt_combo_func_ctrl);

void mt_combo_binfo_ctrl (u32 idx, u32 value) {
    if (cmb_binfo_ctrl_cb) {
        cmb_binfo_ctrl_cb(idx, value);
    }
    else {
        WMT_CMB_ALERT( "cmb_func_binfo_cb null \n");
    }    
}
EXPORT_SYMBOL(mt_combo_binfo_ctrl);

/*platform-related APIs*/
//void clr_device_working_ability(UINT32 clockId, MT6573_STATE state);
//void set_device_working_ability(UINT32 clockId, MT6573_STATE state);

static int _mt_combo_plt_do_deep_idle(COMBO_IF src, int enter){

    int ret = -1;

    const char *combo_if_name[] =
    {   "COMBO_IF_UART", 
        "COMBO_IF_MSDC"   
    };

    if(src != COMBO_IF_UART && src!= COMBO_IF_MSDC){
        WMT_CMB_ALERT("src = %d is error\n", src);
        return ret;
    }
//    
//    if(src >= 0 && src < COMBO_IF_MAX){
//        WMT_CMB_INFO("src = %s, to enter deep idle? %d \n", 
//            combo_if_name[src], 
//            enter);
//    }

    /*TODO: For Common SDIO configuration, we need to do some judgement between STP and WIFI 
            to decide if the msdc will enter deep idle safely*/

    switch(src){
        case COMBO_IF_UART:            
            if(enter == 0){
                //MT6575 still not support this.
               // clr_device_working_ability(MT65XX_PDN_PERI_UART3, DEEP_IDLE_STATE);
                disable_dpidle_by_bit(MT65XX_PDN_PERI_UART2);
            } else {
                //MT6575 still not support this
              //  set_device_working_ability(MT65XX_PDN_PERI_UART3, DEEP_IDLE_STATE);
                enable_dpidle_by_bit(MT65XX_PDN_PERI_UART2);
            }
            ret = 0;
            break;            

        case COMBO_IF_MSDC:
            if(enter == 0){
                //MT6575 still not support this
             //   clr_device_working_ability(MT65XX_PDN_PERI_MSDC, DEEP_IDLE_STATE);
            } else {
                //MT6575 still not support this
             //   set_device_working_ability(MT65XX_PDN_PERI_MSDC, DEEP_IDLE_STATE);
            }
            ret = 0;
            break;

        default:
            ret = -1;
            break;
    }

    return ret;
}

#if defined(MT6620E1) || defined(MT6620E3)
int mt_combo_plt_enter_deep_idle(COMBO_IF src){
    return _mt_combo_plt_do_deep_idle(src, 1);
}
EXPORT_SYMBOL(mt_combo_plt_enter_deep_idle);

int mt_combo_plt_exit_deep_idle(COMBO_IF src){
    return _mt_combo_plt_do_deep_idle(src, 0);
}    
EXPORT_SYMBOL(mt_combo_plt_exit_deep_idle);
#endif

