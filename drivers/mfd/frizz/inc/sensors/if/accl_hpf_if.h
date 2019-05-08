#ifndef __ACCL_HPF_IF_H__
#define __ACCL_HPF_IF_H__

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
 * @brief Accel via HPF
 */
#define ACCEL_HPF_ID	SENSOR_ID_ACCEL_HPF
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief HPF“K‰ž‰Á‘¬“x
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data[3];	/// [G] 0:x, 1:y, 2:z
} accel_hpf_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
