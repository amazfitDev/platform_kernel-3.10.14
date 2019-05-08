/*
 * JZSOC DMA controller
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 */

#undef DEBUG
#define VERBOSE_DEBUG

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <linux/stat.h>

#include <soc/irq.h>
#include <soc/base.h>
#include <soc/gpio.h>

#include <mach/jzdma_v13.h>


/* tsz for 1,2,4,8,16,32,64 bytes */
const static char dcm_tsz[7] = { 1, 2, 0, 0, 3, 4, 5 };

static inline unsigned int get_current_tsz(unsigned long dcmp)
{
	int i;
	int val = (dcmp & DCM_TSZ_MSK) >> DCM_TSZ_SHF;
	for (i = 0; i < sizeof(dcm_tsz) / sizeof(dcm_tsz[0]); i++) {
		if (val == dcm_tsz[i])
			break;
	}
	return (1 << i);
}
static inline unsigned get_max_tsz(unsigned long val, unsigned long *dcmp)
{

	int ord;

	ord = ffs(val) - 1;
	if (ord < 0)
		ord = 0;
	else if (ord > 6)
		ord = 6;

	*dcmp &= ~DCM_TSZ_MSK;
	*dcmp |= dcm_tsz[ord] << DCM_TSZ_SHF;

	/* if tsz == 8, set it to 4 */
	return ord == 3 ? 4 : 1 << ord;
}

#if 1
static void dump_dma_desc(struct jzdma_channel *dmac)
{
	struct dma_desc *desc = dmac->desc;
	int i;

	dev_info(chan2dev(&dmac->chan), "dmac->desc_nr = %d\n", dmac->desc_nr);
	for (i = 0; i < dmac->desc_nr; i++)
		dev_info(chan2dev(&dmac->chan),
			 "DSA: %x, DTA: %x, DCM: %lx, DTC:%lx DRT:%lx\n",
			 desc[i].dsa, desc[i].dta, desc[i].dcm, desc[i].dtc,
			 desc[i].drt);

	dev_info(chan2dev(&dmac->chan), "CH_DSA = 0x%08x\n",
		 readl(dmac->iomem + CH_DSA));
	dev_info(chan2dev(&dmac->chan), "CH_DTA = 0x%08x\n",
		 readl(dmac->iomem + CH_DTA));
	dev_info(chan2dev(&dmac->chan), "CH_DTC = 0x%08x\n",
		 readl(dmac->iomem + CH_DTC));
	dev_info(chan2dev(&dmac->chan), "CH_DRT = 0x%08x\n",
		 readl(dmac->iomem + CH_DRT));
	dev_info(chan2dev(&dmac->chan), "CH_DCS = 0x%08x\n",
		 readl(dmac->iomem + CH_DCS));
	dev_info(chan2dev(&dmac->chan), "CH_DCM = 0x%08x\n",
		 readl(dmac->iomem + CH_DCM));
	dev_info(chan2dev(&dmac->chan), "CH_DDA = 0x%08x\n",
		 readl(dmac->iomem + CH_DDA));
	dev_info(chan2dev(&dmac->chan), "CH_DSD = 0x%08x\n",
		 readl(dmac->iomem + CH_DSD));
}

static void dump_dma(struct jzdma_master *master)
{
	dev_info(master->dev, "DMAC   = 0x%08x\n", readl(master->iomem + DMAC));
	dev_info(master->dev, "DIRQP  = 0x%08x\n",
		 readl(master->iomem + DIRQP));
	dev_info(master->dev, "DDR    = 0x%08x\n", readl(master->iomem + DDR));
	dev_info(master->dev, "DDRS   = 0x%08x\n", readl(master->iomem + DDRS));
	dev_info(master->dev, "DMACP  = 0x%08x\n",
		 readl(master->iomem + DMACP));
	dev_info(master->dev, "DSIRQP = 0x%08x\n",
		 readl(master->iomem + DSIRQP));
	dev_info(master->dev, "DSIRQM = 0x%08x\n",
		 readl(master->iomem + DSIRQM));
	dev_info(master->dev, "DCIRQP = 0x%08x\n",
		 readl(master->iomem + DCIRQP));
	dev_info(master->dev, "DCIRQM = 0x%08x\n",
		 readl(master->iomem + DCIRQM));
}
void jzdma_dump(struct dma_chan *chan)
{
	struct jzdma_channel *dmac = to_jzdma_chan(chan);
	dump_dma_desc(dmac);
	dump_dma(dmac->master);
}
EXPORT_SYMBOL_GPL(jzdma_dump);
#else
void jzdma_dump(struct dma_chan *chan) {return 0;}
EXPORT_SYMBOL_GPL(jzdma_dump);
#define dump_dma_desc(A) (void)(0)
#define dump_dma(A) (void)(0)
#endif

static int build_one_desc(struct jzdma_channel *dmac, dma_addr_t src,
			  dma_addr_t dst, unsigned long dcm, unsigned cnt,
			  enum jzdma_type type)
{
	struct dma_desc *desc = dmac->desc + dmac->desc_nr;
	if (dmac->desc_nr >= dmac->desc_max)
		return -1;

	desc->dsa = src;
	desc->dta = dst;
	desc->dcm = dcm;
	desc->drt = type;
	desc->dtc = (((unsigned int)(desc + 1) >> 4) << 24) + cnt;
	dmac->desc_nr++;

	return 0;
}

static int build_one_dymic_desc(struct jzdma_channel *dmac, dma_addr_t src,
				dma_addr_t dst, unsigned long dcm, unsigned cnt,
				enum jzdma_type type, int flag,
				enum dma_transfer_direction direction)
{
	struct dma_desc *desc = dmac->desc + (dmac->desc_nr % dmac->desc_max);
	unsigned dma_addr = 0;
	unsigned dma_addr_t = 0;
	unsigned addr_mask = ~(flag >> 2);

	if (flag & 0x2) {
		if (dmac->desc_nr > 2) {
			struct dma_desc *desc_pre0 =
			    dmac->desc + (dmac->desc_nr - 1) % dmac->desc_max;
			struct dma_desc *desc_pre1 =
			    dmac->desc + (dmac->desc_nr - 2) % dmac->desc_max;
			if (direction ==  DMA_MEM_TO_DEV) {
				dma_addr_t = readl(dmac->iomem + CH_DSA);
				dma_addr = (dma_addr_t & addr_mask);
				if ((desc_pre0->dsa & addr_mask) == dma_addr ||
				    (desc_pre1->dsa & addr_mask) == dma_addr) {
					return -1;
				}

			} else {
				dma_addr =
				    (readl(dmac->iomem + CH_DTA) & addr_mask);
				if ((desc_pre0->dta & addr_mask) == dma_addr
				    || (desc_pre1->dta & addr_mask) == dma_addr)
					return -1;
			}
			desc_pre0->dcm |= DCM_LINK;
			dcm &= ~DCM_LINK;
		} else
			return -1;
	} else if (dmac->desc_nr >= dmac->desc_max) {
		return -1;
	}


	desc->dsa = src;
	desc->dta = dst;
	desc->dcm = dcm;
	desc->drt = type;

	if (!((dmac->desc_nr + 1) % (dmac->desc_max)))
		desc->dtc = (((unsigned int)(dmac->desc) >> 4) << 24) + cnt;
	else
		desc->dtc = (((unsigned int)(desc + 1) >> 4) << 24) + cnt;

	dmac->desc_nr++;

	return 0;
}

static int build_desc_from_sg(struct jzdma_channel *dmac,
			      struct scatterlist *sgl, unsigned int sg_len,
			      enum dma_transfer_direction direction)
{
	struct dma_slave_config *config = dmac->config;
	struct scatterlist *sg;
	unsigned long dcm;
	unsigned int i,tsz;

	if (direction ==  DMA_MEM_TO_DEV)
		dcm = DCM_SAI | dmac->tx_dcm_def;
	else
		dcm = DCM_DAI | dmac->rx_dcm_def;

	/* clear LINK bit when issue pending */
	dcm |= DCM_TIE;

	if (dmac->id != 1)
		dcm |= DCM_LINK;

	for_each_sg(sgl, sg, sg_len, i) {
		dma_addr_t mem;

		mem = sg_dma_address(sg);

		if (dmac->id == 1) {
			/*
			 * for special channel1
			 * tsz = 7 (auto)
			 */
			dcm |= 7 << 8;
			tsz = sg_dma_len(sg);
		} else {
			tsz =
			    get_max_tsz(mem | sg_dma_len(sg) | config->
					dst_maxburst, &dcm);
			tsz = sg_dma_len(sg) / tsz;
		}

		if (direction ==  DMA_MEM_TO_DEV) {
			build_one_desc(dmac, mem, config->dst_addr, dcm, tsz,
				       dmac->type);
		} else {
			build_one_desc(dmac, config->src_addr, mem, dcm, tsz,
				       dmac->type + 1);
		}
	}

	return i;
}

static void jzdma_mcu_reset(struct jzdma_master *dma)
{
	unsigned long dmcs;
	dmcs = readl(dma->iomem + DMCS);
	dmcs |= 0x1;
	writel(dmcs, dma->iomem + DMCS);
}



/*
 *	get dma current transfer address
 */
static dma_addr_t jzdma_get_current_trans_addr(struct dma_chan *chan,
					       dma_addr_t * dst_addr,
					       dma_addr_t * src_addr,
					       enum dma_transfer_direction
					       direction)
{
	struct jzdma_channel *dmac = to_jzdma_chan(chan);
	dma_addr_t ret_val = 0;

	if (dmac->status == STAT_STOPED || dmac->status == STAT_PREPED)
		return 0;

	if (direction ==  DMA_MEM_TO_DEV) {
		ret_val	= readl(dmac->iomem + CH_DSA);
		if (src_addr)
			*src_addr = ret_val;
		if (dst_addr)
			*dst_addr = readl(dmac->iomem + CH_DTA);
	} else if (direction == DMA_DEV_TO_MEM) {
		ret_val = readl(dmac->iomem + CH_DTA);
		if (dst_addr)
			*dst_addr = ret_val;
		if (src_addr)
			*src_addr = readl(dmac->iomem + CH_DSA);
	} else if (direction == DMA_MEM_TO_MEM) {
		if (dst_addr)
			*dst_addr = readl(dmac->iomem + CH_DTA);
		if (src_addr)
			*src_addr = readl(dmac->iomem + CH_DSA);
	}

	return ret_val;
}

/*
 *	!!(flag&0x1) == 1 tx DCM_SAI
 *	!!(flag&0x1) == 0 tx DCM_SAI DCM_DAI
 *	!!(flag&0x2) == 1 dymic add desc
 *						try to add desc when dma is running or dma is suspend,
 *						at this time function  will retrun	NULL,when
 *						success.
 *	NOTE:	When we used dymic add desc
 *			return NULL for succses or dmac->tx_desc to failed ,
 *			the client cannot submit function
 */
static struct dma_async_tx_descriptor *jzdma_add_desc(struct dma_chan *chan,
						      dma_addr_t src,
						      dma_addr_t dst,
						      unsigned cnt,
						      enum dma_transfer_direction
						      direction, int flag)
{
	unsigned long tsz, dcm = 0, type = 0;
	struct jzdma_channel *dmac = to_jzdma_chan(chan);

	if (!(flag & 0x2)
	    && !(dmac->status == STAT_STOPED || dmac->status == STAT_PREPED)) {
		return NULL;
	}

	if ((flag & 0x2)
	    && (dmac->status == STAT_STOPED || dmac->status == STAT_PREPED)) {
		return &dmac->tx_desc;
	}

	dev_vdbg(chan2dev(chan), "Channel %d add desc\n", dmac->chan.chan_id);

	if (direction ==  DMA_MEM_TO_DEV) {
		if (flag & 0x2)
			tsz =
			    get_max_tsz(src | cnt | dmac->config->dst_maxburst,
					&dcm);
		else
			tsz = get_max_tsz(dmac->config->dst_maxburst, &dcm);

		if (flag & 0x1) {
			dcm |= DCM_SAI | dmac->tx_dcm_def | DCM_LINK | DCM_TIE;
		} else if (!(flag & 0x1)) {
			dcm |=
			    DCM_SAI | DCM_DAI | dmac->
			    tx_dcm_def | DCM_LINK | DCM_TIE;
		}
		type = dmac->type;
	} else {
		if (flag & 0x2)
			tsz =
			    get_max_tsz(dst | cnt | dmac->config->src_maxburst,
					&dcm);
		else
			tsz = get_max_tsz(dmac->config->src_maxburst, &dcm);
		dcm |= DCM_DAI | dmac->rx_dcm_def | DCM_TIE | DCM_LINK;
		type = dmac->type + 1;
	}

	if (flag & 0x2) {
		if (build_one_dymic_desc
		    (dmac, src, dst, dcm, cnt / tsz, type, flag, direction)) {
			return &dmac->tx_desc;
		}
	} else {
		build_one_desc(dmac, src, dst, dcm, cnt, type);
	}

	BUG_ON(!(dmac->flags & CHFLG_SLAVE));

	if (dmac->status == STAT_STOPED || dmac->status == STAT_PREPED) {
		if (!(flag & 0x2)) {
			/* tx descriptor shouldn't reused before dma finished. */
			dmac->tx_desc.flags |= DMA_CTRL_ACK;
			/* use 8-word descriptors */
			writel(1 << 30, dmac->iomem + CH_DCS);
			dmac->cyclic = 0;
			dmac->status = STAT_PREPED;
		}
		return &dmac->tx_desc;
	}

	return NULL;
}

static struct dma_async_tx_descriptor *jzdma_prep_slave_sg(struct dma_chan
							   *chan,
							   struct scatterlist
							   *sgl,
							   unsigned int sg_len,
							   enum
							   dma_transfer_direction
							   direction,
							   unsigned long flags,
							   void *context)
{
	struct jzdma_channel *dmac = to_jzdma_chan(chan);

	if (dmac->status != STAT_STOPED)
		return NULL;

	dev_vdbg(chan2dev(chan), "Channel %d prepare slave sg list\n",
		 dmac->chan.chan_id);

	BUG_ON(!(dmac->flags & CHFLG_SLAVE));

	dmac->last_sg = 0;
	dmac->sgl = sgl;
	dmac->sg_len = sg_len;
	if (sg_len > dmac->desc_max) {
		sg_len = dmac->desc_max;
		dmac->last_sg = sg_len;
	}
	build_desc_from_sg(dmac, sgl, sg_len, direction);

	/* use 8-word descriptors */
	writel(1 << 30, dmac->iomem + CH_DCS);

	/*
	 * special channel1 can not use descriptor mode
	 */
	if (dmac->id == 1) {
		writel(dmac->desc->dcm, dmac->iomem + CH_DCM);
		writel(JZDMA_REQ_AUTO_TXRX, dmac->iomem + CH_DRT);
		writel(dmac->desc->dsa, dmac->iomem + CH_DSA);
		writel(dmac->desc->dta, dmac->iomem + CH_DTA);
		writel(dmac->desc->dtc, dmac->iomem + CH_DTC);
		writel(dmac->desc->sd, dmac->iomem + CH_DSD);
		writel(dmac->desc->dcm, dmac->iomem + CH_DCM);
	}

	/* tx descriptor shouldn't reused before dma finished. */
	dmac->tx_desc.flags |= DMA_CTRL_ACK;
	dmac->cyclic = 0;
	dmac->status = STAT_PREPED;
	return &dmac->tx_desc;
}

static struct dma_async_tx_descriptor *jzdma_prep_memcpy(struct dma_chan *chan,
							 dma_addr_t dma_dest,
							 dma_addr_t dma_src,
							 size_t len,
							 unsigned long flags)
{
	struct jzdma_channel *dmac = to_jzdma_chan(chan);
	unsigned long tsz,dcm = 0;

	if (!(dmac->status == STAT_STOPED || dmac->status == STAT_PREPED))
		return NULL;

	tsz = get_max_tsz(dma_dest | dma_src | len, &dcm);

	dev_vdbg(chan2dev(chan),
		 "Channel %d prepare memcpy d:%x s:%x l:%d %lx %lx\n",
		 dmac->chan.chan_id, dma_dest, dma_src, len, tsz, dcm);

	dcm |= DCM_TIE | DCM_DAI | DCM_SAI | DCM_LINK;
	build_one_desc(dmac, dma_src, dma_dest, dcm, len / tsz, JZDMA_REQ_AUTO);

	/* use 8-word descriptors */
	writel(1 << 30, dmac->iomem + CH_DCS);

	/* tx descriptor can reused before dma finished. */
	dmac->tx_desc.flags &= ~DMA_CTRL_ACK;
	dmac->cyclic = 0;
	dmac->status = STAT_PREPED;
	return &dmac->tx_desc;
}

static struct dma_async_tx_descriptor *jzdma_prep_dma_cyclic(struct dma_chan
							     *chan,
							     dma_addr_t
							     dma_addr,
							     size_t buf_len,
							     size_t period_len,
							     enum
							     dma_transfer_direction
							     direction,
							     unsigned long flags,
							     void *context)
{
	int i = 0;
	unsigned long tsz = 0;
	unsigned long dcm = 0;
	unsigned long cnt = 0;
	unsigned int periods = buf_len / period_len;
	struct jzdma_channel *dmac = to_jzdma_chan(chan);
	struct dma_slave_config *config = dmac->config;
	struct dma_desc *desc = dmac->desc;

	dmac->desc_nr = 0;

	if (direction ==  DMA_MEM_TO_DEV)
		dcm |= DCM_SAI | dmac->tx_dcm_def;
	else
		dcm |= DCM_DAI | dmac->rx_dcm_def;

	/* clear LINK bit when issue pending */
	dcm |= DCM_TIE | DCM_LINK;

	/* the last periods will not be used,
	   |>-------------------------------------->|
	   | desc[0]->desc[1]->...->desc[periods-1] |->desc[periods]
	   |<--------------------------------------<|
	   desc[periods-1] DOA point to desc[0], so it will
	   be a cycle, and desc[periods] will not be used,
	   in that case, the last desc->dcm set ~DCM_LINK
	   when call jzdma_issue_pending(); will not effect
	   the real desc we need to use (desc[0 ~ (periods-1)])
	 */
	if (periods >= dmac->desc_max)
		return NULL;

	for (i = 0; i <= periods; i++) {
		/* get desc address */
		desc = dmac->desc + dmac->desc_nr;
		/* computer tsz */
		if (direction == DMA_MEM_TO_DEV) {
			tsz =
			    get_max_tsz(dma_addr | period_len | config->
					dst_maxburst, &dcm);
			cnt = period_len / tsz;
			desc->dsa = dma_addr;
			desc->dta = config->dst_addr;
			desc->drt = dmac->type;
		} else {
			tsz =
			    get_max_tsz(dma_addr | period_len | config->
					src_maxburst, &dcm);
			cnt = period_len / tsz;
			desc->dsa = config->src_addr;
			desc->dta = dma_addr;
			desc->drt = dmac->type + 1;
		}
		/* set dcm */
		desc->dcm = dcm;
		/* set the last desc point to the first one */
		if (i == (periods - 1))
			desc->dtc = (((unsigned int)(dmac->desc + 0) >> 4) << 24) + cnt;
		else
			desc->dtc = (((unsigned int)(dmac->desc + i + 1) >> 4) << 24) + cnt;
		/* update dma_addr and desc_nr */
		dma_addr += period_len;
		dmac->desc_nr++;
	}

	/* use 8-word descriptors */
	writel(1 << 30, dmac->iomem + CH_DCS);
	dev_dbg(chan2dev(&dmac->chan),"CH_DCS = %x\n", readl(dmac->iomem + CH_DCS));

	/* tx descriptor can reused before dma finished. */
	dmac->tx_desc.flags &= ~DMA_CTRL_ACK;

	/* this is cyclic, may need to fake it (see jzdma_issue_pending) */
	dmac->cyclic = CYCLIC_POSSIBLE;
	dmac->status = STAT_PREPED;

	return &dmac->tx_desc;
}


static dma_cookie_t jzdma_tx_submit(struct dma_async_tx_descriptor *tx)
{
	struct jzdma_channel *dmac = to_jzdma_chan(tx->chan);
	dma_cookie_t cookie = dmac->chan.cookie;
	unsigned long flags;

	if (dmac->status != STAT_PREPED)
		return -EINVAL;

	spin_lock_irqsave(&dmac->lock, flags);

	if (++cookie < 0)
		cookie = 1;
	dmac->chan.cookie = cookie;
	dmac->tx_desc.cookie = cookie;
	dmac->status = STAT_SUBED;
	dmac->residue = -1;

	spin_unlock_irqrestore(&dmac->lock, flags);

	dev_vdbg(chan2dev(&dmac->chan),"Channel %d submit\n",dmac->chan.chan_id);

	return cookie;
}

static enum dma_status jzdma_tx_status(struct dma_chan *chan,
				       dma_cookie_t cookie,
				       struct dma_tx_state *txstate)
{
	struct jzdma_channel *dmac = to_jzdma_chan(chan);
	dma_cookie_t last_used;
	enum dma_status ret;
	int residue;

	last_used = chan->cookie;

	ret = dma_async_is_complete(cookie, dmac->last_completed, last_used);
	if (dmac->residue == -1) {
		residue =
		    readl(dmac->iomem +
			  CH_DTC) * get_current_tsz(readl(dmac->iomem +
							  CH_DCM));
		dma_set_tx_state(txstate, dmac->last_completed, last_used,
				 residue);
	} else {
		dma_set_tx_state(txstate, dmac->last_completed, last_used,
				 dmac->residue);
	}

	if (ret == DMA_SUCCESS
	    && dmac->last_completed != dmac->last_good
	    && (dma_async_is_complete(cookie, dmac->last_good, last_used)
		== DMA_IN_PROGRESS))
		ret = DMA_ERROR;

	return ret;
}

static void jzdma_chan_tasklet(unsigned long data)
{
	struct jzdma_channel *dmac = (struct jzdma_channel *)data;
	unsigned long dcs;

	dcs = dmac->dcs_saved;

	dev_vdbg(chan2dev(&dmac->chan), "tasklet: DCS%d=%lx\n",
		 dmac->chan.chan_id, dcs);

	if (dcs & DCS_AR)
		dev_notice(chan2dev(&dmac->chan), "Addr Error: DCS%d=%lx\n",
			   dmac->chan.chan_id, dcs);
	if (dcs & DCS_HLT)
		dev_notice(chan2dev(&dmac->chan), "DMA Halt: DCS%d=%lx\n",
			   dmac->chan.chan_id, dcs);

	spin_lock(&dmac->lock);
	if (dcs & DCS_TT)
		dmac->last_good = dmac->tx_desc.cookie;

	if (!(dmac->cyclic & CYCLIC_ACTIVE)) {
		dmac->status = STAT_STOPED;
		dmac->desc_nr = 0;
	}

	spin_unlock(&dmac->lock);

	if (dmac->tx_desc.callback)
		dmac->tx_desc.callback(dmac->tx_desc.callback_param);
}

static void jzdma_issue_pending(struct dma_chan *chan)
{
	struct jzdma_channel *dmac = to_jzdma_chan(chan);
	struct dma_desc *desc = dmac->desc + dmac->desc_nr - 1;
	if (dmac->status != STAT_SUBED)
		return;

	dmac->status = STAT_RUNNING;

	dev_vdbg(chan2dev(chan),
			"Channel %d issue pending\n",dmac->chan.chan_id);

	desc->dcm &= ~DCM_LINK;
	desc->dcm |= DCM_TIE;

	if ((dmac->cyclic & CYCLIC_POSSIBLE) && dmac->tx_desc.callback) {
		dmac->cyclic = CYCLIC_ACTIVE;
	} else
		dmac->cyclic = 0;

	/*
	 * special channel1 can not use descriptor mode
	 */
	if (dmac->id != 1) {
		/* dma descriptor address */
		writel(dmac->desc_phys, dmac->iomem + CH_DDA);
		/* initiate descriptor fetch */
		writel(BIT(dmac->id), dmac->master->iomem + DDRS);
	}

	/* DCS.CTE = 1 */
	set_bit(0, dmac->iomem + CH_DCS);

	//dump_dma_desc(dmac);
	//dump_dma(dmac->master);
}

static void jzdma_terminate_all(struct dma_chan *chan)
{
	struct jzdma_channel *dmac = to_jzdma_chan(chan);
	unsigned long flags;

	dev_vdbg(chan2dev(chan), "terminate_all %d\n", dmac->chan.chan_id);
	spin_lock_irqsave(&dmac->lock, flags);

	dmac->status = STAT_STOPED;
	dmac->desc_nr = 0;
	dmac->residue = readl(dmac->iomem + CH_DTC) * get_current_tsz(readl(dmac->iomem + CH_DCM));

	/* clear dma status */
	writel(0, dmac->iomem+CH_DCS);

	spin_unlock_irqrestore(&dmac->lock, flags);
}

static int jzdma_control(struct dma_chan *chan, enum dma_ctrl_cmd cmd,
			 unsigned long arg)
{
	struct jzdma_channel *dmac = to_jzdma_chan(chan);
	struct dma_slave_config *config = (void*)arg;
	int ret = 0;

	switch (cmd) {
	case DMA_TERMINATE_ALL:
		if (dmac->cyclic & CYCLIC_ACTIVE) {
			dmac->cyclic = 0;		/*for alsa audio*/
			return -EPIPE;
		} else {
			jzdma_terminate_all(chan);
		}
		break;

	case DMA_SLAVE_CONFIG:
		ret |= !config->src_addr_width || !config->dst_addr_width;
		ret |= !config->src_maxburst || !config->dst_maxburst;
		ret |= !config->src_addr && !config->dst_addr;
		if (ret) {
			dev_warn(chan2dev(chan), "Bad DMA control argument !");
			ret = -EINVAL;
			break;
		}

		switch (config->dst_addr_width) {
		case DMA_SLAVE_BUSWIDTH_1_BYTE:
			dmac->tx_dcm_def = DCM_SP_8 | DCM_DP_8;
			break;
		case DMA_SLAVE_BUSWIDTH_2_BYTES:
			dmac->tx_dcm_def = DCM_SP_16 | DCM_DP_16;
			break;
		case DMA_SLAVE_BUSWIDTH_4_BYTES:
			dmac->tx_dcm_def &= ~DCM_SP_32;
			dmac->tx_dcm_def &= ~DCM_DP_32;
			break;
		case DMA_SLAVE_BUSWIDTH_8_BYTES:
		case DMA_SLAVE_BUSWIDTH_UNDEFINED:
			dev_warn(chan2dev(chan), "Bad DMA control argument !");
			return -EINVAL;
		}

		/*
		 * check and mark if access NEMC through channal1
		 */
		if (dmac->id == 1 &&
		    (config->dst_addr >= NEMC_CS6_IOBASE &&
		     (config->dst_addr < NEMC_CS1_IOBASE + 0x1000000))) {
			dmac->tx_dcm_def |= DCM_CH1_SRC_DDR | DCM_CH1_DST_NEMC;
		}

		switch (config->src_addr_width) {
		case DMA_SLAVE_BUSWIDTH_1_BYTE:
			dmac->rx_dcm_def = DCM_SP_8 | DCM_DP_8;
			break;
		case DMA_SLAVE_BUSWIDTH_2_BYTES:
			dmac->rx_dcm_def = DCM_SP_16 | DCM_DP_16;
			break;
		case DMA_SLAVE_BUSWIDTH_4_BYTES:
			dmac->tx_dcm_def &= ~DCM_SP_32;
			dmac->tx_dcm_def &= ~DCM_DP_32;
			break;
		case DMA_SLAVE_BUSWIDTH_8_BYTES:
		case DMA_SLAVE_BUSWIDTH_UNDEFINED:
			dev_warn(chan2dev(chan), "Bad DMA control argument !");
			return -EINVAL;
		}

		/*
		 * check and mark if access NEMC through channal1
		 */
		if (dmac->id == 1 &&
		    (config->src_addr >= NEMC_CS6_IOBASE &&
		     config->src_addr <= NEMC_CS1_IOBASE + 0x1000000)) {
			dmac->rx_dcm_def |= DCM_CH1_SRC_NEMC | DCM_CH1_DST_DDR;
		}

		dmac->flags |= CHFLG_SLAVE;
		if (!dmac->config) {
			dmac->config = kzalloc(sizeof(struct dma_slave_config), GFP_KERNEL);
			if (!dmac->config) {
				return -ENOMEM;
			}
		}
		memcpy(dmac->config, config, sizeof(struct dma_slave_config));
		break;

	default:
		return -ENOSYS;
	}

	return ret;
}

irqreturn_t jzdma_int_handler(int irq, void *dev)
{
	struct jzdma_master *master = (struct jzdma_master *)dev;
	unsigned long pending;
	int i;

	pending = readl(master->iomem + DIRQP);

	for (i = 0; i < NR_DMA_CHANNELS; i++) {
		struct jzdma_channel *dmac = master->channel + i;

		if (!(pending & (1 << i)))
			continue;

		dmac->dcs_saved = readl(dmac->iomem + CH_DCS);
		writel(0, dmac->iomem + CH_DCS);
		if (dmac->status != STAT_RUNNING)
			continue;
		tasklet_schedule(&dmac->tasklet);
	}
	pending = readl(master->iomem + DMAC);
	pending &= ~(DMAC_HLT | DMAC_AR);
	writel(pending, master->iomem + DMAC);
	writel(0, master->iomem + DIRQP);
	return IRQ_HANDLED;
}

irqreturn_t jzdma_link_int_handler(int irq, void *dev)
{
	struct jzdma_master *master = (struct jzdma_master *)dev;
	unsigned long pending;
	int i;

	pending = readl(master->iomem + DESIRQP);
	for (i = 0; i < NR_DMA_CHANNELS; i++) {
		struct jzdma_channel *dmac = master->channel + i;

		if (!(pending & (1 << i)))
			continue;
		if (dmac->status != STAT_RUNNING)
			continue;
		if(dmac->cyclic&CYCLIC_ACTIVE)
			dmac->tx_desc.callback(dmac->tx_desc.callback_param);
	}
	writel((readl(master->iomem + DIC)&(~pending)),master->iomem + DIC);
	return IRQ_HANDLED;
}

static int jzdma_alloc_chan_resources(struct dma_chan *chan)
{
	struct jzdma_channel *dmac = to_jzdma_chan(chan);
	int ret = 0;

	dma_async_tx_descriptor_init(&dmac->tx_desc, chan);
	dmac->tx_desc.tx_submit = jzdma_tx_submit;
	/* txd.flags will be overwritten in prep funcs, FIXME */
	dmac->tx_desc.flags = DMA_CTRL_ACK;

	dmac->desc = dma_alloc_coherent(chan2dev(chan), PAGE_SIZE,&dmac->desc_phys, GFP_KERNEL);
	if (!dmac->desc) {
		dev_info(chan2dev(chan),"No Memory! ch %d\n", chan->chan_id);
		return -ENOMEM;
	}
	dmac->desc_max = PAGE_SIZE / sizeof(struct dma_desc);

	dev_info(chan2dev(chan),"Channel %d have been requested.(phy id %d,type 0x%02x desc %p)\n",
			dmac->chan.chan_id,dmac->id,dmac->type, dmac->desc);

	return ret;
}

static void jzdma_free_chan_resources(struct dma_chan *chan)
{
	struct jzdma_channel *dmac = to_jzdma_chan(chan);

	dmac->config = NULL;
	dmac->flags = 0;
	dmac->status = 0;

	dma_free_coherent(chan2dev(chan), PAGE_SIZE, dmac->desc,
			  dmac->desc_phys);
}

unsigned int pdmam_startup_irq(struct irq_data *data)
{
	return 0;
}

void pdmam_irq_dummy(struct irq_data *data)
{
}

int pdmam_set_type(struct irq_data *data, unsigned int flow_type)
{
	return 0;
}

int pdmam_set_wake(struct irq_data *data, unsigned int on)
{
	return 0;
}

static ssize_t jz_regs_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count) {
	struct jzdma_master *dma = dev_get_drvdata(dev);
	struct jzdma_channel *dmac = NULL;
	unsigned long channel = simple_strtoul(buf, NULL, 10);
	printk("dma channel %ld register:\n", channel);
	dmac = &dma->channel[channel];
	dump_dma_desc(dmac);
	dump_dma(dma);
	return count;
}

DEVICE_ATTR(dma_regs, S_IWUGO, NULL, jz_regs_store);

static int __init jzdma_probe(struct platform_device *pdev)
{
	struct jzdma_master *dma;
	struct jzdma_platform_data *pdata;
	struct resource *iores;
	short irq, irq_desc, irq_pdmam, irq_mcu;	/* irq_pdmam for PDMAM irq */
	int i, ret = 0;
	unsigned int pdma_program = 0;	/* set pdma DMACP register */

	dma = kzalloc(sizeof(*dma), GFP_KERNEL);
	if (!dma)
		return -ENOMEM;

	pdata = pdev->dev.platform_data;
	if (!pdata)
		return -ENODATA;

	dma->map = pdata->map;

	dma->irq_chip.name = "pdmam";
	dma->irq_chip.irq_startup = pdmam_startup_irq;
	dma->irq_chip.irq_shutdown = pdmam_irq_dummy;
	dma->irq_chip.irq_unmask = pdmam_irq_dummy;
	dma->irq_chip.irq_mask = pdmam_irq_dummy;
	dma->irq_chip.irq_mask_ack = pdmam_irq_dummy;
	dma->irq_chip.irq_set_type = pdmam_set_type;
	dma->irq_chip.irq_set_wake = pdmam_set_wake;

	for (i = pdata->irq_base; i <= pdata->irq_end; i++)
		irq_set_chip_and_handler(i, &dma->irq_chip, handle_level_irq);

	iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	irq = platform_get_irq_byname(pdev, "irq");
	irq_desc = platform_get_irq_byname(pdev, "irq_desc");
	irq_mcu = platform_get_irq_byname(pdev, "mcu");
	irq_pdmam = platform_get_irq_byname(pdev, "pdmam");

	if (!iores || irq < 0 || irq_pdmam < 0 || irq_desc < 0) {
		ret = -EINVAL;
		goto free_dma;
	}

	if (!request_mem_region(iores->start, resource_size(iores), pdev->name)) {
		ret = -EBUSY;
		goto free_dma;
	}

	dma->clk = clk_get(&pdev->dev, "pdma");
	if (IS_ERR(dma->clk)) {
		goto release_mem;
	}

	clk_enable(dma->clk);

	dma->iomem = ioremap(iores->start, resource_size(iores));
	if (!dma->iomem) {
		ret = -ENOMEM;
		goto release_clk;
	}

	ret = request_irq(irq, jzdma_int_handler, IRQF_DISABLED,"pdma", dma);
	if (ret)
		goto release_iomap;
	ret = request_irq(irq_desc, jzdma_link_int_handler, IRQF_DISABLED,"pdma_desc", dma);
	if (ret)
		goto release_iomap;
	/* Initialize dma engine */
	dma_cap_set(DMA_MEMCPY, dma->dma_device.cap_mask);
	dma_cap_set(DMA_SLAVE, dma->dma_device.cap_mask);
	dma_cap_set(DMA_CYCLIC, dma->dma_device.cap_mask);

	INIT_LIST_HEAD(&dma->dma_device.channels);
	/* Initialize master/channel parameters */
	dma->irq = irq;
	dma->irq_pdmam = irq_pdmam;
	dma->iomem = dma->iomem;
	/* Hardware master enable */

	/*
	 * indeed it think we should
	 * also enable special channel<0,1>
	 * but when you guys enable it (set bit1)
	 * the main cpu will never get interrupt
	 * from dma channel when TC count down to 0
	 */
	writel(1 | (0x3f << 16), dma->iomem + DMAC);

	for (i = 0; i < NR_DMA_CHANNELS; i++) {
		struct jzdma_channel *dmac = &dma->channel[i];

		dmac->id = i;

		if (dma->map)
			dmac->type = GET_MAP_TYPE(dma->map[i]);
		if (dmac->type == JZDMA_REQ_INVAL) {
			dmac->type = JZDMA_REQ_AUTO;
			dmac->chan.private = (void *)dmac->type;
		} else
			dmac->chan.private = (void *)dma->map[i];

		spin_lock_init(&dmac->lock);
		dmac->chan.device = &dma->dma_device;
		tasklet_init(&dmac->tasklet, jzdma_chan_tasklet,
				(unsigned long)dmac);

		dmac->iomem = dma->iomem + i * 0x20;
		dmac->master = dma;

		list_add(&dmac->chan.device_node, &dma->dma_device.channels);
		dev_dbg(&pdev->dev, "add chan (phy id %d , type 0x%02x)\n", i,
			dmac->type);
	}
	/* the corresponding dma channel is set programmable */
	writel(pdma_program, dma->iomem + DMACP);

	dma->dev = &pdev->dev;
	dma->dma_device.dev = &pdev->dev;
	dma->dma_device.device_alloc_chan_resources = jzdma_alloc_chan_resources;
	dma->dma_device.device_free_chan_resources = jzdma_free_chan_resources;
	dma->dma_device.device_control = jzdma_control;
	dma->dma_device.device_tx_status = jzdma_tx_status;
	dma->dma_device.device_issue_pending = jzdma_issue_pending;
	dma->dma_device.device_prep_slave_sg = jzdma_prep_slave_sg;
	dma->dma_device.device_prep_dma_memcpy = jzdma_prep_memcpy;
	dma->dma_device.device_prep_dma_cyclic = jzdma_prep_dma_cyclic;
	dma->dma_device.device_add_desc = jzdma_add_desc;
	dma->dma_device.get_current_trans_addr = jzdma_get_current_trans_addr;

	dma_set_max_seg_size(dma->dma_device.dev, 256);

	ret = dma_async_device_register(&dma->dma_device);
	if (ret) {
		dev_err(&pdev->dev, "unable to register\n");
		goto release_irq;
	}

	jzdma_mcu_reset(dma);
	ret = device_create_file(&pdev->dev, &dev_attr_dma_regs);
	if (ret)
		goto release_dma;

	platform_set_drvdata(pdev, dma);
	dev_info(dma->dev, "JZ SoC DMA initialized\n");
	return 0;
release_dma:
	dma_async_device_unregister(&dma->dma_device);
release_irq:
	free_irq(irq_pdmam, dma);
	free_irq(irq, dma);
release_iomap:
	iounmap(dma->iomem);
release_clk:
	if (dma->clk)
		clk_put(dma->clk);
release_mem:
	release_mem_region(iores->start, resource_size(iores));
free_dma:
	kfree(dma);
	dev_dbg(dma->dev, "init failed %d\n", ret);
	return ret;
}

static int __exit jzdma_remove(struct platform_device *pdev)
{
	struct jzdma_master *dma = platform_get_drvdata(pdev);
	device_remove_file(&pdev->dev, &dev_attr_dma_regs);
	dma_async_device_unregister(&dma->dma_device);
	kfree(dma);
	return 0;
}

static int jzdma_suspend(struct platform_device *pdev, pm_message_t state)
{

	return 0;
}
static int jzdma_resume(struct platform_device * pdev)
{

	return 0;
}
static struct platform_driver jzdma_driver = {
	.driver = {
		   .name = "jz-dma",
		   },
	.remove = __exit_p(jzdma_remove),
	.suspend = jzdma_suspend,
	.resume = jzdma_resume,
};

static int __init jzdma_module_init(void)
{
	return platform_driver_probe(&jzdma_driver, jzdma_probe);
}
subsys_initcall(jzdma_module_init);

MODULE_AUTHOR("Zhong Zhiping <zpzhong@ingenic.cn>");
MODULE_DESCRIPTION("Ingenic jz dma driver");
MODULE_LICENSE("GPL");
