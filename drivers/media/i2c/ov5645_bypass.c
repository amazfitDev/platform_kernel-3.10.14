/*
 * V4L2 Driver for camera sensor ov5640
 *
 * Copyright (C) 2012, Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
//#include <media/ov5645.h>
#include <mach/ovisp-v4l2.h>
#include "ov5645.h"


#define REG_CHIP_ID_HIGH		0x300a
#define REG_CHIP_ID_LOW			0x300b
#define REG_CHIP_REVISION		0x302a
#define CHIP_ID_HIGH			(0x56)
#define CHIP_ID_LOW				(0x45)

#define  ov5645_DEFAULT_WIDTH    320
#define  ov5645_DEFAULT_HEIGHT   240


/* Private v4l2 controls */
#define V4L2_CID_PRIVATE_BALANCE  (V4L2_CID_PRIVATE_BASE + 0)
#define V4L2_CID_PRIVATE_EFFECT  (V4L2_CID_PRIVATE_BASE + 1)

/* In flip, the OV5645 does not need additional settings because the ISP block
 * will auto-detect whether the pixel is in the red line or blue line and make
 * the necessary adjustments.
 */
#define REG_TC_VFLIP			0x3820
#define REG_TC_MIRROR			0x3821
#define OV5645_HFLIP			0x1
#define OV5645_VFLIP			0x2
#define OV5645_FLIP_VAL			((unsigned char)0x06)
#define OV5645_FLIP_MASK		((unsigned char)0x06)

 /* whether sensor support high resolution (> vga) preview or not */
#define SUPPORT_HIGH_RESOLUTION_PRE		1

/*
 * Struct
 */
struct regval_list {
	u16 reg_num;
	u8 value;
};

struct mode_list {
	u16 index;
	const struct regval_list *mode_regs;
};

/* Supported resolutions */
enum ov5645_width {
//	W_QVGA	= 320,
	W_VGA	= 640,
	W_720P	= 1280,
	W_960p	= 1280,
	W_5M	= 2592,
};

enum ov5645_height {
//	H_QVGA	= 240,
	H_VGA	= 480,
	H_720P	= 720,
	H_960p	= 960,
	H_5M	= 1944,
};

struct ov5645_win_size {
	char *name;
	enum ov5645_width width;
	enum ov5645_height height;
	const struct regval_list *regs;
};

struct ov5645_priv {
	struct v4l2_subdev		subdev;
	struct ov5645_camera_info	*info;
	enum v4l2_mbus_pixelcode	cfmt_code;
	struct ov5645_win_size	*win;
	int				model;
	u16				balance_value;
	u16				effect_value;
	u16				flag_vflip:1;
	u16				flag_hflip:1;


};

int cam_t_j = 0, cam_t_i = 0;
unsigned long long cam_t0_buf[10];
unsigned long long cam_t1_buf[10];
static int ov5645_s_power(struct v4l2_subdev *sd, int on);
static inline int sensor_i2c_master_send(struct i2c_client *client,
		const char *buf ,int count)
{
	int ret;
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.len = count;
	msg.buf = (char *)buf;
	ret = i2c_transfer(adap, &msg, 1);



	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	   transmitted, else error code. */
	return (ret == 1) ? count : ret;
}

#if 0
s32 ov5645_read(struct i2c_client *client, u16 reg)
{
	int ret;
	u8 retval;
	u16 r = cpu_to_be16(reg);

	ret = sensor_i2c_master_send(client,(u8 *)&r,2);

	if (ret < 0)
		return ret;
	if (ret != 2)
		return -EIO;

	ret = sensor_i2c_master_recv(client, &retval, 1);
	if (ret < 0)
		return ret;
	if (ret != 1)
		return -EIO;
	return retval;
}

s32 ov5645_write(struct i2c_client *client, u16 reg, u8 val)
{
	unsigned char msg[3];
	int ret;

	reg = cpu_to_be16(reg);

	memcpy(&msg[0], &reg, 2);
	memcpy(&msg[2], &val, 1);

	ret = sensor_i2c_master_send(client, msg, 3);

	if (ret < 0)
	{
		printk("RET<0\n");
		return ret;
	}
	if (ret < 3)
	{
		printk("RET<3\n");
		return -EIO;
	}

	return 0;
}
#endif
int ov5645_read(struct i2c_client *client, unsigned short reg,
		unsigned char *value)
{
	int ret;
	unsigned char buf[2] = {reg >> 8, reg & 0xff};
	struct i2c_msg msg[2] = {
		[0] = {
			.addr   = client->addr,
			.flags  = 0,
			.len    = 2,
			.buf    = buf,
		},
		[1] = {
			.addr   = client->addr,
			.flags  = I2C_M_RD,
			.len    = 1,
			.buf    = value,
		}
	};

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret > 0)
		ret = 0;

	return ret;
}

static int ov5645_write(struct i2c_client *client, unsigned short reg, unsigned char value)
{
	int ret;
	unsigned char buf[3] = {reg >> 8, reg & 0xff, value};
	struct i2c_msg msg = {
		.addr   = client->addr,
		.flags  = 0,
		.len    = 3,
		.buf    = buf,
	};

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0)
		ret = 0;

	return ret;
}
/*
 * Registers settings
 */

#define ENDMARKER { 0xff, 0xff }

static const struct regval_list ov5645_init_regs[] = {

	{0x3103 ,0x11},
	{0x3008 ,0x82},
	{0x4202, 0x0f},


	{0x3008, 0x42},
	{0x3103, 0x03},
	{0x3503, 0x07},
	{0x3002, 0x1c},
	{0x3006, 0xc3},
	{0x300e, 0x45},
	{0x3017, 0x40},
	{0x3018, 0x00},
	{0x302e, 0x0b},
	{0x3037, 0x13},
	{0x3108, 0x01},
	{0x3611, 0x06},
	{0x3612, 0xab},
	{0x3614, 0x50},
	{0x3618, 0x00},
	{0x3034, 0x18},
	{0x3035, 0x21},
	{0x3036, 0x70},
	{0x3500, 0x00},
	{0x3501, 0x01},
	{0x3502, 0x00},
	{0x350a, 0x00},
	{0x350b, 0x3f},
	{0x3600, 0x09},
	{0x3601, 0x43},
	{0x3620, 0x33},
	{0x3621, 0xe0},
	{0x3622, 0x01},
	{0x3630, 0x2d},
	{0x3631, 0x00},
	{0x3632, 0x32},
	{0x3633, 0x52},
	{0x3634, 0x70},
	{0x3635, 0x13},
	{0x3636, 0x03},
	{0x3702, 0x6e},
	{0x3703, 0x52},
	{0x3704, 0xa0},
	{0x3705, 0x33},
	{0x3708, 0x66},
	{0x3709, 0x12},
	{0x370b, 0x61},
	{0x370c, 0xc3},
	{0x370f, 0x10},
	{0x3715, 0x08},
	{0x3717, 0x01},
	{0x371b, 0x20},
	{0x3731, 0x22},
	{0x3739, 0x70},
	{0x3901, 0x0a},
	{0x3905, 0x02},
	{0x3906, 0x10},
	{0x3719, 0x86},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x06},
	{0x3804, 0x0a},
	{0x3805, 0x3f},
	{0x3806, 0x07},
	{0x3807, 0x9d},
	{0x3808, 0x05},
	{0x3809, 0x00},
	{0x380a, 0x03},
	{0x380b, 0xc0},
	{0x380c, 0x07},
	{0x380d, 0x68},
	{0x380e, 0x03},
	{0x380f, 0xd8},
	{0x3810, 0x00},
	{0x3811, 0x10},
	{0x3812, 0x00},
	{0x3813, 0x06},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3820, 0x47},
	{0x3821, 0x07},
	{0x3824, 0x01},
	{0x3826, 0x03},
	{0x3828, 0x08},
	{0x3a02, 0x03},
	{0x3a03, 0xd8},
	{0x3a08, 0x01},
	{0x3a09, 0xf8},
	{0x3a0a, 0x01},
	{0x3a0b, 0xa4},
	{0x3a0e, 0x02},
	{0x3a0d, 0x02},
	{0x3a14, 0x03}, // 50Hz max exposure = 984
	{0x3a15, 0xd8}, // 50Hz max exposure
	{0x3a18, 0x01}, // gain ceiling = 31.5x
	{0x3a19, 0xf8}, // gain ceiling
	// 50/0xauto, 0xde,tect
	{0x3c01, 0x34},
	{0x3c04, 0x28},
	{0x3c05, 0x98},
	{0x3c07, 0x07},
	{0x3c09, 0xc2},
	{0x3c0a, 0x9c},
	{0x3c0b, 0x40},
	{0x3c01, 0x34},
	{0x4001, 0x02},
	{0x4004, 0x02},
	{0x4005, 0x18},
	{0x4300, 0x30},
	{0x4514, 0x00},
	{0x4520, 0xb0},
	{0x460b, 0x37},
	{0x460c, 0x20},
	// MIPI timing
	{0x4818, 0x01},
	{0x481d, 0xf0},
	{0x481f, 0x50},
	{0x4823, 0x70},
	{0x4831, 0x14},
	{0x4837, 0x10},
	// BLC start line
	// B0xline, 0xnu,mber
	// BLC update by gain change
	// YUV 422, YUYV
	// global timing
	{0x5000, 0xa7}, // Lenc on, raw gamma on, BPC on, WPC on, color interpolation on
	{0x5001, 0x83}, // SDE on, scale off, UV adjust off, color matrix on, AWB on
	{0x501d, 0x00},
	{0x501f, 0x00}, // select ISP YUV 422
	{0x503d, 0x00},
	{0x505c, 0x30},
	// AWB control
	{0x5181, 0x59},
	{0x5183, 0x00},
	{0x5191, 0xf0},
	{0x5192, 0x03},
	// AVG control
	{0x5684, 0x10},
	{0x5685, 0xa0},
	{0x5686, 0x0c},
	{0x5687, 0x78},
	{0x5a00, 0x08},
	{0x5a21, 0x00},
	{0x5a24, 0x00},
	{0x3008, 0x02}, // wake 0xfrom, 0xso,ftware standby
	{0x3503, 0x00}, // AGC auto, AEC auto
	// AWB control
	{0x5180, 0xff},
	{0x5181, 0xf2},
	{0x5182, 0x00},
	{0x5183, 0x14},
	{0x5184, 0x25},
	{0x5185, 0x24},
	{0x5186, 0x09},
	{0x5187, 0x09},
	{0x5188, 0x0a},
	{0x5189, 0x75},
	{0x518a, 0x52},
	{0x518b, 0xea},
	{0x518c, 0xa8},
	{0x518d, 0x42},
	{0x518e, 0x38},
	{0x518f, 0x56},
	{0x5190, 0x42},
	{0x5191, 0xf8},
	{0x5192, 0x04},
	{0x5193, 0x70},
	{0x5194, 0xf0},
	{0x5195, 0xf0},
	{0x5196, 0x03},
	{0x5197, 0x01},
	{0x5198, 0x04},
	{0x5199, 0x12},
	{0x519a, 0x04},
	{0x519b, 0x00},
	{0x519c, 0x06},
	{0x519d, 0x82},
	{0x519e, 0x38},
	// matrix
	{0x5381, 0x1e},
	{0x5382, 0x5b},
	{0x5383, 0x08},
	{0x5384, 0x0b},
	{0x5385, 0x84},
	{0x5386, 0x8f},
	{0x5387, 0x82},
	{0x5388, 0x71},
	{0x5389, 0x11},
	{0x538a, 0x01},
	{0x538b, 0x98},
	// CIP
	{0x5300, 0x08},
	{0x5301, 0x30},
	{0x5302, 0x10},
	{0x5303, 0x00},
	{0x5304, 0x08},
	{0x5305, 0x30},
	{0x5306, 0x08},
	{0x5307, 0x16},
	{0x5309, 0x08},
	{0x530a, 0x30},
	{0x530b, 0x04},
	{0x530c, 0x06},
	// Gamma
	{0x5480, 0x01},
	{0x5481, 0x0e},
	{0x5482, 0x18},
	{0x5483, 0x2b},
	{0x5484, 0x52},
	{0x5485, 0x65},
	{0x5486, 0x71},
	{0x5487, 0x7d},
	{0x5488, 0x87},
	{0x5489, 0x91},
	{0x548a, 0x9a},
	{0x548b, 0xaa},
	{0x548c, 0xb8},
	{0x548d, 0xcd},
	{0x548e, 0xdd},
	{0x548f, 0xea},
	{0x5490, 0x1d},
	{0x5580, 0x06},
	{0x5583, 0x40},
	{0x5584, 0x30},
	{0x5589, 0x10},
	{0x558a, 0x00},
	{0x558b, 0xf8},
	// LENC
	{0x5800, 0x3f},
	{0x5801, 0x16},
	{0x5802, 0x0e},
	{0x5803, 0x0d},
	{0x5804, 0x17},
	{0x5805, 0x3f},
	{0x5806, 0x0b},
	{0x5807, 0x06},
	{0x5808, 0x04},
	{0x5809, 0x04},
	{0x580a, 0x06},
	{0x580b, 0x0b},
	{0x580c, 0x09},
	{0x580d, 0x03},
	{0x580e, 0x00},
	{0x580f, 0x00},
	{0x5810, 0x03},
	{0x5811, 0x08},
	{0x5812, 0x0a},
	{0x5813, 0x03},
	{0x5814, 0x00},
	{0x5815, 0x00},
	{0x5816, 0x04},
	{0x5817, 0x09},
	{0x5818, 0x0f},
	{0x5819, 0x08},
	{0x581a, 0x06},
	{0x581b, 0x06},
	{0x581c, 0x08},
	{0x581d, 0x0c},
	{0x581e, 0x3f},
	{0x581f, 0x1e},
	{0x5820, 0x12},
	{0x5821, 0x13},
	{0x5822, 0x21},
	{0x5823, 0x3f},
	{0x5824, 0x68},
	{0x5825, 0x28},
	{0x5826, 0x2c},
	{0x5827, 0x28},
	{0x5828, 0x08},
	{0x5829, 0x48},
	{0x582a, 0x64},
	{0x582b, 0x62},
	{0x582c, 0x64},
	{0x582d, 0x28},
	{0x582e, 0x46},
	{0x582f, 0x62},
	{0x5830, 0x60},
	{0x5831, 0x62},
	{0x5832, 0x26},
	{0x5833, 0x48},
	{0x5834, 0x66},
	{0x5835, 0x44},
	{0x5836, 0x64},
	{0x5837, 0x28},
	{0x5838, 0x66},
	{0x5839, 0x48},
	{0x583a, 0x2c},
	{0x583b, 0x28},
	{0x583c, 0x26},
	{0x583d, 0xae},
	{0x5025, 0x00}, // AEC in H
	{0x3a0f, 0x38}, // AEC in L
	{0x3a10, 0x30}, // AEC out H
	{0x3a1b, 0x38}, // AEC out L
	{0x3a1e, 0x30}, // control zone H
	{0x3a11, 0x70}, // control zone L
	{0x3a1f, 0x18}, // wake up from standby
	{0x3008, 0x02},
	// DPC Setting //
	{0x5780, 0xfc}, // default value
	{0x5781, 0x13}, // default value
	{0x5782, 0x03}, // default 8
	{0x5786, 0x20}, // default 10 -- add
	{0x5787, 0x40}, // default 10
	{0x5788, 0x08},
	// white pixel threshold
	{0x5789, 0x08}, // 1x
	{0x578a, 0x02}, // 8x – set in 5783
	{0x578b, 0x01}, // 12x – set in 5784
	{0x578c, 0x01}, // 128x
	// black pixel threshold
	{0x578d, 0x0c}, // 1x
	{0x578e, 0x02}, // 8x – set in 5783
	{0x578f, 0x01}, // 12x – set in 5784

	ENDMARKER,
};
#if 0
static const struct regval_list ov5645_qvga_regs[] = {
	ENDMARKER,
};

#endif
static const struct regval_list ov5645_vga_regs[] = {
	//Sysclk = 42Mhz, MIPI 2 lane 168MBps
	//0x3612, 0xa9,
	{0x3618, 0x00},
	{0x3035, 0x21},
	{0x3036, 0x54},
	{0x3600, 0x09},
	{0x3601, 0x43},
	{0x3708, 0x66},
	{0x370c, 0xc3},
	{0x3803, 0xfa},// VS L
	{0x3806, 0x06},// VH = 1705
	{0x3807, 0xa9},// VH
	{0x3808, 0x02},// DVPHO = 640
	{0x3809, 0x80},// DVPHO
	{0x380a, 0x01},// DVPVO = 480
	{0x380b, 0xe0},// DVPVO
	{0x380c, 0x07},// HTS = 1892
	{0x380d, 0x64},// HTS
	{0x380e, 0x02},// VTS = 740
	{0x380f, 0xe4},// VTS
	{0x3814, 0x31},// X INC
	{0x3815, 0x31},// X INC
	{0x3820, 0x41},// flip off, V bin on
	{0x3821, 0x07},// mirror on, H bin on
	{0x4514, 0x00},

	{0x3a02, 0x02},// night mode ceiling = 740
	{0x3a03, 0xe4},// night mode ceiling
	{0x3a08, 0x00},// B50 = 222
	{0x3a09, 0xde},// B50
	{0x3a0a, 0x00},// B60 = 185
	{0x3a0b, 0xb9},// B60
	{0x3a0e, 0x03},// max 50
	{0x3a0d, 0x04},// max 60
	{0x3a14, 0x02},//max 50hz exposure = 3/100
	{0x3a15, 0x9a},// max 50hz exposure
	{0x3a18, 0x01},// max gain = 31.5x
	{0x3a19, 0xf8},// max gain
	{0x4004, 0x02},// BLC line number
	{0x4005, 0x18},// BLC update by gain change
	{0x4837, 0x16},// MIPI global timing
	{0x3503, 0x00},// AGC/AEC on

	ENDMARKER,
};
static const struct regval_list ov5645_720p_regs[] = {

	//Sysclk = 42Mhz, MIPI 2 lane 168MBps
	//0x3612, 0xa9,
	{0x3618, 0x00},
	{0x3035, 0x21},
	{0x3036, 0x54},
	{0x3600, 0x09},
	{0x3601, 0x43},
	{0x3708, 0x66},
	{0x370c, 0xc3},
	{0x3803, 0xfa},// VS L
	{0x3806, 0x06},// VH = 1705
	{0x3807, 0xa9},// VH
	{0x3808, 0x05},// DVPHO = 1280
	{0x3809, 0x00},// DVPHO
	{0x380a, 0x02},// DVPVO = 720
	{0x380b, 0xd0},// DVPVO
	{0x380c, 0x07},// HTS = 1892
	{0x380d, 0x64},// HTS
	{0x380e, 0x02},// VTS = 740
	{0x380f, 0xe4},// VTS
	{0x3814, 0x31},// X INC
	{0x3815, 0x31},// X INC
	{0x3820, 0x41},// flip off, V bin on
	{0x3821, 0x07},// mirror on, H bin on
	{0x4514, 0x00},

	{0x3a02, 0x02},// night mode ceiling = 740
	{0x3a03, 0xe4},// night mode ceiling
	{0x3a08, 0x00},// B50 = 222
	{0x3a09, 0xde},// B50
	{0x3a0a, 0x00},// B60 = 185
	{0x3a0b, 0xb9},// B60
	{0x3a0e, 0x03},// max 50
	{0x3a0d, 0x04},// max 60
	{0x3a14, 0x02},//max 50hz exposure = 3/100
	{0x3a15, 0x9a},// max 50hz exposure
	{0x3a18, 0x01},// max gain = 31.5x
	{0x3a19, 0xf8},// max gain
	{0x4004, 0x02},// BLC line number
	{0x4005, 0x18},// BLC update by gain change
	{0x4837, 0x16},// MIPI global timing
	{0x3503, 0x00},// AGC/AEC on
	ENDMARKER,
};

static const struct regval_list ov5645_960p_regs[] = {
	// Sysclk = 56Mhz, MIPI 2 lane 224MBps
	//0x3612, 0xa9,
	{0x3618, 0x00},
	{0x3035, 0x21},
	{0x3036, 0x70},
	{0x3600, 0x09},
	{0x3601, 0x43},
	{0x3708, 0x66},
	{0x370c, 0xc3},
	{0x3803, 0x06},
	{0x3806, 0x07},
	{0x3807, 0x9d},
	{0x3808, 0x05},
	{0x3809, 0x00},
	{0x380a, 0x03},
	{0x380b, 0xc0},
	{0x380c, 0x07},
	{0x380d, 0x68},
	{0x380e, 0x03},
	{0x380f, 0xd8},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3820, 0x41},
	{0x3821, 0x07},
	{0x4514, 0x00},
	{0x3a02, 0x07},
	{0x3a03, 0xb0},
	{0x3a08, 0x01},
	{0x3a09, 0x27},
	{0x3a0a, 0x00},
	{0x3a0b, 0xf6},
	{0x3a0e, 0x03},
	{0x3a0d, 0x04},
	{0x3a14, 0x08},
	{0x3a15, 0x11},
	{0x3a18, 0x01},
	{0x3a19, 0xf8},
	{0x4004, 0x02},
	{0x4005, 0x18},
	{0x4837, 0x10},
	{0x3503, 0x00},
	ENDMARKER,
};
static const struct regval_list ov5645_5M_regs[] = {
	// Sysclk = 84Mhz, 2 lane mipi, 336MBps
	// //0x3612, 0xab,
	{0x3618, 0x04},
	{0x3035, 0x11},
	{0x3036, 0x54},
	{0x3600, 0x08},
	{0x3601, 0x33},
	{0x3708, 0x63},
	{0x370c, 0xc0},
	{0x3803, 0x00},// VS L
	{0x3806, 0x07},// VH = 1951
	{0x3807, 0x9f},// VH
	{0x3808, 0x0a},// DVPHO = 2592
	{0x3809, 0x20},// DVPHO
	{0x380a, 0x07},// DVPVO = 1944
	{0x380b, 0x98},// DVPVO
	{0x380c, 0x0b},// HTS = 2844
	{0x380d, 0x1c},// HTS
	{0x380e, 0x07},// VTS = 1968
	{0x380f, 0xb0},// VTS
	{0x3814, 0x11},// X INC
	{0x3815, 0x11},// Y INC

	{0x3820, 0x40},// flip off, V bin on
	{0x3821, 0x06},// mirror on, H bin on
	{0x4514, 0x00},

	{0x3a02, 0x07},// night mode ceiling = 1968
	{0x3a03, 0xb0},// night mode ceiling
	{0x3a08, 0x01},// B50
	{0x3a09, 0x27},// B50
	{0x3a0a, 0x00},// B60
	{0x3a0b, 0xf6},// B60
	{0x3a0e, 0x06},// max 50
	{0x3a0d, 0x08},// max 60
	{0x3a14, 0x07},// 50Hz max exposure
	{0x3a15, 0xb0},// 50Hz max exposure
	{0x3a18, 0x00},// max gain = 15.5x
	{0x3a19, 0xf8},// max gain
	{0x4004, 0x06},// BLC line number
	{0x4005, 0x1a},// BLC update every frame
	{0x4837, 0x0b},// MIPI global timing
	{0x3503, 0x03},// AGC/AEC off
	ENDMARKER,
};
#if 0
static const struct regval_list ov5645_wb_auto_regs[] = {
	ENDMARKER,
};

static const struct regval_list ov5645_wb_incandescence_regs[] = {
	ENDMARKER,
};

static const struct regval_list ov5645_wb_daylight_regs[] = {
	ENDMARKER,
};

static const struct regval_list ov5645_wb_fluorescent_regs[] = {
	ENDMARKER,
};

static const struct regval_list ov5645_wb_cloud_regs[] = {
	ENDMARKER,
};

static const struct mode_list ov5645_balance[] = {
	{0, ov5645_wb_auto_regs}, {1, ov5645_wb_incandescence_regs},
	{2, ov5645_wb_daylight_regs}, {3, ov5645_wb_fluorescent_regs},
	{4, ov5645_wb_cloud_regs},
};
#endif
#if 0
static const struct regval_list ov5645_effect_normal_regs[] = {
	ENDMARKER,
};

static const struct regval_list ov5645_effect_grayscale_regs[] = {

	ENDMARKER,
};

static const struct regval_list ov5645_effect_sepia_regs[] = {

	ENDMARKER,
};

static const struct regval_list ov5645_effect_colorinv_regs[] = {

	ENDMARKER,
};

static const struct regval_list ov5645_effect_sepiabluel_regs[] = {

	ENDMARKER,
};

static const struct mode_list ov5645_effect[] = {
	{0, ov5645_effect_normal_regs}, {1, ov5645_effect_grayscale_regs},
	{2, ov5645_effect_sepia_regs}, {3, ov5645_effect_colorinv_regs},
	{4, ov5645_effect_sepiabluel_regs},
};
#endif

#define OV5645_SIZE(n, w, h, r) \
	{.name = n, .width = w , .height = h, .regs = r }

static struct ov5645_win_size ov5645_supported_win_sizes[] = {
	OV5645_SIZE("5M", W_5M, H_5M, ov5645_5M_regs),
	OV5645_SIZE("960P", W_960p, H_960p, ov5645_960p_regs),
	OV5645_SIZE("720P", W_720P, H_720P, ov5645_720p_regs),
	OV5645_SIZE("VGA", W_VGA, H_VGA, ov5645_vga_regs),
//	OV5645_SIZE("QVGA", W_QVGA, H_QVGA, ov5645_qvga_regs),
};

#define N_WIN_SIZES (ARRAY_SIZE(ov5645_supported_win_sizes))

static const struct regval_list ov5645_yuv422_regs[] = {
	ENDMARKER,
};

static const struct regval_list ov5645_rgb565_regs[] = {
	ENDMARKER,
};

static enum v4l2_mbus_pixelcode ov5645_codes[] = {
	V4L2_MBUS_FMT_YUYV8_2X8,
	//V4L2_MBUS_FMT_YUYV8_1_5X8,
};

/*
 * Supported balance menus
 */
static const struct v4l2_querymenu ov5645_balance_menus[] = {
	{
		.id		= V4L2_CID_PRIVATE_BALANCE,
		.index		= 0,
		.name		= "auto",
	}, {
		.id		= V4L2_CID_PRIVATE_BALANCE,
		.index		= 1,
		.name		= "incandescent",
	}, {
		.id		= V4L2_CID_PRIVATE_BALANCE,
		.index		= 2,
		.name		= "fluorescent",
	},  {
		.id		= V4L2_CID_PRIVATE_BALANCE,
		.index		= 3,
		.name		= "daylight",
	},  {
		.id		= V4L2_CID_PRIVATE_BALANCE,
		.index		= 4,
		.name		= "cloudy-daylight",
	},

};

/*
 * Supported effect menus
 */
static const struct v4l2_querymenu ov5645_effect_menus[] = {
	{
		.id		= V4L2_CID_PRIVATE_EFFECT,
		.index		= 0,
		.name		= "none",
	}, {
		.id		= V4L2_CID_PRIVATE_EFFECT,
		.index		= 1,
		.name		= "mono",
	}, {
		.id		= V4L2_CID_PRIVATE_EFFECT,
		.index		= 2,
		.name		= "sepia",
	},  {
		.id		= V4L2_CID_PRIVATE_EFFECT,
		.index		= 3,
		.name		= "negative",
	}, {
		.id		= V4L2_CID_PRIVATE_EFFECT,
		.index		= 4,
		.name		= "aqua",
	},
};

/*
 * General functions
 */
static struct ov5645_priv *to_ov5645(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct ov5645_priv,
			    subdev);
}

static int ov5645_write_array(struct i2c_client *client,
			      const struct regval_list *vals)
{
	int ret;

	while ((vals->reg_num != 0xff) || (vals->value != 0xff)) {
		ret = ov5645_write(client, vals->reg_num, vals->value);
		if (ret < 0)
			return ret;
		vals++;
	}
	return 0;
}


static int ov5645_mask_set(struct i2c_client *client,
			   u16  reg, u16  mask, u16  set)
{
	char val = 0;
	ov5645_read(client, reg, &val);
	if (val < 0)
		return val;

	val &= ~mask;
	val |= set & mask;

	dev_vdbg(&client->dev, "masks: 0x%02x, 0x%02x", reg, val);

	return ov5645_write(client, reg, val);
}

/*
 * soc_camera_ops functions
 */
static int ov5645_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	if (enable) {
		ov5645_write(client, 0x4202, 0x00);
		printk("ov5645 stream on\n");
		return 0;
	}

	ov5645_write(client, 0x4202, 0x0f);
	printk("ov5645 stream off\n");
	return 0;
}

static int ov5645_queryctrl(struct v4l2_subdev *sd,	struct v4l2_queryctrl *qc)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

static int ov5645_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	return 0;
}

/* FIXME: Flip function should be update according to specific sensor */
static int ov5645_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	return 0;
}

static int ov5645_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	printk("functiong:%s, line:%d\n", __func__, __LINE__);

	return v4l2_chip_ident_i2c_client(client, chip, 123, 0);

}



static int ov5645_detect(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char h, l;
	int ret;
	ret = ov5645_read(client, 0x300a, &h);
	if (ret < 0)
		return ret;
	if (h != CHIP_ID_HIGH){
		dev_err(&client->dev, "read sensor %s chip_id high %x is error\n", client->name, h);
		return -ENODEV;
	}
	ret = ov5645_read(client, 0x300b, &l);
	if (ret < 0)
		return ret;
	if (l != CHIP_ID_LOW){
		dev_err(&client->dev, "read sensor %s chip_id high %x is error\n", client->name, l);
		return -ENODEV;
	}
	dev_info(&client->dev, "read sensor %s id high:0x%x,low:0x%x successed!\n",
			        client->name, h, l);
	return 0;
}

static int ov5645_querymenu(struct v4l2_subdev *sd,
					struct v4l2_querymenu *qm)
{
	switch (qm->id) {
	case V4L2_CID_PRIVATE_BALANCE:
		memcpy(qm->name, ov5645_balance_menus[qm->index].name,
				sizeof(qm->name));
		break;

	case V4L2_CID_PRIVATE_EFFECT:
		memcpy(qm->name, ov5645_effect_menus[qm->index].name,
				sizeof(qm->name));
		break;
	}

	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int ov5645_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ret = ov5645_read(client, reg->reg & 0xffff, &val);
	reg->val = val;
	reg->size = 2;
	return ret;
}

static int ov5645_s_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	ov5645_write(client, reg->reg & 0xffff, reg->val & 0xff);
	return 0;
}

#endif

/* Select the nearest higher resolution for capture */
static struct ov5645_win_size *ov5645_select_win(u32 *width, u32 *height)
{
	int i, default_size = ARRAY_SIZE(ov5645_supported_win_sizes) - 1;

	for (i = 0; i < ARRAY_SIZE(ov5645_supported_win_sizes); i++) {
		if ((*width >= ov5645_supported_win_sizes[i].width) &&
		    (*height >= ov5645_supported_win_sizes[i].height)) {
			*width = ov5645_supported_win_sizes[i].width;
			*height = ov5645_supported_win_sizes[i].height;
			return &ov5645_supported_win_sizes[i];
		}
	}

	*width = ov5645_supported_win_sizes[default_size].width;
	*height = ov5645_supported_win_sizes[default_size].height;


	return &ov5645_supported_win_sizes[default_size];
}

static int ov5645_g_mbus_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *mf)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov5645_priv *priv = to_ov5645(client);

	mf->width = priv->win->width;
	mf->height = priv->win->height;
	mf->code = priv->cfmt_code;

	mf->colorspace = V4L2_COLORSPACE_JPEG;
	mf->field	= V4L2_FIELD_NONE;

	return 0;
}

static int ov5645_get_sensor_vts(struct v4l2_subdev *sd, unsigned short *value)
{
	unsigned char h,l;
	int ret = 0;
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	ret = ov5645_read(client, 0x380e, &h);
	if (ret < 0)
		return ret;
	ret = ov5645_read(client, 0x380f, &l);
	if (ret < 0)
		return ret;
	*value = h;
	*value = (*value << 8) | l;
	return ret;
}

static int ov5645_get_sensor_lans(struct v4l2_subdev *sd, unsigned char *value)
{
	int ret = 0;
	unsigned char v = 0;
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	ret = ov5645_read(client, 0x300e, &v);
	if (ret < 0)
		return ret;
	*value = v >> 5;
	if(*value > 2 || *value < 1)
		ret = -EINVAL;
	return ret;
}

static int ov5645_s_mbus_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *mf)
{
	/* current do not support set format, use unify format yuv422i */
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct ov5645_priv *priv = to_ov5645(client);
	struct v4l2_fmt_data *data = v4l2_get_fmt_data(mf);

	int ret;

	printk("this is %d %s\n\n", __LINE__, __func__);
	priv->win = ov5645_select_win(&mf->width, &mf->height);
	/* set size win */
	ret = ov5645_write_array(client, priv->win->regs);
	if (ret < 0) {
		dev_err(&client->dev, "%s: Error\n", __func__);
		return ret;
	}


	data->i2cflags = V4L2_I2C_ADDR_16BIT;
	data->mipi_clk = 282;
	ret = ov5645_get_sensor_vts(sd, &(data->vts));
	if(ret < 0){
		printk("[ov5645], problem function:%s, line:%d\n", __func__, __LINE__);
		return ret;
	}
	ret = ov5645_get_sensor_lans(sd, &(data->lans));
	if(ret < 0){
		printk("[ov5645], problem function:%s, line:%d\n", __func__, __LINE__);
		return ret;
	}


	return 0;
}

static int ov5645_try_mbus_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *mf)
{
	const struct ov5645_win_size *win;
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	/*
	 * select suitable win
	 */
	printk("this is %d %s\n\n", __LINE__, __func__);
	win = ov5645_select_win(&mf->width, &mf->height);

	if(mf->field == V4L2_FIELD_ANY) {
		mf->field = V4L2_FIELD_NONE;
	} else if (mf->field != V4L2_FIELD_NONE) {
		dev_err(&client->dev, "Field type invalid.\n");
		return -ENODEV;
	}

	switch (mf->code) {
	case V4L2_MBUS_FMT_YUYV8_2X8:
	case V4L2_MBUS_FMT_YUYV8_1_5X8:
	case V4L2_MBUS_FMT_JZYUYV8_1_5X8:
		mf->colorspace = V4L2_COLORSPACE_JPEG;
		break;

	default:
		mf->code = V4L2_MBUS_FMT_YUYV8_2X8;
		mf->colorspace = V4L2_COLORSPACE_JPEG;
		break;
	}

	return 0;
}

static int ov5645_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned int index,enum v4l2_mbus_pixelcode *code)
{
	if (index >= ARRAY_SIZE(ov5645_codes))
		return -EINVAL;

	printk("this is %d %s\n\n", __LINE__, __func__);
	*code = ov5645_codes[index];
	return 0;
}

static int ov5645_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	return 0;
}

static int ov5645_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *a)
{
	return 0;
}

static int ov5645_reset(struct v4l2_subdev *sd, u32 val)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return 0;
}

/*
 * Frame intervals.  Since frame rates are controlled with the clock
 * divider, we can only do 30/n for integer n values.  So no continuous
 * or stepwise options.  Here we just pick a handful of logical values.
 */

static int ov5645_frame_rates[] = { 30, 15, 10, 5, 1 };

static int ov5645_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *interval)
{
	if (interval->index >= ARRAY_SIZE(ov5645_frame_rates))
		return -EINVAL;
	interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	interval->discrete.numerator = 1;
	interval->discrete.denominator = ov5645_frame_rates[interval->index];
	return 0;
}


/*
 * Frame size enumeration
 */
static int ov5645_enum_framesizes(struct v4l2_subdev *sd,
		struct v4l2_frmsizeenum *fsize)
{
	int i;
	int num_valid = -1;
	__u32 index = fsize->index;
	printk("this is %d %s\n\n", __LINE__, __func__);
	for (i = 0; i < N_WIN_SIZES; i++) {
		struct ov5645_win_size *win = &ov5645_supported_win_sizes[index];
		if (index == ++num_valid) {
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = win->width;
			fsize->discrete.height = win->height;
			return 0;
		}
	}

	return -EINVAL;
}

static int ov5645_s_power(struct v4l2_subdev *sd, int on)
{
	int ret;
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	ret = ov5645_write_array(client, ov5645_init_regs);
	if (ret < 0)
		return ret;
	printk("--%s:%d\n", __func__, __LINE__);
	return 0;
}

static int ov5645_init(struct v4l2_subdev *sd, u32 val)
{
	struct	ov5645_priv *priv = container_of(sd, struct ov5645_priv, subdev);
	int ret = 0;
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	priv->win = NULL;
	if (ret < 0)
		return ret;
	return 0;
}

static int ov5645_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	    printk("functiong:%s, line:%d\n", __func__, __LINE__);
		    return 0;
}

static int ov5645_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	    printk("functiong:%s, line:%d\n", __func__, __LINE__);
		    return 0;
}

static const struct v4l2_subdev_core_ops ov5645_core_ops = {
	.g_chip_ident = ov5645_g_chip_ident,
	.g_ctrl = ov5645_g_ctrl,
	.s_ctrl = ov5645_s_ctrl,
	.queryctrl = ov5645_queryctrl,
	.reset = ov5645_reset,
	.init = ov5645_init,
	.s_power = ov5645_s_power,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = ov5645_g_register,
	.s_register = ov5645_s_register,
#endif
};

static const struct v4l2_subdev_video_ops ov5645_video_ops = {
	.s_stream = ov5645_s_stream,
	.g_mbus_fmt = ov5645_g_mbus_fmt,
	.s_mbus_fmt = ov5645_s_mbus_fmt,
	.try_mbus_fmt = ov5645_try_mbus_fmt,
	.cropcap = ov5645_cropcap,
	.g_crop = ov5645_g_crop,
	.enum_framesizes = ov5645_enum_framesizes,
	.enum_frameintervals = ov5645_enum_frameintervals,
	.enum_mbus_fmt = ov5645_enum_mbus_fmt,
	.s_parm = ov5645_s_parm,
	.g_parm = ov5645_g_parm,
};

static unsigned int rg_ratio_typical = 0x58;
static unsigned int bg_ratio_typical = 0x5a;

static struct v4l2_subdev_ops ov5645_subdev_ops = {
	.core	= &ov5645_core_ops,
	.video	= &ov5645_video_ops,
};

static ssize_t ov5645_rg_ratio_typical_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return sprintf(buf, "%d", rg_ratio_typical);
}

static ssize_t ov5645_rg_ratio_typical_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	char *endp;
	int value;
	printk("functiong:%s, line:%d\n", __func__, __LINE__);

	value = simple_strtoul(buf, &endp, 0);
	if (buf == endp)
		return -EINVAL;

	rg_ratio_typical = (unsigned int)value;

	return size;
}

static ssize_t ov5645_bg_ratio_typical_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	return sprintf(buf, "%d", bg_ratio_typical);
}

static ssize_t ov5645_bg_ratio_typical_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	char *endp;
	int value;

	printk("functiong:%s, line:%d\n", __func__, __LINE__);
	value = simple_strtoul(buf, &endp, 0);
	if (buf == endp)
		return -EINVAL;

	bg_ratio_typical = (unsigned int)value;

	return size;
}

static DEVICE_ATTR(ov5645_rg_ratio_typical, 0664, ov5645_rg_ratio_typical_show,
		ov5645_rg_ratio_typical_store);
static DEVICE_ATTR(ov5645_bg_ratio_typical, 0664, ov5645_bg_ratio_typical_show,
		ov5645_bg_ratio_typical_store);

/*
 * i2c_driver functions
 */

static int ov5645_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct ov5645_priv *priv;
	int ret;

	priv = kzalloc(sizeof(struct ov5645_priv), GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;
	sd = &priv->subdev;
	v4l2_i2c_subdev_init(sd, client, &ov5645_subdev_ops);

	/* Make sure it's an ov5645 */
	ret = ov5645_detect(sd);
	if (ret) {
		v4l_err(client,
			"chip found @ 0x%x (%s) is not an ov5645 chip.\n",
			client->addr, client->adapter->name);
		kfree(priv);
		return ret;
	}
	v4l_info(client, "ov5645 chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);

	ret = device_create_file(&client->dev, &dev_attr_ov5645_rg_ratio_typical);
	if(ret){
		v4l_err(client, "create dev_attr_ov5645_rg_ratio_typical failed!\n");
		goto err_create_dev_attr_ov5645_rg_ratio_typical;
	}

	ret = device_create_file(&client->dev, &dev_attr_ov5645_bg_ratio_typical);
	if(ret){
		v4l_err(client, "create dev_attr_ov5645_bg_ratio_typical failed!\n");
		goto err_create_dev_attr_ov5645_bg_ratio_typical;
	}
	printk("probe ok ------->ov5645\n");
	return 0;

err_create_dev_attr_ov5645_bg_ratio_typical:
	device_remove_file(&client->dev, &dev_attr_ov5645_rg_ratio_typical);
err_create_dev_attr_ov5645_rg_ratio_typical:
	kfree(priv);
	return ret;
}

static int ov5645_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov5645_priv *priv = container_of(sd, struct ov5645_priv, subdev);

	device_remove_file(&client->dev, &dev_attr_ov5645_rg_ratio_typical);
	device_remove_file(&client->dev, &dev_attr_ov5645_bg_ratio_typical);

	v4l2_device_unregister_subdev(sd);
	kfree(priv);
	return 0;
}

static const struct i2c_device_id ov5645_id[] = {
	{ "ov5645",  0 },
	{ }
};


MODULE_DEVICE_TABLE(i2c, ov5645_id);

static struct i2c_driver ov5645_i2c_driver = {
	.driver = {
		.name = "ov5645",
	},
	.probe    = ov5645_probe,
	.remove   = ov5645_remove,
	.id_table = ov5645_id,
};

/*
 * Module functions
 */
static int __init ov5645_module_init(void)
{
	return i2c_add_driver(&ov5645_i2c_driver);
}

static void __exit ov5645_module_exit(void)
{
	i2c_del_driver(&ov5645_i2c_driver);
}

module_init(ov5645_module_init);
module_exit(ov5645_module_exit);

MODULE_DESCRIPTION("camera sensor ov5645 driver");
MODULE_AUTHOR("cxtan <cxtan@ingenic.cn>");
MODULE_LICENSE("GPL");
