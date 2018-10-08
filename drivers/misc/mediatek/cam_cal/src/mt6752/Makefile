#
# Makefile for misc devices that really don't fit anywhere else.

include $(srctree)/drivers/misc/mediatek/Makefile.custom

custom_cam_cal := $(addsuffix /, $(shell echo $(CONFIG_CUSTOM_KERNEL_CAM_CAL_DRV)))        	
ifeq ($(CONFIG_ARCH_MTK_PROJECT), "a50ml")
	obj-y +=s5k3l2xxotp/
else
	obj-y += $(custom_cam_cal)
endif


