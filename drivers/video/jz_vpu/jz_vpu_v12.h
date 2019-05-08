
#ifndef __JZ_VPU_V12_H__
#define __JZ_VPU_V12_H__

#define REG_VPU_GLBC      0x00000
#define VPU_INTE_ACFGERR     (0x1<<20)
#define VPU_INTE_TLBERR      (0x1<<18)
#define VPU_INTE_BSERR       (0x1<<17)
#define VPU_INTE_ENDF        (0x1<<16)

#define REG_VPU_STAT      0x00034
#define VPU_STAT_ENDF    (0x1<<0)
#define VPU_STAT_BPF     (0x1<<1)
#define VPU_STAT_ACFGERR (0x1<<2)
#define VPU_STAT_TIMEOUT (0x1<<3)
#define VPU_STAT_JPGEND  (0x1<<4)
#define VPU_STAT_BSERR   (0x1<<7)
#define VPU_STAT_TLBERR  (0x1F<<10)
#define VPU_STAT_SLDERR  (0x1<<16)

#define REG_VPU_JPGC_STAT 0xE0008
#define JPGC_STAT_ENDF   (0x1<<31)

#define REG_VPU_SDE_STAT  0x90000
#define SDE_STAT_BSEND   (0x1<<1)

#define REG_VPU_DBLK_STAT 0x70070
#define DBLK_STAT_DOEND  (0x1<<0)

#define REG_VPU_AUX_STAT  0xA0010
#define AUX_STAT_MIRQP   (0x1<<0)

enum {
	WAIT_COMPLETE = 0,
	LOCK,
	UNLOCK,
	FLUSH_CACHE,
};

struct flush_cache_info {
	unsigned int	addr;
	unsigned int	len;
#define WBACK		DMA_TO_DEVICE
#define INV		DMA_FROM_DEVICE
#define WBACK_INV	DMA_BIDIRECTIONAL
	unsigned int	dir;
};

struct vpu_dmmu_map_info {
	unsigned int	addr;
	unsigned int	len;
};

#define vpu_readl(vpu, offset)		__raw_readl((vpu)->iomem + offset)
#define vpu_writel(vpu, offset, value)	__raw_writel((value), (vpu)->iomem + offset)

#define REG_VPU_STATUS ( *(volatile unsigned int*)0xb3200034 )
#define REG_VPU_LOCK ( *(volatile unsigned int*)0xb329004c )
#define REG_VPUCDR ( *(volatile unsigned int*)0xb0000030 )
#define REG_CPM_VPU_SWRST ( *(volatile unsigned int*)0xb00000c4 )
#define REG_VPU_TLBBASE	(*(volatile unsigned int *)(0x30 + 0xb3200000))
#define CPM_VPU_SR           (0x1<<31)
#define CPM_VPU_STP          (0x1<<30)
#define CPM_VPU_ACK          (0x1<<29)

#define REG_VPU_SCH_GLBC	(*(volatile unsigned int *)0xb3200000)

//#define MAX_LOCK_DEPTH		(999)
#define CMD_VPU_CACHE		(100)
#define CMD_VPU_PHY		(101)
#define CMD_VPU_DMA_NOTLB	(102)
#define CMD_VPU_DMA_TLB		(103)
#define CMD_VPU_CLEAN_WAIT_FLAG (104)
#define CMD_VPU_RESET		(105)
#define CMD_VPU_SET_REG		(106)
#define CMD_VPU_DMMU_MAP	(107)
#define CMD_VPU_DMMU_UNMAP	(108)
#define CMD_VPU_DMMU_UNMAP_ALL	(109)
#define CMD_VPU_REQUEST_MEM		(110)
#define CMD_VPU_RELEASE_MEM		(111)
#define CMD_VPU_RELEASE_ALL_MEM	(112)
#define CMD_VPU_GET_PHYS_ADDR	(113)
#define CMD_VPU_OPEN		(114)
#define CMD_VPU_CLOSE		(115)

/* we add them to wait for vpu end before suspend */
#define VPU_NEED_WAIT_END_FLAG	(0x80000000)
#define VPU_WAIT_OK		(0x40000000)
#define VPU_END			(0x1)

#define UNMAP			(0xe0)
#define	MAPED			(0xe1)
#define RELEASE_YES		(1)
#define RELEASE_NO		(0)
#endif	/* __JZ_VPU_V12_H__ */
