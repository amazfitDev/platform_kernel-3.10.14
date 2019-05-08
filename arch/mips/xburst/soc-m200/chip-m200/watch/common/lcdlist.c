#include <linux/rootlist.h>

#define HWLIST_LCD_INFO_DEF(chip, name, size, dpi, ppi, exterior) do {	\
	hwlist_lcd_chip(chip);        \
	hwlist_lcd_name(name);        \
	hwlist_lcd_size(size);        \
	hwlist_lcd_dpi(dpi);          \
	hwlist_lcd_ppi(ppi);		  \
	hwlist_lcd_exterior(exterior);\
} while (0);

void setup_lcd_hwlist(void)
{
#if defined(CONFIG_LCD_EDO_E1392AM1)
	HWLIST_LCD_INFO_DEF("edo", "e1392am1",  "1.39", "240", "", "round");
#elif defined(CONFIG_LCD_AUO_H139BLN01)
	HWLIST_LCD_INFO_DEF("auo", "h139bln01", "1.39", "240", "", "round");
#elif defined(CONFIG_LCD_X163)
	HWLIST_LCD_INFO_DEF("auo", "x163", "1.63", "240", "", "square");
#elif defined(CONFIG_LCD_ORISE_OTM3201A)
	HWLIST_LCD_INFO_DEF("orise", "otm3201a", "1.5", "240", "", "round");
#elif defined(CONFIG_LCD_BOE_TFT320320)
	HWLIST_LCD_INFO_DEF("boe", "", "1.54", "240", "", "square");
#elif defined(CONFIG_LCD_H160_TFT320320)
	HWLIST_LCD_INFO_DEF("auo", "h160", "1.54", "240", "", "square");
#else
#error "not support your lcd, please add your lcd's info here"
#endif
}
