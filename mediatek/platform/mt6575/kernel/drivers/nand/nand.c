

#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/dma-mapping.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/time.h>

#include <asm/io.h>
#include <asm/cacheflush.h>
#include <asm/uaccess.h>

#include <linux/miscdevice.h>

//#include <mach/mt6575.h>
//#include <mach/mtk_nand_device.h>
#include <mach/mt6575_nand.h>
//#include "nand_customer.h"
#include "nand_device_list.h"
#include <mach/dma.h>
#include <mach/mt6575_devs.h>
#include <mach/mt6575_reg_base.h>
#include <mach/mt6575_typedefs.h>
//#include <mach/mt6575_irq.h>
/* Koshi for mt6575 porting */
#include <mach/mt6575_clock_manager.h>
/* Koshi for mt6575 porting */
#include "bmt.h"
#include "partition.h"

/* Added for TCM used */
#include <asm/tcm.h>
#include <asm/system.h>
#include "partition_define.h"
#include <mach/mt6575_boot.h>

#define VERSION  	"v2.0"
#define MODULE_NAME	"# MT6575 NAND #"
#define PROCNAME    "driver/nand"
#define PMT 1
#define _MTK_NAND_DUMMY_DRIVER_
//#define NAND_MT6577_E1_CHIP
//#define NAND_MT6577_E1_CHIP
#define __INTERNAL_USE_AHB_MODE__ 	(1)

#if __INTERNAL_USE_AHB_MODE__
BOOL g_bHwEcc=true;
#else
BOOL g_bHwEcc=false;
#endif

//#define NAND_MT6575_E1_CHIP

void show_stack(struct task_struct *tsk, unsigned long *sp);

// Added by jinming.xiang to support OTP
#define NAND_OTP_SUPPORT 1

#ifndef NAND_OTP_SUPPORT
#define NAND_OTP_SUPPORT 0
#endif

#if NAND_OTP_SUPPORT

#define SAMSUNG_OTP_SUPPORT     1
#define TOSHIBA_OTP_SUPPORT     1
#define OTP_MAGIC_NUM           0x4E3AF28B
#define SAMSUNG_OTP_PAGE_NUM    30 // erjing modify 
#define TOSHIBA_OTP_PAGE_NUM     30 // yongming.li  modify 
#define SAMSUNG_B050_OTP_PAGE_NUM     6 // yongming.li  modify 

static const unsigned int Samsung_OTP_Page[SAMSUNG_OTP_PAGE_NUM] = {0x15, 0x16, 0x17, 0x18, 0x19, 0x1b};

static struct mt6575_otp_config g_mt6575_otp_fuc;
static spinlock_t g_OTPLock;

#define OTP_MAGIC           'k'

/* NAND OTP IO control number */
#define OTP_GET_LENGTH 		_IOW(OTP_MAGIC, 1, int)
#define OTP_READ 	        _IOW(OTP_MAGIC, 2, int)
#define OTP_WRITE 			_IOW(OTP_MAGIC, 3, int)

#define FS_OTP_READ         0
#define FS_OTP_WRITE        1

/* NAND OTP Error codes */
#define OTP_SUCCESS                   0
#define OTP_ERROR_OVERSCOPE          -1
#define OTP_ERROR_TIMEOUT            -2
#define OTP_ERROR_BUSY               -3
#define OTP_ERROR_NOMEM              -4
#define OTP_ERROR_RESET              -5

typedef struct
{
	unsigned char imei[8];
	//NAND     17 bytes =     8 BCD bytes + 8 inversed BCD bytes + 1 flag byte
	//unsigned char svn;
	//unsigned char pad;
} IMEI_SVN;

typedef struct
{
	unsigned char imei[8];
	unsigned char imei_checksum[8];
	unsigned char res[16];
} OTP_LAYOUT;

typedef enum {
	E_READ = 0,
	E_WRITE
} E_RW;

#endif

// Begin. Added by jinming.xiang for OTP 2012.11.01
#ifndef DBG_EVT_OTP
#define DBG_EVT_OTP 0x00000100
#endif
// End. Added by jinming.xiang for OTP 2012.11.01

#if NAND_OTP_SUPPORT
struct mt6575_otp_config
{
	u32 (* OTPRead)         (u32 PageAddr, void *BufferPtr, void *SparePtr);
	u32 (* OTPWrite)        (u32 PageAddr, void *BufferPtr, void *SparePtr);
	u32 (* OTPQueryLength)  (u32 *Length);
};

struct otp_ctl
{
    unsigned int QLength;
    unsigned int Offset;
    unsigned int Length;
    char *BufferPtr;
    unsigned int status;
};
#endif

//#define NFI_SET_REG32(reg, value)   (DRV_WriteReg32(reg, DRV_Reg32(reg) | (value))) 
//#define NFI_SET_REG16(reg, value)   (DRV_WriteReg16(reg, DRV_Reg16(reg) | (value)))
//#define NFI_CLN_REG32(reg, value)   (DRV_WriteReg32(reg, DRV_Reg32(reg) & (~(value))))
//#define NFI_CLN_REG16(reg, value)   (DRV_WriteReg16(reg, DRV_Reg16(reg) & (~(value))))

#define NFI_SET_REG32(reg, value) \
do {	\
	g_value = (DRV_Reg32(reg) | (value));\
	DRV_WriteReg32(reg, g_value); \
} while(0)

#define NFI_SET_REG16(reg, value) \
do {	\
	g_value = (DRV_Reg16(reg) | (value));\
	DRV_WriteReg16(reg, g_value); \
} while(0)

#define NFI_CLN_REG32(reg, value) \
do {	\
	g_value = (DRV_Reg32(reg) & (~(value)));\
	DRV_WriteReg32(reg, g_value); \
} while(0)

#define NFI_CLN_REG16(reg, value) \
do {	\
	g_value = (DRV_Reg16(reg) & (~(value)));\
	DRV_WriteReg16(reg, g_value); \
} while(0)


#define NFI_WAIT_STATE_DONE(state) do{;}while (__raw_readl(NFI_STA_REG32) & state)
#define NFI_WAIT_TO_READY()  do{;}while (!(__raw_readl(NFI_STA_REG32) & STA_BUSY2READY))


#define NAND_SECTOR_SIZE (512)
#define OOB_PER_SECTOR      (16)
#define OOB_AVAI_PER_SECTOR (8)

#ifndef PART_SIZE_BMTPOOL
#define BMT_POOL_SIZE       (80)
#else
#define BMT_POOL_SIZE (PART_SIZE_BMTPOOL)
#endif

#define PMT_POOL_SIZE	(2)
#ifdef NAND_PFM
static suseconds_t g_PFM_R = 0;
static suseconds_t g_PFM_W = 0;
static suseconds_t g_PFM_E = 0;
static u32 g_PFM_RNum = 0;
static u32 g_PFM_RD = 0;
static u32 g_PFM_WD = 0;
static struct timeval g_now;

#define PFM_BEGIN(time) \
do_gettimeofday(&g_now); \
(time) = g_now;

#define PFM_END_R(time, n) \
do_gettimeofday(&g_now); \
g_PFM_R += (g_now.tv_sec * 1000000 + g_now.tv_usec) - (time.tv_sec * 1000000 + time.tv_usec); \
g_PFM_RNum += 1; \
g_PFM_RD += n; \
MSG(PERFORMANCE, "%s - Read PFM: %lu, data: %d, ReadOOB: %d (%d, %d)\n", MODULE_NAME , g_PFM_R, g_PFM_RD, g_kCMD.pureReadOOB, g_kCMD.pureReadOOBNum, g_PFM_RNum);

#define PFM_END_W(time, n) \
do_gettimeofday(&g_now); \
g_PFM_W += (g_now.tv_sec * 1000000 + g_now.tv_usec) - (time.tv_sec * 1000000 + time.tv_usec); \
g_PFM_WD += n; \
MSG(PERFORMANCE, "%s - Write PFM: %lu, data: %d\n", MODULE_NAME, g_PFM_W, g_PFM_WD);

#define PFM_END_E(time) \
do_gettimeofday(&g_now); \
g_PFM_E += (g_now.tv_sec * 1000000 + g_now.tv_usec) - (time.tv_sec * 1000000 + time.tv_usec); \
MSG(PERFORMANCE, "%s - Erase PFM: %lu\n", MODULE_NAME, g_PFM_E);
#else
#define PFM_BEGIN(time)
#define PFM_END_R(time, n)
#define PFM_END_W(time, n)
#define PFM_END_E(time)
#endif

#define TIMEOUT_1   0x1fff
#define TIMEOUT_2   0x8ff
#define TIMEOUT_3   0xffff
#define TIMEOUT_4   0xffff//5000   //PIO

#define NFI_ISSUE_COMMAND(cmd, col_addr, row_addr, col_num, row_num) \
   do { \
      DRV_WriteReg(NFI_CMD_REG16,cmd);\
      while (DRV_Reg32(NFI_STA_REG32) & STA_CMD_STATE);\
      DRV_WriteReg32(NFI_COLADDR_REG32, col_addr);\
      DRV_WriteReg32(NFI_ROWADDR_REG32, row_addr);\
      DRV_WriteReg(NFI_ADDRNOB_REG16, col_num | (row_num<<ADDR_ROW_NOB_SHIFT));\
      while (DRV_Reg32(NFI_STA_REG32) & STA_ADDR_STATE);\
   }while(0);

//-------------------------------------------------------------------------------
static struct completion g_comp_AHB_Done;
static struct mt6575_CMD g_kCMD;
static u32 g_u4ChipVer;
bool g_bInitDone;
static int g_i4Interrupt;
static bool g_bcmdstatus;
static u32 g_value = 0;
static int g_page_size;

static u8 *local_buffer_16_align;       // 16 byte aligned buffer, for HW issue
static u8 local_buffer[4096+16];

extern void nand_release_device(struct mtd_info *mtd);
extern int nand_get_device(struct nand_chip *chip, struct mtd_info *mtd, int new_state);

static bmt_struct *g_bmt;
struct mt6575_nand_host *host;

extern struct mtd_partition g_pasStatic_Partition[] ;
int part_num = NUM_PARTITIONS;
#ifdef PMT
extern void part_init_pmt(struct mtd_info *mtd, u8 *buf);
extern struct mtd_partition g_exist_Partition[] ;
#endif
int manu_id;
int dev_id;

static u8 local_oob_buf[NAND_MAX_OOBSIZE];

#ifdef _MTK_NAND_DUMMY_DRIVER_
int dummy_driver_debug;
#endif

void nand_enable_clock(void)
{
    enable_clock(MT65XX_PDN_PERI_NFI, "NAND");
}

void nand_disable_clock(void)
{
	//enable_clock(MT65XX_PDN_PERI_NFI, "NAND");
    disable_clock(MT65XX_PDN_PERI_NFI, "NAND");
}

static struct nand_ecclayout nand_oob_16 = {
	.eccbytes = 8,
	.eccpos = {8, 9, 10, 11, 12, 13, 14, 15},
	.oobfree = {{1,6}, {0, 0}}
};

struct nand_ecclayout nand_oob_64 = {
	.eccbytes = 32,
	.eccpos = {32, 33, 34, 35, 36, 37, 38, 39,
        40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55,
        56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = {{1, 7}, {9, 7}, {17, 7}, {25, 6}, {0, 0}}
};

struct nand_ecclayout nand_oob_128 = {
	.eccbytes = 64,
	.eccpos = {
        64, 65, 66, 67, 68, 69, 70, 71, 
        72, 73, 74, 75, 76, 77, 78, 79,
        80, 81, 82, 83, 84, 85, 86, 86,
        88, 89, 90, 91, 92, 93, 94, 95,
        96, 97, 98, 99, 100, 101, 102, 103,
        104, 105, 106, 107, 108, 109, 110, 111,
        112, 113, 114, 115, 116, 117, 118, 119,
        120, 121, 122, 123, 124, 125, 126, 127},
	.oobfree = {{1, 7}, {9, 7}, {17, 7}, {25, 7}, {33, 7}, {41, 7}, {49, 7}, {57, 6}}
};

flashdev_info devinfo;

void dump_nfi(void)
{
#if __DEBUG_NAND
		printk(KERN_ERR "~~~~Dump NFI Register~~~~\n");
		printk(KERN_ERR "NFI_CNFG_REG16: 0x%x\n",  DRV_Reg16(NFI_CNFG_REG16));
		printk(KERN_ERR "NFI_PAGEFMT_REG16: 0x%x\n",  DRV_Reg16(NFI_PAGEFMT_REG16));
		printk(KERN_ERR "NFI_CON_REG16: 0x%x\n",  DRV_Reg16(NFI_CON_REG16));
		printk(KERN_ERR "NFI_ACCCON_REG32: 0x%x\n",  DRV_Reg32(NFI_ACCCON_REG32));
		printk(KERN_ERR "NFI_INTR_EN_REG16: 0x%x\n",  DRV_Reg16(NFI_INTR_EN_REG16));
		printk(KERN_ERR "NFI_INTR_REG16: 0x%x\n",  DRV_Reg16(NFI_INTR_REG16));
		printk(KERN_ERR "NFI_CMD_REG16: 0x%x\n",  DRV_Reg16(NFI_CMD_REG16));
		printk(KERN_ERR "NFI_ADDRNOB_REG16: 0x%x\n",  DRV_Reg16(NFI_ADDRNOB_REG16));
		printk(KERN_ERR "NFI_COLADDR_REG32: 0x%x\n",  DRV_Reg32(NFI_COLADDR_REG32));
		printk(KERN_ERR "NFI_ROWADDR_REG32: 0x%x\n",  DRV_Reg32(NFI_ROWADDR_REG32));
		printk(KERN_ERR "NFI_STRDATA_REG16: 0x%x\n",  DRV_Reg16(NFI_STRDATA_REG16));
		printk(KERN_ERR "NFI_DATAW_REG32: 0x%x\n",  DRV_Reg32(NFI_DATAW_REG32));
		printk(KERN_ERR "NFI_DATAR_REG32: 0x%x\n",  DRV_Reg32(NFI_DATAR_REG32));
		printk(KERN_ERR "NFI_PIO_DIRDY_REG16: 0x%x\n",  DRV_Reg16(NFI_PIO_DIRDY_REG16));
		printk(KERN_ERR "NFI_STA_REG32: 0x%x\n",  DRV_Reg32(NFI_STA_REG32));
		printk(KERN_ERR "NFI_FIFOSTA_REG16: 0x%x\n",  DRV_Reg16(NFI_FIFOSTA_REG16));
		printk(KERN_ERR "NFI_LOCKSTA_REG16: 0x%x\n",  DRV_Reg16(NFI_LOCKSTA_REG16));
		printk(KERN_ERR "NFI_ADDRCNTR_REG16: 0x%x\n",  DRV_Reg16(NFI_ADDRCNTR_REG16));
		printk(KERN_ERR "NFI_STRADDR_REG32: 0x%x\n",  DRV_Reg32(NFI_STRADDR_REG32));
		printk(KERN_ERR "NFI_BYTELEN_REG16: 0x%x\n",  DRV_Reg16(NFI_BYTELEN_REG16));
		printk(KERN_ERR "NFI_CSEL_REG16: 0x%x\n",  DRV_Reg16(NFI_CSEL_REG16));
		printk(KERN_ERR "NFI_IOCON_REG16: 0x%x\n",  DRV_Reg16(NFI_IOCON_REG16));
		printk(KERN_ERR "NFI_FDM0L_REG32: 0x%x\n",  DRV_Reg32(NFI_FDM0L_REG32));
		printk(KERN_ERR "NFI_FDM0M_REG32: 0x%x\n",  DRV_Reg32(NFI_FDM0M_REG32));
		printk(KERN_ERR "NFI_LOCK_REG16: 0x%x\n",  DRV_Reg16(NFI_LOCK_REG16));
		printk(KERN_ERR "NFI_LOCKCON_REG32: 0x%x\n",  DRV_Reg32(NFI_LOCKCON_REG32));
		printk(KERN_ERR "NFI_LOCKANOB_REG16: 0x%x\n",  DRV_Reg16(NFI_LOCKANOB_REG16));
		printk(KERN_ERR "NFI_FIFODATA0_REG32: 0x%x\n",  DRV_Reg32(NFI_FIFODATA0_REG32));
		printk(KERN_ERR "NFI_FIFODATA1_REG32: 0x%x\n",  DRV_Reg32(NFI_FIFODATA1_REG32));
		printk(KERN_ERR "NFI_FIFODATA2_REG32: 0x%x\n",  DRV_Reg32(NFI_FIFODATA2_REG32));
		printk(KERN_ERR "NFI_FIFODATA3_REG32: 0x%x\n",  DRV_Reg32(NFI_FIFODATA3_REG32));
		printk(KERN_ERR "NFI_MASTERSTA_REG16: 0x%x\n",  DRV_Reg16(NFI_MASTERSTA_REG16));
		printk(KERN_ERR "NFI clock register: 0xF1000010: %s\n", (DRV_Reg32((volatile u32 *)0xF1000010) & (0x1)) ? "Clock Disabled" : "Clock Enabled");
		printk(KERN_ERR "NFI clock:0xF100002C: %s\n",(DRV_Reg32((volatile u32 *)0xF100002C) & (0x1)) ? "133MHz" : "66.5MHz");
#endif
}


bool get_device_info(u16 id, u32 ext_id, flashdev_info *pdevinfo)
{
	u32 index;
	for(index=0;gen_FlashTable[index].id!=0;index++)
	{
	   // if(id==gen_FlashTable[index].id && ext_id == gen_FlashTable[index].ext_id)
     	if (id == gen_FlashTable[index].id) 
      {
	      	ext_id = ((gen_FlashTable[index].ext_id&0xFF)==0xFF)?(ext_id|0xFF) : ext_id;
					if(ext_id == gen_FlashTable[index].ext_id){
			    
			    	pdevinfo->id = gen_FlashTable[index].id;
			    	pdevinfo->ext_id = gen_FlashTable[index].ext_id;
						pdevinfo->blocksize = gen_FlashTable[index].blocksize;
						pdevinfo->addr_cycle = gen_FlashTable[index].addr_cycle;
						pdevinfo->iowidth = gen_FlashTable[index].iowidth;
						pdevinfo->timmingsetting = gen_FlashTable[index].timmingsetting;
						pdevinfo->advancedmode = gen_FlashTable[index].advancedmode;
						pdevinfo->pagesize = gen_FlashTable[index].pagesize;
					pdevinfo->sparesize = gen_FlashTable[index].sparesize;
				    pdevinfo->totalsize = gen_FlashTable[index].totalsize;
						memcpy(pdevinfo->devciename,gen_FlashTable[index].devciename,sizeof(pdevinfo->devciename));
			      printk(KERN_INFO "Device found in MTK table, ID: %x, EXT_ID: %x\n", id, ext_id);
				
				    goto find;		
					}
			}
		
	}

    /*for (index = 0; cust_FlashTable[index].id != 0; index++)
    {
        if (id == cust_FlashTable[index].id)
        {
            pdevinfo->id = cust_FlashTable[index].id;
            pdevinfo->blocksize = cust_FlashTable[index].blocksize;
            pdevinfo->addr_cycle = cust_FlashTable[index].addr_cycle;
            pdevinfo->iowidth = cust_FlashTable[index].iowidth;
            pdevinfo->timmingsetting = cust_FlashTable[index].timmingsetting;
            pdevinfo->advancedmode = cust_FlashTable[index].advancedmode;
            pdevinfo->pagesize = cust_FlashTable[index].pagesize;
            pdevinfo->totalsize = cust_FlashTable[index].totalsize;
            memcpy(pdevinfo->devciename, cust_FlashTable[index].devciename, sizeof(pdevinfo->devciename));
            printk(KERN_INFO "Device found in customer table, ID: %x, EXT_ID: %x\n", id, ext_id);

            goto find;
        }
    }*/


find:
	if(0==pdevinfo->id)
	{
        printk(KERN_INFO "Device not found, ID: %x\n", id);
	    return false;
	}
	else
	{
		return true;
	}
}
/* Modified for TCM used */
//static __tcmfunc irqreturn_t mt6575_nand_irq_handler(int irqno, void *dev_id)
static irqreturn_t mt6575_nand_irq_handler(int irqno, void *dev_id)
{
   u16 u16IntStatus = DRV_Reg16(NFI_INTR_REG16);
   	(void)irqno;

    if (u16IntStatus & (u16)INTR_AHB_DONE_EN)
	{
    	complete(&g_comp_AHB_Done);
    } 
    return IRQ_HANDLED;
}

static void ECC_Config(struct mt6575_nand_host_hw *hw,u32 ecc_bit)
{
	u32 u4ENCODESize;
	u32 u4DECODESize;
  u32 ecc_bit_cfg = ECC_CNFG_ECC4;
  
  switch(ecc_bit){
  	case 4:
  		ecc_bit_cfg = ECC_CNFG_ECC4;
  		break;
  	case 8:
  		ecc_bit_cfg = ECC_CNFG_ECC8;
  		break;
  	case 10:
  		ecc_bit_cfg = ECC_CNFG_ECC10;
  		break;
  	case 12:
  		ecc_bit_cfg = ECC_CNFG_ECC12;
  		break;
    default:
  			break;
  	
  }
    DRV_WriteReg16(ECC_DECCON_REG16, DEC_DE);
    do{;}while (!DRV_Reg16(ECC_DECIDLE_REG16));
 
    DRV_WriteReg16(ECC_ENCCON_REG16, ENC_DE);
    do{;}while (!DRV_Reg32(ECC_ENCIDLE_REG32));

	/* setup FDM register base */
	DRV_WriteReg32(ECC_FDMADDR_REG32, NFI_FDM0L_REG32);

    /* Sector + FDM */
    u4ENCODESize = (hw->nand_sec_size + 8) << 3;
    /* Sector + FDM + YAFFS2 meta data bits */
	u4DECODESize = ((hw->nand_sec_size + 8) << 3) + ecc_bit * 13; 

	/* configure ECC decoder && encoder*/
	DRV_WriteReg32(ECC_DECCNFG_REG32,
		ecc_bit_cfg|DEC_CNFG_NFI|DEC_CNFG_EMPTY_EN|
		(u4DECODESize << DEC_CNFG_CODE_SHIFT));

	DRV_WriteReg32(ECC_ENCCNFG_REG32, 
		ecc_bit_cfg|ENC_CNFG_NFI|
		(u4ENCODESize << ENC_CNFG_MSG_SHIFT));
#ifndef MANUAL_CORRECT
		NFI_SET_REG32(ECC_DECCNFG_REG32, DEC_CNFG_CORRECT);
#else
		NFI_SET_REG32(ECC_DECCNFG_REG32, DEC_CNFG_EL);
#endif
}

static void ECC_Decode_Start(void)
{
   	/* wait for device returning idle */
	while(!(DRV_Reg16(ECC_DECIDLE_REG16) & DEC_IDLE));
	DRV_WriteReg16(ECC_DECCON_REG16, DEC_EN);
}

static void ECC_Decode_End(void)
{
   /* wait for device returning idle */
	while(!(DRV_Reg16(ECC_DECIDLE_REG16) & DEC_IDLE));
	DRV_WriteReg16(ECC_DECCON_REG16, DEC_DE);
}

static void ECC_Encode_Start(void)
{
   /* wait for device returning idle */
	while(!(DRV_Reg32(ECC_ENCIDLE_REG32) & ENC_IDLE));
	mb();
	DRV_WriteReg16(ECC_ENCCON_REG16, ENC_EN);
}

static void ECC_Encode_End(void)
{
   /* wait for device returning idle */
	while(!(DRV_Reg32(ECC_ENCIDLE_REG32) & ENC_IDLE));
	mb();
	DRV_WriteReg16(ECC_ENCCON_REG16, ENC_DE);
}

static bool mt6575_nand_check_bch_error(
	struct mtd_info *mtd, u8* pDataBuf, u32 u4SecIndex, u32 u4PageAddr)
{
	bool bRet = true;
	u16 u2SectorDoneMask = 1 << u4SecIndex;
	u32 u4ErrorNumDebug, i, u4ErrNum;
	u32 timeout = 0xFFFF;
    u32 correct_count = 0;
	// int el;
#ifdef MANUAL_CORRECT
	u32 au4ErrBitLoc[6];
	u32 u4ErrByteLoc, u4BitOffset;
	u32 u4ErrBitLoc1th, u4ErrBitLoc2nd;
#endif

	//4 // Wait for Decode Done
	while (0 == (u2SectorDoneMask & DRV_Reg16(ECC_DECDONE_REG16)))
    {       
		timeout--;
		if (0 == timeout)
        {
			printk(KERN_ERR "[%s]: %d Timeout!\n", __FUNCTION__, __LINE__);
			dump_nfi();
			return false;
		}
	}
#ifndef MANUAL_CORRECT
	u4ErrorNumDebug = DRV_Reg32(ECC_DECENUM_REG32);
	if (0 != (u4ErrorNumDebug & 0xFFFF))
    {
		for (i = 0; i <= u4SecIndex; ++i)
        {
			u4ErrNum = DRV_Reg32(ECC_DECENUM_REG32) >> (i << 2);
			u4ErrNum &= 0xF;
            correct_count += u4ErrNum;
            
			if (0xF == u4ErrNum)
            {
				mtd->ecc_stats.failed++;
				bRet = false;
				printk(KERN_ERR"UnCorrectable at PageAddr=%d, Sector=%d\n", u4PageAddr, i);
			} 
            else 
            {
                if (u4ErrNum)
                {
			    printk(KERN_ERR" In kernel Correct %d at PageAddr=%d, Sector=%d\n", u4ErrNum, u4PageAddr, i);
                }
			}
		}
        if ((correct_count > 2) && bRet)
        {
				mtd->ecc_stats.corrected++;
		}
        else
        {
            printk(KERN_INFO "Less than 2 bit error, ignore\n");
		}
	}
#else
	/* We will manually correct the error bits in the last sector, not all the sectors of the page!*/
	memset(au4ErrBitLoc, 0x0, sizeof(au4ErrBitLoc));
	u4ErrorNumDebug = DRV_Reg32(ECC_DECENUM_REG32);
	u4ErrNum = DRV_Reg32(ECC_DECENUM_REG32) >> (u4SecIndex << 2);
	u4ErrNum &= 0xF;
    
	if (u4ErrNum)
    {
		if (0xF == u4ErrNum)
        {
			mtd->ecc_stats.failed++;
			bRet = false;
			//printk(KERN_ERR"UnCorrectable at PageAddr=%d\n", u4PageAddr);
		} 
        else 
        {
			for (i = 0; i < ((u4ErrNum+1)>>1); ++i)
            {
				au4ErrBitLoc[i] = DRV_Reg32(ECC_DECEL0_REG32 + i);
				u4ErrBitLoc1th = au4ErrBitLoc[i] & 0x1FFF;
                
				if (u4ErrBitLoc1th < 0x1000)
                {
					u4ErrByteLoc = u4ErrBitLoc1th/8;
					u4BitOffset = u4ErrBitLoc1th%8;
					pDataBuf[u4ErrByteLoc] = pDataBuf[u4ErrByteLoc]^(1<<u4BitOffset);
					mtd->ecc_stats.corrected++;
				} 
                else 
                {
					mtd->ecc_stats.failed++;
					//printk(KERN_ERR"UnCorrectable ErrLoc=%d\n", au4ErrBitLoc[i]);
				}
				u4ErrBitLoc2nd = (au4ErrBitLoc[i] >> 16) & 0x1FFF;
				if (0 != u4ErrBitLoc2nd)
                {
					if (u4ErrBitLoc2nd < 0x1000)
                    {
						u4ErrByteLoc = u4ErrBitLoc2nd/8;
						u4BitOffset = u4ErrBitLoc2nd%8;
						pDataBuf[u4ErrByteLoc] = pDataBuf[u4ErrByteLoc]^(1<<u4BitOffset);
						mtd->ecc_stats.corrected++;
					} 
                    else 
                    {
						mtd->ecc_stats.failed++;
						//printk(KERN_ERR"UnCorrectable High ErrLoc=%d\n", au4ErrBitLoc[i]);
					}
				}
			}
		}
		if (0 == (DRV_Reg16(ECC_DECFER_REG16) & (1 << u4SecIndex)))
        {
			bRet = false;
		}
	}
#endif
	return bRet;
}

static bool mt6575_nand_RFIFOValidSize(u16 u2Size)
{
	u32 timeout = 0xFFFF;
	while (FIFO_RD_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) < u2Size)
    {
		timeout--;
		if (0 == timeout){
			printk(KERN_ERR "[%s]: %d Timeout!\n", __FUNCTION__, __LINE__);
			dump_nfi();
			return false;
		}
	}
	return true;
}

static bool mt6575_nand_WFIFOValidSize(u16 u2Size)
{
	u32 timeout = 0xFFFF;
	while (FIFO_WR_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) > u2Size)
    {
		timeout--;
		if (0 == timeout)
        {
			printk(KERN_ERR "[%s]: %d Timeout!\n", __FUNCTION__, __LINE__);
			dump_nfi();
			return false;
		}
	}
	return true;
}

static bool mt6575_nand_status_ready(u32 u4Status)
{
	u32 timeout = 0xFFFF;
	while ((DRV_Reg32(NFI_STA_REG32) & u4Status) != 0)
    {
		timeout--;
		if (0 == timeout)
        {
			printk(KERN_ERR "[%s]: %d Wait nand status ready timeout\n", __FUNCTION__, __LINE__);
			dump_nfi();
			return false;
		}
	}
	return true;
}

static bool mt6575_nand_reset(void)
{
    // HW recommended reset flow
    int timeout = 0xFFFF;
    if (DRV_Reg16(NFI_MASTERSTA_REG16)) // master is busy
    {
    		mb();
        DRV_WriteReg16(NFI_CON_REG16, CON_FIFO_FLUSH | CON_NFI_RST);
        while (DRV_Reg16(NFI_MASTERSTA_REG16))
        {
            timeout--;
            if (0 == timeout)
            {
				printk(KERN_ERR "[%s]: %d Wait MASTERSTA Timeout!\n", __FUNCTION__, __LINE__);
				dump_nfi();
				return false;
            }
        }
    }
	/* issue reset operation */
	mb();
	DRV_WriteReg16(NFI_CON_REG16, CON_FIFO_FLUSH | CON_NFI_RST);

	return mt6575_nand_status_ready(STA_NFI_FSM_MASK|STA_NAND_BUSY) &&
		   mt6575_nand_RFIFOValidSize(0) &&
		   mt6575_nand_WFIFOValidSize(0);
}

static void mt6575_nand_set_mode(u16 u2OpMode)
{
	u16 u2Mode = DRV_Reg16(NFI_CNFG_REG16);
	u2Mode &= ~CNFG_OP_MODE_MASK;
	u2Mode |= u2OpMode;
	DRV_WriteReg16(NFI_CNFG_REG16, u2Mode);
}

static void mt6575_nand_set_autoformat(bool bEnable)
{
	if (bEnable)
    {
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AUTO_FMT_EN);
	}
    else
    {
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AUTO_FMT_EN);
	}
}

static void mt6575_nand_configure_fdm(u16 u2FDMSize)
{
	NFI_CLN_REG16(NFI_PAGEFMT_REG16, PAGEFMT_FDM_MASK | PAGEFMT_FDM_ECC_MASK);
	NFI_SET_REG16(NFI_PAGEFMT_REG16, u2FDMSize << PAGEFMT_FDM_SHIFT);
	NFI_SET_REG16(NFI_PAGEFMT_REG16, u2FDMSize << PAGEFMT_FDM_ECC_SHIFT);
}

static void mt6575_nand_configure_lock(void)
{
	u32 u4WriteColNOB = 2;
	u32 u4WriteRowNOB = 3;
	u32 u4EraseColNOB = 0;
	u32 u4EraseRowNOB = 3;
	DRV_WriteReg16(NFI_LOCKANOB_REG16, 
		(u4WriteColNOB << PROG_CADD_NOB_SHIFT)  |
		(u4WriteRowNOB << PROG_RADD_NOB_SHIFT)  |
		(u4EraseColNOB << ERASE_CADD_NOB_SHIFT) |
		(u4EraseRowNOB << ERASE_RADD_NOB_SHIFT));

	if (CHIPVER_ECO_1 == g_u4ChipVer)
    {
		int i;
		for (i = 0; i < 16; ++i)
        {
			DRV_WriteReg32(NFI_LOCK00ADD_REG32 + (i << 1), 0xFFFFFFFF);
			DRV_WriteReg32(NFI_LOCK00FMT_REG32 + (i << 1), 0xFFFFFFFF);
		}
		//DRV_WriteReg16(NFI_LOCKANOB_REG16, 0x0);
		DRV_WriteReg32(NFI_LOCKCON_REG32, 0xFFFFFFFF);
		DRV_WriteReg16(NFI_LOCK_REG16, NFI_LOCK_ON);
	}	
}

static bool mt6575_nand_pio_ready(void)
{
    int count = 0;
    while ( !(DRV_Reg16(NFI_PIO_DIRDY_REG16) & 1) )
    {
        count++;
        if (count > 0xffff)
        {
			printk(KERN_ERR "[%s]: %d PIO_DIRDY timeout\n", __FUNCTION__, __LINE__);
			dump_nfi();
			return false;
        }
    }

    return true;
}

static bool mt6575_nand_set_command(u16 command)
{
	/* Write command to device */
	mb();
	DRV_WriteReg16(NFI_CMD_REG16, command);
	return mt6575_nand_status_ready(STA_CMD_STATE);
}

static bool mt6575_nand_set_address(u32 u4ColAddr, u32 u4RowAddr, u16 u2ColNOB, u16 u2RowNOB)
{
	/* fill cycle addr */
	mb();
	DRV_WriteReg32(NFI_COLADDR_REG32, u4ColAddr);
	DRV_WriteReg32(NFI_ROWADDR_REG32, u4RowAddr);
	DRV_WriteReg16(NFI_ADDRNOB_REG16, u2ColNOB|(u2RowNOB << ADDR_ROW_NOB_SHIFT));
	return mt6575_nand_status_ready(STA_ADDR_STATE);
}

static bool mt6575_nand_check_RW_count(u16 u2WriteSize)
{
	u32 timeout = 0xFFFF;
	u16 u2SecNum = u2WriteSize >> 9;
    
	while (ADDRCNTR_CNTR(DRV_Reg16(NFI_ADDRCNTR_REG16)) < u2SecNum)
    {
		timeout--;
		if (0 == timeout)
        {
			printk(KERN_ERR "[%s]: %d timeout\n", __FUNCTION__, __LINE__);
			dump_nfi();
			return false;
		}
	}
	return true;
}

static bool mt6575_nand_ready_for_read(struct nand_chip *nand, u32 u4RowAddr, u32 u4ColAddr, bool full, u8 *buf)
{
	/* Reset NFI HW internal state machine and flush NFI in/out FIFO */	
	bool bRet = false;
	u16 sec_num = 1 << (nand->page_shift - 9);
    u32 col_addr = u4ColAddr;
	u32 colnob=2, rownob=devinfo.addr_cycle-2;	
    if (nand->options & NAND_BUSWIDTH_16)
        col_addr /= 2;

	if (!mt6575_nand_reset())
    {
		goto cleanup;
	}
	if(g_bHwEcc){
    	/* Enable HW ECC */
    	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	}else{
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	}

	mt6575_nand_set_mode(CNFG_OP_READ);
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
	DRV_WriteReg16(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);

    if (full)
    {
#if __INTERNAL_USE_AHB_MODE__
	    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
        DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(buf));
#else
    	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
#endif

	if(g_bHwEcc){	
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	}else{
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	}

    }
    else
    {
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
    }

	mt6575_nand_set_autoformat(full);
    if (full){
		if(g_bHwEcc){
			ECC_Decode_Start();
		}
    }

	if (!mt6575_nand_set_command(NAND_CMD_READ0))
    {
		goto cleanup;
	}

	//1 FIXED ME: For Any Kind of AddrCycle
	if (!mt6575_nand_set_address(col_addr, u4RowAddr, colnob, rownob))
    {
		goto cleanup;
	}

	if (!mt6575_nand_set_command(NAND_CMD_READSTART))
    {
		goto cleanup;
	}

	if (!mt6575_nand_status_ready(STA_NAND_BUSY))
    {
		goto cleanup;
	}

	bRet = true;
	
cleanup:
	return bRet;
}

static bool mt6575_nand_ready_for_write(
	struct nand_chip *nand, u32 u4RowAddr, u32 col_addr, bool full, u8 *buf)
{
	bool bRet = false;
	u32 sec_num = 1 << (nand->page_shift - 9);
	u32 colnob=2, rownob=devinfo.addr_cycle-2;
	
    if (nand->options & NAND_BUSWIDTH_16)
        col_addr /= 2;


	/* Reset NFI HW internal state machine and flush NFI in/out FIFO */	
	if (!mt6575_nand_reset())
    {
		return false;
	}

	mt6575_nand_set_mode(CNFG_OP_PRGM);
	
	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
	
	DRV_WriteReg16(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);

    if (full)
    {
#if __INTERNAL_USE_AHB_MODE__
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
    DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(buf));
#else
	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
#endif
	if(g_bHwEcc){
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	}else{
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	}
    }
    else
    {
	    NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	    NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
    }

	mt6575_nand_set_autoformat(full);

    if (full){
		if(g_bHwEcc){
			ECC_Encode_Start();
		}
    	}

	if (!mt6575_nand_set_command(NAND_CMD_SEQIN)){
		goto cleanup;
	}

	//1 FIXED ME: For Any Kind of AddrCycle
	if (!mt6575_nand_set_address(col_addr, u4RowAddr, colnob, rownob)){
		goto cleanup;
	}

	if (!mt6575_nand_status_ready(STA_NAND_BUSY)){
		goto cleanup;
	}

	bRet = true;
cleanup:

	return bRet;
}

static bool mt6575_nand_check_dececc_done(u32 u4SecNum)
{
    u32 timeout, dec_mask;
    timeout = 0xffff;
    dec_mask = (1<<u4SecNum)-1;
    while((dec_mask != DRV_Reg(ECC_DECDONE_REG16)) && timeout>0)
        timeout--;
    if(timeout == 0){
		printk(KERN_ERR "[%s]: %d ECC_DECDONE: timeout\n", __FUNCTION__, __LINE__);
		dump_nfi();
		return false;
    }
    return true;
}

static bool mt6575_nand_dma_read_data(struct mtd_info *mtd, u8 *buf, u32 length)
{
    int interrupt_en = g_i4Interrupt;
    int timeout = 0xffff;
    struct scatterlist sg;
    enum dma_data_direction dir = DMA_FROM_DEVICE;
  
    sg_init_one(&sg, buf, length);
	dma_map_sg(&(mtd->dev), &sg, 1, dir);

	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
	// DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(pDataBuf));
 
    if ((unsigned int)buf % 16)		// TODO: can not use AHB mode here
    {
        printk(KERN_INFO "Un-16-aligned address\n");
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
    }
    else
    {
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
    }

    DRV_Reg16(NFI_INTR_REG16);
    DRV_WriteReg16(NFI_INTR_EN_REG16, INTR_AHB_DONE_EN);

    if (interrupt_en) 
    {
	    init_completion(&g_comp_AHB_Done);
	}
	//dmac_inv_range(pDataBuf, pDataBuf + u4Size);
	mb();
	NFI_SET_REG16(NFI_CON_REG16, CON_NFI_BRD);
   
	if (interrupt_en) 
    {
	    if (!wait_for_completion_timeout(&g_comp_AHB_Done, 2))
        {
            printk(KERN_ERR "[%s]: %d wait for completion timeout happened \n", __FUNCTION__, __LINE__);
            dump_nfi();
            return false;
        }
        while ( (length >> 9) > ((DRV_Reg16(NFI_BYTELEN_REG16) & 0xf000) >> 12) )
        {
		    timeout--;
		    if (0 == timeout)
            {
            	printk(KERN_ERR "[%s]: %d poll BYTELEN error\n", __FUNCTION__, __LINE__);
				dump_nfi();
				return false;// AHB Mode Time Out!
			}
		}	
    } 
    else 
    {
        while (!DRV_Reg16(NFI_INTR_REG16))
        {
		    timeout--;
		    if (0 == timeout)
            {
            	printk(KERN_ERR "[%s]: %d poll nfi_intr error \n", __FUNCTION__, __LINE__);
                dump_nfi();
			    return false; // AHB Mode Time Out!
            }
        }
        while ( (length >> 9) > ((DRV_Reg16(NFI_BYTELEN_REG16) & 0xf000) >> 12) )
        {
		    timeout--;
		    if (0 == timeout)
            {
				printk(KERN_ERR "[%s]: %d poll BYTELEN error\n", __FUNCTION__, __LINE__);
				dump_nfi();
				return false; // AHB Mode Time Out!
			}
		}
	}
	
	dma_unmap_sg(&(mtd->dev), &sg, 1, dir);

    return true;
}

static bool mt6575_nand_mcu_read_data(u8 *buf, u32 length)
{
    int timeout = 0xffff;
	u32 i;
	u32* buf32 = (u32 *)buf;
#ifdef TESTTIME		
	unsigned long long time1,time2;
    time1 = sched_clock();
#endif
    if ((u32)buf % 4 || length % 4)
	    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
    else
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);

    //DRV_WriteReg32(NFI_STRADDR_REG32, 0);
    mb();
	NFI_SET_REG16(NFI_CON_REG16, CON_NFI_BRD);
	
    if ((u32)buf % 4 || length % 4)
    {
        for (i = 0; (i < (length))&&(timeout > 0);)
        {
    		//if (FIFO_RD_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) >= 4)
    		if (DRV_Reg16(NFI_PIO_DIRDY_REG16) & 1)
            {
    			*buf++ = (u8)DRV_Reg32(NFI_DATAR_REG32);
    			i++;
    		} 
            else 
            {
    			timeout--;
    		}
    		if (0 == timeout)
            {
				printk(KERN_ERR "[%s]: %d timeout \n", __FUNCTION__, __LINE__);
				dump_nfi();
				return false;
    		}
    	}
    }
    else
    {
        for (i = 0; (i < (length >> 2))&&(timeout > 0);)
        {
    		//if (FIFO_RD_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) >= 4)
    		if (DRV_Reg16(NFI_PIO_DIRDY_REG16) & 1)
            {
    			*buf32++ = DRV_Reg32(NFI_DATAR_REG32);
    			i++;
    		} 
            else 
            {
    			timeout--;
    		}
    		if (0 == timeout)
            {
				printk(KERN_ERR "[%s]: %d timeout\n", __FUNCTION__, __LINE__);
				dump_nfi();
				return false;
    		}
    	}
     }
#ifdef TESTTIME		
	time2 = sched_clock()-time1;
	if(!readdatatime)
	{
	   readdatatime=(time2);
	}
#endif
    return true;
}

static bool mt6575_nand_read_page_data(struct mtd_info *mtd, u8* pDataBuf, u32 u4Size)
{
#if (__INTERNAL_USE_AHB_MODE__)
    return mt6575_nand_dma_read_data(mtd, pDataBuf, u4Size);
#else
    return mt6575_nand_mcu_read_data(mtd, pDataBuf, u4Size);
#endif
}	

static bool mt6575_nand_dma_write_data(struct mtd_info *mtd, u8 *pDataBuf, u32 u4Size)
{
	int i4Interrupt = 0;	//g_i4Interrupt;
	u32 timeout = 0xFFFF;
	struct scatterlist sg;
	enum dma_data_direction dir = DMA_TO_DEVICE;

    sg_init_one(&sg, pDataBuf, u4Size);
	dma_map_sg(&(mtd->dev), &sg, 1, dir);
	
	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
	DRV_Reg16(NFI_INTR_REG16);
	DRV_WriteReg16(NFI_INTR_EN_REG16, 0);
	// DRV_WriteReg32(NFI_STRADDR_REG32, (u32*)virt_to_phys(pDataBuf));
    
    if ((unsigned int)pDataBuf % 16)		// TODO: can not use AHB mode here
    {
        printk(KERN_INFO "Un-16-aligned address\n");
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
    }
    else
    {
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_DMA_BURST_EN);
    }
    
	if (i4Interrupt) 
    {
		init_completion(&g_comp_AHB_Done);
		DRV_Reg16(NFI_INTR_REG16);
		DRV_WriteReg16(NFI_INTR_EN_REG16, INTR_AHB_DONE_EN);
	}    
	//dmac_clean_range(pDataBuf, pDataBuf + u4Size);
	mb();
	NFI_SET_REG16(NFI_CON_REG16, CON_NFI_BWR);
    
	if (i4Interrupt) 
    {
	    if (!wait_for_completion_timeout(&g_comp_AHB_Done, 2))
        {
            printk(KERN_ERR "[%s]: %d wait for completion timeout happened \n", __FUNCTION__, __LINE__);
            dump_nfi();
            return false;
        }
		// wait_for_completion(&g_comp_AHB_Done);
	} 
    else 
    {
        while ( (u4Size >> 9) > ((DRV_Reg16(NFI_BYTELEN_REG16) & 0xf000) >> 12) )
        {
			timeout--;
			if (0 == timeout)
            {
				printk(KERN_ERR "[%s]: %d Poll BYTELEN error\n", __FUNCTION__, __LINE__);
				dump_nfi();
				return false;// AHB Mode Time Out!
			}
		}
	}	
	
	dma_unmap_sg(&(mtd->dev), &sg, 1, dir);

    return true;
}

static bool mt6575_nand_mcu_write_data(struct mtd_info *mtd, const u8 *buf, u32 length)
{
	u32 timeout = 0xFFFF;
	u32 i;	
	u32* pBuf32;
	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
	mb();
	NFI_SET_REG16(NFI_CON_REG16, CON_NFI_BWR);
	pBuf32 = (u32*)buf;
    
    if ((u32)buf % 4 || length % 4)
	    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);
    else
        NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);

    if ((u32)buf % 4 || length % 4)
    {
        for (i = 0; (i < (length))&&(timeout > 0);)
        {
            if (DRV_Reg16(NFI_PIO_DIRDY_REG16) & 1)
            {
			    DRV_WriteReg32(NFI_DATAW_REG32, *buf++);
    			i++;
    		} 
            else 
            {
    			timeout--;
    		}
    		if (0 == timeout)
            {
				printk(KERN_ERR "[%s]: %d timeout!\n", __FUNCTION__, __LINE__);
				dump_nfi();
				return false;
    		}
    	}
    }
    else
    {
        for (i = 0; (i < (length >> 2)) && (timeout > 0); )
        {
		    // if (FIFO_WR_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)) <= 12)
            if (DRV_Reg16(NFI_PIO_DIRDY_REG16) & 1)
        {
			DRV_WriteReg32(NFI_DATAW_REG32, *pBuf32++);
			i++;
		} 
        else 
        {
			timeout--;
		}
		if (0 == timeout)
        {
			printk(KERN_ERR "[%s]: %d timeout!\n", __FUNCTION__, __LINE__);
			dump_nfi();
			return false;				
		}
	}
    }

	return true;
}


static bool mt6575_nand_write_page_data(struct mtd_info *mtd, u8* buf, u32 size)
{
#if (__INTERNAL_USE_AHB_MODE__)
    return mt6575_nand_dma_write_data(mtd, buf, size);
#else
    return mt6575_nand_mcu_write_data(mtd, buf, size);
#endif
}

static void mt6575_nand_read_fdm_data(u8* pDataBuf, u32 u4SecNum)
{
	u32 i;
	u32* pBuf32 = (u32*)pDataBuf;

    if (pBuf32)
    {
    	for (i = 0; i < u4SecNum; ++i)
    	{
    		*pBuf32++ = DRV_Reg32(NFI_FDM0L_REG32 + (i<<1));
    		*pBuf32++ = DRV_Reg32(NFI_FDM0M_REG32 + (i<<1));
    		//*pBuf32++ = DRV_Reg32((u32)NFI_FDM0L_REG32 + (i<<3));
    		//*pBuf32++ = DRV_Reg32((u32)NFI_FDM0M_REG32 + (i<<3));
    	}
    }
}

static u8 fdm_buf[64];
static void mt6575_nand_write_fdm_data(struct nand_chip *chip, u8* pDataBuf, u32 u4SecNum)
{
	u32 i, j;
    u8 checksum = 0;
    bool empty = true;
    struct nand_oobfree *free_entry;
    u32* pBuf32;

    memcpy(fdm_buf, pDataBuf, u4SecNum * 8);

    free_entry = chip->ecc.layout->oobfree;
    for (i = 0; i < MTD_MAX_OOBFREE_ENTRIES && free_entry[i].length; i++)
    {
        for (j = 0; j < free_entry[i].length; j++)
        {
            if (pDataBuf[free_entry[i].offset + j] != 0xFF)
                empty = false;
            checksum ^= pDataBuf[free_entry[i].offset + j];
        }
    }

    if (!empty)
    {
        fdm_buf[free_entry[i-1].offset + free_entry[i-1].length] = checksum;
    }

	
    pBuf32 = (u32*)fdm_buf;
	for (i = 0; i < u4SecNum; ++i)
	{
		DRV_WriteReg32(NFI_FDM0L_REG32 + (i<<1), *pBuf32++);
		DRV_WriteReg32(NFI_FDM0M_REG32 + (i<<1), *pBuf32++);
		//DRV_WriteReg32((u32)NFI_FDM0L_REG32 + (i<<3), *pBuf32++);
		//DRV_WriteReg32((u32)NFI_FDM0M_REG32 + (i<<3), *pBuf32++);
	}
}

static void mt6575_nand_stop_read(void)
{
	NFI_CLN_REG16(NFI_CON_REG16, CON_NFI_BRD);
    mt6575_nand_reset();
	if(g_bHwEcc){
		ECC_Decode_End();
	}
    DRV_WriteReg16(NFI_INTR_EN_REG16, 0);
}

static void mt6575_nand_stop_write(void)
{
	NFI_CLN_REG16(NFI_CON_REG16, CON_NFI_BWR);
	if(g_bHwEcc){
		ECC_Encode_End();
	}
    DRV_WriteReg16(NFI_INTR_EN_REG16, 0);
}

bool mt6575_nand_exec_read_page(
	struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize, u8* pPageBuf, u8* pFDMBuf)
{
    u8 *buf;
	bool bRet = true;
	struct nand_chip *nand = mtd->priv;
	u32 u4SecNum = u4PageSize >> 9;
#ifdef NAND_PFM
	struct timeval pfm_time_read;
#endif

//#ifdef NAND_MT6575_E1_CHIP
	unsigned short PageFmt_Reg = 0;
	unsigned int NAND_ECC_Enc_Reg = 0;
	unsigned int NAND_ECC_Dec_Reg = 0; 
//#endif
	PFM_BEGIN(pfm_time_read);
	

    if (((u32)pPageBuf % 16) && local_buffer_16_align)
	{
        // printk(KERN_INFO "Data buffer not 16 bytes aligned: %p\n", pPageBuf);
        buf = local_buffer_16_align;
    }
    else
        buf = pPageBuf;

//#ifdef NAND_MT6575_E1_CHIP
    if(get_chip_eco_ver() == CHIP_E1) {
	if(((u4RowAddr*u4PageSize)>=g_exist_Partition[0].offset) && ((u4RowAddr*u4PageSize)<(g_exist_Partition[1].offset+g_exist_Partition[1].size)))
	{	
		u32 u4ENCODESize;
		u32 u4DECODESize;
    	u4ENCODESize = (512 + 3) << 3;
		u4DECODESize = ((512 + 3) << 3) + 8 * 13; 

		//Save FMT register
		PageFmt_Reg = DRV_Reg16(NFI_PAGEFMT_REG16);
		//NFI_CLN_REG16(NFI_PAGEFMT_REG16, PAGEFMT_FDM_MASK | PAGEFMT_FDM_ECC_MASK);
		//NFI_SET_REG16(NFI_PAGEFMT_REG16, 3 << PAGEFMT_FDM_SHIFT);
		//NFI_SET_REG16(NFI_PAGEFMT_REG16, 3 << PAGEFMT_FDM_ECC_SHIFT);	
		mt6575_nand_configure_fdm(3);
		
		//Save ECC register 
		NAND_ECC_Enc_Reg = DRV_Reg32(ECC_ENCCNFG_REG32);
		NAND_ECC_Dec_Reg = DRV_Reg32(ECC_DECCNFG_REG32);
		DRV_WriteReg32(ECC_DECCNFG_REG32,
			ECC_CNFG_ECC8|DEC_CNFG_NFI|DEC_CNFG_EMPTY_EN|
			(u4DECODESize << DEC_CNFG_CODE_SHIFT));
		DRV_WriteReg32(ECC_ENCCNFG_REG32, 
			ECC_CNFG_ECC8|ENC_CNFG_NFI|
			(u4ENCODESize << ENC_CNFG_MSG_SHIFT));
#ifdef MANUAL_CORRECT
		NFI_SET_REG32(ECC_DECCNFG_REG32, DEC_CNFG_CORRECT);
#else
		NFI_SET_REG32(ECC_DECCNFG_REG32, DEC_CNFG_EL);
#endif
	printk(KERN_ERR "-----Preloader,DSP_BL read\n");

	}
//#endif
    }
	if (mt6575_nand_ready_for_read(nand, u4RowAddr, 0, true, buf))
	{
		if (!mt6575_nand_read_page_data(mtd, buf, u4PageSize))
		{
			bRet = false;
		}
        
		if (!mt6575_nand_status_ready(STA_NAND_BUSY))
		{
			bRet = false;
		}
		if(g_bHwEcc){
			if(!mt6575_nand_check_dececc_done(u4SecNum)){
	            bRet = false;
    		}
		}
		mt6575_nand_read_fdm_data(pFDMBuf, u4SecNum);

		if(g_bHwEcc){     
		if (!mt6575_nand_check_bch_error(mtd, buf, u4SecNum - 1, u4RowAddr))
		{
			bRet = false;
		}
        }
		mt6575_nand_stop_read();
	}
//#ifdef NAND_MT6575_E1_CHIP
    if(get_chip_eco_ver() == CHIP_E1) {
	if(((u4RowAddr*u4PageSize)>=g_exist_Partition[0].offset) && ((u4RowAddr*u4PageSize)<(g_exist_Partition[1].offset+g_exist_Partition[1].size)))	
	{
		DRV_WriteReg16(NFI_PAGEFMT_REG16,PageFmt_Reg);
		DRV_WriteReg32(ECC_DECCNFG_REG32,NAND_ECC_Dec_Reg);
		DRV_WriteReg32(ECC_ENCCNFG_REG32,NAND_ECC_Enc_Reg);
	}
//#endif
    }
    if (buf == local_buffer_16_align)
        memcpy(pPageBuf, buf, u4PageSize);

	PFM_END_R(pfm_time_read, u4PageSize + 32);
	return bRet;
}

int mt6575_nand_exec_write_page(
	struct mtd_info *mtd, u32 u4RowAddr, u32 u4PageSize, u8* pPageBuf, u8* pFDMBuf)
{
	struct nand_chip *chip = mtd->priv;
	u32 u4SecNum = u4PageSize >> 9;
    u8 *buf;
    u8 status;

    MSG(WRITE, "mt6575_nand_exec_write_page, page: 0x%x\n", u4RowAddr);

#ifdef _MTK_NAND_DUMMY_DRIVER_
    if (dummy_driver_debug)
    {
	    unsigned long long time = sched_clock();
        if (!((time * 123 + 59 ) % 32768))
        {
            printk(KERN_INFO "[NAND_DUMMY_DRIVER] Simulate write error at page: 0x%x\n", u4RowAddr);
            return -EIO;
        }
    }
#endif
    
#ifdef NAND_PFM
	struct timeval pfm_time_write;
#endif
	PFM_BEGIN(pfm_time_write);
    if (((u32)pPageBuf % 16) && local_buffer_16_align)
    {
        printk(KERN_INFO "Data buffer not 16 bytes aligned: %p\n", pPageBuf);
        memcpy(local_buffer_16_align, pPageBuf, mtd->writesize);
        buf = local_buffer_16_align;
    }
    else
        buf = pPageBuf;

    if (mt6575_nand_ready_for_write(chip, u4RowAddr, 0, true, buf))
	{
		mt6575_nand_write_fdm_data(chip, pFDMBuf, u4SecNum);
		(void)mt6575_nand_write_page_data(mtd, buf, u4PageSize);
		(void)mt6575_nand_check_RW_count(u4PageSize);
		mt6575_nand_stop_write();
        (void)mt6575_nand_set_command(NAND_CMD_PAGEPROG);
		while(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY);		
	}
	PFM_END_W(pfm_time_write, u4PageSize + 32);

    status = chip->waitfunc(mtd, chip);
    if (status & NAND_STATUS_FAIL)
        return -EIO;
    else
        return 0;
}


static int mt6575_nand_write_page(struct mtd_info *mtd, struct nand_chip *chip, 
        const u8 *buf, int page, int cached, int raw)
{
    int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
    int block = page / page_per_block;
    u16 page_in_block = page % page_per_block;
    int mapped_block = get_mapping_block_index(block);

    // write bad index into oob
    if (mapped_block != block)
    {
        set_bad_index_to_oob(chip->oob_poi, block);
    }
    else 
    {
        set_bad_index_to_oob(chip->oob_poi, FAKE_INDEX);
    }

    if (mt6575_nand_exec_write_page(mtd, page_in_block + mapped_block * page_per_block, mtd->writesize, (u8 *)buf, chip->oob_poi))
    {
        MSG(INIT, "write fail at block: 0x%x, page: 0x%x\n", mapped_block, page_in_block);
        if (update_bmt((page_in_block + mapped_block * page_per_block) << chip->page_shift, 
                    UPDATE_WRITE_FAIL, (u8 *)buf, chip->oob_poi))
        {
            MSG(INIT, "Update BMT success\n");
            return 0;
        }
        else
        {
            MSG(INIT, "Update BMT fail\n");
            return -EIO;
        }
    }

    return 0;
}


//-------------------------------------------------------------------------------

static void mt6575_nand_command_bp(struct mtd_info *mtd, unsigned int command,
			 int column, int page_addr)
{
	struct nand_chip* nand = mtd->priv;
#ifdef NAND_PFM
	struct timeval pfm_time_erase;
#endif
    switch (command) 
    {
        case NAND_CMD_SEQIN:
		    /* Reset g_kCMD */
		//if (g_kCMD.u4RowAddr != page_addr) {
			memset(g_kCMD.au1OOB, 0xFF, sizeof(g_kCMD.au1OOB));
			g_kCMD.pDataBuf = NULL;
        //}
		    g_kCMD.u4RowAddr = page_addr;
		    g_kCMD.u4ColAddr = column;
            break;

        case NAND_CMD_PAGEPROG:
           	if (g_kCMD.pDataBuf || (0xFF != g_kCMD.au1OOB[0])) 
    		{
           		u8* pDataBuf = g_kCMD.pDataBuf ? g_kCMD.pDataBuf : nand->buffers->databuf;
    			mt6575_nand_exec_write_page(mtd, g_kCMD.u4RowAddr, mtd->writesize, pDataBuf, g_kCMD.au1OOB);
    			g_kCMD.u4RowAddr = (u32)-1;
    			g_kCMD.u4OOBRowAddr = (u32)-1;
            }
            break;

        case NAND_CMD_READOOB:
    		g_kCMD.u4RowAddr = page_addr;        	
    		g_kCMD.u4ColAddr = column + mtd->writesize;
    		#ifdef NAND_PFM
    		g_kCMD.pureReadOOB = 1;
    		g_kCMD.pureReadOOBNum += 1;
    		#endif
			break;
			
        case NAND_CMD_READ0:
    		g_kCMD.u4RowAddr = page_addr;        	
    		g_kCMD.u4ColAddr = column;
    		#ifdef NAND_PFM
    		g_kCMD.pureReadOOB = 0;
    		#endif		
			break;

        case NAND_CMD_ERASE1:
    		PFM_BEGIN(pfm_time_erase);
    		(void)mt6575_nand_reset();
            mt6575_nand_set_mode(CNFG_OP_ERASE);
    		(void)mt6575_nand_set_command(NAND_CMD_ERASE1);
    		(void)mt6575_nand_set_address(0,page_addr,0,devinfo.addr_cycle-2);
            break;
            
        case NAND_CMD_ERASE2:
       	    (void)mt6575_nand_set_command(NAND_CMD_ERASE2);
			while(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY);
		    PFM_END_E(pfm_time_erase);
            break;
            
        case NAND_CMD_STATUS:
            (void)mt6575_nand_reset(); 
            NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_BYTE_RW);		           	
			mt6575_nand_set_mode(CNFG_OP_SRD);
            mt6575_nand_set_mode(CNFG_READ_EN);
            NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
            NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
		    (void)mt6575_nand_set_command(NAND_CMD_STATUS);
        	NFI_CLN_REG16(NFI_CON_REG16, CON_NFI_NOB_MASK);
        	mb();
			DRV_WriteReg16(NFI_CON_REG16, CON_NFI_SRD|(1 << CON_NFI_NOB_SHIFT));
            g_bcmdstatus = true;            
            break;
            
        case NAND_CMD_RESET:
       	    (void)mt6575_nand_reset();
			//mt6575_nand_exec_reset_device();
            break;

		case NAND_CMD_READID: 
            /* Issue NAND chip reset command */
            //NFI_ISSUE_COMMAND (NAND_CMD_RESET, 0, 0, 0, 0);
            
            //timeout = TIMEOUT_4;
            
            //while (timeout)
            //timeout--;
            
            mt6575_nand_reset();
            /* Disable HW ECC */
            NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
            NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
    
            /* Disable 16-bit I/O */
            //NFI_CLN_REG16(NFI_PAGEFMT_REG16, PAGEFMT_DBYTE_EN);
		
			NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN|CNFG_BYTE_RW);
		    (void)mt6575_nand_reset();
			mb();
			mt6575_nand_set_mode(CNFG_OP_SRD);
		    (void)mt6575_nand_set_command(NAND_CMD_READID);
		    (void)mt6575_nand_set_address(0,0,1,0);
			DRV_WriteReg16(NFI_CON_REG16, CON_NFI_SRD);
			while(DRV_Reg32(NFI_STA_REG32) & STA_DATAR_STATE);
			break;
            
        default:
            BUG();        
            break;
    }
 }

static void mt6575_nand_select_chip(struct mtd_info *mtd, int chip)
{
	if (chip == -1 && false == g_bInitDone)
	{
		struct nand_chip *nand = mtd->priv;
		struct mt6575_nand_host *host = nand->priv;
		struct mt6575_nand_host_hw *hw = host->hw;
		u32 spare_per_sector = mtd->oobsize/( mtd->writesize/512);
		u32 ecc_bit = 4;
		u32 spare_bit = PAGEFMT_SPARE_16;
		
		if(spare_per_sector>=28){
  			spare_bit = PAGEFMT_SPARE_28;
    		ecc_bit = 12;
    		spare_per_sector = 28;
  	}else if(spare_per_sector>=27){
  		    spare_bit = PAGEFMT_SPARE_27;
    			ecc_bit = 8;
 					spare_per_sector = 27;
  	}else if(spare_per_sector>=26){
  		    spare_bit = PAGEFMT_SPARE_26;
    			ecc_bit = 8;
					spare_per_sector = 26;
  	}else if(spare_per_sector>=16){
  		    spare_bit = PAGEFMT_SPARE_16;
    			ecc_bit = 4;
					spare_per_sector = 16;
  	}else{
  		    MSG(INIT, "[NAND]: NFI not support oobsize: %x\n", spare_per_sector);
    			ASSERT(0);
  	}
  	 mtd->oobsize = spare_per_sector*(mtd->writesize/512);
  	 printk("[NAND]select ecc bit:%d, sparesize :%d\n",ecc_bit,mtd->oobsize);
		/* Setup PageFormat */
	if (4096 == mtd->writesize) {
       		NFI_SET_REG16(NFI_PAGEFMT_REG16, (spare_bit << PAGEFMT_SPARE_SHIFT) | PAGEFMT_4K);
                nand->cmdfunc = mt6575_nand_command_bp;		
	} else if (2048 == mtd->writesize) {
       		NFI_SET_REG16(NFI_PAGEFMT_REG16, (spare_bit << PAGEFMT_SPARE_SHIFT) | PAGEFMT_2K);
                nand->cmdfunc = mt6575_nand_command_bp;
        }/* else if (512 == mtd->writesize) {
       		NFI_SET_REG16(NFI_PAGEFMT_REG16, (PAGEFMT_SPARE_16 << PAGEFMT_SPARE_SHIFT) | PAGEFMT_512);
	       	nand->cmdfunc = mt6575_nand_command_sp;
    	}*/
    ECC_Config(hw,ecc_bit);
		g_bInitDone = true;
	}
    switch(chip)
    {
	case -1:
		break;
	case 0: 
	case 1:
            /*  Jun Shen, 2011.04.13  */
            /* Note: MT6575 EVB NAND  is mounted on CS0, but FPGA is CS1  */
            DRV_WriteReg16(NFI_CSEL_REG16, chip);            
            /*  Jun Shen, 2011.04.13 */
		break;
    }
}

static uint8_t mt6575_nand_read_byte(struct mtd_info *mtd)
{
#if 0
	//while(0 == FIFO_RD_REMAIN(DRV_Reg16(NFI_FIFOSTA_REG16)));
	/* Check the PIO bit is ready or not */
    u32 timeout = TIMEOUT_4;
    uint8_t retval = 0;
    WAIT_NFI_PIO_READY(timeout);   

    retval = DRV_Reg8(NFI_DATAR_REG32);
    MSG(INIT, "mt6575_nand_read_byte (0x%x)\n", retval);
    
    if(g_bcmdstatus)
    {
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
        g_bcmdstatus = false;
    }
    
    return retval;
#endif
    uint8_t retval = 0;
    
    if (!mt6575_nand_pio_ready())
    {
		printk(KERN_ERR "[%s]: %d PIO ready timeout\n", __FUNCTION__, __LINE__);
		dump_nfi();
		retval = false;
    }

    if(g_bcmdstatus)
    {
        retval = DRV_Reg8(NFI_DATAR_REG32);
        NFI_CLN_REG16(NFI_CON_REG16, CON_NFI_NOB_MASK); 
        mt6575_nand_reset();
#if (__INTERNAL_USE_AHB_MODE__)        
        NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);
#endif
	if(g_bHwEcc){
       	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	}else{
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	}
        g_bcmdstatus = false;
    }
    else
        retval = DRV_Reg8(NFI_DATAR_REG32);

    return retval;
}

static void mt6575_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct nand_chip* nand = (struct nand_chip*)mtd->priv;
	struct mt6575_CMD* pkCMD = &g_kCMD;
	u32 u4ColAddr = pkCMD->u4ColAddr;
	u32 u4PageSize = mtd->writesize;
		
	if (u4ColAddr < u4PageSize) 
	{
		if ((u4ColAddr == 0) && (len >= u4PageSize)) 
		{
			mt6575_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize, 
									   buf, pkCMD->au1OOB);
			if (len > u4PageSize) 
			{
				u32 u4Size = min(len - u4PageSize, sizeof(pkCMD->au1OOB));
				memcpy(buf + u4PageSize, pkCMD->au1OOB, u4Size);
			}
		} 
		else 
		{
			mt6575_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize, 
									   nand->buffers->databuf, pkCMD->au1OOB);
			memcpy(buf, nand->buffers->databuf + u4ColAddr, len);
		}
		pkCMD->u4OOBRowAddr = pkCMD->u4RowAddr;
	} 
	else 
	{
		u32 u4Offset = u4ColAddr - u4PageSize;
		u32 u4Size = min(len - u4Offset, sizeof(pkCMD->au1OOB));
		if (pkCMD->u4OOBRowAddr != pkCMD->u4RowAddr) 
		{
			mt6575_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize,
									   nand->buffers->databuf, pkCMD->au1OOB);
			pkCMD->u4OOBRowAddr = pkCMD->u4RowAddr;
		}
		memcpy(buf, pkCMD->au1OOB + u4Offset, u4Size);
	}
	pkCMD->u4ColAddr += len;	
}

static void mt6575_nand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	struct mt6575_CMD* pkCMD = &g_kCMD;
	u32 u4ColAddr = pkCMD->u4ColAddr;
	u32 u4PageSize = mtd->writesize;
    int i4Size, i;

	if (u4ColAddr >= u4PageSize) 
    {
	    u32 u4Offset = u4ColAddr - u4PageSize;
		u8* pOOB = pkCMD->au1OOB + u4Offset;
		i4Size = min(len, (int)(sizeof(pkCMD->au1OOB) - u4Offset));
        
		for (i = 0; i < i4Size; i++) 
        {
			pOOB[i] &= buf[i];
		}
	} 
    else 
    {
		pkCMD->pDataBuf = (u8*)buf;
    }
    
	pkCMD->u4ColAddr += len;	
}	

static void mt6575_nand_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf)
{
	mt6575_nand_write_buf(mtd, buf, mtd->writesize);
	mt6575_nand_write_buf(mtd, chip->oob_poi, mtd->oobsize);
}

static int mt6575_nand_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buf, int page)
{
#if 0
	mt6575_nand_read_buf(mtd, buf, mtd->writesize);
	mt6575_nand_read_buf(mtd, chip->oob_poi, mtd->oobsize);
#else
	struct mt6575_CMD* pkCMD = &g_kCMD;
	u32 u4ColAddr = pkCMD->u4ColAddr;
	u32 u4PageSize = mtd->writesize;
		
	if (u4ColAddr == 0) 
    {
        mt6575_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize, buf, chip->oob_poi);
        pkCMD->u4ColAddr += u4PageSize + mtd->oobsize;
	}
#endif
	return 0;
}

static int mt6575_nand_read_page(struct mtd_info *mtd, struct nand_chip *chip, u8 *buf, int page)
{
    int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
    int block = page / page_per_block;
    u16 page_in_block = page % page_per_block;
    int mapped_block = get_mapping_block_index(block);

    if (mt6575_nand_exec_read_page(mtd, page_in_block + mapped_block * page_per_block,
                mtd->writesize, buf, chip->oob_poi))
        return 0;
  /*  else
        return -EIO;*/
    return 0;
}

int mt6575_nand_erase_hw(struct mtd_info *mtd, int page)
{
    struct nand_chip *chip = (struct nand_chip *)mtd->priv;
    
#ifdef _MTK_NAND_DUMMY_DRIVER_
    if (dummy_driver_debug)
    {
	    unsigned long long time = sched_clock();
        if (!((time * 123 + 59 ) % 1024))
        {
            printk(KERN_INFO "[NAND_DUMMY_DRIVER] Simulate erase error at page: 0x%x\n", page);
            return NAND_STATUS_FAIL;
        }
    }
#endif

    chip->erase_cmd(mtd, page);

    return chip->waitfunc(mtd, chip);
}

static int mt6575_nand_erase(struct mtd_info *mtd, int page)
{
    // get mapping 
    struct nand_chip *chip = mtd->priv;
    int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
    int page_in_block = page % page_per_block;
    int block = page / page_per_block;
 
    int mapped_block = get_mapping_block_index(block);

    int status = mt6575_nand_erase_hw(mtd, page_in_block + page_per_block * mapped_block);

    if (status & NAND_STATUS_FAIL)
    {
        if (update_bmt( (page_in_block + mapped_block * page_per_block) << chip->page_shift, 
                    UPDATE_ERASE_FAIL, NULL, NULL))
        {
            MSG(INIT, "Erase fail at block: 0x%x, update BMT success\n", mapped_block);
            return 0;
        }
        else
        {
            MSG(INIT, "Erase fail at block: 0x%x, update BMT fail\n", mapped_block);
            return NAND_STATUS_FAIL;
        }
    }

    return 0;
}



#if 0
static int mt6575_nand_read_multi_page_cache(struct mtd_info *mtd, struct nand_chip *chip,
        int page, struct mtd_oob_ops *ops)
{
    int res = -EIO;
    int len = ops->len;
    struct mtd_ecc_stats stat = mtd->ecc_stats;
    uint8_t *buf = ops->datbuf;

    if (!mt6575_nand_ready_for_read(chip, page, 0, true, buf))
        return -EIO;

    while (len > 0)
    {
        mt6575_nand_set_mode(CNFG_OP_CUST);
        DRV_WriteReg16(NFI_CON_REG16, 8 << CON_NFI_SEC_SHIFT);

        if (len > mtd->writesize)               // remained more than one page
        {
            if (!mt6575_nand_set_command(0x31))      // todo: add cache read command
                goto ret;
        }
        else
        {
            if (!mt6575_nand_set_command(0x3f))      // last page remained
                goto ret;
        }

        mt6575_nand_status_ready(STA_NAND_BUSY);

#ifdef __INTERNAL_USE_AHB_MODE__
        //if (!mt6575_nand_dma_read_data(buf, mtd->writesize))
        if (!mt6575_nand_read_page_data(mtd, buf, mtd->writesize))
            goto ret;
#else
        if (!mt6575_nand_mcu_read_data(buf, mtd->writesize))
            goto ret;
#endif

        // get ecc error info
        mt6575_nand_check_bch_error(mtd, buf, 3, page);
        ECC_Decode_End();

        page++;
        len -= mtd->writesize;
        buf += mtd->writesize;
        ops->retlen += mtd->writesize;

        if (len > 0)
        {
            ECC_Decode_Start();
            mt6575_nand_reset();
        }

    }

    res = 0;

ret:
    mt6575_nand_stop_read();

    if (res)
        return res;

    if (mtd->ecc_stats.failed > stat.failed)
    {
        printk(KERN_INFO "ecc fail happened\n");
        return -EBADMSG;
    }

	return  mtd->ecc_stats.corrected - stat.corrected ? -EUCLEAN: 0;
}
#endif

static int mt6575_nand_read_oob_raw(struct mtd_info *mtd, uint8_t *buf, int page_addr, int len)
{
    struct nand_chip *chip = (struct nand_chip *)mtd->priv;
    u32 col_addr = 0;
    u32 sector = 0;
    int res = 0;
    u32 colnob=2, rawnob=devinfo.addr_cycle-2;
	int randomread =0;
    int read_len = 0;
    int sec_num = 1<<(chip->page_shift-9);
		int spare_per_sector = mtd->oobsize/sec_num;

    if (len > NAND_MAX_OOBSIZE || len % OOB_AVAI_PER_SECTOR || !buf)
    {
        printk(KERN_WARNING "[%s] invalid parameter, len: %d, buf: %p\n",
                __FUNCTION__, len, buf);
        return -EINVAL;
    }
    if(len>spare_per_sector)
    {
        randomread=1;
    }
	if(!randomread||!(devinfo.advancedmode & RAMDOM_READ))
	{
	    while (len > 0)
	    {
	        read_len = min(len, spare_per_sector);
	        col_addr = NAND_SECTOR_SIZE + sector * (NAND_SECTOR_SIZE + spare_per_sector); // TODO: Fix this hard-code 16
	        if (!mt6575_nand_ready_for_read(chip, page_addr, col_addr, false, NULL))
	        {
	            printk(KERN_WARNING "mt6575_nand_ready_for_read return failed\n");
	            res = -EIO;
	            goto error;
	        }
	        if (!mt6575_nand_mcu_read_data(buf + spare_per_sector * sector, read_len))    // TODO: and this 8
	        {
	            printk(KERN_WARNING "mt6575_nand_mcu_read_data return failed\n");
	            res = -EIO;
	            goto error;
	        }
	        mt6575_nand_stop_read();
			//dump_data(buf + 16 * sector,16);
	        sector++;
	        len -= read_len;
			
	    }
	}
    else  //should be 64
	{
	    col_addr = NAND_SECTOR_SIZE;
	    if (chip->options & NAND_BUSWIDTH_16)
	    {
	        col_addr /= 2;
	    }
		
		if (!mt6575_nand_reset())
	    {
			goto error;
		}

		mt6575_nand_set_mode(0x6000);
		NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
	    DRV_WriteReg16(NFI_CON_REG16, 4 << CON_NFI_SEC_SHIFT);


	    NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_AHB);
	    NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);


		mt6575_nand_set_autoformat(false);
		

		if (!mt6575_nand_set_command(NAND_CMD_READ0))
	    {
			goto error;
		}		

		//1 FIXED ME: For Any Kind of AddrCycle
		if (!mt6575_nand_set_address(col_addr, page_addr, colnob, rawnob))
	    {
			goto error;
		}

		if (!mt6575_nand_set_command(NAND_CMD_READSTART))
	    {
			goto error;
		}
		if (!mt6575_nand_status_ready(STA_NAND_BUSY))
	    {
			goto error;
		}
		

		read_len = min(len, spare_per_sector);
		if (!mt6575_nand_mcu_read_data(buf + spare_per_sector * sector, read_len))    // TODO: and this 8
		{
			printk(KERN_WARNING "mt6575_nand_mcu_read_data return failed first 16\n");
			res = -EIO;
			goto error;
		}
		sector++;
		len -= read_len;
		mt6575_nand_stop_read();
		while(len>0)
		{
		    read_len = min(len, spare_per_sector);
			if (!mt6575_nand_set_command(0x05))
			{
				goto error;
			}

			col_addr =  NAND_SECTOR_SIZE + sector * (NAND_SECTOR_SIZE + spare_per_sector);
			if (chip->options & NAND_BUSWIDTH_16)
		    {
		        col_addr /= 2;
		    }
			DRV_WriteReg32(NFI_COLADDR_REG32, col_addr);
			DRV_WriteReg16(NFI_ADDRNOB_REG16, 2);
	        DRV_WriteReg16(NFI_CON_REG16, 4 << CON_NFI_SEC_SHIFT);

			if( !mt6575_nand_status_ready(STA_ADDR_STATE))
			{
				goto error;
			}

			if (!mt6575_nand_set_command(0xE0))
		    {
				goto error;
			}
			if (!mt6575_nand_status_ready(STA_NAND_BUSY))
		    {
				goto error;
			}
			if (!mt6575_nand_mcu_read_data(buf + spare_per_sector * sector, read_len))    // TODO: and this 8
			{
				printk(KERN_WARNING "mt6575_nand_mcu_read_data return failed first 16\n");
				res = -EIO;
				goto error;
			}
			mt6575_nand_stop_read();
			sector++;
			len -= read_len;
		}
		//dump_data(&testbuf[16],16);
		//printk(KERN_ERR "\n");
	}
error:
    NFI_CLN_REG16(NFI_CON_REG16, CON_NFI_BRD);
    return res;
}

static int mt6575_nand_write_oob_raw(struct mtd_info *mtd, const uint8_t *buf, int page_addr, int len)
{
    struct nand_chip *chip = mtd->priv;
    // int i;
    u32 col_addr = 0;
    u32 sector = 0;
    // int res = 0;
    // u32 colnob=2, rawnob=devinfo.addr_cycle-2;
	// int randomread =0;
    int write_len = 0;
    int status;
    int sec_num = 1<<(chip->page_shift-9);
		int spare_per_sector = mtd->oobsize/sec_num;

    if (len > NAND_MAX_OOBSIZE || len % OOB_AVAI_PER_SECTOR || !buf)
    {
        printk(KERN_WARNING "[%s] invalid parameter, len: %d, buf: %p\n",
                __FUNCTION__, len, buf);
        return -EINVAL;
    }

    while (len > 0)
    {
        write_len = min(len, spare_per_sector);
        col_addr = sector * (NAND_SECTOR_SIZE + spare_per_sector) + NAND_SECTOR_SIZE;
        if (!mt6575_nand_ready_for_write(chip, page_addr, col_addr, false, NULL))
        {
            return -EIO;
        }
        
        if (!mt6575_nand_mcu_write_data(mtd, buf + sector * spare_per_sector, write_len))
        {
            return -EIO;
        }
        
		(void)mt6575_nand_check_RW_count(write_len);
	    NFI_CLN_REG16(NFI_CON_REG16, CON_NFI_BWR);
        (void)mt6575_nand_set_command(NAND_CMD_PAGEPROG);
		
        while(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY);		
	
        status = chip->waitfunc(mtd, chip);
        if (status & NAND_STATUS_FAIL)
        {
            printk(KERN_INFO "status: %d\n", status);
            return -EIO;
        }

        len -= write_len;
        sector++;
    }

    return 0;
}

static int mt6575_nand_write_oob_hw(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
    // u8 *buf = chip->oob_poi;
    int i, iter;
 
    int sec_num = 1<<(chip->page_shift-9);
   
		int spare_per_sector = mtd->oobsize/sec_num;
    memcpy(local_oob_buf, chip->oob_poi, mtd->oobsize);

    // copy ecc data
    for (i = 0; i < chip->ecc.layout->eccbytes; i++)
    {
        iter = (i / OOB_AVAI_PER_SECTOR) * spare_per_sector + OOB_AVAI_PER_SECTOR + i % OOB_AVAI_PER_SECTOR;
        local_oob_buf[iter] = chip->oob_poi[chip->ecc.layout->eccpos[i]];
        // chip->oob_poi[chip->ecc.layout->eccpos[i]] = local_oob_buf[iter];
    }

    // copy FDM data
    for (i = 0; i < sec_num; i++)
    {
        memcpy(&local_oob_buf[i * spare_per_sector], &chip->oob_poi[i * OOB_AVAI_PER_SECTOR], OOB_AVAI_PER_SECTOR);
    }

    return mt6575_nand_write_oob_raw(mtd, local_oob_buf, page, mtd->oobsize);
}

static int mt6575_nand_write_oob(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
    int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
    int block = page / page_per_block;
    u16 page_in_block = page % page_per_block;
    int mapped_block = get_mapping_block_index(block);

    // write bad index into oob
    if (mapped_block != block)
    {
        set_bad_index_to_oob(chip->oob_poi, block);
    }
    else 
    {
        set_bad_index_to_oob(chip->oob_poi, FAKE_INDEX);
    }

    if (mt6575_nand_write_oob_hw(mtd, chip, page_in_block + mapped_block * page_per_block /* page */))
    {
        MSG(INIT, "write oob fail at block: 0x%x, page: 0x%x\n", mapped_block, page_in_block);
        if (update_bmt((page_in_block + mapped_block * page_per_block) << chip->page_shift, 
                    UPDATE_WRITE_FAIL, NULL, chip->oob_poi))
        {
            MSG(INIT, "Update BMT success\n");
            return 0;
        }
        else
        {
            MSG(INIT, "Update BMT fail\n");
            return -EIO;
        }
    }

    return 0;
}

int mt6575_nand_block_markbad_hw(struct mtd_info *mtd, loff_t offset)
{
    struct nand_chip *chip = mtd->priv;
    int block = (int)offset >> chip->phys_erase_shift;
    int page = block * (1 << (chip->phys_erase_shift - chip->page_shift));
    int ret;

    u8 buf[8];
    memset(buf, 0xFF, 8);
    buf[0] = 0;

    ret = mt6575_nand_write_oob_raw(mtd, buf, page, 8);
    return ret;
}

static int mt6575_nand_block_markbad(struct mtd_info *mtd, loff_t offset)
{
    struct nand_chip *chip = mtd->priv;
    int block = (int)offset >> chip->phys_erase_shift;
    int mapped_block;
    int ret;

    nand_get_device(chip, mtd, FL_WRITING);

    mapped_block = get_mapping_block_index(block);
    ret = mt6575_nand_block_markbad_hw(mtd, mapped_block << chip->phys_erase_shift);

    nand_release_device(mtd);

    return ret;
}

int mt6575_nand_read_oob_hw(struct mtd_info *mtd,struct nand_chip *chip, int page)
{
    int i;
    u8 iter = 0;
    
    int sec_num = 1<<(chip->page_shift-9);
		int spare_per_sector = mtd->oobsize/sec_num;
#ifdef TESTTIME
	unsigned long long time1,time2;

	time1 = sched_clock();
#endif
    
    if (mt6575_nand_read_oob_raw(mtd, chip->oob_poi, page, mtd->oobsize))
    {
        // printk(KERN_ERR "[%s]mt6575_nand_read_oob_raw return failed\n", __FUNCTION__);
        return -EIO;
    }
#ifdef TESTTIME
    time2= sched_clock()-time1;
	if(!readoobflag)
	{  
	   readoobflag=1;
	   printk(KERN_ERR "[%s] time is %llu",__FUNCTION__,time2);
	}
#endif	

    // adjust to ecc physical layout to memory layout
    /*********************************************************/
    /* FDM0 | ECC0 | FDM1 | ECC1 | FDM2 | ECC2 | FDM3 | ECC3 */
    /*  8B  |  8B  |  8B  |  8B  |  8B  |  8B  |  8B  |  8B  */
    /*********************************************************/
    
    memcpy(local_oob_buf, chip->oob_poi, mtd->oobsize);

    // copy ecc data
    for (i = 0; i < chip->ecc.layout->eccbytes; i++)
    {
        iter = (i / OOB_AVAI_PER_SECTOR) * spare_per_sector + OOB_AVAI_PER_SECTOR + i % OOB_AVAI_PER_SECTOR;
        chip->oob_poi[chip->ecc.layout->eccpos[i]] = local_oob_buf[iter];
    }

    // copy FDM data
    for (i = 0; i < sec_num; i++)
    {
        memcpy(&chip->oob_poi[i * OOB_AVAI_PER_SECTOR], &local_oob_buf[i * spare_per_sector], OOB_AVAI_PER_SECTOR);
    }

    return 0;
}

static int mt6575_nand_read_oob(struct mtd_info *mtd,struct nand_chip *chip, int page, int sndcmd)
{
    int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);
    int block = page / page_per_block;
    u16 page_in_block = page % page_per_block;
    int mapped_block = get_mapping_block_index(block);

    mt6575_nand_read_oob_hw(mtd, chip, page_in_block + mapped_block * page_per_block);

    return 0;       // the return value is sndcmd
}


int mt6575_nand_block_bad_hw(struct mtd_info *mtd, loff_t ofs)
{
    struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	int page_addr = (int)(ofs >> chip->page_shift);
    unsigned int page_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);


    unsigned char oob_buf[8];
    page_addr &= ~(page_per_block - 1);

    if (mt6575_nand_read_oob_raw(mtd, oob_buf, page_addr, sizeof(oob_buf)))
    {
        printk(KERN_WARNING "mt6575_nand_read_oob_raw return error\n");
        return 1;
    }
    
    if (oob_buf[0] != 0xff)
    {
        printk(KERN_WARNING "Bad block detected at 0x%x, oob_buf[0] is 0x%x\n", page_addr, oob_buf[0]);
        // dump_nfi();
        return 1;
    }

    return 0;        // everything is OK, good block
}


static int mt6575_nand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{
    int chipnr = 0;
    
    struct nand_chip *chip = (struct nand_chip *)mtd->priv;
    int block = (int)ofs >> chip->phys_erase_shift;
    int mapped_block;

    int ret;

    if (getchip) {
		chipnr = (int)(ofs >> chip->chip_shift);
		nand_get_device(chip, mtd, FL_READING);
		/* Select the NAND device */
		chip->select_chip(mtd, chipnr);
	}

    mapped_block = get_mapping_block_index(block);

    ret = mt6575_nand_block_bad_hw(mtd, mapped_block << chip->phys_erase_shift);

    if (ret)
    {
        MSG(INIT, "Unmapped bad block: 0x%x\n", mapped_block);
        if (update_bmt(mapped_block << chip->phys_erase_shift, UPDATE_UNMAPPED_BLOCK, NULL, NULL))
        {
            MSG(INIT, "Update BMT success\n");
            ret = 0;
        }
        else
        {
            MSG(INIT, "Update BMT fail\n");
            ret = 1;
        }
    }

    if (getchip)
    {
        nand_release_device(mtd);
    }

    return ret;
}


#ifdef CONFIG_MTD_NAND_VERIFY_WRITE

char gacBuf[4096 + 128];

static int mt6575_nand_verify_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
#if 1
	struct nand_chip* chip = (struct nand_chip*)mtd->priv;
	struct mt6575_CMD* pkCMD = &g_kCMD;
	u32 u4PageSize = mtd->writesize;
	u32 *pSrc, *pDst;
	int i;

    mt6575_nand_exec_read_page(mtd, pkCMD->u4RowAddr, u4PageSize, gacBuf, gacBuf + u4PageSize);

	pSrc = (u32*)buf;
	pDst = (u32*)gacBuf;
	len = len/sizeof(u32);
	for (i = 0; i < len; ++i) 
    {
		if (*pSrc != *pDst) 
        {
			MSG(VERIFY, "mt6575_nand_verify_buf page fail at page %d\n", pkCMD->u4RowAddr);
            return -1;
		}
		pSrc++;
		pDst++;
	}
    
	pSrc = (u32*)chip->oob_poi;
	pDst = (u32*)(gacBuf + u4PageSize);
    
	if ((pSrc[0] != pDst[0]) || (pSrc[1] != pDst[1]) ||
	    (pSrc[2] != pDst[2]) || (pSrc[3] != pDst[3]) ||
	    (pSrc[4] != pDst[4]) || (pSrc[5] != pDst[5]))
	    // TODO: Ask Designer Why?
	    //(pSrc[6] != pDst[6]) || (pSrc[7] != pDst[7])) 
    {
        MSG(VERIFY, "mt6575_nand_verify_buf oob fail at page %d\n", pkCMD->u4RowAddr);
		MSG(VERIFY, "0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
		    pSrc[0], pSrc[1], pSrc[2], pSrc[3], pSrc[4], pSrc[5], pSrc[6], pSrc[7]);
		MSG(VERIFY, "0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
		    pDst[0], pDst[1], pDst[2], pDst[3], pDst[4], pDst[5], pDst[6], pDst[7]);
		return -1;		
    }
	/*
	for (i = 0; i < len; ++i) {
		if (*pSrc != *pDst) {
			printk(KERN_ERR"mt6575_nand_verify_buf oob fail at page %d\n", g_kCMD.u4RowAddr);
			return -1;
		}
		pSrc++;
		pDst++;
	}
	*/
	//printk(KERN_INFO"mt6575_nand_verify_buf OK at page %d\n", g_kCMD.u4RowAddr);
	
	return 0;
#else
    return 0;
#endif
}
#endif

static void mt6575_nand_init_hw(struct mt6575_nand_host *host)
{
	struct mt6575_nand_host_hw *hw = host->hw;
	

    MSG(INIT, "Enable NFI Clock\n");
    nand_enable_clock();

	g_bInitDone = false;
    /* Get the HW_VER */
    //g_u4ChipVer = DRV_Reg32(CONFIG_BASE);
	g_kCMD.u4OOBRowAddr  = (u32)-1;

    /* Set default NFI access timing control */
	DRV_WriteReg32(NFI_ACCCON_REG32, hw->nfi_access_timing);
	DRV_WriteReg16(NFI_CNFG_REG16, 0);
	DRV_WriteReg16(NFI_PAGEFMT_REG16, 0);
	
    /* Reset the state machine and data FIFO, because flushing FIFO */
	(void)mt6575_nand_reset();
	
    /* Set the ECC engine */
    if(hw->nand_ecc_mode == NAND_ECC_HW)
	{
		MSG(INIT, "%s : Use HW ECC\n", MODULE_NAME);
		if(g_bHwEcc){
			NFI_SET_REG32(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
		}
		ECC_Config(host->hw,4);
   		mt6575_nand_configure_fdm(8);
		mt6575_nand_configure_lock();
	}

	/* Initilize interrupt. Clear interrupt, read clear. */
    DRV_Reg16(NFI_INTR_REG16);
	
    /* Interrupt arise when read data or program data to/from AHB is done. */
	DRV_WriteReg16(NFI_INTR_EN_REG16, 0);
}

//-------------------------------------------------------------------------------
static int mt6575_nand_dev_ready(struct mtd_info *mtd)
{	
    return !(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY);
}

static int mt6575_nand_proc_read(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
    int len;
	if (off > 0) 
    {
		return 0;
	}
	// return sprintf(page, "Interrupt-Scheme is %d\n", g_i4Interrupt);
    len = sprintf(page, "ID: 0x%x, total size: %dMiB\n", devinfo.id, devinfo.totalsize);
    len += sprintf(page + len, "Current working in %s mode\n", g_i4Interrupt ? "interrupt" : "polling");

    return len;
}

static int mt6575_nand_proc_write(struct file* file, const char* buffer,
	unsigned long count, void *data)
{
    struct mtd_info *mtd = &host->mtd;
	char buf[16];
	int len = count, n;
	
	if (len >= sizeof(buf)) 
    {
		len = sizeof(buf) - 1;
	}

	if (copy_from_user(buf, buffer, len)) 
    {
		return -EFAULT;
	}

	buf[len] = '\0';
	if (buf[0] == 'I') 
    {
        // sync before switching between polling and interrupt, 
		n = simple_strtol(buf+1, NULL, 10);
 
        if ((n > 0 && !g_i4Interrupt) || 
            (n == 0 && g_i4Interrupt))
        {
            nand_get_device((struct nand_chip *)mtd->priv, mtd, FL_READING);

            g_i4Interrupt = n;
            
            if (g_i4Interrupt)
			{
				DRV_Reg16(NFI_INTR_REG16);
                enable_irq(MT6575_NFI_IRQ_ID);
            }
            else
                disable_irq(MT6575_NFI_IRQ_ID);
            
            nand_release_device(mtd);
    	} 
    } 

    if (buf[0] == 'D')
    {
#ifdef _MTK_NAND_DUMMY_DRIVER_
        printk(KERN_INFO "Enable dummy driver\n");
        dummy_driver_debug = 1;
#endif
    } 
	
#ifdef NAND_PFM
	if (buf[0] == 'P') 
    {
        /* Reset values */
		g_PFM_R = 0;
		g_PFM_W = 0;
		g_PFM_E = 0;
		g_PFM_RD = 0;
		g_PFM_WD = 0;
		g_kCMD.pureReadOOBNum = 0;
	}
#endif

	return len;
}


static int mt6575_nand_probe(struct platform_device *pdev)
{
	
	struct mt6575_nand_host_hw *hw;	
	struct mtd_info *mtd;
	struct nand_chip *nand_chip;
    struct resource *res = pdev->resource;	
	int err = 0;
    int id;
    u32 ext_id;
    u8 ext_id1, ext_id2, ext_id3;
    int i;
    hw = (struct mt6575_nand_host_hw*)pdev->dev.platform_data;
    BUG_ON(!hw);

	if (pdev->num_resources != 4 ||
	    res[0].flags != IORESOURCE_MEM || 
	    res[1].flags != IORESOURCE_MEM ||
	    res[2].flags != IORESOURCE_IRQ ||
   	    res[3].flags != IORESOURCE_IRQ)
   	{
		MSG(INIT, "%s: invalid resource type\n", __FUNCTION__);
		return -ENODEV;
	}

	/* Request IO memory */
	if (!request_mem_region(res[0].start,
				            res[0].end - res[0].start + 1, 
				            pdev->name)) 
	{
		return -EBUSY;
	}
	if (!request_mem_region(res[1].start,
				            res[1].end - res[1].start + 1, 
				            pdev->name)) 
	{
		return -EBUSY;
	}

	/* Allocate memory for the device structure (and zero it) */
	host = kzalloc(sizeof(struct mt6575_nand_host), GFP_KERNEL);	
	if (!host) 
	{
		MSG(INIT, "mt6575_nand: failed to allocate device structure.\n");
		return -ENOMEM;
	}

	/* Allocate memory for 16 byte aligned buffer */
    local_buffer_16_align = local_buffer + 16 - ((u32)local_buffer % 16);
    printk(KERN_INFO "Allocate 16 byte aligned buffer: %p\n", local_buffer_16_align);

    host->hw = hw;

	/* init mtd data structure */
	nand_chip  = &host->nand_chip;
	nand_chip->priv = host;		/* link the private data structures */
	
	mtd        = &host->mtd;	
	mtd->priv  = nand_chip;
	mtd->owner = THIS_MODULE;
	mtd->name  = "MT6575-Nand";

    hw->nand_ecc_mode = NAND_ECC_HW;

	/* Set address of NAND IO lines */
	nand_chip->IO_ADDR_R 	    = (void __iomem*)NFI_DATAR_REG32;
	nand_chip->IO_ADDR_W 	    = (void __iomem*)NFI_DATAW_REG32;
	nand_chip->chip_delay 	    = 20;			/* 20us command delay time */
	nand_chip->ecc.mode 	    = hw->nand_ecc_mode;	/* enable ECC */

	nand_chip->read_byte        = mt6575_nand_read_byte;
	nand_chip->read_buf		    = mt6575_nand_read_buf;
	nand_chip->write_buf	    = mt6575_nand_write_buf;
#ifdef CONFIG_MTD_NAND_VERIFY_WRITE	
	nand_chip->verify_buf       = mt6575_nand_verify_buf;
#endif
    nand_chip->select_chip      = mt6575_nand_select_chip;
    nand_chip->dev_ready 	    = mt6575_nand_dev_ready;
	nand_chip->cmdfunc 		    = mt6575_nand_command_bp;	
   	nand_chip->ecc.read_page    = mt6575_nand_read_page_hwecc;
	nand_chip->ecc.write_page   = mt6575_nand_write_page_hwecc;


    nand_chip->ecc.layout	    = &nand_oob_64;
    nand_chip->ecc.size		    = hw->nand_ecc_size;	//2048
    nand_chip->ecc.bytes	    = hw->nand_ecc_bytes;	//32
	//nand_chip->options		    = NAND_USE_FLASH_BBT;
	nand_chip->options		 = NAND_SKIP_BBTSCAN;
	//nand_chip->options		 = NAND_USE_FLASH_BBT | NAND_NO_AUTOINCR;
								/*
							   BBT_AUTO_REFRESH      | 
		                       NAND_NO_SUBPAGE_WRITE | 
		                       NAND_NO_AUTOINCR;
		                       */

    // For BMT, we need to revise driver architecture
    nand_chip->write_page       = mt6575_nand_write_page;
    nand_chip->ecc.write_oob    = mt6575_nand_write_oob;
    nand_chip->read_page        = mt6575_nand_read_page;
    nand_chip->ecc.read_oob     = mt6575_nand_read_oob;
    nand_chip->block_markbad    = mt6575_nand_block_markbad;   // need to add nand_get_device()/nand_release_device().
    nand_chip->erase            = mt6575_nand_erase;
    nand_chip->block_bad        = mt6575_nand_block_bad;


	mt6575_nand_init_hw(host);
	/* Select the device */
    /* Jun Shen, 2011.04.13  */
    nand_chip->select_chip(mtd, NFI_DEFAULT_CS);
    /* Jun Shen, 2011.04.13  */

	/*
	 * Reset the chip, required by some chips (e.g. Micron MT29FxGxxxxx)
	 * after power-up
	 */
	nand_chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* Send the command for reading device ID */
	nand_chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	/* Read manufacturer and device IDs */
	manu_id = nand_chip->read_byte(mtd);
	dev_id = nand_chip->read_byte(mtd);

	ext_id1 = nand_chip->read_byte(mtd);
	ext_id2 = nand_chip->read_byte(mtd);
	ext_id3 = nand_chip->read_byte(mtd);
    ext_id = ext_id1 << 16 | ext_id2 << 8 | ext_id3;
	//Check NAND Info
	id = dev_id | (manu_id << 8);
	if(!get_device_info(id, ext_id, &devinfo))
	{
        MSG(INIT, "Not Support this Device! \r\n");
	}

    if (devinfo.pagesize == 4096) {
        nand_chip->ecc.layout = &nand_oob_128;
    } else if (devinfo.pagesize == 2048) {
        nand_chip->ecc.layout = &nand_oob_64;
    } else if (devinfo.pagesize == 512) {
        nand_chip->ecc.layout = &nand_oob_16;	
    }
    
	  nand_chip->ecc.layout->eccbytes = devinfo.sparesize-OOB_AVAI_PER_SECTOR*(devinfo.pagesize/NAND_SECTOR_SIZE);
		for(i=0;i<nand_chip->ecc.layout->eccbytes;i++){
			nand_chip->ecc.layout->eccpos[i]=OOB_AVAI_PER_SECTOR*(devinfo.pagesize/NAND_SECTOR_SIZE)+i;
		}
    MSG(INIT, "[NAND] pagesz:%d eccsz: %d, oobsz: %d,eccbytes: %d\n", 
        nand_chip->ecc.size, nand_chip->ecc.bytes, sizeof(g_kCMD.au1OOB),nand_chip->ecc.layout->eccbytes);

	MSG(INIT, "Support this Device in MTK table! %x \r\n",id);
    hw->nfi_bus_width = devinfo.iowidth;
	DRV_WriteReg32(NFI_ACCCON_REG32, devinfo.timmingsetting);

	/* 16-bit bus width */
	if (hw->nfi_bus_width == 16)
	{
	    MSG(INIT, "%s : Set the 16-bit I/O settings!\n", MODULE_NAME);
		nand_chip->options |= NAND_BUSWIDTH_16;
	}

    /*  register NFI IRQ handler. */
    /* Koshi, 2011.03.10 { */
	mt65xx_irq_set_sens(MT6575_NFI_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
    mt65xx_irq_set_polarity(MT6575_NFI_IRQ_ID, MT65xx_POLARITY_LOW);
    err = request_irq(MT6575_NFI_IRQ_ID, mt6575_nand_irq_handler, IRQF_DISABLED, 
            "mt6575-nand", NULL); 
    /* Koshi, 2011.03.10 } */
    if (0 != err) 
	{
        MSG(INIT, "%s : Request IRQ fail: err = %d\n", MODULE_NAME, err);
        goto out;
    }

    if (g_i4Interrupt)
        enable_irq(MT6575_NFI_IRQ_ID);
    else
        disable_irq(MT6575_NFI_IRQ_ID);

#if 0
    if (devinfo.advancedmode & CACHE_READ)
    {
        nand_chip->ecc.read_multi_page_cache = NULL;
        // nand_chip->ecc.read_multi_page_cache = mt6575_nand_read_multi_page_cache;
    	// MSG(INIT, "Device %x support cache read \r\n",id);
    }
    else
        nand_chip->ecc.read_multi_page_cache = NULL;
#endif
	mtd->oobsize = devinfo.sparesize;
	/* Scan to find existance of the device */
	if (nand_scan(mtd, hw->nfi_cs_num)) 
	{
		MSG(INIT, "%s : nand_scan fail.\n", MODULE_NAME);
		err = -ENXIO;
		goto out;
	}

	g_page_size = mtd->writesize;
	
	platform_set_drvdata(pdev, host);
    
    if (hw->nfi_bus_width == 16)
	{
		NFI_SET_REG16(NFI_PAGEFMT_REG16, PAGEFMT_DBYTE_EN);
	}

	nand_chip->select_chip(mtd, 0);
  
    nand_chip->chipsize -= (BMT_POOL_SIZE) << nand_chip->phys_erase_shift;
    mtd->size = nand_chip->chipsize;

    if (!g_bmt)
    {
        if ( !(g_bmt = init_bmt(nand_chip, BMT_POOL_SIZE)) )
        {
            MSG(INIT, "Error: init bmt failed\n");
            // ASSERT(0);
            return 0;
        }
    }
	
   nand_chip->chipsize -= (PMT_POOL_SIZE) << nand_chip->phys_erase_shift;
   mtd->size = nand_chip->chipsize;

#ifdef CONFIG_MTD_PARTITIONS

#ifdef PMT

	part_init_pmt(mtd,(u8 *)&g_exist_Partition[0]);
	err = add_mtd_partitions(mtd, g_exist_Partition, part_num);

#else

	err = add_mtd_partitions(mtd, g_pasStatic_Partition, part_num);
#endif	


#ifdef CONFIG_MTD_NAND_NVRAM

#endif

#else

	err = add_mtd_device(mtd);

#endif

#ifdef _MTK_NAND_DUMMY_DRIVER_
    dummy_driver_debug = 0;
#endif


	/* Successfully!! */
	if (!err)
	{
        MSG(INIT, "[mt6575_nand] probe successfully!\n");
        nand_disable_clock();
		return err;
	}

	/* Fail!! */
out:
	MSG(INIT, "[NFI] mt6575_nand_probe fail, err = %d!\n", err);
	
	nand_release(mtd);
	
	platform_set_drvdata(pdev, NULL);
	
	kfree(host);

    nand_disable_clock();
	return err;
}
static int mt6575_nand_suspend(struct platform_device *pdev, pm_message_t state)
{	
	if ((DRV_Reg32(NFI_STA_REG32) & (STA_NAND_BUSY |STA_NAND_FSM_MASK|STA_NFI_FSM_MASK))||(DRV_Reg16(NFI_MASTERSTA_REG16)))
	{
		printk(KERN_ERR "[NFI] Busy, Suspend Fail !\n");		
		return 1; // BUSY
	}	

	printk(KERN_ERR "[NFI] Suspend !\n");
    return 0;
}
static int mt6575_nand_resume(struct platform_device *pdev)
{
	MSG(POWERCTL, "[NFI] Resume !\n");
    return 0;
}

static int __devexit mt6575_nand_remove(struct platform_device *pdev)
{
	struct mt6575_nand_host *host = platform_get_drvdata(pdev);
	struct mtd_info *mtd = &host->mtd;

	nand_release(mtd);

	kfree(host);

    nand_disable_clock();
	
	return 0;
}

#if (NAND_OTP_SUPPORT && SAMSUNG_OTP_SUPPORT)

typedef struct
{
	unsigned int (* getLength)(unsigned int *QLength);
	unsigned int (* read)(unsigned int PageAddr, void *BufferPtr, void *SparePtr);
	unsigned int (* write)(unsigned int PageAddr, void *BufferPtr, void *SparePtr);
} OTP_OPERATES;

typedef struct
{
       char name[32];    
	int manu_id;
       int dev_id;
       OTP_OPERATES  operates;
} OTP_ENTRY;

unsigned int samsung_OTPQueryLength(unsigned int *QLength);
unsigned int samsung_OTPRead(unsigned int PageAddr, void *BufferPtr, void *SparePtr);
unsigned int samsung_OTPWrite(unsigned int PageAddr, void *BufferPtr, void *SparePtr);
unsigned int toshiba_OTPQueryLength(unsigned int *QLength);
unsigned int toshiba_OTPRead(unsigned int PageAddr, void *BufferPtr, void *SparePtr);
unsigned int toshiba_OTPWrite(unsigned int PageAddr, void *BufferPtr, void *SparePtr);
//CR501334  added by yongming.li support otp for samsung b050
unsigned int samsung_b050_OTPQueryLength(unsigned int *QLength);
unsigned int samsung_b050_OTPRead(unsigned int PageAddr, void *BufferPtr, void *SparePtr);
unsigned int samsung_b050_OTPWrite(unsigned int PageAddr, void *BufferPtr, void *SparePtr);


// added by yongming.li for support toshiba otp
OTP_ENTRY  otp_entrys[]=
{
    	{
		.name = "micro",
		.manu_id = 0X2C,
		.dev_id    = 0XbC,
	       .operates.getLength = samsung_OTPQueryLength,
              .operates.read = samsung_OTPRead,
              .operates.write = samsung_OTPWrite,
	},
       {
		.name = "toshiba",
		.manu_id = 0X98,
		.dev_id    = 0XbC,
	       .operates.getLength = toshiba_OTPQueryLength,
              .operates.read = toshiba_OTPRead,
              .operates.write = toshiba_OTPWrite,
	},
	{
		.name = "samsung_K524G2GACJ_B050",
		.manu_id = 0xEC,
		.dev_id    = 0XbC,
	       .operates.getLength = samsung_b050_OTPQueryLength,
              .operates.read = samsung_b050_OTPRead,
              .operates.write = samsung_b050_OTPWrite,
	},
	
};

#define  IS_OTP_IMEI_PROTECT  1
typedef enum
{
    OTP_IMEI_1 = 0,
    OTP_IMEI_2,
    OTP_IMEI_3,
    OTP_IMEI_4,
} OTP_IMEI_NUMBER;

OTP_IMEI_NUMBER Imei_number = OTP_IMEI_1;

#define  OTP_PAGE_OFFSET      0

static void  dumpBuf(void * buf)
{
   unsigned char * temp;
   int i=0;
   temp=(char *)buf;
   
   for(i=0;i<64;i=i+8)
   {
      printk(KERN_ERR  "[%s]%02x %02x %02x %02x %02x %02x %02x %02x \n",__func__, temp[i+0], \
	   	temp[i+1],temp[i+2],temp[i+3],temp[i+4],temp[i+5],temp[i+6],temp[i+7]);
   }
}
 

void OTP_IMEI_Read (void *BufferPtr, int num)
{
	int i, j;
	BOOL bValid = true;
	OTP_LAYOUT *otp_layout[2];
	IMEI_SVN imei_svn[2];
       i=0;
	memset(imei_svn[i].imei, 0xff, 2*sizeof(IMEI_SVN));
	printk(KERN_ERR  "[%s]:num=%d ,BufferPtr is %x\n", __func__, num,BufferPtr);
	if(Imei_number == OTP_IMEI_2)
	{
	   i=1;
	}
	if(Imei_number == OTP_IMEI_1)
	{
	   i=0;
	}
	{
		otp_layout[i] = (OTP_LAYOUT*)((unsigned char*)BufferPtr + i*sizeof(OTP_LAYOUT));

		for (j=0, bValid = true; j<8; j++)
		{
			if ((otp_layout[i]->imei[j]+otp_layout[i]->imei_checksum[j]) != 0xFF)
			{
				bValid = false;
				MSG(INIT, "[%s]:imei[%d] =%x, imei_checksum =%x \n", __func__, j,otp_layout[i]->imei[j],otp_layout[i]->imei_checksum[j]);
				break;
			}
		}
		if (bValid == true)
		{
			memcpy(imei_svn[0].imei, otp_layout[i]->imei, 8);
			//imei_svn[0].svn = 0x01;
			//imei_svn[0].pad = 0x00;
		}
	}
	dumpBuf(BufferPtr);
	printk(KERN_ERR "[%s]: -----------------------------------\n", __func__);
	memset(BufferPtr, 0xff, 20);
	memcpy(BufferPtr+i*10, imei_svn,8);
	dumpBuf(BufferPtr);
}

void OTP_IMEI_Write (void *BufferPtr, int num)
{
	int i, j;
	OTP_LAYOUT otp_layout[2];
	IMEI_SVN *imei_svn[2];
	BOOL bSkip = true;

	memset(otp_layout, 0xff, 2*sizeof(OTP_LAYOUT));  //no SVN+PAD
	printk(KERN_ERR "[%s]:num=%d\n", __func__, num);

	if(Imei_number == OTP_IMEI_2)
	{
	   i=1;
	}
	if(Imei_number == OTP_IMEI_1)
	{
	   i=0;
	}
      dumpBuf(BufferPtr);
	printk(KERN_ERR "[%s]: -----------------------------------\n", __func__);
	{
             imei_svn[i] = (IMEI_SVN*)((unsigned char*)BufferPtr+i*10);
		for (j=0, bSkip=false; j<7/*8*/; j++)
		{
			#if 1
			if (0xFF == imei_svn[i]->imei[j])
			{
			       MSG(INIT, "[%s]: invalid imei serial number,please try again i is %d j is %d\n", __func__,i,j);
				 bSkip=true;
				 break;
			}
			#endif	
		}
		if (false == bSkip)
		{
		      //bug is here begin
		      memcpy(otp_layout[i].imei, imei_svn[i]->imei, 8);
		      //bug is here end
			for (j=0; j<8; j++)
			{
			
			   otp_layout[i].imei_checksum[j] = 0xFF-otp_layout[i].imei[j];
			}
		}
	}
      dumpBuf(BufferPtr);
	printk(KERN_ERR "[%s]: -----------------------------------\n", __func__);
	memcpy(BufferPtr, otp_layout, 2*sizeof(OTP_LAYOUT)); 
	dumpBuf(BufferPtr);
}

void OTP_PostProcess (unsigned int PageAddr, void *BufferPtr, E_RW bRW)
{
	MSG(INIT, "[%s]:PageAddr=%x, bRW=%d\n", __func__, PageAddr, bRW);
	
	if (0 != PageAddr) return;
      
	if (E_READ == bRW)
	{
		OTP_IMEI_Read (BufferPtr, 2);

	} else
	{
		OTP_IMEI_Write (BufferPtr, 2);
	}
}

#define COLADDR_OFFSET  0x00

//CR501334  added by yongming.li support otp for samsung b050 begin
unsigned int samsung_b050_OTPQueryLength(unsigned int *QLength)
{
    *QLength = SAMSUNG_B050_OTP_PAGE_NUM * g_page_size;
	return 0;
}
unsigned int samsung_b050_OTPWrite(unsigned int PageAddr, void *BufferPtr, void *SparePtr)
{
    struct mtd_info *mtd = &host->mtd;
    unsigned int rowaddr, coladdr;
    unsigned int u4Size = g_page_size;
    unsigned int timeout = 0xFFFF;
    unsigned int bRet;
    unsigned int sec_num = mtd->writesize >> 9;

    if(PageAddr >= SAMSUNG_B050_OTP_PAGE_NUM)
    {
        return OTP_ERROR_OVERSCOPE;
    }

    #if IS_OTP_IMEI_PROTECT
     dumpBuf(BufferPtr);
     printk(KERN_ERR "[%s]: -----------------------------------\n", __func__);
     OTP_PostProcess (PageAddr, BufferPtr, E_WRITE);
    #endif
    /* Col -> Row; LSB first */
    coladdr = 0x00000000;
    rowaddr = 0x15+PageAddr+OTP_PAGE_OFFSET;  // erjing modify
    MSG(OTP, "[%s]:(COLADDR) [0x%08x]/(ROWADDR)[0x%08x]\n", __func__, coladdr, rowaddr);
    nand_get_device((struct nand_chip *)mtd->priv, mtd, FL_READING);
    mt6575_nand_reset();

    MSG(OTP, "[%s]: Start to write data to OTP area\n", __func__);

    if (!mt6575_nand_reset())
    {
	   bRet = OTP_ERROR_RESET;
          goto cleanup;
   }

	mt6575_nand_set_mode(CNFG_OP_PRGM);

	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_READ_EN);

	DRV_WriteReg16(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);

       DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(BufferPtr)); 
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);

     if(g_bHwEcc){
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
      }else{
	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
     }
	mt6575_nand_set_autoformat(true);

	ECC_Encode_Start();

       //30 65 80 ..........10
       if (!mt6575_nand_set_command(0x30))
      {
            bRet = OTP_ERROR_BUSY;
           goto cleanup;
      }
       if (!mt6575_nand_set_command(0x65))
      {
            bRet = OTP_ERROR_BUSY;
           goto cleanup;
      }       
	if (!mt6575_nand_set_command(0x80))
      {
            bRet = OTP_ERROR_BUSY;
           goto cleanup;
      }
      

	if (!mt6575_nand_set_address(coladdr, rowaddr, 2, 3))
       {
              bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	if (!mt6575_nand_status_ready(STA_NAND_BUSY))
       {
              bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	mt6575_nand_write_fdm_data((struct nand_chip *)mtd->priv, BufferPtr, sec_num);
	(void)mt6575_nand_write_page_data(mtd, BufferPtr, u4Size);
	if(!mt6575_nand_check_RW_count(u4Size))
    {
        printk(KERN_ERR "[%s]: Check RW count timeout !\n", __func__);
		dump_nfi();
        bRet = OTP_ERROR_TIMEOUT;
        goto cleanup;
    }

	mt6575_nand_stop_write();
      (void)mt6575_nand_set_command(NAND_CMD_PAGEPROG);
	while(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY);

    bRet = OTP_SUCCESS;

    MSG(OTP, "[%s]: End to write data to OTP area\n", __func__);

cleanup:
    mt6575_nand_reset();
    (void)mt6575_nand_set_command(0xFF);
	nand_release_device(mtd);
    return bRet;
}

unsigned int samsung_b050_OTPRead(unsigned int PageAddr, void *BufferPtr, void *SparePtr)
{
    struct mtd_info *mtd = &host->mtd;
    unsigned int rowaddr, coladdr;
    unsigned int u4Size = g_page_size;
    unsigned int timeout = 0xFFFF;
    unsigned int bRet;
    unsigned int sec_num = mtd->writesize >> 9;

    if(PageAddr >= SAMSUNG_B050_OTP_PAGE_NUM)
    {
        return OTP_ERROR_OVERSCOPE;
    }

    /* Col -> Row; LSB first */
    coladdr = 0x00000000;
    rowaddr = 0x15+PageAddr+OTP_PAGE_OFFSET;     
    MSG(OTP, "[%s]:(COLADDR) [0x%08x]/(ROWADDR)[0x%08x]\n", __func__, coladdr, rowaddr);

    /* Power on NFI HW component. */
    //nand_enable_clock();
    nand_get_device((struct nand_chip *)mtd->priv, mtd, FL_READING);
    mt6575_nand_reset();


    MSG(OTP, "[%s]: Start to read data from OTP area\n", __func__);

    if (!mt6575_nand_reset())
    {
        bRet = OTP_ERROR_RESET;
        goto cleanup;
    }

    mt6575_nand_set_mode(CNFG_OP_READ);
    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
    DRV_WriteReg16(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);

    DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(BufferPtr)); 
    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);

    if(g_bHwEcc){
    	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
    }else{
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
    }
    mt6575_nand_set_autoformat(true);
   if(g_bHwEcc){
    		ECC_Decode_Start();
    }
   //30 65 00 .... 30
    if (!mt6575_nand_set_command(0x30))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }
    if (!mt6575_nand_set_command(0x65))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }
    if (!mt6575_nand_set_command(0x00))  
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6575_nand_set_address(coladdr, rowaddr, 2, 3))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }
    // also 0X30
    if (!mt6575_nand_set_command(NAND_CMD_READSTART))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6575_nand_status_ready(STA_NAND_BUSY))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6575_nand_read_page_data(mtd, BufferPtr, u4Size))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6575_nand_status_ready(STA_NAND_BUSY))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    mt6575_nand_read_fdm_data(SparePtr, sec_num);

    mt6575_nand_stop_read();

    MSG(OTP, "[%s]: End to read data from OTP area\n", __func__);
    #if   IS_OTP_IMEI_PROTECT
    OTP_PostProcess (PageAddr, BufferPtr, E_READ);
    #endif

    bRet = OTP_SUCCESS;

cleanup:

	mt6575_nand_reset();
    (void)mt6575_nand_set_command(0xFF);
	nand_release_device(mtd);
	return bRet;
}
//CR501334  added by yongming.li support otp for samsung b050 end
unsigned int toshiba_OTPQueryLength(unsigned int *QLength)
{
    *QLength = TOSHIBA_OTP_PAGE_NUM * g_page_size;
	return 0;
}
unsigned int toshiba_OTPWrite(unsigned int PageAddr, void *BufferPtr, void *SparePtr)
{
    struct mtd_info *mtd = &host->mtd;
    unsigned int rowaddr, coladdr;
    unsigned int u4Size = g_page_size;
    unsigned int timeout = 0xFFFF;
    unsigned int bRet;
    unsigned int sec_num = mtd->writesize >> 9;

    if(PageAddr >= TOSHIBA_OTP_PAGE_NUM)
    {
        return OTP_ERROR_OVERSCOPE;
    }

    #if IS_OTP_IMEI_PROTECT
     dumpBuf(BufferPtr);
     printk(KERN_ERR "[%s]: -----------------------------------\n", __func__);
     OTP_PostProcess (PageAddr, BufferPtr, E_WRITE);
    #endif
    /* Col -> Row; LSB first */
    coladdr = 0x00000000;
    rowaddr = 0x02+PageAddr+OTP_PAGE_OFFSET;  // erjing modify
    MSG(OTP, "[%s]:(COLADDR) [0x%08x]/(ROWADDR)[0x%08x]\n", __func__, coladdr, rowaddr);
    nand_get_device((struct nand_chip *)mtd->priv, mtd, FL_READING);
    mt6575_nand_reset();

    MSG(OTP, "[%s]: Start to write data to OTP area\n", __func__);

    if (!mt6575_nand_reset())
    {
	   bRet = OTP_ERROR_RESET;
          goto cleanup;
   }

	mt6575_nand_set_mode(CNFG_OP_PRGM);

	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_READ_EN);

	DRV_WriteReg16(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);

       DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(BufferPtr)); 
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);

     if(g_bHwEcc){
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
      }else{
	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
     }
	mt6575_nand_set_autoformat(true);

	ECC_Encode_Start();

       //5A B5 80
       if (!mt6575_nand_set_command(0x5A))
      {
            bRet = OTP_ERROR_BUSY;
           goto cleanup;
      }
       if (!mt6575_nand_set_command(0xB5))
      {
            bRet = OTP_ERROR_BUSY;
           goto cleanup;
      }       
	if (!mt6575_nand_set_command(0x80))
      {
            bRet = OTP_ERROR_BUSY;
           goto cleanup;
      }
      

	if (!mt6575_nand_set_address(coladdr, rowaddr, 2, 3))
       {
              bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	if (!mt6575_nand_status_ready(STA_NAND_BUSY))
       {
              bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	mt6575_nand_write_fdm_data((struct nand_chip *)mtd->priv, BufferPtr, sec_num);
	(void)mt6575_nand_write_page_data(mtd, BufferPtr, u4Size);
	if(!mt6575_nand_check_RW_count(u4Size))
    {
        printk(KERN_ERR "[%s]: Check RW count timeout !\n", __func__);
		dump_nfi();
        bRet = OTP_ERROR_TIMEOUT;
        goto cleanup;
    }

	mt6575_nand_stop_write();
      (void)mt6575_nand_set_command(NAND_CMD_PAGEPROG);
	while(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY);

    bRet = OTP_SUCCESS;

    MSG(OTP, "[%s]: End to write data to OTP area\n", __func__);

cleanup:
    mt6575_nand_reset();
    (void)mt6575_nand_set_command(0xFF);
	nand_release_device(mtd);
    return bRet;
}

unsigned int toshiba_OTPRead(unsigned int PageAddr, void *BufferPtr, void *SparePtr)
{
    struct mtd_info *mtd = &host->mtd;
    unsigned int rowaddr, coladdr;
    unsigned int u4Size = g_page_size;
    unsigned int timeout = 0xFFFF;
    unsigned int bRet;
    unsigned int sec_num = mtd->writesize >> 9;

    if(PageAddr >= TOSHIBA_OTP_PAGE_NUM)
    {
        return OTP_ERROR_OVERSCOPE;
    }

    /* Col -> Row; LSB first */
    coladdr = 0x00000000;
    rowaddr = 0x02+PageAddr+OTP_PAGE_OFFSET;     
    MSG(OTP, "[%s]:(COLADDR) [0x%08x]/(ROWADDR)[0x%08x]\n", __func__, coladdr, rowaddr);

    /* Power on NFI HW component. */
    //nand_enable_clock();
    nand_get_device((struct nand_chip *)mtd->priv, mtd, FL_READING);
    mt6575_nand_reset();


    MSG(OTP, "[%s]: Start to read data from OTP area\n", __func__);

    if (!mt6575_nand_reset())
    {
        bRet = OTP_ERROR_RESET;
        goto cleanup;
    }

    mt6575_nand_set_mode(CNFG_OP_READ);
    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
    DRV_WriteReg16(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);

    DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(BufferPtr)); 
    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);

    if(g_bHwEcc){
    	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
    }else{
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
    }
    mt6575_nand_set_autoformat(true);
   if(g_bHwEcc){
    		ECC_Decode_Start();
    }
   //5A B5 00
    if (!mt6575_nand_set_command(0x5A))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }
    if (!mt6575_nand_set_command(0xB5))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }
    if (!mt6575_nand_set_command(0x00))  
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6575_nand_set_address(coladdr, rowaddr, 2, 3))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }
    // also 0X30
    if (!mt6575_nand_set_command(NAND_CMD_READSTART))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6575_nand_status_ready(STA_NAND_BUSY))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6575_nand_read_page_data(mtd, BufferPtr, u4Size))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6575_nand_status_ready(STA_NAND_BUSY))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    mt6575_nand_read_fdm_data(SparePtr, sec_num);

    mt6575_nand_stop_read();

    MSG(OTP, "[%s]: End to read data from OTP area\n", __func__);
    #if   IS_OTP_IMEI_PROTECT
    OTP_PostProcess (PageAddr, BufferPtr, E_READ);
    #endif

    bRet = OTP_SUCCESS;

cleanup:

	mt6575_nand_reset();
    (void)mt6575_nand_set_command(0xFF);
	nand_release_device(mtd);
	return bRet;
}

unsigned int samsung_OTPQueryLength(unsigned int *QLength)
{
    *QLength = SAMSUNG_OTP_PAGE_NUM * g_page_size;
	return 0;
}

unsigned int samsung_OTPRead(unsigned int PageAddr, void *BufferPtr, void *SparePtr)
{
    struct mtd_info *mtd = &host->mtd;
    unsigned int rowaddr, coladdr;
    unsigned int u4Size = g_page_size;
    unsigned int timeout = 0xFFFF;
    unsigned int bRet;
    unsigned int sec_num = mtd->writesize >> 9;

    if(PageAddr >= SAMSUNG_OTP_PAGE_NUM)
    {
        return OTP_ERROR_OVERSCOPE;
    }

    /* Col -> Row; LSB first */
    coladdr = 0x00000000;
   // rowaddr = Samsung_OTP_Page[PageAddr];
     rowaddr = 0x02+PageAddr+OTP_PAGE_OFFSET;       // erjing modify
    MSG(OTP, "[%s]:(COLADDR) [0x%08x]/(ROWADDR)[0x%08x]\n", __func__, coladdr, rowaddr);

    /* Power on NFI HW component. */
    //nand_enable_clock();
	nand_get_device((struct nand_chip *)mtd->priv, mtd, FL_READING);
	mt6575_nand_reset();
    (void)mt6575_nand_set_command(0xAF); // modify by erjing
    if(0)
    {
    mt6575_nand_reset();
        (void)mt6575_nand_set_command(0x65); //delete by erjing
    }

    MSG(OTP, "[%s]: Start to read data from OTP area\n", __func__);

    if (!mt6575_nand_reset())
    {
        bRet = OTP_ERROR_RESET;
        goto cleanup;
    }

    mt6575_nand_set_mode(CNFG_OP_READ);
    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_READ_EN);
    DRV_WriteReg16(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);

    DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(BufferPtr)); 
    NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);

   if(g_bHwEcc){
    	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
   	}else{
		NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
	}
    mt6575_nand_set_autoformat(true);
	if(g_bHwEcc){
    		ECC_Decode_Start();
	}
    if (!mt6575_nand_set_command(0xAF))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6575_nand_set_address(coladdr, rowaddr, 2, 3))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6575_nand_set_command(NAND_CMD_READSTART))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6575_nand_status_ready(STA_NAND_BUSY))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6575_nand_read_page_data(mtd, BufferPtr, u4Size))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    if (!mt6575_nand_status_ready(STA_NAND_BUSY))
    {
        bRet = OTP_ERROR_BUSY;
        goto cleanup;
    }

    mt6575_nand_read_fdm_data(SparePtr, sec_num);

    mt6575_nand_stop_read();

    MSG(OTP, "[%s]: End to read data from OTP area\n", __func__);
    #if IS_OTP_IMEI_PROTECT
    OTP_PostProcess (PageAddr, BufferPtr, E_READ);
    #endif

    bRet = OTP_SUCCESS;

cleanup:

	mt6575_nand_reset();
    (void)mt6575_nand_set_command(0xFF);
	nand_release_device(mtd);
	return bRet;
}

unsigned int samsung_OTPWrite(unsigned int PageAddr, void *BufferPtr, void *SparePtr)
{
    struct mtd_info *mtd = &host->mtd;
    unsigned int rowaddr, coladdr;
    unsigned int u4Size = g_page_size;
    unsigned int timeout = 0xFFFF;
    unsigned int bRet;
    unsigned int sec_num = mtd->writesize >> 9;

    if(PageAddr >= SAMSUNG_OTP_PAGE_NUM)
    {
        return OTP_ERROR_OVERSCOPE;
    }

    #if IS_OTP_IMEI_PROTECT
     dumpBuf(BufferPtr);
     printk(KERN_ERR "[%s]: -----------------------------------\n", __func__);
     OTP_PostProcess (PageAddr, BufferPtr, E_WRITE);
    #endif
    /* Col -> Row; LSB first */
    coladdr = 0x00000000;
    //rowaddr = Samsung_OTP_Page[PageAddr];
    rowaddr = 0x02+PageAddr+OTP_PAGE_OFFSET;  // erjing modify
    MSG(OTP, "[%s]:(COLADDR) [0x%08x]/(ROWADDR)[0x%08x]\n", __func__, coladdr, rowaddr);
	nand_get_device((struct nand_chip *)mtd->priv, mtd, FL_READING);
	mt6575_nand_reset();
    (void)mt6575_nand_set_command(0xA0);  // erjing modify 
    if(0)
    {
    mt6575_nand_reset();
    (void)mt6575_nand_set_command(0x65);
    }

    MSG(OTP, "[%s]: Start to write data to OTP area\n", __func__);

    if (!mt6575_nand_reset())
    {
		bRet = OTP_ERROR_RESET;
        goto cleanup;
	}

	mt6575_nand_set_mode(CNFG_OP_PRGM);

	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_READ_EN);

	DRV_WriteReg16(NFI_CON_REG16, sec_num << CON_NFI_SEC_SHIFT);

    DRV_WriteReg32(NFI_STRADDR_REG32, __virt_to_phys(BufferPtr)); 
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_AHB);

if(g_bHwEcc){
	NFI_SET_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
}else{
	NFI_CLN_REG16(NFI_CNFG_REG16, CNFG_HW_ECC_EN);
}
	mt6575_nand_set_autoformat(true);

	ECC_Encode_Start();

	if (!mt6575_nand_set_command(0xA0))
    {
        bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	if (!mt6575_nand_set_address(coladdr, rowaddr, 2, 3))
    {
        bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	if (!mt6575_nand_status_ready(STA_NAND_BUSY))
    {
        bRet = OTP_ERROR_BUSY;
		goto cleanup;
	}

	mt6575_nand_write_fdm_data((struct nand_chip *)mtd->priv, BufferPtr, sec_num);
	(void)mt6575_nand_write_page_data(mtd, BufferPtr, u4Size);
	if(!mt6575_nand_check_RW_count(u4Size))
    {
        printk(KERN_ERR "[%s]: Check RW count timeout !\n", __func__);
		dump_nfi();
        bRet = OTP_ERROR_TIMEOUT;
        goto cleanup;
    }

	mt6575_nand_stop_write();
    (void)mt6575_nand_set_command(NAND_CMD_PAGEPROG);
	while(DRV_Reg32(NFI_STA_REG32) & STA_NAND_BUSY);

    bRet = OTP_SUCCESS;

    MSG(OTP, "[%s]: End to write data to OTP area\n", __func__);

cleanup:
    mt6575_nand_reset();
    (void)mt6575_nand_set_command(0xFF);
	nand_release_device(mtd);
    return bRet;
}
#endif

#if NAND_OTP_SUPPORT
static int mt_otp_open(struct inode *inode, struct file *filp)
{
	MSG(OTP, "[%s]:(MAJOR)%d:(MINOR)%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	filp->private_data = (int*)OTP_MAGIC_NUM;
	return 0;
}

static int mt_otp_release(struct inode *inode, struct file *filp)
{
	MSG(OTP, "[%s]:(MAJOR)%d:(MINOR)%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	return 0;
}

static int mt_otp_access(unsigned int access_type, unsigned int offset, void *buff_ptr, unsigned int length, unsigned int *status)
{
    unsigned int i = 0, ret = 0;
    char *BufAddr = (char *)buff_ptr;
    unsigned int PageAddr, AccessLength=0;
    int Status = 0;

    static char *p_D_Buff = NULL;
    char S_Buff[64];

    if (!(p_D_Buff = kmalloc(g_page_size, GFP_KERNEL)))
    {
        ret = -ENOMEM;
        *status = OTP_ERROR_NOMEM;
        goto exit;
    }

    printk(KERN_ERR "[%s]: %s (0x%x) length:(%d bytes) !\n", __func__, access_type?"WRITE":"READ", offset, length);

    if(offset==0X0A)
    {
       Imei_number = OTP_IMEI_2;
    }
    if(offset==0X00)
    {
       Imei_number = OTP_IMEI_1;
    }

    offset += COLADDR_OFFSET;
	
    while(1)
    {
        PageAddr = offset/g_page_size;
        if(FS_OTP_READ == access_type)
        {
            memset(p_D_Buff, 0xff, g_page_size);
            memset(S_Buff, 0xff, (sizeof(char)*64));

            MSG(OTP, "[%s]: Read Access of page (%d)\n",__func__, PageAddr);

            Status = g_mt6575_otp_fuc.OTPRead(PageAddr, p_D_Buff, &S_Buff);
            *status = Status;

            if( OTP_SUCCESS != Status)
            {
                MSG(OTP, "[%s]: Read status (%d)\n", __func__, Status);
                break;
            }

            AccessLength = g_page_size - (offset % g_page_size);

            if(length >= AccessLength)
            {
                memcpy(BufAddr, (p_D_Buff+(offset % g_page_size)), AccessLength);
            }
            else
            {
                //last time
                memcpy(BufAddr, (p_D_Buff+(offset % g_page_size)), length);
            }
        }
        else if(FS_OTP_WRITE == access_type)
        {
            AccessLength = g_page_size - (offset % g_page_size);
            memset(p_D_Buff, 0xff, g_page_size);
            memset(S_Buff, 0xff, (sizeof(char)*64));

            if(length >= AccessLength)
            {
                memcpy((p_D_Buff+(offset % g_page_size)), BufAddr, AccessLength);
            }
            else
            {
                //last time
                memcpy((p_D_Buff+(offset % g_page_size)), BufAddr, length);
            }
            dumpBuf(p_D_Buff);
            printk(KERN_ERR "[%s]: -----------------------------------\n", __func__);
            Status = g_mt6575_otp_fuc.OTPWrite(PageAddr, p_D_Buff, &S_Buff);
            *status = Status;

            if( OTP_SUCCESS != Status)
            {
                MSG(OTP, "[%s]: Write status (%d)\n",__func__, Status);
                break;
            }
        }
        else
        {
            MSG(OTP, "[%s]: Error, not either read nor write operations !\n",__func__);
            break;
        }

        offset += AccessLength;
        BufAddr += AccessLength;
        if(length <= AccessLength)
        {
            length = 0;
            break;
        }
        else
        {
            length -= AccessLength;
            MSG(OTP, "[%s]: Remaining %s (%d) !\n",__func__, access_type?"WRITE":"READ", length);
        }
    }
error:
    kfree(p_D_Buff);
exit:
    return ret;
}

unsigned char  mt_otp_get_operate()
{
    int i=0;
    int size = sizeof(otp_entrys)/sizeof(otp_entrys[0]);
     MSG(INIT, "OTP[%s]:size is %d manu_id = %02x ,dev_id= %02x\n", __func__, size,manu_id, dev_id);
    for (i=0;i<size;i++)
    {
          if((manu_id == otp_entrys[i].manu_id)&&(dev_id == otp_entrys[i].dev_id))	/*MT29F4G16ABBDA*/
         {
           g_mt6575_otp_fuc.OTPQueryLength = otp_entrys[i].operates.getLength;
           g_mt6575_otp_fuc.OTPRead = otp_entrys[i].operates.read;
           g_mt6575_otp_fuc.OTPWrite = otp_entrys[i].operates.write;
	    printk(KERN_ERR "[%s]: select index is %d and name is %s\n", __func__,i,otp_entrys[i].name);
           return 0;
         }
    }
    printk(KERN_ERR "[%s]: $$$$$$$$$$$$$$$ cant find match nand flash operate $$$$$$$$$$$$$$$$$$\n", __func__);
    return 1;
}

static long mt_otp_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0, i=0;
	static char *pbuf = NULL;

	void __user *uarg = (void __user *)arg;
       struct otp_ctl otpctl;

    /* Lock */
    spin_lock(&g_OTPLock);

    //  added by yongming.li for toshiba otp
    if(mt_otp_get_operate())
    {
         goto exit;
    }
    #if 0
    if((manu_id == 0x2c)&&(dev_id == 0xbc))	/*MT29F4G16ABBDA*/
    {
           g_mt6575_otp_fuc.OTPQueryLength = samsung_OTPQueryLength;
           g_mt6575_otp_fuc.OTPRead = samsung_OTPRead;
           g_mt6575_otp_fuc.OTPWrite = samsung_OTPWrite;
    }
    else if((manu_id == 0x98)&&(dev_id == 0xbc))	/*toshiba*/
    {
           g_mt6575_otp_fuc.OTPQueryLength = toshiba_OTPQueryLength;
           g_mt6575_otp_fuc.OTPRead = toshiba_OTPRead;
           g_mt6575_otp_fuc.OTPWrite = toshiba_OTPWrite;
    }
    else
    {
           printk(KERN_ERR "[%s]: Cant find match Nand Flash , now only support  micro and toshiba\n", __func__);
	    goto exit;
    }
    #endif

    if (copy_from_user(&otpctl, uarg, sizeof(struct otp_ctl)))
    {
        ret = -EFAULT;
        goto exit;
    }

    if(false == g_bInitDone)
    {
        MSG(OTP, "ERROR: NAND Flash Not initialized !!\n");
        ret = -EFAULT;
        goto exit;
    }

    if (!(pbuf = kmalloc(sizeof(char)*otpctl.Length, GFP_KERNEL)))
    {
        ret = -ENOMEM;
        goto exit;
    }

	switch (cmd)
    {
	case OTP_GET_LENGTH:
        MSG(OTP, "OTP IOCTL: OTP_GET_LENGTH\n");
        g_mt6575_otp_fuc.OTPQueryLength(&otpctl.QLength);
        otpctl.status = OTP_SUCCESS;
        MSG(OTP, "OTP IOCTL: The Length is %d\n", otpctl.QLength);
        break;
    case OTP_READ:
        MSG(OTP, "OTP IOCTL: OTP_READ Offset(0x%x), Length(0x%x) \n", otpctl.Offset, otpctl.Length);
        memset(pbuf, 0xff, sizeof(char)*otpctl.Length);

        mt_otp_access(FS_OTP_READ, otpctl.Offset, pbuf, otpctl.Length, &otpctl.status);
		
        printk(KERN_ERR "[%s]: -----------------------------------\n", __func__);
	  if(otpctl.Length==0X0A)
	  {
	     	  printk(KERN_ERR "[%s] !! read data is : %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",__func__, \
	  	       pbuf[0], pbuf[1],pbuf[2],pbuf[3],pbuf[4],pbuf[5],\
	  	       pbuf[6], pbuf[7],pbuf[8],pbuf[9]);
	  }
	  printk(KERN_ERR "[%s]: -----------------------------------\n", __func__);

        if (copy_to_user(otpctl.BufferPtr, pbuf, (sizeof(char)*otpctl.Length)))
        {
            MSG(OTP, "OTP IOCTL: Copy to user buffer Error !\n");
            goto error;
        }
        break;
    case OTP_WRITE:
        MSG(OTP, "OTP IOCTL: OTP_WRITE Offset(0x%x), Length(0x%x) \n", otpctl.Offset, otpctl.Length);
        if (copy_from_user(pbuf, otpctl.BufferPtr, (sizeof(char)*otpctl.Length)))
        {
            MSG(OTP, "OTP IOCTL: Copy from user buffer Error !\n");
            goto error;
        }
	  //added by liyongming for test write imei 
	  if(otpctl.Length==0X0A)
	  {
	     	  printk(KERN_ERR "[%s] !!!write data is : %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",__func__, \
	  	       pbuf[0], pbuf[1],pbuf[2],pbuf[3],pbuf[4],pbuf[5],\
	  	       pbuf[6], pbuf[7],pbuf[8],pbuf[9]);
	  }

          int j=0;
	   for (j=0; j<7; j++)
           {
	       if (0xFF == pbuf[j])
	       {
	           MSG(INIT, "OTP[%s][%d]: invalid raw data to write !!\n", __func__,__LINE__);
                   goto error;
               }
	   }

	//added by liyongming for test write imei end
   
        mt_otp_access(FS_OTP_WRITE, otpctl.Offset , pbuf, otpctl.Length, &otpctl.status);
        break;
	default:
		ret = -EINVAL;
	}

    ret = copy_to_user(uarg, &otpctl, sizeof(struct otp_ctl));

error:
    kfree(pbuf);
exit:
    spin_unlock(&g_OTPLock);
    return ret;
}

static struct file_operations nand_otp_fops = {
    .owner=      THIS_MODULE,
    .ioctl=      mt_otp_ioctl,
    .open=       mt_otp_open,
    .release=    mt_otp_release,
};

static struct miscdevice nand_otp_dev = {
    .minor   = MISC_DYNAMIC_MINOR,
    .name    = "otp",
    .fops    = &nand_otp_fops,
};
#endif

static struct platform_driver mt6575_nand_driver = {
	.probe		= mt6575_nand_probe,
	.remove		= mt6575_nand_remove,
	.suspend	= mt6575_nand_suspend,
	.resume	    = mt6575_nand_resume,
	.driver		= {
		.name	= "mt6575-nand",
		.owner	= THIS_MODULE,
	},
};

static int __init mt6575_nand_init(void)
{
	struct proc_dir_entry *entry;
	
//#ifdef CONFIG_MTK_MTD_NAND_INTERRUPT_SCHEME	
    // boot up using polling mode
	g_i4Interrupt = 0;      
//#else
//	g_i4Interrupt = 0;
//#endif

#if NAND_OTP_SUPPORT
    int err = 0;
    MSG(OTP, "OTP: register NAND OTP device ...\n");
	err = misc_register(&nand_otp_dev);
	if (unlikely(err))
    {
		MSG(OTP, "OTP: failed to register NAND OTP device!\n");
		return err;
	}
	spin_lock_init(&g_OTPLock);
#endif

	entry = create_proc_entry(PROCNAME, 0664, NULL);
	if (entry == NULL) 
	{
		MSG(INIT, "MT6575 Nand : unable to create /proc entry\n");
		return -ENOMEM;
	}
	entry->read_proc = mt6575_nand_proc_read;
	entry->write_proc = mt6575_nand_proc_write;	
	//entry->owner = THIS_MODULE;

	MSG(INIT, "MediaTek MT6575 Nand driver init, version %s\n", VERSION);

	return platform_driver_register(&mt6575_nand_driver);
}

static void __exit mt6575_nand_exit(void)
{
	MSG(INIT, "MediaTek MT6575 Nand driver exit, version %s\n", VERSION);
#if NAND_OTP_SUPPORT
	misc_deregister(&nand_otp_dev);
#endif

#ifdef SAMSUNG_OTP_SUPPORT
    g_mt6575_otp_fuc.OTPQueryLength = NULL;
    g_mt6575_otp_fuc.OTPRead = NULL;
    g_mt6575_otp_fuc.OTPWrite = NULL;
#endif

	platform_driver_unregister(&mt6575_nand_driver);
	remove_proc_entry(PROCNAME, NULL);
}

module_init(mt6575_nand_init);
module_exit(mt6575_nand_exit);
MODULE_LICENSE("GPL");
