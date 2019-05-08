#ifndef __ACCL_RAW_IF_H__
#define __ACCL_RAW_IF_H__

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
 * @brief Acceleration from Physical Sensor
 */
#define ACCEL_RAW_ID	SENSOR_ID_ACCEL_RAW
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief ‰Á‘¬“x
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data[3];	/// [G] 0:x, 1:y, 2:z
} accel_raw_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
