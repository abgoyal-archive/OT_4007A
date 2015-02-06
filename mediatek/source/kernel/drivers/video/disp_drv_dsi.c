
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/semaphore.h>
#include <linux/xlog.h>
#include <linux/spinlock.h>

#if defined(CONFIG_ARCH_MT6516)
    #include <mach/mt6516_boot.h>
#elif defined(CONFIG_ARCH_MT6573)
    #include <mach/mt6573_boot.h>
   	#include <mach/mt6573_reg_base.h>
#elif defined(CONFIG_ARCH_MT6575)
   	//#include <mach/mt6573_boot.h>
   	#include <mach/mt6575_reg_base.h>
#else
    #error "unknown arch"
#endif

#include <linux/disp_assert_layer.h>

#include "lcd_drv.h"
#include "lcd_reg.h"
#include "dpi_drv.h"
#include "dpi_reg.h"
#include "dsi_drv.h"
#include "dsi_reg.h"

#include "lcm_drv.h"

extern PLCD_REGS const LCD_REG;
extern PDSI_PHY_REGS const DSI_PHY_REG;

static UINT32 dsiTmpBufBpp = 0;
extern UINT32 dsi_buffer_bpp; 
// ---------------------------------------------------------------------------
//  Private Variables
// ---------------------------------------------------------------------------

extern LCM_DRIVER *lcm_drv;
extern LCM_PARAMS *lcm_params;

typedef struct
{
    UINT32 pa;
    UINT32 pitchInBytes;
} TempBuffer;

static TempBuffer s_tmpBuffers[3];
static bool	dsi_vdo_streaming = false;

// ---------------------------------------------------------------------------
//  Private Functions
// ---------------------------------------------------------------------------

static void init_lcd_te_control(void)
{
    const LCM_DBI_PARAMS *dbi = &(lcm_params->dbi);

    if (LCM_DBI_TE_MODE_DISABLED == dbi->te_mode) {
        LCD_CHECK_RET(LCD_TE_Enable(FALSE));
        return;
    }

    if (LCM_DBI_TE_MODE_VSYNC_ONLY == dbi->te_mode) {
        LCD_CHECK_RET(LCD_TE_SetMode(LCD_TE_MODE_VSYNC_ONLY));
    } else if (LCM_DBI_TE_MODE_VSYNC_OR_HSYNC == dbi->te_mode) {
        LCD_CHECK_RET(LCD_TE_SetMode(LCD_TE_MODE_VSYNC_OR_HSYNC));
        LCD_CHECK_RET(LCD_TE_ConfigVHSyncMode(dbi->te_hs_delay_cnt,
                                              dbi->te_vs_width_cnt,
                     (LCD_TE_VS_WIDTH_CNT_DIV)dbi->te_vs_width_cnt_div));
    } else ASSERT(0);

    LCD_CHECK_RET(LCD_TE_SetEdgePolarity(dbi->te_edge_polarity));
    LCD_CHECK_RET(LCD_TE_Enable(TRUE));
}

__inline DPI_FB_FORMAT get_dsi_tmp_buffer_format(void)
{
    switch(lcm_params->dsi.data_format.format)
    {
    case LCM_DSI_FORMAT_RGB565 : return 0;
    case LCM_DSI_FORMAT_RGB666 :
    case LCM_DSI_FORMAT_RGB888 : return 1;
    default : ASSERT(0);
    }
    return 1;
}


__inline UINT32 get_dsi_tmp_buffer_bpp(void)
{
    static const UINT32 TO_BPP[] = {2, 3};
    dsi_buffer_bpp = TO_BPP[get_dsi_tmp_buffer_format()];
    return TO_BPP[get_dsi_tmp_buffer_format()];
}


__inline LCD_FB_FORMAT get_lcd_tmp_buffer_format(void)
{
    static const UINT32 TO_LCD_FORMAT[] = {
        LCD_FB_FORMAT_RGB565,
        LCD_FB_FORMAT_RGB888
    };
    
    return TO_LCD_FORMAT[get_dsi_tmp_buffer_format()];
}

static __inline LCD_IF_WIDTH to_lcd_if_width(LCM_DBI_DATA_WIDTH data_width)
{
    switch(data_width)
    {
    case LCM_DBI_DATA_WIDTH_8BITS  : return LCD_IF_WIDTH_8_BITS;
    case LCM_DBI_DATA_WIDTH_9BITS  : return LCD_IF_WIDTH_9_BITS;
    case LCM_DBI_DATA_WIDTH_16BITS : return LCD_IF_WIDTH_16_BITS;
    case LCM_DBI_DATA_WIDTH_18BITS : return LCD_IF_WIDTH_18_BITS;
    case LCM_DBI_DATA_WIDTH_24BITS : return LCD_IF_WIDTH_24_BITS;
    default : ASSERT(0);
    }
    return LCD_IF_WIDTH_18_BITS;
}

static BOOL disp_drv_dsi_init_context(void)
{
    if (lcm_drv != NULL && lcm_params != NULL){
		dsiTmpBufBpp=get_dsi_tmp_buffer_bpp();
		return TRUE;
	}

    if (NULL == lcm_drv) {
        return FALSE;
    }

    lcm_drv->get_params(lcm_params);

	dsiTmpBufBpp=get_dsi_tmp_buffer_bpp();
    
    return TRUE;
}

#define ALIGN_TO(x, n)  \
	(((x) + ((n) - 1)) & ~((n) - 1))

static UINT32 get_fb_size(void)
{
    return ALIGN_TO(DISP_GetScreenWidth(), 32) * 
           ALIGN_TO(DISP_GetScreenHeight(), 32) *
           ((DISP_GetScreenBpp() + 7) >> 3) * 
           DISP_GetPages();
}


static UINT32 get_intermediate_buffer_size(void)
{
    disp_drv_dsi_init_context();

    return 
    DISP_GetScreenWidth() *
    DISP_GetScreenHeight() *           
    dsiTmpBufBpp *
    lcm_params->dsi.intermediat_buffer_num;
}


static UINT32 get_assert_layer_size(void)
{
    return DAL_GetLayerSize();
}


static void init_intermediate_buffers(UINT32 fbPhysAddr)
{
    UINT32 tmpFbStartPA = fbPhysAddr + get_fb_size();

    UINT32 tmpFbPitchInBytes = DISP_GetScreenWidth() * dsiTmpBufBpp;
    UINT32 tmpFbSizeInBytes  = tmpFbPitchInBytes * DISP_GetScreenHeight();	

    UINT32 i;

	xlog_printk(ANDROID_LOG_INFO, "DSI", "[DISP] kernel - init_intermediate_buffers \n");
	xlog_printk(ANDROID_LOG_INFO, "DSI", "[DISP] kernel - tmpFbStartPA=%x, fbPhysAddr=%x \n", tmpFbStartPA, fbPhysAddr);
    
    for (i = 0; i < lcm_params->dsi.intermediat_buffer_num; ++ i)
    {
        TempBuffer *b = &s_tmpBuffers[i];
      
        b->pitchInBytes = tmpFbPitchInBytes;
        b->pa = tmpFbStartPA;
        ASSERT((tmpFbStartPA & 0x7) == 0);  // check if 8-byte-aligned
        tmpFbStartPA += tmpFbSizeInBytes;
    }

}


static void init_assertion_layer(UINT32 fbVA, UINT32 fbPA)
{
	UINT32 offset;
    DAL_STATUS ret;

    disp_drv_dsi_init_context();

	if(lcm_params->dsi.mode == CMD_MODE)
		offset = get_fb_size();
	else
		offset = get_fb_size() + get_intermediate_buffer_size();

    ret = DAL_Init(fbVA + offset, fbPA + offset);
    ASSERT(DAL_STATUS_OK == ret);
}


void dsi_enable_dc2lcd(void)
{
    LCD_REG_DSI_DC tmp_reg;

    DSI_WaitForNotBusy();
		
    tmp_reg=LCD_REG->DS_DSI_CON;
    tmp_reg.DC_DSI=1;
    OUTREG32(&LCD_REG->DS_DSI_CON, AS_UINT32(&tmp_reg));  
}

void dsi_disable_dc2lcd(void)
{
    LCD_REG_DSI_DC tmp_reg;

    DSI_WaitForNotBusy();

    tmp_reg=LCD_REG->DS_DSI_CON;
    tmp_reg.DC_DSI=0;
    OUTREG32(&LCD_REG->DS_DSI_CON, AS_UINT32(&tmp_reg));  
}


static void init_lcd(void)
{
	UINT32 i;		

	// Config LCD Controller
	LCD_CHECK_RET(LCD_LayerEnable(LCD_LAYER_ALL, FALSE));
	LCD_CHECK_RET(LCD_LayerSetTriggerMode(LCD_LAYER_ALL, LCD_SW_TRIGGER));
	LCD_CHECK_RET(LCD_EnableHwTrigger(FALSE));

	LCD_CHECK_RET(LCD_SetBackgroundColor(0));

	if(lcm_params->dsi.mode == CMD_MODE)
		LCD_CHECK_RET(LCD_SetRoiWindow(0, 0, lcm_params->width, lcm_params->height));
	else
		LCD_CHECK_RET(LCD_SetRoiWindow(0, 0, DISP_GetScreenWidth(), DISP_GetScreenHeight()));

	if(lcm_params->dsi.mode == CMD_MODE) {
		LCD_CHECK_RET(LCD_ConfigDSIIfFormat(lcm_params->dsi.data_format.color_order,
								lcm_params->dsi.data_format.trans_seq,
								lcm_params->dsi.data_format.padding,
								lcm_params->dsi.data_format.format,
								lcm_params->dsi.packet_size,
								TRUE));

		LCD_CHECK_RET(LCD_SetOutputMode(LCD_OUTPUT_TO_LCM));
		LCD_CHECK_RET(LCD_WaitDPIIndication(FALSE));
	    LCD_CHECK_RET(LCD_FBEnable(LCD_FB_0, FALSE));
	    LCD_CHECK_RET(LCD_FBEnable(LCD_FB_1, FALSE));
	    LCD_CHECK_RET(LCD_FBEnable(LCD_FB_2, FALSE));
	} else {
		LCD_CHECK_RET(LCD_FBSetFormat(get_lcd_tmp_buffer_format()));
		LCD_CHECK_RET(LCD_FBSetPitch(s_tmpBuffers[0].pitchInBytes));
		LCD_CHECK_RET(LCD_FBSetStartCoord(0, 0));

		for (i = 0; i < lcm_params->dsi.intermediat_buffer_num; ++ i)
		{
			LCD_CHECK_RET(LCD_FBSetAddress(LCD_FB_0 + i, s_tmpBuffers[i].pa));
			LCD_CHECK_RET(LCD_FBEnable(LCD_FB_0 + i, TRUE));
		}
	
		LCD_CHECK_RET(LCD_SetOutputMode(LCD_OUTPUT_TO_MEM));
		LCD_CHECK_RET(LCD_WaitDPIIndication(TRUE));
	}
}
static void init_dpi(BOOL isDpiPoweredOn)
{
    const LCM_DSI_PARAMS *dsi = &(lcm_params->dsi);
    UINT32 i;
	DPI_REG_CNTL tmp_reg;

    DPI_CHECK_RET(DPI_Init(isDpiPoweredOn));

    DPI_CHECK_RET(DPI_EnableSeqOutput(FALSE));
	
    DPI_CHECK_RET(DPI_FBSetSize(DISP_GetScreenWidth(), DISP_GetScreenHeight()));	
    
    for (i = 0; i < lcm_params->dsi.intermediat_buffer_num; ++ i)
    {
        DPI_CHECK_RET(DPI_FBSetAddress(DPI_FB_0 + i, s_tmpBuffers[i].pa));
        DPI_CHECK_RET(DPI_FBSetPitch(DPI_FB_0 + i, s_tmpBuffers[i].pitchInBytes));
        DPI_CHECK_RET(DPI_FBEnable(DPI_FB_0 + i, TRUE));
    }
    DPI_CHECK_RET(DPI_FBSetFormat(get_dsi_tmp_buffer_format()));
    DPI_CHECK_RET(DPI_FBSyncFlipWithLCD(TRUE));

    if (LCM_COLOR_ORDER_BGR == dsi->data_format.color_order) {
        DPI_CHECK_RET(DPI_SetRGBOrder(DPI_RGB_ORDER_RGB, DPI_RGB_ORDER_BGR));
    } else {
        DPI_CHECK_RET(DPI_SetRGBOrder(DPI_RGB_ORDER_RGB, DPI_RGB_ORDER_RGB));
    }

	DPI_CHECK_RET(DPI_SetDSIMode(TRUE));

}


void init_dsi(BOOL isDsiPoweredOn)
{
	DSI_TXRX_CTRL_REG tmp_reg1;
	DSI_PSCTRL_REG tmp_reg2;

	DSI_CHECK_RET(DSI_Init(isDsiPoweredOn));

	tmp_reg1=DSI_REG->DSI_TXRX_CTRL;
	tmp_reg1.CKSM_EN=1;
	tmp_reg1.ECC_EN=1;
	tmp_reg1.LANE_NUM=lcm_params->dsi.LANE_NUM;
	tmp_reg1.VC_NUM=0;
	OUTREG32(&DSI_REG->DSI_TXRX_CTRL, AS_UINT32(&tmp_reg1));	
	
	//initialize DSI_PHY
	DSI_PHY_clk_switch(TRUE);
	DSI_PHY_TIMCONFIG(lcm_params);
	DSI_PHY_clk_setting(lcm_params->dsi.pll_div1, lcm_params->dsi.pll_div2, lcm_params->dsi.LANE_NUM);

	if(lcm_params->dsi.mode != CMD_MODE)
	{
		DSI_Config_VDO_Timing(lcm_params);

		tmp_reg2.DSI_PS_SEL=lcm_params->dsi.PS;
		tmp_reg2.DSI_PS_WC=lcm_params->width * dsiTmpBufBpp;

		OUTREG32(&DSI_REG->DSI_PSCTRL, AS_UINT32(&tmp_reg2));	
	}
	
	// FIX ME
#if defined(CONFIG_ARCH_MT6573)
	*(volatile unsigned int *) (APCONFIG_BASE+0x0044) |= 0x00000100;	// enable MIPI TX IO
#else
	//*(volatile unsigned int *) (0xc0001890) |= 0x00000100;	// enable MIPI TX IO
	*(volatile unsigned int *) (INFRA_SYS_CFG_BASE+0x890) |= 0x00000100;	// enable MIPI TX IO	
#endif
	
}


//DECLARE_MUTEX(sem_handle_esd);

// called by DPI ISR.
void dsi_handle_esd_recovery(void)
{
    unsigned long flag;

    //spin_lock_irqsave(&sem_handle_esd, flag);

	DSI_handle_esd_recovery();

    //spin_unlock_irqrestore(&sem_handle_esd, flag);
}

extern spinlock_t g_handle_esd_lock;

// ---------------------------------------------------------------------------
//  DBI Display Driver Public Functions
// ---------------------------------------------------------------------------

bool DDMS_capturing=0;

static DISP_STATUS dsi_init(UINT32 fbVA, UINT32 fbPA, BOOL isLcmInited)
{
	if (!disp_drv_dsi_init_context()) 
		return DISP_STATUS_NOT_IMPLEMENTED;

	if(lcm_params->dsi.mode == CMD_MODE) {
	
		init_lcd();
		init_dsi(isLcmInited);

		init_assertion_layer(fbVA, fbPA);		

		if (NULL != lcm_drv->init && !isLcmInited) 
		{
			lcm_drv->init();
		}
	
		DSI_clk_HS_mode(0);

		DSI_SetMode(lcm_params->dsi.mode);

		DPI_PowerOn();
		DPI_PowerOff();

		init_lcd_te_control();
	}
	else {

		spin_lock(&g_handle_esd_lock);

		init_intermediate_buffers(fbPA);
		
	    init_lcd();
		init_dpi(isLcmInited);
		init_dsi(isLcmInited);

		init_assertion_layer(fbVA, fbPA);

		if (NULL != lcm_drv->init && !isLcmInited) {
			lcm_drv->init();
		}

		DSI_SetMode(lcm_params->dsi.mode);
	
		if(lcm_params->dsi.lcm_ext_te_monitor)
		{
			dsi_vdo_streaming = false;
			LCD_TE_SetMode(LCD_TE_MODE_VSYNC_ONLY);
			LCD_TE_SetEdgePolarity(LCM_POLARITY_RISING);
			LCD_TE_Enable(FALSE);
		}

		if(lcm_params->dsi.noncont_clock)
			DSI_set_noncont_clk(false, lcm_params->dsi.noncont_clock_period);

		if(lcm_params->dsi.lcm_int_te_monitor)
			DSI_set_int_TE(false, lcm_params->dsi.lcm_int_te_period);			

		spin_unlock(&g_handle_esd_lock);
			
	}

	return DISP_STATUS_OK;
}


// protected by sem_early_suspend, sem_update_screen
static DISP_STATUS dsi_enable_power(BOOL enable)
{
	//DSI_PHY_REG_ANACON0 mipitx_con0 = DSI_PHY_REG->ANACON0;

	disp_drv_dsi_init_context();
	
	if(lcm_params->dsi.mode == CMD_MODE) {

		if (enable) {
			// FIX ME
#if defined(CONFIG_ARCH_MT6573)
			*(volatile unsigned int *) (APCONFIG_BASE+0x0044) |= 0x00000100;	// enable MIPI TX IO
#else
			*(volatile unsigned int *) (INFRA_SYS_CFG_BASE+0x890) |= 0x00000100;	// enable MIPI TX IO	
#endif
			DSI_PHY_clk_switch(1);
			DSI_PHY_clk_setting(lcm_params->dsi.pll_div1, lcm_params->dsi.pll_div2, lcm_params->dsi.LANE_NUM);
			DSI_CHECK_RET(DSI_PowerOn());
			DSI_WaitForNotBusy();		
			DSI_clk_HS_mode(0);
			DSI_clk_ULP_mode(0);			
			DSI_lane0_ULP_mode(0);	
			LCD_CHECK_RET(LCD_PowerOn());
		} else {
			LCD_CHECK_RET(LCD_PowerOff());
			DSI_clk_HS_mode(0);
			DSI_lane0_ULP_mode(1);
			DSI_clk_ULP_mode(1);
			DSI_CHECK_RET(DSI_PowerOff());
			DSI_PHY_clk_switch(0);
			// FIX ME
#if defined(CONFIG_ARCH_MT6573)
			*(volatile unsigned int *) (APCONFIG_BASE+0x0044) &= ~0x00000100;	// disable MIPI TX IO
#else
			*(volatile unsigned int *) (INFRA_SYS_CFG_BASE+0x890) &= ~0x00000100;	// disable MIPI TX IO	
#endif
		}
	} else {

		spin_lock(&g_handle_esd_lock);

	    if (enable) {
			// FIX ME
			// Switch bus to MIPI TX.
#if defined(CONFIG_ARCH_MT6573)
			*(volatile unsigned int *) (APCONFIG_BASE+0x0044) |= 0x00000100;		// enable MIPI TX IO
#else
			*(volatile unsigned int *) (INFRA_SYS_CFG_BASE+0x890) |= 0x00000100;	// enable MIPI TX IO	
#endif
			DSI_PHY_clk_switch(1);
			DSI_PHY_clk_setting(lcm_params->dsi.pll_div1, lcm_params->dsi.pll_div2, lcm_params->dsi.LANE_NUM);
	        DSI_CHECK_RET(DSI_PowerOn());			
			DSI_clk_ULP_mode(0);			
			DSI_lane0_ULP_mode(0);
			DSI_clk_HS_mode(0);	
			
	        DPI_CHECK_RET(DPI_PowerOn());
	        LCD_CHECK_RET(LCD_PowerOn());

	    } else {

			dsi_vdo_streaming = false;

			if(lcm_params->dsi.noncont_clock)
				DSI_set_noncont_clk(false, lcm_params->dsi.noncont_clock_period);
			
			if(lcm_params->dsi.lcm_int_te_monitor)
				DSI_set_int_TE(false, lcm_params->dsi.lcm_int_te_period);

	        LCD_CHECK_RET(LCD_PowerOff());		
	        DPI_CHECK_RET(DPI_PowerOff());

			DSI_lane0_ULP_mode(1);
			DSI_clk_ULP_mode(1);	
	        DSI_CHECK_RET(DSI_PowerOff());

			DSI_PHY_clk_switch(0);

			// FIX ME
			// Switch bus to GPIO, then power level will be decided by GPIO setting.
#if defined(CONFIG_ARCH_MT6573)
			*(volatile unsigned int *) (APCONFIG_BASE+0x0044) &= ~0x00000100;		// disable MIPI TX IO
#else
			*(volatile unsigned int *) (INFRA_SYS_CFG_BASE+0x890) &= ~0x00000100;	// disable MIPI TX IO	
#endif			
	    }

		spin_unlock(&g_handle_esd_lock);
		
	}

    return DISP_STATUS_OK;
}

static DISP_STATUS dsi_set_fb_addr(UINT32 fbPhysAddr)
{
    LCD_CHECK_RET(LCD_LayerSetAddress(FB_LAYER, fbPhysAddr));

    return DISP_STATUS_OK;
}


// protected by sem_flipping, sem_early_suspend, sem_overlay_buffer, sem_update_screen
static DISP_STATUS dsi_update_screen(void)
{
	disp_drv_dsi_init_context();

#if defined(CONFIG_ARCH_MT6573)
	*(volatile unsigned int *) (APCONFIG_BASE+0x0044) |= 0x00000100;	// enable MIPI TX IO
#else
	//*(volatile unsigned int *) (0xc0001890) |= 0x00000100;	// enable MIPI TX IO
	*(volatile unsigned int *) (INFRA_SYS_CFG_BASE+0x890) |= 0x00000100;	// enable MIPI TX IO	
#endif

	//printk("[DISP] kernel - DSI_handle_TE \n"); 	
	//DSI_CHECK_RET(DSI_handle_TE());

	DSI_SetMode(lcm_params->dsi.mode);

	LCD_CHECK_RET(LCD_StartTransfer(FALSE));

	if (lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode == CMD_MODE && !DDMS_capturing) {
		DSI_clk_HS_mode(1);
		DSI_CHECK_RET(DSI_EnableClk());
	}
	else if (lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode != CMD_MODE && !DDMS_capturing)
	{
		spin_lock(&g_handle_esd_lock);
	
		DSI_clk_HS_mode(1);
		DPI_CHECK_RET(DPI_EnableClk());
		DSI_CHECK_RET(DSI_EnableClk());

		dsi_vdo_streaming = true;
		
		if(lcm_params->dsi.noncont_clock)
			DSI_set_noncont_clk(true, lcm_params->dsi.noncont_clock_period);
	
		if(lcm_params->dsi.lcm_int_te_monitor)
			DSI_set_int_TE(true, lcm_params->dsi.lcm_int_te_period);

		spin_unlock(&g_handle_esd_lock);
	}

	if (DDMS_capturing)
		xlog_printk(ANDROID_LOG_INFO, "DSI", "[DISP] kernel - dsi_update_screen. DDMS is capturing. Skip one frame. \n");		

	return DISP_STATUS_OK;
}


static UINT32 dsi_get_vram_size(void)
{
    UINT32 vramSize;

    disp_drv_dsi_init_context();

	if(lcm_params->dsi.mode == CMD_MODE) {
		vramSize = get_fb_size() + get_assert_layer_size();
	} else {
		vramSize = get_fb_size() + get_assert_layer_size() + get_intermediate_buffer_size();
	}
    
    return vramSize;
}

static PANEL_COLOR_FORMAT dsi_get_panel_color_format(void)
{
    disp_drv_dsi_init_context();

	{

	    switch(lcm_params->dsi.data_format.format)
	    {
		    case LCM_DSI_FORMAT_RGB565 : return PANEL_COLOR_FORMAT_RGB565;
		    case LCM_DSI_FORMAT_RGB666 : return PANEL_COLOR_FORMAT_RGB666;
		    case LCM_DSI_FORMAT_RGB888 : return PANEL_COLOR_FORMAT_RGB888;
		    default : ASSERT(0);
	    }
		
	}
}

static UINT32 dsi_get_dithering_bpp(void)
{
	return PANEL_COLOR_FORMAT_TO_BPP(dsi_get_panel_color_format());
}


// protected by sem_early_suspend
DISP_STATUS dsi_capture_framebuffer(UINT32 pvbuf, UINT32 bpp)
{
	LCD_REG_DSI_DC tmp_reg;

	tmp_reg=LCD_REG->DS_DSI_CON;

	DSI_CHECK_RET(DSI_WaitForNotBusy());

	DDMS_capturing=1;

	if(lcm_params->dsi.mode == CMD_MODE)
	{
		// W2DSI = 0
		tmp_reg.DC_DSI=0;
		OUTREG32(&LCD_REG->DS_DSI_CON, AS_UINT32(&tmp_reg));

	    LCD_CHECK_RET(LCD_Capture_Framebuffer(pvbuf, bpp));
	}
	else
	{
	    DPI_CHECK_RET(DPI_Capture_Framebuffer(pvbuf, bpp));
	}


	if(lcm_params->dsi.mode == CMD_MODE)
	{
		// W2DSI = 1
		tmp_reg.DC_DSI=1;
		OUTREG32(&LCD_REG->DS_DSI_CON, AS_UINT32(&tmp_reg));
	}

	DDMS_capturing=0;

	return DISP_STATUS_OK;	
}


// called by "esd_recovery_kthread"
// protected by sem_early_suspend, sem_update_screen
BOOL dsi_esd_check(void)
{
	BOOL result = false;

	if(lcm_params->dsi.mode == CMD_MODE || !dsi_vdo_streaming)
		return false;
	else
	{
		if(lcm_params->dsi.lcm_int_te_monitor)
			result = DSI_esd_check();

		if(result)
			return true;

		if(lcm_params->dsi.lcm_ext_te_monitor)
			result = LCD_esd_check();

		return result;
	}
}


// called by "esd_recovery_kthread"
// protected by sem_early_suspend, sem_update_screen
void dsi_esd_reset()
{
     /// we assume the power is on here
    ///  what we need is some setting for LCM init
    if(lcm_params->dsi.mode == CMD_MODE) {
        DSI_PHY_clk_switch(1);
        DSI_PHY_clk_setting(lcm_params->dsi.pll_div1, lcm_params->dsi.pll_div2, lcm_params->dsi.LANE_NUM);
        DSI_clk_HS_mode(0);
        DSI_clk_ULP_mode(0);            
        DSI_lane0_ULP_mode(0);  
    }
	else {

		DSI_SetMode(CMD_MODE);
        DSI_clk_HS_mode(0);
		// clock/data lane go to Ideal
		DSI_Reset();
		DPI_CHECK_RET(DPI_DisableClk());

	}
	
}


const DISP_DRIVER *DISP_GetDriverDSI()
{
    static const DISP_DRIVER DSI_DISP_DRV =
    {
        .init                   = dsi_init,
        .enable_power           = dsi_enable_power,
        .set_fb_addr            = dsi_set_fb_addr,
        .update_screen          = dsi_update_screen,       
        .get_vram_size          = dsi_get_vram_size,

        .get_panel_color_format = dsi_get_panel_color_format,
        .init_te_control        = init_lcd_te_control,
        .get_dithering_bpp		= dsi_get_dithering_bpp,
				.capture_framebuffer		= dsi_capture_framebuffer,
        .esd_reset              = dsi_esd_reset,
        .esd_check				= dsi_esd_check,
    };

    return &DSI_DISP_DRV;
}

