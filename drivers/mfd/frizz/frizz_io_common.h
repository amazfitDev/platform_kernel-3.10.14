/**
 * @file  通信層(SPI/I2C)共通機能
 *
 *
 */
#ifndef __FRIZZ_IO_COMMON_H__
#define __FRIZZ_IO_COMMON_H__

#define DRIVER_NAME  "frizz"   /*!< Device driver name */
#define DEVICE_COUNT  1	       /*!< Number of device */

#define CONVERT_UINT(imm0, imm1, imm2, imm3)				\
    (unsigned int)( (((imm0)&0xFF)<<24) | (((imm1)&0xFF)<<16) | (((imm2)&0xFF)<<8) | ((imm3)&0xFF) )



void prepare_command(unsigned char, unsigned int, unsigned char *);
void prepare_data(unsigned int, unsigned char *);

#endif /*__FRIZZ_IO_COMMON_H__*/

