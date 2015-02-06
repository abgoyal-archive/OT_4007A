#if !defined(__AEE_H__)
#define __AEE_H__

#define AEE_MODULE_NAME_LENGTH 64
#define AEE_PROCESS_NAME_LENGTH 256
#define AEE_BACKTRACE_LENGTH 4096

typedef enum {
    AE_DEFECT_FATAL,
    AE_DEFECT_EXCEPTION,
    AE_DEFECT_WARNING,
    AE_DEFECT_REMINDING,
    AE_DEFECT_ATTR_END
} AE_DEFECT_ATTR;

struct aee_oops
{
	AE_DEFECT_ATTR attr;
	char module[AEE_MODULE_NAME_LENGTH];
	char process_path[AEE_PROCESS_NAME_LENGTH];
	char backtrace[AEE_BACKTRACE_LENGTH];
	char *detail;
	int detail_len;
	char *console;
	int console_len;
// QHQ add for android log ;	
	char *android_main;
	int android_main_len;
//	char *android_event;
//	int android_event_len;
	char *android_radio;
	int android_radio_len;
	char *android_system;
	int android_system_len;
// QHQ add for android log ;		

// lhd add for android log ;	
	char *userspace_info;
	int  userspace_info_len;

// lhd add for android log ;	


};

struct aee_kernel_api {
    void (*kernel_exception)(const char *module, const char *msg);
    void (*kernel_warning)(const char *module, const char *msg);
    void (*kernel_reminding)(const char *module, const char *msg);
    void (*md_exception1)(const int *log, int log_size, const int *phy, int phy_size, 
        const char *assert_type, const char *exp_filename, unsigned int exp_linenum, 
        unsigned int fatal1, unsigned int fatal2);
    void (*md_exception2)(const int *log, int log_size, const int *phy, int phy_size, 
        const char *detail);
};

void aee_trigger_kdb(void);
struct aee_oops *aee_oops_create(AE_DEFECT_ATTR attr, const char *module);
void aee_oops_set_backtrace(struct aee_oops *oops, const char *backtrace);
void aee_oops_set_process_path(struct aee_oops *oops, const char *process_path);
void aee_oops_free(struct aee_oops *oops);

void aee_kernel_exception(const char *module, const char *msg, ...);
void aee_kernel_warning(const char *module, const char *msg, ...);
void aee_kernel_reminding(const char *module, const char *msg, ...);
void aed_md_exception1(const int *log, int log_size, const int *phy, int phy_size, const char *assert_type, const char *exp_filename, unsigned int exp_linenum, unsigned int fatal1, unsigned int fatal2);

#if defined(CONFIG_MTK_AEE_IPANIC)
/* Begin starting panic record */
void ipanic_oops_start(const char *str, int err, struct task_struct *task);
void ipanic_oops_end(void);
/* Record stack trace info into current paniclog */
void ipanic_stack_store(unsigned long where, unsigned long from);
#else
#define ipanic_oops_start(str, err, task)
#define ipanic_oops_end()
#define ipanic_stack_store(where, from)
#endif

#endif // __AEE_H__
