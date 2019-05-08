#ifndef __ACCL_PEDO_IF_H__
#define __ACCL_PEDO_IF_H__

#include "libsensors_id.h"

/**
 * @name Sensor ID
 */
//@{
/**
 * @brief Pedometer using accelerometer
 */
#define ACCEL_PEDOMETER_ID SENSOR_ID_ACCEL_PEDOMETER
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief ステップ検出タイミング
 *
 * @note On-changed
 */
typedef struct {
	unsigned int data;		/// step count
} accel_pedometer_data_t;
//@}

/**
 * @name Command List
 */
//@{
/**
 * @brief Clear counter
 *
 * @param cmd_code ACCEL_PEDOMETER_CMD_CLEAR_COUNT
 *
 * @return 0: OK
 */
#define ACCEL_PEDOMETER_CMD_CLEAR_COUNT		0x00
//@}

#endif
