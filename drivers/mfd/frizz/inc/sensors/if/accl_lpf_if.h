#ifndef __ACCL_LPF_IF_H__
#define __ACCL_LPF_IF_H__

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
 * @brief Accel via LPF
 */
#define ACCEL_LPF_ID	SENSOR_ID_ACCEL_LPF
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief LPF“K‰ž‰Á‘¬“x
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data[3];	/// [G] 0:x, 1:y, 2:z
} accel_lpf_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
