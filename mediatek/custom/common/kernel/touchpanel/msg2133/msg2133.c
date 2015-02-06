

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

unsigned char MSG_FIRMWARE_YJ[94*1024] =
{
  #include "Beetlelite-each.h"
};
unsigned char MSG_FIRMWARE_MT[94*1024] =
{
  #include "Beetlelite-mutto.h"
};


volatile static u8 Fmr_Loader[1024];
 
extern struct tpd_device *tpd;
 
struct i2c_client *i2c_client = NULL;
struct task_struct *thread = NULL;
 
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

#if defined(CTP_CHECK_FM)
static int16_t fm_current_frequency=0;
#endif


static const struct i2c_device_id tpd_id[] = {{TPD_DEVICE,0},{}};
unsigned short force[] = {0,0x4c,I2C_CLIENT_END,I2C_CLIENT_END}; 
static const unsigned short * const forces[] = { force, NULL };
static struct i2c_client_address_data addr_data = { .forces = forces, };
 
 
static struct i2c_driver tpd_i2c_driver =
{
  .driver = {
	 .name = TPD_DEVICE,
	 .owner = THIS_MODULE,
  },
  .probe = tpd_probe,
  .remove = __devexit_p(tpd_remove),
  .id_table = tpd_id,
  .detect = tpd_detect,
  .address_data = &addr_data,
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
struct class *firmware_class;
struct device *firmware_cmd_dev;


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

	rc = i2c_transfer(i2c_client->adapter, msgs, 1);
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
	rc = i2c_transfer(i2c_client->adapter, msgs, 1);
	if( rc < 0 )
    {
		printk("HalTscrCDevWriteI2CSeq error %d,addr = %d\n", rc,addr);
	}
}

#endif

static void HalTscrCReadI2CSeq(u8 addr, u8* read_data, u8 size)
{
	//mutex_lock(&tp_mutex); 
	i2c_client->addr = addr;	
	i2c_master_recv(i2c_client, &read_data[0], size);
	//mutex_unlock(&tp_mutex);
}

static void HalTscrCDevWriteI2CSeq(u8 addr, u8* data, u16 size)
{
	int ret;
	//mutex_lock(&tp_mutex);
	i2c_client->addr = addr;
	i2c_client->addr = i2c_client->addr & I2C_MASK_FLAG;
	ret = i2c_master_send(i2c_client, &data[0], size);
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

static void drvISP_EntryIspMode(void)
{
    u8 bWriteData[5] =
    {
        0x4D, 0x53, 0x54, 0x41, 0x52
    };

    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 5);
    mdelay ( 10 );        // delay about 1ms
}

static u8 drvISP_Read(u8 n, u8* pDataToRead)    //First it needs send 0x11 to notify we want to get flash data back.
{
    u8 Read_cmd = 0x11;
    unsigned char dbbus_rx_data[2] = {0};
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &Read_cmd, 1);
    udelay ( 100 );        // delay about 100us*****
    if (n == 1)
    {
        HalTscrCReadI2CSeq(FW_UPDATE_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
        *pDataToRead = dbbus_rx_data[1];  //poro:2012-04-27
    }
    else
    {
        HalTscrCReadI2CSeq(FW_UPDATE_ADDR_MSG21XX, pDataToRead, n);
    }

    return 0;
}

static void drvISP_WriteEnable(void)
{
    u8 bWriteData[2] =
    {
        0x10, 0x06
    };
    u8 bWriteData1 = 0x12;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
}


static void drvISP_ExitIspMode(void)
{
    u8 bWriteData = 0x24;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData, 1);
}

static u8 drvISP_ReadStatus(void)
{
    u8 bReadData = 0;
    u8 bWriteData[2] =
    {
        0x10, 0x05
    };
    u8 bWriteData1 = 0x12;

    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    udelay ( 100 );        // delay about 100us*****
    drvISP_Read(1, &bReadData);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
    return bReadData;
}


static void drvISP_BlockErase(u32 addr)
{
    u8 bWriteData[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
    u8 bWriteData1 = 0x12;
		u32 timeOutCount=0;
    drvISP_WriteEnable();

    //Enable write status register
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x50;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);

    //Write Status
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x01;
    bWriteData[2] = 0x00;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 3);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);

    //Write disable
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x04;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
		udelay ( 100 );        // delay about 100us*****
    timeOutCount=0;
		while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
		{
		   timeOutCount++;
	     if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
	  }
    drvISP_WriteEnable();

    bWriteData[0] = 0x10;
    bWriteData[1] = 0xC7;        //Block Erase
    //bWriteData[2] = ((addr >> 16) & 0xFF) ;
    //bWriteData[3] = ((addr >> 8) & 0xFF) ;
    //bWriteData[4] = (addr & 0xFF) ;
	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    //HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData, 5);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
		udelay ( 100 );        // delay about 100us*****
		timeOutCount=0;
		while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
		{
		   timeOutCount++;
	     if ( timeOutCount >= 500000 ) break; /* around 5 sec timeout */
	  }
}

static void drvISP_Program(u16 k, u8* pDataToWrite)
{
    u16 i = 0;
    u16 j = 0;
    //u16 n = 0;
    u8 TX_data[133];
    u8 bWriteData1 = 0x12;
    u32 addr = k * 1024;
		u32 timeOutCount=0;
    for (j = 0; j < 8; j++)   //128*8 cycle
    {
        TX_data[0] = 0x10;
        TX_data[1] = 0x02;// Page Program CMD
        TX_data[2] = (addr + 128 * j) >> 16;
        TX_data[3] = (addr + 128 * j) >> 8;
        TX_data[4] = (addr + 128 * j);
        for (i = 0; i < 128; i++)
        {
            TX_data[5 + i] = pDataToWrite[j * 128 + i];
        }
        udelay ( 100 );        // delay about 100us*****
       
        timeOutCount=0;
				while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
				{
		   			timeOutCount++;
	     			if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
	  		}
        
        
        
        drvISP_WriteEnable();
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, TX_data, 133);   //write 133 byte per cycle
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
    }
}


static void drvISP_Verify(u16 k, u8* pDataToVerify)
{
    u16 i = 0, j = 0;
    u8 bWriteData[5] =
    {
        0x10, 0x03, 0, 0, 0
    };
    u8 RX_data[256];
    u8 bWriteData1 = 0x12;
    u32 addr = k * 1024;
    u8 index=0;
    u32 timeOutCount;
    for (j = 0; j < 8; j++)   //128*8 cycle
    {
        bWriteData[2] = (u8)((addr + j * 128) >> 16);
        bWriteData[3] = (u8)((addr + j * 128) >> 8);
        bWriteData[4] = (u8)(addr + j * 128);
        udelay ( 100 );        // delay about 100us*****
        
        
        timeOutCount=0;
				while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
				{
		   		timeOutCount++;
	     		if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
	  		}
        
        
        
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 5);    //write read flash addr
        udelay ( 100 );        // delay about 100us*****
        drvISP_Read(128, RX_data);
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);    //cmd end
        for (i = 0; i < 128; i++)   //log out if verify error
        {
        if((RX_data[i]!=0)&&index<10)
		{
        //TP_DEBUG("j=%d,RX_data[%d]=0x%x\n",j,i,RX_data[i]);
        index++;
		}
            if (RX_data[i] != pDataToVerify[128 * j + i])
            {
                TP_DEBUG("k=%d,j=%d,i=%d===============Update Firmware Error================",k,j,i);
            }
        }
    }
}


static void drvISP_ChipErase(void)
{
    u8 bWriteData[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
    u8 bWriteData1 = 0x12;
		u32 timeOutCount=0;
    drvISP_WriteEnable();

    //Enable write status register
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x50;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);

    //Write Status
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x01;
    bWriteData[2] = 0x00;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 3);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);

    //Write disable
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x04;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
		udelay(100);        // delay about 100us*****
		udelay(100); 
    timeOutCount=0;
		while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
		{
		   timeOutCount++;
	     if ( timeOutCount >= 100000 ) 
	     	{
	     	printk("xxxxxxxxxx timeout xxxxxxxxxx\n");
		 break; /* around 1 sec timeout */
	     	}
		 }
    drvISP_WriteEnable();

//    bWriteData[0] = 0x10;
 //   bWriteData[1] = 0xC7;

 // 	HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
     bWriteData[0] = 0x10;
    bWriteData[1] = 0xD8;        //Block Erase
    bWriteData[2] = 0;
    bWriteData[3] = 0;
    bWriteData[4] = 0 ;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData[0], 5);

    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
		udelay(100);        // delay about 100us*****
		udelay(100); 
		timeOutCount=0;
		while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
		{
		   timeOutCount++;
	     if ( timeOutCount >= 500000 ) {

				     	printk("xxxxxxxxxx timeout 222xxxxxxxxxx\n");
	
			break; /* around 5 sec timeout */
			}
	  }
	printk("drvISP_ChipErase finished\n");
}


static void masterBUT_LoadFwToTarget(unsigned char *pfirmware, long n)
{
	u8 i;
	unsigned short j;
	u8 dbbus_tx_data[4];
	unsigned char dbbus_rx_data[2] = {0};
	//mutex_lock(&tp_mutex);
	i2c_client->timing = 50;
	//mutex_unlock(&tp_mutex);
	//		_HalTscrHWReset();
	//1.Erase TP Flash first
	printk("*** masterBUT_LoadFwToTarget degin \n");
	dbbusDWIICEnterSerialDebugMode();
	dbbusDWIICStopMCU();
	dbbusDWIICIICUseBus();
	dbbusDWIICIICReshape();
	mdelay(300);
		
				
	// Disable the Watchdog
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x3C;
	dbbus_tx_data[2] = 0x60;
	dbbus_tx_data[3] = 0x55;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x3C;
	dbbus_tx_data[2] = 0x61;
	dbbus_tx_data[3] = 0xAA;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	//Stop MCU
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x0F;
	dbbus_tx_data[2] = 0xE6;
	dbbus_tx_data[3] = 0x01;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);


	//set FRO to 50M
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x11;
	dbbus_tx_data[2] = 0xE2;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
	dbbus_rx_data[0] = 0;
	dbbus_rx_data[1] = 0;
	HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
	TP_DEBUG("dbbus_rx_data[0]=0x%x \n", dbbus_rx_data[0]);
	dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7;  //Clear Bit 3
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);



	//set MCU clock,SPI clock =FRO
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x22;
	dbbus_tx_data[3] = 0x00;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x23;
	dbbus_tx_data[3] = 0x00;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);


	// Enable slave's ISP ECO mode
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x08;
	dbbus_tx_data[2] = 0x0c;
	dbbus_tx_data[3] = 0x08;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

	//Enable SPI Pad
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x02;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
	HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
	TP_DEBUG("dbbus_rx_data[0]=0x%x \n", dbbus_rx_data[0]);
	dbbus_tx_data[3] = (dbbus_rx_data[0] | 0x20);  //Set Bit 5
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);


	//WP overwrite
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x0E;
	dbbus_tx_data[3] = 0x02;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);


	//set pin high
	dbbus_tx_data[0] = 0x10;
	dbbus_tx_data[1] = 0x1E;
	dbbus_tx_data[2] = 0x10;
	dbbus_tx_data[3] = 0x08;
	HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
	
	printk("*** write step 1 ok \n");
	dbbusDWIICIICNotUseBus();
	printk("*** dbbusDWIICIICNotUseBus ok \n");
	dbbusDWIICNotStopMCU();
	printk("*** dbbusDWIICNotStopMCU ok \n");
	udelay (100);
	dbbusDWIICExitSerialDebugMode();
	printk("*** dbbusDWIICExitSerialDebugMode ok \n");	
	drvISP_EntryIspMode();
	printk("*** write step 3 ok,first erase then write \n");
	drvISP_ChipErase();
//    _HalTscrHWReset();
//    mdelay(300);

	for (i = 0; i < 94; i++)   // total  94 KB : 1 byte per R/W
	{
		printk("xxxxx i = %d xxxx\n", i);
		for ( j = 0; j < 1024; j++ )        //Read 1k bytes
		{
			Fmr_Loader[j] = pfirmware[(i*1024)+j]; // Read the bin files of slave firmware from the baseband file system
		}

		drvISP_Program(i, Fmr_Loader);    // program to slave's flash
		//drvISP_Verify ( i, Fmr_Loader ); //verify data
	}
	drvISP_ExitIspMode();
	//mutex_lock(&tp_mutex);
	i2c_client->timing = 100;
	i2c_client->addr = FW_ADDR_MSG21XX_TP;			//back to normal addr
	//mutex_unlock(&tp_mutex);
}


#endif


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

/**********************************************/

#define HQ_CTP_PS_TEST

/*********************************************/


#ifdef HQ_CTP_PS_TEST

#define HQALSPS_DEVICE_NAME               		"hq-sprd-alsps-tpd-dev"
#define HQALSPS_INPUT_NAME               		"hq-sprd-alsps-tpd-input"
#define HQALSPS_IOCTL_MAGIC        	0XCF
#define HQALSPS_IOCTL_PROX_ON		_IO(HQALSPS_IOCTL_MAGIC, 7)
#define HQALSPS_IOCTL_PROX_OFF		_IO(HQALSPS_IOCTL_MAGIC, 8)
 int ps_state = -1;
static int mstar2133_pls_opened=0;
extern int tp_pls_status;

static void  msg2133_ps_mode_enable(bool enable)
{
	int err = 0;
	unsigned char dbbus_tx_data[4];
	unsigned char dbbus_rx_data[4];
	if(enable)
	{
	      dbbus_tx_data[0] = 0x52;
	      dbbus_tx_data[1] = 0x00;
	      dbbus_tx_data[2] = 0x62;
	      dbbus_tx_data[3] = 0xA0;
	      err = i2c_master_send(i2c_client, &dbbus_tx_data[0], 4);
		 printk("i2c_client-addr=%d\n",i2c_client->addr);
	      printk(" pls wuyongtao_ctp ctp ps open err = %d \n",err);
	}
	else
	{     
	      dbbus_tx_data[0] = 0x52;
	      dbbus_tx_data[1] = 0x00;
	      dbbus_tx_data[2] = 0x62;
	      dbbus_tx_data[3] = 0xA1;
	      err = i2c_master_send(i2c_client, &dbbus_tx_data[0], 4);
	      printk("pls  wuyongtao_ctp ctp ps close err = %d \n",err);
	}
	printk("pls  wuyongtao_ctp ctp ps msg2133_ps_mode_enable\n");
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
	 .open				 = mstar2133_pls_open,
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
	
#define SWAP_X_Y   (1)
//#define REVERSE_X  (1)
//#define REVERSE_Y  (1)
#ifdef SWAP_X_Y
	int tempx;
	int tempy;
#endif


//printk("*************msg21xx_data_disposal\n");
	i2c_master_recv(i2c_client,&val[0],REPORT_PACKET_LENGTH);
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
                        printk("PS_TEST\n");
#ifdef HQ_CTP_PS_TEST
			if(0x80 == val[5])
			{	
				ps_state = 0;
				printk("ps_state=%d\n", ps_state);//add zy
				return 1;
			}
			else if (0x40 == val[5])
			{	
				ps_state = 1;
				printk("ps_state=%d\n", ps_state);//add zy
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
					touchData.Point[0].X=318;     //change key code to point ABS by wwl
					touchData.Point[0].Y=530;	
				}
			
			if (touchData.nTouchKeyCode == 2)
				{
					touchkeycode =KEY_HOME ;
					touchData.Point[0].X=212;
					touchData.Point[0].Y=530;	
				}
			if (touchData.nTouchKeyCode == 4)
				{
					touchkeycode =KEY_BACK;
					touchData.Point[0].X=106;
					touchData.Point[0].Y=530;
				}
			if (touchData.nTouchKeyCode == 8)
				touchkeycode = KEY_SEARCH;
			
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

					       //change report key code to report abs add by wwl
									input_report_abs(tpd->dev, ABS_MT_PRESSURE, 255);
									input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 255);
									input_report_abs(tpd->dev, ABS_MT_POSITION_X, touchData.Point[0].X);
									input_report_abs(tpd->dev, ABS_MT_POSITION_Y, touchData.Point[0].Y);
									input_mt_sync(tpd->dev);
									touchData.nFingerNum=0;
						//		printk("&&&&&&&&useful key code report touch key code = %d\n",touchkeycode);
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
        //DBG("Packet error 0x%x, 0x%x, 0x%x", val[0], val[1], val[2]);
        //DBG("             0x%x, 0x%x, 0x%x", val[3], val[4], val[5]);
        //DBG("             0x%x, 0x%x, 0x%x", val[6], val[7], Checksum);
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
	 printk("[MSG2133]-------------------------------------- tpd_detect \n",__func__);
	 strcpy(info->type, TPD_DEVICE);	
	 return 0;
 }
 
 static void tpd_eint_interrupt_handler(void)
 {
	 TPD_DEBUG("TPD interrupt has been triggered\n");
	 tpd_flag = 1;
	 wake_up_interruptible(&waiter);
	 
 }
 static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
 {	 
	printk("[MSG2133]--------------------------------------tpd_probe\n",__func__);
	int retval = TPD_OK;
	int ret = -1;
	char data;
	unsigned char dbbus_tx_data[3];
	unsigned char dbbus_rx_data[4];
	i2c_client = client;

	unsigned short current_major_version=0, current_minor_version=0;
	unsigned short wanted_major_version=0,wanted_minor_version=0;
	char version[3][10] = {"MUTTO", "EACH","UNKNOWN"};
	char *pversion=version[2];
    //DavidDa


	hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");


	msleep(100);

	mt_set_gpio_mode(GPIO_CTP_EN_PIN, GPIO_CTP_EN_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_EN_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_CTP_EN_PIN, GPIO_PULL_DISABLE);
	msleep(100);

#ifdef HQ_CTP_PS_TEST
	ret = misc_register(&mstar2133_pls_device);
	if (ret != 0) {
		printk("cannot register miscdev on mstar2133_pls_device\n");
	}
#endif

 #if 0
	if((i2c_smbus_read_i2c_block_data(i2c_client, 0x00, 1, &data))< 0)
	   {
		   TPD_DMESG("I2C transfer error, line: %d\n", __LINE__);
		   return -1; 
	   }
#endif
	dbbus_tx_data[0] = 0x53;
	dbbus_tx_data[1] = 0x00;
	dbbus_tx_data[2] = 0x74;
	i2c_master_send(i2c_client, &dbbus_tx_data[0], 3);
	i2c_master_recv(i2c_client, &dbbus_rx_data[0], 4);

	current_major_version = (dbbus_rx_data[1]<<8)+dbbus_rx_data[0];
	current_minor_version = (dbbus_rx_data[3]<<8)+dbbus_rx_data[2];
	
	if(current_major_version == 0x0001 || current_major_version == 0x0005)
	{
		 tpd_load_status = 1;
		 tp_pls_status = 0;
  		printk("[MSG2133][The CTP is MSG2133 ---llf] %s OK!**********\n", __FUNCTION__);
	}
	else
	{
		tpd_load_status = 0;
  		printk("[MSG2133][The CTP is FT6306 ---llf] %s OK!**********\n", __FUNCTION__);
	}

	/*frameware upgrade*/
#ifdef __FIRMWARE_UPDATE__
     // masterBUT_LoadFwToTarget(MSG_FIRMWARE_YJ, sizeof(MSG_FIRMWARE_YJ));

/**************enter update firmware****************/
	//get version
	dbbus_tx_data[0] = 0x53;
	dbbus_tx_data[1] = 0x00;
	dbbus_tx_data[2] = 0x74;
	i2c_master_send(i2c_client, &dbbus_tx_data[0], 3);
	i2c_master_recv(i2c_client, &dbbus_rx_data[0], 4);


	current_major_version = (dbbus_rx_data[1]<<8)+dbbus_rx_data[0];
	current_minor_version = (dbbus_rx_data[3]<<8)+dbbus_rx_data[2];
	
	if(current_major_version == 0x0001)
		pversion = version[0];  // factory error:now we are using mutto's TP 
	else if(current_major_version == 0x0005)
		pversion = version[1];  // factory error:now we are using each's TP
	
	printk("***current version value is  %d %d\n", current_major_version, current_minor_version);
	printk("***%s 's current version is  %d\n",pversion, current_minor_version); 
	//wanted_major_version= (MSG_FIRMWARE_YJ[0x3076] << 8) + MSG_FIRMWARE_YJ[0x3077];
	//wanted_minor_version= (MSG_FIRMWARE_YJ[0x3074] << 8) + MSG_FIRMWARE_YJ[0x3075];
	//printk("***firmware version is  %d %d\n", wanted_major_version, wanted_minor_version);
	//printk("*** dummy download test\n");
	//masterBUT_LoadFwToTarget(MSG_FIRMWARE_YJ, sizeof(MSG_FIRMWARE_YJ));
	if (current_major_version == 0x0001) //
		{

			wanted_major_version= (MSG_FIRMWARE_MT[0x3076] << 8) + MSG_FIRMWARE_MT[0x3077];
			wanted_minor_version= (MSG_FIRMWARE_MT[0x3074] << 8) + MSG_FIRMWARE_MT[0x3075];
			printk("***firmware version is  %d %d\n", wanted_major_version, wanted_minor_version);
			if(current_minor_version <wanted_minor_version) 
				{
					printk("needed version %d is bigger than current version ,now updating...\n",wanted_minor_version);
					masterBUT_LoadFwToTarget(MSG_FIRMWARE_MT, sizeof(MSG_FIRMWARE_MT));

				}
			else if(current_minor_version==2)
				{
					printk("update YJ toupanel\n");
					masterBUT_LoadFwToTarget(MSG_FIRMWARE_YJ, sizeof(MSG_FIRMWARE_YJ));
				}
		}
		
	else if(current_major_version == 0x0005)
		{	
			wanted_major_version= (MSG_FIRMWARE_YJ[0x3076] << 8) + MSG_FIRMWARE_YJ[0x3077];
			wanted_minor_version= (MSG_FIRMWARE_YJ[0x3074] << 8) + MSG_FIRMWARE_YJ[0x3075];
			printk("***firmware version is  %d %d\n", wanted_major_version, wanted_minor_version);
			if(current_minor_version < wanted_minor_version) 
				{
					printk("needed version %d is bigger than current version ,now updating...\n",wanted_minor_version);
					masterBUT_LoadFwToTarget(MSG_FIRMWARE_YJ, sizeof(MSG_FIRMWARE_YJ));
				}

		
		}
	/****************end update****************/
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
	thread = kthread_run(touch_event_handler, 0, TPD_DEVICE);
	 if (IS_ERR(thread))
		 { 
		  retval = PTR_ERR(thread);
		  TPD_DMESG(TPD_DEVICE " failed to create kernel thread: %d\n", retval);
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
	printk("[MSG2133]--------------------------------------tpd_local_init\n",__func__);
   if(i2c_add_driver(&tpd_i2c_driver)!=0)
   	{
  		TPD_DMESG("unable to add i2c driver.\n");
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

  #ifdef HQ_CTP_PS_TEST

	if(mstar2133_pls_opened)
		{
			printk("==%s= mstar2133_pls_opened ! return\n", __func__);
			return;
		}
#endif
 
   TPD_DEBUG("TPD wake up\n");
//	hwPowerOn(MT65XX_POWER_LDO_VGP2,VOL_3300,"TP"); 

	hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");



	mt_set_gpio_mode(GPIO_CTP_EN_PIN, GPIO_CTP_EN_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_EN_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_CTP_EN_PIN, GPIO_PULL_DISABLE);
   mdelay(200);
   mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);  
	
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


	hwPowerDown(MT65XX_POWER_LDO_VGP2,"TP");


	mt_set_gpio_mode(GPIO_CTP_EN_PIN, GPIO_CTP_EN_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_EN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ZERO); 
	 return retval;
 } 


 static struct tpd_driver_t tpd_device_driver =
 {
		 .tpd_device_name = "msg21xx",
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
	 printk("MediaTek msg21xx touch panel driver init\n");
	 if(tpd_driver_add(&tpd_device_driver) < 0)
         TPD_DMESG("add msg21xx driver failed\n");
	 return 0;
 }
 
 /* should never be called */
 static void __exit tpd_driver_exit(void)
 {
	 TPD_DMESG("MediaTek msg21xx touch panel driver exit\n");
	 //input_unregister_device(tpd->dev);
	 tpd_driver_remove(&tpd_device_driver);
 }
 
 module_init(tpd_driver_init);
 module_exit(tpd_driver_exit);



