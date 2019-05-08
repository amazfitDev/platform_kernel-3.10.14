/*
 * drivers/input/touchscreen/focaltech.c
 *
 * FocalTech focaltech TouchScreen driver.
 *
 * Copyright (c) 2014  Focal tech Ltd.
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
 * VERSION      	DATE			AUTHOR
 *    1.1		  2014-09			mshl
 *
 */

#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/input/mt.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/hwmon-sysfs.h>

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/err.h>
#include <linux/suspend.h>

#include <linux/tsc.h>
#include <linux/i2c/focaltech.h>
#include <linux/i2c/focaltech_ex_fun.h>
#include <linux/i2c/focaltech_ctl.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

//#define FTS_DBG
#ifdef FTS_DBG
#define ENTER printk(KERN_INFO "[FTS_DBG] func: %s  line: %04d\n", __func__, __LINE__);
#define PRINT_DBG(x...)  printk(KERN_INFO "[FTS_DBG] " x)
#define PRINT_INFO(x...)  printk(KERN_INFO "[FTS_INFO] " x)
#define PRINT_WARN(x...)  printk(KERN_INFO "[FTS_WARN] " x)
#define PRINT_ERR(format,x...)  printk(KERN_ERR "[FTS_ERR] func: %s  line: %04d  info: " format, __func__, __LINE__, ## x)
#else
#define ENTER
#define PRINT_DBG(x...)
#define PRINT_INFO(x...)  printk(KERN_INFO "[FTS_INFO] " x)
#define PRINT_WARN(x...)  printk(KERN_INFO "[FTS_WARN] " x)
#define PRINT_ERR(format,x...)  printk(KERN_ERR "[FTS_ERR] func: %s  line: %04d  info: " format, __func__, __LINE__, ## x)
#endif

//#define CONFIG_FOCALTECH_GESTRUE //no test for use
#define	USE_WAIT_QUEUE						0
#define	USE_THREADED_IRQ					0
#define	USE_WORK_QUEUE						1

//#define	TOUCH_VIRTUAL_KEYS
#define	MULTI_PROTOCOL_TYPE_B				0
#define	TS_MAX_FINGER						5

struct ts_event {
	u16 x1;
	u16 y1;
	u16 x2;
	u16 y2;
	u16 x3;
	u16 y3;
	u16 x4;
	u16 y4;
	u16 x5;
	u16 y5;
	u16 pressure;
	u8  touch_point;
};

struct fts_data {
	struct input_dev	*input_dev;
	struct i2c_client	*client;
	struct ts_event		event;
	struct mutex		lock;
	int is_suspend;
	int gestrue_state;	//=0 disable gestrue wakeup system
						//=1 enable  gestrue wakeup system
#if USE_WORK_QUEUE
	struct work_struct	pen_event_work;
	struct workqueue_struct	*fts_workqueue;
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct work_struct		resume_work;
	struct workqueue_struct	*fts_resume_workqueue;
	struct early_suspend	early_suspend;
#endif

	struct fts_platform_data	*platform_data;
	struct regulator *vcc_reg;
};

struct Upgrade_Info fts_updateinfo_curr;
struct Upgrade_Info fts_updateinfo[] =
{
	{0x0e,"FT3x0x", TPD_MAX_POINTS_2, AUTO_CLB_NONEED, 10, 10, 0x79, 0x18, 10,  2000},
	{0x55,"FT5x06", TPD_MAX_POINTS_5, AUTO_CLB_NEED,   50, 30, 0x79, 0x03, 10,  2000},
	{0x08,"FT5606", TPD_MAX_POINTS_5, AUTO_CLB_NEED,   50, 10, 0x79, 0x06, 100, 2000},
	{0x0a,"FT5x16", TPD_MAX_POINTS_5, AUTO_CLB_NEED,   50, 30, 0x79, 0x07, 10,  1500},
	{0x06,"FT6x06", TPD_MAX_POINTS_2, AUTO_CLB_NONEED, 100,30, 0x79, 0x08, 10,  2000},
	{0x36,"FT6x36", TPD_MAX_POINTS_2, AUTO_CLB_NONEED, 10, 10, 0x79, 0x18, 10,  2000},
	{0x55,"FT5x06i",TPD_MAX_POINTS_5, AUTO_CLB_NEED,   50, 30, 0x79, 0x03, 10,  2000},
	{0x14,"FT5336", TPD_MAX_POINTS_5, AUTO_CLB_NONEED, 30, 30, 0x79, 0x11, 10,  2000},
	{0x13,"FT3316", TPD_MAX_POINTS_5, AUTO_CLB_NONEED, 30, 30, 0x79, 0x11, 10,  2000},
	{0x12,"FT5436i",TPD_MAX_POINTS_5, AUTO_CLB_NONEED, 30, 30, 0x79, 0x11, 10,  2000},
	{0x11,"FT5336i",TPD_MAX_POINTS_5, AUTO_CLB_NONEED, 30, 30, 0x79, 0x11, 10,  2000},
	{0x54,"FT5x46", TPD_MAX_POINTS_5, AUTO_CLB_NONEED, 2,  2,  0x54, 0x2c, 10,  2000},

};

#if USE_WAIT_QUEUE
static struct task_struct *thread = NULL;
static DECLARE_WAIT_QUEUE_HEAD(waiter);
static int tpd_flag = 0;
#endif

#ifdef CONFIG_FOCALTECH_GESTRUE
#include "ft_gesture_lib.h"
#define GESTURE_LEFT                     0x20
#define GESTURE_RIGHT                    0x21
#define GESTURE_UP                       0x22
#define GESTURE_DOWN                     0x23
#define GESTURE_DOUBLECLICK              0x24
#define GESTURE_O                        0x30
#define GESTURE_W                        0x31
#define GESTURE_M                        0x32
#define GESTURE_E                        0x33
#define GESTURE_C                        0x34
#define FTS_GESTRUE_POINTS               255
#define FTS_GESTRUE_POINTS_ONETIME       62
#define FTS_GESTRUE_POINTS_HEADER        8
#define FTS_GESTURE_OUTPUT_ADRESS        0xD3
#define FTS_GESTURE_OUTPUT_UNIT_LENGTH   4

unsigned short coordinate_x[150] = {0};
unsigned short coordinate_y[150] = {0};

extern int fetch_object_sample(unsigned char *buf,short pointnum);
extern void init_para(int x_pixel,int y_pixel,int time_slot,int cut_x_pixel,int cut_y_pixel);
suspend_state_t get_suspend_state(void);
#endif

#if defined(READ_FW_VER)
static unsigned char fts_read_fw_ver(struct fts_data *ts);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void fts_ts_suspend(struct early_suspend *handler);
static void fts_ts_resume(struct early_suspend *handler);
#endif


#ifdef TOUCH_VIRTUAL_KEYS
static ssize_t virtual_keys_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct fts_data *data = dev_get_drvdata(dev);
	struct fts_platform_data *pdata = data->platform_data;
	return sprintf(buf,"%s:%s:%d:%d:%d:%d:%s:%s:%d:%d:%d:%d:%s:%s:%d:%d:%d:%d\n"
		,__stringify(EV_KEY), __stringify(KEY_MENU),pdata ->virtualkeys[0],pdata ->virtualkeys[1],pdata ->virtualkeys[2],pdata ->virtualkeys[3]
		,__stringify(EV_KEY), __stringify(KEY_HOMEPAGE),pdata ->virtualkeys[4],pdata ->virtualkeys[5],pdata ->virtualkeys[6],pdata ->virtualkeys[7]
		,__stringify(EV_KEY), __stringify(KEY_BACK),pdata ->virtualkeys[8],pdata ->virtualkeys[9],pdata ->virtualkeys[10],pdata ->virtualkeys[11]);
}

static SENSOR_DEVICE_ATTR(virtual_keys, S_IRUGO | S_IWUGO | S_IXUGO,
		virtual_keys_show, NULL, 0);

static struct attribute *properties_attrs[] = {
	&sensor_dev_attr_virtual_keys.dev_attr.attr,
	NULL
};

static struct attribute_group properties_attr_group = {
	.attrs = properties_attrs,
};

static void fts_virtual_keys_init(void)
{
	int ret = 0;
	struct kobject *properties_kobj;

	pr_info("[FST] %s\n",__func__);

	properties_kobj = kobject_create_and_add("focaltech_virtual_keys", NULL);
	if (properties_kobj)
		ret = sysfs_create_group(properties_kobj,  &properties_attr_group);

	if (!properties_kobj || ret)
		pr_err("failed to create board_properties\n");
}

#endif

/***********************************************************************************************
Name		: fts_read_fw_ver
Input		: struct fts_data *ts
Output		: firmware version
function	: read TP firmware version
***********************************************************************************************/
#if defined(READ_FW_VER)
static unsigned char fts_read_fw_ver(struct fts_data *ts)
{
	unsigned char ver;
	fts_read_reg(ts->client, FTS_REG_FIRMID, &ver);
	return(ver);
}
#endif

static void fts_clear_report_data(struct fts_data *ts)
{
	int i;
	input_report_key(ts->input_dev, BTN_TOUCH, 0);

#if MULTI_PROTOCOL_TYPE_B
	for(i = 0; i < TS_MAX_FINGER; i++) {
		input_mt_slot(ts->input_dev, i);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);

	}
#else
	input_mt_sync(ts->input_dev);
#endif
	input_sync(ts->input_dev);
}

static int fts_update_data(struct fts_data *ts)
{
	struct fts_data *data = ts;
	struct ts_event *event = &data->event;
	u8 buf[33] = {0};
	int ret = -1;
	int i;
	u16 x , y;
	u8 ft_pressure , ft_size;

	ret = fts_i2c_Read(data->client, buf, 1, buf, 33);

	if (ret < 0) {
		pr_err("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return ret;
	}

	memset(event, 0, sizeof(struct ts_event));
	event->touch_point = buf[2] & 0x07;

	for(i = 0; i < TS_MAX_FINGER; i++) {
		if((buf[6*i+3] & 0xc0) == 0xc0)
			continue;
		x = (s16)(buf[6*i+3] & 0x0F)<<8 | (s16)buf[6*i+4];
		y = (s16)(buf[6*i+5] & 0x0F)<<8 | (s16)buf[6*i+6];

#ifdef CONFIG_TSC_SWAP_XY
		tsc_swap_xy(&x,&y);
#endif

#ifdef CONFIG_TSC_SWAP_X
		tsc_swap_x(&x,data->platform_data->TP_MAX_X);
#endif

#ifdef CONFIG_TSC_SWAP_Y
		tsc_swap_y(&y,data->platform_data->TP_MAX_Y);
#endif
		ft_pressure = buf[6*i+7];

		if (ft_pressure > 127 || ft_pressure == 0)
			ft_pressure = 127;

		ft_size = (buf[6*i+8]>>4) & 0x0F;
		if (ft_size == 0) {
			ft_size = 0x09;
		}
		if ((buf[6*i+3] & 0x40) == 0x0) {
		#if MULTI_PROTOCOL_TYPE_B
			input_mt_slot(data->input_dev, buf[6*i+5]>>4);
			input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, true);
		#else
			input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, buf[6*i+5]>>4);
		#endif
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, y);
			input_report_abs(data->input_dev, ABS_MT_PRESSURE, ft_pressure);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, ft_size);
			input_report_key(data->input_dev, BTN_TOUCH, 1);

		#if !MULTI_PROTOCOL_TYPE_B
			input_mt_sync(data->input_dev);
		#endif
			pr_debug("===x%d = %d,y%d = %d ====",i, x, i, y);
		}
		else {
		#if MULTI_PROTOCOL_TYPE_B
			input_mt_slot(data->input_dev, buf[6*i+5]>>4);
			input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, false);
		#endif
		}
	}
	if (0 == event->touch_point) {
		input_report_key(data->input_dev, BTN_TOUCH, 0);

		#if MULTI_PROTOCOL_TYPE_B
		for(i = 0; i < TS_MAX_FINGER; i ++) {

			input_mt_slot(data->input_dev, i);
			input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, false);
		}
		#else
		input_mt_sync(data->input_dev);
		#endif
	}
	input_sync(data->input_dev);

	return 0;
}

static void fts_reset(struct fts_data *ts)
{
	struct fts_platform_data *pdata = ts->platform_data;

	gpio_direction_output(pdata->reset_gpio_number, 1);
	msleep(10);
	gpio_set_value(pdata->reset_gpio_number, 0);
	msleep(10);
	gpio_set_value(pdata->reset_gpio_number, 1);
	msleep(200);
}

#ifdef CONFIG_FOCALTECH_GESTRUE

static ssize_t gesture_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct fts_data *ts = dev_get_drvdata(dev);
	int state = ts->gestrue_state;

	if(state > 0) {
		snprintf(buf, PAGE_SIZE, "Enable\n");
	} else {
		snprintf(buf, PAGE_SIZE, "Disable\n");
	}

	return strlen(buf) + 1;
}


static ssize_t gesture_status_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	int val = 0;
	struct fts_data *ts = dev_get_drvdata(dev);

	ret = strict_strtoul(buf, 10, &val);
	if (ret)
		return ret;

	mutex_lock(&ts->lock);
	if(val > 0) {
		/*Enable gesture*/

		if(ts->gestrue_state == 0 && ts->is_suspend == 1) {
			regulator_enable(ts->vcc_reg);
			msleep(100);
			enable_irq(ts->client->irq);
			fts_reset(ts);
			ret = fts_write_reg(ts->client, 0xd0, 0x01);
			if(ret < 0){
				PRINT_ERR("==gesture_status_store==  fts_write_reg fail ret = %d\n", ret);
			}

			if (fts_updateinfo_curr.CHIP_ID==0x54) {
				fts_write_reg(ts->client, 0xd1, 0xff);
				fts_write_reg(ts->client, 0xd2, 0xff);
				fts_write_reg(ts->client, 0xd5, 0xff);
				fts_write_reg(ts->client, 0xd6, 0xff);
				fts_write_reg(ts->client, 0xd7, 0xff);
				fts_write_reg(ts->client, 0xd8, 0xff);
			}
		}
		ts->gestrue_state = 1;
	} else {
		/*Disable gesture*/
		if(ts->gestrue_state == 1 && ts->is_suspend == 1) {
			ret = fts_write_reg(ts->client, FTS_REG_PMODE, PMODE_HIBERNATE);
			if(ret < 0){
				PRINT_ERR("==fts_suspend==  fts_write_reg fail ret= %d\n", ret);
			}
			disable_irq(ts->client->irq);
			regulator_force_disable(ts->vcc_reg);
		}
		ts->gestrue_state = 0;
	}

	mutex_unlock(&ts->lock);
	return count;
}

/*
*/
static DEVICE_ATTR(gesture_status, S_IRUGO | S_IWUSR | S_IWGRP | S_IWOTH, gesture_status_show,
		gesture_status_store);

/*add your attr in here*/
static struct attribute *fts_gesture_attributes[] = {
	&dev_attr_gesture_status.attr,
	NULL
};

static struct attribute_group fts_gesture_attribute_group = {
	.attrs = fts_gesture_attributes
};

/*create sysfs for gesture*/
int fts_create_gesture_sysfs(struct i2c_client *client)
{
	int err;
	err = sysfs_create_group(&client->dev.kobj, &fts_gesture_attribute_group);
	if (0 != err) {
		dev_err(&client->dev,
					 "%s() - ERROR: sysfs_create_group() failed.\n",
					 __func__);
		sysfs_remove_group(&client->dev.kobj, &fts_gesture_attribute_group);
		return -EIO;
	}

	return err;
}


static void check_gesture(struct fts_data *ts, int gesture_id)
{
	struct fts_data *data = ts;
	pr_info("kaka gesture_id==0x%x\n ",gesture_id);
	switch(gesture_id)
	{
		case GESTURE_LEFT:
			input_report_key(data->input_dev, KEY_LEFT, 1);
			input_sync(data->input_dev);
			input_report_key(data->input_dev, KEY_LEFT, 0);
			input_sync(data->input_dev);
			break;
		case GESTURE_RIGHT:
			input_report_key(data->input_dev, KEY_RIGHT, 1);
			input_sync(data->input_dev);
			input_report_key(data->input_dev, KEY_RIGHT, 0);
			input_sync(data->input_dev);
			break;
		case GESTURE_UP:
			input_report_key(data->input_dev, KEY_UP, 1);
			input_sync(data->input_dev);
			input_report_key(data->input_dev, KEY_UP, 0);
			input_sync(data->input_dev);
			break;
		case GESTURE_DOWN:
			input_report_key(data->input_dev, KEY_DOWN, 1);
			input_sync(data->input_dev);
			input_report_key(data->input_dev, KEY_DOWN, 0);
			input_sync(data->input_dev);
			break;
		case GESTURE_DOUBLECLICK:
			input_report_key(data->input_dev, KEY_POWER, 1);
			input_sync(data->input_dev);
			input_report_key(data->input_dev, KEY_POWER, 0);
			input_sync(data->input_dev);
			break;
		case GESTURE_O:
			input_report_key(data->input_dev, KEY_O, 1);
			input_sync(data->input_dev);
			input_report_key(data->input_dev, KEY_O, 0);
			input_sync(data->input_dev);
			break;
		case GESTURE_W:
			input_report_key(data->input_dev, KEY_W, 1);
			input_sync(data->input_dev);
			input_report_key(data->input_dev, KEY_W, 0);
			input_sync(data->input_dev);
			break;
		case GESTURE_M:
			input_report_key(data->input_dev, KEY_M, 1);
			input_sync(data->input_dev);
			input_report_key(data->input_dev, KEY_M, 0);
			input_sync(data->input_dev);
			break;
		case GESTURE_E:
			input_report_key(data->input_dev, KEY_E, 1);
			input_sync(data->input_dev);
			input_report_key(data->input_dev, KEY_E, 0);
			input_sync(data->input_dev);
			break;
		case GESTURE_C:
			input_report_key(data->input_dev, KEY_C, 1);
			input_sync(data->input_dev);
			input_report_key(data->input_dev, KEY_C, 0);
			input_sync(data->input_dev);
			break;
		default:
			break;
	}
}

static int fts_read_Gestruedata(struct fts_data *ts)
{
	unsigned char buf[FTS_GESTRUE_POINTS * 3] = { 0 };
	int ret = -1;
	buf[0] = 0xd3;

	ret = fts_i2c_Read(ts->client, buf, 1, buf, FTS_GESTRUE_POINTS_HEADER);
	if (ret < 0) {
		printk( "%s read touchdata failed.\n", __func__);
		return ret;
	}

	if (0x24 == buf[0]) {
		check_gesture(ts, buf[0]);
	}

	return 0;
}
#endif

#if USE_WAIT_QUEUE
static int touch_event_handler(void *unused)
{
	struct fts_data *ts = (struct fts_data *)unused;
	struct sched_param param = { .sched_priority = 5 };
	u8 state;
	sched_setscheduler(current, SCHED_RR, &param);
	do {
		set_current_state(TASK_INTERRUPTIBLE);
		wait_event_interruptible(waiter, (0 != tpd_flag));
		tpd_flag = 0;
		set_current_state(TASK_RUNNING);
#ifdef CONFIG_FOCALTECH_GESTRUE
		i2c_smbus_read_i2c_block_data(ts->client, 0xd0, 1, &state);
		if ( state ==1) {
			fts_read_Gestruedata(ts);
			continue;
		}
#endif
		fts_update_data(ts);

	} while (!kthread_should_stop());

	return 0;
}
#elif USE_WORK_QUEUE
static void fts_pen_irq_work(struct work_struct *work)
{
	int state = 0;
    struct fts_data *ts = container_of(work, struct fts_data, pen_event_work);
#ifdef CONFIG_FOCALTECH_GESTRUE
	i2c_smbus_read_i2c_block_data(ts->client, 0xd0, 1, &state);
	if( state ==1 ) {
		fts_read_Gestruedata(ts);
	}
#endif
	fts_update_data(ts);
	enable_irq(ts->client->irq);
}
#endif

static irqreturn_t fts_interrupt(int irq, void *dev_id)
{
	struct fts_data *ts = (struct fts_data *)dev_id;

#if USE_WAIT_QUEUE
	tpd_flag = 1;
	wake_up_interruptible(&waiter);
	return IRQ_HANDLED;
#elif USE_THREADED_IRQ
	fts_update_data(ts);
	return IRQ_HANDLED;
#elif USE_WORK_QUEUE
	disable_irq_nosync(ts->client->irq);
	if (!work_pending(&ts->pen_event_work)) {
		queue_work(ts->fts_workqueue, &ts->pen_event_work);
	}
	return IRQ_HANDLED;
#endif
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void fts_ts_suspend(struct early_suspend *handler)
{
	int ret = -1;
	struct fts_data *ts = container_of(handler, struct fts_data, early_suspend);

	mutex_lock(&ts->lock);
	ts->is_suspend = 1;
	fts_reset(ts);
	fts_write_reg(ts->client, FTS_REG_PMODE, 0x00);
	fts_read_reg(ts->client, FTS_REG_PMODE, &ret);
	printk(KERN_DEBUG "==%s== read FTS_REG_PMODE[0x%0X] = 0x%X gestrue_state = %d\n", __FUNCTION__, FTS_REG_PMODE, ret, ts->gestrue_state);

	if(ts->gestrue_state) {
		ret = fts_write_reg(ts->client, 0xd0, 0x01);
		if(ret < 0){
			PRINT_ERR("==fts_suspend FTS_GESTRUE==  fts_write_reg fail ret = %d\n", ret);
		}

		if (fts_updateinfo_curr.CHIP_ID==0x54) {
			fts_write_reg(ts->client, 0xd1, 0xff);
			fts_write_reg(ts->client, 0xd2, 0xff);
			fts_write_reg(ts->client, 0xd5, 0xff);
			fts_write_reg(ts->client, 0xd6, 0xff);
			fts_write_reg(ts->client, 0xd7, 0xff);
			fts_write_reg(ts->client, 0xd8, 0xff);
		}
	} else {
		ret = fts_write_reg(ts->client, FTS_REG_PMODE, PMODE_HIBERNATE);
		if(ret < 0){
			PRINT_ERR("==fts_suspend==  fts_write_reg fail ret= %d\n", ret);
		}

		disable_irq(ts->client->irq);
		fts_clear_report_data(ts);
		regulator_force_disable(ts->vcc_reg);
	}
	mutex_unlock(&ts->lock);
	return;
}

static void fts_ts_resume(struct early_suspend *handler)
{
	struct fts_data *ts = container_of(handler, struct fts_data, early_suspend);
	printk(KERN_DEBUG "==%s==\n", __FUNCTION__);

	mutex_lock(&ts->lock);
	ts->is_suspend = 0;
	if(ts->gestrue_state) {
		fts_write_reg(ts->client,0xD0,0x00);

	} else {
		regulator_enable(ts->vcc_reg);
		msleep(100);
		enable_irq(ts->client->irq);
	}


	fts_reset(ts);
	fts_clear_report_data(ts);
	mutex_unlock(&ts->lock);
}
#endif

static void fts_hw_init(struct fts_data *ts)
{
	struct i2c_client *client = ts->client;
	struct fts_platform_data *pdata = ts->platform_data;

	pr_info("[FST] %s [irq=%d];[rst=%d]\n",__func__,
		pdata->irq_gpio_number,pdata->reset_gpio_number);

	gpio_request(pdata->irq_gpio_number,   "ts_irq_pin");
	gpio_request(pdata->reset_gpio_number, "ts_rst_pin");
	gpio_direction_output(pdata->reset_gpio_number, 1);
	gpio_direction_input(pdata->irq_gpio_number);

	ts->vcc_reg = regulator_get(&client->dev, pdata->vdd_name);
	if (!WARN(IS_ERR(ts->vcc_reg), "[FST] fts_hw_init regulator: failed to get %s.\n", pdata->vdd_name)) {
		regulator_set_voltage(ts->vcc_reg, 2800000, 2800000);
		regulator_enable(ts->vcc_reg);
	}
	msleep(100);

	fts_reset(ts);
}

void focaltech_get_upgrade_array(struct i2c_client *client)
{
	u8 chip_id;
	u32 i;

	i2c_smbus_read_i2c_block_data(client,FT_REG_CHIP_ID,1,&chip_id);
	printk("%s chip_id = %x\n", __func__, chip_id);

	for(i=0; i<sizeof(fts_updateinfo)/sizeof(struct Upgrade_Info); i++) {
		if(chip_id==fts_updateinfo[i].CHIP_ID) {
			memcpy(&fts_updateinfo_curr, &fts_updateinfo[i], sizeof(struct Upgrade_Info));
			break;
		}
	}

	if(i >= sizeof(fts_updateinfo)/sizeof(struct Upgrade_Info)) {
		memcpy(&fts_updateinfo_curr, &fts_updateinfo[0], sizeof(struct Upgrade_Info));
	}
}

#ifdef CONFIG_OF
static struct fts_platform_data *fts_parse_dt(struct device *dev)
{
	struct fts_platform_data *pdata;
	struct device_node *np = dev->of_node;
	int ret;

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		dev_err(dev, "Could not allocate struct fts_platform_data");
		return NULL;
	}

	pdata->reset_gpio_number = of_get_gpio(np, 0);
	if(pdata->reset_gpio_number < 0){
		dev_err(dev, "fail to get reset_gpio_number\n");
		goto fail;
	}

	pdata->irq_gpio_number = of_get_gpio(np, 1);
	if(pdata->reset_gpio_number < 0){
		dev_err(dev, "fail to get reset_gpio_number\n");
		goto fail;
	}

	ret = of_property_read_string(np, "vdd_name", &pdata->vdd_name);
	if(ret){
		dev_err(dev, "fail to get vdd_name\n");
		goto fail;
	}

	ret = of_property_read_u32_array(np, "virtualkeys", &pdata->virtualkeys,12);
	if(ret){
		dev_err(dev, "fail to get virtualkeys\n");
		goto fail;
	}

	ret = of_property_read_u32(np, "TP_MAX_X", &pdata->TP_MAX_X);
	if(ret){
		dev_err(dev, "fail to get TP_MAX_X\n");
		goto fail;
	}

	ret = of_property_read_u32(np, "TP_MAX_Y", &pdata->TP_MAX_Y);
	if(ret){
		dev_err(dev, "fail to get TP_MAX_Y\n");
		goto fail;
	}

	return pdata;
fail:
	kfree(pdata);
	return NULL;
}
#endif


static int fts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct fts_data *fts;
	struct input_dev *input_dev;
	struct fts_platform_data *pdata = client->dev.platform_data;
	int err = 0;
	unsigned char uc_reg_value;

	pr_info("[FST] %s: probe\n",__func__);
#ifdef CONFIG_OF
	struct device_node *np = client->dev.of_node;
	if (np && !pdata) {
		pdata = fts_parse_dt(&client->dev);
		if(pdata) {
			client->dev.platform_data = pdata;
		}
		else {
			err = -ENOMEM;
			goto exit_alloc_platform_data_failed;
		}
	}
#endif
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	fts = kzalloc(sizeof(*fts), GFP_KERNEL);
	if (!fts) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	fts->platform_data = pdata;
	fts->client = client;
	i2c_set_clientdata(client, fts);
	client->irq = gpio_to_irq(pdata->irq_gpio_number);
	fts->is_suspend = 0;

	fts_hw_init(fts);

	err = fts_read_reg(fts->client, FTS_REG_CIPHER, &uc_reg_value);
	if (err < 0) {
		pr_err("[FST] read chip id error %x\n", uc_reg_value);
		err = -ENODEV;
		goto exit_chip_check_failed;
	}

	mutex_init(&fts->lock);

#if USE_WORK_QUEUE
	INIT_WORK(&fts->pen_event_work, fts_pen_irq_work);
	fts->fts_workqueue = create_singlethread_workqueue("focal-work-queue");
	if (!fts->fts_workqueue) {
		err = -ESRCH;
		goto exit_create_singlethread;
	}
#endif

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "[FST] failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	input_dev->name = FOCALTECH_TS_NAME;
	fts->input_dev = input_dev;

#ifdef TOUCH_VIRTUAL_KEYS
	fts_virtual_keys_init();
#endif

	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
	__set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	__set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	__set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	__set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);
	__set_bit(KEY_MENU,  input_dev->keybit);
	__set_bit(KEY_BACK,  input_dev->keybit);
	__set_bit(KEY_HOMEPAGE,  input_dev->keybit);
	__set_bit(KEY_POWER,  input_dev->keybit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	input_set_abs_params(input_dev,ABS_MT_POSITION_X, 0, pdata->TP_MAX_X, 0, 0);
	input_set_abs_params(input_dev,ABS_MT_POSITION_Y, 0, pdata->TP_MAX_Y, 0, 0);
	input_set_abs_params(input_dev,ABS_MT_TOUCH_MAJOR, 0, 15, 0, 0);
	input_set_abs_params(input_dev,ABS_MT_WIDTH_MAJOR, 0, 15, 0, 0);
	input_set_abs_params(input_dev,ABS_MT_PRESSURE, 0, 127, 0, 0);
#if MULTI_PROTOCOL_TYPE_B
	input_mt_init_slots(input_dev, TS_MAX_FINGER);
#else
	input_set_abs_params(input_dev,ABS_MT_TRACKING_ID, 0, 255, 0, 0);
#endif

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);

	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
		"[FST] fts_probe: failed to register input device: %s\n",
		dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

#if USE_WAIT_QUEUE
		thread = kthread_run(touch_event_handler, (void *)fts, "focal-wait-queue");
		if (IS_ERR(thread)) {
			err = PTR_ERR(thread);
			PRINT_ERR("failed to create kernel thread: %d\n", err);
		}
#elif USE_THREADED_IRQ
	err = request_threaded_irq(client->irq, NULL, fts_interrupt,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_NO_SUSPEND, client->name, fts);
	if (err < 0) {
		dev_err(&client->dev, "[FST] ft5x0x_probe: request irq failed %d\n",err);
		goto exit_irq_request_failed;
	}
#elif USE_WORK_QUEUE
	err = request_irq(client->irq, fts_interrupt,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_NO_SUSPEND, client->name, fts);
	if (err < 0) {
		dev_err(&client->dev, "[FST] ft5x0x_probe: request irq failed %d\n",err);
		goto exit_irq_request_failed;
	}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	fts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	fts->early_suspend.suspend = fts_ts_suspend;
	fts->early_suspend.resume	= fts_ts_resume;
	register_early_suspend(&fts->early_suspend);
#endif

	focaltech_get_upgrade_array(client);

#ifdef CONFIG_FOCALTECH_SYSFS_DEBUG
	fts_create_sysfs(client);
#endif

#ifdef CONFIG_FOCALTECH_FTS_CTL_IIC
	if (ft_rw_iic_drv_init(client) < 0) {
		dev_err(&client->dev, "%s:[FTS] create fts control iic driver failed\n",	__func__);
	}
#endif

#ifdef CONFIG_FOCALTECH_GESTRUE
	//init_para(720,1280,0,0,0);
	fts->gestrue_state = 1;
	enable_irq_wake(client->irq);
	fts_create_gesture_sysfs(client);
#else
	fts->gestrue_state = 0;
#endif

#ifdef CONFIG_FOCALTECH_AUTO_UPGRADE
	printk("********************Enter CTP Auto Upgrade********************\n");
	fts_ctpm_auto_upgrade(client);
#endif

#ifdef CONFIG_FOCALTECH_APK_DEBUG
	fts_create_apk_debug_channel(client);
#endif

	return 0;

exit_irq_request_failed:
	input_unregister_device(input_dev);
exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
exit_create_singlethread:
exit_chip_check_failed:
	gpio_free(pdata->irq_gpio_number);
	gpio_free(pdata->reset_gpio_number);
	kfree(fts);
exit_alloc_data_failed:
exit_check_functionality_failed:
	fts = NULL;
	i2c_set_clientdata(client, fts);
#ifdef CONFIG_OF
exit_alloc_platform_data_failed:
#endif
	return err;
}

static int fts_remove(struct i2c_client *client)
{
	struct fts_data *fts = i2c_get_clientdata(client);

	pr_info("==fts_remove=\n");

#ifdef CONFIG_FOCALTECH_SYSFS_DEBUG
	fts_release_sysfs(client);
#endif

#ifdef CONFIG_FOCALTECH_FTS_CTL_IIC
	ft_rw_iic_drv_exit();
#endif

#ifdef CONFIG_FOCALTECH_APK_DEBUG
	fts_release_apk_debug_channel();
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&fts->early_suspend);
#endif
	free_irq(client->irq, fts);
	input_unregister_device(fts->input_dev);
	input_free_device(fts->input_dev);

#if USE_WORK_QUEUE
	cancel_work_sync(&fts->pen_event_work);
	destroy_workqueue(fts->fts_workqueue);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	cancel_work_sync(&fts->resume_work);
	destroy_workqueue(fts->fts_resume_workqueue);
#endif

	kfree(fts);
	fts = NULL;
	i2c_set_clientdata(client, fts);

	return 0;
}

static int fts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	PRINT_INFO("fts_suspend\n");
	return 0;
}
static int fts_resume(struct i2c_client *client)
{
	PRINT_INFO("fts_resume\n");
	return 0;
}

static const struct i2c_device_id fts_id[] = {
	{ FOCALTECH_TS_NAME, 0 },{ }
};

static const struct of_device_id focaltech_of_match[] = {
	{ .compatible = "focaltech,focaltech_ts", },
	{ }
};

MODULE_DEVICE_TABLE(i2c, fts_id);
MODULE_DEVICE_TABLE(of, focaltech_of_match);

static struct i2c_driver fts_driver = {
	.probe		= fts_probe,
	.remove		= fts_remove,
	.id_table	= fts_id,
	.driver	= {
		.name	= FOCALTECH_TS_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = focaltech_of_match,
	},
	.suspend = fts_suspend,
	.resume = fts_resume,
};

static int __init fts_init(void)
{
	return i2c_add_driver(&fts_driver);
}

static void __exit fts_exit(void)
{
	i2c_del_driver(&fts_driver);
}

module_init(fts_init);
module_exit(fts_exit);

MODULE_AUTHOR("<mshl>");
MODULE_DESCRIPTION("FocalTech fts TouchScreen driver");
MODULE_LICENSE("GPL");
