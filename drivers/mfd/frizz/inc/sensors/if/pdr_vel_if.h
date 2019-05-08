#ifndef __PDR_VEL_IF_H__
#define __PDR_VEL_IF_H__

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
 * @brief velocity depend on Pedestrian Dead Reckoning
 */
#define VELOCITY_ID	SENSOR_ID_VELOCITY
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief ���Α��x
 *
 * @note Continuous
 * @note ���E���W�nENU(x:East, y:North, z:Up)�ɂ�����e�����̏o��
 */
typedef struct {
	FLOAT			velo[2];	///< ���x (0:x, 1:y) [m/sec]
} pdr_vel_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
