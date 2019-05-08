#ifndef __LINUX_JZ_ADC_H__
#define __LINUX_JZ_ADC_H__

#include <linux/device.h>
#include <linux/jz4780_ts.h>
#include <linux/power/jz4780-battery.h>
/*
 * jz_adc_set_config - Configure a JZ4780 adc device
 * @dev: Pointer to a jz4780-adc device
 * @mask: Mask for the config value to be set
 * @val: Value to be set
 *
 * This function can be used by the JZ4780
 * ADC mfd cells to confgure their options in the shared config register.
 */
#if 0
int jz_adc_set_config(struct device *dev, uint32_t mask, uint32_t val);
#endif
struct jz_adc_platform_data{
	struct jz_battery_info battery_info;
	struct jz_ts_info ts_info;
};

int adc_write_reg(struct device *dev,uint8_t addr_offset,uint32_t mask,uint32_t val);
uint32_t adc_read_reg(struct device *dev,uint8_t addr_offset);
#endif /*jz_adc.h*/
