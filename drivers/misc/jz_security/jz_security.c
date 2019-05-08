/*
 * linux/drivers/misc/jz_security/jz_security.c - Ingenic security driver
 *
 * Copyright (C) 2015 Ingenic Semiconductor Co., Ltd.
 * Author: liu yang <king.lyang@ingenic.com>.
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
#include <linux/dma-mapping.h>

#include <soc/base.h>
#include <jz_proc.h>
#include "jz_security.h"

extern int init_seboot(void);
extern int change_aes_key(struct change_key_param *);
extern void cpu_do_aes(struct aes_param *, unsigned int dmamode,unsigned int cbcmode);
extern int do_rsa(struct rsa_param *);

struct jz_security {
	struct device      *dev;
	struct miscdevice  mdev;
	struct mutex       lock;
	void __iomem       *iomem;
	unsigned int       open_count;
};
struct jz_security *security;

static int security_open(struct inode *inode, struct file *filp)
{
	debug_printk("security_init\n");
	struct miscdevice *dev = filp->private_data;
	struct jz_security *security = container_of(dev,struct jz_security, mdev);

	mutex_lock(&security->lock);
	security->open_count++;

	debug_printk("open_count = %d\n",security->open_count);
	if(security->open_count == 1){
		init_seboot();
	}else {
		mutex_unlock(&security->lock);
		return -EBUSY;
	}
	mutex_unlock(&security->lock);

	return 0;
}

static int security_release(struct inode *inode, struct file *filp)
{
	struct miscdevice *dev = filp->private_data;
	struct jz_security *security = container_of(dev,struct jz_security, mdev);

	mutex_lock(&security->lock);
	security->open_count--;
	debug_printk("open_count = %d\n",security->open_count);
	mutex_unlock(&security->lock);

	return 0;
}

static long security_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct miscdevice *dev = filp->private_data;
	struct jz_security *security = container_of(dev, struct jz_security, mdev);
	int ret = 0;
	struct change_key_param key_param;
	struct aes_param  aes_p;
	struct rsa_param  rsa_p;
	unsigned int ker_rsa_enc_data[31];
	unsigned int ker_nku_kr[62];
	unsigned int ker_input[31];
	unsigned int ker_output[31];
	unsigned int ker_k[31];
	unsigned int ker_n[31];

	void __user *argp = (void __user *)arg;
	unsigned int i=0;
	switch (cmd) {
	case SECURITY_INTERNAL_CHANGE_KEY:
			debug_printk("\n\n test ---->setup aes key\n");
		if (copy_from_user(&key_param, argp, sizeof(struct change_key_param))) {
			dev_err(security->dev, "copy_from_user error!!!,[%s,%d]\n",__func__,__LINE__);
			 return -EFAULT;
		}
		{
			unsigned int * user_rsa_enc_data = key_param.rsa_enc_data;
			unsigned int * user_nku_kr = key_param.nku_kr;

			copy_from_user(ker_rsa_enc_data, user_rsa_enc_data,sizeof(ker_rsa_enc_data));
			copy_from_user(ker_nku_kr, user_nku_kr, sizeof(ker_nku_kr));
			key_param.rsa_enc_data = ker_rsa_enc_data;
			key_param.nku_kr = ker_nku_kr;
			ret = change_aes_key(&key_param);
		}
		break;
	case SECURITY_INTERNAL_AES:
		if (copy_from_user(&aes_p, argp, sizeof(struct aes_param))) {
			dev_err(security->dev, "copy_from_user error!!!,[%s,%d]\n",__func__,__LINE__);
			return -EFAULT;
		}
		{
			debug_printk("\n\n test ---->aes in [CPU mode]\n");
			unsigned int *user_input = aes_p.input;
			unsigned int *user_output = aes_p.output;
			if(copy_from_user(ker_input,user_input,sizeof(ker_input))) {
				dev_err(security->dev, "copy_from_user error!!!,[%s,%d]\n",__func__,__LINE__);
				return -EFAULT;
			}
			aes_p.input = ker_input;
			aes_p.output = ker_output;
			cpu_do_aes(&aes_p, 0, 0);

			aes_p.input = user_input;
			aes_p.output = user_output;

			if (copy_to_user(argp, &aes_p, sizeof(struct aes_param))) {
				dev_err(security->dev, "copy_to_user error!!!,[%s,%d]\n",__func__,__LINE__);
				return -EFAULT;
			}
			if(copy_to_user(user_output,ker_output,sizeof(ker_output))) {
				dev_err(security->dev, "copy_to_user error!!!,[%s,%d]\n",__func__,__LINE__);
				return -EFAULT;
			}
			for(i=0;i<4;i++)
				debug_printk("aes_p_output[%d]= 0x%x\n",i, aes_p.output[i]);

		}
		break;
	case SECURITY_RSA:
		debug_printk("\n\n test ---->rsa encode\n");
		if (copy_from_user(&rsa_p, argp, sizeof(struct rsa_param))) {
			 dev_err(security->dev, "copy_from_usrr error!!!,[%s,%d]\n",__func__,__LINE__);
			return -EFAULT;
		}
		{
			unsigned int *user_input = rsa_p.input;
			unsigned int *user_output = rsa_p.output;
			unsigned int *user_k = rsa_p.key;
			unsigned int *user_n = rsa_p.n;
			copy_from_user(ker_input, user_input,sizeof(ker_input));
			copy_from_user(ker_k, user_k, sizeof(ker_k));
			copy_from_user(ker_n,user_n,sizeof(ker_n));
			rsa_p.input = ker_input;
			rsa_p.output = ker_output;
			rsa_p.key = ker_k;
			rsa_p.n = ker_n;
			do_rsa(&rsa_p);
			rsa_p.input = user_input;
			rsa_p.output = user_output;
			rsa_p.key = user_k;
			rsa_p.n = user_n;
			if (copy_to_user(argp, &rsa_p, sizeof(struct rsa_param))) {
				dev_err(security->dev, "copy_to_user error!!!,[%s,%d]\n",__func__,__LINE__);
				return -EFAULT;
			}
			copy_to_user(user_output, ker_output, sizeof(ker_output));
		}
		break;
	default:
		ret = -1;
		printk("no support other cmd\n");
	}
	return ret;
}
static struct file_operations security_misc_fops = {
	.open		= security_open,
	.release	= security_release,
	.unlocked_ioctl	= security_ioctl,
};

static int jz_security_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct clk *devclk;

	security= kzalloc(sizeof(struct jz_security), GFP_KERNEL);
	if (!security) {
		printk("security:malloc faile\n");
		return -ENOMEM;
	}

	security->dev = &pdev->dev;

	devclk = clk_get(NULL, "aes");
	if (IS_ERR(devclk)) {
		dev_err(security->dev, "get devclk rate fail!\n");
		return -1;
	}
	clk_enable(devclk);

	security->iomem = ioremap(SECURITY_IOBASE, 0x44);
	if (!security->iomem) {
		dev_err(security->dev, "ioremap failed!\n");
		ret = -EBUSY;
		goto fail_free_io;
	}

	security->mdev.minor = MISC_DYNAMIC_MINOR;
	security->mdev.name =  DRV_NAME;
	security->mdev.fops = &security_misc_fops;

	ret = misc_register(&security->mdev);
	if (ret < 0) {
		dev_err(security->dev, "misc_register failed\n");
		goto fail;
	}
	platform_set_drvdata(pdev, security);

	mutex_init(&security->lock);

	dev_info(security->dev, "ingenic security interface module registered success.\n");

	return 0;
fail:
fail_free_io:
	iounmap(security->iomem);

	return ret;
}


static int jz_security_remove(struct platform_device *dev)
{
	struct jz_security *security = platform_get_drvdata(dev);


	misc_deregister(&security->mdev);
	iounmap(security->iomem);
	kfree(security);

	return 0;
}

static struct platform_driver jz_security_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
	},
	.probe		= jz_security_probe,
	.remove		= jz_security_remove,
};

static int __init jz_security_init(void)
{
	return platform_driver_register(&jz_security_driver);
}

static void __exit jz_security_exit(void)
{
	platform_driver_unregister(&jz_security_driver);
}


module_init(jz_security_init);
module_exit(jz_security_exit);

MODULE_DESCRIPTION("X1000 security driver");
MODULE_AUTHOR("liu yang <king.lyang@ingenic.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("20151028");
