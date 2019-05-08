#include <linux/kernel.h>
#include <linux/fs.h>
#include "frizz_connection.h"
#include "frizz_debug.h"
#include "frizz_data_sensor.h"
#include "frizz_data_mcc.h"
#include "frizz_callback.h"
//#include "frizz_sensor_list.h"
#include "sensors.h"
#include "inc/sensors/libsensors_id.h"
#include "inc/hub_mgr/hub_mgr_if.h"

INTERRUPT_TIMER_P  interrupt_timer_p;
INTERRUPT_POLL_P   interrupt_poll_p;

unsigned int timestamp = 0;

void init_callback(void) {
    interrupt_timer_p = interrupt_timer;
    interrupt_poll_p  = interrupt_poll;
}

void func_interrupt_timer(unsigned int header, unsigned int sensor_data[], INTERRUPT_TIMER_P p) {
    p(header, sensor_data);
}

void func_interrupt_poll(INTERRUPT_POLL_P p) {
    p();
}

//change update list if update_list is added sensor type
#define UPDATE_ID_SENSOR(id) \
	{id, update_data_sensor}

#define UPDATE_ID_MCC(id) \
    {id, update_data_mcc}

struct {
  int id;
  int (*update_data)(packet_t *packet);
} update_list[] = {

    UPDATE_ID_SENSOR(SENSOR_TYPE_FIFO_EMPTY),

    UPDATE_ID_SENSOR(SENSOR_ID_ACCEL_RAW),
    UPDATE_ID_SENSOR(SENSOR_ID_GYRO_HPF),
    UPDATE_ID_SENSOR(SENSOR_ID_MAGNET_CALIB_HARD),
    UPDATE_ID_SENSOR(SENSOR_ID_ORIENTATION),
    UPDATE_ID_SENSOR(SENSOR_ID_PRESSURE_RAW),
    UPDATE_ID_SENSOR(SENSOR_ID_GRAVITY),
    UPDATE_ID_SENSOR(SENSOR_ID_ACCEL_HPF),
    UPDATE_ID_SENSOR(SENSOR_ID_ROTATION_VECTOR),
    UPDATE_ID_SENSOR(SENSOR_ID_MAGNET_UNCALIB),
    UPDATE_ID_SENSOR(SENSOR_ID_ROTATION_GRAVITY_VECTOR),
    UPDATE_ID_SENSOR(SENSOR_ID_GYRO_UNCALIB),
    UPDATE_ID_SENSOR(SENSOR_ID_ACCEL_MOVE),
    UPDATE_ID_SENSOR(SENSOR_ID_ACCEL_STEP_DETECTOR),
    UPDATE_ID_SENSOR(SENSOR_ID_ACCEL_PEDOMETER),
    UPDATE_ID_SENSOR(SENSOR_ID_ROTATION_LPF_VECTOR),
    UPDATE_ID_SENSOR(SENSOR_ID_MAGNET_RAW),
    UPDATE_ID_SENSOR(SENSOR_ID_GYRO_RAW),
    UPDATE_ID_SENSOR(SENSOR_ID_ACCEL_POWER),
    UPDATE_ID_SENSOR(SENSOR_ID_ACCEL_LPF),
    UPDATE_ID_SENSOR(SENSOR_ID_ACCEL_LINEAR),
    UPDATE_ID_SENSOR(SENSOR_ID_MAGNET_PARAMETER),
    UPDATE_ID_SENSOR(SENSOR_ID_MAGNET_CALIB_SOFT),
    UPDATE_ID_SENSOR(SENSOR_ID_MAGNET_LPF),
    UPDATE_ID_SENSOR(SENSOR_ID_GYRO_LPF),
    UPDATE_ID_SENSOR(SENSOR_ID_DIRECTION),
    UPDATE_ID_SENSOR(SENSOR_ID_POSTURE),
    UPDATE_ID_SENSOR(SENSOR_ID_VELOCITY),
    UPDATE_ID_SENSOR(SENSOR_ID_RELATIVE_POSITION),
    UPDATE_ID_SENSOR(SENSOR_ID_MIGRATION_LENGTH),
    UPDATE_ID_SENSOR(SENSOR_ID_CYCLIC_TIMER),
    UPDATE_ID_SENSOR(SENSOR_ID_ISP),
    UPDATE_ID_SENSOR(SENSOR_ID_ACCEL_FALL_DOWN),
    UPDATE_ID_SENSOR(SENSOR_ID_ACCEL_POS_DET),
    UPDATE_ID_SENSOR(SENSOR_ID_PDR_GEOFENCING),
    UPDATE_ID_SENSOR(SENSOR_ID_GESTURE),
	UPDATE_ID_SENSOR(SENSOR_ID_ACTIVITY),
    UPDATE_ID_SENSOR(HUB_MGR_ID),

    UPDATE_ID_SENSOR(SENSOR_ID_PDR),
    UPDATE_ID_SENSOR(SENSOR_ID_PROXIMITY),
    UPDATE_ID_SENSOR(SENSOR_ID_LIGHT_RAW),
};

#define MAX_UPDATE_COUNT (sizeof(update_list) / sizeof(update_list[0]))

int analysis_packet(packet_t* packet)
{
    int i;

    for (i = 0; i < MAX_UPDATE_COUNT; i++) {
        if (packet->header.sen_id == update_list[i].id) {
            update_list[i].update_data(packet);
        }
    }

    return 0;
}

int analysis_fifo(unsigned int *fifo_data, int *index) {

    packet_t packet;
    int i, num;

    if (fifo_data[*index] == OUTPUT_INTERVAL_TIME) {

        packet.header.w = fifo_data[*index];
        packet.data[0] = fifo_data[*index + 1]; //timestamp
        packet.data[1] = fifo_data[*index + 2]; //update interval
        packet.data[2] = fifo_data[*index + 3];

        //DEBUG_PRINT("OUTPUT_INTERVAL_TIME : %d %d \n", packet.data[0], packet.data[1]);

        *index = *index + 2;

        //execute callback function of timer
        func_interrupt_timer(packet.header.w, packet.data, interrupt_timer_p);
        return 0;

    } else {

        //DEBUG_PRINT("OUTPUT_DATA %x %x\n", fifo_data[*index], fifo_data[*index + 1]);
        packet.header.w = fifo_data[*index];

        //id  = (fifo_data[*index] >> 8) & 0xFF;
        num = fifo_data[*index] & 0xFF;

        for (i = 0; i < num; i++) {
            packet.data[i] = fifo_data[i + *index + 1];
        }

        timestamp = packet.data[0];

        analysis_packet(&packet);

        func_interrupt_poll(interrupt_poll);

        return 0;
    }

    return 0;

}

int create_sensor_type_fifo_fake(unsigned char id, unsigned long *data, int data_len) {
    packet_t packet;
	int i;

	if((data == NULL) || (data_len <= 0) || (data_len > (MAX_DATA_NUM - 1)))
		return -1;
    packet.header.prefix = 0xFF;
    packet.header.type   = 0x80;
    packet.header.sen_id = id;
    packet.header.num    = data_len + 1;

    packet.data[0] = timestamp;
	for(i = 0; i < data_len; i++) {
		packet.data[i + 1] = *(data + i);
	}

    analysis_packet(&packet);
    func_interrupt_poll(interrupt_poll);

    return 0;
}
