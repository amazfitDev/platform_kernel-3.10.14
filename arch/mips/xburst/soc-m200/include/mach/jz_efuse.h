#ifndef __JZ_EFUSE_H__
#define __JZ_EFUSE_H__

struct jz_efuse_platform_data {
	int gpio_vddq_en;	/* supply 2.5V to VDDQ */
	int gpio_vddq_en_level;
};

#ifdef CONFIG_JZ_EFUSE
int read_jz_efuse(uint32_t xaddr, uint32_t xlen, void *buf);
#else
static inline int read_jz_efuse(uint32_t xaddr, uint32_t xlen, void *buf)
{
	return -ENODEV;
}
#endif
#endif
