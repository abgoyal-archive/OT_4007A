include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(SOC).a

COBJS := mt6575.o \
	mt6575_board.o \
	mt65xx_leds.o \
	boot_mode.o \
	load_image.o \
	addr_trans.o \
	mmc_core.o \
	mmc_test.o \
	msdc_utils.o \
	msdc.o \
	mtk_serial.o \
	mtk_timer.o \
	mt6575_nand.o \
	mt6575_meta.o \
	mt6575_key.o \
	mt6575_rtc.o \
	mt6575_recovery.o \
	mt6575_factory.o \
	mtk_auxadc.o\
	mtk_wdt.o\
	mt6575_gpio.o \
	mt6575_gpio_init.o \
	mt6575_pwm.o \
	mt6575_i2c.o \
	mt6575_pmic6329.o \
    mt6575_disp_drv.o \
 	mt6575_disp_assert_layer.o \
    mt6575_disp_drv_dbi.o \
   	mt6575_lcd_drv.o \
  mt6575_disp_drv_dsi.o \
 	mt6575_dsi_drv.o \
        mt6575_disp_drv_dpi.o \
 	mt6575_dpi_drv.o \
    mt6575_logo.o \
    mt6575_pll.o \
  bmt.o \
  partition_mt.o \
  mt6575_sleep.o \
  irq.o
  
ifeq ($(MTK_FAN5405_SUPPORT),yes)
	COBJS += mt6575_bat_fan5405.o mt6575_i2c0.o fan5405.o
else
    COBJS += mt6575_bat.o
endif

###	custom/$(MTK_PROJECT)/mt65xx_partition.o \
			
SOBJS	:= lowlevel_init.o

SRCS	:= $(SOBJS:.o=.S) $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))
SOBJS	:= $(addprefix $(obj),$(SOBJS))

$(LIB):	$(obj).depend $(OBJS) $(SOBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS) $(SOBJS)

clean:
	rm -f $(SOBJS) $(OBJS)

distclean:	clean
	rm -f $(LIB) core *.bak $(obj).depend

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
