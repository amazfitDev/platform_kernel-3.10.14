#ifndef __PDR_LEN_IF_H__
#define __PDR_LEN_IF_H__

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
 * @brief migration length depend on Pedestrian Dead Reckoning
 */
#define MIGRATION_LENGTH_ID	SENSOR_ID_MIGRATION_LENGTH
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief —İÏ‹——£
 *
 * @note Continuous
 * @note ¢ŠEÀ•WŒnENU(x:East, y:North, z:Up)‚É‚¨‚¯‚éŠe¬•ª‚Ìo—Í
 */
typedef struct {
	FLOAT			total_dst;	///< —İÏ‹——£ [m]
} pdr_vel_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
/**
 * @brief Clear Migration length
 *
 * @param cmd_code MIGRATION_LENGTH_CMD_CLEAR
 *
 * @return 0: OK
 */
#define MIGRATION_LENGTH_CMD_CLEAR		0x00
//@}

#endif
