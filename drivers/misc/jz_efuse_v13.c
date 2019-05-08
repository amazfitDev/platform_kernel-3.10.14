/*
 * linux/drivers/misc/jz_efuse_v12.c - Ingenic efuse driver
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 * Author: liu bo <bliu@ingenic.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


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
#include <linux/seq_file.h>

#include <soc/base.h>
#include <mach/jz_efuse.h>
#include <jz_proc.h>

/*#define DEBUG*/
#define DRV_NAME "jz-efuse-v13"

#define CMD_GET_CHIPID  _IOW('E', 0, unsigned int)
#define CMD_GET_RANDOM  _IOW('E', 1, unsigned int)
#define CMD_GET_USERID  _IOR('E', 2, unsigned int)
#define CMD_GET_PROTECTID  _IOR('E', 3, unsigned int)
#define CMD_WRITE       _IOR('E', 4, unsigned int)
#define CMD_READ        _IOR('E', 5, unsigned int)

#define EFUSE_CTRL		0x0
#define EFUSE_CFG		0x4
#define EFUSE_STATE		0x8
#define EFUSE_DATA(n)		(0xC + (n)*4)
#define CUSTID_PRT (1 << 15)
#define CHIP_PRT (1 << 14)

#define CHIP_ID_ADDR	(0x200)
#define RN_ADDR		(0x210)
#define CUT_ID_ADDR	(0x220)
#define PTR_ADDR	(0x23e)

uint32_t seg_addr[] = {
	CHIP_ID_ADDR,
	RN_ADDR,
	CUT_ID_ADDR,
	PTR_ADDR,
};
struct efuse_wr_info {
	uint32_t seg_id;
	uint32_t data_length;
	uint32_t *buf;
};

struct jz_efuse {
	struct jz_efuse_platform_data *pdata;
	struct device *dev;
	struct miscdevice mdev;
	uint32_t *id2addr;
	struct efuse_wr_info *wr_info;
	spinlock_t lock;
	void __iomem *iomem;
	int gpio_vddq_en_n;
	struct timer_list vddq_protect_timer;
};
struct jz_efuse *efuse;

static uint32_t efuse_readl(uint32_t reg_off)
{
	return readl(efuse->iomem + reg_off);
}


static void efuse_writel(uint32_t val, uint32_t reg_off)
{
	writel(val, efuse->iomem + reg_off);
}

static void efuse_vddq_set(unsigned long is_on)
{
	if (is_on) {
		mod_timer(&efuse->vddq_protect_timer, jiffies + HZ);
	}

	gpio_set_value(efuse->gpio_vddq_en_n, !is_on);
}

static int efuse_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int efuse_release(struct inode *inode, struct file *filp)
{
	/*clear configer register*/
	/*efuse_writel(0, EFUSE_CFG);*/
	return 0;
}
int jz_efuse_read(uint32_t seg_id, uint32_t data_length, uint32_t *buf)
{
	int bit_num, word_num;
	unsigned long flags;
	unsigned int val, addr = 0;
	unsigned int offset;
	bit_num = data_length * 8;
	switch(seg_id){
		case CHIP_ID:
			if(bit_num != 128) {
				dev_err(efuse->dev, "read segment %d data must length %d = 128 bit", seg_id, bit_num);
				return -1;
			}
			val = efuse_readl(EFUSE_STATE);
			if(val & CHIP_PRT) {
				dev_err(efuse->dev, "segment chip_id is protected\n");
				return -1;
			}
			break;
		case RANDOM_ID:
			if(bit_num != 128) {
				dev_err(efuse->dev, "read segment %d data must length %d = 128 bit", seg_id, bit_num);
			}
			break;
		case USER_ID:
			if(bit_num != 256) { /* read must word align */
				dev_err(efuse->dev, "read segment %d data must length %d = 256 bit", seg_id, bit_num);
				return -1;
			}
			val = efuse_readl(EFUSE_STATE);
			if(val & CUSTID_PRT) {
				dev_err(efuse->dev, "segment user_id is protected\n");
			}
			break;
		case PROTECT_ID:
			if(bit_num > 32) { /* read must word align */
				dev_err(efuse->dev, "read segment %d data length %d > 32 bit", seg_id, bit_num);
				return -1;
			}
			break;
		default:
			dev_warn(efuse->dev, "segment id set err!\n");
			return -1;
	}
	addr = efuse->id2addr[seg_id];
	spin_lock_irqsave(&efuse->lock, flags);
	/* set read Programming address and data length */
	data_length = data_length - 1;
	if(seg_id == PROTECT_ID)
		data_length = 1;
	if(seg_id == USER_ID)
		data_length = 29;
	val =  addr << 21 | data_length << 16;
	efuse_writel(val, EFUSE_CTRL);
	/* enable read */
	val = efuse_readl(EFUSE_CTRL);
	val |= 1;
	efuse_writel(val, EFUSE_CTRL);
	/* wait read done status */
	while(!(efuse_readl(EFUSE_STATE) & 1));

	word_num = data_length / 4;
	word_num += data_length % 4 ? 1 : 0;

	if(word_num > 8) {
		dev_warn(efuse->dev, "strongly recommend operate each segment separately\n");
	} else {
		for(offset = 0; offset < word_num; offset ++) {
			val = efuse_readl(EFUSE_DATA(offset));
			if(offset == 0 && seg_id == PROTECT_ID) {
				val &= 0xffff;
				data_length = 1;
			}
			if(offset == 7 && seg_id == USER_ID) {
				val &= 0xffff;
				data_length = 29;
			}
			*(buf + offset) = val;
		}
	}
	/* clear read done staus */
	efuse_writel(0, EFUSE_STATE);
	spin_unlock_irqrestore(&efuse->lock, flags);
	return word_num;
}
EXPORT_SYMBOL_GPL(jz_efuse_read);


static int jz_efuse_write(uint32_t seg_id, uint32_t data_length, uint32_t *buf)
{
	unsigned long flags;
	unsigned int val, bit_num;
	unsigned int addr, word_num;
	unsigned int offset;

	bit_num = data_length * 8;
	switch(seg_id){
		case CHIP_ID:
			if(bit_num != 128) {
				dev_err(efuse->dev, "write segment %d data must length %d = 128 bit", seg_id, bit_num);
				return -1;
			}
			val = efuse_readl(EFUSE_STATE);
			if(val & CHIP_PRT) {
				dev_err(efuse->dev, "segment chip_id is protected\n");
				return -1;
			}
			break;
		case RANDOM_ID:
			dev_err(efuse->dev, "segment %d only support read\n", seg_id);
			return -1;
		case USER_ID:
			if(bit_num != 256) { /* write must word align */
				dev_err(efuse->dev, "write segment %d data must length %d > 256 bit", seg_id, bit_num);
				return -1;
			}
			val = efuse_readl(EFUSE_STATE);
			if(val & CUSTID_PRT) {
				dev_err(efuse->dev, "segment user_id is protected\n");
				return -1;
			}
			break;
		case PROTECT_ID:
			if(bit_num > 32) { /* write must word align */
				dev_err(efuse->dev, "write segment %d data length %d > 32 bit", seg_id, bit_num);
				return -1;
			}
			break;
		default:
			dev_warn(efuse->dev, "segment id set err!\n");
			return -1;
	}
	addr = efuse->id2addr[seg_id];

	spin_lock_irqsave(&efuse->lock, flags);
	word_num = data_length / 4;
	word_num += data_length % 4 ? 1 : 0;
	data_length = data_length - 1;
	if(word_num > 8) {
		dev_warn(efuse->dev, "strongly recommend operate each segment separately\n");
	} else {
		for(offset = 0; offset < word_num; offset ++) {
			val = *(buf + offset);
			if(offset == 0 && seg_id == PROTECT_ID) {
				val &= 0xffff;
				data_length = 1;
			}
			if(offset == 7 && seg_id == USER_ID) {
				val &= 0xffff;
				data_length = 29;
			}
			efuse_writel(val, EFUSE_DATA(offset));
		}
	}

	/*
	 * set write Programming address and data length
	 */
	val =  addr << 21 | data_length << 16 | 1 << 15;
	efuse_writel(val, EFUSE_CTRL);
	/* Connect VDDQ pin from 2.5V */
	efuse_vddq_set(1);
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
	while(!(efuse_readl(EFUSE_STATE) & 0x2));

	/* Disconnect VDDQ pin from 2.5V. */
	efuse_vddq_set(0);

	efuse_writel(0, EFUSE_CTRL);
	spin_unlock_irqrestore(&efuse->lock, flags);

	return word_num;
}

static long efuse_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct miscdevice *dev = filp->private_data;
	struct jz_efuse *efuse = container_of(dev, struct jz_efuse, mdev);
	unsigned int * arg_r;
	int ret = 0;

	arg_r =(unsigned int *)arg;

	efuse->wr_info = (struct efuse_wr_info *)arg_r;
	switch (cmd) {
	case CMD_GET_CHIPID:
	case CMD_GET_RANDOM:
	case CMD_GET_USERID:
	case CMD_GET_PROTECTID:
	case CMD_READ:
		ret = jz_efuse_read(efuse->wr_info->seg_id, efuse->wr_info->data_length, efuse->wr_info->buf);
		break;
	case CMD_WRITE:
		ret = jz_efuse_write(efuse->wr_info->seg_id, efuse->wr_info->data_length, efuse->wr_info->buf);
		break;
	default:
		ret = -1;
		printk("no support other cmd\n");
	}
	return ret;
}
static struct file_operations efuse_misc_fops = {
	.open		= efuse_open,
	.release	= efuse_release,
	.unlocked_ioctl	= efuse_ioctl,
};

static int efuse_read_chip_id_proc(struct seq_file *m, void *v)
{
	int len = 0, i;
	unsigned int buf[4] = {0};

	jz_efuse_read(CHIP_ID, 16, buf);
	len = seq_printf(m,"--------> chip id: ");
	for(i = 0; i < 4; i++) {
		len += seq_printf(m,"%08x",buf[i]);
	}
	len += seq_printf(m,"\n");
	return len;
	return len;
}

static int efuse_read_user_id_proc(struct seq_file *m, void *v)
{
	int len = 0, i;
	unsigned int buf[8] = {0};

	jz_efuse_read(USER_ID, 32,buf);
	len = seq_printf(m,"--------> user id: ");
	for(i = 0; i < 8; i++)
		len += seq_printf(m,"%08x",buf[i]);
	len += seq_printf(m,"\n");

	return len;
}

static int efuse_read_chipID_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, efuse_read_chip_id_proc, PDE_DATA(inode));
}
static int efuse_read_userID_proc_open(struct inode *inode,struct file *file)
{
	return single_open(file, efuse_read_user_id_proc, PDE_DATA(inode));
}

static const struct file_operations efuse_proc_read_chipID_fops ={
	.read = seq_read,
	.open = efuse_read_chipID_proc_open,
	.write = NULL,
	.llseek = seq_lseek,
	.release = single_release,
};
static const struct file_operations efuse_proc_read_userID_fops ={
	.read = seq_read,
	.open = efuse_read_userID_proc_open,
	.write = NULL,
	.llseek = seq_lseek,
	.release = single_release,
};

static int jz_efuse_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct clk *h2clk;
	struct clk *devclk;
	unsigned long rate;
	uint32_t val, ns;
	int i, rd_strobe, wr_strobe;
	uint32_t rd_adj, wr_adj;
	struct proc_dir_entry *res,*p;

	h2clk = clk_get(NULL, "h2clk");
	if (IS_ERR(h2clk)) {
		dev_err(efuse->dev, "get h2clk rate fail!\n");
		return -1;
	}
	devclk = clk_get(NULL, "efuse");
	if (IS_ERR(devclk)) {
		dev_err(efuse->dev, "get efuse clk rate fail!\n");
		return -1;
	}
	clk_enable(devclk);

	rate = clk_get_rate(h2clk);
	ns = 1000000000 / rate;
	printk("rate = %lu, ns = %d\n", rate, ns);

	efuse= kzalloc(sizeof(struct jz_efuse), GFP_KERNEL);
	if (!efuse) {
		printk("efuse:malloc faile\n");
		return -ENOMEM;
	}

	efuse->pdata = pdev->dev.platform_data;
	if(!efuse->pdata) {
		dev_err(&pdev->dev, "No platform data\n");
		ret = -1;
		goto fail_free_efuse;
	}

	efuse->dev = &pdev->dev;
	efuse->gpio_vddq_en_n = -ENODEV;

	efuse->gpio_vddq_en_n = efuse->pdata->gpio_vddq_en_n;
	if (efuse->gpio_vddq_en_n == -ENODEV) {
		dev_err(efuse->dev, "gpio error\n");
		ret = -ENODEV;
		goto fail_free_efuse;
	}

	efuse->iomem = ioremap(EFUSE_IOBASE, 0xfff);
	if (!efuse->iomem) {
		dev_err(efuse->dev, "ioremap failed!\n");
		ret = -EBUSY;
		goto fail_free_efuse;
	}

	ret = gpio_request(efuse->gpio_vddq_en_n, dev_name(efuse->dev));
	if (ret) {
		dev_err(efuse->dev, "Failed to request gpio pin: %d\n", ret);
		goto fail_free_io;
	}
	ret = gpio_direction_output(efuse->gpio_vddq_en_n, 1); /* power off by default */
	if (ret) {
		dev_err(efuse->dev, "Failed to set gpio as output: %d\n", ret);
		goto fail_free_gpio;
	}

	dev_info(efuse->dev, "setup vddq_protect_timer!\n");
	setup_timer(&efuse->vddq_protect_timer, efuse_vddq_set, 0);
	add_timer(&efuse->vddq_protect_timer);

	spin_lock_init(&efuse->lock);

	efuse->mdev.minor = MISC_DYNAMIC_MINOR;
	efuse->mdev.name =  DRV_NAME;
	efuse->mdev.fops = &efuse_misc_fops;

	spin_lock_init(&efuse->lock);

	ret = misc_register(&efuse->mdev);
	if (ret < 0) {
		dev_err(efuse->dev, "misc_register failed\n");
		goto fail_free_gpio;
	}
	platform_set_drvdata(pdev, efuse);


	efuse->id2addr = seg_addr;
	efuse->wr_info = NULL;


	for(i = 0; i < 0x4; i++)
		if((( i + 1) * ns ) > 2)
			break;
	if(i == 0x4) {
		dev_err(efuse->dev, "get efuse cfg rd_adj fail!\n");
		return -1;
	}
	rd_adj = wr_adj = i;

	for(i = 0; i < 0x8; i++)
		if(((rd_adj + i + 3) * ns ) > 15)
			break;
	if(i == 0x8) {
		dev_err(efuse->dev, "get efuse cfg rd_strobe fail!\n");
		return -1;
	}
	rd_strobe = i;

	for(i = 0; i < 0x1f; i += 100) {
		val = (wr_adj + i + 916) * ns;
		if( val > 4 * 1000 && val < 6 *1000)
			break;
	}
	if(i >= 0x1f) {
		dev_err(efuse->dev, "get efuse cfg wd_strobe fail!\n");
		return -1;
	}
	wr_strobe = i;

	dev_info(efuse->dev, "rd_adj = %d | rd_strobe = %d | "
		 "wr_adj = %d | wr_strobe = %d\n", rd_adj, rd_strobe,
		 wr_adj, wr_strobe);
	/*set configer register*/
	val = rd_adj << 20 | rd_strobe << 16 | wr_adj << 12 | wr_strobe;
	efuse_writel(val, EFUSE_CFG);

	clk_put(h2clk);

	p = jz_proc_mkdir("efuse");
	if (!p) {
		pr_warning("create_proc_entry for common efuse failed.\n");
		return -ENODEV;
	}

	res = proc_create("efuse_chip_id", 0444,p,&efuse_proc_read_chipID_fops);
	if(!res){
		pr_err("create proc of efuse_chip_id error!!!!\n");
	}
	res = proc_create("efuse_user_id",0444,p,&efuse_proc_read_userID_fops);
	if(!res){
		pr_err("create proc of efuse_user_id error!!!!\n");
	}



	dev_info(efuse->dev, "ingenic efuse interface module registered success.\n");
	return 0;

fail_free_efuse:
	kfree(efuse);
fail_free_gpio:
	gpio_free(efuse->gpio_vddq_en_n);
fail_free_io:
	iounmap(efuse->iomem);

	return ret;
}


static int jz_efuse_remove(struct platform_device *dev)
{
	struct jz_efuse *efuse = platform_get_drvdata(dev);


	misc_deregister(&efuse->mdev);
	if (efuse->gpio_vddq_en_n != -ENODEV) {
		gpio_free(efuse->gpio_vddq_en_n);
		dev_info(efuse->dev, "del vddq_protect_timer!\n");
		del_timer(&efuse->vddq_protect_timer);
	}
	iounmap(efuse->iomem);
	kfree(efuse);

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

static int __init jz_efuse_init(void)
{
	return platform_driver_register(&jz_efuse_driver);
}

static void __exit jz_efuse_exit(void)
{
	platform_driver_unregister(&jz_efuse_driver);
}


module_init(jz_efuse_init);
module_exit(jz_efuse_exit);

MODULE_DESCRIPTION("X1000 efuse driver");
MODULE_AUTHOR("liu bo <bo.liu@ingenic.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("20151123");
