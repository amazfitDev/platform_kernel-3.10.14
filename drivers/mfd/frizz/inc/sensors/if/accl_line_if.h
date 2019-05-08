#ifndef __ACCL_LINE_IF_H__
#define __ACCL_LINE_IF_H__

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
 * @brief Linear Accel via HPF
 */
#define ACCEL_LINE_ID	SENSOR_ID_ACCEL_LINE
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief ê¸å`â¡ë¨ìx
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data[3];	/// [G] 0:x, 1:y, 2:z
} accel_line_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
