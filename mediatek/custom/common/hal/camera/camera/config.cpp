#include <stdlib.h>
#include <stdio.h>
#include "camera_custom_if.h"

namespace NSCamCustom
{

#if defined(MT6573)
    #include "cfg_tuning_mt6573.h"
#endif

#if defined(MT6575)
    #include "cfg_tuning_mt6575.h"
    #include "sensor_tuning_mt6575.h"
#endif

//#define EN_CUSTOM_EXIF_INFO
MINT32 custom_SetExif(void **ppCustomExifTag)
{
#ifdef EN_CUSTOM_EXIF_INFO
#define CUSTOM_EXIF_STRING_MAKE  "custom make"
#define CUSTOM_EXIF_STRING_MODEL "custom model"
#define CUSTOM_EXIF_STRING_SOFTWARE "custom software"
static customExifInfo_t exifTag = {CUSTOM_EXIF_STRING_MAKE,CUSTOM_EXIF_STRING_MODEL,CUSTOM_EXIF_STRING_SOFTWARE};
    if (0 != ppCustomExifTag) {
        *ppCustomExifTag = (void*)&exifTag;
    }
    return 0;
#else
    return -1;
#endif
}

MUINT32
getLCMPhysicalOrientation()
{
    return ::atoi(MTK_LCM_PHYSICAL_ROTATION); 
}

MUINT32 custom_GetFlashlightGain10X(void)
{
    // x10 , 1 mean 0.1x gain
    //10 means no difference. use torch mode for preflash and cpaflash
    //> 10 means capture flashlight is lighter than preflash light. < 10 is opposite condition.
#define FLASHLIGHT_CALI_LED_GAIN_PRV_TO_CAP_10X 10

    return (MUINT32)FLASHLIGHT_CALI_LED_GAIN_PRV_TO_CAP_10X;
}


MINT32
getSensorInputDataBitOrder(EDevId const eDevId)
{
    switch  (eDevId)
    {
    case eDevId_ImgSensor0:
        return  0;
    case eDevId_ImgSensor1:
        return  0;
    default:
        break;
    }
    return  -1;
}


MINT32
getSensorPadPclkInv(EDevId const eDevId)
{
    switch  (eDevId)
    {
    case eDevId_ImgSensor0:
        return  0;
    case eDevId_ImgSensor1:
        return  0;
    default:
        break;
    }
    return  -1;
}


};  //NSCamCustom

