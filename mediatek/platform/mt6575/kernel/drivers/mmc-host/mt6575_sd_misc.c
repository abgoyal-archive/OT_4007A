#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/blkdev.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/dma-mapping.h>

#include <mach/dma.h>
#include <mach/board.h> /* FIXME */
#include <mach/mt6575_clock_manager.h>
#include <asm/tcm.h>

#include "mt6575_sd.h"
#include "mt6575_sd_misc.h"
#include "board-custom.h"
#include <mach/mt6575_pm_ldo.h>


#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#ifdef MTK_EMMC_SUPPORT
#include "part_info_emmc.h"
extern struct excel_info PartInfoEmmc[PART_NUM];
extern int mt6575_sd_request(struct mmc_host*mmc, struct mmc_request*mrq);
#endif

#define DRV_NAME_MISC            "misc-mt6575-sd"

#define DEBUG_MMC_IOCTL   0
#define FORCE_IN_DMA (0x11)
#define FORCE_IN_PIO (0x10)
#define FORCE_NOTHING (0x0)
static int    dma_force[4] = //used for sd ioctrol
{
	FORCE_NOTHING,
	FORCE_NOTHING,
	FORCE_NOTHING,
	FORCE_NOTHING
};
#define dma_is_forced(host_id) 	(dma_force[host_id] & 0x10)
#define get_forced_transfer_mode(host_id)  (dma_force[host_id] & 0x01)


static u32* sg_msdc_multi_buffer = NULL;

static int simple_mt6575_sd_open(struct inode *inode, struct file *file)
{
	return 0;
}

extern struct msdc_host *msdc_6575_host[];
extern int mmc_switch(struct mmc_card *card, u8 set, u8 index, u8 value);
extern int mmc_send_ext_csd(struct mmc_card *card, u8 *ext_csd);
extern struct msdc_host *msdc_6575_host[];

static int simple_mt6575_sd_ioctl_single_rw(struct msdc_ioctl* msdc_ctl)
{
	char l_buf[512];
	struct scatterlist msdc_sg;
	struct mmc_data    msdc_data;
	struct mmc_command msdc_cmd;
	struct mmc_request msdc_mrq;
    struct msdc_host *host_ctl;
   
    host_ctl = msdc_6575_host[msdc_ctl->host_num];
	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);

	mmc_claim_host(host_ctl->mmc);

#if DEBUG_MMC_IOCTL
	printk("user want access %d partition\n",msdc_ctl->partition);
#endif
	switch (msdc_ctl->partition){
	case BOOT_PARTITION_1:
		mmc_send_ext_csd(host_ctl->mmc->card,l_buf);

		/* change to access boot partition 1 */
		l_buf[179] &= ~0x7;
		l_buf[179] |= 0x1;
		mmc_switch(host_ctl->mmc->card, 0, 179, l_buf[179]);
		break;
	case BOOT_PARTITION_2:
		mmc_send_ext_csd(host_ctl->mmc->card,l_buf);

		/* change to access boot partition 2 */
		l_buf[179] &= ~0x7;
		l_buf[179] |= 0x2;
		mmc_switch(host_ctl->mmc->card, 0, 179, l_buf[179]);
		break;
	default:
		break;
	}

	if(msdc_ctl->total_size > 512){
		msdc_ctl->result = -1;
		return  msdc_ctl->result;
	}

#if DEBUG_MMC_IOCTL
	printk("start MSDC_SINGLE_READ_WRITE !!\n");
#endif    
	memset(&msdc_data, 0, sizeof(struct mmc_data));
	memset(&msdc_mrq, 0, sizeof(struct mmc_request));
	memset(&msdc_cmd, 0, sizeof(struct mmc_command));

	msdc_mrq.cmd = &msdc_cmd;
	msdc_mrq.data = &msdc_data;

	if(msdc_ctl->trans_type)
		dma_force[host_ctl->id] = FORCE_IN_DMA;
	else
		dma_force[host_ctl->id] = FORCE_IN_PIO;

	if (msdc_ctl->iswrite){
		msdc_data.flags = MMC_DATA_WRITE;
		msdc_cmd.opcode = MMC_WRITE_BLOCK;
		msdc_data.blocks = msdc_ctl->total_size / 512;
		if (MSDC_CARD_DUNM_FUNC != msdc_ctl->opcode) {
            if (copy_from_user(sg_msdc_multi_buffer, msdc_ctl->buffer, 512)){
                dma_force[host_ctl->id] = FORCE_NOTHING;
                return -EFAULT;
                }
		} else {
			/* called from other kernel module */
			memcpy(sg_msdc_multi_buffer, msdc_ctl->buffer, 512);
		}
	} else {
		msdc_data.flags = MMC_DATA_READ;
		msdc_cmd.opcode = MMC_READ_SINGLE_BLOCK;
		msdc_data.blocks = msdc_ctl->total_size / 512;

		memset(sg_msdc_multi_buffer, 0 , 512);
	}

	msdc_cmd.arg = msdc_ctl->address;

	BUG_ON(!host_ctl->mmc->card);
	if (!mmc_card_blockaddr(host_ctl->mmc->card)){
		printk("the device is used byte address!\n");
		msdc_cmd.arg <<= 9;
	}

	msdc_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	msdc_data.stop = NULL;
	msdc_data.blksz = 512;
	msdc_data.sg = &msdc_sg;
	msdc_data.sg_len = 1;

#if DEBUG_MMC_IOCTL
	printk("single block: ueser buf address is 0x%p!\n",msdc_ctl->buffer);
#endif    
	sg_init_one(&msdc_sg, sg_msdc_multi_buffer, msdc_ctl->total_size);
	mmc_set_data_timeout(&msdc_data, host_ctl->mmc->card);

	mmc_wait_for_req(host_ctl->mmc, &msdc_mrq);

	if (!msdc_ctl->iswrite){
		if (MSDC_CARD_DUNM_FUNC != msdc_ctl->opcode) {
            if (copy_to_user(msdc_ctl->buffer,sg_msdc_multi_buffer,512)){
                dma_force[host_ctl->id] = FORCE_NOTHING;
                return -EFAULT;
                }
		} else {
			/* called from other kernel module */
			memcpy(msdc_ctl->buffer,sg_msdc_multi_buffer,512);
        }
	}

	if (msdc_ctl->partition){
		mmc_send_ext_csd(host_ctl->mmc->card,l_buf);

		if (l_buf[179] & 0x8) {
			/* set back to access user area */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x0;
			mmc_switch(host_ctl->mmc->card, 0, 179, l_buf[179]);
		}
	}

	mmc_release_host(host_ctl->mmc);

	if (msdc_cmd.error)
		msdc_ctl->result= msdc_cmd.error;

	if (msdc_data.error)
		msdc_ctl->result= msdc_data.error;
	else
		msdc_ctl->result= 0;

        dma_force[host_ctl->id] = FORCE_NOTHING;
	return msdc_ctl->result;
}

static int simple_mt6575_sd_ioctl_multi_rw(struct msdc_ioctl* msdc_ctl)
{
	char l_buf[512];
	struct scatterlist msdc_sg;
	struct mmc_data  msdc_data;
	struct mmc_command msdc_cmd;
	struct mmc_command msdc_stop;
	struct mmc_request  msdc_mrq;
    struct msdc_host *host_ctl;
   
    host_ctl = msdc_6575_host[msdc_ctl->host_num];
	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);

	mmc_claim_host(host_ctl->mmc);

#if DEBUG_MMC_IOCTL
	printk("user want access %d partition\n",msdc_ctl->partition);
#endif
	switch (msdc_ctl->partition){
	case BOOT_PARTITION_1:
		mmc_send_ext_csd(host_ctl->mmc->card,l_buf);
		printk("extend CSD: partition config is 0x%x\n", l_buf[179]);

		/* change to access boot partition 1 */
		l_buf[179] &= ~0x7;
		l_buf[179] |= 0x1;
		mmc_switch(host_ctl->mmc->card, 0, 179, l_buf[179]);
		break;
	case BOOT_PARTITION_2:
		mmc_send_ext_csd(host_ctl->mmc->card,l_buf);
		printk("extend CSD: partition config is 0x%x\n", l_buf[179]);

		/* change to access boot partition 2 */
		l_buf[179] &= ~0x7;
		l_buf[179] |= 0x2;
		mmc_switch(host_ctl->mmc->card, 0, 179, l_buf[179]);
		break;
	default:
		break;
	}

	if(msdc_ctl->total_size > 64*1024){
		msdc_ctl->result = -1;
		return  msdc_ctl->result;
	}

	memset(&msdc_data, 0, sizeof(struct mmc_data));
	memset(&msdc_mrq, 0, sizeof(struct mmc_request));
	memset(&msdc_cmd, 0, sizeof(struct mmc_command));
	memset(&msdc_stop, 0, sizeof(struct mmc_command));

	msdc_mrq.cmd = &msdc_cmd;
	msdc_mrq.data = &msdc_data;

	if(msdc_ctl->trans_type)
		dma_force[host_ctl->id] = FORCE_IN_DMA;
	else
		dma_force[host_ctl->id] = FORCE_IN_PIO;

	if (msdc_ctl->iswrite){
		msdc_data.flags = MMC_DATA_WRITE;
		msdc_cmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
		msdc_data.blocks = msdc_ctl->total_size / 512;
		if (MSDC_CARD_DUNM_FUNC != msdc_ctl->opcode) {
            if (copy_from_user(sg_msdc_multi_buffer, msdc_ctl->buffer, msdc_ctl->total_size)){
                dma_force[host_ctl->id] = FORCE_NOTHING;
                return -EFAULT; 
            }
		} else {
			/* called from other kernel module */
			memcpy(sg_msdc_multi_buffer, msdc_ctl->buffer, msdc_ctl->total_size);
		}
	} else {
		msdc_data.flags = MMC_DATA_READ;
		msdc_cmd.opcode = MMC_READ_MULTIPLE_BLOCK;
		msdc_data.blocks = msdc_ctl->total_size / 512;
		memset(sg_msdc_multi_buffer, 0 , msdc_ctl->total_size);
	}

	msdc_cmd.arg = msdc_ctl->address;

	BUG_ON(!host_ctl->mmc->card);
	if (!mmc_card_blockaddr(host_ctl->mmc->card)){
		printk("this device use byte address!!\n");
		msdc_cmd.arg <<= 9;
	}
	msdc_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	msdc_stop.opcode = MMC_STOP_TRANSMISSION;
	msdc_stop.arg = 0;
	msdc_stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;

	msdc_data.stop = &msdc_stop;
	msdc_data.blksz = 512;
	msdc_data.sg = &msdc_sg;
	msdc_data.sg_len = 1;

#if DEBUG_MMC_IOCTL
	printk("total size is %d\n",msdc_ctl->total_size);
#endif
	sg_init_one(&msdc_sg, sg_msdc_multi_buffer, msdc_ctl->total_size);
	mmc_set_data_timeout(&msdc_data, host_ctl->mmc->card);
	mmc_wait_for_req(host_ctl->mmc, &msdc_mrq);

	if (!msdc_ctl->iswrite){
		if (MSDC_CARD_DUNM_FUNC != msdc_ctl->opcode) {
            if (copy_to_user(msdc_ctl->buffer, sg_msdc_multi_buffer, msdc_ctl->total_size)){
                dma_force[host_ctl->id] = FORCE_NOTHING;
                return -EFAULT;
                }
		} else {
			/* called from other kernel module */
			memcpy(msdc_ctl->buffer, sg_msdc_multi_buffer, msdc_ctl->total_size);
		}
	}

	if (msdc_ctl->partition){
		mmc_send_ext_csd(host_ctl->mmc->card,l_buf);

		if (l_buf[179] & 0x8) {
			/* set back to access user area */
			l_buf[179] &= ~0x7;
			l_buf[179] |= 0x0;
			mmc_switch(host_ctl->mmc->card, 0, 179, l_buf[179]);
		}
	}

	mmc_release_host(host_ctl->mmc);

	if (msdc_cmd.error)
		msdc_ctl->result = msdc_cmd.error;

	if (msdc_data.error){
		msdc_ctl->result = msdc_data.error;
	} else {
		msdc_ctl->result = 0;
	}
        dma_force[host_ctl->id] = FORCE_NOTHING;
	return msdc_ctl->result;

}

static int simple_mt6575_sd_ioctl_get_cid(struct msdc_ioctl* msdc_ctl)
{
    struct msdc_host *host_ctl;
   
    host_ctl = msdc_6575_host[msdc_ctl->host_num];

	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);
	BUG_ON(!host_ctl->mmc->card);

#if DEBUG_MMC_IOCTL
	printk("user want the cid in msdc slot%d \n",msdc_ctl->host_num);
#endif

    if (copy_to_user(msdc_ctl->buffer, &host_ctl->mmc->card->raw_cid, 16))
        return -EFAULT;

#if DEBUG_MMC_IOCTL
	printk("cid:0x%x,0x%x,0x%x,0x%x\n",host_ctl->mmc->card->raw_cid[0],
			host_ctl->mmc->card->raw_cid[1],
			host_ctl->mmc->card->raw_cid[2],
			host_ctl->mmc->card->raw_cid[3]);
#endif
	return 0;

}

static int simple_mt6575_sd_ioctl_get_csd(struct msdc_ioctl* msdc_ctl)
{
    struct msdc_host *host_ctl;
   
    host_ctl = msdc_6575_host[msdc_ctl->host_num];

	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);
	BUG_ON(!host_ctl->mmc->card);

#if DEBUG_MMC_IOCTL
	printk("user want the csd in msdc slot%d \n",msdc_ctl->host_num);
#endif

    if (copy_to_user(msdc_ctl->buffer, &host_ctl->mmc->card->raw_csd, 16))
        return -EFAULT;

#if DEBUG_MMC_IOCTL
	printk("csd:0x%x,0x%x,0x%x,0x%x\n",host_ctl->mmc->card->raw_csd[0],
			host_ctl->mmc->card->raw_csd[1],
			host_ctl->mmc->card->raw_csd[2],
			host_ctl->mmc->card->raw_csd[3]);
#endif
	return 0;

}

static int simple_mt6575_sd_ioctl_get_excsd(struct msdc_ioctl* msdc_ctl)
{
    char l_buf[512];
    struct msdc_host *host_ctl;
   
    host_ctl = msdc_6575_host[msdc_ctl->host_num];

	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);
	BUG_ON(!host_ctl->mmc->card);

	mmc_claim_host(host_ctl->mmc);

#if DEBUG_MMC_IOCTL
	printk("user want the extend csd in msdc slot%d \n",msdc_ctl->host_num);
#endif
	mmc_send_ext_csd(host_ctl->mmc->card,l_buf);
    if (copy_to_user(msdc_ctl->buffer, l_buf, 512))
        return -EFAULT;

#if DEBUG_MMC_IOCTL
	int i;
	for (i = 0; i < 512; i++)
	{
		printk("%x", l_buf[i]);
		if (0 == ((i + 1) % 16)){
			printk("\n");
		}
	}
#endif

    if (copy_to_user(msdc_ctl->buffer, l_buf, 512))
        return -EFAULT;

    mmc_release_host(host_ctl->mmc);

    return 0;

}

int __simple_mt6575_sd_ioctl_get_excsd(struct msdc_ioctl* msdc_ctl,u8 *excsd)
{
    char l_buf[512];
    struct msdc_host *host_ctl = NULL;
printk("in %s\n", __func__);
   
    host_ctl = msdc_6575_host[msdc_ctl->host_num];
printk("host_ctl = %p\n", host_ctl);
printk("msdc_ctl->host_num = %d\n", msdc_ctl->host_num);
printk("host_ctl->mmc = %p\n", host_ctl->mmc);
printk("host_ctl->mmc->card = %p\n", host_ctl->mmc->card);

	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);
	BUG_ON(!host_ctl->mmc->card);

printk("before mmc_claim_host\n");

	mmc_claim_host(host_ctl->mmc);

//#if DEBUG_MMC_IOCTL
	printk("user want the extend csd in msdc slot%d \n",msdc_ctl->host_num);
//#endif
	mmc_send_ext_csd(host_ctl->mmc->card,l_buf);
    //if (copy_to_user(msdc_ctl->buffer, l_buf, 512))
        //return -EFAULT;
        memcpy(excsd, l_buf, 512);

//#if DEBUG_MMC_IOCTL
	int i;
	for (i = 0; i < 512; i++)
	{
		printk("0x%x ", l_buf[i]);
		if (0 == ((i + 1) % 16)){
			printk("\n");
		}
	}

    printk("boot = %d", l_buf[226]);
    printk("rpmb = %d", l_buf[168]);
    printk("usersize %d %d %d %d", l_buf[215], l_buf[214], l_buf[213], l_buf[212]);
//#endif

    mmc_release_host(host_ctl->mmc);

    return 0;
}

EXPORT_SYMBOL(__simple_mt6575_sd_ioctl_get_excsd);


static int simple_mt6575_sd_ioctl_set_driving(struct msdc_ioctl* msdc_ctl)
{
    u32 base;
#if DEBUG_MMC_IOCTL    
    unsigned int l_value;
#endif 

    if (msdc_ctl->host_num == 0){
#ifndef CFG_DEV_MSDC0
        printk("host%d is not config\n", msdc_ctl->host_num);
        return -1;
#endif
    } else if (msdc_ctl->host_num == 1) {
#ifndef CFG_DEV_MSDC1
        printk("host%d is not config\n", msdc_ctl->host_num);
        return -1;
#endif
    } else if (msdc_ctl->host_num == 2) {
#ifndef CFG_DEV_MSDC2
        printk("host%d is not config\n", msdc_ctl->host_num);
        return -1;
#endif
    } else if (msdc_ctl->host_num == 3) {
#ifndef CFG_DEV_MSDC3
        printk("host%d is not config\n", msdc_ctl->host_num);
        return -1;
#endif
    }

    base = msdc_6575_host[msdc_ctl->host_num]->base;

    enable_clock(15 + msdc_6575_host[msdc_ctl->host_num]->id, "SD"); 

#if DEBUG_MMC_IOCTL
    printk("set: clk pull down driving is 0x%x\n", msdc_ctl->clk_pd_driving);
    printk("set: clk pull up driving is 0x%x\n", msdc_ctl->clk_pu_driving);
    printk("set: cmd pull down driving is 0x%x\n", msdc_ctl->cmd_pd_driving);
    printk("set: cmd pull up driving is 0x%x\n", msdc_ctl->cmd_pu_driving);
    printk("set: dat pull down driving is 0x%x\n", msdc_ctl->dat_pd_driving);
    printk("set: dat pull up driving is 0x%x\n", msdc_ctl->dat_pu_driving);
#endif

    sdr_set_field(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKDRVN, msdc_ctl->clk_pd_driving);
    sdr_set_field(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKDRVP, msdc_ctl->clk_pu_driving);
    sdr_set_field(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDDRVN, msdc_ctl->cmd_pd_driving);
    sdr_set_field(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDDRVP, msdc_ctl->cmd_pu_driving);
    sdr_set_field(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATDRVN, msdc_ctl->dat_pd_driving);
    sdr_set_field(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATDRVP, msdc_ctl->dat_pu_driving);

#if DEBUG_MMC_IOCTL
    l_value = 0;
    sdr_get_field(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKDRVN, l_value);
    printk("read: clk pull down driving is 0x%x\n", l_value);
    l_value = 0;
    sdr_get_field(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKDRVP, l_value);
    printk("read: clk pull up driving is 0x%x\n", l_value);
    l_value = 0;
    sdr_get_field(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDDRVN, l_value);
    printk("read: cmd pull down driving is 0x%x\n", l_value);
    l_value = 0;
    sdr_get_field(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDDRVP, l_value);
    printk("read: cmd pull up driving is 0x%x\n", l_value);
    l_value = 0;
    sdr_get_field(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATDRVN, l_value);
	printk("read: dat pull down driving is 0x%x\n", l_value);
    l_value = 0;
    sdr_get_field(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATDRVP, l_value);
	printk("read: dat pull up driving is 0x%x\n", l_value);
#endif

    return 0;
}

static int simple_mt6575_sd_ioctl_get_driving(struct msdc_ioctl* msdc_ctl)
{
    u32 base;
    unsigned int l_value;

    
    if (msdc_ctl->host_num == 0){
#ifndef CFG_DEV_MSDC0
    	  printk("host%d is not config\n", msdc_ctl->host_num);
        return -1;
#endif
    } else if (msdc_ctl->host_num == 1) {
#ifndef CFG_DEV_MSDC1
    	  printk("host%d is not config\n", msdc_ctl->host_num);
        return -1;
#endif
    } else if (msdc_ctl->host_num == 2) {
#ifndef CFG_DEV_MSDC2
    	  printk("host%d is not config\n", msdc_ctl->host_num);
        return -1;
#endif
    } else if (msdc_ctl->host_num == 3) {
#ifndef CFG_DEV_MSDC3
    	  printk("host%d is not config\n", msdc_ctl->host_num);
        return -1;
#endif
    }

    base = msdc_6575_host[msdc_ctl->host_num]->base;
    
    enable_clock(15 + msdc_6575_host[msdc_ctl->host_num]->id, "SD"); 
    
    sdr_get_field(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKDRVN, l_value);
    msdc_ctl->clk_pd_driving = l_value;

    sdr_get_field(MSDC_PAD_CTL0, MSDC_PAD_CTL0_CLKDRVP, l_value);
    msdc_ctl->clk_pu_driving = l_value;

    sdr_get_field(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDDRVN, l_value);
    msdc_ctl->cmd_pd_driving = l_value;

    sdr_get_field(MSDC_PAD_CTL1, MSDC_PAD_CTL1_CMDDRVP, l_value);
    msdc_ctl->cmd_pu_driving = l_value;

    sdr_get_field(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATDRVN, l_value);
    msdc_ctl->dat_pd_driving = l_value;

    sdr_get_field(MSDC_PAD_CTL2, MSDC_PAD_CTL2_DATDRVP, l_value);
    msdc_ctl->dat_pu_driving = l_value;


#if DEBUG_MMC_IOCTL
	printk("read: clk pull down driving is 0x%x\n", msdc_ctl->clk_pd_driving);
	printk("read: clk pull up driving is 0x%x\n", msdc_ctl->clk_pu_driving);
	printk("read: cmd pull down driving is 0x%x\n", msdc_ctl->cmd_pd_driving);
	printk("read: cmd pull up driving is 0x%x\n", msdc_ctl->cmd_pu_driving);
	printk("read: dat pull down driving is 0x%x\n", msdc_ctl->dat_pd_driving);
	printk("read: dat pull up driving is 0x%x\n", msdc_ctl->dat_pu_driving);
#endif

    return 0;
}

static long simple_mt6575_sd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct msdc_ioctl *msdc_ctl = (struct msdc_ioctl *)arg;

    switch (msdc_ctl->opcode){
        case MSDC_SINGLE_READ_WRITE:
            msdc_ctl->result = simple_mt6575_sd_ioctl_single_rw(msdc_ctl);
            break;
        case MSDC_MULTIPLE_READ_WRITE:
            msdc_ctl->result = simple_mt6575_sd_ioctl_multi_rw(msdc_ctl);
            break;
        case MSDC_GET_CID:
            msdc_ctl->result = simple_mt6575_sd_ioctl_get_cid(msdc_ctl);
            break;
        case MSDC_GET_CSD:
            msdc_ctl->result = simple_mt6575_sd_ioctl_get_csd(msdc_ctl);
            break;
        case MSDC_GET_EXCSD:
            msdc_ctl->result = simple_mt6575_sd_ioctl_get_excsd(msdc_ctl);
            break;
        case MSDC_DRIVING_SETTING:
            printk("in ioctl to change driving\n");
            if (1 == msdc_ctl->iswrite){
                msdc_ctl->result = simple_mt6575_sd_ioctl_set_driving(msdc_ctl);
            } else {
                msdc_ctl->result = simple_mt6575_sd_ioctl_get_driving(msdc_ctl);
            }
            break;
        default:
            printk("mt6575_sd_ioctl:this opcode value is illegal!!\n");
            return -EINVAL;
    }

    return msdc_ctl->result;
}

static struct file_operations simple_msdc_em_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl	= simple_mt6575_sd_ioctl,
	.open           = simple_mt6575_sd_open,
};

static struct miscdevice simple_msdc_em_dev[] = {
    {
        .minor	= MISC_DYNAMIC_MINOR,
        .name	= "misc-mt6575-sd0",
        .fops	= &simple_msdc_em_fops,
    },
    {
        .minor	= MISC_DYNAMIC_MINOR,
        .name	= "misc-mt6575-sd1",
        .fops	= &simple_msdc_em_fops,
    },
    {
        .minor	= MISC_DYNAMIC_MINOR,
        .name	= "misc-mt6575-sd2",
        .fops	= &simple_msdc_em_fops,
    },
    {
        .minor	= MISC_DYNAMIC_MINOR,
        .name	= "misc-mt6575-sd3",
        .fops	= &simple_msdc_em_fops,
    },
};


static int simple_mt6575_sd_probe(struct platform_device *pdev)
{
    int ret = 0;

    printk("in misc_mt6575_sd_probe function\n");

    return ret;
}

static int simple_mt6575_sd_remove(struct platform_device *pdev)
{
    return 0;
}


static struct platform_driver simple_mt6575_sd_driver = {
    .probe   = simple_mt6575_sd_probe,
    .remove  = simple_mt6575_sd_remove,

    .driver  = {
        .name  = DRV_NAME_MISC,
        .owner = THIS_MODULE,
    },
};

static int __init simple_mt6575_sd_init(void)
{
    int ret;

    sg_msdc_multi_buffer =(u32*)kmalloc(64*1024,GFP_KERNEL);
    if( sg_msdc_multi_buffer ==NULL)
        printk("allock 64KB memory failed\n");


    ret = platform_driver_register(&simple_mt6575_sd_driver);
    if (ret) {
        printk(KERN_ERR DRV_NAME_MISC ": Can't register driver\n");
        return ret;
    }
    printk(KERN_INFO DRV_NAME_MISC ": MediaTek mt6575 SD/MMC Card Driver\n");

    /*msdc0 is for emmc only, just for emmc*/
    //ret = misc_register(&simple_msdc_em_dev[host->id]);
    ret = misc_register(&simple_msdc_em_dev[0]);
    if (ret) {
        printk("register MSDC Slot[0] misc driver failed (%d)\n",ret);
        return ret;
    }

    return 0;
}

static void __exit simple_mt6575_sd_exit(void)
{
    if (sg_msdc_multi_buffer != NULL){
        kfree(sg_msdc_multi_buffer);
        sg_msdc_multi_buffer = NULL;
    }

    misc_deregister(&simple_msdc_em_dev[0]);
    
    platform_driver_unregister(&simple_mt6575_sd_driver);
}

#ifdef MTK_EMMC_SUPPORT
static int mmc_claim_host_simple(struct mmc_host *host, char *abort)
{
    host->claimed = 1;
    host->claimer = current;
    host->claim_cnt += 1;

    return 0;
}

static void mmc_do_release_host_simple(struct mmc_host *host)
{
    if (--host->claim_cnt) {
    } else {
        host->claimed = 0;
        host->claimer = NULL;
    }
}

    static void
mmc_start_request_simple(struct mmc_host *host, struct mmc_request *mrq)
{
    mrq->cmd->error = 0;
    mrq->cmd->mrq = mrq;
    if (mrq->data) {

        mrq->cmd->data = mrq->data;
        mrq->data->error = 0;
        mrq->data->mrq = mrq;
        if (mrq->stop) {
            mrq->data->stop = mrq->stop;
            mrq->stop->error = 0;
            mrq->stop->mrq = mrq;
        }
    }

    mt6575_sd_request(host, mrq);
}

static int simple_mt6575_sd_stop(struct msdc_ioctl* msdc_ctl)
{
	struct mmc_command msdc_cmd;
	struct mmc_request msdc_mrq;
    struct msdc_host *host_ctl;

    host_ctl = msdc_6575_host[msdc_ctl->host_num];

    mmc_claim_host_simple(host_ctl->mmc, NULL);

	if(msdc_ctl->total_size > 512){
		msdc_ctl->result = -1;
		return  msdc_ctl->result;
	}

	memset(&msdc_mrq, 0, sizeof(struct mmc_request));
	memset(&msdc_cmd, 0, sizeof(struct mmc_command));

	msdc_mrq.cmd = &msdc_cmd;
	msdc_mrq.data = NULL;

    dma_force[host_ctl->id] = FORCE_IN_PIO;

    msdc_cmd.opcode = MMC_STOP_TRANSMISSION;
	msdc_cmd.arg = 0;

	BUG_ON(!host_ctl->mmc->card);

	msdc_cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1B | MMC_CMD_AC;

    mmc_start_request_simple(host_ctl->mmc, &msdc_mrq);

    mmc_do_release_host_simple(host_ctl->mmc);
	
    if (msdc_cmd.error)
		msdc_ctl->result= msdc_cmd.error;

	return msdc_ctl->result;
}


static int simple_mt6575_sd_status(struct msdc_ioctl* msdc_ctl)
{
	struct mmc_command msdc_cmd;
	struct mmc_request msdc_mrq;
    struct msdc_host *host_ctl;

    host_ctl = msdc_6575_host[msdc_ctl->host_num];

    mmc_claim_host_simple(host_ctl->mmc, NULL);


	if(msdc_ctl->total_size > 512){
		msdc_ctl->result = -1;
		return  msdc_ctl->result;
	}

	memset(&msdc_mrq, 0, sizeof(struct mmc_request));
	memset(&msdc_cmd, 0, sizeof(struct mmc_command));

	msdc_mrq.cmd = &msdc_cmd;
	msdc_mrq.data = NULL;

    dma_force[host_ctl->id] = FORCE_IN_PIO;

    msdc_cmd.opcode = MMC_SEND_STATUS;
	msdc_cmd.arg = host_ctl->mmc->card->rca << 16;

	BUG_ON(!host_ctl->mmc->card);

	msdc_cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;

    mmc_start_request_simple(host_ctl->mmc, &msdc_mrq);

    mmc_do_release_host_simple(host_ctl->mmc);
	
    //if (msdc_cmd.error)
	//	msdc_ctl->result= msdc_cmd.error;
	msdc_ctl->result= msdc_cmd.resp[0];

	return msdc_ctl->result;
}


static int simple_mt6575_sd_panic_dump_single_w(struct msdc_ioctl* msdc_ctl)
{
    struct scatterlist msdc_sg;
	struct mmc_data    msdc_data;
	struct mmc_command msdc_cmd;
	struct mmc_request msdc_mrq;
    struct msdc_host *host_ctl;

    host_ctl = msdc_6575_host[msdc_ctl->host_num];

    mmc_claim_host_simple(host_ctl->mmc, NULL);


	if(msdc_ctl->total_size > 512){
		msdc_ctl->result = -1;
		return  msdc_ctl->result;
	}

	memset(&msdc_data, 0, sizeof(struct mmc_data));
	memset(&msdc_mrq, 0, sizeof(struct mmc_request));
	memset(&msdc_cmd, 0, sizeof(struct mmc_command));

	msdc_mrq.cmd = &msdc_cmd;
	msdc_mrq.data = &msdc_data;

    dma_force[host_ctl->id] = FORCE_IN_PIO;

    msdc_data.flags = MMC_DATA_WRITE;
    msdc_cmd.opcode = MMC_WRITE_BLOCK;
    msdc_data.blocks = msdc_ctl->total_size / 512;
    if (MSDC_CARD_DUNM_FUNC != msdc_ctl->opcode) {
        if (copy_from_user(sg_msdc_multi_buffer, msdc_ctl->buffer, 512))
            return -EFAULT;
    } else {
        /* called from other kernel module */
        memcpy(sg_msdc_multi_buffer, msdc_ctl->buffer, 512);
    }

	msdc_cmd.arg = msdc_ctl->address;

	BUG_ON(!host_ctl->mmc->card);
	if (!mmc_card_blockaddr(host_ctl->mmc->card)){
		printk("the device is used byte address!\n");
		msdc_cmd.arg <<= 9;
	}

	msdc_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	msdc_data.stop = NULL;
	msdc_data.blksz = 512;
	msdc_data.sg = &msdc_sg;
	msdc_data.sg_len = 1;

	sg_init_one(&msdc_sg, sg_msdc_multi_buffer, msdc_ctl->total_size);

    mmc_start_request_simple(host_ctl->mmc, &msdc_mrq);

    mmc_do_release_host_simple(host_ctl->mmc);
	
    if (msdc_cmd.error)
		msdc_ctl->result= msdc_cmd.error;

	if (msdc_data.error)
		msdc_ctl->result= msdc_data.error;
	else
		msdc_ctl->result= 0;

	return msdc_ctl->result;
}

//#define PART_NUM  (18)
#define DUMP_INTO_CARD_TYPE_MASK     (0xFFFF0000)
#define DUMP_INTO_SLOT_NUM_MASK      (0xFFFF)
#define DUMP_INTO_SD_CARD            (0x00000000)
#define DUMP_INTO_MMC_CARD           (0x00010000)
#define SD_FALSE             -1
#define SD_TRUE               0


static int emmc_dump_read(unsigned char* buf, unsigned int len, unsigned int offset,unsigned int slot)
{
	/* maybe delete in furture */
	struct msdc_ioctl     msdc_ctl;
    unsigned int i;
	unsigned int l_user_begin_num;
	unsigned int l_dest_num;
	unsigned int l_start_offset;
	unsigned int ret = SD_FALSE;
	//unsigned int l_tmp = PART_NUM;

	if ((0 != slot) || (0 != offset % 512) || (0 != len % 512)) {
		/* emmc always in slot0 */
		printk("debug: slot is not use for emmc!\n");
		return ret;
	}

	/* find the offset in emmc */
	for (i = 0; i < PART_NUM; i++) {
	//for (i = 0; i < 1; i++) {
		if ('m' == *(PartInfoEmmc[i].name) && 'b' == *(PartInfoEmmc[i].name + 1) &&
				'r' == *(PartInfoEmmc[i].name + 2)){
			l_user_begin_num = i;
		}

		if ('e' == *(PartInfoEmmc[i].name) && 'x' == *(PartInfoEmmc[i].name + 1) &&
				'p' == *(PartInfoEmmc[i].name + 2) && 'd' == *(PartInfoEmmc[i].name + 3) &&
				'b' == *(PartInfoEmmc[i].name + 4)){
			l_dest_num = i;
		}
	}

#if DEBUG_MMC_IOCTL
	printk("l_user_begin_num = %d l_dest_num = %d\n",l_user_begin_num,l_dest_num);
#endif

	if (l_user_begin_num >= PART_NUM && l_dest_num >= PART_NUM) {
		printk("not find in scatter file error!\n");
		return ret;
	}

	if (PartInfoEmmc[l_dest_num].size < (len + offset)) {
		printk("read operation oversize!\n");
		return ret;
	}

#if DEBUG_MMC_IOCTL
    printk("read start address=0x%x\n", PartInfoEmmc[l_dest_num].start_address - PartInfoEmmc[l_user_begin_num].start_address);
#endif 
	l_start_offset = offset + PartInfoEmmc[l_dest_num].start_address - PartInfoEmmc[l_user_begin_num].start_address;

	msdc_ctl.partition = 0;
	msdc_ctl.iswrite = 0;
	msdc_ctl.host_num = slot;
	msdc_ctl.opcode = MSDC_CARD_DUNM_FUNC;
	msdc_ctl.total_size = 512;
	msdc_ctl.trans_type = 0;
	for (i = 0; i < (len/512); i++) {
		/* code */
		msdc_ctl.address = l_start_offset / 512 + i; //blk address
        msdc_ctl.buffer =(u32*)(buf + i * 512);

#if DEBUG_MMC_IOCTL
		printk("l_start_offset = %d\n", msdc_ctl.address);
#endif        
		msdc_ctl.result = simple_mt6575_sd_ioctl_single_rw(&msdc_ctl);
	}
    
#if DEBUG_MMC_IOCTL
    printk("read data:");
    for (i = 0; i < 32; i++) {
        printk("0x%x", buf[i]);
        if (0 == (i+1)%32)
            printk("\n");
    }
#endif
	return 0;
}

static int mt6575_emmc_dump_write(unsigned char* buf, unsigned int len, unsigned int offset,unsigned int slot)
{
	/* maybe delete in furture */
	struct msdc_ioctl     msdc_ctl;
    unsigned int i;
	unsigned int l_user_begin_num;
	unsigned int l_dest_num;
	unsigned int l_start_offset;
	unsigned int ret = SD_FALSE;

    static int sg_test = 0;

	if ((0 != slot) || (0 != offset % 512) || (0 != len % 512)) {
		/* emmc always in slot0 */
		printk("debug: slot is not use for emmc!\n");
		return ret;
	}

#if DEBUG_MMC_IOCTL
    printk("write data:");
    for (i = 0; i < 32; i++) {
        printk("0x%x", buf[i]);
        if (0 == (i+1)%32)
            printk("\n");
    }
#endif



	/* find the offset in emmc */
	for (i = 0; i < PART_NUM; i++) {
		if ('m' == *(PartInfoEmmc[i].name) && 'b' == *(PartInfoEmmc[i].name + 1) &&
				'r' == *(PartInfoEmmc[i].name + 2)){
			l_user_begin_num = i;
		}

		if ('e' == *(PartInfoEmmc[i].name) && 'x' == *(PartInfoEmmc[i].name + 1) &&
				'p' == *(PartInfoEmmc[i].name + 2) && 'd' == *(PartInfoEmmc[i].name + 3) &&
				'b' == *(PartInfoEmmc[i].name + 4)){
			l_dest_num = i;
		}
	}

	if (l_user_begin_num >= PART_NUM && l_dest_num >= PART_NUM) {
		printk("not find in scatter file error!\n");
		return ret;
	}

	if (PartInfoEmmc[l_dest_num].size < (len + offset)) {
		printk("write operation oversize!\n");
		return ret;
	}

#if DEBUG_MMC_IOCTL
    printk("write start address=0x%x\n", PartInfoEmmc[l_dest_num].start_address - PartInfoEmmc[l_user_begin_num].start_address);
#endif 

	l_start_offset = offset + PartInfoEmmc[l_dest_num].start_address - PartInfoEmmc[l_user_begin_num].start_address;
	msdc_ctl.partition = 0;
	msdc_ctl.iswrite = 1;
	msdc_ctl.host_num = slot;
	msdc_ctl.opcode = MSDC_CARD_DUNM_FUNC;
	msdc_ctl.total_size = 512;
	msdc_ctl.trans_type = 0;
    if (sg_test == 0) {
        sg_test = 1;

        msdc_ctl.result = simple_mt6575_sd_stop(&msdc_ctl);
        do{
            msdc_ctl.result = simple_mt6575_sd_status(&msdc_ctl);
            printk("card state: %d\n", (msdc_ctl.result & 0x1f00)>>9);
        } while( ((msdc_ctl.result & 0x1f00)>>9) != 4);
    }
	for (i = 0; i < (len/512); i++) {
		/* code */
		msdc_ctl.address = l_start_offset / 512 + i; //blk address
        msdc_ctl.buffer = (u32*)(buf + i * 512);

#if DEBUG_MMC_IOCTL
		printk("l_start_offset = %d\n", msdc_ctl.address);
#endif        
		msdc_ctl.result = simple_mt6575_sd_panic_dump_single_w(&msdc_ctl);
	}

	return 0;
}
#endif

int card_dump_func_read(unsigned char* buf, unsigned int len, unsigned int offset, unsigned int dev)
{
#ifdef MTK_EMMC_SUPPORT
	unsigned int l_slot;
	unsigned int ret = SD_FALSE;

	l_slot = dev & DUMP_INTO_SLOT_NUM_MASK;
	if (l_slot > 3){
		printk("big slot number, error!\n");
		return ret;
	}

	if (DUMP_INTO_MMC_CARD == (dev & DUMP_INTO_CARD_TYPE_MASK)) {
		/* emmc card */
		ret = emmc_dump_read(buf, len, offset, l_slot);
	} else if(DUMP_INTO_SD_CARD == (dev & DUMP_INTO_CARD_TYPE_MASK)) {
		/* not support now */
	} else {
		printk("unknown card type, error!\n");
        return ret;
	}
    return SD_TRUE;
#else
    return 0;
#endif
}

int card_dump_func_write(unsigned char* buf, unsigned int len, unsigned int offset, unsigned int dev)
{
#ifdef MTK_EMMC_SUPPORT
	unsigned int l_slot;
	unsigned int ret = SD_FALSE;

	l_slot = dev & DUMP_INTO_SLOT_NUM_MASK;
	if (l_slot > 3){
		printk("big slot number, error!\n");
		return ret;
	}

	if (DUMP_INTO_MMC_CARD == (dev & DUMP_INTO_CARD_TYPE_MASK)) {
		/* emmc card */
		printk("in card_dump_func_write func!\n");
		ret = mt6575_emmc_dump_write(buf, len, offset, l_slot);
	} else if(DUMP_INTO_SD_CARD == (dev & DUMP_INTO_CARD_TYPE_MASK)) {
		/* not support now */
	} else {
		printk("unknown card type, error!\n");
        return ret;
	}
    return SD_TRUE;
#else
    return 0;
#endif
}

#if 0
static void card_dump_func_test(void)
{
	u32 i;
	char l_test_buf[1024];
	memset(l_test_buf, 0, 1024);
	//buf, len, dev, offset
	printk("l_test_buf = 0x%p\n",l_test_buf);
	card_dump_func_read(l_test_buf, 1024, 0, 0x10000);

	printk("read as follow:\n");
	for (i = 0; i < 1024; i++) {
		printk("0x%x", l_test_buf[i]);

		if (((i + 1) % 64) == 0){
			printk("\n");
		}
	}

	//write test
	memset(l_test_buf, 0x5a, 1024);
	card_dump_func_write(l_test_buf, 1024, 0, 0x10000);

	memset(l_test_buf, 0, 1024);
	card_dump_func_read(l_test_buf, 1024, 0, 0x10000);
	for (i = 0; i < 1024; i++) {
		printk("0x%x", l_test_buf[i]);

		if (((i + 1) % 64) == 0){
			printk("\n");
		}
	}

	/* read size not aligned */
	card_dump_func_read(l_test_buf, 1004, 0, 0x10000);

	/* read addterss not aligned */
	card_dump_func_read(l_test_buf, 1024, 3, 0x10000);

	/* read size beyond the partition size */
	card_dump_func_read(l_test_buf, 1004, 3, 0x10000);

	/* write size not aligned */
	card_dump_func_write(l_test_buf, 1004, 0, 0x10000);

	/* write addterss not aligned */
	card_dump_func_write(l_test_buf, 1024, 3, 0x10000);

	/* write size beyond the partition size */
	card_dump_func_write(l_test_buf, 1004, 3, 0x10000);
}
#endif

EXPORT_SYMBOL(card_dump_func_read);
EXPORT_SYMBOL(card_dump_func_write);


module_init(simple_mt6575_sd_init);
module_exit(simple_mt6575_sd_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("simple MediaTek mt6575 SD/MMC Card Driver");
MODULE_AUTHOR("feifei.wang <feifei.wang@mediatek.com>");
