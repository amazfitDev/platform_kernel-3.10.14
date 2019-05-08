/*
 * linux/drivers/misc/efuse/jz_efuse_v12.c - Ingenic efuse driver
 *
 * Copyright (C) 2015 Ingenic Semiconductor Co., Ltd.
 * Author: chen.li <chen.li@ingenic.com>.
 *
 * Base On: linux/drivers/misc/jz_efuse_v12.c version: 20140411
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


/*#define DEBUG*/
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <linux/io.h>
#include <soc/base.h>
#include <mach/jz_efuse.h>
#include <linux/seq_file.h>
#include <jz_proc.h>
#include <uapi/ingenic/jz_uapi_efuse.h>
#define DRV_NAME "efuse"
#define EFUSE_CTRL		0x0
#define EFUSE_CFG		0x4
#define EFUSE_STATE		0x8
#define EFUSE_DATA(n)		(0xC + (n)*4)

typedef enum {
	CHIP_ID =0,
	CHIP_NUM,
	CUSTOMER_ID,
	TRIM0,
	TRIM1,
	TRIM2,
	TRIM3,
	PROTECT_BIT,
	ROOT_KEY,
	CHIP_KEY,
	USER_KEY,
	MD5,
	FIXBTROM,
	END
} efuse_segment_t;

uint32_t seg_addr[] = {
	0x200U, 0x210U, 0x220U, 0x230U, 0x234U,
	0x238U, 0x23CU, 0x23EU, 0x240U, 0x250U,
	0x260U, 0x270U, 0x280U, 0x400U/*end*/
};

struct jz_efuse {
	struct device *dev;
	struct miscdevice mdev;
	spinlock_t lock;
	void __iomem *iomem;
	struct clk *clk;
	/*vddq*/
	int gpio_vddq_en;
	int gpio_vddq_en_level;
	struct timer_list vddq_protect_timer;
	unsigned long h2clk_rate;
	struct proc_dir_entry *parent_entry;
};

static struct jz_efuse *efuse = NULL;

static uint32_t efuse_readl(uint32_t reg_off)
{
	uint32_t val;
	val = readl(efuse->iomem + reg_off);
	pr_debug("efuse read %p(%x)\n", efuse->iomem + reg_off, val);
	return val;
}

static void efuse_writel(uint32_t val, uint32_t reg_off)
{
	pr_debug("efuse write %p(%x)\n", efuse->iomem + reg_off, val);
	writel(val, efuse->iomem + reg_off);
}

#if IS_ENABLED(CONFIG_JZ_EFUSE_WRITE)
static void efuse_clear_vddq(unsigned long data)
{
	struct jz_efuse *efuse = (struct jz_efuse *)data;
	gpio_set_value(efuse->gpio_vddq_en, !efuse->gpio_vddq_en_level);
}

static void efuse_vddq_set(bool is_on)
{
	if (is_on) {
		mod_timer(&efuse->vddq_protect_timer, jiffies + HZ);
		gpio_set_value(efuse->gpio_vddq_en, efuse->gpio_vddq_en_level);
	} else {
		del_timer_sync(&efuse->vddq_protect_timer);
		gpio_set_value(efuse->gpio_vddq_en, !efuse->gpio_vddq_en_level);
	}
}
#endif

static int set_efuse_cfg(void)
{
	struct clk *h2clk = NULL;
	unsigned long h2clk_rate;
	unsigned long val, h2clk_ns;
	unsigned long ns;
	int i, rd_strobe, wr_strobe;
	uint32_t rd_adj, wr_adj;

	h2clk = clk_get(efuse->dev, "h2clk");
	if (IS_ERR(h2clk))
		return PTR_ERR(h2clk);

	h2clk_rate = clk_get_rate(h2clk);
	clk_put(h2clk);
	if (h2clk_rate == efuse->h2clk_rate)
		return 0;

	efuse->h2clk_rate = h2clk_rate;
	h2clk_ns = (100 * 1000)/(efuse->h2clk_rate/1000000);
	ns = 1000/(efuse->h2clk_rate/1000000);
	dev_dbg(efuse->dev, "h2clkrate = %lu, h2clk_ns = %lu/100\n", efuse->h2clk_rate, h2clk_ns);

	rd_adj = wr_adj = 0xf;	/*fix to maximun*/
	rd_strobe = 0x7;	/*fix to maximun*/
	for (i = -0x800; i < 0x800; i++) {
		val = ((wr_adj + i + 1666) * h2clk_ns)/100;
		if( val > 9000 && val < 11000)
			break;
	}
	if(i == 0x800) {
		dev_err(efuse->dev, "get efuse cfg wd_strobe fail!\n");
		return -ENODEV;
	}
	wr_strobe = (i & 0xfff);
	dev_dbg(efuse->dev, "rd_adj = %d | rd_strobe = %d | "
			"wr_adj = %d | wr_strobe = %d(%d)\n", rd_adj, rd_strobe,
			wr_adj, wr_strobe, i);
	/*set configer register*/
	val = /*1 << 31 | */rd_adj << 20 | rd_strobe << 16 | wr_adj << 12 | wr_strobe;
	efuse_writel(val, EFUSE_CFG);
	return 0;
}

static int __jz_efuse_read(uint32_t xaddr, uint32_t xlen, void *buf)
{
	unsigned long flags;
	int next_seg_id = 0;
	int ret = 0 , i;
	unsigned buf_tmp[4];
	unsigned length, word_num, val;
	uint32_t addr = 0;
	unsigned char *pbuf = (unsigned char *)buf_tmp;

	dev_dbg(efuse->dev, "efuse read xaddr %x xlen %d\n", xaddr, xlen);
	if (!(xaddr+xlen <= seg_addr[ROOT_KEY] || xaddr >= seg_addr[FIXBTROM]))
		return -EPERM;
	if (xaddr+xlen > seg_addr[END] || xaddr < seg_addr[CHIP_ID])
		return -ENODEV;

	/* Strongly recommend operate each segment separately
	 * So we read one segment one cycle*/
	for (next_seg_id = CHIP_NUM; xaddr >= seg_addr[next_seg_id];
			next_seg_id++);
	length = min((uint32_t)(seg_addr[next_seg_id] - xaddr), xlen);
	spin_lock_irqsave(&efuse->lock, flags);
	clk_enable(efuse->clk);
	ret = set_efuse_cfg();
	if (ret) {
		clk_disable(efuse->clk);
		spin_unlock_irqrestore(&efuse->lock, flags);
		return ret;
	}
	val = efuse_readl(EFUSE_STATE);
	if (val & 1)
		efuse_writel(0, EFUSE_STATE);
	addr = (xaddr - 0x200) & 0x1ff;
	dev_dbg(efuse->dev, "efuse read addr %x length %d\n", addr, length);
	/* set read Programming address and data length */
	val = addr << 21 | (length - 1)  << 16;
	efuse_writel(val, EFUSE_CTRL);
	/* enable read */
	val = efuse_readl(EFUSE_CTRL);
	val |= 1;
	efuse_writel(val, EFUSE_CTRL);
	/* wait read done status */
	while(!(efuse_readl(EFUSE_STATE) & 1));

	word_num = (length+3)/4;
	/*Needless to read more than 128bit data*/
	switch(word_num) {
	case 4:
		buf_tmp[3] = efuse_readl(EFUSE_DATA(3));
	case 3:
		buf_tmp[2] = efuse_readl(EFUSE_DATA(2));
	case 2:
		buf_tmp[1] = efuse_readl(EFUSE_DATA(1));
	case 1:
		buf_tmp[0] = efuse_readl(EFUSE_DATA(0));
	}
	/* clear read done staus */
	efuse_writel(0, EFUSE_STATE);
	clk_disable(efuse->clk);
	spin_unlock_irqrestore(&efuse->lock, flags);

	for (i = 0; i < length; i++)
		((unsigned char*)buf)[i] = pbuf[i];
	return length;
}

/*
 *	Read operation does not require aligned address, and can read cross segments
 *	, but reserved zone in efuse cannot be read.
 */
static int jz_efuse_read(uint32_t xaddr, uint32_t xlen, void *buf)
{
	unsigned long flags;
	int ret = 0;

	if (xaddr + xlen < seg_addr[END] &&
			xaddr >= seg_addr[FIXBTROM] &&
			xaddr%4 == 0 && xlen%4 == 0) {
		spin_lock_irqsave(&efuse->lock, flags);
		clk_enable(efuse->clk);
		ret = set_efuse_cfg();
		if (ret) {
			clk_disable(efuse->clk);
			spin_unlock_irqrestore(&efuse->lock, flags);
			return ret;
		}
		pr_debug("addr = %p, length = %d\n", (void *)(xaddr + 0xb3540000), xlen);
		memcpy(buf, (void *)(efuse->iomem + xaddr), xlen);
		clk_disable(efuse->clk);
		spin_unlock_irqrestore(&efuse->lock, flags);
	} else {
		unsigned char *pbuf = buf;
		uint32_t addr = xaddr, length = xlen;
		while(length > 0) {
			pr_debug("addr = %x, length = %d, pbuf = %p, ret = %d\n", addr, length , pbuf, ret);
			ret = __jz_efuse_read(addr, length, pbuf);
			if (ret < 0)
				break;
			pbuf += ret;
			length -= ret;
			addr += ret;
		}
	}

	return ret;
}

int read_jz_efuse(uint32_t xaddr, uint32_t xlen, void *xbuf)
{
	int ret = -ENODEV;
	uint32_t addr = xaddr , len = xlen;
	void *buf = xbuf;
	if (efuse && get_device(efuse->dev)) {
		ret = jz_efuse_read(addr, len, buf);
		put_device(efuse->dev);
	}
	return ret;
}
EXPORT_SYMBOL_GPL(read_jz_efuse);

#if IS_ENABLED(CONFIG_JZ_EFUSE_WRITE)
static int __jz_efuse_write(uint32_t xaddr, uint32_t xlen, uint32_t* buf)
{
	int addr = (xaddr - 0x200) & 0x1ff;
	int total_words = (xlen + 3)/4;
	int write_cycles = (total_words + 7)/8;
	int cycle_write_words;
	uint32_t *pbuf = buf;
	int i = 0;
	int val, ret;
	unsigned long flags;

	spin_lock_irqsave(&efuse->lock, flags);
	clk_enable(efuse->clk);
	ret = set_efuse_cfg();
	if (ret) {
		clk_disable(efuse->clk);
		spin_unlock_irqrestore(&efuse->lock, flags);
		return ret;
	}
	for (i=0; i < write_cycles; i++) {
		for (cycle_write_words = 0;
				cycle_write_words < 8 && total_words > 0;
				total_words--, pbuf++, cycle_write_words++) {
			efuse_writel(*pbuf, EFUSE_DATA(cycle_write_words));
		}
		val =  addr << 21 | ((cycle_write_words*4) - 1) << 16;
		/*
		 * set write Programming address and data length
		 */
		efuse_writel(val, EFUSE_CTRL);
		/* Connect VDDQ pin from 2.5V */
		efuse_vddq_set(true);
		/*
		 * Programming EFUSE enable
		 */
		val = efuse_readl(EFUSE_CTRL);
		val |= 1 << 15;
		efuse_writel(val, EFUSE_CTRL);

		/* enable write */
		val = efuse_readl(EFUSE_CTRL);
		val |= 2;
		efuse_writel(val, EFUSE_CTRL);
		/* wait write done status */
		while(!(efuse_readl(EFUSE_STATE) & 0x2) ||
				(gpio_get_value(efuse->gpio_vddq_en) != efuse->gpio_vddq_en_level));
		if ((gpio_get_value(efuse->gpio_vddq_en)  != efuse->gpio_vddq_en_level) &&
				!(efuse_readl(EFUSE_STATE) & 0x2)) {
			efuse_writel(0, EFUSE_CTRL);
			clk_disable(efuse->clk);
			spin_unlock_irqrestore(&efuse->lock, flags);
			dev_err(efuse->dev, "efuse write timeout\n");
			return -EIO;
		}
		/* Disconnect VDDQ pin from 2.5V. */
		efuse_vddq_set(false);
		efuse_writel(0, EFUSE_CTRL);
		addr += (cycle_write_words * 4);
	}
	clk_disable(efuse->clk);
	spin_unlock_irqrestore(&efuse->lock, flags);
	return 0;
}

/*
 *	Write operation can just write trim, protect bit and fixbtrom segment in slt test or debugging
 *	Trim and protect segment:
 *		requires the segment starting address is fixed, and write the whole segment at a time
 *	Fixbtrom segment:
 *		the starting address needs 4 byte aligned
 *	Note : 1, normal kernel should not support write function
 */
static int jz_efuse_write(uint32_t xaddr, uint32_t xlen, uint32_t *buf)
{
	if (xaddr == seg_addr[TRIM0] ||
			xaddr == seg_addr[TRIM1] ||
			xaddr == seg_addr[TRIM2]) {
		if (xlen != 4) return -EINVAL;
	} else if (xaddr == seg_addr[TRIM3] ||
			xaddr == seg_addr[PROTECT_BIT]) {
		if (xlen != 2) return -EINVAL;
	} else if (xaddr > seg_addr[FIXBTROM] &&
			xaddr + xlen < seg_addr[END] &&
			(xaddr%4 == 0) && (xlen%4 == 0)) {
	} else {
		dev_err(efuse->dev, "efuse write not permitted addr %x length %x\n",
				xaddr, xlen);
		return -EPERM;
	}
	return __jz_efuse_write(xaddr, xlen, buf);
}
#else
static int jz_efuse_write(uint32_t xaddr, uint32_t xlen, uint32_t *buf)
{
	return -EPERM;
}
#endif

static long efuse_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct efuse_wr_info info;
	void * buf = NULL;
	int ret = 0;

	ret = copy_from_user(&info, (void __user *)arg, sizeof(struct efuse_wr_info));
	if (ret) {
		printk("efuse copy from user failed %d\n",__LINE__);
		return -EFAULT;
	}

	if (info.data_length) {
		buf = kzalloc(info.data_length*sizeof(char), GFP_KERNEL);
		if (!buf)
			return -ENOMEM;
	} else
		return-EINVAL;

	switch (cmd) {
	case EFUSE_CMD_READ:
		ret = jz_efuse_read(info.seg_addr, info.data_length, buf);
		if (ret < 0)
			goto out;
		ret = copy_to_user((void __user *)info.buf, buf, info.data_length);
		if (ret) {
			ret = -EFAULT;
			goto out;
		}
		break;
	case EFUSE_CMD_WRITE:
		ret = copy_from_user(buf, info.buf, info.data_length);
		if (ret) {
			ret = -EFAULT;
			goto out;
		}
		ret = jz_efuse_write(info.seg_addr, info.data_length, buf);
		if (ret < 0)
			goto out;
		ret = 0;
		break;
	default:
		ret = -ENOTTY;
		goto out;
	}
	ret = copy_to_user((void __user *)arg, &info, sizeof(struct efuse_wr_info));
	if (ret)
		ret = -EFAULT;
out:
	kfree(buf);
	return ret;
}

static int efuse_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int efuse_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations efuse_misc_fops = {
	.open		= efuse_open,
	.release	= efuse_release,
	.unlocked_ioctl	= efuse_ioctl,
};

#define DECLARE_EFUSE_PROC_FOPS(name)	\
static inline int efuse_##name##_open(struct inode *inode, struct file *file) {	\
	return single_open(file, efuse_show_##name, PDE_DATA(inode)); \
} \
static const struct file_operations efuse_##name##_fops ={	\
	.read = seq_read,	\
	.open = efuse_##name##_open,	\
	.release = single_release,	\
}

static int efuse_show_chip_id(struct seq_file *m, void *v)
{
	uint32_t buf[4];
	jz_efuse_read(seg_addr[CHIP_ID], 16, buf);
	return seq_printf(m, "chip id: %08x-%08x-%08x-%08x\n",
			buf[0], buf[1], buf[2], buf[3]);
}
DECLARE_EFUSE_PROC_FOPS(chip_id);

static int efuse_show_chip_num(struct seq_file *m, void *v)
{
	uint32_t buf[4];
	jz_efuse_read(seg_addr[CHIP_NUM], 16, buf);
	return seq_printf(m, "chip_num: %08x-%08x-%08x-%08x\n",
			buf[0], buf[1], buf[2], buf[3]);
}
DECLARE_EFUSE_PROC_FOPS(chip_num);

static int efuse_show_user_id(struct seq_file *m, void *v)
{
	uint32_t buf[4];
	jz_efuse_read(seg_addr[CUSTOMER_ID], 16, buf);
	return seq_printf(m, "user id: %08x-%08x-%08x-%08x\n",
			buf[0], buf[1], buf[2], buf[3]);
}
DECLARE_EFUSE_PROC_FOPS(user_id);

static int efuse_show_trim0(struct seq_file *m, void *v)
{
	uint32_t buf;
	jz_efuse_read(seg_addr[TRIM0], 4, &buf);
	return seq_printf(m, "trim0 id: %08x\n", buf);
}
DECLARE_EFUSE_PROC_FOPS(trim0);

static int efuse_show_trim1(struct seq_file *m, void *v)
{
	uint32_t buf;
	jz_efuse_read(seg_addr[TRIM1], 4, &buf);
	return seq_printf(m, "trim1 id: %08x\n", buf);
}
DECLARE_EFUSE_PROC_FOPS(trim1);

static int efuse_show_trim2(struct seq_file *m, void *v)
{
	uint32_t buf;
	jz_efuse_read(seg_addr[TRIM2], 4, &buf);
	return seq_printf(m, "trim2 id: %08x\n", buf);
}
DECLARE_EFUSE_PROC_FOPS(trim2);

static int efuse_show_trim3(struct seq_file *m, void *v)
{
	uint32_t buf;
	jz_efuse_read(seg_addr[TRIM3], 4, &buf);
	return seq_printf(m, "trim3 id: %08x\n", buf);
}
DECLARE_EFUSE_PROC_FOPS(trim3);

static int efuse_show_segment_map(struct seq_file *m, void *v)
{
	seq_printf(m,"seg1:    0x200-->0x20F ro [chip id]\n");
	seq_printf(m,"seg2:    0x210-->0x21F ro [chip num]\n");
	seq_printf(m,"seg3:    0x220-->0x22F ro [user id]\n");
	seq_printf(m,"seg4:    0x230-->0x233 %s [trim0]\n", IS_ENABLED(CONFIG_JZ_EFUSE_WRITE) ? "rw": "ro");
	seq_printf(m,"seg5:    0x234-->0x237 %s [trim1]\n", IS_ENABLED(CONFIG_JZ_EFUSE_WRITE) ? "rw": "ro");
	seq_printf(m,"seg6:    0x238-->0x23B %s [trim2]\n", IS_ENABLED(CONFIG_JZ_EFUSE_WRITE) ? "rw": "ro");
	seq_printf(m,"seg7:    0x23C-->0x23D %s [trim3]\n", IS_ENABLED(CONFIG_JZ_EFUSE_WRITE) ? "rw": "ro");
	seq_printf(m,"seg8:    0x23E-->0x23F %s [protect bit]\n", IS_ENABLED(CONFIG_JZ_EFUSE_WRITE) ? "rw": "ro");
	seq_printf(m,"seg9-12: 0x240-->0x27F -- [reserved zone]\n");
	seq_printf(m,"seg13:   0x280-->0x3FF %s [fix bootrom]\n", IS_ENABLED(CONFIG_JZ_EFUSE_WRITE) ? "rw": "ro");
	return 0;
}
DECLARE_EFUSE_PROC_FOPS(segment_map);

#if IS_ENABLED(CONFIG_JZ_EFUSE_WRITE)
static ssize_t efuse_vddq_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count) {
	if (strncmp(buf, "on", 2) == 0 || strncmp(buf, "1", 1) == 0||
			strncmp(buf, "On", 2) == 0 || strncmp(buf, "ON", 2) == 0)
		gpio_set_value(efuse->gpio_vddq_en, efuse->gpio_vddq_en_level);
	else
		gpio_set_value(efuse->gpio_vddq_en, !efuse->gpio_vddq_en_level);

	return count;
}
DEVICE_ATTR(vddq, S_IWUSR, NULL, efuse_vddq_store);
#endif

static int jz_efuse_probe(struct platform_device *pdev)
{
	struct jz_efuse_platform_data *pdata = NULL;
	struct proc_dir_entry *parent_entry = NULL;
	unsigned long flags;
	int ret = 0;

	efuse = devm_kzalloc(&pdev->dev , sizeof(struct jz_efuse), GFP_KERNEL);
	if (!efuse)
		return -ENOMEM;

	efuse->dev = &pdev->dev;

	spin_lock_init(&efuse->lock);

	/*Note: efuse's clock used nemc clock*/
	efuse->clk = devm_clk_get(&pdev->dev, "nemc");
	if (IS_ERR(efuse->clk))
		return PTR_ERR(efuse->clk);

	efuse->iomem = devm_ioremap(&pdev->dev, EFUSE_IOBASE, 0xfff);
	if (!efuse->iomem)
		return -EBUSY;

	spin_lock_init(&efuse->lock);

	pdata = pdev->dev.platform_data;
	if (pdata) {
		efuse->gpio_vddq_en = pdata->gpio_vddq_en;
		efuse->gpio_vddq_en_level = (int)(!!pdata->gpio_vddq_en_level);

		if (!efuse->gpio_vddq_en)
			return -ENODEV;

		if (efuse->gpio_vddq_en_level)
			flags = GPIOF_OUT_INIT_LOW;
		else
			flags = GPIOF_OUT_INIT_HIGH;

		ret = devm_gpio_request_one(&pdev->dev, efuse->gpio_vddq_en,
				flags, "efuse_vddq");
		if (ret)
			return ret;
	}
#if IS_ENABLED(CONFIG_JZ_EFUSE_WRITE)
	else {
		return -ENODEV;
	}
#endif

	efuse->mdev.minor = MISC_DYNAMIC_MINOR;
	efuse->mdev.name =  DRV_NAME;
	efuse->mdev.fops = &efuse_misc_fops;
	ret = misc_register(&efuse->mdev);
	if (ret) {
		return ret;
	}
#if IS_ENABLED(CONFIG_JZ_EFUSE_WRITE)
	ret = device_create_file(&pdev->dev, &dev_attr_vddq);
	if (ret) {
		misc_deregister(&efuse->mdev);
		return ret;
	}
	setup_timer(&efuse->vddq_protect_timer, efuse_clear_vddq, (unsigned long)efuse);
#endif
	if (!!(parent_entry = jz_proc_mkdir("efuse"))) {
		proc_create("chip_id", 0444, parent_entry , &efuse_chip_id_fops);
		proc_create("chip_num", 0444, parent_entry , &efuse_chip_num_fops);
		proc_create("user_id", 0444, parent_entry , &efuse_user_id_fops);
		proc_create("segment_map", 0444, parent_entry , &efuse_segment_map_fops);
		proc_create("trim0", 0444, parent_entry , &efuse_trim0_fops);
		proc_create("trim1", 0444, parent_entry , &efuse_trim1_fops);
		proc_create("trim2", 0444, parent_entry , &efuse_trim2_fops);
		proc_create("trim3", 0444, parent_entry , &efuse_trim3_fops);
	}
	efuse->parent_entry = parent_entry;

	platform_set_drvdata(pdev, efuse);
	dev_info(efuse->dev, "efuse registered success.\n");
	return 0;
}

static int jz_efuse_remove(struct platform_device *pdev)
{
	struct jz_efuse *efuse = platform_get_drvdata(pdev);
	if (efuse->parent_entry) {
		remove_proc_entry("chip_id", efuse->parent_entry);
		remove_proc_entry("chip_num", efuse->parent_entry);
		remove_proc_entry("user_id", efuse->parent_entry);
		remove_proc_entry("segment_map", efuse->parent_entry);
		proc_remove(efuse->parent_entry);
		efuse->parent_entry = NULL;
	}
	misc_deregister(&efuse->mdev);
#if IS_ENABLED(CONFIG_JZ_EFUSE_WRITE)
	device_remove_file(&pdev->dev, &dev_attr_vddq);
	del_timer_sync(&efuse->vddq_protect_timer);
#endif
	return 0;
}

static struct platform_driver jz_efuse_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
	},
	.probe		= jz_efuse_probe,
	.remove		= jz_efuse_remove,
};
module_platform_driver(jz_efuse_driver);

MODULE_DESCRIPTION("M200 efuse driver");
MODULE_AUTHOR("cli <chen.li@ingenic.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("20150401");
