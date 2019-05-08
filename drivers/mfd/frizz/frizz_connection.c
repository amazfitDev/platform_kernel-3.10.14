#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/delay.h>

#include "frizz_reg.h"
#include "frizz_serial.h"
#include "frizz_debug.h"
#include "frizz_connection.h"
#include "inc/hub_mgr/hub_mgr_if.h"
#include "frizz_packet.h"
#include "frizz_gpio.h"
#include "frizz_hal_if.h"


unsigned int fifo_data[MAX_HWFRIZZ_FIFO_SIZE];

//this is a debug counter, add when the irq is coming.
extern int irq_counter;
struct semaphore frizz_fifo_sem;

volatile int interrupt_flag = 0;
volatile int fifo_count = 100;

volatile int on_isq = 0;
volatile int hub_res = HUB_MGR_CMD_RET_E_PARAMETER;

unsigned long Firmware_version = 0;

static struct mutex lock;

void init_frizz_fifo_sem(void) {
	sema_init(&frizz_fifo_sem, 1);
	mutex_init(&lock);
	init_callback();
}

int serial_read_fifo_size(void) {

	int status;
	unsigned int fifo_cnr;
	status = serial_read_reg_32(FIFO_CNR_REG_ADDR, &fifo_cnr);
	if (status == 0) {
		return (int) (fifo_cnr >> 16);
	} else {
		return status;
	}
	return fifo_cnr;
}

void print_fifo_data(int num) {
	int i;

	printk("dunm fifo data: ");

	for (i = 0; i < num; i++) {
		printk("%08x ", fifo_data[i]);
	}
	printk("\n");

	return;
}

int process_connection(packet_t packet) {
	int i, j;
	int ret = 0;
	int res = 0;

	mutex_lock(&lock);

	DEBUG_PRINT("packet header %x %x %x\n", packet.header.w, packet.data[0],
			packet.data[1]);

	//writing header
	res = serial_write_reg_32(MES_REG_ADDR, packet.header.w);
	if(res) {
		dump_stack();
		printk("%s serial failed. result = %d \n", __func__, res);
	}

	//send parameter
	for (i = 0; i < packet.header.num; i++) {
		for (j = 0; j < 25; j++) {
			if (!on_isq) {
				if(read_fifo() < 0)
					goto I2CERROR;
			}
			if (interrupt_flag == MESSAGE_ACK) {
				interrupt_flag = 0;
				DEBUG_PRINT("mk -->read packet data %d %x\n", i, packet.data[i]);
				res = serial_write_reg_32(MES_REG_ADDR, packet.data[i]);
				if(res) {
					dump_stack();
					printk("%s serial failed. result = %d \n", __func__, res);
				}
				break;
			}
			if (j == 23) {
				kprint("Frizz %s irq not respone, try poll mode %d", __func__, __LINE__);
				if(read_fifo() < 0)
				goto I2CERROR;
			}
			if (j == 24) {
				kprint("Frizz %s -- the lost command is =  %x, irq_counter = %d", __func__, packet.data[i], irq_counter);
				ret = ERR_NOSENSOR;
				break;
			}
			//this is a magic sleep time for waitting for the isq return.
			msleep(2);
		}
	}

	//waiting response
	for (j = 0; j < 25; j++) {
		if (!on_isq) {
			if(read_fifo() < 0)
				goto I2CERROR;
		}
		if (interrupt_flag == RESPONSE_HUB_MGR) {
			interrupt_flag = 0;
			if(hub_res != HUB_MGR_CMD_RET_OK) {
				ret = ERR_NOSENSOR;
			} else {
				ret = HUB_MGR_CMD_RET_OK;
				hub_res = HUB_MGR_CMD_RET_E_PARAMETER;
			}
			DEBUG_PRINT("RESPONSE_HUB_MGR\n");
			break;
		}
		if(j == 23) {
//			printk("Frizz %s irq not respone, try poll mode  %d\n", __func__, __LINE__);
			if(read_fifo() < 0)
				goto I2CERROR;
		}
		if(j == 24) {
			printk("%s HUB_DO_NOT_RESPONE, OUT OF TIME, ID: %x, irq_counter = %d , try poll mode\n", __func__, packet.header.sen_id, irq_counter);
		}
		msleep(2);
	}

	mutex_unlock(&lock);
	return ret;
I2CERROR:
	mutex_unlock(&lock);
	return ERR_NOSENSOR;
}

int read_fifo() {
	int fifo_size;
	int i;
	int ret = 0;
	unsigned int return_code;

    keep_frizz_wakeup();

	if (down_interruptible(&frizz_fifo_sem)) {
		kprint("%s: Error!!!!", __func__);
		return -ERESTARTSYS;
	}

	fifo_size = serial_read_fifo_size();
	if(fifo_size < 0) {
		dump_stack();
		ret = fifo_size;
		printk("%s serial failed. result = %d \n", __func__, ret);
		goto I2CERROR;
	}

	if (fifo_size > 0) {
		memset(fifo_data, 0, MAX_HWFRIZZ_FIFO_SIZE);

		ret = serial_read_reg_uint_array(FIFO_REG_ADDR, fifo_data, fifo_size);
		if(ret) {
			dump_stack();
			printk("%s serial failed. result = %d \n", __func__, ret);
			goto I2CERROR;
		}
#ifdef DEBUG_FRIZZ
		print_fifo_data(fifo_size);
#endif
		for (i = 0; i < fifo_size; i++) {

			return_code = fifo_data[i];
			switch (return_code) {
			case MESSAGE_ACK:
				interrupt_flag = MESSAGE_ACK;
				break;
			case RESPONSE_HUB_MGR_PEDO_INTER:
			case RESPONSE_HUB_MGR_GESTURE_INTER:
			case RESPONSE_HUB_MGR_ACC_ORI:
			case RESPONSE_HUB_MGR_GYRO_ORI:
			case RESPONSE_HUB_MGR_MAGN_ORI:
			case RESPONSE_HUB_MGR_FALL_PARA:
				interrupt_flag = RESPONSE_HUB_MGR;
				hub_res = (int)fifo_data[i + 2];
				break;
			case RESPONSE_HUB_MGR:
				interrupt_flag = RESPONSE_HUB_MGR;
				hub_res = (int)fifo_data[i + 2];
				DEBUG_PRINT("response %x %d\n", fifo_data[i + 1],
						fifo_data[i + 2]);

					//get the irq command commit
				if (fifo_data[i + 1] == 0x05010100) {
					printk("frizz_ioctl_enable_gpio_interrupt successed! \n");
					DEBUG_PRINT("On isq now ,stop the polling. \n");
					on_isq = 1;
				}
				//get the version
				if (fifo_data[i + 1] == 0x07000000) {
					Firmware_version = fifo_data[i + 2];
				}
				if (fifo_data[i + 1] == (HUB_MGR_CMD_PULL_SENSOR_DATA << 24)) {
					DEBUG_PRINT("FIFO SIZE %d\n", fifo_data[i + 2]);
					fifo_count = fifo_data[i + 2];
				}

				break;

			default:
				break;
			}

			if (((fifo_data[i] >> 16) == 0xFF80) && ((fifo_data[i] & 0xff)
					<= (fifo_size - i))) {
				analysis_fifo(fifo_data, &i);
			}

		}

	}

	release_frizz_wakeup();
	up(&frizz_fifo_sem);

	return 0;
I2CERROR:
	release_frizz_wakeup();
	up(&frizz_fifo_sem);
	return ret;
}
