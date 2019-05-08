#include <asm/uaccess.h>
#include <linux/math64.h>
#include "frizz_workqueue.h"
#include "frizz_data_sensor.h"
#include "frizz_debug.h"
//#include "frizz_packet.h"
#include "frizz_ioctl.h"
#include "frizz_sensor_list.h"
#include "inc/sensors/libsensors_id.h"
#include "inc/hub_mgr/hub_mgr_if.h"
#include "frizz_timer.h"
#include "frizz_connection.h"
#include "frizz_hal_if.h"
#include <linux/delay.h>
#include "frizz_serial.h"
#include "sensors.h"
#include "frizz_chip_orientation.h"

static COPY_FROM_USER(sensor_enable_t)
static COPY_FROM_USER(sensor_delay_t)
static COPY_FROM_USER(batch_t)
static COPY_FROM_USER(pedometer_counter_t)

static int Gsensor_min_delay = 10000;
static int clean_pedo_data(void);

unsigned int pedo_onoff = 0;
unsigned int gesture_on = 0;
int frizz_ioctl_enable_gpio_interrupt(void)
{
    packet_t packet;

    packet.header.prefix = 0xFF;
    packet.header.type = 0x81;
    packet.header.sen_id = HUB_MGR_ID;
    packet.header.num = 1;

    packet.data[0] =
            HUB_MGR_GEN_CMD_CODE(HUB_MGR_CMD_SET_SETTING, 0x01, 0x01, 0x00);

    create_frizz_workqueue((void*) &packet);

    return 0;
}

int frizz_ioctl_sensor_get_version() {
    packet_t packet;
    packet.header.prefix = 0xFF;
    packet.header.type = 0x81;
    packet.header.sen_id = 0xFF;
    packet.header.num = 1;
    packet.data[0] = 0x07000000;
    return create_frizz_workqueue((void*) &packet);
}

unsigned int set_sensor_active(int sensor_id) {
    if ((sensor_id == SENSOR_ID_ACCEL_MOVE)) {
        return HUB_MGR_GEN_CMD_CODE(HUB_MGR_CMD_SET_SENSOR_ACTIVATE, sensor_id,
                0x00, 0x01);
    } else {
        return HUB_MGR_GEN_CMD_CODE(HUB_MGR_CMD_SET_SENSOR_ACTIVATE, sensor_id,
                0x00, 0x01);
    }
}

int frizz_ioctl_sensor(struct file_id_node *node, unsigned int cmd, unsigned long arg) {

    int sensor_id;
    int status = 0;
    packet_t packet;
    int period_ms;

    sensor_enable_t sensor_enable = { 0 };
    sensor_delay_t sensor_delay = { 0 };
    pedometer_counter_t pedo_counter = { 0 };
    batch_t batch = { 0 };

    packet.header.prefix = 0xFF;
    packet.header.type = 0x81;
    packet.header.sen_id = HUB_MGR_ID;

    switch (cmd) {

    case FRIZZ_IOCTL_SENSOR_SET_ENABLE:

        status = copy_from_user_sensor_enable_t(cmd, (void*) arg,
                &sensor_enable);

        sensor_id = convert_code_to_id(sensor_enable.code);

        packet.header.num = 1;

        switch (sensor_enable.flag) {
        case 0:
            packet.data[0] = HUB_MGR_GEN_CMD_CODE(HUB_MGR_CMD_DEACTIVATE_SENSOR,
                    sensor_id, 0x00, 0x00);
            break;
        case 1:
            packet.data[0] = set_sensor_active(sensor_id);
            break;
        }

        DEBUG_PRINT("SENSOR_SET_ENABLE %x %d \n", packet.data[0], sensor_id);

        status = create_frizz_workqueue((void*) &packet);
        //if is the pedometer sensor shutdown, clean the data in the firmware
        if((sensor_enable.code == SENSOR_TYPE_STEP_COUNTER) && (sensor_enable.flag == 0)) {
			pedo_onoff = 0;
        } else if((sensor_enable.code == SENSOR_TYPE_STEP_COUNTER) && (sensor_enable.flag == 1)){
			pedo_onoff = 1;
		}
		if(sensor_enable.code == SENSOR_TYPE_GESTURE) {
			if(sensor_enable.flag == 1) {
				gesture_on = 1;
			}else if(sensor_enable.flag == 0) {
				gesture_on = 0;
			}
		}
		//if close the gsensor, set the Gsensor_min_delay to default
		if(sensor_enable.code == SENSOR_TYPE_ACCELEROMETER && sensor_enable.flag == 0) {
			Gsensor_min_delay = 10000;
		}

		if(sensor_enable.code == SENSOR_TYPE_ACCEL_FALL_DOWN || sensor_enable.code == SENSOR_TYPE_ACCEL_POS_DET) {
			set_fall_parameter();
		}

        break;

    case FRIZZ_IOCTL_SENSOR_SET_DELAY:

        copy_from_user_sensor_delay_t(cmd, (void*) arg, &sensor_delay);
        sensor_id = convert_code_to_id(sensor_delay.code);

        packet.header.num = 2;

        packet.data[0] = HUB_MGR_GEN_CMD_CODE(HUB_MGR_CMD_SET_SENSOR_INTERVAL,
                sensor_id, 0x00, 0x00);
        packet.data[1] = sensor_delay.ms;
		if(sensor_delay.code == SENSOR_TYPE_ACCELEROMETER) {
			if(sensor_delay.ms < Gsensor_min_delay) {
				packet.data[1] = sensor_delay.ms;
				Gsensor_min_delay = sensor_delay.ms;
			} else {
				break;
			}
		}

        DEBUG_PRINT("SENSOR_SET_DELAY %d %x\n", sensor_id, packet.data[0]);

        status = create_frizz_workqueue((void*) &packet);

        break;

    case FRIZZ_IOCTL_SENSOR_GET_ENABLE:
        get_sensor_enable(cmd, (void*) arg);
        break;

    case FRIZZ_IOCTL_SENSOR_GET_DELAY:
        get_sensor_delay(cmd, (void*) arg);
        break;

    case FRIZZ_IOCTL_SENSOR_GET_DATA:
        get_sensor_data(node, cmd, (void*) arg);
        break;

    case FRIZZ_IOCTL_SENSOR_SIMULATE_TIMEOUT:
        simulate_timeout(cmd, (void*) arg);
        break;

    case FRIZZ_IOCTL_SENSOR_SET_BATCH:
        copy_from_user_batch_t(cmd, (void*) arg, &batch);
        sensor_id = convert_code_to_id(batch.code);

        DEBUG_PRINT("FIFO_FULL_FLAG %d %d \n", batch.fifo_full_flag,
                batch.code);
        if (batch.fifo_full_flag == 1) {

            update_fifo_full_flag(batch.fifo_full_flag);
        }

        update_timeout(batch.timeout_ns);

        if (batch.period_ns < 1000000) {
            period_ms = 1;
            DEBUG_PRINT("1ms\n");
        } else {
            period_ms = div64_u64(batch.period_ns, 1000000);
            DEBUG_PRINT("period ms %d\n", period_ms);
        }
        DEBUG_PRINT("batch sensor id %d %d \n", sensor_id, period_ms);
        packet.header.num = 2;
        packet.data[0] = HUB_MGR_GEN_CMD_CODE(HUB_MGR_CMD_SET_SENSOR_INTERVAL,
                sensor_id, 0x00, 0x00);
        packet.data[1] = period_ms;

        status = create_frizz_workqueue((void*) &packet);

        packet.header.num = 1;

        if (batch.timeout_ns == 0) {
            packet.data[0] = HUB_MGR_GEN_CMD_CODE(HUB_MGR_CMD_DEACTIVATE_SENSOR,
                    sensor_id, 0x00, 0x00);
        } else {
            DEBUG_PRINT("enable batch\n");
            packet.data[0] = HUB_MGR_GEN_CMD_CODE(
                    HUB_MGR_CMD_SET_SENSOR_ACTIVATE, sensor_id, 0x01, 0x00);
        }

        status = create_frizz_workqueue((void*) &packet);
        break;

    case FRIZZ_IOCTL_SENSOR_FLUSH_FIFO:
		DEBUG_PRINT("FLUSH\n");
		break;
	case FRIZZ_IOCTL_GET_SENSOR_DATA_CHANGED:
		DEBUG_PRINT("GET data changed flag\n");
		get_sensor_data_changed(node, cmd, (void*) arg);
		break;
	case FRIZZ_IOCTL_GET_FIRMWARE_VERSION:
		DEBUG_PRINT("GET firmware_version");
		get_firmware_version(cmd, (void*) arg);
		break;
	case FRIZZ_IOCTL_SET_PEDO_COUNTER:
		copy_from_user_pedometer_counter_t(cmd, (void*)arg, &pedo_counter);

		if(pedo_counter.counter < 0) {
			kprint("Frizz pedo_counter EVALUE (%d)!!, ignore.",pedo_counter.counter);
			break;
		}
		set_pedo_interval(pedo_counter.counter);

		break;
	default:
		kprint("%s :NONE IOCTL SENSORxxx %x", __func__, cmd);
		return -1;
        break;
    }

    return status;
}

int set_pedo_interval(int interval) {
	packet_t packet;
	int status;
	hubhal_format_header_t head;
	head.w = 0;

	head.prefix = 0x01;
	packet.header.prefix = 0xFF;
	packet.header.type = 0x81;
	packet.header.sen_id = 0x88;
	packet.header.num = 0x02;
	packet.data[0] = head.w;
	packet.data[1] = interval;

	status = create_frizz_workqueue((void*) &packet);
	return status;
}

static int clean_pedo_data(void) {
	packet_t packet;
	int status;
	hubhal_format_header_t head;
	head.w = 0;

	head.prefix = 0x00;
	packet.header.prefix = 0xFF;
	packet.header.type = 0x81;
	packet.header.sen_id = 0x88;
	packet.header.num = 0x02;
	packet.data[0] = head.prefix;
	packet.data[1] = 0;

	status = create_frizz_workqueue((void*) &packet);
	return status;
}

int set_gesture_state(unsigned int gesture) {
	packet_t packet;
	hubhal_format_header_t head;
	int status;
	head.w = 0;

	head.prefix = 0x01;
	packet.header.prefix = 0xFF;
	packet.header.type = 0x81;
	packet.header.sen_id = 0xA6;
	packet.header.num = 0x02;
	packet.data[0] = head.w;
	packet.data[1] = gesture;

	status = create_frizz_workqueue((void*) &packet);
	return status;
}

int init_g_chip_orientation(unsigned int position) {
	packet_t acc_packet;
	packet_t gyro_packet;
	packet_t magn_packet;
	int i;
	int status;

	acc_packet.header.w = acc_sensor_orientation[position][0];
	acc_packet.data[0] = acc_sensor_orientation[position][1];

	gyro_packet.header.w = gyro_sensor_orientation[position][0];
	gyro_packet.data[0] = gyro_sensor_orientation[position][1];

	magn_packet.header.w = magn_sensor_orientation[position][0];
	magn_packet.data[0] = magn_sensor_orientation[position][1];

	for(i = 0; i < 6; i++) {
		acc_packet.data[1 + i] = acc_sensor_orientation[position][2 + i];
		gyro_packet.data[1 + i] = gyro_sensor_orientation[position][2 + i];
		magn_packet.data[1 + i] = magn_sensor_orientation[position][2 + i];
	}



	status = create_frizz_workqueue((void*) &acc_packet);
	if(status)
		goto return_err;
	status = create_frizz_workqueue((void*) &gyro_packet);
	if(status)
		goto return_err;
	status = create_frizz_workqueue((void*) &magn_packet);
return_err:
	return status;
}

int right_hand_wear(unsigned int isrighthand) {
	packet_t packet;
	hubhal_format_header_t head;
	int status;
	head.w = 0;

	head.prefix = 0x02;
	packet.header.prefix = 0xFF;
	packet.header.type = 0x81;
	packet.header.sen_id = 0x88;
	packet.header.num = 0x02;
	packet.data[0] = head.w;
	packet.data[1] = isrighthand;

	status = create_frizz_workqueue((void*) &packet);
	return status;
}

//the android system should shutdown gsensor when suspend
//and renable and reset interval when resume
//so here do not store the interval which been used for android
int set_gsensor_interval_for_sleep_motion(unsigned int ms) {
	int status;
	int sensor_id;
	packet_t packet;

	packet.header.prefix = 0xFF;
    packet.header.type = 0x81;
    packet.header.sen_id = HUB_MGR_ID;

    packet.header.num = 1;

	sensor_id =  convert_code_to_id(SENSOR_TYPE_ACCELEROMETER);
//1.enable it
    packet.data[0] = set_sensor_active(sensor_id);
    status = create_frizz_workqueue((void*) &packet);
	if(status) {
		return status;
	}
//set interval
    packet.header.num = 2;

    packet.data[0] = HUB_MGR_GEN_CMD_CODE(HUB_MGR_CMD_SET_SENSOR_INTERVAL,
                sensor_id, 0x00, 0x00);
    packet.data[1] = ms;

    status = create_frizz_workqueue((void*) &packet);
	return status;
}

int set_fall_parameter(void) {
	int status;
	packet_t packet;

	packet.header.prefix = 0xFF;
	packet.header.type = 0x81;
	packet.header.sen_id = 0xA4;
	packet.header.num = 7;

	packet.data[0] = 0x01000000;
	packet.data[1] = 0x43A50000;
	packet.data[2] = 0x41980000;
	packet.data[3] = 0x41700000;
	packet.data[4] = 0x3DCCCCCD;
	packet.data[5] = 0x64;
	packet.data[6] = 0xC8;

	status = create_frizz_workqueue((void*) &packet);
	return status;
}
