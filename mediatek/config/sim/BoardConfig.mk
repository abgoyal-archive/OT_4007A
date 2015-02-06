TARGET_COMPRESS_MODULE_SYMBOLS := false
TARGET_PRELINK_MODULE := false

# Don't try to build a bootloader.
TARGET_NO_BOOTLOADER := true

# Don't bother with a kernel
TARGET_NO_KERNEL := true

# The simulator does not support native code at all
TARGET_CPU_ABI := none

#the simulator partially emulates the original HTC /dev/eac audio interface
HAVE_HTC_AUDIO_DRIVER := true
BOARD_USES_GENERIC_AUDIO := true
