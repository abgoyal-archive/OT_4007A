#ifndef _CFG_TUNING_MT6575_H_
#define _CFG_TUNING_MT6575_H_

#include "flicker_tuning_mt6575.h"
#include <cutils/properties.h>

namespace NSLce
{
    /***************************************************************************
    * LCE Gain Threshold
    ***************************************************************************/
    TuningParam_GainThreshold_T const&
    getParam_Gain_Threshold(InfoParam_T const& rInfo)
    {
        static TuningParam_GainThreshold_T const inst = {
            //  lce gain threshold [0~63]; [0, 64)=[0/16, 64/16)
            //  0 <= Thr1 <= Thr2 <= Thr3 <= 63
            uThr1   : 24, 
            uThr2   : 28, 
            uThr3   : 32, 
        };
        return  inst;
    }

    /***************************************************************************
    * LCE Quality
    ***************************************************************************/
    TuningParam_Quality_T const&
    getParam_Quality(InfoParam_T const& rInfo)
    {
        static TuningParam_Quality_T inst = {
            // LCE Level           :[0~10] Smaller number means weaker LCE effect.
            SW_LEVEL        : 4, 
            // min black-keeping   :[0~64] Higher LCE_SW_MINBA value may be needed if you want to sacrifice LCE strength to trade off better lowlight noise performance.
            SW_MINBA        : 80,
            // ba mode             :[0] old method [1] new method
            SW_BA_MODE      : 1, 
            // lowlight percetage threshold.
            SW_lowlight_thr : 0, 
            // gamma for ba        :[1~] the first two gamma levels used to determine ba.
            SW_ba_gamma : {26, 63}, 
            // ba strength         :[0~128] much larger, much more black-keeping
            SW_ba       : {8, 16, 24, 32, 64, 96, 128}, 
            // o2 strength         :[400~3000] smaller value means stronger LCE effect.
            SW_o2       : {2700, 2400, 2100, 1800, 1600, 1400, 1200, 1000, 800, 600, 400}, 
        };

#if 0
        if  ( M < rInfo.i4ISOSpeed || N < rInfo.i4EVSelect )
        {
            inst.SW_LEVEL = X;
        }
#endif
			// FixMe: Use _backup because inst.SW_LEVEL/inst.SW_MINBA won't be initialized again after 1st entering this function.
			static MINT32 SW_LEVEL_backup = 0;
			static MINT32 SW_MINBA_backup = 0;
			static MINT32 S_bFirstTime = 1;	// First time execution?
			if (S_bFirstTime)	// If first time, back up value.
			{
				SW_LEVEL_backup = inst.SW_LEVEL;
				SW_MINBA_backup = inst.SW_MINBA;
				S_bFirstTime = 0;
			}
			else	// if not first time, restore value.
			{
				inst.SW_LEVEL = SW_LEVEL_backup;
				inst.SW_MINBA = SW_MINBA_backup;
			}
			// FixMe: End.

			switch(rInfo.i4EVSelect)
			{
			case 0: // 0 EV
				inst.SW_LEVEL  = inst.SW_LEVEL;
				inst.SW_MINBA  = inst.SW_MINBA;
				break;
			case 1: // - 1/3 EV
				inst.SW_LEVEL  = inst.SW_LEVEL+1;
				if(inst.SW_LEVEL>4)
					{
						inst.SW_LEVEL = 4;
					}
				inst.SW_MINBA  = inst.SW_MINBA*8/10;
				break;
			case 2: // - 2/3 EV
				inst.SW_LEVEL  = inst.SW_LEVEL+2;
				inst.SW_MINBA  = inst.SW_MINBA*63/100;
				if(inst.SW_LEVEL>4)
					{
						inst.SW_LEVEL = 4;
					}
				break;
			case 3: // - 1 EV
				inst.SW_LEVEL  = inst.SW_LEVEL+3;
				inst.SW_MINBA  = inst.SW_MINBA/2;
				if(inst.SW_LEVEL>5)
					{
						inst.SW_LEVEL = 5;
					}
				break;
			default:
				inst.SW_LEVEL  = inst.SW_LEVEL;
				inst.SW_MINBA  = inst.SW_MINBA;
				break;
			}

        return inst;
    }

};  //NSLce

SensorOrientation_T const&
getSensorOrientation()
{
    static SensorOrientation_T const inst = {
        u4Degree_0  : 90,//270,   //  main sensor in degree (0, 90, 180, 270)
        u4Degree_1  : 0,    //  sub  sensor in degree (0, 90, 180, 270)
    };
    return inst;
}


TuningParam_CRZ_T const&
getParam_CRZ_Video()
{
    static TuningParam_CRZ_T inst = {
        uUpScaleCoeff   : 8,    //  [5 bits; 1~19] Up sample coeff. choose > 12 may get undesirable result, '8' is recommended.
        uDnScaleCoeff   : 15,   //  [5 bits; 1~19] Down sample coeff. '1':blur,'19':sharpest; '15' is recommended.
    };
    char value[32] = {'\0'};
    property_get("Tuning.Vio.up", value, "8");
    inst.uUpScaleCoeff = atoi(value); 
    property_get("Tuning.Vio.dw", value, "15");
    inst.uDnScaleCoeff = atoi(value);   
    return inst;
}

TuningParam_CRZ_T const&
getParam_CRZ_Preview()
{
    static TuningParam_CRZ_T inst = {
        uUpScaleCoeff   : 8,    //  [5 bits; 1~19] Up sample coeff. choose > 12 may get undesirable result, '8' is recommended.
        uDnScaleCoeff   : 15,   //  [5 bits; 1~19] Down sample coeff. '1':blur,'19':sharpest; '15' is recommended.
    };
    char value[32] = {'\0'};
    property_get("Tuning.Prv.up", value, "8");
    inst.uUpScaleCoeff = atoi(value); 
    property_get("Tuning.Prv.dw", value, "15");
    inst.uDnScaleCoeff = atoi(value); 
    return inst;
}

TuningParam_CRZ_T const&
getParam_CRZ_Capture()
{
    static TuningParam_CRZ_T inst = {
        uUpScaleCoeff   : 8,    //  [5 bits; 1~19] Up sample coeff. choose > 12 may get undesirable result, '8' is recommended.
        uDnScaleCoeff   : 15,   //  [5 bits; 1~19] Down sample coeff. '1':blur,'19':sharpest; '15' is recommended.
    };
    char value[32] = {'\0'}; 
    property_get("Tuning.Cap.up", value, "8");
    inst.uUpScaleCoeff = atoi(value); 
    property_get("Tuning.Cap.dw", value, "15");
    inst.uDnScaleCoeff = atoi(value);    
    return inst;
}

TuningParam_PRZ_T const&
getParam_PRZ_QuickView()
{
    static TuningParam_PRZ_T inst = {
        uUpScaleCoeff   : 8,    //  [5 bits; 1~19] Up sample coeff. choose > 12 may get undesirable result, '8' is recommended.
        uDnScaleCoeff   : 15,   //  [5 bits; 1~19] Down sample coeff. '1':blur,'19':sharpest; '15' is recommended.
        uEEHCoeff       : 0,    //  [4 bits] The strength for horizontal edge.
        uEEVCoeff       : 0,    //  [4 bits] The strength for vertial edge.
    };
    char value[32] = {'\0'}; 
    property_get("Tuning.Quv.up", value, "8");
    inst.uUpScaleCoeff = atoi(value); 
    property_get("Tuning.Quv.dw", value, "15");
    inst.uDnScaleCoeff = atoi(value);  
    property_get("Tuning.Quv.eeh", value, "0");
    inst.uEEHCoeff = atoi(value); 
    property_get("Tuning.Quv.eev", value, "0");
    inst.uEEVCoeff = atoi(value);  
    return inst;
}

MINT32 get_atv_input_data()
{
	MINT32 AtvInputdata = 1;
	return AtvInputdata;
}


MINT8 get_fdvt_threshold()  
{
	MINT8 FeatureThreshold = 7;    
	return FeatureThreshold;
}

MINT32 get_atv_disp_delay(MINT32 mode)
{
    return ((ATV_MODE_NTSC == mode)?ATV_MODE_NTSC_DELAY:((ATV_MODE_PAL == mode)?ATV_MODE_PAL_DELAY:0));
};


ASDThreshold_T const&
get_ASD_threshold()
{
	static ASDThreshold_T const inst = {
	     s2IdxWeightBlAe : 1,           /*0~10 */
	     s2IdxWeightBlScd : 8,        /*0~10 */
	     s2IdxWeightLsAe: 1,           /*0~10 */
	    	s2IdxWeightLsAwb: 1,       /*0~10 */
	    	s2IdxWeightLsAf: 1,            /*0~10 */
	    	s2IdxWeightLsScd: 2,        /*0~10 */ 
	    	u1TimeWeightType: 1,       /*0~1 */ 
	    	u1TimeWeightRange: 10,  /*1~10 */ 
	    	s2EvLoThrNight: 50,           /* 50 -> Lv=5 */
	    	s2EvHiThrNight: 80,            /* 70 -> Lv=7 */
	    	s2EvLoThrOutdoor: 90,     /*110 -> Lv=11 */
	    	s2EvHiThrOutdoor: 130,    /*140 -> Lv=14 */
	    	u1ScoreThrNight: 60,          /* 40~100 */
	    	u1ScoreThrBacklit: 50,       /* 40~100 */
	    	u1ScoreThrPortrait: 50,      /* 40~100 */
	    	u1ScoreThrLandscape: 55,   /* 40~100 */
	    	boolBacklitLockEnable: true,
	    	s2BacklitLockEvDiff: -10,   /* -50~0 */
     };	
	return inst;
}

#endif //  _CFG_TUNING_MT6575_H_

