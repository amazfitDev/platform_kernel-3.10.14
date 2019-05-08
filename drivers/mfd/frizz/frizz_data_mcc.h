#include "frizz_packet.h"


/*!
 * initialize rw_lock
 */
void init_mcc_data_rwlock(void);

/*!
 * update sensor data from packet
 *
 * @param[in] packet from frizzFW
 * @return 0=sucess, otherwise 0=fail
 */
int update_data_mcc(packet_t*);
/*!
 * get android sensor enable
 *
 * @param[in] ioctl code
 * @param[out] struct of sensor enable
 * @return 0=sucess, otherwise 0=fail
 */
int get_mcc_enable(unsigned int, void *);

/*!
 * get android sensor delay
 *
 * @param[in] ioctl code
 * @param[out] struct of sensor delay
 * @return 0=sucess, otherwise 0=fail
 */
int get_mcc_delay(unsigned int, void *);

/*!
 * get android sensor data
 *
 * @param[in] ioctl code
 * @param[out] struct of sensor delay
 * @return 0=sucess, otherwise 0=fail
 */
int get_mcc_data(unsigned int, void *);
