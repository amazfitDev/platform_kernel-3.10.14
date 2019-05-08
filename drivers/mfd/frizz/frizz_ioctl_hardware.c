#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/spinlock.h>

#include "frizz_ioctl.h"
#include "frizz_hal_if.h"
#include "frizz_serial.h"
#include "frizz_reg.h"
#include "frizz_debug.h"
#include "frizz_packet.h"
#include "inc/sensors/libsensors_id.h"
#include "inc/hub_mgr/hub_mgr_if.h"
#include "sensors.h"
#include "frizz_workqueue.h"
#include "frizz_gpio.h"
#include "frizz_connection.h"

#define ERR_FILE_BAD_FORM -77

const uint8_t Firmware_hex[] =
{
#include "frizz_firmware.h"
};

unsigned int chip_orientation;

extern const int config_verify;
extern struct mutex burn_lock;
//this is the the controller flag in frizz_connection.c
extern int on_isq;
#define GEN_CMD_CODE(sensor_id, on) \
    (uint32_t) (((uint32_t)((on)&0xFF)<<24) | \
    ((uint32_t)((sensor_id)&0xFF)<<16) | \
    ((uint32_t)((0x00)&0xFF)<<8) | \
    ((uint32_t)((0x00)&0xFF)))

static void print_fifo_data(unsigned int data[], int num) {
    int i;

    for(i = 0; i < num; i++) {
        printk("%08x ", data[i]);
    }
    printk("\n");
}

static int serial_read_fifo_size(void) {

    int status;
    unsigned int fifo_cnr;

    status = serial_read_reg_32(FIFO_CNR_REG_ADDR, &fifo_cnr);

    if(status == 0) {
        //printk("get FIFO counter: %08x\n", fifo_cnr);
        return (int)(fifo_cnr >> 16);
    } else {
        return -1;
    }
}

int frizz_fw_command_test(uint32_t sensor_id, uint32_t test_loop, unsigned long arg)
{
	int ret=0;
	int iii;
	int fifo_size,timeout;
	unsigned int fifo_data[MAX_HWFRIZZ_FIFO_SIZE];
	sensor_info *p_info = (sensor_info __user *)arg;
	int count = 0;

	/*  send command to frizz
	prefix    kind    sen_id    num
	0xFF      0x81    0xFF      0x01
	*/
	printk("RESPONSE_HUB_MGR = 0x%08x\n", RESPONSE_HUB_MGR);

	// ===================== Enable sensor =========================
	ret = serial_write_reg_32(MES_REG_ADDR, 0xFF81FF01);
	if (ret == 0)
		printk("frizz fw command write MES_REG_ADDR successful.\n");
	else
		printk("frizz fw command write MES_REG_ADDR failed.\n");

	udelay(1000);

	ret = serial_write_reg_32(MES_REG_ADDR, GEN_CMD_CODE(sensor_id, 0x01));
	if (ret == 0)
		printk("frizz fw command write MES_REG_ADDR successful.\n");
	else
	{
		printk("frizz fw command write MES_REG_ADDR failed.\n");
		return -1;
	}

	for(timeout = 0; timeout<50; timeout++)
	{
		udelay(1000);
		fifo_size = serial_read_fifo_size();
		if(fifo_size > 0)
		{
			memset(fifo_data, 0, MAX_HWFRIZZ_FIFO_SIZE);
			serial_read_reg_uint_array(FIFO_REG_ADDR, fifo_data, fifo_size);

			for(iii = 0; iii<fifo_size; iii++)
			{
				if(fifo_data[iii] == RESPONSE_HUB_MGR)
				{
					if(fifo_data[iii+2] != 0)
						return -1;		//sensor enable fail
					else
					{
						ret = 1;			// sensor enable pass
						break;
					}
				}
			}
			if(ret == 1) {
				break;
			}
		}
	}
	// ===================== Sensor Interval =========================
	ret = serial_write_reg_32(MES_REG_ADDR, 0xFF81FF02);
	udelay(1000);
	ret = serial_write_reg_32(MES_REG_ADDR, GEN_CMD_CODE(sensor_id, 0x02));
	udelay(1000);
	ret = serial_write_reg_32(MES_REG_ADDR, 1000);   // sameple 1000 --> interval time(unit : msec), min: 10msec

	ret = 0;
	for(timeout = 0; timeout<50; timeout++)
	{
		udelay(1000);
		fifo_size = serial_read_fifo_size();
		if(fifo_size > 0)
		{
			memset(fifo_data, 0, MAX_HWFRIZZ_FIFO_SIZE);
			serial_read_reg_uint_array(FIFO_REG_ADDR, fifo_data, fifo_size);

			for(iii = 0; iii<fifo_size; iii++)
			{
				if(fifo_data[iii] == RESPONSE_HUB_MGR)
				{
					if(fifo_data[iii+2] != 0)
						return -1;		//sensor enable fail
					else
					{
						printk("sensor interval pass.\n");

						ret = 1;			// sensor enable pass
						break;
					}
				}
			}
			if(ret == 1)
				break;
		}
	}
	// ===================== Get sensor raw data =========================
	while(test_loop--)
	{
		msleep(1000);

		fifo_size = serial_read_fifo_size();

		if(fifo_size > 0)
		{
			memset(fifo_data, 0, MAX_HWFRIZZ_FIFO_SIZE);
			serial_read_reg_uint_array(FIFO_REG_ADDR, fifo_data, fifo_size);
			print_fifo_data(fifo_data, fifo_size);

			printk("igit (fifo_size > 0)fifo_size = %d\n", fifo_size);
			//print_fifo_data(fifo_data, fifo_size);
			for(iii=0; iii<fifo_size; iii++)
				printk("CQF git (fifo_size > 0)fifo_data[%d] = [%8d]=[0x%08x].\n", iii, fifo_data[iii], fifo_data[iii]);
			printk(KERN_ERR "p_info->sensor_id = 0x%x\n", p_info->sensor_id);
			printk(KERN_ERR "p_info->test_loop = 0x%x\n", p_info->test_loop);
			if (copy_to_user(&p_info->raw_data[0], &fifo_data[0], fifo_size * sizeof(fifo_data[0]))) {
				printk(KERN_ERR "couldn't copy sensor raw data to user space\n");
			}

			count = fifo_size;
			break;
		}
		else
		{
			printk("git fifo_size = 0\n");
		}
	}


	// ===================== Disable sensor =========================
	ret = serial_write_reg_32(MES_REG_ADDR, 0xFF81FF01);
	if (ret == 0)
		printk("frizz fw command write MES_REG_ADDR successful.\n");
	else
		printk("frizz fw command write MES_REG_ADDR failed.\n");

	udelay(100);
	ret = serial_write_reg_32(MES_REG_ADDR, GEN_CMD_CODE(sensor_id, 0x00));

	for(timeout = 0; timeout<50; timeout++)
	{
		udelay(1000);
		fifo_size = serial_read_fifo_size();
		if(fifo_size > 0)
		{
			memset(fifo_data, 0, MAX_HWFRIZZ_FIFO_SIZE);
			serial_read_reg_uint_array(FIFO_REG_ADDR, fifo_data, fifo_size);

			for(iii = 0; iii<fifo_size; iii++)
			{
				if(fifo_data[iii] == RESPONSE_HUB_MGR)
				{
					if(fifo_data[iii+2] != 0)
						return -1;		//sensor disable fail
					else
					{
						ret = 1;			// sensor disable pass
						break;
					}
				}
			}
			if(ret == 1)
				break;
		}
	}
	udelay(100);
    return count;
}

int frizz_ioctl_hardware_stall(void) {
	int ret = 0;
    ret = serial_write_reg_32(CTRL_REG_ADDR, CTRL_REG_SYSTEM_RESET);
	if(ret)
		kprint("frizz i2c bus error, when reset the hardware!!, ret = %d", ret);
//after sw-reset, mast delay 100ms.
	msleep(100);
    ret = serial_write_reg_32(CTRL_REG_ADDR, CTRL_REG_STALL);
	if(ret)
		kprint("frizz i2c bus error, when stall the hardware!!, ret = %d", ret);

    return 0;
}

int frizz_ioctl_hardware_run(void) {

    serial_write_reg_32(CTRL_REG_ADDR, CTRL_REG_RUN);

    return 0;
}

int frizz_ioctl_hardware_reset(void) {
    unsigned int xdata = 0;
    on_isq = 0;
    keep_frizz_wakeup();
    if(serial_write_reg_32(CTRL_REG_ADDR, CTRL_REG_SYSTEM_RESET))
		kprint("frizz i2c bus error, when stall the hardware!!");
    serial_write_reg_32(CTRL_REG_ADDR, CTRL_REG_RUN);
    //fail frizz command when immediate reset command. so need to set delay 100ms.
	//set control reg bit10 ---> 0 to enter low-power mode
    msleep(100);
    serial_read_reg_32(MODE_REG_ADDR, &xdata);
    xdata &= ~((unsigned int)1 << 10);
    serial_write_reg_32(MODE_REG_ADDR, xdata);

    frizz_ioctl_sensor_get_version();
	init_g_chip_orientation(chip_orientation);
    frizz_ioctl_enable_gpio_interrupt();

	release_frizz_wakeup();
    return 0;
}

void read_file(struct file *filep, unsigned char* read_data, int read_data_size, int *read_file_size) {

    mm_segment_t fs;

    memset(read_data, 0, sizeof(read_data));

    fs = get_fs();
    set_fs(get_ds());

    *read_file_size = filep->f_op->read(filep, read_data, read_data_size, &filep->f_pos);

    set_fs(fs);
}
//this download function is depends on the *.bin which in the file-system.
//u can use this function only when the file-system regist.
int frizz_ioctl_hardware_download_firmware(char *name) {

    struct file *filep;
	int ret = ERR_FILE_BAD_FORM;

    int read_file_size;
    unsigned int header;
    int modified_ram_size;
    int data_size;
    unsigned int cmd;
    unsigned int ram_addr;
    unsigned char *write_ram_data;
    unsigned char *read_ram_data;

    unsigned char read_data[4];
	unsigned char tmp = 0;

    int i;

    mutex_lock(&burn_lock);
    //when redownload the firmware, the on_isq must been close and wait for open again.
    on_isq = 0;

    printk("frizz_ioctl_hardware_download_firmware(%s)\n", name);

    filep = filp_open(name, O_RDONLY | O_LARGEFILE, 0);

    if (IS_ERR(filep)) {
        printk(KERN_ERR "can't open %s (%d)\n", name, (int) filep);
        mutex_unlock(&burn_lock);
        return 1;
    }


    //before download, must set the frizz GPIO2 to high to wakeup the frizz.
	keep_frizz_wakeup();
    printk("frizz start download firmware by bin\n");
    frizz_ioctl_hardware_stall();
    do {
        read_file(filep, read_data, 2, &read_file_size);
        header = (read_data[0] << 8);
		tmp = read_data[1];

        if (header == 0xC900) {

            //get file data size. remove cmd (2byte) and ram addr (4byte)
            read_file(filep, read_data, 2, &read_file_size);
            data_size = ((tmp << 16) | (read_data[0] << 8) | (read_data[1])) - 6;

            //get frizz command (command of writting ram)
            read_file(filep, read_data, 2, &read_file_size);
            cmd = (read_data[0] << 8) | (read_data[1]);

            //get ram address
            read_file(filep, read_data, 4, &read_file_size);
            ram_addr = (read_data[0] << 24) | (read_data[1] << 16)
                    | (read_data[2] << 8) | (read_data[3]);

            //keep memory
            write_ram_data = kmalloc(data_size, GFP_KERNEL | GFP_DMA);
            read_file(filep, write_ram_data, data_size, &read_file_size);
            modified_ram_size = data_size + 3;
            modified_ram_size &= 0xFFFFFFFC;

            ret = serial_write_reg_ram_data(RAM_ADDR_REG_ADDR, ram_addr,
                    write_ram_data, modified_ram_size);
			if(ret){kfree(write_ram_data); break;}

            if(config_verify)
			{
                //keep memory (verify)
                read_ram_data = kmalloc(data_size, GFP_KERNEL | GFP_DMA);

                ret = serial_write_reg_32(RAM_ADDR_REG_ADDR, ram_addr);
				if(ret){kfree(write_ram_data); kfree(read_ram_data); break;}

                ret = serial_read_reg_array(RAM_DATA_REG_ADDR, read_ram_data,
                        modified_ram_size);
				if(ret){kfree(write_ram_data); kfree(read_ram_data); break;}

                for (i = 0; i < modified_ram_size; i++) {
                    if (write_ram_data[i] != read_ram_data[i]) {
                        break;
                    }
                }

                if (i == modified_ram_size) {
                    kprint("Verify Success. start address %x ", ram_addr);
                } else {
                    kprint("[%d] write ram data %x read ram data %x", i,
                            write_ram_data[i], read_ram_data[i]);
                    kprint("Verify Failed. start address %x ", ram_addr);
					kfree(write_ram_data);
					kfree(read_ram_data);
					ret = -1;
					break;
                }
                kfree(read_ram_data);
			}
            kfree(write_ram_data);

        } else if (header == 0xED00) {

            //don't execute command and skip data.
            read_file(filep, read_data, 2, &read_file_size);
            data_size = ((read_data[0] << 8) | (read_data[1]));

            read_file(filep, read_data, 2, &read_file_size);
            cmd = (read_data[0] << 8) | (read_data[1]);

            read_file(filep, read_data, 4, &read_file_size);
        }

    } while (read_file_size != 0);
	if(!ret) {
		printk("frizz download firmware by bin done\n");
	} else {
		kprint("frizz download firmware by bin failed!, ret = %d", ret);
	}
    frizz_ioctl_hardware_reset();

    filp_close(filep, NULL);
    release_frizz_wakeup();
    mutex_unlock(&burn_lock);
    return 0;
}

static int get_data(int offset, unsigned char *data, unsigned long length)
{
	unsigned long count;
	unsigned long length_left;
	length_left = sizeof(Firmware_hex)/sizeof(unsigned char) - offset;
	length = length_left >= length ? length : length_left;
	for(count = 0; count < length; count++ )
	{
		*(data + count) = *(Firmware_hex + offset + count);
	}
	return length;
}
//this download function is depend on the HEX-firmware include by *.h
//you can use it only when the driver regist, or changed *.h not __initdata type, then you can use it
//whenever u want
int frizz_download_firmware(unsigned int g_chip_orientation) {

	int ret = ERR_FILE_BAD_FORM;
	unsigned long offset = 0;
	unsigned long length = 0;

    unsigned int header;
    int modified_ram_size;
    int data_size;

    unsigned int cmd;
    unsigned int ram_addr;
    unsigned char *write_ram_data;
    unsigned char *read_ram_data;

    unsigned char read_data[4];
	unsigned char tmp = 0;

    int i;

    mutex_lock(&burn_lock);
    //when redownload the firmware, the on_isq must been close and wait for open again.
    on_isq = 0;

	chip_orientation = g_chip_orientation;
	length = sizeof(Firmware_hex)/sizeof(unsigned char);

    //before download the firmeware, must keep the frizz GPIO2 high to wakeup the frizz.
	keep_frizz_wakeup();
	printk("frizz start download firmware by hex \n");
    frizz_ioctl_hardware_stall();
    do {
        offset += get_data(offset, read_data, 2);
        header = (read_data[0] << 8);
		tmp = read_data[1];

        if (header == 0xC900) {

            //get file data size. remove cmd (2byte) and ram addr (4byte)
            offset += get_data(offset, read_data, 2);
            data_size = ((tmp << 16) | (read_data[0] << 8) | (read_data[1])) - 6;

            //get frizz command (command of writting ram)
            offset += get_data(offset, read_data, 2);
            cmd = (read_data[0] << 8) | (read_data[1]);

            //get ram address
            offset += get_data(offset, read_data, 4);
            ram_addr = (read_data[0] << 24) | (read_data[1] << 16)
                    | (read_data[2] << 8) | (read_data[3]);

            //keep memory
            write_ram_data = kmalloc(data_size, GFP_KERNEL | GFP_DMA);
            offset += get_data(offset, write_ram_data, data_size);

            modified_ram_size = data_size + 3;
            modified_ram_size &= 0xFFFFFFFC;

            ret = serial_write_reg_ram_data(RAM_ADDR_REG_ADDR, ram_addr,
                    write_ram_data, modified_ram_size);
			if(ret){kfree(write_ram_data); break;}

			if(config_verify)
			{
                //keep memory (verify)
                read_ram_data = kmalloc(data_size, GFP_KERNEL | GFP_DMA);

                ret = serial_write_reg_32(RAM_ADDR_REG_ADDR, ram_addr);
				if(ret){kfree(write_ram_data); kfree(read_ram_data); break;}

                ret = serial_read_reg_array(RAM_DATA_REG_ADDR, read_ram_data,
                    modified_ram_size);
				if(ret){kfree(write_ram_data); kfree(read_ram_data); break;}

                for (i = 0; i < modified_ram_size; i++) {
                    if (write_ram_data[i] != read_ram_data[i]) {
                        break;
                    }
                }

                if (i == modified_ram_size) {
                    kprint("Verify Success. start address %x ", ram_addr);
                } else {
                    printk("[%d] write ram data %x read ram data %x", i,
                            write_ram_data[i], read_ram_data[i]);
                    kprint("Verify Failed. start address %x ", ram_addr);
					kfree(write_ram_data);
					kfree(read_ram_data);
					ret = -1;
					break;
                }
            kfree(read_ram_data);
			}

            kfree(write_ram_data);

        } else if (header == 0xED00) {

            //don't execute command and skip data.
            offset += get_data(offset, read_data, 2);
            data_size = ((read_data[0] << 8) | (read_data[1]));

            offset += get_data(offset, read_data, 2);
            cmd = (read_data[0] << 8) | (read_data[1]);

            offset += get_data(offset, read_data, 4);
        }

    } while (offset < length);
    if(!ret) {
		printk("frizz download firmware by hex done \n");
	} else {
		kprint("frizz download firmware by hex failed!, ret = %d", ret);
	}
    frizz_ioctl_hardware_reset();
	release_frizz_wakeup();
	mutex_unlock(&burn_lock);
    return 0;
}

int frizz_ioctl_hardware(unsigned int cmd, unsigned long arg) {

    char file_name[64] = { };
    sensor_info test_sensor_info = { 0 };

    switch (cmd) {
    case FRIZZ_IOCTL_HARDWARE_RESET:
        frizz_ioctl_hardware_reset();
        break;

    case FRIZZ_IOCTL_HARDWARE_STALL:
        frizz_ioctl_hardware_stall();
        break;

    case FRIZZ_IOCTL_HARDWARE_DOWNLOAD_FIRMWARE:

        if (!access_ok(VERIFY_WRITE, (void __user * )arg, _IOC_SIZE(cmd))) {
            return -EFAULT;
        }
        if (copy_from_user(file_name, (int __user *) arg, sizeof(file_name))) {
            return -EFAULT;
        }
        frizz_ioctl_hardware_download_firmware(file_name);
        break;

    case FRIZZ_IOCTL_HARDWARE_ENABLE_GPIO:
        msleep(100);
        frizz_ioctl_enable_gpio_interrupt();
        break;

    case FRIZZ_IOCTL_FW_TEST:
        if (copy_from_user(&test_sensor_info, (int __user *) arg,
                sizeof(sensor_info))) {
            return -EFAULT;
        }
        return frizz_fw_command_test(test_sensor_info.sensor_id,
                test_sensor_info.test_loop, arg);

    default:
        return -1;
        break;
    }

    return 0;
}
