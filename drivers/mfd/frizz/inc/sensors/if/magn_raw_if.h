#ifndef __MAGN_RAW_IF_H__
#define __MAGN_RAW_IF_H__

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
 * @brief Magnetic force from Physical Sensor
 */
#define MAGNET_RAW_ID	SENSOR_ID_MAGNET_RAW
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief ����
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data[3];	/// [��T] 0:x, 1:y, 2:z
} magnet_raw_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
