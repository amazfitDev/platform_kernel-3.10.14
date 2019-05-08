#ifndef __PDR_POS_IF_H__
#define __PDR_POS_IF_H__

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
 * @brief relative position depend on Pedestrian Dead Reckoning
 */
#define RELATIVE_POSITION_ID	SENSOR_ID_RELATIVE_POSITION
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief 相対位置
 *
 * @note Continuous
 * @note 世界座標系ENU(x:East, y:North, z:Up)における各成分の出力
 */
typedef struct {
	FLOAT			rpos[2];	///< 相対位置 (0:x, 1:y) [m]
} pdr_vel_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
/**
 * @brief Set Offset of Relative Position
 *
 * @param cmd_code RELATIVE_POSITION_CMD_SET_OFFSET
 * @param param[0] float ofst_x
 * @param param[1] float ofst_y
 *
 * @return 0: OK
 */
#define RELATIVE_POSITION_CMD_SET_OFFSET		0x00
//@}

#endif
