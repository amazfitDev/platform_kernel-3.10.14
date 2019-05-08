#ifndef __GYRO_UNCALIB_IF_H__
#define __GYRO_UNCALIB_IF_H__

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
 * @brief uncalibrated Angular rate
 */
#define GYRO_UNCALIB_ID	SENSOR_ID_GYRO_UNCALIB
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief 未較正ジャイロデータ + ジャイロオフセット
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data[6];	/// [rad/s] uncalibrated(0:x, 1:y, 2:z), offset(3:x, 4:y, 5:z)
} gyro_uncalibrated_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
