#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <mach/jzssi.h>
#include "board_base.h"


#if defined(CONFIG_MTD_JZ_SPI_NORFLASH)|| defined(CONFIG_MTD_JZ_SFC_NORFLASH)
#ifdef CONFIG_SUPPORT_OTA
#define BOOTLOADER_SIZE     0x60000  /* 384k */
#define BOOTLOADER_OFFSET   0  /* 0k */
#define JIFFS2_SIZE     0xa0000  /* 640k */
#define JIFFS2_OFFSET   BOOTLOADER_SIZE  /* 418k */
#define KERNEL_SIZE     0x300000  /* 3M */
#define KERNEL_OFFSET   JIFFS2_OFFSET +  JIFFS2_SIZE /* 1M */
#define UPDATAFS_SIZE   0x400000  /* 3M */
#define UPDATAFS_OFFSET KERNEL_OFFSET +  KERNEL_SIZE /* 4M */
#define USERFS_SIZE     0x800000  /* 8M */
#define USERFS_OFFSET   UPDATAFS_OFFSET +  UPDATAFS_SIZE /* 8M */
#else
#define BOOTLOADER_SIZE     0x40000 /* 256K */
#define BOOTLOADER_OFFSET   0  /* 0K */
#define KERNEL_SIZE     0x300000  /* 3M */
#define KERNEL_OFFSET   BOOTLOADER_OFFSET +  BOOTLOADER_SIZE /* 256K */
#define USERFS_SIZE     0xC80000  /* 12.5M */
#define USERFS_OFFSET   0x360000  /* 3.5M */
#endif

struct mtd_partition jz_mtd_partition1[] = {
	{
		.name =     "bootloader",
		.offset =   BOOTLOADER_OFFSET,
		.size =     BOOTLOADER_SIZE,
	},
#ifdef CONFIG_SUPPORT_OTA
	{
		.name =     "jiffs2",
		.offset =   JIFFS2_OFFSET,
		.size =     JIFFS2_SIZE,
	},
#endif
	{
		.name =     "kernel",
		.offset =   KERNEL_OFFSET,
		.size =     KERNEL_SIZE,
	},
#ifdef CONFIG_SUPPORT_OTA
	{
		.name =     "updatafs",
		.offset =   UPDATAFS_OFFSET,
		.size =     UPDATAFS_SIZE,
	},
#endif
	{
		.name =     "userfs",
		.offset =   USERFS_OFFSET,
		.size =     USERFS_SIZE,
	},
};
#endif	/* defined(CONFIG_MTD_JZ_SPI_NORFLASH)|| defined(CONFIG_MTD_JZ_SFC_NORFLASH) */

#if defined(CONFIG_MTD_JZ_SPI_NAND) || defined(CONFIG_JZ_SFCNAND)
#define SIZE_UBOOT  0x100000    /* 1M */
#define SIZE_KERNEL 0x800000    /* 8M */
#define SIZE_ROOTFS (0x100000 * 40)        /* -1: all of left */

struct mtd_partition jz_mtd_spinand_partition[] = {
	{
		.name =     "uboot",
		.offset =   0,
		.size =     SIZE_UBOOT,
	},
	{
		.name =     "kernel",
		.offset =   MTDPART_OFS_APPEND,
		.size =     SIZE_KERNEL,
	},
	{
		.name   =       "rootfs",
		.offset =   MTDPART_OFS_APPEND,
		.size   =   SIZE_ROOTFS,
	},
	{
		.name   =       "data",
		.offset =   MTDPART_OFS_APPEND,
		.size   =   MTDPART_SIZ_FULL,
	}
};
static struct jz_spi_support jz_spi_nand_support_table[] = {
	{
		.id_manufactory = 0xc8,
		.id_device = 0xd1,
		.name = "GD5F1GQ4UBY1G",
		.page_size = 2 * 1024,
		.oobsize = 128,
		.block_size = 128 * 1024,
		.size = 128 * 1024 * 1024,
		.column_cmdaddr_bits = 24,

		.tRD_maxbusy = 120, /* unit: ns*/
		.tPROG_maxbusy = 700,
		.tBERS_maxbusy = 5000,
	},
	{
		.id_manufactory = 0xc9,
		.id_device = 0x51,
		.name = "QPSYG01AW0A-A1",
		.page_size = 2 * 1024,
		.oobsize = 128,
		.block_size = 128 * 1024,
		.size = 128 * 1024 * 1024,
		.column_cmdaddr_bits = 24,

		.tRD_maxbusy = 120, /* unit: ns*/
		.tPROG_maxbusy = 700,
		.tBERS_maxbusy = 5000,
	},
	{
		.id_manufactory = 0xb2,
		.id_device = 0x48,
		.name = "GD5F2GQ4U",
		.page_size = 2 * 1024,
		.oobsize = 128,
		.block_size = 128 * 1024,
		.size = 256 * 1024 * 1024,
		.column_cmdaddr_bits = 32,

		.tRD_maxbusy = 120, /* unit: ns*/
		.tPROG_maxbusy = 700,
		.tBERS_maxbusy = 5000,
	},
	{
		.id_manufactory = 0xa1,
		.id_device = 0xe1,
		.name = "PN26G01AWSIUG-1Gbit",
		.page_size = 2 * 1024,
		.oobsize = 128,
		.block_size = 128 * 1024,
		.size = 128 * 1024 * 1024,
		.column_cmdaddr_bits = 24,

		.tRD_maxbusy = 240, /* unit: ns*/
		.tPROG_maxbusy = 1400,
		.tBERS_maxbusy = 10 * 1000,
	},

};
struct jz_spi_nand_platform_data jz_spi_nand_data = {
	.jz_spi_support = jz_spi_nand_support_table,
	.num_spi_flash	= ARRAY_SIZE(jz_spi_nand_support_table),
	.mtd_partition  = jz_mtd_spinand_partition,
	.num_partitions = ARRAY_SIZE(jz_mtd_spinand_partition),
};
#endif/* defined(CONFIG_MTD_JZ_SPI_NAND) || defined(CONFIG_JZ_SFCNAND) */


#if defined(CONFIG_JZ_SPI0) || defined(CONFIG_JZ_SFC) || defined(CONFIG_SPI_GPIO)
#if defined(CONFIG_JZ_SPI_NOR) || defined(CONFIG_JZ_SFC_NOR)
struct spi_nor_block_info flash_block_info[] = {
	{
		.blocksize      = 64 * 1024,
		.cmd_blockerase = 0xD8,
		.be_maxbusy     = 1200,  /* 1.2s */
	},

	{
		.blocksize      = 32 * 1024,
		.cmd_blockerase = 0x52,
		.be_maxbusy     = 1000,  /* 1s */
	},
};
#ifdef CONFIG_SPI_QUAD
struct spi_quad_mode  flash_quad_mode[] = {
	{
		.RDSR_CMD = CMD_RDSR_1,
		.WRSR_CMD = CMD_WRSR_1,
		.RDSR_DATE = 0x2,//the data is write the spi status register for QE bit
		.RD_DATE_SIZE = 1,
		.WRSR_DATE = 0x2,//this bit should be the flash QUAD mode enable
		.WD_DATE_SIZE = 1,
		.cmd_read = CMD_QUAD_READ,//
		.sfc_mode = TRAN_SPI_QUAD,
	},
	{
		.RDSR_CMD = CMD_RDSR,
		.WRSR_CMD = CMD_WRSR,
		.RDSR_DATE = 0x40,//the data is write the spi status register for QE bit
		.RD_DATE_SIZE = 1,
		.WRSR_DATE = 0x40,//this bit should be the flash QUAD mode enable
		.WD_DATE_SIZE = 1,
		.cmd_read = CMD_QUAD_IO_FAST_READ,
		.sfc_mode = TRAN_SPI_IO_QUAD,
	},
	{
		.RDSR_CMD = CMD_RDSR_1,
		.WRSR_CMD = CMD_WRSR,
		.RDSR_DATE = 0x20,//the data is write the spi status register for QE bit
		.RD_DATE_SIZE = 1,
		.WRSR_DATE = 0x200,//this bit should be the flash QUAD mode enable
		.WD_DATE_SIZE = 2,
		.cmd_read = CMD_QUAD_READ,
		.sfc_mode = TRAN_SPI_QUAD,
	},
	{
		.RDSR_CMD = CMD_RDSR,
		.WRSR_CMD = CMD_WRSR,
		.RDSR_DATE = 0x40,//the data is write the spi status register for QE bit
		.RD_DATE_SIZE = 1,
		.WRSR_DATE = 0x40,//this bit should be the flash QUAD mode enable
		.WD_DATE_SIZE = 1,
		.cmd_read = CMD_QUAD_READ,
		.sfc_mode = TRAN_SPI_QUAD,
	},

};
#endif

struct spi_nor_platform_data spi_nor_pdata[] = {
	{
		.name           = "GD25Q128C",
		.pagesize       = 256,
		.sectorsize     = 4 * 1024,
		.chipsize       = 16384 * 1024,
		.erasesize      = 32 * 1024,//4 * 1024,
		.id             = 0xc84018,

		.block_info     = flash_block_info,
		.num_block_info = ARRAY_SIZE(flash_block_info),

		.addrsize       = 3,
		.pp_maxbusy     = 3,            /* 3ms */
		.se_maxbusy     = 400,          /* 400ms */
		.ce_maxbusy     = 8 * 10000,    /* 80s */

		.st_regnum      = 3,
#if defined(CONFIG_MTD_JZ_SPI_NORFLASH) || defined(CONFIG_MTD_JZ_SFC_NORFLASH)
		.mtd_partition  = jz_mtd_partition1,
		.num_partition_info = ARRAY_SIZE(jz_mtd_partition1),
#endif
#ifdef CONFIG_SPI_QUAD
		.quad_mode = &flash_quad_mode[0],
#endif
	},
	{
		.name           = "GD25LQ128C",
		.pagesize       = 256,
		.sectorsize     = 4 * 1024,
		.chipsize       = 16384 * 1024,
		.erasesize      = 32 * 1024,//4 * 1024,
		.id             = 0xc86018,

		.block_info     = flash_block_info,
		.num_block_info = ARRAY_SIZE(flash_block_info),

		.addrsize       = 3,
		.pp_maxbusy     = 3,            /* 3ms */
		.se_maxbusy     = 400,          /* 400ms */
		.ce_maxbusy     = 8 * 10000,    /* 80s */

		.st_regnum      = 3,
#if defined(CONFIG_MTD_JZ_SPI_NORFLASH) || defined(CONFIG_MTD_JZ_SFC_NORFLASH)
		.mtd_partition  = jz_mtd_partition1,
		.num_partition_info = ARRAY_SIZE(jz_mtd_partition1),
#endif
#ifdef CONFIG_SPI_QUAD
		.quad_mode = &flash_quad_mode[2],
#endif
	},
	{
		.name           = "IS25LP128",
		.pagesize       = 256,
		.sectorsize     = 4 * 1024,
		.chipsize       = 16384 * 1024,
		.erasesize      = 32 * 1024,//4 * 1024,
		.id             = 0x9d6018,

		.block_info     = flash_block_info,
		.num_block_info = ARRAY_SIZE(flash_block_info),

		.addrsize       = 3,
		.pp_maxbusy     = 3,            /* 3ms */
		.se_maxbusy     = 400,          /* 400ms */
		.ce_maxbusy     = 8 * 10000,    /* 80s */

		.st_regnum      = 3,
#if defined(CONFIG_MTD_JZ_SPI_NORFLASH) || defined(CONFIG_MTD_JZ_SFC_NORFLASH)
		.mtd_partition  = jz_mtd_partition1,
		.num_partition_info = ARRAY_SIZE(jz_mtd_partition1),
#endif
#ifdef CONFIG_SPI_QUAD
		.quad_mode = &flash_quad_mode[1],
#endif
	},
	{
		.name           = "MX25L12835F",
		.pagesize       = 256,
		.sectorsize     = 4 * 1024,
		.chipsize       = 16384 * 1024,
		.erasesize      = 32 * 1024,//4 * 1024,
		.id             = 0xc22018,

		.block_info     = flash_block_info,
		.num_block_info = ARRAY_SIZE(flash_block_info),

		.addrsize       = 3,
		.pp_maxbusy     = 3,            /* 3ms */
		.se_maxbusy     = 400,          /* 400ms */
		.ce_maxbusy     = 8 * 10000,    /* 80s */

		.st_regnum      = 3,
#if defined(CONFIG_MTD_JZ_SPI_NORFLASH) || defined(CONFIG_MTD_JZ_SFC_NORFLASH)
		.mtd_partition  = jz_mtd_partition1,
		.num_partition_info = ARRAY_SIZE(jz_mtd_partition1),
#endif
#ifdef CONFIG_SPI_QUAD
		.quad_mode = &flash_quad_mode[1],
#endif
	},
	{
			.name           = "WIN25Q128",
			.pagesize       = 256,
			.sectorsize     = 4 * 1024,
			.chipsize       = 16384 * 1024,
			.erasesize      = 32 * 1024,//4 * 1024,
			.id             = 0xef4018,

			.block_info     = flash_block_info,
			.num_block_info = ARRAY_SIZE(flash_block_info),

			.addrsize       = 3,
			.pp_maxbusy     = 3,            /* 3ms */
			.se_maxbusy     = 400,          /* 400ms */
			.ce_maxbusy     = 8 * 10000,    /* 80s */

			.st_regnum      = 3,
#if defined(CONFIG_MTD_JZ_SPI_NORFLASH) || defined(CONFIG_MTD_JZ_SFC_NORFLASH)
			.mtd_partition  = jz_mtd_partition1,
			.num_partition_info = ARRAY_SIZE(jz_mtd_partition1),
#endif
#ifdef CONFIG_SPI_QUAD
			.quad_mode = &flash_quad_mode[2],
#endif
	},
	{
		.name           = "GD25Q256C",
		.pagesize       = 256,
		.sectorsize     = 4 * 1024,
		.chipsize       = 32768 * 1024,
		.erasesize      = 32 * 1024,//4 * 1024,
		.id             = 0xc84019,

		.block_info     = flash_block_info,
		.num_block_info = ARRAY_SIZE(flash_block_info),

		.addrsize       = 4,
		.pp_maxbusy     = 3,            /* 3ms */
		.se_maxbusy     = 400,          /* 400ms */
		.ce_maxbusy     = 8 * 10000,    /* 80s */

		.st_regnum      = 3,
#if defined(CONFIG_MTD_JZ_SPI_NORFLASH) || defined(CONFIG_MTD_JZ_SFC_NORFLASH)
		.mtd_partition  = jz_mtd_partition1,
		.num_partition_info = ARRAY_SIZE(jz_mtd_partition1),
#endif
#ifdef CONFIG_SPI_QUAD
		.quad_mode = &flash_quad_mode[3],
#endif
	},
};
#endif	/* defined(CONFIG_JZ_SPI_NOR) || defined(CONFIG_JZ_SFC_NOR) */

struct spi_board_info jz_spi0_board_info[]  = {
#ifdef CONFIG_MTD_JZ_SPI_NOR
	[0] ={
		.modalias       =  "jz_spi_norflash",
		.platform_data          = &spi_nor_pdata[0],
		.controller_data        = (void *)SPI_CHIP_ENABLE, /* cs for spi gpio */
		.max_speed_hz           = 12000000,
		.bus_num                = 0,
		.chip_select            = 0,
	},
#endif
#ifdef CONFIG_MTD_JZ_SPI_NAND
	[0] = {
		.modalias	=	"jz_spi_nand",
		.platform_data	= &jz_spi_nand_data,
		.controller_data        = (void *)SPI_CHIP_ENABLE, /* cs for spi gpio */
		//.controller_data	=	(void *)( -1),
		.max_speed_hz		=	12000000,
		.bus_num			=	0,
		.chip_select		=	0,
	},
#endif
};
int jz_spi0_devs_size = ARRAY_SIZE(jz_spi0_board_info);
#endif	/* defined(CONFIG_JZ_SPI0) || defined(CONFIG_JZ_SFC) || defined(CONFIG_SPI_GPIO) */

#ifdef CONFIG_JZ_SPI0
struct jz_spi_info spi0_info_cfg = {
	.chnl = 0,
	.bus_num = 0,
	.max_clk = 54000000,
	.num_chipselect = 1,
	.allow_cs_same  = 1,
	.chipselect     = {SPI_CHIP_ENABLE,SPI_CHIP_ENABLE},
};
#endif

#ifdef CONFIG_JZ_SFC
#ifdef CONFIG_JZ_SFC_NOR
struct jz_sfc_info sfc_info_cfg = {
	.chnl = 0,
	.bus_num = 0,
	.num_chipselect = 1,
	.board_info = spi_nor_pdata,
	.board_info_size = ARRAY_SIZE(spi_nor_pdata),
};
#elif defined(CONFIG_JZ_SFCNAND)
struct jz_sfc_info sfc_info_cfg = {
	.chnl = 0,
	.bus_num = 0,
	.num_chipselect = 1,
	.board_info = &jz_spi_nand_data,
};
#endif
#endif	/* CONFIG_JZ_SFC */

#ifdef CONFIG_SPI_GPIO
static struct spi_gpio_platform_data jz_spi_gpio_data = {

	.sck	= GPIO_SPI_SCK,
	.mosi	= GPIO_SPI_MOSI,
	.miso	= GPIO_SPI_MISO,
	.num_chipselect = 1,
};

struct platform_device jz_spi_gpio_device = {
	.name   = "spi_gpio",
	.dev    = {
		.platform_data = &jz_spi_gpio_data,
	},
};
#endif

