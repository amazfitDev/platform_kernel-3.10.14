#ifndef __MAGN_PARAM_IF_H__
#define __MAGN_PARAM_IF_H__

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
 * @brief ���C�L�����u���[�V�����p�����[�^�Ǘ�
 */
#define MAGNET_PARAMETER_ID	SENSOR_ID_MAGNET_PARAMETER
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief �L�����u���[�V�����p�����[�^
 *
 * @note One-shot
 */
typedef struct {
	FLOAT		data[12];	/// 0~8: SoftIron, 9-11:
} magnet_parameter_data_t;
//@}

/**
 * @name Command List
 *
 * @note none
 */
//@{
/**
 * @brief Calculate Parameter for Calibration
 *
 * @param cmd_code MAGNET_PARAMETER_CMD_CALC_PARAMETER
 *
 * @return 0: OK, -1:Not active
 *
 * @note ���O�ɖ{�Z���T��Activate���Ă�������
 * @note ��̑S���ʂ̎��͂���͏o������{�R�}���h�𔭍s���Ă�������
 */
#define MAGNET_PARAMETER_CMD_CALC_PARAMETER		0x00

/**
 * @brief set Parameter for Calibration
 *
 * @param cmd_code MAGNET_PARAMETER_CMD_SET_PARAMETER
 * @param cmd_param[] 0~11 Output Data from this sensor
 *
 * @return 0:OK
 */
#define MAGNET_PARAMETER_CMD_SET_PARAMETER		0x01
//@}

#endif
