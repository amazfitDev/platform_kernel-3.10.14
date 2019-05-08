#ifndef _JZ_PDMA_H_
#define _JZ_PDMA_H_

/* For Pdma Core */
#ifdef PDMA_CORE
#define PDMA_TCSM_BANK_BASE        0xf4000000
#define PDMA_TCSM_BANK_BASE_PA     0xf3422000
#else
/* For Main Core */
#ifndef PDMA_BASE
#define PDMA_BASE 0xB3420000
#endif

#define REG32(addr)	(*(volatile unsigned int *)(addr))
#define PDMA_TCSM_BANK_BASE  0xB3422000
#define PDMA_TCSM_BANK_BASE_PA     0x13422000
#define DMCS_OFF 0x1030
#endif

#define SE_ROM_BASE          0xF5000000

#define BANK_SIZE			0x1000
#define TCSM_BANK(x)		(PDMA_TCSM_BANK_BASE + BANK_SIZE * x)
#define TCSM_BANK_PA(x)		(PDMA_TCSM_BANK_BASE_PA + BANK_SIZE * x)



#define TCSM_BANK0       TCSM_BANK(0)
	#define TCSM_BANK0_PART0	(TCSM_BANK0)				/* MCU BOOT UP:	2048*/
	#define TCSM_BANK0_PART1	(TCSM_BANK0_PART0)			/* Reserved:	   0*/
	#define	TCSM_BANK0_PART2	(TCSM_BANK0_PART1 + 2048)	/* SC ARGS :	1792*/
	#define TCSM_BANK0_PART3	(TCSM_BANK0_PART2 + 1792)	/* DBG:			256 */

#define TCSM_BANK1       TCSM_BANK(1)						/* For scboot : */
	#define TCSM_BANK1_PART0	(TCSM_BANK1)				/* spl_bin crypt. data */
	#define TCSM_BANK1_PART1	(TCSM_BANK1 + 2048)			/* SC_KEY */


#define TCSM_BANK0_PA     TCSM_BANK_PA(0)
#define TCSM_BANK1_PA     TCSM_BANK_PA(1)

#define TCSM_BANK_END     2

struct pdma_message {
	unsigned int signature;
	unsigned int msg_id;
	unsigned int msg[8];
	volatile unsigned int ret;
};

/*
   -----------       <- BANK_BASE
  |   BANK0   |
  |   BANK1   |
  |   .....   |
  |   BANKn   |
   -----------       <- BANK_END

   BANK_END - 1 * K - 128 for scboot
*/
/* **************** TCSM_BANK0 ***************/
#define MCU_TCSM_PADDR(a)        ((unsigned int)a - PDMA_TCSM_BANK_BASE + 0xF4000000)
#define MCU_TCSM_PDMA_MSG_LEN 128
#define MCU_TCSM_SECALL_MSG_LEN 128
/*#define MCU_TCSM_PDMA_MSG TCSM_BANK7*/
#define MCU_TCSM_PDMA_MSG TCSM_BANK0_PART2
#define MCU_TCSM_SECALL_MSG (MCU_TCSM_PDMA_MSG + MCU_TCSM_PDMA_MSG_LEN)

#define MCU_TCSM_NKULEN (4+4+256)
#define MCU_TCSM_KEYLEN 16
#define MCU_TCSM_ARGLEN 64
#define MCU_TCSM_NKU (MCU_TCSM_SECALL_MSG + MCU_TCSM_SECALL_MSG_LEN)
#define MCU_TCSM_NKU1 (MCU_TCSM_NKU + MCU_TCSM_NKULEN)
#define MCU_TCSM_NKUSIG (MCU_TCSM_NKU1 + MCU_TCSM_NKULEN)
#define MCU_TCSM_RSAENCKEY (MCU_TCSM_NKUSIG + MCU_TCSM_KEYLEN)  //通过rsa加密的ck
#define MCU_TCSM_RSAENCKEYLEN  (MCU_TCSM_RSAENCKEY + 264)
#define MCU_TCSM_PUTUKEY (MCU_TCSM_RSAENCKEYLEN + 4)  //外部传进来需要写UKEY
#define MCU_TCSM_RETVAL (MCU_TCSM_PUTUKEY+MCU_TCSM_KEYLEN)
#define MCU_TCSM_ARG1  (MCU_TCSM_RETVAL + 4)   //se_call的参数
#define MCU_TCSM_SPLSHA1ENC (MCU_TCSM_ARG1 + MCU_TCSM_ARGLEN)
#define MCU_TCSM_SPLSHA1ENCBUF (MCU_TCSM_SPLSHA1ENC + 4)


/* ************ TCSM_BANK1 *****************/
#define SC_KEY_ADDR		(TCSM_BANK1_PART1)
#define SPL_CODE_ADDR	(TCSM_BANK1_PART0)



/*#define MCU_TCSM_DBG	TCSM_BANK0_PART3*/


#if 0
#define MCU_TCSM_BUF  TCSM_BANK4
#endif

/* these debug TCSM space must not be used with scboot func */
#define MCU_TCSM_OUTDATA (TCSM_BANK1 + 0)
#define MCU_TCSM_INDATA (MCU_TCSM_OUTDATA + 512)
#define MCU_TCSM_DBG (MCU_TCSM_INDATA+512)

#define PDMA_SIGNATURE  (('P' << 24) | ('D' << 16) | ('M' << 8) | ('A' << 0))
#define GET_PDMA_MESSAGE() ({ \
	struct pdma_message * msg = (struct pdma_message *)(MCU_TCSM_PDMA_MSG); \
	msg->signature = PDMA_SIGNATURE;				\
	msg;								\
	})
#endif /* _JZ_PDMA_H_ */
