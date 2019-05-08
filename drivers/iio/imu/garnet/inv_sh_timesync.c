/*
 * Copyright (C) 2015 InvenSense, Inc.
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
#include <linux/device.h>
#include <linux/ktime.h>
#include <linux/math64.h>

#include "inv_sh_timesync.h"
#include "inv_mpu_iio.h"

static inline ktime_t get_sh_time(const struct inv_sh_timesync *timesync,
        uint32_t sh_ts)
{
    ktime_t time;
    uint64_t val;

    val = (uint64_t) sh_ts * (uint64_t) timesync->resolution;
    time = ns_to_ktime(val);

    return time;
}

static int64_t compute_offset(const struct inv_sh_timesync *timesync,
        uint32_t time_sh)
{
    int64_t delta, delta_ms;
    int64_t offset;

    /* compute delta time */
    delta = (int64_t) time_sh - (int64_t) timesync->sync_sh;
    delta *= timesync->resolution;
    delta_ms = div_s64(delta, 1000000);

    /* compute corresponding offset */
    if (timesync->new_offset >= timesync->offset) {
        offset = timesync->offset + delta_ms * timesync->rate;
        if (offset > timesync->new_offset)
            offset = timesync->new_offset;
        else if (offset < timesync->offset)
            offset = timesync->offset;
    } else {
        offset = timesync->offset - delta_ms * timesync->rate;
        if (offset < timesync->new_offset)
            offset = timesync->new_offset;
        else if (offset > timesync->offset)
            offset = timesync->offset;
    }

    return offset;
}

void inv_sh_timesync_init(struct inv_sh_timesync *timesync, struct device *dev)
{
    timesync->dev = dev;
    /* TODO: get dynamically time resolution */
    timesync->resolution = 100 * 1000; /* 100us in ns */
    timesync->rate = 50 * 1000; /* 50us in ns per ms */
    timesync->sync_sh = 0;
    timesync->sync = ktime_set(0, 0);
    timesync->offset = 0;
    timesync->new_offset = timesync->offset;
}

ktime_t inv_sh_timesync_get_timestamp(const struct inv_sh_timesync *timesync,
        uint32_t sh_ts, ktime_t ts)
{
    const ktime_t time_sh = get_sh_time(timesync, sh_ts);
    int64_t offset;
    int64_t val;
    ktime_t time;

    /* use host timestamp if no synchro event received */
    if (ktime_to_ns(timesync->sync) == 0) {
        time = ts;
        goto exit;
    }

    /* correct sh time with corresponding offset */
    offset = compute_offset(timesync, sh_ts);
    val = ktime_to_ns(time_sh) + offset;

    /* clamp timestamp if computed value is in the future */
    if (val > ktime_to_ns(ts)) {
        dev_warn(timesync->dev, "clamping timestamp %lld\n", val);
        time = ts;
    } else {
        time = ns_to_ktime(val);
    }

    exit: return time;
}

void inv_sh_timesync_synchronize(struct inv_sh_timesync *timesync,
        uint32_t sh_ts, ktime_t ts)
{
    const ktime_t time_sh = get_sh_time(timesync, sh_ts);
    int64_t old_offset, new_offset;

    /* compute new offset */
    new_offset = ktime_to_ns(ts) - ktime_to_ns(time_sh);
    old_offset = timesync->new_offset;

    /* update timesync offset */
    timesync->new_offset = new_offset;
    if (ktime_to_ns(timesync->sync) == 0) {
        timesync->offset = new_offset;
        dev_dbg(timesync->dev, "first offset: %lld\n", timesync->new_offset);
    } else {
        timesync->offset = old_offset;
        dev_dbg(timesync->dev, "new offset: %lld\n", timesync->new_offset);
    }

    /* save synchronization time values */
    timesync->sync_sh = sh_ts;
    timesync->sync = ts;
}

void inv_update_clock(struct inv_mpu_state *st)
{
    long long tmp_tick, diff, diff_tick, th, ts;
    struct inv_sh_timesync *ts_ctl = &st->timesync;
    struct inv_sh_timesync *ts_ctl_old = &st->timesync_old;
    u32 multi, m_th;
    u8 buf[4];

    if (st->chip_type != ICM30628)
        return;

    ts = get_time_ns();
    th = NSEC_PER_SEC;
    th *= 4;
    if ((ts - ts_ctl->prev_ts < th) && (ts_ctl->prev_ts != 0))
        return;

    mpu_memory_read(st, 0x50020004, 4, buf);
    tmp_tick = 0xffffffff - (u32) inv_to_int(buf);
    th = NSEC_PER_SEC;
    th *= 20;

    if ((ts_ctl->prev_ts == 0) || (ts - ts_ctl->prev_ts > th)
            || (tmp_tick < ts_ctl->prev_tick)) {
        ts_ctl->prev_tick = tmp_tick;
        ts_ctl->prev_ts = ts;
        ts_ctl->base_ts = ts;
        ts_ctl_old->prev_tick = tmp_tick;
        ts_ctl_old->prev_ts = ts;
        ts_ctl_old->base_ts = ts;
        return;
    }

    ts_ctl_old->prev_tick = ts_ctl->prev_tick;
    ts_ctl_old->prev_ts = ts_ctl->prev_ts;
    ts_ctl_old->base_ts = ts_ctl->base_ts;
    ts_ctl_old->multi = ts_ctl->multi;

    diff = ts - ts_ctl->prev_ts;
    diff_tick = tmp_tick - ts_ctl->prev_tick;
    ts_ctl->prev_tick = tmp_tick;
    ts_ctl->prev_ts = ts;
    ts_ctl->base_ts += (diff_tick * ts_ctl->multi);

    if (diff_tick)
        multi = (u32) div_u64(diff, diff_tick);
    else
        multi = INV_DEFAULT_MULTI;
    m_th = (INV_DEFAULT_MULTI >> 10);
    if (abs(INV_DEFAULT_MULTI - multi) < m_th)
        ts_ctl->multi = multi;

    if (ts > ts_ctl->base_ts + NSEC_PER_MSEC)
        ts_ctl->base_ts += NSEC_PER_MSEC;
    else if (ts < ts_ctl->base_ts - NSEC_PER_MSEC)
        ts_ctl->base_ts -= NSEC_PER_MSEC;
    else
        ts_ctl->base_ts = ts;

    return;

}

long long inv_get_ts(struct inv_mpu_state *st, u32 tick, long long curr_ts,
        int id)
{
    long long c_ts, ts;
    struct inv_sh_timesync *ts_ctl;

    if (tick >= st->timesync.prev_tick)
        ts_ctl = &st->timesync;
    else
        ts_ctl = &st->timesync_old;

    if (ts_ctl->prev_tick > tick) {
        c_ts = ts_ctl->prev_tick - tick;
        c_ts *= ts_ctl->multi;
        ts = ts_ctl->base_ts - c_ts;
    } else {
        c_ts = tick - ts_ctl->prev_tick;
        c_ts *= ts_ctl->multi;
        ts = ts_ctl->base_ts + c_ts;
    }
    if (ts > curr_ts) {
        dev_info(st->dev, "back, tick=%d, cuts=%lld, ts=%lld, id=%d\n",
                tick, curr_ts, ts, id);
        if (st->flush[id])
            st->send_data = false;
        ts = curr_ts;
    } else {
        if (st->flush[id])
            st->send_data = true;
    }

    return ts;
}
