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
#include <linux/kernel.h>
#include <linux/atomic.h>
#include <linux/ktime.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/sysfs.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/wakelock.h>

#include <linux/iio/iio.h>
#include <linux/iio/buffer.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/triggered_buffer.h>

#include "inv_mpu_iio.h"
#include "inv_sh_data.h"
#include "inv_sh_sensor.h"
#include "inv_sh_misc.h"

#define MAX_IO_READ_SIZE	128
#if 0
static irqreturn_t inv_irq_handler(int irq, void *dev_id)
{
    struct inv_mpu_state *st = (struct inv_mpu_state *)dev_id;

    st->timestamp = ktime_get_boottime();

    return IRQ_WAKE_THREAD;
}
#endif
static int inv_process_data(struct inv_mpu_state *st, bool wake,
        bool *wake_data)
{
    u8 data[2], tmp_buf[INV_SH_DATA_MAX_SIZE], reg;
    long long timestamp_icm;
    u16 fifo_count;
    u8 *dptr;
    size_t size;
    size_t total_bytes, rem_bytes;
    struct inv_sh_data sensor_data;
    bool recover_wm;
    ktime_t timestamp;
    int ret;

    if (wake)
        reg = REG_B0_SCRATCH_INT0_STATUS;
    else
        reg = REG_B0_SCRATCH_INT1_STATUS;

    recover_wm = false;
    *wake_data = false;
    inv_wake_start(st);
    inv_update_clock(st);
    ret = inv_plat_single_read(st, 0, reg, data);
    if (ret)
        goto error_wake;

    timestamp_icm = get_time_ns();

    if (data[0] == 0) {
        ret = 0;
        goto error_wake;
    }
    ret = inv_plat_read(st, 0, REG_FIFO_COUNTH, 2, data);
    if (ret)
        goto error_wake;
    fifo_count = ((data[0] << 8) | data[1]);
    if ((fifo_count >= INV_WATER_MARK_THRESHOLD)
            && (st->chip_type == ICM30628)) {
        inv_plat_single_write(st, 0, REG_FIFO_1_WM, 0xff);
        recover_wm = true;
    }
    if (!fifo_count) {
        ret = 0;
        goto error_wake;
    }
    if (fifo_count > st->fifo_length) {
        dev_err(st->dev, "FIFO count (%hu) internal error truncating\n",
                fifo_count);
        fifo_count = st->fifo_length;
    }

    /* losing bytes if buffer is not big enough, should never happen */
    rem_bytes = st->datasize - st->datacount;
    if (fifo_count > rem_bytes) {
        total_bytes = fifo_count - rem_bytes;
        dev_err(st->dev, "not enough space in buffer losing %zu bytes\n",
                total_bytes);
        dptr = st->databuf + total_bytes;
        size = st->datacount - total_bytes;
        memmove(st->databuf, dptr, size);
        st->datacount = size;
    }

    /* reading fifo data in buffer */
    total_bytes = fifo_count;
    while (total_bytes > 0) {
        dptr = &st->databuf[st->datacount];
        size = total_bytes;
        /* clip with max io read size */
        if (size > MAX_IO_READ_SIZE)
            size = MAX_IO_READ_SIZE;
        ret = inv_plat_read(st, 0, REG_FIFO_R_W, size, dptr);
        if (ret)
            goto error_wake;
        total_bytes -= size;
        st->datacount += size;
    }

    inv_wake_stop(st);

    /* parsing and dispatching data to sensors */
    dptr = st->databuf;
    size = st->datacount;
    /* search for first valid data */
    while (inv_sh_data_parse(dptr, size, &sensor_data) == 0) {
        /* manage command answer */
        if (sensor_data.is_answer) {
            inv_sh_misc_send_raw_data(st, sensor_data.raw, sensor_data.size);
        } else {
            if (st->chip_type == ICM30628)
                inv_sensor_process(st, &sensor_data, timestamp_icm);
            else
                /* manage sensor data */
                timestamp = inv_sh_sensor_timestamp(st, &sensor_data,
                        st->timestamp);

            ret = inv_sh_sensor_dispatch_data(st, &sensor_data, timestamp);
            if (ret == -ENODEV)
                inv_sh_misc_send_sensor_data(st, &sensor_data, timestamp);
            else if (ret != 0)
                dev_err(st->dev, "error %d dispatching data\n", ret);
            /* mark if there is wake-up sensor data */
            if (sensor_data.id & INV_SH_DATA_SENSOR_ID_FLAG_WAKE_UP)
                *wake_data = true;
        }
        dptr += sensor_data.size;
        size -= sensor_data.size;
    }
    ret = 0;
    /* update data buffer */
    if (size < INV_SH_DATA_MAX_SIZE) {
        memcpy(tmp_buf, dptr, size);
        memcpy(st->databuf, tmp_buf, size);
        st->datacount = size;
    } else {
        st->datacount = 0;
    }

    error_wake: if (recover_wm)
        ret = inv_plat_single_write(st, 0, REG_FIFO_1_WM,
                BIT_FIFO_1_WM_DEFAULT);
    inv_wake_stop(st);
    return ret;
}

/*
 *  inv_read_fifo() - Transfer data from FIFO to ring buffer.
 */
static irqreturn_t inv_read_fifo(struct inv_mpu_state *st, bool wake)
{
    int ret;
    bool wake_data = false;

    if (st->loading)
        return IRQ_HANDLED;

    mutex_lock(&st->lock);
    ret = inv_process_data(st, wake, &wake_data);
    mutex_unlock(&st->lock);
    if (ret)
        dev_err(st->dev, "error reading fifo: %d\n", ret);

    /* Android spec: the driver must hold a "timeout wake lock" for 200 ms
     * each time an event is being reported
     */
    if (ret == 0 && wake_data)
        wake_lock_timeout(&st->wake_lock, msecs_to_jiffies(200));

    return IRQ_HANDLED;
}

static irqreturn_t inv_read_fifo_wake(int irq, void *dev_id)
{
    struct inv_mpu_state *st = (struct inv_mpu_state *) dev_id;

    inv_read_fifo(st, true);

    return IRQ_HANDLED;
}

static irqreturn_t inv_read_fifo_nowake(int irq, void *dev_id)
{
    struct inv_mpu_state *st = (struct inv_mpu_state *) dev_id;

    inv_read_fifo(st, false);

    return IRQ_HANDLED;
}

static irqreturn_t inv_irq_handler_wake(int irq, void *dev_id)
{
    struct inv_mpu_state *st = (struct inv_mpu_state *) dev_id;

    st->timestamp = ktime_get_boottime();

    return IRQ_WAKE_THREAD;
}

static irqreturn_t inv_irq_handler_nowake(int irq, void *dev_id)
{
    return IRQ_WAKE_THREAD;
}

void inv_mpu_unconfigure_ring(struct iio_dev *indio_dev)
{
    struct inv_mpu_state *st = iio_priv(indio_dev);

    free_irq(st->irq, st);
    kfree(st->databuf);
    if (st->irq_nowake >= 0)
        free_irq(st->irq_nowake, st);
    iio_triggered_buffer_cleanup(indio_dev);
}

int inv_mpu_configure_ring(struct iio_dev *indio_dev)
{
    struct inv_mpu_state *st = iio_priv(indio_dev);
    int ret;

    ret = iio_triggered_buffer_setup(indio_dev, &iio_pollfunc_store_time, NULL,
            NULL);
    if (ret)
        goto error;

    st->databuf = kmalloc(st->fifo_length + INV_SH_DATA_MAX_SIZE, GFP_KERNEL);
    if (!st->databuf) {
        ret = -ENOMEM;
        goto error_iio_buffer_free;
    }
    /* Size is (FIFO size + 1 frame) for avoiding overflow */
    st->datasize = st->fifo_length + INV_SH_DATA_MAX_SIZE;
    st->datacount = 0;

    ret = request_threaded_irq(st->irq, inv_irq_handler_wake,
            inv_read_fifo_wake,
            IRQF_TRIGGER_RISING | IRQF_SHARED | IRQF_ONESHOT, "inv_mpu_wake",
            st);
    if (ret) {
        goto error_databuf_free;
    } else {
        enable_irq_wake(st->irq);
    }

    if (st->irq_nowake >= 0) {
        ret = request_threaded_irq(st->irq_nowake, inv_irq_handler_nowake,
                inv_read_fifo_nowake,
                IRQF_TRIGGER_RISING | IRQF_SHARED | IRQF_ONESHOT, "inv_mpu",
                st);
        if (ret)
            goto error_irq_free;
    }

    return 0;

error_irq_free:
    free_irq(st->irq, st);
error_databuf_free:
    kfree(st->databuf);
error_iio_buffer_free:
    iio_triggered_buffer_cleanup(indio_dev);
error:
    return ret;
}
