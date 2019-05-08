#include <linux/module.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/wait.h>

#include "frizz_hal_if.h"
#include "frizz_poll.h"
#include "frizz_ioctl.h"
#include "frizz_serial.h"
#include "frizz_connection.h"
#include "frizz_timer.h"
#include "frizz_workqueue.h"
#include "frizz_data_sensor.h"
#include "frizz_data_mcc.h"
#include "frizz_debug.h"
#include "frizz_poll.h"
#include "frizz_gpio.h"
#include "frizz_file_id_list.h"
#include "frizz_ioctl_hardware.h"
#include "frizz_sysfs.h"
#include "frizz_hal_if.h"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION(DRIVER_NAME);

struct mutex burn_lock;
struct mutex close_lock;
struct list_head head;
struct frizz_platform_data pdata;
/*!< Major Number*/
static int major_no = 0;

/*!< Setting of module parameter*/
module_param(major_no, uint, 0);
static COPY_FROM_USER(sensor_enable_t)
//this recorder list is for sys_surface, nothing else.
sensor_data_changed_t record_sensor_list;


static void record_sensors(int id)
{
	//do not need a lock here, bc the data been read by sysfs, has been the static one,
	//by all the sensors have been open once
    if(id < 31) {
		if(!(record_sensor_list.aflag & 1 << id))
			record_sensor_list.aflag |= 1 << id;
	} else {
		if(!(record_sensor_list.mflag & 1 << (id - 31)))
			record_sensor_list.mflag |= 1 << (id - 31);
	}
}

static int frizz_open(struct inode *inode, struct file *file)
{
	//create file_node for each thread.
	struct file_id_node *node = 0;
    //lock and wait the burning done.
    mutex_lock(&burn_lock);

	node = (struct file_id_node *) kzalloc(sizeof(struct file_id_node), GFP_KERNEL);
    if(!node) {
	    printk("frizz malloc error in %s", __FUNCTION__);
		mutex_unlock(&burn_lock);
		return -1;
	}
	list_add_tail(&node->list, &head);
	file->private_data = (void*)node;
	mutex_unlock(&burn_lock);
    return 0;
}

static int frizz_close(struct inode *inode, struct file *file)
{
	struct list_head *plist;
	struct list_head *nlist;
	struct file_id_node *tmp;
	struct file_id_node *node;
	node = (struct file_id_node*)(file->private_data);
	list_for_each_safe(plist, nlist, &head)
	{
		tmp = list_entry(plist, struct file_id_node, list);
		if(node == tmp) {
			mutex_lock(&close_lock);
			list_del(plist);
			kfree(node);
			mutex_unlock(&close_lock);
		}
	}
    return 0;
}

static long frizz_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

    long ret = 0;
    sensor_enable_t sensor_enable = { 0,0 };
    //Get low 8bit. specific of linux ioctl.
    unsigned int cmd_id = cmd & 0xFF;
	struct file_id_node *node = (struct file_id_node*)(file->private_data);

    keep_frizz_wakeup();

    if ((FRIZZ_IOCTL_SENSOR <= cmd_id) && (cmd_id < FRIZZ_IOCTL_MCC)) {

        ret = frizz_ioctl_sensor(node, cmd, arg);

        if((cmd == FRIZZ_IOCTL_SENSOR_SET_ENABLE) && (!ret)) {
            copy_from_user_sensor_enable_t(cmd, (void*) arg,
            &sensor_enable);
            //this function is not multthread safety, but the designed is straight,
            //so be care for the sysfs read(get_sensor_list)
            record_sensors(sensor_enable.code);
        }
    } else if ((FRIZZ_IOCTL_MCC <= cmd_id) && (cmd_id < FRIZZ_IOCTL_HARDWARE)) {
        ret = frizz_ioctl_mcc(cmd, arg);
    } else {
        ret = frizz_ioctl_hardware(cmd, arg);
    }

    release_frizz_wakeup();

    return ret;
}

static unsigned int frizz_poll(struct file* file, poll_table* wait)
{
    return frizz_poll_sensor(file, wait);
}

/*! @struct frizz_operations
 *  @brief define system call handler
 */
static struct file_operations frizz_operations = {
    .owner = THIS_MODULE,
    .open = frizz_open,
    .release = frizz_close,
    .unlocked_ioctl = frizz_ioctl,
    .poll = frizz_poll
};
int __init frizz_init(void)
{
    int status;

    status = serial_open(&frizz_operations, &pdata);

    if (status == 0) {
        printk(KERN_INFO "%s driver init.\n", DRIVER_NAME);
        printk(KERN_INFO "%s driver(major:%d) installed.\n", DRIVER_NAME, major_no);

		mutex_init(&burn_lock);
		mutex_init(&close_lock);

		INIT_LIST_HEAD(&head);
        init_frizz_fifo_sem();
        init_frizz_workqueue();
		init_poll_queue();

        init_sensor_data_rwlock();
        init_mcc_data_rwlock();

        create_sysfs_interface();
        init_frizz_gpio(&pdata);
        workqueue_download_firmware(pdata.g_chip_orientation);
        enable_irq_gpio();
    } else {
        printk(KERN_ERR "%s driver failed \n", DRIVER_NAME);
        delete_frizz_workqueue();
        return -1;
    }

    return 0;
}

void __exit frizz_exit(void) {
	struct list_head *plist;
	struct list_head *nlist;
	struct file_id_node *node;
    serial_close();
	list_for_each_safe(plist, nlist, &head)
	{
		node = list_entry(plist, struct file_id_node, list);
		list_del(plist);
		kfree(node);
	}
    delete_frizz_workqueue();
    delete_frizz_gpio();

    printk(KERN_INFO "%s driver removed.\n", DRIVER_NAME);
}

//arch_init(frizz_init);
//arch_exit(frizz_exit);
