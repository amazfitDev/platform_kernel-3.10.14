#ifndef __PRES_RAW_IF_H__
#define __PRES_RAW_IF_H__

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
 * @brief Pressure from Physical Sensor
 */
#define PRESSURE_RAW_ID	SENSOR_ID_PRESSURE_RAW
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief ‹Cˆ³
 *
 * @note Continuous
 */
typedef struct {
	FLOAT		data;	/// [hPa]
} pressure_raw_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
