#include "frizz_packet.h"

#ifndef __FRIZZ_CONNECTION_H__
#define __FRIZZ_CONNECTION_H__

#define MESSAGE_ACK   ( (((0xFF)&0xFF)<<24) | (((0x82)&0xFF)<<16) | (((0xFF)&0xFF)<<8) | ((0x00)&0xFF) )
#define MESSAGE_NACK  ( (((0xFF)&0xFF)<<24) | (((0x83)&0xFF)<<16) | (((0xFF)&0xFF)<<8) | ((0x00)&0xFF) )

//                            0xFF                  0x84                  sen_id              num
#define RESPONSE_HUB_MGR ( (((0xFF)&0xFF)<<24) | (((0x84)&0xFF)<<16) | (((0xFF)&0xFF)<<8) | ((0x02)&0xFF) )
#define RESPONSE_HUB_MGR_PEDO_INTER ( (((0xFF)&0xFF)<<24) | (((0x84)&0xFF)<<16) | (((0xFF)&0x88)<<8) | ((0x02)&0xFF) )
#define RESPONSE_HUB_MGR_GESTURE_INTER ( (((0xFF)&0xFF)<<24) | (((0x84)&0xFF)<<16) | (((0xFF)&0xA6)<<8) | ((0x02)&0xFF) )
#define RESPONSE_HUB_MGR_ACC_ORI ( (((0xFF)&0xFF)<<24) | (((0x84)&0xFF)<<16) | (((0xFF)&0x80)<<8) | ((0x02)&0xFF) )
#define RESPONSE_HUB_MGR_GYRO_ORI ( (((0xFF)&0xFF)<<24) | (((0x84)&0xFF)<<16) | (((0xFF)&0x82)<<8) | ((0x02)&0xFF) )
#define RESPONSE_HUB_MGR_MAGN_ORI ( (((0xFF)&0xFF)<<24) | (((0x84)&0xFF)<<16) | (((0xFF)&0x81)<<8) | ((0x02)&0xFF) )
#define RESPONSE_HUB_MGR_FALL_PARA ( (((0xFF)&0xFF)<<24) | (((0x84)&0xFF)<<16) | (((0xFF)&0xA4)<<8) | ((0x02)&0xFF) )

/*!< Sensor output parameter*/
//                                0xFF                  0x80                 sen_id               num
#define OUTPUT_INTERVAL_TIME ( (((0xFF)&0xFF)<<24) | (((0x80)&0xFF)<<16) | (((0xFF)&0xFF)<<8) | ((0x03)&0xFF) )


#define CONVERT_UINT(imm0, imm1, imm2, imm3)				\
    (unsigned int)( (((imm0)&0xFF)<<24) | (((imm1)&0xFF)<<16) | (((imm2)&0xFF)<<8) | ((imm3)&0xFF) )


#define MAX_LOOP_COUNT 10 /*!< Number of wait */

/*!
 * comminication processing with frizzFW
 *
 * @param[in] packet data
 * @return 0=sucess, otherwise 0=fail
 */
int process_connection(packet_t);

/*!
 * initialize semaphore
 */
void init_frizz_fifo_sem(void);

/*!
 * read frizz hw fifo
 *
 * @return 0=sucess, otherwise 0=fail
 */
int read_fifo(void);

#endif

