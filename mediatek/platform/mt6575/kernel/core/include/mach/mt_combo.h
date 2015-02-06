
#ifndef __ARCH_ARM_MACH_MT6573_INCLUDE_MACHINE_MT_COMBO_H
#define __ARCH_ARM_MACH_MT6573_INCLUDE_MACHINE_MT_COMBO_H


#include <linux/types.h>
#include <linux/xlog.h>


//#if defined(MT6628)

typedef enum {
    COMBO_AUDIO_STATE_0 = 0, /* 0000: BT_PCM_OFF & FM analog (line in/out) */
    COMBO_AUDIO_STATE_1 = 1, /* 0001: BT_PCM_ON & FM analog (in/out) */
    COMBO_AUDIO_STATE_2 = 2, /* 0010: BT_PCM_OFF & FM digital (I2S) */
    COMBO_AUDIO_STATE_3 = 3, /* 0011: BT_PCM_ON & FM digital (I2S) (invalid in 73evb & 1.2 phone configuration) */
    COMBO_AUDIO_STATE_MAX = 4,
} COMBO_AUDIO_STATE;

typedef enum {
    COMBO_FUNC_TYPE_BT = 0,
    COMBO_FUNC_TYPE_FM = 1,
    COMBO_FUNC_TYPE_GPS = 2,
    COMBO_FUNC_TYPE_WIFI = 3,
    COMBO_FUNC_TYPE_WMT = 4,
    COMBO_FUNC_TYPE_STP = 5,
    COMBO_FUNC_TYPE_NUM = 6
} COMBO_FUNC_TYPE;

typedef enum {
    COMBO_IF_UART = 0,
    COMBO_IF_MSDC = 1,
    COMBO_IF_MAX,
} COMBO_IF;




extern int mt_combo_audio_ctrl_ex(COMBO_AUDIO_STATE state, u32 clt_ctrl);
static inline int mt_combo_audio_ctrl(COMBO_AUDIO_STATE state) {
    return mt_combo_audio_ctrl_ex(state, 1);
}
extern int mt_combo_plt_enter_deep_idle(COMBO_IF src);
extern int mt_combo_plt_exit_deep_idle(COMBO_IF src);

extern void mtk_wcn_cmb_stub_func_ctrl (unsigned int type, unsigned int on);
extern int board_sdio_ctrl (unsigned int sdio_port_num, unsigned int on);


//#else

#define COMBO_AUDIO_BT_MASK (0x1UL)
#define COMBO_AUDIO_BT_PCM_ON (0x1UL << 0)
#define COMBO_AUDIO_BT_PCM_OFF (0x0UL << 0)

#define COMBO_AUDIO_FM_MASK (0x2UL)
#define COMBO_AUDIO_FM_LINEIN (0x0UL << 1)
#define COMBO_AUDIO_FM_I2S (0x1UL << 1)

#define COMBO_AUDIO_PIN_MASK     (0x4UL)
#define COMBO_AUDIO_PIN_SHARE    (0x1UL << 2)
#define COMBO_AUDIO_PIN_SEPARATE (0x0UL << 2)

#ifndef	WMT_CMB_LOG_TAG
#define WMT_CMB_LOG_TAG "WCN_CMN/CMB"
#endif


#if 0
//Non-ALPS platform
#define WMT_CMB_INFO(fmt, arg...)
#define WMT_CMB_ERR(fmt, arg...)
#define WMT_CMB_ALERT(fmt, arg...)
#define WMT_CMB_WARN(fmt, arg...)
#define WMT_CMB_DBG(fmt, arg...)
#else
//For ALPS
#define WMT_CMB_INFO(fmt, arg...)   xlog_printk(ANDROID_LOG_INFO, WMT_CMB_LOG_TAG, "%s:"  fmt, __FUNCTION__ ,##arg)
#define WMT_CMB_ERR(fmt, arg...)   xlog_printk(ANDROID_LOG_ERROR, WMT_CMB_LOG_TAG, "%s:"  fmt, __FUNCTION__ ,##arg)
#define WMT_CMB_ALERT(fmt, arg...)   xlog_printk(ANDROID_LOG_WARN, WMT_CMB_LOG_TAG, "%s:"  fmt, __FUNCTION__ ,##arg)
#define WMT_CMB_WARN(fmt, arg...)   xlog_printk(ANDROID_LOG_WARN, WMT_CMB_LOG_TAG, "%s:"  fmt, __FUNCTION__ ,##arg)
#define WMT_CMB_DBG(fmt, arg...)   xlog_printk(ANDROID_LOG_DEBUG, WMT_CMB_LOG_TAG, "%s:"  fmt, __FUNCTION__ ,##arg)
#endif

#if 0
typedef enum {
    COMBO_AUDIO_STATE_0 = 0, /* 0000: BT_PCM_OFF & FM analog (line in/out) */
    COMBO_AUDIO_STATE_1 = 1, /* 0001: BT_PCM_ON & FM analog (in/out) */
    COMBO_AUDIO_STATE_2 = 2, /* 0010: BT_PCM_OFF & FM digital (I2S) */
    COMBO_AUDIO_STATE_3 = 3, /* 0011: BT_PCM_ON & FM digital (I2S) (invalid in 73evb & 1.2 phone configuration) */
} COMBO_AUDIO_STATE;

typedef enum {
    COMBO_FUNC_TYPE_BT = 0,
    COMBO_FUNC_TYPE_FM = 1,
    COMBO_FUNC_TYPE_GPS = 2,
    COMBO_FUNC_TYPE_WIFI = 3,
    COMBO_FUNC_TYPE_WMT = 4,
    COMBO_FUNC_TYPE_STP = 5,
    COMBO_FUNC_TYPE_NUM = 6
} COMBO_FUNC_TYPE;
#endif
typedef enum {
    COMBO_BOARD_INFO_BTYPE = 0, /*record board type*/
} COMBO_BOARD_INFO;

typedef enum {
    COMBO_BOARD_TYPE_MT6573EVB = 0, /*record board type*/
    COMBO_BOARD_TYPE_ZTE73V1_2 = 1,
    COMBO_BOARD_TYPE_MT6620E3_GENERIC = 2,
    COMBO_BOARD_TYPE_ZTEMT73V2 = 3,
    COMBO_BOARD_TYPE_MT6620E4_GENERIC = 4,
    COMBO_BAORD_TYPE_ZTEMT73V3 = 5,
    COMBO_BAORD_TYPE_MAX,
    COMBO_BAORD_TYPE_INVALID,
} COMBO_BOARD_TYPE;
#if 0
typedef enum {
    COMBO_IF_UART = 0,
    COMBO_IF_MSDC = 1,
    COMBO_IF_MAX,
} COMBO_IF;
#endif
struct combo_ctrl {
    u32 size;
    void (*bgf_eirq_cb)(void);
    int (*audio_ctrl_cb)(u32, u32);
    void (*func_ctrl_cb)(u32, u32);
    void (*binfo_ctrl_cb)(u32, u32); /*index, value*/ 
};
/* functions for driver module init and deinit with kernel */
extern int mt_combo_init(struct combo_ctrl *ctrl);
extern int mt_combo_deinit(struct combo_ctrl *ctrl);

/* functions for driver module bgf irq control */
extern void mt_combo_bgf_enable_irq(void);
extern void mt_combo_bgf_disable_irq(void);

/* stub functions for kernel */
extern void mt_combo_bgf_eirq_handler(void *par);
#if 0
/* stub functions for kernel to control audio path pin mux */
extern int mt_combo_audio_ctrl_ex(COMBO_AUDIO_STATE state, u32 clt_ctrl);
static inline int mt_combo_audio_ctrl(COMBO_AUDIO_STATE state) {
    return mt_combo_audio_ctrl_ex(state, 1);
}
#endif
/* stub functions for kernel to control function on/off */
/* defined in mt_combo.c */
extern void mt_combo_func_ctrl(u32 type, u32 on);
extern void mt_combo_binfo_ctrl(u32 type, u32 on);

/* defined in project custom board.c */
extern void mt6620_power_on(void);
extern void mt6620_power_off(void);
extern int mt_combo_sdio_ctrl (unsigned int sdio_port_num, unsigned int on);
extern int mt_combo_gps_sync_pin_ctrl(int on);
extern int mt_combo_gps_lna_pin_ctrl(unsigned int on);

//extern int mt_combo_plt_enter_deep_idle(COMBO_IF src);
//extern int mt_combo_plt_exit_deep_idle(COMBO_IF src);
//#endif

#endif

