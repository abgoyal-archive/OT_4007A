#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <mach/mt6575_gpt.h>
#include "dbg.h"
#include "mt6575_sd.h"

static char cmd_buf[256];

/* for debug zone */
unsigned int sd_debug_zone[4]={
	0,
	0,
	0,
	0
};

/* mode select */
u32 dma_size[4]={
	512,
	512,
	512,
	512
};
msdc_mode drv_mode[4]={
	MODE_SIZE_DEP, /* using DMA or not depend on the size */
	MODE_SIZE_DEP,
	MODE_SIZE_DEP,
	MODE_SIZE_DEP
};

/* for driver profile */
#define TICKS_ONE_MS  (13000)
u32 gpt_enable = 0;
u32 sdio_pro_enable = 0;   /* make sure gpt is enabled */
u32 sdio_pro_time = 0;     /* no more than 30s */
struct sdio_profile sdio_perfomance = {0};  

void msdc_init_gpt(void)
{
    GPT_CONFIG config;	
    
    config.num  = GPT6;
    config.mode = GPT_FREE_RUN;
    config.clkSrc = GPT_CLK_SRC_SYS;
    config.clkDiv = GPT_CLK_DIV_1;   /* 13MHz GPT6 */
            
    if (GPT_Config(config) == FALSE )
        return;                       
        
    GPT_Start(GPT6);	
}

u32 msdc_time_calc(u32 old_L32, u32 old_H32, u32 new_L32, u32 new_H32)
{
    u32 ret = 0; 
    
    if (new_H32 == old_H32) {
        ret = new_L32 - old_L32;
    } else if(new_H32 == (old_H32 + 1)) {
        if (new_L32 > old_L32) {	
            printk("msdc old_L<0x%x> new_L<0x%x>\n", old_L32, new_L32);	
        }
        ret = (0xffffffff - old_L32);  	      
        ret += new_L32; 
    } else {
        printk("msdc old_H<0x%x> new_H<0x%x>\n", old_H32, new_H32);		
    }

    return ret;        	
}

void msdc_sdio_profile(struct sdio_profile* result)
{
    struct cmd_profile*  cmd;
    u32 i; 	
    
    printk("sdio === performance dump ===\n");
    printk("sdio === total execute tick<%d> time<%dms> Tx<%dB> Rx<%dB>\n", 
                    result->total_tc, result->total_tc / TICKS_ONE_MS, 
                    result->total_tx_bytes, result->total_rx_bytes);    

    /* CMD52 Dump */
    cmd = &result->cmd52_rx; 
    printk("sdio === CMD52 Rx <%d>times tick<%d> Max<%d> Min<%d> Aver<%d>\n", cmd->count, cmd->tot_tc, 
                    cmd->max_tc, cmd->min_tc, cmd->tot_tc/cmd->count);     
    cmd = &result->cmd52_tx; 
    printk("sdio === CMD52 Tx <%d>times tick<%d> Max<%d> Min<%d> Aver<%d>\n", cmd->count, cmd->tot_tc, 
                    cmd->max_tc, cmd->min_tc, cmd->tot_tc/cmd->count);   
                    
    /* CMD53 Rx bytes + block mode */
    for (i=0; i<512; i++) {
        cmd = &result->cmd53_rx_byte[i];
        if (cmd->count) {
            printk("sdio<%6d><%3dB>_Rx_<%9d><%9d><%6d><%6d>_<%9dB><%2dM>\n", cmd->count, i, cmd->tot_tc, 
                             cmd->max_tc, cmd->min_tc, cmd->tot_tc/cmd->count,
                             cmd->tot_bytes, (cmd->tot_bytes/10)*13 / (cmd->tot_tc/10));                        	
        }	
    }  
    for (i=0; i<100; i++) {
        cmd = &result->cmd53_rx_blk[i];
        if (cmd->count) {
            printk("sdio<%6d><%3d>B_Rx_<%9d><%9d><%6d><%6d>_<%9dB><%2dM>\n", cmd->count, i, cmd->tot_tc, 
                             cmd->max_tc, cmd->min_tc, cmd->tot_tc/cmd->count,
                             cmd->tot_bytes, (cmd->tot_bytes/10)*13 / (cmd->tot_tc/10));                        	
        }	
    }

    /* CMD53 Tx bytes + block mode */
    for (i=0; i<512; i++) {
        cmd = &result->cmd53_tx_byte[i];
        if (cmd->count) {
            printk("sdio<%6d><%3dB>_Tx_<%9d><%9d><%6d><%6d>_<%9dB><%2dM>\n", cmd->count, i, cmd->tot_tc, 
                             cmd->max_tc, cmd->min_tc, cmd->tot_tc/cmd->count,
                             cmd->tot_bytes, (cmd->tot_bytes/10)*13 / (cmd->tot_tc/10));                          	
        }	
    }          
    for (i=0; i<100; i++) {
        cmd = &result->cmd53_tx_blk[i];
        if (cmd->count) {
            printk("sdio<%6d><%3d>B_Tx_<%9d><%9d><%6d><%6d>_<%9dB><%2dM>\n", cmd->count, i, cmd->tot_tc, 
                             cmd->max_tc, cmd->min_tc, cmd->tot_tc/cmd->count,
                             cmd->tot_bytes, (cmd->tot_bytes/10)*13 / (cmd->tot_tc/10));                            	
        }	
    }     
    
    printk("sdio === performance dump done ===\n");      
}

//========= sdio command table ===========
void msdc_performance(u32 opcode, u32 sizes, u32 bRx, u32 ticks)
{
    struct sdio_profile* result = &sdio_perfomance; 
    struct cmd_profile*  cmd; 
    u32 block;     	

    if (sdio_pro_enable == 0) {
        return;
    }

    if (opcode == 52) {
        cmd = bRx ?  &result->cmd52_rx : &result->cmd52_tx;   	
    } else if (opcode == 53) {
        if (sizes < 512) {
            cmd = bRx ?  &result->cmd53_rx_byte[sizes] : &result->cmd53_tx_byte[sizes];    	
        } else {
            block = sizes / 512; 
            if (block >= 99) {
               printk("cmd53 error blocks\n"); 
               while(1);	
            }
            cmd = bRx ?  &result->cmd53_rx_blk[block] : &result->cmd53_tx_blk[block];       	
        }   	
    } else {
        return; 	
    }
        
    /* update the members */
    if (ticks > cmd->max_tc){
        cmd->max_tc = ticks;	
    }
    if (cmd->min_tc == 0 || ticks < cmd->min_tc) {
        cmd->min_tc = ticks; 	  
    }
    cmd->tot_tc += ticks;
    cmd->tot_bytes += sizes; 
    cmd->count ++; 
    
    if (bRx) {
        result->total_rx_bytes += sizes;    	
    } else {
        result->total_tx_bytes += sizes; 	
    }
    result->total_tc += ticks; 
    
    /* dump when total_tc > 30s */
    if (result->total_tc >= sdio_pro_time * TICKS_ONE_MS * 1000) {
        msdc_sdio_profile(result);       
        memset(result, 0 , sizeof(struct sdio_profile));                                             
    }
}

//========== driver proc interface ===========
static int msdc_debug_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    char *p = page;
    int len = 0;

    p += sprintf(p, "\n=========================================\n");

    p += sprintf(p, "Index<0> + Id + Zone\n");   
    p += sprintf(p, "-> PWR<9> WRN<8> | FIO<7> OPS<6> FUN<5> CFG<4> | INT<3> RSP<2> CMD<1> DMA<0>\n");        
    p += sprintf(p, "-> echo 0 3 0x3ff >msdc_bebug -> host[3] debug zone set to 0x3ff\n");              
    p += sprintf(p, "-> MSDC[0] Zone: 0x%.8x\n", sd_debug_zone[0]);
    p += sprintf(p, "-> MSDC[1] Zone: 0x%.8x\n", sd_debug_zone[1]);
    p += sprintf(p, "-> MSDC[2] Zone: 0x%.8x\n", sd_debug_zone[2]);
    p += sprintf(p, "-> MSDC[3] Zone: 0x%.8x\n", sd_debug_zone[3]);

    p += sprintf(p, "Index<1> + ID:4|Mode:4 + DMA_SIZE\n");  
    p += sprintf(p, "-> 0)PIO 1)DMA 2)SIZE\n");      
    p += sprintf(p, "-> echo 1 22 0x200 >msdc_bebug -> host[2] size mode, dma when >= 512\n");        
    p += sprintf(p, "-> MSDC[0] mode<%d> size<%d>\n", drv_mode[0], dma_size[0]);
    p += sprintf(p, "-> MSDC[1] mode<%d> size<%d>\n", drv_mode[1], dma_size[1]);    
    p += sprintf(p, "-> MSDC[2] mode<%d> size<%d>\n", drv_mode[2], dma_size[2]);
    p += sprintf(p, "-> MSDC[3] mode<%d> size<%d>\n", drv_mode[3], dma_size[3]);

    p += sprintf(p, "Index<3> + SDIO_PROFILE + TIME\n"); 
    p += sprintf(p, "-> echo 3 1 0x1E >msdc_bebug -> enable sdio_profile, 30s\n"); 
    p += sprintf(p, "-> SDIO_PROFILE<%d> TIME<%ds>\n", sdio_pro_enable, sdio_pro_time);  
               
    p += sprintf(p, "=========================================\n\n");
    
    *start = page + off;

    len = p - page;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len : count;
} 

static int msdc_debug_proc_write(struct file *file, const char *buf, unsigned long count, void *data)
{
	int ret;
	
	int cmd, p1, p2;   
	int id, zone;
	int mode, size;  
  
	if (count == 0)return -1;
	if(count > 255)count = 255;

	ret = copy_from_user(cmd_buf, buf, count);
	if (ret < 0)return -1;
	
	cmd_buf[count] = '\0';
	printk("msdc Write %s\n", cmd_buf);

	sscanf(cmd_buf, "%x %x %x", &cmd, &p1, &p2);
	
	if(cmd == SD_TOOL_ZONE) {
		id = p1; zone = p2; zone &= 0x3ff;		
		printk("msdc host_id<%d> zone<0x%.8x>\n", id, zone);
		if(id >=0 && id<=3){
			sd_debug_zone[id] = zone;
		}
		else if(id == 4){
			sd_debug_zone[0] = sd_debug_zone[1] = zone;
			sd_debug_zone[2] = sd_debug_zone[3] = zone;
		}
		else{
			printk("msdc host_id error when set debug zone\n");
		}
	} else if (cmd == SD_TOOL_DMA_SIZE) {
		id = p1>>4;  mode = (p1&0xf); size = p2; 
		if(id >=0 && id<=3){
			drv_mode[id] = mode;
			dma_size[id] = p2; 
		}
		else if(id == 4){
			drv_mode[0] = drv_mode[1] = mode;
			drv_mode[2] = drv_mode[3] = mode;
			dma_size[0] = dma_size[1] = p2; 
			dma_size[2] = dma_size[3] = p2;
		}
		else{
			printk("msdc host_id error when select mode\n");
		}	
	} else if (cmd == SD_TOOL_SDIO_PROFILE) {
		if (p1 == 1) { /* enable profile */
			if (gpt_enable == 0) {
				msdc_init_gpt();
				gpt_enable = 1;
			} 
			sdio_pro_enable = 1;
			if (p2 == 0) p2 = 1; if (p2 >= 30) p2 = 30; 				
			sdio_pro_time = p2 ; 
		}	else if (p1 == 0) {
			/* todo */
			sdio_pro_enable = 0;
		}			
	}
	
	return count;
}

int msdc_debug_proc_init(void) 
{   	
    struct proc_dir_entry *de = create_proc_entry("msdc_debug", 0664, 0);
    de->read_proc  = msdc_debug_proc_read;
    de->write_proc = msdc_debug_proc_write;
    
    return 0 ;
}
EXPORT_SYMBOL_GPL(msdc_debug_proc_init);