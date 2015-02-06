LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

dest_dir := $(TARGET_OUT)/etc/bluetooth

files := \
	audio.conf \
	input.conf \
	main.conf \
	blacklist.conf \
	auto_pairing.conf

copy_to := $(addprefix $(dest_dir)/,$(files))

$(copy_to): PRIVATE_MODULE := bluetooth_etcdir
$(copy_to): $(dest_dir)/%: $(LOCAL_PATH)/% | $(ACP)
	$(transform-prebuilt-to-target)

ALL_PREBUILT += $(copy_to)
