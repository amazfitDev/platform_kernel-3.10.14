
#ifndef _IMAGE_ENH_H_
#define _IMAGE_ENH_H_

struct enh_gamma {
	__u32 gamma_en:1;
	__u16 gamma_data[1024];
};

struct enh_csc {
	__u32 rgb2ycc_en:1;
	__u32 rgb2ycc_mode;
	__u32 ycc2rgb_en:1;
	__u32 ycc2rgb_mode;
};

struct enh_luma {
	__u32 brightness_en:1;
	__u32 brightness;
	__u32 contrast_en:1;
	__u32 contrast;
};

struct enh_hue {
	__u32 hue_en:1;
	__u32 hue_sin;
	__u32 hue_cos;
};

struct enh_chroma {
	__u32 saturation_en:1;
	__u32 saturation;
};

struct enh_vee {
	__u32 vee_en:1;
	__u16 vee_data[1024];
};

struct enh_dither {
	__u32 dither_en:1;
	__u32 dither_red;
	__u32 dither_green;
	__u32 dither_blue;
};

/* image enhancement ioctl commands */
#define JZFB_GET_GAMMA			_IOR('F', 0x141, struct enh_gamma)
#define JZFB_SET_GAMMA			_IOW('F', 0x142, struct enh_gamma)
#define JZFB_GET_CSC			_IOR('F', 0x143, struct enh_csc)
#define JZFB_SET_CSC			_IOW('F', 0x144, struct enh_csc)
#define JZFB_GET_LUMA			_IOR('F', 0x145, struct enh_luma)
#define JZFB_SET_LUMA			_IOW('F', 0x146, struct enh_luma)
#define JZFB_GET_HUE			_IOR('F', 0x147, struct enh_hue)
#define JZFB_SET_HUE			_IOW('F', 0x148, struct enh_hue)
#define JZFB_GET_CHROMA			_IOR('F', 0x149, struct enh_chroma)
#define JZFB_SET_CHROMA			_IOW('F', 0x150, struct enh_chroma)
#define JZFB_GET_VEE			_IOR('F', 0x151, struct enh_vee)
#define JZFB_SET_VEE			_IOW('F', 0x152, struct enh_vee)
/* Reserved for future extend */
#define JZFB_GET_DITHER			_IOR('F', 0x158, struct enh_dither)
#define JZFB_SET_DITHER			_IOW('F', 0x159, struct enh_dither)
#define JZFB_ENABLE_ENH			_IOW('F', 0x160, struct enh_dither)


//#define SOURCE_BUFFER_SIZE 0x400000 /* 4M */
#define SOURCE_BUFFER_SIZE 0x1000000 /* 16M */
#define START_ADDR_ALIGN 0x1000 /* 4096 byte */
#define STRIDE_ALIGN 0x800 /* 2048 byte */
#define PIXEL_ALIGN 16 /* 16 pixel */

//#define FRAME_SIZE  70 /* 70 frame of y and u/v video data */
#define FRAME_SIZE 60  /* 60 frame of y and u/v video data */ /* test */

/* LCDC ioctl commands */
#define JZFB_GET_MODENUM		_IOR('F', 0x100, int)
#define JZFB_GET_MODELIST		_IOR('F', 0x101, int)
#define JZFB_SET_VIDMEM			_IOW('F', 0x102, unsigned int *)
#define JZFB_SET_MODE			_IOW('F', 0x103, int)
#define JZFB_ENABLE			_IOW('F', 0x104, int)
#define JZFB_GET_RESOLUTION		_IOWR('F', 0x105, struct jzfb_mode_res)

#define JZFB_SET_ALPHA			_IOW('F', 0x123, struct jzfb_fg_alpha)
#define JZFB_SET_COLORKEY		_IOW('F', 0x125, struct jzfb_color_key)
#define JZFB_SET_BACKGROUND		_IOW('F', 0x124, struct jzfb_bg)

#define JZFB_ENABLE_FG0			_IOW('F', 0x139, int)

/* hdmi ioctl commands */
#define HDMI_POWER_OFF			_IO('F', 0x301)
#define	HDMI_VIDEOMODE_CHANGE		_IOW('F', 0x302, int)
#define	HDMI_POWER_ON			_IO('F', 0x303)

static unsigned int tlb_base_phys;

struct source_info {
	unsigned int width;
	unsigned int height;
	void *y_buf_v;
	void *u_buf_v;
	void *v_buf_v;
};

struct dest_info {
	unsigned int width;
	unsigned int height;
	unsigned long size;
	int y_stride;
	int u_stride;
	int v_stride;
	void *out_buf_v;
};

struct jzfb_fg_alpha {
	__u32 fg; /* 0:fg0, 1:fg1 */
	__u32 enable;
	__u32 mode; /* 0:global alpha, 1:pixel alpha */
	__u32 value; /* 0x00-0xFF */
};
struct jzfb_color_key {
	__u32 fg; /* 0:fg0, 1:fg1 */
	__u32 enable;
	__u32 mode; /* 0:color key, 1:mask color key */
	__u32 red;
	__u32 green;
	__u32 blue;
};

struct jzfb_bg {
	__u32 fg; /* 0:fg0, 1:fg1 */
	__u32 red;
	__u32 green;
	__u32 blue;
};

struct jzfb_mode_res {
	__u32 index; /* 1-64 */
	__u32 w;
	__u32 h;
};






#endif /* _IMAGE_ENH_H_ */
