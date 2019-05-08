#ifndef __GYRO_RAW_IF_H__
#define __GYRO_RAW_IF_H__

#include "libsensors_id.h"

#ifdef RUN_ON_FRIZZ
	#define FLOAT	frizz_tie_fp
#else
	#define FLOAT	float
#endif

/**
 * @name Sensor ID
 */
//@{
/**
 * @brief Angular rate from Physical Sensor
 */
#define GYRO_RAW_ID	SENSOR_ID_GYRO_RAW
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief Šp‘¬“x
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data[3];	/// [rad/s] 0:x, 1:y, 2:z
} gyro_raw_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
