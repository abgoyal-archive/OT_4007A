#ifndef __DUMCHAR_H__
#define __DUMCHAR_H__


#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/mtd/mtd.h>
#include <linux/semaphore.h>

#define DUMCHAR_DEBUG
//#undef DUMCHAR_DEBUG             /* undef it, just in case */

#ifdef DUMCHAR_DEBUG
#define DDEBUG(fmt, args...) printk( KERN_DEBUG "dumchar_debug: " fmt, ## args)
#else
#define DDEBUG(fmt, args...) 
#endif


#define DUMCHAR_MAJOR        0   /* dynamic major by default */
#define MAX_SD_BUFFER		(512)
#define ALIE_LEN		512

#define  PMT_MAGIC	 'p'
#define PMT_READ		_IOW(PMT_MAGIC, 1, int)
#define PMT_WRITE 		_IOW(PMT_MAGIC, 2, int)

//#define PrintBuff 1

typedef enum  {
	MTD = 1,
	SDCARD = 2,
} devtype;

typedef enum {
	USER_PARTITION = 0,
	BOOT_PARTITION_1,
	BOOT_PARTITION_2,
	RPMB_PARTITION,
	GP_PARTITION_1,
	GP_PARTITION_2,
	GP_PARTITION_3,
	GP_PARTITION_4,
} PARTITION_ACCESS_T;

struct actdevice_info {
	unsigned int size;
	char actname[64];	//full act name /dev/mt6573_sd0 /dev/mtd/mtd1
	union {
		unsigned int start_address;//for emmc
		unsigned int index; //for mtd
	} pri;
	PARTITION_ACCESS_T part;
	unsigned int partition_idx;
};

struct Region_Info {
	PARTITION_ACCESS_T region;
	int	size_Byte;
};

struct excel_info{
	char * name;
	unsigned int size;
	unsigned int start_address;
	unsigned int partition_idx;
};

struct dumchar_dev {
	char *dumname;	//nvram boot userdata...
	struct semaphore sem;     /* Mutual exclusion */
	devtype type; //mtd device or emmc device?
	struct actdevice_info device_info;
	struct file *actfile; 
	struct cdev cdev;
};


struct msdc_ioctl{
	int  opcode;
	int  host_num;
	int  iswrite;
	int  trans_type;
	u32  total_size;
	u32  address;
	u32* buffer;
	int  cmd_pu_driving;
	int  cmd_pd_driving;
	int  dat_pu_driving;
	int  dat_pd_driving;
	int  clk_pu_driving;
	int  clk_pd_driving;
	int  clock_freq;
    PARTITION_ACCESS_T  partition;
	int  result;
};

#define REGION_NUM 						8
#define EXT_CSD_BOOT_SIZE_MULT          226 /* R */
#define EXT_CSD_RPMB_SIZE_MULT          168 /* R */

#define MSDC_DRIVING_SETTING			(0)
#define MSDC_CLOCK_FREQUENCY			(1)
#define MSDC_SINGLE_READ_WRITE   		(2)
#define MSDC_MULTIPLE_READ_WRITE   		(3)

#define mtd_for_each_device(mtd)			\
	for ((mtd) = __mtd_next_device(0);		\
	     (mtd) != NULL;				\
	     (mtd) = __mtd_next_device(mtd->index + 1))

#endif /*__DUMCHAR_H__ */

