/**
 * Frizz i2c Linux Driver Implimentation for BBB(BeagleBone Black Rec.C)
 *
 */
#include "frizz_i2c.h"
#include "frizz_reg.h"
#include "frizz_gpio.h"
#include "frizz_debug.h"
#include "frizz_ioctl.h"
#include "frizz_gpio.h"
#include "frizz_packet.h"
#include "inc/sensors/libsensors_id.h"

#include <linux/input.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/module.h>

#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/i2c/frizz.h>
#include <linux/slpt.h>
#include <linux/input.h>

/***************************************************************************/
#define MAX_ACCESS_LIMIT  	(32)		/*!< max transfer size         */
#define I2C_RW_TRY_TIMES    20

#define FRIZZ_VIO_MIN_UV	1750000
#define FRIZZ_VIO_MAX_UV	1950000

/***************************************************************************/
#define OFF 0

unsigned long pedo_data = 0;
unsigned long pedo_data_tmp = 0;
unsigned int pedo_interval = 0;
//these_onoff just for save suspend and resume time
extern unsigned int pedo_onoff;
extern unsigned int slpt_onoff;
extern unsigned int gesture_on;


extern const unsigned int config_wakeup_by_raise;

struct input_dev *frizz_input_dev;


/*!< information of general I2C device*/
static struct frizz_i2c frizz_i2c;

struct frizz_platform_data *frizz_pdata;
/*!< information of general spi device communication*/

/*!< write command*/
static unsigned char *tx_buff = 0;
/*!< read fifo data from i2c */
static unsigned char *rx_buff = 0;

static int __frizz_i2c_read(unsigned int reg_addr, unsigned char * const rx_buff, int buff_size)
{
	const unsigned char tx_buff = (unsigned char)reg_addr;
	int data_left = buff_size;
	int times_out = I2C_RW_TRY_TIMES;

	// (1) set address
	int status = i2c_master_send(frizz_i2c.i2c_client, &tx_buff, sizeof(tx_buff));
	if(status < 0){
		dump_stack();
		return status;
	}
	else{
		// receive data.
		do {
			status = i2c_master_recv(frizz_i2c.i2c_client, rx_buff, buff_size);
			if(status < 0) {
				return status;
			}
			data_left -= status;
			times_out -= 1;
		}while(data_left != 0 && times_out != 0);
		if(!times_out && data_left != 0) {
			status = data_left;
			kprint("%s rereceive data failed!!!, bytes_left = %d", __func__, data_left);
		}
	}
	//if all data receive, data_left should be Zero;
	status = data_left;
	return status;
}

static int __frizz_i2c_write(const unsigned char * const data, int data_size)
{
	int data_left = data_size;
	int status = 0;
	int times_out = I2C_RW_TRY_TIMES;

	do{
		status = i2c_master_send(frizz_i2c.i2c_client, data + (data_size - data_left), data_left);
		if(status < 0) {
			if(data_size == 5 && data[0]==0 && data[1]==0 && data[2]==0 && data[3]==0 && data[4]==1){
				DEBUG_PRINT("frizz: __frizz_i2c_write() ignore ctrl reset\n" );
				status = 0;
			}
			if(status < 0) {
				dump_stack();
			}
			return status;
		}
		data_left -= status;
		times_out -= 1;
	}while(data_left != 0 && times_out != 0);

	if(!times_out && data_left) {
		status = data_left;
		kprint("%s resend data failed!!!, bytes_left = %d", __func__, data_left);
	}

	//if all data receive, data_left should be Zero;
	status = data_left;
	return status;
}

static int __i2c_write_reg_32(unsigned int reg_addr, unsigned int data)
{
	unsigned char tx_buff[5];
	int status = 0;

	tx_buff[0] = (unsigned char)reg_addr;
	tx_buff[1] = (data >> 24) & 0xff;
	tx_buff[2] = (data >> 16) & 0xff;
	tx_buff[3] = (data >>  8) & 0xff;
	tx_buff[4] = (data >>  0) & 0xff;

	status = __frizz_i2c_write(tx_buff, sizeof(tx_buff));

	return status;
}

int i2c_write_reg_32(unsigned int reg_addr, unsigned int data)
{
	int status = 0;
	if (!frizz_i2c.i2c_client) {
		return -ENODEV;
	}

	status = down_interruptible(&frizz_i2c.semaphore);
	if (status) {
		return status;
	}
	else{
		status = __i2c_write_reg_32(reg_addr, data);

		up(&frizz_i2c.semaphore);

		return status;
	}
}

static int __i2c_read_reg_32(unsigned int reg_addr, unsigned int* data)
{
	unsigned char rx_buff[4];

	int status = __frizz_i2c_read(reg_addr, rx_buff, sizeof(rx_buff));

	if (!status) {
		*data = CONVERT_UINT(rx_buff[0], rx_buff[1] , rx_buff[2] , rx_buff[3] );
	}
	return status;
}

int i2c_read_reg_32(unsigned int reg_addr, unsigned int* data)
{
	int status = 0;
	if (!frizz_i2c.i2c_client) {
		return -ENODEV;
	}
	status = down_interruptible(&frizz_i2c.semaphore);
	if (status) {
		return status;
	}
	else {
		status = __i2c_read_reg_32(reg_addr, data);

		up(&frizz_i2c.semaphore);

		return status;
	}
}

static int __i2c_write_reg_ram_data(unsigned int reg_addr, unsigned int ram_addr, unsigned char *ram_data, int ram_size)
{
	int status = 0;

	const int send_size = ram_size + 5;

	unsigned char * const tx_reg_addr    = tx_buff;
	unsigned char * const tx_ram_addr    = tx_buff + 1;
	unsigned char * const tx_data        = tx_buff + 5;

	memset(tx_buff, 0, send_size);

	*tx_reg_addr   = (unsigned char)reg_addr;				// register address
	tx_ram_addr[0] = (ram_addr >> 24 ) & 0xff;				// ram address
	tx_ram_addr[1] = (ram_addr >> 16 ) & 0xff;
	tx_ram_addr[2] = (ram_addr >>  8 ) & 0xff;
	tx_ram_addr[3] = (ram_addr >>  0 ) & 0xff;

	memcpy(tx_data, ram_data, ram_size);					// data

	status = __frizz_i2c_write(tx_buff, send_size );

	return status;
}

int i2c_write_reg_ram_data(unsigned int reg_addr, unsigned int ram_addr, unsigned char *ram_data, int ram_size)
{
	int status = 0;


	if (!frizz_i2c.i2c_client) {
		return -ENODEV;
	}

	status = down_interruptible(&frizz_i2c.semaphore);
	if (status) {
		return status;
	} else {
		int rest_size = ram_size;
		unsigned int   dst = ram_addr;
		unsigned char* src = ram_data;

		for(rest_size = ram_size; 0 < rest_size && status == 0;){
			const int write_size = (rest_size < MAX_ACCESS_LIMIT ? rest_size : MAX_ACCESS_LIMIT );

			status = __i2c_write_reg_ram_data(reg_addr, dst, src, write_size );
			if(status)
				break;

			rest_size -= write_size;
			dst += (write_size/4);
			src += write_size;
		}
	}

	up(&frizz_i2c.semaphore);

	return status;
}

static int __i2c_read_reg_array(unsigned int reg_addr, unsigned char *array, int array_size)
{
	int status=0;

	status = __frizz_i2c_read(reg_addr, array, array_size);

	return status;
}

int i2c_read_reg_array(unsigned int reg_addr, unsigned char *array, int array_size)
{
	int status=0;

	if (!frizz_i2c.i2c_client) {
		return -ENODEV;
	}
	status = down_interruptible(&frizz_i2c.semaphore);
	if (status) {
		return status;
	} else {
		int rest_size = array_size;
		unsigned char* p = array;
		unsigned int access_addr = 0;

		if(MAX_ACCESS_LIMIT < rest_size ){
			__i2c_read_reg_32(RAM_ADDR_REG_ADDR, &access_addr);
			__i2c_write_reg_32(RAM_ADDR_REG_ADDR, access_addr);
		}

		for(rest_size = array_size; 0 < rest_size && status == 0;){
			const int read_size = (rest_size < MAX_ACCESS_LIMIT ? rest_size : MAX_ACCESS_LIMIT );

			status = __i2c_read_reg_array(reg_addr, p, read_size );
			if(status)
				break;

			rest_size -= read_size;
			p += read_size;

			if(0 < rest_size ){
				access_addr += (read_size/4);
				__i2c_write_reg_32(RAM_ADDR_REG_ADDR, access_addr);
			}
		}
	}

	up(&frizz_i2c.semaphore);

	return status;
}

static int __i2c_read_reg_uint_array(unsigned int reg_addr, unsigned int *array, int array_size)
{
	int status = 0;
	unsigned char * const rx_buff = (unsigned char*)array;
	const int rx_size             = (array_size * 4);

	status = __frizz_i2c_read(reg_addr, rx_buff, rx_size);

	if (!status) {
		const unsigned char* src         = rx_buff;
		unsigned int* dst                = array;
		const unsigned int* const beyond = dst + array_size;

		for(; dst < beyond; dst++, src+=4 ){
			*dst = CONVERT_UINT(src[0], src[1], src[2], src[3] );
		}
		status = 0;
	}
	return status;
}

int i2c_read_reg_uint_array(unsigned int reg_addr, unsigned int *array, int array_size)
{
	int status = 0;

	if (!frizz_i2c.i2c_client) {
		return -ENODEV;
	}

	status = down_interruptible(&frizz_i2c.semaphore);
	if (status) {
		return status;
	} else {
		int rest_size = (array_size * 4);
		unsigned char* p = (unsigned char*)array;

		while(0 < rest_size && status == 0){
			const int read_size = (rest_size < MAX_ACCESS_LIMIT ? rest_size : MAX_ACCESS_LIMIT );

			status = __i2c_read_reg_uint_array(reg_addr, (unsigned int *)p, read_size/4 );
			if(status)
				break;
			rest_size -= read_size;
			p += read_size;
		}
	}
	up(&frizz_i2c.semaphore);

	return status;
}

static int __init init_cdev(struct file_operations *frizz_operations)
{
	int error;

	printk("frizz: init_cdev()\n" );

	frizz_i2c.devt = MKDEV(0, 0);

	error = alloc_chrdev_region(&frizz_i2c.devt,
			0,
			DEVICE_COUNT,
			DRIVER_NAME);

	if (error < 0) {
		printk(KERN_ALERT "alloc_chrdev_region() failed: %d \n", error);
		return -1;
	}

	cdev_init(&frizz_i2c.cdev, frizz_operations);

	frizz_i2c.cdev.owner = THIS_MODULE;

	error = cdev_add(&frizz_i2c.cdev, frizz_i2c.devt, DEVICE_COUNT);

	if (error) {
		printk(KERN_ALERT "cdev_add() failed: %d\n", error);
		unregister_chrdev_region(frizz_i2c.devt, DEVICE_COUNT);
		return -1;
	}

	return 0;
}

static int __init init_sysfs(void)
{
	printk("frizz: init_sysfs()\n" );

	frizz_i2c.class = class_create(THIS_MODULE, DRIVER_NAME);

	if (IS_ERR(frizz_i2c.class)) {
		printk(KERN_ERR "class_create(..., %s) failed\n", DRIVER_NAME);
		return -1;
	}

	if (!device_create(frizz_i2c.class, NULL, frizz_i2c.devt, NULL, DRIVER_NAME)) {
		printk(KERN_ERR "device_create(..., %s) failed\n", DRIVER_NAME);
		class_destroy(frizz_i2c.class);
		return -1;
	}

	return 0;
}

int frizz_i2c_transfer_test(void)
{
	int ret;
	unsigned int tx_rx_data;

	ret = i2c_write_reg_32( RAM_ADDR_REG_ADDR, 0x00);
	if (ret == 0) {
		kprint("frizz i2c write RAM_ADDR_REG_ADDR successful.");
	}
	else {
		kprint("frizz i2c write RAM_ADDR_REG_ADDR failed.");
	}

	ret = i2c_read_reg_32(VER_REG_ADDR, &tx_rx_data);
	if (ret == 0){
		kprint("frizz i2c read VER_REG_ADDR successful, receive data = %u.", tx_rx_data);
	}else {
		kprint("frizz read VER_REG_ADDR failed.");
	}

	return ret;
}

int init_wakeup_by_raise(struct device *dev, struct input_dev *idev) {
	int ret = -1;
	if(config_wakeup_by_raise) {
		frizz_input_dev = input_allocate_device();
		if(!frizz_input_dev) {
			printk("Frizz %s, init inpt_dev failed!! \n", __func__);
			return -1;
		}
		frizz_input_dev->evbit[0] = BIT_MASK(EV_KEY);
		frizz_input_dev->name = "frizz_raise";
		frizz_input_dev->phys = "frizz_raise/input0";
		frizz_input_dev->dev.parent = dev;
		set_bit(KEY_POWER, frizz_input_dev->keybit);
		ret = input_register_device(frizz_input_dev);
		return ret;
	}
	return -1;
}


int wakeup_system_by_raise(int israise) {
	if(config_wakeup_by_raise) {
		if(israise && (frizz_input_dev)) {
			input_report_key(frizz_input_dev, KEY_POWER, 1);
			input_report_key(frizz_input_dev, KEY_POWER, 0);
			input_sync(frizz_input_dev);
			return 0;
		}
		return 1;
	}
	return -1;
}
static int frizz_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int err = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev,
				"%s: check_functionality failed.", __func__);
		err = -ENODEV;
		goto exit0;
	}

	memcpy(frizz_pdata,
			(struct frizz_platform_data *)client->dev.platform_data,
			sizeof(struct frizz_platform_data));

	frizz_i2c.i2c_client = client;
	client->addr = I2C_FRIZZ_SLAVE_ADDRESS;
	i2c_set_clientdata(client, &frizz_i2c);

	frizz_i2c.pdata = kzalloc(sizeof(struct frizz_i2c_dev_data), GFP_KERNEL);
	if (!frizz_i2c.pdata) {
		dev_err(&client->dev,
				"alloc memory error %s\n", __func__);
		return -ENODEV;
	}
	if(config_wakeup_by_raise) {
		if(init_wakeup_by_raise(&(client->dev), frizz_input_dev))
			printk("Frizz init wakeup_by_raise failed!!!\n");
	}

	printk("frizz probe done.\n");

	return 0;

exit0:
	return err;
}

static int frizz_i2c_remove(struct i2c_client *client)
{
	delete_frizz_gpio();
	printk("frizz: %s\n", frizz_i2c.i2c_client->name );

	return 0;
}

int frizz_i2c_suspend(struct i2c_client * client, pm_message_t mesg) {
	if(slpt_onoff == OFF) {
		//if slpt shutdown , do not report pedo data when system slpt.
		pedo_interval = PEDO_NOT_REPORT;
	}
	if(pedo_onoff == OFF) {
		//do nothing
	} else {
		keep_frizz_wakeup();
		set_pedo_interval(pedo_interval);
		release_frizz_wakeup();
	}

	if(gesture_on == OFF) {
		//do nothing
	}else {
		keep_frizz_wakeup();
		set_gesture_state(GESTURE_SHAKE_HAND_SILENCE);
		release_frizz_wakeup();
	}

	if(slpt_onoff == OFF) {
		//do nothing
	} else {
		slpt_set_key_by_id(SLPT_K_FRIZZ_PEDO, pedo_data);
		pedo_data_tmp = pedo_data;
		kprint("Suspend set into slpt pedo data : %ld", pedo_data);
	}
	return 0;
}
int frizz_i2c_resume(struct i2c_client * client) {
	unsigned long gesture_data = -1;
	if(pedo_onoff == OFF) {
		//do nothing
	} else {
		keep_frizz_wakeup();
		set_pedo_interval(PEDO_REPORT_NORMAL);
		release_frizz_wakeup();
	}
	if(gesture_on == OFF) {
		//do nothing
	}else {
		keep_frizz_wakeup();
		set_gesture_state(GESTURE_REPORT_ALL);
		release_frizz_wakeup();
	}

	if(slpt_onoff == OFF) {
		//do nothing
	} else {
		slpt_get_key_by_id(SLPT_K_FRIZZ_PEDO, &pedo_data);
		slpt_get_key_by_id(SLPT_K_FRIZZ_GESTURE, &gesture_data);
		kprint("Resume : get slpt PEDO data : %ld, Gesture data : %ld", pedo_data, gesture_data);

		if(pedo_data_tmp != pedo_data) {
			//report the data when data changed, or to save resume time
			create_sensor_type_fifo_fake(SENSOR_ID_ACCEL_PEDOMETER, &pedo_data, 1);
		}

		if(gesture_data > 0) {
			//means the gesture changed, report it, or save resume time
			create_sensor_type_fifo_fake(SENSOR_ID_GESTURE, &gesture_data, 1);
		}
	}
	return 0;
}

static const struct i2c_device_id frizz_device_id[] = {
	{ "frizz", 0 },
	{ }
};

static struct i2c_driver i2c_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.probe    = frizz_i2c_probe,
	.remove   = frizz_i2c_remove,
	.suspend  = frizz_i2c_suspend,
	.resume   = frizz_i2c_resume,
	.id_table = frizz_device_id,
};




static int __init init_frizz_i2c(void)
{
	int error = 0;

	printk("frizz: init_frizz_i2c()\n" );

	// regist Driver
	error =  i2c_add_driver(&i2c_driver);
	if (error < 0) {
		printk(KERN_ERR "frizz: i2c_add_driver() failed %d\n", error);
		goto fail_3;
	}

	tx_buff = kmalloc(I2C_BUFF_SIZE, GFP_KERNEL | GFP_DMA);
	rx_buff = kmalloc(I2C_BUFF_SIZE, GFP_KERNEL | GFP_DMA);
	return 0;

fail_3:

	i2c_del_driver(&i2c_driver);
	return error;

}

int i2c_open(struct file_operations *frizz_operations, struct frizz_platform_data *pdata) {

	printk("frizz: i2c_open()\n" );
	frizz_pdata = pdata;

	memset(&frizz_i2c, 0, sizeof(frizz_i2c));

	sema_init(&frizz_i2c.semaphore, 1);

	if (init_cdev(frizz_operations) < 0) {
		goto fail_1;
	}
	if (init_sysfs() < 0) {
		goto fail_2;
	}
	if (init_frizz_i2c() < 0) {
		goto fail_3;
	}
	return 0;

fail_1:
	return 1;
fail_2:
	// releasing of cdev.
	cdev_del(&frizz_i2c.cdev);
	unregister_chrdev_region(frizz_i2c.devt, DEVICE_COUNT);
	return 2;

fail_3:
	// unregister class
	device_destroy(frizz_i2c.class, frizz_i2c.devt);
	class_destroy(frizz_i2c.class);
	return 3;
}

void i2c_close(){

	printk("frizz: i2c_close()\n" );

	i2c_del_driver(&i2c_driver);

	// unregister class
	device_destroy(frizz_i2c.class, frizz_i2c.devt);
	class_destroy(frizz_i2c.class);

	// releasing of cdev
	cdev_del(&frizz_i2c.cdev);
	unregister_chrdev_region(frizz_i2c.devt, DEVICE_COUNT);

	if(tx_buff) {
		kfree(tx_buff);
	}

	if(rx_buff) {
		kfree(rx_buff);
	}
}

