#ifndef __FRIZZ_REG_H__
#define __FRIZZ_REG_H__


#define WRITE_COMMAND	  0x80  /*!< Write command to frizz register */
#define READ_COMMAND	  0x00  /*!< Read command to frizz register */

#define	CTRL_REG_ADDR	  0x00  /*!< Address of frizz control register */
#define VER_REG_ADDR	  0x01  /*!< Address of frizz version register */
#define MES_REG_ADDR	  0x02  /*!< Address of frizz message register */
#define MODE_REG_ADDR	  0x03  /*!< Address of frizz mode register */
#define FIFO_CNR_REG_ADDR 0x3f  /*!< Address of frizz fifo control register */
#define FIFO_REG_ADDR	  0x40  /*!< Address of frizz fifo data register */
#define RAM_ADDR_REG_ADDR 0x52  /*!< Address of frizz ram address register */
#define RAM_DATA_REG_ADDR 0x53  /*!< Address of frizz ram data register */

#define CTRL_REG_RUN	       0x0000 /*!< Run command for control register*/
#define CTRL_REG_SYSTEM_RESET  0x0001 /*!< system reset command for control register*/
#define CTRL_REG_STALL	       0x0002 /*!< stall command for control register*/
#define CTRL_REG_FIFO_RESET    0x0008 /*!< fifo reset command for control register*/

#define TRANS_BUFF_SIZE	        131072  /*!< Max transfer buffer size (Byte)*/
#define MAX_HWFRIZZ_FIFO_SIZE   64      /*!< Max frizz hw fifo size (Byte)*/

#endif
