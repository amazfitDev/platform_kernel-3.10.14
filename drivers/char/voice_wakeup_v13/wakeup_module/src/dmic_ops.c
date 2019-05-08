#include <jz_cpm.h>
#include <jz_gpio.h>
#include "dmic_config.h"
#include "dmic_ops.h"
#include "voice_wakeup.h"
#include "interface.h"
#include "trigger_value_adjust.h"
#include "tcu_timer.h"


int dmic_current_state;
unsigned int cur_thr_value;
unsigned int wakeup_failed_times;
unsigned int dmic_recommend_thr;
unsigned int last_dma_count;

//#define DMIC_FIFO_THR	(48) /* 48 * 4Bytes, or 48 * 2 Bytes.*/
#define DMIC_FIFO_THR	(48) /* 48 * 4Bytes, or 48 * 2 Bytes.*/
//#define DMIC_FIFO_THR	(128)


#define __dmic_reset()									\
	do {												\
		REG_DMIC_CR0 |= (1<< DMICCR0_RESET);		\
		while (REG_DMIC_CR0 & (1 << DMICCR0_RESET));		\
	} while (0)
#define __dmic_reset_tri()			\
	do {							\
		REG_DMIC_CR0 |= (1 << DMICCR0_RESET_TRI);		\
		while(REG_DMIC_CR0 & (1<<DMICCR0_RESET_TRI));	\
	} while(0)

#ifdef DMIC_VOICE_DEBUG
void dump_dmic_regs(void)
{
	printk("REG_DMIC_CR0			%08x\n",	REG_DMIC_CR0		);
	printk("REG_DMIC_GCR            %08x\n",    REG_DMIC_GCR		);
	printk("REG_DMIC_IMR            %08x\n",    REG_DMIC_IMR		);
	printk("REG_DMIC_ICR            %08x\n",    REG_DMIC_ICR		);
	printk("REG_DMIC_TRICR          %08x\n",    REG_DMIC_TRICR	);
	printk("REG_DMIC_THRH           %08x\n",    REG_DMIC_THRH		);
	printk("REG_DMIC_THRL           %08x\n",    REG_DMIC_THRL		);
	printk("REG_DMIC_TRIMMAX        %08x\n",    REG_DMIC_TRIMMAX	);
	printk("REG_DMIC_TRINMAX        %08x\n",    REG_DMIC_TRINMAX	);
	printk("REG_DMIC_DR             %08x\n",    REG_DMIC_DR		);
	printk("REG_DMIC_FCR            %08x\n",    REG_DMIC_FCR		);
	printk("REG_DMIC_FSR            %08x\n",    REG_DMIC_FSR		);
	printk("REG_DMIC_CGDIS          %08x\n",    REG_DMIC_CGDIS	);
}


void dump_gpio()
{
	printk("REG_GPIO_PXINT(5)%08x\n", REG_GPIO_PXINT(5));
	printk("REG_GPIO_PXMASK(5):%08x\n", REG_GPIO_PXMASK(5));
	printk("REG_GPIO_PXPAT1(5):%08x\n", REG_GPIO_PXPAT1(5));
	printk("REG_GPIO_PXPAT0(5):%08x\n", REG_GPIO_PXPAT0(5));
}
void dump_cpm_reg()
{
	printk("CPM_LCR:%08x\n", REG32(CPM_IOBASE + CPM_LCR));
	printk("CPM_CLKGR0:%08x\n", REG32(CPM_IOBASE + CPM_CLKGR0));
	printk("I2S_DEVCLK:%08x\n", REG32(CPM_IOBASE + CPM_I2SCDR));
}
#endif
void dmic_clk_config(void)
{
	/*1. cgu config 24MHz , extern clock */
	/* dmic need a 24MHZ clock from cpm */
	REG32(CPM_IOBASE + CPM_I2SCDR) = 0;
	REG32(CPM_IOBASE + CPM_I2SCDR) |= 2 << 30;
	REG32(CPM_IOBASE + CPM_I2SCDR) |= ((1 << 13) | 1);
	REG32(CPM_IOBASE + CPM_I2SCDR) |= 1 << 29;
	REG32(CPM_IOBASE + CPM_I2SCDR1) |= 1;

	/*2.gate clk*/
	REG32(CPM_IOBASE + CPM_CLKGR0) &= ~(1 << 17); /* clk on */

	/*3.extern clk sleep mode enable*/
	REG32(CPM_IOBASE + CPM_OPCR) |= 1 << 4;
	/*4. l2 cache power on */
	REG32(CPM_IOBASE + CPM_OPCR) &= ~(1 << 25);
#ifdef DMIC_VOICE_DEBUG
	dump_cpm_reg();
#endif
}
void gpio_as_dmic(void)
{
	/* dmic 0, PB21,PB22, func 0*/
	REG_GPIO_PXINTC(1)	|= 3 << 21;
	REG_GPIO_PXMASKC(1) |= 3 << 21;
	REG_GPIO_PXPAT1C(1) |= 3 << 21;
	REG_GPIO_PXPAT0C(1) |= 3 << 21;

	/* dmic 1, PB05 func 1*/
	REG_GPIO_PXINTC(1)	|= 1 << 5;
	REG_GPIO_PXMASKC(1) |= 1 << 5;
	REG_GPIO_PXPAT1C(1) |= 1 << 5;
	REG_GPIO_PXPAT0S(1) |= 1 << 5;
#ifdef DMIC_VOICE_DEBUG
	dump_gpio();
#endif
}

int dmic_set_channel(unsigned long channels)
{
	int ret;
	if(channels > 0 && channels < 4) {
		REG_DMIC_CR0 &= ~(0x7<<16);
		REG_DMIC_CR0 |= (--channels) << 16;
		ret = 0;
	} else {
		printk("err channel num :%d\n", channels);
		ret = -1;
	}

	return ret;

}
void dmic_init(void)
{
	gpio_as_dmic();
	dmic_clk_config();
	__dmic_reset();
	__dmic_reset_tri();
}

int cpu_should_sleep(void)
{
	return ((REG_DMIC_FSR & 0x7f) < (DMIC_FIFO_THR - 10)) &&(last_dma_count != REG_DMADTC(5)) ? 1 : 0;
}

int dmic_init_mode(int mode)
{
	dmic_init();
	switch (mode) {
		case EARLY_SLEEP:

			break;
		case DEEP_SLEEP:
			dmic_current_state = WAITING_TRIGGER;
			wakeup_failed_times = 0;
			cur_thr_value = thr_table[0];
			dmic_recommend_thr = 0;

			REG_DMIC_CR0 = 0;
			REG_DMIC_CR0 |= 1<<6;
			REG_DMIC_CR0 |= 0 << 16;
			REG_DMIC_CR0 |= 1<<8;
			REG_DMIC_FCR |= 1 << 31 | 48;
			REG_DMIC_IMR |= 0x3f; /*mask all ints*/
			REG_DMIC_GCR = 10;
			REG_DMIC_CR0 |= 1<<2; /*hpf1en*/
			REG_DMIC_THRL = cur_thr_value;/*SET A MIDDLE THR_VALUE, AS INIT */
			REG_DMIC_TRICR	|= 1 <<3; /*hpf2 en*/
			REG_DMIC_TRICR	|= 2 << 1; /* prefetch 16k*/

#ifdef CONFIG_CPU_IDLE_SLEEP
			/* trigger mode config */
			REG_DMIC_TRICR &= ~(0xf << 16); /* tri mode: larger than thr trigger.*/
#else
			REG_DMIC_TRICR &= ~(0xf << 16);
			REG_DMIC_TRICR |= 2<<16;
			REG_DMIC_TRINMAX = 5;
			REG_DMIC_TRIMMAX = 3000000;
#endif

			REG_DMIC_CR0 |= 1<<1; /*ENABLE DMIC tri*/
			REG_DMIC_ICR |= 0x3f; /*clear all ints*/
			REG_DMIC_IMR &= ~(1 << 0);/*enable tri int*/
			REG_DMIC_IMR &= ~(1 << 4);/*enable tri int*/

			break;
		case NORMAL_RECORD:
			REG_DMIC_CR0 = 0;
			REG_DMIC_CR0 |= 1<<6;
			/* channel = 1 */
			REG_DMIC_CR0 |= 0 << 16;
			/*packen ,unpack disable*/
			REG_DMIC_CR0 |= 1<<8 ;//| 1 << 12;
			//REG_DMIC_CR0 &= ~(1<<8 | 1 << 12);

			REG_DMIC_FCR |= 1 << 31 | 48;
			//REG_DMIC_IMR &= ~(0x1f);
			REG_DMIC_IMR |= 0x3f; /*mask all ints*/
			REG_DMIC_ICR |= 0x3f; /*mask all ints*/
			REG_DMIC_GCR = 9;

			REG_DMIC_CR0 |= 1<<2; /*hpf1en*/

			REG_DMIC_TRICR &= ~(0xf << 16);
			REG_DMIC_TRICR |= 0 << 16; /*trigger mode*/
			REG_DMIC_TRICR	|= 1 <<3; /*hpf2 en*/
			REG_DMIC_TRICR	|= 2 << 1; /* prefetch 16k*/
			break;
		default:
			break;

	}
	dmic_set_channel(1);
	return 0;
}

int dmic_enable(void)
{
	REG_DMIC_CR0 |= 1 << 0; /*ENABLE DMIC*/
#ifdef DMIC_VOICE_DEBUG
	dump_dmic_regs();
#endif
	return 0;
}

int dmic_disable_tri(void)
{
	REG_DMIC_CR0 &= ~(1<<1);
	REG_DMIC_ICR |= 0x3f;
	REG_DMIC_IMR |= 0x3f;

	return 0;
}
int dmic_disable(void)
{
	REG_DMIC_CR0 &= ~(1 << 0); /*DISABLE DMIC*/
	return 0;
}



void reconfig_thr_value()
{
	if(dmic_current_state == WAITING_TRIGGER) {
		/* called by rtc timer, or last wakeup failed .*/
		if(dmic_recommend_thr != 0) {
			cur_thr_value = dmic_recommend_thr;
			REG_DMIC_THRL = cur_thr_value;
			dmic_recommend_thr = 0;
		} else {
			REG_DMIC_THRL = cur_thr_value;
		}
	} else if(dmic_current_state == WAITING_DATA) {
		/* called only by rtc timer, we can not change thr here.
		 * should wait dmic waiting trigger state.
		 * */
		dmic_recommend_thr = adjust_trigger_value(wakeup_failed_times+1, cur_thr_value);

		wakeup_failed_times = 0;
	}
}

int dmic_set_samplerate(unsigned long rate)
{
	int ret;
	if(rate == 8000) {
		REG_DMIC_CR0 &= ~(3<<6);
		ret = 0;
	} else if(rate == 16000) {
		REG_DMIC_CR0 &= ~(3<<6);
		REG_DMIC_CR0 |= (1<<6);
		ret = 0;
	} else {
		printk("dmic does not support samplerate:%d\n", rate);
		ret = -1;
	}

	return ret;

}
int dmic_ioctl(int cmd, unsigned long args)
{
	switch (cmd) {
		case DMIC_IOCTL_SET_SAMPLERATE:
			dmic_set_samplerate(args);
			break;
		case DMIC_IOCTL_SET_CHANNEL:
			dmic_set_channel(args);
			break;
		default:
			break;
	}

	return 0;
}

#define is_int_rtc(in)		(in & (1 << 0))

int dmic_handler(int pre_ints)
{
	volatile int ret;

	REG_DMIC_ICR |= 0x3f;
	REG_DMIC_IMR |= 1<<0 | 1<<4;

	last_dma_count = REG_DMADTC(5);

	if(dmic_current_state == WAITING_TRIGGER) {
		if(is_int_rtc(pre_ints)) {
//			TCSM_PCHAR('F');
			REG_DMIC_ICR |= 0x3f;
			REG_DMIC_IMR &= ~(1<<0 | 1<<4);
			return SYS_WAKEUP_FAILED;
		}

		dmic_current_state = WAITING_DATA;
		/*change trigger value to 0, make dmic wakeup cpu all the time*/
#ifdef CONFIG_CPU_IDLE_SLEEP
		tcu_timer_mod(ms_to_count(TCU_TIMER_MS)); /* start a timer */
#else
		REG_DMIC_THRL = 0;
#endif
	} else if (dmic_current_state == WAITING_DATA){

	}
#ifdef CONFIG_CPU_SWITCH_FREQUENCY
	ret = process_dma_data_3();
#else
	ret = process_dma_data_2();
#endif
	if(ret == SYS_WAKEUP_OK) {
		return SYS_WAKEUP_OK;

	} else if(ret == SYS_NEED_DATA) {
#ifdef CONFIG_CPU_IDLE_SLEEP
		/* do nothing */
#else
		REG_DMIC_TRINMAX = 5;
		REG_DMIC_CR0 |= 1 << 6;
		REG_DMIC_IMR &= ~( 1<<4 | 1<<0);
#endif
		last_dma_count = REG_DMADTC(5);
		return SYS_NEED_DATA;
	} else if(ret == SYS_WAKEUP_FAILED) {
		/*
		 * if current wakeup operation failed. we need reconfig dmic
		 * to work at appropriate mode.
		 * */
		dmic_current_state = WAITING_TRIGGER;
		wakeup_failed_times++;
		TCSM_PCHAR('F');
		TCSM_PCHAR('A');
		TCSM_PCHAR('I');
		TCSM_PCHAR('L');
		TCSM_PCHAR('E');
		TCSM_PCHAR('D');
		reconfig_thr_value();
#ifdef CONFIG_CPU_IDLE_SLEEP
		/* del a timer, when dmic trigger. it will re start a timer */
		tcu_timer_del();
#endif

		/* change trigger mode to > N times*/
		//REG_DMIC_TRICR |= 2 << 16;
		REG_DMIC_TRINMAX = 5;
		REG_DMIC_TRICR |= 1<<0; /*clear trigger*/


		REG_DMIC_CR0 |= 3<<6; /* disable data path*/

		REG_DMIC_ICR |= 0x3f;
		REG_DMIC_IMR &= ~(1<<0 | 1<<4);

		return SYS_WAKEUP_FAILED;
	}
	return SYS_WAKEUP_FAILED;
}
