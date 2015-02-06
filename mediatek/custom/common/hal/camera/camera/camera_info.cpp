#if defined(MT6516)
// #include "drv_features.h"
//#if (defined(ISP_SUPPORT)&&defined(DRV_ISP_6516_SERIES))
//#include "camera_para.h"
//#include "med_api.h"
//#include "sensor_common.h"

#define	NSLibcameracustom_IMPLEMENT		//	2010.09.16	Jonas
#include "../inc/MediaTypes.h"
#include "MTKAECmd.h"
#include "../src/lib/inc/CamFeatureCommon.h"

#include "../src/lib/libisp/isp_feature_comm.h"
#include "kd_imgsensor_define.h"


//#include "camera_custom_cs.h"

#include "camera_custom_sensor.h"

namespace NSLibcameracustom
{


CamCustomSupport
stCustomSupport_MAIN_RAW =
{
//..............................................................................
//	Custom-specific Feature Options
//..............................................................................
//	Scene Mode
CCS_TBL_NODE_BEGIN(SceneMode)
	SCENE_MODE_OFF, SCENE_MODE_ACTION, SCENE_MODE_PORTRAIT, SCENE_MODE_LANDSCAPE, SCENE_MODE_NIGHTSCENE, SCENE_MODE_NIGHTPORTRAIT, 
	SCENE_MODE_THEATRE, SCENE_MODE_BEACH, SCENE_MODE_SNOW, SCENE_MODE_SUNSET, SCENE_MODE_STEADYPHOTO, SCENE_MODE_FIREWORKS, 
	SCENE_MODE_SPORTS, SCENE_MODE_PARTY, SCENE_MODE_CANDLELIGHT, SCENE_MODE_ISO_ANTI_SHAKE, SCENE_MODE_BRACKET_AE, 
	CCS_TBL_NODE_END()
//	Color Effect
CCS_TBL_NODE_BEGIN(ColorEffect)
	MEFFECT_OFF, MEFFECT_MONO, MEFFECT_SEPIA, MEFFECT_NEGATIVE, MEFFECT_SOLARIZE, MEFFECT_AQUA, MEFFFECT_BLACKBOARD, MEFFECT_POSTERIZE, MEFFECT_WHITEBOARD, 
	CCS_TBL_NODE_END()
//	AE ISO
CCS_TBL_NODE_BEGIN(AEISO)
	AE_ISO_SPEED_AUTO, AE_ISO_SPEED_100, AE_ISO_SPEED_200, AE_ISO_SPEED_400, AE_ISO_SPEED_800, AE_ISO_SPEED_1600, 
	CCS_TBL_NODE_END()
//	Capture Size
CCS_TBL_NODE_BEGIN(CapSize)
	CAP_SIZE_1280_960, CAP_SIZE_1600_1200, CAP_SIZE_2048_1536, CAP_SIZE_2560_1920, 
	CCS_TBL_NODE_END()
//	Preview Size
CCS_TBL_NODE_BEGIN(PreviewSize)
	PREVIEW_SIZE_176_144, PREVIEW_SIZE_320_240, PREVIEW_SIZE_352_288, PREVIEW_SIZE_640_480, 
	CCS_TBL_NODE_END()
//	Frame Rate
CCS_TBL_NODE_BEGIN(FrameRate)
	FRAME_RATE_150FPS, FRAME_RATE_300FPS, 
	CCS_TBL_NODE_END()
//	AE Strobe
CCS_TBL_NODE_BEGIN(AEStrobe)
	AE_STROBE_MODE_AUTO, AE_STROBE_MODE_FORCE_ON, AE_STROBE_MODE_FORCE_OFF, AE_STROBE_MODE_REDEYE, 
	CCS_TBL_NODE_END()
//..............................................................................
//	Default Option of Each Custom-specific Scene Mode.
//..............................................................................
//	Default Scene Mode
CCS_TBL_NODE_BEGIN(SceneModeDefault)
	SCENE_MODE_OFF, SCENE_MODE_ACTION, SCENE_MODE_PORTRAIT, SCENE_MODE_LANDSCAPE, SCENE_MODE_NIGHTSCENE, SCENE_MODE_NIGHTPORTRAIT, 
	SCENE_MODE_THEATRE, SCENE_MODE_BEACH, SCENE_MODE_SNOW, SCENE_MODE_SUNSET, SCENE_MODE_STEADYPHOTO, SCENE_MODE_FIREWORKS, 
	SCENE_MODE_SPORTS, SCENE_MODE_PARTY, SCENE_MODE_CANDLELIGHT, SCENE_MODE_ISO_ANTI_SHAKE, SCENE_MODE_BRACKET_AE, 
	CCS_TBL_NODE_END()
//	Default Color Effect
CCS_TBL_NODE_BEGIN(ColorEffectDefault)
	MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, 
	MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, 
	CCS_TBL_NODE_END()
//	Default AE ISO
CCS_TBL_NODE_BEGIN(AEISODefault)
	AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, 
	AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, 
	AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, 
	CCS_TBL_NODE_END()
//	Default Capture Size
CCS_TBL_NODE_BEGIN(CapSizeDefault)
	CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, 
	CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, 
	CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, 
	CCS_TBL_NODE_END()
//	Default Preview Size
CCS_TBL_NODE_BEGIN(PreviewSizeDefault)
	PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, 
	PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, 
	PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, 
	CCS_TBL_NODE_END()
//	Default Frame Rate
CCS_TBL_NODE_BEGIN(FrameRateDefault)
	FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, 
	FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, 
	FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, 
	CCS_TBL_NODE_END()
//	Default AE Strobe
CCS_TBL_NODE_BEGIN(AEStrobeDefault)
	AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, 
	AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, 
	AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, 
	CCS_TBL_NODE_END()
};

CamCustomSupport
stCustomSupport_SUB_RAW =
{
//..............................................................................
//	Custom-specific Feature Options
//..............................................................................
//	Scene Mode
CCS_TBL_NODE_BEGIN(SceneMode)
	SCENE_MODE_OFF, SCENE_MODE_ACTION, SCENE_MODE_PORTRAIT, SCENE_MODE_LANDSCAPE, SCENE_MODE_NIGHTSCENE, SCENE_MODE_NIGHTPORTRAIT, 
	SCENE_MODE_THEATRE, SCENE_MODE_BEACH, SCENE_MODE_SNOW, SCENE_MODE_SUNSET, SCENE_MODE_STEADYPHOTO, SCENE_MODE_FIREWORKS, 
	SCENE_MODE_SPORTS, SCENE_MODE_PARTY, SCENE_MODE_CANDLELIGHT, SCENE_MODE_ISO_ANTI_SHAKE, SCENE_MODE_BRACKET_AE, 
	CCS_TBL_NODE_END()
//	Color Effect
CCS_TBL_NODE_BEGIN(ColorEffect)
	MEFFECT_OFF, MEFFECT_MONO, MEFFECT_SEPIA, MEFFECT_NEGATIVE, MEFFECT_SOLARIZE, MEFFECT_AQUA, MEFFFECT_BLACKBOARD, MEFFECT_POSTERIZE, MEFFECT_WHITEBOARD, 
	CCS_TBL_NODE_END()
//	AE ISO
CCS_TBL_NODE_BEGIN(AEISO)
	AE_ISO_SPEED_AUTO, AE_ISO_SPEED_100, AE_ISO_SPEED_200, AE_ISO_SPEED_400, AE_ISO_SPEED_800, AE_ISO_SPEED_1600, 
	CCS_TBL_NODE_END()
//	Capture Size
CCS_TBL_NODE_BEGIN(CapSize)
	CAP_SIZE_1280_960, CAP_SIZE_1600_1200, CAP_SIZE_2048_1536, CAP_SIZE_2560_1920, 
	CCS_TBL_NODE_END()
//	Preview Size
CCS_TBL_NODE_BEGIN(PreviewSize)
	PREVIEW_SIZE_176_144, PREVIEW_SIZE_320_240, PREVIEW_SIZE_352_288, PREVIEW_SIZE_640_480, 
	CCS_TBL_NODE_END()
//	Frame Rate
CCS_TBL_NODE_BEGIN(FrameRate)
	FRAME_RATE_150FPS, FRAME_RATE_300FPS, 
	CCS_TBL_NODE_END()
//	AE Strobe
CCS_TBL_NODE_BEGIN(AEStrobe)
	AE_STROBE_MODE_AUTO, AE_STROBE_MODE_FORCE_ON, AE_STROBE_MODE_FORCE_OFF, AE_STROBE_MODE_REDEYE, 
	CCS_TBL_NODE_END()
//..............................................................................
//	Default Option of Each Custom-specific Scene Mode.
//..............................................................................
//	Default Scene Mode
CCS_TBL_NODE_BEGIN(SceneModeDefault)
	SCENE_MODE_OFF, SCENE_MODE_ACTION, SCENE_MODE_PORTRAIT, SCENE_MODE_LANDSCAPE, SCENE_MODE_NIGHTSCENE, SCENE_MODE_NIGHTPORTRAIT, 
	SCENE_MODE_THEATRE, SCENE_MODE_BEACH, SCENE_MODE_SNOW, SCENE_MODE_SUNSET, SCENE_MODE_STEADYPHOTO, SCENE_MODE_FIREWORKS, 
	SCENE_MODE_SPORTS, SCENE_MODE_PARTY, SCENE_MODE_CANDLELIGHT, SCENE_MODE_ISO_ANTI_SHAKE, SCENE_MODE_BRACKET_AE, 
	CCS_TBL_NODE_END()
//	Default Color Effect
CCS_TBL_NODE_BEGIN(ColorEffectDefault)
	MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, 
	MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, MEFFECT_OFF, 
	CCS_TBL_NODE_END()
//	Default AE ISO
CCS_TBL_NODE_BEGIN(AEISODefault)
	AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, 
	AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, 
	AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, AE_ISO_SPEED_AUTO, 
	CCS_TBL_NODE_END()
//	Default Capture Size
CCS_TBL_NODE_BEGIN(CapSizeDefault)
	CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, 
	CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, 
	CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, CAP_SIZE_2560_1920, 
	CCS_TBL_NODE_END()
//	Default Preview Size
CCS_TBL_NODE_BEGIN(PreviewSizeDefault)
	PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, 
	PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, 
	PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, PREVIEW_SIZE_640_480, 
	CCS_TBL_NODE_END()
//	Default Frame Rate
CCS_TBL_NODE_BEGIN(FrameRateDefault)
	FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, 
	FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, 
	FRAME_RATE_300FPS, FRAME_RATE_300FPS, FRAME_RATE_300FPS, 
	CCS_TBL_NODE_END()
//	Default AE Strobe
CCS_TBL_NODE_BEGIN(AEStrobeDefault)
	AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, 
	AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, 
	AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, AE_STROBE_MODE_AUTO, 
	CCS_TBL_NODE_END()
};

CamCustomSupport
stCustomSupport_MAIN_YUV =
{
//..............................................................................
//	Custom-specific Feature Options
//..............................................................................
//	Scene Mode
CCS_TBL_NODE_BEGIN(SceneMode)
	SCENE_MODE_OFF, SCENE_MODE_NIGHTSCENE, 
	CCS_TBL_NODE_END()
//	Color Effect
CCS_TBL_NODE_BEGIN(ColorEffect)
	MEFFECT_OFF, MEFFECT_MONO, MEFFECT_SEPIA, MEFFECT_NEGATIVE, MEFFECT_SEPIAGREEN, MEFFECT_SEPIABLUE, 
	CCS_TBL_NODE_END()
//	AE ISO
CCS_TBL_NODE_BEGIN(AEISO)			//	CAM_FEATURE_NOT_SUPPORT
	CCS_TBL_NODE_END()
//	Capture Size
CCS_TBL_NODE_BEGIN(CapSize)
	CCS_TBL_NODE_END()
//	Preview Size
CCS_TBL_NODE_BEGIN(PreviewSize)
	CCS_TBL_NODE_END()
//	Frame Rate
CCS_TBL_NODE_BEGIN(FrameRate)
	FRAME_RATE_150FPS, FRAME_RATE_300FPS, 
	CCS_TBL_NODE_END()
//	AE Strobe
CCS_TBL_NODE_BEGIN(AEStrobe)
	CCS_TBL_NODE_END()
//..............................................................................
//	Default Option of Each Custom-specific Scene Mode.
//..............................................................................
//	Default Scene Mode
CCS_TBL_NODE_BEGIN(SceneModeDefault)
	SCENE_MODE_OFF, SCENE_MODE_NIGHTSCENE, 
	CCS_TBL_NODE_END()
//	Default Color Effect
CCS_TBL_NODE_BEGIN(ColorEffectDefault)
	MEFFECT_OFF, MEFFECT_OFF, 
	CCS_TBL_NODE_END()
//	Default AE ISO
CCS_TBL_NODE_BEGIN(AEISODefault)	//	CAM_FEATURE_NOT_SUPPORT
	CCS_TBL_NODE_END()
//	Default Capture Size
CCS_TBL_NODE_BEGIN(CapSizeDefault)
	CAP_SIZE_1600_1200, CAP_SIZE_1600_1200, 
	CCS_TBL_NODE_END()
//	Default Preview Size
CCS_TBL_NODE_BEGIN(PreviewSizeDefault)
	PREVIEW_SIZE_720_480, PREVIEW_SIZE_720_480, 
	CCS_TBL_NODE_END()
//	Default Frame Rate
CCS_TBL_NODE_BEGIN(FrameRateDefault)
	FRAME_RATE_300FPS, FRAME_RATE_300FPS, 
	CCS_TBL_NODE_END()
//	Default AE Strobe
CCS_TBL_NODE_BEGIN(AEStrobeDefault)
	CCS_TBL_NODE_END()
};

CamCustomSupport
stCustomSupport_SUB_YUV =
{
//..............................................................................
//	Custom-specific Feature Options
//..............................................................................
//	Scene Mode
CCS_TBL_NODE_BEGIN(SceneMode)
	SCENE_MODE_OFF, SCENE_MODE_NIGHTSCENE, 
	CCS_TBL_NODE_END()
//	Color Effect
CCS_TBL_NODE_BEGIN(ColorEffect)
	MEFFECT_OFF, MEFFECT_MONO, MEFFECT_SEPIA, MEFFECT_NEGATIVE, MEFFECT_SEPIAGREEN, MEFFECT_SEPIABLUE, 
	CCS_TBL_NODE_END()
//	AE ISO
CCS_TBL_NODE_BEGIN(AEISO)			//	CAM_FEATURE_NOT_SUPPORT
	CCS_TBL_NODE_END()
//	Capture Size
CCS_TBL_NODE_BEGIN(CapSize)
	CCS_TBL_NODE_END()
//	Preview Size
CCS_TBL_NODE_BEGIN(PreviewSize)
	CCS_TBL_NODE_END()
//	Frame Rate
CCS_TBL_NODE_BEGIN(FrameRate)
	FRAME_RATE_150FPS, FRAME_RATE_300FPS, 
	CCS_TBL_NODE_END()
//	AE Strobe
CCS_TBL_NODE_BEGIN(AEStrobe)
	CCS_TBL_NODE_END()
//..............................................................................
//	Default Option of Each Custom-specific Scene Mode.
//..............................................................................
//	Default Scene Mode
CCS_TBL_NODE_BEGIN(SceneModeDefault)
	SCENE_MODE_OFF, SCENE_MODE_NIGHTSCENE, 
	CCS_TBL_NODE_END()
//	Default Color Effect
CCS_TBL_NODE_BEGIN(ColorEffectDefault)
	MEFFECT_OFF, MEFFECT_OFF, 
	CCS_TBL_NODE_END()
//	Default AE ISO
CCS_TBL_NODE_BEGIN(AEISODefault)	//	CAM_FEATURE_NOT_SUPPORT
	CCS_TBL_NODE_END()
//	Default Capture Size
CCS_TBL_NODE_BEGIN(CapSizeDefault)
	CAP_SIZE_1600_1200, CAP_SIZE_1600_1200, 
	CCS_TBL_NODE_END()
//	Default Preview Size
CCS_TBL_NODE_BEGIN(PreviewSizeDefault)
	PREVIEW_SIZE_720_480, PREVIEW_SIZE_720_480, 
	CCS_TBL_NODE_END()
//	Default Frame Rate
CCS_TBL_NODE_BEGIN(FrameRateDefault)
	FRAME_RATE_300FPS, FRAME_RATE_300FPS, 
	CCS_TBL_NODE_END()
//	Default AE Strobe
CCS_TBL_NODE_BEGIN(AEStrobeDefault)
	CCS_TBL_NODE_END()
};


CamCustomSupport*const
CamCustomSupportSet[2][2] =
{
	{//	MAIN Sensor
		//	RAW Sensor
		&stCustomSupport_MAIN_RAW, 
		//	YUV Sensor
		&stCustomSupport_MAIN_YUV
	}, 
	{//	SUB Sensor
		//	RAW Sensor
		&stCustomSupport_SUB_RAW, 
		//	YUV Sensor
		&stCustomSupport_SUB_YUV
	}
};


}	//NSLibcameracustom


CamCCMSatCtrl stCamCCMSatCtrl=
{
	1, 700, 1600
};

CRZDownSampleCoeff stCRZDownSampleCoeff=
{
	1, //Preview DN
	1, //Video DN
	8, //Preview UP
	8 //Video UP	
};
	
#if 0
UINT32 u4ReduceCCMEnable;
INT32 i4ReduceCCMSatISOTH;  
INT32 i4ReduceSatForce;	//This i4ReduceSatForce value must bigger than i4ReduceCCMSatISOTH
i4ReduceSatForce Vi4ReduceCCMSatISOTHCCMܤp{ױײvVj, ]NO󤣹M
#endif 

ISPSetIndexInfo ISPIndexInfo[2][HAL_ISP_PARAM_ISO_NUM][SCENE_MODE_NUM]=
{//{Shading, NR1, NR2, Edge, AutoDefect, Saturation, Contrast, CCM, Gamma, Reserved}
    {//Preview sensor mode
        {//ISO100 {NR1, NR2, Edge, AutoDeffect, Saturation, Contrast, Gamma}
            //OFF                                    ACTION                                 PORTRAIT                              LANDSCAPE                           NIGHTSCENE 
            {0, 3, 5, 0, 3, 1, 0},    {0, 3, 5, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},     {0, 3, 5, 0, 3, 1, 0},     {0, 3, 5, 0, 3, 1, 0},
            //NIGHTPORTRAIT                 THEATRE                                BEACH                                 SNOW                                    SUNSET  
            {1, 3, 5, 0, 3, 1, 0},    {0, 3, 5, 0, 3, 1, 0},     {0, 3, 5, 0, 3, 1, 0},     {0, 3, 5, 0, 3, 1, 0},     {0, 3, 5, 0, 3, 1, 0},
            //STEADYPHOTO                    FIREWORKS                             SPORTS                               PARTY                                    CANDLELIGHT  
            {0, 3, 5, 0, 3, 1, 0},    {0, 3, 5, 0, 3, 1, 0},     {0, 3, 5, 0, 3, 1, 0},     {0, 3, 5, 0, 3, 1, 0},     {0, 3, 5, 0, 3, 1, 0},
            //SHAKE                                BRACKET_AE  
            {0, 3, 5, 0, 3, 1, 0},    {0, 3, 5, 0, 3, 1, 0}
    	 },
        {//ISO200 {NR1, NR2, Edge, Saturation, Contrast, Gamma}
            //OFF                                    ACTION                                 PORTRAIT                              LANDSCAPE                           NIGHTSCENE 
            {0, 3, 4, 0, 3, 1, 0},    {0, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},     {0, 3, 4, 0, 3, 1, 0},     {0, 3, 4, 0, 3, 1, 0},
            //NIGHTPORTRAIT                 THEATRE                                BEACH                                     SNOW                                        SUNSET  
            {1, 3, 4, 0, 3, 1, 0},    {0, 3, 4, 0, 3, 1, 0},     {0, 3, 4, 0, 3, 1, 0},     {0, 3, 4, 0, 3, 1, 0},     {0, 3, 4, 0, 3, 1, 0},
            //STEADYPHOTO                    FIREWORKS                             SPORTS                                   PARTY                                        CANDLELIGHT  
            {0, 3, 4, 0, 3, 1, 0},    {0, 3, 4, 0, 3, 1, 0},     {0, 3, 4, 0, 3, 1, 0},     {0, 3, 4, 0, 3, 1, 0},     {0, 3, 4, 0, 3, 1, 0},
            //SHAKE                                BRACKET_AE  
            {0, 3, 4, 0, 3, 1, 0},    {0, 3, 4, 0, 3, 1, 0}
    	 },
        {//ISO400 {NR1, NR2, Edge, Saturation, Contrast, Gamma}
            //OFF                                    ACTION                                 PORTRAIT                              LANDSCAPE                           NIGHTSCENE 
            {1, 3, 4, 0, 3, 1, 0},    {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},
            //NIGHTPORTRAIT                 THEATRE                                BEACH                                     SNOW                                        SUNSET  
            {1, 3, 4, 0, 3, 1, 0},    {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},
            //STEADYPHOTO                    FIREWORKS                             SPORTS                                   PARTY                                        CANDLELIGHT  
            {1, 3, 4, 0, 3, 1, 0},    {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},
            //SHAKE                                BRACKET_AE  
            {1, 3, 4, 0, 3, 1, 0},    {1, 3, 4, 0, 3, 1, 0}
    	 },
        {//ISO800 {NR1, NR2, Edge, Saturation, Contrast, Gamma}
            //OFF                                    ACTION                                 PORTRAIT                              LANDSCAPE                           NIGHTSCENE 
            {4, 5, 3, 0, 3, 1, 0},    {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},
            //NIGHTPORTRAIT                 THEATRE                                    BEACH                                     SNOW                                    SUNSET  
            {4, 5, 3, 0, 3, 1, 0},    {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},
            //STEADYPHOTO                    FIREWORKS                                 SPORTS                                   PARTY                                    CANDLELIGHT  
            {4, 5, 3, 0, 3, 1, 0},    {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},
            //SHAKE                                BRACKET_AE  
            {4, 5, 3, 0, 3, 1, 0},    {4, 5, 3, 0, 3, 1, 0}
    	 },
        {//ISO1600 {NR1, NR2, Edge, Saturation, Contrast, Gamma}
            //OFF                                    ACTION                                 PORTRAIT                              LANDSCAPE                           NIGHTSCENE 
            {4, 5, 3, 0, 3, 1, 0},    {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},
            //NIGHTPORTRAIT                 THEATRE                                    BEACH                                     SNOW                                    SUNSET  
            {4, 5, 3, 0, 3, 1, 0},    {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},
            //STEADYPHOTO                    FIREWORKS                                 SPORTS                                   PARTY                                    CANDLELIGHT  
            {4, 5, 3, 0, 3, 1, 0},    {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},
            //SHAKE                                BRACKET_AE  
            {4, 5, 3, 0, 3, 1, 0},    {4, 5, 3, 0, 3, 1, 0}
    	 }
    },
    {//Capture sensor mode
        {//ISO100 {NR1, NR2, Edge, Saturation, Contrast, Gamma}
            //OFF                                    ACTION                                 PORTRAIT                              LANDSCAPE                           NIGHTSCENE 
            {0, 3, 5, 0, 3, 1, 0},    {0, 3, 5, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},     {0, 3, 5, 0, 3, 1, 0},     {0, 3, 5, 0, 3, 1, 0},
            //NIGHTPORTRAIT                 THEATRE                                    BEACH                                     SNOW                                    SUNSET  
            {1, 3, 5, 0, 3, 1, 0},    {0, 3, 5, 0, 3, 1, 0},     {0, 3, 5, 0, 3, 1, 0},     {0, 3, 5, 0, 3, 1, 0},     {0, 3, 5, 0, 3, 1, 0},
            //STEADYPHOTO                    FIREWORKS                                 SPORTS                                   PARTY                                    CANDLELIGHT  
            {0, 3, 5, 0, 3, 1, 0},    {0, 3, 5, 0, 3, 1, 0},     {0, 3, 5, 0, 3, 1, 0},     {0, 3, 5, 0, 3, 1, 0},     {0, 3, 5, 0, 3, 1, 0},
            //SHAKE                                BRACKET_AE  
            {0, 3, 5, 0, 3, 1, 0},    {0, 3, 5, 0, 3, 1, 0}
    	 },
        {//ISO200 {NR1, NR2, Edge, Saturation, Contrast, Gamma}
            //OFF                                    ACTION                                 PORTRAIT                              LANDSCAPE                           NIGHTSCENE 
            {0, 3, 4, 0, 3, 1, 0},    {0, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},     {0, 3, 4, 0, 3, 1, 0},     {0, 3, 4, 0, 3, 1, 0},
            //NIGHTPORTRAIT                 THEATRE                                    BEACH                                     SNOW                                    SUNSET  
            {1, 3, 4, 0, 3, 1, 0},    {0, 3, 4, 0, 3, 1, 0},     {0, 3, 4, 0, 3, 1, 0},     {0, 3, 4, 0, 3, 1, 0},     {0, 3, 4, 0, 3, 1, 0},
            //STEADYPHOTO                    FIREWORKS                                 SPORTS                                   PARTY                                    CANDLELIGHT  
            {0, 3, 4, 0, 3, 1, 0},    {0, 3, 4, 0, 3, 1, 0},     {0, 3, 4, 0, 3, 1, 0},     {0, 3, 4, 0, 3, 1, 0},     {0, 3, 4, 0, 3, 1, 0},
            //SHAKE                                BRACKET_AE  
            {0, 3, 4, 0, 3, 1, 0},    {0, 3, 4, 0, 3, 1, 0}
    	 },
        {//ISO400 {NR1, NR2, Edge, Saturation, Contrast, Gamma}
            //OFF                                    ACTION                                 PORTRAIT                              LANDSCAPE                           NIGHTSCENE 
            {1, 3, 4, 0, 3, 1, 0},    {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},
            //NIGHTPORTRAIT                 THEATRE                                    BEACH                                     SNOW                                    SUNSET  
            {1, 3, 4, 0, 3, 1, 0},    {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},
            //STEADYPHOTO                    FIREWORKS                                 SPORTS                                   PARTY                                    CANDLELIGHT  
            {1, 3, 4, 0, 3, 1, 0},    {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},     {1, 3, 4, 0, 3, 1, 0},
            //SHAKE                                BRACKET_AE  
            {1, 3, 4, 0, 3, 1, 0},    {1, 3, 4, 0, 3, 1, 0}
    	 },
        {//ISO800 {NR1, NR2, Edge, Saturation, Contrast, Gamma}
            //OFF                                    ACTION                                 PORTRAIT                              LANDSCAPE                           NIGHTSCENE 
            {4, 5, 3, 0, 3, 1, 0},    {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},
            //NIGHTPORTRAIT                 THEATRE                                    BEACH                                     SNOW                                    SUNSET  
            {4, 5, 3, 0, 3, 1, 0},    {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},
            //STEADYPHOTO                    FIREWORKS                                 SPORTS                                   PARTY                                    CANDLELIGHT  
            {4, 5, 3, 0, 3, 1, 0},    {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},
            //SHAKE                                BRACKET_AE  
            {4, 5, 3, 0, 3, 1, 0},    {4, 5, 3, 0, 3, 1, 0}
    	 },
        {//ISO1600 {NR1, NR2, Edge, Saturation, Contrast, Gamma}
            //OFF                                    ACTION                                 PORTRAIT                              LANDSCAPE                           NIGHTSCENE 
            {4, 5, 3, 0, 3, 1, 0},    {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},
            //NIGHTPORTRAIT                 THEATRE                                    BEACH                                     SNOW                                    SUNSET  
            {4, 5, 3, 0, 3, 1, 0},    {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},
            //STEADYPHOTO                    FIREWORKS                                 SPORTS                                   PARTY                                    CANDLELIGHT  
            {4, 5, 3, 0, 3, 1, 0},    {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},     {4, 5, 3, 0, 3, 1, 0},
            //SHAKE                                BRACKET_AE  
            {4, 5, 3, 0, 3, 1, 0},    {4, 5, 3, 0, 3, 1, 0}
    	 }
    }
};


INT32 PassReduceCCMPara(PCamCCMSatCtrl  pstCamCCMSatCtrl)
{
    *pstCamCCMSatCtrl=stCamCCMSatCtrl;
    
    return MFEATURE_NO_ERROR;
}

INT32 GetCRZDownSampleCoeff(PCRZDownSampleCoeff  pstCRZDownSampleCoeff)
{
    *pstCRZDownSampleCoeff=stCRZDownSampleCoeff;
    
    return MFEATURE_NO_ERROR;
}

INT32 GetISPIndex(ISPSetInfo *stISPSetInfo)
{
	UINT32 u4CamMode;
	UINT32 u4ISO;
	UINT32 u4Scene;
	UINT32 u4Zoom;
	UINT32 i4AEEV;


	// Get current condition:
	//		u4ISO:
	//		u4Scene:
	//		u4CamMode: 0(preview)/1(capture)/2(video preview and recording)
	//		u4Zoom: the value is Zoom index: 0(1.000x)/1(1.149x)/2(1.320x)/3(1.516x)/4(1.741x)/5(2.000x)/6(2.297x)/7(2.639x)/8(3.031x)/9(3.482x)/10(4.000x)/11(4.594x)/12(5.278x)/13(6.062x)/14(6.964x)/15(8.000x)
	//		i4AEEV: the value is 10xReal Value (i.e. 70 is actually EV7.0)
	u4CamMode	= stISPSetInfo->stISPSetCamInfo.u4CamMode;
	u4ISO		= stISPSetInfo->stISPSetCamInfo.u4ISO;
	u4Scene		= stISPSetInfo->stISPSetCamInfo.u4Scene;
	u4Zoom		= stISPSetInfo->stISPSetCamInfo.u4Zoom;
	i4AEEV		= stISPSetInfo->stISPSetCamInfo.i4AEEV;
	#if 0	// For Debug info.
	MHAL_LOG("##########GetISPIndex():: ISO: %d, Scene: %d, CamMode: %d, Zoom: %d, AEEV: %d\n", u4ISO, u4Scene, u4CamMode, u4Zoom, i4AEEV);
	#endif	// For Debug info.

	// Get default setting.
	if ((u4CamMode == 0) || (u4CamMode==2))	// PV or Video
	{
		stISPSetInfo->stISPSetIndexInfo = ISPIndexInfo[0][u4ISO][u4Scene];
	}
	else	// u4CamMode == 1	// CAP
	{
		stISPSetInfo->stISPSetIndexInfo = ISPIndexInfo[1][u4ISO][u4Scene];
	}
	#if 0	// For Debug info.
	MHAL_LOG("##########GetISPIndex()[Before]:: NR1: %d, NR2: %d, Edge: %d, ADefect: %d, Contrast: %d, Saturation: %d, Gamma: %d}\n",
		stISPSetInfo->stISPSetIndexInfo.u4NR1Idx,
		stISPSetInfo->stISPSetIndexInfo.u4NR2Idx,
		stISPSetInfo->stISPSetIndexInfo.u4EdgeIdx,
		stISPSetInfo->stISPSetIndexInfo.u4ADefectIdx,
		stISPSetInfo->stISPSetIndexInfo.u4ContrastIdx,
		stISPSetInfo->stISPSetIndexInfo.u4SatIdx,
		stISPSetInfo->stISPSetIndexInfo.u4GammaIdx
	);
	#endif	// For Debug info.


	#if 0	// Modify setting according to condition.
	if (u4CamMode == 2)	// When camera is in Video Preview/Recording Mode.
	{
		// You can set parameters that are different from Preview Mode for Video Preview/Recording Mode.
		stISPSetInfo->stISPSetIndexInfo.u4NR1Idx		= yy;
		stISPSetInfo->stISPSetIndexInfo.u4NR2Idx		= yy;
		stISPSetInfo->stISPSetIndexInfo.u4EdgeIdx		= yy;
		stISPSetInfo->stISPSetIndexInfo.u4ADefectIdx	= yy;
		stISPSetInfo->stISPSetIndexInfo.u4ContrastIdx	= yy;
		stISPSetInfo->stISPSetIndexInfo.u4SatIdx		= yy;
		stISPSetInfo->stISPSetIndexInfo.u4GammaIdx		= yy;
	}

	if (u4Zoom == xxx)	// When camera is using ?x digital zoom.
	{
		// You can set parameters for a specified digital zoom ratio.
		stISPSetInfo->stISPSetIndexInfo.u4NR1Idx		= xx;
		stISPSetInfo->stISPSetIndexInfo.u4NR2Idx		= xx;
		stISPSetInfo->stISPSetIndexInfo.u4EdgeIdx		= xx;
		stISPSetInfo->stISPSetIndexInfo.u4ADefectIdx	= xx;
		stISPSetInfo->stISPSetIndexInfo.u4ContrastIdx	= xx;
		stISPSetInfo->stISPSetIndexInfo.u4SatIdx		= xx;
		stISPSetInfo->stISPSetIndexInfo.u4GammaIdx		= xx;
	}
	#endif	// Modify setting according to condition
	#if 0	// For Debug info.
	MHAL_LOG("##########GetISPIndex()[After ]:: NR1: %d, NR2: %d, Edge: %d, ADefect: %d, Contrast: %d, Saturation: %d, Gamma: %d}\n",
		stISPSetInfo->stISPSetIndexInfo.u4NR1Idx,
		stISPSetInfo->stISPSetIndexInfo.u4NR2Idx,
		stISPSetInfo->stISPSetIndexInfo.u4EdgeIdx,
		stISPSetInfo->stISPSetIndexInfo.u4ADefectIdx,
		stISPSetInfo->stISPSetIndexInfo.u4ContrastIdx,
		stISPSetInfo->stISPSetIndexInfo.u4SatIdx,
		stISPSetInfo->stISPSetIndexInfo.u4GammaIdx
	);
	#endif	// For Debug info.

	return MFEATURE_NO_ERROR;
}


int get_sensor_angle(int current_sensor)
{
//#if 0
    int sensor_angle;
    switch(current_sensor) {
        case DUAL_CAMERA_SUB_SENSOR:
            sensor_angle = ORIENTATION_ANGLE_90;
            break;
        case DUAL_CAMERA_SUB_SECOND_SENSOR:
            sensor_angle = ORIENTATION_ANGLE_90;
            break;
        case DUAL_CAMERA_MAIN_SECOND_SENSOR:
            sensor_angle = ORIENTATION_ANGLE_0;
            break;
        case DUAL_CAMERA_MAIN_SENSOR:
        default:
            sensor_angle = ORIENTATION_ANGLE_0;
            break;
    }

    return sensor_angle;
//#else
//    return 0;
//#endif
}

#endif  //  defined(MT6516)
