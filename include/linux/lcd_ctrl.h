#ifndef _LCD_CTRL_H_
#define _LCD_CTRL_H_

struct jzfb;

extern void jzfb_ctrl_register(struct jzfb *jzfb);
extern void jzfb_ctrl_unregister(struct jzfb *jzfb);
extern const char *jzfb_get_lcd_name(int id);
extern struct jzfb *get_jzfb_by_lcd_name(const char *name);

#endif /* _LCD_CTRL_H_ */
