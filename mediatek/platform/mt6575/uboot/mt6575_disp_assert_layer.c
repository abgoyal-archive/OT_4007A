
#include <asm/arch/mt65xx_typedefs.h>
#include <asm/arch/mt65xx_disp_drv.h>
#include <asm/arch/mt65xx_disp_assert_layer.h>

#ifdef CONFIG_FB_MT6516_SUPPORT_ASSERTION_LAYER

#include <linux/string.h>
#include <linux/semaphore.h>
#include <asm/cacheflush.h>

#include "lcd_drv.h"
#include "mtkfb_console.h"

// ---------------------------------------------------------------------------

#define DAL_BPP             (2)
#define DAL_FORMAT          (LCD_LAYER_FORMAT_RGB565)
#define DAL_BG_COLOR        (0xF800)
#define DAL_FG_COLOR        (0xFFFF)

#define DAL_WIDTH           (DISP_GetScreenWidth())
#define DAL_HEIGHT          (DISP_GetScreenHeight())

#define MAKE_TWO_RGB565_COLOR(high, low)  (((low) << 16) | (high))

#define DAL_LOCK()                                                          \
    do {                                                                    \
        if (down_interruptible(&dal_sem)) {                                 \
            printk("[DAL] ERROR: Can't get semaphore in %s()\n",            \
                   __FUNCTION__);                                           \
            ASSERT(0);                                                      \
            return DAL_STATUS_LOCK_FAIL;                                    \
        }                                                                   \
    } while (0)
    
#define DAL_UNLOCK()                                                        \
    do {                                                                    \
        up(&dal_sem);                                                       \
    } while (0)


// ---------------------------------------------------------------------------

static MFC_HANDLE mfc_handle = NULL;

static void *dal_fb_addr = NULL;    
static BOOL  dal_shown   = FALSE;
    
DECLARE_MUTEX(dal_sem);

// ---------------------------------------------------------------------------

UINT32 DAL_GetLayerSize(void)
{
    return DAL_WIDTH * DAL_HEIGHT * DAL_BPP;
}


DAL_STATUS DAL_Init(UINT32 layerVA, UINT32 layerPA)
{
    dal_fb_addr = (void *)layerVA;
    
    MFC_CHECK_RET(MFC_Open(&mfc_handle, dal_fb_addr,
                           DAL_WIDTH, DAL_HEIGHT, DAL_BPP,
                           DAL_FG_COLOR, DAL_BG_COLOR));

    DAL_Clean();

    LCD_CHECK_RET(LCD_LayerSetAddress(ASSERT_LAYER, layerPA));
    LCD_CHECK_RET(LCD_LayerSetFormat(ASSERT_LAYER, DAL_FORMAT));
    LCD_CHECK_RET(LCD_LayerSetAlphaBlending(ASSERT_LAYER, TRUE, 0x80));
    LCD_CHECK_RET(LCD_LayerSetOffset(ASSERT_LAYER, 0, 0));
    LCD_CHECK_RET(LCD_LayerSetSize(ASSERT_LAYER,
                                   DAL_WIDTH,
                                   DAL_HEIGHT));
    return DAL_STATUS_OK;
}


DAL_STATUS DAL_Clean(void)
{
    const UINT32 BG_COLOR = MAKE_TWO_RGB565_COLOR(DAL_BG_COLOR, DAL_BG_COLOR);

    UINT32 i, *ptr;
    
    if (NULL == mfc_handle) 
        return DAL_STATUS_NOT_READY;

    DAL_LOCK();

    MFC_CHECK_RET(MFC_ResetCursor(mfc_handle));

    LCD_CHECK_RET(LCD_LayerEnable(ASSERT_LAYER, FALSE));

    ptr = (UINT32 *)dal_fb_addr;
    for(i = 0; i < DAL_GetLayerSize() / sizeof(UINT32); ++ i) {
        *ptr ++ = BG_COLOR;
    }

    dal_shown = FALSE;

    DAL_UNLOCK();

    return DAL_STATUS_OK;
}


DAL_STATUS DAL_Printf(const char *fmt, ...)
{
	va_list args;
	uint i;
	char printbuffer[1024] = "";

    if (NULL == mfc_handle) 
        return DAL_STATUS_NOT_READY;

    if (NULL == fmt)
        return DAL_STATUS_INVALID_ARGUMENT;

    DAL_LOCK();

	va_start (args, fmt);
	i = vsprintf(printbuffer, fmt, args);
	va_end (args);

    MFC_CHECK_RET(MFC_Print(mfc_handle, printbuffer));

    if (!dal_shown) {
        LCD_CHECK_RET(LCD_LayerEnable(ASSERT_LAYER, TRUE));
        dal_shown = TRUE;
    }

    flush_cache_all();

    DISP_CHECK_RET(DISP_UpdateScreen(0, 0, 
                                     DAL_WIDTH,
                                     DAL_HEIGHT));

    DAL_UNLOCK();

    return DAL_STATUS_OK;
}

// ##########################################################################
//  !CONFIG_FB_MT6516_SUPPORT_ASSERTION_LAYER
// ##########################################################################
#else

UINT32 DAL_GetLayerSize(void)
{
    return DISP_GetScreenWidth() * DISP_GetScreenHeight() * 2;
}
DAL_STATUS DAL_Init(UINT32 layerVA, UINT32 layerPA)
{
    NOT_REFERENCED(layerVA);
    NOT_REFERENCED(layerPA);
    
    return DAL_STATUS_OK;
}
DAL_STATUS DAL_Clean(void)
{
    return DAL_STATUS_OK;
}
DAL_STATUS DAL_Printf(const char *fmt, ...)
{
    NOT_REFERENCED(fmt);
    return DAL_STATUS_OK;
}

#endif  // CONFIG_FB_MT6516_SUPPORT_ASSERTION_LAYER

