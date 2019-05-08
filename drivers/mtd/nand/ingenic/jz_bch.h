/*
 * JZ BCH controller
 *
 * Copyright (c) 2014 Imagination Technologies
 * Author: Alex Smith <alex.smith@imgtec.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __JZ_BCH_H__
#define __JZ_BCH_H__
/**
 * struct bch_params - BCH parameters
 * @size: data bytes per ECC step.
 * @bytes: ECC bytes per step.
 * @strength: number of correctable bits per ECC step.
 */
struct jz_bch_params {
	int size;
	int bytes;
	int strength;
};

extern int jz_bch_calculate(struct device *dev,
		struct jz_bch_params *params,
		const uint8_t *buf, uint8_t *ecc_code);
extern int jz_bch_correct(struct device *dev,
		struct jz_bch_params *params, uint8_t *buf,
		uint8_t *ecc_code);
#endif /*__JZ_BCH_H__*/
