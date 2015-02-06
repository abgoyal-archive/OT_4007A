#ifndef _CAMERA_CUSTOM_IF_
#define _CAMERA_CUSTOM_IF_
//
#include "camera_custom_types.h"
//
namespace NSCamCustom
{
//
//
enum EDevId
{
    eDevId_ImgSensor0, 
    eDevId_ImgSensor1, 
};

/*******************************************************************************
* Sensor Input Data Bit Order
*   Return:
*       0   : raw data input [9:2]
*       1   : raw data input [7:0]
*       -1  : error
*******************************************************************************/
MINT32  getSensorInputDataBitOrder(EDevId const eDevId);

/*******************************************************************************
* Sensor Pixel Clock Inverse in PAD side.
*   Return:
*       0   : no inverse
*       1   : inverse
*       -1  : error
*******************************************************************************/
MINT32  getSensorPadPclkInv(EDevId const eDevId);

/*******************************************************************************
* LCE
*******************************************************************************/
namespace NSLce
{
    typedef struct TuningParam_GainThreshold_S
    {
        //  lce gain threshold [0~63]; [0, 64)=[0/16, 64/16)
        //  0 <= Thr1 <= Thr2 <= Thr3 <= 63
        MUINT8  uThr1;
        MUINT8  uThr2;
        MUINT8  uThr3;
    } TuningParam_GainThreshold_T;

    typedef struct TuningParam_Quality_S
    {
        MINT32  SW_LEVEL;       // LCE Level           :[0~10] Smaller number means weaker LCE effect.
        MINT32  SW_MINBA;       // min black-keeping   :[0~64] Higher LCE_SW_MINBA value may be needed if you want to sacrifice LCE strength to trade off better lowlight noise performance.
        MINT32  SW_BA_MODE;     // ba mode             :[0] old method [1] new method
        MINT32  SW_lowlight_thr;// lowlight percetage threshold.
        MINT32  SW_ba_gamma[2]; // gamma for ba        :[1~] the first two gamma levels used to determine ba.
        MINT32  SW_ba[7];       // ba strength         :[0~128] much larger, much more black-keeping
        MINT32  SW_o2[11];      // o2 strength         :[400~3000] smaller value means stronger LCE effect.
    } TuningParam_Quality_T;

    typedef struct InfoParam_S
    {
        MINT32	i4ISOSpeed;
        MINT32	i4EVSelect;
    } InfoParam_T;

    TuningParam_GainThreshold_T const&  getParam_Gain_Threshold(InfoParam_T const& rInfo);
    TuningParam_Quality_T const&        getParam_Quality(InfoParam_T const& rInfo);

};  //NSLce

/*******************************************************************************
* Image Sensor Orientation
*******************************************************************************/
typedef struct SensorOrientation_S
{
    MUINT32 u4Degree_0;     //  main sensor in degree (0, 90, 180, 270)
    MUINT32 u4Degree_1;     //  sub  sensor in degree (0, 90, 180, 270)
} SensorOrientation_T;

SensorOrientation_T const&  getSensorOrientation();

/*******************************************************************************
* Auto flicker detection
*******************************************************************************/
typedef struct FlickerThresholdSetting_S
{
    MUINT32 u4FlickerPoss1;         // impossible flicker
    MUINT32 u4FlickerPoss2;         // maybe flicker exist
    MUINT32 u4FlickerFreq1;         // flicker frequency detect 
    MUINT32 u4FlickerFreq2;         // flicker frequency detect
    MUINT32 u4ConfidenceLevel1;   // flicker confidence level
    MUINT32 u4ConfidenceLevel2;   // flicker confidence level
    MUINT32 u4ConfidenceLevel3;   // flicker confidence level
}FlickerThresholdSetting_T;

FlickerThresholdSetting_T const&  getFlickerThresPara();

/*******************************************************************************
* MDP
*******************************************************************************/
typedef struct TuningParam_CRZ_S
{
    MUINT8  uUpScaleCoeff;  //  [5 bits; 1~19] Up sample coeff. choose > 12 may get undesirable result, '8' is recommended.
    MUINT8  uDnScaleCoeff;  //  [5 bits; 1~19] Down sample coeff. '15' is recommended.
} TuningParam_CRZ_T;

typedef struct TuningParam_PRZ_S
{
    MUINT8  uUpScaleCoeff;  //  [5 bits; 1~19] Up sample coeff. choose > 12 may get undesirable result, '8' is recommended.
    MUINT8  uDnScaleCoeff;  //  [5 bits; 1~19] Down sample coeff. '15' is recommended.
    MUINT8  uEEHCoeff;      //  [4 bits] The strength for horizontal edge.
    MUINT8  uEEVCoeff;      //  [4 bits] The strength for vertial edge.
} TuningParam_PRZ_T;

TuningParam_CRZ_T const&  getParam_CRZ_Video();
TuningParam_CRZ_T const&  getParam_CRZ_Preview();
TuningParam_CRZ_T const&  getParam_CRZ_Capture();
TuningParam_PRZ_T const&  getParam_PRZ_QuickView();

//
/*******************************************************************************
* Dynamic Frame Rate
******************************************************************************/
typedef struct DynamicFrameRate_S
{
    MUINT32 EVThresNormal;
    MUINT32 EVThresNight;
    MBOOL   isEnableDFps;
} DynamicFrameRate_T;

DynamicFrameRate_T const& getParamDynamicFrameRate();


//
/*******************************************************************************
* Custom EXIF
******************************************************************************/
#define SET_EXIF_TAG_STRING(tag,str) \
    if (strlen((const char*)str) <= 32) { \
        strcpy((char *)pexifApp1Info->tag, (const char*)str); }
        
typedef struct customExifInfo_s {
    unsigned char strMake[32];
    unsigned char strModel[32];
    unsigned char strSoftware[32];
} customExifInfo_t;

MINT32 custom_SetExif(void **ppCustomExifTag);
MUINT32 custom_GetFlashlightGain10X(void);  //cotta : added for high current solution

/*******************************************************************************
* Get the LCM Physical Orientation, the LCM physical orientation 
* will be defined in ProjectConfig.mk 
*******************************************************************************/
MUINT32 getLCMPhysicalOrientation();
/*******************************************************************************
* ATV
*******************************************************************************/
MINT32 get_atv_input_data();

/*******************************************************************************
* FD Threshold
*******************************************************************************/
MINT8 get_fdvt_threshold();

/*******************************************************************************
* ATV disp delay time
*******************************************************************************/

#define ATV_MODE_NTSC 30000
#define ATV_MODE_PAL  25000

#ifdef MTK_MT5192
//unit: us
#define ATV_MODE_NTSC_DELAY 5000
#define ATV_MODE_PAL_DELAY  10000

#else 
#ifdef MTK_MT5193
//unit: us
#define ATV_MODE_NTSC_DELAY 18000
#define ATV_MODE_PAL_DELAY  26000
#else
//unit: us
#define ATV_MODE_NTSC_DELAY 0
#define ATV_MODE_PAL_DELAY  0
#endif

#endif

MINT32 get_atv_disp_delay(MINT32 mode);

/*******************************************************************************
* ASD Threshold
*******************************************************************************/

typedef struct ASDThreshold_S
{
	MINT16 s2IdxWeightBlAe;
     MINT16 s2IdxWeightBlScd;    
	MINT16 s2IdxWeightLsAe;        
  	MINT16 s2IdxWeightLsAwb;
  	MINT16 s2IdxWeightLsAf;    
     MINT16 s2IdxWeightLsScd;
     MUINT8 u1TimeWeightType;
     MUINT8 u1TimeWeightRange;
     MINT16 s2EvLoThrNight;
     MINT16 s2EvHiThrNight;
     MINT16 s2EvLoThrOutdoor;
     MINT16 s2EvHiThrOutdoor;
     MUINT8 u1ScoreThrNight;
     MUINT8 u1ScoreThrBacklit;
     MUINT8 u1ScoreThrPortrait;
     MUINT8 u1ScoreThrLandscape;
     MBOOL boolBacklitLockEnable;
     MINT16 s2BacklitLockEvDiff;  
}ASDThreshold_T;

ASDThreshold_T const&  get_ASD_threshold();

/*******************************************************************************
* 
*******************************************************************************/
};  //NSCamCustom
#endif  //  _CAMERA_CUSTOM_IF_

