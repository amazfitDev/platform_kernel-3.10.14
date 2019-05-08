#include <linux/fb.h>

#define VFB_XRES CONFIG_VFB_XRES
#define VFB_YRES CONFIG_VFB_YRES
#define VFB_BPP	 CONFIG_VFB_BPP
#ifdef CONFIG_TWO_FRAME_BUFFERS
#define NUM_FRAME_BUFFERS 2
#endif
#ifdef CONFIG_THREE_FRAME_BUFFERS
#define NUM_FRAME_BUFFERS 3
#endif

#define PIXEL_ALIGN 4

enum jzfb_format_order {
	FORMAT_X8R8G8B8 = 1,
	FORMAT_X8B8G8R8,
};

struct jzfb {
	struct fb_info *fb;
	struct device *dev;
	struct jzfb_platform_data *pdata;

	size_t vidmem_size;
	void *vidmem;
	dma_addr_t vidmem_phys;

	int current_buffer;

	enum jzfb_format_order fmt_order;	/* frame buffer pixel format order */
};

struct jzfb_platform_data {
	size_t num_modes;
	struct fb_videomode *modes;
	unsigned int bpp;
	unsigned int width;
	unsigned int height;
};
