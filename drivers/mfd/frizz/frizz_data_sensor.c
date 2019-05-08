#include <linux/module.h>
#include <linux/fs.h>
#include <linux/math64.h>
#include <asm/uaccess.h>
#include <linux/bitops.h>
#include <linux/list.h>

#include "inc/sensors/libsensors_id.h"
#include "inc/hub_mgr/hub_mgr_if.h"
#include "frizz_debug.h"
#include "frizz_ioctl.h"
#include "frizz_packet.h"
#include "frizz_callback.h"
#include "frizz_sensor_list.h"
#include "frizz_hal_if.h"
#include "sensors.h"
#include "frizz_file_id_list.h"
#include "frizz_workqueue.h"
#include "frizz_motion_code_remap.h"

extern const unsigned int config_wakeup_by_raise;

extern unsigned long pedo_data;

extern unsigned long Firmware_version;
extern struct list_head head;

static rwlock_t sensor_data_rwlock;

static firmware_version_t fv;

static COPY_FROM_USER(sensor_enable_t)
static COPY_FROM_USER(sensor_delay_t)
static COPY_FROM_USER(sensor_data_t)
static COPY_FROM_USER(batch_t)

static COPY_TO_USER(sensor_enable_t)
static COPY_TO_USER(sensor_delay_t)
static COPY_TO_USER(sensor_data_t)
static COPY_TO_USER(sensor_data_changed_t)
static COPY_TO_USER(firmware_version_t)
static COPY_TO_USER(batch_t)

#define INIT_SENSOR_STRUCT(code, num) code, num

static struct {
    libsensors_type_e code;
    int num;
    sensor_data_t   data;
    sensor_delay_t  delay;
    sensor_enable_t enable;
    batch_t batch;
    int batch_flag;
    unsigned int prev_us;

} struct_list[] = {
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_FIFO_EMPTY,                  1)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_ACCELEROMETER,               3)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_MAGNETIC_FIELD,              3)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_GYROSCOPE,                   3)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_ORIENTATION,                 3)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_PRESSURE,                    1)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_GRAVITY,                     3)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_LINEAR_ACCELERATION,         3)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_ROTATION_VECTOR,             4)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED, 6)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_GAME_ROTATION_VECTOR,        4)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_GYROSCOPE_UNCALIBRATED,      6)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_SIGNIFICANT_MOTION,          1)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_STEP_DETECTOR,               1)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_STEP_COUNTER,                1)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR, 5)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_PDR,                         6)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_MAGNET,                      3)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_GYRO,                        3)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_ACCEL_POWER,                 1)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_ACCEL_LPF,                   3)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_ACCEL_LINEAR,                3)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_MAGNET_PARAMETER,           12)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_MAGNET_CALIB_SOFT,           3)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_MAGNET_LPF,                  3)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_GYRO_LPF,                    4)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_DIRECTION,                   3)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_POSTURE,                     6)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_VELOCITY,                    2)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_RELATIVE_POSITION,           2)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_MIGRATION_LENGTH,            1)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_CYCLIC_TIMER,                1)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_ISP,                         1)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_ACCEL_FALL_DOWN,             1)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_ACCEL_POS_DET,               1)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_PDR_GEOFENCING,              3)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_GESTURE,                     1)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_LIGHT,                       1)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_PROXIMITY,                   1)},
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_ACTIVITY,                    2)},
    {INIT_SENSOR_STRUCT(HUB_MANAGER,                             2)},
};

#define MAX_SENSOR_COUNT (sizeof struct_list / sizeof struct_list[0])

void input_sensor_struct(void) {
    int i;

    for(i = 0; i < MAX_SENSOR_COUNT; i++) {
	struct_list[i].data.code   = struct_list[i].code;
	struct_list[i].delay.code  = struct_list[i].code;
	struct_list[i].enable.code = struct_list[i].code;

	struct_list[i].batch.code = struct_list[i].code;

	struct_list[i].delay.ms = 1000;

	struct_list[i].batch_flag = 0;

	struct_list[i].enable.flag = 0;
    }
}

int get_struct_index(int sensor_id) {
    int i;

    for(i = 0; i < MAX_SENSOR_COUNT; i++) {
	if(sensor_id == convert_code_to_id(struct_list[i].code)) {
	    return i;
	}
    }

    return -1;

}

inline static void convert_float(unsigned int id, float *val) {
    float *fd = (float*) &id;
    *val = *fd;
}

void init_sensor_data_rwlock(void) {
    rwlock_init(&sensor_data_rwlock);

    input_sensor_struct();
}

static void remap_motion_code(int sensor_type, int* origin)
{
	int i = 0;
	for(;i < MAX_MOTION_CODE_COUNT; i++) {
		if(sensor_type == motion_code_remap[i].sensor_typ
				&& (*origin) == motion_code_remap[i].code_origin) {
			*origin = motion_code_remap[i].code_remap;
			break;
		}
	}
}

int update_data_sensor(packet_t *packet)
{

    int index;
    int command_id;
    int i;

	struct list_head *plist;
	struct file_id_node *node = NULL;

    read_lock(&sensor_data_rwlock);

    if (packet->header.sen_id == HUB_MGR_ID) {
        index = get_struct_index(((packet->data[0] >> 16) & 0xFF));
        command_id = packet->data[0] >> 24;
    } else {
        index = get_struct_index(packet->header.sen_id);
        command_id = 0xff80;
    }

    if (index < 0) {
        read_unlock(&sensor_data_rwlock);
        return -1;
    }

    switch (command_id) {
    case HUB_MGR_CMD_DEACTIVATE_SENSOR:
        struct_list[index].enable.flag = 0;
        break;

    case HUB_MGR_CMD_SET_SENSOR_ACTIVATE:
        //batch flag set 1 when sw fifo is on.
        struct_list[index].batch_flag = (packet->data[0] >> 8) & 0xFF;
        struct_list[index].enable.flag = 1;
        break;

    case HUB_MGR_CMD_SET_SENSOR_INTERVAL:
        struct_list[index].delay.ms = packet->data[1];
        break;

    case 0xff80:
        //add the changed flag when the data is been changed.
        i = convert_id_to_code(packet->header.sen_id);

		list_for_each(plist,&head)
		{
			node = list_entry(plist, struct file_id_node, list);
			if (i > 31)
				node->sdc.mflag |= BIT(i - 31);
			else
				node->sdc.aflag |= BIT(i);

		}

		if (struct_list[index].data.code == SENSOR_TYPE_ACTIVITY
				&& (packet->data[2] == ACTIVITY_DEEP_SLEEP
						|| packet->data[2] == ACTIVITY_SLEEP)) {
			packet->data[0] = packet->data[1];
		}
		struct_list[index].data.time.tv_sec = packet->data[0] / 1000;
		struct_list[index].data.time.tv_usec = (packet->data[0] % 1000) * 1000;

//重定义 跌倒，睡眠检测的编码，因为android只监听motion，
		if (struct_list[index].data.code == SENSOR_TYPE_ACTIVITY) {
			remap_motion_code(SENSOR_TYPE_ACTIVITY, &(packet->data[2]));

			struct_list[index].data.u32_value[0] = packet->data[2]; //activity
			struct_list[index].data.u32_value[1] = packet->data[4]; //tossAndTurn_cnt
		}
//结束
		else if(struct_list[index].data.code == SENSOR_TYPE_PDR) {
            struct_list[index].data.count = packet->data[1];
	        convert_float(packet->data[2], &struct_list[index].data.rpos[0]);
	        convert_float(packet->data[3], &struct_list[index].data.rpos[1]);
	        convert_float(packet->data[4], &struct_list[index].data.velo[0]);
	        convert_float(packet->data[5], &struct_list[index].data.velo[1]);
	        convert_float(packet->data[6], &struct_list[index].data.total_dst);
        } else if (struct_list[index].data.code == SENSOR_TYPE_STEP_COUNTER) {
			//store the pedo data in kernel for SLPT to show
			pedo_data = packet->data[1];
            struct_list[index].data.u32_value[0] = packet->data[1];
        } else if (struct_list[index].data.code == SENSOR_TYPE_STEP_DETECTOR) {
            struct_list[index].data.f32_value[0] = 1;
        } else if (struct_list[index].data.code == SENSOR_TYPE_GESTURE) {
			if(config_wakeup_by_raise && (packet->data[1] == 4)) {
				send_wakeup_cmd();
			}
			printk("Frizz debug: gesture report: %d \n", packet->data[1]);
			struct_list[index].data.u32_value[0] = packet->data[1];
		} else if (struct_list[index].data.code == SENSOR_TYPE_ISP) {
			remap_motion_code(SENSOR_TYPE_ISP, &(packet->data[1]));
			struct_list[index].data.u32_value[0] = packet->data[1];
		} else {
            for (i = 0; i < packet->header.num - 1; i++) {
                convert_float(packet->data[i + 1],
                        &struct_list[index].data.f32_value[i]);
            }
        }


        break;

    default:
        break;
    }

    read_unlock(&sensor_data_rwlock);

    return 0;
}

int get_sensor_enable(unsigned int ioctl_code, void *arg) {

    sensor_enable_t sensor_enable;
    int index;

    copy_from_user_sensor_enable_t(ioctl_code, (void*)arg, &sensor_enable);

    index = get_struct_index(convert_code_to_id(sensor_enable.code));

    if(index < 0) {
	return -1;
    }

    copy_to_user_sensor_enable_t(ioctl_code, arg, &struct_list[index].enable, sensor_data_rwlock);

    return 0;
}

int get_sensor_data_changed(struct file_id_node* node, unsigned int ioctl_code, void *arg)
{
	sensor_data_changed_t sensor_changed;

	sensor_changed.aflag = node->sdc.aflag;
	sensor_changed.mflag = node->sdc.mflag;

    DEBUG_PRINT("the changed bit been send is here: aflag = [%x], mflag = [%x] \n",
            sensor_changed.aflag, sensor_changed.mflag);
    copy_to_user_sensor_data_changed_t(ioctl_code, arg,
            &sensor_changed, sensor_data_rwlock);

    return 0;
}

int get_firmware_version(unsigned int ioctl_code, void *arg)
{
    DEBUG_PRINT("Firmware Version is : %ld \n", Firmware_version);
    fv.version = Firmware_version;
    copy_to_user_firmware_version_t(ioctl_code, arg, &fv, sensor_data_rwlock);
    return 0;
}

int get_sensor_delay(unsigned int ioctl_code, void *arg) {

    sensor_delay_t sensor_delay;
    int index;

    copy_from_user_sensor_delay_t(ioctl_code, (void*)arg, &sensor_delay);

    index = get_struct_index(convert_code_to_id(sensor_delay.code));

    if(index < 0) {
	return -1;
    }

    copy_to_user_sensor_delay_t(ioctl_code, arg, &struct_list[index].delay, sensor_data_rwlock);

    return 0;
}

int get_sensor_data(struct file_id_node* node, unsigned int ioctl_code, void *arg) {

    sensor_data_t sensor_data;
    int index;

    copy_from_user_sensor_data_t(ioctl_code, (void*) arg, &sensor_data);

    index = get_struct_index(convert_code_to_id(sensor_data.code));

    if (index < 0) {
        return -1;
    }

    //clear the data update bit, bc the data will be read by HAL
	if (sensor_data.code > 31)
		node->sdc.mflag &= ~(BIT(sensor_data.code - 31));
	else
		node->sdc.aflag &= ~(BIT(sensor_data.code));

    copy_to_user_sensor_data_t(ioctl_code, arg, &struct_list[index].data, sensor_data_rwlock);

    return 0;
}

int simulate_timeout(unsigned int ioctl_code, void *arg)
{
    batch_t batch;
    int index;
    int data_num, i;
    uint64_t period_ns, timeout_ns;

    copy_from_user_batch_t(ioctl_code, (void*) arg, &batch);

    index = get_struct_index(convert_code_to_id(batch.code));

    if (index < 0) {
        return -1;
    }

    if (batch.timeout_ns < 1000000) {
        timeout_ns = 1000000;
    } else {
        timeout_ns = batch.timeout_ns;
    }

    if (batch.period_ns < 1000000) {
        period_ns = 1000000;
    } else {
        period_ns = batch.period_ns;
    }

    struct_list[index].batch_flag = 1;
    data_num = 0;

    for (i = 0; i < MAX_SENSOR_COUNT; i++) {

        data_num = data_num
                + (struct_list[i].batch_flag * (struct_list[i].num + 2));
    }

    struct_list[index].batch_flag = 0;

    batch.simulate_timeout_ns = div64_u64((128 * 1024 * period_ns),
            (data_num * 4));

    // DEBUG_PRINT("simulate timeout ns %d %d %d \n", period_ns, data_num, sensor->batch.simulate_timeout_ns);

    copy_to_user_batch_t(ioctl_code, arg, &batch, sensor_data_rwlock);

    return 0;
}

