#
# Makefile for miscellaneous I2C chip drivers.
#
# Do not add new drivers to this directory! It is DEPRECATED.
#
# Device drivers are better grouped according to the functionality they
# implement rather than to the bus they are connected to. In particular:
# * Hardware monitoring chip drivers go to drivers/hwmon
# * RTC chip drivers go to drivers/rtc
# * I/O expander drivers go to drivers/gpio
#

include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-$(CONFIG_AMP_TFA9895)       += tfa9895.o
obj-$(CONFIG_AMP_TFA9895L)      += tfa9895l.o
obj-$(CONFIG_AMP_RT5506)	+= rt5506.o
obj-$(CONFIG_BOSCH_BMA253)      += bma253_bosch.o
obj-$(CONFIG_KIONIX_KXTJ2)      += kxtj2.o
obj-$(CONFIG_INPUT_CAPELLA_CM36686)     += cm36686.o
obj-$(CONFIG_SENSORS_AK09911)   += akm09911.o
obj-$(CONFIG_INPUT_CWSTM32)	+= CwMcuSensor.o
ifeq ($(CONFIG_I2C_DEBUG_CHIP),y)
EXTRA_CFLAGS += -DDEBUG
endif
#NFC ++
obj-$(CONFIG_SENSORS_NFC_PN544) += pn544.o
obj-$(CONFIG_SENSORS_NFC_PN544) += pn544_htc.o

#NFC --
