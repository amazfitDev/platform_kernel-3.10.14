#ifndef __MAGN_LPF_IF_H__
#define __MAGN_LPF_IF_H__

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
 * @brief Magnetic force via LPF
 */
#define MAGNET_LPF_ID	SENSOR_ID_MAGNET_LPF
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief LPF“K‰žŽ¥—Í
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data[3];	/// [ƒÊT] 0:x, 1:y, 2:z
} magnet_lpf_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}
#endif
