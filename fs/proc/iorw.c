#include <linux/fs.h>
#include <linux/hugetlb.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/mmzone.h>
#include <linux/proc_fs.h>
#include <linux/quicklist.h>
#include <linux/seq_file.h>
#include <linux/swap.h>
#include <linux/vmstat.h>
#include <asm/atomic.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/uaccess.h>
#include "internal.h"

#define MAP 0xA0000000
#define REG32(addr) *((volatile unsigned int*)((addr)|MAP))

void __attribute__((weak)) arch_report_iorw(struct seq_file *m)
{
}

static int iorw_proc_show(struct seq_file *m, void *v)
{
	return 0;
}

static int iorw_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, iorw_proc_show, NULL);
}

static void reg_prompt(void)
{
	printk("example:\n");
	printk("#echo 0x130a0030 > /proc/iorw\n");
	printk("0x130a0030: 0x4000080d\n");
	printk("#echo 0x130a0030 0x40000805 > /proc/iorw\n");
	printk("0x130a0030: 0x4000080d -->  0x40000805\n");
}

static ssize_t reg_read(struct file *file, char __user *buf,
					 size_t count, loff_t *ppos)
{
	reg_prompt();
	return count;
}

static ssize_t reg_write(struct file * file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	int ret = -1, i = 0;
	char param[50]={0};
	unsigned int reg_param[2]={0,-1};
	unsigned int tmp_value0,tmp_value1;
	char *tmp;

	char* const delim = " \0";
	char *token, *cur;

	/* printk("----reg write  buf=%s len=%d\n",buf,strlen(buf)); */
	if (copy_from_user(param, buf, strlen(buf))) {
		ret = -EFAULT;
	}
	tmp = param;
	tmp[strlen(param)-1] = '\0';
	/* printk("----reg write  param   %d   =%s,\n",strlen(param),param); */

	cur = param;
	i = 0;
	while ((token = strsep(&cur, delim))) {
			/* reg_param[i++] = atoi(token); */
			reg_param[i++] = simple_strtoul(token, NULL, 0);
	}
	/* printk("-----i=%d----reg_param [0]=0x%08x [1]0x%08x\n",i,reg_param[0],reg_param[1]); */
	if(i == 1 && reg_param[0] != 0){
		tmp_value0 = REG32(reg_param[0]);
		printk("0x%08x: 0x%08x\n",reg_param[0],tmp_value0);
	}else if(i == 2){
		tmp_value0 = REG32(reg_param[0]);
		REG32(reg_param[0]) = reg_param[1];
		tmp_value1 = REG32(reg_param[0]);
		printk("0x%08x: 0x%08x --> 0x%08x\n",reg_param[0],tmp_value0,tmp_value1);
	}else{
		reg_prompt();
	}
	return count;
}


static const struct file_operations iorw_proc_fops = {
	.open		= iorw_proc_open,
	.read		= reg_read,
	.write		= reg_write,
	.release	= single_release,
};
extern struct proc_dir_entry * get_jz_proc_root(void);
static int __init proc_iorw_init(void)
{
	//struct proc_dir_entry *p = get_jz_proc_root();
	proc_create("iorw", 0x777, NULL, &iorw_proc_fops);
	return 0;
}
module_init(proc_iorw_init);
