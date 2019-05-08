frizz-objs = 					\
	frizz_fake.o				\
	frizz_manager.o 			\
	frizz_serial.o 				\
	frizz_connection.o  		\
	frizz_ioctl_sensor.o 		\
	frizz_ioctl_hardware.o 		\
	frizz_timer.o 				\
	frizz_workqueue.o 			\
	frizz_data_sensor.o 		\
	frizz_poll.o 				\
	frizz_gpio.o 				\
	frizz_packet.o 				\
	frizz_sensor_list.o 		\
	frizz_ioctl_mcc.o 			\
	frizz_data_mcc.o 			\
	frizz_io_common.o           \
	frizz_i2c.o                 \
	frizz_sysfs.o

obj-$(CONFIG_FRIZZ) += frizz.o

ccflags-y  := -DCONFIG_FRIZZ_I2C
