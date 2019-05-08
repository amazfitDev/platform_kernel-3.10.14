#ifndef __GYRO_HPF_IF_H__
#define __GYRO_HPF_IF_H__

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
 * @brief Angular rate via HPF
 */
#define GYRO_HPF_ID	SENSOR_ID_GYRO_HPF
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief HPF“K‰žŠp‘¬“x
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data[3];	/// [rad/s] 0:x, 1:y, 2:z
} gyro_hpf_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
