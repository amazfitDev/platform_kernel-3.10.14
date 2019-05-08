/*
 * gc0308 Camera Driver
 *
 * Copyright (C) 2014, Ingenic Semiconductor Inc.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

//#define DEBUG	1
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-subdev.h>
#include <media/soc_camera.h>
#include <media/soc_mediabus.h>


#define REG_CHIP_ID		0x00
#define PID_GC0308		0x9b

/* Private v4l2 controls */
#define V4L2_CID_PRIVATE_BALANCE  (V4L2_CID_PRIVATE_BASE + 0)
#define V4L2_CID_PRIVATE_EFFECT  (V4L2_CID_PRIVATE_BASE + 1)

#define REG14				0x14
#define REG14_HFLIP_IMG		0x01 /* Horizontal mirror image ON/OFF */
#define REG14_VFLIP_IMG     0x02 /* Vertical flip image ON/OFF */

 /* whether sensor support high resolution (> vga) preview or not */
#define SUPPORT_HIGH_RESOLUTION_PRE		1

/*
 * Struct
 */
struct regval_list {
	u8 reg_num;
	u8 value;
};

struct mode_list {
	u8 index;
	const struct regval_list *mode_regs;
};

/* Supported resolutions */
enum gc0308_width {
	W_QCIF	= 176,
	W_QVGA	= 320,
	W_CIF	= 352,
	W_VGA	= 640,
};

enum gc0308_height {
	H_QCIF	= 144,
	H_QVGA	= 240,
	H_CIF	= 288,
	H_VGA	= 480,
};

struct gc0308_win_size {
	char *name;
	enum gc0308_width width;
	enum gc0308_height height;
	const struct regval_list *regs;
};


struct gc0308_priv {
	struct v4l2_subdev subdev;
	struct gc0308_camera_info *info;
	enum v4l2_mbus_pixelcode cfmt_code;
	const struct gc0308_win_size *win;
	int	model;
	u8 balance_value;
	u8 effect_value;
	u16	flag_vflip:1;
	u16	flag_hflip:1;
};

static inline int gc0308_write_reg(struct i2c_client * client, unsigned char addr, unsigned char value)
{
	return i2c_smbus_write_byte_data(client, addr, value);
}
static inline char gc0308_read_reg(struct i2c_client *client, unsigned char addr)
{
	char ret;
	ret = i2c_smbus_read_byte_data(client, addr);
	if (ret < 0)
		return ret;

	return ret;
}
/*
 * Registers settings
 */

#define ENDMARKER { 0xff, 0xff }

static const struct regval_list gc0308_init_regs[] = {
	{0xfe, 0x80},
	{0xfe, 0x00},	// set page0
	{0xd2, 0x10},	// close AEC
	{0x22, 0x55},	// close AWB

	{0x03, 0x01},
	{0x04, 0x2c},
	{0x5a, 0x56},
	{0x5b, 0x40},
	{0x5c, 0x4a},

	{0x22, 0x57},	// Open AWB
	{0x01, 0xfa},
	{0x02, 0x70},
	{0x0f, 0x01},


	{0xe2, 0x00},	//anti-flicker step [11:8]
	{0xe3, 0x64},	//anti-flicker step [7:0]

	{0xe4, 0x02},	//exp level 1  16.67fps
	{0xe5, 0x58},
	{0xe6, 0x03},	//exp level 2  12.5fps
	{0xe7, 0x20},
	{0xe8, 0x04},	//exp level 3  8.33fps
	{0xe9, 0xb0},
	{0xea, 0x09},	//exp level 4  4.00fps
	{0xeb, 0xc4},

	{0x05, 0x00},
	{0x06, 0x00},
	{0x07, 0x00},
	{0x08, 0x00},
	{0x09, 0x01},
	{0x0a, 0xe8},
	{0x0b, 0x02},
	{0x0c, 0x88},
	{0x0d, 0x02},
	{0x0e, 0x02},
	{0x10, 0x22},
	{0x11, 0xfd},
	{0x12, 0x2a},
	{0x13, 0x00},

	{0x15, 0x0a},
	{0x16, 0x05},
	{0x17, 0x01},
	{0x18, 0x44},
	{0x19, 0x44},
	{0x1a, 0x1e},
	{0x1b, 0x00},
	{0x1c, 0xc1},
	{0x1d, 0x08},
	{0x1e, 0x60},
	{0x1f, 0x03},	//16


	{0x20, 0xff},
	{0x21, 0xf8},
	{0x22, 0x57},
	{0x24, 0xa2},
	{0x25, 0x0f},
	{0x28, 0x11},	//add

	{0x26, 0x03},//	03
	{0x2f, 0x01},
	{0x30, 0xf7},
	{0x31, 0x50},
	{0x32, 0x00},
	{0x39, 0x04},
	{0x3a, 0x18},
	{0x3b, 0x20},
	{0x3c, 0x00},
	{0x3d, 0x00},
	{0x3e, 0x00},
	{0x3f, 0x00},
	{0x50, 0x10},
	{0x53, 0x82},
	{0x54, 0x80},
	{0x55, 0x80},
	{0x56, 0x82},
	{0x8b, 0x40},
	{0x8c, 0x40},
	{0x8d, 0x40},
	{0x8e, 0x2e},
	{0x8f, 0x2e},
	{0x90, 0x2e},
	{0x91, 0x3c},
	{0x92, 0x50},
	{0x5d, 0x12},
	{0x5e, 0x1a},
	{0x5f, 0x24},
	{0x60, 0x07},
	{0x61, 0x15},
	{0x62, 0x08},
	{0x64, 0x03},
	{0x66, 0xe8},
	{0x67, 0x86},
	{0x68, 0xa2},
	{0x69, 0x18},
	{0x6a, 0x0f},
	{0x6b, 0x00},
	{0x6c, 0x5f},
	{0x6d, 0x8f},
	{0x6e, 0x55},
	{0x6f, 0x38},
	{0x70, 0x15},
	{0x71, 0x33},
	{0x72, 0xdc},
	{0x73, 0x80},
	{0x74, 0x02},
	{0x75, 0x3f},
	{0x76, 0x02},
	{0x77, 0x36},
	{0x78, 0x88},
	{0x79, 0x81},
	{0x7a, 0x81},
	{0x7b, 0x22},
	{0x7c, 0xff},
	{0x93, 0x48},
	{0x94, 0x00},
	{0x95, 0x05},
	{0x96, 0xe8},
	{0x97, 0x40},
	{0x98, 0xf0},
	{0xb1, 0x38},
	{0xb2, 0x38},
	{0xbd, 0x38},
	{0xbe, 0x36},
	{0xd0, 0xc9},
	{0xd1, 0x10},

	{0xd3, 0x80},
	{0xd5, 0xf2},
	{0xd6, 0x16},
	{0xdb, 0x92},
	{0xdc, 0xa5},
	{0xdf, 0x23},
	{0xd9, 0x00},
	{0xda, 0x00},
	{0xe0, 0x09},

	{0xed, 0x04},
	{0xee, 0xa0},
	{0xef, 0x40},
	{0x80, 0x03},
	{0x80, 0x03},
	{0x9F, 0x10},
	{0xA0, 0x20},
	{0xA1, 0x38},
	{0xA2, 0x4E},
	{0xA3, 0x63},
	{0xA4, 0x76},
	{0xA5, 0x87},
	{0xA6, 0xA2},
	{0xA7, 0xB8},
	{0xA8, 0xCA},
	{0xA9, 0xD8},
	{0xAA, 0xE3},
	{0xAB, 0xEB},
	{0xAC, 0xF0},
	{0xAD, 0xF8},
	{0xAE, 0xFD},
	{0xAF, 0xFF},
	{0xc0, 0x00},
	{0xc1, 0x10},
	{0xc2, 0x1C},
	{0xc3, 0x30},
	{0xc4, 0x43},
	{0xc5, 0x54},
	{0xc6, 0x65},
	{0xc7, 0x75},
	{0xc8, 0x93},
	{0xc9, 0xB0},
	{0xca, 0xCB},
	{0xcb, 0xE6},
	{0xcc, 0xFF},
	{0xf0, 0x02},
	{0xf1, 0x01},
	{0xf2, 0x01},
	{0xf3, 0x30},
	{0xf9, 0x9f},
	{0xfa, 0x78},

	//---------------------------------------------------------------
	{0xfe, 0x01},// set page1

	{0x00, 0xf5},
	{0x02, 0x1a},
	{0x0a, 0xa0},
	{0x0b, 0x60},
	{0x0c, 0x08},
	{0x0e, 0x4c},
	{0x0f, 0x39},
	{0x11, 0x3f},
	{0x12, 0x72},
	{0x13, 0x13},
	{0x14, 0x42},
	{0x15, 0x43},
	{0x16, 0xc2},
	{0x17, 0xa8},
	{0x18, 0x18},
	{0x19, 0x40},
	{0x1a, 0xd0},
	{0x1b, 0xf5},
	{0x70, 0x40},
	{0x71, 0x58},
	{0x72, 0x30},
	{0x73, 0x48},
	{0x74, 0x20},
	{0x75, 0x60},
	{0x77, 0x20},
	{0x78, 0x32},
	{0x30, 0x03},
	{0x31, 0x40},
	{0x32, 0xe0},
	{0x33, 0xe0},
	{0x34, 0xe0},
	{0x35, 0xb0},
	{0x36, 0xc0},
	{0x37, 0xc0},
	{0x38, 0x04},
	{0x39, 0x09},
	{0x3a, 0x12},
	{0x3b, 0x1C},
	{0x3c, 0x28},
	{0x3d, 0x31},
	{0x3e, 0x44},
	{0x3f, 0x57},
	{0x40, 0x6C},
	{0x41, 0x81},
	{0x42, 0x94},
	{0x43, 0xA7},
	{0x44, 0xB8},
	{0x45, 0xD6},
	{0x46, 0xEE},
	{0x47, 0x0d},

	//Registers of Page0
	{0xfe, 0x00}, // set page0
	{0x10, 0x26},
	{0x11, 0x0d},  // fd,modified by mormo 2010/07/06
	{0x1a, 0x2a},  // 1e,modified by mormo 2010/07/06

	{0x1c, 0x49}, // c1,modified by mormo 2010/07/06
	{0x1d, 0x9a}, // 08,modified by mormo 2010/07/06
	{0x1e, 0x61}, // 60,modified by mormo 2010/07/06

	{0x3a, 0x20},

	{0x50, 0x14},  // 10,modified by mormo 2010/07/06
	{0x53, 0x80},
	{0x56, 0x80},

	{0x8b, 0x20}, //LSC
	{0x8c, 0x20},
	{0x8d, 0x20},
	{0x8e, 0x14},
	{0x8f, 0x10},
	{0x90, 0x14},

	{0x94, 0x02},
	{0x95, 0x07},
	{0x96, 0xe0},

	{0xb1, 0x40}, // YCPT
	{0xb2, 0x40},
	{0xb3, 0x40},
	{0xb6, 0xe0},

	{0xd0, 0xcb}, // AECT  c9,modifed by mormo 2010/07/06
	{0xd3, 0x48}, // 80,modified by mormor 2010/07/06
	{0xf2, 0x02},
	{0xf7, 0x12},
	{0xf8, 0x0a},

	//Registers of Page1
	{0xfe, 0x01},// set page1
	{0x02, 0x20},
	{0x04, 0x10},
	{0x05, 0x08},
	{0x06, 0x20},
	{0x08, 0x0a},

	{0x0e, 0x44},
	{0x0f, 0x32},
	{0x10, 0x41},
	{0x11, 0x37},
	{0x12, 0x22},
	{0x13, 0x19},
	{0x14, 0x44},
	{0x15, 0x44},

	{0x19, 0x50},
	{0x1a, 0xd8},

	{0x32, 0x10},

	{0x35, 0x00},
	{0x36, 0x80},
	{0x37, 0x00},
	//-----------Update the registers end---------//

	{0xfe, 0x00}, // set page0
	{0xd2, 0x90},

	//-----------GAMMA Select(3)---------------//
	{0x9F, 0x10},
	{0xA0, 0x20},
	{0xA1, 0x38},
	{0xA2, 0x4E},
	{0xA3, 0x63},
	{0xA4, 0x76},
	{0xA5, 0x87},
	{0xA6, 0xA2},
	{0xA7, 0xB8},
	{0xA8, 0xCA},
	{0xA9, 0xD8},
	{0xAA, 0xE3},
	{0xAB, 0xEB},
	{0xAC, 0xF0},
	{0xAD, 0xF8},
	{0xAE, 0xFD},
	{0xAF, 0xFF},

	{0x14, 0x10},
//	{0x14 ,0x12},
//	{0x14, 0x11},
//	{0x14 ,0x13},


	/*set_antibanding*/
	{0x01,0x6a},
	{0x02,0x70},
	{0x0f,0x00},
	{0xe2,0x00},

	{0xe3,0x96},
	{0xe4,0x02},
	{0xe5,0x58},
	{0xe6,0x02},
	{0xe7,0x58},

	{0xe8,0x02},
	{0xe9,0x58},
	{0xea,0x0b},
	{0xeb,0xb8},

	ENDMARKER,
};


static const struct regval_list gc0308_qcif_regs[] = {
	{0xfe, 0x01}, {0x54, 0x33},
	{0x55, 0x03}, {0x56, 0x00},
	{0x57, 0x00}, {0x58, 0x00},
	{0x59, 0x00}, {0xfe, 0x00},
	{0x46, 0x80}, {0x47, 0x00},
	{0x48, 0x00}, {0x49, 0x00},
	{0x4a, 0x90}, {0x4b, 0x00},
	{0x4c, 0xb0},
	ENDMARKER,
};

static const struct regval_list gc0308_qvga_regs[] = {
	{0xfe, 0x01}, {0x54, 0x55},
	{0x55, 0x03}, {0x56, 0x02},
	{0x57, 0x04}, {0x58, 0x02},
	{0x59, 0x04}, {0xfe, 0x00},
	{0x46, 0x80}, {0x47, 0x00},
	{0x48, 0x00}, {0x49, 0x00},
	{0x4a, 0xf0}, {0x4b, 0x01},
	{0x4c, 0x40},
	ENDMARKER,
};

static const struct regval_list gc0308_cif_regs[] = {
	{0xfe, 0x01}, {0x54, 0x55},
	{0x55, 0x03}, {0x56, 0x02},
	{0x57, 0x04}, {0x58, 0x02},
	{0x59, 0x04}, {0xfe, 0x00},
	{0x46, 0x80}, {0x47, 0x00},
	{0x48, 0x00}, {0x49, 0x01},
	{0x4a, 0x20}, {0x4b, 0x01},
	{0x4c, 0x60},
	ENDMARKER,
};

static const struct regval_list gc0308_vga_regs[] = {
	{0xfe, 0x01}, {0x54, 0x11},
	{0x55, 0x03}, {0x56, 0x00},
	{0x57, 0x00}, {0x58, 0x00},
	{0x59, 0x00}, {0xfe, 0x00},
	{0x46, 0x00},
	ENDMARKER,
};

static const struct regval_list gc0308_wb_auto_regs[] = {
	{0x5a,0x56}, {0x5b,0x40},
	{0x5c,0x4a}, {0x22,0x57},
	ENDMARKER,
};

static const struct regval_list gc0308_wb_incandescence_regs[] = {
	{0x22,0x55}, {0x5a,0x48},
	{0x5b,0x40}, {0x5c,0x5c},
	ENDMARKER,
};

static const struct regval_list gc0308_wb_daylight_regs[] = {
	{0x22,0x55}, {0x5a,0x74},
	{0x5b,0x52}, {0x5c,0x40},
	ENDMARKER,
};

static const struct regval_list gc0308_wb_fluorescent_regs[] = {
	{0x22,0x55}, {0x5a,0x40},
	{0x5b,0x42}, {0x5c,0x50},
	ENDMARKER,
};

static const struct regval_list gc0308_wb_cloud_regs[] = {
	{0x22,0x55}, {0x5a,0x8c},
	{0x5b,0x50}, {0x5c,0x40},
	ENDMARKER,
};

static const struct mode_list gc0308_balance[] = {
	{0, gc0308_wb_auto_regs}, {1, gc0308_wb_incandescence_regs},
	{2, gc0308_wb_daylight_regs}, {3, gc0308_wb_fluorescent_regs},
	{4, gc0308_wb_cloud_regs},
};


static const struct regval_list gc0308_effect_normal_regs[] = {
	{0x23,0x00}, {0x2d,0x0a},
	{0x20,0x7f}, {0xd2,0x90},
	{0x73,0x00}, {0x77,0x38},
	{0xb3,0x40}, {0xb4,0x80},
	{0xba,0x00}, {0xbb,0x00},
	ENDMARKER,
};

static const struct regval_list gc0308_effect_grayscale_regs[] = {
	{0x23,0x02}, {0x2d,0x0a},
	{0x20,0xff}, {0xd2,0x90},
	{0x73,0x00}, {0xb3,0x40},
	{0xb4,0x80}, {0xba,0x00},
	{0xbb,0x00},
	ENDMARKER,
};

static const struct regval_list gc0308_effect_sepia_regs[] = {
	{0x23,0x02}, {0x2d,0x0a},
	{0x20,0xff}, {0xd2,0x90},
	{0x73,0x00}, {0xb3,0x40},
	{0xb4,0x80}, {0xba,0xd0},
	{0xbb,0x28},
	ENDMARKER,
};

static const struct regval_list gc0308_effect_colorinv_regs[] = {
	{0x23,0x01}, {0x2d,0x0a},
	{0x20,0xff}, {0xd2,0x90},
	{0x73,0x00}, {0xb3,0x40},
	{0xb4,0x80}, {0xba,0x00},
	{0xbb,0x00},
	ENDMARKER,
};

static const struct regval_list gc0308_effect_sepiabluel_regs[] = {
	{0x23,0x02}, {0x2d,0x0a},
	{0x20,0x7f}, {0xd2,0x90},
	{0x73,0x00}, {0xb3,0x40},
	{0xb4,0x80}, {0xba,0x50},
	{0xbb,0xe0},
	ENDMARKER,
};

static const struct mode_list gc0308_effect[] = {
	{0, gc0308_effect_normal_regs}, {1, gc0308_effect_grayscale_regs},
	{2, gc0308_effect_sepia_regs}, {3, gc0308_effect_colorinv_regs},
	{4, gc0308_effect_sepiabluel_regs},
};

#define GC0308_SIZE(n, w, h, r) \
	{.name = n, .width = w , .height = h, .regs = r }

static const struct gc0308_win_size gc0308_supported_win_sizes[] = {
	GC0308_SIZE("QCIF", W_QCIF, H_QCIF, gc0308_qcif_regs),
	GC0308_SIZE("QVGA", W_QVGA, H_QVGA, gc0308_qvga_regs),
	GC0308_SIZE("CIF", W_CIF, H_CIF, gc0308_cif_regs),
	GC0308_SIZE("VGA", W_VGA, H_VGA, gc0308_vga_regs),
};

#define N_WIN_SIZES (ARRAY_SIZE(gc0308_supported_win_sizes))

static const struct regval_list gc0308_yuv422_regs[] = {
	ENDMARKER,
};

static const struct regval_list gc0308_rgb565_regs[] = {
	ENDMARKER,
};

static enum v4l2_mbus_pixelcode gc0308_codes[] = {
	V4L2_MBUS_FMT_YUYV8_2X8,
	V4L2_MBUS_FMT_YUYV8_1_5X8,
	V4L2_MBUS_FMT_JZYUYV8_1_5X8,
};

/*
 * Supported controls
 */
static const struct v4l2_queryctrl gc0308_controls[] = {
	{
		.id		= V4L2_CID_PRIVATE_BALANCE,
		.type		= V4L2_CTRL_TYPE_MENU,
		.name		= "whitebalance",
		.minimum	= 0,
		.maximum	= 4,
		.step		= 1,
		.default_value	= 0,
	}, {
		.id		= V4L2_CID_PRIVATE_EFFECT,
		.type		= V4L2_CTRL_TYPE_MENU,
		.name		= "effect",
		.minimum	= 0,
		.maximum	= 4,
		.step		= 1,
		.default_value	= 0,
	},
	{
		.id		= V4L2_CID_VFLIP,
		.type		= V4L2_CTRL_TYPE_BOOLEAN,
		.name		= "Flip Vertically",
		.minimum	= 0,
		.maximum	= 1,
		.step		= 1,
		.default_value	= 0,
	}, {
		.id		= V4L2_CID_HFLIP,
		.type		= V4L2_CTRL_TYPE_BOOLEAN,
		.name		= "Flip Horizontally",
		.minimum	= 0,
		.maximum	= 1,
		.step		= 1,
		.default_value	= 0,
	},
};

/*
 * Supported balance menus
 */
static const struct v4l2_querymenu gc0308_balance_menus[] = {
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
static const struct v4l2_querymenu gc0308_effect_menus[] = {
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
static struct gc0308_priv *to_gc0308(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct gc0308_priv,
			    subdev);
}

static int gc0308_write_array(struct i2c_client *client,
			      const struct regval_list *vals)
{
	int ret;

	while ((vals->reg_num != 0xff) || (vals->value != 0xff)) {
		ret = i2c_smbus_write_byte_data(client,
						vals->reg_num, vals->value);
		dev_vdbg(&client->dev, "array: 0x%02x, 0x%02x",
			 vals->reg_num, vals->value);

		if (ret < 0)
			return ret;
		vals++;
	}
	return 0;
}

static int gc0308_mask_set(struct i2c_client *client,
			   u8  reg, u8  mask, u8  set)
{
	s32 val = i2c_smbus_read_byte_data(client, reg);
	if (val < 0)
		return val;

	val &= ~mask;
	val |= set & mask;

	dev_vdbg(&client->dev, "masks: 0x%02x, 0x%02x", reg, val);

	return i2c_smbus_write_byte_data(client, reg, val);
}

 /* current use hardware reset */
#if 0
static int gc0308_reset(struct i2c_client *client)
{
	int ret;
	ret = i2c_smbus_write_byte_data(client, 0xfe, 0x80);
	if (ret)
		goto err;

	msleep(5);
err:
	dev_dbg(&client->dev, "%s: (ret %d)", __func__, ret);
	return ret;
}
#endif

/*
 * soc_camera_ops functions
 */
static int gc0308_s_stream(struct v4l2_subdev *sd, int enable)
{
	return 0;
}

#if 0
static int gc0308_set_bus_param(struct soc_camera_device *icd,
				unsigned long flags)
{
	struct soc_camera_link *icl = to_soc_camera_link(icd);
	unsigned long width_flag = flags & SOCAM_DATAWIDTH_MASK;

	/* Only one width bit may be set */
	if (!is_power_of_2(width_flag))
		return -EINVAL;

	if (icl->set_bus_param)
		return icl->set_bus_param(icl, width_flag);

	/*
	 * Without board specific bus width settings we support only the
	 * sensors native bus width which are tested working
	 */
	if (width_flag & SOCAM_DATAWIDTH_8)
		return 0;

	return 0;
}

static unsigned long gc0308_query_bus_param(struct soc_camera_device *icd)
{
	struct soc_camera_link *icl = to_soc_camera_link(icd);
	unsigned long flags = SOCAM_PCLK_SAMPLE_FALLING | SOCAM_MASTER |
		SOCAM_VSYNC_ACTIVE_HIGH | SOCAM_HSYNC_ACTIVE_HIGH |
		SOCAM_DATA_ACTIVE_HIGH;

	if (icl->query_bus_param)
		flags |= icl->query_bus_param(icl) & SOCAM_DATAWIDTH_MASK;
	else
		flags |= SOCAM_DATAWIDTH_8;

	return soc_camera_apply_sensor_flags(icl, flags);
}
#endif
static int gc0308_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct gc0308_priv *priv = to_gc0308(client);

	switch (ctrl->id) {
	case V4L2_CID_VFLIP:
		ctrl->value = priv->flag_vflip;
		break;
	case V4L2_CID_HFLIP:
		ctrl->value = priv->flag_hflip;
		break;
	case V4L2_CID_PRIVATE_BALANCE:
		ctrl->value = priv->balance_value;
		break;
	case V4L2_CID_PRIVATE_EFFECT:
		ctrl->value = priv->effect_value;
		break;
	default:
		break;
	}
	return 0;
}

static int gc0308_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct gc0308_priv *priv = to_gc0308(client);
	int ret = 0;
	int i = 0;
	u8 value;

	int balance_count = ARRAY_SIZE(gc0308_balance);
	int effect_count = ARRAY_SIZE(gc0308_effect);

	switch (ctrl->id) {
	case V4L2_CID_PRIVATE_BALANCE:
		if(ctrl->value > balance_count)
			return -EINVAL;

		for(i = 0; i < balance_count; i++) {
			if(ctrl->value == gc0308_balance[i].index) {
				ret = gc0308_write_array(client,
						gc0308_balance[ctrl->value].mode_regs);
				priv->balance_value = ctrl->value;
				break;
			}
		}
		break;

	case V4L2_CID_PRIVATE_EFFECT:
		if(ctrl->value > effect_count)
			return -EINVAL;

		for(i = 0; i < effect_count; i++) {
			if(ctrl->value == gc0308_effect[i].index) {
				ret = gc0308_write_array(client,
						gc0308_effect[ctrl->value].mode_regs);
				priv->effect_value = ctrl->value;
				break;
			}
		}
		break;

	case V4L2_CID_VFLIP:
		value = ctrl->value ? REG14_VFLIP_IMG : 0x00;
		priv->flag_vflip = ctrl->value ? 1 : 0;
		ret = gc0308_mask_set(client, REG14, REG14_VFLIP_IMG, value);
		break;

	case V4L2_CID_HFLIP:
		value = ctrl->value ? REG14_HFLIP_IMG : 0x00;
		priv->flag_hflip = ctrl->value ? 1 : 0;
		ret = gc0308_mask_set(client, REG14, REG14_HFLIP_IMG, value);
		break;

	default:
		dev_err(&client->dev, "no V4L2 CID: 0x%x ", ctrl->id);
		return -EINVAL;
	}

	return ret;
}

static int gc0308_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *id)
{
	id->ident    = SUPPORT_HIGH_RESOLUTION_PRE;
	id->revision = 0;

	return 0;
}

static int gc0308_querymenu(struct v4l2_subdev *sd,
					struct v4l2_querymenu *qm)
{
	switch (qm->id) {
	case V4L2_CID_PRIVATE_BALANCE:
		memcpy(qm->name, gc0308_balance_menus[qm->index].name,
				sizeof(qm->name));
		break;

	case V4L2_CID_PRIVATE_EFFECT:
		memcpy(qm->name, gc0308_effect_menus[qm->index].name,
				sizeof(qm->name));
		break;
	}

	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int gc0308_g_register(struct v4l2_subdev *sd,
			     struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	reg->size = 1;
	if (reg->reg > 0xff)
		return -EINVAL;

	ret = i2c_smbus_read_byte_data(client, reg->reg);
	if (ret < 0)
		return ret;

	reg->val = ret;

	return 0;
}

static int gc0308_s_register(struct v4l2_subdev *sd,
			     struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (reg->reg > 0xff ||
	    reg->val > 0xff)
		return -EINVAL;

	return i2c_smbus_write_byte_data(client, reg->reg, reg->val);
}
#endif

/* Select the nearest higher resolution for capture */
static const struct gc0308_win_size *gc0308_select_win(u32 *width, u32 *height)
{
	int i, default_size = ARRAY_SIZE(gc0308_supported_win_sizes) - 1;

	for (i = 0; i < ARRAY_SIZE(gc0308_supported_win_sizes); i++) {
		if (gc0308_supported_win_sizes[i].width  >= *width &&
		    gc0308_supported_win_sizes[i].height >= *height) {
			*width = gc0308_supported_win_sizes[i].width;
			*height = gc0308_supported_win_sizes[i].height;
			return &gc0308_supported_win_sizes[i];
		}
	}

	*width = gc0308_supported_win_sizes[default_size].width;
	*height = gc0308_supported_win_sizes[default_size].height;
	return &gc0308_supported_win_sizes[default_size];
}

static int gc0308_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct gc0308_priv *priv = to_gc0308(client);

	int bala_index = priv->balance_value;
	int effe_index = priv->effect_value;

	/* initialize the sensor with default data */
	ret = gc0308_write_array(client, gc0308_init_regs);
	ret = gc0308_write_array(client, gc0308_balance[bala_index].mode_regs);
	ret = gc0308_write_array(client, gc0308_effect[effe_index].mode_regs);
	if (ret < 0)
		goto err;

	dev_info(&client->dev, "%s: Init default", __func__);
	return 0;

err:
	dev_err(&client->dev, "%s: Error %d", __func__, ret);
	return ret;
}

#if 0
static int gc0308_set_params(struct i2c_client *client, u32 *width, u32 *height,
			     enum v4l2_mbus_pixelcode code)
{
	struct gc0308_priv *priv = to_gc0308(client);
	const struct regval_list *selected_cfmt_regs;
	int ret;

	/* select win */
	priv->win = gc0308_select_win(width, height);

	/* select format */
	priv->cfmt_code = 0;
	switch (code) {
	case V4L2_MBUS_FMT_YUYV8_2X8:
		dev_dbg(&client->dev, "%s: Selected cfmt YUV422", __func__);
		selected_cfmt_regs = gc0308_rgb565_regs;
		break;
	default:
	case V4L2_MBUS_FMT_YUYV8_1_5X8:
		dev_dbg(&client->dev, "%s: Selected cfmt YUV422", __func__);
		selected_cfmt_regs = gc0308_yuv422_regs;
	}

	/* initialize the sensor with default data */
	dev_info(&client->dev, "%s: Init default", __func__);
	ret = gc0308_write_array(client, gc0308_init_regs);
	if (ret < 0)
		goto err;

	/* set size win */
	ret = gc0308_write_array(client, priv->win->regs);
	if (ret < 0)
		goto err;

	priv->cfmt_code = code;
	*width = priv->win->width;
	*height = priv->win->height;

	return 0;

err:
	dev_err(&client->dev, "%s: Error %d", __func__, ret);
	gc0308_reset(client);
	priv->win = NULL;

	return ret;
}
#endif

static int gc0308_g_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *mf)
{
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct gc0308_priv *priv = to_gc0308(client);

	mf->width = priv->win->width;
	mf->height = priv->win->height;
	mf->code = priv->cfmt_code;

	mf->colorspace = V4L2_COLORSPACE_JPEG;
	mf->field	= V4L2_FIELD_NONE;

	return 0;
}

static int gc0308_s_fmt(struct v4l2_subdev *sd,
			struct v4l2_mbus_framefmt *mf)
{
	/* current do not support set format, use unify format yuv422i */
	struct i2c_client  *client = v4l2_get_subdevdata(sd);
	struct gc0308_priv *priv = to_gc0308(client);
	int ret;

	gc0308_init(sd, 1);

	priv->win = gc0308_select_win(&mf->width, &mf->height);
	/* set size win */
	ret = gc0308_write_array(client, priv->win->regs);
	if (ret < 0) {
		dev_err(&client->dev, "%s: Error\n", __func__);
		return ret;
	}

	return 0;
}

static int gc0308_try_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *mf)
{
	const struct gc0308_win_size *win;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	/*
	 * select suitable win
	 */
	win = gc0308_select_win(&mf->width, &mf->height);

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
		break;
	}

	return 0;
}

static int gc0308_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
			   enum v4l2_mbus_pixelcode *code)
{
	if (index >= ARRAY_SIZE(gc0308_codes))
		return -EINVAL;

	*code = gc0308_codes[index];
	return 0;
}

static int gc0308_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *a)
{
	return 0;
}

static int gc0308_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *a)
{
	return 0;
}


/*
 * Frame intervals.  Since frame rates are controlled with the clock
 * divider, we can only do 30/n for integer n values.  So no continuous
 * or stepwise options.  Here we just pick a handful of logical values.
 */

static int gc0308_frame_rates[] = { 30, 15, 10, 5, 1 };

static int gc0308_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *interval)
{
	if (interval->index >= ARRAY_SIZE(gc0308_frame_rates))
		return -EINVAL;
	interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	interval->discrete.numerator = 1;
	interval->discrete.denominator = gc0308_frame_rates[interval->index];
	return 0;
}


/*
 * Frame size enumeration
 */
static int gc0308_enum_framesizes(struct v4l2_subdev *sd,
		struct v4l2_frmsizeenum *fsize)
{
	int i;
	int num_valid = -1;
	__u32 index = fsize->index;

	for (i = 0; i < N_WIN_SIZES; i++) {
		const struct gc0308_win_size *win = &gc0308_supported_win_sizes[index];
		if (index == ++num_valid) {
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = win->width;
			fsize->discrete.height = win->height;
			return 0;
		}
	}

	return -EINVAL;
}

static int gc0308_video_probe(struct i2c_client *client)
{
	unsigned char retval = 0, retval_high = 0, retval_low = 0;
	struct v4l2_subdev *subdev = i2c_get_clientdata(client);
	int ret = 0;

	struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);

	ret = soc_camera_power_on(&client->dev, ssdd);
	if (ret < 0)
		return ret;

	/*
	 * check and show product ID and manufacturer ID
	 */

	retval_high = gc0308_read_reg(client, REG_CHIP_ID);
	if (retval_high != PID_GC0308) {
		dev_err(&client->dev, "read sensor %s chip_id high %x is error\n",
				client->name, retval_high);
		return -1;
	}

	dev_info(&client->dev, "read sensor %s id high:0x%x,low:%x successed!\n",
			client->name, retval_high, retval_low);

	ret = soc_camera_power_off(&client->dev, ssdd);

	return 0;
}

static int gc0308_s_power(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);
	struct gc0308_priv *priv = to_gc0308(client);
	int ret;



	return soc_camera_set_power(&client->dev, ssdd, on);
#if 0
	int bala_index = priv->balance_value;
	int effe_index = priv->effect_value;


	if (!on)
		return soc_camera_power_off(&client->dev, ssdd);

	ret = soc_camera_power_on(&client->dev, ssdd);
	if (ret < 0)
		return ret;


	///* initialize the sensor with default data */
	ret = gc0308_write_array(client, gc0308_init_regs);
	ret = gc0308_write_array(client, gc0308_balance[bala_index].mode_regs);
	ret = gc0308_write_array(client, gc0308_effect[effe_index].mode_regs);
	if (ret < 0)
		goto err;


	dev_info(&client->dev, "%s: Init default", __func__);
	return 0;

err:
	dev_err(&client->dev, "%s: Error %d", __func__, ret);
	return ret;
#endif

}

static int gc0308_g_mbus_config(struct v4l2_subdev *sd,
		struct v4l2_mbus_config *cfg)
{

	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);

	cfg->flags = V4L2_MBUS_PCLK_SAMPLE_RISING | V4L2_MBUS_MASTER |
		    V4L2_MBUS_VSYNC_ACTIVE_HIGH | V4L2_MBUS_HSYNC_ACTIVE_HIGH |
			    V4L2_MBUS_DATA_ACTIVE_HIGH;
	cfg->type = V4L2_MBUS_PARALLEL;

	cfg->flags = soc_camera_apply_board_flags(ssdd, cfg);

	return 0;
}

#if 0
static struct soc_camera_ops gc0308_ops = {
	.set_bus_param		= gc0308_set_bus_param,
	.query_bus_param	= gc0308_query_bus_param,
	.controls		= gc0308_controls,
	.num_controls		= ARRAY_SIZE(gc0308_controls),
};
#endif
static struct v4l2_subdev_core_ops gc0308_subdev_core_ops = {
//	.init		= gc0308_init,
	.s_power 	= gc0308_s_power,
	.g_ctrl		= gc0308_g_ctrl,
	.s_ctrl		= gc0308_s_ctrl,
	.g_chip_ident	= gc0308_g_chip_ident,
	.querymenu	= gc0308_querymenu,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register	= gc0308_g_register,
	.s_register	= gc0308_s_register,
#endif
};

static struct v4l2_subdev_video_ops gc0308_subdev_video_ops = {
	.s_stream	= gc0308_s_stream,
	.g_mbus_fmt	= gc0308_g_fmt,
	.s_mbus_fmt	= gc0308_s_fmt,
	.try_mbus_fmt	= gc0308_try_fmt,
	.cropcap	= gc0308_cropcap,
	.g_crop		= gc0308_g_crop,
	.enum_mbus_fmt	= gc0308_enum_fmt,
	.g_mbus_config  = gc0308_g_mbus_config,
#if 0
	.enum_framesizes = gc0308_enum_framesizes,
	.enum_frameintervals = gc0308_enum_frameintervals,
#endif
};

static struct v4l2_subdev_ops gc0308_subdev_ops = {
	.core	= &gc0308_subdev_core_ops,
	.video	= &gc0308_subdev_video_ops,
};

/*
 * i2c_driver functions
 */

static int gc0308_probe(struct i2c_client *client,
			const struct i2c_device_id *did)
{
	struct gc0308_priv *priv;
	struct soc_camera_subdev_desc *ssdd = soc_camera_i2c_to_desc(client);
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	int ret = 0,default_wight = 640,default_height = 480;

	if (!ssdd) {
		dev_err(&client->dev, "gc0308: missing platform data!\n");
		return -EINVAL;
	}


	if(!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE
			| I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "client not i2c capable\n");
		return -ENODEV;
	}

	priv = kzalloc(sizeof(struct gc0308_priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&adapter->dev,
			"Failed to allocate memory for private data!\n");
		return -ENOMEM;
	}


	v4l2_i2c_subdev_init(&priv->subdev, client, &gc0308_subdev_ops);

	priv->win = gc0308_select_win(&default_wight, &default_height);

	priv->cfmt_code  =  V4L2_MBUS_FMT_YUYV8_2X8;


	ret = gc0308_video_probe(client);
	if (ret) {
		kfree(priv);
	}
	return ret;
}

static int gc0308_remove(struct i2c_client *client)
{
	struct gc0308_priv       *priv = to_gc0308(client);
//	struct soc_camera_device *icd = client->dev.platform_data;

//	icd->ops = NULL;
	kfree(priv);
	return 0;
}

static const struct i2c_device_id gc0308_id[] = {
	{ "gc0308",  0 },
	{ }
};


MODULE_DEVICE_TABLE(i2c, gc0308_id);

static struct i2c_driver gc0308_i2c_driver = {
	.driver = {
		.name = "gc0308",
	},
	.probe    = gc0308_probe,
	.remove   = gc0308_remove,
	.id_table = gc0308_id,
};

/*
 * Module functions
 */
static int __init gc0308_module_init(void)
{
	return i2c_add_driver(&gc0308_i2c_driver);
}

static void __exit gc0308_module_exit(void)
{
	i2c_del_driver(&gc0308_i2c_driver);
}

module_init(gc0308_module_init);
module_exit(gc0308_module_exit);

MODULE_DESCRIPTION("camera sensor gc0308 driver");
MODULE_AUTHOR("qipengzhen <aric.pzqi@ingenic.com>");
MODULE_LICENSE("GPL");
