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
 * @brief �s�����
 */
typedef enum {
	ISP_SEN_REST=0,			/// �Î~
	ISP_SEN_STOP,			/// ��~
	ISP_SEN_WALK,			/// ���s��
	ISP_SEN_RUN,			/// ���s��
	ISP_SEN_VEHICLE,		/// ��Ԓ�
	ISP_SEN_STAT_MAX
} isp_sen_status_e;

/**
 * @name Output
 */
//@{
/**
 * @brief �s�����
 *
 * @note On-changeed
 */
typedef struct {
	isp_sen_status_e	status;		///< �s�����
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
