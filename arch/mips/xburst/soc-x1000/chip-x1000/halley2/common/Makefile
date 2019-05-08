
obj-y += board_base.o misc.o

obj-$(CONFIG_KEYBOARD_GPIO)	+= keyboard_gpio.o

obj-$(CONFIG_SOUND) += sound.o
obj-$(CONFIG_JZ_SPI) += spi_bus.o
obj-$(CONFIG_JZ_SFC) += spi_bus.o
obj-$(CONFIG_SPI_GPIO) += spi_bus.o
obj-$(CONFIG_I2C2_DMA_V12) += i2c_bus.o
obj-$(CONFIG_I2C_GPIO)          += i2c_bus.o
obj-$(CONFIG_I2C0_V12_JZ)       += i2c_bus.o
obj-$(CONFIG_I2C1_V12_JZ)       += i2c_bus.o
obj-$(CONFIG_I2C2_V12_JZ)       += i2c_bus.o
obj-$(CONFIG_I2C3_V12_JZ)       += i2c_bus.o
obj-$(CONFIG_VIDEO_JZ_CIM_HOST_V13) += cim_board.o
obj-$(CONFIG_JZMMC_V12) += mmc.o

ifeq ($(CONFIG_HALLEY2_CORE_V10), y)
## HALLEY2 CORE BOARD use BCM43348 wifi
obj-$(CONFIG_BCMDHD_1_141_66)    += 43438_wlan_power_control.o 43438_wlan_device.o
obj-$(CONFIG_BCM_43438_RFKILL)     += 43438_bt_power_control.o 43438_wlan_power_control.o
endif

ifeq ($(CONFIG_HALLEY2_MINI_CORE_V10), y)
ifeq ($(CONFIG_SSV6XXX), m)
## HALLEY2 MINI CORE use wifi SSV6051P
obj-y		+= ssv6051_wlan_power_control.o

endif
endif
obj-y       += lcd/
