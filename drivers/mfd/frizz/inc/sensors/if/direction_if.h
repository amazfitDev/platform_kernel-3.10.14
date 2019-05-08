#ifndef __DIRECTION_IF_H__
#define __DIRECTION_IF_H__

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
 * @brief Direction
 */
#define DIRECTION_ID	SENSOR_ID_DIRECTION
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief Ž¥–k•ûŒü
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data[3];	/// magnetic north vector [ƒÊTs] 0:x, 1:y, 2:z
} direction_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
