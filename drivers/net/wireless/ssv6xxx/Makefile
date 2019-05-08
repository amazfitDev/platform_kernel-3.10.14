PLATFORMS = x1000

KMODULE_NAME=ssv6051

KBUILD_TOP := drivers/net/wireless/ssv6xxx
ifeq ($(MAKELEVEL),0)
KBUILD_TOP := .
endif

obj-$(CONFIG_SSV6XXX) := $(KMODULE_NAME).o
$(KMODULE_NAME)-objs  := $(PLATFORMS)-generic-wlan.o liba.hex

#.PHONY: all modules clean

#all: modules

#modules:
#	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(shell pwd) modules

#strip:
#	$(CROSS_COMPILE)strip $(MODULE_NAME).ko --strip-unneeded

clean:
	rm -fr *.mod.c *.mod *.o .*.cmd *.ko *~
	rm -fr .tmp_versions
	rm -fr Module.symvers
	rm -fr Module.markers
	rm -fr modules.order
