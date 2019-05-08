/*
 * Copyright (C) 2015 Ingenic, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __XBURST_NAND_H__
#define __XBURST_NAND_H__
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/bbm.h>

struct xburst_nand_chip_platform_data {
	int chip_delay;			/*us*/
	/*chip options*/
	unsigned int options;		/*see linux/mtd/nand.h*/
	unsigned int bbt_options;	/*see linux/mtd/bbm.h*/
	/*ecc params*/
	unsigned int ecc_strength;		/*max number of correctible bits per ECC step*/
	unsigned int ecc_size;		/*data bytes per ECC step*/
	nand_ecc_modes_t ecc_mode;	/*hw ecc mode*/
	/*rb&wp gpio*/
	int rb_gpio;			/*zero is unused , assume PA0 is not used for rb*/
	int wp_gpio;
	/*timing params ...duration/width/time*/
	unsigned int tALS;
	unsigned int tALH;
	unsigned int tRP;
	unsigned int tWP;
	unsigned int tRHW;
	unsigned int tWHR;
	unsigned int tWHR2;
	unsigned int tRR;
	unsigned int tWB;
	unsigned int tADL;
	unsigned int tCWAW;
	unsigned int tCS;
	unsigned int tCLH;
	unsigned int tWH;
	unsigned int tCH;
	unsigned int tDH;
	unsigned int tREH;
	/*special nand chip (which can not use onfi ,ext_id or id decode size params) init*/
	int (*init_size)(struct mtd_info *mtd, struct nand_chip *this,
			u8 *id_data);
	int has_ids;			/*private nand ids table avail*/
	struct nand_flash_dev	nand_flash_ids[2];	/*private nand ids table*/
	/*mtd partitions*/
	int nr_partitions;
	struct mtd_partition	*partitions;
};
#endif /*__XBURST_NAND_H__*/
