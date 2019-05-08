#ifndef __ORIENTATION_IF_H__
#define __ORIENTATION_IF_H__

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
 * @brief Orientation
 */
#define ORIENTATION_ID	SENSOR_ID_ORIENTATION
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief •ûŒü
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data[3];	/// gravity vector [degrees] 0:azimuth, 1:pitch, 2:roll
} orientation_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
