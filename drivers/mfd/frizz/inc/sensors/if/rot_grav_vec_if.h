#ifndef __ROT_GRAV_VEC_IF_H__
#define __ROT_GRAV_VEC_IF_H__

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
 * @brief Gravity Rotation Vector
 */
#define ROTATION_GRAVITY_VECTOR_ID	SENSOR_ID_ROTATION_GRAVITY_VECTOR
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief ‰ñ“]ƒxƒNƒgƒ‹
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data[4];	/// quaternion 0~2:vector, 3:scaler
} rot_grav_vec_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
