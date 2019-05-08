#ifndef __ACCL_LINE_GLOBAL_IF_H__
#define __ACCL_LINE_GLOBAL_IF_H__

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
 * @brief Linear Accel @ global coord for Dead Reckoning
 */
#define ACCEL_LINE_GLOBAL_ID	SENSOR_ID_ACCEL_LINE_GLOBAL
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief 加速度 @ グローバル座標系ENU
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data[3];	/// [G] 0:x(East), 1:y(North), 2:z(Up)
} accel_line_global_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
