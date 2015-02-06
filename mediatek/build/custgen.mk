include mediatek/build/Makefile
$(call codebase-path)
#modified by sima.chen for PR335367 begin	
JRD_PROJECT_NAME := pixo
include ./device/jrdcom/common/jrd_macro.mk

#generate the jrd_sys_properties.prop & jrd_build_properties.mk
$(shell $(JRD_BUILD_PATH)/common/process_sys_plf.sh \
	$(JRD_TOOLS_ARCT) $(JRD_PROPERTIES_PLF) $(JRD_CUSTOM_RES) 1>/dev/null \
	)
include $(JRD_BUILD_PROPERTIES_OUTPUT)
MTK_DEFINE := $(filter MTK_DRM,$(JRD_PRODUCT_PACKAGES))
PRJ_MF       := $(MTK_ROOT_CONFIG_OUT)/ProjectConfig.mk

all:mtk-config-files mtk-custom-files gen-jrdcust-file 
	@echo "done"

gen-jrdcust-file:
ifeq ($(MTK_DEFINE), )
	$(hide) perl mediatek/build/tools/optgenfile.pl $(PRJ_MF) no MTK_DRM_APP
else
	$(hide) perl mediatek/build/tools/optgenfile.pl $(PRJ_MF) yes MTK_DRM_APP
endif
#modified by sima.chen for PR335367 end	
	
mtk-config-files := $(strip $(call mtk.config.generate-rules,mtk-config-files))
mtk-custom-files := $(strip $(call mtk.custom.generate-rules,mtk-custom-files))
