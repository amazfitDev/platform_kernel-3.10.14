#ifndef __MAGN_UNCALIB_IF_H__
#define __MAGN_UNCALIB_IF_H__

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
 * @brief uncalibrated Magnetic force
 */
#define MAGNET_UNCALIB_ID	SENSOR_ID_MAGNET_UNCALIB
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief ��Hard-Iron�r�� + Hard-Iron�p�����[�^
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data[6];	/// [��T] uncalibrated(0:x, 1:y, 2:z), offset(3:x, 4:y, 5:z)
} magnet_uncalibrated_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
