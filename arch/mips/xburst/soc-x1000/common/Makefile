

## WARNING! DO NOT change this lists order because power sleep order depends on it!! ##

obj-y += setup.o
obj-y += reset.o
obj-y += ddr.o
obj-$(CONFIG_TIMER_SYS_OST) += timer_sys_ost.o
obj-$(CONFIG_TIMER_TCU) += timer_tcu.o

obj-y += serial.o
obj-y += tcu.o

obj-y += clk/

obj-y += gpio.o
# only for debug
#obj-y += pmmon.o
obj-y += irq.o
obj-y += cpm_usb.o

obj-y += platform.o
obj-y += regs_save_restore.o

obj-y += pm_p0.o

obj-$(CONFIG_DWC_LM_INTERFACE)  += lm.o lm-device.o

obj-y += rtc.o
obj-y += proc_idle.o
