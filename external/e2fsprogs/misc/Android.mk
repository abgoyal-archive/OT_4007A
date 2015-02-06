LOCAL_PATH := $(call my-dir)

#########################################################################
# Build mke2fs
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	mke2fs.c \
	prof_err.c \
	util.c \
	default_profile.c

LOCAL_C_INCLUDES := \
	external/e2fsprogs/lib \
	external/e2fsprogs/e2fsck

LOCAL_CFLAGS := -O2 -g -W -Wall \
	-DHAVE_UNISTD_H \
	-DHAVE_ERRNO_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_MOUNT_H \
	-DHAVE_SYS_PRCTL_H \
	-DHAVE_SYS_RESOURCE_H \
	-DHAVE_SYS_SELECT_H \
	-DHAVE_SYS_STAT_H \
	-DHAVE_SYS_TYPES_H \
	-DHAVE_STDLIB_H \
	-DHAVE_STRCASECMP \
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_UTIME_H \
	-DHAVE_GETPAGESIZE \
	-DHAVE_LSEEK64 \
	-DHAVE_LSEEK64_PROTOTYPE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_LINUX_FD_H \
	-DHAVE_TYPE_SSIZE_T \
	-DROOT_SYSCONFDIR="" \
	-DHAVE_GETOPT_H

LOCAL_CFLAGS += -DNO_CHECK_BB

LOCAL_MODULE := mke2fs
LOCAL_MODULE_TAGS := user
LOCAL_FORCE_STATIC_EXECUTABLE := true

#LOCAL_SYSTEM_SHARED_LIBRARIES := \
#	libc

LOCAL_STATIC_LIBRARIES := \
	libext2fs \
	libext2_blkid \
	libext2_uuid \
	libext2_profile \
	libext2_com_err \
	libext2_e2p \
        libc

#include $(BUILD_STATIC_LIBRARY)
include $(BUILD_EXECUTABLE)

###########################################################################
# Build tune2fs
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	tune2fs.c \
	util.c

LOCAL_C_INCLUDES := \
	external/e2fsprogs/lib \
	external/e2fsprogs/e2fsck

LOCAL_CFLAGS := -O2 -g -W -Wall \
	-DHAVE_UNISTD_H \
	-DHAVE_ERRNO_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_MOUNT_H \
	-DHAVE_SYS_PRCTL_H \
	-DHAVE_SYS_RESOURCE_H \
	-DHAVE_SYS_SELECT_H \
	-DHAVE_SYS_STAT_H \
	-DHAVE_SYS_TYPES_H \
	-DHAVE_STDLIB_H \
	-DHAVE_STRCASECMP \
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_UTIME_H \
	-DHAVE_GETPAGESIZE \
	-DHAVE_LSEEK64 \
	-DHAVE_LSEEK64_PROTOTYPE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_LINUX_FD_H \
	-DHAVE_TYPE_SSIZE_T \
	-DHAVE_GETOPT_H

LOCAL_CFLAGS += -DNO_CHECK_BB

LOCAL_MODULE := tune2fs
LOCAL_MODULE_TAGS := user
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_SBIN_UNSTRIPPED)

#LOCAL_SYSTEM_SHARED_LIBRARIES := \
#	libc

LOCAL_STATIC_LIBRARIES := \
	libext2fs \
	libext2_blkid \
	libext2_uuid \
	libext2_profile \
	libext2_com_err \
	libext2_e2p \
        libc

#include $(BUILD_STATIC_LIBRARY)
include $(BUILD_EXECUTABLE)

#########################################################################
# Build badblocks
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	badblocks.c

LOCAL_C_INCLUDES := \
	external/e2fsprogs/lib

LOCAL_CFLAGS := -O2 -g -W -Wall \
	-DHAVE_UNISTD_H \
	-DHAVE_ERRNO_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_MOUNT_H \
	-DHAVE_SYS_PRCTL_H \
	-DHAVE_SYS_RESOURCE_H \
	-DHAVE_SYS_SELECT_H \
	-DHAVE_SYS_STAT_H \
	-DHAVE_SYS_TYPES_H \
	-DHAVE_STDLIB_H \
	-DHAVE_STRCASECMP \
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_UTIME_H \
	-DHAVE_GETPAGESIZE \
	-DHAVE_LSEEK64 \
	-DHAVE_LSEEK64_PROTOTYPE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_LINUX_FD_H \
	-DHAVE_TYPE_SSIZE_T \
	-DHAVE_GETOPT_H

LOCAL_MODULE := badblocks
LOCAL_MODULE_TAGS := systembuilder

#LOCAL_SYSTEM_SHARED_LIBRARIES := \
#	libc

LOCAL_STATIC_LIBRARIES := \
	libext2fs \
	libext2_blkid \
	libext2_e2p \
	libext2_uuid \
	libext2_profile \
	libext2_com_err \
        libc

#include $(BUILD_STATIC_LIBRARY)
include $(BUILD_EXECUTABLE)
