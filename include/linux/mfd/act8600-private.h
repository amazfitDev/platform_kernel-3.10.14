/*
 * act8600-private.h - Head file for PMU ACT8600.
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 * Author: Large Dipper <ykli@ingenic.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_MFD_ACT8600_PRIV_H
#define __LINUX_MFD_ACT8600_PRIV_H

#define VCON_OK				(1 << 0)
#define VCON_DIS			(1 << 2)
#define VCON_ON				(1 << 7)

#define APCH_INTR0	0xa1
	#define SUSCHG		(1 << 7)
#define APCH_INTR1	0xa8
	#define CHGDAT 	(1 << 0)
	#define INDAT 	(1 << 1)
	#define TEMPDAT 	(1 << 2)
	#define TIMRDAT 	(1 << 3)
	#define CHGSTAT 	(1 << 4)
	#define INSTAT 	(1 << 5)
	#define TEMPSTAT (1 << 6)
	#define TIMRSTAT (1 << 7)


#define APCH_INTR2	0xa9
	#define CHGEOCOUT 	(1 << 0)
	#define INDIS 		(1 << 1)
	#define TEMPOUT 		(1 << 2)
	#define TIMRPRE 		(1 << 3)
	#define CHGEOCIN 	(1 << 4)
	#define INCON 		(1 << 5)
	#define TEMPIN 		(1 << 6)
	#define TIMRTOT 		(1 << 7)



#define APCH_STAT	0xaa
	#define CSTATE_MASK	(0x30)
	#define CSTATE_PRE	(0x30)
	#define CSTATE_CHAGE	(0x20)
	#define CSTATE_EOC	(0x10)
	#define CSTATE_SUSPEND	(0x00)

#define OTG_CON		0xb0
	#define ONQ1		(1 << 7)
	#define ONQ2		(1 << 6)
	#define ONQ3		(1 << 5)
	#define Q1OK		(1 << 4)
	#define Q2OK		(1 << 3)
	#define VBUSSTAT (1 << 2)
	#define DBILIMQ3	(1 << 1)
	#define VBUSDAT		(1 << 0)
#define OTG_INTR	0xb2
	#define INVBUSR	((1 << 7) | 0x3)
	#define INVBUSF	((1 << 6) | 0x3)
	#define VBUSMSK	((1 << 1) | 0x3)


#define ACT8600_SYS0		0x00
#define ACT8600_SYS1		0x01


struct act8600 {
	struct device *dev;
	struct i2c_client *client;
};

struct charger_board_info {
	short gpio;
	short enable_level;
};

extern int act8600_read_reg(struct i2c_client *client, unsigned char reg, unsigned char *val);
extern int act8600_write_reg(struct i2c_client *client, unsigned char reg, unsigned char val);

#endif /* __LINUX_MFD_ACT8600_PRIV_H */
