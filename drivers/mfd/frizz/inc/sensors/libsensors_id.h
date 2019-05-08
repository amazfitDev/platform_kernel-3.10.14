#ifndef __LIBSENSORS_ID_H__
#define __LIBSENSORS_ID_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
/* Physical Sensors */
	SENSOR_ID_ACCEL_RAW					= 0x80,	/// need implementation at each System
	SENSOR_ID_MAGNET_RAW				= 0x81,	/// need implementation at each System
	SENSOR_ID_GYRO_RAW					= 0x82,	/// need implementation at each System
	SENSOR_ID_PRESSURE_RAW				= 0x83,	/// need implementation at each System

/* Application Sensors */
	// Accelerometer
	SENSOR_ID_ACCEL_POWER				= 0x84,	/// �����x�p���[
	SENSOR_ID_ACCEL_LPF					= 0x85,	/// �����xLPF => �ȈՏd�͕���
	SENSOR_ID_ACCEL_HPF					= 0x86,	/// �����xHPF => �ȈՐ��`�����x
	SENSOR_ID_ACCEL_STEP_DETECTOR		= 0x87,	/// �����x�v�X�e�b�v���o
	SENSOR_ID_ACCEL_PEDOMETER			= 0x88,	/// �����x�v�����v
	SENSOR_ID_ACCEL_LINEAR				= 0x89,	/// ���`�����x�̂�
	// Magnetometer
	SENSOR_ID_MAGNET_PARAMETER			= 0x8A,	/// ���C�L�����u���[�V�����p�����[�^�Ǘ�
	SENSOR_ID_MAGNET_CALIB_SOFT			= 0x8B,	/// Soft Iron�␳�ςݎ��C�Z���T
	SENSOR_ID_MAGNET_CALIB_HARD			= 0x8C,	/// Soft Iron + Hard Iron�␳�ςݎ��C�Z���T
	SENSOR_ID_MAGNET_LPF				= 0x8D,	/// ���CLPF
	SENSOR_ID_MAGNET_UNCALIB			= 0x8E,	/// Soft Iron�␳�ς� + Hard Iron�p�����[�^
	// Gyroscope
	SENSOR_ID_GYRO_LPF					= 0x8F,	/// �p���xLPF => �ȈՃI�t�Z�b�g
	SENSOR_ID_GYRO_HPF					= 0x90,	/// �p���xHPF => �ȈՃI�t�Z�b�g�r��
	SENSOR_ID_GYRO_UNCALIB				= 0x91,	/// �p���x���r�� + �I�t�Z�b�g�l
	// Fusion 6D
	SENSOR_ID_GRAVITY					= 0x92,	/// �����x + �p���x �ɂ��d�͕���
	SENSOR_ID_DIRECTION					= 0x93,	/// ���C + �p���x �ɂ�鎥�k����
	// Fusion 9D
	SENSOR_ID_POSTURE					= 0x94,	/// �����x + ���C + �p���x �ɂ�� �d��+���k ����
	SENSOR_ID_ROTATION_MATRIX			= 0x95,	/// �o�͋֎~�i��]�s��: �Z���T���W�n => ���E���W�n[ENU]�j
	SENSOR_ID_ORIENTATION				= 0x96,	/// azimuth, pitch, roll
	SENSOR_ID_ROTATION_VECTOR			= 0x97,	/// �l���� + ���x
	// PDR
	SENSOR_ID_PDR						= 0x98,	/// PDR
	SENSOR_ID_VELOCITY					= 0x99,	/// ���x�v
	SENSOR_ID_RELATIVE_POSITION			= 0x9A,/// ���Έʒu
	SENSOR_ID_MIGRATION_LENGTH			= 0x9B,	/// �ړ�����
	// Util
	SENSOR_ID_CYCLIC_TIMER				= 0x9C,	/// ������^�C�}
	SENSOR_ID_DEBUG_QUEUE_IN			= 0x9D,	/// QUEUE ���� for DEBUG
	SENSOR_ID_DEBUG_STD_IN				= 0x9E,	/// standard input
	// Accelerometer
	SENSOR_ID_ACCEL_MOVE				= 0x9F,	/// �����x�������o
	// Libs
	SENSOR_ID_ISP						= 0xBD,	/// ISP PRESENCE
	// Rotation
	SENSOR_ID_ROTATION_GRAVITY_VECTOR	= 0xA1,	/// Gravity Rotation Vector
	SENSOR_ID_ROTATION_LPF_VECTOR		= 0xA2,	/// Rotation Vector without Gyro
	SENSOR_ID_ACCEL_FALL_DOWN			= 0xA3,
	SENSOR_ID_ACCEL_POS_DET				= 0xA4,
	SENSOR_ID_PDR_GEOFENCING			= 0xA5,
	SENSOR_ID_GESTURE					= 0xA6,
	SENSOR_ID_LIGHT_RAW                 = 0xA9,
	SENSOR_ID_PROXIMITY                 = 0xAA,
	SENSOR_ID_ACTIVITY                  = 0xBA,
	HUB_MGR_ID							= 0xFF,
} libsensors_id_e;

#ifdef __cplusplus
}
#endif

#endif
