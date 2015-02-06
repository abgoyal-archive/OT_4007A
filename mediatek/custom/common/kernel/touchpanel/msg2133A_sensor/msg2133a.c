

#include "tpd.h"
#include <linux/interrupt.h>
#include <cust_eint.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <mach/mt6575_boot.h>
#include <linux/miscdevice.h>

#include <mach/mt6575_pm_ldo.h>


#include "cust_gpio_usage.h"
#include"tpd_custom_mcs6024.h"
#define GPIO_CTP_EN_PIN   GPIO52
#define  GPIO_CTP_EN_PIN_M_GPIO  0 


#define TPD_HAVE_BUTTON

unsigned char MSG_FIRMWARE_MT[94*1024] =
{
  #include "Pixo_MT_V105_130813.h"
};
unsigned char MSG_FIRMWARE_EA[94*1024] =
{
  #include "Pixo_EA_V504_130806.h"
};

volatile static u8 Fmr_Loader[1024];
 
extern struct tpd_device *tpd;
 
struct i2c_client *i2c_clientma = NULL;
struct task_struct *mthread = NULL;
 
static DECLARE_WAIT_QUEUE_HEAD(waiter);
 
 
static void tpd_eint_interrupt_handler(void);
 
 
 extern void mt65xx_eint_unmask(unsigned int line);
 extern void mt65xx_eint_mask(unsigned int line);
 extern void mt65xx_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
 extern kal_uint32 mt65xx_eint_set_sens(kal_uint8 eintno, kal_bool sens);
 extern void mt65xx_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
									  kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
									  kal_bool auto_umask);
 extern int Log_record_msg(unsigned int module_id,unsigned int exception_id,char *buf);

 
static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int __devexit tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);
 
//---------------------------------------------------------------------------------------------
static int tpd_flag = 0;

#define u8         unsigned char
#define u32        unsigned int
#define s32        signed int

#define MAX_TOUCH_FINGER 2
typedef struct
{
    u16 X;
    u16 Y;
} TouchPoint_t;

typedef struct
{
    u8 nTouchKeyMode;
    u8 nTouchKeyCode;
    u8 nFingerNum;
    TouchPoint_t Point[MAX_TOUCH_FINGER];
} TouchScreenInfo_t;


#define REPORT_PACKET_LENGTH    (8)
#define MSG21XX_INT_GPIO       (42)
#define MSG21XX_RESET_GPIO     (22)
#define MS_TS_MSG21XX_X_MAX   (320)
#define MS_TS_MSG21XX_Y_MAX   (480)


#define TPD_OK 0
#define TPD_DEVICEMA       "MSG2133A"
#if defined(CTP_CHECK_FM)
static int16_t fm_current_frequency=0;
#endif


static const struct i2c_device_id tpd_idm[] = {{TPD_DEVICEMA,0},{}};
unsigned short forcem[] = {0,0x4c,I2C_CLIENT_END,I2C_CLIENT_END}; 
static const unsigned short * const forcesm[] = { forcem, NULL };
static struct i2c_client_address_data addr_datam = { .forces = forcesm, };
 
 
static struct i2c_driver tpd_i2c_driver =
{
  .driver = {
	 .name = TPD_DEVICEMA,
	 .owner = THIS_MODULE,
  },
  .probe = tpd_probe,
  .remove = __devexit_p(tpd_remove),
  .id_table = tpd_idm,
  .detect = tpd_detect,
  .address_data = &addr_datam,
};
 
#define	__FIRMWARE_UPDATE__
#ifdef __FIRMWARE_UPDATE__

#define FW_ADDR_MSG21XX   (0xC4)
#define FW_ADDR_MSG21XX_TP   (0x4C)
#define FW_UPDATE_ADDR_MSG21XX   (0x92)
#define TP_DEBUG	printk
static  char *fw_version;
static u8 temp[94][1024];
static int FwDataCnt;
static char *TPMSG="EACH";//default TP is Each
unsigned short fw_major_version=0,fw_minor_version=0;

#if 0
static void HalTscrCReadI2CSeq(u8 addr, u8* read_data, u8 size)
{
   //according to your platform.
   	int rc;

	struct i2c_msg msgs[] =
    {
		{
			.addr = addr,
			.flags = I2C_M_RD,
			.len = size,
			.buf = read_data,
		},
	};

	rc = i2c_transfer(i2c_clientma->adapter, msgs, 1);
	if( rc < 0 )
    {
		printk("HalTscrCReadI2CSeq error %d\n", rc);
	}
}

static void HalTscrCDevWriteI2CSeq(u8 addr, u8* data, u16 size)
{
    //according to your platform.
   	int rc;

	struct i2c_msg msgs[] =
    {
		{
			.addr = addr,
			.flags = 0,
			.len = size,
			.buf = data,
		},
	};
	rc = i2c_transfer(i2c_clientma->adapter, msgs, 1);
	if( rc < 0 )
    {
		printk("HalTscrCDevWriteI2CSeq error %d,addr = %d\n", rc,addr);
	}
}

#endif

static void HalTscrCReadI2CSeq(u8 addr, u8* read_data, u8 size)
{
	//mutex_lock(&tp_mutex); 
	i2c_clientma->addr = addr;	
	i2c_master_recv(i2c_clientma, &read_data[0], size);
	//mutex_unlock(&tp_mutex);
}

static void HalTscrCDevWriteI2CSeq(u8 addr, u8* data, u16 size)
{
	int ret;
	//mutex_lock(&tp_mutex);
	i2c_clientma->addr = addr;
	i2c_clientma->addr = i2c_clientma->addr & I2C_MASK_FLAG;
	ret = i2c_master_send(i2c_clientma, &data[0], size);
	if(ret <=  0) {
	printk("addr: %d i2c write interface error!\n",addr);	
	}    
	//mutex_unlock(&tp_mutex);
}
static void dbbusDWIICEnterSerialDebugMode(void)
{
    u8 data[5];

    // Enter the Serial Debug Mode
    data[0] = 0x53;
    data[1] = 0x45;
    data[2] = 0x52;
    data[3] = 0x44;
    data[4] = 0x42;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 5);
}

static void dbbusDWIICStopMCU(void)
{
    u8 data[1];

    // Stop the MCU
    data[0] = 0x37;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICIICUseBus(void)
{
    u8 data[1];

    // IIC Use Bus
    data[0] = 0x35;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICIICReshape(void)
{
    u8 data[1];

    // IIC Re-shape
    data[0] = 0x71;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICIICNotUseBus(void)
{
    u8 data[1];

    // IIC Not Use Bus
    data[0] = 0x34;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICNotStopMCU(void)
{
    u8 data[1];

    // Not Stop the MCU
    data[0] = 0x36;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICExitSerialDebugMode(void)
{
    u8 data[1];

    // Exit the Serial Debug Mode
    data[0] = 0x45;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);

    // Delay some interval to guard the next transaction
    udelay ( 200 );        // delay about 0.2ms
}


int crc_tab[256];
static u8 g_dwiic_info_data[1024];
static u8 Auto_APK_flag=0;//if apk upgrade =1;

/*reset the chip*/
static void _HalTscrHWReset(void)
{
	mt_set_gpio_mode(GPIO_CTP_EN_PIN, GPIO_CTP_EN_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_EN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ONE);
	mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ZERO);
	mdelay(10);
	mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ONE);
	mdelay(50);
}
/*add by liukai for release the touch action*/
static void msg21xx_release(void)
{
	printk("[%s]: Enter!\n", __func__);
	input_report_key(tpd->dev,KEY_BACK , 0);
	input_report_key(tpd->dev, KEY_HOME, 0);
	input_report_key(tpd->dev, KEY_MENU, 0);
	input_report_key(tpd->dev, KEY_SEARCH, 0);
	input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 0);
	input_mt_sync(tpd->dev);
	input_sync(tpd->dev);
}

/* update the firmware part, used by apk*/
/*show the fw version*/

static u32 Reflect ( u32 ref, char ch ) //unsigned int Reflect(unsigned int ref, char ch)
{
    u32 value = 0;
    u32 i = 0;

    for ( i = 1; i < ( ch + 1 ); i++ )
    {
        if ( ref & 1 )
        {
            value |= 1 << ( ch - i );
        }
        ref >>= 1;
    }
    return value;
}

u32 Get_CRC ( u32 text, u32 prevCRC, u32 *crc32_table )
{
    u32  ulCRC = prevCRC;
	ulCRC = ( ulCRC >> 8 ) ^ crc32_table[ ( ulCRC & 0xFF ) ^ text];
    return ulCRC ;
}

static void Init_CRC32_Table ( u32 *crc32_table )
{
    u32 magicnumber = 0x04c11db7;
    u32 i = 0, j;

    for ( i = 0; i <= 0xFF; i++ )
    {
        crc32_table[i] = Reflect ( i, 8 ) << 24;
        for ( j = 0; j < 8; j++ )
        {
            crc32_table[i] = ( crc32_table[i] << 1 ) ^ ( crc32_table[i] & ( 0x80000000L ) ? magicnumber : 0 );
        }
        crc32_table[i] = Reflect ( crc32_table[i], 32 );
    }
}

static void Init_CRC32_Table_A ( u32 *crc32_table )
{
    u32 magicnumber = 0x04c11db7;
    u32 i = 0, j;

    for ( i = 0; i <= 0xFF; i++ )
    {
        crc32_table[i] = Reflect ( i, 8 ) << 24;
        for ( j = 0; j < 8; j++ )
        {
            crc32_table[i] = ( crc32_table[i] << 1 ) ^ ( crc32_table[i] & ( 0x80000000L ) ? magicnumber : 0 );
        }
        crc32_table[i] = Reflect ( crc32_table[i], 32 );
    }
}

typedef enum
{
	EMEM_ALL = 0,
	EMEM_MAIN,
	EMEM_INFO,
} EMEM_TYPE_t;

static void drvDB_WriteReg8Bit ( u8 bank, u8 addr, u8 data )
{
    u8 tx_data[4] = {0x10, bank, addr, data};
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, tx_data, 4 );
}

static void drvDB_WriteReg ( u8 bank, u8 addr, u16 data )
{
    u8 tx_data[5] = {0x10, bank, addr, data & 0xFF, data >> 8};
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, tx_data, 5 );
}

static unsigned short drvDB_ReadReg ( u8 bank, u8 addr )
{
    u8 tx_data[3] = {0x10, bank, addr};
    u8 rx_data[2] = {0};

    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &rx_data[0], 2 );
    return ( rx_data[1] << 8 | rx_data[0] );
}


static int drvTP_erase_emem_c33 ( EMEM_TYPE_t emem_type )
{
    // stop mcu
    drvDB_WriteReg ( 0x0F, 0xE6, 0x0001 );

    //disable watch dog
    drvDB_WriteReg8Bit ( 0x3C, 0x60, 0x55 );
    drvDB_WriteReg8Bit ( 0x3C, 0x61, 0xAA );

    // set PROGRAM password
    drvDB_WriteReg8Bit ( 0x16, 0x1A, 0xBA );
    drvDB_WriteReg8Bit ( 0x16, 0x1B, 0xAB );

    //proto.MstarWriteReg(F1.loopDevice, 0x1618, 0x80);
    drvDB_WriteReg8Bit ( 0x16, 0x18, 0x80 );

    if ( emem_type == EMEM_ALL )
    {
        drvDB_WriteReg8Bit ( 0x16, 0x08, 0x10 ); //mark
    }

    drvDB_WriteReg8Bit ( 0x16, 0x18, 0x40 );
    mdelay ( 10 );

    drvDB_WriteReg8Bit ( 0x16, 0x18, 0x80 );

    // erase trigger
    if ( emem_type == EMEM_MAIN )
    {
        drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x04 ); //erase main
    }
    else
    {
        drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x08 ); //erase all block
    }

    return ( 1 );
}

static int drvTP_read_emem_dbbus_c33 ( EMEM_TYPE_t emem_type, u16 addr, size_t size, u8 *p, size_t set_pce_high )
{
    u32 i;

    // Set the starting address ( must before enabling burst mode and enter riu mode )
    drvDB_WriteReg ( 0x16, 0x00, addr );

    // Enable the burst mode ( must before enter riu mode )
    drvDB_WriteReg ( 0x16, 0x0C, drvDB_ReadReg ( 0x16, 0x0C ) | 0x0001 );

    // Set the RIU password
    drvDB_WriteReg ( 0x16, 0x1A, 0xABBA );

    // Enable the information block if pifren is HIGH
    if ( emem_type == EMEM_INFO )
    {
        // Clear the PCE
        drvDB_WriteReg ( 0x16, 0x18, drvDB_ReadReg ( 0x16, 0x18 ) | 0x0080 );
        mdelay ( 10 );

        // Set the PIFREN to be HIGH
        drvDB_WriteReg ( 0x16, 0x08, 0x0010 );
    }

    // Set the PCE to be HIGH
    drvDB_WriteReg ( 0x16, 0x18, drvDB_ReadReg ( 0x16, 0x18 ) | 0x0040 );
    mdelay ( 10 );

    // Wait pce becomes 1 ( read data ready )
    while ( ( drvDB_ReadReg ( 0x16, 0x10 ) & 0x0004 ) != 0x0004 );

    for ( i = 0; i < size; i += 4 )
    {
        // Fire the FASTREAD command
        drvDB_WriteReg ( 0x16, 0x0E, drvDB_ReadReg ( 0x16, 0x0E ) | 0x0001 );

        // Wait the operation is done
        while ( ( drvDB_ReadReg ( 0x16, 0x10 ) & 0x0001 ) != 0x0001 );

        p[i + 0] = drvDB_ReadReg ( 0x16, 0x04 ) & 0xFF;
        p[i + 1] = ( drvDB_ReadReg ( 0x16, 0x04 ) >> 8 ) & 0xFF;
        p[i + 2] = drvDB_ReadReg ( 0x16, 0x06 ) & 0xFF;
        p[i + 3] = ( drvDB_ReadReg ( 0x16, 0x06 ) >> 8 ) & 0xFF;
    }

    // Disable the burst mode
    drvDB_WriteReg ( 0x16, 0x0C, drvDB_ReadReg ( 0x16, 0x0C ) & ( ~0x0001 ) );

    // Clear the starting address
    drvDB_WriteReg ( 0x16, 0x00, 0x0000 );

    //Always return to main block
    if ( emem_type == EMEM_INFO )
    {
        // Clear the PCE before change block
        drvDB_WriteReg ( 0x16, 0x18, drvDB_ReadReg ( 0x16, 0x18 ) | 0x0080 );
        mdelay ( 10 );
        // Set the PIFREN to be LOW
        drvDB_WriteReg ( 0x16, 0x08, drvDB_ReadReg ( 0x16, 0x08 ) & ( ~0x0010 ) );

        drvDB_WriteReg ( 0x16, 0x18, drvDB_ReadReg ( 0x16, 0x18 ) | 0x0040 );
        while ( ( drvDB_ReadReg ( 0x16, 0x10 ) & 0x0004 ) != 0x0004 );
    }

    // Clear the RIU password
    drvDB_WriteReg ( 0x16, 0x1A, 0x0000 );

    if ( set_pce_high )
    {
        // Set the PCE to be HIGH before jumping back to e-flash codes
        drvDB_WriteReg ( 0x16, 0x18, drvDB_ReadReg ( 0x16, 0x18 ) | 0x0040 );
        while ( ( drvDB_ReadReg ( 0x16, 0x10 ) & 0x0004 ) != 0x0004 );
    }

    return ( 1 );
}


static int drvTP_read_info_dwiic_c33 ( void )
{
    u8  dwiic_tx_data[5];
    u8  index=0;
    u16 reg_data=0;
    //unsigned char dbbus_rx_data[2] = {0};

    mdelay ( 300 );


    // Stop Watchdog
    drvDB_WriteReg8Bit ( 0x3C, 0x60, 0x55 );
    drvDB_WriteReg8Bit ( 0x3C, 0x61, 0xAA );

    drvDB_WriteReg ( 0x3C, 0xE4, 0xA4AB );

	drvDB_WriteReg ( 0x1E, 0x04, 0x7d60 );

    // TP SW reset
    drvDB_WriteReg ( 0x1E, 0x04, 0x829F );
	mdelay (1);

    dwiic_tx_data[0] = 0x10;
    dwiic_tx_data[1] = 0x0F;
    dwiic_tx_data[2] = 0xE6;
    dwiic_tx_data[3] = 0x00;
    mdelay(15);
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dwiic_tx_data, 4 );

        // stop mcu
  //  drvDB_WriteReg ( 0x1E, 0xE6, 0x0001 );

    mdelay ( 100 );
	TP_DEBUG ( "read infor 1\n");

    do{
        reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
    }
    while ( reg_data != 0x5B58 );
	TP_DEBUG ( "read infor +++2\n");
    dwiic_tx_data[0] = 0x72;

   // dwiic_tx_data[3] = 0x04;
  //  dwiic_tx_data[4] = 0x00;
    dwiic_tx_data[3] = 0x00;
    dwiic_tx_data[4] = 0x80;

    for(reg_data=0;reg_data<1;reg_data++)
    {
    	dwiic_tx_data[1] = 0x80+(((reg_data*128)&0xff00)>>8);
    	dwiic_tx_data[2] = (reg_data*128)&0x00ff;
    	HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX_TP , dwiic_tx_data, 5 );

    	mdelay (50 );

    // recive info data
    	HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX_TP, &g_dwiic_info_data[reg_data*128], 128);
    }

    fw_major_version = (g_dwiic_info_data[15]<<8)+g_dwiic_info_data[14];
    fw_minor_version = (g_dwiic_info_data[17]<<8)+g_dwiic_info_data[16];

    TP_DEBUG("***major = %d ***\n", fw_major_version);
    TP_DEBUG("***minor = %d ***\n", fw_minor_version);
    for(index=0;index<16;index++){
		printk("g_dwiic_info_data [%d] = %c \n",index,g_dwiic_info_data[index]);
    }
    return ( 1 );
}

static int drvTP_info_updata_C33 ( u16 start_index, u8 *data, u16 size )
{
    // size != 0, start_index+size !> 1024
    u16 i;
    for ( i = 0; i < size; i++ )
    {
        g_dwiic_info_data[start_index] = * ( data + i );
        start_index++;
    }
    return ( 1 );
}

static ssize_t firmware_update_c33 ( size_t size, EMEM_TYPE_t emem_type )
{
    u8  dbbus_tx_data[4];
    u8  dbbus_rx_data[2] = {0};
    u8  life_counter[2];
    u32 i, j;
    u32 crc_main, crc_main_tp;
    u32 crc_info, crc_info_tp;

    int update_pass = 1;
    u16 reg_data = 0;
    int count=0;

    crc_main = 0xffffffff;
    crc_info = 0xffffffff;

    printk ( "[MSG2133A]++firmware_update_c33++\n" );

/*-----------------------------------//Auto upgrade to get date-------------------------------------------*/
	if(Auto_APK_flag==0){ 
		if(TPMSG=="MUTTON")//Mutton
		{
			for (i = 0; i < 33; i++)   // total  33 KB : 1 byte per R/W
			{
				for ( j = 0; j < 1024; j++ )        //Read 1k bytes
				{
					temp[i][j] = MSG_FIRMWARE_MT[(i*1024)+j]; // Read the bin files of slave firmware from the baseband file system
				}
			}
		}
		else if(TPMSG=="EACH")//Each
		{
			for (i = 0; i < 33; i++)   // total  33 KB : 1 byte per R/W
			{
				for ( j = 0; j < 1024; j++ )        //Read 1k bytes
				{
					temp[i][j] = MSG_FIRMWARE_EA[(i*1024)+j]; // Read the bin files of slave firmware from the baseband file system
				}
			}
		}
		printk("[MSG2133A] Date transf completed !\n");
	}
/*-----------------------------------//Auto upgrade to get date-------------------------------------------*/ 
#if (0)
    drvTP_read_info_dwiic_c33();//check

    //if ( g_dwiic_info_data[0] == 'M' && g_dwiic_info_data[1] == 'S' && g_dwiic_info_data[2] == 'T' && g_dwiic_info_data[3] == 'A' && g_dwiic_info_data[4] == 'R' && g_dwiic_info_data[5] == 'T' && g_dwiic_info_data[6] == 'P' && g_dwiic_info_data[7] == 'C' )
    if(1)
    {
		// updata FW Version
		//drvTP_info_updata_C33 ( 8, &temp[32][8], 5 );

		g_dwiic_info_data[8]=temp[32][8];
		g_dwiic_info_data[9]=temp[32][9];
		g_dwiic_info_data[10]=temp[32][10];
		g_dwiic_info_data[11]=temp[32][11];
		// updata life counter
		life_counter[1] = (( ( (g_dwiic_info_data[13] << 8 ) | g_dwiic_info_data[12]) + 1 ) >> 8 ) & 0xFF;
		life_counter[0] = ( ( (g_dwiic_info_data[13] << 8 ) | g_dwiic_info_data[12]) + 1 ) & 0xFF;
		g_dwiic_info_data[12]=life_counter[0];
		g_dwiic_info_data[13]=life_counter[1];
		//drvTP_info_updata_C33 ( 10, &life_counter[0], 3 );
		drvDB_WriteReg ( 0x3C, 0xE4, 0x78C5 );
		drvDB_WriteReg ( 0x1E, 0x04, 0x7d60 );
		// TP SW reset
		drvDB_WriteReg ( 0x1E, 0x04, 0x829F );

		mdelay ( 50 );

		//polling 0x3CE4 is 0x2F43
		do
		{
		    reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );

		}
		while ( reg_data != 0x2F43 );

		// transmit lk info data
		for(count=0;count<8;count++)
		{
			HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX_TP , &g_dwiic_info_data[count*128], 128 );
		}
		//polling 0x3CE4 is 0xD0BC
		do
		{
		    reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
		}
		while ( reg_data != 0xD0BC );

    	}
	else
	{
		printk ( "[MSG2133A] update math faild!\n" );
		_HalTscrHWReset();
		FwDataCnt = 0;
		//enable_irq(msg21xx_irq);
		mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
		return size;
	}
#endif
    //erase main
    drvTP_erase_emem_c33 ( EMEM_MAIN );
    mdelay ( 1000 );

    printk("[MSG2133A]  Earse flash completed! \n");

    //ResetSlave();
    _HalTscrHWReset();

    //drvDB_EnterDBBUS();
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();
    mdelay ( 100 );

    /////////////////////////
    // Program
    /////////////////////////

    //polling 0x3CE4 is 0x1C70
    if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
    {
        do
        {
            reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
        }
        while ( reg_data != 0x1C70 );
    }

	printk("[MSG2133] Polling OK \n");

    switch ( emem_type )
    {
        case EMEM_ALL:
            drvDB_WriteReg ( 0x3C, 0xE4, 0xE38F );  // for all-blocks
            break;
        case EMEM_MAIN:
            drvDB_WriteReg ( 0x3C, 0xE4, 0x7731 );  // for main block
            break;
        case EMEM_INFO:
            drvDB_WriteReg ( 0x3C, 0xE4, 0x7731 );  // for info block

            drvDB_WriteReg8Bit ( 0x0F, 0xE6, 0x01 );

            drvDB_WriteReg8Bit ( 0x3C, 0xE4, 0xC5 ); //
            drvDB_WriteReg8Bit ( 0x3C, 0xE5, 0x78 ); //

            drvDB_WriteReg8Bit ( 0x1E, 0x04, 0x9F );
            drvDB_WriteReg8Bit ( 0x1E, 0x05, 0x82 );

            drvDB_WriteReg8Bit ( 0x0F, 0xE6, 0x00 );
            mdelay ( 100 );
            break;
    }


    printk("[MSG2133] Emem_type init OK\n");
    mdelay ( 100 );
    // polling 0x3CE4 is 0x2F43
    do
    {
        reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
    }
    while ( reg_data != 0x2F43 );

     printk("[MSG2133] Polling again OK\n");

    // calculate CRC 32
    Init_CRC32_Table_A ( &crc_tab[0] );

	for ( i = 0; i < 32; i++ ) // total  33 KB, last 1KB don't write : 2 byte per R/W
	{
		if ( emem_type == EMEM_INFO )
			i = 32;

		if ( i < 32 )   //emem_main
		{
			if ( i == 31 )
			{
				temp[i][1014] = 0x5A; //Fmr_Loader[1014]=0x5A;
				temp[i][1015] = 0xA5; //Fmr_Loader[1015]=0xA5;

				for ( j = 0; j < 1016; j++ )
				{
					//crc_main=Get_CRC(Fmr_Loader[j],crc_main,&crc_tab[0]);
					crc_main = Get_CRC ( temp[i][j], crc_main, &crc_tab[0] );
				}
			}	
			else
			{
				for ( j = 0; j < 1024; j++ )
				{
					//crc_main=Get_CRC(Fmr_Loader[j],crc_main,&crc_tab[0]);
					crc_main = Get_CRC ( temp[i][j], crc_main, &crc_tab[0] );
				}
			}
		}
		else  //emem_info
		{
			for ( j = 0; j < 1024; j++ )
			{
				//crc_info=Get_CRC(Fmr_Loader[j],crc_info,&crc_tab[0]);
				crc_info = Get_CRC ( g_dwiic_info_data[j], crc_info, &crc_tab[0] );
			}
			if ( emem_type == EMEM_MAIN ) 
				break;
		}

		//drvDWIIC_MasterTransmit( DWIIC_MODE_DWIIC_ID, 1024, Fmr_Loader );
		for( j = 0; j < 8; j++)
		{
			HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX_TP, &temp[i][j*128], 128 );
       		}
		printk("[MSG2133A]Wrete Flash the %dKB OK \n",i+1);
		mdelay ( 100 );

		// polling 0x3CE4 is 0xD0BC
		do
		{
			reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
		}
		while ( reg_data != 0xD0BC );
		mdelay ( 100 );

		drvDB_WriteReg ( 0x3C, 0xE4, 0x2F43 );
	}

	printk("[MSG2133A] Write Flash Completed \n");

	if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
	{
		// write file done and check crc
		drvDB_WriteReg ( 0x3C, 0xE4, 0x1380 );
	}
	mdelay ( 100 ); //MCR_CLBK_DEBUG_DELAY ( 10, MCU_LOOP_DELAY_COUNT_MS );

	if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
	{
	// polling 0x3CE4 is 0x9432
		do
		{
			reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
		}while ( reg_data != 0x9432 );
	}

	crc_main = crc_main ^ 0xffffffff;
	crc_info = crc_info ^ 0xffffffff;

	if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
	{
		// CRC Main from TP
		crc_main_tp = drvDB_ReadReg ( 0x3C, 0x80 );
		crc_main_tp = ( crc_main_tp << 16 ) | drvDB_ReadReg ( 0x3C, 0x82 );

		// CRC Info from TP
		crc_info_tp = drvDB_ReadReg ( 0x3C, 0xA0 );
		crc_info_tp = ( crc_info_tp << 16 ) | drvDB_ReadReg ( 0x3C, 0xA2 );
	}
	printk ( "crc_main=0x%x, crc_info=0x%x, crc_main_tp=0x%x, crc_info_tp=0x%x\n",crc_main, crc_info, crc_main_tp, crc_info_tp );

	//drvDB_ExitDBBUS();

	update_pass = 1;
	if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
	{
		if ( crc_main_tp != crc_main )
			update_pass = 0;

		if ( crc_info_tp != crc_info )
			update_pass = 0;
	}

#if 0
	if ( !update_pass )
	{
	printk ( "update FAILED\n" );
	_HalTscrHWReset();
	FwDataCnt = 0;
	//enable_irq(msg21xx_irq);

	mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	return ( 0 );
	}
#endif

	printk ( "[MSG2133A] update OK\n" );
	_HalTscrHWReset();
	FwDataCnt = 0;
	//enable_irq(msg21xx_irq);
	mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	return size;
}

static void firmware_version()
{
    unsigned char dbbus_tx_data[3];
    unsigned char dbbus_rx_data[4] ;

    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();


    fw_version = kzalloc(sizeof(char), GFP_KERNEL);

    //Get_Chip_Version();
    dbbus_tx_data[0] = 0x53;
    dbbus_tx_data[1] = 0x00;
    dbbus_tx_data[2] = 0x2A;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_tx_data[0], 3);
    HalTscrCReadI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_rx_data[0], 4);

    fw_major_version = (dbbus_rx_data[1]<<8)+dbbus_rx_data[0];
    fw_minor_version = (dbbus_rx_data[3]<<8)+dbbus_rx_data[2];

    TP_DEBUG("***major = %d ***\n", fw_major_version);
    TP_DEBUG("***minor = %d ***\n", fw_minor_version);
    sprintf(fw_version,"%03d%03d", fw_major_version, fw_minor_version);
    //TP_DEBUG(printk("***fw_version = %s ***\n", fw_version);)
}

static int firmware_updata()
{
	u8 dbbus_tx_data[4];
	unsigned char dbbus_rx_data[2] = {0};
	mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);

	_HalTscrHWReset();
	i2c_clientma->timing = 50;

	// Erase TP Flash first
	dbbusDWIICEnterSerialDebugMode();
	dbbusDWIICStopMCU();
	dbbusDWIICIICUseBus();
	dbbusDWIICIICReshape();
	mdelay ( 300 );

	// Disable the Watchdog
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x3C;
	dbbus_tx_data[2] = 0x60;
	dbbus_tx_data[3] = 0x55;
	HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x3C;
	dbbus_tx_data[2] = 0x61;
	dbbus_tx_data[3] = 0xAA;
	HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
	// Stop MCU
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x0F;
	dbbus_tx_data[2] = 0xE6;
	dbbus_tx_data[3] = 0x01;
	HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
	/////////////////////////
	// Difference between C2 and C3
	/////////////////////////
	// c2:2133 c32:2133a(2) c33:2133a
	//check id
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0xCC;
	HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
	HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
	if ( dbbus_rx_data[0] == 2 )
	{
	    // check version
	    dbbus_tx_data[0] = 0x10;
	    dbbus_tx_data[1] = 0x3C;
	    dbbus_tx_data[2] = 0xEA;
	    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
	    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
	    TP_DEBUG ( "dbbus_rx version[0]=0x%x", dbbus_rx_data[0] );

	    if ( dbbus_rx_data[0] == 3 ){
			firmware_update_c33 ( 1, EMEM_MAIN );
		}
	}
	 i2c_clientma->timing = 100;
	 i2c_clientma->addr = FW_ADDR_MSG21XX_TP;
	return 1;
}

//#define APK_UPGRADE

#ifdef APK_UPGRADE

struct class *firmware_class;
struct device *firmware_cmd_dev;
static ssize_t firmware_update_store ( struct device *dev,
                                       struct device_attribute *attr, const char *buf, size_t size )
{
    u8 i;
    u8 dbbus_tx_data[4];
    unsigned char dbbus_rx_data[2] = {0};
	
    mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);

    _HalTscrHWReset();
    i2c_clientma->timing = 50;

    // Erase TP Flash first
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();
    mdelay ( 300 );

    // Disable the Watchdog
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x60;
    dbbus_tx_data[3] = 0x55;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x61;
    dbbus_tx_data[3] = 0xAA;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    // Stop MCU
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x0F;
    dbbus_tx_data[2] = 0xE6;
    dbbus_tx_data[3] = 0x01;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    /////////////////////////
    // Difference between C2 and C3
    /////////////////////////
    // c2:2133 c32:2133a(2) c33:2133a
    //check id
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0xCC;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    if ( dbbus_rx_data[0] == 2 )
    {
        // check version
        dbbus_tx_data[0] = 0x10;
        dbbus_tx_data[1] = 0x3C;
        dbbus_tx_data[2] = 0xEA;
        HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
        HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
        TP_DEBUG ( "dbbus_rx version[0]=0x%x\n", dbbus_rx_data[0] );

        if ( dbbus_rx_data[0] == 3 ){
		firmware_update_c33 ( size, EMEM_MAIN );
	}
    }
     i2c_clientma->timing = 100;
    i2c_clientma->addr = FW_ADDR_MSG21XX_TP;
    return size;
}

static ssize_t firmware_update_show ( struct device *dev,
                                      struct device_attribute *attr, char *buf )
{
    return sprintf ( buf, "%s\n", fw_version );
}

static DEVICE_ATTR(update, 0777, firmware_update_show, firmware_update_store);
/*test=================*/
/*Add by Tracy.Lin for update touch panel firmware and get fw version*/

static ssize_t firmware_version_show(struct device *dev,
                                     struct device_attribute *attr, char *buf)
{
    TP_DEBUG("*** firmware_version_show fw_version = %s***\n", fw_version);
    return sprintf(buf, "%s\n", fw_version);
}

static ssize_t firmware_version_store(struct device *dev,
                                      struct device_attribute *attr, const char *buf, size_t size)
{
	firmware_version();
    	return size;
}
static DEVICE_ATTR(version, 0777, firmware_version_show, firmware_version_store);

static ssize_t firmware_data_show(struct device *dev,
                                  struct device_attribute *attr, char *buf)
{
    return FwDataCnt;
}

static ssize_t firmware_data_store(struct device *dev,
                                   struct device_attribute *attr, const char *buf, size_t size)
{
    int i;
    Auto_APK_flag=1;
    TP_DEBUG("***FwDataCnt = %d ***\n", FwDataCnt);
    memcpy(temp[FwDataCnt], buf, 1024);
    FwDataCnt++;
    return size;
}
static DEVICE_ATTR(data, 0777, firmware_data_show, firmware_data_store);
#endif //APK_UPGRADE
#endif  //__FIRMWARE_UPDATE__

/**********************************************/

//#define HQ_CTP_PS_TEST

/*********************************************/


#ifdef HQ_CTP_PS_TEST

#define HQALSPS_DEVICE_NAME               		"hq-sprd-alsps-tpd-dev"
#define HQALSPS_INPUT_NAME               		"hq-sprd-alsps-tpd-input"
#define HQALSPS_IOCTL_MAGIC        			0XCF
#define HQALSPS_IOCTL_PROX_ON		_IO(HQALSPS_IOCTL_MAGIC, 7)
#define HQALSPS_IOCTL_PROX_OFF		_IO(HQALSPS_IOCTL_MAGIC, 8)
int ps_state = -1;
static int mstar2133_pls_opened=0;
//extern int tp_pls_status;

static void  msg2133_ps_mode_enable(bool enable)
{
	int err = 0;
	unsigned char dbbus_tx_data[4];
	unsigned char dbbus_rx_data[4];
	if(enable)
	{
	      dbbus_tx_data[0] = 0x52;
	      dbbus_tx_data[1] = 0x00;
	      dbbus_tx_data[2] = 0x4A;
	      dbbus_tx_data[3] = 0xA0;
	      err = i2c_master_send(i2c_clientma, &dbbus_tx_data[0], 4);
	      printk("i2c_client-addr=%d\n",i2c_clientma->addr);
	      printk(" pls wuyongtao_ctp ctp ps open err = %d \n",err);
	}
	else
	{     
	      dbbus_tx_data[0] = 0x52;
	      dbbus_tx_data[1] = 0x00;
	      dbbus_tx_data[2] = 0x4A;
	      dbbus_tx_data[3] = 0xA1;
	      err = i2c_master_send(i2c_clientma, &dbbus_tx_data[0], 4);
	      printk("pls  wuyongtao_ctp ctp ps close err = %d \n",err);
	}
	printk("pls  wuyongtao_ctp ctp ps msg2133a_ps_mode_enable\n");
} 

 int mstar2133_pls_enable(void)
 {
	 printk("<6> wuyongtao ==>ap3212c_pls_enable\n");
	 msg2133_ps_mode_enable(true);
	 mstar2133_pls_opened = 1;
	 //input_report_abs(input_dev, ABS_DISTANCE, 1);
	 //input_sync(input_dev);
	 //mod_timer(&ps_timer, jiffies + msecs_to_jiffies(200) );
	 return 0;
 }
 
 int mstar2133_pls_disable(void)
 {

	 printk("<6> wuyongtao ==>ap3212c_pls_disable");
	msg2133_ps_mode_enable(false);
	 mstar2133_pls_opened = 0;
	// del_timer_sync(&ps_timer);	 
	 return 0;
 }
 static int mstar2133_pls_open(struct inode *inode, struct file *file)
 {
	 printk("%s\n", __func__);
	 if (mstar2133_pls_opened)
		 return -EBUSY;
	 mstar2133_pls_opened = 1;
	 return 0;
 }
 static int mstar2133_pls_release(struct inode *inode, struct file *file)
 {
	 printk("%s", __func__);
	 mstar2133_pls_opened = 0;
	 return 0;//ap3212c_pls_disable(AP3212C_PLS_BOTH);
 }
 static long mstar2133_pls_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
 {
	 void __user *argp = (void __user *)arg;
	 printk("%s: cmd %d", __func__, _IOC_NR(cmd));
	 switch (cmd) {
	 case 1:
		 printk("%s: HQALSPS_IOCTL_PROX_ON*************************\n", __func__);
		 mstar2133_pls_enable();
		 break;
	 case 2:
		 printk("%s: HQALSPS_IOCTL_PROX_OFF*************************\n", __func__);
		 mstar2133_pls_disable();
		 break;
	 default:
		 printk("%s: invalid cmd %d\n", __func__, _IOC_NR(cmd));
		 return -EINVAL;
	 }
	 return 0;
 }

 static struct file_operations mstar2133_pls_fops = {
	 .owner 			 = THIS_MODULE,
	 .open			 = mstar2133_pls_open,
	 .release			 = mstar2133_pls_release,
	 .ioctl 			 = mstar2133_pls_ioctl,
 };
 static struct miscdevice mstar2133_pls_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = HQALSPS_DEVICE_NAME,
	.fops = &mstar2133_pls_fops,
};

int  get_msg2133_data(void )
{
	return  ps_state;
}

#endif

static ssize_t firmware_versions_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	TPD_DMESG("*** firmware_version_show fw_version = %s***\n", fw_version);
	return sprintf(buf, "%d.%03d \n", fw_major_version, fw_minor_version);
}

static DEVICE_ATTR(versions, 0444, firmware_versions_show,NULL);

 static u8 Calculate_8BitsChecksum( u8 *msg, s32 s32Length )
 {
	 s32 s32Checksum = 0;
	 s32 i;
 
	 for ( i = 0 ; i < s32Length; i++ )
	 {
		 s32Checksum += msg[i];
	 }
 
	 return (u8)( ( -s32Checksum ) & 0xFF );
 }

 #ifdef TPD_HAVE_BUTTON 
static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;
#endif

 static void msg21xx_data_disposal(void)
 {
       u8 val[8] = {0};
       u8 Checksum = 0;
	u8 i;
	u32 delta_x = 0, delta_y = 0;
	u32 u32X = 0;
	u32 u32Y = 0;
	u8 touchkeycode = 0;
	TouchScreenInfo_t  touchData;
	static u32 preKeyStatus=0;
	//static u32 preFingerNum=0;

//#define SWAP_X_Y   (1)
#define REVERSE_X  (1)
//#define REVERSE_Y  (1)
#ifdef SWAP_X_Y
	int tempx;
	int tempy;
#endif


//printk("*************msg21xx_data_disposal\n");
	i2c_master_recv(i2c_clientma,&val[0],REPORT_PACKET_LENGTH);
     Checksum = Calculate_8BitsChecksum(&val[0], 7); //calculate checksum
    if ((Checksum == val[7]) && (val[0] == 0x52))   //check the checksum  of packet
    {
        u32X = (((val[1] & 0xF0) << 4) | val[2]);         //parse the packet to coordinates
        u32Y = (((val[1] & 0x0F) << 8) | val[3]);

        delta_x = (((val[4] & 0xF0) << 4) | val[5]);
        delta_y = (((val[4] & 0x0F) << 8) | val[6]);

    #ifdef SWAP_X_Y
		tempy = u32X;
		tempx = u32Y;
        u32X = tempx;
        u32Y = tempy;

		tempy = delta_x;
		tempx = delta_y;
        delta_x = tempx;
        delta_y = tempy;
	#endif
	  #ifdef REVERSE_X
	  u32X = 2047 - u32X;
	  delta_x = 4095 - delta_x;
	  #endif
	  #ifdef REVERSE_Y
	  u32Y = 2047 - u32Y;
	  delta_y = 4095 - delta_y;
	  #endif
 //  printk ("[HAL] u32X = %x, u32Y = %x", u32X, u32Y);
//  printk ("[HAL] delta_x = %x, delta_y = %x", delta_x, delta_y);

        if ((val[1] == 0xFF) && (val[2] == 0xFF) && (val[3] == 0xFF) && (val[4] == 0xFF) && (val[6] == 0xFF))
        {
            touchData.Point[0].X = 0; // final X coordinate
            touchData.Point[0].Y = 0; // final Y coordinate

           if((val[5]==0x0)||(val[5]==0xFF))
            {
                touchData.nFingerNum = 0; //touch end
                touchData.nTouchKeyCode = 0; //TouchKeyMode
                touchData.nTouchKeyMode = 0; //TouchKeyMode
            }
            else
            {
                         printk("[MSG2133A]PS_TEST\n");
#ifdef HQ_CTP_PS_TEST
			if(0x80 == val[5])
			{	
				ps_state = 0;
				printk("[MSG2133A] ps_state=%d\n", ps_state);//add zy
				return 1;
			}
			else if (0x40 == val[5])
			{	
				ps_state = 1;
				printk("[MSG2133A] ps_state=%d\n", ps_state);//add zy
				return 1;
			}	
#endif
                touchData.nTouchKeyMode = 1; //TouchKeyMode
				        touchData.nTouchKeyCode = val[5]; //TouchKeyCode
                touchData.nFingerNum = 1;
	//	printk("key***%d****wwl****\n",val[5]);
	//	printk("finger num X= %d\n",touchData.Point[0].X);
	//	printk("finger num Y= %d\n",touchData.Point[0].Y);
		
            }
        }
		else
		{
		    touchData.nTouchKeyMode = 0; //Touch on screen...

			if(
			   #ifdef REVERSE_X
			       (delta_x == 4095)
			   #else
			       (delta_x == 0) 
			   #endif
			   && 
			   #ifdef REVERSE_Y
			       (delta_y == 4095)
			   #else
			       (delta_y == 0)
			   #endif
			  )
			{
				touchData.nFingerNum = 1; //one touch
				touchData.Point[0].X = (u32X * MS_TS_MSG21XX_X_MAX) / 2048;
				touchData.Point[0].Y = (u32Y * MS_TS_MSG21XX_Y_MAX) / 2048;
			}
			else
			{
				u32 x2, y2;

				touchData.nFingerNum = 2; //two touch

				/* Finger 1 */
				touchData.Point[0].X = (u32X * MS_TS_MSG21XX_X_MAX) / 2048;
				touchData.Point[0].Y = (u32Y * MS_TS_MSG21XX_Y_MAX) / 2048;

				/* Finger 2 */
				if (delta_x > 2048)     //transform the unsigh value to sign value
				{
					delta_x -= 4096;
				}
				if (delta_y > 2048)
				{
					delta_y -= 4096;
				}

				x2 = (u32)(u32X + delta_x);
				y2 = (u32)(u32Y + delta_y);

				touchData.Point[1].X = (x2 * MS_TS_MSG21XX_X_MAX) / 2048;
				touchData.Point[1].Y = (y2 * MS_TS_MSG21XX_Y_MAX) / 2048;
			}
		}
		
		//report...
		if(touchData.nTouchKeyMode)
		{

			if (touchData.nTouchKeyCode == 1)
				{
					touchkeycode = KEY_MENU;
					touchData.Point[0].X=265;     //change key code to point ABS by wwl
					touchData.Point[0].Y=505;	
				}
			
			if (touchData.nTouchKeyCode == 2)
				{
					touchkeycode =KEY_HOME;
					touchData.Point[0].X=159;
					touchData.Point[0].Y=505;	
				}
			if (touchData.nTouchKeyCode == 4)
				{
					touchkeycode =KEY_BACK;
					touchData.Point[0].X=53;
					touchData.Point[0].Y=505;
				}
			if (touchData.nTouchKeyCode == 8)
				touchkeycode = KEY_SEARCH;

			//printk("[msg2133A]**********touchKeyCode= %d  KEY_XXX.X= %d **********\n",touchData.nTouchKeyCode,touchData.Point[0].X);
			
//	printk("&&&&&&&&wwl useful key code report touch key code = %d\n",touchkeycode);
			if(preKeyStatus!=touchkeycode)
			{
				
				preKeyStatus=touchkeycode;
				
				if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
					{   
						printk("factory mode \n");
						input_report_key(tpd->dev, touchkeycode, 1);
						input_mt_sync(tpd->dev);
					}	 
				else
					{
						//printk("[msg2133A]--------------- touchKeyCode= %d  KEY_XXX.X= %d ****Input_report*****\n",touchData.nTouchKeyCode,touchData.Point[0].X);
						input_report_abs(tpd->dev, ABS_MT_PRESSURE, 255);
						input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 255);
						input_report_abs(tpd->dev, ABS_MT_POSITION_X, touchData.Point[0].X);
						input_report_abs(tpd->dev, ABS_MT_POSITION_Y, touchData.Point[0].Y);
						input_mt_sync(tpd->dev);
						touchData.nFingerNum=0;
					}
			}
			input_sync(tpd->dev);
		}
        else
        {
		    preKeyStatus=0; //clear key status..
//		printk("finger num = %d\n",touchData.nFingerNum);
            if((touchData.nFingerNum) == 0)   //touch end
            {

			if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
				{   
					printk("factory mode \n");
					input_report_key(tpd->dev,BTN_TOUCH,0);
			
				}
			//preFingerNum=0;
			input_report_key(tpd->dev,KEY_BACK , 0);
			input_report_key(tpd->dev, KEY_HOME, 0);
			input_report_key(tpd->dev, KEY_MENU, 0);
			input_report_key(tpd->dev, KEY_SEARCH, 0);
			input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 0);
			input_mt_sync(tpd->dev);
			input_sync(tpd->dev);
            }
            else //touch on screen
            {
			    /*
				if(preFingerNum!=touchData.nFingerNum)   //for one touch <--> two touch issue
				{
					printk("langwenlong number has changed\n");
					preFingerNum=touchData.nFingerNum;
					input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 0);
				    input_mt_sync(tpd->dev);
				    input_sync(tpd->dev);
				}*/

				for(i = 0;i < (touchData.nFingerNum);i++)
				{
					if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
						{   
							printk("factory mode \n");
							input_report_key(tpd->dev,BTN_TOUCH,1);
			
						}
					input_report_abs(tpd->dev, ABS_MT_PRESSURE, 255);
					input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 255);
					input_report_abs(tpd->dev, ABS_MT_POSITION_X, touchData.Point[i].X);
					input_report_abs(tpd->dev, ABS_MT_POSITION_Y, touchData.Point[i].Y);
					input_mt_sync(tpd->dev);
				}

					//		printk("finger num X= %d\n",touchData.Point[0].X);
						//	printk("finger num Y= %d\n",touchData.Point[0].Y);

							
				input_sync(tpd->dev);
			}
		}
    }
    else
    {
        printk("Packet error 0x%x, 0x%x, 0x%x\n", val[0], val[1], val[2]);
        printk("             0x%x, 0x%x, 0x%x\n", val[3], val[4], val[5]);
        printk("             0x%x, 0x%x, 0x%x\n", val[6], val[7], Checksum);
	printk(KERN_ERR "err status in tp\n");
    }
 }


#if defined(CTP_CHECK_FM)
void tpd_get_fm_frequency(int16_t freq) {
		fm_current_frequency = freq;
}
EXPORT_SYMBOL(tpd_get_fm_frequency);
#endif


 static int touch_event_handler(void *unused)
 {
	 struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
	 sched_setscheduler(current, SCHED_RR, &param);
 
	 do
	 {
	  mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM); 
		 set_current_state(TASK_INTERRUPTIBLE); 
		  wait_event_interruptible(waiter,tpd_flag!=0);		 
		tpd_flag = 0;
		 set_current_state(TASK_RUNNING);
		 msg21xx_data_disposal();
 }while(!kthread_should_stop());
 
	 return 0;
 }
 
 static int tpd_detect (struct i2c_client *client, int kind, struct i2c_board_info *info) 
 {
	 printk("[MSG2133A]-------------------------------------- tpd_detect \n",__func__);
	 strcpy(info->type, TPD_DEVICEMA);	
	 return 0;
 }
 
 static void tpd_eint_interrupt_handler(void)
 {
	 //TPD_DEBUG("TPD interrupt has been triggered\n");
	 tpd_flag = 1;
	 wake_up_interruptible(&waiter);
	 
 }

static int FW_repair_detect (void)
{
    u8 i;
    u8 dbbus_tx_data[4];
    unsigned char dbbus_rx_data[2] = {0};
	
    mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);

    _HalTscrHWReset();
    i2c_clientma->timing = 50;

    // Erase TP Flash first
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();
    mdelay ( 300 );

    // Disable the Watchdog
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x60;
    dbbus_tx_data[3] = 0x55;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x61;
    dbbus_tx_data[3] = 0xAA;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    // Stop MCU
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x0F;
    dbbus_tx_data[2] = 0xE6;
    dbbus_tx_data[3] = 0x01;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    /////////////////////////
    // Difference between C2 and C3
    /////////////////////////
    // c2:2133 c32:2133a(2) c33:2133a
    //check id
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0xCC;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    if ( dbbus_rx_data[0] == 2 )
    {
        // check version
        dbbus_tx_data[0] = 0x10;
        dbbus_tx_data[1] = 0x3C;
        dbbus_tx_data[2] = 0xEA;
        HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
        HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
        TP_DEBUG ( "dbbus_rx version[0]=0x%x\n", dbbus_rx_data[0] );

        if ( dbbus_rx_data[0] == 3 ){
		drvTP_read_info_dwiic_c33();
	}
    }
    i2c_clientma->timing = 100;
    i2c_clientma->addr = FW_ADDR_MSG21XX_TP;
    return 0;
}

static int Upgrade_detect()
{
	unsigned short wanted_major_version=0,wanted_minor_version=0;


	if(fw_major_version==1)//mutto
	{
		TPMSG="MUTTON";
		wanted_major_version= (MSG_FIRMWARE_MT[0x7f4f] << 8) + MSG_FIRMWARE_MT[0x7f4e];
		wanted_minor_version= (MSG_FIRMWARE_MT[0x7f51] << 8) + MSG_FIRMWARE_MT[0x7f50];
	}
	else if(fw_major_version==5) //EACH
	{
		TPMSG="EACH";
		wanted_major_version= (MSG_FIRMWARE_EA[0x7f4f] << 8) + MSG_FIRMWARE_EA[0x7f4e];
		wanted_minor_version= (MSG_FIRMWARE_EA[0x7f51] << 8) + MSG_FIRMWARE_EA[0x7f50];
	}
	else
	{
		FW_repair_detect();
		if(fw_major_version==1)//mutto
		{
			TPMSG="MUTTON";
			wanted_major_version= (MSG_FIRMWARE_MT[0x7f4f] << 8) + MSG_FIRMWARE_MT[0x7f4e];
			wanted_minor_version= (MSG_FIRMWARE_MT[0x7f51] << 8) + MSG_FIRMWARE_MT[0x7f50];
		}
		else if(fw_major_version==5) //EACH
		{
			TPMSG="EACH";
			wanted_major_version= (MSG_FIRMWARE_EA[0x7f4f] << 8) + MSG_FIRMWARE_EA[0x7f4e];
			wanted_minor_version= (MSG_FIRMWARE_EA[0x7f51] << 8) + MSG_FIRMWARE_EA[0x7f50];
		}
		else
		{
			return -1;
		}
	}

	printk("[MSG2133A][File_fw_version ] major = %d ,minor = %d ************************************\n",wanted_major_version,wanted_minor_version);
	printk("[MSG2133A][CTP_fw_version] major = %d ,minor = %d ************************************\n",fw_major_version,fw_minor_version);

	if(wanted_major_version==fw_major_version && wanted_minor_version>fw_minor_version)//version detect
		firmware_updata();
	//else if(fw_major_version==2 || fw_minor_version==6)//the test CTP is 2.001 and then force to upgrade
		//firmware_updata();
	return 0;
}

 static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
 {	 
	printk("[MSG2133A]--------------------------------------tpd_probe\n",__func__);
	int ret=0;
	int retval = TPD_OK;
	char data;
	unsigned char dbbus_tx_data[3];
	unsigned char dbbus_rx_data[4];
	i2c_clientma = client;

	//power on
	mt_set_gpio_mode(GPIO_CTP_EN_PIN, GPIO_CTP_EN_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_EN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ONE);
	mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ZERO);
	msleep(5);
	hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");
	msleep(5);
	mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ONE);
	msleep(500);

	if((i2c_smbus_read_i2c_block_data(i2c_clientma, 0x00, 1, &data))< 0)
	   {
		   printk("[CTP-I2C-ERROR] [Msg2133A]transfer error addr=0x%x------llf, line: %d\n", i2c_clientma->addr,__LINE__);
		   if(tpd_load_mode==1)
		   {
			   ret=Upgrade_detect();
			   if(ret<0){
			   	return -1;
			   }
			   else
			   {
				tpd_load_status = 1;
				//tp_pls_status = 0;
				printk("[MSG2133A][The CTP is MSG2133A ---llf] OK!**********\n");
			   }
		   }
	    	   else
		   {
			return -1;
		   }		 
	   }
	
#ifdef HQ_CTP_PS_TEST
	retval = misc_register(&mstar2133_pls_device);
	if (retval != 0) {
		printk("cannot register miscdev on mstar2133_pls_device\n");
	}
#endif
	/*frameware upgrade*/

	if(tpd_load_status==0){	
		dbbusDWIICEnterSerialDebugMode();
		dbbusDWIICStopMCU();
		dbbusDWIICIICUseBus();
		dbbusDWIICIICReshape();
		mdelay ( 300 );
		dbbus_tx_data[0] = 0x10;
		dbbus_tx_data[1] = 0x1E;
		dbbus_tx_data[2] = 0xCC;
		HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
		HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
		printk("[MSG2133A]The CTP ID ---:0x%x !**********\n",dbbus_rx_data[0]);
		if ( dbbus_rx_data[0] == 2 )
		{
			// check version
			dbbus_tx_data[0] = 0x10;
			dbbus_tx_data[1] = 0x3C;
			dbbus_tx_data[2] = 0xEA;
			HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
			HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
			TP_DEBUG ( "[MSG2133A] dbbus_rx version[0]=0x%x \n", dbbus_rx_data[0] );

			if ( dbbus_rx_data[0] == 3 ){
				tpd_load_status = 1;
				//tp_pls_status = 0;
				printk("[MSG2133A][The CTP is MSG2133A ---llf] OK!**********\n");
#ifdef __FIRMWARE_UPDATE__
			firmware_version();
			Upgrade_detect();
#endif
			}
			else
			{
				printk("[MSG2133A][The CTP is  Not MSG2133A ---llf] %s OK!**********\n");
				return -1;
			}
		}
		else
		{
			printk("[MSG2133A][The CTP is  Not MSG2133A ---llf] %s OK!**********\n");
			return -1;
		}
	}

#ifdef APK_UPGRADE
	firmware_class = class_create(THIS_MODULE, "ms-touchscreen-msg20xx");
	if (IS_ERR(firmware_class))
		pr_err("Failed to create class(firmware)!\n");
	firmware_cmd_dev = device_create(firmware_class,
	                             NULL, 0, NULL, "device");
	if (IS_ERR(firmware_cmd_dev))
		pr_err("Failed to create device(firmware_cmd_dev)!\n");

	// version
	if (device_create_file(firmware_cmd_dev, &dev_attr_version) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_version.attr.name);
	// update
	if (device_create_file(firmware_cmd_dev, &dev_attr_update) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_update.attr.name);
	// data
	if (device_create_file(firmware_cmd_dev, &dev_attr_data) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_data.attr.name);
	dev_set_drvdata(firmware_cmd_dev, NULL);
#endif

	//mdelay(100);reduce waiting time to speed up booting up add by wwl   

	mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
	mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);

	mt65xx_eint_set_sens(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_SENSITIVE);
	mt65xx_eint_set_hw_debounce(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);
	mt65xx_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, 1, tpd_eint_interrupt_handler, 1); 
	mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);


	msleep(100);
	mthread = kthread_run(touch_event_handler, 0, TPD_DEVICEMA);
	 if (IS_ERR(mthread))
		 { 
		  retval = PTR_ERR(mthread);
		  TPD_DMESG(TPD_DEVICEMA " failed to create kernel mthread: %d\n", retval);
		}
	if(device_create_file(&i2c_clientma->dev, &dev_attr_versions)>=0)
	{            
		printk("device_create_file_version \n");        
	}
	TPD_DMESG("Touch Panel Device Probe %s\n", (retval < TPD_OK) ? "FAIL" : "PASS");
	return 0;
   
 }

 static int __devexit tpd_remove(struct i2c_client *client)
 {
   
	 TPD_DEBUG("TPD removed\n");
 
   return 0;
 }
 
 
 static int tpd_local_init(void)
 {
	printk("[MSG2133A]--------------------------------------tpd_local_init\n",__func__);
	if(i2c_add_driver(&tpd_i2c_driver)!=0)
	{
			TPD_DMESG("unable to add i2c driver.\n");
	  		return -1;
	}
	if(tpd_load_status==0)  
	{
		TPD_DMESG("[MSG2133A] add i2c driver faild.\n");
		i2c_del_driver(&tpd_i2c_driver);
		return -1;
	}
  
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))    
	TPD_DO_WARP = 1;
	memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT*4);
	memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT*4);
#endif 

#ifdef TPD_HAVE_BUTTON     
	tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);// initialize tpd button data
#endif   

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
	memcpy(tpd_calmat, tpd_def_calmat_local, 8*4);
	memcpy(tpd_def_calmat, tpd_def_calmat_local, 8*4);	
#endif  
	TPD_DMESG("end %s, %d\n", __FUNCTION__, __LINE__);  
	tpd_type_cap = 1;
	return 0; 
 }

 static int tpd_resume(struct i2c_client *client)
 {
	int retval = TPD_OK;
 
	TPD_DEBUG("TPD wake up\n");
	mt_set_gpio_mode(GPIO_CTP_EN_PIN, GPIO_CTP_EN_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_EN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ONE);
	mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ZERO);
	msleep(5);
	hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");
	msleep(5);
	mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ONE);
	msleep(500);
	mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);  

 #ifdef HQ_CTP_PS_TEST

	if(mstar2133_pls_opened)
		{
			//power on
			//unmask();
			msg2133_ps_mode_enable(1);
			printk("==%s= mstar2133_pls_opened ! return\n", __func__);
		}
#endif

	 return retval;
 }
 
 static int tpd_suspend(struct i2c_client *client, pm_message_t message)
 {
	 int retval = TPD_OK;

#ifdef HQ_CTP_PS_TEST
 
	if(mstar2133_pls_opened)
	{
		printk("==%s== mstar2133_pls_opened ! return \n", __func__);
		return;
	}
	
#endif
 
	TPD_DEBUG("TPD enter sleep\n");
	mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);

	mt_set_gpio_mode(GPIO_CTP_EN_PIN, GPIO_CTP_EN_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_EN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ONE);
	mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ZERO);
	msleep(5);
	hwPowerDown(MT65XX_POWER_LDO_VGP2,"TP");
	msleep(5);

	return retval;
 } 


 static struct tpd_driver_t tpd_device_driver =
 {
		 .tpd_device_name = "MSG2133A",
		 .tpd_local_init = tpd_local_init,
		 .suspend = tpd_suspend,
		 .resume = tpd_resume,
#ifdef TPD_HAVE_BUTTON
		 .tpd_have_button = 1,
#else
		 .tpd_have_button = 0,
#endif		
 };
 /* called when loaded into kernel */
 static int __init tpd_driver_init(void)
 {
	 printk("MediaTek msg2133a touch panel driver init\n");
	 if(tpd_driver_add(&tpd_device_driver) < 0)
         TPD_DMESG("add msg21xx driver failed\n");
	 return 0;
 }
 
 /* should never be called */
 static void __exit tpd_driver_exit(void)
 {
	 TPD_DMESG("MediaTek msg2133a touch panel driver exit\n");
	 //input_unregister_device(tpd->dev);
	 tpd_driver_remove(&tpd_device_driver);
 }
 
 module_init(tpd_driver_init);
 module_exit(tpd_driver_exit);



