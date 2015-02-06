
#ifndef AUDIO_CUSTOM_EXP_H
#define AUDIO_CUSTOM_EXP_H

namespace android {


#ifndef DEVICE_MAX_VOLUME
#define DEVICE_MAX_VOLUME      (12)
#endif

#ifndef DEVICE_MIN_VOLUME
#define DEVICE_MIN_VOLUME      (-32)
#endif

#ifndef DEVICE_VOICE_MAX_VOLUME
#define DEVICE_VOICE_MAX_VOLUME      (12)
#endif

#ifndef DEVICE_VOICE_MIN_VOLUME
#define DEVICE_VOICE_MIN_VOLUME      (-32)
#endif

#define ENABLE_AUDIO_TRACK_LOUDNESS       //Define this will enable loudness in audio track
                                          //This allows loudness only apply on Ringtone
                                          //We suggest if it is open, do not configure ACF with DRC.
                                          //Or the Ringtone will pass 2 DRC and will be very loud.

#define ENABLE_AUDIO_COMPENSATION_FILTER  //Define this will enable audio compensation filter 
                                          //For 32MP, the library only supports ACF with DRC
                                          //For 48MP and later, it can be "pure ACF" or "ACF with DRC"
                                          //Please see ACF Document for detail
#define ENABLE_HEADPHONE_COMPENSATION_FILTER  //Define this will enable audio compensation filter
                                          //For 32MP, the library only supports ACF with DRC
                                          //For 48MP and later, it can be "pure ACF" or "ACF with DRC"
                                          //Please see ACF Document for detail
                                          
//#define FORCE_CAMERA_SHUTTER_SOUND_AUDIBLE   //Define this will enable camera shutter sound audible 
                                               //even if in silent mode                                          

//#define ENABLE_STEREO_SPEAKER
                                          // if define Stereo speaker , speaker output will not do stero to mono, keep in stereo format
                                          //because stereo output can apply on more than 1 speaker.

//#define ALL_USING_VOICEBUFFER_INCALL     //Define this will enable Voice  to VoiceBuffer when using speaker and headphone in incall mode.

// when defein AUDIO_HQA_SUPPORT audioflinger will use first active stream samplerate as hardware setting.
// generally is is only use for verifying hardware 
//#define AUDIO_HQA_SUPPORT


#ifdef ENABLE_AUDIO_COMPENSATION_FILTER
    #define AUDIO_COMPENSATION_FLT_MODE (5)
#endif

#ifdef ENABLE_HEADPHONE_COMPENSATION_FILTER
    #define HEADPHONE_COMPENSATION_FLT_MODE (4)
#endif

#define ENABLE_AUDIO_SW_STEREO_TO_MONO    //Define this will enable SW stereo to mono on LCH & RCH
                                          //If not define this, HW stereo to mono (only LCH) will be applied

#define USE_REFMIC_IN_LOUDSPK (0)         //(1)->Use Ref Mic as main mic; (0)->Use original main mic.

#define ENABLE_HIGH_SAMPLERATE_RECORD

}

#endif
