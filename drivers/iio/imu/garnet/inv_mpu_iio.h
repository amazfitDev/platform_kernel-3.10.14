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
#ifndef _INV_MPU_IIO_H_
#define _INV_MPU_IIO_H_

#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/wakelock.h>
#include <linux/iio/iio.h>
#include <linux/iio/imu/mpu.h>

#include "inv_sh_data.h"
#include "inv_sh_timesync.h"

#define REG_WHO_AM_I                    0x00

#define REG_MOD_EN                      0x02
#define BIT_I2C_IF_DIS                  0x01
#define BIT_SERIF_FIFO_EN               0x02
#define BIT_DMP_EN                      0x04
#define BIT_MCU_EN                      0x08
#define BIT_I2C_MST_EN                  0x10
#define BIT_SPI_MST_EN                  0x20
#define BIT_TIMERS_EN                   0x40

#define REG_MOD_RST                     0x03

#define REG_MOD_RUN_ONCE_0              0x04
#define BIT_SEC_INTF_CH0_RUN            0x1

#define REG_PWR_MGMT_1                  0x06
#define BIT_SLEEP_REQ                   0x01
#define BIT_POWER_DOWN_REQ              0x02
#define BIT_LOW_POWER_EN                0x04
#define BIT_STANDBY_EN                  0x08
#define BIT_DEEP_SLEEP_EN               0x10
#define BIT_SOFT_RESET                  0x80

#define REG_INT_PIN_CFG                 0x0E
#define BIT_INT_FSYNC_ACTIVE_LOW        0x01
#define BIT_INT_LATCH_EN                0x02
#define BIT_INT_ACTIVE_LOW              0x04

#define REG_SCRATCH_INT_EN              0x0F
#define BIT_RAW_DATA_RDY_INT_PIN        0x08

#define REG_WM_INT_EN                   0x13
#define BIT_COMMAND_FIFO_EN             0x1
#define BIT_DATA_FIFO_EN                0x2

#define REG_B0_SCRATCH_INT0_STATUS      0x19
#define REG_B0_SCRATCH_INT1_STATUS      0x20
#define REG_B0_SCRATCH_INT2_STATUS      0x21

#define REG_EXT_SLV_SENS_DATA_00        0x2A

#define REG_MEM_ADDR_SEL_0              0x70

#define REG_MEM_R_W                     0x74
#define REG_FIFO_INDEX                  0x75
#define REG_FIFO_COUNTH                 0x76
#define REG_FIFO_R_W                    0x78

#define REG_FLASH_ERASE                 0x7C
#define BIT_FLASH_ERASE_MASS_EN         0x80

#define REG_IDLE_STATUS                 0x7D
#define BIT_FLASH_IDLE                  0x01
#define BIT_FLASH_LOAD_DONE             0x02

#define REG_MOD_CTRL                    0x89
#define BIT_SERIF_FIFO_DIS              0x04
#define BIT_M0_DIS                      0x08
#define BIT_SPI_MST_DIS                 0x40

#define REG_MOD_CTRL2                   0x8A
#define BIT_FIFO_EMPTY_IND_DIS          0x08

#define REG_SEC_INTF_CH0_CONFIG         0xA0
#define BIT_SEC_INTF_CH0_EN             0x80
#define BIT_SEC_INTF_CH0_MST_MAP_SPI    0x40

#define REG_SEC_INTF_PRGM_START_ADDR_0  0xB9

#define REG_SEC_INTF_SLV_INT_CFG        0xBD

#define REG_FIFO_3_SIZE                 0xD4
#define REG_FIFO_2_SIZE                 0xD5
#define REG_FIFO_1_SIZE                 0xD6
#define REG_FIFO_0_SIZE                 0xD7

#define REG_FIFO_1_WM                   0xDE
#define BIT_FIFO_1_WM_DEFAULT           0xF8

#define REG_FIFO_0_WM                   0xDF

#define REG_FIFO_3_PKT_SIZE             0xE4
#define REG_FIFO_2_PKT_SIZE             0xE5
#define REG_FIFO_1_PKT_SIZE             0xE6
#define REG_FIFO_0_PKT_SIZE             0xE7
#define REG_PKT_SIZE_OVERRIDE           0xE8

#define REG_FIFO_CFG                    0xE9
#define REG_FIFO_RST                    0xEA
#define REG_FIFO_MODE                   0xEB

#define REG_FLASH_CFG                   0xFC
#define BIT_FLASH_IFM_DIS               0x02
#define BIT_FLASH_CACHE_BYPASS          0x10

#define REG_TEST_MODES                  0xFD
#define BIT_TEST_MODES_DAP_EN           0x01
#define BIT_TEST_MODES_SLAVE_TM_EN      0x02
#define BIT_TEST_MODES_SKU_TYPE         0x04

#define REG_BANK_SEL                    0xFF
#define BIT_SPI_ADDR_MSB                0x02

/* bank 1 */
#define REG_PRGRM_STRT_ADDR_DRDY_0      0x00
#define REG_PRGRM_STRT_ADDR_TIMER_0     0x03
#define REG_PRGRM_STRT_ADDR_DEMAND_0    0x06

#define REG_M0_INT_ENABLE               0x20
#define BIT_CH_DATA_RDY_M0_INT_EN       0x01

#define REG_B1_SCRATCH_INT              0x26
#define BIT_SCRATCH_INT_0               0x01
#define BIT_SCRATCH_INT_1               0x02
#define BIT_SCRATCH_INT_2               0x04
#define BIT_SCRATCH_INT_3               0x08
#define BIT_SCRATCH_INT_4               0x10
#define BIT_SCRATCH_INT_5               0x20
#define BIT_SCRATCH_INT_6               0x40
#define BIT_SCRATCH_INT_7               0x80

#define IIO_BUFFER_SIZE                 1
#define DMP_IMAGE_SIZE                  65536
#define BANK0_BASE_ADDR                 0x50000000
#define FLASH_START_ADDR                0
#define SRAM_START_ADDR                 0x20000000
#define MPU_MEM_BANK_SIZE               256
#define MAX_FLASH_PAGE_ADDRESS          0x1F
#define INV_WATER_MARK_THRESHOLD        1536
#define INV_DEFAULT_MULTI               (NSEC_PER_SEC / 32768)
#define FIFO_SIZE                       4096

#define GARNET_DMA_CH_0_START_ADDR                          0x40002000
#define GARNET_DMA_CHANNEL_ADDRESS_OFFSET                   0x20

#define GARNET_DMA_INTERRUPT_REGISTER                       0x40002100

#define GARNET_DMA_SOURCE_ADDRESS_OFFSET                    0x00
#define GARNET_DMA_DEST_ADDRESS_OFFSET                      0x04
#define GARNET_DMA_TRANSFER_COUNT_OFFSET                    0x0C

#define GARNET_DMA_CONTROL_REGISTER_BYTE_0_OFFSET           0x08
#define GARNET_DMA_CONTROL_REGISTER_BYTE_0_WORD_SIZE_BITS   0x02
#define GARNET_DMA_CONTROL_REGISTER_BYTE_1_OFFSET           0x09
#define GARNET_DMA_CONTROL_REGISTER_BYTE_1_MAX_BURST_BITS   0x00
#define GARNET_DMA_CONTROL_REGISTER_BYTE_2_OFFSET           0x0A
#define GARNET_DMA_CONTROL_REGISTER_BYTE_2_START_BIT        0x02
#define GARNET_DMA_CONTROL_REGISTER_BYTE_2_TYPE_BITS        0x00
#define GARNET_DMA_CONTROL_REGISTER_BYTE_2_CHG_BIT          0x04
#define GARNET_DMA_CONTROL_REGISTER_BYTE_2_STRT_BIT         0x02

#define GARNET_DMA_CONTROL_REGISTER_BYTE_3_OFFSET           0x0B
#define GARNET_DMA_CONTROL_REGISTER_BYTE_3_INT_BIT          0x01
#define GARNET_DMA_CONTROL_REGISTER_BYTE_3_TC_BIT           0x02
#define GARNET_DMA_CONTROL_REGISTER_BYTE_3_SINC_BIT         0x04
#define GARNET_DMA_CONTROL_REGISTER_BYTE_3_SDEC_BIT         0x08
#define GARNET_DMA_CONTROL_REGISTER_BYTE_3_DINC_BIT         0x20
#define GARNET_DMA_CONTROL_REGISTER_BYTE_3_DDEC_BIT         0x40

#define GARNET_PRGRM_STRT_ADDR_DRDY_0_B1                    0x00
#define GARNET_PRGRM_STRT_ADDR_TIMER_0_B1                   0x03
#define GARNET_PRGRM_STRT_ADDR_DEMAND_0_B1                  0x06

#define SRAM_SHARED_MEMROY_START_ADDR   (SRAM_START_ADDR + 0xE000)

/* device enum */
enum inv_devices {
    ICM30628, SENSORHUB_V4, INV_NUM_PARTS
};

enum FifoProtocolType {
    FIFOPROTOCOL_TYPE_COMMAND = 0,
    FIFOPROTOCOL_TYPE_NORMAL = 1,
    FIFOPROTOCOL_TYPE_WAKEUP = 2,
    FIFOPROTOCOL_TYPE_MAX
};

/** @brief command id definition */
enum FifoProtocolCmd {
    FIFOPROTOCOL_CMD_SENSOROFF = 0,
    FIFOPROTOCOL_CMD_SENSORON = 1,
    FIFOPROTOCOL_CMD_SETPOWER = 2,
    FIFOPROTOCOL_CMD_BATCHON = 3,
    FIFOPROTOCOL_CMD_FLUSH = 4,
    FIFOPROTOCOL_CMD_SETDELAY = 5,
    FIFOPROTOCOL_CMD_SETCALIBRATIONGAINS = 6,
    FIFOPROTOCOL_CMD_GETCALIBRATIONGAINS = 7,
    FIFOPROTOCOL_CMD_SETCALIBRATIONOFFSETS = 8,
    FIFOPROTOCOL_CMD_GETCALIBRATIONOFFSETS = 9,
    FIFOPROTOCOL_CMD_SETREFERENCEFRAME = 10,
    FIFOPROTOCOL_CMD_GETFIRMWAREINFO = 11,
    FIFOPROTOCOL_CMD_GETDATA = 12,
    FIFOPROTOCOL_CMD_GETCLOCKRATE = 13,
    FIFOPROTOCOL_CMD_PING = 14,
    FIFOPROTOCOL_CMD_RESET = 15,
    FIFOPROTOCOL_CMD_LOAD = 0x10,
    FIFOPROTOCOL_CMD_MAX,
};

enum FifoProtocolLoadWho {
    FIFOPROTOCOL_LOAD_WHO_RESERVED = 0,
    FIFOPROTOCOL_LOAD_WHO_DMP3_FW = 1,
    FIFOPROTOCOL_LOAD_WHO_DMP4_FW = 2,
    FIFOPROTOCOL_LOAD_WHO_MAX,
};

enum FifoProtocolLoadWhat {
    FIFOPROTOCOL_LOAD_WHAT_RESERVED = 0,
    FIFOPROTOCOL_LOAD_WHAT_MALLOC = 1,
    FIFOPROTOCOL_LOAD_WHAT_FREE = 2,
    FIFOPROTOCOL_LOAD_WHAT_CHECK = 3,
    FIFOPROTOCOL_LOAD_WHAT_MAX,
};

/**
 *  struct inv_hw_s - Other important hardware information.
 *  @whoami:	Who am I identifier
 *  @num_reg:	Number of registers on device.
 *  @name:      name of the chip
 */
struct inv_hw_s {
    uint8_t whoami;
    uint8_t num_reg;
    char *name;
};

/**
 *  struct inv_mpu_state - Driver state variables.
 *  @dev:               device structure.
 *  @trig;              iio trigger.
 *  @hw:		Other hardware-specific information.
 *  @chip_type:		chip type.
 *  @fifo_length:	length of data fifo.
 *  @lock:		mutex lock.
 *  @client:		i2c client handle.
 *  @plat_data:		platform data.
 *  @spi_mode:		spi activated or i2c by default.
 *  @loading:           firmware loading is going on.
 *  @irq:               irq number store.
 *  @timestamp		last irq timestamp.
 *  @databuf:           data buffer.
 *  @datasize:          data buffer size.
 *  @datacount:         data count in buffer.
 *  @data_enable:       enable data.
 *  @sensors_list:	list of available sensors.
 *  @timesync:          timestamp sync structure.
 *  @timesync_old:      timestamp sync structure for the previous session.
 *  @firmware_loaded:   flag indicats firmware loaded.
 *  @firmware:          pointer to the firmware memory allocation.
 *  @bank:              current bank information.
 *  @fifo_index:	current FIFO index.
 .*  @wake_lock:         Android wake_lock.
 *  @power_on:          power on function.
 *  @power_off:         power off function.
 *  @flush:         flag to indicate flush.
 *  @send_data:     flag to send data or not.
 *  @ts_ctl:        time stamp control params.
 *  @ts_ctl_old:    previous time stamp control params.
 *  @send_status:   status of sending data.
 *  @gyro_st_data[3]: gyro self test data.
 *  @accel_st_data[3]: accel self test data.
 *  @crc:           crc number for the firmware.
 */
struct inv_mpu_state {
    struct device *dev;
    struct iio_trigger *trig;
    const struct inv_hw_s *hw;
    enum inv_devices chip_type;
    size_t fifo_length;
    struct mutex lock;
    struct mutex lock_cmd;
    struct mpu_platform_data plat_data;
    bool spi_mode;
    bool loading;
    int irq;
    int irq_nowake;
    ktime_t timestamp;
    u8 *databuf;
    size_t datasize;
    size_t datacount;
    atomic_t data_enable;
    struct list_head sensors_list;
    struct inv_sh_timesync timesync;
    struct inv_sh_timesync timesync_old;
    bool firmware_loaded;
    u8 *firmware;
    u8 bank;
    u8 fifo_index;
    struct wake_lock wake_lock;
    int (*power_on)(const struct inv_mpu_state *);
    int (*power_off)(const struct inv_mpu_state *);
    int (*write)(struct inv_mpu_state *, u8 bank, u8 reg, u16 len,
            const u8 * data);
    int (*read)(struct inv_mpu_state *, u8 bank, u8 reg, u16 len, u8 * data);
    bool flush[INV_SH_DATA_SENSOR_ID_CUSTOM_MAX];
    bool send_data;
    int send_status;
    u8 gyro_st_data[3];
    u8 accel_st_data[3];
    int accel_st_bias[3];
    int gyro_st_bias[3];
    u32 crc;
};

/* IIO attribute address */
enum MPU_IIO_ATTR_ADDR {
    ATTR_CMD = 1,
    ATTR_RESET,
    ATTR_REG_DUMP,
    ATTR_MATRIX_ACCEL,
    ATTR_MATRIX_MAGN,
    ATTR_MATRIX_GYRO,
    ATTR_DMP_LOAD,
};

static inline int inv_set_power_on(const struct inv_mpu_state *st)
{
    int ret = 0;

    if (st->power_on)
        ret = st->power_on(st);

    return ret;
}

static inline int inv_set_power_off(const struct inv_mpu_state *st)
{
    int ret = 0;

    if (st->power_off)
        ret = st->power_off(st);

    return ret;
}

static inline int inv_plat_write(struct inv_mpu_state *st, u8 bank, u8 reg,
        u16 len, const u8 * data)
{
    int ret = -1;

    if (st->write)
        ret = st->write(st, bank, reg, len, data);

    return ret;
}

static inline int inv_plat_read(struct inv_mpu_state *st, u8 bank, u8 reg,
        u16 len, u8 * data)
{
    int ret = -1;

    if (st->read)
        ret = st->read(st, bank, reg, len, data);

    return ret;
}

static inline int inv_plat_single_write(struct inv_mpu_state *st, u8 bank,
        u8 reg, u8 data)
{
    return inv_plat_write(st, bank, reg, 1, &data);
}

static inline int inv_plat_single_read(struct inv_mpu_state *st, u8 bank,
        u8 reg, u8 * data)
{
    return inv_plat_read(st, bank, reg, 1, data);
}

static inline int inv_mem_reg_write(struct inv_mpu_state *st, u32 mem)
{
    u32 data = cpu_to_be32(mem);

    return inv_plat_write(st, 0, REG_MEM_ADDR_SEL_0, sizeof(data), (u8 *) &data);
}

static inline int mpu_memory_write(struct inv_mpu_state *st, u32 mem_addr,
        u16 len, const u8 * data)
{
    int ret;

    ret = inv_mem_reg_write(st, mem_addr);
    if (ret)
        return ret;

    return inv_plat_write(st, 0, REG_MEM_R_W, len, data);
}

static inline int mpu_memory_read(struct inv_mpu_state *st, u32 mem_addr,
        u16 len, u8 * data)
{
    int ret;

    ret = inv_mem_reg_write(st, mem_addr);
    if (ret)
        return ret;

    return inv_plat_read(st, 0, REG_MEM_R_W, len, data);
}

int inv_mpu_configure_ring(struct iio_dev *indio_dev);
int inv_mpu_probe_trigger(struct iio_dev *indio_dev);
void inv_mpu_unconfigure_ring(struct iio_dev *indio_dev);
void inv_mpu_remove_trigger(struct iio_dev *indio_dev);
int inv_set_fifo_index(struct inv_mpu_state *st, u8 fifo_index);
int inv_check_chip_type(struct iio_dev *indio_dev, int chip);
int inv_fifo_config(struct inv_mpu_state *st);
int inv_soft_reset(struct inv_mpu_state *st);
int inv_firmware_load(struct inv_mpu_state *st);
int inv_dmp_read(struct inv_mpu_state *st, int off, int size, u8 * buf);
int inv_create_dmp_sysfs(struct iio_dev *ind);
int inv_send_command_down(struct inv_mpu_state *st, const u8 * data, int len);
void inv_wake_start(struct inv_mpu_state *st);
void inv_wake_stop(struct inv_mpu_state *st);
void inv_init_power(struct inv_mpu_state *st);
int inv_proto_set_power(struct inv_mpu_state *st, bool power_on);
int inv_load_dmp3_4(struct inv_mpu_state *st);
int inv_set_sleep(struct inv_mpu_state *st, bool set);
int inv_set_low_power(struct inv_mpu_state *st, bool set);
int inv_to_int(u8 * buf);
s64 get_time_ns(void);
int inv_write_mems_reg(struct inv_mpu_state *st, u8 reg, int length, u8 * data);
int inv_read_mems_reg(struct inv_mpu_state *st, u8 reg, int length, u8 * data);
int inv_configure_spi_master2(struct inv_mpu_state *st, u8 numBytesForCh0);
int inv_hw_self_test(struct inv_mpu_state *st);
void inv_update_clock(struct inv_mpu_state *st);
long long inv_get_ts(struct inv_mpu_state *st, u32 tick, long long curr_ts,
        int id);
int inv_le_to_int32(const uint8_t * frame);
void inv_sensor_process(struct inv_mpu_state *st,
        struct inv_sh_data *sensor_data, long long timestamp);

#endif /* #ifndef _INV_MPU_IIO_H_ */
