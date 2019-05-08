/*
 * Copyright (C) 2012 Invensense, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/device.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/sysfs.h>
#include <linux/jiffies.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kfifo.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/iio/sysfs.h>

#include "inv_mpu_iio.h"

static const struct inv_hw_s hw_info[INV_NUM_PARTS] = {
    [ICM30628] = {
        .whoami = 0xc0,
        .num_reg = 128,
        .name = "ICM30628",
    },
    [SENSORHUB_V4] = {
        .whoami = 0x5a,
        .num_reg = 128,
        .name = "SensorHub-v4",
    },
};

/*
 * inv_dmp_firmware_write() -  calling this function will load the firmware.
 */
static ssize_t inv_dmp_firmware_write(struct file *fp, struct kobject *kobj,
        struct bin_attribute *attr, char *buf, loff_t pos, size_t size)
{
    int result, offset;
    struct device *dev = container_of(kobj, struct device, kobj);
    struct iio_dev *indio_dev = dev_get_drvdata(dev);
    struct inv_mpu_state *st = iio_priv(indio_dev);

    st->firmware_loaded = 0;
    if (!st->firmware) {
        st->firmware = kmalloc(DMP_IMAGE_SIZE, GFP_KERNEL);
        if (!st->firmware) {
            dev_err(dev, "no memory while loading firmware\n");
            return -ENOMEM;
        }
    }
    offset = pos;
    memcpy(st->firmware + pos, buf, size);
    if ((!size) && (DMP_IMAGE_SIZE != pos)) {
        dev_err(dev, "wrong size for DMP firmware 0x%08x vs 0x%08x\n", offset,
                DMP_IMAGE_SIZE);
        kfree(st->firmware);
        st->firmware = 0;
        return -EINVAL;
    }
    if (DMP_IMAGE_SIZE == (pos + size)) {
        st->loading = 1;
        inv_soft_reset(st);
        inv_set_sleep(st, false);
        msleep(300);
        result = inv_firmware_load(st);
        kfree(st->firmware);
        st->firmware = 0;
        st->loading = 0;
        inv_set_low_power(st, true);
        if (result) {
            dev_err(dev, "firmware load failed\n");
            return result;
        }
    }

    return size;
}

static ssize_t inv_dmp_firmware_read(struct file *filp, struct kobject *kobj,
        struct bin_attribute *bin_attr, char *buf, loff_t off, size_t count)
{
    int result, offset;
    struct iio_dev *indio_dev;
    struct inv_mpu_state *st;

    indio_dev = dev_get_drvdata(container_of(kobj, struct device, kobj));
    st = iio_priv(indio_dev);

    mutex_lock(&st->lock);
    offset = off;
    inv_set_sleep(st, false);
    result = inv_dmp_read(st, offset, count, buf);
    inv_set_sleep(st, true);
    mutex_unlock(&st->lock);
    if (result)
        return result;

    return count;
}

int inv_send_command_down(struct inv_mpu_state *st, const u8 *data, int len)
{
    int ret;
    int i;
    u8 d[1];

    mutex_lock(&st->lock_cmd);
    mutex_lock(&st->lock);
    inv_wake_start(st);

    ret = inv_set_fifo_index(st, FIFOPROTOCOL_TYPE_COMMAND);
    if (ret)
        goto exit;
    ret = inv_plat_write(st, 0, REG_FIFO_R_W, len, data);
    if (ret)
        goto exit;
    ret = inv_set_fifo_index(st, FIFOPROTOCOL_TYPE_NORMAL);
    if (ret)
        goto exit;

    if (ret)
        goto exit;
    ret = inv_plat_read(st, 1, REG_B1_SCRATCH_INT, 1, d);
    if (ret)
        goto exit;
    d[0] |= BIT_SCRATCH_INT_7;
    ret = inv_plat_single_write(st, 1, REG_B1_SCRATCH_INT, d[0]);
    if (ret)
        goto exit;

    inv_wake_stop(st);
    mutex_unlock(&st->lock);

    /* sleep a little bit to let sensor hub to process the command */
    msleep(20);

    mutex_unlock(&st->lock_cmd);

    for (i = 0; i < len; ++i)
        dev_info(st->dev, "sending command[%d]=0x%x\n", i, data[i]);

    return 0;
    exit: inv_wake_stop(st);
    mutex_unlock(&st->lock);
    mutex_unlock(&st->lock_cmd);
    return ret;
}

static int inv_reset_chip(struct inv_mpu_state *st, u8 *data, int len)
{
    int ret;

    mutex_lock(&st->lock);
    inv_wake_start(st);

    ret = inv_soft_reset(st);

    inv_wake_stop(st);
    mutex_unlock(&st->lock);

    return ret;
}

static ssize_t inv_self_test(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    struct iio_dev *indio_dev = dev_get_drvdata(dev);
    struct inv_mpu_state *st = iio_priv(indio_dev);
    int res;

    res = inv_hw_self_test(st);

    return sprintf(buf, "%d\n", res);
}

static int inv_reg_dump(struct inv_mpu_state *st)
{
    int i;
    u8 d;

    mutex_lock(&st->lock);
    inv_wake_start(st);

    for (i = 0; i < 0xFF; i++) {
        if (i != REG_FIFO_R_W) {
            inv_plat_single_read(st, 0, i, &d);
            dev_info(st->dev, "reg=%x, %x\n", i, d);
        }
    }

    inv_wake_stop(st);
    mutex_unlock(&st->lock);

    return 0;
}

static ssize_t inv_matrix_show(struct inv_mpu_state *st, int sensor, char *buf)
{
    const struct mpu_platform_data *pdata = &st->plat_data;
    const s8 *orient;

    switch (sensor) {
    case ATTR_MATRIX_ACCEL:
        orient = pdata->accel_orient;
        break;
    case ATTR_MATRIX_MAGN:
        orient = pdata->magn_orient;
        break;
    case ATTR_MATRIX_GYRO:
        orient = pdata->gyro_orient;
        break;
    default:
        return -EINVAL;
    }

    return snprintf(buf, 100, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n", orient[0],
            orient[1], orient[2], orient[3], orient[4], orient[5], orient[6],
            orient[7], orient[8]);
}

static ssize_t inv_misc_store(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t count)
{
    struct iio_dev *indio_dev = dev_get_drvdata(dev);
    struct inv_mpu_state *st = iio_priv(indio_dev);
    struct iio_dev_attr *this_attr = to_iio_dev_attr(attr);
    int result;
    u8 data[64];
    int idx = 0;
    char *str, *parse, *tok;

    if (buf[count - 1] != '\0' && buf[count - 1] != '\n')
        return -EINVAL;
    str = kmemdup(buf, count, GFP_KERNEL);
    if (!str)
        return -ENOMEM;
    str[count - 1] = '\0';

    parse = str;
    while ((tok = strsep(&parse, ",")) != NULL) {
        result = sscanf(tok, "%hhx", &data[idx]);
        if (result != 1)
            break;
        ++idx;
        if (idx >= sizeof(data))
            break;
    }

    switch (this_attr->address) {
    case ATTR_CMD:
        if (!st->firmware_loaded)
            return -EINVAL;
        inv_send_command_down(st, data, idx);
        break;
    case ATTR_RESET:
        inv_reset_chip(st, data, idx);
        break;
    default:
        return -EINVAL;
    }

    kfree(str);
    return count;
}

static ssize_t inv_misc_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    struct iio_dev *indio_dev = dev_get_drvdata(dev);
    struct inv_mpu_state *st = iio_priv(indio_dev);
    struct iio_dev_attr *this_attr = to_iio_dev_attr(attr);
    ssize_t ret;

    switch (this_attr->address) {
    case ATTR_REG_DUMP:
        inv_reg_dump(st);
        ret = snprintf(buf, 10, "1\n");
        break;
    case ATTR_DMP_LOAD:
        st->loading = 1;
        msleep(100);
        dev_info(st->dev, "mpu: sleep 100ms\n");
        inv_set_sleep(st, false);
        ret = inv_load_dmp3_4(st);
        st->loading = 0;
        inv_set_low_power(st, true);
        if (!ret)
            dev_info(st->dev, "mpu:loading dmp3_4 done\n");
        st->firmware_loaded = 1;
        break;
    case ATTR_MATRIX_ACCEL:
    case ATTR_MATRIX_MAGN:
    case ATTR_MATRIX_GYRO:
        ret = inv_matrix_show(st, this_attr->address, buf);
        break;
    default:
        return -EINVAL;
    }

    return ret;
}

static const struct iio_chan_spec inv_mpu_channels[] = {
    {
        .type = IIO_ACCEL,
        .scan_type = {
            .sign = 's',
            .realbits = 8,
            .shift = 0,
            .storagebits = 8,
            .endianness = IIO_LE,
        },
    }
};

/* sensor on/off sysfs control */
static IIO_DEVICE_ATTR(out_command, S_IWUGO, NULL, inv_misc_store, ATTR_CMD);
static IIO_DEVICE_ATTR(misc_reset, S_IWUGO, NULL, inv_misc_store, ATTR_RESET);
static IIO_DEVICE_ATTR(misc_load_dmp, S_IRUGO,
        inv_misc_show, NULL, ATTR_DMP_LOAD);
static IIO_DEVICE_ATTR(debug_reg_dump, S_IRUGO,
        inv_misc_show, NULL, ATTR_REG_DUMP);
static IIO_DEVICE_ATTR(in_accel_matrix, S_IRUGO,
        inv_misc_show, NULL, ATTR_MATRIX_ACCEL);
static IIO_DEVICE_ATTR(in_magn_matrix, S_IRUGO,
        inv_misc_show, NULL, ATTR_MATRIX_MAGN);
static IIO_DEVICE_ATTR(in_anglvel_matrix, S_IRUGO,
        inv_misc_show, NULL, ATTR_MATRIX_GYRO);
static DEVICE_ATTR(misc_self_test, S_IRUGO | S_IWUGO, inv_self_test, NULL);

static struct attribute *inv_attributes[] = {
    &dev_attr_misc_self_test.attr,
    &iio_dev_attr_out_command.dev_attr.attr,
    &iio_dev_attr_misc_reset.dev_attr.attr,
    &iio_dev_attr_misc_load_dmp.dev_attr.attr,
    &iio_dev_attr_debug_reg_dump.dev_attr.attr,
    &iio_dev_attr_in_accel_matrix.dev_attr.attr,
    &iio_dev_attr_in_magn_matrix.dev_attr.attr,
    &iio_dev_attr_in_anglvel_matrix.dev_attr.attr,
    NULL,
};

static const struct attribute_group inv_attribute_group = {
    .name = "mpu",
    .attrs = inv_attributes
};

static const struct iio_info mpu_info = {
    .driver_module = THIS_MODULE,
    .attrs = &inv_attribute_group,
};

/*
 *  inv_check_chip_type() - check and setup chip type.
 */
int inv_check_chip_type(struct iio_dev *indio_dev, int chip)
{
    int ret = 0;
    struct inv_mpu_state *st;
    u8 id;

    st = iio_priv(indio_dev);
    st->chip_type = chip;
    st->hw = &hw_info[st->chip_type];

    inv_wake_start(st);
    ret = inv_soft_reset(st);
    if (ret)
        goto exit;

    ret = inv_plat_single_read(st, 0, REG_WHO_AM_I, &id);
    if (ret)
        goto exit;
    dev_info(st->dev, "whoami=%x\n", id);
    if (id != st->hw->whoami) {
        dev_err(st->dev, "incorrect whoami, bad chip\n");
        ret = -ENODEV;
        goto exit;
    }

    ret = inv_fifo_config(st);
    if (ret)
        goto exit;

    indio_dev->channels = inv_mpu_channels;
    indio_dev->num_channels = ARRAY_SIZE(inv_mpu_channels);
    indio_dev->info = &mpu_info;
    indio_dev->modes = INDIO_DIRECT_MODE;

    if (st->chip_type == ICM30628)
        st->timesync.multi = INV_DEFAULT_MULTI;
    else
        st->firmware_loaded = 1;

    exit: inv_wake_stop(st);
    return ret;
}

/*
 *  inv_create_dmp_sysfs() - create binary sysfs dmp entry.
 */
static const struct bin_attribute dmp_firmware = {
    .attr = {
        .name = "misc_bin_dmp_firmware",
        .mode = S_IRUGO | S_IWUGO
    },
    .size = DMP_IMAGE_SIZE,
    .read = inv_dmp_firmware_read,
    .write = inv_dmp_firmware_write,
};

int inv_create_dmp_sysfs(struct iio_dev *ind)
{
    int result;

    result = sysfs_create_bin_file(&ind->dev.kobj, &dmp_firmware);

    return result;
}
