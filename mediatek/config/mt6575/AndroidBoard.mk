LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

TARGET_PROVIDES_INIT_RC := true

# add your file here (as target) and place the file in this directory
files := $(TARGET_OUT_KEYLAYOUT)/mt6575-kpd.kl \
         $(TARGET_ROOT_OUT)/init.rc \
         $(TARGET_OUT_ETC)/vold.fstab \
         $(TARGET_OUT_ETC)/vold.fstab.nand \
         $(TARGET_OUT_ETC)/vold.fstab.fat.nand \
         $(TARGET_OUT_DATA)/http-proxy-cfg \
         $(TARGET_OUT_ETC)/player.cfg \
         $(TARGET_OUT_ETC)/mtk_omx_core.cfg \
         $(TARGET_ROOT_OUT_SBIN)/advanced_meta_init \
         $(TARGET_ROOT_OUT_SBIN)/meta_init \
         $(TARGET_ROOT_OUT)/advanced_meta_init.rc \
         $(TARGET_ROOT_OUT)/meta_init.rc

_init_project_rc := $(MTK_ROOT_CONFIG_OUT)/init.project.rc
ifneq ($(wildcard $(_init_project_rc)),)
files += $(TARGET_ROOT_OUT)/init.project.rc
endif

_meta_init_project_rc := $(MTK_ROOT_CONFIG_OUT)/meta_init.project.rc
ifneq ($(wildcard $(_meta_init_project_rc)),)
files += $(TARGET_ROOT_OUT)/meta_init.project.rc
endif

_advanced_meta_init_project_rc := $(MTK_ROOT_CONFIG_OUT)/advanced_meta_init.project.rc
ifneq ($(wildcard $(_advanced_meta_init_project_rc)),)
files += $(TARGET_ROOT_OUT)/advanced_meta_init.project.rc
endif

files += $(strip \
             $(foreach file,$(call wildcard2, $(LOCAL_PATH)/*.xml), \
                  $(addprefix $(PRODUCT_OUT)/system/etc/permissions/,$(notdir $(file))) \
              ) \
          )


ifeq ($(HAVE_AEE_FEATURE),yes)
ifeq ($(PARTIAL_BUILD),true)
files += $(TARGET_ROOT_OUT)/init.aee.customer.rc
else
files += $(TARGET_ROOT_OUT)/init.aee.mtk.rc
endif
endif

ifeq ($(strip $(HAVE_SRSAUDIOEFFECT_FEATURE)),yes)
files += $(PRODUCT_OUT)/system/data/srs_processing.cfg
endif 

# 1. for all files, 
# 2. make them as targets,
# 3. make their filename as prerequisite.
$(foreach file,$(files), $(eval $(call copy-one-file, \
    $(LOCAL_PATH)/$(lastword $(subst /, ,$(file))),$(file))))
ALL_PREBUILT += $(files)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := mt6575-kpd.kcm
LOCAL_MODULE_TAGS := user
include $(BUILD_KEY_CHAR_MAP)

$(call config-custom-folder,modem:modem)
##### INSTALL MODEM FIRMWARE #####
##removed by shan.liao.hz@jrdcom.com for FR-337459
#include $(CLEAR_VARS)
#LOCAL_MODULE := modem.img
#LOCAL_MODULE_TAGS := user
#LOCAL_MODULE_CLASS := ETC
#LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
#LOCAL_SRC_FILES := modem/$(LOCAL_MODULE)
#include $(BUILD_PREBUILT)

##################################

## INSTALL catcher_filter.bin ##

ifeq ($(MTK_MDLOGGER_SUPPORT),yes)
include $(CLEAR_VARS)
LOCAL_MODULE := catcher_filter.bin
LOCAL_MODULE_TAGS := user
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := modem/catcher_filter.bin
include $(BUILD_PREBUILT)
endif

#################################

##### INSTALL DSP FIRMWARE #####

include $(CLEAR_VARS)
LOCAL_MODULE := DSP_ROM
LOCAL_MODULE_TAGS := user
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/firmware
LOCAL_SRC_FILES := modem/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

$(eval $(call copy-one-file,$(LOCAL_PATH)/modem/DSP_BL,$(PRODUCT_OUT)/DSP_BL))
ALL_PREBUILT += $(PRODUCT_OUT)/DSP_BL
##################################

