ifdef CONFIG_WATCH_AW808
BOARD_DIR = aw808
endif
ifdef CONFIG_WATCH_NEWTON2
BOARD_DIR = newton2
endif
ifdef CONFIG_WATCH_ACRAB
BOARD_DIR = acrab
endif
ifdef CONFIG_WATCH_SOLAR
BOARD_DIR = solar
endif


CFLAGS_KERNEL	+= -I$(srctree)/arch/mips/xburst/soc-m200/chip-m200/$(CONFIG_PRODUCT_NAME)/$(BOARD_DIR)

obj-y		+= common/
obj-$(CONFIG_WATCH_NEWTON2)		+= $(BOARD_DIR)/
obj-$(CONFIG_WATCH_AW808)		+= $(BOARD_DIR)/
obj-$(CONFIG_WATCH_ACRAB)		+= $(BOARD_DIR)/
obj-$(CONFIG_WATCH_SOLAR)		+= $(BOARD_DIR)/
