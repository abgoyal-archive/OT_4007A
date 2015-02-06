#ifndef __MT65XX_MON_H__
#define __MT65XX_MON_H__

struct mt65xx_mon_log
{
    __u32 cpu_cyc;
    __u32 cpu_cnt0;
    __u32 cpu_cnt1;
    __u32 cpu_cnt2;
    __u32 cpu_cnt3;
    __u32 cpu_cnt4;
    __u32 cpu_cnt5;
    __u32 l2c_cnt0;
    __u32 l2c_cnt1;

    __u32 BM_BCNT;
    __u32 BM_TACT;
    __u32 BM_TSCT;
    __u32 BM_WACT;
    __u32 BM_WSCT;
    __u32 BM_BACT;
    __u32 BM_BSCT;
    __u32 BM_TSCT2;
    __u32 BM_WSCT2;
    __u32 BM_TSCT3;
    __u32 BM_WSCT3;
    __u32 BM_WSCT4;

    __u32 BM_TPCT1;
    
    __u32 DRAMC_PageHit;
    __u32 DRAMC_PageMiss;
    __u32 DRAMC_Interbank;
    __u32 DRAMC_Idle;   
};


typedef enum 
{
    MODE_SCHED_SWITCH,
    MODE_PERIODIC,
    MODE_MANUAL    
} MonitorMode;

extern void set_mt65xx_mon_period(long time_ns);
extern long get_mt65xx_mon_period(void);
extern void set_mt65xx_mon_manual_start(unsigned int bStart);
extern unsigned int get_mt65xx_mon_manual_start(void);
extern void set_mt65xx_mon_mode(MonitorMode mode);
extern MonitorMode get_mt65xx_mon_mode();

extern int mt65xx_mon_init(void);
extern int mt65xx_mon_deinit(void);
extern int mt65xx_mon_enable(void);
extern int mt65xx_mon_disable(void);
extern unsigned int mt65xx_mon_log(void);
extern int mt65xx_mon_print_log(unsigned int log, struct trace_iterator *iter);

#endif  /* !__MT65XX_MON_H__ */
