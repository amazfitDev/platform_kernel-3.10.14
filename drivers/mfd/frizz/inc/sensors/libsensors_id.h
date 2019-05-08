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
	SENSOR_ID_ACCEL_POWER				= 0x84,	/// 加速度パワー
	SENSOR_ID_ACCEL_LPF					= 0x85,	/// 加速度LPF => 簡易重力方向
	SENSOR_ID_ACCEL_HPF					= 0x86,	/// 加速度HPF => 簡易線形加速度
	SENSOR_ID_ACCEL_STEP_DETECTOR		= 0x87,	/// 加速度計ステップ検出
	SENSOR_ID_ACCEL_PEDOMETER			= 0x88,	/// 加速度計歩数計
	SENSOR_ID_ACCEL_LINEAR				= 0x89,	/// 線形加速度のみ
	// Magnetometer
	SENSOR_ID_MAGNET_PARAMETER			= 0x8A,	/// 磁気キャリブレーションパラメータ管理
	SENSOR_ID_MAGNET_CALIB_SOFT			= 0x8B,	/// Soft Iron補正済み磁気センサ
	SENSOR_ID_MAGNET_CALIB_HARD			= 0x8C,	/// Soft Iron + Hard Iron補正済み磁気センサ
	SENSOR_ID_MAGNET_LPF				= 0x8D,	/// 磁気LPF
	SENSOR_ID_MAGNET_UNCALIB			= 0x8E,	/// Soft Iron補正済み + Hard Ironパラメータ
	// Gyroscope
	SENSOR_ID_GYRO_LPF					= 0x8F,	/// 角速度LPF => 簡易オフセット
	SENSOR_ID_GYRO_HPF					= 0x90,	/// 角速度HPF => 簡易オフセット較正
	SENSOR_ID_GYRO_UNCALIB				= 0x91,	/// 角速度未較正 + オフセット値
	// Fusion 6D
	SENSOR_ID_GRAVITY					= 0x92,	/// 加速度 + 角速度 による重力方向
	SENSOR_ID_DIRECTION					= 0x93,	/// 磁気 + 角速度 による磁北方向
	// Fusion 9D
	SENSOR_ID_POSTURE					= 0x94,	/// 加速度 + 磁気 + 角速度 による 重力+磁北 方向
	SENSOR_ID_ROTATION_MATRIX			= 0x95,	/// 出力禁止（回転行列: センサ座標系 => 世界座標系[ENU]）
	SENSOR_ID_ORIENTATION				= 0x96,	/// azimuth, pitch, roll
	SENSOR_ID_ROTATION_VECTOR			= 0x97,	/// 四元数 + 精度
	// PDR
	SENSOR_ID_PDR						= 0x98,	/// PDR
	SENSOR_ID_VELOCITY					= 0x99,	/// 速度計
	SENSOR_ID_RELATIVE_POSITION			= 0x9A,/// 相対位置
	SENSOR_ID_MIGRATION_LENGTH			= 0x9B,	/// 移動距離
	// Util
	SENSOR_ID_CYCLIC_TIMER				= 0x9C,	/// 定周期タイマ
	SENSOR_ID_DEBUG_QUEUE_IN			= 0x9D,	/// QUEUE 入力 for DEBUG
	SENSOR_ID_DEBUG_STD_IN				= 0x9E,	/// standard input
	// Accelerometer
	SENSOR_ID_ACCEL_MOVE				= 0x9F,	/// 加速度動き検出
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
