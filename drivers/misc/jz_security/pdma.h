#ifndef _PDMA_H_
#define _PDMA_H_
#include "jz_pdma.h"
#define SE_PASS 0
#define SE_FAILURE 1
//#define REG32(addr)	*((volatile unsigned int *)(addr))
#define reset_mcu() (REG32(PDMA_BASE + DMCS_OFF) = 1)
#define boot_up_mcu() (REG32(PDMA_BASE + DMCS_OFF) = 0)
#define sc_mcu() (REG32(PDMA_BASE + DMCS_OFF) = 0x8)
#endif
