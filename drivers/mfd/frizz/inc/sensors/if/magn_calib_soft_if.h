#ifndef __MAGN_CALIB_SOFT_IF_H__
#define __MAGN_CALIB_SOFT_IF_H__

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
 * @brief Soft-Iron�r��
 */
#define MAGNET_CALIB_SOFT_ID	SENSOR_ID_MAGNET_CALIB_SOFT
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief Soft-Iron�r���㎥��
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data[3];	/// [��T] 0:x, 1:y, 2:z
} magnet_calib_soft_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
