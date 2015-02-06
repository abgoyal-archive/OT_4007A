#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <asm/uaccess.h>
#include "prof_ctl.h"
#include "prof_mem.h"
#include <linux/module.h>
#include <linux/pid.h>
int mtsched_mon_enabled;
void mt_sched_monitor_switch(int on);
static DEFINE_MUTEX(mt_sched_mon_lock);
#define SEQ_printf(m, x...)	    \
 do {			    \
    if (m)		    \
	seq_printf(m, x);	\
    else		    \
	printk(x);	    \
 } while (0)

#define MT_DEBUG_ENTRY(name) \
static int mt_##name##_show(struct seq_file *m, void *v);\
static int mt_##name##_write(struct file *filp, const char *ubuf, size_t cnt, loff_t *data);\
static int mt_##name##_open(struct inode *inode, struct file *file) \
{ \
    return single_open(file, mt_##name##_show, inode->i_private); \
} \
\
static const struct file_operations mt_##name##_fops = { \
    .open = mt_##name##_open, \
    .write = mt_##name##_write,\
    .read = seq_read, \
    .llseek = seq_lseek, \
    .release = single_release, \
};\
void mt_##name##_switch(int on);

/*
 * Ease the printing of nsec fields:
 */
static long long nsec_high(unsigned long long nsec)
{
    if ((long long)nsec < 0) {
	nsec = -nsec;
	do_div(nsec, 1000000);
	return -nsec;
    }
    do_div(nsec, 1000000);

    return nsec;
}

static unsigned long nsec_low(unsigned long long nsec)
{
    if ((long long)nsec < 0)
	nsec = -nsec;

    return do_div(nsec, 1000000);
}
#define SPLIT_NS(x) nsec_high(x), nsec_low(x)

static long long usec_high(unsigned long long usec)
{
    if ((long long)usec < 0) {
	usec = -usec;
	do_div(usec, 1000);
	return -usec;
    }
    do_div(usec, 1000);

    return usec;
}

static unsigned long usec_low(unsigned long long usec)
{
    if ((long long)usec < 0)
	usec = -usec;

    return do_div(usec, 1000);
}

#define SPLIT_US(x) usec_high(x), usec_low(x)

/* --------------------------------------------------- */
/* Real work */
#define TIME_3MS  3000000
#define TIME_10MS 10000000
static DEFINE_PER_CPU(int, MT_tracing_cpu);
static DEFINE_PER_CPU(unsigned long long, t_irq_on);
static DEFINE_PER_CPU(unsigned long long, t_irq_off);
void MT_trace_softirqs_on(unsigned long ip);
void MT_trace_softirqs_off(unsigned long ip);

void MT_trace_hardirqs_on(void)
{
    unsigned long long t_diff, t_on, t_off;
    if(unlikely(2 == mtsched_mon_enabled || 3 == mtsched_mon_enabled)){
	if( 0 == current->pid) /* Ignore swap thread */
	    return;
	if(__raw_get_cpu_var(MT_tracing_cpu) == 1) 
	{
	    t_on = sched_clock();
	    t_off = __raw_get_cpu_var(t_irq_off);
	    t_diff = t_on - t_off;

	    __raw_get_cpu_var(t_irq_on) = t_on;
	    if(t_diff > TIME_10MS){
		printk(KERN_WARNING "[PREMMPT MONITOR][%d:%s] IRQ disable too long:%lu.%lu ms!\n",current->pid, current->comm, SPLIT_NS(t_diff));
		dump_stack();
	    }
	    __raw_get_cpu_var(t_irq_off) = 0; 
	}
	__raw_get_cpu_var(MT_tracing_cpu) = 0;
    }
}
EXPORT_SYMBOL(MT_trace_hardirqs_on);
void MT_trace_hardirqs_off(void)
{
    if(unlikely(2 == mtsched_mon_enabled || 3 == mtsched_mon_enabled)){
	if( 0 == current->pid) /* Ignore swap thread */
	    return;
	if(__raw_get_cpu_var(MT_tracing_cpu) == 0){
	    __raw_get_cpu_var(t_irq_off) = sched_clock();
	}
	__raw_get_cpu_var(MT_tracing_cpu) = 1;
    }
}
EXPORT_SYMBOL(MT_trace_hardirqs_off);
/* --------------------------------------------------- */
/*                     Define Proc entry               */
/* --------------------------------------------------- */
MT_DEBUG_ENTRY(sched_monitor);
static int mt_sched_monitor_show(struct seq_file *m, void *v)
{
    SEQ_printf(m, "=== mt Scheduler monitoring ===\n");
    if(mtsched_mon_enabled){
	SEQ_printf(m, "  OO  Monitor is enable now:%d\n", mtsched_mon_enabled);
    }else{
	SEQ_printf(m, "  XX  Monitor is Disable now:%d\n", mtsched_mon_enabled);
    }
    return 0;
}
static ssize_t mt_sched_monitor_write(struct file *filp, const char *ubuf,
	   size_t cnt, loff_t *data)
{
    char buf[64];
    int val;
    int ret;
    if (cnt >= sizeof(buf))
	return -EINVAL;

    if (copy_from_user(&buf, ubuf, cnt))
	return -EFAULT;

    buf[cnt] = 0;

    ret = strict_strtoul(buf, 10, &val);
    if (ret < 0)
	return ret;
    //0: off, 1:on
    //val = !!val;
    mt_sched_monitor_switch(val);
    printk(" to 0x%lu\n", val);
    return cnt;
}
void reset_sched_monitor()
{}
void start_sched_monitor()
{
    mtsched_mon_enabled = 1;
}
void stop_sched_monitor()
{
    mtsched_mon_enabled = 0;
}
void mt_sched_monitor_switch(int on)
{
    printk("[mtprof] sched monitor switch from %d to %d", mtsched_mon_enabled, on);
    preempt_disable_notrace();
    mutex_lock(&mt_sched_mon_lock);
    mtsched_mon_enabled = on;
    mutex_unlock(&mt_sched_mon_lock);
    preempt_enable_notrace();
}

static int __init init_mtsched_mon(void)
{
    struct proc_dir_entry *pe;
    struct path mtpath;
    mtsched_mon_enabled = 0;
     __raw_get_cpu_var(MT_tracing_cpu) = 0;

    if (!proc_mkdir("mtmon", NULL)){
	return -1;
    }
    pe = proc_create("mtmon/sched_mon", 0664, NULL, &mt_sched_monitor_fops);
    if (!pe)
	return -ENOMEM;
    //start_record_task();
    return 0;
}
__initcall(init_mtsched_mon);
