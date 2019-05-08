#include <asm/uaccess.h>
#include <linux/math64.h>
#include "frizz_workqueue.h"
#include "frizz_data_mcc.h"
#include "frizz_debug.h"
#include "frizz_packet.h"
#include "frizz_ioctl.h"
#include "frizz_sensor_list.h"
#include "inc/sensors/libsensors_id.h"
#include "inc/hub_mgr/hub_mgr_if.h"
#include "frizz_timer.h"
#include "frizz_connection.h"
#include "frizz_hal_if.h"

static COPY_FROM_USER(sensor_enable_t)
static COPY_FROM_USER(sensor_delay_t)

static unsigned int set_sensor_active(int sensor_id) {

    if((sensor_id == SENSOR_ID_ACCEL_MOVE)) {
	return  HUB_MGR_GEN_CMD_CODE(HUB_MGR_CMD_SET_SENSOR_ACTIVATE, sensor_id, 0x00, 0x01);
    }else {
	return  HUB_MGR_GEN_CMD_CODE(HUB_MGR_CMD_SET_SENSOR_ACTIVATE, sensor_id, 0x00, 0x00);
    }

}

int frizz_ioctl_mcc(unsigned int cmd, unsigned long arg) {

    int sensor_id;
    int status;
    packet_t packet;

    sensor_enable_t sensor_enable = {0};
    sensor_delay_t  sensor_delay  = {0};

    packet.header.prefix = 0xFF;
    packet.header.type   = 0x81;
    packet.header.sen_id = HUB_MGR_ID;

    switch(cmd) {

    case FRIZZ_IOCTL_MCC_SET_ENABLE :

	status = copy_from_user_sensor_enable_t(cmd, (void*)arg, &sensor_enable);
	sensor_id = convert_code_to_id(sensor_enable.code);

	packet.header.num    = 1;

	switch (sensor_enable.flag) {
	case 0:
	    packet.data[0] = HUB_MGR_GEN_CMD_CODE(HUB_MGR_CMD_DEACTIVATE_SENSOR, sensor_id, 0x00, 0x00);
	    break;
	case 1:
	    packet.data[0] = set_sensor_active(sensor_id);
	    break;
	}
	DEBUG_PRINT("MCC_SET_ENABLE %x %d %d\n", packet.data[0], sensor_id, sensor_enable.code);
	status = create_frizz_workqueue((void*)&packet);

	break;

    case FRIZZ_IOCTL_MCC_SET_DELAY :

	copy_from_user_sensor_delay_t(cmd, (void*)arg, &sensor_delay);
	sensor_id = convert_code_to_id(sensor_delay.code);

	packet.header.num    = 2;

	packet.data[0] = HUB_MGR_GEN_CMD_CODE(HUB_MGR_CMD_SET_SENSOR_INTERVAL, sensor_id, 0x00, 0x00);
	packet.data[1] = sensor_delay.ms;

	DEBUG_PRINT("MCC_SET_DELAY %d %x\n", sensor_id, packet.data[0]);

	status = create_frizz_workqueue((void*)&packet);

	break;

    case  FRIZZ_IOCTL_MCC_GET_ENABLE :
	get_mcc_enable(cmd, (void*)arg);
	break;

    case FRIZZ_IOCTL_MCC_GET_DELAY :
	get_mcc_delay(cmd, (void*)arg);
	break;

    case FRIZZ_IOCTL_MCC_GET_DATA :
	//DEBUG_PRINT("MCC_GET_DATA\n");
	get_mcc_data(cmd, (void*)arg);
	break;

    default :
	DEBUG_PRINT("NONE IOCTL MCC\n");
	return -1;
	break;
    }

    return 0;
}
