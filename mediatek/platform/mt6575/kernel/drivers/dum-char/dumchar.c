#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... filp_open*/
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>  /*proc*/
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/aio.h>
#include <asm/uaccess.h>   /*set_fs get_fs mm_segment_t*/
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/unistd.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mtd/mtd.h>
#include <linux/autoconf.h>
#include <linux/sched.h>	//show_stack(current,NULL)


#include "part_info.h" /* PART_NUM */
#include "dumchar.h"		/* local definitions */

extern struct excel_info PartInfo[PART_NUM];

struct device;

static struct class *dumchar_class;
static struct device *dumchar_device[PART_NUM];
static struct dumchar_dev *dumchar_devices; /* allocated in dumchar_init */
static struct proc_dir_entry *dumchar_proc_entry=NULL;
static unsigned int USER_offset_Byte=0;
static unsigned int	USER_size_Byte=0;

static struct Region_Info region_info[REGION_NUM];

static unsigned int major;

#if defined(CONFIG_MTK_MTD_NAND)	
extern struct mtd_info *__mtd_next_device(int i);
#endif
#ifdef MTK_EMMC_SUPPORT
extern int __simple_mt6575_sd_ioctl_get_excsd(struct msdc_ioctl* msdc_ctl,u8 *excsd);
#endif
int IsEmmc(){
#ifdef MTK_EMMC_SUPPORT
		return 1;
#else
		return 0;
#endif
}

EXPORT_SYMBOL(IsEmmc);

static int pmt_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int pmt_release(struct inode *inode, struct file *filp)
{
	return 0;
}
static int pmt_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;    

	switch (cmd)
	{
		case PMT_READ:
			printk(KERN_INFO "PMT IOCTL: PMT_READ\n");
			break;
		case PMT_WRITE:
			printk(KERN_INFO "PMT IOCTL: PMT_WRITE\n");
			break;
		default:
			ret = -EINVAL;
	}
	return ret;
}


static struct file_operations pmt_fops = {
    .owner=      THIS_MODULE,
    .ioctl=      pmt_ioctl,
    .open=       pmt_open,
    .release=    pmt_release,
};

static struct miscdevice pmt_dev = {
    .minor   = MISC_DYNAMIC_MINOR,
    .name    = "pmt",
    .fops    = &pmt_fops,
};

static int  part_init_pmt(void)
{
	int err = 0;
	err = misc_register(&pmt_dev);
	if (unlikely(err)){
		printk("PMT failed to register device in part_init_pmt!\n");		
	}
	return err;
}

#ifdef MTK_EMMC_SUPPORT
static int init_region_info(void){
	int result=0;
	struct msdc_ioctl cmd;
	memset(&cmd,0,sizeof(cmd));
	u8 ext_csd[512];
	memset(ext_csd,0,512);
	memset(region_info,0,sizeof(struct Region_Info)*REGION_NUM);
	int i;
	cmd.host_num=0;
	if(__simple_mt6575_sd_ioctl_get_excsd(&cmd,ext_csd))
		printk("[Dumchar]Get excsd Fail!\n");

	for(i=0;i<REGION_NUM;i++){
		region_info[i].region=i;
		switch (region_info[i].region) {
			case BOOT_PARTITION_1:
				region_info[i].size_Byte=ext_csd[EXT_CSD_BOOT_SIZE_MULT]*128*1024;
				break;
			case BOOT_PARTITION_2:
				region_info[i].size_Byte=ext_csd[EXT_CSD_BOOT_SIZE_MULT]*128*1024;
				break;
			case RPMB_PARTITION:
				region_info[i].size_Byte=ext_csd[EXT_CSD_RPMB_SIZE_MULT]*128*1024;
			default:
				break;
		}
	}

	USER_size_Byte = (ext_csd[212+0]<<0| ext_csd[212+1]<< 8 |ext_csd[212+2]<<16| ext_csd[212+3]<<24)*512;//from emmc spec , only for emmc 4.1-4.41
	printk("[Dumchar]:USER_size_Byte=%u KB\n",USER_size_Byte/1024);

	for(i=0;i<REGION_NUM;i++){
		if(region_info[i].region == BOOT_PARTITION_1 ||region_info[i].region == BOOT_PARTITION_2 ||region_info[i].region == RPMB_PARTITION){
			USER_offset_Byte+=region_info[i].size_Byte;
		}
		printk("[Dumchar]:region %d  size=%d KB \n",region_info[i].region,region_info[i].size_Byte/1024);
	}

	printk("[Dumchar]:USER_offset_Byte=%u KB\n",USER_offset_Byte/1024);

	return 0;	
}
#endif
static int init_sd_cmd(struct msdc_ioctl * cmd,u32 addr,
						u32 __user *buffer, int host_num, int iswrite,
						u32 totalsize, int transtype, PARTITION_ACCESS_T part)
{
	if(!cmd){
		printk("DumChar:no space for msdc_ioctl \n");
		return -EINVAL;
	}
	if(addr <0){
		printk("DumChar:Wrong Address %d for emmc!\n",addr);
		return -EINVAL;
	}

	if(totalsize > MAX_SD_BUFFER){
		printk("DumChar:too mucn bytes for msdc");
		return -EINVAL;
	}	
	memset(cmd, 0,sizeof (struct msdc_ioctl));
	if(addr%512 == 0)
	cmd->address = addr/512;
	else {
		printk("DumChar: Wrong Address\n");
		return -EINVAL;
	}
	//cmd->address =0x100000;
	cmd->buffer = buffer;
	cmd->clock_freq = 0;
	//cmd->cmd_driving =-1;
	//cmd->dat_driving = -1;
	cmd->host_num = host_num;
	cmd->iswrite = iswrite;
	cmd->result = -1;
	cmd->trans_type =transtype;
	cmd->total_size = totalsize;
	cmd->partition = part;
	
	/*	cmdtype:MSDC_SINGLE_READ_WRITE  while MAX_SD_BUFFER >totalsize >512 byte;
			MSDC_MULTIPLE_READ_WRITE   while totalsize <=512 byte;
	*/
	if(totalsize<=512)
	cmd->opcode = MSDC_SINGLE_READ_WRITE;
	else
	cmd->opcode = MSDC_MULTIPLE_READ_WRITE;	
	return 0; 

}

//count<MAX_SD_BUFFER
static ssize_t sd_single_read(struct file *filp,char __user *buf,size_t count,loff_t addr,PARTITION_ACCESS_T part){
	struct dumchar_dev *dev = filp->private_data;
	struct msdc_ioctl cmd; 
	int result;

	printk("DumCharDebug: in  sd_single_read\n");
	
	/*mt6573_sd0 host0 Todo: Only support host 0*/
	if(init_sd_cmd(&cmd,(u32)addr,(u32 *)buf, 0, 0,count,0,part)){
		printk("DumChar:init sd_cmd fail \n");
		return -EINVAL;
	}
			
	if ( dev->actfile->f_op->unlocked_ioctl ) {
		//printk("DumCharDebug: in  sd_single_read unlocked_ioctl\n");
		result = dev->actfile->f_op->unlocked_ioctl( dev->actfile, 1, (unsigned long )&cmd);
		//printk("DumCharDebug: out  sd_single_read unlocked_ioctl result =%d\n",result);
	} else if ( dev->actfile->f_op->ioctl ) {
		//printk("DumCharDebug: in  sd_single_read ioctl\n");
		result= dev->actfile->f_op->ioctl( dev->actfile->f_path.dentry->d_inode,dev->actfile, 1, (unsigned long)&cmd);
		//printk("DumCharDebug: out  sd_single_read ioctl\n");
	}

	if(result ==0 )
		result = count;

	//printk("DumCharDebug: out  sd_single_read result=%d\n",result);
	return result;
}

static ssize_t sd_read(struct file *filp, char __user *buf, size_t count,loff_t *pos,PARTITION_ACCESS_T part)
{
	struct dumchar_dev *dev = filp->private_data; 
	size_t total_retlen=0;
	int retlen=0;
	int len;
	loff_t addr = *pos;
	
	
	//printk("DumCharDebug: in  sd_read count=%ld,*pos=%lld,addr=0x%llx start=%d dev->device_info.size=%d\n",count,*pos,addr,dev->device_info.pri.start_address,dev->device_info.size);

	if (*pos - dev->device_info.pri.start_address + count > dev->device_info.size)
		count = dev->device_info.size - (*pos - dev->device_info.pri.start_address);
	

	//printk("DumCharDebug: in  sd_read count=%ld,*pos=%lld,addr=0x%llx start=%d count=%ld\n",count,*pos,addr,dev->device_info.pri.start_address,count);

	if (!count)		
		return 0;	
	if(addr % ALIE_LEN != 0 ||(addr + count)% ALIE_LEN != 0) {
		loff_t startaddr = addr;
		loff_t endaddr = addr + count;
		loff_t startaddr2,endaddr2;
		loff_t buflen;
		char *pbuf;
		char *pbuf2;
		mm_segment_t curr_fs;

		//printk("DumCharDebug: in sd_read, not alie startaddr=0x%llx endaddr=0x%llx\n",startaddr,endaddr);
		
		if(addr % ALIE_LEN != 0 ){
			startaddr= (addr /ALIE_LEN) *ALIE_LEN;	
			//printk("DumCharDebug: in sd_read, addr=0x%llx, not alian addr/512=0x%llx,startaddr=0x%llx\n",addr,addr%512,startaddr);
		}
		
		if((addr + count)% ALIE_LEN != 0){
			endaddr= ((addr + count)/ALIE_LEN +1)*ALIE_LEN;
			//printk("DumCharDebug: in sd_read, addr+count=0x%llx, not alian (addr+conut)/512=0x%llx,endaddr=0x%llx\n",addr + count,(addr + count)%512,endaddr);
		}
		
		buflen = endaddr - startaddr;
		//printk("DumCharDebug: in sd_read, buflen=0x%llx,endaddr=0x%llx,startaddr=0x%llx\n",buflen,endaddr,startaddr);
		startaddr2=startaddr;
		endaddr2=endaddr;
		
		pbuf  = kmalloc(buflen, GFP_KERNEL);
		if (!pbuf) {
			printk("DumChar: malloc buff fail\n");
			return -ENOMEM;
		}
		memset(pbuf, 0, buflen);
		pbuf2=pbuf;
		
		curr_fs = get_fs();
		set_fs(KERNEL_DS);

		while (buflen>0) {
			
			//printk("DumCharDebug: not alien sd_read while buflen= %lld\n",buflen);
			if (buflen> MAX_SD_BUFFER)
				len = MAX_SD_BUFFER;
			else
				len = buflen;
			
			retlen= sd_single_read(filp, pbuf2,len,startaddr2,part);
			
			if (retlen>0) {
				startaddr2 += retlen;
				total_retlen += retlen;
				buflen-= retlen;
				pbuf2 += retlen;
				printk("while retlen > 0 total_retlen=%d\n",(int)total_retlen);
			}else
				break;
		}
		set_fs(curr_fs);

		//printk("out while \n");
		//printk("read complete total_retlen =%d,end=%d,start=%d pbuf=%p,pbuf2=%p\n",(int)total_retlen,(int)endaddr,(int)startaddr,pbuf,pbuf2);
//for debug
#if defined(PrintBuff)
	int iter=0;
	printk("******************************\nGet %d bytes from %d to %d in %s in kernel\n",(int)total_retlen,(int)startaddr,(int)endaddr,dev->dumname);
	for(iter=0;iter<total_retlen;iter++){
			if(iter%16 == 0)
				printk("\n");
			printk(" %02x",pbuf[iter]);

	}
	printk("\n********************************************************************\n");
#endif

		if(total_retlen == (endaddr - startaddr)){
			int n=copy_to_user(buf,pbuf+(addr-startaddr),count);
			if(n!=0){
				printk(KERN_ERR "read fail in DumChar_sd_read\n");				
			}
			total_retlen = count -n;
		}else{
			
			printk(KERN_ERR "read fail DumChar_sd_read!\n");
		}
		
#if defined(PrintBuff)
		printk("******************************\nGet %ld bytes from %d in %s in user:\n",count,(int)*pos,dev->dumname);
			for(iter=0;iter<count;iter++){
					if(iter%16 == 0)
						printk("\n");
					printk(" %02x",buf[iter]);
		
			}
			printk("\n********************************************************************\n");
#endif

		kfree(pbuf);
	}else{
		while (count>0) {
			//printk("DumCharDebug: alien sd_read while count= %lld\n",count);
			if (count > MAX_SD_BUFFER)
				len = MAX_SD_BUFFER;
			else
				len = count;
			retlen= sd_single_read(filp, buf,len,addr,part);
			if (retlen>0) {
				addr += retlen;
				total_retlen += retlen;
				count -= retlen;
				buf += retlen;
			}else
				break;
		}
		
	}
	//pf((__u8 *)addr,60,"sd_read");
	*pos += total_retlen;
	//printk("out static ssize_t sd_read pos=%lld\n",*pos);
	return total_retlen;
} /* mtd_read */


ssize_t dumchar_read (struct file *filp, char __user *buf, size_t count,loff_t *f_pos) {
	struct dumchar_dev *dev = filp->private_data; 
	int result=0,i,errno;
	loff_t pos=0;
	
	//printk("DumChar:in dumchar_read [ori] %s  address=%llx count=%ld\n",dev->dumname,pos,count);
	if(dev->actfile == 0xffffffff){
		printk("[dumchar_read] It is forbidded to access %s with dumchar(/dev/%s), the %s partition is managed by filesystem!\n",dev->dumname,dev->dumname,dev->dumname);
		printk("[dumchar_read] show_stack*************************************\n");
		show_stack(NULL,NULL);
		return -EINVAL;
	}
	if(dev->type!= MTD && dev->type!=SDCARD){
		printk("DumChar:Wrong Dummy device Type %d ,it should be MTD or SDCARD!\n",dev->type);
		return -EINVAL;
	}
		
	if(dev->type == SDCARD){
		pos = *f_pos+dev->device_info.pri.start_address;
		//printk("DumChar:in dumchar_read [ori] %s  address in mmcblk0=%llx,  address in partition=%llx,count=%llx\n",dev->dumname,pos,*f_pos,count);
		switch(dev->device_info.part){
			case USER_PARTITION:
				result = vfs_read(dev->actfile,buf,count,&pos);
				break;
			case BOOT_PARTITION_1:
				result = sd_read(filp,buf,count,&pos,BOOT_PARTITION_1);
				break;				
			default:
				printk("DumChar: Wrong EMMC Region\n");
				return -EINVAL;
		}
		dev->actfile->f_pos = pos -dev->device_info.pri.start_address;
		*f_pos =  pos -dev->device_info.pri.start_address;
	}else{
		result = vfs_read(dev->actfile, buf, count, f_pos);
		dev->actfile->f_pos = *f_pos;
	}
	return result;	
}

static ssize_t sd_single_write(struct file *filp,char __user *buf,size_t count,loff_t addr,PARTITION_ACCESS_T part){
	struct dumchar_dev *dev = filp->private_data;
	struct msdc_ioctl cmd; 
	int result;
	
	/*mt6573_sd0 host0 Todo: Only support host 0*/
	if(init_sd_cmd(&cmd,(u32)addr,(u32 *)buf, 0, 1,count,0,part)){
		printk("DumChar:init sd_cmd fail \n");
		return -EINVAL;
	}
			
	if ( dev->actfile->f_op->unlocked_ioctl ) {
		result = dev->actfile->f_op->unlocked_ioctl( dev->actfile, 1, (unsigned long)&cmd);
	} else if ( dev->actfile->f_op->ioctl ) {
		result= dev->actfile->f_op->ioctl( dev->actfile->f_path.dentry->d_inode,dev->actfile, 1, (unsigned long)&cmd);
	}

	if(result >=0 )
		result = count;
	else
		result = 0;
	
	return result;
}

static ssize_t sd_write(struct file *filp, char __user *buf, size_t count,loff_t *pos,PARTITION_ACCESS_T part)
{
	struct dumchar_dev *dev = filp->private_data; 
	size_t total_retlen=0;
	int retlen=0;
	int len;
	loff_t addr = *pos+dev->device_info.pri.start_address;
	//loff_t addr = *pos+(loff_t)0x200200;
	if (*pos + count > dev->device_info.size)
		count = dev->device_info.size - *pos;
	if (!count)
		return 0;

	while (count>0) {

		if (count > MAX_SD_BUFFER)
			len = MAX_SD_BUFFER;
		else
			len = count;
			retlen= sd_single_write(filp, buf,len,addr,part);
		if (retlen>0) {
			addr += retlen;
			total_retlen += retlen;
			count -= retlen;
			buf += retlen;
		}
	}
	//pf((__u8 *)addr,60,"sd_write");
	*pos += total_retlen;
	return total_retlen;
} /* mtd_read */


ssize_t dumchar_write (struct file *filp, char __user *buf, size_t count,loff_t *f_pos) {
	struct dumchar_dev *dev = filp->private_data; 
	int result=0,i,errno;
	loff_t pos=0;
	
	//printk("DumChar:in dumchar_write [ori] %s  address=%llx count=%llx\n",dev->dumname,pos,count);
	if(dev->actfile == 0xffffffff){
		printk("[dumchar_write] It is forbidded to access %s with dumchar(/dev/%s), the %s partition is managed by filesystem!\n",dev->dumname,dev->dumname,dev->dumname);
		printk("[dumchar_write] show_stack*************************************\n");
		show_stack(NULL,NULL);
		return -EINVAL;
	}
	
	if(dev->type!= MTD && dev->type!=SDCARD){
		printk("DumChar:Wrong Dummy device Type %d ,it should be MTD or SDCARD!\n",dev->type);
		return -EINVAL;
	}
		
	if(dev->type == SDCARD){
		pos = *f_pos+dev->device_info.pri.start_address;
		//printk("DumChar:in dumchar_write [ori] %s  address in mmcblk0=%llx,  address in partition=%llx,count=%llx\n",dev->dumname,pos,*f_pos,count);
		switch(dev->device_info.part){
			case USER_PARTITION:
				result = vfs_write(dev->actfile,buf,count,&pos);
				break;
			case BOOT_PARTITION_1:
				result = sd_write(filp,buf,count,&pos,BOOT_PARTITION_1);
				break;				
			default:
				printk("DumChar: Wrong EMMC Region\n");
				return -EINVAL;
		}
		dev->actfile->f_pos = pos -dev->device_info.pri.start_address;
		*f_pos =  pos -dev->device_info.pri.start_address;
	}else{
		result = vfs_write(dev->actfile, buf, count, f_pos);
		dev->actfile->f_pos = *f_pos;
	}
	return result;	
}

static long dumchar_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct dumchar_dev *dev = filp->private_data;
	int result;
	
	//printk("DumChar: in dumchar_ioctl\n");
	if(dev->actfile == 0xffffffff){
			printk("[dumchar_ioctl] It is forbidded to access %s with dumchar(/dev/%s), the %s partition is managed by filesystem!\n",dev->dumname,dev->dumname,dev->dumname);
			printk("[dumchar_loctl] show_stack*************************************\n");
			show_stack(NULL,NULL);
			return -EINVAL;
		}

	if(!strcmp(dev->dumname,"pmt")){

		switch (cmd)
			{
				case PMT_READ:
					printk(KERN_INFO "PMT IOCTL: PMT_READ\n");
					break;
				case PMT_WRITE:
					printk(KERN_INFO "PMT IOCTL: PMT_WRITE\n");
					break;
				default:
					result= -EINVAL;
			}
			return result;


		
		}else{
		
	
	if(dev->type!= MTD && dev->type!=SDCARD){
		printk("DumChar:Wrong Dummy device Type %d ,it should be MTD or SDCARD!\n",dev->type);
		return -EINVAL;
	}
		
	if(dev->type == MTD){
		if ( dev->actfile->f_op->unlocked_ioctl ) {
			result = dev->actfile->f_op->unlocked_ioctl(dev->actfile, cmd, arg);
		} else if ( dev->actfile->f_op->ioctl ) {
			result= dev->actfile->f_op->ioctl( dev->actfile->f_path.dentry->d_inode,dev->actfile, cmd, arg);
		}
	}else{
		struct mtd_info_user info;
		void __user *argp = (void __user *)arg;
		result = 0;
		u_long size;
		
		size = (cmd & IOCSIZE_MASK) >> IOCSIZE_SHIFT;
		if (cmd & IOC_IN) {
			if (!access_ok(VERIFY_READ, argp, size))
				return -EFAULT;
		}
		if (cmd & IOC_OUT) {
			if (!access_ok(VERIFY_WRITE, argp, size))
				return -EFAULT;
		}

		switch (cmd) {
		case MEMGETREGIONCOUNT:
			break;
		case MEMGETREGIONINFO:
			break;
		case MEMGETINFO:
			info.type	= MTD_NANDFLASH;
			info.flags	= MTD_WRITEABLE;
			info.size	= dev->device_info.size;
			info.erasesize	= 128*1024;
			info.writesize	= 2048;
			info.oobsize	= 0;
			/* The below fields are obsolete */
			info.ecctype	= -1;
			info.eccsize	= 0;
			if (copy_to_user(argp, &info, sizeof(struct mtd_info_user)))
				return -EFAULT;
			break;
		case MEMERASE:
			break;
		case MEMERASE64:
			break;
		case MEMWRITEOOB:
			break;
		case MEMREADOOB:
			break;
		case MEMWRITEOOB64:
			break;
		case MEMREADOOB64:
			break;
		case MEMLOCK:
			break;
		case MEMUNLOCK:
			break;
		case MEMGETOOBSEL:
			break;
		case MEMGETBADBLOCK:
			break;
		case MEMSETBADBLOCK:
			break;
		case ECCGETLAYOUT:
			break;
		case ECCGETSTATS:
			break;
		case MTDFILEMODE:
			break;
		case MTD_MODE_NORMAL:
			break;
		default:
			result = -EINVAL;
		}
	}
	return result;
}
}

loff_t dumchar_llseek (struct file *filp, loff_t off, int whence)
{
	struct dumchar_dev *dev = filp->private_data;
	loff_t newpos;
	
//	printk("DumChar:in dumchar_llseek: off=%d where=%d filp->f_pos=%d, dev->actfile->f_pos=%d\n",(int)off,whence,(int)filp->f_pos,(int)dev->actfile->f_pos);

	//printk("DumChar:in dumchar_llseek %d\n",lseek++);
	if(dev->actfile == 0xffffffff){
		printk("[dumchar_llseek] It is forbidded to access %s with dumchar(/dev/%s), the %s partition is managed by filesystem!\n",dev->dumname,dev->dumname,dev->dumname);
		printk("[dumchar_llseek] show_stack*************************************\n");
		show_stack(NULL,NULL);
		return -EINVAL;
	}
	if(dev->type!= MTD && dev->type!=SDCARD){
		printk("DumChar:Wrong Dummy device Type %d ,it should be MTD or SDCARD!\n",dev->type);
		return -EINVAL;
	}

	if(dev->type == MTD){
		if ( dev->actfile->f_op->llseek ) {
			newpos =  dev->actfile->f_op->llseek( dev->actfile, off,whence);
		} else {
			switch(whence) {
			case SEEK_SET: 
				newpos = off;
				break;
			case SEEK_CUR: 
				newpos = dev->actfile->f_pos + off;
				break;
			case SEEK_END: 
				newpos = dev->device_info.size + off;
				break;
			default: 
				return -EINVAL;
			}
		}
			
	}else {//SDCARD
		switch(whence) {
		case SEEK_SET: 
			newpos = off;
			break;
		case SEEK_CUR: 
			newpos = dev->actfile->f_pos + off;
			break;
		case SEEK_END: 
			newpos = dev->device_info.size + off;
			break;
		default: 
			return -EINVAL;
		}

	}

	if (newpos >= 0 && newpos <= dev->device_info.size){
		dev->actfile->f_pos = newpos;
		filp->f_pos = newpos;
//		printk("DumChar:out dumchar_llseek: off=%d where=%d newpos=%d filp->f_pos=%d, dev->actfile->f_pos=%d\n",(int)off,whence,(int)newpos,(int)filp->f_pos,(int)dev->actfile->f_pos);
		return dev->actfile->f_pos ;
	}

	return -EINVAL;	
}

static int open_for_info(char * PNO){
	char PN[80];
	int RD=0;
	struct file *filp;
	if(!strcmp("preloader",PNO)||!strcmp("dsp_bl",PNO)||!strcmp("uboot",PNO)||!strcmp("bootimg",PNO))
		RD=1;
	
	sprintf(PN,"/dev/%s",PNO);

	//printk( " DumChar:open %s .....\n",PN);

	if(RD)
		filp=filp_open( PN,O_RDONLY,0777);
	else
		filp= filp_open( PN,O_RDWR,0777);

	if (IS_ERR(filp)) {
		printk( " DumChar:open files %s fail !\n",PN);
		return PTR_ERR(filp);
	}

	filp_close(filp,NULL);
	return 0;

}
static int dumchar_proc_read(char *page, char **start, off_t off,int count, int *eof, void *data)
{
	int i,len=0;
#if 0
	for(i=0;i<PART_NUM;i++)
		if(open_for_info(PartInfo[i].name))
			break;
#endif
	len += sprintf(page+len, "PN\t\tSize\tAddr\tTyp\tRN\n");	
	for(i=0;i<PART_NUM;i++){
		if(down_interruptible(&(dumchar_devices[i].sem))){
			printk("DumChar:Get sem  fail in dumchar_proc_read %d\n",dumchar_devices[i].sem);
			return -ERESTARTSYS;
		}
		if(dumchar_devices[i].type == MTD){
		len += sprintf(page+len, "%-10s   0x%08x   0x%08x   %2d   %s\n",
		    dumchar_devices[i].dumname,
		    dumchar_devices[i].device_info.size,
		    dumchar_devices[i].device_info.pri,
		    dumchar_devices[i].type,
		    dumchar_devices[i].device_info.actname);
		up(&(dumchar_devices[i].sem));
		}else{
		if(dumchar_devices[i].device_info.partition_idx !=0){
		len += sprintf(page+len, "%-10s   0x%08x   0x%08x   %2d   /dev/block/mmcblk0p%d\n",
		    dumchar_devices[i].dumname,
		    dumchar_devices[i].device_info.size,
		    dumchar_devices[i].device_info.pri,
		    dumchar_devices[i].type,
		    dumchar_devices[i].device_info.partition_idx);
		up(&(dumchar_devices[i].sem));	
		}else{
		len += sprintf(page+len, "%-10s   0x%08x   0x%08x   %2d   %s\n",
		    dumchar_devices[i].dumname,
		    dumchar_devices[i].device_info.size,
		    dumchar_devices[i].device_info.pri,
		    dumchar_devices[i].type,
		    dumchar_devices[i].device_info.actname);
		up(&(dumchar_devices[i].sem));	
		}
		}
	}

	len += sprintf(page+len, "PN:Partition name you should open;\tSize:size of partition\nAddr:Start Address of partition(EMMC) or Index (MTD);\nTyp:Type of partition, MTD=1,EMMC=2;\nActname: the real name of partition \n");
	*eof = 1;
	return len;
}
#if 1 //wschen 2011-11-09
static int first_read = 0;
#endif
int dumchar_open (struct inode *inode, struct file *filp)
{
	printk("DumCharDebug: in dumchar_open\n");
	struct dumchar_dev *dev; /* device information */
	struct actdevice_info * dev_info ;
	struct mtd_info *mtd;
	int i,result=-1,found=0,index=0,dumfound=0;
#if 0 //wschen 2011-11-09
	if (!first_read) {
            first_read = 1;
            for (i = 0; i < PART_NUM; i++) {
                if (open_for_info(PartInfo[i].name)) {
                    //break;
                }
            }
        }
#endif
	/*  Find the device */
	//printk("in dumchar_open \n");
	for(i = 0 ;i < PART_NUM ; i++){
		if(!strcmp(filp->f_path.dentry->d_name.name,dumchar_devices[i].dumname)){
			//printk("DumCharDebug: in dumchar_open  %s = %s\n",filp->f_path.dentry->d_name.name,dumchar_devices[i].dumname);
			dev = &(dumchar_devices[i]);
			filp->private_data = dev;
			dev_info = &(dev->device_info);
			index = i;
			dumfound = 1;
			//printk( "DumChar: find dev %s index=%d\n",dumchar_devices[i].dumname,index);
			break;
		}
	}

	if(dumfound == 0){
		printk(" DumChar:ERROR:No Such Dummy Device %s \n ",filp->f_path.dentry->d_name.name);
		return -EINVAL;
	}
	
	if (down_interruptible (&dev-> sem))
		return -ERESTARTSYS;
#if 0	 // Mergered by xiangjinming from JRD378867
	/*init dev->type*/
#ifdef MTK_EMMC_SUPPORT
	dev->type = SDCARD;
#else
	dev->type = MTD;
#endif

	//printk( "DumChar: type is  SDCARD=2 MTD=1 act=%d \n",dev->type);

	/*init emmc actname*/
	if(dev->type == SDCARD){
#ifdef MTK_EMMC_SUPPORT
		if(!strcmp("preloader",dev->dumname) ||!strcmp("dsp_bl",dev->dumname)){
			sprintf(dev_info->actname,"/dev/misc-mt6575-sd0");
			dev_info->part = BOOT_PARTITION_1;
			dev_info->pri.start_address = PartInfo[index].start_address;
			found =1;
		}else{
			sprintf(dev_info->actname,"/dev/block/mmcblk0");
			dev_info->part = USER_PARTITION;
			dev_info->pri.start_address = PartInfo[index].start_address - MBR_START_ADDRESS_BYTE;
			//printk( "DumChar:  %s start_address=%d KB\n",dumchar_devices[i].dumname,dev_info->pri.start_address);
			dev_info->partition_idx = PartInfo[index].partition_idx;
			found =1;
		}
		/*init emmc size statt_address */
		if(PartInfo[index].size == 0){
			dev_info->size = USER_size_Byte;
		}else{
			dev_info->size = PartInfo[index].size;
		}
			printk( "DumChar:  %s: logical start address=%u KB, size = %u KB\n",dumchar_devices[i].dumname,dev_info->pri.start_address,dev_info->size);
		if(found == 0){
			result = -EINVAL;
			printk("DumChar:No actdevice match dum device\n");
			goto open_fail2;
		}	
#endif				
	}else{
#if defined(CONFIG_MTK_MTD_NAND)
		/*init mtd actname index size*/
		char *mtdname = dev->dumname;
		
		if(!strcmp(dev->dumname,"seccfg"))
			mtdname = "seccnfg";

		if(!strcmp(dev->dumname,"bootimg"))
			mtdname = "boot";
		
		if(!strcmp(dev->dumname,"sec_ro"))
			mtdname = "secstatic";
		
		if(!strcmp(dev->dumname,"android"))
			mtdname = "system";
		
		if(!strcmp(dev->dumname,"usrdata"))
			mtdname = "userdata";
		
			
		mtd_for_each_device(mtd) {
				//printk("%s is /dev/mtd/mtd%d\n",mtd->name,mtd->index);
			if(!strcmp(mtd->name,mtdname)){
				dev_info->pri.index = mtd->index;
				sprintf(dev_info->actname,"/dev/mtd/mtd%d",mtd->index); 
				dev_info->size = mtd->size;
				found = 1;
				//printk( "DumChar: (%s) is on (%s)\n",dev->dumname,dev_info->actname);
				break;
			}
		}

		//printk("**********************all the mtd device name Over\n");
		if(found == 0){
			result = -EINVAL;
			printk("DumChar:No actdevice match dum device\n");
			goto open_fail2;
		}
#endif	
	}
	
#endif 

	/*open device*/
	//printk("DumChar:before filp_open( %s,%d,%d) O_RDWR = %d\n",dev_info->actname,filp->f_flags, filp->f_mode,O_RDWR);
	if(!strcmp(dev->dumname,"usrdata") ||!strcmp(dev->dumname,"cache") ||!strcmp(dev->dumname,"android") ){
			printk("[dumchar_open] It is forbidded to access %s with dumchar(/dev/%s), the %s partition is managed by filesystem!\n",dev->dumname,dev->dumname,dev->dumname);
			printk("[dumchar_open] show_stack*************************************\n");
			show_stack(NULL,NULL);
			up (&dev->sem);
			dev->actfile = 0xffffffff;
			return -EINVAL;
		
	}else{
	dev->actfile = filp_open( dev_info->actname,filp->f_flags, 0777); 
	if (IS_ERR(dev->actfile)) {
			result=PTR_ERR(dev->actfile);
			printk( " DumChar:open files failed dev->actfile=%p!\n",dev->actfile);
			goto open_fail2;
	} else {
		if (!(dev->actfile->f_op)) {
			printk( " DumChar:File has no file operations registered!\n");
			result = -EIO;
			goto open_fail1;
		}
	}
			}
	//printk( " DumChar:open file sucess!dev->actfile = %p\n",dev->actfile);
	up (&dev->sem);
	//printk("DumCharDebug: out dumchar_open\n");
	return 0; 	/* success */
	
open_fail1:
	filp_close(dev->actfile, NULL);
open_fail2:
	dev->actfile = NULL;
	up (&dev->sem);
	return result;	
}

#ifndef MTK_EMMC_SUPPORT 
#include <linux/syscalls.h>

int mtd_create_symlink(void)
{
	char *link_target;
	char link_name[256];

	int i;
	for (i = 0; i < PART_NUM; i++) {
		if(dumchar_devices[i].device_info.actname){
			memset(link_name,0x0,sizeof(link_name));
			link_target=dumchar_devices[i].device_info.actname;
			sprintf(link_name,"/mtd@%s",dumchar_devices[i].dumname);
			printk("[mtd_create_symlink]: target=%s, name=%s  \n", link_target, link_name);
		 	sys_symlink(link_target, link_name);
		}
	}
}
#endif

int dumchar_release (struct inode *inode, struct file *filp)
{
	printk("DumCharDebug: in dumchar_release\n");
	struct dumchar_dev *dev = filp->private_data;	
	if(!strcmp(dev->dumname,"usrdata") ||!strcmp(dev->dumname,"cache") ||!strcmp(dev->dumname,"android") ){
		printk("[dumchar_release]It is forbidded to access %s with dumchar(/dev/%s), the %s partition is managed by filesystem!\n",dev->dumname,dev->dumname,dev->dumname);
		
	}else{
	filp_close(dev->actfile,NULL);
		}
	if (down_interruptible (&dev->sem))
		return -ERESTARTSYS;
	dev->actfile = NULL;
//	dev->type = 0;
//	memset(&(dev->device_info),0,sizeof (struct actdevice_info));
	up (&dev->sem);
	return 0;
}


struct file_operations dumchar_fops = {
	.owner =     THIS_MODULE,
	.llseek =    dumchar_llseek,
	.read =	     dumchar_read,
	.write =     dumchar_write,
	.unlocked_ioctl =     dumchar_ioctl,
	.open =	  dumchar_open,
	.release =   dumchar_release,
};

int dumchar_probe(void)
{
	printk("DumCharDebug: in dumchar_probe\n");
	int result, i,m,l;
	dev_t devno;
	struct dumchar_dev *dev;
	struct actdevice_info * dev_info ;
	struct mtd_info *mtd;
#if 0

	result = part_init_pmt();
	if (unlikely(result)){
		printk("PMT failed to register device! in dumchar_probe result=%d\n",result);	
		goto fail_register_PMT;
	}
#endif

#ifdef MTK_EMMC_SUPPORT
	init_region_info();
	int zeroIndex=0;
	USER_size_Byte -=USER_offset_Byte;
	printk("[Dumchar]USER_size_Byte=%u KB\n",USER_size_Byte/1024);
	
	for(i=0;i<PART_NUM;i++){
		if((!strcmp(PartInfo[i].name,"usrdata") || !strcmp(PartInfo[i].name,"fat")) && PartInfo[i].size == 0){
			zeroIndex = i;
		}
		if(strcmp(PartInfo[i].name,"dsp_bl")&& strcmp(PartInfo[i].name,"preloader")){
			USER_size_Byte -=PartInfo[i].size;
			printk("[Dumchar]after sub %s=%d B, USER_size_Byte=%u KB\n",PartInfo[i].name,PartInfo[i].size, USER_size_Byte/1024);
		}
	}

	if(USER_size_Byte < 0){
		printk("[Dumchar]Error USER_size_Byte=%u\n",USER_size_Byte);
	}
	printk("[Dumchar]MBR start address in User Region=%u KB, Varity Partition(%s) size=%u KB\n",USER_offset_Byte/1024,PartInfo[zeroIndex].name,USER_size_Byte/1024);
#endif

		
	result = alloc_chrdev_region(&devno,0,PART_NUM,"DumChar");
	if(result <0){
		printk("DumChar: Get chrdev region fail\n");
		goto fail_alloc_chrdev_region;
	}
	major = MAJOR(devno);
	


	for (i = 0; i < PART_NUM; i++) {
		dumchar_devices[i].dumname = PartInfo[i].name;
		init_MUTEX (&(dumchar_devices[i].sem)); 
		
		devno=MKDEV(major,i);
		cdev_init(&dumchar_devices[i].cdev,&dumchar_fops);
		dumchar_devices[i].cdev.owner = THIS_MODULE;
		dumchar_devices[i].cdev.ops = &dumchar_fops;
		result = cdev_add(&dumchar_devices[i].cdev,devno,1);
		if(result){
			printk("DumChar: register char device dumchar fail!\n");
			goto fail_register_chrdev;
		}
		dev = &dumchar_devices[i];
		dev_info = &(dev->device_info);
		#ifdef MTK_EMMC_SUPPORT
		dev->type = SDCARD;
		#else
		dev->type = MTD;
		#endif
		
		//printk( "DumChar: type is  SDCARD=2 MTD=1 act=%d \n",dev->type);
		
		/*init emmc actname*/
		if(dev->type == SDCARD){
	#ifdef MTK_EMMC_SUPPORT
			if(!strcmp("preloader",dev->dumname) ||!strcmp("dsp_bl",dev->dumname)){
				sprintf(dev_info->actname,"/dev/misc-mt6575-sd0");
				dev_info->part = BOOT_PARTITION_1;
				dev_info->pri.start_address = PartInfo[i].start_address;
				//printk("DumCharDebug: the partition %s is in /dev/mt6573-sd0\n",dev->dumname);
	
			}else{
	
				sprintf(dev_info->actname,"/dev/block/mmcblk0");
				dev_info->part = USER_PARTITION;
				dev_info->pri.start_address = PartInfo[i].start_address -MBR_START_ADDRESS_BYTE;
				dev_info->partition_idx = PartInfo[i].partition_idx;

			}
			/*init emmc size statt_address */
	
		if(PartInfo[i].size == 0){
			dev_info->size = USER_size_Byte;
		}else{
			dev_info->size = PartInfo[i].size;
		}
			printk( "DumChar:  %s: logical start address=%u KB, size = %u KB\n",dumchar_devices[i].dumname,dev_info->pri.start_address,dev_info->size);

			//printk("DumCharDebug: the partition %s is in %s dev_info->size=%d ,index=%d\n",dev->dumname,dev_info->actname,dev_info->size,index);
		
	#endif								
		}else{
#if defined(CONFIG_MTK_MTD_NAND)

			/*init mtd actname index size*/
			char *mtdname = dev->dumname;
			
			if(!strcmp(dev->dumname,"seccfg"))
				mtdname = "seccnfg";
		
			if(!strcmp(dev->dumname,"bootimg"))
				mtdname = "boot";
			
			if(!strcmp(dev->dumname,"sec_ro"))
				mtdname = "secstatic";
			
			if(!strcmp(dev->dumname,"android"))
				mtdname = "system";
			
			if(!strcmp(dev->dumname,"usrdata"))
				mtdname = "userdata";
			
		
			mtd_for_each_device(mtd) {
					//printk("%s is /dev/mtd/mtd%d\n",mtd->name,mtd->index);
				if(!strcmp(mtd->name,mtdname)){
					dev_info->pri.index = mtd->index;
					sprintf(dev_info->actname,"/dev/mtd/mtd%d",mtd->index); 
					dev_info->size = mtd->size;
				
					printk( "DumChar: (%s) is on (%s)\n",dev->dumname,dev_info->actname);
					break;
				}
			}
		#endif
		}
}
	dumchar_class = class_create(THIS_MODULE, "dumchar");
	if (IS_ERR(dumchar_class)){
		printk("DumChar: fail in class create");
		result = PTR_ERR(dumchar_class);
		goto fail_create_class;
	}
		
	for (l = 0; l< PART_NUM; l++){
		dumchar_device[l] = device_create(dumchar_class, NULL, MKDEV(major, l),NULL, dumchar_devices[l].dumname);
		if (IS_ERR(dumchar_device[l])) {
				result = PTR_ERR(dumchar_device[l]);
				printk("DumChar: fail in device_create name = %s  minor = %d\n",dumchar_devices[l].dumname,i);
				goto fail_create_device;
		}
	}

#ifdef CONFIG_MTK_MTD_NAND
	mtd_create_symlink();
#endif
//open for info

	return 0;
	
fail_create_device:
	for(m=0;m<l;m++)
		device_destroy(dumchar_class, MKDEV(major, m));	
	class_destroy(dumchar_class);
fail_create_class:
	
fail_register_chrdev:
	for (m = 0; m < i; m++) {
		cdev_del(&dumchar_devices[m].cdev);
	}
	unregister_chrdev_region(MKDEV(major,0),PART_NUM);
	

fail_alloc_chrdev_region:
#ifdef MTK_EMMC_SUPPORT	
	misc_deregister(&pmt_dev);
#endif
fail_register_PMT:
	return result;
}

void dumchar_remove(void)
{
	printk("DumCharDebug: in dumchar_remove\n");
	int i;
	for (i = 0; i < PART_NUM; i++) {
		device_destroy(dumchar_class, MKDEV(major, i));
		cdev_del(&dumchar_devices[i].cdev);
	}
		
	class_destroy(dumchar_class);
	unregister_chrdev_region(MKDEV(major,0),PART_NUM);
#ifdef MTK_EMMC_SUPPORT
	misc_deregister(&pmt_dev);
#endif	

}

static struct platform_driver dumchar_driver = {
		.probe	= dumchar_probe,
		.remove  	= dumchar_remove,
		.driver  = {
			.name  = "dummy_char",
			.owner = THIS_MODULE,
		},
};

static int __init dumchar_init(void)
{
	int result;
	
	printk( "DumChar: in dumchar_init \n");
	
	dumchar_devices = kmalloc(PART_NUM*sizeof (struct dumchar_dev), GFP_KERNEL);
	if (!dumchar_devices) {
		result = -ENOMEM;
		printk("DumChar: malloc dumchar_dev fail\n");
		goto fail_malloc;
	}
	memset(dumchar_devices, 0, PART_NUM*sizeof (struct dumchar_dev));

	dumchar_proc_entry = create_proc_entry("dumchar_info", S_IFREG|S_IRUGO, NULL);
	if (dumchar_proc_entry){
		dumchar_proc_entry->read_proc = dumchar_proc_read;
		printk( "dumchar: register /proc/dumchar_info success %p\n",dumchar_proc_entry->read_proc);
	}
	else{
		printk( "dumchar: unable to register /proc/dumchar_info\n");
		result = -ENOMEM;
		goto fail_create_proc;
	}
	
	result= platform_driver_register(&dumchar_driver);
	if (result) {
        	printk("DUMCHAR: Can't register driver\n");
        	goto fail_driver_register;
    	}
	
	return 0;
fail_driver_register:
	remove_proc_entry("dumchar_info", NULL);
fail_create_proc:
	kfree(dumchar_devices);
fail_malloc:
	return result;
}


static void __exit dumchar_cleanup(void)
{
	remove_proc_entry("dumchar_info", NULL);
	platform_driver_unregister(&dumchar_driver);
	kfree(dumchar_devices);	
}


module_init(dumchar_init);
module_exit(dumchar_cleanup);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek Dummy Char Device Driver");
MODULE_AUTHOR("Kai Zhu <kai.zhu@mediatek.com>");

