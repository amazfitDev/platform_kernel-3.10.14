/*
 * aosd.h - AOSD registers and structure definition
 *
 * Copyright (c) 2012 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 */

#ifndef __AOSD_H__
#define __AOSD_H__

#include <linux/io.h>

/* Register Map Of AOSD */
#define AOSD_CTRL	(0x00) /* Control Register */
#define AOSD_CFG	(0x04) /* Configure Register */
#define AOSD_STATE	(0x08) /* Status Register */
#define AOSD_FWDCNT	(0x0c) /* Output Words Counter Register */
#define AOSD_WADDR	(0x104) /* Address of the Destination of DMA Register */
#define AOSD_WCMD	(0x108) /* Output Command Register */
#define AOSD_WOFFS	(0x10c) /* Output Offset Register */
#define AOSD_RADDR	(0x114) /* Input Address Register */
#define AOSD_RSIZE	(0x118) /* Input size Register */
#define AOSD_ROFFS	(0x11c) /* Input Offset Register */

/* Control Register */
#define CTRL_QUICK_DISABLE	(1 << 2) /* 1:quick disable */
#define CTRL_ENABLE	(1 << 0) /* 1:enable */

/* Configure Register */
#define CFG_TMODE	(1 << 4) /* Trans mode, 0:128 words; 1:64 words */
#define CFG_QDM	(1 << 2) /* Quick disable int mask */
#define CFG_ADM	(1 << 0) /* Auto disable int mask */

/* Status Register */
#define STATE_QD_FLAG	(1 << 2) /* Quick disable flag */
#define STATE_AD_FLAG	(1 << 0) /* Auto disable flag */

/* Output Command Register */
#define WCMD_BPP_BIT	27 /* 0,1:bpp 16; 2:bpp 24 */
#define WCMD_BPP_MASK	(0x3 << WCMD_BPP_BIT)
#define WCMD_BPP_16	(1 << WCMD_BPP_BIT)
#define WCMD_BPP_24	(0x2 << WCMD_BPP_BIT)
/* compress mode: 0:bpp24 alpha; 1:bpp24 common; 2:bpp16 */
#define WCMD_COMPMD_BIT	29
#define WCMD_COMPMD_MASK	(0x3 << WCMD_COMPMD_BIT)
#define WCMD_COMPMD_BPP24_ALPHA	(0 << WCMD_COMPMD_BIT)
#define WCMD_COMPMD_BPP24_COM	(1 << WCMD_COMPMD_BIT)
#define WCMD_COMPMD_BPP16	(2 << WCMD_COMPMD_BIT)

/* Input size Register */
#define RSIZE_RHEIGHT_BIT	12 /* Vsize, count in pixel */
#define RSIZE_RHEIGHT_MASK	(0xfff << RSIZE_RHEIGHT_BIT)
#define RSIZE_RWIDTH_BIT	0 /* Hsize, count in pixel */
#define RSIZE_RWIDTH_MASK	(0xfff << RSIZE_RWIDTH_BIT)

struct jzfb_aosd_info {
	unsigned int bpp;
	unsigned int width;
	unsigned int height;
	unsigned int src_stride;
	unsigned int dst_stride;

	unsigned long raddr;
	unsigned long waddr;
	unsigned long virt_waddr;
	unsigned long buf_phys_addr[2];
	unsigned long buf_virt_addr;
	unsigned long buf_offset;

	unsigned int next_frame_id;
	unsigned int is_desc_init;
	unsigned int burst_128_words;
	unsigned int with_alpha;

	unsigned int aligned_64;
	unsigned int alpha_mode;
	unsigned int alpha_value;
	unsigned int order;
	unsigned int format_mode;
};

struct jzaosd {
	struct device *dev;
	struct resource *mem;
	void __iomem *base;

	int compress_done;
	int is_enabled; /* 0:disable, 1:enable */
	struct mutex state_lock;
	wait_queue_head_t aosd_wq;

	struct clk *clk;
	struct clk *debug_clk;
	int irq;
};

static inline unsigned long aosd_readl(struct jzaosd *jzaosd, int offset)
{
	return readl(jzaosd->base + offset);
}

static inline void aosd_writel(struct jzaosd *jzaosd, int offset, unsigned long val)
{
	writel(val, jzaosd->base + offset);
}

/* define in aosd.c */
extern struct jzfb_aosd_info aosd_info; /* aosd data info */
extern void aosd_init(struct jzfb_aosd_info *info);
extern void aosd_start(void);
extern unsigned int aosd_read_fwdcnt(void);
extern void aosd_clock_enable(int enable);
extern void print_aosd_registers(void);

#endif /* __AOSD_H__ */
