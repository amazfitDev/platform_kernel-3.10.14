/*
 * SFC controller for SPI protocol, use FIFO and DMA;
 *
 * Copyright (c) 2015 Ingenic
 * Author: <xiaoyang.fu@ingenic.com>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>

#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>

#include"jz_sfcnand.h"

#define SWAP_BUF_SIZE (4 * 1024)
//#define SFC_DEBUG
#define GET_PHYADDR(a)                                          \
({                                              \
	 unsigned int v;                                        \
	 if (unlikely((unsigned int)(a) & 0x40000000)) {                            \
	 v = page_to_phys(vmalloc_to_page((const void *)(a))) | ((unsigned int)(a) & ~PAGE_MASK); \
	 } else                                         \
	 v = ((int)(a) & 0x1fffffff);                   \
	 v;                                             \
})



#ifdef SFC_DEGUG
#define  print_dbg(format,arg...)	printk(format,## arg)
#else
#define  print_dbg(format,arg...)
#endif

#define MAX_ADDR_LEN 4
#define JZ_NOR_INIT(cmd1,addr_len1,daten1,pollen1,M7_01,dummy_byte1,dma_mode1) \
{										\
	.cmd = cmd1,                \
	.addr_len = addr_len1,        \
	.daten = daten1,               \
	.pollen = pollen1,           \
	.M7_0 = M7_01,              \
	.dummy_byte = dummy_byte1,   \
	.dma_mode = dma_mode1,      \
}

#define JZ_NOR_REG   \
	JZ_NOR_INIT(CMD_WREN,0,0,0,0,0,0), \
	JZ_NOR_INIT(CMD_WRDI,0,0,0,0,0,0), \
	JZ_NOR_INIT(CMD_RDSR,0,1,0,0,0,1),\
	JZ_NOR_INIT(CMD_RDSR_1,0,1,0,0,0,1),\
	JZ_NOR_INIT(CMD_RDSR_2,0,1,0,0,0,1),\
	JZ_NOR_INIT(CMD_WRSR,0,1,0,0,0,0),\
	JZ_NOR_INIT(CMD_WRSR_1,0,1,0,0,0,0),\
	JZ_NOR_INIT(CMD_WRSR_2,0,1,0,0,0,0),\
	JZ_NOR_INIT(CMD_READ,3,1,0,0,0,1),\
	JZ_NOR_INIT(CMD_DUAL_READ,3,1,0,0,8,1),\
	JZ_NOR_INIT(CMD_QUAD_READ,3,1,0,0,8,1),\
	JZ_NOR_INIT(CMD_QUAD_IO_FAST_READ,4,1,0,1,4,1),\
	JZ_NOR_INIT(CMD_QUAD_IO_WORD_FAST_READ,4,1,0,1,2,1),\
	JZ_NOR_INIT(CMD_PP,3,1,0,0,0,1),\
	JZ_NOR_INIT(CMD_QPP,3,1,0,0,0,1),\
	JZ_NOR_INIT(CMD_BE_32K,3,0,0,0,0,0),\
	JZ_NOR_INIT(CMD_BE_64K,3,0,0,0,0,0),\
	JZ_NOR_INIT(CMD_SE,3,0,0,0,0,0),\
	JZ_NOR_INIT(CMD_CE,0,0,0,0,0,0),\
	JZ_NOR_INIT(CMD_DP,0,0,0,0,0,0),\
	JZ_NOR_INIT(CMD_RES,0,0,0,0,0,0),\
	JZ_NOR_INIT(CMD_REMS,0,0,0,0,0,0),\
	JZ_NOR_INIT(CMD_RDID,0,1,0,0,0,0), \
	JZ_NOR_INIT(CMD_FAST_READ,3,1,0,0,8,1), \
	JZ_NOR_INIT(CMD_NON,0,0,0,0,0,0)
#define THRESHOLD                       31


struct sfc_nor_info jz_sfc_nor_info[] = {
//	JZ_NAND_REG,
	JZ_NOR_REG,
};

static  u_char *command_write;
int jz_nor_info_num = ARRAY_SIZE(jz_sfc_nor_info);
/* Max time can take up to 3 seconds! */
#define MAX_READY_WAIT_TIME 3000    /* the time of erase BE(64KB) */

struct mtd_partition *jz_mtd_partition;
struct spi_nor_platform_data *board_info;
int quad_mode = 0;

static unsigned int cpu_write_txfifo(struct jz_sfc_nand *flash)
{
	int i;
	unsigned long  len = (flash->sfc_tran->len + 3) / 4;
	unsigned int write_tofifo_num = 0;
	if ((len - flash->sfc_tran->finally_len) > THRESHOLD)
		write_tofifo_num = THRESHOLD;
	else {
		write_tofifo_num = len - flash->sfc_tran->finally_len;
	}
	for(i = 0; i < write_tofifo_num; i++) {
		sfc_write_data(flash, (flash->sfc_tran->tx_buf));
		flash->sfc_tran->tx_buf++;
		flash->sfc_tran->finally_len ++;
	}
	if(len == flash->sfc_tran->finally_len){
		print_dbg("transfer ok\n");
		flash->sfc_tran->finally_len = flash->sfc_tran->len;
		return flash->sfc_tran->finally_len;
	}
	return -1;
}
static unsigned int cpu_read_rxfifo(struct jz_sfc_nand *flash)
{
	int i;
	unsigned long  len = (flash->sfc_tran->len + 3) / 4 ;
	unsigned int fifo_num = 0;
	fifo_num = sfc_fifo_num(flash);
	if (fifo_num > THRESHOLD)
		fifo_num = THRESHOLD;
	for(i = 0; i < fifo_num; i++) {
		sfc_read_data(flash, (flash->sfc_tran->rx_buf));
		flash->sfc_tran->rx_buf++;
		flash->sfc_tran->finally_len ++;
	}
	if(len == flash->sfc_tran->finally_len){
		print_dbg("recive ok\n");
		//              sfc_flush_fifo(flash);
		flash->sfc_tran->finally_len = flash->sfc_tran->len;
		return flash->sfc_tran->finally_len;
	}
	return -1;
}
static  struct sfc_nor_info * check_cmd(int cmd)
{
	int i = 0;
	for(i = 0;i < jz_nor_info_num ;i++){
		if(jz_sfc_nor_info[i].cmd == cmd){
			return &jz_sfc_nor_info[i];
		}
	}

	return &jz_sfc_nor_info[i - 1];
}

#define SPI_BITS_8		8
#define SPI_BITS_16		16
#define SPI_BITS_24		24
#define SPI_BITS_32		32
#define SPI_8BITS		1
#define SPI_16BITS		2
#define SPI_32BITS		4
#define STATUS_SUSPND    (1<<0)
#define STATUS_BUSY   (1<<1)
#define R_MODE			0x1
#define W_MODE			0x2
#define RW_MODE			(R_MODE | W_MODE)
#define MODEBITS			(SPI_CPOL | SPI_CPHA | SPI_CS_HIGH)
#define SPI_BITS_SUPPORT		(SPI_BITS_8 | SPI_BITS_16 | SPI_BITS_24 | SPI_BITS_32)
#define NUM_CHIPSELECT			8
#define BUS_NUM				0
#define THRESHOLD			31


struct jz_sfc_nand *to_jz_spi_nand(struct mtd_info *mtd_info)
{
	return container_of(mtd_info, struct jz_sfc_nand, mtd);
}
int sfc_nand_plat_resource_init(struct jz_sfc_nand *flash,struct platform_device *pdev)
{
	struct resource *res;
	int err;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "Cannot get IORESOURCE_MEM\n");
		err = -1;
		return err;
	}
	flash->ioarea = request_mem_region(res->start, resource_size(res),pdev->name);
        if (flash->ioarea == NULL) {
		dev_err(&pdev->dev, "Cannot reserve iomem region\n");
		err = -2;
		return err;
	}
	flash->phys = res->start;
	flash->iomem = ioremap(res->start, (res->end - res->start)+1);
	if (flash->iomem == NULL){
		dev_err(&pdev->dev, "Cannot map IO\n");
		err = -3;
		return err;
	}
	flash->irq = platform_get_irq(pdev, 0);
	if (flash->irq <= 0) {
		dev_err(&pdev->dev, "No IRQ specified\n");
		err = -4;
		return err;
	}
	flash->clk = clk_get(&pdev->dev, "cgu_ssi");
	if (IS_ERR(flash->clk)) {
		dev_err(&pdev->dev, "Cannot get ssi clock\n");
		err=-5;
		return err;
	}
	flash->clk_gate = clk_get(&pdev->dev, "sfc");
	if (IS_ERR(flash->clk_gate)) {
		dev_err(&pdev->dev, "Cannot get sfc clock\n");
		err=-6;
		return err;
	}
        res = platform_get_resource(pdev, IORESOURCE_BUS, 0);
        if (res == NULL) {
                dev_err(&pdev->dev, "Cannot get IORESOURCE_BUS\n");
                err = -7;
		return err;
        }

       flash->src_clk = res->start * 1000000;

        if (clk_get_rate(flash->clk) >= flash->src_clk) {
                clk_set_rate(flash->clk, flash->src_clk);
        } else {
                clk_set_rate(flash->clk, flash->src_clk);
        }

        clk_enable(flash->clk);
        clk_enable(flash->clk_gate);

        platform_set_drvdata(pdev, flash);
	return 0;
}
static int sfc_pio_transfer(struct jz_sfc_nand *flash)
{
	print_dbg("function : %s, line : %d\n", __func__, __LINE__);
	sfc_flush_fifo(flash);
	sfc_set_length(flash, flash->sfc_tran->len);
	/*use one phase for transfer*/
	sfc_set_addr_length(flash, 0, flash->sfc_tran->sfc_cmd.addr_len);//
	sfc_cmd_en(flash, 0, 0x1);
	sfc_write_cmd(flash, 0, flash->sfc_tran->sfc_cmd.cmd);
	sfc_data_en(flash, 0, flash->sfc_tran->date_en);
	sfc_dev_addr(flash, 0, flash->sfc_tran->sfc_cmd.addr_low);
	sfc_dev_addr_plus(flash, 0, flash->sfc_tran->sfc_cmd.addr_high);
	sfc_dev_addr_dummy_bytes(flash,0,flash->sfc_tran->sfc_cmd.dummy_byte);
	//sfc_dev_pollen(flash,0,flash->sfc_tran->pollen);
	if(flash->sfc_tran->rw_mode & R_MODE)
	{
		if((flash->use_dma ==1)&&(flash->sfc_tran->dma_mode)){
			dma_cache_sync(NULL, (void *)(flash->sfc_tran->rx_buf), flash->sfc_tran->len, DMA_FROM_DEVICE);
			sfc_set_mem_addr(flash, GET_PHYADDR(flash->sfc_tran->rx_buf));
			sfc_transfer_mode(flash, DMA_MODE);
		}else{
			sfc_transfer_mode(flash, SLAVE_MODE);
		}
		sfc_transfer_direction(flash, GLB_TRAN_DIR_READ);

	}else if(flash->sfc_tran->rw_mode & W_MODE)
	{
		if((flash->use_dma ==1)&&(flash->sfc_tran->dma_mode)){
			dma_cache_sync(NULL, (void *)(flash->sfc_tran->tx_buf), flash->sfc_tran->len,DMA_TO_DEVICE);
			sfc_set_mem_addr(flash, GET_PHYADDR(flash->sfc_tran->tx_buf));
			sfc_transfer_mode(flash, DMA_MODE);
		}else{
			sfc_transfer_mode(flash, SLAVE_MODE);
		}
		sfc_transfer_direction(flash, GLB_TRAN_DIR_WRITE);
	}
	else {
		sfc_transfer_mode(flash, SLAVE_MODE);
		sfc_transfer_direction(flash, GLB_TRAN_DIR_WRITE);
	}
	sfc_mode(flash,0,flash->sfc_tran->sfc_cmd.transmode);   //现好使再说一会再提控制器I/O传输模式
/*	if(flash->nor_info->pollen){
		if(flash->nor_info->cmd == CMD_RDSR){
			//sfc_set_length(flash, 0);
			sfc_set_length(flash, 0);
			sfc_dev_sta_exp(flash,0);
			sfc_dev_sta_msk(flash,0x1);
		}
		if(flash->nor_info->cmd == CMD_RDSR_1){
			//sfc_set_length(flash, 0);
			sfc_set_length(flash, 0);
			sfc_dev_sta_exp(flash,2);
			sfc_dev_sta_msk(flash,0x2);
		}
	}
*/
	sfc_enable_all_intc(flash);
	sfc_start(flash);
	return 0;
}
static int jz_sfc_pio_txrx(struct jz_sfc_nand *flash, struct sfc_transfer_nand *t)
{
	int ret;
	unsigned long flags;
	int tmp_cmd = 0;
	int err = 0;
	print_dbg("function : %s, line : %d\n", __func__, __LINE__);
	if((t->tx_buf == NULL) && (t->rx_buf == NULL)&&(t->date_en)) {
		dev_info(flash->dev, "the tx and rx buf of spi_transfer is NULL !\n");
		return 0;
	}
	if((!t->rx_buf)&&(t->rw_mode==R_MODE)){
		printk("please check the read buf is NULL\n");
		return 0;
	}
	if((t->tx_buf==NULL)&&(t->rw_mode==W_MODE))
	{
		printk("please check the write buf is NULL\n");
		return 0;
	}
	spin_lock_irqsave(&flash->lock_rxtx, flags);
	flash->sfc_tran=t;
	ret = sfc_pio_transfer(flash);
	if (ret < 0) {
			dev_err(flash->dev,"data transfer error!,please check the cmd,and the driver do not support spi nand      flash\n");
			sfc_mask_all_intc(flash);
			sfc_clear_all_intc(flash);
			spin_unlock_irqrestore(&flash->lock_rxtx, flags);
			return ret;
	}
	spin_unlock_irqrestore(&flash->lock_rxtx, flags);
		//dump_sfc_reg(flash);
        err = wait_for_completion_timeout(&flash->done,10*HZ);
        if (!err) {
                dump_sfc_reg(flash);
                dev_err(flash->dev, "Timeout for ACK from SFC device\n");
                return -ETIMEDOUT;
        }
    /*fix the cache line problem,when use jffs2 filesystem must be flush cache twice*/
        if(flash->sfc_tran->rw_mode & R_MODE)
                dma_cache_sync(NULL, (void *)flash->sfc_tran->rx_buf, flash->sfc_tran->len, DMA_FROM_DEVICE);

        if(flash->use_dma == 1)
	{
                flash->sfc_tran->finally_len=flash->sfc_tran->len;
		return 0;
	}
        if(flash->sfc_tran->finally_len != flash->sfc_tran->len){
             printk("the length is not mach,flash->rlen = %d,flash->len = %d,return !\n",flash->sfc_tran->finally_len,flash->sfc_tran->len);
                if(flash->sfc_tran->finally_len < flash->sfc_tran->len)
                            flash->sfc_tran->finally_len = flash->sfc_tran->len;
                else
                        return EINVAL;

        }
        return 0;
}

static irqreturn_t jz_sfc_pio_irq_callback(struct jz_sfc_nand *flash)
{
	print_dbg("function : %s, line : %d\n", __func__, __LINE__);
	if (ssi_underrun(flash)) {
		sfc_clear_under_intc(flash);
		dev_err(flash->dev, "sfc UNDR !\n");
		complete(&flash->done);
		return IRQ_HANDLED;
	}
	if (ssi_overrun(flash)) {
		sfc_clear_over_intc(flash);
		dev_err(flash->dev, "sfc OVER !\n");
		complete(&flash->done);
		return IRQ_HANDLED;
	}
	if (rxfifo_rreq(flash)) {
		sfc_clear_rreq_intc(flash);
		//flush_work(&flash->rw_work);
		//queue_work(flash->workqueue, &flash->rw_work);
		cpu_read_rxfifo(flash);
		return IRQ_HANDLED;
	}
	if (txfifo_treq(flash)) {
		sfc_clear_treq_intc(flash);
		//flush_work(&flash->rw_work);
		//queue_work(flash->workqueue, &flash->rw_work);
		cpu_write_txfifo(flash);
		return IRQ_HANDLED;
	}
	if(sfc_end(flash)){
		sfc_clear_end_intc(flash);
		/*this is a bug that the sfc_real_time_register
		* not be the value we want
		* to get , so we have to
		* fource a value to the spi device driver
		*
		* */
	/*if(flash->nor_info->pollen){
		if(flash->rx){
			if(flash->nor_info->cmd == CMD_RDSR)
				*(flash->rx) = 0x0;
			if(flash->nor_info->cmd == CMD_RDSR_1)
				*(flash->rx) = 0x2;
				flash->sfc_transfer_nand->finally_len = flash->sfc_transfer_nand->len;
		}
	}
*/
	complete(&flash->done);
	return IRQ_HANDLED;
	}
/*err_no_clk:
        clk_put(flash->clk_gate);
        clk_put(flash->clk);
err_no_irq:
        free_irq(flash->irq, flash);
err_no_iomap:
        iounmap(flash->iomem);
err_no_iores:
err_no_pdata:
        release_resource(flash->ioarea);
        kfree(flash->ioarea);
        //return err;
*/
}
static irqreturn_t jz_sfc_irq(int irq, void *dev)
{
        struct jz_sfc_nand *flash = dev;

        //printk("function : %s, line : %d\n", __func__, __LINE__);

        return flash->irq_callback(flash);
}
struct jz_spi_support *jz_sfc_nand_probe(struct jz_sfc_nand *flash)
{
	int ret,i;
	unsigned int id = 0;
	unsigned char rx_chipid[4];
	struct sfc_transfer_nand transfer[1];
	struct jz_spi_support *params;
	struct jz_spi_support *jz_spi_nand_support_table;
	mutex_lock(&flash->lock);
	int num_spi_flash=((struct jz_spi_nand_platform_data *)(flash->pdata->board_info))->num_spi_flash;
	jz_spi_nand_support_table=((struct jz_spi_nand_platform_data *)(flash->pdata->board_info))->jz_spi_support;
	transfer[0].sfc_cmd.cmd=CMD_RDID;
	transfer[0].sfc_cmd.addr_low=0;
	transfer[0].sfc_cmd.addr_high=0;
	transfer[0].sfc_cmd.dummy_byte=0;
	transfer[0].sfc_cmd.addr_len=1;
	transfer[0].sfc_cmd.cmd_len=1;
	transfer[0].sfc_cmd.transmode=0;
	flash->use_dma=1;

	transfer[0].tx_buf  = NULL;
	transfer[0].rx_buf =  rx_chipid;
	transfer[0].len=2;
	transfer[0].date_en=1;
	transfer[0].dma_mode=DMA_MODE;
	transfer[0].pollen=0;
	transfer[0].rw_mode=R_MODE;
	transfer[0].sfc_mode=0;
	jz_sfc_pio_txrx(flash,transfer);
	printk("-----------read chip id------------\n");
	for (i = 0; i < num_spi_flash; i++) {
		params = &jz_spi_nand_support_table[i];
		if ( (params->id_manufactory == rx_chipid[0]) && (params->id_device == rx_chipid[1]) )
			break;
	}
	if (i >= num_spi_flash) {
		printk("ingenic: Unsupported ID %02x,%02x\n", rx_chipid[0],rx_chipid[1]);
		mutex_unlock(&flash->lock);
		return NULL;
	}
	mutex_unlock(&flash->lock);
	return params;
}
static int jz_sfc_init_setup(struct jz_sfc_nand *flash)
{
	sfc_init(flash);
	sfc_stop(flash);
	/*set hold high*/
	sfc_hold_invalid_value(flash, 1);
	 /*set wp high*/
	sfc_wp_invalid_value(flash, 1);
	sfc_clear_all_intc(flash);
	sfc_mask_all_intc(flash);

	sfc_threshold(flash, flash->threshold);
	/*config the sfc pin init state*/
	sfc_clock_phase(flash, 0);
	sfc_clock_polarity(flash, 0);
	sfc_ce_invalid_value(flash, 1);
	sfc_transfer_mode(flash, SLAVE_MODE);
	if(flash->src_clk >= 100000000){
		//              printk("############## the cpm = %x\n",*(volatile unsigned int*)(0xb0000074));
		sfc_smp_delay(flash,DEV_CONF_HALF_CYCLE_DELAY);
	}
	flash->swap_buf = kmalloc(SWAP_BUF_SIZE + PAGE_SIZE,GFP_KERNEL);
	if(flash->swap_buf == NULL){
		dev_err(flash->dev,"alloc mem error\n");
		return ENOMEM;
	}
#if defined(CONFIG_SFC_DMA)
	flash->use_dma = 1;
#endif
	flash->irq_callback = &jz_sfc_pio_irq_callback;
	return 0;
}
static int jz_sfc_nandflash_write_enable(struct jz_sfc_nand *flash)
{

        int ret,i;
        unsigned int id = 0;
        unsigned char rx_chipid[4];
        struct sfc_transfer_nand transfer[1];
        transfer[0].sfc_cmd.cmd=SPINAND_CMD_WREN;
        transfer[0].sfc_cmd.addr_low=0;
        transfer[0].sfc_cmd.addr_high=0;
        transfer[0].sfc_cmd.dummy_byte=0;
        transfer[0].sfc_cmd.addr_len=0;
        transfer[0].sfc_cmd.cmd_len=1;
        transfer[0].sfc_cmd.transmode=0;

	transfer[0].dma_mode=0;
        transfer[0].tx_buf  = NULL;
        transfer[0].rx_buf =  NULL;
        transfer[0].len=0;
        transfer[0].date_en=0;
        transfer[0].dma_mode=SLAVE_MODE;
        transfer[0].pollen=0;
        transfer[0].rw_mode=0;
        transfer[0].sfc_mode=0;
        jz_sfc_pio_txrx(flash,transfer);
	return 0;
}
static int jz_sfc_nandflash_get_status(struct jz_sfc_nand *flash)
{
        int ret;
        unsigned int id = 0;
        unsigned char rx_chipid[4];
        struct sfc_transfer_nand transfer[1];
	u8 status;
        transfer[0].sfc_cmd.cmd=SPINAND_CMD_GET_FEATURE;
        transfer[0].sfc_cmd.addr_low=SPINAND_FEATURE_ADDR;
        transfer[0].sfc_cmd.addr_high=0;
        transfer[0].sfc_cmd.dummy_byte=0;
        transfer[0].sfc_cmd.addr_len=1;
        transfer[0].sfc_cmd.cmd_len=1;
        transfer[0].sfc_cmd.transmode=0;
        flash->use_dma=1;

        transfer[0].tx_buf  = NULL;
        transfer[0].rx_buf =  &status;
        transfer[0].len=1;
        transfer[0].date_en=1;
        transfer[0].dma_mode=DMA_MODE;
        transfer[0].pollen=0;
        transfer[0].rw_mode=R_MODE;
        transfer[0].sfc_mode=0;
        transfer[0].finally_len=0;
        jz_sfc_pio_txrx(flash,transfer);

	ret=flash->sfc_tran->finally_len;
        if(ret != transfer[0].len){
                dev_err(flash->dev,"the transfer length is error,%d,%s\n",__LINE__,__func__);
		return -1;
	}
	return status;
}
static int jz_sfc_nandflash_erase_blk(struct jz_sfc_nand *flash,uint32_t addr)
{
	 int ret;
        struct sfc_transfer_nand transfer[1];
        int page = addr / flash->mtd.writesize;
        int timeout = 2000;

        ret = jz_sfc_nandflash_write_enable(flash);
	if(ret)
		return ret;
	switch(flash->mtd.erasesize){
		case SPINAND_OP_BL_128K:
			transfer[0].sfc_cmd.cmd = SPINAND_CMD_ERASE_128K;
			break;
		default:
			pr_info("Don't support the blksize to erase ! \n");
			break;
	}

        transfer[0].sfc_cmd.addr_low=page;
        transfer[0].sfc_cmd.addr_high=0;
        transfer[0].sfc_cmd.dummy_byte=0;
        transfer[0].sfc_cmd.addr_len=3;
        transfer[0].sfc_cmd.cmd_len=1;
        transfer[0].sfc_cmd.transmode=0;
        flash->use_dma=1;

        transfer[0].tx_buf  = NULL;
        transfer[0].rx_buf =  NULL;
        transfer[0].len=0;
        transfer[0].date_en=0;
        transfer[0].dma_mode=SLAVE_MODE;
        transfer[0].pollen=0;
        transfer[0].rw_mode=0;
        transfer[0].sfc_mode=0;
        jz_sfc_pio_txrx(flash,transfer);
//	msleep((flash->tBERS + 999) / 1000);
        do{
                ret = jz_sfc_nandflash_get_status(flash);
                timeout--;
        }while((ret & SPINAND_IS_BUSY) && (timeout > 0));
        if(ret & E_FALI){
                pr_info("Erase error,get state error ! %s %s %d \n",__FILE__,__func__,__LINE__);
                return -1;
        }

        return 0;

}
static int jz_sfc_nandflash_erase(struct mtd_info *mtd, struct erase_info *instr)
{
        int ret,i;
        uint32_t addr, end;
        int check_addr;
        struct jz_sfc_nand *flash;
	flash=to_jz_spi_nand(mtd);
        mutex_lock(&flash->lock);
        check_addr = ((unsigned int)instr->addr) % (mtd->erasesize);
        if (check_addr) {
                pr_info("%s line %d eraseaddr no align\n", __func__,__LINE__);
		mutex_unlock(&flash->lock);
                return -EINVAL;
        }
        addr = (uint32_t)instr->addr;
        end = addr + (uint32_t)instr->len;
        if(end > mtd->size){
                pr_info("ERROR: the erase addr over the spi_nand size !!! %s %s %d \n",__FILE__,__func__,__LINE__);
                return -EINVAL;
        }
        while (addr < end) {
                ret = jz_sfc_nandflash_erase_blk(flash, addr);
                if (ret) {
                        pr_info("spi nand erase error blk id  %d !\n",addr / mtd->erasesize);
			mutex_unlock(&flash->lock);
                        instr->state = MTD_ERASE_FAILED;
                }

                addr += mtd->erasesize;
        }
        mutex_unlock(&flash->lock);

        instr->state = MTD_ERASE_DONE;
        mtd_erase_callback(instr);

	return 0;
}
size_t jz_sfc_nandflash_read_ops(struct jz_sfc_nand *flash,u_char *buffer,int page, int column,size_t rlen,size_t *rel_rlen)
{
        int ret,timeout = 2000;
	struct sfc_transfer_nand transfer[1];

	memset(transfer,0,sizeof(struct sfc_transfer_nand));
	transfer[0].sfc_cmd.cmd=SPINAND_CMD_PARD;
	transfer[0].sfc_cmd.addr_low=page;
	transfer[0].sfc_cmd.addr_high=0;
	transfer[0].sfc_cmd.dummy_byte=0;
	transfer[0].sfc_cmd.addr_len=3;
	transfer[0].sfc_cmd.cmd_len=1;
	transfer[0].sfc_cmd.transmode=0;
	transfer[0].finally_len=0;

	transfer[0].tx_buf = NULL;
	transfer[0].rx_buf = NULL;
	transfer[0].len=0;
	transfer[0].date_en=0;
	transfer[0].dma_mode=SLAVE_MODE;
	transfer[0].pollen=0;
	transfer[0].rw_mode=0;
	transfer[0].sfc_mode=0;
	jz_sfc_pio_txrx(flash,transfer);
//	udelay(flash->tRD);
        do{
                ret = jz_sfc_nandflash_get_status(flash);
                timeout--;
        }while((ret & SPINAND_IS_BUSY) && (timeout > 0));
        if((ret & 0x30) == 0x20) {
		mutex_unlock(&flash->lock);
                pr_info("spi nand read error page %d column = %d ret = %02x !!! %s %s %d \n",page,column,ret,__FILE__,__func__,__LINE__);
                return -EBADMSG;
        }
        switch(flash->column_cmdaddr_bits){
                case 24:
			memset(transfer,0,sizeof(struct sfc_transfer_nand));
			transfer[0].sfc_cmd.cmd=SPINAND_CMD_RDCH;
			transfer[0].sfc_cmd.addr_low=(column<<8)&0xffffff00;
			transfer[0].sfc_cmd.addr_len=3;

			break;
                case 32:
                        transfer[0].sfc_cmd.cmd = SPINAND_CMD_FRCH;//SPINAND_CMD_RDCH;/* SPINAND_CMD_RDCH read odd addr may be error */
			transfer[0].sfc_cmd.addr_low=(column<<8)&0xffffff00;
			transfer[0].sfc_cmd.addr_len=4;
                        break;
                default:
                        pr_info("can't support the format of column addr ops !!\n");
                        break;
        }
        transfer[0].sfc_cmd.dummy_byte=0;
        transfer[0].sfc_cmd.cmd_len=1;
        transfer[0].sfc_cmd.transmode=0;
        transfer[0].finally_len=0;

        transfer[0].tx_buf = NULL;
        transfer[0].rx_buf = buffer;
        transfer[0].len=rlen;
        transfer[0].date_en=1;
        transfer[0].dma_mode=DMA_MODE;
        transfer[0].pollen=0;
        transfer[0].rw_mode=R_MODE;
	transfer[0].sfc_mode=0;

	flash->sfc_tran->finally_len=jz_sfc_pio_txrx(flash,transfer);
	*rel_rlen=flash->sfc_tran->finally_len;
        return 0;
}
static int jz_sfc_nandflash_read(struct mtd_info *mtd,loff_t addr,int column,size_t len,u_char *buf,size_t *retlen)
{
	int ret,read_num,i,rlen,page=0;
        int page_size = mtd->writesize;
        struct jz_sfc_nand *flash;
        u_char *buffer = buf;
        size_t page_overlength;
        size_t ops_addr;
        size_t ops_len,rel_rlen = 0;
        if(addr > mtd->size){
                pr_info("ERROR:the read addr over the spi_nand size !!! %s %s %d \n",__FILE__,__func__,__LINE__);
                return -EINVAL;
        }
	flash=to_jz_spi_nand(mtd);
        mutex_lock(&flash->lock);


        if(column){
                ops_addr = (unsigned int)addr;
                ops_len = len;
                if(len <= (page_size - column))
                        page_overlength = len;
                else
                        page_overlength = page_size - column;/*random read but len over a page */
                while(ops_addr < addr + len){
                        page = ops_addr / page_size;
                        if(page_overlength){
                                ret = jz_sfc_nandflash_read_ops(flash,buffer,page,column,page_overlength,&rel_rlen);
				if(ret<0)
				{
					mutex_unlock(&flash->lock);
					return ret;
				}
                                ops_len -= page_overlength;
                                buffer += page_overlength;
                                ops_addr += page_overlength;
                                page_overlength = 0;
                        }else{
                                column = 0;
                                if(ops_len >= page_size)
                                        rlen = page_size;
                                else
                                        rlen = ops_len;
                                ret = jz_sfc_nandflash_read_ops(flash,buffer,page,column,rlen,&rel_rlen);
				if(ret<0)
				{
					mutex_unlock(&flash->lock);
					return ret;
				}
                                buffer += rlen;
                                ops_len -= rlen;
                                ops_addr += rlen;
                        }
                        *retlen += rel_rlen;
                }
        }else{
			read_num = (len + page_size - 1) / page_size;
                page = ((unsigned int)addr) / mtd->writesize;
                for(i = 0; i < read_num; i++){
                        if(len >= page_size)
                                rlen = page_size;
                        else
                                rlen = len;
                        ret = jz_sfc_nandflash_read_ops(flash,buffer,page,column,rlen,&rel_rlen);
			if(ret<0)
			{
				mutex_unlock(&flash->lock);
				return ret;
			}
                        buffer += rlen;
                        len -= rlen;
                        page++;
                        *retlen += rel_rlen;
                }
        }
        mutex_unlock(&flash->lock);
        return ret;
}
static size_t jz_sfc_nandflash_write(struct mtd_info *mtd,loff_t addr,int column,size_t len,u_char *buf)
{
	size_t retlen = 0;
        int write_num,page_size,wlen = 0,i;
        struct jz_sfc_nand *flash;
        int page = ((unsigned int )addr) / mtd->writesize;
        u_char *buffer = buf;
	int ret,timeout = 2000;
	struct sfc_transfer_nand transfer[1];
	if(addr & (mtd->writesize - 1)){
                pr_info("wirte add don't align ,error ! %s %s %d \n",__FILE__,__func__,__LINE__);
                return -EINVAL;
        }

        if(addr > mtd->size){
                pr_info("ERROR:the Write addr over the spi_nand size !!! %s %s %d \n",__FILE__,__func__,__LINE__);
                return -EINVAL;
        }
	flash = to_jz_spi_nand(mtd);
        mutex_lock(&flash->lock);

        page_size = mtd->writesize;
        write_num = (len + page_size - 1) / (page_size);

        for(i = 0; i < write_num; i++){

                if(len >= page_size)
                        wlen = page_size;
                else
                        wlen = len;

        memset(transfer,0,sizeof(struct sfc_transfer_nand));
        transfer[0].sfc_cmd.cmd=SPINAND_CMD_PRO_LOAD;
        transfer[0].sfc_cmd.addr_low=column;
        transfer[0].sfc_cmd.addr_high=0;
        transfer[0].sfc_cmd.dummy_byte=0;
        transfer[0].sfc_cmd.addr_len=2;
        transfer[0].sfc_cmd.cmd_len=1;
        transfer[0].sfc_cmd.transmode=0;
        transfer[0].finally_len=0;

        transfer[0].tx_buf = buffer;
        transfer[0].rx_buf = NULL;
        transfer[0].len=wlen;
        transfer[0].date_en=1;
        transfer[0].dma_mode=DMA_MODE;
        transfer[0].pollen=0;
        transfer[0].rw_mode=W_MODE;
        transfer[0].sfc_mode=0;
	ret=jz_sfc_pio_txrx(flash,transfer);
	retlen += wlen;/* delete the len of command */
	jz_sfc_nandflash_write_enable(flash);

        memset(transfer,0,sizeof(struct sfc_transfer_nand));
        transfer[0].sfc_cmd.cmd=SPINAND_CMD_PRO_EN;
        transfer[0].sfc_cmd.addr_low=page;
        transfer[0].sfc_cmd.addr_high=0;
        transfer[0].sfc_cmd.dummy_byte=0;
        transfer[0].sfc_cmd.addr_len=3;
        transfer[0].sfc_cmd.cmd_len=1;
        transfer[0].sfc_cmd.transmode=0;
        transfer[0].finally_len=0;

        transfer[0].tx_buf = NULL;
        transfer[0].rx_buf = NULL;
        transfer[0].len=0;
        transfer[0].date_en=0;
        transfer[0].dma_mode=SLAVE_MODE;
        transfer[0].pollen=0;
        transfer[0].rw_mode=0;
        transfer[0].sfc_mode=0;
	jz_sfc_pio_txrx(flash,transfer);
//	udelay(flash->tPROG);
        do{
		ret = jz_sfc_nandflash_get_status(flash);
                timeout--;
        }while((ret & SPINAND_IS_BUSY) && (timeout > 0));
	if(ret & p_FAIL){
		pr_info("spi nand write fail %s %s %d\n",__FILE__,__func__,__LINE__);
		mutex_unlock(&flash->lock);
		return (len - retlen);
	}
                len -= wlen;
                buffer += wlen;
                page++;
        }
        mutex_unlock(&flash->lock);
        return retlen;
}
static int jz_sfcnand_read_oob(struct mtd_info *mtd,loff_t addr,struct mtd_oob_ops *ops)
{
        struct jz_sfc_nand *flash;
        int column = mtd->writesize;
        int page = (unsigned int)addr / mtd->writesize;
        struct sfc_transfer_nand transfer[1];
        int ret,timeout = 2000;

	flash = to_jz_spi_nand(mtd);
	mutex_lock(&flash->lock);
	memset(transfer,0,sizeof(struct sfc_transfer_nand));
	transfer[0].sfc_cmd.cmd=SPINAND_CMD_PARD;
	transfer[0].sfc_cmd.addr_low=page;
	transfer[0].sfc_cmd.addr_high=0;
	transfer[0].sfc_cmd.dummy_byte=0;
	transfer[0].sfc_cmd.addr_len=3;
	transfer[0].sfc_cmd.cmd_len=1;
	transfer[0].sfc_cmd.transmode=0;
	transfer[0].finally_len=0;

	transfer[0].tx_buf = NULL;
	transfer[0].rx_buf = NULL;
	transfer[0].len=0;
	transfer[0].date_en=0;
	transfer[0].dma_mode=SLAVE_MODE;
	transfer[0].pollen=0;
	transfer[0].rw_mode=0;
	transfer[0].sfc_mode=0;
	jz_sfc_pio_txrx(flash,transfer);

 //       udelay(flash->tRD);
        do{
                ret = jz_sfc_nandflash_get_status(flash);
                timeout--;
        }while((ret & SPINAND_IS_BUSY) && (timeout > 0));

        if((ret & 0x30) == 0x20) {
                pr_info("spi nand read error page %d column = %d ret = %02x !!! %s %s %d \n",page,column,ret,__FILE__,__func__,__LINE__);
                memset(ops->oobbuf,0x0,ops->ooblen);
                mutex_unlock(&flash->lock);
                return -EBADMSG;
        }
        memset(transfer,0,sizeof(struct sfc_transfer_nand));
        switch(flash->column_cmdaddr_bits){
                case 24:
			transfer[0].sfc_cmd.cmd=SPINAND_CMD_RDCH;
			transfer[0].sfc_cmd.addr_len=3;

                        break;
                case 32:
			transfer[0].sfc_cmd.cmd=SPINAND_CMD_FRCH;
			transfer[0].sfc_cmd.addr_len=4;


                        break;
                default:
                        pr_info("can't support the format of column addr ops !!\n");
                        break;
        }
        transfer[0].sfc_cmd.addr_low=(column<<8)& 0xffffff00;
        transfer[0].sfc_cmd.addr_high=0;
        transfer[0].sfc_cmd.dummy_byte=0;
        transfer[0].sfc_cmd.cmd_len=1;
        transfer[0].sfc_cmd.transmode=0;
        transfer[0].finally_len=0;
	transfer[0].tx_buf = NULL;
        transfer[0].rx_buf = ops->oobbuf;
        transfer[0].len=ops->ooblen;
        transfer[0].date_en=1;
        transfer[0].dma_mode=DMA_MODE;
        transfer[0].pollen=0;
        transfer[0].rw_mode=R_MODE;
        transfer[0].sfc_mode=0;
	ret=jz_sfc_pio_txrx(flash,transfer);
	ops->retlen=ret;
	mutex_unlock(&flash->lock);
        return ret;


}
static int jz_sfcnand_write_oob(struct mtd_info *mtd,loff_t addr,struct mtd_oob_ops *ops)
{
        struct spi_message message;
        struct sfc_transfer_nand transfer[1];
        struct jz_sfc_nand *flash;
        int ret,timeout=2000;
        int column = mtd->writesize;
        int page = ((unsigned int)addr) / mtd->writesize;
        flash = to_jz_spi_nand(mtd);

        mutex_lock(&flash->lock);
        memset(transfer,0,sizeof(struct sfc_transfer_nand));

        transfer[0].sfc_cmd.cmd=SPINAND_CMD_PARD;
        transfer[0].sfc_cmd.addr_low=page;
        transfer[0].sfc_cmd.addr_high=0;
        transfer[0].sfc_cmd.dummy_byte=0;
        transfer[0].sfc_cmd.addr_len=3;
        transfer[0].sfc_cmd.cmd_len=1;
        transfer[0].sfc_cmd.transmode=0;
        transfer[0].finally_len=0;

        transfer[0].tx_buf = NULL;
        transfer[0].rx_buf = NULL;
        transfer[0].len=0;
        transfer[0].date_en=0;
        transfer[0].dma_mode=SLAVE_MODE;
        transfer[0].pollen=0;
        transfer[0].rw_mode=0;
        transfer[0].sfc_mode=0;
        jz_sfc_pio_txrx(flash,transfer);

//	udelay(flash->tRD);
        memset(transfer,0,sizeof(struct sfc_transfer_nand));

        transfer[0].sfc_cmd.cmd=SPINAND_CMD_PLRd;
        transfer[0].sfc_cmd.addr_low=column;
        transfer[0].sfc_cmd.addr_high=0;
        transfer[0].sfc_cmd.dummy_byte=0;
        transfer[0].sfc_cmd.addr_len=2;
        transfer[0].sfc_cmd.cmd_len=1;
        transfer[0].sfc_cmd.transmode=0;
        transfer[0].finally_len=0;

        transfer[0].tx_buf = ops->oobbuf;
        transfer[0].rx_buf = NULL;
        transfer[0].len=ops->ooblen;
        transfer[0].date_en=1;
        transfer[0].dma_mode=DMA_MODE;
        transfer[0].pollen=0;
        transfer[0].rw_mode=W_MODE;
        transfer[0].sfc_mode=0;
        jz_sfc_pio_txrx(flash,transfer);

        jz_sfc_nandflash_write_enable(flash);

	transfer[0].sfc_cmd.cmd=SPINAND_CMD_PRO_EN;
        transfer[0].sfc_cmd.addr_low=page;
        transfer[0].sfc_cmd.addr_high=0;
        transfer[0].sfc_cmd.dummy_byte=0;
        transfer[0].sfc_cmd.addr_len=3;
        transfer[0].sfc_cmd.cmd_len=1;
        transfer[0].sfc_cmd.transmode=0;
        transfer[0].finally_len=0;

        transfer[0].tx_buf = NULL;
        transfer[0].rx_buf = NULL;
        transfer[0].len=0;
        transfer[0].date_en=0;
        transfer[0].dma_mode=SLAVE_MODE;
        transfer[0].pollen=0;
        transfer[0].rw_mode=0;
        transfer[0].sfc_mode=0;
        jz_sfc_pio_txrx(flash,transfer);

        udelay(flash->tPROG);
        do{
		ret = jz_sfc_nandflash_get_status(flash);
		timeout--;
        }while((ret & SPINAND_IS_BUSY) && (timeout > 0));
		if(ret & p_FAIL){
                	pr_info("spi nand write fail %s %s %d\n",__FILE__,__func__,__LINE__);
			mutex_unlock(&flash->lock);
                	return -EIO;
		}
	ops->retlen=ret;
        mutex_unlock(&flash->lock);
        return 0;
}

static int jz_sfcnand_read(struct mtd_info *mtd, loff_t from,size_t len, size_t *retlen, unsigned char *buf)
{
        int ret;
        size_t column = ((unsigned int)from) % mtd->writesize;
        ret = jz_sfc_nandflash_read(mtd,from,column,len,buf,retlen);
        return ret;
}
static int jz_sfcnand_write(struct mtd_info *mtd, loff_t to, size_t len,size_t *retlen, u_char *buf)
{
        size_t ret;
        size_t column = ((unsigned int)to) % mtd->writesize;
        ret = jz_sfc_nandflash_write(mtd,to,column,len,buf);

        *retlen = ret;
        return 0;
}

static int jz_sfc_nandflash_block_checkbad(struct mtd_info *mtd, loff_t ofs,int getchip,int allowbbt)
{
        struct nand_chip *chip = mtd->priv;
        if (!chip->bbt)
                return chip->block_bad(mtd, ofs,getchip);
        /* Return info from the table */
        return nand_isbad_bbt(mtd, ofs, allowbbt);
}
static int jz_sfcnand_block_isbab(struct mtd_info *mtd,loff_t ofs)
{
	int ret;
	ret = jz_sfc_nandflash_block_checkbad(mtd, ofs,1, 0);
	return ret;
}
static int jz_sfcnand_block_markbad(struct mtd_info *mtd,loff_t ofs)
{
        struct nand_chip *chip = mtd->priv;
        int ret;
        pr_info("jz_sfcnand_block_markbad\n");
        ret = jz_sfcnand_block_isbab(mtd, ofs);
        if (ret) {
                /* If it was bad already, return success and do nothing */
                if (ret > 0)
                        return 0;
                return ret;
        }
        return chip->block_markbad(mtd, ofs);
}
static int badblk_check(int len,unsigned char *buf)
{
         int i,bit0_cnt = 0;
         unsigned short *check_buf = (unsigned short *)buf;

         if(check_buf[0] != 0xff){
                 for(i = 0; i < len * 8; i++){
                         if(!((check_buf[0] >> 1) & 0x1))
                                 bit0_cnt++;
                 }
         }
         if(bit0_cnt > 6 * len)
                 return 1; // is bad blk
         return 0;
}
static int jz_sfcnand_block_bad_check(struct mtd_info *mtd, loff_t ofs,int getchip)
{
	int check_len = 2;
	unsigned char check_buf[] = {0xaa,0xaa};
	struct mtd_oob_ops ops;
	ops.oobbuf = check_buf;
	ops.ooblen = check_len;
	jz_sfcnand_read_oob(mtd,ofs,&ops);
	if(badblk_check(check_len,check_buf))
		return 1;
	return 0;
}
static int jz_sfcnand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
        int addr,ret;
        addr = instr->addr;

        ret = jz_sfc_nandflash_erase(mtd,instr);
        if(ret){
                pr_info("WARNING: block %d erase fail !\n",addr / mtd->erasesize);

                ret = jz_sfcnand_block_markbad(mtd,addr);
                if(ret){
                        pr_info("mark bad block error, there will occur error,so exit !\n");
                        return -1;
                }
        }
        instr->state = MTD_ERASE_DONE;
        return 0;
}
static int jz_sfc_nandflash_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd->priv;
        uint8_t buf[2] = { 0, 0 };
        int  res, ret = 0, i = 0;
        int write_oob = !(chip->bbt_options & NAND_BBT_NO_OOB_BBM);

        /* Write bad block marker to OOB */
        if (write_oob) {
                struct mtd_oob_ops ops;
                loff_t wr_ofs = ofs;
                ops.datbuf = NULL;
                ops.oobbuf = buf;
                ops.ooboffs = chip->badblockpos;
                if (chip->options & NAND_BUSWIDTH_16) {
                        ops.ooboffs &= ~0x01;
                        ops.len = ops.ooblen = 2;
                } else {
                        ops.len = ops.ooblen = 1;
                }
                ops.mode = MTD_OPS_PLACE_OOB;

                /* Write to first/last page(s) if necessary */
                if (chip->bbt_options & NAND_BBT_SCANLASTPAGE)
                        wr_ofs += mtd->erasesize - mtd->writesize;
                do {
                        res = jz_sfcnand_write_oob(mtd, wr_ofs, &ops);
                        if (!ret)
                                wr_ofs += mtd->writesize;
                } while ((chip->bbt_options & NAND_BBT_SCAN2NDPAGE) && i < 2);
        }
        /* Update flash-based bad block table */
        if (chip->bbt_options & NAND_BBT_USE_FLASH) {
                res = nand_update_bbt(mtd, ofs);
                if (!ret)
                        ret = res;
        }
        if (!ret)
                mtd->ecc_stats.badblocks++;

        return ret;
}
static int jz_sfc_nand_ext_init(struct jz_sfc_nand *flash)
{
        int ret,i;
        struct sfc_transfer_nand transfer[1];
        transfer[0].sfc_cmd.cmd=0x1f;
        transfer[0].sfc_cmd.addr_low=0xb010;
        transfer[0].sfc_cmd.addr_high=0;
        transfer[0].sfc_cmd.dummy_byte=0;
        transfer[0].sfc_cmd.addr_len=2;
        transfer[0].sfc_cmd.cmd_len=1;
        transfer[0].sfc_cmd.transmode=0;

        transfer[0].dma_mode=0;
        transfer[0].tx_buf  = NULL;
        transfer[0].rx_buf =  NULL;
        transfer[0].len=0;
        transfer[0].date_en=0;
        transfer[0].dma_mode=SLAVE_MODE;
        transfer[0].pollen=0;
        transfer[0].rw_mode=0;
        transfer[0].sfc_mode=0;
        jz_sfc_pio_txrx(flash,transfer);

	return 0;
}
void test_read_write_erase()
{
}
static int __init jz_sfc_probe(struct platform_device *pdev)
{
	struct jz_sfc_nand *flash;//flash->board_info是jz_spi_nand_platform_data
	const char *jz_probe_types[] = {"cmdlinepart",NULL};
	struct jz_spi_support *spi_flash;

	int err = 0,ret = 0;

	struct nand_chip *chip;
	struct mtd_partition *mtd_sfcnand_partition;
	int num_partitions;

	chip = kzalloc(sizeof(struct nand_chip),GFP_KERNEL);
	if(!chip)
		return -ENOMEM;

	print_dbg("function : %s, line : %d\n", __func__, __LINE__);

	flash = kzalloc(sizeof(struct jz_sfc_nand), GFP_KERNEL);
	if (!flash) {
		printk("%s---%s---%d\n", __FILE__, __func__, __LINE__);
		printk("kzalloc() error !\n");
		return -ENOMEM;
	}

	flash->dev = &pdev->dev;
	flash->pdata = pdev->dev.platform_data;
	mtd_sfcnand_partition = ((struct jz_spi_nand_platform_data *)(flash->pdata->board_info))->mtd_partition;
        num_partitions =((struct jz_spi_nand_platform_data *)(flash->pdata->board_info))->num_partitions;
	if (flash->pdata == NULL) {
		dev_err(&pdev->dev, "No platform data supplied\n");
		goto err_no_pdata;
	}
	flash->threshold = THRESHOLD;
	ret=sfc_nand_plat_resource_init(flash,pdev);
	if(ret<0)
	switch (ret)
	{
		case -1:goto err_no_iores;break;
		case -2:goto err_no_iores;break;
		case -3:goto err_no_iomap;break;
		case -4:goto err_no_irq;break;
		case -5:goto err_no_clk;break;
		case -6:goto err_no_iomap;break;
		case -7:goto err_no_iores;break;
	};
	flash->chnl= flash->pdata->chnl;
	flash->tx_addr_plus = 0;
	flash->rx_addr_plus = 0;
	flash->use_dma = 0;
	err=0;

	/* find and map our resources */
	/* SFC controller initializations for SFC */
	jz_sfc_init_setup(flash);
        platform_set_drvdata(pdev, flash);
        init_completion(&flash->done);
        spin_lock_init(&flash->lock_rxtx);
        spin_lock_init(&flash->lock_status);
        mutex_init(&flash->lock);
	flash->threshold = THRESHOLD;
        err = request_irq(flash->irq, jz_sfc_irq, 0, pdev->name, flash);
        if (err) {
		dev_err(&pdev->dev, "Cannot claim IRQ\n");
        }
	spi_flash = jz_sfc_nand_probe(flash);
	flash->mtd.name = "sfc_nand";
	flash->mtd.owner = THIS_MODULE;
	flash->mtd.type = MTD_NANDFLASH;
	flash->mtd.flags |= MTD_CAP_NANDFLASH;
	flash->mtd.erasesize = spi_flash->block_size;
	flash->mtd.writesize = spi_flash->page_size;
	flash->mtd.size = spi_flash->size;
	flash->mtd.oobsize = spi_flash->oobsize;
	flash->mtd.writebufsize = flash->mtd.writesize;
	flash->column_cmdaddr_bits = spi_flash->column_cmdaddr_bits;
	flash->tRD = spi_flash->tRD_maxbusy;
	flash->tPROG = spi_flash->tPROG_maxbusy;
	flash->tBERS = spi_flash->tBERS_maxbusy;

        flash->mtd.bitflip_threshold = flash->mtd.ecc_strength = 1;
        chip->select_chip = NULL;
        chip->badblockbits = 8;
        chip->scan_bbt = nand_default_bbt;
	chip->block_bad = jz_sfcnand_block_bad_check;
	chip->block_markbad = jz_sfc_nandflash_block_markbad;
        //chip->ecc.layout= &gd5f_ecc_layout_128; // for erase ops
        chip->bbt_erase_shift = chip->phys_erase_shift = ffs(flash->mtd.erasesize) - 1;
        if (!(chip->options & NAND_OWN_BUFFERS))
                chip->buffers = kmalloc(sizeof(*chip->buffers), GFP_KERNEL);

        /* Set the bad block position */
        if (flash->mtd.writesize > 512 || (chip->options & NAND_BUSWIDTH_16))
		chip->badblockpos = NAND_LARGE_BADBLOCK_POS;
        else
                chip->badblockpos = NAND_SMALL_BADBLOCK_POS;

        flash->mtd.priv = chip;

        flash->mtd._erase = jz_sfcnand_erase;
        flash->mtd._read = jz_sfcnand_read;
        flash->mtd._write = jz_sfcnand_write;
        flash->mtd._read_oob = jz_sfcnand_read_oob;
        flash->mtd._write_oob = jz_sfcnand_write_oob;
        flash->mtd._block_isbad = jz_sfcnand_block_isbab;
        flash->mtd._block_markbad = jz_sfcnand_block_markbad;

	jz_sfc_nand_ext_init(flash);
	chip->scan_bbt(&flash->mtd);
	ret = mtd_device_parse_register(&flash->mtd,jz_probe_types,NULL, mtd_sfcnand_partition, num_partitions);
	if (ret) {
		kfree(flash);
		return -ENODEV;
	}
/*	unsigned char user_buf[2]={};
	int rrrr=0,llen;
	struct erase_info ssa={
		.addr=0x900000,
		.len=flash->mtd.erasesize,
		.mtd=&(flash->mtd),
		.callback=0,
	};
	jz_sfcnand_erase(&(flash->mtd), &ssa);
	unsigned char  *bufff=(unsigned char *)kmalloc(2048*3,GFP_KERNEL);
	for(rrrr=0;rrrr<2049;rrrr++)
		bufff[rrrr]=rrrr;
	ret=jz_sfcnand_write(&(flash->mtd),0x900002, 2049,&llen, bufff);
	memset(bufff,0,3*2048);
	jz_sfcnand_read(&(flash->mtd),0x900000,2080, &rrrr, bufff);
	printk("-----------readbuff-------------\n");
	for(rrrr=0;rrrr<2080;)
	{
			printk("%x,",bufff[rrrr]);
			rrrr++;
			if(rrrr%16==0)printk("\n");
	}
*/
       return 0;
err_no_clk:
        clk_put(flash->clk_gate);
        clk_put(flash->clk);
err_no_irq:
        free_irq(flash->irq, flash);
err_no_iomap:
        iounmap(flash->iomem);
err_no_iores:
err_no_pdata:
        release_resource(flash->ioarea);
        kfree(flash->ioarea);
        return err;
}

static int __exit jz_sfc_remove(struct platform_device *pdev)
{
	struct jz_sfc_nand *flash = platform_get_drvdata(pdev);


	clk_disable(flash->clk_gate);
	clk_put(flash->clk_gate);

	clk_disable(flash->clk);
	clk_put(flash->clk);

	free_irq(flash->irq, flash);

	iounmap(flash->iomem);

	release_mem_region(flash->resource->start, resource_size(flash->resource));

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static int jz_sfc_suspend(struct platform_device *pdev, pm_message_t msg)
{
	unsigned long flags;
	struct jz_sfc_nand *flash = platform_get_drvdata(pdev);

	spin_lock_irqsave(&flash->lock_status, flags);
	flash->status |= STATUS_SUSPND;
	disable_irq(flash->irq);
	spin_unlock_irqrestore(&flash->lock_status, flags);

	clk_disable(flash->clk_gate);

	clk_disable(flash->clk);

	return 0;
}

static int jz_sfc_resume(struct platform_device *pdev)
{
	unsigned long flags;
	struct jz_sfc_nand *flash = platform_get_drvdata(pdev);

	clk_enable(flash->clk);

	clk_enable(flash->clk_gate);

	spin_lock_irqsave(&flash->lock_status, flags);
	flash->status &= ~STATUS_SUSPND;
	enable_irq(flash->irq);
	spin_unlock_irqrestore(&flash->lock_status, flags);

	return 0;
}

void jz_sfc_shutdown(struct platform_device *pdev)
{
	unsigned long flags;
	struct jz_sfc_nand *flash = platform_get_drvdata(pdev);

	spin_lock_irqsave(&flash->lock_status, flags);
	flash->status |= STATUS_SUSPND;
	disable_irq(flash->irq);
	spin_unlock_irqrestore(&flash->lock_status, flags);

	clk_disable(flash->clk_gate);

	clk_disable(flash->clk);

	return ;
}

static struct platform_driver jz_sfcdrv = {
	.driver		= {
		.name	= "jz-sfc",
		.owner	= THIS_MODULE,
	},
	.remove		= jz_sfc_remove,
	.suspend	= jz_sfc_suspend,
	.resume		= jz_sfc_resume,
	.shutdown	= jz_sfc_shutdown,
};

static int __init jz_sfc_init(void)
{
	print_dbg("function : %s, line : %d\n", __func__, __LINE__);
	return platform_driver_probe(&jz_sfcdrv, jz_sfc_probe);
}

static void __exit jz_sfc_exit(void)
{
	print_dbg("function : %s, line : %d\n", __func__, __LINE__);
    platform_driver_unregister(&jz_sfcdrv);
}

module_init(jz_sfc_init);
module_exit(jz_sfc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("JZ SFC Driver");
