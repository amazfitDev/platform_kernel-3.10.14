#ifndef __GRAVITY_IF_H__
#define __GRAVITY_IF_H__

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
 * @brief Gravity
 */
#define GRAVITY_ID	SENSOR_ID_GRAVITY
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief èdóÕï˚å¸
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data[3];	/// gravity vector [G] 0:x, 1:y, 2:z
} gravity_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
