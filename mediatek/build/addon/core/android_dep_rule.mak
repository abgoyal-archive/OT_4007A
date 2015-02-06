ifneq ($(CUSTOM_MODEM),)
MTK_MODEM_PATH = mediatek/custom/common/modem/$(CUSTOM_MODEM)
MTK_MODEM_FILE = $(if $(wildcard $(MTK_MODEM_PATH)/modem_E2.mak),$(MTK_MODEM_PATH)/modem_E2.mak,$(MTK_MODEM_PATH)/modem.mak)
$(shell cat $(MTK_MODEM_FILE) | sed -e '/^[ \t]*#/'d | awk -F# '{print $1}' | sed -n '/^\S\+[ \t]*=[ \t]*\S\+/'p | \
      sed -e 's/^/MODEM_/' > $(MTK_MODEM_PATH)/modem_feature.mak)
include $(MTK_MODEM_PATH)/modem_feature.mak
endif

#######################################################
# dependency check between AP side & modem side


ifeq (yes,$(strip $(MTK_TTY_SUPPORT)))
   ifeq (FALSE, $(strip $(MODEM_CTM_SUPPORT)))
    $(call dep-err-seta-or-setb,MTK_TTY_SUPPORT,no,CTM_SUPPORT,true)
   endif
endif

ifeq (yes,$(strip $(MTK_VT3G324M_SUPPORT)))
   ifeq (FALSE,$(MODEM_SP_VIDEO_CALL_SUPPORT))
    $(call dep-err-ona-or-offb,SP_VIDEO_CALL_SUPPORT,MTK_VT3G324M_SUPPORT)
   endif
endif

ifeq (yes,$(strip $(MTK_VT3G324M_SUPPORT)))
  ifneq (modem_3g,$(MTK_MODEM_SUPPORT))
     $(call dep-err-seta-or-offb,MTK_MODEM_SUPPORT,modem_3g,MTK_VT3G324M_SUPPORT)
  endif
endif

ifeq (modem_3g,$(MTK_MODEM_SUPPORT))
  ifeq ($(findstring hspa,$(CUSTOM_MODEM)),)
    $(call dep-err-seta-or-setb,CUSTOM_MODEM,xxx_hspa_xxx,MTK_MODEM_SUPPORT,none modem_3g)
  endif
endif

# for dual mic 
ifeq (yes, $(strip $(MTK_DUAL_MIC_SUPPORT)))
  ifneq (TRUE,$(strip $(MODEM_DUAL_MIC_SUPPORT)))
    $(call dep-err-ona-or-offb, DUAL_MIC_SUPPORT, MTK_DUAL_MIC_SUPPORT)
  endif
endif

################################################################
# for LCA feature

ifeq (yes, $(strip $(MTK_LCA_SUPPORT)))
  ifneq (no,$(strip $(MTK_SNS_SUPPORT)))
    $(call dep-err-offa-or-offb,MTK_SNS_SUPPORT,MTK_LCA_SUPPORT)
  endif
endif

ifeq (yes, $(strip $(MTK_LCA_SUPPORT)))
  ifneq (no,$(strip $(MTK_SMS_NATIONAL_LANGUAGE_SUPPORT)))
    $(call dep-err-offa-or-offb,MTK_SMS_NATIONAL_LANGUAGE_SUPPORT,MTK_LCA_SUPPORT)
  endif
endif

ifeq (yes, $(strip $(MTK_LCA_SUPPORT)))
  ifneq (no,$(strip $(MTK_CAMERA_APP_3DHW_SUPPORT)))
    $(call dep-err-offa-or-offb,MTK_CAMERA_APP_3DHW_SUPPORT,MTK_LCA_SUPPORT)
  endif
endif

ifeq (yes, $(strip $(MTK_LCA_SUPPORT)))
  ifneq (no,$(strip $(MTK_LIVEWALLPAPER_APP)))
    $(call dep-err-offa-or-offb,MTK_LIVEWALLPAPER_APP,MTK_LCA_SUPPORT)
  endif
endif

############################################################
# for wapi feature

ifeq (yes,$(strip $(MTK_WAPI_SUPPORT)))
  ifneq (yes, $(strip $(MTK_WLAN_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_WLAN_SUPPORT, MTK_WAPI_SUPPORT)
  endif
endif

############################################################
# for wifi_hotspot feature

ifeq (yes,$(strip $(MTK_WIFI_HOTSPOT_SUPPORT)))
  ifneq (yes, $(strip $(MTK_WLAN_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_WLAN_SUPPORT, MTK_WIFI_HOTSPOT_SUPPORT)
  endif
endif

##############################################################
# for camera feature

ifeq (no,$(strip $(MTK_CAMERA_APP)))
  ifeq (yes,$(strip $(MTK_CAMERA_VIDEO_ZOOM)))
     $(call dep-err-ona-or-offb, MTK_CAMERA_APP, MTK_CAMERA_VIDEO_ZOOM)
  endif
endif

ifeq (yes,$(strip $(MTK_CAMERA_APP)))
  ifeq (,$(strip $(CUSTOM_HAL_CAMERA)))
     $(call dep-err-seta-or-offb, CUSTOM_HAL_CAMERA,camera, MTK_CAMERA_APP)
  endif
endif

##############################################################
# for matv feature

ifeq (yes,$(strip $(HAVE_MATV_FEATURE)))
  ifeq (,$(strip $(CUSTOM_HAL_MATV)))
     $(call dep-err-common, PLEASE turn off HAVE_MATV_FEATURE or set CUSTOM_HAL_MATV)
  endif
endif

ifeq (yes,$(strip $(HAVE_MATV_FEATURE)))
  ifeq (,$(strip $(CUSTOM_KERNEL_MATV)))
     $(call dep-err-common, PLEASE turn off HAVE_MATV_FEATURE or set CUSTOM_KERNEL_MATV)
  endif
endif


##############################################################
# for fm feature

ifeq (yes,$(strip $(MTK_MT519X_FM_SUPPORT)))
  ifeq (no,$(strip $(HAVE_MATV_FEATURE)))
     $(call dep-err-ona-or-offb, HAVE_MATV_FEATURE, MTK_MT519X_FM_SUPPORT)
  endif
endif

##############################################################
# for wcdma feature
ifeq (yes,$(strip $(MTK_WCDMA_SUPPORT)))
   ifeq (2, $(words,$(subst gprs, ,$(CUSTOM_MODEM))))
    $(call dep-err-seta-or-setb,MTK_WCDMA_SUPPORT,no,CUSTOM_MODEM,$(subst gprs,hspa,$(CUSTOM_MODEM)))
   endif
endif

##############################################################
# for gps feature

ifeq (yes,$(strip $(MTK_AGPS_APP)))
  ifeq (no,$(strip $(MTK_GPS_SUPPORT)))
     $(call dep-err-ona-or-offb, MTK_GPS_SUPPORT, MTK_AGPS_APP)
  endif
endif

##############################################################
# for BT feature

ifeq (yes,$(strip $(MTK_WLANBT_SINGLEANT)))
  ifneq (yes,$(strip $(MTK_BT_SUPPORT)))
     $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_WLANBT_SINGLEANT)
  endif
endif

ifeq (yes,$(strip $(MTK_WLANBT_SINGLEANT)))
  ifneq (yes,$(strip $(MTK_WLAN_SUPPORT)))
     $(call dep-err-ona-or-offb, MTK_WLAN_SUPPORT, MTK_WLANBT_SINGLEANT)
  endif
endif

##############################################################
# for GEMINI feature
ifeq (yes, $(strip $(MTK_GEMINI_ENHANCEMENT)))
  ifneq (yes, $(strip $(GEMINI)))
    $(call dep-err-ona-or-offb, GEMINI, MTK_GEMINI_ENHANCEMENT)
  endif
endif

##############################################################
# for DSPIRDBG feature
ifeq (yes, $(strip $(MTK_DSPIRDBG)))
  ifneq (yes, $(strip $(MTK_MDLOGGER_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_MDLOGGER_SUPPORT, MTK_DSPIRDBG)
  endif
endif

##############################################################
# for ExLog2Mail feature

ifeq (no, $(strip $(HAVE_AEE_FEATURE)))
  ifeq (yes, $(strip $(MTK_EXLOG2MAIL_APP)))
    $(call dep-err-ona-or-offb, HAVE_AEE_FEATURE, MTK_EXLOG2MAIL_APP)
  endif
endif

##############################################################
# for Log2Server feature

ifeq (no, $(strip $(HAVE_AEE_FEATURE)))
  ifeq (yes, $(strip $(MTK_LOG2SERVER_APP)))
    $(call dep-err-ona-or-offb, HAVE_AEE_FEATURE, MTK_LOG2SERVER_APP)
  endif
endif

##############################################################
# for MTK_FOTA_SUPPORT feature

ifeq (no, $(strip $(MTK_DM_APP)))
  ifeq (yes, $(strip $(MTK_FOTA_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_DM_APP, MTK_FOTA_SUPPORT)
  endif
endif

ifeq (no, $(strip $(MTK_SMSREG_APP)))
  ifeq (yes, $(strip $(MTK_FOTA_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_SMSREG_APP, MTK_FOTA_SUPPORT)
  endif
endif

ifeq (no, $(strip $(MTK_FOTA_RELEASE)))
  ifeq (yes,$(strip $(MTK_FOTA_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_FOTA_RELEASE, MTK_FOTA_SUPPORT)
  endif
endif


##############################################################
# for IME

ifeq (yes,$(MTK_INTERNAL))
  ifeq (no,$(MTK_INTERNAL_LANG_SET))
     $(call dep-err-ona-or-offb, MTK_INTERNAL_LANG_SET, MTK_INTERNAL)
  endif
endif

ifeq (yes,$(MTK_INTERNAL_LANG_SET))
  ifeq (no,$(MTK_INTERNAL))
     $(call dep-err-ona-or-offb, MTK_INTERNAL, MTK_INTERNAL_LANG_SET)
  endif
endif


ifneq (yes, $(strip $(MTK_IME_SUPPORT)))
  ifeq (yes, $(strip $(MTK_IME_FRENCH_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_IME_SUPPORT, MTK_IME_FRENCH_SUPPORT)
  endif
  
  ifeq (yes, $(strip $(MTK_IME_RUSSIAN_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_IME_SUPPORT, MTK_IME_RUSSIAN_SUPPORT)
  endif

  ifeq (yes, $(strip $(MTK_IME_GERMAN_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_IME_SUPPORT, MTK_IME_GERMAN_SUPPORT)
  endif

  ifeq (yes, $(strip $(MTK_IME_SPANISH_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_IME_SUPPORT, MTK_IME_SPANISH_SUPPORT)
  endif

  ifeq (yes, $(strip $(MTK_IME_ITALIAN_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_IME_SUPPORT, MTK_IME_ITALIAN_SUPPORT)
  endif

  ifeq (yes, $(strip $(MTK_IME_PORTUGUESE_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_IME_SUPPORT, MTK_IME_PORTUGUESE_SUPPORT)
  endif

  ifeq (yes, $(strip $(MTK_IME_TURKISH_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_IME_SUPPORT, MTK_IME_TURKISH_SUPPORT)
  endif

  ifeq (yes, $(strip $(MTK_IME_MALAY_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_IME_SUPPORT, MTK_IME_MALAY_SUPPORT)
  endif

  ifeq (yes, $(strip $(MTK_IME_HINDI_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_IME_SUPPORT, MTK_IME_HINDI_SUPPORT)
  endif

  ifeq (yes, $(strip $(MTK_IME_ARABIC_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_IME_SUPPORT, MTK_IME_ARABIC_SUPPORT)
  endif

  ifeq (yes, $(strip $(MTK_IME_THAI_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_IME_SUPPORT, MTK_IME_THAI_SUPPORT)
  endif

  ifeq (yes, $(strip $(MTK_IME_VIETNAM_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_IME_SUPPORT, MTK_IME_VIETNAM_SUPPORT)
  endif

  ifeq (yes, $(strip $(MTK_IME_INDONESIAN_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_IME_SUPPORT, MTK_IME_INDONESIAN_SUPPORT)
  endif
endif

##############################################################
# for MtkWeatherProvider

ifeq (yes,$(MTK_WEATHER_WIDGET_APP))
  ifeq (no,$(MTK_WEATHER_PROVIDER_APP))
    $(call dep-err-ona-or-offb, MTK_WEATHER_PROVIDER_APP, MTK_WEATHER_WIDGET_APP)
  endif
endif

##############################################################
# for FD feature

ifeq (yes,$(MTK_FD_SUPPORT))
  ifneq (modem_3g,$(MTK_MODEM_SUPPORT))
     $(call dep-err-seta-or-setb, MTK_FD_SUPPORT,no,MTK_MODEM_SUPPORT,modem_3g)
  endif
endif

ifeq (no,$(strip $(MTK_FD_SUPPORT)))
  ifeq (yes,$(strip $(MTK_FD_FORCE_REL_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_FD_SUPPORT, MTK_FD_FORCE_REL_SUPPORT)
  endif
endif

##############################################################
# for SNS feature

ifeq (yes,$(MTK_SNS_SINAWEIBO_APP))
  ifeq (no,$(MTK_SNS_SUPPORT))
     $(call dep-err-ona-or-offb, MTK_SNS_SUPPORT,MTK_SNS_SINAWEIBO_APP)
  endif
endif

##############################################################
# for VT voice answer feature

ifeq (OP01_SPEC0200_SEGC,$(OPTR_SPEC_SEG_DEF))
  ifneq (yes,$(MTK_PHONE_VT_VOICE_ANSWER))
     $(call dep-err-seta-or-onb, OPTR_SPEC_SEG_DEF,non OP01_SPEC0200_SEGC,MTK_PHONE_VT_VOICE_ANSWER)
  endif
endif

ifeq (yes,$(MTK_PHONE_VT_VOICE_ANSWER))
  ifneq (OP01_SPEC0200_SEGC,$(OPTR_SPEC_SEG_DEF))
     $(call dep-err-seta-or-offb, OPTR_SPEC_SEG_DEF,OP01_SPEC0200_SEGC,MTK_PHONE_VT_VOICE_ANSWER)
  endif
endif

##############################################################
# for VT multimedia ringtone

ifeq (OP01_SPEC0200_SEGC,$(OPTR_SPEC_SEG_DEF))
  ifneq (yes,$(MTK_PHONE_VT_MM_RINGTONE))
     $(call dep-err-seta-or-onb, OPTR_SPEC_SEG_DEF,non OP01_SPEC0200_SEGC,MTK_PHONE_VT_MM_RINGTONE)
  endif
endif

ifeq (yes,$(MTK_PHONE_VT_MM_RINGTONE))
  ifneq (OP01_SPEC0200_SEGC,$(OPTR_SPEC_SEG_DEF))
     $(call dep-err-seta-or-offb, OPTR_SPEC_SEG_DEF,OP01_SPEC0200_SEGC,MTK_PHONE_VT_MM_RINGTONE)
  endif
endif

ifeq (yes,$(HAVE_CMMB_FEATURE))
  ifneq (OP01_SPEC0200_SEGC,$(OPTR_SPEC_SEG_DEF))
     $(call dep-err-seta-or-offb, OPTR_SPEC_SEG_DEF,OP01_SPEC0200_SEGC,HAVE_CMMB_FEATURE)
  endif
endif

##############################################################
# for BT 

ifeq (no,$(strip $(MTK_BT_SUPPORT)))
  ifeq (yes,$(strip $(MTK_BT_21_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_21_SUPPORT)
  endif
  ifeq (yes,$(strip $(MTK_BT_30_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_30_SUPPORT)
  endif
  ifeq (yes,$(strip $(MTK_BT_30_HS_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_30_HS_SUPPORT)
  endif
  ifeq (yes,$(strip $(MTK_BT_40_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_40_SUPPORT)
  endif
  ifeq (yes,$(strip $(MTK_BT_FM_OVER_BT_VIA_CONTROLLER)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_FM_OVER_BT_VIA_CONTROLLER)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_A2DP)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_A2DP)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_AVRCP)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_AVRCP)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_AVRCP14)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_AVRCP14)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_BIP)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_BIP)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_BPP)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_BPP)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_DUN)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_DUN)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_FTP)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_FTP)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_HFP)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_HFP)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_HIDH)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_HIDH)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_MANAGER)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_MANAGER)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_MAPC)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_MAPC)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_MAPS)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_MAPS)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_OPP)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_OPP)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_PAN)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_PAN)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_PBAP)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_PBAP)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_PRXM)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_PRXM)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_PRXR)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_PRXR)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_SIMAP)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_SIMAP)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_SPP)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_SPP)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_TIMEC)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_TIMEC)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_TIMES)))
    $(call dep-err-ona-or-offb, MTK_BT_SUPPORT, MTK_BT_PROFILE_TIMES)
  endif
endif

ifeq (no,$(strip $(MTK_BT_30_HS_SUPPORT)))
  ifeq (yes,$(strip $(MTK_BT_PROFILE_AVRCP14)))
    $(call dep-err-ona-or-offb, MTK_BT_30_HS_SUPPORT, MTK_BT_PROFILE_AVRCP14)
  endif
endif

ifeq (no,$(strip $(MTK_BT_40_SUPPORT)))
  ifeq (yes,$(strip $(MTK_BT_PROFILE_PRXM)))
    $(call dep-err-ona-or-offb, MTK_BT_40_SUPPORT, MTK_BT_PROFILE_PRXM)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_PRXR)))
    $(call dep-err-ona-or-offb, MTK_BT_40_SUPPORT, MTK_BT_PROFILE_PRXR)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_TIMEC)))
    $(call dep-err-ona-or-offb, MTK_BT_40_SUPPORT, MTK_BT_PROFILE_TIMEC)
  endif
  ifeq (yes,$(strip $(MTK_BT_PROFILE_TIMES)))
    $(call dep-err-ona-or-offb, MTK_BT_40_SUPPORT, MTK_BT_PROFILE_TIMES)
  endif
endif

##############################################################
# for emmc feature
ifeq (no,$(strip $(MTK_EMMC_SUPPORT)))
  ifneq (,$(strip $(EMMC_CHIP)))
    $(call dep-err-common, PLEASE set EMMC_CHIP as NULL when MTK_EMMC_SUPPORT=no)
  endif
endif

##############################################################
# for NFC feature
ifeq (yes,$(strip $(MTK_NFC_SUPPORT)))
  ifeq (no,$(strip $(CONFIG_NFC_PN544)))
    $(call dep-err-ona-or-offb, CONFIG_NFC_PN544, MTK_NFC_SUPPORT)
  endif
endif

ifeq (no,$(strip $(MTK_NFC_SUPPORT)))
  ifeq (yes,$(strip $(CONFIG_NFC_PN544)))
    $(call dep-err-ona-or-offb, MTK_NFC_SUPPORT, CONFIG_NFC_PN544)
  endif
endif

##############################################################
# for fm feature
ifeq (no,$(strip $(MTK_FM_SUPPORT)))
  ifeq (yes,$(strip $(MTK_FM_RECORDING_SUPPORT)))
    $(call dep-err-ona-or-offb, MTK_FM_SUPPORT, MTK_FM_RECORDING_SUPPORT)
  endif
endif

##############################################################
# for launcher2 feature
ifeq (2G,$(strip $(CUSTOM_DRAM_SIZE)))
  ifneq (no,$(strip $(MTK_LAUNCHER_ALLAPPSGRID)))
     $(call dep-err-seta-or-offb, CUSTOM_DRAM_SIZE,non $(CUSTOM_DRAM_SIZE), MTK_LAUNCHER_ALLAPPSGRID)
  endif
endif

##############################################################
# for Brazil customization
ifeq (no,$(strip $(MTK_BRAZIL_CUSTOMIZATION)))
  ifeq (yes,$(strip $(MTK_BRAZIL_CUSTOMIZATION_TIM)))
    $(call dep-err-ona-or-offb, MTK_BRAZIL_CUSTOMIZATION, MTK_BRAZIL_CUSTOMIZATION_TIM)
  endif
endif

##############################################################
# for mtk brazil customization feature

ifeq (no,$(strip $(MTK_BRAZIL_CUSTOMIZATION)))
  ifeq (yes,$(strip $(MTK_BRAZIL_CUSTOMIZATION_VIVO)))
    $(call dep-err-common, please set MTK_BRAZIL_CUSTOMIZATION_VIVO=no when MTK_BRAZIL_CUSTOMIZATION=no)
  endif
  ifeq (yes,$(strip $(MTK_BRAZIL_CUSTOMIZATION_CLARO)))
    $(call dep-err-common, please set MTK_BRAZIL_CUSTOMIZATION_CLARO=no when MTK_BRAZIL_CUSTOMIZATION=no)
  endif
endif

##############################################################
# for 6575 display quality enhancement
ifneq (MT6575,$(strip $(MTK_PLATFORM)))
  ifdef MTK_75DISPLAY_ENHANCEMENT_SUPPORT
    $(call dep-err-common, MTK_75DISPLAY_ENHANCEMENT_SUPPORT could only be defined when MTK_PLATFORM = MT6575)
  endif
endif

##############################################################
# for APKInstaller
ifeq (OP02_SPEC0200_SEGA,$(strip $(OPTR_SPEC_SEG_DEF)))
  ifeq (no,$(strip $(MTK_APKINSTALLER_APP)))
    $(call dep-err-seta-or-onb,OPTR_SPEC_SEG_DEF,non OP02_SPEC0200_SEGA,MTK_APKINSTALLER_APP)
  endif
endif

##############################################################
# for dram size 
ifeq (2G,$(strip $(CUSTOM_DRAM_SIZE)))
  ifneq (no, $(strip $(MTK_HDMI_SUPPORT)))
    $(call dep-err-offa-or-offb,MTK_HDMI_SUPPORT,CUSTOM_DRAM_SIZE=2G)
  endif
endif

ifeq (2G,$(strip $(CUSTOM_DRAM_SIZE)))
  ifneq (no,$(strip $(MTK_VLW_APP)))
    $(call dep-err-offa-or-offb,MTK_VLW_APP,CUSTOM_DRAM_SIZE=2G)
  endif
endif 

ifeq (2G,$(strip $(CUSTOM_DRAM_SIZE)))
  ifneq (no,$(strip $(MTK_SNS_SUPPORT)))
    $(call dep-err-offa-or-offb,MTK_SNS_SUPPORT,CUSTOM_DRAM_SIZE=2G)
  endif
endif

ifeq (2G,$(strip $(CUSTOM_DRAM_SIZE)))
  ifneq (no,$(strip $(MTK_SNS_SINAWEIBO_APP)))
    $(call dep-err-offa-or-offb,MTK_SNS_SINAWEIBO_APP,CUSTOM_DRAM_SIZE=2G)
  endif
endif

ifeq (2G,$(strip $(CUSTOM_DRAM_SIZE)))
  ifneq (no,$(strip $(MTK_SMS_NATIONAL_LANGUAGE_SUPPORT)))
    $(call dep-err-offa-or-offb,MTK_SMS_NATIONAL_LANGUAGE_SUPPORT,CUSTOM_DRAM_SIZE=2G)
  endif
endif

ifeq (2G,$(strip $(CUSTOM_DRAM_SIZE)))
  ifneq (no,$(strip $(MTK_SNS_KAIXIN_APP)))
    $(call dep-err-offa-or-offb,MTK_SNS_KAIXIN_APP,CUSTOM_DRAM_SIZE=2G)
  endif
endif

ifeq (2G,$(strip $(CUSTOM_DRAM_SIZE)))
  ifneq (no,$(strip $(MTK_SNS_RENREN_APP)))
    $(call dep-err-offa-or-offb,MTK_SNS_RENREN_APP,CUSTOM_DRAM_SIZE=2G)
  endif
endif
ifeq (2G,$(strip $(CUSTOM_DRAM_SIZE)))
  ifneq (no,$(strip $(MTK_SNS_FACEBOOK_APP)))
    $(call dep-err-offa-or-offb,MTK_SNS_FACEBOOK_APP,CUSTOM_DRAM_SIZE=2G)
  endif
endif

ifeq (2G,$(strip $(CUSTOM_DRAM_SIZE)))
  ifneq (no,$(strip $(MTK_SNS_FLICKR_APP)))
    $(call dep-err-offa-or-offb,MTK_SNS_FLICKR_APP,CUSTOM_DRAM_SIZE=2G)
  endif
endif

ifeq (2G,$(strip $(CUSTOM_DRAM_SIZE)))
  ifneq (no,$(strip $(MTK_SNS_TWITTER_APP)))
    $(call dep-err-offa-or-offb,MTK_SNS_TWITTER_APP,CUSTOM_DRAM_SIZE=2G)
  endif
endif


#ifeq (2G,$(strip $(CUSTOM_DRAM_SIZE)))
#  ifneq (no,$(strip $(MTK_CAMERA_APP_3DHW_SUPPORT)))
#    $(call dep-err-offa-or-offb,MTK_CAMERA_APP_3DHW_SUPPORT,CUSTOM_DRAM_SIZE)
#  endif
#endif

#ifeq (2G,$(strip $(CUSTOM_DRAM_SIZE)))
#  ifneq (no,$(strip $(MTK_LIVEWALLPAPER_APP)))
#    $(call dep-err-offa-or-offb,MTK_LIVEWALLPAPER_APP,MTK_LCA_SUPPORT)
#  endif
#endif

ifeq (yes,$(strip $(MTK_FAT_ON_NAND)))
  ifneq (yes,$(strip $(MTK_2SDCARD_SWAP)))
    $(call dep-err-ona-or-offb,MTK_2SDCARD_SWAP,MTK_FAT_ON_NAND)
  endif
  ifneq (yes,$(strip $(MTK_MULTI_STORAGE_SUPPORT)))
    $(call dep-err-ona-or-offb,MTK_MULTI_STORAGE_SUPPORT,MTK_FAT_ON_NAND)
  endif
endif
#############################################################
# for MTK_RELEASE_PACKAGE
OPTR_LIST := $(subst _, ,$(OPTR_SPEC_SEG_DEF))
OPTR := $(word 1,$(OPTR_LIST))
ifeq (OP01,$(OPTR))
  ifeq ($(filter rel_customer_operator_cmcc,$(MTK_RELEASE_PACKAGE)),)
    $(call dep-err-common, Please add rel_customer_operator_cmcc to MTK_RELEASE_PACKAGE because OPTR_SPEC_SEG_DEF is $(OPTR_SPEC_SEG_DEF))
  endif
endif
ifeq (OP02,$(OPTR))
  ifeq ($(filter rel_customer_operator_cu,$(MTK_RELEASE_PACKAGE)),)
    $(call dep-err-common, Please add rel_customer_operator_cu to MTK_RELEASE_PACKAGE because OPTR_SPEC_SEG_DEF is $(OPTR_SPEC_SEG_DEF))
  endif
endif
ifeq (OP03,$(OPTR))
  ifeq ($(filter rel_customer_operator_orange,$(MTK_RELEASE_PACKAGE)),)
    $(call dep-err-common, Please add rel_customer_operator_orange to MTK_RELEASE_PACKAGE because OPTR_SPEC_SEG_DEF is $(OPTR_SPEC_SEG_DEF))
  endif
endif

