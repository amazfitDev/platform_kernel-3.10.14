#
# Makefile for Invensense inv-mpu-iio device.
#

obj-$(CONFIG_ICM30630_INV_MPU_IIO) += inv-mpu-iio.o

ifeq ($(CONFIG_ICM30630_INV_MPU_IIO_I2C), y)
inv-mpu-iio-objs += inv_mpu_i2c.o
endif
ifeq ($(CONFIG_ICM30630_INV_MPU_IIO_SPI), y)
inv-mpu-iio-objs += inv_mpu_spi.o
endif
inv-mpu-iio-objs += inv_mpu_core.o
inv-mpu-iio-objs += inv_mpu_ring.o
inv-mpu-iio-objs += inv_mpu_trigger.o
inv-mpu-iio-objs += inv_mpu_common.o
inv-mpu-iio-objs += inv_mpu_load_image.o
inv-mpu-iio-objs += inv_mpu_dts.o
inv-mpu-iio-objs += inv_sh_data.o
inv-mpu-iio-objs += inv_sh_command.o
inv-mpu-iio-objs += inv_sh_timesync.o
inv-mpu-iio-objs += inv_sh_misc.o
inv-mpu-iio-objs += inv_sh_iio.o
inv-mpu-iio-objs += inv_sh_sensor.o
inv-mpu-iio-objs += inv_sh_sensor_activity.o
inv-mpu-iio-objs += inv_mpu_self_test.o
