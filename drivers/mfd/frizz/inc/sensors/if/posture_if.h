#ifndef __POSTURE_IF_H__
#define __POSTURE_IF_H__

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
 * @brief Posture
 */
#define POSTURE_ID	SENSOR_ID_POSTURE
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief �d�� + ���k ����
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		grav[3];	/// gravity vector [G] 0:x, 1:y, 2:z
	FLOAT		magn[3];	/// magnetic north vector [��T] 0:x, 1:y, 2:z
} posture_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
