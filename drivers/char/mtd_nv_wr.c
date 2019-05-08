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

#define DRIVER_NAME "nv_wr"

static int nv_major = 0;
static int nv_minor = 0;
static int nv_devs = 1;

#define CMD_WRITE_URL_1 _IOW('N', 0, char)
#define CMD_WRITE_URL_2 _IOW('N', 1, char)
#define CMD_READ_URL_1  _IOR('N', 3, char)
#define CMD_READ_URL_2  _IOR('N', 4, char)
#define CMD_EARASE_ALL  _IOR('N', 5, int)
#define CMD_GET_INFO    _IOR('N', 6, unsigned int)
#define CMD_GET_VERSION    _IOR('N', 7, unsigned int)
#define CMD_CHANGE_START_ADDR    _IOR('N', 8, unsigned int)
#define CMD_CHANGE_MTD_SIZE    _IOR('N', 9, unsigned int)

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
#define MTD_EARASE_SIZE MTD_DEVICES_SIZE
#define NV_AREA_START (288 * 1024)
#define NV_AREA_SIZE MTD_DEVICES_SIZE
#define NV_AREA_END (384 * 1024)
#define DEFAULT_START_FLAG (0x5a5a5a5a)
#define DEFAULT_END_FLAG (0xa5a5a5a5)

#define WRITE_FLAG 0
#define READ_FLAG 1

static struct nv_devices *nv_dev;
static struct class *nv_class;

static int nv_open(struct inode *inode, struct file *filp)
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
static int nv_release(struct inode *inode, struct file *filp)
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
static loff_t nv_llseek(struct file *filp, loff_t offset, int orig)
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
static int nv_map_area(int flag)
{
	unsigned int buf[3][2];
	unsigned int current_nv_num = 0, i;
	int tmp, err;

	tmp = -1;
	for(i = 0; i < 3; i++) {
		nv_dev->wr_base = nv_dev->nv_start_base + i * nv_dev->wr_size;
		err = nv_read_area((char *)buf[i], 4, 0);
		if(err < 0)
			continue;
		if(buf[i][0] == DEFAULT_START_FLAG) {
			err = nv_read_area((char *)buf[i], 8, nv_dev->wr_size - 8);
			if(err < 0)
				continue;
			if(buf[i][1] == DEFAULT_END_FLAG) {
				tmp = 0;
				if(nv_dev->nv_count <= buf[i][0]) {
					nv_dev->nv_count = buf[i][0];
					current_nv_num = i;
				}
			}
		}
	}

	if(tmp) {
		for(i = 0; i < 3; i++) {
			if(buf[i][0] == 0xffffffff || buf[i][0] == 0)
				continue;
			break;
		}
		if(i < 3) {
			printk("WARN: not found right nv wr region!!!!!buf[%d][0] = %x\n", i, buf[i][0]);
			return tmp;
		}
	}

	for(i = 0; i < 3; i++) {
		if(flag == READ_FLAG) {
			tmp = (current_nv_num + i) % 3;
		} else if(flag == WRITE_FLAG) {
			tmp = (current_nv_num + i + 1) % 3;
		}
		nv_dev->wr_base = nv_dev->nv_start_base + tmp * nv_dev->wr_size;
		err = nv_read_area((char *)buf[i], 4, 0);
		if(err < 0)
			continue;
		break;
	}
	if(i == 3) {
		printk("WARN: ALL nv wr region read error\n");
		return -1;
	}
	if(flag == WRITE_FLAG) {
		nv_dev->nv_count++;
	}

	return 0;
}
static int nv_map_and_read_area(char *buf, size_t count, loff_t offset)
{
	int err;

	err = nv_map_area(READ_FLAG);
	if(!err)
		err = nv_read_area(buf, count, offset);

	return err;
}
static ssize_t nv_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
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
	err = nv_map_and_read_area(nv_dev->buf, count, *f_pos);
	if (err < 0 || copy_to_user((void *)buf, (void*)nv_dev->buf, count))
		err = -EFAULT;
	mutex_unlock(&nv_dev->mutex);
	return err;
}
static int erase_eraseblock(unsigned int addr, unsigned int count)
{
	int err;
	struct erase_info ei;

	memset(&ei, 0, sizeof(struct erase_info));
	ei.mtd  = nv_dev->mtd;
	ei.addr = addr;
	ei.len  = count;

	err = mtd_erase(nv_dev->mtd, &ei);
	if (unlikely(err)) {
		pr_err("error %d while erasing NV 0x%x\n", err, addr);
		return err;
	}

	if (unlikely(ei.state == MTD_ERASE_FAILED)) {
		pr_err("some erase error occurred at NV %d\n",
		       addr);
		return -EIO;
	}

	return 0;
}
static int nv_map_and_write_area(const char *buf, size_t count, loff_t offset)
{
	unsigned int addr;
	size_t written;
	int err;

	err = nv_map_area(WRITE_FLAG);
	if(err < 0)
		return err;
	*(unsigned int *)(&buf[0]) = DEFAULT_START_FLAG;
	*(unsigned int *)(&buf[nv_dev->wr_size - 8]) = nv_dev->nv_count;
	*(unsigned int *)(&buf[nv_dev->wr_size - 4]) = DEFAULT_END_FLAG;

	addr = nv_dev->wr_base + offset;
	err = erase_eraseblock(addr, nv_dev->wr_erase_size);
	if (unlikely(err)) {
		pr_err("error: erase failed at 0x%llx\n",
		       (long long)addr);
		if (!err)
			err = -EINVAL;
		return err;
	}
	err = mtd_write(nv_dev->mtd, addr, count, &written, (u_char *)buf);
	if (unlikely(err || written != count)) {
		pr_err("error: write failed at 0x%llx\n",
		       (long long)addr);
		if (!err)
			err = -EINVAL;
		return err;
	}
	return 0;
}
static ssize_t nv_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int err = 0;

	if(count != nv_dev->wr_size) {
		pr_err("error:  write count 0x%x must equl 0x%x\n",
			count, nv_dev->wr_size);
		err = -EFAULT;
		return err;
	}
	if(*f_pos + count > nv_dev->wr_size) {
		pr_err("error:  write current location %x plus count 0x%x more than nv max size 0x%x\n",
		       (unsigned int)*f_pos, count, nv_dev->wr_size);
		err = -EFAULT;
		return err;
	}
	mutex_lock(&nv_dev->mutex);
	if (copy_from_user(nv_dev->buf, (void *)buf, count)) {
		err = -EFAULT;
		mutex_unlock(&nv_dev->mutex);
		return err;
	}
	err = nv_map_and_write_area(nv_dev->buf, count, *f_pos);
	mutex_unlock(&nv_dev->mutex);

	return err;
}
struct wr_info {
	char *wr_buf[512];
	unsigned int size;
};
static struct wr_info * get_wr_info( unsigned long arg)
{
	unsigned int size;
	struct wr_info *wr_info = NULL;

	if(arg) {
		wr_info = (struct  wr_info *)arg;
		size = wr_info->size;
		if(size > 512) {
			pr_err("err: ioctl size  must little 512\n");
			return NULL;
		}
	}
	return wr_info;
}

static long nv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	struct  wr_info *wr_info = NULL;
	char *dst_buf;
	unsigned int offset = 0;

	switch (cmd) {
	case CMD_WRITE_URL_1:
		offset = 4;
	case CMD_WRITE_URL_2:
		if(!offset)
			offset = 516;
		{
			if((wr_info = get_wr_info(arg)) == NULL)
				return -1;
			mutex_lock(&nv_dev->mutex);
			err = nv_map_and_read_area(nv_dev->buf, nv_dev->wr_size, 0);
			if(err < 0) {
				pr_err("err: ioctl read err\n");
				mutex_unlock(&nv_dev->mutex);
				return -1;
			}
			dst_buf = nv_dev->buf + offset;
			if (copy_from_user(dst_buf, (void *)wr_info->wr_buf, wr_info->size))
			{
				pr_err("err: ioctl copy_from_user err\n");
				mutex_unlock(&nv_dev->mutex);
				return -1;
			}
			err = nv_map_and_write_area(nv_dev->buf, nv_dev->wr_size, 0);
			mutex_unlock(&nv_dev->mutex);
			break;
		}
	case CMD_READ_URL_1:
		offset = 4;
	case CMD_READ_URL_2:
		if(!offset)
			offset = 516;
		{
			if((wr_info = get_wr_info(arg)) == NULL)
				return -1;
			mutex_lock(&nv_dev->mutex);
			err = nv_map_and_read_area(nv_dev->buf, wr_info->size, offset);
			if(err) {
				mutex_unlock(&nv_dev->mutex);
				return -1;
			}
			wr_info->size = strlen(nv_dev->buf);
			if(wr_info->size > 512)
				wr_info->size = 512;
			if (copy_to_user(wr_info->wr_buf, (void *)nv_dev->buf, wr_info->size))
			{
				pr_err("err: ioctl copy_from_user err\n");
				mutex_unlock(&nv_dev->mutex);
				return -1;
			}
			mutex_unlock(&nv_dev->mutex);
			break;

		}
	case CMD_EARASE_ALL:
		mutex_unlock(&nv_dev->mutex);
		err = erase_eraseblock(nv_dev->wr_base, nv_dev->wr_size);
		mutex_unlock(&nv_dev->mutex);
		break;
	case CMD_GET_INFO:
		mutex_unlock(&nv_dev->mutex);
		*(unsigned int *)arg = nv_dev->wr_size;
		mutex_unlock(&nv_dev->mutex);
		break;
	case CMD_GET_VERSION:
		mutex_unlock(&nv_dev->mutex);
		offset = 1028;
		dst_buf = nv_dev->buf + offset;
		err = nv_map_and_read_area(dst_buf, 4, offset);
		*(unsigned int *)arg = *(unsigned int *)dst_buf;
		mutex_unlock(&nv_dev->mutex);
		break;
	case CMD_CHANGE_START_ADDR:
		mutex_unlock(&nv_dev->mutex);
		if(!nv_dev->count) {
			nv_dev->nv_start_base = *(unsigned int *)arg;
		}else{
			pr_err("ERROR:current devices is used on others\n");
			err = -1;
		}
		mutex_unlock(&nv_dev->mutex);
		break;
	case CMD_CHANGE_MTD_SIZE:
		mutex_unlock(&nv_dev->mutex);
		if(!nv_dev->count) {
			vfree(nv_dev->buf);
			nv_dev->mtd_device_size = *(unsigned int *)arg;
			nv_dev->buf = vmalloc(nv_dev->mtd_device_size);
			if (!nv_dev->buf) {
				pr_err("error: cannot allocate memory\n");
				err = -1;
			}
		}else{
			pr_err("ERROR:current devices is used on others\n");
			err = -1;
		}
		mutex_unlock(&nv_dev->mutex);
		break;
	default:
		pr_err("not support cmd %x\n", cmd);
		err = -1;
	}
	return err;
}

static int nv_mmap(struct file *file, struct vm_area_struct *vma)
{
	return 0;
}

/* Driver Operation structure */
static struct file_operations nv_fops = {
	.owner = THIS_MODULE,
	.write = nv_write,
	.read =  nv_read,
	.open = nv_open,
	.unlocked_ioctl	= nv_ioctl,
	.mmap	= nv_mmap,
	.llseek = nv_llseek,
	.release = nv_release,
};

static int __init nv_init(void)
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

static void __exit nv_exit(void)
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
module_init(nv_init);
module_exit(nv_exit);

MODULE_AUTHOR("bliu<bo.liu@ingenic.com>");
MODULE_DESCRIPTION("mtd nor nv area write and read");
MODULE_LICENSE("GPL");
