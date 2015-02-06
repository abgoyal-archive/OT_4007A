
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/earlysuspend.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/xlog.h>

#include "mach/mt6575_clock_manager.h"
#include "mach/mt6575_typedefs.h"
#include "mach/mt6575_sc.h"
#include "mach/mt6575_cpe.h"
#include "mach/mt6575_boot.h"

#include "mach/pmic_mt6329_hw_bank1.h"
#include "mach/pmic_mt6329_sw_bank1.h"
#include "mach/pmic_mt6329_hw.h"
#include "mach/pmic_mt6329_sw.h"
#include "mach/upmu_common_sw.h"
#include "mach/upmu_hw.h"

#define cpu_ss_dprintk(fmt, args...)    \
do {                                    \
    if (cpu_ss_debug_mode) {            \
        printk(fmt, ##args);            \
    }                                   \
} while(0)

static struct hrtimer cpu_ss_timer;
struct task_struct *cpu_ss_thread = NULL;
static DECLARE_WAIT_QUEUE_HEAD(cpu_ss_timer_waiter);

static int cpu_ss_period = 100;
static int cpu_ss_timer_flag = 0;

static bool cpu_ss_debug_mode = false;
static bool cpu_ss_period_mode = false;

enum hrtimer_restart cpu_ss_timer_func(struct hrtimer *timer)
{
    cpu_ss_dprintk("[%s]: enter timer function\n", __FUNCTION__);
    
    cpu_ss_timer_flag = 1; wake_up_interruptible(&cpu_ss_timer_waiter);
    
    return HRTIMER_NORESTART;
}

int cpu_ss_thread_handler(void *unused)
{
    kal_uint32 ret = 0, flag = 0, clk_mux1, clk_mux2;
    
    do
    {
        ktime_t ktime = ktime_set(0, cpu_ss_period);
        
        wait_event_interruptible(cpu_ss_timer_waiter, cpu_ss_timer_flag != 0);
        cpu_ss_timer_flag = 0;
        
        if (!flag)
        {
            DRV_WriteReg32(TOP_CKMUXSEL, (DRV_Reg32(TOP_CKMUXSEL) & 0x01ff));
            DRV_WriteReg32(TOP_CKMUXSEL, (DRV_Reg32(TOP_CKMUXSEL) & 0x0ff3));
            flag = 1;
        }
        else
        {
            DRV_WriteReg32(TOP_CKMUXSEL, (DRV_Reg32(TOP_CKMUXSEL) | 0x0008));
            DRV_WriteReg32(TOP_CKMUXSEL, (DRV_Reg32(TOP_CKMUXSEL) | 0x0200));
            flag = 0;
        }
        
        cpu_ss_dprintk("[%s]: CLKMUX = 0x%x\n", __FUNCTION__, DRV_Reg32(TOP_CKMUXSEL));
        
        hrtimer_start(&cpu_ss_timer, ktime, HRTIMER_MODE_REL);
        
    } while (!kthread_should_stop());
    
    return 0;
}

static int cpu_ss_mode_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;
    
    if ((DRV_Reg32(TOP_CKMUXSEL) & 0x000C) == 0)
        p += sprintf(p, "CA9 runs at 26Mhz\n");
    else
        p += sprintf(p, "CA9 runs at 1001Mhz\n");
        
    *start = buf + off;
    
    len = p - buf;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len  : count;
}

static ssize_t cpu_ss_mode_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = 0, mode = 0;
    char desc[32];

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
    {
        return 0;
    }
    desc[len] = '\0';
    
    printk("\n=============== CPU Speed Switch Mode ================\n");
    if (sscanf(desc, "%d", &mode) == 1)
    {
        if (mode)
        {
            printk("  config cpu speed switch mode = 1001Mhz\n");
            DRV_WriteReg32(TOP_CKMUXSEL, (DRV_Reg32(TOP_CKMUXSEL) | 0x0008));
            DRV_WriteReg32(TOP_CKMUXSEL, (DRV_Reg32(TOP_CKMUXSEL) | 0x0200));
        }
        else
        {
            printk("  config cpu speed switch mode = 26Mhz\n");
            DRV_WriteReg32(TOP_CKMUXSEL, (DRV_Reg32(TOP_CKMUXSEL) & 0x01ff));
            DRV_WriteReg32(TOP_CKMUXSEL, (DRV_Reg32(TOP_CKMUXSEL) & 0x0ff3));
        }
        
        printk("==========================================\n");
        return count;
    }
    else
    {
        printk("  bad argument!!\n");
        printk(  "==========================================\n");
    }
    
    return -EINVAL;
}


static int cpu_ss_period_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;
    
    p += sprintf(p, "%d", cpu_ss_period);

    *start = buf + off;
    
    len = p - buf;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len  : count;
}

static ssize_t cpu_ss_period_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = 0, time = 0;
    char desc[32];

    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len))
    {
        return 0;
    }
    desc[len] = '\0';
    
    printk("\n=============== CPU Speed Switch Period Mode ================\n");
    if (sscanf(desc, "%d", &time) == 1)
    {
        printk("  config cpu speed switch period = %d (ns)\n", time);
        cpu_ss_period = time;
        printk("==========================================\n");
        return count;
    }
    else
    {
        printk("  bad argument!!\n");
        printk(  "==========================================\n");
    }
    
    return -EINVAL;
}

static int cpu_ss_period_mode_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;
    
    if (cpu_ss_period_mode)
        p += sprintf(p, "enable");
    else
        p += sprintf(p, "disable");

    *start = buf + off;
    
    len = p - buf;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len  : count;
}

static ssize_t cpu_ss_period_mode_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
   int len = 0;
    char mode[20], desc[32];
    ktime_t ktime = ktime_set(0, cpu_ss_period);
    
    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len)) 
    {
        return 0;
    }
    desc[len] = '\0';
        
    printk("\n=============== CPU Speed Switch Period Mode ================\n");
    if (sscanf(desc, "%s", &mode) == 1)
    {
        if (!strcmp(mode, "enable"))
        {
            printk("  enable cpu speed switch period mode\n");
            cpu_ss_period_mode = true;
                    
            cpu_ss_thread = kthread_run(cpu_ss_thread_handler, 0, "cpu speed switch");
            if (IS_ERR(cpu_ss_thread))
            {
                printk("[%s]: failed to create cpu speed switch thread\n", __FUNCTION__);
            }
    
            hrtimer_start(&cpu_ss_timer, ktime, HRTIMER_MODE_REL);
            printk("==========================================\n");
            return count;
        }
        else if (!strcmp(mode, "disable"))
        {
            printk("  disable cpu speed switch period mode\n");
            cpu_ss_period_mode = false;
            
            kthread_stop(cpu_ss_thread);
            
            DRV_WriteReg32(TOP_CKMUXSEL, (DRV_Reg32(TOP_CKMUXSEL) | 0x0008));
            DRV_WriteReg32(TOP_CKMUXSEL, (DRV_Reg32(TOP_CKMUXSEL) | 0x0200));
            
            cpu_ss_dprintk("[%s]: CLKMUX = 0x%x\n", __FUNCTION__, DRV_Reg32(TOP_CKMUXSEL));
            
            hrtimer_cancel(&cpu_ss_timer);
            printk("==========================================\n");
            return count;
        }
        else
        {
            printk("  bad argument!! argument should be \"enable\" or \"disable\"\n");
            printk(  "==========================================\n");
        }
    }
    else
    {
        printk("  bad argument!!\n");
        printk(  "==========================================\n");
    }
    
    return -EINVAL;
}

static int cpu_ss_debug_mode_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;
    
    if (cpu_ss_debug_mode)
        p += sprintf(p, "enable");
    else
        p += sprintf(p, "disable");

    *start = buf + off;
    
    len = p - buf;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len  : count;
}

static ssize_t cpu_ss_debug_mode_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int len = 0;
    char mode[20], desc[32];
    
    len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
    if (copy_from_user(desc, buffer, len)) 
    {
        return 0;
    }
    desc[len] = '\0';
        
    printk("\n=============== CPU Speed Switch Debug Mode ================\n");
    if (sscanf(desc, "%s", &mode) == 1)
    {
        if (!strcmp(mode, "enable"))
        {
            printk("  enable cpu speed switch debug mode\n");
            cpu_ss_debug_mode = true;
            printk("==========================================\n");
            return count;
        }
        else if (!strcmp(mode, "disable"))
        {
            printk("  disable cpu speed switch debug mode\n");
            cpu_ss_debug_mode = false;
            printk("==========================================\n");
            return count;
        }
        else
        {
            printk("  bad argument!! argument should be \"enable\" or \"disable\"\n");
            printk(  "==========================================\n");
        }
    }
    else
    {
        printk("  bad argument!!\n");
        printk(  "==========================================\n");
    }
    
    return -EINVAL;
}

void mtk_cpufreq_set_max_cpu_vol(void)
{

}
EXPORT_SYMBOL(mtk_cpufreq_set_max_cpu_vol);

int cpufreq_state_set(int enable)
{
    return 0;
}
EXPORT_SYMBOL(cpufreq_state_set);

static int __init cpufreq_init(void)
{
    /*******************************************
     * Add for CPU 26Mhz <-> 1Ghz stress begin 
     *******************************************/
    struct proc_dir_entry *entry = NULL;
    struct proc_dir_entry *cpu_ss_dir = NULL;
    
    hrtimer_init(&cpu_ss_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    cpu_ss_timer.function = cpu_ss_timer_func;
    
    cpu_ss_dir = proc_mkdir("cpu_ss", NULL);
    if (!cpu_ss_dir) 
    {
        pr_err("[%s]: mkdir /proc/cpu_ss failed\n", __FUNCTION__);
    } 
    else 
    {
        entry = create_proc_entry("cpu_ss_debug_mode", S_IRUGO | S_IWUSR | S_IWGRP, cpu_ss_dir);
        if (entry) 
        {
            entry->read_proc = cpu_ss_debug_mode_read;
            entry->write_proc = cpu_ss_debug_mode_write;
        }
        
        entry = create_proc_entry("cpu_ss_period_mode", S_IRUGO | S_IWUSR | S_IWGRP, cpu_ss_dir);
        if (entry) 
        {
            entry->read_proc = cpu_ss_period_mode_read;
            entry->write_proc = cpu_ss_period_mode_write;
        }
        
        entry = create_proc_entry("cpu_ss_period", S_IRUGO | S_IWUSR | S_IWGRP, cpu_ss_dir);
        if (entry) 
        {
            entry->read_proc = cpu_ss_period_read;
            entry->write_proc = cpu_ss_period_write;
        }
        
        entry = create_proc_entry("cpu_ss_mode", S_IRUGO | S_IWUSR | S_IWGRP, cpu_ss_dir);
        if (entry) 
        {
            entry->read_proc = cpu_ss_mode_read;
            entry->write_proc = cpu_ss_mode_write;
        }
    }
    /*******************************************
     * Add for CPU 26Mhz <-> 1Ghz stress end 
     *******************************************/
    
    return 0;
}
module_init(cpufreq_init);

static void __exit cpufreq_exit(void)
{

}
module_exit(cpufreq_exit);

MODULE_DESCRIPTION("MediaTek CPU Frequency Scaling driver");
MODULE_LICENSE("GPL");