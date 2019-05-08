/*
 * V4L2 Driver for Ingenic jz camera (CIM) host
 *
 * Copyright (C) 2014, Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * This program is support Continuous physical address mapping,
 * not support sg mode now.
 * fix by xyfu@ingenic.com
 */
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-common.h>
#include <media/v4l2-dev.h>
#include <media/videobuf-dma-contig.h>
#include <media/soc_camera.h>
#include <media/soc_mediabus.h>
#include <asm/dma.h>
/*kzalloc*/
#include <mach/jz_camera.h>

//#define CIM_DUMP_REG
//#define PRINT_CIM_REG

#ifdef CIM_DUMP_REG
static void cim_dump_reg(struct jz_camera_dev *pcdev)
{
	if(pcdev == NULL) {
		printk("===>>%s,%d pcdev is NULL!\n",__func__,__LINE__);
		return;
	}
#define STRING  "\t=\t0x%08x\n"
	printk("REG_CIM_CFG" STRING, readl(pcdev->base + CIM_CFG));
	printk("REG_CIM_CTRL" STRING, readl(pcdev->base + CIM_CTRL));
	printk("REG_CIM_CTRL2" STRING, readl(pcdev->base + CIM_CTRL2));
	printk("REG_CIM_STATE" STRING, readl(pcdev->base + CIM_STATE));

	printk("REG_CIM_IMR" STRING, readl(pcdev->base + CIM_IMR));
	printk("REG_CIM_IID" STRING, readl(pcdev->base + CIM_IID));
	printk("REG_CIM_DA" STRING, readl(pcdev->base + CIM_DA));
	printk("REG_CIM_FA" STRING, readl(pcdev->base + CIM_FA));

	printk("REG_CIM_FID" STRING, readl(pcdev->base + CIM_FID));
	printk("REG_CIM_CMD" STRING, readl(pcdev->base + CIM_CMD));
	printk("REG_CIM_WSIZE" STRING, readl(pcdev->base + CIM_SIZE));
	printk("REG_CIM_WOFFSET" STRING, readl(pcdev->base + CIM_OFFSET));

	printk("REG_CIM_FS" STRING, readl(pcdev->base + CIM_FS));
	printk("REG_CIM_YFA" STRING, readl(pcdev->base + CIM_YFA));
	printk("REG_CIM_YCMD" STRING, readl(pcdev->base + CIM_YCMD));
	printk("REG_CIM_CBFA" STRING, readl(pcdev->base + CIM_CBFA));

	printk("REG_CIM_CBCMD" STRING, readl(pcdev->base + CIM_CBCMD));
	printk("REG_CIM_CRFA" STRING, readl(pcdev->base + CIM_CRFA));
	printk("REG_CIM_CRCMD" STRING, readl(pcdev->base + CIM_CRCMD));
	printk("REG_CIM_TC" STRING, readl(pcdev->base + CIM_TC));

	printk("REG_CIM_TINX" STRING, readl(pcdev->base + CIM_TINX));
	printk("REG_CIM_TCNT" STRING, readl(pcdev->base + CIM_TCNT));
}
#endif

static int is_cim_enable(struct jz_camera_dev *pcdev)
{
	unsigned int regval = 0;

	regval = readl(pcdev->base + CIM_CTRL);
	return  (regval & 0x1);
}
static int is_cim_disabled(struct jz_camera_dev *pcdev)
{
	unsigned int regval = 0;

	regval = readl(pcdev->base + CIM_CTRL);
	return  ((regval & 0x1) == 0x0);
}

void cim_disable_without_dma(struct jz_camera_dev *pcdev)
{
	unsigned long temp = 0;

	/* clear status register */
	writel(0, pcdev->base + CIM_STATE);

	/* disable cim */
	temp = readl(pcdev->base + CIM_CTRL);
	temp &= (~CIM_CTRL_ENA);
	writel(temp, pcdev->base + CIM_CTRL);

	/* reset rx fifo */
	temp = readl(pcdev->base + CIM_CTRL);
	temp |= CIM_CTRL_RXF_RST;
	writel(temp, pcdev->base + CIM_CTRL);

	/* stop resetting RXFIFO */
	temp = readl(pcdev->base + CIM_CTRL);
	temp &= (~CIM_CTRL_RXF_RST);
	writel(temp, pcdev->base + CIM_CTRL);
}

static int jz_camera_querycap(struct soc_camera_host *ici, struct v4l2_capability *cap)
{
	strlcpy(cap->card, "jz-Camera", sizeof(cap->card));
	cap->version = VERSION_CODE;
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;

	return 0;
}

unsigned long jiff_temp;
static unsigned int jz_camera_poll(struct file *file, poll_table *pt)
{
	struct soc_camera_device *icd = file->private_data;
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;
	struct jz_buffer *buf;
	unsigned long temp = 0;
	unsigned int regval;

	if(!pcdev->poll_flag)
		pcdev->poll_flag = 1;

	buf = list_entry(icd->vb_vidq.stream.next, struct jz_buffer, vb.stream);

	if(pcdev->is_first_start) {
		if(pcdev->is_tlb_enabled == 0) {
			/* disable tlb error interrupt */
			regval = readl(pcdev->base + CIM_IMR);
			regval |= CIM_IMR_TLBEM;
			writel(regval, pcdev->base + CIM_IMR);

			/* disable tlb */
			regval = readl(pcdev->base + CIM_TC);
			regval &= ~CIM_TC_ENA;
			writel(regval, pcdev->base + CIM_TC);
		} else {
			/* enable tlb error interrupt */
			regval = readl(pcdev->base + CIM_IMR);
			regval &= ~CIM_IMR_TLBEM;
			writel(regval, pcdev->base + CIM_IMR);

			/* enable tlb */
			regval = readl(pcdev->base + CIM_TC);
			regval |= CIM_TC_ENA;
			writel(regval, pcdev->base + CIM_TC);
		}
		writel(0, pcdev->base + CIM_STATE);

		/* please enable dma first, enable cim ctrl later.
		 * if enable cim ctrl first, RXFIFO can easily overflow.
		 */
		regval = (unsigned int)(pcdev->dma_desc);
		writel(regval, pcdev->base + CIM_DA);

		/*enable dma*/
		temp = readl(pcdev->base + CIM_CTRL);
		temp |= CIM_CTRL_DMA_EN;
		writel(temp, pcdev->base + CIM_CTRL);

		/* clear rx fifo */
		temp = readl(pcdev->base + CIM_CTRL);
		temp |= CIM_CTRL_RXF_RST;
		writel(temp, pcdev->base + CIM_CTRL);

		temp = readl(pcdev->base + CIM_CTRL);
		temp &= ~CIM_CTRL_RXF_RST;
		writel(temp, pcdev->base + CIM_CTRL);

		/* enable cim */
		temp = readl(pcdev->base + CIM_CTRL);
		temp |= CIM_CTRL_ENA;
		writel(temp, pcdev->base + CIM_CTRL);

		pcdev->is_first_start = 0;
	}

	jiff_temp = jiffies;
	poll_wait(file, &buf->vb.done, pt);

	if (buf->vb.state == VIDEOBUF_DONE ||
			buf->vb.state == VIDEOBUF_ERROR) {
		return POLLIN | POLLRDNORM;
	}

	return 0;
}
static int jz_camera_alloc_desc(struct jz_camera_dev *pcdev, struct v4l2_requestbuffers *p)
{
	struct jz_camera_dma_desc *dma_desc_paddr;
	struct jz_camera_dma_desc *dma_desc;

	pcdev->buf_cnt = p->count;
	pcdev->desc_vaddr = dma_alloc_coherent(pcdev->soc_host.v4l2_dev.dev,
			sizeof(*pcdev->dma_desc) * pcdev->buf_cnt,
			(dma_addr_t *)&pcdev->dma_desc, GFP_KERNEL);

	dma_desc_paddr = (struct jz_camera_dma_desc *) pcdev->dma_desc;
	dma_desc = (struct jz_camera_dma_desc *) pcdev->desc_vaddr;

#if 0
	int i = 0;
	printk("pcdev->desc_vaddr = 0x%p, pcdev->dma_desc = 0x%p\n",pcdev->desc_vaddr, (dma_addr_t *)pcdev->dma_desc);
	for(i = 0; i < pcdev->buf_cnt; i++){
		printk("pcdev->desc_vaddr[%d] = 0x%p\n", i, &dma_desc[i]);
	}
	for(i = 0; i < pcdev->buf_cnt; i++){
		printk("dma_desc_paddr[%d] = 0x%p\n", i, &dma_desc_paddr[i]);
	}
#endif

	if (!pcdev->dma_desc)
		return -ENOMEM;

	return 0;
}

static int jz_camera_reqbufs(struct soc_camera_device *icd, struct v4l2_requestbuffers *p) {
	int i;
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;

	for (i = 0; i < p->count; i++) {
		struct jz_buffer *buf = container_of(icd->vb_vidq.bufs[i],
				struct jz_buffer, vb);
		buf->inwork = 0;
		INIT_LIST_HEAD(&buf->vb.queue);
	}

	if(jz_camera_alloc_desc(pcdev, p))
		return -ENOMEM;

	return 0;
}
static int jz_videobuf_setup(struct videobuf_queue *vq, unsigned int *count,
		unsigned int *size) {
	struct soc_camera_device *icd = vq->priv_data;
	int bytes_per_line = soc_mbus_bytes_per_line(icd->user_width,
			icd->current_fmt->host_fmt);

	if (bytes_per_line < 0)
		return bytes_per_line;

	*size = bytes_per_line * icd->user_height;

	if (!*count)
		*count = 32;

	if (*size * *count > MAX_VIDEO_MEM * 1024 * 1024)
		*count = (MAX_VIDEO_MEM * 1024 * 1024) / *size;

	dprintk(7, "count=%d, size=%d\n", *count, *size);

	return 0;
}

static void free_buffer(struct videobuf_queue *vq, struct jz_buffer *buf) {
	struct videobuf_buffer *vb = &buf->vb;
	struct soc_camera_device *icd = vq->priv_data;
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;

	BUG_ON(in_interrupt());

	dprintk(7, "%s (vb=0x%p) 0x%08lx %d\n", __func__,
			vb, vb->baddr, vb->bsize);
	/*
	 * This waits until this buffer is out of danger, i.e., until it is no
	 * longer in STATE_QUEUED or STATE_ACTIVE
	 */

	videobuf_waiton(vq, vb, 0, 0);
	videobuf_dma_contig_free(vq, vb);

	vb->state = VIDEOBUF_NEEDS_INIT;

	if(pcdev->poll_flag)
		pcdev->poll_flag = 0;

	if(pcdev->vb_address_flag)
		pcdev->vb_address_flag = 0;
}

static int jz_init_dma(struct videobuf_queue *vq, struct videobuf_buffer *vbuf) {
	struct soc_camera_device *icd = vq->priv_data;
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;
	struct jz_camera_dma_desc *dma_desc;
	dma_addr_t dma_address;

	dma_desc = (struct jz_camera_dma_desc *) pcdev->desc_vaddr;

	if(!pcdev->is_tlb_enabled) {
		dma_address = videobuf_to_dma_contig(vbuf);
	} else {
		dma_address = icd->vb_vidq.bufs[0]->baddr;
	}
	if(!dma_address) {
		dprintk(3, "Failed to setup DMA address\n");
		return -ENOMEM;
	}

	dma_desc[vbuf->i].id = vbuf->i;
	dma_desc[vbuf->i].buf = dma_address;
	if(icd->current_fmt->host_fmt->fourcc == V4L2_PIX_FMT_YUYV) {
		dma_desc[vbuf->i].cmd = icd->sizeimage >> 2 |
			CIM_CMD_EOFINT | CIM_CMD_OFRCV;
	}else if(icd->current_fmt->host_fmt->fourcc == V4L2_PIX_FMT_SBGGR8) {
		dma_desc[vbuf->i].cmd = icd->sizeimage >> 2 |
			CIM_CMD_EOFINT | CIM_CMD_OFRCV;
	} else {
		printk("current fourcc is not support, use default fourcc!\n");
		dma_desc[vbuf->i].cmd = (icd->sizeimage * 8 / 12) >> 2 |
			CIM_CMD_EOFINT | CIM_CMD_OFRCV;

		dma_desc[vbuf->i].cb_len = (icd->user_width >> 1) *
			(icd->user_height >> 1) >> 2;

		dma_desc[vbuf->i].cr_len = (icd->user_width >> 1) *
			(icd->user_height >> 1) >> 2;
	}

	if(icd->current_fmt->host_fmt->fourcc == V4L2_PIX_FMT_YUV420) {
		dma_desc[vbuf->i].cb_frame = dma_desc[vbuf->i].buf + icd->sizeimage * 8 / 12;
		dma_desc[vbuf->i].cr_frame = dma_desc[vbuf->i].cb_frame + (icd->sizeimage / 6);
	} else if(icd->current_fmt->host_fmt->fourcc == V4L2_PIX_FMT_JZ420B) {
		dma_desc[vbuf->i].cb_frame = dma_desc[vbuf->i].buf + icd->sizeimage;
		dma_desc[vbuf->i].cr_frame = dma_desc[vbuf->i].cb_frame + 64;
	}

	if(vbuf->i == 0) {
		pcdev->dma_desc_head = (struct jz_camera_dma_desc *)(pcdev->desc_vaddr);
	}

	if(vbuf->i == (pcdev->buf_cnt - 1)) {
		//dma_desc[vbuf->i].next = (dma_addr_t) (pcdev->dma_desc);
		dma_desc[vbuf->i].next = 0;
		dma_desc[vbuf->i].cmd |= CIM_CMD_STOP;
		dma_desc[vbuf->i].cmd &= (~CIM_CMD_EOFINT);

		pcdev->dma_desc_tail = (struct jz_camera_dma_desc *)(&dma_desc[vbuf->i]);
	} else {
		dma_desc[vbuf->i].next = (dma_addr_t) (&pcdev->dma_desc[vbuf->i + 1]);
	}

	dprintk(7, "cim dma desc[vbuf->i] address is: 0x%x\n", dma_desc[vbuf->i].buf);

	return 0;
}
static int jz_videobuf_prepare(struct videobuf_queue *vq,
		struct videobuf_buffer *vb, enum v4l2_field field)
{
	struct soc_camera_device *icd = vq->priv_data;
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;
	struct device *dev = pcdev->soc_host.v4l2_dev.dev;
	struct jz_buffer *buf = container_of(vb, struct jz_buffer, vb);
	int bytes_per_line = soc_mbus_bytes_per_line(icd->user_width, icd->current_fmt->host_fmt);
	int ret;

	if (bytes_per_line < 0)
		return bytes_per_line;

	dprintk(7, "%s (vb=0x%p) 0x%08lx %d\n", __func__,
			vb, vb->baddr, vb->bsize);

	/* Added list head initialization on alloc */
	WARN_ON(!list_empty(&vb->queue));

	BUG_ON(NULL == icd->current_fmt);

	/*
	 * I think, in buf_prepare you only have to protect global data,
	 * the actual buffer is yours
	 */
	buf->inwork = 1;

	if (buf->code != icd->current_fmt->code ||
			vb->width   != icd->user_width ||
			vb->height  != icd->user_height ||
			vb->field   != field) {
		buf->code	= icd->current_fmt->code;
		vb->width	= icd->user_width;
		vb->height	= icd->user_height;
		vb->field	= field;
		vb->state	= VIDEOBUF_NEEDS_INIT;
	}

	vb->size = bytes_per_line * vb->height;
	if (0 != vb->baddr && vb->bsize < vb->size) {
		ret = -EINVAL;
		goto out;
	}
	if (vb->state == VIDEOBUF_NEEDS_INIT) {
		if(pcdev->is_tlb_enabled == 0) {
			ret = videobuf_iolock(vq, vb, NULL);
			if (ret) {
				dprintk(3, "%s error!\n", __FUNCTION__);
				goto fail;
			}
		}
		jz_init_dma(vq, vb);
		if(ret) {
			dev_err(dev, "%s:DMA initialization for Y/RGB failed\n", __func__);
			goto fail;
		}
		vb->state = VIDEOBUF_PREPARED;
	}

	buf->inwork = 0;

	return 0;
fail:
	free_buffer(vq, buf);
out:
	buf->inwork = 0;
	return ret;
}

static void jz_videobuf_queue(struct videobuf_queue *vq, struct videobuf_buffer *vb)
{
	struct soc_camera_device *icd = vq->priv_data;
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;
	struct jz_camera_dma_desc *dma_desc_vaddr;
	int i = 0;
	unsigned int regval = 0;
	struct jz_camera_dma_desc *dma_desc_paddr;

	dma_desc_vaddr = (struct jz_camera_dma_desc *) pcdev->desc_vaddr;

	vb->state = VIDEOBUF_ACTIVE;

	/* judged vb->i */
	if((vb->i > pcdev->buf_cnt) || (vb->i < 0)) {
		printk("Warning: %s, %d, vb->i > pcdev->buf_cnt || vb->i < 0, please check vb->i !!!\n",
				 __func__, vb->i);
		return;
	}

	/* set pcdev->vb_address */
	if(!pcdev->vb_address_flag) {
		pcdev->vb_address_flag = 1;
		for(i = 0; i < pcdev->buf_cnt; i++)
			pcdev->vb_address[i] = (struct  videobuf_buffer *)vq->bufs[i];
	}

	/* has select */
	if(pcdev->poll_flag) {
		if(pcdev->dma_desc_head != pcdev->dma_desc_tail) {
			dma_desc_vaddr[vb->i].cmd |= CIM_CMD_STOP;
			dma_desc_vaddr[vb->i].cmd &= (~CIM_CMD_EOFINT);

			pcdev->dma_desc_tail->next = (dma_addr_t) (&pcdev->dma_desc[vb->i]);
			pcdev->dma_desc_tail->cmd &= (~CIM_CMD_STOP);
			pcdev->dma_desc_tail->cmd |=  CIM_CMD_EOFINT;
			pcdev->dma_desc_tail = (struct jz_camera_dma_desc *)(&dma_desc_vaddr[vb->i]);
		} else {
			if(is_cim_disabled(pcdev)) {
				dma_desc_vaddr[vb->i].cmd |= CIM_CMD_STOP;
				dma_desc_vaddr[vb->i].cmd &= (~CIM_CMD_EOFINT);

				pcdev->dma_desc_head = (struct jz_camera_dma_desc *)(&dma_desc_vaddr[vb->i]);
				pcdev->dma_desc_tail = (struct jz_camera_dma_desc *)(&dma_desc_vaddr[vb->i]);

				dma_desc_paddr = (struct jz_camera_dma_desc *) pcdev->dma_desc;

				/* Configure register CIMDA */
				regval = (unsigned int) (&dma_desc_paddr[vb->i]);
				writel(regval, pcdev->base + CIM_DA);

				/* clear state register */
				writel(0, pcdev->base + CIM_STATE);

				/* Reset RXFIFO */
				regval = readl(pcdev->base + CIM_CTRL);
				regval |= CIM_CTRL_DMA_EN | CIM_CTRL_RXF_RST;
				regval &= (~CIM_CTRL_ENA);
				writel(regval, pcdev->base + CIM_CTRL);

				/* stop resetting RXFIFO */
				regval = readl(pcdev->base + CIM_CTRL);
				regval |= CIM_CTRL_DMA_EN;
				regval &= ((~CIM_CTRL_RXF_RST) & (~CIM_CTRL_ENA));
				writel(regval, pcdev->base + CIM_CTRL);

				/* enable CIM */
				regval = readl(pcdev->base + CIM_CTRL);
				regval |= CIM_CTRL_DMA_EN | CIM_CTRL_ENA;
				regval &= (~CIM_CTRL_RXF_RST);
				writel(regval, pcdev->base + CIM_CTRL);
			} else {
				dma_desc_vaddr[vb->i].cmd &= (~CIM_CMD_EOFINT);
				dma_desc_vaddr[vb->i].cmd |= CIM_CMD_STOP;

				pcdev->dma_desc_tail->next = (dma_addr_t) (&pcdev->dma_desc[vb->i]);
				pcdev->dma_desc_tail->cmd &= (~CIM_CMD_STOP);
				pcdev->dma_desc_tail->cmd |= CIM_CMD_EOFINT;
				pcdev->dma_desc_tail = (struct jz_camera_dma_desc *)(&dma_desc_vaddr[vb->i]);
			}
		}
	}

}

static void jz_videobuf_release(struct videobuf_queue *vq,
		struct videobuf_buffer *vb) {

	struct jz_buffer *buf = container_of(vb, struct jz_buffer, vb);

	dprintk(7, "%s (vb=0x%p) 0x%08lx %d\n", __func__,
			vb, vb->baddr, vb->bsize);

	switch (vb->state) {
		case VIDEOBUF_ACTIVE:
			dprintk(7, "%s (active)\n", __func__);
			break;
		case VIDEOBUF_QUEUED:
			dprintk(7, "%s (queued)\n", __func__);
			break;
		case VIDEOBUF_PREPARED:
			dprintk(7, "%s (prepared)\n", __func__);
			break;

		case VIDEOBUF_DONE:
			dprintk(7, "%s (DONE)\n", __func__);
			break;
		case VIDEOBUF_IDLE:
			dprintk(7, "%s (IDLE)\n", __func__);
			break;

		case VIDEOBUF_ERROR:
			dprintk(7, "%s (ERROR)\n", __func__);
			break;
		case VIDEOBUF_NEEDS_INIT:
			dprintk(7, "%s (INIT)\n", __func__);
			break;
		default:
			dprintk(7, "%s (unknown)\n", __func__);
			break;
	}

	free_buffer(vq, buf);
}

static int test_platform_param(struct jz_camera_dev *pcdev,
			       unsigned char buswidth, unsigned long *flags)
{
	/*
	 * Platform specified synchronization and pixel clock polarities are
	 * only a recommendation and are only used during probing. The PXA270
	 * quick capture interface supports both.
	 */
	*flags = V4L2_MBUS_MASTER |
		V4L2_MBUS_HSYNC_ACTIVE_HIGH |
		V4L2_MBUS_HSYNC_ACTIVE_LOW |
		V4L2_MBUS_VSYNC_ACTIVE_HIGH |
		V4L2_MBUS_VSYNC_ACTIVE_LOW |
		V4L2_MBUS_DATA_ACTIVE_HIGH |
		V4L2_MBUS_PCLK_SAMPLE_RISING |
		V4L2_MBUS_PCLK_SAMPLE_FALLING;

	/* If requested data width is supported by the platform, use it */
	//if ((1 << (buswidth - 1)) & pcdev->width_flags)
	//	return 0;
	return 0;

//	return -EINVAL;
}
static struct videobuf_queue_ops jz_videobuf_ops = {
	.buf_setup      = jz_videobuf_setup,
	.buf_prepare    = jz_videobuf_prepare,
	.buf_queue      = jz_videobuf_queue,
	.buf_release    = jz_videobuf_release,
};
static void jz_camera_init_videobuf(struct videobuf_queue *q, struct soc_camera_device *icd) {
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;

	videobuf_queue_dma_contig_init(q, &jz_videobuf_ops, icd->parent,
			&pcdev->lock, V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_FIELD_NONE,
			sizeof(struct jz_buffer), icd, &ici->host_lock);
}

static int jz_camera_try_fmt(struct soc_camera_device *icd, struct v4l2_format *f) {
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	const struct soc_camera_format_xlate *xlate;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct v4l2_mbus_framefmt mf;
	__u32 pixfmt = pix->pixelformat;
	int ret;
	/* TODO: limit to jz hardware capabilities */

	xlate = soc_camera_xlate_by_fourcc(icd, pix->pixelformat);
	if (!xlate) {
		printk("Format %x not found\n", pix->pixelformat);
		return -EINVAL;
	}

	v4l_bound_align_image(&pix->width, 48, 2048, 1,
			&pix->height, 32, 2048, 0,
			pixfmt == V4L2_PIX_FMT_YUV422P ? 4 : 0);


	mf.width	= pix->width;
	mf.height	= pix->height;
	mf.field	= pix->field;
	mf.colorspace	= pix->colorspace;
	mf.code		= xlate->code;


	/* limit to sensor capabilities */
	ret = v4l2_subdev_call(sd, video, try_mbus_fmt, &mf);
	if (ret < 0)
		return ret;

	pix->width	= mf.width;
	pix->height	= mf.height;
	pix->field	= mf.field;
	pix->colorspace	= mf.colorspace;

	return 0;
}

static int jz_camera_set_fmt(struct soc_camera_device *icd, struct v4l2_format *f) {
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	const struct soc_camera_format_xlate *xlate;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	struct v4l2_mbus_framefmt mf;
	int ret, buswidth;
	xlate = soc_camera_xlate_by_fourcc(icd, pix->pixelformat);
	if (!xlate) {
		dprintk(4, "Format %x not found\n", pix->pixelformat);
		return -EINVAL;
	}

	buswidth = xlate->host_fmt->bits_per_sample;
	if (buswidth > 8) {
		dprintk(4, "bits-per-sample %d for format %x unsupported\n",
				buswidth, pix->pixelformat);
		return -EINVAL;
	}

	mf.width        = pix->width;
	mf.height       = pix->height;
	mf.field        = pix->field;
	mf.colorspace   = pix->colorspace;
	mf.code         = xlate->code;

	ret = v4l2_subdev_call(sd, video, s_mbus_fmt, &mf);
	if (ret < 0)
		return ret;

	if (mf.code != xlate->code)
		return -EINVAL;

	pix->width              = mf.width;
	pix->height             = mf.height;
	pix->field              = mf.field;
	pix->colorspace         = mf.colorspace;

	icd->current_fmt        = xlate;

	return ret;
}

static int jz_camera_set_crop(struct soc_camera_device *icd, const struct v4l2_crop *a) {
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);

	return v4l2_subdev_call(sd, video, s_crop, a);
}

static int jz_camera_set_bus_param(struct soc_camera_device *icd) {
	struct v4l2_subdev *sd = soc_camera_to_subdev(icd);
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;
	struct v4l2_mbus_config cfg = {.type = V4L2_MBUS_PARALLEL,};
	u32 pixfmt = icd->current_fmt->host_fmt->fourcc;
	unsigned long common_flags;
	unsigned long bus_flags = 0;
	unsigned long cfg_reg = 0;
	unsigned long ctrl_reg = 0;
	unsigned long ctrl2_reg = 0;
	unsigned long fs_reg = 0;
	unsigned long temp = 0;
	int ret;

	ret = test_platform_param(pcdev, icd->current_fmt->host_fmt->bits_per_sample,
			&bus_flags);

	ret = v4l2_subdev_call(sd, video, g_mbus_config, &cfg);
	if (!ret) {
		common_flags = soc_mbus_config_compatible(&cfg,
				bus_flags);
		if (!common_flags) {
			dev_warn(icd->parent,
					"Flags incompatible: camera 0x%x, host 0x%lx\n",
					cfg.flags, bus_flags);
			return -EINVAL;
		}
	} else if (ret != -ENOIOCTLCMD) {
		return ret;
	} else {
		common_flags = bus_flags;
	}

	/* Make choises, based on platform preferences */

	if ((common_flags & V4L2_MBUS_VSYNC_ACTIVE_HIGH) &&
	    (common_flags & V4L2_MBUS_VSYNC_ACTIVE_LOW)) {
		if (pcdev->pdata->flags & jz_CAMERA_VSYNC_HIGH)
			common_flags &= ~V4L2_MBUS_VSYNC_ACTIVE_HIGH;
		else
			common_flags &= ~V4L2_MBUS_VSYNC_ACTIVE_LOW;
	}

	if ((common_flags & V4L2_MBUS_PCLK_SAMPLE_RISING) &&
	    (common_flags & V4L2_MBUS_PCLK_SAMPLE_FALLING)) {
		if (pcdev->pdata->flags & jz_CAMERA_PCLK_RISING)
			common_flags &= ~V4L2_MBUS_PCLK_SAMPLE_FALLING;
		else
			common_flags &= ~V4L2_MBUS_PCLK_SAMPLE_RISING;
	}

	if ((common_flags & V4L2_MBUS_DATA_ACTIVE_HIGH) &&
	    (common_flags & V4L2_MBUS_DATA_ACTIVE_LOW)) {
		if (pcdev->pdata->flags & jz_CAMERA_DATA_HIGH)
			common_flags &= ~V4L2_MBUS_DATA_ACTIVE_LOW;
		else
			common_flags &= ~V4L2_MBUS_DATA_ACTIVE_HIGH;
	}

	cfg.flags = common_flags;
	ret = v4l2_subdev_call(sd, video, s_mbus_config, &cfg);
	if (ret < 0 && ret != -ENOIOCTLCMD) {
		dev_dbg(icd->parent, "camera s_mbus_config(0x%lx) returned %d\n",
			common_flags, ret);
		return ret;
	}

	/*PCLK Polarity Set*/
	cfg_reg = (common_flags & V4L2_MBUS_PCLK_SAMPLE_RISING) ?
		cfg_reg | CIM_CFG_PCP_HIGH : cfg_reg & (~CIM_CFG_PCP_HIGH);

	/*VSYNC Polarity Set*/
	cfg_reg = (common_flags & V4L2_MBUS_VSYNC_ACTIVE_LOW) ?
		cfg_reg | CIM_CFG_VSP_HIGH : cfg_reg & (~CIM_CFG_VSP_HIGH);

	/*HSYNC Polarity Set*/
	cfg_reg = (common_flags & V4L2_MBUS_HSYNC_ACTIVE_LOW) ?
		cfg_reg | CIM_CFG_HSP_HIGH : cfg_reg & (~CIM_CFG_HSP_HIGH);

	cfg_reg |= CIM_CFG_DMA_BURST_INCR32 | CIM_CFG_DF_YUV422
		| CIM_CFG_DSM_GCM | CIM_CFG_PACK_Y0UY1V;

	ctrl_reg |= CIM_CTRL_DMA_SYNC | CIM_CTRL_FRC_1;

	ctrl2_reg |= CIM_CTRL2_APM | CIM_CTRL2_EME | CIM_CTRL2_OPE |
		(1 << CIM_CTRL2_OPG_BIT) | CIM_CTRL2_FSC | CIM_CTRL2_ARIF;

	fs_reg = (icd->user_width -1) << CIM_FS_FHS_BIT | (icd->user_height -1)
		<< CIM_FS_FVS_BIT | 1 << CIM_FS_BPP_BIT;

	if((pixfmt == V4L2_PIX_FMT_YUV420) || (pixfmt == V4L2_PIX_FMT_JZ420B)) {
		ctrl2_reg |= CIM_CTRL2_CSC_YUV420;
		cfg_reg |= CIM_CFG_SEP | CIM_CFG_ORDER_YUYV;
	}

	if(pixfmt == V4L2_PIX_FMT_JZ420B)
		ctrl_reg |= CIM_CTRL_MBEN;

	if(pixfmt == V4L2_PIX_FMT_SBGGR8) {
		fs_reg = (icd->user_width -1) << CIM_FS_FHS_BIT | (icd->user_height -1)
			<< CIM_FS_FVS_BIT | 0 << CIM_FS_BPP_BIT;
		ctrl2_reg |= CIM_CTRL2_CSC_BYPASS;
	}

	writel(cfg_reg, pcdev->base + CIM_CFG);
	writel(ctrl_reg, pcdev->base + CIM_CTRL);
	writel(ctrl2_reg, pcdev->base + CIM_CTRL2);
	writel(fs_reg, pcdev->base + CIM_FS);

#ifdef PRINT_CIM_REG
	cim_dump_reg(pcdev);
#endif
	/* enable end of frame interrupt */
	temp = readl(pcdev->base + CIM_IMR);
	temp &= (~CIM_IMR_EOFM);
	writel(temp, pcdev->base + CIM_IMR);

	/* enable stop of frame interrupt */
	temp = readl(pcdev->base + CIM_IMR);
	temp &= (~CIM_IMR_STPM);
	writel(temp, pcdev->base + CIM_IMR);

	/* enable rx overflow interrupt */
	temp = readl(pcdev->base + CIM_IMR);
	temp &= (~CIM_IMR_RFIFO_OFM);
	writel(temp, pcdev->base + CIM_IMR);

	return 0;
}
static void jz_camera_activate(struct jz_camera_dev *pcdev) {
	int ret = -1;

	dprintk(7, "Activate device\n");
	if(pcdev->clk) {
		ret = clk_enable(pcdev->clk);
	}
	if(pcdev->mclk) {
		ret = clk_set_rate(pcdev->mclk, pcdev->mclk_freq);
		ret = clk_enable(pcdev->mclk);
	}

	if(ret) {
		printk("enable clock failed!\n");
	}
}
static void jz_camera_deactivate(struct jz_camera_dev *pcdev) {
	unsigned long temp = 0;

	if(pcdev->clk) {
		clk_disable(pcdev->clk);
	}
	if(pcdev->mclk) {
		clk_disable(pcdev->mclk);
	}

	/* clear status register */
	writel(0, pcdev->base + CIM_STATE);

	/* disable end of frame interrupt */
	temp = readl(pcdev->base + CIM_IMR);
	temp |= CIM_IMR_EOFM;
	writel(temp, pcdev->base + CIM_IMR);

	/* disable rx overflow interrupt */
	temp = readl(pcdev->base + CIM_IMR);
	temp |= CIM_IMR_RFIFO_OFM;
	writel(temp, pcdev->base + CIM_IMR);

	/* disable dma */
	temp = readl(pcdev->base + CIM_CTRL);
	temp &= ~CIM_CTRL_DMA_EN;
	writel(temp, pcdev->base + CIM_CTRL);

	/* clear rx fifo */
	temp = readl(pcdev->base + CIM_CTRL);
	temp |= CIM_CTRL_RXF_RST;
	writel(temp, pcdev->base + CIM_CTRL);

	temp = readl(pcdev->base + CIM_CTRL);
	temp &= ~CIM_CTRL_RXF_RST;
	writel(temp, pcdev->base + CIM_CTRL);

	/* disable cim */
	temp = readl(pcdev->base + CIM_CTRL);
	temp &= ~CIM_CTRL_ENA;
	writel(temp, pcdev->base + CIM_CTRL);

	if(pcdev && pcdev->desc_vaddr) {
		dma_free_coherent(pcdev->soc_host.v4l2_dev.dev,
				sizeof(*pcdev->dma_desc) * pcdev->buf_cnt,
				pcdev->desc_vaddr, (dma_addr_t )pcdev->dma_desc);

		pcdev->desc_vaddr = NULL;
	}
	dprintk(7, "Deactivate device\n");
}

static int jz_camera_add_device(struct soc_camera_device *icd) {
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;

	int icd_index = icd->devnum;

	if (pcdev->icd[icd_index])
		return -EBUSY;

	printk("jz Camera driver attached to camera %d\n",
			icd->devnum);

	pcdev->icd[icd_index] = icd;
	pcdev->is_first_start = 1;

	/* disable tlb when open camera every time */
	pcdev->is_tlb_enabled = 0;

	jz_camera_activate(pcdev);

	return 0;
}
static void jz_camera_remove_device(struct soc_camera_device *icd) {
	struct soc_camera_host *ici = to_soc_camera_host(icd->parent);
	struct jz_camera_dev *pcdev = ici->priv;

	int icd_index = icd->devnum;

	BUG_ON(icd != pcdev->icd[icd_index]);

	jz_camera_deactivate(pcdev);
	dprintk(6, "jz Camera driver detached from camera %d\n",
			icd->devnum);

	pcdev->icd[icd_index] = NULL;
}

static void jz_camera_wakeup(struct jz_camera_dev *pcdev,
		struct videobuf_buffer *vb) {

	vb->state = VIDEOBUF_DONE;
	do_gettimeofday(&vb->ts);

	vb->field_count++;
	wake_up(&vb->done);

	dprintk(7, "after wake up cost %d ms\n", jiffies_to_msecs(jiffies - jiff_temp));
}

static irqreturn_t jz_camera_irq_handler(int irq, void *data) {
	struct jz_camera_dev *pcdev = (struct jz_camera_dev *)data;
	unsigned long status = 0, temp = 0;
	unsigned long flags = 0;
	int index = 0, regval = 0;
	struct jz_camera_dma_desc *dma_desc_paddr;
	//cim_dump_reg(pcdev);
	for (index = 0; index < ARRAY_SIZE(pcdev->icd); index++) {
		if (pcdev->icd[index]) {
			break;
		}
	}

	if(index == MAX_SOC_CAM_NUM)
		return IRQ_HANDLED;

	/* judged pcdev->dma_desc_head->id */
	if((pcdev->dma_desc_head->id > pcdev->buf_cnt) || (pcdev->dma_desc_head->id < 0)) {
		printk("Warning: %s, %d, pcdev->dma_desc_head->id >pcdev->buf_cnt || pcdev->dma_desc_head->id < 0, please check pcdev->dma_desc_head->id !!!\n",
				 __func__, pcdev->dma_desc_head->id);
		return IRQ_NONE;
	}

	spin_lock_irqsave(&pcdev->lock, flags);

	/* read interrupt status register */
	status = readl(pcdev->base + CIM_STATE);
	if (!status) {
		printk("status is NULL! \n");
		spin_unlock_irqrestore(&pcdev->lock, flags);

		return IRQ_NONE;
	}

	if(!(status & CIM_STATE_RXOF_STOP_EOF)) {
		/* other irq */
		printk("irq_handle status is 0x%lx, not judged in irq_handle\n", status);

		spin_unlock_irqrestore(&pcdev->lock, flags);
		return IRQ_HANDLED;
	}

	if (status & CIM_STATE_RXF_OF) {
		printk("RX FIFO OverFlow interrupt!\n");

		/* clear rx overflow interrupt */
		temp = readl(pcdev->base + CIM_STATE);
		temp &= ~CIM_STATE_RXF_OF;
		writel(temp, pcdev->base + CIM_STATE);

		/* disable cim */
		temp = readl(pcdev->base + CIM_CTRL);
		temp &= ~CIM_CTRL_ENA;
		writel(temp, pcdev->base + CIM_CTRL);

		/* clear rx fifo */
		temp = readl(pcdev->base + CIM_CTRL);
		temp |= CIM_CTRL_RXF_RST;
		writel(temp, pcdev->base + CIM_CTRL);

		temp = readl(pcdev->base + CIM_CTRL);
		temp &= ~CIM_CTRL_RXF_RST;
		writel(temp, pcdev->base + CIM_CTRL);

		/* clear status register */
		writel(0, pcdev->base + CIM_STATE);

		/* enable cim */
		temp = readl(pcdev->base + CIM_CTRL);
		temp |= CIM_CTRL_ENA;
		writel(temp, pcdev->base + CIM_CTRL);

		spin_unlock_irqrestore(&pcdev->lock, flags);
		return IRQ_HANDLED;
	}

	if(status & CIM_STATE_DMA_STOP) {
		/* clear dma interrupt status */
		temp = readl(pcdev->base + CIM_STATE);
		temp &= (~CIM_STATE_DMA_STOP);
		writel(temp, pcdev->base + CIM_STATE);

		jz_camera_wakeup(pcdev, (pcdev->vb_address[pcdev->dma_desc_head->id]));

		if(pcdev->dma_desc_head == pcdev->dma_desc_tail) {
			if(is_cim_enable(pcdev)) {
				cim_disable_without_dma(pcdev);
			} else {
				printk("@@@@@@ not complate!\n");
			}
		} else {
			pcdev->dma_desc_head =
				(struct jz_camera_dma_desc *)UNCAC_ADDR(phys_to_virt(pcdev->dma_desc_head->next));

			/* Configure register CIMDA */
			dma_desc_paddr = (struct jz_camera_dma_desc *) pcdev->dma_desc;
			regval = (unsigned int) (&dma_desc_paddr[pcdev->dma_desc_head->id]);
			writel(regval, pcdev->base + CIM_DA);
		}

		spin_unlock_irqrestore(&pcdev->lock, flags);

		return IRQ_HANDLED;
	}

	if(status & CIM_STATE_DMA_EOF) {
		/* clear dma interrupt status */
		temp = readl(pcdev->base + CIM_STATE);
		temp &= (~CIM_STATE_DMA_EOF);
		writel(temp, pcdev->base + CIM_STATE);

		jz_camera_wakeup(pcdev, (struct videobuf_buffer *)(pcdev->vb_address[pcdev->dma_desc_head->id]));
		if(pcdev->dma_desc_head != pcdev->dma_desc_tail) {
			pcdev->dma_desc_head =
				(struct jz_camera_dma_desc *)UNCAC_ADDR(phys_to_virt(pcdev->dma_desc_head->next));
		}

		spin_unlock_irqrestore(&pcdev->lock, flags);

		return IRQ_HANDLED;
	}
	spin_unlock_irqrestore(&pcdev->lock, flags);

	return IRQ_HANDLED;
}
/* initial camera sensor reset and power gpio */
static int camera_sensor_gpio_init(struct platform_device *pdev) {
	int ret, i, cnt = 0;
	struct jz_camera_pdata *pdata = pdev->dev.platform_data;
	char gpio_name[20];




	if(!pdata) {
		dev_err(&pdev->dev, "%s:have not cim platform data\n", __func__);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(pdata->cam_sensor_pdata); i++) {
		if(pdata->cam_sensor_pdata[i].gpio_rst) {
			sprintf(gpio_name, "sensor_rst%d", i);
			printk("sensor_rst%d,%d\n", i,pdata->cam_sensor_pdata[i].gpio_rst);
			ret = gpio_request(pdata->cam_sensor_pdata[i].gpio_rst,
					gpio_name);
			if(ret) {
				dev_err(&pdev->dev, "%s:request cim%d reset gpio fail\n",
						__func__, i);
				goto err_request_gpio;
			}
			gpio_direction_output(pdata->cam_sensor_pdata[i].gpio_rst, 0);
			cnt++;
		}

		if(pdata->cam_sensor_pdata[i].gpio_power) {
			sprintf(gpio_name, "sensor_en%d", i);
			printk("sensor_en%d,%d\n", i,pdata->cam_sensor_pdata[i].gpio_power);
			ret = gpio_request(pdata->cam_sensor_pdata[i].gpio_power,
					gpio_name);
			if(ret) {
				dev_err(&pdev->dev, "%s:request cim%d en gpio fail\n",
						__func__, i);
				goto err_request_gpio;
			}
			//gpio_direction_output(pdata->cam_sensor_pdata[i].gpio_power, 1);
			gpio_direction_output(pdata->cam_sensor_pdata[i].gpio_power, 0);
			cnt++;
		}
	}

	return 0;

err_request_gpio:
	for (i = 0; i < cnt / 2; i++) {
		gpio_free(pdata->cam_sensor_pdata[i].gpio_rst);
		gpio_free(pdata->cam_sensor_pdata[i].gpio_power);
	}

	if ((cnt % 2) != 0) {
		gpio_free(pdata->cam_sensor_pdata[i].gpio_rst);
	}

	return ret;
}

static struct soc_camera_host_ops jz_soc_camera_host_ops = {
	.owner = THIS_MODULE,
	.add = jz_camera_add_device,
	.remove = jz_camera_remove_device,
	.set_bus_param = jz_camera_set_bus_param,
	.set_crop = jz_camera_set_crop,
	.set_fmt = jz_camera_set_fmt,
	.try_fmt = jz_camera_try_fmt,
	.init_videobuf = jz_camera_init_videobuf,
	.reqbufs = jz_camera_reqbufs,
	.poll = jz_camera_poll,
	.querycap = jz_camera_querycap,
	//.num_controls = 0,
};

static int __init jz_camera_probe(struct platform_device *pdev) {
	int err = 0;
	unsigned int irq;
	struct resource *res;
	void __iomem *base;
	struct jz_camera_dev *pcdev;
	char clk_name[16];

	/* malloc */
	pcdev = kzalloc(sizeof(*pcdev), GFP_KERNEL);
	if (!pcdev) {
		dprintk(3, "Could not allocate pcdev\n");
		err = -ENOMEM;
		goto err_kzalloc;
	}

	/* resource */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res) {
		dprintk(3, "Could not get resource!\n");
		err = -ENODEV;
		goto err_get_resource;
	}

	/* irq */
	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		dprintk(3, "Could not get irq!\n");
		err = -ENODEV;
		goto err_get_irq;
	}

	/*get cim1 clk*/
	sprintf(clk_name, "cim%d", pdev->id);
	pcdev->clk = clk_get(&pdev->dev, clk_name);
	if (IS_ERR(pcdev->clk)) {
		err = PTR_ERR(pcdev->clk);
		dev_err(&pdev->dev, "%s:can't get clk %s\n", __func__, "cim");
		goto err_clk_get_cim1;
	}

	/*get cgu_cimmclk1 clk*/
	sprintf(clk_name, "cgu_cimmclk%d", pdev->id);
	pcdev->mclk = clk_get(&pdev->dev, clk_name);
	if (IS_ERR(pcdev->mclk)) {
		err = PTR_ERR(pcdev->mclk);
		dev_err(&pdev->dev, "%s:can't get clk %s\n",
				__func__, "cgu_cimmclk");
		goto err_clk_get_cgu_cimmclk1;
	}

#if 0
	/*regulator maybe used in some board, so if you need to use regulator,
	 * you should modify vcim_2_8 according to the name of your board.
	 */
	pcdev->soc_host.regul = regulator_get(&pdev->dev, "vcim_2_8");
	if(IS_ERR(pcdev->soc_host.regul)) {
		dprintk(3, "get regulator fail !, if you need regulator, please check this place!\n");
		err = -ENODEV;
		pcdev->soc_host.regul = NULL;
		//goto exit_put_clk_cim;
	}
#endif

	pcdev->pdata = pdev->dev.platform_data;
	if (pcdev->pdata)
		pcdev->mclk_freq = pcdev->pdata->mclk_10khz * 10000;

	if (!pcdev->mclk_freq) {
		dprintk(4, "mclk_10khz == 0! Please, fix your platform data."
				"Using default 24MHz\n");
		pcdev->mclk_freq = 24000000;
	}

	/* Request the regions. */
	if (!request_mem_region(res->start, resource_size(res), DRIVER_NAME)) {
		err = -EBUSY;
		goto err_request_mem_region;
	}
	base = ioremap(res->start, resource_size(res));
	if (!base) {
		err = -ENOMEM;
		goto err_ioremap;
	}

	spin_lock_init(&pcdev->lock);

	pcdev->res = res;
	pcdev->irq = irq;
	pcdev->base = base;

	/* request irq */
	err = request_irq(pcdev->irq, jz_camera_irq_handler, IRQF_DISABLED,
			dev_name(&pdev->dev), pcdev);
	if(err) {
		dprintk(3, "request irq failed!\n");
		goto err_request_irq;
	}

	/* request sensor reset and power gpio */
	err = camera_sensor_gpio_init(pdev);
	if(err) {
		goto err_camera_sensor_gpio_init;
	}

	pcdev->soc_host.drv_name        = DRIVER_NAME;
	pcdev->soc_host.ops             = &jz_soc_camera_host_ops;
	pcdev->soc_host.priv            = pcdev;
	pcdev->soc_host.v4l2_dev.dev    = &pdev->dev;
	pcdev->soc_host.nr              = pdev->id;
	pcdev->poll_flag		= 0;
	pcdev->vb_address_flag		= 0;

	err = soc_camera_host_register(&pcdev->soc_host);
	if (err)
		goto err_soc_camera_host_register;

	printk("JZ Camera driver loaded!\n");

	return 0;

err_soc_camera_host_register:
err_camera_sensor_gpio_init:
	free_irq(pcdev->irq, pcdev);
err_request_irq:
	iounmap(base);
err_ioremap:
	release_mem_region(res->start, resource_size(res));
err_request_mem_region:
	clk_put(pcdev->mclk);
err_clk_get_cgu_cimmclk1:
	clk_put(pcdev->clk);
err_clk_get_cim1:
err_get_irq:
err_get_resource:
	kfree(pcdev);
err_kzalloc:
	return err;

}

static int __exit jz_camera_remove(struct platform_device *pdev)
{
	struct soc_camera_host *soc_host = to_soc_camera_host(&pdev->dev);
	struct jz_camera_dev *pcdev = container_of(soc_host,
					struct jz_camera_dev, soc_host);
	struct resource *res;

	free_irq(pcdev->irq, pcdev);

	clk_put(pcdev->clk);
	clk_put(pcdev->mclk);

	soc_camera_host_unregister(soc_host);

	iounmap(pcdev->base);

	res = pcdev->res;
	release_mem_region(res->start, resource_size(res));

	kfree(pcdev);

	dprintk(6, "jz Camera driver unloaded\n");

	return 0;
}

static struct platform_driver jz_camera_driver = {
	.remove		= __exit_p(jz_camera_remove),
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init jz_camera_init(void) {
	/*
	 * platform_driver_probe() can save memory,
	 * but this Driver can bind to one device only.
	 */
	return platform_driver_probe(&jz_camera_driver, jz_camera_probe);
}

static void __exit jz_camera_exit(void) {
	return platform_driver_unregister(&jz_camera_driver);
}

late_initcall(jz_camera_init);
module_exit(jz_camera_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yefei <feiye@ingenic.cn>");
MODULE_DESCRIPTION("jz Soc Camera Host Driver");
MODULE_ALIAS("a jz-cim platform");
