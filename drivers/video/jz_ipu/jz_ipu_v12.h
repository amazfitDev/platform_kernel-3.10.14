#ifndef _JZ_IPU_H_
#define _JZ_IPU_H_

#define IPU_LUT_LEN      32
#define JZIPU_IOC_MAGIC  'I'

/* ipu output mode */
#define IPU_OUTPUT_TO_LCD_FG1           0x00000002
#define IPU_OUTPUT_TO_LCD_FB0           0x00000004
#define IPU_OUTPUT_TO_LCD_FB1           0x00000008
#define IPU_OUTPUT_TO_FRAMEBUFFER       0x00000010 /* output to user defined buffer */
#define IPU_OUTPUT_MODE_MASK            0x000000FF
#define IPU_DST_USE_COLOR_KEY           0x00000100
#define IPU_DST_USE_ALPHA               0x00000200
#define IPU_OUTPUT_BLOCK_MODE           0x00000400
#define IPU_OUTPUT_MODE_FG0_OFF         0x00000800
#define IPU_OUTPUT_MODE_FG1_TVOUT       0x00001000
#define IPU_DST_USE_PERPIXEL_ALPHA      0x00010000

#ifdef __KERNEL__

/* match HAL_PIXEL_FORMAT_ in system/core/include/system/graphics.h */
enum {
	HAL_PIXEL_FORMAT_RGBA_8888    = 1,
	HAL_PIXEL_FORMAT_RGBX_8888    = 2,
	HAL_PIXEL_FORMAT_RGB_888      = 3,
	HAL_PIXEL_FORMAT_RGB_565      = 4,
	HAL_PIXEL_FORMAT_BGRA_8888    = 5,
	//HAL_PIXEL_FORMAT_BGRX_8888    = 0x8000, /* Add BGRX_8888, Wolfgang, 2010-07-24 */
	HAL_PIXEL_FORMAT_BGRX_8888  	= 0x1ff, /* 2012-10-23 */
	HAL_PIXEL_FORMAT_RGBA_5551    = 6,
	HAL_PIXEL_FORMAT_RGBA_4444    = 7,
	HAL_PIXEL_FORMAT_YCbCr_422_SP = 0x10,
	HAL_PIXEL_FORMAT_YCbCr_420_SP = 0x11,
	HAL_PIXEL_FORMAT_YCbCr_422_P  = 0x12,
	HAL_PIXEL_FORMAT_YCbCr_420_P  = 0x13,
	HAL_PIXEL_FORMAT_YCbCr_420_B  = 0x24,
	HAL_PIXEL_FORMAT_YCbCr_422_I  = 0x14,
	HAL_PIXEL_FORMAT_YCbCr_420_I  = 0x15,
	HAL_PIXEL_FORMAT_CbYCrY_422_I = 0x16,
	HAL_PIXEL_FORMAT_CbYCrY_420_I = 0x17,

	/* suport for YUV420 */
	HAL_PIXEL_FORMAT_JZ_YUV_420_P       = 0x47700001, // YUV_420_P
	HAL_PIXEL_FORMAT_JZ_YUV_420_B       = 0x47700002, // YUV_420_P BLOCK MODE
};

typedef struct {
	unsigned int coef;
	unsigned short int in_n;
	unsigned short int out_n;
} rsz_lut;

#endif /* ifdef __KERNEL__ */

enum {
	ZOOM_MODE_BILINEAR = 0,
	ZOOM_MODE_BICUBE,
	ZOOM_MODE_BILINEAR_ENHANCE,
};

struct YuvCsc
{									// YUV(default)	or	YCbCr
	unsigned int csc0;				//	0x400			0x4A8
	unsigned int csc1;              //	0x59C   		0x662
	unsigned int csc2;              //	0x161   		0x191
	unsigned int csc3;              //	0x2DC   		0x341
	unsigned int csc4;              //	0x718   		0x811
	unsigned int chrom;             //	128				128
	unsigned int luma;              //	0				16
};

struct YuvStride
{
	unsigned int y;
	unsigned int u;
	unsigned int v;
	unsigned int out;
};

struct ipu_img_param
{
	unsigned int 		fb_w;
	unsigned int 		fb_h;
	unsigned int 		version;			/* sizeof(struct ipu_img_param) */
	int			        ipu_cmd;			/* IPU command by ctang 2012/6/25 */
	unsigned int        stlb_base;      /* IPU src tlb table base phys */
	unsigned int        dtlb_base;      /* IPU dst tlb table base phys */
	unsigned int		output_mode;		/* IPU output mode: fb0, fb1, fg1, alpha, colorkey and so on */
	unsigned int		alpha;
	unsigned int		colorkey;
	unsigned int		in_width;
	unsigned int		in_height;
	unsigned int		in_bpp;
	unsigned int		out_x;
	unsigned int		out_y;
	unsigned int		in_fmt;
	unsigned int		out_fmt;
	unsigned int		out_width;
	unsigned int		out_height;
	unsigned char*		y_buf_v;            /* Y buffer virtual address */
	unsigned char*		u_buf_v;
	unsigned char*		v_buf_v;
	unsigned int		y_buf_p;            /* Y buffer physical address */
	unsigned int		u_buf_p;
	unsigned int		v_buf_p;
	unsigned char*		out_buf_v;
	unsigned int		out_buf_p;
	unsigned int 		src_page_mapping;
	unsigned int 		dst_page_mapping;
	unsigned char*		y_t_addr;				// table address
	unsigned char*		u_t_addr;
	unsigned char*		v_t_addr;
	unsigned char*		out_t_addr;
	struct YuvCsc*		csc;
	struct YuvStride	stride;
	int 			    Wdiff;
	int 			    Hdiff;
	unsigned int		zoom_mode;

	int					v_coef;
	int					h_coef;


};

struct ipu_dmmu_map_info {
	unsigned int	addr;
	unsigned int	len;
};

#ifdef __KERNEL__

/* 
 * IPU driver's native data
 */

struct ipu_reg_struct {
	char *name;
	unsigned int addr;
};

struct ipu_proc_info {
	struct list_head list;

	pid_t pid;
	struct file *ipu_filp;

	struct ipu_img_param img;
};

struct jz_ipu {
	int irq;
	int inited;
	char name[16];
	int open_cnt;
	int proc_num;
	struct file *cur_proc;
	unsigned int cur_output_mode;

	struct clk *clk;
	void __iomem *iomem;
	struct device *dev;
	struct resource *res;
	struct miscdevice misc_dev;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	struct mutex lock;
	struct mutex run_lock;
	spinlock_t update_lock;

	unsigned int frame_done;
	unsigned int frame_requested;	
	wait_queue_head_t frame_wq;

	struct proc_dir_entry *pde;
	struct list_head process_list;

	int suspend_entered;
	spinlock_t suspend_lock;
    unsigned int g_reg_harb_prior;

	void *ipumem_v;
	unsigned int ipumem_p;
};


#define	IOCTL_IPU_SHUT               _IO(JZIPU_IOC_MAGIC, 102)
#define IOCTL_IPU_INIT               _IOW(JZIPU_IOC_MAGIC, 103, struct ipu_img_param)
#define IOCTL_IPU_SET_BUFF           _IOW(JZIPU_IOC_MAGIC, 105, struct ipu_img_param)
#define IOCTL_IPU_START              _IO(JZIPU_IOC_MAGIC, 106)
#define IOCTL_IPU_STOP               _IO(JZIPU_IOC_MAGIC, 107)
#define IOCTL_IPU_DUMP_REGS          _IO(JZIPU_IOC_MAGIC, 108)
#define IOCTL_IPU_SET_BYPASS         _IO(JZIPU_IOC_MAGIC, 109)
#define IOCTL_IPU_GET_BYPASS_STATE   _IOR(JZIPU_IOC_MAGIC, 110, int)
#define IOCTL_IPU_CLR_BYPASS         _IO(JZIPU_IOC_MAGIC, 111)
#define IOCTL_IPU_ENABLE_CLK         _IO(JZIPU_IOC_MAGIC, 112)
#define IOCTL_IPU_TO_BUF             _IO(JZIPU_IOC_MAGIC, 113)
#define IOCTL_IPU_DMMU_MAP		_IO(JZIPU_IOC_MAGIC, 114)
#define IOCTL_IPU_DMMU_UNMAP		_IO(JZIPU_IOC_MAGIC, 115)
#define IOCTL_IPU_DMMU_UNMAP_ALL	_IO(JZIPU_IOC_MAGIC, 116)
//#define IOCTL_GET_FREE_IPU       _IOR(JZIPU_IOC_MAGIC, 109, int)
#define IOCTL_IPU_GET_PBUFF			_IO(JZIPU_IOC_MAGIC, 117)

static inline unsigned int reg_read(struct jz_ipu *jzipu, int offset)
{
	return readl(jzipu->iomem + offset); 
}

static inline void reg_write(struct jz_ipu *jzipu, int offset, unsigned int val)
{
	writel(val, jzipu->iomem + offset); 
}

#endif	/* #ifdef __KERNEL__ */

#endif // _JZ_IPU_H_
