/*
 * linux/arch/mips/xburst/soc-xxx/common/pm_p0.c
 *
 *  X1000 Power Management Routines
 *  Copyright (C) 2006 - 2012 Ingenic Semiconductor Inc.
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 */

#include <linux/init.h>
#include <linux/pm.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/suspend.h>
#include <linux/proc_fs.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/sysctl.h>
#include <linux/delay.h>
#include <asm/fpu.h>
#include <linux/syscore_ops.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/notifier.h>
#include <asm/cacheops.h>
#include <soc/cache.h>
#include <asm/r4kcache.h>
#include <soc/base.h>
#include <soc/cpm.h>
#include <soc/tcu.h>
#include <soc/gpio.h>
#include <soc/ddr.h>
#include <tcsm.h>
#include <smp_cp0.h>

#include <soc/tcsm_layout.h>

#ifdef CONFIG_JZ_DMIC_WAKEUP_V13
#include<linux/voice_wakeup_module.h>
#endif

extern long long save_goto(unsigned int);
extern int restore_goto(void);
extern unsigned int get_pmu_slp_gpio_info(void);
extern unsigned int _regs_stack[64];
static noinline void cpu_resume(void);

#define get_cp0_ebase()	__read_32bit_c0_register($15, 1)

#define OFF_TDR         (0x00)
#define OFF_LCR         (0x0C)
#define OFF_LSR         (0x14)

#define LSR_TDRQ        (1 << 5)
#define LSR_TEMT        (1 << 6)

//#define DDR_TRAINING
//#define DDR_TEST

#define PRINT_DEBUG

#ifdef PRINT_DEBUG
#define U_IOBASE (UART2_IOBASE + 0xa0000000)
#define TCSM_PCHAR(x)							\
	*((volatile unsigned int*)(U_IOBASE+OFF_TDR)) = x;		\
	while ((*((volatile unsigned int*)(U_IOBASE + OFF_LSR)) & (LSR_TDRQ | LSR_TEMT)) != (LSR_TDRQ | LSR_TEMT))
#else
#define TCSM_PCHAR(x)
#endif

#define TCSM_DELAY(x)						\
	do{							\
		register unsigned int i = x;			\
		while(i--)					\
			__asm__ volatile(".set mips32\n\t"	\
					 "nop\n\t"		\
					 ".set mips32");	\
	}while(0)						\

static inline void serial_put_hex(unsigned int x) {
	int i;
	unsigned int d;
	for(i = 7;i >= 0;i--) {
		d = (x  >> (i * 4)) & 0xf;
		if(d < 10) d += '0';
		else d += 'A' - 10;
		TCSM_PCHAR(d);
	}
	/* TCSM_PCHAR('\r'); */
	/* TCSM_PCHAR('\n'); */
}
static inline void set_gpio_func(int gpio, int type) {
	int i;
	int port = gpio / 32;
	int pin = gpio & 0x1f;
	int addr = 0xb0010010 + port * 0x100;

	for(i = 0;i < 4;i++){
		REG32(addr + 0x10 * i) &= ~(1 << pin);
		REG32(addr + 0x10 * i) |= (((type >> (3 - i)) & 1) << pin);
	}
}
#define UNIQUE_ENTRYHI(idx) (CKSEG0 + ((idx) << (PAGE_SHIFT + 1)))
static inline void local_flush_tlb_all(void)
{
	unsigned long old_ctx;
	int entry;

	/* Save old context and create impossible VPN2 value */
	old_ctx = read_c0_entryhi();
	write_c0_entrylo0(0);
	write_c0_entrylo1(0);

	entry = read_c0_wired();

	/* Blast 'em all away. */
	while (entry < current_cpu_data.tlbsize) {
		/* Make sure all entries differ. */
		write_c0_entryhi(UNIQUE_ENTRYHI(entry));
		write_c0_index(entry);
		mtc0_tlbw_hazard();
		tlb_write_indexed();
		entry++;
	}
	tlbw_use_hazard();
	write_c0_entryhi(old_ctx);
}

static inline void config_powerdown_core(unsigned int *resume_pc)
{
	/* set SLBC and SLPC */
	cpm_outl(1,CPM_SLBC);
	/* Clear previous reset status */
	cpm_outl(0,CPM_RSR);
	/* set resume pc */
	cpm_outl((unsigned int)resume_pc,CPM_SLPC);
}
#ifdef CONFIG_SUSPEND_TEST
#define INTC_BASE       (0xb0001000)
#define TCU_BASE       (0xb0002000)
#define IMSR0_OFF	(0x08)
#define IMCR0_OFF	(0x0c)
#define ICPR0_OFF	(0x10)

#define CLKEVENT_CH 2
static inline void write_tcu(unsigned int off, unsigned int val,
			     unsigned int result_reg, unsigned int result)
{
	REG32(TCU_BASE + off) = val;
	if(result_reg) {
		if(!result)
			while((REG32(TCU_BASE + result_reg) & val));
		else
			while(!(REG32(TCU_BASE + result_reg) & val));
	} else {
		while((REG32(TCU_BASE + off) & val) != val);
	}
}
static inline void config_tcu2_alarm(void)
{
	unsigned int val;

	val = 1 << CLKEVENT_CH;
	write_tcu(TCU_TECR, val, TCU_TER, 0);

	val = REG32(TCU_BASE + CH_TCSR(CLKEVENT_CH));
	val &= ~(CSR_DIV_MSK | CSR_CLK_MSK);
	val |= CSR_DIV1024 | CSR_RTC_EN;// | (TCSR_CNT_CLRZ);
	write_tcu(CH_TCSR(CLKEVENT_CH), val, 0, 0);

	val = 1 << CLKEVENT_CH;
	write_tcu(TCU_TMCR, val, TCU_TMR, 0);

	val = 1 << (CLKEVENT_CH + 16);
	write_tcu(TCU_TMSR, val, TCU_TMR, 1);

	val = 0;
	write_tcu(CH_TDHR(CLKEVENT_CH), val, 0, 0);

	val = REG32(TCU_BASE + CH_TCNT(CLKEVENT_CH));
	val += 0xff;
	write_tcu(CH_TDFR(CLKEVENT_CH), val, 0, 0);

	val = 1 << CLKEVENT_CH;
	write_tcu(TCU_TESR, val, TCU_TER, 1);

	val = REG32(INTC_BASE + IMCR0_OFF);
	val |= (1  << 25);
	REG32(INTC_BASE + IMCR0_OFF) = val;
}
#endif
struct resume_reg
{
	unsigned int sleep_cpm_lcr;
	unsigned int sleep_cpm_opcr;
	unsigned int sleep_cpm_cpccr;
	unsigned int sleep_voice_enable;
};

#ifdef DDR_TEST
#define MEM_TEST_SIZE  (1024 * 1024 * 4)
static unsigned int test_mem_space[MEM_TEST_SIZE / 4];
static inline void test_ddr_data_init(void)
{
	int i;
	unsigned int *test_mem;
	test_mem = (unsigned int *)((unsigned int)test_mem_space /* | 0x80000000 */);
	dma_cache_wback_inv((unsigned int)test_mem_space,0x100000);
	for(i = 0;i < MEM_TEST_SIZE / 4;i++) {
		test_mem[i] = (unsigned int)&test_mem[i];
	}
}
static inline void check_ddr_data(void) {
	int i;
	unsigned int *test_mem;
	test_mem = (unsigned int *)((unsigned int)test_mem_space /* | 0x80000000 */);
	for(i = 0;i < MEM_TEST_SIZE / 4;i++) {
		unsigned int dd;
		dd = test_mem[i];
		if(dd != (unsigned int)&test_mem[i]) {
			serial_put_hex(dd);
			TCSM_PCHAR(' ');
			/* serial_put_hex(i); */
			/* TCSM_PCHAR(' '); */
			serial_put_hex((unsigned int)&test_mem[i]);
			TCSM_PCHAR('\r');
			TCSM_PCHAR('\n');
		}
	}
}
#endif
#ifdef DDR_TRAINING
static unsigned int ddr_training_space[20];
#endif
static noinline void cpu_sleep(void)
{
	struct resume_reg *resume_reg = (struct resume_reg*)SLEEP_TCSM_RESUME_DATA;
#ifdef DDR_TRAINING
	memcpy(ddr_training_space,(void*)0x80000000,20 * 4);
#endif
	config_powerdown_core((unsigned int *)SLEEP_TCSM_BOOT_TEXT);
	resume_reg->sleep_cpm_cpccr = cpm_inl(CPM_CPCCR);

#ifdef CONFIG_SUSPEND_TEST
	config_tcu2_alarm();
#endif
	printk("icmr0 = %x\n",REG32(0xb0001004));
	printk("icmr1 = %x\n",REG32(0xb0001024));
	printk("icpr0 = %x\n",REG32(0xb0001010));
	printk("icpr1 = %x\n",REG32(0xb0001030));
	printk("CPAPCR = %x\n",cpm_inl(CPM_CPAPCR));
	printk("CPMPCR = %x\n",cpm_inl(CPM_CPMPCR));
	printk("opcr = %x\n",cpm_inl(CPM_OPCR));
	printk("lcr = %x\n",cpm_inl(CPM_LCR));
	printk("slpc = %x\n",cpm_inl(CPM_SLPC));
	printk("slbc = %x\n",cpm_inl(CPM_SLBC));
	printk("clkgate = %x\n",cpm_inl(CPM_CLKGR));
	printk("tcunt = %x\n",REG32(0xb0002068));

#ifdef CONFIG_JZ_DMIC_WAKEUP_V13
	wakeup_module_open(DEEP_SLEEP);
	resume_reg->sleep_voice_enable=wakeup_module_is_wakeup_enabled();
	//wakeup_module_cache_prefetch();
#endif
	cache_prefetch(LABLE1,1024);
LABLE1:
	blast_icache32();
	blast_dcache32();
	__sync();
	__fast_iob();
#ifdef DDR_TRAINING
	val = ddr_readl(DDRC_CTRL);//DDR keep selrefresh,when it exit the sleep state.
	val |= (1 << 17);//enter to hold ddr state
	ddr_writel(val,DDRC_CTRL);
	*((volatile unsigned int*)(0xb30100bc)) &= ~(0x1);
#endif

	/*
	 * (1) SCL_SRC source clock changes APLL to EXCLK
	 * (2) AH0/2 source clock changes MPLL to EXCLK
	 * (3) set PDIV H2DIV H0DIV L2CDIV CDIV = 0
	 */
	/* REG32(0xb0000000) = 0x95800000; */
	/* while((REG32(0xB00000D4) & 7)) */
	/* 	TCSM_PCHAR('A'); */
#ifndef CONFIG_JZ_DMIC_WAKEUP_V13
	{
		unsigned int val;
		val = REG32(0xb0000000);
		val &= ~(0xff);
		val |= 0x73;
		REG32(0xb0000000) = val;
		while((REG32(0xB00000D4) & 7))
			TCSM_PCHAR('A');

	}
#endif

		__asm__ volatile(".set mips32\n\t"
			 "sync\n\t"
			 "nop\n\t"
			 "wait\n\t"
			 "nop\n\t"
			 "nop\n\t"
			 "nop\n\t"
			 "jr %0\n\t"
			 "nop\n\t"
			 ".set mips32 \n\t"
			 :: "r" (SLEEP_TCSM_BOOT_TEXT)
		);

	while(1)
		TCSM_PCHAR('n');

}
static noinline void cpu_resume_boot(void)
{
//	TCSM_PCHAR('O');
	__asm__ volatile(".set mips32\n\t"
		"move $29, %0\n\t"
		".set mips32\n\t"
		:
		:"r" (SLEEP_TCSM_CPU_RESMUE_SP)
		:
		);
	__asm__ volatile(".set mips32\n\t"
		"jr %0\n\t"
		"nop\n\t"
		".set mips32 \n\t"
		:: "r" (SLEEP_TCSM_RESUME_TEXT));
}

static noinline void cpu_resume(void)
{
	register unsigned int val;
	register struct resume_reg *resume_reg = (struct resume_reg *)SLEEP_TCSM_RESUME_DATA;

	/* restore  CPM CPCCR */
	val = resume_reg->sleep_cpm_cpccr;
	val |= (7 << 20);
	REG32(0xb0000000) = val;
	while((REG32(0xB00000D4) & 7))
		TCSM_PCHAR('w');

#ifdef CONFIG_SUSPEND_TEST
	/* clear tcu2 flag */
	val = 1 << 2;
	write_tcu(TCU_TFCR, val, TCU_TFR, 0);
	/* intc mask tcu2 and clear pending */
	REG32(INTC_BASE + ICPR0_OFF) &= ~(1  << 25);
	val = REG32(INTC_BASE + IMSR0_OFF);
	val |= (1  << 25);
	REG32(INTC_BASE + IMSR0_OFF) = val;
#endif
#ifdef DDR_TRAINING
	/* *(volatile unsigned *) 0xb00000d0 = 0x73; //reset the DLL in DDR PHY */
	/* TCSM_DELAY(0x1ff); */

	val = ddr_readl(DDRC_CTRL);
	val |= 1 << 1;
	val &= ~(1<< 17);// exit to hold ddr state
	ddr_writel(val,DDRC_CTRL);
	val = ddr_readl(DDRC_CTRL);
	TCSM_DELAY(0x1ff);

	/* *(volatile unsigned *) 0xb00000d0 = 0x71; //disable the reset */
	/* TCSM_DELAY(0x1ff); */

	*((volatile unsigned int*)(0xb30100bc)) |= (0x1);
	TCSM_DELAY(0x1ff);
	TCSM_PCHAR('c');

	/* ddr_writel(0x30c00813, DDRP_ACIOCR); */
	/* ddr_writel(0x4802, DDRP_DXCCR); */

	for(i=0;i<4;i++) {
		serial_put_hex(ddr_readl(DDRP_DXGSR0(i)));
		serial_put_hex(ddr_readl(DDRP_DXDQSTR(i)));
	}

	TCSM_PCHAR('A');

	ddr_writel(0, DDRP_DTAR);

	val = DDRP_PIR_INIT | DDRP_PIR_DLLBYP | 1 << 29 | DDRP_PIR_QSTRN | DDRP_PIR_DLLLOCK | DDRP_PIR_DRAMINT;
	ddr_writel(val, DDRP_PIR);
	while (ddr_readl(DDRP_PGSR) != (DDRP_PGSR_IDONE | DDRP_PGSR_DLDONE | DDRP_PGSR_ZCDONE
					| DDRP_PGSR_DIDONE | DDRP_PGSR_DTDONE)) {
		if (ddr_readl(DDRP_PGSR) & (DDRP_PGSR_DTERR | DDRP_PGSR_DTIERR)) {
			TCSM_PCHAR('e');
			while(1);
			break;
		}
	}

	for(i=0;i<4;i++) {
		serial_put_hex(ddr_readl(DDRP_DXGSR0(i)));
		serial_put_hex(ddr_readl(DDRP_DXDQSTR(i)));
	}

	TCSM_PCHAR('E');
#endif
#ifdef DDR_TEST
	check_ddr_data();
#endif
	__jz_cache_init();
	__asm__ volatile(".set mips32\n\t"
			 "jr %0\n\t"
			 "nop\n\t"
			 ".set mips32 \n\t" :: "r" (restore_goto));
}

static void load_func_to_tcsm(unsigned int *tcsm_addr,unsigned int *f_addr,unsigned int size)
{
	unsigned int instr;
	int offset;
	int i;
	printk("tcsm addr = %p %p size = %d\n",tcsm_addr,f_addr,size);
	for(i = 0;i < size / 4;i++) {
		instr = f_addr[i];
		if((instr >> 26) == 2){
			offset = instr & 0x3ffffff;
			offset = (offset << 2) - ((unsigned int)f_addr & 0xfffffff);
			if(offset > 0) {
				offset = ((unsigned int)tcsm_addr & 0xfffffff) + offset;
				instr = (2 << 26) | (offset >> 2);
			}
		}
		tcsm_addr[i] = instr;
	}
}
static int x1000_pm_enter(suspend_state_t state)
{
	volatile unsigned int lcr,opcr;
	struct resume_reg *resume_reg = (struct resume_reg *)SLEEP_TCSM_RESUME_DATA;
#ifdef CONFIG_JZ_DMIC_WAKEUP_V13
	volatile unsigned int val;
	int (*volatile func)(int);
	int temp;
#endif
#if 0
	bypassmode = ddr_readl(DDRP_PIR) & DDRP_PIR_DLLBYP;
	printk("\nddr mode  = %d\n",bypassmode);
#endif
#ifdef DDR_TEST
	test_ddr_data_init();
#endif

#ifdef CONFIG_JZ_DMIC_WAKEUP_V13
	int ret=wakeup_module_get_sleep_process();
	if(ret == SYS_WAKEUP_OK)
		return 0;
#endif
	disable_fpu();
	resume_reg->sleep_cpm_lcr =  cpm_inl(CPM_LCR);
	resume_reg->sleep_cpm_opcr =  cpm_inl(CPM_OPCR);

	lcr = cpm_inl(CPM_LCR);
	lcr &= ~(3|(0xfff<<8));
	lcr |= 0xfff << 8;	/* power stable time */
	lcr |= LCR_LPM_SLEEP;
	cpm_outl(lcr,CPM_LCR);

	opcr = cpm_inl(CPM_OPCR);
#ifdef CONFIG_JZ_DMIC_WAKEUP_V13
	opcr &= ~((1 << 25) | (1 << 22) | (0xfff << 8) | (1 << 7) | (1 << 6) | (1 << 4) | (1 << 3) | (1 << 2));
	opcr |= (1 << 31) | (1 << 30) | (1 << 25)| (1 << 23) | (0xfff << 8) | (1 << 4);
#else
	opcr &= ~((1 << 22) | (0xfff << 8) | (1 << 7) | (1 << 6) | (1 << 2));
	opcr |= (1 << 31) | (1 << 30) | (1 << 25) | (1 << 23) | (0xfff << 8) | (1 << 4) | (1 << 3);
#endif
	cpm_outl(opcr,CPM_OPCR);

	load_func_to_tcsm((unsigned int *)SLEEP_TCSM_BOOT_TEXT,(unsigned int *)cpu_resume_boot,SLEEP_TCSM_BOOT_LEN);
	load_func_to_tcsm((unsigned int *)SLEEP_TCSM_RESUME_TEXT,(unsigned int *)cpu_resume,SLEEP_TCSM_RESUME_LEN);

	mb();
	save_goto((unsigned int)cpu_sleep);
	mb();
#ifdef DDR_TRAINING
	memcpy((void*)0x80000000,ddr_training_space,20 * 4);
	dma_cache_wback_inv(0x80000000,20 * 4);
#endif
	__jz_flush_cache_all();
	local_flush_tlb_all();

#ifdef CONFIG_JZ_DMIC_WAKEUP_V13
	if(resume_reg->sleep_voice_enable==1)
	{
		temp = *(unsigned int *)WAKEUP_HANDLER_ADDR;
		func = (int(*)(int))temp;
		val = func(1);
	}
#endif

	cpm_outl(resume_reg->sleep_cpm_lcr,CPM_LCR);
	cpm_outl(resume_reg->sleep_cpm_opcr,CPM_OPCR);


#ifdef CONFIG_JZ_DMIC_WAKEUP_V13
	wakeup_module_close(DEEP_SLEEP);
#endif
	return 0;
}
/*
 * Initialize power interface
 */
struct platform_suspend_ops pm_ops = {
	.valid = suspend_valid_only_mem,
	.enter = x1000_pm_enter,
};


int __init x1000_pm_init(void)
{
	volatile unsigned int lcr,opcr;
        /* init opcr and lcr for idle */
	lcr = cpm_inl(CPM_LCR);
	lcr &= ~(0x3);		/* LCR.SLEEP.DS=0'b0,LCR.LPM=1'b00*/
	lcr |= 0xff << 8;	/* power stable time */
	cpm_outl(lcr,CPM_LCR);

	opcr = cpm_inl(CPM_OPCR);
	opcr |= 0xff << 8;	/* EXCLK stable time */
	opcr &= ~(1 << 4);	/* EXCLK stable time */
	cpm_outl(opcr,CPM_OPCR);

	suspend_set_ops(&pm_ops);
	return 0;
}

arch_initcall(x1000_pm_init);
