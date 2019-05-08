#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <asm/gpio.h>

#include "frizz_io_common.h"
//#define DRIVER_NAME  "frizz"   /*!< Device driver name */
//#define DEVICE_COUNT  1	       /*!< Number of device */

#define SPI_BUS		     1        /*!< SPI BUS Number */
#define SPI_BUS_CS1	     1        /*!< Chip Select Number */
#define SPI_BUS_MAX_SPEED_HZ 10000000 /*!< Max clock rate (Hz) */
#define SPI_BUFF_SIZE	     131072   /*!< Max transfer buffer size (Byte)*/

/*! @struct frizz_spi
 *  @brief  General-purpose SPI information
 */
struct frizz_spi{
    struct semaphore   semaphore; /*!< Exclusive access control */
    struct cdev	       cdev;	  /*!< cdev information */
    struct class      *class;	  /*!< class information */
    struct input_dev  *input_dev; /*!< represents an input device */
    struct spi_device *device;    /*!< General-purpose SPI information */
    dev_t	       devt;	  /*!< device information */
};


/*! @struct frizz_spi_com
 *  @brief General-purpose SPI transfer information
 */
struct frizz_spi_com {
  struct spi_message   message;	 /*!< SPI message information (This data is transfered) */
  struct spi_transfer  transfer; /*!< A record of SPI transfer information */

  u8  *tx_buff;	 /*!< buffer of transfer data */
  u8  *rx_buff;	 /*!< buffer of receive data  */
};

/*! @struct frizz_spi_dev_data
 *  @brief Store spi interface data from device tree.
 */
struct frizz_spi_dev_data {
  int power_ctrl_v1;   /*!< power control signal pin 3.3v/1.8v */
  int power_ctrl_v2;   /*!< power control signal pin 1.2v */
  int gpio_interrupt;  /*!< frizz interrupt to host CPU */
  int gpio_reset;      /*!< frizz reset */
};

/*!
 * SPI initialize processing
 *
 * @param[in] struct of character device
 * @return 0=success, otherwise 0 =fail
 */
int spi_open(struct file_operations *);

/*!
 * SPI termination processing
 *
 * @param[in]
 * @return
 */
void spi_close(void);

/*!
 * SPI 32bit data of writing processing
 *
 * @param[in] register address
 * @param[in] writing data
 * @return 0=sucess, otherwise 0=fail
 */
int spi_write_reg_32(unsigned int, unsigned int);

/*!
 * SPI 32bit data of reading processing
 *
 * @param[in] register address
 * @param[out] reading data
 * @return 0=sucess, otherwise 0=fail
 */
int spi_read_reg_32(unsigned int, unsigned int*);

/*!
 * SPI ram data of writing processing
 *
 * @param[in] register address
 * @param[in] ram address
 * @param[in] ram data
 * @param[in] ram data size
 * @return 0=sucess, otherwise 0=fail
 */
int spi_write_reg_ram_data(unsigned int, unsigned int, unsigned char*, int);

/*!
 * SPI array data (unsigned char) of reading processing
 *
 * @param[in] register address
 * @param[out] reading data
 * @param[in] reading data size
 * @return 0=sucess, otherwise 0=fail
 */
int spi_read_reg_array(unsigned int, unsigned char*, int);

/*!
 * SPI array data of (unsigned int) reading processing
 *
 * @param[in] register address
 * @param[out] reading data
 * @param[in] reading data size
 * @return 0=sucess, otherwise 0=fail
 */
int spi_read_reg_uint_array(unsigned int, unsigned int*, int);
