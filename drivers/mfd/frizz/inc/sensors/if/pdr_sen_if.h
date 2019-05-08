#ifndef __PDR_SEN_IF_H__
#define __PDR_SEN_IF_H__

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
 * @brief Pedestrian Dead Reckoning
 */
#define PDR_ID	SENSOR_ID_PDR
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief PDR情報
 *
 * @note Continuous
 * @note 世界座標系ENU(x:East, y:North, z:Up)における各成分の出力
 */
typedef struct {
	unsigned int	count;		///< ステップ数カウント
	FLOAT			rpos[2];	///< 相対位置 (0:x, 1:y) [m]
	FLOAT			velo[2];	///< 速度 (0:x, 1:y) [m/sec]
	FLOAT			total_dst;	///< 累積距離 [m]
} pdr_sen_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
