/* drivers/misc/apanic.c
 *
 * Copyright (C) 2009 Google, Inc.
 * Author: San Mehat <san@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */
#include <asm/ptrace.h>

#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/wakelock.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/mtd/mtd.h>
#include <linux/notifier.h>
#include <linux/mtd/mtd.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/preempt.h>
#include <linux/aee.h>

#include <linux/mm.h>
#include "ipanic.h"

#ifdef CONFIG_ARCH_MT6516
#include <mach/mt6516_wdt.h>
#endif

struct mtd_ipanic_data {
	struct mtd_info		*mtd;
	struct ipanic_header	curr;
	void			*bounce;
	u32 blk_offset[IPANIC_OOPS_BLOCK_COUNT];

	struct proc_dir_entry	*oops;
};

static struct mtd_ipanic_data mtd_drv_ctx;

static int mtd_ipanic_block_scan(struct mtd_ipanic_data *ctx) 
{
	int index = 0, offset;
	
	for (offset = 0; (index < IPANIC_OOPS_BLOCK_COUNT) && (offset < ctx->mtd->size); offset += ctx->mtd->writesize) {
		if (!ctx->mtd->block_isbad(ctx->mtd, offset)) {
			ctx->blk_offset[index++] = offset;
		}
	}
	if (index != IPANIC_OOPS_BLOCK_COUNT) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "aee-ipanic: No enough partition space\n");
		return 0;
	}
	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "aee-ipanic: blocks: ");
	for (index = 0; index < IPANIC_OOPS_BLOCK_COUNT; index++) {
		xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, " %d", ctx->blk_offset[index]);
	}
	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "\n");
	return 1;
}

static int mtd_ipanic_block_read_single(struct mtd_ipanic_data *ctx, loff_t offset)
{
	int rc, len;
	int index = offset >> ctx->mtd->writesize_shift;

	if ((index < 0) || (index >= IPANIC_OOPS_BLOCK_COUNT)) {
		return -EINVAL;
	}

	rc = ctx->mtd->read(ctx->mtd, ctx->blk_offset[index], ctx->mtd->writesize, &len, ctx->bounce);
	ipanic_block_scramble(ctx->bounce, ctx->mtd->writesize);
#if 0
	if (rc == -EBADMSG) {
		xlog_printk(ANDROID_LOG_WARN, IPANIC_LOG_TAG, "Check sum error (ignore)\n");
		rc = 0;
	}
#endif
	if (rc == -EUCLEAN) {
		xlog_printk(ANDROID_LOG_WARN, IPANIC_LOG_TAG, "ECC Check sum error corrected %lld\n", offset);
		rc = 0;
	}
	if ((rc == 0) && (len != ctx->mtd->writesize)) {
		xlog_printk(ANDROID_LOG_WARN, IPANIC_LOG_TAG, "aee-ipanic: read size mismatch %d\n", len);
		return -EINVAL;
	}
	return rc;
}

static int mtd_ipanic_block_write(struct mtd_ipanic_data *ctx, loff_t to, int bounce_len)
{
	int rc;
	size_t wlen;
	int panic = in_interrupt() | in_atomic();
	int index = to >> ctx->mtd->writesize_shift;

	if ((index < 0) || (index >= IPANIC_OOPS_BLOCK_COUNT)) {
		return -EINVAL;
	}

	if (bounce_len > ctx->mtd->writesize) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "aee-ipanic(%s) len too large\n", __func__);
		return -EINVAL;
	}
	if (panic && !ctx->mtd->panic_write) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: No panic_write available\n", __func__);
		return 0;
	} else if (!panic && !ctx->mtd->write) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: No write available\n", __func__);
		return 0;
	}

	if (bounce_len < ctx->mtd->writesize)
		memset(ctx->bounce + bounce_len, 0, ctx->mtd->writesize - bounce_len);
	ipanic_block_scramble(ctx->bounce, ctx->mtd->writesize);

	if (panic)
		rc = ctx->mtd->panic_write(ctx->mtd, ctx->blk_offset[index], ctx->mtd->writesize, &wlen, ctx->bounce);
	else
		rc = ctx->mtd->write(ctx->mtd, ctx->blk_offset[index], ctx->mtd->writesize, &wlen, ctx->bounce);

	if (rc) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,
		       "%s: Error writing data to flash (%d)\n",
		       __func__, rc);
		return rc;
	}

	return wlen;
}

static int mtd_ipanic_block_read(struct mtd_ipanic_data *ctx, off_t file_offset, int file_length, void *buf)
{
#if 0
	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "%s: ctx:%p file_offset:%d file_length:%lu\n", __func__, ctx, file_offset, file_length);
#endif
	while (file_length > 0) {
		unsigned int page_no;
		off_t page_offset;
		int rc;
		size_t count = file_length;

		/* We only support reading a maximum of a flash page */
		if (count > ctx->mtd->writesize)
			count = ctx->mtd->writesize;
		page_no = file_offset / ctx->mtd->writesize;
		page_offset = file_offset % ctx->mtd->writesize;

		rc = mtd_ipanic_block_read_single(ctx, page_no * ctx->mtd->writesize);
		if (rc < 0) {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "aee-ipanic(%s): mtd read error page_no(%d) error(%d)\n", __func__, page_no, rc);
			goto error_return;
		}
		if (page_offset)
			count -= page_offset;
		memcpy(buf, ctx->bounce + page_offset, count);

		file_length -= count;
		buf += count;
		file_offset += count;
	}
	return 0;
error_return:
	return -1;
}

static void mtd_ipanic_block_erase_callback(struct erase_info *done)
{
	wait_queue_head_t *wait_q = (wait_queue_head_t *) done->priv;
	wake_up(wait_q);
}

static void mtd_ipanic_block_erase(void)
{
	struct mtd_ipanic_data *ctx = &mtd_drv_ctx;
	struct erase_info erase;
	DECLARE_WAITQUEUE(wait, current);
	wait_queue_head_t wait_q;
	int rc, i;

	init_waitqueue_head(&wait_q);
	erase.mtd = ctx->mtd;
	erase.callback = mtd_ipanic_block_erase_callback;
	erase.len = ctx->mtd->erasesize;
	erase.priv = (u_long)&wait_q;
	for (i = 0; i < ctx->mtd->size; i += ctx->mtd->erasesize) {
		erase.addr = i;
		set_current_state(TASK_INTERRUPTIBLE);
		add_wait_queue(&wait_q, &wait);

		rc = ctx->mtd->block_isbad(ctx->mtd, erase.addr);
		if (rc < 0) {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,
			       "aee-ipanic: Bad block check "
			       "failed (%d)\n", rc);
			goto out;
		}
		if (rc) {
			xlog_printk(ANDROID_LOG_WARN, IPANIC_LOG_TAG,
			       "aee-ipanic: Skipping erase of bad "
			       "block @%llx\n", erase.addr);
			set_current_state(TASK_RUNNING);
			remove_wait_queue(&wait_q, &wait);
			continue;
		}

		rc = ctx->mtd->erase(ctx->mtd, &erase);
		if (rc) {
			set_current_state(TASK_RUNNING);
			remove_wait_queue(&wait_q, &wait);
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,
			       "aee-ipanic: Erase of 0x%llx, 0x%llx failed\n",
			       (unsigned long long) erase.addr,
			       (unsigned long long) erase.len);
			if (rc == -EIO) {
				if (ctx->mtd->block_markbad(ctx->mtd,
							    erase.addr)) {
					xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,
					       "aee-ipanic: Err marking blk bad\n");
					goto out;
				}
				xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG,
				       "aee-ipanic: Marked a bad block"
				       " @%llx\n", erase.addr);
				continue;
			}
			goto out;
		}
		schedule();
		remove_wait_queue(&wait_q, &wait);
	}
	xlog_printk(ANDROID_LOG_DEBUG, IPANIC_LOG_TAG, "aee-ipanic: %s partition erased\n",
	       AEE_IPANIC_PLABEL);
out:
	return;
}

static int mtd_ipanic_proc_oops(char *page, char **start,
				off_t off, int count,
				int *eof, void *data)
{
	int len;
	struct aee_oops *oops = ipanic_oops_copy();
	len = sprintf(page, "aee-ipanic Oops\n");
	if (oops) {
		len += sprintf(page + len, "Process\n%s\nBacktrace\n%s", oops->process_path,
			       oops->backtrace);
		ipanic_oops_free(oops, 0);
	}
	else {
		len += sprintf(page + len, "No available\n");
	}
	return len;
}

static void mtd_ipanic_oops_dump(void)
{
	struct aee_oops *oops = ipanic_oops_copy();
	if (oops != NULL) {
		char *log_buf;
		int i;

		xlog_printk(ANDROID_LOG_WARN, IPANIC_LOG_TAG, "[---- Panic Log ----]\n");
		xlog_printk(ANDROID_LOG_WARN, IPANIC_LOG_TAG, "Process path: %s\n", oops->process_path);
		xlog_printk(ANDROID_LOG_WARN, IPANIC_LOG_TAG, "Backtrace:\n%s", oops->backtrace);

                xlog_printk(ANDROID_LOG_WARN, IPANIC_LOG_TAG, "Detail:\n");
                log_buf = kmalloc(1024, GFP_KERNEL);
		for (i = 0; i < oops->detail_len; i+= 1023)
		{
			int len = (oops->detail_len - i) > 1023 ? 1023 : oops->detail_len - i;
			memcpy(log_buf, oops->detail + i, len);
			log_buf[len] = 0;

			xlog_printk(ANDROID_LOG_WARN, IPANIC_LOG_TAG, "%s", log_buf);
		}
                kfree(log_buf);
		xlog_printk(ANDROID_LOG_WARN, IPANIC_LOG_TAG, "\n[---- Panic Log End ----]\n");

		ipanic_oops_free(oops, 0);
	}
	else {
		xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "[---- No Panic Log available ----]\n");
	}
}

static void mtd_panic_notify_add(struct mtd_info *mtd)
{
	struct mtd_ipanic_data *ctx = &mtd_drv_ctx;
	struct ipanic_header *hdr = ctx->bounce;
	int rc;

	if (strcmp(mtd->name, AEE_IPANIC_PLABEL))
		return;

	ctx->mtd = mtd;

	if (!mtd_ipanic_block_scan(ctx))
		goto out_err;

	rc = mtd_ipanic_block_read_single(ctx, 0);
	if (rc < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "aee-ipanic: Error reading block 0 (%d)\n", rc);
		mtd_ipanic_block_erase();
		goto out_err;
	}

		xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "aee-ipanic: Bound to mtd partition '%s'\n", mtd->name);

	switch (ipanic_check_header(hdr)) {
	case 0:
		break;
	case 1:
		return;
	case 2:
		mtd_ipanic_block_erase();
		return;
	}

	memcpy(&ctx->curr, hdr, sizeof(struct ipanic_header));

	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "aee-ipanic: c(%u, %u) oh(%u, %u) od(%u, %u)\n",
	       hdr->console_offset, hdr->console_length,
	       hdr->oops_header_offset, hdr->oops_header_length,
	       hdr->oops_detail_offset, hdr->oops_detail_length);

	ctx->oops = create_proc_read_entry("aee_ipanic_oops", 
					   0444, NULL, 
					   mtd_ipanic_proc_oops,
					   NULL);
	if (ctx->oops == NULL) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, " %s: failed crate procfile apanic_oops\n", __func__);
	}

	mtd_ipanic_oops_dump();
	return;

out_err:
	ctx->mtd = NULL;
}

static void mtd_panic_notify_remove(struct mtd_info *mtd)
{
	struct mtd_ipanic_data *ctx = &mtd_drv_ctx;
	if (mtd == ctx->mtd) {
		ctx->mtd = NULL;
		xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "aee-ipanic: Unbound from %s\n", mtd->name);
	}
}

static struct mtd_notifier mtd_panic_notifier = {
	.add	= mtd_panic_notify_add,
	.remove	= mtd_panic_notify_remove,
};

static int in_panic = 0;

/*
 * Writes the contents of the console to the specified offset in flash.
 * Returns number of bytes written
 */
static int ipanic_write_log_buf(struct mtd_info *mtd, unsigned int off, int log_copy_start, int log_copy_end)
{
	struct mtd_ipanic_data *ctx = &mtd_drv_ctx;
	int saved_oip;
	int rc, rc2;
	unsigned int last_chunk = 0, copy_count = 0;

	while (!last_chunk) {
		saved_oip = oops_in_progress;
		oops_in_progress = 1;
		rc = log_buf_copy2(ctx->bounce, mtd->writesize, log_copy_start, log_copy_end);
		BUG_ON(rc < 0);
		log_copy_start += rc;
		copy_count += rc;
		if (rc != mtd->writesize)
			last_chunk = rc;

		oops_in_progress = saved_oip;
		if (rc <= 0)
			break;

		rc2 = mtd_ipanic_block_write(ctx, off, rc);
		if (rc2 <= 0) {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,
			       "aee-ipanic: Flash write failed (%d)\n", rc2);
			return rc2;
		}
		off += rc2;
	}
	return copy_count;
}

// QHQ add for android log ;
static int ipanic_write_android_buf(struct mtd_info *mtd, unsigned int off, int type)
{
	struct mtd_ipanic_data *ctx = &mtd_drv_ctx;
	int saved_oip;
	int rc, rc2;
	unsigned int last_chunk = 0, copy_count = 0;

	while (!last_chunk) {
		saved_oip = oops_in_progress;
		oops_in_progress = 1;
		rc = panic_dump_android_log(ctx->bounce, mtd->writesize, type);
		BUG_ON(rc < 0);
		copy_count += rc;
		if (rc == 0)
			last_chunk = rc;

		oops_in_progress = saved_oip;
		if (rc <= 0)
			break;

		rc2 = mtd_ipanic_block_write(ctx, off, rc);
		if (rc2 <= 0) {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,
			       "aee-ipanic: Flash write failed (%d)\n", rc2);
			return rc2;
		}
		off += rc2;
	}
	//xlog_printk(ANDROID_LOG_DEBUG, IPANIC_LOG_TAG, "dump droid log type %d, count %d\n", type, copy_count) ;
	return copy_count;
}

/*
 * Writes the contents of the console to the specified offset in flash.
 * Returns number of bytes written
 */
static int mtd_ipanic_write_console(struct mtd_info *mtd, unsigned int off)
{
    #define __LOG_BUF_LEN	(1 << CONFIG_LOG_BUF_SHIFT)
    if (log_end > __LOG_BUF_LEN)
	    return ipanic_write_log_buf(mtd, off, log_end-__LOG_BUF_LEN, log_end);
	else
	    return ipanic_write_log_buf(mtd, off, 0, log_end);
}

static int mtd_ipanic_write_oops_header(struct mtd_info *mtd, unsigned int off)
{
	int wlen = 0, rc;
	struct mtd_ipanic_data *ctx = &mtd_drv_ctx;
	u8 *raw_oops_header = (u8 *)&oops_header;
	while (wlen < sizeof(struct ipanic_oops_header)) {
		int writesize = sizeof(struct ipanic_oops_header) - wlen;
		if (writesize > mtd->writesize)
			writesize = mtd->writesize;

		memcpy(ctx->bounce, raw_oops_header + wlen, writesize);
		rc = mtd_ipanic_block_write(ctx, off + wlen, writesize);
		if (rc < 0) {
			return rc;
		}
		wlen += writesize;
	}
	return wlen;
}

static int ipanic_write_oops_detail(struct mtd_info *mtd, unsigned int off)
{
	return ipanic_write_log_buf(mtd, off, ipanic_detail_start, ipanic_detail_end);
}

static struct aee_oops *mtd_ipanic_oops_copy(void)
{
	struct mtd_ipanic_data *ctx = &mtd_drv_ctx;
	struct aee_oops *oops;

	if ((ctx->curr.magic != AEE_IPANIC_MAGIC) || (ctx->curr.version != AEE_IPANIC_PHDR_VERSION)) {
		return NULL;
	}

	oops = aee_oops_create(AE_DEFECT_FATAL, IPANIC_MODULE_TAG);
	if (oops != NULL) {
		struct ipanic_oops_header *oops_header = kzalloc(sizeof(struct ipanic_oops_header), GFP_KERNEL);
		if (oops_header == NULL)
			goto error_return;

		if (mtd_ipanic_block_read(ctx, ctx->curr.oops_header_offset, ctx->curr.oops_header_length, oops_header) != 0) {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: mtd read header failed\n", __FUNCTION__);
			kfree(oops_header);
			goto error_return;
		}
		aee_oops_set_process_path(oops, oops_header->process_path);
		aee_oops_set_backtrace(oops, oops_header->backtrace);
		kfree(oops_header);

		oops->detail = kmalloc(ctx->curr.oops_detail_length, GFP_KERNEL);
		oops->detail_len = ctx->curr.oops_detail_length;
		if (oops->detail != NULL) {
			if (mtd_ipanic_block_read(ctx, ctx->curr.oops_detail_offset, ctx->curr.oops_detail_length, oops->detail) != 0) {
				xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: mtd read detail failed\n", __FUNCTION__);
				kfree(oops->detail);
				goto error_return;
			}
		}
		else {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: kmalloc failed at detail\n", __FUNCTION__);
			kfree(oops);
			return NULL;
		}

		oops->console = kmalloc(ctx->curr.console_length, GFP_KERNEL);
		oops->console_len = ctx->curr.console_length;
		if (oops->console != NULL) {
			if (mtd_ipanic_block_read(ctx, ctx->curr.console_offset, ctx->curr.console_length, oops->console) != 0) {
				xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: mtd read console failed\n", __FUNCTION__);
				kfree(oops->detail);
				goto error_return;
			}
		}
		else {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: kmalloc failed at detail\n", __FUNCTION__);
			kfree(oops);
			return NULL;
		}
		
// QHQ add for android log ;		
		oops->android_main      = kmalloc(ctx->curr.android_main_length, GFP_KERNEL);
		oops->android_main_len  = ctx->curr.android_main_length ;
		if (oops->android_main)
		{
			if (mtd_ipanic_block_read(ctx, ctx->curr.android_main_offset, ctx->curr.android_main_length, oops->android_main) != 0) 
			{
				xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: mtd read android_main failed\n", __FUNCTION__);
				kfree(oops->detail);
				kfree(oops->console);
				goto error_return;
			}
		}
		else
		{
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: kmalloc failed at android_main\n", __FUNCTION__);
			aee_oops_free(oops);
			return NULL;
		}
		
/*		
		oops->android_event     = kmalloc(ctx->curr.android_event_length, GFP_KERNEL);
		oops->android_event_len  = ctx->curr.android_event_length ;
		if (oops->android_event)
		{
			if (mtd_ipanic_block_read(ctx, ctx->curr.android_event_offset, ctx->curr.android_event_length, oops->android_event) != 0) 
			{
				xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: mtd read android_event failed\n", __FUNCTION__);
				kfree(oops->detail) ;
				kfree(oops->console) ;
				kfree(oops->android_main) ;
				goto error_return;
			}
		}
		else
		{
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: kmalloc failed at android_event\n", __FUNCTION__);
			aee_oops_free(oops);
			return NULL;
		}
*/
		oops->android_radio     = kmalloc(ctx->curr.android_radio_length, GFP_KERNEL);
		oops->android_radio_len  = ctx->curr.android_radio_length ;
		if (oops->android_radio)
		{
			if (mtd_ipanic_block_read(ctx, ctx->curr.android_radio_offset, ctx->curr.android_radio_length, oops->android_radio) != 0) 
			{
				xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: mtd read android_radio failed\n", __FUNCTION__);
				kfree(oops->detail) ;
				kfree(oops->console) ;
				kfree(oops->android_main) ;
//				kfree(oops->android_event) ;
				goto error_return;
			}		    
		}
		else
		{
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: kmalloc failed at android_radio\n", __FUNCTION__);
			aee_oops_free(oops);
			return NULL;
		}
		
		oops->android_system    = kmalloc(ctx->curr.android_system_length, GFP_KERNEL);
		oops->android_system_len  = ctx->curr.android_system_length ;
		if (oops->android_system)
		{
			if (mtd_ipanic_block_read(ctx, ctx->curr.android_system_offset, ctx->curr.android_system_length, oops->android_system) != 0) 
			{
				xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: mtd read android_system failed\n", __FUNCTION__);
				kfree(oops->detail) ;
				kfree(oops->console) ;
				kfree(oops->android_main) ;
//				kfree(oops->android_event) ;
				kfree(oops->android_radio) ;
				goto error_return;
			}		    
		}
		else
		{
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: kmalloc failed at android_system\n", __FUNCTION__);
			aee_oops_free(oops);
			return NULL;
		}
		
//		xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "android log length, 0x%x, 0x%x, 0x%x, 0x%x\n", 
//			oops->android_main_len,oops->android_event_len,oops->android_radio_len,oops->android_system_len) ;
// QHQ add for android log ; --		
		oops->userspace_info    = kmalloc(ctx->curr.userspace_info_length, GFP_KERNEL);
		oops->userspace_info_len  = ctx->curr.userspace_info_length ;
		if (oops->userspace_info)
		{
			if (mtd_ipanic_block_read(ctx, ctx->curr.userspace_info_offset, ctx->curr.userspace_info_length, oops->userspace_info) != 0) 
			{
				xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: mtd read usrespace info failed\n", __FUNCTION__);
				kfree(oops->detail) ;
				kfree(oops->console) ;
				kfree(oops->android_main) ;
//				kfree(oops->android_event) ;
				kfree(oops->android_radio) ;
				kfree(oops->userspace_info) ;
				goto error_return;
			}		    
		}
		else
		{
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: kmalloc failed at userspace info failed\n", __FUNCTION__);
			aee_oops_free(oops);
			return NULL;
		}
		
		xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "android userspaceinfo length, 0x%x\n", oops->userspace_info_len) ; 
		xlog_printk(ANDROID_LOG_DEBUG, IPANIC_LOG_TAG, "ipanic_oops_copy return OK\n") ;
		return oops;
	}
	else {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: kmalloc failed at header\n", __FUNCTION__);
		return NULL;
	}
error_return:
	kfree(oops);
	memset(&ctx->curr, 0, sizeof(struct ipanic_header));
	mtd_ipanic_block_erase();
	return NULL;
}

static void mtd_ipanic_oops_free(struct aee_oops *oops, int erase)
{
	if (oops) {
		struct mtd_ipanic_data *ctx = &mtd_drv_ctx;

		aee_oops_free(oops);

		if (erase) {
			memset(&ctx->curr, 0, sizeof(struct ipanic_header));
			mtd_ipanic_block_erase();
		}
	}
}

// QHQ add for android log ;
static int android_main_offset, android_main_length , 
		android_event_offset, android_event_length ,
		android_radio_offset, android_radio_length ,
		android_system_offset, android_system_length,userspace_info_offset,userspace_info_length ;


// lhd add for user space info when KE ;
#define MAX_NATIVEINFO  32*1024
#define MAX_NATIVEHEAP  2048

static char NativeInfo[MAX_NATIVEINFO]; //check that 32k is enought??
static unsigned long User_Stack[MAX_NATIVEHEAP];//8K Heap

void _LOG(const char *fmt, ...)
{
    char buf[256];
	int len=0;
    va_list ap;
    va_start(ap, fmt);
	len = strlen(NativeInfo);
	if(len+sizeof(buf)<MAX_NATIVEINFO)
    	vsnprintf(&NativeInfo[len], sizeof(buf), fmt, ap);
    
}


void dump_registers(struct pt_regs *r)
{
   if(r!=NULL)
   	{
    _LOG(" r0 %08x  r1 %08x  r2 %08x  r3 %08x\n",
         r->ARM_r0, r->ARM_r1, r->ARM_r2, r->ARM_r3);
    _LOG(" r4 %08x  r5 %08x  r6 %08x  r7 %08x\n",
         r->ARM_r4, r->ARM_r5, r->ARM_r6, r->ARM_r7);
    _LOG(" r8 %08x  r9 %08x  10 %08x  fp %08x\n",
         r->ARM_r8, r->ARM_r9, r->ARM_r10, r->ARM_fp);
    _LOG(" ip %08x  sp %08x  lr %08x  pc %08x  cpsr %08x\n",
         r->ARM_ip, r->ARM_sp, r->ARM_lr, r->ARM_pc, r->ARM_cpsr);
   	}
}

static int DumpNativeInfo (void)
{
    struct task_struct *current_task ;
    struct pt_regs *user_ret=NULL ; 
    struct vm_area_struct *vma;
	unsigned long userstack_start = NULL ;
    unsigned long userstack_end = NULL,length=0;
	int mapcount=0;
    struct file *file;//=vm_struct->vm_file;	
	int flags ;//= vm_struct->vm_flags;
	struct mm_struct *mm ;//= vma->vm_mm;
	int ret=0,i=0;
	//unsigned long *User_Stack=NULL; //8k user stack
	//User_Stack= kmalloc(4096*2, GFP_KERNEL);
    current_task = get_current() ;
    user_ret = task_pt_regs(current_task) ;
    
    if (!user_mode(user_ret))
        return 0 ;
   
    //printk ("start to dump native stack,UserStack(0x%x),sizeof_int:%d,sizeof_long:%d\n",User_Stack,sizeof(int),sizeof(long)) ;
	dump_registers(user_ret);
    
	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "pc/lr/sp 0x%08x/0x%08x/0x%08x\n", user_ret->ARM_pc, user_ret->ARM_lr, user_ret->ARM_sp) ;   
    _LOG("pc/lr/sp 0x%08x/0x%08x/0x%08x\n", user_ret->ARM_pc, user_ret->ARM_lr, user_ret->ARM_sp) ;
    userstack_start = (unsigned long)user_ret->ARM_sp ;
    if(current_task->mm==NULL)
    		return 0 ;
    vma = current_task->mm->mmap ;
    while (vma && (mapcount< current_task->mm->map_count))
    {
		file=vma->vm_file;
		flags = vma->vm_flags;
		if(file)
		{
			
			_LOG("%08lx-%08lx %c%c%c%c    %s\n",vma->vm_start,vma->vm_end,
					flags & VM_READ ? 'r' : '-',
					flags & VM_WRITE ? 'w' : '-',
					flags & VM_EXEC ? 'x' : '-',
					flags & VM_MAYSHARE ? 's' : 'p',file->f_path.dentry->d_iname);
		}
		else
		{
			const char *name = arch_vma_name(vma);
			mm = vma->vm_mm;
			if (!name) 
			{
				if (mm) 
				{
					if (vma->vm_start <= mm->start_brk &&
							vma->vm_end >= mm->brk) {
						name = "[heap]";
					} else if (vma->vm_start <= mm->start_stack &&
						   vma->vm_end >= mm->start_stack) {
						name = "[stack]";
					}
				} 
				else 
				{
					name = "[vdso]";
				}
			}
			//if (name) 
			{	
				
				_LOG("%08lx-%08lx %c%c%c%c    %s\n",vma->vm_start,vma->vm_end,
					flags & VM_READ ? 'r' : '-',
					flags & VM_WRITE ? 'w' : '-',
					flags & VM_EXEC ? 'x' : '-',
					flags & VM_MAYSHARE ? 's' : 'p',name);
			
			
			}
		}
        vma = vma->vm_next ;
		mapcount++;
        
    }
	vma = current_task->mm->mmap ;
    while (vma != NULL)
    {
        if (vma->vm_start <= userstack_start 
            && vma->vm_end >= userstack_start)
        {
            userstack_end= vma->vm_end ;
            break ;
        }
        vma = vma->vm_next ;
        if (vma == current_task->mm->mmap)
        {
            break ;
        }
    }
    if (userstack_end ==0)
    {
	xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "Dump native stack failed:\n") ;
        return 0 ;
    }
                
	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "Dump stack range (0x%08x:0x%08x)\n", userstack_start, userstack_end) ;
	_LOG ("Dump stack range (0x%08x:0x%08x)\n", userstack_start, userstack_end) ;

    length = ((userstack_end-userstack_start) < (sizeof(User_Stack) - 1)) ? (userstack_end-userstack_start) : (sizeof(User_Stack) - 1);
	ret=copy_from_user((void *)(User_Stack), (const void __user *)( userstack_start), length);
	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "copy_from_user ret(0x%08x),len:%x\n",ret,length) ;
	i=0;
    while (userstack_start< userstack_end)
    {
        //printk ("0x%08x: 0x%08x\n", userstack_start, *((unsigned long *)userstack_start),i);	
        _LOG ("0x%08x: 0x%08x\n", userstack_start, User_Stack[i]);
		userstack_start+=4;
		i++;
    }
    
	xlog_printk(ANDROID_LOG_DEBUG, IPANIC_LOG_TAG, "end dump native stack:\n") ;
	_LOG ("end dump native stack:\n") ;
    return 0 ;
}

int panic_dump_user_info(char *buf, size_t size) {
	static size_t offset = 0; //offset of log buffer
	static int isFirst = 0;
	size_t ret = 0;
	size_t len = 0;
	size_t distance = 0;
	size_t realsize = 0;
	
	distance = (strlen(NativeInfo) - offset);
	if(distance > size)
		realsize = size;
	else
		realsize = distance;
	memcpy(buf, NativeInfo + offset, realsize);
	offset += realsize;
	return realsize;
}

static int ipanic_write_userspace(struct mtd_info *mtd, unsigned int off)
{
	struct mtd_ipanic_data *ctx = &mtd_drv_ctx;
	int saved_oip;
	int rc, rc2;
	unsigned int last_chunk = 0, copy_count = 0;

	while (!last_chunk) {
		saved_oip = oops_in_progress;
		oops_in_progress = 1;
		rc = panic_dump_user_info(ctx->bounce, mtd->writesize);
		BUG_ON(rc < 0);
		copy_count += rc;
		if (rc == 0)
			last_chunk = rc;

		oops_in_progress = saved_oip;
		if (rc <= 0)
			break;

		rc2 = mtd_ipanic_block_write(ctx, off, rc);
		if (rc2 <= 0) {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,
			       "ipanic_write_userspace: Flash write failed (%d)\n", rc2);
			return rc2;
		}
		off += rc2;
	}
	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "ipanic_write_userspace, count %d,strlen(NativeInfo):%d,off:%x\n", copy_count,strlen(NativeInfo),off) ;
	return copy_count;
}

static int ipanic_write_userspace_buf (struct mtd_ipanic_data *ctx, int offset)
{
	int rc ;
	// userspace buffer:
	offset = ALIGN(offset, ctx->mtd->writesize) ;
	
	rc = ipanic_write_userspace(ctx->mtd, offset) ; // userspace buffer
	if (rc < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "Error writing android log(1) to panic log! (%d)\n", rc);
		rc = 0;
	}
	userspace_info_offset = offset ;
	userspace_info_length = rc ;
	return offset;
}




static int ipanic_write_all_android_buf (struct mtd_ipanic_data *ctx, int offset, struct ipanic_header *hdr)
{
	int rc ;
	
	// main buffer:
	offset = ALIGN(offset, ctx->mtd->writesize) ;
	
	rc = ipanic_write_android_buf (ctx->mtd, offset, 1) ; // main buffer
	if (rc < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "Error writing android log(1) to panic log! (%d)\n", rc);
		rc = 0;
	}
	android_main_offset = offset ;
	android_main_length = rc ;
	
	offset += rc ;

/*
	// event buffer:
	offset = ALIGN(offset, ctx->mtd->writesize);
	
	rc = ipanic_write_android_buf (ctx->mtd, offset, 2) ; // event buffer
	if (rc < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "Error writing android log(2) to panic log! (%d)\n", rc);
		rc = 0;
	}
	android_event_offset = offset ;
	android_event_length = rc ;
	
	offset += rc ;
*/	
	// radio buffer:
	offset = ALIGN(offset, ctx->mtd->writesize);
	
	rc = ipanic_write_android_buf (ctx->mtd, offset, 3) ; // radio buffer
	if (rc < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "Error writing android log(3) to panic log! (%d)\n", rc);
		rc = 0;
	}
	android_radio_offset = offset ;
	android_radio_length = rc ;

	offset += rc ;
	

	// system buffer:
	offset = ALIGN(offset, ctx->mtd->writesize);

	rc = ipanic_write_android_buf (ctx->mtd, offset, 4) ; // system buffer.
	if (rc < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "Error writing android log(4) to panic log! (%d)\n", rc);
		rc = 0;
	}
    
	android_system_offset = offset ;
	android_system_length = rc ;

	offset += rc ;
	
    
	return offset;
}

static int mtd_ipanic(struct notifier_block *this, unsigned long event,
		      void *ptr)
{
	struct mtd_ipanic_data *ctx = &mtd_drv_ctx;
	struct ipanic_header *hdr = (struct ipanic_header *) ctx->bounce;
	int console_offset = 0;
	int console_len = 0;
	int userinfo_offset = 0;
	int userinfo_len = 0;
	int oops_header_offset = 0;
	int oops_header_len = 0;
	int oops_detail_offset = 0;
	int oops_detail_len = 0;
	int rc;

	if (in_panic)
		return NOTIFY_DONE;

	in_panic = 1;
#ifdef CONFIG_PREEMPT
	/* Ensure that cond_resched() won't try to preempt anybody */
	add_preempt_count(PREEMPT_ACTIVE);
#endif

	if (!ctx->mtd)
		goto out;
#if 0
	if (ctx->curr.magic) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "Crash partition in use!\n");
		goto out;
	}
#endif

#if defined(CONFIG_ARCH_MT6516)
	/* Disable hardware watchdog to prevent reboot in progress of write */
	mt6516_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE);
#endif

	/*
	 * Write out the console
	 * Section 0 is reserved for ipanic header, we start at section 1
	 */
	oops_header_offset =  ctx->mtd->writesize;
	oops_header_len = mtd_ipanic_write_oops_header(ctx->mtd, oops_header_offset);
	if (oops_header_len < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "Error writing oops header to panic log! (%d)\n",
		       oops_header_len);
		oops_header_len  = 0;
	}

	oops_detail_offset = ALIGN(oops_header_offset + oops_header_len,
				   ctx->mtd->writesize);
	oops_detail_len = ipanic_write_oops_detail(ctx->mtd, oops_detail_offset);
	if (oops_detail_len < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "Error writing oops header to panic log! (%d)\n",
		       oops_detail_len);
		oops_detail_len  = 0;
	}

	console_offset = ALIGN(oops_detail_offset + oops_detail_len,
			       ctx->mtd->writesize);
	console_len = mtd_ipanic_write_console(ctx->mtd, console_offset);
	if (console_len < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,  "Error writing console to panic log! (%d)\n",
		       console_len);
		console_len = 0;
	}
	
	memset(ctx->bounce, 0, PAGE_SIZE);
	DumpNativeInfo();
	userinfo_offset=ALIGN(console_offset + console_len,
				   ctx->mtd->writesize);
	userinfo_len=ipanic_write_userspace_buf(ctx, console_offset+console_len);
	xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "console_offset (%d),console_len(%d),ctx->mtd->writesize:%x,userinfo_offset:%x,userinfo_len:%x\n",
		       console_offset,console_len,ctx->mtd->writesize,userinfo_offset,userinfo_len);
	/*
	 * Finally write the ipanic header
	 */
	memset(ctx->bounce, 0, PAGE_SIZE);

// QHQ add for android log ;
// notic that hdr is equal ctx->bounce, and in the following function we will 
// modify the content of hdr, so, this function call MUST after memset operation.
	rc = ipanic_write_all_android_buf (ctx, userinfo_offset+userinfo_len, hdr) ;
// QHQ add for android log ; --    


	hdr->magic = AEE_IPANIC_MAGIC;
	hdr->version = AEE_IPANIC_PHDR_VERSION;

	hdr->oops_header_offset = oops_header_offset;
	hdr->oops_header_length = oops_header_len;

	hdr->oops_detail_offset = oops_detail_offset;
	hdr->oops_detail_length = oops_detail_len;

	hdr->console_offset = console_offset;
	hdr->console_length = console_len;
	hdr->android_main_offset = android_main_offset ;
	hdr->android_main_length = android_main_length ;
//	hdr->android_event_offset = android_event_offset ;
//	hdr->android_event_length = android_event_length ;
	hdr->android_radio_offset = android_radio_offset ;
	hdr->android_radio_length = android_radio_length ;
	hdr->android_system_offset = android_system_offset ;
	hdr->android_system_length = android_system_length ;
	hdr->userspace_info_offset= userspace_info_offset ;
	hdr->userspace_info_length= userspace_info_length;
/*
	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "header : 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x ,0x%x, 0x%x\n", 
		hdr->android_main_offset, hdr->android_main_length, 
		hdr->android_event_offset, hdr->android_event_length, 
		hdr->android_radio_offset, hdr->android_radio_length, 
		hdr->android_system_offset, hdr->android_system_length,
		hdr->userspace_info_offset,hdr->userspace_info_length
		) ;
*/
	rc = mtd_ipanic_block_write(ctx, 0, sizeof(struct ipanic_header));
	if (rc <= 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,  "aee-ipanic: Header write failed (%d)\n",
		       rc);
		goto out;
	}
	xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "aee-ipanic: Panic dump sucessfully written to flash (detail len: %d, console len: %d)\n", oops_detail_len, console_len);

out:
#ifdef CONFIG_PREEMPT
	sub_preempt_count(PREEMPT_ACTIVE);
#endif
	in_panic = 0;
	return NOTIFY_DONE;
}

static struct notifier_block panic_blk = {
	.notifier_call	= mtd_ipanic,
};

static struct ipanic_ops mtd_ipanic_ops = {
	.oops_copy = mtd_ipanic_oops_copy,
	.oops_free = mtd_ipanic_oops_free,
};

int __init aee_mtd_ipanic_init(void)
{
	if (!IsEmmc()) {
		register_mtd_user(&mtd_panic_notifier);
		atomic_notifier_chain_register(&panic_notifier_list, &panic_blk);
		memset(&mtd_drv_ctx, 0, sizeof(mtd_drv_ctx));
		mtd_drv_ctx.bounce = (void *) __get_free_page(GFP_KERNEL);
		register_ipanic_ops(&mtd_ipanic_ops);

		xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "aee-mtd-ipanic: startup, partition assgined %s\n",
		       AEE_IPANIC_PLABEL);
	}
	return 0;
}

module_init(aee_mtd_ipanic_init);
