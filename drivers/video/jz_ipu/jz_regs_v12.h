#ifndef _REGS_IPU_H_
#define _REGS_IPU_H_

#ifdef __KERNEL__

/* Module for CLKGR */
#define IDCT_CLOCK      (1 << 27)
#define DBLK_CLOCK      (1 << 26)
#define ME_CLOCK        (1 << 25)
#define MC_CLOCK        (1 << 24)
#define IPU_CLOCK       (1 << 13)


/* Register offset */
#define  IPU_FM_CTRL            0x0
#define  IPU_STATUS             0x4
#define  IPU_D_FMT              0x8
#define  IPU_Y_ADDR             0xc
#define  IPU_U_ADDR             0x10
#define  IPU_V_ADDR             0x14
#define  IPU_IN_FM_GS           0x18
#define  IPU_Y_STRIDE           0x1c
#define  IPU_UV_STRIDE          0x20
#define  IPU_OUT_ADDR           0x24
#define  IPU_OUT_GS             0x28
#define  IPU_OUT_STRIDE         0x2c
#define  IPU_CSC_C0_COEF        0x34
#define  IPU_CSC_C1_COEF        0x38
#define  IPU_CSC_C2_COEF        0x3c
#define  IPU_CSC_C3_COEF        0x40
#define  IPU_CSC_C4_COEF        0x44
#define  IPU_CSC_OFFSET_PARA    0x50
#define  IPU_SRC_TLB_ADDR       0x54
#define  IPU_DEST_TLB_ADDR      0x58
#define  IPU_ADDR_CTRL          0x64
#define  IPU_Y_ADDR_N           0x84
#define  IPU_U_ADDR_N           0x88
#define  IPU_V_ADDR_N           0x8c
#define  IPU_OUT_ADDR_N         0x90
#define  IPU_SRC_TLB_ADDR_N     0x94
#define  IPU_DEST_TLB_ADDR_N    0x98
#define  IPU_TRIG               0x74
#define  IPU_FM_XYOFT           0x78
#define  IPU_GLB_CTRL           0x7c
#define  IPU_OSD_CTRL           0x9c

#define  IPU_RSZ_COEF			0xb0
#define  IPU_TLB_PARA			0xa0
#define  IPU_VADDR_STLB_ERR		0xa4
#define  IPU_VADDR_DTLB_ERR		0xa8



/* IPU_GLB_CTRL Register */
#define IRQ_EN          (1 << 0)
#define SHARP_LEVE		(1 << 8)
#define TFILL_MOD		(1 << 16)
#define TLB_IRQ_MSK     (1 << 17)


/* IPU_OSD_CTRL Register */
#define OSD_PM          (1 << 2)

/* IPU_TRIG Register */
#define IPU_RUN         (1 << 0)
#define IPU_STOP        (1 << 1)
#define IPU_STOP_LCD    (1 << 2)
#define IPU_RESET       (1 << 3)
#define	SRC_RF			(1 << 4)
#define	DST_RF          (1 << 5)

/* IPU_D_FMT Register */
#define BLK_SEL         (1 << 4)
#define RGB_POS			(1 << 5)
#define AL_PIX_EN		(1 << 6)

/* REG_FM_CTRL field define */
#define HRSZ_EN         (1 << 2)
#define VRSZ_EN         (1 << 3)
#define CSC_EN          (1 << 4)
#define SPKG_SEL        (1 << 10)
#define LCDC_SEL        (1 << 11)
#define SPAGE_MAP       (1 << 12)
#define DPAGE_MAP       (1 << 13)
#define DISP_SEL        (1 << 14)
#define FIELD_CONF_EN   (1 << 15)
#define FIELD_SEL       (1 << 16)
#define DFIX_SEL        (1 << 17)
#define ZOOM_SEL        (1 << 18)

/* REG_STATUS field define */
#define OUT_END         (1 << 0)
#define SIZE_ERR        (1 << 2)
#define STLB_ERR		(1 << 8)
#define DTLB_ERR		(1 << 9)
#define ID				(1 << 16)

#define MSTATUS_SFT     (4)
#define MSTATUS_MSK     (3)
#define MSTATUS_IPU_FREE        (0 << MSTATUS_SFT)
#define MSTATUS_IPU_RUNNING     (1 << MSTATUS_SFT)
#define MSTATUS                 (2 << MSTATUS_SFT)


/* REG_IPU_ADDR_CTRL */
#define Y_RY            (1<<0)
#define U_RY            (1<<1)
#define V_RY            (1<<2)
#define D_RY            (1<<3)
#define PTS_RY          (1<<4)
#define PTD_RY          (1<<5)
#define CTRL_RY          (1<<6)


/* REG_TLB_MONITOR */
#define MIS_CNT_SFT     (1)
#define MIS_CNT_MSK     (0x3FF)

/* REG_TLB_CTRL */
#define SRC_PAGE_SFT        (0)
#define SRC_PAGE_MSK        (0xF)
#define SRC_BURST_SFT       (4)
#define SRC_BURST_MSK       (0xF)
#define DEST_PAGE_SFT       (16)
#define DEST_PAGE_MSK       (0xF)
#define DEST_BURST_SFT      (20)
#define DEST_BURST_MSK      (0xF)

/* OSD_CTRL field define */
#define GLB_ALPHA(val)  ((val) << 8)
#define MOD_OSD(val)    ((val) << 0)

/* FM_XYOFT field define */
#define SCREEN_YOFT(val)    ((val) << 16)
#define SCREEN_XOFT(val)    ((val) << 0)

/* REG_IN_GS field define */
#define IN_FM_W(val)    ((val) << 16)
#define IN_FM_H(val)    ((val) << 0)

/* REG_OUT_GS field define */
#define OUT_FM_W(val)    ((val) << 16)
#define OUT_FM_H(val)    ((val) << 0)

/* REG_UV_STRIDE field define */
#define U_STRIDE(val)     ((val) << 16)
#define V_STRIDE(val)     ((val) << 0)

/* REG_RSZ_COEF field definei */
#define HCOEF(val)			((val) << 16)
#define VCOEF(val)			((val) << 0)


#if 1
#define YUV_CSC_C0 0x4A8        /* 1.164 * 1024 */
#define YUV_CSC_C1 0x662        /* 1.596 * 1024 */
#define YUV_CSC_C2 0x191        /* 0.392 * 1024 */
#define YUV_CSC_C3 0x341        /* 0.813 * 1024 */
#define YUV_CSC_C4 0x811        /* 2.017 * 1024 */

#define YUV_CSC_OFFSET_PARA         0x800010  /* chroma,luma */
#else
#define YUV_CSC_C0 0x400
#define YUV_CSC_C1 0x59C
#define YUV_CSC_C2 0x161
#define YUV_CSC_C3 0x2DC
#define YUV_CSC_C4 0x718
#endif


#endif	/* #ifdef __KERNEL__ */

///////////////////////////////////////////

/* Data Format Register, export to libipu */
#define RGB_888_OUT_FMT				( 1 << 25 )

#define RGB_OUT_OFT_BIT				( 22 )
#define RGB_OUT_OFT_MASK			( 7 << RGB_OUT_OFT_BIT )
#define RGB_OUT_OFT_RGB				( 0 << RGB_OUT_OFT_BIT )
#define RGB_OUT_OFT_RBG				( 1 << RGB_OUT_OFT_BIT )
#define RGB_OUT_OFT_GBR				( 2 << RGB_OUT_OFT_BIT )
#define RGB_OUT_OFT_GRB				( 3 << RGB_OUT_OFT_BIT )
#define RGB_OUT_OFT_BRG				( 4 << RGB_OUT_OFT_BIT )
#define RGB_OUT_OFT_BGR				( 5 << RGB_OUT_OFT_BIT )

#define OUT_FMT_BIT					( 19 )
#define OUT_FMT_MASK				( 3 <<  OUT_FMT_BIT )
#define OUT_FMT_RGB555				( 0 <<  OUT_FMT_BIT )
#define OUT_FMT_RGB565				( 1 <<  OUT_FMT_BIT )
#define OUT_FMT_RGB888				( 2 <<  OUT_FMT_BIT )
#define OUT_FMT_YUV422				( 3 <<  OUT_FMT_BIT )
#define OUT_FMT_RGBAAA				( 3 <<  OUT_FMT_BIT )

#define YUV_PKG_OUT_OFT_BIT			( 16 )
#define YUV_PKG_OUT_OFT_MASK		( 7 << YUV_PKG_OUT_OFT_BIT )
#define YUV_PKG_OUT_OFT_Y1UY0V		( 0 << YUV_PKG_OUT_OFT_BIT )
#define YUV_PKG_OUT_OFT_Y1VY0U		( 1 << YUV_PKG_OUT_OFT_BIT )
#define YUV_PKG_OUT_OFT_UY1VY0		( 2 << YUV_PKG_OUT_OFT_BIT )
#define YUV_PKG_OUT_OFT_VY1UY0		( 3 << YUV_PKG_OUT_OFT_BIT )
#define YUV_PKG_OUT_OFT_Y0UY1V		( 4 << YUV_PKG_OUT_OFT_BIT )
#define YUV_PKG_OUT_OFT_Y0VY1U		( 5 << YUV_PKG_OUT_OFT_BIT )
#define YUV_PKG_OUT_OFT_UY0VY1		( 6 << YUV_PKG_OUT_OFT_BIT )
#define YUV_PKG_OUT_OFT_VY0UY1		( 7 << YUV_PKG_OUT_OFT_BIT )

#define IN_OFT_BIT					( 2 )
#define IN_OFT_MASK					( 3 << IN_OFT_BIT )
#define IN_OFT_Y1UY0V				( 0 << IN_OFT_BIT )
#define IN_OFT_Y1VY0U				( 1 << IN_OFT_BIT )
#define IN_OFT_UY1VY0				( 2 << IN_OFT_BIT )
#define IN_OFT_VY1UY0				( 3 << IN_OFT_BIT )

#define IN_FMT_BIT					( 0 )
#define IN_FMT_MASK					( 7 << IN_FMT_BIT )
#define IN_FMT_YUV420				( 0 << IN_FMT_BIT )
#define IN_FMT_YUV420_B				( 4 << IN_FMT_BIT )
#define IN_FMT_YUV422				( 1 << IN_FMT_BIT )
#define IN_FMT_YUV444				( 2 << IN_FMT_BIT )
#define IN_FMT_RGB_555				( 0 << IN_FMT_BIT )
#define IN_FMT_RGB_888			    ( 2 << IN_FMT_BIT )
#define IN_FMT_RGB_565				( 3 << IN_FMT_BIT )
#define IN_FMT_YUV411				( 3 << IN_FMT_BIT )

#define __enable_csc_mode() bit_set(ipu, IPU_FM_CTRL, CSC_EN)
#define __disable_csc_mode() bit_clr(ipu, IPU_FM_CTRL, CSC_EN)
#define __enable_lcdc_mode() bit_set(ipu, IPU_FM_CTRL, LCDC_SEL)
#define __disable_lcdc_mode() bit_clr(ipu, IPU_FM_CTRL, LCDC_SEL)
#define __enable_pkg_mode() bit_set(ipu, IPU_FM_CTRL, SPKG_SEL)
#define __disable_pkg_mode() bit_clr(ipu, IPU_FM_CTRL, SPKG_SEL)
#define __enable_blk_mode() bit_set(ipu, IPU_D_FMT, BLK_SEL)
#define __disable_blk_mode() bit_clr(ipu, IPU_D_FMT, BLK_SEL)
#define __enable_hrsz() bit_set(ipu, IPU_FM_CTRL, HRSZ_EN)
#define __disable_hrsz() bit_clr(ipu, IPU_FM_CTRL, HRSZ_EN)
#define __enable_vrsz() bit_set(ipu, IPU_FM_CTRL, VRSZ_EN)
#define __disable_vrsz()  bit_clr(ipu, IPU_FM_CTRL, VRSZ_EN)
#define __sel_zoom_mode() bit_set(ipu, IPU_FM_CTRL, ZOOM_SEL)
#define __disable_zoom_mode() bit_clr(ipu, IPU_FM_CTRL, ZOOM_SEL)
#define __clear_ipu_out_end() bit_clr(ipu, IPU_STATUS, OUT_END)
#define __ipu_enable_irq() do {unsigned int val = IRQ_EN ;	\
		bit_set(ipu, IPU_GLB_CTRL, val);								\
	}while(0)
#define __ipu_disable_irq() do {unsigned int val = IRQ_EN ; \
		bit_clr(ipu, IPU_GLB_CTRL, val);								\
	}while(0)
#define __start_ipu() bit_set(ipu, IPU_TRIG, IPU_RUN)
#define __reset_ipu() bit_set(ipu, IPU_TRIG, IPU_RESET)
#define __enable_spage_map() bit_set(ipu, IPU_FM_CTRL, SPAGE_MAP)
#define __disable_spage_map() bit_clr(ipu, IPU_FM_CTRL, SPAGE_MAP)
#define __enable_dpage_map() bit_set(ipu, IPU_FM_CTRL, DPAGE_MAP)
#define __disable_dpage_map() bit_clr(ipu, IPU_FM_CTRL, DPAGE_MAP)

#endif // _REGS_IPU_H_
