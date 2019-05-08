#ifndef __FRIZZ_DATA_SENSOR_H__
#define __FRIZZ_DATA_SENSOR_H__
#include "frizz_packet.h"
#include "frizz_file_id_list.h"
/*!
 * initialize rw_lock
 */
void init_sensor_data_rwlock(void);

/*!
 * update sensor data from packet
 *
 * @param[in] packet from frizzFW
 * @return 0=sucess, otherwise 0=fail
 */
int update_data_sensor(packet_t*);

/*!
 * get android sensor enable
 *
 * @param[in] ioctl code
 * @param[out] struct of sensor enable
 * @return 0=sucess, otherwise 0=fail
 */
int get_sensor_enable(unsigned int, void *);

int get_sensor_data_changed(struct file_id_node*, unsigned int, void *);

int get_firmware_version(unsigned int, void* );
/*!
 * get android sensor delay
 *
 * @param[in] ioctl code
 * @param[out] struct of sensor delay
 * @return 0=sucess, otherwise 0=fail
 */
int get_sensor_delay(unsigned int, void *);

/*!
 * get android sensor data
 *
 * @param[in] ioctl code
 * @param[out] struct of sensor data
 * @return 0=sucess, otherwise 0=fail
 */
int get_sensor_data(struct file_id_node*, unsigned int, void *);

/*!
 * get simulate timeout
 *
 * @param[in] ioctl code
 * @param[out] struct of sensor data
 * @return 0=sucess, otherwise 0=fail
 */
int simulate_timeout(unsigned int, void *);
#endif
