#ifndef _SC_H_
#define _SC_H_

#define SC_FUNC_AES             0x1
#define SC_FUNC_OTP             0x2
#define SC_FUNC_SCOTP           0x3
#define SC_FUNC_SPLCKENC        0x4
#define SC_FUNC_BURNUK          0x5
#define SC_FUNC_AESBYKEY        0x6
#define SC_FUNC_SPLCKENCLOOP    0x8
#define SC_FUNC_TESTMD5         0x9
#define SC_FUNC_BURNNKU         0xa
#define SC_FUNC_WTOTP           0xb
#define SC_FUNC_TESTRAM         0xc
#define SC_FUNC_CHECKNKU        0xd
#define SC_FUNC_CHECKKEYFIX     0xe
#define SC_FUNC_SPLSHA          0xf
#define SC_FUNC_TESTSHA1        0x10
#define SC_FUNC_SPLSHA1         0x11
#define SC_FUNC_RSA             0x12
#define SC_FUNC_RSAENCK         0x13
#define SC_FUNC_SPLSHA1CHECK    0x14
#define SC_FUNC_SPLKEYCRYPT     0x15
#define SC_FUNC_SCBOOT          0x16
#define SC_FUNC_RNG             0x17
#define SC_FUNC_BURNRKCK        0x18
#define SC_FUNC_SPLHASH			0x19
#define SC_FUNC_SPLHASHCHECK	0x20
#define SC_FUNC_INTERNAL_RSA	0x21
#define SC_FUNC_INTERNAL_AES_INIT 0x22
#define SC_FUNC_INTERNAL_AES_EXIT 0x23
#define SC_FUNC_INTERNAL_AES_CHANGE	0x24

#define SC_FUNC_TEST		0x7

#define SC_STATE_UPDATE		0x3
#define SC_STATE_UPDATECHECK    0x7

#define SC_BURN_STAGE_0		0x1
#define SC_BURN_STAGE_1		0x3
#define SC_BURN_STAGE_2		0x7

#define SC_ERR_SUCC             0x0e000000
#define SC_ERR_RKZERO           0x0e000001
#define SC_ERR_CKZERO           0x0e000002
#define SC_ERR_RKCKEQ           0x0e000003
#define SC_ERR_RKNKUEQ          0x0e000004
#define SC_ERR_CKNKUEQ          0x0e000005
#define SC_ERR_SIGPTK_UNSET     0x0e000006
#define SC_ERR_NKU_INVALID      0x0e000007
#define SC_ERR_SIG_UNEQ         0x0e000008
#define SC_ERR_SIGPTK_SET       0x0e000009
#define SC_ERR_UKPTK_SET        0x0e00000a
#define SC_ERR_UKPTK_UNSET      0x0e00000b
#define SC_ERR_ILLEGAL_BURN     0x0e00000c
#define SC_ERR_ILLEGAL_SPLSHA1  0x0e00000d
#define SC_ERR_BINSHA1_UNEQ     0x0e00000e
#define SC_ERR_INVALID_SPLCRYPT 0x0e00000f
#define SC_ERR_INVALID_KEY      0x0e000010
#define SC_ERR_INVALID_RNCNT    0x0e000011
#define SC_ERR_CPAPCR_LOCK      0x0e000012
#define SC_ERR_OKEY_UNMATCH      0x0e000013

struct sc_args {
	unsigned int func;
	unsigned int state;
	volatile unsigned int retval;
	unsigned int arg[16];
};

#define GET_SC_ARGS() ((struct sc_args *)MCU_TCSM_SECALL_MSG)

#endif /* _SC_H_ */
