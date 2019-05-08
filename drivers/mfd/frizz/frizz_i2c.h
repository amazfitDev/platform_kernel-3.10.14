/**
 * frizz I2Cドライバ実装 for BBB
 *
 *  - H/W:  BeagleBone Black Rev.C
 *
 *  - PIN:
 *
 *    <pre><code>
 *    ====  ====  =====
 *    端子  Pin    Data
 *    ====  ====  =====
 *     P9    19    SCL
 *     P9    20    SDA
 *    ====  ====  =====
 *    </code></pre>
 *
 *    ===> Channel: 3
 *
 *    note: Pin.17,18(CH1)はデフォルトはGPIOモードなので、今回は使用しない。
 *
 */

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
//#include "inc/linux/i2c-dev.h"
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/i2c/frizz.h>

#include "frizz_reg.h"
#include "frizz_io_common.h"

//#define STRING(str) #str

//#define FRIZZ_I2C_BUS                   (0)
//#define I2C_DEVICE_NAME           "/dev/i2c-" STRING(I2C_BUS)


//#define DEBUG_ON_MPL115A2
//#ifdef DEBUG_ON_MPL115A2
//#define I2C_FRIZZ_SLAVE_ADDRESS   (0x60)
//#else
#define I2C_FRIZZ_SLAVE_ADDRESS   (0x1c)
//#endif

#define I2C_BUFF_SIZE             (TRANS_BUFF_SIZE)   /*!< Max transfer buffer size (Byte)*/


/*! @struct frizz_i2c
 *  @brief  General-purpose SPI information
 */
struct frizz_i2c{
    struct semaphore   semaphore; /*!< Exclusive access control */
    struct cdev        cdev;      /*!< cdev information */
    struct class      *class;     /*!< class information */
    struct input_dev  *input_dev; /*!< represents an input device */
    struct i2c_client *i2c_client;
    struct i2c_adapter *i2c_adapter;
    struct frizz_i2c_dev_data *pdata;
    dev_t              devt;      /*!< device information */
};

/*! @struct frizz_i2c_dev_data
 *  @brief Store i2c interface data from device tree.
 */
struct frizz_i2c_dev_data {
  int gpio_interrupt;  /*!< frizz interrupt to host CPU */
  int gpio_reset;      /*!< frizz reset */
  struct regulator    *vio; /*!< frizz power source 1.8v */
};

#if 0
/*! @struct frizz_spi_com
 *  @brief General-purpose SPI transfer information
 */
struct frizz_spi_com {
  struct spi_message   message;	 /*!< SPI message information (This data is transfered) */
  struct spi_transfer  transfer; /*!< A record of SPI transfer information */

  u8  *tx_buff;	 /*!< buffer of transfer data */
  u8  *rx_buff;	 /*!< buffer of receive data  */
};
#endif

/*!
 * I2C initialize processing
 *
 * @param[in] struct of character device
 * @return 0=success, otherwise 0 =fail
 */
int i2c_open(struct file_operations *, struct frizz_platform_data *);

/*!
 * I2C termination processing
 *
 * @param[in]
 * @return
 */
void i2c_close(void);

/*!
 * I2C 32bit data of writing processing
 *
 * @param[in] register address
 * @param[in] writing data
 * @return 0=sucess, otherwise 0=fail
 */
int i2c_write_reg_32(unsigned int, unsigned int);

/*!
 * I2C 32bit data of reading processing
 *
 * @param[in] register address
 * @param[out] reading data
 * @return 0=sucess, otherwise 0=fail
 */
int i2c_read_reg_32(unsigned int, unsigned int*);

/*!
 * I2C ram data of writing processing
 *
 * @param[in] register address
 * @param[in] ram address
 * @param[in] ram data
 * @param[in] ram data size
 * @return 0=sucess, otherwise 0=fail
 */
int i2c_write_reg_ram_data(unsigned int, unsigned int, unsigned char*, int);

/*!
 * I2C array data (unsigned char) of reading processing
 *
 * @param[in] register address
 * @param[out] reading data
 * @param[in] reading data size
 * @return 0=sucess, otherwise 0=fail
 */
int i2c_read_reg_array(unsigned int, unsigned char*, int);

/*!
 * I2C array data of (unsigned int) reading processing
 *
 * @param[in] register address
 * @param[out] reading data
 * @param[in] reading data size
 * @return 0=sucess, otherwise 0=fail
 */
int i2c_read_reg_uint_array(unsigned int, unsigned int*, int);

/*!
 * Test frizz i2c communicate with HostCPU.
 *
 * @return 0=sucess, otherwise 0=fail
 */
int frizz_i2c_transfer_test(void);


int wakeup_system_by_raise(int israise);
