/*
 * Driver support for AOSD controller.
 *
 * Copyright (c) 2012 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/*
 * NOTE:
 * AOSD(Alpha On Screen Display) is used to deal with UI
 * when video directly output to LCD controller foreground 1.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#include "aosd.h"

#define PIXEL_ALIGN 16
#define STRIDE_ALIGN 64 /* 64 words aligned */

struct jzfb_aosd_info aosd_info;
struct jzaosd *jzaosd=NULL;

unsigned int aosd_read_fwdcnt(void)
{
	return (unsigned int)aosd_readl(jzaosd, AOSD_FWDCNT);
}

/* compute the compress ratio */
static void calc_comp_ratio(unsigned int width, unsigned int height)
{
	unsigned int ratio;
	unsigned int outSize;

	outSize = aosd_read_fwdcnt();
	ratio = (outSize * 100) / (width * height);
	dev_info(jzaosd->dev, "Out size: %d Words, ratio: %d%%\n",
		 outSize, ratio);
}

void print_aosd_registers(void)	/* debug */
{
	struct device *dev = jzaosd->dev;

	clk_enable(jzaosd->debug_clk);

	dev_info(dev, "AOSD_CTRL:0x%08lx\n", aosd_readl(jzaosd, AOSD_CTRL));
	dev_info(dev, "AOSD_CFG:0x%08lx\n", aosd_readl(jzaosd, AOSD_CFG));
	dev_info(dev, "AOSD_STATE:0x%08lx\n", aosd_readl(jzaosd, AOSD_STATE));
	dev_info(dev, "AOSD_FWDCNT:0x%08lx\n", aosd_readl(jzaosd, AOSD_FWDCNT));
	dev_info(dev, "AOSD_WADDR:0x%08lx\n", aosd_readl(jzaosd, AOSD_WADDR));
	dev_info(dev, "AOSD_WCMD:0x%08lx\n", aosd_readl(jzaosd, AOSD_WCMD));
	dev_info(dev, "AOSD_WOFFS:0x%08lx\n", aosd_readl(jzaosd, AOSD_WOFFS));
	dev_info(dev, "AOSD_RADDR:0x%08lx\n", aosd_readl(jzaosd, AOSD_RADDR));
	dev_info(dev, "AOSD_RSIZE:0x%08lx\n", aosd_readl(jzaosd, AOSD_RSIZE));
	dev_info(dev, "AOSD_ROFFS:0x%08lx\n", aosd_readl(jzaosd, AOSD_ROFFS));

	if (aosd_info.width > 0 && aosd_info.height > 0)
		calc_comp_ratio(aosd_info.width, aosd_info.height);

	clk_disable(jzaosd->debug_clk);
}

void aosd_clock_enable(int enable)
{
	int count = 30;

	if (enable) {
		clk_enable(jzaosd->clk);
	} else {
		/* Cann't disable clock immediately when AOSD is busy */
		while (count--) {
			mutex_lock(&jzaosd->state_lock);
			if (!jzaosd->is_enabled) {
				mutex_unlock(&jzaosd->state_lock);
				clk_disable(jzaosd->clk);
				return;
			} else {
				mutex_unlock(&jzaosd->state_lock);
				msleep(1);
			}
		}
		if (count < 0) {
			clk_disable(jzaosd->clk);
			dev_err(jzaosd->dev, "Wait AOSD stop timeout\n");
		}
	}
}

static void aosd_enable(void)
{
	unsigned long tmp;

	mutex_lock(&jzaosd->state_lock);
	if (jzaosd->is_enabled) {
		mutex_unlock(&jzaosd->state_lock);
		return;
	}

	tmp = aosd_readl(jzaosd, AOSD_CTRL); /* Start compress */
	tmp |= CTRL_ENABLE;
	aosd_writel(jzaosd, AOSD_CTRL, tmp);
	jzaosd->is_enabled = 1;
	mutex_unlock(&jzaosd->state_lock);
}

static void aosd_disable(void)
{
	mutex_lock(&jzaosd->state_lock);
	if (!jzaosd->is_enabled) {
		mutex_unlock(&jzaosd->state_lock);
		return;
	}

	/* After finished compress, hardware will clear
	   CTRL_ENABLE bit and auto stop */
	jzaosd->is_enabled = 0;
	mutex_unlock(&jzaosd->state_lock);
}

static void aosd_change_address(void)
{
	/*SET SCR AND DES ADDR*/
	aosd_writel(jzaosd, AOSD_RADDR, aosd_info.raddr);
	aosd_writel(jzaosd, AOSD_WADDR, aosd_info.waddr);
}

static irqreturn_t aosd_interrupt_handler(int irq, void *dev_id)
{
	unsigned int state;
	struct jzaosd *jzaosd = (struct jzaosd *)dev_id;

	state = aosd_readl(jzaosd, AOSD_STATE);
	if (state & STATE_AD_FLAG) {
		jzaosd->compress_done = 1;
		/* hardware need to set 1 to clear auto disable flag */
		state |= STATE_AD_FLAG;
		aosd_writel(jzaosd, AOSD_STATE, state);

		wake_up_interruptible(&jzaosd->aosd_wq);
	}/* else {
		dev_info(jzaosd->dev, "AOSD auto disable flag not set\n");
		}*/

	return IRQ_HANDLED;
}

void aosd_init(struct jzfb_aosd_info *info)
{
	unsigned int width, height, bpp;
	unsigned int src_stride, dst_stride;
	unsigned int word_per_line;
	unsigned int wcmd;
	unsigned int cfg;

	if (!jzaosd){
		printk("aosd do not init!\n");
		return;
	}

	aosd_clock_enable(1); /* enable AOSD clock */

	/* burst length: 128 words */
	info->burst_128_words = 1;
	height = info->height;

	word_per_line = (info->width * info->bpp) >> 5;
	//word_per_line = ALIGN(info->width * (info->bpp >> 3) >> 2,
	//		      STRIDE_ALIGN);

	if (info->bpp != 16) {
		if (info->with_alpha) {
			/* 24bpp with alpha is equal to double 16bpp */
			bpp = 16;
			width = info->width * 2;
			src_stride = info->src_stride * 2 >> 2; /* in word */
		} else {
			bpp = info->bpp;
			width = info->width;
			src_stride = info->src_stride >> 2; /* in word */
		}
	} else {
		bpp = info->bpp;
		width = info->width;
		src_stride = info->src_stride >> 2; /* in word */
	}

	/*
	 * In words. If out data is not a sequential access,
	 * word_per_line = (panel.w * bpp) >> 5
	 */
	dst_stride = (word_per_line + (word_per_line + STRIDE_ALIGN - 1) / STRIDE_ALIGN);
	//dst_stride = word_per_line + STRIDE_ALIGN;

	aosd_info.src_stride = src_stride;
	aosd_info.dst_stride = dst_stride;

/*	dev_info(jzaosd->dev, "width:%d, height:%d, bpp:%d, with_alpha:%d\n",
		 info->width, info->height, info->bpp, info->with_alpha);
	dev_info(jzaosd->dev, "src_stride = %d, dst_stride = %d\n",
		 src_stride, dst_stride);
*/

	if (bpp != 16) {
		wcmd = WCMD_BPP_24;
		if (info->with_alpha) {
			wcmd |= WCMD_COMPMD_BPP24_ALPHA;
		} else {
			wcmd |= WCMD_COMPMD_BPP24_COM;
		}
	} else {
		wcmd = WCMD_BPP_16 | WCMD_COMPMD_BPP16;
	}

	cfg = CFG_ADM; /* enable auto disable interrupt */
	cfg &= ~CFG_QDM; /* disable quick disable interrupt */
	/* Trans mode, 0:128 words; 1:64 words */
	cfg |= info->burst_128_words ? 0 : CFG_TMODE;

	aosd_writel(jzaosd, AOSD_CFG, cfg);
	aosd_writel(jzaosd, AOSD_STATE, 0); /* clear state register */
	aosd_writel(jzaosd, AOSD_WCMD, wcmd);

	/*SET SCR AND DST OFFSIZE */
	aosd_writel(jzaosd, AOSD_ROFFS, src_stride);
	aosd_writel(jzaosd, AOSD_WOFFS, dst_stride);

	/*SET SIZE*/
	/*frame's actual size, no need to align */
	width = width - 1;
	height = height - 1;
	aosd_writel(jzaosd, AOSD_RSIZE, height << RSIZE_RHEIGHT_BIT
		    | width << RSIZE_RWIDTH_BIT);

	aosd_clock_enable(0);
}

void aosd_start(void)
{
	unsigned long tmp;
	unsigned int word_per_line;
	int timeout;

	if (!jzaosd) {
		printk("aosd do not init!\n");
		return;
	}
	aosd_clock_enable(1);

	/* set buffer address, the value of RADDR will auto increase */
	aosd_change_address();

	tmp = aosd_readl(jzaosd, AOSD_CFG);
	tmp |= CFG_ADM; /* enable auto disable interrupt */
	tmp &= ~CFG_QDM; /* disable quick disable interrupt */
	aosd_writel(jzaosd, AOSD_CFG, tmp);

	//print_aosd_registers();

	jzaosd->compress_done = 0;
	aosd_enable();

	/* wait irq  */
	timeout = wait_event_interruptible_timeout(jzaosd->aosd_wq,
						   jzaosd->compress_done == 1,
						   msecs_to_jiffies(100));
	if (!timeout) {
		dev_err(jzaosd->dev, "AOSD wait compress down timeout\n");
	}

	tmp = aosd_readl(jzaosd, AOSD_CFG);
	tmp &= ~CFG_ADM; /* disable auto disable interrupt */
	aosd_writel(jzaosd, AOSD_CFG, tmp);

	aosd_disable();

	//calc_comp_ratio(aosd_info.width, aosd_info.height);
	//print_aosd_registers();
	word_per_line = (aosd_info.width * aosd_info.bpp) >> 5;
	//word_per_line = ALIGN(aosd_info.width * (aosd_info.bpp >> 3) >> 2,
	//		      STRIDE_ALIGN);

	/* hardware fault: the last line need to deal with software */
	if (aosd_info.with_alpha != 0 || aosd_info.bpp == 16) {
		unsigned int *src, *dst;
		unsigned int cmd_num = word_per_line / STRIDE_ALIGN;
		unsigned int mod128 = word_per_line % STRIDE_ALIGN;
		unsigned int cmd;

		volatile int i = 0, j = 0;
		int cpsize = STRIDE_ALIGN * 4;
		if((unsigned long)aosd_info.raddr < 0x10000000){
			if (aosd_info.with_alpha != 0 && aosd_info.bpp != 16) {
				src = (unsigned int *)phys_to_virt((unsigned long)aosd_info.raddr)
					+ ((aosd_info.src_stride / 2) * (aosd_info.height - 1));
			} else {
				src = (unsigned int *)phys_to_virt((unsigned long)aosd_info.raddr)
					+ (aosd_info.src_stride * (aosd_info.height - 1));
			}
		}else{
			/*Dealing with  the last line in third buf */
			src = (unsigned int *)aosd_info.buf_virt_addr
				+ (aosd_info.dst_stride *aosd_info.height*2)
				+ (aosd_info.dst_stride * (aosd_info.height - 1));
		}
//		dst = (unsigned int *)phys_to_virt((unsigned long)aosd_info.waddr)
		dst = (unsigned int *)aosd_info.virt_waddr
			+ (aosd_info.dst_stride * (aosd_info.height - 1));

		if (word_per_line < STRIDE_ALIGN) {
			cmd = ((word_per_line + 1) << 16 | mod128) | (1 << 29);
			cpsize = word_per_line *4 ;

			*(dst+i++) = cmd;
			memcpy((dst + i), src, cpsize);

		} else if ((cmd_num == 1) && (mod128 != 0)) {
			cmd = (word_per_line + cmd_num) << 16 | (0x40 + mod128)
				| (1 << 29);
			cpsize = word_per_line *4 ;

			*(dst+i++) = cmd;
			memcpy((dst + i), src, cpsize);

		} else {
			/*the first cmd of the line*/
			cmd = (word_per_line + cmd_num) << 16 | 0x40 | (1 << 29);
			cpsize = 0x40 * 4;
			*(dst + i++) = cmd;
			memcpy((dst + i), src+(j * STRIDE_ALIGN), cpsize);
			i += (cpsize >> 2);
			j++;

			cmd = 0x20000040;
			while (j < (cmd_num - 1)) {
				*(dst+i++) = cmd;
				memcpy((dst + i), src+(j * STRIDE_ALIGN), cpsize);
				i += (cpsize >> 2);
				j++;
			}

			cmd = (cmd & ~(0xff)) | (0x40 + mod128);
			cpsize += (mod128 * 4);
			*(dst+i++) = cmd;
			memcpy((dst + i), src+(j * STRIDE_ALIGN), cpsize);
		}
/*		dst = (unsigned int *)virt_to_phys((void *)dst);
		dma_sync_single_for_device(jzaosd->dev, (dma_addr_t)dst,
					   (word_per_line + cmd_num) << 2,
					   DMA_TO_DEVICE);
*/
	}

	aosd_clock_enable(0); /* disable AOSD clock */
}

static int jzaosd_probe(struct platform_device *pdev)
{
	int ret;
	struct jzaosd *aosd_dev;
	struct resource *mem;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "AOSD get IO memory resource fail\n");
		return -ENXIO;
	}
	mem = request_mem_region(mem->start, resource_size(mem), pdev->name);
	if (!mem) {
		dev_err(&pdev->dev, "AOSD request IO memory region fail\n");
		return -EBUSY;
	}

	aosd_dev = kzalloc(sizeof(struct jzaosd), GFP_KERNEL);
	if (!aosd_dev) {
		dev_err(&pdev->dev, "Failed to allocate AOSD device\n");
		ret = -ENOMEM;
		goto err_release_mem_region;
	}

	aosd_dev->base = ioremap(mem->start, resource_size(mem));
	if (!aosd_dev->base) {
		dev_err(&pdev->dev, "AOSD ioremap memory region fail\n");
		ret = -EBUSY;
		goto err_aosd_release;
	}

	aosd_dev->clk = clk_get(&pdev->dev, "compress");
	aosd_dev->debug_clk = clk_get(&pdev->dev, "compress");
	if (IS_ERR(aosd_dev->clk) || IS_ERR(aosd_dev->debug_clk)) {
		ret = PTR_ERR(aosd_dev->clk);
		dev_err(&pdev->dev, "Failed to get aosd clock: %d\n", ret);
		goto err_iounmap;
	}

	aosd_dev->irq = platform_get_irq(pdev, 0);
	if (request_irq(aosd_dev->irq, aosd_interrupt_handler, IRQF_DISABLED,
			pdev->name, aosd_dev)) {
		dev_err(&pdev->dev,"AOSD request irq failed\n");
		ret = -EINVAL;
		goto err_put_clk;
	}

	aosd_dev->dev = &pdev->dev;
	jzaosd = aosd_dev;

	init_waitqueue_head(&aosd_dev->aosd_wq);
	mutex_init(&aosd_dev->state_lock);
	platform_set_drvdata(pdev, aosd_dev);

	dev_info(&pdev->dev, "Frame buffer support AOSD\n");

	return 0;

err_put_clk:
	if (aosd_dev->clk)
		clk_put(aosd_dev->clk);
	if (aosd_dev->debug_clk)
		clk_put(aosd_dev->debug_clk);
err_aosd_release:
	kzfree(aosd_dev);
err_iounmap:
	iounmap(aosd_dev->base);
err_release_mem_region:
	release_mem_region(mem->start, resource_size(mem));

	return ret;
}

static int jzaosd_remove(struct platform_device *pdev)
{
	struct jzaosd *aosd_dev = platform_get_drvdata(pdev);

	iounmap(aosd_dev->base);
	release_mem_region(aosd_dev->mem->start, resource_size(aosd_dev->mem));
	clk_put(aosd_dev->clk);
	clk_put(aosd_dev->debug_clk);

	platform_set_drvdata(pdev, NULL);
	kzfree(aosd_dev);

	return 0;
}

static struct platform_driver jzaosd_driver = {
	.probe 	= jzaosd_probe,
	.remove = jzaosd_remove,
	.driver = {
		.name = "jz-aosd",
	},
};

static int __init jzaosd_init(void)
{
	platform_driver_register(&jzaosd_driver);
	return 0;
}

static void __exit jzaosd_cleanup(void)
{
	platform_driver_unregister(&jzaosd_driver);
}

#ifdef CONFIG_EARLY_INIT_RUN
rootfs_initcall(jzaosd_init);
#else
module_init(jzaosd_init);
#endif

module_exit(jzaosd_cleanup);

MODULE_DESCRIPTION("Graphics Internal AOSD controller driver");
MODULE_AUTHOR("<jrzhou@ingenic.cn>");
MODULE_LICENSE("GPL");
MODULE_VERSION("20121120");
