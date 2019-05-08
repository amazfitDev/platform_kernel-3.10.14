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
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/iio/imu/mpu.h>
#include <linux/android_alarm.h>
#include <linux/ktime.h>
#include <linux/time.h>

#include "inv_mpu_iio.h"

#define INV_SLEEP_MODE          BIT_DEEP_SLEEP_EN

s64 get_time_ns(void)
{
    struct timespec ts;

    ts = ktime_to_timespec(alarm_get_elapsed_realtime());

    return timespec_to_ns(&ts);
}

static int inv_mpu_power_on(const struct inv_mpu_state *st)
{
    const struct mpu_platform_data *pdata = &st->plat_data;
    int err;

    if (!IS_ERR(pdata->vdd_ana)) {
        err = regulator_enable(pdata->vdd_ana);
        if (err) {
            dev_err(st->dev, "error enabling vdd_ana: %d\n", err);
            return err;
        }
    }

    if (!IS_ERR(pdata->vdd_i2c)) {
        err = regulator_enable(pdata->vdd_i2c);
        if (err) {
            dev_err(st->dev, "error enabling vdd_i2c: %d\n", err);
            return err;
        }
    }

    return 0;
}

static int inv_mpu_power_off(const struct inv_mpu_state *st)
{
    const struct mpu_platform_data *pdata = &st->plat_data;
    int err1 = 0;
    int err2 = 0;

    if (!IS_ERR(pdata->vdd_ana)) {
        err1 = regulator_disable(pdata->vdd_ana);
        if (err1)
            dev_err(st->dev, "error disabling vdd_ana: %d\n", err1);
    }

    if (!IS_ERR(pdata->vdd_i2c)) {
        err2 = regulator_disable(pdata->vdd_i2c);
        if (err2)
            dev_err(st->dev, "error disabling vdd_i2c: %d\n", err2);
    }

    if (err1 || err2)
        return -EPERM;
    else
        return 0;
}

void inv_init_power(struct inv_mpu_state *st)
{
    const struct mpu_platform_data *pdata = &st->plat_data;

    if (!IS_ERR(pdata->vdd_ana) || !IS_ERR(pdata->vdd_i2c)) {
        st->power_on = inv_mpu_power_on;
        st->power_off = inv_mpu_power_off;
    }
}

int inv_proto_set_power(struct inv_mpu_state *st, bool power_on)
{
    u8 frame[] = { 0x00, 0x02, 0x00, 0x00 };

    if (power_on)
        frame[2] = 0x01;

    return inv_send_command_down(st, frame, sizeof(frame));
}

int inv_set_fifo_index(struct inv_mpu_state *st, u8 fifo_index)
{
    int ret;

    if (st->fifo_index == fifo_index)
        return 0;

    ret = inv_plat_single_write(st, 0, REG_FIFO_INDEX, fifo_index);
    if (ret)
        return ret;

    st->fifo_index = fifo_index;
    return 0;
}

int inv_soft_reset(struct inv_mpu_state *st)
{
    int ret;

    /* Warning: soft reset reset the bus, ACK is not received! */
    inv_plat_single_write(st, 0, REG_PWR_MGMT_1, BIT_SOFT_RESET);
    msleep(100);
    ret = inv_plat_single_write(st, 0, REG_PWR_MGMT_1, 0);
    if (ret)
        return ret;

    /* reset bank and fifo_index */
    st->bank = 0;
    st->fifo_index = 0;

    return 0;
}

/* workaround some hardware bug when writing to PWR_MGMT_1 register */
static int inv_set_pwr_1(struct inv_mpu_state *st, uint8_t v)
{
    int ret = 0;

    ret |= inv_plat_single_write(st, 0, REG_PWR_MGMT_1, v);
    ret |= inv_plat_single_write(st, 0, REG_PWR_MGMT_1, v);
    ret |= inv_plat_single_write(st, 0, REG_PWR_MGMT_1, v);

    return ret;
}

int inv_set_sleep(struct inv_mpu_state *st, bool set)
{
    uint8_t w;

    if (st->chip_type != ICM30628)
        return 0;

    if (set)
        w = BIT_SLEEP_REQ | INV_SLEEP_MODE;
    else
        w = 0;

    return inv_set_pwr_1(st, w);
}

int inv_set_low_power(struct inv_mpu_state *st, bool set)
{
    uint8_t w = INV_SLEEP_MODE;

    if (st->chip_type != ICM30628)
        return 0;

    if (set)
        w |= BIT_LOW_POWER_EN;

    return inv_set_pwr_1(st, w);
}

int inv_fifo_config(struct inv_mpu_state *st)
{
    int result;
    u8 reg_mod;

    /* Disabling FIFO and assert reset */
    result = inv_plat_single_read(st, 0, REG_MOD_EN, &reg_mod);
    if (result)
        return result;
    reg_mod |= BIT_SERIF_FIFO_EN;
    result = inv_plat_single_write(st, 0, REG_MOD_EN, reg_mod);
    if (result)
        return result;
    result = inv_plat_single_write(st, 0, REG_FIFO_RST, 0x03);
    if (result)
        return result;

    /* Override FIFOs size to 1 byte data */
    result = inv_plat_single_write(st, 0, REG_PKT_SIZE_OVERRIDE, 0x03);
    if (result)
        return result;
    result = inv_plat_single_write(st, 0, REG_FIFO_0_PKT_SIZE, 0);
    if (result)
        return result;
    result = inv_plat_single_write(st, 0, REG_FIFO_1_PKT_SIZE, 0);
    if (result)
        return result;

    /* set FIFO 0 size to 127 bytes */
    result = inv_plat_single_write(st, 0, REG_FIFO_0_SIZE, 0x5F);
    if (result)
        return result;

    /* set FIFO 1 size to 4095 bytes */
    result = inv_plat_single_write(st, 0, REG_FIFO_1_SIZE, 0xFF);
    if (result)
        return result;

    /* set FIFO 1 WM */
    result = inv_plat_single_write(st, 0, REG_FIFO_1_WM, BIT_FIFO_1_WM_DEFAULT);
    if (result)
        return result;

    /* set FIFO 0 WM */
    result = inv_plat_single_write(st, 0, REG_FIFO_0_WM, 0xE1);
    if (result)
        return result;

    /* configure FIFOs in data streaming rollover mode */
    result = inv_plat_single_write(st, 0, REG_FIFO_MODE, 0x03);
    if (result)
        return result;

    /* disable the empty indicator */
    result = inv_plat_single_write(st, 0, REG_MOD_CTRL2,
            BIT_FIFO_EMPTY_IND_DIS);
    if (result)
        return result;

    /* Deassert reset and enable FIFO */
    result = inv_plat_single_write(st, 0, REG_FIFO_RST, 0x0);
    if (result)
        return result;
    /* Enable chip */
    result = inv_plat_single_read(st, 0, REG_MOD_EN, &reg_mod);
    if (result)
        return result;
    /* Enable MCU */
    reg_mod |= BIT_MCU_EN;
    /* Disable I2C if SPI is used */
    if (st->spi_mode)
        reg_mod |= BIT_I2C_IF_DIS;
    result = inv_plat_single_write(st, 0, REG_MOD_EN, reg_mod);
    if (result)
        return result;
    st->fifo_length = FIFO_SIZE;

    return result;
}

void inv_wake_start(struct inv_mpu_state *st)
{
    const struct mpu_platform_data *pdata = &st->plat_data;

    if (gpio_is_valid(pdata->wake_gpio)) {
        gpio_set_value(pdata->wake_gpio, 1);
        usleep_range(pdata->wake_delay_min, pdata->wake_delay_max);
    }

    inv_set_low_power(st, false);
}

void inv_wake_stop(struct inv_mpu_state *st)
{
    const struct mpu_platform_data *pdata = &st->plat_data;

    /* inv_set_low_power(st, true); */
    inv_set_low_power(st, false);

    if (gpio_is_valid(pdata->wake_gpio))
        gpio_set_value(pdata->wake_gpio, 0);
}
