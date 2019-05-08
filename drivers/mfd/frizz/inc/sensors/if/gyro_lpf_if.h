#ifndef __GYRO_LPF_IF_H__
#define __GYRO_LPF_IF_H__

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
 * @brief Offset of Gyroscope
 */
#define GYRO_LPF_ID	SENSOR_ID_GYRO_LPF
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief Offset of Gyroscope
 *
 * @note One-shot
 */
typedef struct {
	FLOAT		data[3];	/// [rad/s] 0:x, 1:y, 2:z
} gyro_lpf_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
