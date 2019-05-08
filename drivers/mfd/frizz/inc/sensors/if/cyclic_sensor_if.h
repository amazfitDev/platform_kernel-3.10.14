#ifndef __CYCLIC_SENSOR_IF_H__
#define __CYCLIC_SENSOR_IF_H__

#include "libsensors_id.h"

/**
 * @name Sensor ID
 */
//@{
/**
 * @brief 定周期通知センサ
 */
#define CYCLIC_TIMER_ID	SENSOR_ID_CYCLIC_TIMER
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief カウント値
 *
 * @note Continuous
 */
typedef struct {
	unsigned int		count;	/// count value
} accel_hpf_data_t;
//@}

/**
 * @name Command List
 */
//@{
/**
 * @brief Clear count
 *
 * @param cmd_code CYCLIC_TIMER_CMD_CLEAR_COUNT
 *
 * @return 0: OK
 */
#define CYCLIC_TIMER_CMD_CLEAR_COUNT		0x00
//@}

#endif
