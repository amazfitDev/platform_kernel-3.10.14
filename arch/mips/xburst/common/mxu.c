#include <asm/processor.h>
#include <linux/sched.h>
#include <mxu.h>

void __save_mxu(void *tsk_void)
{
	struct task_struct *tsk = tsk_void;
	register unsigned int reg_val asm("t0");
	asm volatile(".word	0x7008042e \n\t");
	tsk->thread.mxu.regs[0] = reg_val;
	asm volatile(".word	0x7008006e \n\t");
	tsk->thread.mxu.regs[1] = reg_val;
	asm volatile(".word	0x700800ae \n\t");
	tsk->thread.mxu.regs[2] = reg_val;
	asm volatile(".word	0x700800ee \n\t");
	tsk->thread.mxu.regs[3] = reg_val;
	asm volatile(".word	0x7008012e \n\t"); 
	tsk->thread.mxu.regs[4] = reg_val;
	asm volatile(".word	0x7008016e \n\t");		
	tsk->thread.mxu.regs[5] = reg_val;
	asm volatile(".word	0x700801ae \n\t");
	tsk->thread.mxu.regs[6] = reg_val;
	asm volatile(".word	0x700801ee \n\t");
	tsk->thread.mxu.regs[7] = reg_val;
	asm volatile(".word	0x7008022e \n\t");
	tsk->thread.mxu.regs[8] = reg_val;
	asm volatile(".word	0x7008026e \n\t");
	tsk->thread.mxu.regs[9] = reg_val;
	asm volatile(".word	0x700802ae \n\t");
	tsk->thread.mxu.regs[10] = reg_val;
	asm volatile(".word	0x700802ee \n\t");
	tsk->thread.mxu.regs[11] = reg_val;
	asm volatile(".word	0x7008032e \n\t");
	tsk->thread.mxu.regs[12] = reg_val;
	asm volatile(".word	0x7008036e \n\t");
	tsk->thread.mxu.regs[13] = reg_val;
	asm volatile(".word	0x700803ae \n\t");
	tsk->thread.mxu.regs[14] = reg_val;
	asm volatile(".word	0x700803ee \n\t");
	tsk->thread.mxu.regs[15] = reg_val;
}

void __restore_mxu(void * tsk_void)
{
	struct task_struct *tsk = tsk_void;
	register unsigned int reg_val asm("t0");

	reg_val = tsk->thread.mxu.regs[0];
	asm volatile(".word	0x7008042f \n\t"::"r"(reg_val));
	reg_val = tsk->thread.mxu.regs[1];
	asm volatile(".word	0x7008006f \n\t"::"r"(reg_val));
	reg_val = tsk->thread.mxu.regs[2];
	asm volatile(".word	0x700800af \n\t"::"r"(reg_val));
	reg_val = tsk->thread.mxu.regs[3];
	asm volatile(".word	0x700800ef \n\t"::"r"(reg_val));
	reg_val = tsk->thread.mxu.regs[4];
	asm volatile(".word	0x7008012f \n\t"::"r"(reg_val));
	reg_val = tsk->thread.mxu.regs[5];
	asm volatile(".word	0x7008016f \n\t"::"r"(reg_val));
	reg_val = tsk->thread.mxu.regs[6];
	asm volatile(".word	0x700801af \n\t"::"r"(reg_val));
	reg_val = tsk->thread.mxu.regs[7];
	asm volatile(".word	0x700801ef \n\t"::"r"(reg_val));
	reg_val = tsk->thread.mxu.regs[8];
	asm volatile(".word	0x7008022f \n\t"::"r"(reg_val));
	reg_val = tsk->thread.mxu.regs[9];
	asm volatile(".word	0x7008026f \n\t"::"r"(reg_val));
	reg_val = tsk->thread.mxu.regs[10];
	asm volatile(".word	0x700802af \n\t"::"r"(reg_val));
	reg_val = tsk->thread.mxu.regs[11];
	asm volatile(".word	0x700802ef \n\t"::"r"(reg_val));
	reg_val = tsk->thread.mxu.regs[12];
	asm volatile(".word	0x7008032f \n\t"::"r"(reg_val));
	reg_val = tsk->thread.mxu.regs[13];
	asm volatile(".word	0x7008036f \n\t"::"r"(reg_val));
	reg_val = tsk->thread.mxu.regs[14];
	asm volatile(".word	0x700803af \n\t"::"r"(reg_val));
	reg_val = tsk->thread.mxu.regs[15];
	asm volatile(".word	0x700803ef \n\t"::"r"(reg_val));
}

