

#include <linux/string.h>

#if defined(BUILD_UBOOT)
#include <asm/arch/mt6575_gpio.h>
#else
#include <mach/mt6575_gpio.h>
#endif

#ifndef BUILD_UBOOT
#include <linux/kernel.h>
#endif

#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(320)
#define FRAME_HEIGHT 										(480)
#define LCM_ID       (0x53)
#define REGFLAG_DELAY             							                  0XFE
#define REGFLAG_END_OF_TABLE      							0xFF   // END OF REGISTERS MARKER


#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
#define LCM_DSI_CMD_MODE									1

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

struct LCM_setting_table {
    unsigned char cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {

	/*
	Note :

	Data ID will depends on the following rule.

		count of parameters > 1	=> Data ID = 0x39
		count of parameters = 1	=> Data ID = 0x15
		count of parameters = 0	=> Data ID = 0x05

	Structure Format :

	{DCS command, count of parameters, {parameter list}}
	{REGFLAG_DELAY, milliseconds of time, {}},

	...

	Setting ending by predefined flag

	{REGFLAG_END_OF_TABLE, 0x00, {}}
	*/

        {0xED,	2,	{0x01, 0xFE}},
        {0xEE,	2,	{0xDE, 0x21}},

	{0xF1,	1,	{0x01}},

       {0xDF,	1,	{0x10}},
      
       {0xB7,	1,	{0x00}},
       {0xC0,	4,	{0x44, 0x44, 0x10, 0x10}},
       {0xC2,	3,	{0x44, 0x44, 0x44}},
       {0xC4,	1,	{0x72}},
       {0xC6,	4,	{0x00, 0xE2, 0xE2, 0xE2}},
       {0xBF,	1,	{0xAA}},
       {0xB0,	18,	{0x0D, 0x00, 0x0D, 0x00, 0x11, 0x00, 0x19, 0x00, 0x21, 0x00, 0x2D,0x00, 0x3D, 0x00, 0x5D, 0x00, 0x5D, 0x00}},

       {0xB1,	6,	{0x80, 0x00, 0x8B, 0x00, 0x96, 0x00}},
       {0xB2,	6,	{0x00, 0x00, 0x02, 0x00, 0x03, 0x00}},
       {0xB3,	24,	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00,0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
       {0xB4,	6,	{0x8B, 0x00, 0x96, 0x00, 0xA1, 0x00}},
       {0xB5,	6,	{0x02, 0x00, 0x03, 0x00, 0x04, 0x00}},
       {0xB6,	2,	{0x00, 0x00}},
       {0xB7,	22,	{0x00, 0x00, 0x3F, 0x00, 0x5E, 0x00, 0x64, 0x00, 0x8C, 0x00, 0xAC,0x00, 0xDC, 0x00, 0x70, 0x00, 0x90, 0x00, 0xEB, 0x00, 0xDC, 0x00}},
       {0xB8,	8,	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00}},
       {0xBA,	4,	{0x24, 0x00, 0x00, 0x00}},
       {0xC1,	6,	{0x20, 0x00, 0x54, 0x00, 0xFF, 0x00}},
       {0xC2,	4,	{0x0A, 0x00, 0x04, 0x00}},
       {0xC3,	48,	{0x3C, 0x00, 0x3A, 0x00, 0x39, 0x00, 0x37, 0x00, 0x3C, 0x00, 0x36,0x00, 0x32, 0x00, 0x2F, 0x00,0x2C,0x00, 0x29, 0x00, 0x26, 0x00, 0x24, 0x00, 0x24, 0x00, 0x23, 0x00, 0x3C,0x00,0x36,0x00,0x32,0x00,0x2F,0x00,0x2C,0x00,0x29,0x00,0x26,0x00,0x24,0x00,0x24,0x00,0x23,0x00}},
       {0xC4,	26,	{0x62,0x00,0x05,0x00,0x84,0x00,0xF0,0x00,0x18,0x00,0xA4,0x00,0x18,0x00,0x50,0x00,0x0C,0x00,0x17,0x00,0x95,0x00,0xF3,0x00,0xE6,0x00 }},
       {0xC5,	10,	{0x32,0x00,0x44,0x00,0x65,0x00,0x76,0x00,0x88,0x00}},
       {0xC6,	6,	{0x20,0x00,0x17,0x00,0x01,0x00}},
       {0xC7,	4,	{0x00,0x00,0x00,0x00}},
       {0xC8,	4,	{0x00,0x00,0x00,0x00}},
       {0xC9,	16,	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
       {0xE0,	36,	{0x01,0x00,0x0F,0x00,0x26,0x00,0x44,0x00,0x56,0x00,0x62,0x00,0x73,0x00,0x87,0x00,0x95,0x00,0xA1,0x00,0xAC,0x00,0xB8,0x00,0xC0,0x00,0xC6,0x00,0xCC,0x00,0xD1,0x00,0xD8,0x00,0xF3,0x00}},
       {0xE1,	36,	{0x01,0x00,0x0F,0x00,0x26,0x00,0x44,0x00,0x56,0x00,0x62,0x00,0x73,0x00,0x87,0x00,0x95,0x00,0xA1,0x00,0xAC,0x00,0xB8,0x00,0xC0,0x00,0xC6,0x00,0xCC,0x00,0xD1,0x00,0xD8,0x00,0xF3,0x00}},
       {0xE2,	36,	{0x01,0x00,0x0F,0x00,0x26,0x00,0x44,0x00,0x56,0x00,0x62,0x00,0x73,0x00,0x87,0x00,0x95,0x00,0xA1,0x00,0xAC,0x00,0xB8,0x00,0xC0,0x00,0xC6,0x00,0xCC,0x00,0xD1,0x00,0xD8,0x00,0xF3,0x00}},
       {0xE3,	36,	{0x01,0x00,0x0F,0x00,0x26,0x00,0x44,0x00,0x56,0x00,0x62,0x00,0x73,0x00,0x87,0x00,0x95,0x00,0xA1,0x00,0xAC,0x00,0xB8,0x00,0xC0,0x00,0xC6,0x00,0xCC,0x00,0xD1,0x00,0xD8,0x00,0xF3,0x00}},
       {0xE4,	36,	{0x01,0x00,0x0F,0x00,0x26,0x00,0x44,0x00,0x56,0x00,0x62,0x00,0x73,0x00,0x87,0x00,0x95,0x00,0xA1,0x00,0xAC,0x00,0xB8,0x00,0xC0,0x00,0xC6,0x00,0xCC,0x00,0xD1,0x00,0xD8,0x00,0xF3,0x00}},
       {0xE5,	36,	{0x01,0x00,0x0F,0x00,0x26,0x00,0x44,0x00,0x56,0x00,0x62,0x00,0x73,0x00,0x87,0x00,0x95,0x00,0xA1,0x00,0xAC,0x00,0xB8,0x00,0xC0,0x00,0xC6,0x00,0xCC,0x00,0xD1,0x00,0xD8,0x00,0xF3,0x00}},
        
       {0xE6,	32,	{0x11,0x00,0x22,0x00,0x77,0x00,0x88,0x00,0x99,0x00,0x88,0x00,0x99,0x00,0xAA,0x00,0xBB,0x00,0x99,0x00,0x77,0x00,0x66,0x00,0x55,0x00,0x55,0x00,0x55,0x00,0x55,0x00}},
       {0xE7,	32,	{0x11,0x00,0x22,0x00,0x77,0x00,0x88,0x00,0x99,0x00,0x88,0x00,0x99,0x00,0xAA,0x00,0xBB,0x00,0x99,0x00,0x77,0x00,0x66,0x00,0x55,0x00,0x55,0x00,0x55,0x00,0x55,0x00}},
       {0xE8,	32,	{0x11,0x00,0x22,0x00,0x77,0x00,0x88,0x00,0x99,0x00,0x88,0x00,0x99,0x00,0xAA,0x00,0xBB,0x00,0x99,0x00,0x77,0x00,0x66,0x00,0x55,0x00,0x55,0x00,0x55,0x00,0x55,0x00}},
       {0xE9,	4,	{0xAA, 0x00, 0x00, 0x00}},
       {0x00,	1,	{0xAA}},
       {0xF3,	1,	{0x00}},
       {0xF9,	4,	{0x06,0x10,0x29,0x00}},
       {0xCF,	17,	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
       {0xF0,	5,	{0x00,0x50,0x00,0x00,0x00}},

       {0x3A,	1,	{0x66}},
   
      {0xB3,	1,	{0x20}},

    	

       {0x11,	1,	{0x00}},
	{REGFLAG_DELAY, 120, {}},

	{0x29,	1,	{0x00}},
	{REGFLAG_DELAY, 50, {}},

     //	{0x2C,	1,	{0x00}},

   	//{REGFLAG_DELAY, 50, {}},


	// Note
	// Strongly recommend not to set Sleep out / Display On here. That will cause messed frame to be shown as later the backlight is on.

	// Setting ending by predefined flag

	{REGFLAG_END_OF_TABLE, 0x00, {}}  
};


#if 0
static struct LCM_setting_table lcm_set_window[] = {
	{0x2A,	4,	{0x00, 0x00, (FRAME_WIDTH>>8), (FRAME_WIDTH&0xFF)}},
	{0x2B,	4,	{0x00, 0x00, (FRAME_HEIGHT>>8), (FRAME_HEIGHT&0xFF)}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif

static struct LCM_setting_table lcm_sleep_out_setting[] = {

      // {0x11, 1, {0x00}},

	//{0x29, 1, {0x00}},
    // Sleep Out
	{0x11, 1, {0x00}},
        {REGFLAG_DELAY, 150, {}},

    // Display ON
	{0x29, 1, {0x00}},
        {REGFLAG_DELAY, 50, {}},

	{0x2C, 1, {0x00}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 1, {0x00}},
        {REGFLAG_DELAY, 50, {}},

    // Sleep Mode On
	{0x10, 1, {0x00}},
        {REGFLAG_DELAY, 120, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {

        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {

            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;

            case REGFLAG_END_OF_TABLE :
                break;

            default:
		dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
       	}
    }

}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
		memset(params, 0, sizeof(LCM_PARAMS));
	
		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;

		// enable tearing-free
		params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
		params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

#if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
#else
		params->dsi.mode   = SYNC_PULSE_VDO_MODE;
#endif

		// DSI
		/* Command mode setting */
		params->dsi.LANE_NUM				= LCM_ONE_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB666;

		// Highly depends on LCD driver capability.
		params->dsi.packet_size=256;

		// Video mode setting		
		params->dsi.PS=LCM_PACKED_PS_18BIT_RGB666;

		params->dsi.word_count=FRAME_WIDTH*3;	
		params->dsi.vertical_sync_active=2;
		params->dsi.vertical_backporch=2;
		params->dsi.vertical_frontporch=2;
		params->dsi.vertical_active_line=FRAME_HEIGHT;
	
		params->dsi.line_byte=2180;		// 2256 = 752*3
		params->dsi.horizontal_sync_active_byte=26;
		params->dsi.horizontal_backporch_byte=206;
		params->dsi.horizontal_frontporch_byte=206;	
		params->dsi.rgb_byte=(FRAME_WIDTH*3+6);		// NC
	
		params->dsi.horizontal_sync_active_word_count=20;	
		params->dsi.horizontal_backporch_word_count=200;
		params->dsi.horizontal_frontporch_word_count=200;
                // added by zhuqiang for one lane speed maximum is 270MHz start.20121024
		// Bit rate calculation
		params->dsi.pll_div1=22;		// fref=26MHz, fvco=fref*(div1+1)	(div1=0~63, fvco=500MHZ~1GHz)
		params->dsi.pll_div2=1;			// div2=0~15: fout=fvo/(2*div2) 300MHZ
                // added by zhuqiang for one lane speed maximum is 270MHz end.20121024
            
            
         //Add by zhuqiang for PR478601 start.2013.6.26  
                                    params->dsi.HS_TRAIL	= 10;//10;//min max(n*8*UI, 60ns+n*4UI)
		params->dsi.HS_ZERO 	= 8;//8;//min 105ns+6*UI
		params->dsi.HS_PRPR 	= 4;//min 40ns+4*UI; max 85ns+6UI
		params->dsi.LPX 		= 12;//min 50ns
		
		params->dsi.TA_SACK     = 1;
		params->dsi.TA_GET      = 60;
		params->dsi.TA_SURE     = 18;
		params->dsi.TA_GO          = 12;//12;//4*LPX
	
		params->dsi.CLK_TRAIL	= 10;//5;//min 60ns
		params->dsi.CLK_ZERO	= 18;//18;//min 300ns-38ns
		params->dsi.LPX_WAIT	= 10;
		params->dsi.CONT_DET	= 0;
		params->dsi.CLK_HS_PRPR = 4;//min 38ns; max 95ns
		//Add by zhuqiang for PR478601 end.2013.6.26
}


static void lcm_init(void)
{
    unsigned int data_array[16];

    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(20);
    SET_RESET_PIN(1);
    MDELAY(120);

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);

}


static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_resume(void)
{
        //Remove by ygm due to white display problem. 20121010.     
	//lcm_init();

	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	data_array[3]= 0x00053902;
	data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[5]= (y1_LSB);
	data_array[6]= 0x002c3909;

	dsi_set_cmdq(data_array, 7, 0);

}

// added by zhuqiang for lcd esd begin 2012.11.19

static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_UBOOT

       unsigned char buffer_vcom[4];

       unsigned char buffer_0a[1];

       unsigned char buffer_0b[5];

       unsigned int array[16];


       array[0]=0x00341500;

       dsi_set_cmdq(&array, 1, 1);

       array[0] = 0x00013700;

       dsi_set_cmdq(array, 1, 1);

      read_reg_v2(0x0A,buffer_0a, 1);

        array[0] = 0x00053700;

       dsi_set_cmdq(array, 1, 1);

      read_reg_v2(0x09,buffer_0b, 5);

      array[0] = 0x00043700;

      dsi_set_cmdq(array, 1, 1);

      read_reg_v2(0xC5, buffer_vcom, 4);

      array[0]=0x00351500;

      dsi_set_cmdq(&array, 1, 1);
     //printk("lcm 0x0a is %x--------------\n", buffer_0a[0]);
     //printk("lcm 0xc5 is %x,%x,%x,%x--------------\n", buffer_vcom[0], buffer_vcom[1] ,buffer_vcom[2], buffer_vcom[3]);
     if ((buffer_vcom[0]==0x00)&&(buffer_vcom[1]==0x18)&&(buffer_vcom[3]==0x18)&&(buffer_0a[0]==0x9C)&&(buffer_0b[1]==0x63)){

               return 0;


      }else{

              return 1;
      }
#endif
}


static unsigned int lcm_esd_recover(void)
{

   #ifndef BUILD_UBOOT


       lcm_init();

       return 1;

      #endif 
}

 // added by zhuqiang for lcd esd end 2012.11.19

static unsigned int lcm_compare_id(void)
{

       unsigned char buffer_vcom[4];

       unsigned int data_array[16];

       unsigned int id=0;
       unsigned int id1=0;
  //     array[0]=0x00341500;

   //    dsi_set_cmdq(&array, 1, 1);

       SET_RESET_PIN(1);
       MDELAY(10);
      SET_RESET_PIN(0);
       MDELAY(20);
      SET_RESET_PIN(1);
      MDELAY(120);
   
   
   	data_array[0] = 0x00110500;		// Sleep Out
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
	
	//data_array[0]=0x00033902;
	//data_array[1]=0x00FE01ED;
	//dsi_set_cmdq(data_array, 2, 1);	
	//MDELAY(5); 
	data_array[0] = 0x00033700;// read id return 3 bytes,version and id
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(5); 
                  
                  read_reg_v2(0xD4, buffer_vcom, 3);
                  MDELAY(5); 
              //    id = buffer_vcom[0];//01
                  id1 = buffer_vcom[1];//53
              //    id2 = buffer_vcom[2];//10
  //    array[0]=0x00351500;

  //    dsi_set_cmdq(&array, 1, 1);


      mt_set_gpio_mode(47,0);  // gpio mode   high
	mt_set_gpio_pull_enable(47,0);
	mt_set_gpio_dir(47,0);  //input
   	//mt_set_gpio_pull_select(50, 1);
        MDELAY(5);
	id = mt_get_gpio_in(47) ;
 /*
       #ifdef BUILD_UBOOT
      printf("lcm  txd txd txd txd txd txd txd ili9488 driver-----pin number id is %x---------\n",id);
#else
      printk("lcm  txd txd txd txd txd txd txd ili9488 driver-----pin number id is %x---------\n",id);
#endif
*/


      if ((id==1)&&(id1==0x53)){

               return 1;

     }else{

              return 0;
   }

    

}
// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER nt35510_dsi_6575_lcm_drv =
{
    .name			= "nt35510_dsi_6575",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
#if (LCM_DSI_CMD_MODE)
	.update         = lcm_update,
//	.set_backlight	= lcm_setbacklight,
//	.set_pwm        = lcm_setpwm,
//	.get_pwm        = lcm_getpwm,
   
      // added by zhuqiang for lcd esd begin 2012.11.19
//	.esd_check   = lcm_esd_check,
  // 	.esd_recover   = lcm_esd_recover,
      // added by zhuqiang for lcd esd end 2012.11.19
	.compare_id    = lcm_compare_id,
#endif
};
