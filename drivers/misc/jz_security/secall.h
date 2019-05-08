#ifndef _SECALL_H_
#define _SECALL_H_

#include "pdma.h"
#include "sc.h"

#define send_secall(func) ( REG32(PDMA_BASE + DMCS_OFF) = (REG32(PDMA_BASE + DMCS_OFF) & 0xff0000ff) | 0x8 | ((func)<<8) | 1<<24)

#define polling_done(args) ({ \
			while (args->retval & 0x80000000);	\
    })

static int secall(volatile struct sc_args *argsx,unsigned int func,unsigned state){
	argsx->func = (func);
	argsx->state = (state);
	argsx->retval = 0x80000000;
	debug_printk("1..mcu control:%08x, func:%x\n", *(volatile unsigned int *)0xb3421030, func);
	send_secall(1);
	debug_printk("2..mcu control:%08x\n", *(volatile unsigned int *)0xb3421030);
	while (argsx->retval & 0x80000000) {
		/*printf("dbg :%x\n", *(volatile unsigned int *)MCU_TCSM_DBG);*/
		/*printf("tcsm bank 1:%08x\n", *(volatile unsigned int *)0xb3423000);*/
		/*printf("3..mcu control:%08x\n", *(volatile unsigned int *)0xb3421030);*/
		/*printf("wait secall excuted: retval:%x\n", argsx->retval);*/
	}
	debug_printk("wait secall excuted: retval:%x\n", argsx->retval);
	return argsx->retval;
}

#define get_secall_off(x) ((unsigned int)x - TCSM_BANK(0))
#endif /* _SECALL_H_ */
