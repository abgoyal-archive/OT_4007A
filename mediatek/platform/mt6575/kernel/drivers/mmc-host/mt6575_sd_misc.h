#ifndef MT6575_SD_MISC_H
#define MT6575_SD_MISC_H

#include <linux/bitops.h>
#include <linux/mmc/host.h>

#include <mach/mt6575_reg_base.h>


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
    int  partition;
	int  result;
};


#define MSDC_DRIVING_SETTING              (0)
#define MSDC_CLOCK_FREQUENCY              (1)
#define MSDC_SINGLE_READ_WRITE            (2)
#define MSDC_MULTIPLE_READ_WRITE          (3)
#define MSDC_GET_CID                      (4)
#define MSDC_GET_CSD                      (5)
#define MSDC_GET_EXCSD                    (6)
#define MSDC_CARD_DUNM_FUNC               (0xff)

typedef enum {
    USER_PARTITION = 0,
    BOOT_PARTITION_1,
    BOOT_PARTITION_2,
    RPMB_PARTITION,
    GP_PARTITION_1,
    GP_PARTITION_2,
    GP_PARTITION_3,
    GP_PARTITION_4,
}PARTITON_ACCESS_T;


#endif /* end of MT6575_SD_MISC_H */
