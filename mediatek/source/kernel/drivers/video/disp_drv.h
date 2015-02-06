
#ifndef __DISP_DRV_H__
#define __DISP_DRV_H__

#if defined(CONFIG_ARCH_MT6516)
    #include <mach/mt6516_typedefs.h>
#elif defined(CONFIG_ARCH_MT6573)
    #include <mach/mt6573_typedefs.h>
#elif defined(CONFIG_ARCH_MT6575)
    #include <mach/mt6575_typedefs.h>
#else
    #error "unknown arch"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------

#define DISP_CHECK_RET(expr)                                                \
    do {                                                                    \
        DISP_STATUS ret = (expr);                                           \
        if (DISP_STATUS_OK != ret) {                                        \
            printk("[ERROR][mtkfb] DISP API return error code: 0x%x\n"      \
                   "  file : %s, line : %d\n"                               \
                   "  expr : %s\n", ret, __FILE__, __LINE__, #expr);        \
        }                                                                   \
    } while (0)


// ---------------------------------------------------------------------------

typedef enum
{	
   DISP_STATUS_OK = 0,

   DISP_STATUS_NOT_IMPLEMENTED,
   DISP_STATUS_ALREADY_SET,
   DISP_STATUS_ERROR,
} DISP_STATUS;


typedef enum {
   DISP_STATE_IDLE = 0,
   DISP_STATE_BUSY,
} DISP_STATE;


#define MAKE_PANEL_COLOR_FORMAT(R, G, B) ((R << 16) | (G << 8) | B)
#define PANEL_COLOR_FORMAT_TO_BPP(x) ((x&0xff) + ((x>>8)&0xff) + ((x>>16)&0xff))

typedef enum {
    PANEL_COLOR_FORMAT_RGB332 = MAKE_PANEL_COLOR_FORMAT(3, 3, 2),
    PANEL_COLOR_FORMAT_RGB444 = MAKE_PANEL_COLOR_FORMAT(4, 4, 4),
    PANEL_COLOR_FORMAT_RGB565 = MAKE_PANEL_COLOR_FORMAT(5, 6, 5),
    PANEL_COLOR_FORMAT_RGB666 = MAKE_PANEL_COLOR_FORMAT(6, 6, 6),
    PANEL_COLOR_FORMAT_RGB888 = MAKE_PANEL_COLOR_FORMAT(8, 8, 8),
} PANEL_COLOR_FORMAT;



// ---------------------------------------------------------------------------
// Public Functions
// ---------------------------------------------------------------------------

DISP_STATUS DISP_Init(UINT32 fbVA, UINT32 fbPA, BOOL isLcmInited);
DISP_STATUS DISP_Deinit(void);
DISP_STATUS DISP_PowerEnable(BOOL enable);
DISP_STATUS DISP_PanelEnable(BOOL enable);
DISP_STATUS DISP_SetFrameBufferAddr(UINT32 fbPhysAddr);
DISP_STATUS DISP_EnterOverlayMode(void);
DISP_STATUS DISP_LeaveOverlayMode(void);
DISP_STATUS DISP_EnableDirectLinkMode(UINT32 layer);
DISP_STATUS DISP_DisableDirectLinkMode(UINT32 layer);
DISP_STATUS DISP_UpdateScreen(UINT32 x, UINT32 y, UINT32 width, UINT32 height);


// Retrieve Information
UINT32 DISP_GetScreenWidth(void);
UINT32 DISP_GetScreenHeight(void);
UINT32 DISP_GetScreenBpp(void);
UINT32 DISP_GetPages(void);
BOOL   DISP_IsDirectLinkMode(void);
BOOL   DISP_IsInOverlayMode(void);
UINT32 DISP_GetVRamSize(void);
PANEL_COLOR_FORMAT DISP_GetPanelColorFormat(void);
UINT32 DISP_GetPanelBPP(void);
BOOL DISP_IsLcmFound(void);
BOOL DISP_IsImmediateUpdate(void);
DISP_STATUS DISP_ConfigImmediateUpdate(BOOL enable);

DISP_STATUS DISP_SetBacklight(UINT32 level);
DISP_STATUS DISP_SetPWM(UINT32 divider);
DISP_STATUS DISP_GetPWM(UINT32 divider, unsigned int *freq);

DISP_STATUS DISP_Set3DPWM(BOOL enable, BOOL landscape);

// FM De-sense
DISP_STATUS DISP_FMDesense_Query(void);
DISP_STATUS DISP_FM_Desense(unsigned long);
DISP_STATUS DISP_Reset_Update(void);
DISP_STATUS DISP_Get_Default_UpdateSpeed(unsigned int *speed);
DISP_STATUS DISP_Get_Current_UpdateSpeed(unsigned int *speed);
DISP_STATUS DISP_Change_Update(unsigned int);
///////////////

DISP_STATUS DISP_InitM4U(void);
DISP_STATUS DISP_ConfigAssertLayerMva(void);
DISP_STATUS DISP_AllocUILayerMva(unsigned int pa, unsigned int *mva, unsigned int size);
DISP_STATUS DISP_AllocOverlayMva(unsigned int va, unsigned int *mva, unsigned int size);
DISP_STATUS DISP_DeallocMva(unsigned int va, unsigned int mva, unsigned int size);
// ---------------------------------------------------------------------------
// Private Functions
// ---------------------------------------------------------------------------

typedef struct
{
    DISP_STATUS (*init)(UINT32 fbVA, UINT32 fbPA, BOOL isLcmInited);
    DISP_STATUS (*enable_power)(BOOL enable);
    DISP_STATUS (*set_fb_addr)(UINT32 fbPhysAddr);
    DISP_STATUS (*update_screen)(void);

    UINT32 (*get_vram_size)(void);
    PANEL_COLOR_FORMAT (*get_panel_color_format)(void);
    void (*init_te_control)(void);
	UINT32 (*get_dithering_bpp)(void);

	DISP_STATUS (*capture_framebuffer)(unsigned int pvbuf, unsigned int bpp);

    void (*esd_reset)(void);
	BOOL (*esd_check)(void);
} DISP_DRIVER;


const DISP_DRIVER *DISP_GetDriverDBI(void);
const DISP_DRIVER *DISP_GetDriverDPI(void);
const DISP_DRIVER *DISP_GetDriverDSI(void);


BOOL DISP_SelectDevice(const char* lcm_name);
BOOL DISP_DetectDevice(void);
BOOL DISP_SelectDeviceBoot(const char* lcm_name);
UINT32 DISP_GetVRamSizeBoot(char *cmdline);
DISP_STATUS DISP_Capture_Framebuffer(unsigned int pvbuf, unsigned int bpp);
BOOL DISP_IsContextInited(void);

DISP_STATUS DISP_Capture_Videobuffer(unsigned int pvbuf, unsigned int bpp, unsigned int video_rotation);
UINT32 DISP_GetOutputBPPforDithering(void);
BOOL DISP_IsLCDBusy(void);
DISP_STATUS DISP_ChangeLCDWriteCycle(void);
DISP_STATUS DISP_M4U_On(BOOL enable);

BOOL DISP_EsdRecoverCapbility(void);
BOOL DISP_EsdCheck(void);
BOOL DISP_EsdRecover(void);

// ---------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // __DISP_DRV_H__
