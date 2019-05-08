#ifndef __JZ_NAND_H__
#define __JZ_NAND_H__
struct jz_nand_params {
	int buswidth;
	unsigned int tALS;
	unsigned int tALH;
	unsigned int tRP;
	unsigned int tWP;
	unsigned int tRHW;
	unsigned int tWHR;
	unsigned int tWHR2;
	unsigned int tRR;
	unsigned int tWB;
	unsigned int tADL;
	unsigned int tCWAW;
	unsigned int tCS;
	unsigned int tCLH;
	unsigned int tWH;
	unsigned int tCH;
	unsigned int tDH;
	unsigned int tREH;
};


static inline void dump_nand_info(struct mtd_info *mtd, struct nand_chip *chip)
{
	int i;

	printk("chip->ecc.size = %d\n", chip->ecc.size);
	printk("chip->ecc.bytes = %d\n", chip->ecc.bytes);
	printk("chip->ecc.steps = %d\n", chip->ecc.steps);
	printk("mtd->writesize = %d\n", mtd->writesize);
	printk("mtd->oobsize = %d\n", mtd->oobsize);
	printk("chip->badblockpos = %d\n", chip->badblockpos);
	printk("mtd->erasesize = %d\n", mtd->erasesize);
	printk("chip->chipsize = %lld\n", chip->chipsize);
	printk("chip->numchips = %d\n", chip->numchips);
	printk("ecclayout.eccbytes %d\n", chip->ecc.layout->eccbytes);
	printk("ecclayout.oobfree->offset = %d\n", chip->ecc.layout->oobfree->offset);
	printk("ecclayout.oobfree->length = %d\n", chip->ecc.layout->oobfree->length);
	for (i = 0; i < chip->ecc.layout->eccbytes; i++) {
		printk("ecclayout.eccpos[%d] = %d\n", i, chip->ecc.layout->eccpos[i]);
	}
}
extern int jz_nandc_select_chip(struct device *dev, struct nand_chip *nand_chip, int chip);
extern void jz_nandc_cmd_ctrl(struct device *dev, struct nand_chip *nand_chip, int cmd, unsigned int ctrl);
extern int jz_nandc_dev_is_ready(struct device *dev, struct nand_chip *nand_chip);
#endif /*__JZ_NAND_H__*/
