#ifndef __JZ_SPI_NANDFLASH__
#define __JZ_SPI_NANDFLASH__

#include <mach/jzssi.h>

#define SPINAND_OP_BL_128K	(128 * 1024)

#define SPINAND_CMD_RDID		0x9f	/* read spi nand id */
#define SPINAND_CMD_WREN		0x06	/* spi nand write enable */
#define SPINAND_CMD_PRO_LOAD	0x02	/* program load */
#define SPINAND_CMD_PRO_EN		0x10	/* program load execute */
#define SPINAND_CMD_PARD		0x13	/* read page data to spi nand cache */
#define SPINAND_CMD_PLRd		0X84	/*program load random data */
#define SPINAND_CMD_RDCH		0x03	/* read from spi nand cache */
#define SPINAND_CMD_FRCH		0x0b	/* fast read from spi nand cache */
#define SPINAND_CMD_ERASE_128K	0xd8	/* erase spi nand block 128K */

#define SPINAND_CMD_GET_FEATURE	0x0f	/* get spi nand feature	*/
#define SPINAND_FEATURE_ADDR	0xc0	/* get feature addr */

#define E_FALI		(1 << 2)	/* erase fail */
#define p_FAIL		(1 << 3)	/* write fail */
#define SPINAND_IS_BUSY	(1 << 0)/* PROGRAM EXECUTE, PAGE READ, BLOCK ERASE, or RESET command is executing */

#endif /*__JZ_SPI_NANDFLASH__*/
