#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include "frizz_connection.h"
#include "frizz_debug.h"
#include "frizz_ioctl_hardware.h"
#include "frizz_i2c.h"

typedef struct {
    struct work_struct work;
    packet_t data;
} frizz_work_t;

typedef struct {
	struct work_struct work;
	unsigned int g_chip_orientation;
} frizz_download_work_t;

static struct workqueue_struct *frizz_workqueue;

static frizz_work_t *frizz_work_read_fifo;
static frizz_work_t *frizz_work_process_connection;
static frizz_work_t *frizz_work_wakeup_by_raise;
static frizz_download_work_t *frizz_download_work;

void function_read_fifo(struct work_struct *work) {
    read_fifo();
}

void function_download_work(struct work_struct *work) {
	frizz_download_work_t *frizz_work = (frizz_download_work_t *)work;
	frizz_download_firmware(frizz_work->g_chip_orientation);
}

void function_process_connection(struct work_struct *work) {
    frizz_work_t *frizz_work = (frizz_work_t *)work;
    process_connection(frizz_work->data);
}

void function_frizz_work_wakeup_by_raise(struct work_struct *work) {
	wakeup_system_by_raise(1);
}

int init_frizz_workqueue(void) {
    frizz_workqueue = create_workqueue("frizz_queue");

    if (frizz_workqueue) {

        frizz_work_read_fifo = NULL;
        frizz_work_read_fifo = (frizz_work_t *) kmalloc(sizeof(frizz_work_t),
                GFP_KERNEL);

        if (frizz_work_read_fifo) {
            INIT_WORK((struct work_struct * )frizz_work_read_fifo,
                    function_read_fifo);
        } else {
            printk("frizz malloc failed ---- frizz_work_read_fifo\n");
		}

        frizz_work_process_connection = NULL;
        frizz_work_process_connection = (frizz_work_t *) kmalloc(
                sizeof(frizz_work_t), GFP_KERNEL);

        if (frizz_work_process_connection) {
            INIT_WORK((struct work_struct * )frizz_work_process_connection,
                    function_process_connection);
        } else {
		    printk("frizz malloc failed ---- frizz_work_process_connection\n");
		}

		frizz_download_work = NULL;
		frizz_download_work = (frizz_download_work_t *)kmalloc(
				sizeof(frizz_download_work_t), GFP_KERNEL);
		if(frizz_download_work) {
			INIT_WORK((struct work_struct * )frizz_download_work,
					function_download_work);
		} else {
		    printk("frizz malloc failed ---- frizz_download_work\n");
		}
		frizz_work_wakeup_by_raise = NULL;
		frizz_work_wakeup_by_raise = (frizz_work_t *) kmalloc(
                sizeof(frizz_work_t), GFP_KERNEL);
		if(frizz_work_wakeup_by_raise) {
			INIT_WORK((struct work_struct * )frizz_work_wakeup_by_raise,
                    function_frizz_work_wakeup_by_raise);
		} else {
			printk("frizz malloc failed --- frizz_work_wakeup_by_raise \n");
		}

        return 0;
    } else {
        return -1;
    }
}

void delete_frizz_workqueue(void) {

    kfree(frizz_work_read_fifo);
    kfree(frizz_work_process_connection);
	kfree(frizz_download_work);

    flush_workqueue(frizz_workqueue);

    destroy_workqueue(frizz_workqueue);
}

int workqueue_analysis_fifo(void)
{
    if(work_pending( (struct work_struct *)frizz_work_process_connection) == 0) {
        return queue_work(frizz_workqueue, (struct work_struct *)frizz_work_read_fifo);
    } else {
        return -1;
    }
//    return queue_work(frizz_workqueue, (struct work_struct *)frizz_work_read_fifo);
}

//this function only been called when the driver regist, or the system will broken by the *.h is __initdata
int workqueue_download_firmware(unsigned int chip_orientation) {
	frizz_download_work->g_chip_orientation = chip_orientation;
	return queue_work(frizz_workqueue, (struct work_struct *)frizz_download_work);
}

int create_frizz_workqueue(void *data) {
    int ret = 0;
    ret = process_connection(*(packet_t*)data);

    return ret;
}
int send_wakeup_cmd(void) {
	return queue_work(frizz_workqueue, (struct work_struct *)frizz_work_wakeup_by_raise);
}
