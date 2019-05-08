#include "frizz_spi.h"
#include "frizz_reg.h"
#include "frizz_gpio.h"

#include <linux/input.h>
#include <linux/spinlock.h>
#include <linux/module.h>

#if 0
#define CONVERT_UINT(imm0, imm1, imm2, imm3)				\
    (unsigned int)( (((imm0)&0xFF)<<24) | (((imm1)&0xFF)<<16) | (((imm2)&0xFF)<<8) | ((imm3)&0xFF) )
#endif

/*!< information of general spi device*/
static struct frizz_spi frizz_spi;

/*!< information of general spi device communication*/
static struct frizz_spi_com frizz_spi_com;
/*!< write command*/
static unsigned char *tx_buff;
/*!< read fifo data from sp*/
static unsigned char *rx_buff;

//static struct mutex lock;

static void prepare_spi_message(int len)
{
  spi_message_init(&frizz_spi_com.message);

  memset(frizz_spi_com.rx_buff, 0, SPI_BUFF_SIZE);

  frizz_spi_com.transfer.tx_buf = frizz_spi_com.tx_buff;
  frizz_spi_com.transfer.rx_buf = frizz_spi_com.rx_buff;
  frizz_spi_com.transfer.len = len;

  spi_message_add_tail(&frizz_spi_com.transfer, &frizz_spi_com.message);
}


static int spi_transfer(unsigned char *tx_buff, unsigned char *rx_buff, int buff_size) {

  int status = 0;
  int i;

  if (down_interruptible(&frizz_spi.semaphore)) {
	return -ERESTARTSYS;
  }

  if (!frizz_spi.device) {
      up(&frizz_spi.semaphore);
      return -ENODEV;
  }

  //mutex_lock(&lock);

  for(i = 0; i < buff_size; i++) {
      frizz_spi_com.tx_buff[i] = tx_buff[i];
  }



  prepare_spi_message(buff_size);

  status = spi_sync(frizz_spi.device, &frizz_spi_com.message);

  if(rx_buff != NULL) {

	for(i = 2; i < buff_size; i++) {
	  rx_buff[i - 2] = frizz_spi_com.rx_buff[i];
	}

  }
  //mutex_unlock(&lock);
  up(&frizz_spi.semaphore);

  return status;
}

#if 0
void prepare_command(unsigned char command, unsigned int reg_addr, unsigned char *tx_buff) {
  tx_buff[0] = command;
  tx_buff[1] = (unsigned char)reg_addr;
}

void prepare_data(unsigned int data, unsigned char *tx_buff) {
  tx_buff[2] = (data >> 24) & 0xff;
  tx_buff[3] = (data >> 16) & 0xff;
  tx_buff[4] = (data >>  8) & 0xff;
  tx_buff[5] = (data >>  0) & 0xff;
}
#endif

int spi_write_reg_32(unsigned int reg_addr, unsigned int data) {

  unsigned char tx_buff[6];
  int status;

  prepare_command(WRITE_COMMAND, reg_addr, tx_buff);
  prepare_data(data, tx_buff);

  status = spi_transfer(tx_buff, NULL, 6);

  return status;
}

int spi_read_reg_32(unsigned int reg_addr, unsigned int *data) {

  unsigned char tx_buff[6];
  unsigned char rx_buff[4];

  int status;

  prepare_command(READ_COMMAND, reg_addr, tx_buff);

  status = spi_transfer(tx_buff, rx_buff, 6);

  //Attention.
  *data = (rx_buff[0] << 24) | (rx_buff[1] << 16) | (rx_buff[2] <<  8) | (rx_buff[3] << 0);
  //printk("%x %x %x %x\n", rx_buff[0], rx_buff[1], rx_buff[2], rx_buff[3]);

  return status;
}

int spi_write_reg_ram_data(unsigned int reg_addr, unsigned int ram_addr, unsigned char *ram_data, int ram_size) {

    //unsigned char *tx_buff;
  int buff_size;
  int i;

  int status;

  buff_size = ram_size + 6;

  //tx_buff = kmalloc(buff_size, GFP_KERNEL | GFP_DMA);

  memset(tx_buff, 0, buff_size);

  prepare_command(WRITE_COMMAND, reg_addr, tx_buff);
  prepare_data(ram_addr, tx_buff);

  for(i = 0; i < ram_size; i++) {
    tx_buff[i + 6] = ram_data[i];
  }

  status = spi_transfer(tx_buff, NULL, buff_size);

  //kfree(tx_buff);

  return status;
}

int spi_read_reg_array(unsigned int reg_addr, unsigned char *array, int array_size) {

    //unsigned char *tx_buff;
  int buff_size;
  int status;

  buff_size = array_size + 2;

  //tx_buff = kmalloc(buff_size, GFP_KERNEL | GFP_DMA);

  prepare_command(READ_COMMAND, reg_addr, tx_buff);

  status = spi_transfer(tx_buff, array, buff_size);

  return status;
}

int spi_read_reg_uint_array(unsigned int reg_addr, unsigned int *array, int array_size) {


    int tx_size, rx_size;
    int status;
    int i;
    int count = 0;
    tx_size = (array_size * 4) + 2;
 //   rx_size = (array_size * 4);

    prepare_command(READ_COMMAND, reg_addr, tx_buff);

    status = spi_transfer(tx_buff, rx_buff, tx_size);

    for(i = 0; i < array_size; i++) {

	array[i] = CONVERT_UINT(rx_buff[count], rx_buff[count + 1], rx_buff[count + 2], rx_buff[count + 3]);

	count = count + 4;
    }

    return status;

}

static int __init init_cdev(struct file_operations *frizz_operations)
{
	int error;

	frizz_spi.devt = MKDEV(0, 0);
	error = alloc_chrdev_region(&frizz_spi.devt, 0, DEVICE_COUNT, DRIVER_NAME);

	if (error < 0) {
		printk(KERN_ALERT "alloc_chrdev_region() failed: %d \n", error);
		return -1;
	}

	cdev_init(&frizz_spi.cdev, frizz_operations);

	frizz_spi.cdev.owner = THIS_MODULE;

	error = cdev_add(&frizz_spi.cdev, frizz_spi.devt, DEVICE_COUNT);

	if (error) {
		printk(KERN_ALERT "cdev_add() failed: %d\n", error);
		unregister_chrdev_region(frizz_spi.devt, DEVICE_COUNT);
		return -1;
	}

	return 0;
}

static int __init init_sysfs(void)
{

    frizz_spi.class = class_create(THIS_MODULE, DRIVER_NAME);

    if (IS_ERR(frizz_spi.class)) {
	printk(KERN_ERR "class_create(..., %s) failed\n", DRIVER_NAME);
	return -1;
    }

    if (!device_create(frizz_spi.class, NULL, frizz_spi.devt, NULL, DRIVER_NAME)) {
	printk(KERN_ERR "device_create(..., %s) failed\n", DRIVER_NAME);
	class_destroy(frizz_spi.class);
	return -1;
    }

    return 0;
}

static int spi_probe(struct spi_device *spi_device)
{

	printk("=== frizz probe start ===\n");

	if (down_interruptible(&frizz_spi.semaphore)) {
		return -EBUSY;
	}
	frizz_spi.device = spi_device;
	up(&frizz_spi.semaphore);

	frizz_spi.device = spi_device;

	printk("=== frizz probe end ===\n");
	return 0;
}

static int spi_remove(struct spi_device *spi_device)
{

	if (down_interruptible(&frizz_spi.semaphore)) {
		return -EBUSY;
	}
	frizz_spi.device = NULL;
	up(&frizz_spi.semaphore);


	frizz_spi.device = NULL;

	return 0;
}

static struct spi_driver spi_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.probe = spi_probe,
	.remove = __devexit_p(spi_remove),
};

static int __init add_spi_device_to_bus(void)
{
	struct spi_master *spi_master;
	struct spi_device *spi_device;
	struct device *pdev;
	char buff[64];
	int status = 0;

	spi_master = spi_busnum_to_master(SPI_BUS);
	if (!spi_master) {
		printk(KERN_ALERT "spi_busnum_to_master(%d) returned NULL\n", SPI_BUS);
		printk(KERN_ALERT "Missing modprobe omap2_mcspi?\n");
		return -1;
	}

	spi_device = spi_alloc_device(spi_master);
	if (!spi_device) {
		put_device(&spi_master->dev);
		printk(KERN_ALERT "spi_alloc_device() failed\n");
		return -1;
	}

	spi_device->chip_select = SPI_BUS_CS1;
	snprintf(buff, sizeof(buff), "%s.%u", dev_name(&spi_device->master->dev), spi_device->chip_select);
	pdev = bus_find_device_by_name(spi_device->dev.bus, NULL, buff);
	if (pdev) {
		if (pdev->driver && pdev->driver->name && strcmp(DRIVER_NAME, pdev->driver->name)) {
			printk(KERN_ERR "Driver [%s] already registered for %s\n", pdev->driver->name, buff);
			status = -1;
		}
	} else {
		spi_device->max_speed_hz = SPI_BUS_MAX_SPEED_HZ;
		spi_device->mode = SPI_MODE_0;
		spi_device->bits_per_word = 8;
		//spi_device->irq = NULL;//gpio_to_irq(GPIO_INTERRUPT_PIN);
		spi_device->controller_state = NULL;
		spi_device->controller_data = NULL;
		strlcpy(spi_device->modalias, DRIVER_NAME, SPI_NAME_SIZE);

		status = spi_add_device(spi_device);
		if (status < 0) {
			spi_dev_put(spi_device);
			printk(KERN_ERR "spi_add_device() failed: %d\n", status);
		}
	}
	put_device(&spi_master->dev);

	return status;
}

static int __init init_frizz_spi(void)
{
	int error;

	frizz_spi_com.tx_buff = kmalloc(SPI_BUFF_SIZE, GFP_KERNEL | GFP_DMA);
	if (!frizz_spi_com.tx_buff) {
		error = -ENOMEM;
		goto spi_init_error;
	}

	frizz_spi_com.rx_buff = kmalloc(SPI_BUFF_SIZE, GFP_KERNEL | GFP_DMA);
	if (!frizz_spi_com.rx_buff) {
		error = -ENOMEM;
		goto spi_init_error;
	}

	tx_buff = kmalloc(SPI_BUFF_SIZE, GFP_KERNEL | GFP_DMA);
	rx_buff = kmalloc(SPI_BUFF_SIZE, GFP_KERNEL | GFP_DMA);

        //mutex_init(&lock);

	error = spi_register_driver(&spi_driver);
	if (error < 0) {
		printk(KERN_ERR "spi_register_driver() failed %d\n", error);
		goto spi_init_error;
	}

	error = add_spi_device_to_bus();
	if (error < 0) {
		printk(KERN_ERR "add_spike_device_to_bus() failed\n");
		spi_unregister_driver(&spi_driver);
		goto spi_init_error;
	}

	return 0;

spi_init_error:
	if (frizz_spi_com.tx_buff) {
		kfree(frizz_spi_com.tx_buff);
		frizz_spi_com.tx_buff = 0;
	}
	if (frizz_spi_com.rx_buff) {
		kfree(frizz_spi_com.rx_buff);
		frizz_spi_com.rx_buff = 0;
	}

	return error;
}

int spi_open(struct file_operations *frizz_operations) {

  memset(&frizz_spi, 0, sizeof(frizz_spi));
  memset(&frizz_spi_com, 0, sizeof(frizz_spi_com));

  sema_init(&frizz_spi.semaphore, 1);

  if (init_cdev(frizz_operations) < 0) {
	goto fail_1;
  }

  if (init_sysfs() < 0) {
	goto fail_2;
  }

  if (init_frizz_spi() < 0) {
	goto fail_3;
  }

  return 0;

 fail_1:
  return 1;

 fail_2:
  // releasing of cdev.
  cdev_del(&frizz_spi.cdev);
  unregister_chrdev_region(frizz_spi.devt, DEVICE_COUNT);
  return 2;

 fail_3:
  // unregister class
  device_destroy(frizz_spi.class, frizz_spi.devt);
  class_destroy(frizz_spi.class);
  return 3;
}


void spi_close() {

  // releasing of spi
  spi_unregister_device(frizz_spi.device);
  spi_unregister_driver(&spi_driver);

  // unregister class
  device_destroy(frizz_spi.class, frizz_spi.devt);
  class_destroy(frizz_spi.class);

  // releasing of cdev
  cdev_del(&frizz_spi.cdev);
  unregister_chrdev_region(frizz_spi.devt, DEVICE_COUNT);

  // releasing of spi buffer
  if (frizz_spi_com.tx_buff) {
	kfree(frizz_spi_com.tx_buff);
  }
  if (frizz_spi_com.rx_buff) {
	kfree(frizz_spi_com.rx_buff);
  }

  if(tx_buff) {
      kfree(tx_buff);
  }

  if(rx_buff) {
      kfree(rx_buff);
  }

}
