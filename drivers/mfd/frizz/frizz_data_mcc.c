#include <linux/module.h>
#include <linux/fs.h>
#include <linux/math64.h>
#include <asm/uaccess.h>
//#include "inc/sensors/libsensors_id.h"
#include "inc/hub_mgr/hub_mgr_if.h"
#include "frizz_debug.h"
#include "frizz_ioctl.h"
#include "frizz_packet.h"
#include "frizz_callback.h"
#include "frizz_sensor_list.h"
#include "frizz_hal_if.h"
#include "sensors.h"

static rwlock_t sensor_data_rwlock;

static COPY_FROM_USER(sensor_enable_t)
static COPY_FROM_USER(sensor_delay_t)
static COPY_FROM_USER(sensor_data_t)

static COPY_TO_USER(sensor_enable_t)
static COPY_TO_USER(sensor_delay_t)
static COPY_TO_USER(sensor_data_t)

#define INIT_SENSOR_STRUCT(code, num) code, num

static struct {
    libsensors_type_e code;
    int num;
    sensor_data_t   data;
    sensor_delay_t  delay;
    sensor_enable_t enable;
    batch_t batch;
    int batch_flag;

} struct_list[] = {
    {INIT_SENSOR_STRUCT(SENSOR_TYPE_PDR, 8)},
};

#define MAX_SENSOR_COUNT (sizeof struct_list / sizeof struct_list[0])

static void input_sensor_struct(void) {
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

static int get_struct_index(int sensor_id) {
    int i;

    for(i = 0; i < MAX_SENSOR_COUNT; i++) {
	if(sensor_id == convert_code_to_id(struct_list[i].code)) {
	    return i;
	}
    }

    return -1;

}

static void convert_float(unsigned int id, float *val) {
  float *fd = (float*)&id;

  *val = *fd;
}

void init_mcc_data_rwlock(void) {
    rwlock_init(&sensor_data_rwlock);

    input_sensor_struct();
}

int update_data_mcc(packet_t *packet) {

    int index;
    int command_id;
    //int i;

    read_lock(&sensor_data_rwlock);

    if(packet->header.sen_id == HUB_MGR_ID) {

	index = get_struct_index(((packet->data[0] >> 16) & 0xFF));
	command_id = packet->data[0]  >> 24;

    } else {
	index = get_struct_index(packet->header.sen_id);
	command_id = 0xff80;

    }

    if(index < 0) {
	read_unlock(&sensor_data_rwlock);
	return -1;
    }

    switch(command_id) {

    case HUB_MGR_CMD_DEACTIVATE_SENSOR :
	struct_list[index].enable.flag = 0;
	break;

    case HUB_MGR_CMD_SET_SENSOR_ACTIVATE :

	struct_list[index].batch_flag  = (packet->data[0]  >> 8) & 0xFF;
	struct_list[index].enable.flag = 1;

	break;

    case HUB_MGR_CMD_SET_SENSOR_INTERVAL :
	struct_list[index].delay.ms = packet->data[1];
	break;

    case 0xff80 :

	struct_list[index].data.time.tv_sec  = packet->data[0] / 1000;
	struct_list[index].data.time.tv_usec = packet->data[0] * 1000;

	if(struct_list[index].data.code == SENSOR_TYPE_PDR) {

	    struct_list[index].data.count = packet->data[1];

	    convert_float(packet->data[2], &struct_list[index].data.rpos[0]);
	    convert_float(packet->data[3], &struct_list[index].data.rpos[1]);
	    convert_float(packet->data[4], &struct_list[index].data.velo[0]);
	    convert_float(packet->data[5], &struct_list[index].data.velo[1]);
	    convert_float(packet->data[6], &struct_list[index].data.total_dst);

	}

	break;

    default :
	break;
    }

    read_unlock(&sensor_data_rwlock);

    return 0;
}

int get_mcc_enable(unsigned int ioctl_code, void *arg) {

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

int get_mcc_delay(unsigned int ioctl_code, void *arg) {

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

int get_mcc_data(unsigned int ioctl_code, void *arg) {

    sensor_data_t sensor_data;
    int index;

    copy_from_user_sensor_data_t(ioctl_code, (void*)arg, &sensor_data);

    index = get_struct_index(convert_code_to_id(sensor_data.code));

    if(index < 0) {
	return -1;
    }

    copy_to_user_sensor_data_t(ioctl_code, arg, &struct_list[index].data, sensor_data_rwlock);

    return 0;
}
