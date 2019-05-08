#include <linux/fs.h>
#include <linux/i2c/frizz.h>
#include "frizz_io_common.h"

#ifndef __FRIZZ_SERIAL_H__
#define __FRIZZ_SERIAL_H__

//#define DRIVER_NAME "frizz" /*!< Device driver name */

/*!
 * serial communication of initialize processing
 *
 * @param[in] struct of character device
 * @return 0=success, otherwise 0 =fail
 */
int serial_open(struct file_operations*, struct frizz_platform_data *);

/*!
 * serial communication of termination processing
 *
 * @param[in]
 * @return
 */
void serial_close(void);

/*!
 * serial communication 32bit data of writing processing
 *
 * @param[in] register address
 * @param[in] writing data
 * @return 0=sucess, otherwise 0=fail
 */
int serial_write_reg_32(unsigned int, unsigned int);

/*!
 * serial communication 32bit data of reading processing
 *
 * @param[in] register address
 * @param[out] reading data
 * @return 0=sucess, otherwise 0=fail
 */
int serial_read_reg_32(unsigned int, unsigned int*);

/*!
 * serial communication ram data of writing processing
 *
 * @param[in] register address
 * @param[in] ram address
 * @param[in] ram data
 * @param[in] ram data size
 * @return 0=sucess, otherwise 0=fail
 */
int serial_write_reg_ram_data(unsigned int, unsigned int, unsigned char*, int);

/*!
 * serial communication array data (unsigned char) of reading processing
 *
 * @param[in] register address
 * @param[out] reading data
 * @param[in] reading data size
 * @return 0=sucess, otherwise 0=fail
 */
int serial_read_reg_array(unsigned int, unsigned char*, int);

/*!
 * serial communication array data of (unsigned int) reading processing
 *
 * @param[in] register address
 * @param[out] reading data
 * @param[in] reading data size
 * @return 0=sucess, otherwise 0=fail
 */
int serial_read_reg_uint_array(unsigned int, unsigned int*, int);

int check_frizz_read_write_i2c(void);

void frizz_hard_reset(void);

#endif

