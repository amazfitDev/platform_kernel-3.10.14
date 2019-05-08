#ifndef __HUB_MGR_IF_H__
#define __HUB_MGR_IF_H__

/**
 * @name Version
 */
//@{
#define HUB_MGR_VERSION	2014041400
//@}

/**
 * @name Sensor ID
 */
//@{
/**
 * @brief HUB Manager's Sensor ID
 */
#define HUB_MGR_ID	0xFF
//@}

/**
 * @name Output
 */
//@{
/**
 * @brief output data
 * @note On-changed
 * @note this sensor always pushes to HW FIFO
 */
typedef struct {
	int	interval;		/// Interval Time [msec] 0: On-event, 0<: interval time
	int f_status;		/// status flag #HUB_MGR_IF_STATUS_ACTIVE ...
} hub_mgr_data_t;
//@}

/**
 * @name Status Flag
 */
//@{
#define HUB_MGR_IF_STATUS_ACTIVE				(1 << 0)		/// some sensor is active
#define HUB_MGR_IF_STATUS_FIFO_FULL_NOTIFY		(1 << 1)		/// detect SW FIFO threshold
#define HUB_MGR_IF_STATUS_UPDATED				(1 <<30)		/// Flag of updated (for Internal)
#define HUB_MGR_IF_STATUS_END					(1 <<31)		/// Force stop (for DEBUG)
//@}

/**
 * @name Command Code Generater
 */
//@{
/**
 * @brief Generate Command Code for HUB Manager
 *
 * @param cmd_id: command ID
 * @param imm0: immediate value0
 * @param imm1: immediate value1
 * @param imm2: immediate value2
 *
 * @return Command Code for HUB Manager
 */
#define HUB_MGR_GEN_CMD_CODE(cmd_id, imm0, imm1, imm2)	\
	(unsigned int)( (((cmd_id)&0xFF)<<24) | (((imm0)&0xFF)<<16) | (((imm1)&0xFF)<<8) | ((imm2)&0xFF) )
//@}

/**
 * @name Command List
 */
//@{
/**
 * @brief Return code for Command
 */
typedef enum {
	HUB_MGR_CMD_RET_OK				= 0,	/// OK
	HUB_MGR_CMD_RET_E_SENSOR_ID		= -1,	/// Ivalid Sensor ID
	HUB_MGR_CMD_RET_E_CMD_ID		= -2,	/// Ivalid CMD ID
	HUB_MGR_CMD_RET_E_PARAMETER		= -3,	/// Parameter error
} HUB_MGR_CMD_RET_CODE;

/**
 * @brief deactivate sensor
 *
 * @param cmd_code #HUB_MGR_GEN_CMD_CODE (0x00, sen_id, 0x00, 0x00)
 *
 * @note unsigned char sen_id: sensor id
 *
 * @return #HUB_MGR_CMD_RET_CODE
 */
#define HUB_MGR_CMD_DEACTIVATE_SENSOR		0x00

/**
 * @brief set sensor activate
 *
 * @param cmd_code #HUB_MGR_GEN_CMD_CODE (0x01, sen_id, dst, int_no)
 *
 * @note unsigned char sen_id: sensor id
 * @note unsigned char dst: (0:to HW FIFO, 1:to SW FIFO(#HUB_MGR_CMD_PULL_SENSOR_DATA)
 * @note unsigned char f_int: (0:output without interrupt signal, 1:output with interrupt signal)
 *
 * @note (dst == 0): If HW FIFO is full, you will drop some sensor data after timing when HW FIFO is full.
 * @note (dst == 1): It pushes sensor data to SW FIFO.
 * If SW FIFO was nearly full, you will get HUB_MGR's output with interrupt signal.
 * If you don't pull data from SW FIFO, It is overwritten on old datas.
 *
 * @return #HUB_MGR_CMD_RET_CODE
 */
#define HUB_MGR_CMD_SET_SENSOR_ACTIVATE		0x01

/**
 * @brief set sensor interval
 *
 * @param cmd_code #HUB_MGR_GEN_CMD_CODE (0x02, sen_id, 0x00, 0x00)
 * @param cmd_param[0] int interval tick_num [msec]
 *
 * @note sen_id sensor id
 *
 * @return #HUB_MGR_CMD_RET_CODE
 */
#define HUB_MGR_CMD_SET_SENSOR_INTERVAL		0x02

/**
 * @brief get sensor data
 *
 * @param cmd_code #HUB_MGR_GEN_CMD_CODE (0x03, sen_id, 0x00, 0x00)
 *
 * @note sen_id sensor id
 *
 * @return #HUB_MGR_CMD_RET_CODE
 */
#define HUB_MGR_CMD_GET_SENSOR_DATA			0x03

/**
 * @brief pull unit of sensor data from SW FIFO
 *
 * @param cmd_code #HUB_MGR_GEN_CMD_CODE (0x04, 0x00, 0x00, 0x00)
 *
 * @return <0: #HUB_MGR_CMD_RET_CODE, >=0:remain FIFO unit
 */
#define HUB_MGR_CMD_PULL_SENSOR_DATA		0x04

/**
 * @brief setting HUB_MGR
 *
 * @param cmd_code #HUB_MGR_GEN_CMD_CODE (0x05, int_no, 0x00, 0x00)
 *
 * @note char int_no: (0~n:with falling edge signal on specific GPIO#int_no at HUB_MGR data output, <0:without interrupt signal)
 *
 * @note defualt is without signal
 *
 * @return #HUB_MGR_CMD_RET_CODE
 */
#define HUB_MGR_CMD_SET_SETTING				0x05

/**
 * @brief get FIFO empty words
 *
 * @param cmd_code #HUB_MGR_GEN_CMD_CODE (0x06, 0x00, 0x00, 0x00)
 *
 * @return <0: #HUB_MGR_CMD_RET_CODE, >=0:remain FIFO empty words
 *
 * @note 1 unit = <header>(1word) + <timestamp>(1 word) + (word of sensor output data)
 */
#define HUB_MGR_CMD_GET_FIFO_EMPTY_SIZE		0x06

/**
 * @brief get HUB_MGR Version
 *
 * @param cmd_code #HUB_MGR_GEN_CMD_CODE (0x07, 0, 0, 0)
 *
 * @return #HUB_MGR_VERSION
 */
#define HUB_MGR_CMD_GET_VERSION				0x07

/**
 * @brief get sensor activate
 *
 * @param cmd_code #HUB_MGR_GEN_CMD_CODE (0x08, sen_id, 0, 0)
 *
 * @note sen_id sensor id
 *
 * @return <0: #HUB_MGR_CMD_RET_CODE, 0: deactive, 1: active
 */
#define HUB_MGR_CMD_GET_SENSOR_ACTIVATE		0x08
//@}

#endif
