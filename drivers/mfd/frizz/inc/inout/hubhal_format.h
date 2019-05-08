#ifndef __HUBHAL_FORMAT_H__
#define __HUBHAL_FORMAT_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Header
 */
typedef union {
	unsigned int	w;
	struct {
		unsigned char	num;		///< payload word num
		unsigned char	sen_id;		///< sensor ID
		unsigned char	type;		///< 0x80: SensorOutput, 0x81: Command, 0x82: MessageACK, 0x83: MessageNACK, 0x84: Response, 0x8F: BreakCode
		unsigned char	prefix;		///< 0xFF
	};
} hubhal_format_header_t;

/**
 * @brief ack code
 * @note frizz 側からの ACK
 */
#define HUBHAL_FORMAT_ACK_CODE		0xFF82FF00

/**
 * @brief nack code
 * @note frizz 側からの NACK
 */
#define HUBHAL_FORMAT_NACK_CODE		0xFF83FF00

/**
 * @brief break code
 * @note このコード発行でコマンド設定状態をクリアする
 */
#define HUBHAL_FORMAT_BREAK_CODE	0xFF8FFF00

#ifdef __cplusplus
}
#endif

#endif//__HUBHAL_FORMAT_H__
