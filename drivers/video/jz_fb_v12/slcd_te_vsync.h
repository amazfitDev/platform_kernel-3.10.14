#ifndef __SLCD_TE_VSYNC_H__
#define __SLCD_TE_VSYNC_H__

/*
struct slcd_te {
	int te_gpio;
	int te_gpio_level;
	int te_irq_no;
	int refresh_request;
}; */


extern void request_te_vsync_refresh(struct jzfb *jzfb);
extern int jzfb_te_irq_register(struct jzfb *jzfb);


#endif /* __SLCD_TE_VSYNC_H__ */

