
#ifndef __M200_FIX_H__
#define __M200_FIX_H__

#define EFUSE_PATH     "/dev/efuse"
#ifdef DEBUG
#define debug printf
#else
#define debug(...) {}
#endif

/*efuse fixcode test*/
#define FIXCODE_START_ADDR 0x290
static const unsigned int fixcode[] = {
	0xa7c0800A,     /*"pc: 0xbfc053e0 no: 0x800 len: 10"*/
	0x3c05b354,     /*"lui    $5, 0xb354\n\t"*/
	0x24a502a4,     /*"addiu  $5, $5, 0x02a4\n\t"*/
	0x03e00008,     /*"jr    $31\n\t"*/
	0xaf650008,	/*"sw     $5, 8($27)\n\t"*/
	0x02000112,     /*"New Device Descriptor"*/
	0x40000000,
	0x4785a108,
	0x02010100,
	0x00000100,
	0xa7d0c004,     /*"pc: 0xbfc053e8 no: 0xc00 len 4"*/
	0x3c02bfc0,     /* "lui    $2, 0xbfc0\n\t"*/
	0x03e00008,     /* "jr     $31\n\t" */
	0x244253e0,     /*"addiu  $2, $2, 0x53e0\n\t"*/
};

#define TRIM0_START_ADDR 0x230
static const unsigned int trim0[] = {
	0x55aa55aa
};

#define TRIM1_START_ADDR 0x234
static const unsigned int trim1[] = {
	0x01010101
};

#define TRIM2_START_ADDR 0x238
static const unsigned int trim2[] = {
	0xaa55aa55
};

struct efuse_wr_info {
	unsigned int seg_addr;
	unsigned int data_length;
	void *buf;
};
#define EFUSE_CMD_READ	_IOWR('k', 51, struct efuse_wr_info *)
#define EFUSE_CMD_WRITE	_IOWR('k', 52, struct efuse_wr_info *)
#endif /*__M200_FIX_H__*/
