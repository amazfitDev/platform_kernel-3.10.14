#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/mm.h>
#include <linux/mtd/mtd.h>
#include <linux/vmalloc.h>
//#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/processor.h>

#define DRIVER_NAME "nv_ro"

static int nv_major = 0;
static int nv_minor = 0;
static int nv_devs = 1;

struct nv_devices {
	struct mtd_info *mtd;
	char *buf;
	struct mutex	mutex;
	unsigned int count;
	unsigned int mtd_device_size;
	unsigned int nv_start_base;	/* nv write/read region start_address */
	unsigned int wr_base;
	unsigned int wr_size;
	unsigned int wr_erase_size;
	unsigned int nv_count;
	struct cdev cdev;	  /* Char device structure */
};

#define MTD_DEVICES_ID 0
#define MTD_DEVICES_SIZE (32 * 1024)
#define NV_AREA_START (256 * 1024)
#define NV_AREA_SIZE MTD_DEVICES_SIZE
#define NV_AREA_END (288 * 1024)

static struct nv_devices *nv_dev;
static struct class *nv_class;

static int nv_ro_open(struct inode *inode, struct file *filp)
{
	int err = 0;
	mutex_lock(&nv_dev->mutex);
	if(!nv_dev->count) {
		nv_dev->mtd = get_mtd_device(NULL, MTD_DEVICES_ID);
		if (IS_ERR(nv_dev->mtd)) {
			err = PTR_ERR(nv_dev->mtd);
			pr_err("error: cannot get MTD device\n");
			goto open_fail;
		}
		nv_dev->mtd_device_size = MTD_DEVICES_SIZE;
		nv_dev->buf = vmalloc(nv_dev->mtd_device_size);
		if (!nv_dev->buf) {
			pr_err("error: cannot allocate memory\n");
			err = -1;
			goto open_fail;
		}
		nv_dev->nv_count = 0;
		nv_dev->nv_start_base = NV_AREA_START;
		nv_dev->wr_base = nv_dev->nv_start_base;
		nv_dev->wr_size = nv_dev->mtd_device_size;
		nv_dev->wr_erase_size = nv_dev->mtd_device_size;
		if(nv_dev->wr_base & (nv_dev->wr_size - 1)) {
			pr_err("error:  addr must 0x%x align\n", nv_dev->wr_size);
			err = -1;
			goto open_fail;
		}
	}
	nv_dev->count++;
open_fail:
	mutex_unlock(&nv_dev->mutex);

	return err;
}
static int nv_ro_release(struct inode *inode, struct file *filp)
{
	mutex_lock(&nv_dev->mutex);
	nv_dev->count--;
	if(!nv_dev->count) {
		vfree(nv_dev->buf);
		put_mtd_device(nv_dev->mtd);
	}
	mutex_unlock(&nv_dev->mutex);

	return 0;
}

static loff_t nv_ro_llseek(struct file *filp, loff_t offset, int orig)
{
	mutex_lock(&nv_dev->mutex);
	switch (orig) {
	case 0:
		filp->f_pos = offset;
		break;
	case 1:
		filp->f_pos += offset;
		if (filp->f_pos > nv_dev->wr_size)
			filp->f_pos = nv_dev->wr_size;
		break;
	case 2:
		filp->f_pos = nv_dev->wr_size;
		break;
	default:
		return -EINVAL;
	}
	mutex_unlock(&nv_dev->mutex);

	return filp->f_pos;
}

static int nv_read_area(char *buf, size_t count, loff_t offset)
{
	unsigned int addr;
	size_t read;
	int err;

	addr = nv_dev->wr_base + offset;
	/* printk("read addr == %x offset = 0x%x\n", addr, (unsigned int)offset); */

	err = mtd_read(nv_dev->mtd, addr, count, &read, (u_char *)buf);
	if (mtd_is_bitflip(err))
		err = 0;
	if (unlikely(err || read != count)) {
		pr_err("error: read failed at 0x%llx\n",
		       (long long)addr);
		if (!err)
			err = -EINVAL;
	}
	return err;
}

static ssize_t nv_ro_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	int err = 0;

	if(count > nv_dev->wr_size) {
		pr_err("error:  read count 0x%x more than nv max size 0x%x\n",
			count, nv_dev->wr_size);
		err = -EFAULT;
		return err;
	}
	if(*f_pos + count > nv_dev->wr_size) {
		pr_err("error:  read current location %x plus count 0x%x more than nv max size 0x%x\n",
		       (unsigned int)*f_pos, count, nv_dev->wr_size);
		err = -EFAULT;
		return err;
	}

	mutex_lock(&nv_dev->mutex);
	err =nv_read_area (nv_dev->buf, count, *f_pos);
	if (err < 0 || copy_to_user((void *)buf, (void*)nv_dev->buf, count))
		err = -EFAULT;
	mutex_unlock(&nv_dev->mutex);
	return err;
}

static int nv_mmap(struct file *file, struct vm_area_struct *vma)
{
	return 0;
}

/* Driver Operation structure */
static struct file_operations nv_fops = {
	.owner = THIS_MODULE,
	.read =  nv_ro_read,
	.open = nv_ro_open,
	.llseek = nv_ro_llseek,
	.release = nv_ro_release,
};

static int __init nv_ro_init(void)
{
	int result, devno;
	dev_t dev = 0;

	/*
	 * Get a range of minor numbers to work with, asking for a dynamic
	 * major unless directed otherwise at load time.
	 */
	if (nv_major) {
		dev = MKDEV(nv_major, nv_minor);
		result = register_chrdev_region(dev, nv_devs, DRIVER_NAME);
	} else {
		result = alloc_chrdev_region(&dev, nv_minor, nv_devs,
					     DRIVER_NAME);
		nv_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "nor_nv: can't get major %d\n", nv_major);
		goto fail;
	}

	nv_class = class_create(THIS_MODULE, DRIVER_NAME);

	/*
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
	nv_dev = kmalloc(nv_devs * sizeof(struct nv_devices), GFP_KERNEL);
	if (!nv_dev) {
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(nv_dev, 0, nv_devs * sizeof(struct nv_devices));

	devno = MKDEV(nv_major, nv_minor);

	cdev_init(&nv_dev->cdev, &nv_fops);
	nv_dev->cdev.owner = THIS_MODULE;
	nv_dev->cdev.ops = &nv_fops;
	result = cdev_add(&nv_dev->cdev, devno, 1);

	if (result) {
		printk(KERN_NOTICE "Error %d:adding vprivilege\n", result);
		goto err_free_mem;
	}

	device_create(nv_class, NULL, devno, NULL, DRIVER_NAME);

	mutex_init(&nv_dev->mutex);

	printk("register vprivilege driver OK! Major = %d\n", nv_major);

	return 0; /* succeed */

err_free_mem:
	kfree(nv_dev);
fail:
	return result;
}

static void __exit nv_ro_exit(void)
{
	dev_t devno = MKDEV(nv_major, nv_minor);

	/* Get rid of our char dev entries */
	if (nv_dev) {
		cdev_del(&nv_dev->cdev);
		kfree(nv_dev);
	}

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, nv_devs);

	device_destroy(nv_class, devno);
	class_destroy(nv_class);

	return;
}
module_init(nv_ro_init);
module_exit(nv_ro_exit);

MODULE_AUTHOR("bliu<bo.liu@ingenic.com>");
MODULE_DESCRIPTION("mtd nor nv area read only");
MODULE_LICENSE("GPL");
