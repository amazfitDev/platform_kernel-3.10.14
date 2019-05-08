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

#include "inv_mpu_iio.h"

#define SPI_CLK_FREQ_PCLK_DIV_8                          0x30
#define SPI_SLAVE_0_ID                                   0x00
#define MAX_SPI_READ_LEN                                 16

#define SPI_MASTER_WRITE_COMMAND                         0x10
#define SPI_MASTER_READ_COMMAND                          0x20
#define SPI_MASTER_WAIT_COMMAND                          0x40
#define SPI_MASTER_READ_UPDATE_COMMAND                   0x80
#define SPI_MASTER_STOP_COMMAND                          0xB7

// Memory Map

#define BANK0_BASE_ADDR         0x50000000
#define GARNET_GPIO_START_ADDR  0x40000000
#define GARNET_FLASH_START_ADDR 0x00000000
#define GARNET_SRAM_START_ADDR  0x20000000

#define GARNET_SRAM_DMP4_START_ADDR     GARNET_SRAM_START_ADDR + 0x0000C000
// DEFINE THE SHARED MEMORY SO WE CAN USE IT FOR SPI AND I2C
#define GARNET_SRAM_SHARED_MEMORY_START_ADDR    GARNET_SRAM_START_ADDR + 0x0000E000
#define GARNET_SRAM_SHARED_MEMORY_DMP4_INTERFACE    GARNET_SRAM_SHARED_MEMORY_START_ADDR + 0x00000180
#define GARNET_SRAM_SHARED_MEMORY_DMP4_INTERFACE_SZ     512

// FIXME: to be removed
#define GARNET_SRAM_SHARED_MEMORY_DMP_IN    GARNET_SRAM_SHARED_MEMORY_START_ADDR + 0x00000180
#define GARNET_SRAM_SHARED_MEMORY_DMP_OUT   GARNET_SRAM_SHARED_MEMORY_START_ADDR + 0x000001C0

// Define Secondary interface addresses in shared SRAM
#define GARNET_SPI_SCRIPT_START_ADDR    GARNET_SRAM_SHARED_MEMORY_START_ADDR
// I2C_0 START ADDR is 32 BYTES AFTER SPI... and so on
#define GARNET_I2C_0_SCRIPT_START_ADDR  GARNET_SPI_SCRIPT_START_ADDR + 32
#define GARNET_I2C_1_SCRIPT_START_ADDR  GARNET_I2C_0_SCRIPT_START_ADDR + 32

static int inv_set_secondary_interface_start_address(struct inv_mpu_state *st,
        u32 address)
{
    int status = 0;
    u8 temp[4] = {0};

    // need to convert the 4 bytes of the actual address into 3 bytes:
    // Secondary interface channel 0 program start adress Bits {[31:28],[19:16]} 0xAB CD EF 12... we want AD.. 2000 4000... should get 20 40 00
    temp[0] = (address >> 24) & 0xF0; // this gets bits [31:28]]
    temp[0] |= (address >> 16) & 0x0F; // should get bits [19:16]
    // Secondary interface channel 0 program start adress Bits {[15:8]}
    temp[1] = (address >> 8) & 0xFF;
    // Secondary interface channel 0 program start adress Bits {[7:0]}
    temp[2] = (address & 0xFF);

    status += inv_plat_write(st, 0, REG_SEC_INTF_PRGM_START_ADDR_0, 3, temp);

    // so specifying 20 40 00 really means 20 00 40  00

    return status;

}

int inv_write_mems_reg(struct inv_mpu_state *st, u8 reg, int length, u8 *data)
{
    int status = 0, result;
    u8 spi_program[32] = { 0 };
    int i = 0, j = 0;
    u8 regData[2] = { 0 };
    u8 temp[2] = { 0 };

    if (length > 16)
        return -EINVAL;

    spi_program[i++] = SPI_CLK_FREQ_PCLK_DIV_8 | SPI_SLAVE_0_ID;
    spi_program[i++] = (SPI_MASTER_WRITE_COMMAND | (length - 1));
    spi_program[i++] = reg;
    for (j = 0; j < length; j++)
        spi_program[j + i] = data[j];
    spi_program[j + i] = SPI_MASTER_STOP_COMMAND;

    // write the spi program to SRAM
    result = mpu_memory_write(st, GARNET_SPI_SCRIPT_START_ADDR, 32,
            spi_program);
    //status = inv_serial_write_mem_Garnet(GARNET_SPI_SCRIPT_START_ADDR,32,spi_program);

    //set the starting address for the SPI transaction
    status += inv_set_secondary_interface_start_address(st,
    GARNET_SPI_SCRIPT_START_ADDR);

    // enable
    inv_plat_read(st, 0, REG_SEC_INTF_CH0_CONFIG, 1, temp);
    temp[0] = (temp[0] | BIT_SEC_INTF_CH0_EN);
    inv_plat_write(st, 0, REG_SEC_INTF_CH0_CONFIG, 1, temp);
    //dev_info(st->dev, "ch0_config=%x, status=%d\n", temp[0], status);

    // kick off the SPI transaction
    regData[0] = BIT_SEC_INTF_CH0_RUN;
    status += inv_plat_write(st, 0, REG_MOD_RUN_ONCE_0, 1, regData);
    //  disable
    inv_plat_read(st, 0, REG_SEC_INTF_CH0_CONFIG, 1, temp);
    temp[0] = temp[0] & ~BIT_SEC_INTF_CH0_EN;
    inv_plat_write(st, 0, REG_SEC_INTF_CH0_CONFIG, 1, temp);

    return status;
}

int inv_read_mems_reg(struct inv_mpu_state *st, u8 reg, int length, u8 *data)
{
    int status = 0;
    u8 spi_program[33] = {0};
    int i = 0;
    u8 regData[2] = {0};
    u8 temp[2] = {0};

    if (length > 16)
        return -EINVAL;

    //dev_info(st->dev, "reg=%d, len=%d\n", reg, length);

    spi_program[i++] = SPI_CLK_FREQ_PCLK_DIV_8 | SPI_SLAVE_0_ID;
    spi_program[i++] = (SPI_MASTER_READ_COMMAND | (length - 1));
    spi_program[i++] = reg;
    spi_program[i++] = SPI_MASTER_READ_UPDATE_COMMAND;
    spi_program[i++] = SPI_MASTER_STOP_COMMAND;

    status = mpu_memory_write(st, GARNET_SPI_SCRIPT_START_ADDR, 32,
            spi_program);

    //set the starting address for the SPI transaction
    status += inv_set_secondary_interface_start_address(st,
    GARNET_SPI_SCRIPT_START_ADDR);
    //dev_info(st->dev, "status_11=%d\n", status);
    // enable
    inv_plat_read(st, 0, REG_SEC_INTF_CH0_CONFIG, 1, temp);
    temp[0] = temp[0] | BIT_SEC_INTF_CH0_EN;
    inv_plat_write(st, 0, REG_SEC_INTF_CH0_CONFIG, 1, temp);

    // kick off the SPI transaction
    regData[0] = BIT_SEC_INTF_CH0_RUN;
    status += inv_plat_write(st, 0, REG_MOD_RUN_ONCE_0, 1, regData);
    //dev_info(st->dev, "ch0_config=%x, status=%d\n", temp[0], status);

    // and read/return the data
    status += inv_plat_read(st, 0, REG_EXT_SLV_SENS_DATA_00, length, data);

    // disable
    inv_plat_read(st, 0, REG_SEC_INTF_CH0_CONFIG, 1, temp);
    temp[0] = temp[0] & ~BIT_SEC_INTF_CH0_EN;
    inv_plat_write(st, 0, REG_SEC_INTF_CH0_CONFIG, 1, temp);

    return status;
}

int inv_ivory_read(struct inv_mpu_state *st, u8 reg, int len, u8 *data)
{
    int total, r_l, r;
    u8 *d;

    d = data;
    total = len;
    while (total > 0) {
        if (total > 16)
            r_l = 16;
        else
            r_l = total;
        r = inv_read_mems_reg(st, reg, r_l, d);
        if (r)
            return r;
        d += r_l;
        total -= r_l;
    }

    return 0;
}

int inv_ivory_single_write(struct inv_mpu_state *st, u8 reg, u8 v)
{
    return inv_write_mems_reg(st, reg, 1, &v);
}

int inv_configure_spi_master2(struct inv_mpu_state *st, u8 numBytesForCh0)
{
    u8 temp[4] = { 0 };
    int status = 0;

    /// enable spi master nicely
    status = inv_plat_read(st, 0, REG_MOD_EN, 1, temp);
    temp[0] = temp[0] | (BIT_SPI_MST_EN | BIT_SERIF_FIFO_EN);
    status = inv_plat_write(st, 0, REG_MOD_EN, 1, temp);

    //inv_display_registers(GARNET_REG_BANK_0,GARNET_MOD_EN_B0,1);

    // clear spi_mst_dis bit, to be safe
    status = inv_plat_read(st, 0, REG_MOD_CTRL, 1, temp);
    temp[0] = temp[1] & (~BIT_SPI_MST_DIS);
    status = inv_plat_write(st, 0, REG_MOD_CTRL, 1, temp);

    //inv_display_registers(GARNET_REG_BANK_0,GARNET_MOD_CTRL_B0,1);

    // set secondary interface slave int config to 0x00

    temp[0] = 0x00;
    status += inv_plat_write(st, 0, REG_SEC_INTF_SLV_INT_CFG, 1, temp);

    //inv_display_registers(GARNET_REG_BANK_0,GARNET_SEC_INTF_SLV_INT_CFG_B0,1);

    // Now configure the GPIO registers to set up the pin(s) between Garnet and Ivory

    // we are going to write  0x1F to address 0x40 00 00 18
    // wonder what this actually does???

    temp[0] = 0x1F;
    mpu_memory_write(st, GARNET_GPIO_START_ADDR + 0x18, 1, temp);

    // set Raw Data Ready Interrupt
    status = inv_plat_read(st, 0, REG_SCRATCH_INT_EN, 1, temp);
    temp[0] = temp[0] | BIT_RAW_DATA_RDY_INT_PIN;
    status += inv_plat_write(st, 0, REG_SCRATCH_INT_EN, 1, temp);

    //inv_display_registers(GARNET_REG_BANK_0,GARNET_SCRATCH_INT_EN_B0,1);

    // enable the ch0 data ready interrupt

    status = inv_plat_read(st, 1, REG_M0_INT_ENABLE, 1, temp);
    temp[0] = temp[0] | BIT_CH_DATA_RDY_M0_INT_EN;
    status += inv_plat_write(st, 1, REG_M0_INT_ENABLE, 1, temp);

    //inv_display_registers(GARNET_REG_BANK_1,GARNET_M0_INT_ENABLE_B1,1);

    // allocate up to 16 bytes to channel 0
    // could error check on the value -- if > 0x0F, set to 0x0F.
    temp[0] = numBytesForCh0; // there are only 4 bits for size... so specified as n-1
    // enable
    temp[0] |= (BIT_SEC_INTF_CH0_MST_MAP_SPI | BIT_SEC_INTF_CH0_EN);
    status += inv_plat_write(st, 0, REG_SEC_INTF_CH0_CONFIG, 1, temp);

    //inv_display_registers(GARNET_REG_BANK_0,GARNET_SEC_INTF_CH0_CONFIG_B0,1);

    return status;

}

#define REG_IVORY_BANK_SEL              0x7F
#define BANK_SEL_0                      0x00
#define BANK_SEL_1                      0x10
#define BANK_SEL_2                      0x20
#define BANK_SEL_3                      0x30

#define REG_IVORY_USER_CTRL             0x03
#define BIT_FIFO_EN                     0x40
#define BIT_DMP_RST                     0x08

#define REG_IVORY_LP_CONFIG             0x05
#define BIT_I2C_MST_CYCLE               0x40
#define BIT_ACCEL_CYCLE                 0x20
#define BIT_GYRO_CYCLE                  0x10

#define REG_IVORY_PWR_MGMT_1            0x06
#define BIT_H_RESET                     0x80
#define BIT_SLEEP                       0x40
#define BIT_LP_EN                       0x20
#define BIT_CLK_PLL                     0x01

#define REG_IVORY_PWR_MGMT_2            0x07
#define BIT_PWR_PRESSURE_STBY           0x40
#define BIT_PWR_ACCEL_STBY              0x38
#define BIT_PWR_GYRO_STBY               0x07
#define BIT_PWR_ALL_OFF                 0x7f

#define REG_IVORY_INT_ENABLE            0x10
#define BIT_DMP_INT_EN                  0x02
#define BIT_WOM_INT_EN                  0x08

#define REG_IVORY_INT_ENABLE_1          0x11
#define BIT_DATA_RDY_3_EN               0x08
#define BIT_DATA_RDY_2_EN               0x04
#define BIT_DATA_RDY_1_EN               0x02
#define BIT_DATA_RDY_0_EN               0x01

#define REG_IVORY_INT_ENABLE_2          0x12
#define BIT_FIFO_OVERFLOW_EN_0          0x1

#define REG_IVORY_INT_ENABLE_3          0x13
#define REG_DMP_INT_STATUS              0x18
#define REG_INT_STATUS                  0x19
#define REG_INT_STATUS_1                0x1A

#define REG_IVORY_FIFO_EN               0x66
#define BIT_SLV_0_FIFO_EN               1

#define REG_IVORY_FIFO_EN_2             0x67
#define BIT_PRS_FIFO_EN                 0x20
#define BIT_ACCEL_FIFO_EN               0x10
#define BITS_GYRO_FIFO_EN               0x0E

#define REG_IVORY_FIFO_RST              0x68

#define REG_IVORY_FIFO_SIZE_0           0x6E
#define BIT_ACCEL_FIFO_SIZE_128         0x00
#define BIT_ACCEL_FIFO_SIZE_256         0x04
#define BIT_ACCEL_FIFO_SIZE_512         0x08
#define BIT_ACCEL_FIFO_SIZE_1024        0x0C
#define BIT_GYRO_FIFO_SIZE_128          0x00
#define BIT_GYRO_FIFO_SIZE_256          0x01
#define BIT_GYRO_FIFO_SIZE_512          0x02
#define BIT_GYRO_FIFO_SIZE_1024         0x03
#define BIT_FIFO_SIZE_1024              0x01
#define BIT_FIFO_SIZE_512               0x00
#define BIT_FIFO_3_SIZE_256             0x40
#define BIT_FIFO_3_SIZE_64              0x00

#define REG_IVORY_FIFO_COUNT_H          0x70
#define REG_IVORY_FIFO_R_W              0x72

#define REG_MEM_START_ADDR              0x7C
#define REG_MEM_BANK_SEL                0x7E

/* bank 1 register map */
#define REG_XA_OFFS_H                   0x14
#define REG_YA_OFFS_H                   0x17
#define REG_ZA_OFFS_H                   0x1A

#define REG_TIMEBASE_CORRECTION_PLL     0x28
#define REG_TIMEBASE_CORRECTION_RCOSC   0x29
#define REG_IVORY_SELF_TEST1            0x02
#define REG_IVORY_SELF_TEST2            0x03
#define REG_IVORY_SELF_TEST3            0x04
#define REG_IVORY_SELF_TEST4            0x0E
#define REG_IVORY_SELF_TEST5            0x0F
#define REG_IVORY_SELF_TEST6            0x10

/* bank 2 register map */
#define REG_IVORY_GYRO_SMPLRT_DIV       0x00

#define REG_IVORY_GYRO_CONFIG_1         0x01
#define SHIFT_GYRO_FS_SEL               1
#define SHIFT_GYRO_DLPCFG               3

#define REG_IVORY_GYRO_CONFIG_2         0x02
#define BIT_GYRO_CTEN                   0x38

#define REG_IVORY_ACCEL_SMPLRT_DIV_1    0x10
#define REG_IVORY_ACCEL_SMPLRT_DIV_2    0x11

#define REG_IVORY_ACCEL_CONFIG          0x14
#define SHIFT_ACCEL_FS                  1

#define REG_IVORY_ACCEL_CONFIG_2        0x15
#define BIT_ACCEL_CTEN                  0x1C

enum INV_SENSORS {
    SENSOR_ACCEL = 0, SENSOR_GYRO,
};

#if 1
#define GYRO_ENGINE_UP_TIME             50
#define THREE_AXES                      3
#define BYTES_PER_SENSOR                6
#define FIFO_COUNT_BYTE                 2

/* full scale and LPF setting */
#define SELFTEST_GYRO_FS                ((0 << 3) | 1)
#define SELFTEST_ACCEL_FS               ((7 << 3) | 0)

/* register settings */
#define SELFTEST_GYRO_SMPLRT_DIV        1
#define SELFTEST_GYRO_AVGCFG            0
#define SELFTEST_ACCEL_SMPLRT_DIV       1
#define SELFTEST_ACCEL_DEC3_CFG         0

#define DEF_SELFTEST_GYRO_SENS          (32768 / 250)
/* wait time before collecting data */
#define SELFTEST_WAIT_TIME              (20)
#define DEF_ST_STABLE_TIME              20
#define DEF_GYRO_SCALE                  131
#define DEF_ST_PRECISION                1000
#define DEF_ST_ACCEL_FS_MG              2000UL
#define DEF_ST_SCALE                    32768
#define DEF_ST_TRY_TIMES                2
#define DEF_ST_ACCEL_RESULT_SHIFT       1
#define DEF_ST_SAMPLES                  200

#define DEF_ACCEL_ST_SHIFT_DELTA_MIN    500
#define DEF_ACCEL_ST_SHIFT_DELTA_MAX    1500
#define DEF_GYRO_CT_SHIFT_DELTA         500

/* Gyro Offset Max Value (dps) */
#define DEF_GYRO_OFFSET_MAX             20
/* Gyro Self Test Absolute Limits ST_AL (dps) */
#define DEF_GYRO_ST_AL                  60
/* Accel Self Test Absolute Limits ST_AL (mg) */
#define DEF_ACCEL_ST_AL_MIN             225
#define DEF_ACCEL_ST_AL_MAX             675

int inv_set_bank_ivory(struct inv_mpu_state *st, u8 bank)
{
    int r;

    r = inv_write_mems_reg(st, REG_IVORY_BANK_SEL, 1, &bank);

    return r;
}

static const u16 mpu_st_tb[256] = { 2620, 2646, 2672, 2699, 2726, 2753, 2781,
        2808, 2837, 2865, 2894, 2923, 2952, 2981, 3011, 3041, 3072, 3102, 3133,
        3165, 3196, 3228, 3261, 3293, 3326, 3359, 3393, 3427, 3461, 3496, 3531,
        3566, 3602, 3638, 3674, 3711, 3748, 3786, 3823, 3862, 3900, 3939, 3979,
        4019, 4059, 4099, 4140, 4182, 4224, 4266, 4308, 4352, 4395, 4439, 4483,
        4528, 4574, 4619, 4665, 4712, 4759, 4807, 4855, 4903, 4953, 5002, 5052,
        5103, 5154, 5205, 5257, 5310, 5363, 5417, 5471, 5525, 5581, 5636, 5693,
        5750, 5807, 5865, 5924, 5983, 6043, 6104, 6165, 6226, 6289, 6351, 6415,
        6479, 6544, 6609, 6675, 6742, 6810, 6878, 6946, 7016, 7086, 7157, 7229,
        7301, 7374, 7448, 7522, 7597, 7673, 7750, 7828, 7906, 7985, 8065, 8145,
        8227, 8309, 8392, 8476, 8561, 8647, 8733, 8820, 8909, 8998, 9088, 9178,
        9270, 9363, 9457, 9551, 9647, 9743, 9841, 9939, 10038, 10139, 10240,
        10343, 10446, 10550, 10656, 10763, 10870, 10979, 11089, 11200, 11312,
        11425, 11539, 11654, 11771, 11889, 12008, 12128, 12249, 12371, 12495,
        12620, 12746, 12874, 13002, 13132, 13264, 13396, 13530, 13666, 13802,
        13940, 14080, 14221, 14363, 14506, 14652, 14798, 14946, 15096, 15247,
        15399, 15553, 15709, 15866, 16024, 16184, 16346, 16510, 16675, 16842,
        17010, 17180, 17352, 17526, 17701, 17878, 18057, 18237, 18420, 18604,
        18790, 18978, 19167, 19359, 19553, 19748, 19946, 20145, 20347, 20550,
        20756, 20963, 21173, 21385, 21598, 21814, 22033, 22253, 22475, 22700,
        22927, 23156, 23388, 23622, 23858, 24097, 24338, 24581, 24827, 25075,
        25326, 25579, 25835, 26093, 26354, 26618, 26884, 27153, 27424, 27699,
        27976, 28255, 28538, 28823, 29112, 29403, 29697, 29994, 30294, 30597,
        30903, 31212, 31524, 31839, 32157, 32479, 32804
};

/**
 * inv_check_gyro_self_test() - check gyro self test. this function
 *                                   returns zero as success. A non-zero return
 *                                   value indicates failure in self test.
 *  @*st: main data structure.
 *  @*reg_avg: average value of normal test.
 *  @*st_avg:  average value of self test
 */
static int inv_check_gyro_self_test(struct inv_mpu_state *st, int *reg_avg,
        int *st_avg)
{
    u8 *regs;
    int ret_val;
    int otp_value_zero = 0;
    int st_shift_prod[3], st_shift_cust[3], i;

    ret_val = 0;
    regs = st->gyro_st_data;
    pr_debug("self_test gyro shift_code - %02x %02x %02x\n",
            regs[0], regs[1], regs[2]);

    for (i = 0; i < 3; i++) {
        if (regs[i] != 0) {
            st_shift_prod[i] = mpu_st_tb[regs[i] - 1];
        } else {
            st_shift_prod[i] = 0;
            otp_value_zero = 1;
        }
    }
    pr_debug("self_test gyro st_shift_prod - %+d %+d %+d\n",
            st_shift_prod[0], st_shift_prod[1], st_shift_prod[2]);

    for (i = 0; i < 3; i++) {
        st_shift_cust[i] = st_avg[i] - reg_avg[i];
        if (!otp_value_zero) {
            /* Self Test Pass/Fail Criteria A */
            if (st_shift_cust[i] < DEF_GYRO_CT_SHIFT_DELTA * st_shift_prod[i])
                ret_val = 1;
        } else {
            /* Self Test Pass/Fail Criteria B */
            if (st_shift_cust[i]
                    < DEF_GYRO_ST_AL * DEF_SELFTEST_GYRO_SENS * DEF_ST_PRECISION)
                ret_val = 1;
        }
    }
    pr_debug("self_test gyro st_shift_cust - %+d %+d %+d\n",
            st_shift_cust[0], st_shift_cust[1], st_shift_cust[2]);
    if (ret_val == 0) {
        /* Self Test Pass/Fail Criteria C */
        for (i = 0; i < 3; i++)
            if (abs(reg_avg[i])
                    > DEF_GYRO_OFFSET_MAX * DEF_SELFTEST_GYRO_SENS
                            * DEF_ST_PRECISION)
                ret_val = 1;
    }

    return ret_val;
}

/**
 * inv_check_accel_self_test() - check accel self test. this function
 *                                   returns zero as success. A non-zero return
 *                                   value indicates failure in self test.
 *  @*st: main data structure.
 *  @*reg_avg: average value of normal test.
 *  @*st_avg:  average value of self test
 */
static int inv_check_accel_self_test(struct inv_mpu_state *st, int *reg_avg,
        int *st_avg)
{
    int ret_val;
    int st_shift_prod[3], st_shift_cust[3], i;
    u8 *regs;
    int otp_value_zero = 0;

#define ACCEL_ST_AL_MIN ((DEF_ACCEL_ST_AL_MIN * DEF_ST_SCALE \
				 / DEF_ST_ACCEL_FS_MG) * DEF_ST_PRECISION)
#define ACCEL_ST_AL_MAX ((DEF_ACCEL_ST_AL_MAX * DEF_ST_SCALE \
				 / DEF_ST_ACCEL_FS_MG) * DEF_ST_PRECISION)

    ret_val = 0;
    regs = st->accel_st_data;
    pr_debug("self_test accel shift_code - %02x %02x %02x\n",
            regs[0], regs[1], regs[2]);

    for (i = 0; i < 3; i++) {
        if (regs[i] != 0) {
            st_shift_prod[i] = mpu_st_tb[regs[i] - 1];
        } else {
            st_shift_prod[i] = 0;
            otp_value_zero = 1;
        }
    }
    pr_debug("self_test accel st_shift_prod - %+d %+d %+d\n",
            st_shift_prod[0], st_shift_prod[1], st_shift_prod[2]);
    if (!otp_value_zero) {
        /* Self Test Pass/Fail Criteria A */
        for (i = 0; i < 3; i++) {
            st_shift_cust[i] = st_avg[i] - reg_avg[i];
            if (st_shift_cust[i]
                    < DEF_ACCEL_ST_SHIFT_DELTA_MIN * st_shift_prod[i])
                ret_val = 1;
            if (st_shift_cust[i]
                    > DEF_ACCEL_ST_SHIFT_DELTA_MAX * st_shift_prod[i])
                ret_val = 1;
        }
    } else {
        /* Self Test Pass/Fail Criteria B */
        for (i = 0; i < 3; i++) {
            st_shift_cust[i] = abs(st_avg[i] - reg_avg[i]);
            if ((st_shift_cust[i] < ACCEL_ST_AL_MIN)
                    || (st_shift_cust[i] > ACCEL_ST_AL_MAX))
                ret_val = 1;
        }
    }
    pr_debug("self_test accel st_shift_cust - %+d %+d %+d\n",
            st_shift_cust[0], st_shift_cust[1], st_shift_cust[2]);

    return ret_val;
}

static int inv_setup_selftest(struct inv_mpu_state *st)
{
    int result;

    result = inv_set_bank_ivory(st, BANK_SEL_0);
    if (result)
        return result;

    /* Wake up */
    result = inv_ivory_single_write(st, REG_IVORY_PWR_MGMT_1, BIT_CLK_PLL);
    if (result)
        return result;

    /* Stop sensors */
    result = inv_ivory_single_write(st, REG_IVORY_PWR_MGMT_2,
            BIT_PWR_PRESSURE_STBY | BIT_PWR_ACCEL_STBY | BIT_PWR_GYRO_STBY);
    if (result)
        return result;

    /* Enable FIFO */
    result = inv_ivory_single_write(st, REG_IVORY_USER_CTRL, BIT_FIFO_EN);
    if (result)
        return result;

    /* Set cycle mode */
    result = inv_ivory_single_write(st, REG_IVORY_LP_CONFIG,
            BIT_I2C_MST_CYCLE | BIT_ACCEL_CYCLE | BIT_GYRO_CYCLE);
    if (result)
        return result;

    result = inv_set_bank_ivory(st, BANK_SEL_2);
    if (result)
        return result;

    /* Configure FSR and DLPF */
    result = inv_ivory_single_write(st, REG_IVORY_GYRO_SMPLRT_DIV,
            SELFTEST_GYRO_SMPLRT_DIV);
    if (result)
        return result;
    result = inv_ivory_single_write(st, REG_IVORY_GYRO_CONFIG_1,
            SELFTEST_GYRO_FS);
    if (result)
        return result;
    result = inv_ivory_single_write(st, REG_IVORY_GYRO_CONFIG_2,
            SELFTEST_GYRO_AVGCFG);
    if (result)
        return result;

    result = inv_ivory_single_write(st, REG_IVORY_ACCEL_SMPLRT_DIV_1, 0);
    if (result)
        return result;
    result = inv_ivory_single_write(st, REG_IVORY_ACCEL_SMPLRT_DIV_2,
            SELFTEST_ACCEL_SMPLRT_DIV);
    if (result)
        return result;
    result = inv_ivory_single_write(st, REG_IVORY_ACCEL_CONFIG,
            SELFTEST_ACCEL_FS);
    if (result)
        return result;
    result = inv_ivory_single_write(st, REG_IVORY_ACCEL_CONFIG_2,
            SELFTEST_ACCEL_DEC3_CFG);
    if (result)
        return result;

    result = inv_set_bank_ivory(st, BANK_SEL_1);
    if (result)
        return result;

    /* Read selftest values */
    result = inv_ivory_read(st, REG_IVORY_SELF_TEST1, 1, &st->gyro_st_data[0]);
    if (result)
        return result;
    result = inv_ivory_read(st, REG_IVORY_SELF_TEST2, 1, &st->gyro_st_data[1]);
    if (result)
        return result;
    result = inv_ivory_read(st, REG_IVORY_SELF_TEST3, 1, &st->gyro_st_data[2]);
    if (result)
        return result;

    result = inv_ivory_read(st, REG_IVORY_SELF_TEST4, 1, &st->accel_st_data[0]);
    if (result)
        return result;
    result = inv_ivory_read(st, REG_IVORY_SELF_TEST5, 1, &st->accel_st_data[1]);
    if (result)
        return result;
    result = inv_ivory_read(st, REG_IVORY_SELF_TEST6, 1, &st->accel_st_data[2]);
    if (result)
        return result;

    result = inv_set_bank_ivory(st, BANK_SEL_0);

    /* Restart sensors */
    result = inv_ivory_single_write(st, REG_IVORY_PWR_MGMT_2,
            BIT_PWR_PRESSURE_STBY);
    if (result)
        return result;
    msleep(GYRO_ENGINE_UP_TIME);

    return result;
}

static u8 fifo_data[8192];
static int inv_selftest_read_samples(struct inv_mpu_state *st,
        enum INV_SENSORS type, int *sum_result, int *s)
{
    u8 w;
    u16 fifo_count;
    s16 vals[3];
    u8 *d;
    int r, i, j, t, packet_count;

    d = fifo_data;
    r = inv_ivory_single_write(st, REG_IVORY_FIFO_EN_2, 0);
    if (r)
        return r;

    /* Reset FIFO */
    r = inv_ivory_single_write(st, REG_IVORY_FIFO_RST, 0x1F);
    if (r)
        return r;
    r = inv_ivory_single_write(st, REG_IVORY_FIFO_RST, 0x1E);
    if (r)
        return r;

    if (SENSOR_GYRO == type)
        w = BITS_GYRO_FIFO_EN;
    else
        w = BIT_ACCEL_FIFO_EN;

    while (*s < DEF_ST_SAMPLES) {
        r = inv_ivory_single_write(st, REG_IVORY_FIFO_EN_2, w);
        if (r)
            return r;

        mdelay(SELFTEST_WAIT_TIME);

        r = inv_ivory_single_write(st, REG_IVORY_FIFO_EN_2, 0);
        if (r)
            return r;

        r = inv_ivory_read(st, REG_IVORY_FIFO_COUNT_H, FIFO_COUNT_BYTE, d);
        if (r)
            return r;
        fifo_count = be16_to_cpup((__be16 *) (&d[0]));
        pr_debug("self_test fifo_count - %d\n", fifo_count);

        packet_count = fifo_count / BYTES_PER_SENSOR;
        r = inv_ivory_read(st, REG_IVORY_FIFO_R_W,
                packet_count * BYTES_PER_SENSOR, d);
        pr_debug("rrr=%d, total=%d\n", r, *s);
        if (r)
            return r;
        i = 0;
        while (i < packet_count) {
            for (j = 0; j < THREE_AXES; j++) {
                t = 2 * j + i * BYTES_PER_SENSOR;
                vals[j] = (s16) be16_to_cpup((__be16 *) (&d[t]));
                sum_result[j] += vals[j];
            }
            pr_debug("self_test data - %d %+d %+d %+d",
                    *s, vals[0], vals[1], vals[2]);
            (*s)++;
            i++;
        }
    }

    return 0;
}

/*
 *  inv_do_test_accel() - do the actual test of self testing
 */
static int inv_do_test_accel(struct inv_mpu_state *st, int *accel_result,
        int *accel_st_result)
{
    int result, i, j;
    int accel_s;
    u8 w;

    for (i = 0; i < THREE_AXES; i++) {
        accel_result[i] = 0;
        accel_st_result[i] = 0;
    }

    accel_s = 0;
    result = inv_selftest_read_samples(st, SENSOR_ACCEL, accel_result,
            &accel_s);
    if (result)
        return result;

    for (j = 0; j < THREE_AXES; j++) {
        accel_result[j] = accel_result[j] / accel_s;
        accel_result[j] *= DEF_ST_PRECISION;
    }

    /* Set Self-Test Bit */
    result = inv_set_bank_ivory(st, BANK_SEL_2);
    if (result)
        return result;
    w = BIT_ACCEL_CTEN | SELFTEST_ACCEL_DEC3_CFG;
    result = inv_ivory_single_write(st, REG_IVORY_ACCEL_CONFIG_2, w);
    if (result)
        return result;
    result = inv_set_bank_ivory(st, BANK_SEL_0);
    msleep(DEF_ST_STABLE_TIME);

    accel_s = 0;
    result = inv_selftest_read_samples(st, SENSOR_ACCEL, accel_st_result,
            &accel_s);
    if (result)
        return result;

    for (j = 0; j < THREE_AXES; j++) {
        accel_st_result[j] = accel_st_result[j] / accel_s;
        accel_st_result[j] *= DEF_ST_PRECISION;
    }
    /* recover Self-Test Bit */
    result = inv_set_bank_ivory(st, BANK_SEL_2);
    if (result)
        return result;
    result = inv_ivory_single_write(st, REG_IVORY_ACCEL_CONFIG_2, 0);
    if (result)
        return result;
    result = inv_set_bank_ivory(st, BANK_SEL_0);

    pr_debug("accel=%d, %d, %d\n",
            accel_result[0], accel_result[1], accel_result[2]);

    return 0;
}

/*
 *  inv_do_test() - do the actual test of self testing
 */
static int inv_do_test_gyro(struct inv_mpu_state *st, int *gyro_result,
        int *gyro_st_result)
{
    int result, i, j;
    int gyro_s;
    u8 w;

    for (i = 0; i < THREE_AXES; i++) {
        gyro_result[i] = 0;
        gyro_st_result[i] = 0;
    }

    gyro_s = 0;
    result = inv_selftest_read_samples(st, SENSOR_GYRO, gyro_result, &gyro_s);
    if (result)
        return result;

    for (j = 0; j < THREE_AXES; j++) {
        gyro_result[j] = gyro_result[j] / gyro_s;
        gyro_result[j] *= DEF_ST_PRECISION;
    }

    /* Set Self-Test Bit */
    result = inv_set_bank_ivory(st, BANK_SEL_2);
    if (result)
        return result;
    w = BIT_GYRO_CTEN | SELFTEST_GYRO_AVGCFG;
    result = inv_ivory_single_write(st, REG_IVORY_GYRO_CONFIG_2, w);
    if (result)
        return result;
    result = inv_set_bank_ivory(st, BANK_SEL_0);
    msleep(DEF_ST_STABLE_TIME);

    gyro_s = 0;
    result = inv_selftest_read_samples(st, SENSOR_GYRO, gyro_st_result,
            &gyro_s);
    if (result)
        return result;

    for (j = 0; j < THREE_AXES; j++) {
        gyro_st_result[j] = gyro_st_result[j] / gyro_s;
        gyro_st_result[j] *= DEF_ST_PRECISION;
    }

    /* recover Self-Test Bit */
    result = inv_set_bank_ivory(st, BANK_SEL_2);
    if (result)
        return result;
    result = inv_ivory_single_write(st, REG_IVORY_GYRO_CONFIG_2, 0);
    if (result)
        return result;
    result = inv_set_bank_ivory(st, BANK_SEL_0);
    pr_debug("gyro=%d, %d, %d\n",
            gyro_result[0], gyro_result[1], gyro_result[2]);

    return 0;
}

/*
 *  inv_hw_self_test() - main function to do hardware self test
 */
int inv_hw_self_test(struct inv_mpu_state *st)
{
    int result;
    int gyro_bias_st[THREE_AXES], gyro_bias_regular[THREE_AXES];
    int accel_bias_st[THREE_AXES], accel_bias_regular[THREE_AXES];
    int test_times, i;
    char accel_result, gyro_result;

    accel_result = 0;
    gyro_result = 0;

    pr_debug("hw test111\n");
    inv_wake_start(st);
    inv_set_sleep(st, false);
    inv_configure_spi_master2(st, 15);
    result = inv_setup_selftest(st);
    if (result)
        goto test_fail;

    /* gyro test */
    test_times = DEF_ST_TRY_TIMES;
    while (test_times > 0) {
        result = inv_do_test_gyro(st, gyro_bias_regular, gyro_bias_st);
        if (result == -EAGAIN)
            test_times--;
        else
            test_times = 0;
    }
    if (result)
        goto test_fail;
    pr_debug("self_test gyro bias_regular - %+d %+d %+d\n",
            gyro_bias_regular[0], gyro_bias_regular[1], gyro_bias_regular[2]);
    pr_debug("self_test gyro bias_st - %+d %+d %+d\n",
            gyro_bias_st[0], gyro_bias_st[1], gyro_bias_st[2]);

    /* accel test */
    test_times = DEF_ST_TRY_TIMES;
    while (test_times > 0) {
        result = inv_do_test_accel(st, accel_bias_regular, accel_bias_st);
        if (result == -EAGAIN)
            test_times--;
        else
            break;
    }
    if (result)
        goto test_fail;
    pr_debug("self_test accel bias_regular - %+d %+d %+d\n",
            accel_bias_regular[0], accel_bias_regular[1], accel_bias_regular[2]);
    pr_debug("self_test accel bias_st - %+d %+d %+d\n",
            accel_bias_st[0], accel_bias_st[1], accel_bias_st[2]);

    for (i = 0; i < 3; i++) {
        st->gyro_st_bias[i] = gyro_bias_regular[i] / DEF_ST_PRECISION;
        st->accel_st_bias[i] = accel_bias_regular[i] / DEF_ST_PRECISION;
    }

    accel_result = !inv_check_accel_self_test(st, accel_bias_regular,
            accel_bias_st);
    gyro_result = !inv_check_gyro_self_test(st, gyro_bias_regular,
            gyro_bias_st);

    test_fail: return (accel_result << DEF_ST_ACCEL_RESULT_SHIFT) | gyro_result;
}
#endif
