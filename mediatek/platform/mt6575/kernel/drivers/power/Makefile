obj-$(CONFIG_MTK_PMIC)          += pmic_mt6329.o pmic_mt6329_bank1.o upmu_common.o
    ifeq ($(MTK_FAN5405_SUPPORT),yes)
        obj-$(CONFIG_MTK_SMART_BATTERY) += mt6575_battery_fan5405.o mt6575_fuel_gauge.o fan5405_jrd.o
        #obj-$(CONFIG_MTK_SMART_BATTERY) += mt6575_battery_fan5405.o mt6575_fuel_gauge.o fan5405.o
    else
        #obj-$(CONFIG_MTK_SMART_BATTERY) += mt6575_battery_fan5405.o mt6575_fuel_gauge.o fan5405.o
        obj-$(CONFIG_MTK_SMART_BATTERY) += mt6575_battery.o mt6575_fuel_gauge.o
        #obj-$(CONFIG_MTK_SMART_BATTERY) += mt6575_battery.o mt6575_fuel_gauge.o fan5405_jrd.o
    endif

