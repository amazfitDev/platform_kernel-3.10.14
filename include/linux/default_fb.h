#ifndef _DEFAULT_FB_H_
#define _DEFAULT_FB_H_

struct fb_ctrl {
	void (*power_off)(struct fb_ctrl *data);
	void (*power_on)(struct fb_ctrl *data);
};

extern void register_default_fb(struct fb_ctrl *fb);
extern void unregister_default_fb(struct fb_ctrl *fb);
extern void power_off_default_fb(void);
extern void power_on_default_fb(void);

#endif /* _DEFAULT_FB_H_ */
