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
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/crc32.h>
#include <linux/kernel.h>

#include "inv_mpu_iio.h"

static unsigned char dmp3_image[] = {
#include "dmp3Default_30630.txt"
};

static unsigned char dmp4_image[] = {
#include "dmp4_image.txt"
};

#define DMP3_IMAGE_SIZE         ARRAY_SIZE(dmp3_image)
#define DMP4_IMAGE_SIZE         ARRAY_SIZE(dmp4_image)

static int inv_verify_firmware(struct inv_mpu_state *st, u32 memaddr);
static int inv_dmp_data_load(struct inv_mpu_state *st, int image_size,
        u8 *firmware, enum FifoProtocolLoadWho fifo_pl);

int inv_to_int(u8 *buf)
{
    int dd;

    dd = ((buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);

    return dd;
}

static int inv_write_to_sram(struct inv_mpu_state *st, u32 memaddr,
        u32 image_size, u8 *source)
{
    int bank, write_size;
    int result, size;
    u8 *data;

    data = source;
    size = image_size;

    for (bank = 0; size > 0; bank++, size -= write_size) {
        if (size > MPU_MEM_BANK_SIZE)
            write_size = MPU_MEM_BANK_SIZE;
        else
            write_size = size;
        result = mpu_memory_write(st, memaddr, write_size, data);
        if (result) {
            dev_err(st->dev, "error writing firmware:%d\n", bank);
            return result;
        }
        memaddr += write_size;
        data += write_size;
    }

    return 0;
}

static int inv_erase_flash(struct inv_mpu_state *st, int page)
{
    u8 d[1];
    int result;
    int erase_time;

    inv_plat_single_read(st, 0, REG_FLASH_CFG, d);
    d[0] |= BIT_FLASH_IFM_DIS;
    result = inv_plat_single_write(st, 0, REG_FLASH_CFG, d[0]);
    if (result)
        return result;

    erase_time = 0;
    if (page > MAX_FLASH_PAGE_ADDRESS) {
        inv_plat_single_write(st, 0, REG_FLASH_ERASE, BIT_FLASH_ERASE_MASS_EN);
        inv_plat_single_read(st, 0, REG_FLASH_ERASE, d);
        while (d[0] & BIT_FLASH_ERASE_MASS_EN) {
            usleep_range(1000, 1000);
            result = inv_plat_single_read(st, 0, REG_FLASH_ERASE, d);
            if (result)
                return result;
            erase_time++;
        }
        dev_info(st->dev, "mass flash erase time=%d\n", erase_time);
    }

    return 0;
}

static void inv_int32_to_little8(u32 d, u8 *out)
{
    int i;

    for (i = 0; i < 4; i++)
        out[i] = ((d >> (i * 8)) & 0xff);
}

static int inv_init_dma(struct inv_mpu_state *st, u8 dma_channel,
        u32 source_addr, u32 dest_addr, u32 num_bytes)
{
    u8 control_reg_dma_bytes[] = {
            GARNET_DMA_CONTROL_REGISTER_BYTE_0_WORD_SIZE_BITS,
            GARNET_DMA_CONTROL_REGISTER_BYTE_1_MAX_BURST_BITS,
            (GARNET_DMA_CONTROL_REGISTER_BYTE_2_CHG_BIT
                    | GARNET_DMA_CONTROL_REGISTER_BYTE_2_STRT_BIT),
            (GARNET_DMA_CONTROL_REGISTER_BYTE_3_INT_BIT
                    | GARNET_DMA_CONTROL_REGISTER_BYTE_3_TC_BIT
                    | GARNET_DMA_CONTROL_REGISTER_BYTE_3_SINC_BIT
                    | GARNET_DMA_CONTROL_REGISTER_BYTE_3_DINC_BIT) };
    u32 dma_addr = GARNET_DMA_CH_0_START_ADDR
            + dma_channel * GARNET_DMA_CHANNEL_ADDRESS_OFFSET;
    u8 dma_source_dest_addrs[8] = { 0 };
    u8 dma_length[4] = { 0 };
    u8 int_stat[2];
    u8 tmp[4];
    u32 result;

    /* Form DMA configuration message
     write source and dest addresses to dma registers */
    inv_int32_to_little8(source_addr, dma_source_dest_addrs);
    inv_int32_to_little8(dest_addr, &dma_source_dest_addrs[4]);

    /* Write Source and Destination Addresses to the
     dma controller registers.
     NOTE:  memory writes are always through Bank_0,
     so write_mem function handles that */
    inv_write_to_sram(st, dma_addr, 8, dma_source_dest_addrs);

    /* write the length (dmaAddr + 0x0C) */
    inv_int32_to_little8(num_bytes, dma_length);
    inv_write_to_sram(st, dma_addr + GARNET_DMA_TRANSFER_COUNT_OFFSET, 4,
            dma_length);
    /* write the dma control register.  NOTE.  must write the 3rd byte
     (arm DMA) last
     the following writes could more efficiently be done individually...
     per Andy's code, just need to write MEM_ADDR_SEL_3 and then the bytes */
    result = inv_write_to_sram(st,
            dma_addr + GARNET_DMA_CONTROL_REGISTER_BYTE_0_OFFSET, 2,
            control_reg_dma_bytes);
    result = inv_write_to_sram(st,
            dma_addr + GARNET_DMA_CONTROL_REGISTER_BYTE_3_OFFSET, 1,
            &control_reg_dma_bytes[3]);
    mpu_memory_read(st, dma_addr, 4, tmp);
    mpu_memory_read(st, dma_addr + 4, 4, tmp);
    mpu_memory_read(st, dma_addr + 8, 4, tmp);

    msleep(100);
    result = inv_write_to_sram(st,
            dma_addr + GARNET_DMA_CONTROL_REGISTER_BYTE_2_OFFSET, 1,
            &control_reg_dma_bytes[2]);
    int_stat[0] = 0;
    while (!(int_stat[0] & (1 << dma_channel))) {
        result = mpu_memory_read(st, GARNET_DMA_INTERRUPT_REGISTER, 1,
                int_stat);
        usleep_range(10000, 10000);
    }
    do {
        inv_plat_single_read(st, 0, REG_IDLE_STATUS, int_stat);
        usleep_range(1000, 1000);
    } while (!(int_stat[0] & (BIT_FLASH_IDLE | BIT_FLASH_LOAD_DONE)));

    return result;
}

static int inv_load_firmware(struct inv_mpu_state *st)
{
    int result;

    result = inv_write_to_sram(st, SRAM_START_ADDR, DMP_IMAGE_SIZE,
            st->firmware);
    if (result)
        return result;
    result = inv_erase_flash(st, 65);
    if (result)
        return result;
    result = inv_soft_reset(st);
    if (result)
        return result;

    result = inv_init_dma(st, 1, SRAM_START_ADDR, FLASH_START_ADDR,
            DMP_IMAGE_SIZE);
    if (result)
        return result;

    return 0;
}

static int inv_write_dmp_start_address(struct inv_mpu_state *st)
{
    int result;
    u8 address[3] = { 0, 0xFF, 0 };

    result = inv_plat_write(st, 1, REG_PRGRM_STRT_ADDR_DRDY_0, 3, address);
    if (result)
        return result;
    address[2] = 0x08;
    inv_plat_write(st, 1, REG_PRGRM_STRT_ADDR_TIMER_0, 3, address);
    address[2] = 0x10;
    inv_plat_write(st, 1, REG_PRGRM_STRT_ADDR_DEMAND_0, 3, address);

    dev_info(st->dev, "dmp start done\n");

    return 0;
}

static int inv_verify_firmware(struct inv_mpu_state *st, u32 memaddr)
{
    int bank, write_size, size;
    int result;
    u8 firmware[MPU_MEM_BANK_SIZE], d[2];
    u8 *data;

    inv_plat_single_read(st, 0, REG_FLASH_CFG, d);
    d[0] |= BIT_FLASH_CACHE_BYPASS;
    inv_plat_single_write(st, 0, REG_FLASH_CFG, d[0]);

    msleep(100);
    data = st->firmware;
    size = DMP_IMAGE_SIZE;
    dev_info(st->dev, "verify=%d, addr=%x\n", size, memaddr);
    for (bank = 0; size > 0; bank++, size -= write_size) {
        usleep_range(10000, 10000);
        if (size > MPU_MEM_BANK_SIZE)
            write_size = MPU_MEM_BANK_SIZE;
        else
            write_size = size;

        result = mpu_memory_read(st, memaddr, write_size, firmware);
        if (result)
            return result;
        if (0 != memcmp(firmware, data, write_size))
            dev_err(st->dev, "mem error\n");
        memaddr += write_size;
        data += write_size;
    }

    inv_plat_single_read(st, 0, REG_FLASH_CFG, d);
    d[0] &= (~BIT_FLASH_CACHE_BYPASS);
    inv_plat_single_write(st, 0, REG_FLASH_CFG, d[0]);

    return 0;
}

int inv_check_firmware(struct inv_mpu_state *st)
{
    int ret;
    u8 data[21], *fifo_ptr;
    bool back_check;
    s16 fifo_count;
    int count;

    data[0] = 0; /*meta data */
    data[1] = FIFOPROTOCOL_CMD_GETFIRMWAREINFO;
    inv_send_command_down(st, data, 2);
    /* wake_stop is called inside send_command_down */
    inv_wake_start(st);
    back_check = 0;
    while (!back_check) {
        msleep(100);
        ret = inv_plat_single_write(st, 0, REG_FIFO_INDEX,
                FIFOPROTOCOL_TYPE_NORMAL);
        if (ret)
            return ret;
        ret = inv_plat_read(st, 0, REG_FIFO_COUNTH, 2, data);
        if (ret)
            return ret;
        fifo_count = ((data[0] << 8) | data[1]);
        ret = inv_plat_read(st, 0, REG_FIFO_INDEX, 1, data);
        count = 0;
        while ((fifo_count < 15) && (count < 5)) {
            msleep(500);
            ret = inv_plat_single_write(st, 0, REG_FIFO_INDEX,
                    FIFOPROTOCOL_TYPE_NORMAL);
            if (ret)
                return ret;
            ret = inv_plat_read(st, 0, REG_FIFO_COUNTH, 2, data);
            if (ret)
                return ret;
            fifo_count = ((data[0] << 8) | data[1]);

            count++;
        }
        if (fifo_count) {
            back_check = 1;
            ret = inv_plat_read(st, 0, REG_FIFO_R_W, fifo_count, data);
            fifo_ptr = &data[6];
        }
    }

    return 0;
}

static int inv_get_version(struct inv_mpu_state *st)
{
    u8 dd[2], data[21];
    u8 *dptr;
    int ret;
    int fifo_count, count;
    unsigned long h;

    dd[0] = 0;
    dd[1] = 0xb;
    inv_send_command_down(st, dd, ARRAY_SIZE(dd));
    inv_wake_start(st);
    ret = inv_plat_single_write(st, 0, REG_FIFO_INDEX,
            FIFOPROTOCOL_TYPE_NORMAL);
    if (ret)
        return ret;
    ret = inv_plat_read(st, 0, REG_FIFO_COUNTH, 2, dd);
    fifo_count = ((dd[0] << 8) | dd[1]);
    count = 0;
    while ((fifo_count < 21) && (count < 5)) {
        msleep(500);
        ret = inv_plat_single_write(st, 0, REG_FIFO_INDEX,
                FIFOPROTOCOL_TYPE_NORMAL);
        if (ret)
            return ret;
        ret = inv_plat_read(st, 0, REG_FIFO_COUNTH, 2, dd);
        if (ret)
            return ret;
        fifo_count = ((dd[0] << 8) | dd[1]);
        count++;
        dev_info(st->dev, "dataf=%d\n", fifo_count);
        if (!fifo_count) {
            ret = inv_plat_single_write(st, 0, REG_FIFO_INDEX,
                    FIFOPROTOCOL_TYPE_COMMAND);
            if (ret)
                return ret;
            ret = inv_plat_read(st, 0, REG_FIFO_COUNTH, 2, dd);
            if (ret)
                return ret;
            fifo_count = ((dd[0] << 8) | dd[1]);
            dev_info(st->dev, "cmdf=%d\n", fifo_count);
        }

    }
    if (fifo_count) {
        ret = inv_plat_read(st, 0, REG_FIFO_R_W, fifo_count, data);
        dptr = &data[6];
        dev_info(st->dev, "version=%d, %d, %d\n", dptr[0], dptr[1], dptr[2]);
        dptr += 3;
        ret = kstrtoul(dptr, 16, &h);
        dev_info(st->dev, "hash=%lx, ret=%d\n", h, ret);
        dptr += 8;
        st->crc = inv_le_to_int32(dptr);
    } else {
        st->crc = 0;
        return -EINVAL;
    }

    return 0;
}

/*
 * inv_firmware_load() -  calling this function will load the firmware.
 */
int inv_firmware_load(struct inv_mpu_state *st)
{
    int result;
    int crc;

    inv_write_dmp_start_address(st);
    result = inv_fifo_config(st);
    msleep(300);
    result = inv_get_version(st);
    crc = inv_le_to_int32(st->firmware + DMP_IMAGE_SIZE - 4);
    dev_info(st->dev, "r=%d, crc=%x, stcrc=%x\n", result, crc, st->crc);
    if (result || (st->crc != crc)) {
        dev_info(st->dev, "load flash\n");
        result = inv_load_firmware(st);
        if (result) {
            dev_err(st->dev, "load firmware eror\n");
            goto firmware_write_fail;
        }

        result = inv_verify_firmware(st, FLASH_START_ADDR);
        if (result) {
            dev_err(st->dev, "verify firmware error\n");
            goto firmware_write_fail;
        }
        inv_write_dmp_start_address(st);
        result = inv_fifo_config(st);
        if (result)
            return result;
        dev_info(st->dev, "new firmware is flashed\n");
        msleep(300);
        result = inv_get_version(st);
        if (result)
            return result;
    }

    firmware_write_fail:

    return 0;
}

static int inv_send_load_cmd(struct inv_mpu_state *st,
        enum FifoProtocolLoadWho fifo_pl, enum FifoProtocolLoadWhat fifo_w,
        int size)
{
    u8 data[8];

    data[0] = 0; /*meta data */
    data[1] = FIFOPROTOCOL_CMD_LOAD;
    data[2] = fifo_pl;
    data[3] = fifo_w;
    data[7] = ((size >> 24) & 0xff);
    data[6] = ((size >> 16) & 0xff);
    data[5] = ((size >> 8) & 0xff);
    data[4] = (size & 0xff);

    inv_send_command_down(st, data, ARRAY_SIZE(data));
    inv_wake_start(st);

    return 0;
}

static int inv_dmp_data_load(struct inv_mpu_state *st, int image_size,
        u8 *firmware, enum FifoProtocolLoadWho fifo_pl)
{
    u32 addr;
    int ret, count;
    u8 data[8], *fifo_ptr;
    bool back_check;
    s16 fifo_count;
    enum FifoProtocolLoadWhat what;
    enum FifoProtocolLoadWho who;
    static u8 data_fifo[24];

    inv_send_load_cmd(st, fifo_pl, FIFOPROTOCOL_LOAD_WHAT_MALLOC, image_size);

    back_check = 0;
    while (!back_check) {
        msleep(100);
        ret = inv_plat_single_write(st, 0, REG_FIFO_INDEX,
                FIFOPROTOCOL_TYPE_NORMAL);
        if (ret) {
            return ret;
        }
        ret = inv_plat_read(st, 0, REG_FIFO_COUNTH, 2, data);
        if (ret) {
            return ret;
        }
        fifo_count = ((data[0] << 8) | data[1]);
        count = 0;
        while ((fifo_count < 12) && (count < 2)) {
            msleep(500);
            ret = inv_plat_single_write(st, 0, REG_FIFO_INDEX,
                    FIFOPROTOCOL_TYPE_NORMAL);
            if (ret) {
                return ret;
            }
            ret = inv_plat_read(st, 0, REG_FIFO_COUNTH, 2, data);
            if (ret) {
                return ret;
            }
            fifo_count = ((data[0] << 8) | data[1]);
            count++;
        }
        if (fifo_count) {
            back_check = 1;
            ret = inv_plat_read(st, 0, REG_FIFO_R_W, fifo_count, data_fifo);
            fifo_ptr = &data_fifo[0];
            fifo_ptr = &data_fifo[6];
            who = fifo_ptr[0];
            what = fifo_ptr[1];
            addr = inv_to_int(&fifo_ptr[2]);
        } else {
            return -EINVAL;
        }
    }

    if (addr == 0)
        dev_err(st->dev, "command addr is zero!\n");
    else
        dev_info(st->dev, "firmware addr=%x\n", addr);

    ret = inv_write_to_sram(st, addr, image_size, firmware);
    inv_send_load_cmd(st, fifo_pl, FIFOPROTOCOL_LOAD_WHAT_CHECK, 0);

    back_check = 0;
    count = 0;
    count++;
    while ((!back_check) && (count < 20)) {
        ret = inv_plat_single_write(st, 0, REG_FIFO_INDEX,
                FIFOPROTOCOL_TYPE_NORMAL);
        if (ret) {
            return ret;
        }
        ret = inv_plat_read(st, 0, REG_FIFO_COUNTH, 2, data);
        if (ret) {
            return ret;
        }
        fifo_count = ((data[0] << 8) | data[1]);
        if (fifo_count) {
            back_check = 1;
            ret = inv_plat_read(st, 0, REG_FIFO_R_W, fifo_count, data_fifo);
        }
        msleep(100);
        count++;
    }

    return !back_check;
}

int inv_load_dmp3_4(struct inv_mpu_state *st)
{
    int result;

    inv_wake_start(st);
    dev_info(st->dev, "DMP3 CRC=0x%x, size=%d\n",
            crc32(0, dmp3_image, DMP3_IMAGE_SIZE), (int)DMP3_IMAGE_SIZE);
    dev_info(st->dev, "DMP4 CRC=0x%x, size=%d\n",
            crc32(0, dmp4_image, DMP4_IMAGE_SIZE), (int)DMP4_IMAGE_SIZE);
    result = inv_dmp_data_load(st, DMP3_IMAGE_SIZE, dmp3_image,
            FIFOPROTOCOL_LOAD_WHO_DMP3_FW);
    if (result) {
        dev_info(st->dev, "load DMP3 failed\n");
        return 0;
    }
    msleep(300);
    result = inv_dmp_data_load(st, DMP4_IMAGE_SIZE, dmp4_image,
            FIFOPROTOCOL_LOAD_WHO_DMP4_FW);
    if (result) {
        dev_info(st->dev, "load DMP4 failed\n");
        return 0;
    }

    inv_wake_stop(st);
    return 0;
}

int inv_dmp_read(struct inv_mpu_state *st, int off, int size, u8 *buf)
{
    int read_size, data, result;
    u32 memaddr;

    data = 0;
    memaddr = FLASH_START_ADDR + off;
    while (size > 0) {
        if (size > MPU_MEM_BANK_SIZE)
            read_size = MPU_MEM_BANK_SIZE;
        else
            read_size = size;
        result = mpu_memory_read(st, memaddr, read_size, &buf[data]);
        if (result)
            return result;
        memaddr += read_size;
        data += read_size;
        size -= read_size;
    }

    return 0;
}
