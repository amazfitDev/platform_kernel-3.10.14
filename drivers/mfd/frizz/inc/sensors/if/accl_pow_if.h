#ifndef __ACCL_POW_IF_H__
#define __ACCL_POW_IF_H__

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
 * @brief Power of Resultant Acceleration
 */
#define ACCEL_POWER_ID	SENSOR_ID_ACCEL_POWER
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief çáê¨â¡ë¨ìxÉpÉèÅ[
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data;	/// [G^2]
} accel_power_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
