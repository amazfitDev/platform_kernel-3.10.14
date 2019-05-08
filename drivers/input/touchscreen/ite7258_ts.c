/* drivers/input/touchscreen/ite7258_ts.c
 *
 * FocalTech ite7258 TouchScreen driver.
 *
 * Copyright (c) 2010  Focal tech Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
//#include <mach/irqs.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <soc/gpio.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/regulator/consumer.h>
#include <linux/device.h>
#include <linux/tsc.h>
#include <linux/i2c/ite7258_firmware.h>
#include <jz_notifier.h>
#include <linux/fb.h>

#include "ite7258_ts.h"

#if defined(CONFIG_ITE7258_EXT_FUNC)
#if defined(CONFIG_ITE7258_SS_114_33FPB1501A) /* SS_114_33FPB1501A is the FPC type, the follow is the same */
#include "ITE7258_SS_114_33FPB1501A.h" /* newton(s2131e_16t) board */
#elif defined(CONFIG_ITE7258_TWS20192A0)
#include "ITE7258_TWS20192A0.h" /* acrab board */
#elif defined(CONFIG_ITE7258_H215H1606A)
#include "ITE7258_H215H1606A.h" /* aw808 board && aw808_v11_wisesquare */
#elif defined(CONFIG_ITE7258_TWS20278A0)
#include "ITE7258_TWS20278A0.h" /* aw808_v11_naturalround board */
#elif defined(CONFIG_ITE7258_FPB1504A)
#include "ITE7258_FPB1504A.h" /* foxconn board */
#elif defined(CONFIG_ITE7258_HDL015SQ01)
#include "ITE7258_HDL015SQ01.h" /* x3 board */
#elif defined(CONFIG_ITE7258_TWS20381A0)
#include "ITE7258_TWS20381A0.h" /* F1 board */
#else
unsigned char *algorithm_raw_data = NULL;
unsigned char *configurate_raw_data = NULL;
#endif
#endif
enum ite_ts_type {
	TP_ITE7258_SQUARE,    /* IT7258 , 4X4方形TP，固件FW版本号5.13.x.x */
	TP_ITE7258_FLAT,      /* IT7258, 圆形缺角TP，固件FW版本号5.17.x.x */
	TP_ITE7258_CIRCLE,    /* IT7258, 全圆TP，固件FW版本号5.16.x.x */
	TP_ITE7262_CIRCLE,    /* IT7262, 全圆TP，固件FW版本号5.15.x.x */
	TP_ITE7252_SQUARE,    /* IT7252, 4X4方形TP， 固件FW版本号3.13.x.x */
	TP_ITE_UNSUPPORT,
};

#define		CONFIG_ITE7258_MULTITOUCH
struct ite7258_ts_data {
	unsigned int irq;
	unsigned int rst;
	unsigned int rst_level;
	unsigned int x_max;
	unsigned int y_max;
	unsigned int x_pos;
	unsigned int y_pos;
	unsigned int pressure;
	unsigned int is_suspend;
	unsigned int tp_firmware_algo_ver;
	unsigned int tp_firmware_cfg_ver;
	enum ite_ts_type type;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct jztsc_platform_data *pdata;
	struct mutex lock;
	struct work_struct  work;
	struct workqueue_struct *workqueue;
	char *vcc_name;
	struct regulator *vcc_reg;
	char *vio_name;
	struct regulator *vio_reg;
	struct notifier_block tp_notif;
};

static struct ite7258_update_data *update;
static const struct attribute_group it7258_attr_group;

/*
 *ite7258_i2c_Read-read data and write data by i2c
 *@client: handle of i2c
 *@writebuf: Data that will be written to the slave
 *@writelen: How many bytes to write
 *@readbuf: Where to store data read from slave
 *@readlen: How many bytes to read
 *Returns negative errno, else the number of messages executed
 */
int ite7258_i2c_Read(struct i2c_client *client, char *writebuf,
		int writelen, char *readbuf, int readlen)
{
	int ret;
	struct ite7258_ts_data *ts = i2c_get_clientdata(client);
	if(ts->is_suspend)
		return 0;
	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
				.addr = client->addr,
				.flags = 0,
				.len = writelen,
				.buf = writebuf,
			},
			{
				.addr = client->addr,
				.flags = I2C_M_RD,
				.len = readlen,
				.buf = readbuf,
			},
		};
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret < 0)
			dev_err(&client->dev, "f%s: i2c read error.\n",
					__func__);
	} else {
		struct i2c_msg msgs[] = {
			{
				.addr = client->addr,
				.flags = I2C_M_RD,
				.len = readlen,
				.buf = readbuf,
			},
		};
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&client->dev, "%s:i2c read error.\n", __func__);
	}
	return ret;
}

/*write data by i2c*/
int ite7258_i2c_Write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret;

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = writelen,
			.buf = writebuf,
		},
	};

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret < 0)
		dev_err(&client->dev, "%s i2c write error.\n", __func__);

	return ret;
}

static void  ite7258_wait_command_done(struct i2c_client *client)
{
	unsigned char wbuffer[2];
	unsigned char rbuffer[2];
	unsigned int count = 0;

	do{
		wbuffer[0] = QUERY_BUF_ADDR;
		rbuffer[0] = 0x00;
		ite7258_i2c_Read(client, wbuffer, 1, rbuffer, 1);
		count++;
		if(rbuffer[0] & 0x01)
			mdelay(1);
	}while(rbuffer[0] & 0x01 && count < 500);
}

static void ite7258_enter_sleep_mode(struct ite7258_ts_data *ts)
{
	unsigned char wbuf[8] = {0xFF};
	int ret = 0;
	struct i2c_client *client = ts->client;

	ite7258_wait_command_done(client);
	wbuf[0] = CMD_BUF_ADDR;
	wbuf[1] = 0x04;
	wbuf[2] = 0x00;
	wbuf[3] = 0x02;
	ret = ite7258_i2c_Write(client, wbuf, 4); //sleep power mode
	if (ret < 0)
		printk("ite7258_enter_sleep_mode failed i2c write error\n");
}

static void ite7258_exit_sleep_mode(struct ite7258_ts_data *ts)
{
	unsigned char wbuf[8] = {0xFF};
	int ret = 0;
	struct i2c_client *client = ts->client;

	switch (ts->type) {
		case TP_ITE7252_SQUARE:
			wbuf[0] = CMD_BUF_ADDR;
			wbuf[1] = 0x6f;
			ret = ite7258_i2c_Write(client, wbuf, 2); //softwave  reset
			if (ret < 0)
				printk("softwave  reset failed\n");
			mdelay(20);
			break;
		case TP_ITE7258_SQUARE:
		case TP_ITE7262_CIRCLE:
			wbuf[0] = QUERY_BUF_ADDR;
			ret = ite7258_i2c_Read(client, wbuf, 1, wbuf, 1);
			if (ret < 0)
				printk("ite7258_exit_sleep_mode failed\n");
			break;
		case TP_ITE_UNSUPPORT:
			printk("driver not support this TP\n");
			break;
		default:
			printk("unkonw ITE TP type\n");
			break;
	}
}

static bool ite7258_enter_update_mode(struct i2c_client *client)
{
	unsigned char cmd_data_buf[MAX_BUFFER_SIZE];
	char cmd_response[2] = {0xFF, 0xFF};

	ite7258_wait_command_done(client);

	cmd_data_buf[1] = 0x60;
	cmd_data_buf[2] = 0x00;
	cmd_data_buf[3] = 'I';
	cmd_data_buf[4] = 'T';
	cmd_data_buf[5] = '7';
	cmd_data_buf[6] = '2';
	cmd_data_buf[7] = '6';
	cmd_data_buf[8] = '0';
	cmd_data_buf[9] = 0x55;
	cmd_data_buf[10] = 0xAA;

	cmd_data_buf[0] = 0x20;
	if(!ite7258_i2c_Write(client, cmd_data_buf, 11)){
		printk("XXX %s, %d\n", __func__, __LINE__);
		return false;
	}

	ite7258_wait_command_done(client);
	cmd_data_buf[0] = 0xA0;
	if(!ite7258_i2c_Read(client, cmd_data_buf, 1, cmd_response, 2) ){
		printk("XXX %s, %d\n", __func__, __LINE__);
		return false;
	}

	if(cmd_response[0] | cmd_response[1] ){
		printk("XXX %s, %d\n", __func__, __LINE__);
		return false;
	}
	return true;
}

bool ite7258_exit_update_mode(struct i2c_client *client)
{
	char cmd_data_buf[MAX_BUFFER_SIZE];
	char cmd_response[2] = {0xFF, 0xFF};

	ite7258_wait_command_done(client);

	cmd_data_buf[0] = 0x20;
	cmd_data_buf[1] = 0x60;
	cmd_data_buf[2] = 0x80;
	cmd_data_buf[3] = 'I';
	cmd_data_buf[4] = 'T';
	cmd_data_buf[5] = '7';
	cmd_data_buf[6] = '2';
	cmd_data_buf[7] = '6';
	cmd_data_buf[8] = '0';
	cmd_data_buf[9] = 0xAA;
	cmd_data_buf[10] = 0x55;

	if(!ite7258_i2c_Write(client, cmd_data_buf, 11)){
		printk("XXX %s, %d\n", __func__, __LINE__);
		return false;
	}

	ite7258_wait_command_done(client);

	cmd_data_buf[0] = 0xA0;
	if(!ite7258_i2c_Read(client, cmd_data_buf, 1, cmd_response, 2)){
		printk("XXX %s, %d\n", __func__, __LINE__);
		return false;
	}

	if(cmd_response[0] | cmd_response[1]){
		printk("XXX %s, %d\n", __func__, __LINE__);
		return false;
	}
	return true;
}

static bool ite7258_firmware_reinit(struct i2c_client *client)
{
	u8 cmd_data_buf[2];
	ite7258_wait_command_done(client);
	cmd_data_buf[0] = 0x20;
	cmd_data_buf[1] = 0x6F;
	if(ite7258_i2c_Write(client, cmd_data_buf, 2) <= 0){
		printk("XXX %s, %d\n", __func__, __LINE__);
		return false;
	}
	return true;
}

static bool ite7258_setupdate_offset(struct i2c_client *client,
		unsigned short offset)
{
	u8 command_buf[MAX_BUFFER_SIZE];
	char command_respon_buf[2] = {0xFF, 0xFF};

	ite7258_wait_command_done(client);

	command_buf[0] = 0x20;
	command_buf[1] = 0x61;
	command_buf[2] = 0;
	command_buf[3] = (offset & 0x00FF);
	command_buf[4] = ((offset & 0xFF00) >> 8);

	if(!ite7258_i2c_Write(client, command_buf, 5)){
		printk("XXX %s, %d\n", __func__, __LINE__);
		return false;
	}

	ite7258_wait_command_done(client);

	command_buf[0] = 0xA0;
	if(ite7258_i2c_Read(client, command_buf, 1, command_respon_buf, 2) <= 0){
		printk("XXX %s, %d\n", __func__, __LINE__);
		return false;
	} else {
		printk(".");
	}

	if(command_respon_buf[0] | command_respon_buf[1]){
		printk("XXX %s, %d\n", __func__, __LINE__);
		return false;
	}

	return true;
}

static bool ite7258_really_update(struct i2c_client *client,
		unsigned int length, char *date, unsigned short offset)
{
#define READ_BUFFER_LENGTH       128
	unsigned int index = 0;
	unsigned char buffer[READ_BUFFER_LENGTH+3] = {0};
	unsigned char buf_write[READ_BUFFER_LENGTH+3] = {0};
	unsigned char buf_read[READ_BUFFER_LENGTH+3] = {0};

	int retry_count;
	int i = 0;
	int ret = -1;
	while(index < length){
		retry_count = 0;
		do{
			ite7258_setupdate_offset(client, offset + index);
			buffer[0] = 0x20;
			buffer[1] = 0x62;
			buffer[2] = READ_BUFFER_LENGTH;
			memcpy(&buf_write[0],  &date[index], READ_BUFFER_LENGTH);
			memcpy(&buffer[3], &date[index], READ_BUFFER_LENGTH);
			ret = ite7258_i2c_Write(client, buffer, READ_BUFFER_LENGTH+3);
			if (ret < 0)
				printk("ite7258_really_update i2c write error\n");

			// Read from Flash
			buffer[0] = 0x20;
			buffer[1] = 0x63;
			buffer[2] = READ_BUFFER_LENGTH;

			ite7258_setupdate_offset(client, offset + index);
			ret = ite7258_i2c_Write(client, buffer, 3);
			if (ret < 0)
				printk("ite7258_really_update i2c write error\n");
			ite7258_wait_command_done(client);

			buffer[0] = 0xA0;
			ite7258_i2c_Read(client, buffer, 1, buf_read, READ_BUFFER_LENGTH);
			// Compare
			ret = memcmp(buf_read,buf_write, READ_BUFFER_LENGTH);
			if(ret == 0) {
				i = READ_BUFFER_LENGTH;
				break;
			}
			else {
				i = 0;
				continue;
			}
		}while (retry_count++ < 4);

		if (retry_count == 4 && i != READ_BUFFER_LENGTH){
			printk("XXX %s, %d\n", __func__, __LINE__);
			return false;
		}
		index += READ_BUFFER_LENGTH;
	}
	printk("\n");
	return true;
}
#ifdef CONFIG_ITE7258_SYSFS_DEBUG
static bool ite7258_sys_firmware_down(struct i2c_client *client,
		unsigned int length, char *data, unsigned short offset)
{
	if((length == 0 || data == NULL)){
		printk("failed %s, %d\n", __func__, __LINE__);
		return false;
	}

	if(!ite7258_enter_update_mode(client)){
		printk("failed %s, %d\n", __func__, __LINE__);
		return false;
	}

	if(length != 0 && data != NULL){
		// Download firmware
		if(!ite7258_really_update(client, length, data, offset)){
			printk("XXX %s, %d\n", __func__, __LINE__);
			return false;
		}
	}

	if(!ite7258_exit_update_mode(client)){
		printk("failed %s, %d\n", __func__, __LINE__);
		return false;
	}

	if(!ite7258_firmware_reinit(client)){
		printk("failed %s, %d\n", __func__, __LINE__);
		return false;
	}
	printk("Download firmware OK %s, %d\n", __func__, __LINE__);
	return true;
}

/* upgrade firmware with *.bin file
 * */
static int ite7258_fw_upgrade_with_app_file(struct i2c_client *client, char *name)
{
	int ret = 0;
	unsigned int fw_size = 0;
	struct file *fw_fd = NULL;
	unsigned short wFlashSize = 0x8000;
	mm_segment_t fs;
	const char algo_firmware_flag[] = "IT7260FW";
	const char config_firmware_flag[] = "CONFIG";

	u8 *firmware_data = kzalloc(0x8000, GFP_KERNEL);
	if(firmware_data  == NULL ) {
		pr_err("kzalloc firmware_data failed\n");
		return -1;
	}

	fs = get_fs();
	set_fs(get_ds());

	fw_fd = filp_open(name, O_RDONLY, 0);
	if (IS_ERR(fw_fd)) {
		pr_err("error occured while opening file %s.\n", name);
		kfree(firmware_data);
		return -EIO;
	}

	fw_size = fw_fd->f_op->read(fw_fd, firmware_data, 0x8000, &fw_fd->f_pos);

	set_fs(fs);
	filp_close(fw_fd,NULL);

	/* check is configurate or algo firmware  */
	if (strncmp(algo_firmware_flag, firmware_data, strlen(algo_firmware_flag)) == 0) {
		pr_err("the file is algo firmware\n");
		ret = ite7258_sys_firmware_down(client, fw_size, firmware_data, 0);
	} else if (strncmp(config_firmware_flag, &firmware_data[fw_size-16], strlen(config_firmware_flag)) == 0) {
		pr_err("the file is config firmware\n");

		ret = ite7258_sys_firmware_down(client, fw_size, firmware_data, wFlashSize-fw_size);

	} else {
		pr_err("the file is neither config or algo firmware\n");
		ret = false;
	}

	kfree(firmware_data);
	if (ret == false){
		pr_err("Upgrage firmware Failed\n");
		return -1;
	} else {
		pr_err("Upgrage firmware Sucess\n");
		return 0;
	}
}

static int ite7258_sys_power_on(struct ite7258_ts_data *ts)
{
	ite7258_exit_sleep_mode(ts);

	gpio_direction_output(ts->rst, 1);
	msleep(10);
	gpio_direction_output(ts->rst, 0);
	if (ts->rst_level) {
		msleep(10);
		gpio_direction_output(ts->rst, ts->rst_level);
	}
	gpio_direction_input(ts->client->irq);

	msleep(10);

	return 0;
}

static int ite7258_sys_power_off(struct ite7258_ts_data *ts)
{
	ts->is_suspend = 1;
	ite7258_enter_sleep_mode(ts);

	return 0;
}

static ssize_t ite7258_firmware_verion_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	unsigned char wbuffer[9];
	unsigned char rbuffer[9];
	int power_onoff = 0; // =1 is suspend need to power on and off
	// =0 no need to power on off
	int ret = -1;
	int buf_len = 0;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct ite7258_ts_data *ts = (struct ite7258_ts_data *)i2c_get_clientdata(client);
	mutex_lock(&ts->lock);
	/*power on*/
	if (ts->is_suspend) {
		ite7258_sys_power_on(ts);
		ts->is_suspend = 0;
		power_onoff = 1;  //=1 is suspend need to power on and off
	} else
		disable_irq(ts->irq);

	ite7258_wait_command_done(client);
	/* Firmware Information */
	wbuffer[0] = CMD_BUF_ADDR;
	wbuffer[1] = 0x01;
	wbuffer[2] = 0x00;
	ret = ite7258_i2c_Write(client, wbuffer, 3);
	if (ret < 0) {
		buf_len = -1;
		goto exit_i2c_write_error;
	}

	ite7258_wait_command_done(client);

	wbuffer[0] = CMD_RESPONSE_BUF_ADDR;
	memset(rbuffer, 0xFF,  8);
	ret = ite7258_i2c_Read(client, wbuffer, 1, rbuffer, 9);
	if (ret < 0) {
		buf_len = -1;
		goto exit_i2c_read_error;
	}

	sprintf(buf, "ITE7258 Touch Panel FW Version:%d.%d.%d.%d\tExtension ROM Version:%d.%d.%d.%d\n",\
			rbuffer[1], rbuffer[2], rbuffer[3], rbuffer[4], \
			rbuffer[5], rbuffer[6], rbuffer[7], rbuffer[8]);
	buf_len = strlen(buf);
	ite7258_wait_command_done(client);
	/* Configuration Version */
	wbuffer[0] = CMD_BUF_ADDR;
	wbuffer[1] = 0x01;
	wbuffer[2] = 0x06;
	ret = ite7258_i2c_Write(client, wbuffer, 3);
	if (ret < 0) {
		buf_len = -1;
		goto exit_i2c_write_error;
	}

	ite7258_wait_command_done(client);
	memset(rbuffer, 0xFF,  8);
	wbuffer[0] = CMD_RESPONSE_BUF_ADDR;
	ret = ite7258_i2c_Read(client, wbuffer, 1, rbuffer, 7);
	if (ret < 0) {
		buf_len = -1;
		goto exit_i2c_read_error;
	}

	sprintf(&buf[buf_len], "ITE7258 Touch Panel Configuration Version:%x.%x.%x.%x\n",
			rbuffer[1], rbuffer[2], rbuffer[3], rbuffer[4]);
	buf_len = strlen(buf);

exit_i2c_write_error:
exit_i2c_read_error:
	if(power_onoff) {  //=1 is suspend need to power on and off
		ite7258_sys_power_off(ts);
		ts->is_suspend = 1;
		power_onoff = 0;
	} else
		enable_irq(ts->irq);

	mutex_unlock(&ts->lock);

	return buf_len+1;
}

static ssize_t ite7258_upgrade_firmware_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret = 0;
	char fwname[128];
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct ite7258_ts_data *ts = (struct ite7258_ts_data *)i2c_get_clientdata(client);
	int power_onoff = 0;  // =1 is suspend need to power on and off
	// =0 no need to power on off

	memset(fwname, 0, sizeof(fwname));
	sprintf(fwname, "%s", buf);
	fwname[count - 1] = '\0';

	mutex_lock(&ts->lock);
	/* power on */
	if (ts->is_suspend) {
		ite7258_sys_power_on(ts);
		ts->is_suspend = 0;
		power_onoff = 1;  //=1 is suspend need to power on and off
	} else
		disable_irq(ts->irq);

	/* start to upgrade binary file*/
	ret = ite7258_fw_upgrade_with_app_file(client, fwname);

	if(power_onoff) {  //=1 is suspend need to power on and off
		ite7258_sys_power_off(ts);
		ts->is_suspend = 1;
		power_onoff = 0;
	} else
		enable_irq(ts->irq);

	mutex_unlock(&ts->lock);

	return count;
}

static ssize_t ite7258_tp_info_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct ite7258_ts_data *ts = (struct ite7258_ts_data *)i2c_get_clientdata(client);
	int n;

	switch (ts->type) {
		case TP_ITE7258_SQUARE:
			n = sprintf(buf, "IC: IT7258, Shape: 4X4 Square, ROW Version: 5.13.x.x\n");
			break;
		case TP_ITE7258_FLAT:
			n = sprintf(buf, "IC: IT7258, Shape: Circle Missing Angle, ROW Version: 5.17.x.x\n");
			break;
		case TP_ITE7258_CIRCLE:
			n = sprintf(buf, "IC: IT7258, Shape: Circle, ROW Version: 5.16.x.x\n");
			break;
		case TP_ITE7262_CIRCLE:
			n = sprintf(buf, "IC: IT7262, Shape: Circle, ROW Version: 5.15.x.x\n");
			break;
		case TP_ITE7252_SQUARE:
			n = sprintf(buf, "IC: ITE7252, Shape: 4X4 Square, ROW Version: 3.13.x.x\n");
			break;
		case TP_ITE_UNSUPPORT:
			n = sprintf(buf, "IC: Unsupport, Shape: Unsupport, ROW Version: Unsupport\n");
			break;
		default:
			n = sprintf(buf, "IC: Unknow, Shape: Unknow, ROW Version: Unknow\n");
			break;
	}

	return n + 1;
}

/* show the firmware version
 *  example:cat ite7258_firmware_verion
 */
static DEVICE_ATTR(ite7258_firmware_verion, S_IRUGO | S_IWUSR,
		ite7258_firmware_verion_show,
		NULL);

/* upgrade configurate and algo firmware from app.bin
 *  example:echo "*_app.bin" > ite7258_upgrade_configurate_firmware
 */

static DEVICE_ATTR(ite7258_upgrade_firmware, S_IRUGO | S_IWUSR,
		NULL,
		ite7258_upgrade_firmware_store);

/* show the TP infomation
 *  example:cat ite7258_tp_info
 */
static DEVICE_ATTR(ite7258_tp_info, S_IRUGO | S_IWUSR,
		ite7258_tp_info_show,
		NULL);

/*add your attr in here*/
static struct attribute *ite7258_attributes[] = {
	&dev_attr_ite7258_firmware_verion.attr,
	&dev_attr_ite7258_upgrade_firmware.attr,
	&dev_attr_ite7258_tp_info.attr,
	NULL
};

static const struct attribute_group it7258_attr_group = {
	.attrs = ite7258_attributes
};

/*create sysfs for debug update firmware*/
int ite7258_create_sysfs(struct i2c_client *client)
{
	int err;

	err = sysfs_create_group(&(client->dev.kobj), &it7258_attr_group);
	if(err){
		dev_err(&client->dev, "failed to register sysfs\n");
		sysfs_remove_group(&client->dev.kobj, &it7258_attr_group);
		return -EIO;
	}else{
		printk("create it7260 sysfs attr_group sucontinues\n");
	}

	return err;
}

void ite7258_release_sysfs(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &it7258_attr_group);
}
#endif

#if defined(CONFIG_ITE7258_EXT_FUNC)
/*return: -3 firmware is not match could not upgrade
 *      : -2 firmware is lastest   no need upgrade
 *      : -1 firmware is invalid   could not upgrade
 *      :  0 firmware will be  upgrade */
static int ite7258_firmware_version_check(struct ite7258_ts_data *ite7258_ts)
{
	unsigned int host_firmware_algo_ver;
	unsigned int host_firmware_cfg_ver;
	unsigned int host_firmware_algo_ver_tmp;
	unsigned int tp_firmware_algo_ver_tmp;
	int length;

	if (update == NULL) {
		printk("ite7258_firmware_down firmware is invalid\n");
		return -1;
	}

	if((update->algo_firmware_length == 0 || update->algo_firmware_data == NULL) && \
			(update->configurate_firmware_length == 0 || update->configurate_firmware_data == NULL)){
		dev_err(&update->client->dev, "fw or cfg is invalid. no need to upgrage\n");
		return -1;
	}
	length = update->configurate_firmware_length;
	host_firmware_algo_ver = (update->algo_firmware_data[8] << 24) | \
				 (update->algo_firmware_data[9] << 16) | \
				 (update->algo_firmware_data[10] << 8) | \
				 (update->algo_firmware_data[11]);

	host_firmware_cfg_ver = (update->configurate_firmware_data[length-8] << 24) | \
				(update->configurate_firmware_data[length-7] << 16) | \
				(update->configurate_firmware_data[length-6] << 8) | \
				(update->configurate_firmware_data[length-5]);

	host_firmware_algo_ver_tmp = host_firmware_algo_ver & 0xFFFF0000;
	tp_firmware_algo_ver_tmp   = ite7258_ts->tp_firmware_algo_ver & 0xFFFF0000;
	if ((host_firmware_algo_ver_tmp == tp_firmware_algo_ver_tmp) || (tp_firmware_algo_ver_tmp == 0x0)) {

		if (host_firmware_algo_ver != ite7258_ts->tp_firmware_algo_ver ||
				host_firmware_cfg_ver != ite7258_ts->tp_firmware_cfg_ver) {
			//printk("ite7258_firmware_version_check  need to upgrade\n");
			return 0;
		}
		else {
			//printk("ite7258_firmware_version_check  no need to upgrade\n");
			return -2;
		}
	} else {
		//printk("ite7258_firmware_version do not match could not upgrade\n");
		return -3;
	}
}

static bool ite7258_auto_firmware_down(struct ite7258_ts_data *ite7258_ts)
{
	unsigned int host_fw_ver  = 0;
	unsigned int host_cfg_ver = 0;
	int ret = 0;

	if (update == NULL) {
		printk("ite7258_firmware_down firmware is invalid\n");
		return false;
	}
	if((update->algo_firmware_length == 0 || update->algo_firmware_data == NULL) && \
			(update->configurate_firmware_length == 0 || update->configurate_firmware_data == NULL)){
		dev_err(&update->client->dev, "fw or cfg is invalid. no need to upgrage\n");
		return false;
	}
	printk("ite7258_firmware_down %s, %d\n", __func__, __LINE__);

	if(!ite7258_enter_update_mode(update->client)){
		dev_err(&update->client->dev, "enter update mode failed\n");
		return false;
	}
	printk("ite7258_enter_update_mode %s, %d\n", __func__, __LINE__);

	if(update->algo_firmware_length != 0 && update->algo_firmware_data != NULL){
		// Download firmware
		host_fw_ver = (update->algo_firmware_data[8] << 24) | (update->algo_firmware_data[9] << 16) | \
			      (update->algo_firmware_data[10] << 8) | (update->algo_firmware_data[11]);

		if (host_fw_ver != ite7258_ts->tp_firmware_algo_ver) {
			ret = ite7258_really_update(update->client, update->algo_firmware_length,
					update->algo_firmware_data, 0);
			if (!ret) {
				dev_err(&update->client->dev, "update firmware failed version = %d\n", host_fw_ver);
				return false;
			}
		} else {
			printk("The algo firmware is the latest. No need to update.\n");
		}
		printk("ite7258_auto_update old_fw_ver = 0x%08x\n", ite7258_ts->tp_firmware_algo_ver);
		printk("ite7258_auto_update new_fw_ver = 0x%08x\n", host_fw_ver);
	}

	if(update->configurate_firmware_length != 0 && update->configurate_firmware_data != NULL) {
		// Download configuration
		int length = update->configurate_firmware_length;
		unsigned short wFlashSize = 0x8000;
		host_cfg_ver = (update->configurate_firmware_data[length-8] << 24) | (update->configurate_firmware_data[length-7] << 16) | \
			       (update->configurate_firmware_data[length-6] << 8) | (update->configurate_firmware_data[length-5]);

		if (host_cfg_ver != ite7258_ts->tp_firmware_cfg_ver) {
			ret = ite7258_really_update(update->client, update->configurate_firmware_length, \
					update->configurate_firmware_data,\
					wFlashSize - (unsigned short)update->configurate_firmware_length);
			if (!ret) {
				dev_err(&update->client->dev, "update configuration failed version = %d\n", host_cfg_ver);
				return false;
			}
		} else {
			printk("The configuration is the latest. No need to update.\n");
		}
		printk("ite7258_auto_update old_cfg_ver = 0x%08x\n", ite7258_ts->tp_firmware_cfg_ver);
		printk("ite7258_auto_update new_cfg_ver = 0x%08x\n", host_cfg_ver);
	}

	if(!ite7258_exit_update_mode(update->client)) {
		dev_err(&update->client->dev, "exit update mode failed\n");
		return false;
	}
	printk("ite7258_exit_update_mode %s, %d\n", __func__, __LINE__);

	if(!ite7258_firmware_reinit(update->client)){
		dev_err(&update->client->dev, "firmware_reinit failed\n");
		return false;
	}
	printk("ite7258_firmware_reinit OK\n");

	return true;
}

static int IT7258_auto_upgrade_fw(struct ite7258_ts_data *ite7258_ts)
{
	int ret = -1;
	ret = ite7258_firmware_version_check(ite7258_ts);

	switch (ret) {
		case 0:
			pr_debug("IT7258 update firmware will upgrade\n");
			break;
		case -1:
			printk("IT7258 update firmware is invalid could not upgrade\n");
			break;
		case -2:
			pr_debug("IT7258 update firmware is the latest no need to upgrade\n");
			break;
		case -3:
			printk("IT7258 update firmware do not match could not upgrade\n");
			break;
		default:
			printk("IT7258 firmware_version_check ret not defined\n");
			break;

	}
	if (ret < 0)
		return 0;

	if (ite7258_auto_firmware_down(ite7258_ts) == true){
		printk("IT7258_auto upgrade_OK\n");
		return 1;
	} else {
		printk("IT7258_auto upgrade Unfinished\n");
		return -1;
	}
}
#endif
static void ite7258_report_value(struct ite7258_ts_data *data)
{
	/* if you swap xy, your x_max and y_max should better be the same value */
#ifdef CONFIG_TSC_SWAP_XY
	tsc_swap_xy(&data->x_pos, &data->y_pos);
#endif

#ifdef CONFIG_TSC_SWAP_X
	tsc_swap_x(&data->x_pos, data->x_max);
#endif

#ifdef CONFIG_TSC_SWAP_Y
	tsc_swap_y(&data->y_pos, data->y_max);
#endif

#ifdef  CONFIG_ITE7258_MULTITOUCH
	input_report_abs(data->input_dev, ABS_MT_POSITION_X, data->x_pos);
	input_report_abs(data->input_dev, ABS_MT_POSITION_Y, data->y_pos);
	input_report_abs(data->input_dev, ABS_MT_PRESSURE,   data->pressure);
	input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 128);
	input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR, 128);
#else
	input_report_abs(data->input_dev, ABS_X, data->x_pos);
	input_report_abs(data->input_dev, ABS_Y, data->y_pos);
	//input_report_abs(data->input_dev, ABS_PRESSURE, 0xF);
#endif
}

static int ite7258_read_Touchdata(struct ite7258_ts_data *data)
{
	int ret = -1;
	int xraw, yraw,pressure;
	unsigned char pucPoint[14];

	if (data->is_suspend) {
		input_mt_sync(data->input_dev);
		input_sync(data->input_dev);
		return 0;
	}
	pucPoint[0] = QUERY_BUF_ADDR; //reg addr
	ret = ite7258_i2c_Read(data->client, pucPoint, 1, pucPoint, 1);

	if((pucPoint[0] & 0x01)) { // is busy
		return 0;
	}
	if((pucPoint[0] & 0x80)) {
		pucPoint[0] = POINT_INFO_BUF_ADDR;
		ret = ite7258_i2c_Read(data->client, pucPoint, 1, pucPoint, 14);
		if(pucPoint[1] & 0x01) { //Palm
			return 0;
		}

#ifdef  CONFIG_ITE7258_MULTITOUCH
		if (pucPoint[0] & 0x01) {
			xraw = ((pucPoint[3] & 0x0F) << 8) + pucPoint[2];
			yraw = ((pucPoint[3] & 0xF0) << 4) + pucPoint[4];
			pressure = (pucPoint[5] & 0x0F);
			data->x_pos = xraw;
			data->y_pos = yraw;
			data->pressure = pressure;
			ite7258_report_value(data);
			input_mt_sync(data->input_dev);
			//input_sync(data->input_dev);
		}
		if (pucPoint[0] & 0x02) {
			xraw = ((pucPoint[7] & 0x0F) << 8) + pucPoint[6];
			yraw = ((pucPoint[7] & 0xF0) << 4) + pucPoint[8];
			pressure = (pucPoint[9] & 0x0F);
			data->x_pos = xraw;
			data->y_pos = yraw;
			data->pressure = pressure;
			ite7258_report_value(data);
			input_mt_sync(data->input_dev);
			//input_sync(data->input_dev);
		}
		input_mt_sync(data->input_dev);
		input_sync(data->input_dev);
#else
		if(pucPoint[0] & 0x01){
			xraw = ((pucPoint[3] & 0x0F) << 8) + pucPoint[2];
			yraw = ((pucPoint[3] & 0xF0) << 4) + pucPoint[4];
			data->x_pos = xraw;
			data->y_pos = yraw;
			pressure = (pucPoint[5] & 0x0F);
			input_report_key(data->input_dev, BTN_TOUCH, 1);
			ite7258_report_value(data);
			input_sync(data->input_dev);
		}
#endif
	}
	return 0;

}

static void ite7258_work_handler(struct work_struct *work)
{
	struct ite7258_ts_data *ite7258_ts = \
					     container_of(work, struct ite7258_ts_data, work);
	int ret = 0;

	ret = ite7258_read_Touchdata(ite7258_ts);

	enable_irq(ite7258_ts->irq);
}

/*The ite7258 device will signal the host about TRIGGER_FALLING.
 *Processed when the interrupt is asserted.
 */
static irqreturn_t ite7258_ts_interrupt(int irq, void *dev_id)
{
	struct ite7258_ts_data *ite7258_ts = dev_id;

	jz_notifier_call(NOTEFY_PROI_NORMAL, JZ_CLK_CHANGING, NULL);
	disable_irq_nosync(ite7258_ts->irq);

#if 1
	if (ite7258_ts->is_suspend)
		return IRQ_HANDLED;
#endif

	if (!work_pending(&ite7258_ts->work)) {
		queue_work(ite7258_ts->workqueue, &ite7258_ts->work);
	} else {
		enable_irq(ite7258_ts->irq);
	}

	return IRQ_HANDLED;
}

static void ite7258_idle_mode(struct ite7258_ts_data *ts)
{
	unsigned char wbuf[8] = {0xFF};
	int ret = 0;
	struct i2c_client *client = ts->client;

	wbuf[0] = CMD_BUF_ADDR;
	wbuf[1] = 0x12;
	wbuf[2] = 0x00;
	wbuf[3] = 0xB8;
	wbuf[4] = 0x1B;
	wbuf[5] = 0x00;
	wbuf[6] = 0x00;
	ret = ite7258_i2c_Write(client, wbuf, 7);
	if (ret < 0)
		printk("ite7258_idle_mode i2c write error\n");
	ite7258_wait_command_done(client);

	wbuf[0] = CMD_BUF_ADDR;
	wbuf[1] = 0x11;
	wbuf[2] = 0x00;
	wbuf[3] = 0x01;
	ret = ite7258_i2c_Write(client, wbuf, 4);
	if (ret < 0)
		printk("ite7258_idle_mode i2c write error\n");
	ite7258_wait_command_done(client);

	ite7258_wait_command_done(client);
	wbuf[0] = CMD_BUF_ADDR;
	wbuf[1] = 0x04;
	wbuf[2] = 0x00;
	wbuf[3] = 0x01;
	ret = ite7258_i2c_Write(client, wbuf, 4);
	if (ret < 0)
		printk("ite7258_idle_mode i2c write error\n");
}


static int ite7258_print_version(struct ite7258_ts_data *ite7258_ts)
{
#ifdef  ITE7258_DEBUG
	unsigned char wbuffer[9];
	unsigned char rbuffer[9];
	struct i2c_client *client = ite7258_ts->client;
	int ret = -1;

	ite7258_wait_command_done(client);
	/* Firmware Information */
	wbuffer[0] = CMD_BUF_ADDR;
	wbuffer[1] = 0x01;
	wbuffer[2] = 0x00;
	ret = ite7258_i2c_Write(client, wbuffer, 3);
	if (ret < 0)
		return -1;

	//msleep(10);
	ite7258_wait_command_done(client);

	wbuffer[0] = CMD_RESPONSE_BUF_ADDR;
	memset(rbuffer, 0xFF,  8);
	ret = ite7258_i2c_Read(client, wbuffer, 1, rbuffer, 9);
	if (ret < 0)
		return -1;

	ite7258_ts->tp_firmware_algo_ver = (rbuffer[5] << 24) |
		(rbuffer[6] << 16) | (rbuffer[7] << 8) | rbuffer[8];

	printk("ITE7258 Touch Panel FW Version:%d.%d.%d.%d\tExtension ROM Version:%d.%d.%d.%d\n",\
			rbuffer[1], rbuffer[2], rbuffer[3], rbuffer[4], \
			rbuffer[5], rbuffer[6], rbuffer[7], rbuffer[8]);

	ite7258_wait_command_done(client);
	/* Configuration Version */
	wbuffer[0] = CMD_BUF_ADDR;
	wbuffer[1] = 0x01;
	wbuffer[2] = 0x06;
	ret = ite7258_i2c_Write(client, wbuffer, 3);
	if (ret < 0)
		return -1;
	//msleep(10);
	ite7258_wait_command_done(client);
	memset(rbuffer, 0xFF,  8);
	wbuffer[0] = CMD_RESPONSE_BUF_ADDR;
	ret = ite7258_i2c_Read(client, wbuffer, 1, rbuffer, 7);
	if (ret < 0)
		return -1;

	ite7258_ts->tp_firmware_cfg_ver = (rbuffer[1] << 24) |
		(rbuffer[2] << 16) | (rbuffer[3] << 8) | rbuffer[4];
	printk("ITE7258 Touch Panel Configuration Version:%x.%x.%x.%x\n",
			rbuffer[1], rbuffer[2], rbuffer[3], rbuffer[4]);

	/* IRQ status */
	ite7258_wait_command_done(client);
	wbuffer[0] = CMD_BUF_ADDR;
	wbuffer[1] = 0x01;
	wbuffer[2] = 0x04;
	ret = ite7258_i2c_Write(client, wbuffer, 3);
	if (ret < 0)
		return -1;
	//msleep(10);
	ite7258_wait_command_done(client);

	memset(rbuffer, 0xFF,  8);
	wbuffer[0] = CMD_RESPONSE_BUF_ADDR;
	ret = ite7258_i2c_Read(client, wbuffer, 1, rbuffer, 2);
	printk("ITE7258 Touch irq %x, %x \n",rbuffer[0], rbuffer[1]);


	/* vendor ID && Device ID */
	ite7258_wait_command_done(client);
	wbuffer[0] = CMD_BUF_ADDR;
	wbuffer[1] = 0x0;
	wbuffer[2] = 0x0;
	ret = ite7258_i2c_Write(client, wbuffer, 2);
	if (ret < 0)
		return -1;
	//msleep(10);
	ite7258_wait_command_done(client);
	wbuffer[0] = CMD_RESPONSE_BUF_ADDR;
	memset(rbuffer, 0xFF,  8);
	ret = ite7258_i2c_Read(client, wbuffer, 1, rbuffer, 8);

	/*	if(strncmp(rbuffer, "ITE7260", 7)){
		return -1;
		}
		*/
	if(rbuffer[1] != 0x49 && rbuffer[2] != 0x54 && rbuffer[3] != 0x45) {
		return -1;
	}

	printk("ITE7258 Touch Panel Firmware Version %c%c%c%c%c%c%c\n",
			rbuffer[1], rbuffer[2], rbuffer[3], rbuffer[4],
			rbuffer[5], rbuffer[6], rbuffer[7]);
#endif
	return 0;

}

static int ite7258_get_version(struct ite7258_ts_data *ite7258_ts)
{
	unsigned char wbuffer[9];
	unsigned char rbuffer[9];
	struct i2c_client *client = ite7258_ts->client;
	int ret = -1;

	ite7258_wait_command_done(client);
	/* Firmware Information */
	wbuffer[0] = CMD_BUF_ADDR;
	wbuffer[1] = 0x01;
	wbuffer[2] = 0x00;
	ret = ite7258_i2c_Write(client, wbuffer, 3);
	if (ret < 0)
		return -1;

	ite7258_wait_command_done(client);

	wbuffer[0] = CMD_RESPONSE_BUF_ADDR;
	memset(rbuffer, 0xFF,  8);
	ret = ite7258_i2c_Read(client, wbuffer, 1, rbuffer, 9);
	if (ret < 0)
		return -1;

	switch (rbuffer[6]) {
		case 17:
			if (rbuffer[5] == 5)
				ite7258_ts->type = TP_ITE7258_FLAT;
			break;
		case 16:
			if (rbuffer[5] == 5)
				ite7258_ts->type = TP_ITE7258_CIRCLE;
			break;
		case 15:
			if (rbuffer[5] == 5)
				ite7258_ts->type = TP_ITE7262_CIRCLE;
			break;
		case 13:
			if (rbuffer[5] == 5) {
				ite7258_ts->type = TP_ITE7258_SQUARE;
			} else if (rbuffer[5] == 3) {
				ite7258_ts->type = TP_ITE7252_SQUARE;
			}
			break;
		default:
			break;
	}

	ite7258_ts->tp_firmware_algo_ver = (rbuffer[5] << 24) |
		(rbuffer[6] << 16) | (rbuffer[7] << 8) | rbuffer[8];
	printk(KERN_DEBUG "ITE7258 Touch Panel FW Version:%d.%d.%d.%d\tExtension ROM Version:%d.%d.%d.%d\n",\
			rbuffer[1], rbuffer[2], rbuffer[3], rbuffer[4], \
			rbuffer[5], rbuffer[6], rbuffer[7], rbuffer[8]);

	ite7258_wait_command_done(client);
	/* Configuration Version */
	wbuffer[0] = CMD_BUF_ADDR;
	wbuffer[1] = 0x01;
	wbuffer[2] = 0x06;
	ret = ite7258_i2c_Write(client, wbuffer, 3);
	if (ret < 0)
		return -1;

	ite7258_wait_command_done(client);
	memset(rbuffer, 0xFF,  8);
	wbuffer[0] = CMD_RESPONSE_BUF_ADDR;
	ret = ite7258_i2c_Read(client, wbuffer, 1, rbuffer, 7);
	if (ret < 0)
		return -1;

	ite7258_ts->tp_firmware_cfg_ver = (rbuffer[1] << 24) |
		(rbuffer[2] << 16) | (rbuffer[3] << 8) | rbuffer[4];
	printk(KERN_DEBUG "ITE7258 Touch Panel Configuration Version:%x.%x.%x.%x\n",
			rbuffer[1], rbuffer[2], rbuffer[3], rbuffer[4]);
	return 0;

}

static int ite7258_get_resolution(struct ite7258_ts_data *ite7258_ts)
{
	unsigned char wbuffer[11];
	unsigned char rbuffer[11];
	struct i2c_client *client = ite7258_ts->client;
	int ret = -1;

	ite7258_wait_command_done(client);
	/* Firmware Information */
	wbuffer[0] = CMD_BUF_ADDR;
	wbuffer[1] = 0x01;
	wbuffer[2] = 0x02;
	wbuffer[3] = 0x00;
	ret = ite7258_i2c_Write(client, wbuffer, 4);
	if (ret < 0)
		return -1;

	ite7258_wait_command_done(client);
	wbuffer[0] = CMD_RESPONSE_BUF_ADDR;
	memset(rbuffer, 0xFF,  11);
	ret = ite7258_i2c_Read(client, wbuffer, 1, rbuffer, 11);
	if (ret < 0)
		return -1;

	ite7258_ts->x_max = rbuffer[2] + (rbuffer[3]<<8);
	ite7258_ts->y_max = rbuffer[4] + (rbuffer[5]<<8);
	pr_debug("ite7258_ts->x_max = %d\n", ite7258_ts->x_max);
	pr_debug("ite7258_ts->y_max = %d\n", ite7258_ts->y_max);

	return 0;
}

static void ite7258_ts_do_suspend(struct ite7258_ts_data *ts)
{
	if (ts->is_suspend == 0) {
		mutex_lock(&ts->lock);
		disable_irq(ts->irq);
		ts->is_suspend = 1;
		ite7258_enter_sleep_mode(ts);
		mutex_unlock(&ts->lock);
		dev_dbg(&ts->client->dev, "[FTS]ite7258 suspend\n");
	}
}

static void ite7258_ts_do_resume(struct ite7258_ts_data *ts)
{
	if (ts->is_suspend) {
		mutex_lock(&ts->lock);
		ite7258_exit_sleep_mode(ts);
		dev_dbg(&ts->client->dev, "[FTS]ite7258 resume.\n");
		gpio_direction_output(ts->rst, 1);
		msleep(10);
		gpio_direction_output(ts->rst, 0);
		if (ts->rst_level) {
			msleep(10);
			gpio_direction_output(ts->rst, ts->rst_level);
		}
		msleep(10);
		gpio_direction_input(ts->client->irq);
		ts->is_suspend = 0;
		enable_irq(ts->irq);
		mutex_unlock(&ts->lock);
		dev_dbg(&ts->client->dev, "[FTS]ite7258 resume.\n");
	}
}

#if defined(CONFIG_PM)
static int ite7258_ts_suspend(struct device *dev)
{
	struct ite7258_ts_data *ts = dev_get_drvdata(dev);
	ite7258_ts_do_suspend(ts);
	return 0;
}

static int ite7258_ts_resume(struct device *dev)
{
	struct ite7258_ts_data *ts = dev_get_drvdata(dev);
	ite7258_ts_do_resume(ts);
	return 0;
}
#endif

#if 0
static void ite7258_interrupt_mode(struct i2c_client *client)
{
	unsigned char buf[12];
	unsigned char tmp[2];
	int ret;
	do{
		tmp[0] = QUERY_BUF_ADDR;
		buf[0] = 0xFF;
		ite7258_i2c_Read(client, tmp, 1, buf, 1);
	}while( buf[0] & 0x01 );

	buf[0] = CMD_BUF_ADDR;
	buf[1] = 0x02;
	buf[2] = 0x04;
	buf[3] = 0x01; //enable interrupt
	buf[4] = 0x11; //Falling edge trigger
	ret = ite7258_i2c_Write(client, buf, 5);
	if (ret < 0)
		printk("ite7258_interrupt_mode i2c write error\n");
	do{
		tmp[0] = QUERY_BUF_ADDR;
		buf[0] = 0xFF;
		ite7258_i2c_Read(client, tmp, 1, buf, 1);
	}while( buf[0] & 0x01 );
	buf[0] = CMD_RESPONSE_BUF_ADDR;
	ite7258_i2c_Read(client, buf, 1, buf, 2);
	printk("DDD_____ 0xA0 : %X, %X\n", buf[0], buf[1]);
}

static void ite7258_hw_init(struct i2c_client *client)
{
	ite7258_interrupt_mode(client);
}
#endif



static int ite7258_gpio_request(struct ite7258_ts_data *ts)
{
	int err = 0;

	err = gpio_request(ts->rst, "ite7258 reset");
	if (err < 0) {
		printk("touch: %s failed to set gpio reset.\n",
				__func__);
		return err;
	}

	err = gpio_request(ts->irq,"ite7258 irq");
	if (err < 0) {
		printk("touch: %s failed to set gpio irq.\n",
				__func__);
		gpio_free(ts->rst);
		return err;
	}
	gpio_direction_input(ts->irq);

	gpio_direction_output(ts->rst, 1);
	msleep(10);
	gpio_direction_output(ts->rst, 0);
	msleep(10);
	if (ts->rst_level) {
		gpio_direction_output(ts->rst, ts->rst_level);
		msleep(10);
	}

	return 0;
}

static int ite7258_regulator_get(struct ite7258_ts_data *ite7258_ts)
{
	int err = 0;

	ite7258_ts->vcc_reg = regulator_get(NULL, ite7258_ts->vcc_name);
	if (IS_ERR(ite7258_ts->vcc_reg)) {
		printk("failed to get VCC regulator.");
		err = PTR_ERR(ite7258_ts->vcc_reg);
		return -EINVAL;
	}

	regulator_enable(ite7258_ts->vcc_reg);

	if (ite7258_ts->vio_name) {
		ite7258_ts->vio_reg = regulator_get(NULL, ite7258_ts->vio_name);
		if (IS_ERR(ite7258_ts->vio_reg)) {
			printk("failed to get VIO regulator.");
			err = PTR_ERR(ite7258_ts->vio_reg);
			regulator_disable(ite7258_ts->vcc_reg);
			regulator_put(ite7258_ts->vcc_reg);
			return -EINVAL;
		}
		regulator_enable(ite7258_ts->vio_reg);
	}

	return 0;
}

static void ite7258_input_set(struct input_dev *input_dev, struct ite7258_ts_data *ts)
{
#ifdef CONFIG_ITE7258_MULTITOUCH
	set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_PRESSURE, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, ts->x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, ts->y_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE,   0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, PRESS_MAX, 0, 0);
#else
	set_bit(ABS_X, input_dev->absbit);
	set_bit(ABS_Y, input_dev->absbit);

	//set_bit(ABS_PRESSURE, input_dev->absbit);
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);

	input_set_abs_params(input_dev, ABS_X, 0, ts->x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, ts->y_max, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, 0xFF, 0, 0);
#endif
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	input_dev->name = ITE7258_NAME;
	input_dev->id.bustype = BUS_I2C;
	input_dev->id.vendor = 0xDEAD;
	input_dev->id.product = 0xBEEF;
	input_dev->id.version = 10427;

}

static int tp_notifier_callback(struct notifier_block *self,unsigned long event, void *data)
{
	struct ite7258_ts_data *ite7258_ts;
	struct device *dev;
	struct fb_event *evdata = data;
	int mode;

	/* If we aren't interested in this event, skip it immediately ... */
	switch (event) {
		case FB_EVENT_BLANK:
		case FB_EVENT_MODE_CHANGE:
		case FB_EVENT_MODE_CHANGE_ALL:
		case FB_EARLY_EVENT_BLANK:
		case FB_R_EARLY_EVENT_BLANK:
			break;
		default:
			return 0;
	}

	mode = *(int *)evdata->data;
	ite7258_ts = container_of(self, struct ite7258_ts_data, tp_notif);

	if(event == FB_EVENT_BLANK){
		if(mode)
			ite7258_ts_do_suspend(ite7258_ts);
		else
			ite7258_ts_do_resume(ite7258_ts);
	}
	return 0;
}

static void ite7258_register_notifier(struct ite7258_ts_data *ite7258_ts)
{
	memset(&ite7258_ts->tp_notif,0,sizeof(ite7258_ts->tp_notif));
	ite7258_ts->tp_notif.notifier_call = tp_notifier_callback;

	/* register on the fb notifier  and work with fb*/
	fb_register_client(&ite7258_ts->tp_notif);
}

static void ite7258_unregister_notifier(struct ite7258_ts_data *ite7258_ts)
{
	fb_unregister_client(&ite7258_ts->tp_notif);
}

static int ite7258_ts_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct jztsc_platform_data *pdata =
		(struct jztsc_platform_data *)client->dev.platform_data;
	struct ite7258_ts_data *ite7258_ts;
	struct input_dev *input_dev;
	int err = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}
	ite7258_ts = kzalloc(sizeof(struct ite7258_ts_data), GFP_KERNEL);
	if (!ite7258_ts) {
		dev_err(&client->dev, "failed to allocate input driver data\n");
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	update = kzalloc(sizeof(struct ite7258_update_data), GFP_KERNEL);
	if(!update){
		dev_err(&client->dev, "failed to allocate driver data\n");
		err = -ENOMEM;
		goto exit_alloc_update_failed;
	}

	i2c_set_clientdata(client, ite7258_ts);
	ite7258_ts->irq = pdata->gpio[0].num;
	ite7258_ts->rst = pdata->gpio[1].num;
	ite7258_ts->rst_level = pdata->gpio[1].enable_level;
	client->irq = ite7258_ts->irq;
	ite7258_ts->client = client;
	ite7258_ts->pdata = pdata;
	//ite7258_ts->x_max = pdata->x_max - 1;
	//ite7258_ts->y_max = pdata->y_max - 1;
	ite7258_ts->vcc_name = pdata->vcc_name;
	ite7258_ts->vio_name = pdata->vccio_name;

	err = ite7258_regulator_get(ite7258_ts);
	if(err){
		dev_err(&client->dev, "failed to get regulator\n");
		goto exit_regulator_failed;
	}
	msleep(10);

	err = ite7258_gpio_request(ite7258_ts);
	if(err){
		dev_err(&client->dev, "failed to request gpio\n");
		goto exit_gpio_failed;
	}
	if(ite7258_print_version(ite7258_ts)){
		dev_err(&client->dev, "print version failed\n");
		err = -ENOMEM;
		goto exit_get_version;
	}

	err = ite7258_get_version(ite7258_ts);
	if (err < 0) {
		dev_err(&client->dev, "print version failed\n");
		err = -ENODEV;
		goto exit_get_version;
	}

	err = ite7258_get_resolution(ite7258_ts);
	if (err < 0) {
		dev_err(&client->dev, "get resolution failed\n");
		err = -ENODEV;
		goto exit_get_version;
	}
#if defined(CONFIG_ITE7258_EXT_FUNC)

	update->client = client;
	update->algo_firmware_length = sizeof(algorithm_raw_data);
	update->algo_firmware_data = algorithm_raw_data;
	update->configurate_firmware_length = sizeof(configurate_raw_data);
	update->configurate_firmware_data = configurate_raw_data;

	err = IT7258_auto_upgrade_fw(ite7258_ts);
	if (err > 0) {
		gpio_direction_output(ite7258_ts->rst, 1);
		msleep(10);
		gpio_direction_output(ite7258_ts->rst, 0);
		msleep(10);
		if (ite7258_ts->rst_level) {
			gpio_direction_output(ite7258_ts->rst, ite7258_ts->rst_level);
			msleep(10);
		}
		msleep(10);
		if(ite7258_get_version(ite7258_ts)){
			dev_err(&client->dev, "print version failed\n");
			err = -ENOMEM;
			goto exit_get_version;
		}
	}
#endif
	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	ite7258_ts->input_dev = input_dev;
	ite7258_input_set(input_dev, ite7258_ts);

	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
				"ite7258_ts_probe: failed to register input device: %s\n",
				dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

	ite7258_register_notifier(ite7258_ts);

#ifdef CONFIG_ITE7258_SYSFS_DEBUG
	ite7258_create_sysfs(client);
#endif


	mutex_init(&ite7258_ts->lock);
	//ite7258_hw_init(client);

	INIT_WORK(&ite7258_ts->work, ite7258_work_handler);
	ite7258_ts->workqueue = create_singlethread_workqueue("ite7258_ts");
	pr_debug("%s: ts data: %p\n", __func__, ite7258_ts);

	ite7258_ts->irq = gpio_to_irq(pdata->gpio[0].num);
	err = request_irq(ite7258_ts->irq, ite7258_ts_interrupt,
			pdata->irqflags, client->dev.driver->name,
			ite7258_ts);

	if (err < 0) {
		dev_err(&client->dev, "ite7258_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}
	ite7258_exit_sleep_mode(ite7258_ts);

	return 0;


exit_irq_request_failed:
	ite7258_unregister_notifier(ite7258_ts);
	cancel_work_sync(&ite7258_ts->work);
	input_unregister_device(input_dev);

exit_input_register_device_failed:
	input_free_device(input_dev);

exit_input_dev_alloc_failed:
exit_get_version:
	gpio_free(ite7258_ts->rst);
	gpio_free(ite7258_ts->irq);

exit_gpio_failed:
	regulator_disable(ite7258_ts->vcc_reg);
	regulator_put(ite7258_ts->vcc_reg);
	if (ite7258_ts->vio_reg) {
		regulator_disable(ite7258_ts->vio_reg);
		regulator_put(ite7258_ts->vio_reg);
	}
	i2c_set_clientdata(client, NULL);

exit_regulator_failed:
	kfree(update);

exit_alloc_update_failed:
	kfree(ite7258_ts);

exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}

static int ite7258_ts_remove(struct i2c_client *client)
{
	struct ite7258_ts_data *ite7258_ts;
	ite7258_ts = i2c_get_clientdata(client);
	ite7258_unregister_notifier(ite7258_ts);
	input_unregister_device(ite7258_ts->input_dev);
	input_free_device(ite7258_ts->input_dev);
	gpio_free(ite7258_ts->rst);
	gpio_free(ite7258_ts->irq);

#ifdef CONFIG_ITE7258_SYSFS_DEBUG
	ite7258_release_sysfs(client);
#endif

	free_irq(ite7258_ts->irq, ite7258_ts);
	if (!IS_ERR(ite7258_ts->vcc_reg)) {
		regulator_disable(ite7258_ts->vcc_reg);
		regulator_put(ite7258_ts->vcc_reg);
	}

	if (ite7258_ts->vio_reg) {
		regulator_disable(ite7258_ts->vio_reg);
		regulator_put(ite7258_ts->vio_reg);
	}
	kfree(update);
	kfree(ite7258_ts);

	i2c_set_clientdata(client, NULL);
	return 0;
}

#if defined(CONFIG_PM)
static const struct dev_pm_ops ite7258_ts_pm_ops = {
	.suspend        = ite7258_ts_suspend,
	.resume		= ite7258_ts_resume,
};
#endif

static const struct i2c_device_id ite7258_ts_id[] = {
	{ITE7258_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, ite7258_ts_id);

static struct i2c_driver ite7258_ts_driver = {
	.probe = ite7258_ts_probe,
	.remove = ite7258_ts_remove,
	.id_table = ite7258_ts_id,
	.driver = {
		.name = ITE7258_NAME,
		.owner = THIS_MODULE,
#if defined(CONFIG_PM)
		.pm	= &ite7258_ts_pm_ops,
#endif
	},
};

static int __init ite7258_ts_init(void)
{
	int ret;
	ret = i2c_add_driver(&ite7258_ts_driver);
	return ret;
}

static void __exit ite7258_ts_exit(void)
{
	i2c_del_driver(&ite7258_ts_driver);
}

module_init(ite7258_ts_init);
module_exit(ite7258_ts_exit);

MODULE_AUTHOR("<Rejion>");
MODULE_DESCRIPTION("FocalTech ite7258 TouchScreen driver");
MODULE_LICENSE("GPL");
