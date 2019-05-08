#include <linux/fb.h>

#ifdef CONFIG_JZ4780_AOSD
#include "aosd.h"
#endif

#ifdef CONFIG_TWO_FRAME_BUFFERS
#define NUM_FRAME_BUFFERS 2
#endif

#ifdef CONFIG_THREE_FRAME_BUFFERS
#define NUM_FRAME_BUFFERS 3
#endif

#define PIXEL_ALIGN 16
#define MAX_DESC_NUM 4

/*
  Jz-SLCDC will ignore DMA_RESTART when it is doing a dma transfer.
  It is to say, if the interval time that a new jzfb_pan_display() operation
  between last jzfb_pan_display() less than a dma transfer time(16ms),
  Jz-SLCDC will ignore the new DMA_RESTART. The new frame cann't display on screen.

  Fix method: Use JZ-LCDC end-of-frame(EOF) interrupt, when last frame EOF
  interrupt occur, than trigger DMA_RESTART.
*/
#define SLCD_DMA_RESTART_WORK_AROUND

/**
 * @next: physical address of next frame descriptor
 * @databuf: physical address of buffer
 * @id: frame ID
 * @cmd: DMA command and buffer length(in word)
 * @offsize: DMA off size, in word
 * @page_width: DMA page width, in word
 * @cpos: smart LCD mode is commands' number, other is bpp,
 * premulti and position of foreground 0, 1
 * @desc_size: alpha and size of foreground 0, 1
 */
struct jzfb_framedesc {
	uint32_t next;
	uint32_t databuf;
	uint32_t id;
	uint32_t cmd;
	uint32_t offsize;
	uint32_t page_width;
	uint32_t cpos;
	uint32_t desc_size;
} __packed;

struct jzfb_display_size {
	u32 fg0_line_size;
	u32 fg0_frm_size;
	u32 fg1_line_size;
	u32 fg1_frm_size;
	u32 panel_line_size;
	u32 height_width;
	u32 fg1_height_width;
};

enum jzfb_format_order {
	FORMAT_X8R8G8B8 = 1,
	FORMAT_X8B8G8R8,
};

/**
 * @fg: foreground 0 or foreground 1
 * @bpp: foreground bpp
 * @x: foreground start position x
 * @y: foreground start position y
 * @w: foreground width
 * @h: foreground height
 */
struct jzfb_fg_t {
	u32 fg;
	u32 bpp;
	u32 x;
	u32 y;
	u32 w;
	u32 h;
};

/**
 *@decompress: enable decompress function, used by FG0
 *@block: enable 16x16 block function
 *@fg0: fg0 info
 *@fg1: fg1 info
 */
struct jzfb_osd_t {
	int decompress;
	int block;
	struct jzfb_fg_t fg0;
	struct jzfb_fg_t fg1;
};

struct jzfb {
	int id;           /* 0, lcdc0  1, lcdc1 */
	int is_enabled;   /* 0, disable  1, enable */
	int irq;          /* lcdc interrupt num */
	int flag;	/* fb_videomode->flag, but without FB_MODE_IS_HDMI */
	/* need_syspan
	 * 0: not need system pan display only hdmi (use in only hdmi)
	 * 1: need system pan display (used in lcd or(lcd and hdmi))
	 * */
	int need_syspan;
	int open_cnt;
	int irq_cnt;
	int desc_num;
	char clk_name[16];
	char pclk_name[16];
	char irq_name[16];

	struct fb_info *fb;
	struct device *dev;
	struct jzfb_platform_data *pdata;
	void __iomem *base;
	struct resource *mem;

	size_t vidmem_size;
	void *vidmem;
	dma_addr_t vidmem_phys;
	void *desc_cmd_vidmem;
	dma_addr_t desc_cmd_phys;

	int frm_size;
	int current_buffer0;
	int current_buffer1;
	/* dma 0 descriptor base address */
	struct jzfb_framedesc *framedesc[MAX_DESC_NUM];
	struct jzfb_framedesc *fg1_framedesc; /* FG 1 dma descriptor */
	dma_addr_t framedesc_phys;
	struct mutex framedesc_lock;

	wait_queue_head_t vsync_wq;
	struct task_struct *vsync_thread;
	ktime_t	vsync_timestamp;
	unsigned int vsync_skip_map; /* 10 bits width */
	int vsync_skip_ratio;
	int pan_sync; /* frame update sync */

	struct mutex lock;
	struct mutex suspend_lock;
	spinlock_t reg_lock;  /* Read and write registers need to lock. */

	enum jzfb_format_order fmt_order; /* frame buffer pixel format order */
	struct jzfb_osd_t osd; /* osd's config information */

	struct clk *clk;
	struct clk *pclk;
	struct clk *ipu_clk;
	struct clk *hdmi_clk;
	struct clk *hdmi_pclk;

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	int is_suspend;

	char eventbuf[64];
	int is_vsync;

	/* Add a new buffer and to display */
	struct jzfb_framedesc *new_fb_framedesc; /* new buffer descriptor */
	dma_addr_t new_fb_framedesc_phys; /* new buffer descriptor physical address */
	void *new_fb_vidmem; /* new framebuffer address */
	dma_addr_t new_fb_vidmem_phys; /* new framebuffer physical address */
#ifdef SLCD_DMA_RESTART_WORK_AROUND
        int slcd_dma_start_count;
#endif
	unsigned int pan_display_count;
};

static inline unsigned long reg_read(struct jzfb *jzfb, int offset)
{
	return readl(jzfb->base + offset);
}

static inline void reg_write(struct jzfb *jzfb, int offset, unsigned long val)
{
	writel(val, jzfb->base + offset);
}

/* structures for frame buffer ioctl */
struct jzfb_fg_pos {
	__u32 fg; /* 0:fg0, 1:fg1 */
	__u32 x;
	__u32 y;
};

struct jzfb_fg_size {
	__u32 fg;
	__u32 w;
	__u32 h;
};

struct jzfb_fg_alpha {
	__u32 fg; /* 0:fg0, 1:fg1 */
	__u32 enable;
	__u32 mode; /* 0:global alpha, 1:pixel alpha */
	__u32 value; /* 0x00-0xFF */
};

struct jzfb_bg {
	__u32 fg; /* 0:fg0, 1:fg1 */
	__u32 red;
	__u32 green;
	__u32 blue;
};

struct jzfb_color_key {
	__u32 fg; /* 0:fg0, 1:fg1 */
	__u32 enable;
	__u32 mode; /* 0:color key, 1:mask color key */
	__u32 red;
	__u32 green;
	__u32 blue;
};

struct jzfb_mode_res {
	__u32 index; /* 1-64 */
	__u32 w;
	__u32 h;
};

struct jzfb_aosd {
	__u32 aosd_enable;
	__u32 with_alpha;
	__u32 buf_phys_addr;
	__u32 buf_virt_addr;
	__u32 buf_size;
};

/* ioctl commands base fb.h FBIO_XXX */
/* image_enh.h: 0x142 -- 0x162 */
#define JZFB_GET_MODENUM		_IOR('F', 0x100, int)
#define JZFB_GET_MODELIST		_IOR('F', 0x101, int)
#define JZFB_SET_VIDMEM			_IOW('F', 0x102, unsigned int *)
#define JZFB_SET_MODE			_IOW('F', 0x103, int)
#define JZFB_ENABLE			_IOW('F', 0x104, int)
#define JZFB_GET_RESOLUTION		_IOWR('F', 0x105, struct jzfb_mode_res)

/* Reserved for future extend */
#define JZFB_SET_FG_SIZE		_IOW('F', 0x116, struct jzfb_fg_size)
#define JZFB_GET_FG_SIZE		_IOWR('F', 0x117, struct jzfb_fg_size)
#define JZFB_SET_FG_POS			_IOW('F', 0x118, struct jzfb_fg_pos)
#define JZFB_GET_FG_POS			_IOWR('F', 0x119, struct jzfb_fg_pos)
#define JZFB_GET_BUFFER			_IOR('F', 0x120, int)
#define JZFB_GET_LCDC_ID		_IOR('F', 0x121, int)
#define JZFB_GET_LCDTYPE		_IOR('F', 0x122, int)
/* Reserved for future extend */
#define JZFB_SET_ALPHA			_IOW('F', 0x123, struct jzfb_fg_alpha)
#define JZFB_SET_BACKGROUND		_IOW('F', 0x124, struct jzfb_bg)
#define JZFB_SET_COLORKEY		_IOW('F', 0x125, struct jzfb_color_key)
#define JZFB_AOSD_EN			_IOW('F', 0x126, struct jzfb_aosd)
#define JZFB_16X16_BLOCK_EN		_IOW('F', 0x127, int)
//#define JZFB_IPU0_TO_BUF		_IOW('F', 0x128, int)
//#define JZFB_ENABLE_IPU_CLK		_IOW('F', 0x129, int)
#define JZFB_ENABLE_LCDC_CLK		_IOW('F', 0x130, int)
/* Reserved for future extend */
#define JZFB_ENABLE_FG0			_IOW('F', 0x139, int)
#define JZFB_ENABLE_FG1			_IOW('F', 0x140, int)

/* Reserved for future extend */
#define JZFB_SET_VSYNCINT		_IOW('F', 0x210, int)

#define JZFB_SET_PAN_SYNC		_IOW('F', 0x220, int)

#define JZFB_SET_NEED_SYSPAN	_IOR('F', 0x310, int)
#define NOGPU_PAN				_IOR('F', 0x311, int)

/* Support for multiple buffer, can be switched. */
#define JZFB_ACQUIRE_BUFFER		_IOR('F', 0x441, int)	//acquire new buffer and to display
#define JZFB_RELEASE_BUFFER		_IOR('F', 0x442, int)	//release the acquire buffer
#define JZFB_CHANGE_BUFFER		_IOR('F', 0x443, int)	//change the acquire buffer

/* define in image_enh.c */
extern int jzfb_config_image_enh(struct fb_info *info);
extern int jzfb_image_enh_ioctl(struct fb_info *info, unsigned int cmd,
				unsigned long arg);
extern int update_slcd_frame_buffer(void);
