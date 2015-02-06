LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

KEYPADLAYOUT_PLF := $(TOPDIR)mediatek/config/${MTK_PROJECT}/plf/isdm_KeypadLayout.plf

KEYPADLAYOUT_KL := $(TOPDIR)out/target/product/${MTK_PROJECT}/custpack/usr/keylayout/isdm_KeypadLayout.kl

$(KEYPADLAYOUT_KL) : $(KEYPADLAYOUT_PLF)
	$(hide) mkdir -p $(JRD_OUT_CUSTPACK)/usr/keylayout
	$(TOPDIR)development/jrdtools/arct/prebuilt/arct \
       k \
	$(KEYPADLAYOUT_PLF) \
	$(KEYPADLAYOUT_KL)

ALL_PREBUILT += $(KEYPADLAYOUT_KL)


