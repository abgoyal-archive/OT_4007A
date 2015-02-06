#if !defined(__AEE_IPANIC_H__)
#define __AEE_IPANIC_H__

#include <linux/autoconf.h>
#include <linux/kallsyms.h>
#include <linux/xlog.h>

#define AEE_IPANIC_PLABEL "expdb"

#define IPANIC_MODULE_TAG "AEE-IPANIC"
#define IPANIC_LOG_TAG "aee/ipanic"
#define IPANIC_OOPS_BLOCK_COUNT 320

#define AEE_IPANIC_MAGIC 0xaee0dead
#define AEE_IPANIC_PHDR_VERSION   0x03
#define AEE_IPANIC_DATALENGTH_MAX (512 * 1024)

struct ipanic_header {
	/* The magic/version field cannot be moved or resize */
	u32 magic;
	u32 version;

	u32 oops_header_offset;
	u32 oops_header_length;

	u32 oops_detail_offset;
	u32 oops_detail_length;

	u32 console_offset;
	u32 console_length;

// QHQ add for android log ;	
	u32 android_main_offset;
	u32 android_main_length;
	
	u32 android_event_offset;
	u32 android_event_length;
	
	u32 android_radio_offset;
	u32 android_radio_length;
	
	u32 android_system_offset;
	u32 android_system_length;
  // QHQ add for android log ; --

  // lhd add for android log ;	
  u32 userspace_info_offset;
  u32 userspace_info_length;
	 // lhd add for android log ; --
};

#define IPANIC_OOPS_HEADER_PROCESS_NAME_LENGTH 256
#define IPANIC_OOPS_HEADER_BACKTRACE_LENGTH 3840

struct ipanic_oops_header 
{
	char process_path[IPANIC_OOPS_HEADER_PROCESS_NAME_LENGTH];
	char backtrace[IPANIC_OOPS_HEADER_BACKTRACE_LENGTH];
};

struct ipanic_ops {

	struct aee_oops *(*oops_copy)(void);
  
	void (*oops_free)(struct aee_oops *oops, int erase);
};

void register_ipanic_ops(struct ipanic_ops *op);

extern int log_buf_copy2(char *dest, int dest_len, int log_copy_start, int log_copy_end);

struct aee_oops *ipanic_oops_copy(void);

void ipanic_oops_free(struct aee_oops *oops, int erase);

extern int panic_dump_android_log(char *buf, size_t size, int type) ;

extern unsigned log_start;

extern unsigned log_end;

extern unsigned ipanic_detail_start;

extern unsigned ipanic_detail_end;

extern struct ipanic_oops_header oops_header;

/*
 * Check if valid header is legitimate
 * return
 *  0: contain good panic data 
 *  1: no panic data
 *  2: contain bad panic data
 */
int ipanic_check_header(const struct ipanic_header *hdr);

void ipanic_block_scramble(u8 *buf, int buflen);

void ipanic_oops_start(const char *str, int err, struct task_struct *tsk);

void ipanic_oops_end(void);

/* support emmc */
int IsEmmc(void);

int card_dump_func_read(unsigned char* buf, unsigned int len, unsigned int offset, unsigned int dev);

int card_dump_func_write(unsigned char* buf, unsigned int len, unsigned int offset, unsigned int dev);

#endif
