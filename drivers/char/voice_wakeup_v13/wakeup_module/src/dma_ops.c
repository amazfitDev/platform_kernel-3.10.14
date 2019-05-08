#include <common.h>
#include <jz_cpm.h>
#include "jz_dma.h"
#include "jz_dmic.h"
#include "dmic_config.h"
#include "interface.h"
#include "dma_ops.h"

int *src_buf;
int *dst_buf;

struct dma_config config;
struct dma_desc *desc;  /* pointer to desc in tcsm */
struct dma_desc sleep_desc[NR_BUFFERS]; /* pointer to desc in ddr, when suspend to deep sleep */

int dma_channel = 5; /* default channel 5, but this should be set by kernel */

void build_circ_descs(struct dma_desc *desc)
{
	int i;
	struct dma_desc *cur;
	struct dma_desc *next;
	unsigned int off_base,count;

	off_base=VOICE_TCSM_DATA_BUF_SIZE/NR_DESC/sizeof(dst_buf);
	count = VOICE_TCSM_DATA_BUF_SIZE/NR_DESC/config.burst_len;
	for(i=0; i< NR_DESC; i++) {
		config.src = V_TO_P(src_buf);
		config.dst = V_TO_P(dst_buf + (off_base * i));
		config.count = count; /*count of data unit*/
		config.link = 1;
		cur = &desc[i];
		if(i == NR_DESC -1) {
			next = &desc[0];
		} else {
			next = &desc[i+1];
		}
		if(0) {
		printk("cur_desc:%x, next_desc:%x\n", cur, next);
		}
		build_one_desc(&config, cur, next);
	}

}
#ifdef DMA_VOICE_DEBUG
void dump_descs(struct dma_desc *desc)
{
	int i;
	struct dma_desc *temp;
	struct dma_desc *temp_phy;
	for(i=0; i<NR_DESC; i++) {
		temp = &desc[i];
		temp_phy = (struct dma_desc *)((unsigned int)temp | 0xA0000000);

		printk("desc[%d].dcm:%08x:dcm:%08x\n",i,temp->dcm, temp_phy->dcm);
		printk("desc[%d].dsa:%08x:dsa:%08x\n",i,temp->dsa, temp_phy->dsa);
		printk("desc[%d].dta:%08x:dta:%08x\n",i,temp->dta, temp_phy->dta);
		printk("desc[%d].dtc:%08x:dtc:%08x\n",i,temp->dtc, temp_phy->dtc);
		printk("desc[%d].sd:%08x:sd:%08x\n",i,temp->sd, temp_phy->sd);
		printk("desc[%d].drt:%08x:drt:%08x\n",i,temp->drt, temp_phy->drt);
		printk("desc[%d].reserved[0]:%08x:reserved[0]:%08x\n",i,temp->reserved[0], temp_phy->reserved[0]);
		printk("desc[%d].reserved[1]:%08x:reserved[1]:%08x\n",i,temp->reserved[1], temp_phy->reserved[1]);

	}

}
void dump_tcsm()
{
	int i;
	unsigned int *t = (unsigned int *)VOICE_TCSM_DATA_BUF;
	for(i = 0; i< 256; i++) {
		printk("t[%d]:%x\n", i, *(t+i));
	}
}

void dump_dma_register(int chn)
{
	printk("=============== dma dump register ===============\n");
	printk("DMAC   : 0x%08X\n", REG_DMADMAC);
	printk("DIRQP  : 0x%08X\n", REG_DMADIRQP);
	printk("DDB    : 0x%08X\n", REG_DMADDB);
	printk("DDS    : 0x%08X\n", REG_DMADDS);
	printk("DMACP  : 0x%08X\n", REG_DMADMACP);
	printk("DSIRQP : 0x%08X\n", REG_DMADSIRQP);
	printk("DSIRQM : 0x%08X\n", REG_DMADSIRQM);
	printk("DCIRQP : 0x%08X\n", REG_DMADCIRQP);
	printk("DCIRQM : 0x%08X\n", REG_DMADCIRQM);
	printk("DMCS   : 0x%08X\n", REG_DMADMCS);
	printk("DMNMB  : 0x%08X\n", REG_DMADMNMB);
	printk("DMSMB  : 0x%08X\n", REG_DMADMSMB);
	printk("DMINT  : 0x%08X\n", REG_DMADMINT);

	printk("* DMAC.FMSC = 0x%d\n", DMA_READBIT(REG_DMADMAC, 31, 1));
	printk("* DMAC.FSSI = 0x%d\n", DMA_READBIT(REG_DMADMAC, 30, 1));
	printk("* DMAC.FTSSI= 0x%d\n", DMA_READBIT(REG_DMADMAC, 29, 1));
	printk("* DMAC.FUART= 0x%d\n", DMA_READBIT(REG_DMADMAC, 28, 1));
	printk("* DMAC.FAIC = 0x%d\n", DMA_READBIT(REG_DMADMAC, 27, 1));
	printk("* DMAC.INTCC= 0x%d\n", DMA_READBIT(REG_DMADMAC, 17, 5));
	printk("* DMAC.INTCE= 0x%d\n", DMA_READBIT(REG_DMADMAC, 16, 1));
	printk("* DMAC.HLT  = 0x%d\n", DMA_READBIT(REG_DMADMAC,  3, 1));
	printk("* DMAC.AR   = 0x%d\n", DMA_READBIT(REG_DMADMAC,  2, 1));
	printk("* DMAC.CH01 = 0x%d\n", DMA_READBIT(REG_DMADMAC,  1, 1));
	printk("* DMAC.DMAE = 0x%d\n", DMA_READBIT(REG_DMADMAC,  0, 1));

	if(chn>=0 && chn<=31){
		printk("");
		printk("DSA(%02d) : 0x%08X\n", chn, REG_DMADSA(chn));
		printk("DTA(%02d) : 0x%08X\n", chn, REG_DMADTA(chn));
		printk("DTC(%02d) : 0x%08X\n", chn, REG_DMADTC(chn));
		printk("DRT(%02d) : 0x%08X\n", chn, REG_DMADRT(chn));
		printk("DCS(%02d) : 0x%08X\n", chn, REG_DMADCS(chn));
		printk("DCM(%02d) : 0x%08X\n", chn, REG_DMADCM(chn));
		printk("DDA(%02d) : 0x%08X\n", chn, REG_DMADDA(chn));
		printk("DSD(%02d) : 0x%08X\n", chn, REG_DMADSD(chn));

		printk("* DCS(%02d).NDES = 0x%d\n", chn, DMA_READBIT(REG_DMADCS(chn), 31, 1));
		printk("* DCS(%02d).DES8 = 0x%d\n", chn, DMA_READBIT(REG_DMADCS(chn), 30, 1));
		printk("* DCS(%02d).CDOA = 0x%d\n", chn, DMA_READBIT(REG_DMADCS(chn), 8, 8));
		printk("* DCS(%02d).AR   = 0x%d\n", chn, DMA_READBIT(REG_DMADCS(chn), 4, 1));
		printk("* DCS(%02d).TT   = 0x%d\n", chn, DMA_READBIT(REG_DMADCS(chn), 3, 1));
		printk("* DCS(%02d).HLT  = 0x%d\n", chn, DMA_READBIT(REG_DMADCS(chn), 2, 1));
		printk("* DCS(%02d).CTE  = 0x%d\n", chn, DMA_READBIT(REG_DMADCS(chn), 0, 1));

		printk("* DCM(%02d).SDI  = 0x%d\n", chn, DMA_READBIT(REG_DMADCM(chn), 26, 2));
		printk("* DCM(%02d).DID  = 0x%d\n", chn, DMA_READBIT(REG_DMADCM(chn), 24, 2));
		printk("* DCM(%02d).SAI  = 0x%d\n", chn, DMA_READBIT(REG_DMADCM(chn), 23, 1));
		printk("* DCM(%02d).DAI  = 0x%d\n", chn, DMA_READBIT(REG_DMADCM(chn), 22, 1));
		printk("* DCM(%02d).RDIL = 0x%d\n", chn, DMA_READBIT(REG_DMADCM(chn), 16, 4));
		printk("* DCM(%02d).SP   = 0x%d\n", chn, DMA_READBIT(REG_DMADCM(chn), 14, 2));
		printk("* DCM(%02d).DP   = 0x%d\n", chn, DMA_READBIT(REG_DMADCM(chn), 12, 2));
		printk("* DCM(%02d).TSZ  = 0x%d\n", chn, DMA_READBIT(REG_DMADCM(chn),  8, 3));
		printk("* DCM(%02d).STDE = 0x%d\n", chn, DMA_READBIT(REG_DMADCM(chn),  2, 1));
		printk("* DCM(%02d).TIE  = 0x%d\n", chn, DMA_READBIT(REG_DMADCM(chn),  1, 1));
		printk("* DCM(%02d).LINK = 0x%d\n", chn, DMA_READBIT(REG_DMADCM(chn),  0, 1));

		printk("* DDA(%02d).DBA = 0x%08X\n", chn, DMA_READBIT(REG_DMADDA(chn),  12, 20));
		printk("* DDA(%02d).DOA = 0x%08X\n", chn, DMA_READBIT(REG_DMADDA(chn),  4, 8));

		printk("* DSD(%02d).TSD = 0x%08X\n", chn, DMA_READBIT(REG_DMADSD(chn),  16, 16));
		printk("* DSD(%02d).SSD = 0x%08X\n", chn, DMA_READBIT(REG_DMADSD(chn),   0, 16));
	}
	printk("INT_ICSR0:0x%08X\n",*(int volatile*)0xB0001000);
	printk("\n");
}
#endif
void dma_set_channel(int channel)
{
	dma_channel = channel;
}
void dma_config_normal(void)
{
	REG32(CPM_IOBASE + CPM_CLKGR0) &= ~(1 << 21);

	desc = (struct dma_desc *)(DMA_DESC_ADDR);
	src_buf = (int *)DMIC_RX_FIFO;
	dst_buf = (int *)VOICE_TCSM_DATA_BUF;

	config.type = DMIC_REQ_TYPE; /* dmic reveive request */
	config.channel = dma_channel;
	config.increment = 1; /*src no inc, dst inc*/
	config.rdil = 64;
	config.sp_dp = 0x0; /*32bit*/
	config.sp_dp = 0xa; /*32bit*/
	config.stde = 0;
	config.descriptor = 1;
	config.des8 = 1;
	config.sd = 0;
	config.tsz	 = 6;
	config.burst_len = 128;

	config.desc = V_TO_P(desc);
//	config.tie = 1;
	build_circ_descs(desc);
	pdma_config(&config);
#ifdef DMA_VOICE_DEBUG
	dump_descs(desc);
#endif

}

void dma_config_early_sleep(struct sleep_buffer *sleep_buffer)
{
	int i;
	struct dma_desc *cur;
	struct dma_desc *next;

	for(i = 0; i<sleep_buffer->nr_buffers; i++) {

		config.src = V_TO_P(src_buf);
		config.dst = V_TO_P(sleep_buffer->buffer[i]);
		config.count = sleep_buffer->total_len/sleep_buffer->nr_buffers/config.burst_len; /*count of data unit*/
		config.link = 1;
		cur = &sleep_desc[i];
		if(i == sleep_buffer->nr_buffers -1) {
			next = &sleep_desc[0];
		} else {
			next = &sleep_desc[i+1];
		}
		if(0) {
			printk("cur_desc:%x, next_desc:%x\n", cur, next);
		}
		build_one_desc(&config, cur, next);
	}
	/* now sleep_desc stores the desc , we make a new pdma_config, other configs remains
	 * what it is when open dma.
	 * */
	pdma_config(&config);
}

void dma_close(void)
{
	/*disable dma*/
	//pdma_end(DMA_CHANNEL);
	pdma_end(dma_channel);
#ifdef DMA_VOICE_DEBUG
	dump_dma_register(DMA_CHANNEL);
#endif
}

void dma_start(int channel)
{
	pdma_start(channel);
}
void dma_stop(int channel)
{
	pdma_end(channel);
}
