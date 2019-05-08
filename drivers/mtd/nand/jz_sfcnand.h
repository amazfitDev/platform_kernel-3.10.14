#ifndef __LINUX_SFC_JZ_H
#define __LINUX_SFC_JZ_H

#include <mach/jzssi.h>

/*nand flash*/
#define UNCACHE(addr)   ((typeof(addr))(((unsigned long)(addr)) | 0xa0000000))
#define SPINAND_OP_BL_128K      (128 * 1024)

#define SPINAND_CMD_RDID                0x9f    /* read spi nand id */
#define SPINAND_CMD_WREN                0x06    /* spi nand write enable */
#define SPINAND_CMD_PRO_LOAD    0x02    /* program load */
#define SPINAND_CMD_PRO_EN              0x10    /* program load execute */
#define SPINAND_CMD_PARD                0x13    /* read page data to spi nand cache */
#define SPINAND_CMD_PLRd                0X84    /*program load random data */
#define SPINAND_CMD_RDCH                0x03    /* read from spi nand cache */
#define SPINAND_CMD_FRCH                0x0b    /* fast read from spi nand cache */
#define SPINAND_CMD_ERASE_128K  0xd8    /* erase spi nand block 128K */

#define SPINAND_CMD_GET_FEATURE 0x0f    /* get spi nand feature */
#define SPINAND_FEATURE_ADDR    0xc0    /* get feature addr */
#define E_FALI          (1 << 2)        /* erase fail */
#define p_FAIL          (1 << 3)        /* write fail */
#define SPINAND_IS_BUSY (1 << 0)/* PROGRAM EXECUTE, PAGE READ, BLOCK ERASE, or RESET command is executing */


/* Used for SST flashes only. */
#define SPINOR_OP_BP		0x02	/* Byte program */
#define SPINOR_OP_WRDI		0x04	/* Write disable */
#define SPINOR_OP_AAI_WP	0xad	/* Auto address increment word program */

/* Used for Macronix and Winbond flashes. */
#define SPINOR_OP_EN4B		0xb7	/* Enter 4-byte mode */
#define SPINOR_OP_EX4B		0xe9	/* Exit 4-byte mode */

/* Used for Spansion flashes only. */
#define SPINOR_OP_BRWR		0x17	/* Bank register write */

/* Status Register bits. */
#define SR_WIP			1	/* Write in progress */
#define SR_WEL			2	/* Write enable latch */
#define SR_SQE			(1 << 1)	/* QUAD MODE enable */
/* meaning of other SR_* bits may differ between vendors */
#define SR_BP0			4	/* Block protect 0 */
#define SR_BP1			8	/* Block protect 1 */
#define SR_BP2			0x10	/* Block protect 2 */
#define SR_SRWD			0x80	/* SR write protect */

#define SR_QUAD_EN_MX		0x40	/* Macronix Quad I/O */

/* Flag Status Register bits */
#define FSR_READY		0x80

/* Configuration Register bits. */
#define CR_QUAD_EN_SPAN		0x2	/* Spansion Quad I/O */



#define BUFFER_SIZE PAGE_SIZE

/* SFC register */

#define	SFC_GLB				(0x0000)
#define	SFC_DEV_CONF			(0x0004)
#define	SFC_DEV_STA_EXP			(0x0008)
#define	SFC_DEV_STA_RT			(0x000c)
#define	SFC_DEV_STA_MSK			(0x0010)
#define	SFC_TRAN_CONF(n)		(0x0014 + (n * 4))
#define	SFC_TRAN_LEN			(0x002c)
#define	SFC_DEV_ADDR(n)			(0x0030 + (n * 4))
#define	SFC_DEV_ADDR_PLUS(n)		(0x0048 + (n * 4))
#define	SFC_MEM_ADDR			(0x0060)
#define	SFC_TRIG			(0x0064)
#define	SFC_SR				(0x0068)
#define	SFC_SCR				(0x006c)
#define	SFC_INTC			(0x0070)
#define	SFC_FSM				(0x0074)
#define	SFC_CGE				(0x0078)
#define	SFC_RM_DR			(0x1000)

/* For SFC_GLB */
#define	GLB_TRAN_DIR			(1 << 13)
#define GLB_TRAN_DIR_WRITE		(1)
#define GLB_TRAN_DIR_READ		(0)
#define	GLB_THRESHOLD_OFFSET		(7)
#define GLB_THRESHOLD_MSK		(0x3f << GLB_THRESHOLD_OFFSET)
#define GLB_OP_MODE			(1 << 6)
#define SLAVE_MODE			(0x0)
#define DMA_MODE			(0x1)
#define GLB_PHASE_NUM_OFFSET		(3)
#define GLB_PHASE_NUM_MSK		(0x7  << GLB_PHASE_NUM_OFFSET)
#define GLB_WP_EN			(1 << 2)
#define GLB_BURST_MD_OFFSET		(0)
#define GLB_BURST_MD_MSK		(0x3  << GLB_BURST_MD_OFFSET)

/* For SFC_DEV_CONF */
#define	DEV_CONF_ONE_AND_HALF_CYCLE_DELAY	(3)
#define	DEV_CONF_ONE_CYCLE_DELAY	(2)
#define	DEV_CONF_HALF_CYCLE_DELAY	(1)
#define	DEV_CONF_NO_DELAY	        (0)
#define	DEV_CONF_SMP_DELAY_OFFSET	(16)
#define	DEV_CONF_SMP_DELAY_MSK		(0x3 << DEV_CONF_SMP_DELAY_OFFSET)
#define DEV_CONF_CMD_TYPE		(0x1 << 15)
#define DEV_CONF_STA_TYPE_OFFSET	(13)
#define DEV_CONF_STA_TYPE_MSK		(0x1 << DEV_CONF_STA_TYPE_OFFSET)
#define DEV_CONF_THOLD_OFFSET		(11)
#define	DEV_CONF_THOLD_MSK		(0x3 << DEV_CONF_THOLD_OFFSET)
#define DEV_CONF_TSETUP_OFFSET		(9)
#define DEV_CONF_TSETUP_MSK		(0x3 << DEV_CONF_TSETUP_OFFSET)
#define DEV_CONF_TSH_OFFSET		(5)
#define DEV_CONF_TSH_MSK		(0xf << DEV_CONF_TSH_OFFSET)
#define DEV_CONF_CPHA			(0x1 << 4)
#define DEV_CONF_CPOL			(0x1 << 3)
#define DEV_CONF_CEDL			(0x1 << 2)
#define DEV_CONF_HOLDDL			(0x1 << 1)
#define DEV_CONF_WPDL			(0x1 << 0)

/* For SFC_TRAN_CONF */
#define	TRAN_CONF_TRAN_MODE_OFFSET	(29)
#define	TRAN_CONF_TRAN_MODE_MSK		(0x7)
#define	TRAN_CONF_ADDR_WIDTH_OFFSET	(26)
#define	TRAN_CONF_ADDR_WIDTH_MSK	(0x7 << ADDR_WIDTH_OFFSET)
#define TRAN_CONF_POLLEN		(1 << 25)
#define TRAN_CONF_CMDEN			(1 << 24)
#define TRAN_CONF_FMAT			(1 << 23)
#define TRAN_CONF_DMYBITS_OFFSET	(17)
#define TRAN_CONF_DMYBITS_MSK		(0x3f << DMYBITS_OFFSET)
#define TRAN_CONF_DATEEN		(1 << 16)
#define	TRAN_CONF_CMD_OFFSET		(0)
#define	TRAN_CONF_CMD_MSK		(0xffff << CMD_OFFSET)

/* For SFC_TRIG */
#define TRIG_FLUSH			(1 << 2)
#define TRIG_STOP			(1 << 1)
#define TRIG_START			(1 << 0)

/* For SFC_SCR */
#define	CLR_END			(1 << 4)
#define CLR_TREQ		(1 << 3)
#define CLR_RREQ		(1 << 2)
#define CLR_OVER		(1 << 1)
#define CLR_UNDER		(1 << 0)

/* For SFC_TRAN_CONFx */
#define	TRAN_MODE_OFFSET	(29)
#define	TRAN_MODE_MSK		(0x7 << TRAN_MODE_OFFSET)
#define TRAN_SPI_STANDARD   (0x0)
#define TRAN_SPI_DUAL   (0x1 )
#define TRAN_SPI_QUAD   (0x5 )
#define TRAN_SPI_IO_QUAD   (0x6 )


#define	ADDR_WIDTH_OFFSET	(26)
#define	ADDR_WIDTH_MSK		(0x7 << ADDR_WIDTH_OFFSET)
#define POLLEN			(1 << 25)
#define CMDEN			(1 << 24)
#define FMAT			(1 << 23)
#define DMYBITS_OFFSET		(17)
#define DMYBITS_MSK		(0x3f << DMYBITS_OFFSET)
#define DATEEN			(1 << 16)
#define	CMD_OFFSET		(0)
#define	CMD_MSK			(0xffff << CMD_OFFSET)

#define N_MAX				6
#define MAX_SEGS        128
struct sfc_cmd{
	uint16_t cmd;
	uint32_t addr_low;
	uint32_t addr_high;
	unsigned char dummy_byte;
	unsigned char addr_len;
	unsigned char cmd_len;
	unsigned char transmode;
};
struct sfc_transfer_nand {
	/* it's ok if tx_buf == rx_buf (right?)
	 * for MicroWire, one buffer must be null
	 * buffers must work with dma_*map_single() calls, unless
	 * spi_message.is_dma_mapped reports a pre-existing mapping
	 */
	struct	sfc_cmd sfc_cmd;
	void        *tx_buf;
	void        *rx_buf;
	unsigned int len;
	unsigned int finally_len;
	u8 date_en;
	u8 dma_mode;
	u8 pollen;
	u8 rw_mode;
	u8 sfc_mode;
};

struct jz_sfc_nand {
	struct mtd_info     mtd;
	struct device		*dev;
	struct resource		*resource;
	void __iomem		*iomem;

	struct resource     *ioarea;

	unsigned int        clk_flag;
	unsigned int        set_clk_flag;

	u8          chnl;
	int			irq;
	struct clk		*clk;
	struct clk		*clk_gate;
	unsigned long src_clk;
	struct completion	done;
	struct completion	done_rx;
	spinlock_t		lock_status;
	spinlock_t		txrx_lock;
	int			threshold;
	int			clk_gate_flag;
	int			status;
	u8          use_dma;


	/* temp buffers */
	unsigned char   *swap_buf;

	unsigned int		rx_addr_plus;
	unsigned int		tx_addr_plus;

	spinlock_t		lock_rxtx;

	unsigned long		phys;
	struct jz_sfc_nand_info *pdata;
	irqreturn_t (*irq_callback)(struct jz_sfc_nand *);
	struct workqueue_struct *workqueue;
	struct work_struct rw_work;
	struct mutex	lock;
	/*sfc use*/
	struct sfc_transfer_nand *sfc_tran;
	/*sfc nand*/
	int column_cmdaddr_bits;
	int tRD;
	int tPROG;
	int tBERS;
};

static void sfc_writel(struct jz_sfc_nand *sfc, unsigned short offset, u32 value)
{
	writel(value, sfc->iomem + offset);
}

static unsigned int sfc_readl(struct jz_sfc_nand *sfc, unsigned short offset)
{
	return readl(sfc->iomem + offset);
}


void sfc_init(struct jz_sfc_nand *sfc)
{
	int n;
	for(n = 0; n < N_MAX; n++) {
		sfc_writel(sfc, SFC_TRAN_CONF(n), 0);
		sfc_writel(sfc, SFC_DEV_ADDR(n), 0);
		sfc_writel(sfc, SFC_DEV_ADDR_PLUS(n), 0);
	}

	sfc_writel(sfc, SFC_GLB, ((1 << 7) | (1 << 3)));
	sfc_writel(sfc, SFC_DEV_CONF, 0);
	sfc_writel(sfc, SFC_DEV_STA_EXP, 0);
	sfc_writel(sfc, SFC_DEV_STA_MSK, 0);
	sfc_writel(sfc, SFC_TRAN_LEN, 0);
	sfc_writel(sfc, SFC_MEM_ADDR, 0);
	sfc_writel(sfc, SFC_TRIG, 0);
	sfc_writel(sfc, SFC_SCR, 0);
	sfc_writel(sfc, SFC_INTC, 0);
	sfc_writel(sfc, SFC_CGE, 0);
	sfc_writel(sfc, SFC_RM_DR, 0);
}

void sfc_stop(struct jz_sfc_nand *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_TRIG);
	tmp |= TRIG_STOP;
	sfc_writel(sfc, SFC_TRIG, tmp);
}

void sfc_start(struct jz_sfc_nand *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_TRIG);
	tmp |= TRIG_START;
	sfc_writel(sfc, SFC_TRIG, tmp);
}

void sfc_flush_fifo(struct jz_sfc_nand *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_TRIG);
	tmp |= TRIG_FLUSH;
	sfc_writel(sfc, SFC_TRIG, tmp);
}

void sfc_ce_invalid_value(struct jz_sfc_nand *sfc, int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp &= ~DEV_CONF_CEDL;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp |= DEV_CONF_CEDL;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	}
}

void sfc_hold_invalid_value(struct jz_sfc_nand *sfc, int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp &= ~DEV_CONF_HOLDDL;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp |= DEV_CONF_HOLDDL;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	}
}

void sfc_wp_invalid_value(struct jz_sfc_nand *sfc, int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp &= ~DEV_CONF_WPDL;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp |= DEV_CONF_WPDL;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	}
}

void sfc_clear_end_intc(struct jz_sfc_nand *sfc)
{
	int tmp = 0;
	tmp = sfc_readl(sfc, SFC_SCR);
	tmp |= CLR_END;
	sfc_writel(sfc, SFC_SCR, tmp);
}

void sfc_clear_treq_intc(struct jz_sfc_nand *sfc)
{
	int tmp = 0;
	tmp = sfc_readl(sfc, SFC_SCR);
	tmp |= CLR_TREQ;
	sfc_writel(sfc, SFC_SCR, tmp);
}

void sfc_clear_rreq_intc(struct jz_sfc_nand *sfc)
{
	int tmp = 0;
	tmp = sfc_readl(sfc, SFC_SCR);
	tmp |= CLR_RREQ;
	sfc_writel(sfc, SFC_SCR, tmp);
}

void sfc_clear_over_intc(struct jz_sfc_nand *sfc)
{
	int tmp = 0;
	tmp = sfc_readl(sfc, SFC_SCR);
	tmp |= CLR_OVER;
	sfc_writel(sfc, SFC_SCR, tmp);
}

void sfc_clear_under_intc(struct jz_sfc_nand *sfc)
{
	int tmp = 0;
	tmp = sfc_readl(sfc, SFC_SCR);
	tmp |= CLR_UNDER;
	sfc_writel(sfc, SFC_SCR, tmp);
}

void sfc_clear_all_intc(struct jz_sfc_nand *sfc)
{
	sfc_writel(sfc, SFC_SCR, 0x1f);
}

void sfc_mask_all_intc(struct jz_sfc_nand *sfc)
{
	sfc_writel(sfc, SFC_INTC, 0x1f);
}

void sfc_mode(struct jz_sfc_nand *sfc, int channel, int value)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
	tmp &= ~(TRAN_CONF_TRAN_MODE_MSK << TRAN_CONF_TRAN_MODE_OFFSET);
	tmp |= (value << TRAN_CONF_TRAN_MODE_OFFSET);
	sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
}

void sfc_clock_phase(struct jz_sfc_nand *sfc, int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp &= ~DEV_CONF_CPHA;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp |= DEV_CONF_CPHA;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	}
}

void sfc_clock_polarity(struct jz_sfc_nand *sfc, int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp &= ~DEV_CONF_CPOL;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_DEV_CONF);
		tmp |= DEV_CONF_CPOL;
		sfc_writel(sfc, SFC_DEV_CONF, tmp);
	}
}

void sfc_threshold(struct jz_sfc_nand *sfc, int value)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_GLB);
	tmp &= ~GLB_THRESHOLD_MSK;
	tmp |= value << GLB_THRESHOLD_OFFSET;
	sfc_writel(sfc, SFC_GLB, tmp);
}


void sfc_smp_delay(struct jz_sfc_nand *sfc, int value)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_DEV_CONF);
	tmp &= ~DEV_CONF_SMP_DELAY_MSK;
	tmp |= value << DEV_CONF_SMP_DELAY_OFFSET | 1 << 9;
	sfc_writel(sfc, SFC_DEV_CONF, tmp);
}


void sfc_transfer_direction(struct jz_sfc_nand *sfc, int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_GLB);
		tmp &= ~GLB_TRAN_DIR;
		sfc_writel(sfc, SFC_GLB, tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_GLB);
		tmp |= GLB_TRAN_DIR;
		sfc_writel(sfc, SFC_GLB, tmp);
	}
}

void sfc_set_length(struct jz_sfc_nand *sfc, int value)
{
	sfc_writel(sfc, SFC_TRAN_LEN, value);
}

void sfc_transfer_mode(struct jz_sfc_nand *sfc, int value)
{
	if(value == 0) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_GLB);
		tmp &= ~GLB_OP_MODE;
		sfc_writel(sfc, SFC_GLB, tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_GLB);
		tmp |= GLB_OP_MODE;
		sfc_writel(sfc, SFC_GLB, tmp);
	}
}

void sfc_read_data(struct jz_sfc_nand *sfc, unsigned int *value)
{
	*value = sfc_readl(sfc, SFC_RM_DR);
}

void sfc_write_data(struct jz_sfc_nand *sfc, const unsigned int *value)
{
	sfc_writel(sfc, SFC_RM_DR, *value);
}

unsigned int sfc_fifo_num(struct jz_sfc_nand *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_SR);
	tmp &= (0x7f << 16);
	tmp = tmp >> 16;
	return tmp;
}

int ssi_underrun(struct jz_sfc_nand *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_SR);
	if(tmp & CLR_UNDER)
		return 1;
	else
		return 0;
}

int ssi_overrun(struct jz_sfc_nand *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_SR);
	if(tmp & CLR_OVER)
		return 1;
	else
		return 0;
}

int rxfifo_rreq(struct jz_sfc_nand *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_SR);
	if(tmp & CLR_RREQ)
		return 1;
	else
		return 0;
}
int txfifo_treq(struct jz_sfc_nand *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_SR);
	if(tmp & CLR_TREQ)
		return 1;
	else
		return 0;
}
int sfc_end(struct jz_sfc_nand *sfc)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_SR);
	if(tmp & CLR_END)
		return 1;
	else
		return 0;
}

void sfc_set_addr_length(struct jz_sfc_nand *sfc, int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
	tmp &= ~(ADDR_WIDTH_MSK);
	tmp |= (value << ADDR_WIDTH_OFFSET);
	sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
}

void sfc_cmd_en(struct jz_sfc_nand *sfc, int channel, unsigned int value)
{
	if(value == 1) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
		tmp |= TRAN_CONF_CMDEN;
		sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
		tmp &= ~TRAN_CONF_CMDEN;
		sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
	}
}

void sfc_data_en(struct jz_sfc_nand *sfc, int channel, unsigned int value)
{
	if(value == 1) {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
		tmp |= TRAN_CONF_DATEEN;
		sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
	} else {
		unsigned int tmp;
		tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
		tmp &= ~TRAN_CONF_DATEEN;
		sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
	}
}

void sfc_write_cmd(struct jz_sfc_nand *sfc, int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
	tmp &= ~TRAN_CONF_CMD_MSK;
	tmp |= value;
	sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
}

void sfc_dev_addr(struct jz_sfc_nand *sfc, int channel, unsigned int value)
{
	sfc_writel(sfc, SFC_DEV_ADDR(channel), value);
}


void sfc_dev_addr_dummy_bytes(struct jz_sfc_nand *sfc, int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
	tmp &= ~TRAN_CONF_DMYBITS_MSK;
	tmp |= value << DMYBITS_OFFSET;
	sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
}

void sfc_dev_addr_plus(struct jz_sfc_nand *sfc, int channel, unsigned int value)
{
	sfc_writel(sfc, SFC_DEV_ADDR_PLUS(channel), value);
}

void sfc_dev_pollen(struct jz_sfc_nand *sfc, int channel, unsigned int value)
{
	unsigned int tmp;
	tmp = sfc_readl(sfc, SFC_TRAN_CONF(channel));
	if(value == 1)
		tmp |= TRAN_CONF_POLLEN;
	else
		tmp &= ~(TRAN_CONF_POLLEN);

	sfc_writel(sfc, SFC_TRAN_CONF(channel), tmp);
}

void sfc_dev_sta_exp(struct jz_sfc_nand *sfc, unsigned int value)
{
	sfc_writel(sfc, SFC_DEV_STA_EXP, value);
}

void sfc_dev_sta_msk(struct jz_sfc_nand *sfc, unsigned int value)
{
	sfc_writel(sfc, SFC_DEV_STA_MSK, value);
}

void sfc_enable_all_intc(struct jz_sfc_nand *sfc)
{
	sfc_writel(sfc, SFC_INTC, 0);
}

void sfc_set_mem_addr(struct jz_sfc_nand *sfc,unsigned int addr )
{
	sfc_writel(sfc, SFC_MEM_ADDR, addr);
}
void dump_sfc_reg(struct jz_sfc_nand *sfc)
{
	int i = 0;
	printk("SFC_GLB			:%08x\n", sfc_readl(sfc, SFC_GLB ));
	printk("SFC_DEV_CONF	:%08x\n", sfc_readl(sfc, SFC_DEV_CONF ));
	printk("SFC_DEV_STA_EXP	:%08x\n", sfc_readl(sfc, SFC_DEV_STA_EXP));
	printk("SFC_DEV_STA_RT	:%08x\n", sfc_readl(sfc, SFC_DEV_STA_RT ));
	printk("SFC_DEV_STA_MSK	:%08x\n", sfc_readl(sfc, SFC_DEV_STA_MSK ));
	printk("SFC_TRAN_LEN		:%08x\n", sfc_readl(sfc, SFC_TRAN_LEN ));

	for(i = 0; i < 6; i++)
		printk("SFC_TRAN_CONF(%d)	:%08x\n", i,sfc_readl(sfc, SFC_TRAN_CONF(i)));

	for(i = 0; i < 6; i++)
		printk("SFC_DEV_ADDR(%d)	:%08x\n", i,sfc_readl(sfc, SFC_DEV_ADDR(i)));

	printk("SFC_MEM_ADDR :%08x\n", sfc_readl(sfc, SFC_MEM_ADDR ));
	printk("SFC_TRIG	 :%08x\n", sfc_readl(sfc, SFC_TRIG));
	printk("SFC_SR		 :%08x\n", sfc_readl(sfc, SFC_SR));
	printk("SFC_SCR		 :%08x\n", sfc_readl(sfc, SFC_SCR));
	printk("SFC_INTC	 :%08x\n", sfc_readl(sfc, SFC_INTC));
	printk("SFC_FSM		 :%08x\n", sfc_readl(sfc, SFC_FSM ));
	printk("SFC_CGE		 :%08x\n", sfc_readl(sfc, SFC_CGE ));
//	printk("SFC_RM_DR 	 :%08x\n", sfc_readl(spi, SFC_RM_DR));
}
#endif
