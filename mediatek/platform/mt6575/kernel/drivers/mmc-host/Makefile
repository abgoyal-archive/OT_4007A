obj-$(CONFIG_MTK_MMC) += dbg.o
obj-$(CONFIG_MTK_MMC) += sd.o mt6575_sd_misc.o

#ifeq ($(CONFIG_MTK_AEE_KDUMP),y)
obj-$(CONFIG_MTK_MMC) += sd_dump.o
#endif

#ifeq ($(MTK_EMMC_SUPPORT),yes)
obj-$(CONFIG_MTK_MMC) += part_info_emmc.o
#endif
