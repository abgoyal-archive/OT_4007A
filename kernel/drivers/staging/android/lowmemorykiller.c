/* drivers/misc/lowmemorykiller.c
 *
 * The lowmemorykiller driver lets user-space specify a set of memory thresholds
 * where processes with a range of oom_adj values will get killed. Specify the
 * minimum oom_adj values in /sys/module/lowmemorykiller/parameters/adj and the
 * number of free pages in /sys/module/lowmemorykiller/parameters/minfree. Both
 * files take a comma separated list of numbers in ascending order.
 *
 * For example, write "0,8" to /sys/module/lowmemorykiller/parameters/adj and
 * "1024,4096" to /sys/module/lowmemorykiller/parameters/minfree to kill processes
 * with a oom_adj value of 8 or higher when the free memory drops below 4096 pages
 * and kill processes with a oom_adj value of 0 or higher when the free memory
 * drops below 1024 pages.
 *
 * The driver considers memory used for caches to be free, but if a large
 * percentage of the cached memory is locked this can be very inaccurate
 * and processes may not get killed until the normal oom killer is triggered.
 *
 * Copyright (C) 2007-2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/oom.h>
#include <linux/sched.h>
#include <linux/notifier.h>

#ifdef CONFIG_MTK_AEE_FEATURE
//#include <linux/aee.h>
#include <linux/disp_assert_layer.h>
int lowmem_indicator = 0;
int in_lowmem = 0;
#endif

/*
 * MTK_LMK_THREAD
 *
 * Define this macro to enable periodic low memory killer
 */
#define MTK_LMK_THREAD

/*
 * MTK_LCA_SUPPORT
 *
 * Define this macro to enable scenario-based computation
 */
// #define MTK_LCA_SUPPORT

#ifdef MTK_LMK_THREAD
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
static DECLARE_WAIT_QUEUE_HEAD(lowmemkill_wait);
#endif

static uint32_t lowmem_debug_level = 2;
static uint32_t lowmem_debug_adj = 4;
static int lowmem_adj[6] = {
	0,
	1,
	6,
	12,
};

static int lowmem_adj_size = 4;
static size_t lowmem_minfree[6] = {
	3 * 512,	/* 6MB */
	2 * 1024,	/* 8MB */
	4 * 1024,	/* 16MB */
	16 * 1024,	/* 64MB */
};

static int lowmem_minfree_size = 4;
static struct task_struct *lowmem_deathpending;
static unsigned long lowmem_deathpending_timeout;

/*
 * dump detailed memory log 
 * if low memory killer is activated 5 times within 5s
 */
#define LOWMEM_SAMPLE_WINDOW 6
#define LOWMEM_DEBUG_DURATION 2*1000000000 // in nsecs
static int nr_samples = LOWMEM_SAMPLE_WINDOW;
static unsigned long long duration = 0;
static unsigned long long prev_ns = 0;

#ifdef MTL_LCA_SUPPORT
EXPORT_SYMBOL(lowmem_adj);
EXPORT_SYMBOL(lowmem_adj_size);
EXPORT_SYMBOL(lowmem_minfree);
EXPORT_SYMBOL(lowmem_minfree_size);
#endif

void (*scenarios)(struct task_struct *) = NULL;
EXPORT_SYMBOL(scenarios);

#define lowmem_print(level, x...)			\
	do {						\
		if (lowmem_debug_level >= (level))	\
			printk(x);			\
	} while (0)

static int
task_notify_func(struct notifier_block *self, unsigned long val, void *data);

static struct notifier_block task_nb = {
	.notifier_call	= task_notify_func,
};

static int
task_notify_func(struct notifier_block *self, unsigned long val, void *data)
{
	struct task_struct *task = data;

	if (task == lowmem_deathpending)
		lowmem_deathpending = NULL;

	return NOTIFY_OK;
}

static int lowmem_shrink(struct shrinker *s, int nr_to_scan, gfp_t gfp_mask)
{
	struct task_struct *p;
	struct task_struct *selected = NULL;
	int rem = 0;
	int tasksize;
	int i;
	int min_adj = OOM_ADJUST_MAX + 1;
	int selected_tasksize = 0;
	int selected_oom_adj;
	int array_size = ARRAY_SIZE(lowmem_adj);
	int other_free = global_page_state(NR_FREE_PAGES);
	int other_file = global_page_state(NR_FILE_PAGES) -
						global_page_state(NR_SHMEM);
	uint32_t prev_debug_level = lowmem_debug_level;
	uint32_t prev_debug_adj = lowmem_debug_adj;

	/*
	 * If we already have a death outstanding, then
	 * bail out right away; indicating to vmscan
	 * that we have nothing further to offer on
	 * this pass.
	 *
	 */
	if (lowmem_deathpending &&
	    time_before_eq(jiffies, lowmem_deathpending_timeout))
		return 0;

	if (lowmem_adj_size < array_size)
		array_size = lowmem_adj_size;
	if (lowmem_minfree_size < array_size)
		array_size = lowmem_minfree_size;
	for (i = 0; i < array_size; i++) {
#if 1
		if (other_free + other_file < lowmem_minfree[i]) {
#else
		if (other_free < lowmem_minfree[i] &&
		    other_file < lowmem_minfree[i]) {
#endif
			min_adj = lowmem_adj[i];
			break;
		}
	}
	if (nr_to_scan > 0)
		lowmem_print(3, "lowmem_shrink %d, %x, ofree %d %d, ma %d\n",
			     nr_to_scan, gfp_mask, other_free, other_file,
			     min_adj);
	rem = global_page_state(NR_ACTIVE_ANON) +
		global_page_state(NR_ACTIVE_FILE) +
		global_page_state(NR_INACTIVE_ANON) +
		global_page_state(NR_INACTIVE_FILE);
	if (nr_to_scan <= 0 || min_adj == OOM_ADJUST_MAX + 1) {
		lowmem_print(5, "lowmem_shrink %d, %x, return %d\n",
			     nr_to_scan, gfp_mask, rem);
#ifndef MTK_LMK_THREAD
		return rem;
#endif
	}
	selected_oom_adj = min_adj;

	/*
	 * print detailed log when low memory killer is invoked 
	 * LOWMEM_SAMPLE_WINDOW times in less than LOWMEM_DEBUG_DURATION
	 *
	 * Assume this function is not re-entrant. No lock is used for global variable
	 *
	 * ToDo: check if this function is re-entrant
	 */

	if (prev_ns == 0)
		prev_ns = sched_clock();
	else {
		duration = sched_clock() - prev_ns;
	}

	if (duration > LOWMEM_DEBUG_DURATION) {
		nr_samples = LOWMEM_SAMPLE_WINDOW;
		prev_ns = sched_clock();
	}

	// trigger detailed logs
	if (nr_samples < 0) {
		prev_debug_level = lowmem_debug_level;
		prev_debug_adj = lowmem_debug_adj;
		lowmem_debug_level = 100;
		lowmem_debug_adj = 100;
	}

	// add debug log
	if (min_adj <= lowmem_debug_adj) {
		lowmem_print(1, "======low memory killer=====\n");
		lowmem_print(1, "Free memory now is %d pages\n", other_free+other_file);
	}		

#ifdef MTK_LCA_SUPPORT
	if (scenarios != NULL)
		scenarios(0);
#endif

	read_lock(&tasklist_lock);
	for_each_process(p) {
		struct mm_struct *mm;
		struct signal_struct *sig;
		int oom_adj;

		task_lock(p);
		mm = p->mm;
		sig = p->signal;
		if (!mm || !sig) {
			task_unlock(p);
			continue;
		}
		oom_adj = sig->oom_adj;

#ifdef MTK_LCA_SUPPORT
		if (oom_adj == 0 && scenarios != NULL)
			scenarios(p);
#endif
		// add debug log
		if (min_adj <= lowmem_debug_adj)
			lowmem_print(1, "Candidate %d (%s), adj %d, size %lu, to kill\n",
				     p->pid, p->comm, oom_adj, get_mm_rss(mm));

		if (oom_adj < min_adj) {
			task_unlock(p);
			continue;
		}
		tasksize = get_mm_rss(mm);
		task_unlock(p);
		if (tasksize <= 0)
			continue;
		if (selected) {
			if (oom_adj < selected_oom_adj)
				continue;
			if (oom_adj == selected_oom_adj &&
			    tasksize <= selected_tasksize)
				continue;
		}
		selected = p;
		selected_tasksize = tasksize;
		selected_oom_adj = oom_adj;
		lowmem_print(2, "select %d (%s), adj %d, size %d, to kill\n",
			     p->pid, p->comm, oom_adj, tasksize);
	}

#ifdef MTK_LCA_SUPPORT
	if (scenarios != NULL)
		scenarios(1);
#endif

	if (selected) {
		lowmem_print(1, "send sigkill to %d (%s), adj %d, size %d\n",
			     selected->pid, selected->comm,
			     selected_oom_adj, selected_tasksize);
		lowmem_deathpending = selected;
		lowmem_deathpending_timeout = jiffies + HZ;

		/*
		 * show an indication if low memory
		 */
#ifdef CONFIG_MTK_AEE_FEATURE
		if (lowmem_indicator && selected_oom_adj <= 4) {
			lowmem_print(5, "low memory: raise aee warning\n");
			in_lowmem = 1;
			DAL_LowMemoryOn();
			//aee_kernel_warning(module_name, lowmem_warning);
		}
#endif

		force_sig(SIGKILL, selected);

		nr_samples--;

		rem -= selected_tasksize;
	}

	lowmem_print(4, "lowmem_shrink %d, %x, return %d\n",
		     nr_to_scan, gfp_mask, rem);

	read_unlock(&tasklist_lock);

	if (nr_samples < 0) {
		lowmem_debug_level = prev_debug_level;
		lowmem_debug_adj = prev_debug_adj;
		show_mem();
	}

	return rem;
}

static struct shrinker lowmem_shrinker = {
	.shrink = lowmem_shrink,
	.seeks = DEFAULT_SEEKS * 16
};

#ifdef MTK_LMK_THREAD
static void lowmem_killer(void) 
{
	set_user_nice(current, 5);
        set_freezable();

        do {
		int other_free;
		int other_file;

		lowmem_shrink(NULL, 1, 0);

#ifdef CONFIG_MTK_AEE_FEATURE
		other_free = global_page_state(NR_FREE_PAGES);
		other_file = global_page_state(NR_FILE_PAGES) -
							global_page_state(NR_SHMEM);

		lowmem_print(5, "memory %d\n", other_free + other_file);
		if (lowmem_indicator && ((other_free + other_file) >= lowmem_minfree[3]) && in_lowmem == 1) {
			lowmem_print(5, "low memory: down aee warning\n");
			in_lowmem = 0;
			DAL_LowMemoryOff();
		}
#endif

		wait_event_freezable_timeout(lowmemkill_wait, false, msecs_to_jiffies(1000));

        } while (1);

        pr_debug("lowmemorykiller exiting\n");
        return 0;
}
#endif

static int __init lowmem_init(void)
{
#ifdef MTK_LMK_THREAD
	kthread_run(lowmem_killer, NULL, "lowmemorykiller");
#else
	task_free_register(&task_nb);
	register_shrinker(&lowmem_shrinker);
#endif
	return 0;
}

static void __exit lowmem_exit(void)
{
#ifndef MTK_LMK_THREAD
	unregister_shrinker(&lowmem_shrinker);
	task_free_unregister(&task_nb);
#endif
}

module_param_named(cost, lowmem_shrinker.seeks, int, S_IRUGO | S_IWUSR);
module_param_array_named(adj, lowmem_adj, int, &lowmem_adj_size,
			 S_IRUGO | S_IWUSR);
module_param_array_named(minfree, lowmem_minfree, uint, &lowmem_minfree_size,
			 S_IRUGO | S_IWUSR);
module_param_named(debug_level, lowmem_debug_level, uint, S_IRUGO | S_IWUSR);
module_param_named(debug_adj, lowmem_debug_adj, uint, S_IRUGO | S_IWUSR);
#ifdef CONFIG_MTK_AEE_FEATURE
module_param_named(lowmem_indicator, lowmem_indicator, uint, S_IRUGO | S_IWUSR);
#endif

module_init(lowmem_init);
module_exit(lowmem_exit);

MODULE_LICENSE("GPL");

