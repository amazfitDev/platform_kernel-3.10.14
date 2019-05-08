#ifndef __ISP_SEN_IF_H__
#define __ISP_SEN_IF_H__

#include "libsensors_id.h"

/**
 * @name Sensor ID
 */
//@{
/**
 * @brief ISP Presence
 */
#define PDR_ID	SENSOR_ID_ISP
//@}


/**
 * @brief 行動状態
 */
typedef enum {
	ISP_SEN_REST=0,			/// 静止
	ISP_SEN_STOP,			/// 停止
	ISP_SEN_WALK,			/// 歩行中
	ISP_SEN_RUN,			/// 走行中
	ISP_SEN_VEHICLE,		/// 乗車中
	ISP_SEN_STAT_MAX
} isp_sen_status_e;

/**
 * @name Output
 */
//@{
/**
 * @brief 行動状態
 *
 * @note On-changeed
 */
typedef struct {
	isp_sen_status_e	status;		///< 行動状態
} isp_sen_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
//@}

#endif
