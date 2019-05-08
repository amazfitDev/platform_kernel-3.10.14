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
 * @brief PDR���
 *
 * @note Continuous
 * @note ���E���W�nENU(x:East, y:North, z:Up)�ɂ�����e�����̏o��
 */
typedef struct {
	unsigned int	count;		///< �X�e�b�v���J�E���g
	FLOAT			rpos[2];	///< ���Έʒu (0:x, 1:y) [m]
	FLOAT			velo[2];	///< ���x (0:x, 1:y) [m/sec]
	FLOAT			total_dst;	///< �ݐϋ��� [m]
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
