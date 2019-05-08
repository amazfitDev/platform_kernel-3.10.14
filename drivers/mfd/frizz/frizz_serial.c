#include <linux/i2c/frizz.h>
#ifdef CONFIG_FRIZZ_SPI
#include "frizz_spi.h"
#elif defined CONFIG_FRIZZ_I2C
#include "frizz_i2c.h"
#endif
//#define CONFIG_FRIZZ_SPI

//TODO: create i2c function (frizz_i2c.c frizz_i2c.h)
static int frizz_test_switch = 1;

int serial_open(struct file_operations *frizz_operations, struct frizz_platform_data *pdata) {

#ifdef CONFIG_FRIZZ_SPI
    spi_open(frizz_operations);
#elif defined CONFIG_FRIZZ_I2C
    i2c_open(frizz_operations, pdata);
#endif

    return 0;
}

void serial_close(void) {

#ifdef CONFIG_FRIZZ_SPI
    spi_close();
#elif defined CONFIG_FRIZZ_I2C
    i2c_close();
#endif

}

int check_frizz_read_write_i2c(void) {
	if (frizz_test_switch == 1)
		return frizz_i2c_transfer_test();
	else
		return 0;
}

int serial_write_reg_32(unsigned int reg_addr, unsigned int data) {
#ifdef CONFIG_FRIZZ_SPI
    return  spi_write_reg_32(reg_addr, data);
#elif defined CONFIG_FRIZZ_I2C
    return  i2c_write_reg_32(reg_addr, data);
#endif
}

int serial_read_reg_32(unsigned int reg_addr, unsigned int *data) {
#ifdef CONFIG_FRIZZ_SPI
    return spi_read_reg_32(reg_addr, data);
#elif defined CONFIG_FRIZZ_I2C
    return i2c_read_reg_32(reg_addr, data);
#endif
}

int serial_write_reg_ram_data(unsigned int reg_addr, unsigned int ram_addr, unsigned char *ram_data, int ram_size) {
#ifdef CONFIG_FRIZZ_SPI
    return spi_write_reg_ram_data(reg_addr, ram_addr, ram_data, ram_size);
#elif defined CONFIG_FRIZZ_I2C
    return i2c_write_reg_ram_data(reg_addr, ram_addr, ram_data, ram_size);
#endif

}

int serial_read_reg_array(unsigned int reg_addr, unsigned char *array, int array_size) {
#ifdef CONFIG_FRIZZ_SPI
    return  spi_read_reg_array(reg_addr, array, array_size);
#elif defined CONFIG_FRIZZ_I2C
    return  i2c_read_reg_array(reg_addr, array, array_size);
#endif
}

int serial_read_reg_uint_array(unsigned int reg_addr, unsigned int *array, int array_size) {
#ifdef CONFIG_FRIZZ_SPI
    return spi_read_reg_uint_array(reg_addr, array, array_size);
#elif defined CONFIG_FRIZZ_I2C
    return i2c_read_reg_uint_array(reg_addr, array, array_size);
#endif
}
