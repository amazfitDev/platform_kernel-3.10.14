
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/workqueue.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <jz_proc.h>
#include <linux/seq_file.h>
#include <asm/cpu.h>

#define REG32(addr) *(volatile unsigned int *)(addr)
/* ------------------------------cpu  proc------------------------------- */
static void get_str_from_user(unsigned char *str, int strlen,
			      const char *buffer, unsigned long count)
{
	int len = count > strlen-1 ? strlen-1 : count;
	int i;

	if(len == 0) {
		str[0] = 0;
		return;
	}
	copy_from_user(str,buffer,len);
	str[len] = 0;
	for(i = len;i >= 0;i--) {
		if((str[i] == '\r') || (str[i] == '\n'))
			str[i] = 0;
	}
}

#include <soc/base.h>
#include <soc/cpm.h>
#define PART_OFF	0x20
#define IMR_OFF		(0x04)
/* #define DDR_AUTOSR_EN   (0xb34f0304) */
/* #define DDR_DDLP        (0xb34f00bc) */
/* #define REG32_ADDR(addr) *((volatile unsigned int*)(addr)) */
static void cpu_entry_idle(void)
{
	unsigned int imr0_val, imr1_val;
	unsigned int *imr0_addr, *imr1_addr;
	void __iomem *intc_base;

	intc_base = ioremap(INTC_IOBASE, 0xfff);
	if(!intc_base) {
		printk("cpu switch ioremap intc reg addr error\n");
		return;
	}

	printk("opcr = %x\n",cpm_inl(CPM_OPCR));
	printk("lcr = %x\n",cpm_inl(CPM_LCR));
	printk("clkgr = %x\n",cpm_inl(CPM_CLKGR));

	imr0_addr = intc_base + IMR_OFF;
	imr1_addr = intc_base + PART_OFF + IMR_OFF;
	/* save interrupt */
	imr0_val = readl(imr0_addr);
	imr1_val = readl(imr1_addr);
	printk("entry idle\n");
	printk("imr0_val = %x\n", imr0_val);
	printk("entry idle\n");
	/* mask all interrupt except GPIO */
	writel(0xfffc0fff, imr0_addr);
	writel(0xffffffff, imr1_addr);
	/* REG32_ADDR(DDR_AUTOSR_EN) = 0; */
	/* udelay(100); */
	/* REG32_ADDR(DDR_DDLP) = 0x0000f003; */
#ifdef CONFIG_TIMER_SYS_OST
	/* stop sys-ost */
	REG32(0xb2000038) = 1;
#endif
	__asm__ __volatile__(
		"wait \n\t"
		);
#ifdef CONFIG_TIMER_SYS_OST
	REG32(0xb2000034) = 1;
#endif
	/* REG32_ADDR(DDR_DDLP) = 0; */
	/* udelay(100); */
	/* REG32_ADDR(DDR_AUTOSR_EN) = 1; */

	/* restore  interrupt */
	writel(imr0_val, imr0_addr);
	writel(imr1_val, imr1_addr);
	printk("exit idle\n");
	iounmap(intc_base);
}
static int cpu_idle_write(struct file *file, const char __user *buffer,
			  size_t count, loff_t *off)
{
	char str[10];
	get_str_from_user(str,10,buffer,count);
	if(strlen(str) > 0) {
		if (strncmp(str, "idle", 5) == 0) {
			cpu_entry_idle();
		}
	}
	return count;
}
static int cpu_idle_show(struct seq_file *filq, void *v)
{
	int len = 0;

	len += seq_printf(filq, "idle\n");

	return len;
}

static struct jz_single_file_ops idle_proc_fops = {
	.read = cpu_idle_show,
	.write = cpu_idle_write,
};

int __init create_cpu_core(void)
{
	struct proc_dir_entry *res,*p;

	p = jz_proc_mkdir("cpu");
	if (!p) {
		pr_warning("create_proc_entry for common cpu switch failed.\n");
		return -1;
	}
	res = jz_proc_create("idle", 0600, p, &idle_proc_fops);
	if (!res) {
		pr_warning("proc_create for idle failed.\n");
		return -ENODEV;
	}

	return 0;
}
module_init(create_cpu_core);
