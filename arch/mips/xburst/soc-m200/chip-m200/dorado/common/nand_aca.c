/*
 *  Copyright (C) 2015 cli <chen.li <chen.li@ingenic.com>
 *  NAND-MTD support template
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include <linux/sizes.h>
#include <linux/platform_data/xburst_nand.h>
#include <gpio.h>

struct mtd_partition mtd_partition[] = {
	[0] = {
		.name = "uboot",
		.offset = 0,
		.size = 16 * 1024 * 1024,
	},
	[1] = {
		.name = "boot",
		.offset = MTDPART_OFS_APPEND,
		.size = 8 * 1024 * 1024,
	},
	[2] = {
		.name = "root",
		.offset = MTDPART_OFS_APPEND,
		.size = MTDPART_SIZ_FULL,
	},
};

struct xburst_nand_chip_platform_data nand_chip_data = {
	.options = NAND_NO_SUBPAGE_WRITE,
	.bbt_options = NAND_BBT_USE_FLASH,
	.ecc_strength = 24,
	.ecc_size = 1024,
	.ecc_mode = NAND_ECC_HW_OOB_FIRST,
	.rb_gpio = GPIO_PA(20),
	.wp_gpio = -1,
	.tALS = 10,
	.tALH = 5,
	.tRP = 25,
	.tWP = 15,
	.tRHW = 100,
	.tWHR = 60,
	.tWHR2 = 200,
	.tRR = 20,
	.tWB = 100,
	.tADL = 100,
	.tCWAW = 0,
	.tCS = 25,
	.tCLH = 5,
	.tWH = 10,
	.tCH = 5,
	.tDH = 5,
	.tREH = 10,
	.init_size = NULL,
	.nr_partitions = ARRAY_SIZE(mtd_partition),
	.partitions = mtd_partition,
	.has_ids = 0,
};
