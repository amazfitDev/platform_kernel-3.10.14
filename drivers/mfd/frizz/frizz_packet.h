
#ifndef __FRIZZ_PACKET_H__
#define __FRIZZ_PACKET_H__

#pragma once

#include "inc/inout/hubhal_format.h"

#define MAX_DATA_NUM 10

/*! @struct packet_t
 *  @brief  sent data and received data from frizz fw
 */
typedef struct {
    hubhal_format_header_t header;    /*!< frizz fw header */
    unsigned int data[MAX_DATA_NUM];  /*!< sent and received data*/
}packet_t;

/*!
 * analyze fifo which received from frizz
 *
 * @param[in] received frizz fifo data
 * @param[out] start place of analysis
 * @return 0=sucess, otherwise 0=fail
 */
int analysis_fifo(unsigned int*, int*);

/*!
 * analyze packet which received from frizz
 *
 * @param[in] received frizz packet data
 * @return 0=sucess, otherwise 0=fail
 */
int analysis_packet(packet_t*);

/*!
 * initialize callback function
 *
 */
void init_callback(void);

/*!
 * create fifo empty packet and unlock polling process
 *
 * @return 0=sucess, otherwise 0=fail
 */
int create_sensor_type_fifo_fake(unsigned char id, unsigned long *data, int data_len);
#endif
