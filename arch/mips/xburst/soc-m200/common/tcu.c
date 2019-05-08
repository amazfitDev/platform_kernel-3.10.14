/* arch/mips/xburst/soc-m200/common/tcu.c
 * TCU driver for SOC M200
 *
 * NOTE: In M200, every channel tcu can be used as PWM, but, only channel
 * 0~3 can are available(can be output from gpio pin).
 *
 * Copyright (C) 2015 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 * Author: Sun Jiwei <jiwei.sun@ingenic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>

#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/syscore_ops.h>
#include <irq.h>
#include <linux/wakelock.h>
#include <linux/mutex.h>

#include <asm/div64.h>

#include <soc/base.h>
#include <soc/extal.h>
#include <soc/gpio.h>
#include <soc/tcu.h>
#include <soc/irq.h>

#include <mach/jztcu.h>

#define TCU_SIZE 0x150
#define regr(off)	readl(jz_tcu->reg + (off))
#define regw(val, off)	writel(val, jz_tcu->reg + (off))

#define tcu_enable_counter(id)	regw(BIT(id), TCU_TESR)
#define tcu_disable_counter(id)	regw(BIT(id), TCU_TECR)
#define tcu_start_clock(id)	regw(BIT(id), TCU_TSCR)
#define tcu_stop_clock(id)	regw(BIT(id), TCU_TSSR)

#define set_tcu_counter_value(id, count_value)	regw(count_value, CH_TCNT(id))

struct jz_tcu_t {
	void __iomem	*reg;
	struct mutex	lock;

	unsigned int irq;
	unsigned int irq_base;
};

static struct jz_tcu_t *jz_tcu;

static DECLARE_BITMAP(tcu_map, NR_TCU_CH) = {(1<<NR_TCU_CH) - 1, };

static int tcu_pwm_output_enable(int id)
{
	int tmp;
	if (id > 3) {
		pr_err("TCU %d is not support PWM output\n", id);
		dump_stack();
		return -ENXIO;
	}
	tmp = regr(CH_TCSR(id));
	regw((tmp | TCSR_PWM_EN), CH_TCSR(id));
	return 0;
}

static void tcu_pwm_output_disable(int id)
{
	int tmp;
	tmp = regr(CH_TCSR(id));
	regw(tmp & (~TCSR_PWM_EN), CH_TCSR(id));
}

static void tcu_shutdown_counter(struct tcu_device *tcu)
{
	if (tcu->pwm_flag)
		tcu_pwm_output_disable(tcu->id); /*disable PWM_EN*/

	tcu_disable_counter(tcu->id);

	if (tcu->tcumode == TCU2_MODE)
		while (regr(TCU_TSTR) & (1 << tcu->id)); /* noot endless loop */
}

static void tcu_clear_counter_to_zero(int id, enum tcu_mode mode)
{
	if (mode == TCU2_MODE) {
		int tmp = 0;
		tmp = regr(CH_TCSR(id));
		regw((tmp | TCSR_CNT_CLRZ), CH_TCSR(id));
	} else {
		regw(0, CH_TCNT(id));
	}
}

static void set_tcu_full_half_value(int id, unsigned int full_num,
		unsigned int half_num)
{
	regw(full_num, CH_TDFR(id));
	regw(half_num, CH_TDHR(id));
}

static void tcu_set_pwm_shutdown(int id, unsigned int shutdown)/*only use in TCU1_MODE*/
{
	int tmp;
	tmp = regr(CH_TCSR(id));
	if (shutdown)
		regw((tmp | TCSR_PWM_SD), CH_TCSR(id));
	else {
		regw((tmp & (~TCSR_PWM_SD)), CH_TCSR(id));
	}
}

static void tcu_set_start_state(struct tcu_device *tcu)
{
	tcu_disable_counter(tcu->id);
	tcu_start_clock(tcu->id);
	regw(0, CH_TCSR(tcu->id));
	if (tcu->tcumode == TCU1_MODE)
		tcu_set_pwm_shutdown(tcu->id, tcu->pwm_shutdown);
}

static void tcu_select_division_ratio(int id, unsigned int ratio)
{
	/*division ratio 1/4/16/256/1024/mask->no internal input clock*/
	int tmp;
	tmp = regr(CH_TCSR(id));
	switch (ratio) {
	case 0: regw((tmp | CSR_DIV1), CH_TCSR(id)); break;
	case 1: regw((tmp | CSR_DIV4), CH_TCSR(id)); break;
	case 2: regw((tmp | CSR_DIV16), CH_TCSR(id)); break;
	case 3: regw((tmp | CSR_DIV64), CH_TCSR(id)); break;
	case 4: regw((tmp | CSR_DIV256), CH_TCSR(id)); break;
	case 5: regw((tmp | CSR_DIV1024), CH_TCSR(id)); break;
	default:
		regw((tmp | CSR_DIV_MSK), CH_TCSR(id)); break;
	}
}

static void tcu_select_clk(int id, int clock)
{
	int tmp;
	tmp = regr(CH_TCSR(id));
	switch (clock) {
	case EXT_EN:  regw((tmp | CSR_EXT_EN), CH_TCSR(id)); break;
	case RTC_EN:  regw((tmp | CSR_RTC_EN), CH_TCSR(id)); break;
	case PCLK_EN: regw((tmp | CSR_PCK_EN), CH_TCSR(id)); break;
	case CLK_MASK:
		regw((tmp & (~CSR_CLK_MSK)), CH_TCSR(id));
	}
}

static void tcu_set_pwm_output_init_level(int id, int level)
{
	int tmp;
	tmp = regr(CH_TCSR(id));
	if (level){
		regw((tmp | TCSR_PWM_HIGH), CH_TCSR(id));
	} else {
		regw((tmp &(~TCSR_PWM_HIGH)), CH_TCSR(id));
	}
}

static void tcu_set_close_state(struct tcu_device *tcu)
{
	tcu_stop_clock(tcu->id);
	tcu_clear_counter_to_zero(tcu->id, tcu->tcumode);

	tcu->clock = CLK_MASK;
	tcu->pwm_flag = 0;
}

static void tcu_pwm_input_enable(int id)
{
	int tmp;
	tmp = regr(CH_TCSR(id));
	regw((tmp | (1 << 6)), CH_TCSR(id));
}

#if 0
static void tcu_pwm_input_disable(int id)
{
	int tmp;
	tmp = regr(CH_TCSR(id));
	regw(tmp & (~(1 << 6)), CH_TCSR(id));
}
#endif

int tcu_as_counter_config(struct tcu_device *tcu)
{
	mutex_lock(&tcu->tcu_lock);
	if ((tcu->id < 0) || (tcu->id > (NR_TCU_CH - 1))) {
		mutex_unlock(&tcu->tcu_lock);
		return -EINVAL;
	}
	if (test_bit(tcu->id, tcu_map)) {
		mutex_unlock(&tcu->tcu_lock);
		return -EINVAL;
	}
	tcu_set_start_state(tcu);

	set_tcu_counter_value(tcu->id, tcu->count_value);
	set_tcu_full_half_value(tcu->id, tcu->full_num, tcu->half_num);

	tcu_select_division_ratio(tcu->id, 0);

	tcu_pwm_output_disable(tcu->id);
	tcu_select_clk(tcu->id, CLK_MASK);
	tcu_pwm_input_enable(tcu->id);

	mutex_unlock(&tcu->tcu_lock);
	return 0;
}

int tcu_counter_read(struct tcu_device *tcu)
{
	int i = 0;
	int tmp = 0;
	if ((tcu->id < 0) || (tcu->id > (NR_TCU_CH - 1)))
		return -EINVAL;

	if (tcu->tcumode == TCU2_MODE) {
		while ((tmp == 0) && (i < 5)) {
			tmp = regr(TCU_TSTR) & (1 << (tcu->id + 16));
			i++;
		}
		if (tmp == 0)
			return -EINVAL; /*TCU MODE 2 may not read success*/
	}

	return (regr(CH_TCNT(tcu->id)));

}

/* NOTE: user selete irq mode according irq request */
int tcu_as_timer_config(struct tcu_device *tcu)
{
	mutex_lock(&tcu->tcu_lock);
	if ((tcu->id < 0) || (tcu->id > (NR_TCU_CH - 1))) {
		pr_err("numbuer isn't in 0~7\n");
		mutex_unlock(&tcu->tcu_lock);
		return -EINVAL;
	}
	if (test_bit(tcu->id, tcu_map)) {
		printk("already been used!\n");
		mutex_unlock(&tcu->tcu_lock);
		return -EINVAL;
	}

	tcu_set_start_state(tcu);

	set_tcu_counter_value(tcu->id, tcu->count_value);
	set_tcu_full_half_value(tcu->id, tcu->full_num, tcu->half_num);

	tcu_select_division_ratio(tcu->id, tcu->divi_ratio);
	if (tcu->pwm_flag)
		tcu_pwm_output_enable(tcu->id);
	tcu_select_clk(tcu->id, tcu->clock);

	if (tcu->init_level)
		tcu_set_pwm_output_init_level(tcu->id, 1);

	mutex_unlock(&tcu->tcu_lock);

	return 0;
}

struct tcu_device *tcu_request(int channel_num)
{
	struct tcu_device *tcu;

	tcu = kzalloc(sizeof(struct tcu_device), GFP_KERNEL);

	mutex_lock(&jz_tcu->lock);
	if ((channel_num < 0) || (channel_num > (NR_TCU_CH - 1))) {
		mutex_unlock(&jz_tcu->lock);
		kfree(tcu);
		return ERR_PTR(-ENODEV);
	}
	if (!test_bit(channel_num, tcu_map)) {
		mutex_unlock(&jz_tcu->lock);
		kfree(tcu);
		return ERR_PTR(-EBUSY);
	}
#ifdef RESERVED_CH
	if (channel_num == RESERVED_CH) {
		mutex_unlock(&jz_tcu->lock);
		kfree(tcu);
		return ERR_PTR(-EBUSY);
	}
#endif

	tcu->id = channel_num;

	if ((tcu->id == 1) || (tcu->id == 2))
		tcu->tcumode = TCU2_MODE;
	else
		tcu->tcumode = TCU1_MODE;
	pr_debug("request tcu channel number:%d\n", channel_num);

	mutex_init(&(tcu->tcu_lock));
	clear_bit(channel_num, tcu_map);
	mutex_unlock(&jz_tcu->lock);
	return tcu;
}

/*FIXME, The function is not standard, it is because
 * the TCU controller designed shortage, only TCU1 mode can
 * set default output level, but TCU2 doesn't. Only for M200*/
static int tcu_set_fun_pin(struct tcu_device *tcu)
{
	int ret = -EIO;
	switch (tcu->id) {
	case 0:
		ret = jzgpio_set_func(GPIO_PORT_E, GPIO_FUNC_0, BIT(0));
		break;
	case 1:
		ret = jzgpio_set_func(GPIO_PORT_E, GPIO_FUNC_0, BIT(1));
		break;
	case 2:
		ret = jzgpio_set_func(GPIO_PORT_E, GPIO_FUNC_0, BIT(2));
		break;
	case 3:
		ret = jzgpio_set_func(GPIO_PORT_E, GPIO_FUNC_0, BIT(3));
		break;
	default:
		pr_warn("TCU:%d, has not default pin,"
				" Please check your code\n",
				tcu->id);
		break;
	}

	return ret;
}

/*FIXME, The function is not standard, it is because
 * the TCU controller designed shortage, only TCU1 mode can
 * set default output level, but TCU2 doesn't. Only for M200*/
static int tcu_set_pin_to_default_level(struct tcu_device *tcu)
{
	int ret = -EIO;
	switch (tcu->id) {
	case 0:
		ret = jzgpio_set_func(GPIO_PORT_E, (tcu->init_level ?
				GPIO_OUTPUT1 : GPIO_OUTPUT0), BIT(0));
		break;
	case 1:
		ret = jzgpio_set_func(GPIO_PORT_E, (tcu->init_level ?
				GPIO_OUTPUT1 : GPIO_OUTPUT0), BIT(1));
		break;
	case 2:
		ret = jzgpio_set_func(GPIO_PORT_E, (tcu->init_level ?
				GPIO_OUTPUT1 : GPIO_OUTPUT0), BIT(2));
		break;
	case 3:
		ret = jzgpio_set_func(GPIO_PORT_E, (tcu->init_level ?
				GPIO_OUTPUT1 : GPIO_OUTPUT0), BIT(3));
		break;
	default:
		pr_warn("TCU:%d, has not default pin,"
				" Please check your code\n",
				tcu->id);
		break;
	}
	return ret;
}

int tcu_enable(struct tcu_device *tcu)
{
	int ret = 0;
	mutex_lock(&tcu->tcu_lock);


	if (!tcu->enable_cnt) {
		ret = tcu_set_fun_pin(tcu);
		if (ret) {
			printk("Err:tcu set function pin error\n");
			mutex_unlock(&tcu->tcu_lock);
			return ret;
		}
		if (tcu->pwm_flag) {
			ret = tcu_pwm_output_enable(tcu->id);
		}
		tcu_enable_counter(tcu->id);
	}
	tcu->enable_cnt++;

	mutex_unlock(&tcu->tcu_lock);

	return ret;
}

void tcu_disable(struct tcu_device *tcu)
{
	mutex_lock(&tcu->tcu_lock);
	if (!tcu || test_bit(tcu->id, tcu_map)) {
		mutex_unlock(&tcu->tcu_lock);
		return;
	}
	if (!tcu->enable_cnt) {
		pr_warn("TCU enable and disable is not corresponding\n ");
		dump_stack();
		mutex_unlock(&tcu->tcu_lock);
		return;
	}
	tcu->enable_cnt--;

	if (!tcu->enable_cnt) {
		int ret;
		tcu_shutdown_counter(tcu);
		ret = tcu_set_pin_to_default_level(tcu);
		if (ret) {
			printk("Err:tcu set pin to default level error\n");
			WARN_ON(1);
			dump_stack();
		}
	}


	mutex_unlock(&tcu->tcu_lock);
}

void tcu_free(struct tcu_device *tcu)
{
	mutex_lock(&tcu->tcu_lock);
	tcu_set_close_state(tcu);
	set_bit(tcu->id, tcu_map);
	mutex_unlock(&tcu->tcu_lock);
	kfree(tcu);
}

static void jz_tcu_irq_mask(struct irq_data *data)
{
	struct jz_tcu_t *tcu = irq_data_get_irq_chip_data(data);
	unsigned int bit_tmp;
	bit_tmp = data->irq - tcu->irq_base;
	if (bit_tmp >= 12) {
		bit_tmp += 3;
	}

	regw(1 << BIT(bit_tmp), TCU_TMSR);
}

static void jz_tcu_irq_unmask(struct irq_data *data)
{
	struct jz_tcu_t *tcu = irq_data_get_irq_chip_data(data);
	unsigned int bit_tmp;

	bit_tmp = data->irq - tcu->irq_base;
	if (bit_tmp >= 12) {
		bit_tmp += 3;
	}

	regw(BIT(bit_tmp), TCU_TMCR);
}

static void jz_tcu_irq_ack(struct irq_data *data)
{
	struct jz_tcu_t *tcu = irq_data_get_irq_chip_data(data);
	unsigned int bit_tmp = data->irq - tcu->irq_base;
	if (bit_tmp >= 12) {
		bit_tmp += 3;
	}

	regw(BIT(bit_tmp), TCU_TFCR);
}

static struct irq_chip jz_tcu_irq_chip = {
	.name = "jz-tcu",
	.irq_mask = jz_tcu_irq_mask,
	.irq_disable = jz_tcu_irq_mask,
	.irq_unmask = jz_tcu_irq_unmask,
	.irq_ack = jz_tcu_irq_ack,
};

static void jz_tcu_irq_demux(unsigned int irq, struct irq_desc *desc)
{
	struct jz_tcu_t *tcu = irq_desc_get_handler_data(desc);
	unsigned int flags;
	unsigned int mask;
	unsigned int i;

	flags = regr(TCU_TFR);
	mask = ~regr(TCU_TMR);

	/* NOTE: CHANNEL 5 is used TCU1 irq */
	for (i = 0; i < 5; i++) {
		if ((mask & BIT(i)) && (flags & BIT(i))) {
			generic_handle_irq(tcu->irq_base + i);
		}
	}

	for (i = 6; i < 12; i++) {
		if ((mask & BIT(i)) && (flags & BIT(i))) {
			generic_handle_irq(tcu->irq_base + i);
		}
	}

	/* NOTE: CHANNEL 5 is used TCU1 irq, OST use TCU0 irq */
	for (i = 13; i < 18; i++) {
		if ((mask & BIT(i + 3)) && (flags & BIT(i + 3))) {
			generic_handle_irq(tcu->irq_base + i);
		}
	}
	for (i = 19; i < TCU_NR_IRQS; i++) {
		if ((mask & BIT(i + 3)) && (flags & BIT(i + 3))) {
			generic_handle_irq(tcu->irq_base + i);
		}
	}
}

int __init tcu_init(void)
{
	unsigned int irq;
	jz_tcu = (struct jz_tcu_t *)kzalloc(sizeof(struct jz_tcu_t),
			GFP_KERNEL);
	jz_tcu->reg = ioremap(TCU_IOBASE, TCU_SIZE);
	mutex_init(&jz_tcu->lock);
	jz_tcu->irq = IRQ_TCU2;
	jz_tcu->irq_base = IRQ_TCU_BASE;

	for (irq = IRQ_TCU_BASE; irq <= IRQ_TCU_END; ++irq) {
		irq_set_chip_data(irq, jz_tcu);
		irq_set_chip_and_handler(irq, &jz_tcu_irq_chip,
				handle_level_irq);
	}

	irq_set_handler_data(jz_tcu->irq, jz_tcu);
	irq_set_chained_handler(jz_tcu->irq, jz_tcu_irq_demux);

	return 0;
}
arch_initcall(tcu_init);
