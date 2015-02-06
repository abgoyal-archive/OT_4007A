#include "dumchar.h"
#include "part_info.h"
struct excel_info PartInfo[PART_NUM]={
			{"preloader",262144,0x0},
			{"dsp_bl",786432,0x40000},
			{"nvram",3145728,0x100000},
			{"seccfg",131072,0x400000},
			{"uboot",393216,0x420000},
			{"bootimg",5242880,0x480000},
			{"recovery",5242880,0x980000},
			{"sec_ro",1179648,0xe80000},
			{"misc",393216,0xfa0000},
			{"logo",3145728,0x1000000},
			{"expdb",655360,0x1300000},
			{"android",309329920,0x13a0000},
			{"cache",62914560,0x13aa0000},
			{"usrdata",0,0x176a0000},
			{"bmtpool",51200,0xFFFF0050},
 };

EXPORT_SYMBOL(PartInfo);
