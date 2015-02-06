#
# Makefile for the linux kernel.
#

obj-y := core.o irq.o system.o timer.o mt6575_devs.o mt6575_board.o mt6575_gpt.o clock.o mt6575_dma.o mt6575_eint.o mt6575_gpio.o mt6575_outercache.o mt6575_ca9_power.o mt6575_pwm.o
obj-y += mt_rfkill.o
obj-y += kdump_sdhc_fs.o
obj-y += mt6575_isp.o
obj-y += mt6575_sysram.o 
obj-y += mt6575_eis.o
obj-y += mt6575_fdvt.o
obj-y += mt6575_mdp.o
obj-y += mt6575_mdp_kernel_trap.o
obj-y += mt6575_emi_bwl.o
obj-y += mt6575_res_mgr.o
ifneq ($(CONFIG_MT6575_FPGA),y)
ifneq ($(CONFIG_MT6575_ESL),y)
obj-y += mt6575_emi_mpu.o
endif
endif
obj-y += mt6575_boot.o
obj-y += mt6575_pm_ldo.o mt6575_pmic_feature_api.o
obj-y += mt6575_auxadc.o
obj-y += mt6575_dcm.o mt6575_thermal.o mt6575_clock_manager.o mt6575_pm_log.o mt6575_pm_init.o cpufreq-mt6575.o mt6575_cpe.o
obj-y += mt6575_sleep.o mt6575_sc.o
obj-y += ca9_power.o
obj-y += mt6575_wdt.o
ifneq ($(MTK_EMMC_SUPPORT),yes)
obj-y += sbchk_base.o
endif
obj-$(CONFIG_MT65XX_TRACER) += armV7_perf_mon.o mt6575_emi_bm.o mt6575_mon.o
obj-y += innercache.o
obj-y += ple.o
obj-y += dram_overclock.o
obj-y += ca9_speed.o ca9_speed_indicative.o ca9_speed_test.o ca9_speed_neon.o ca9_neon.o ca9_neon_indicative.o ca9_neon_test.o slt_ca9_helpers.o slt_ca9_helpers_c.o  mt6575_efuse.o 
obj-y += mtk_ccci_helper.o

ccflags-y += -I$(MTK_PATH_PLATFORM)/drivers/power \
             -I$(MTK_PATH_SOURCE)/drivers/wdk

# MTK COMBO chip or not
ifneq ($(CONFIG_MTK_COMBO),)
ifeq ($(CONFIG_MTK_COMBO),n)
obj-y += mt_bt.o
else
obj-y += mt_combo.o
endif
else
obj-y += mt_bt.o
endif

obj-y += mt6575_dormant.o cpu_dormant.o
KBUILD_AFLAGS	:=$(KBUILD_AFLAGS:-msoft-float=-Wa,-mfpu=neon)
