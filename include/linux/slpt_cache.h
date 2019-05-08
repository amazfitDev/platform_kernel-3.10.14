#ifndef _SLPT_CACHE_H_
#define _SLPT_CACHE_H_

#include <linux/smp.h>
#include <asm/page.h>
#include <asm/cacheops.h> 		/* this file is mark */
#include <soc/cache.h>
#include <asm/addrspace.h>
#include <asm/barrier.h>
#include <asm/r4kcache.h>

extern unsigned long slpt_icache_line_size;
extern unsigned long slpt_scache_line_size;
extern unsigned long slpt_dcache_line_size;

extern unsigned long slpt_icache_waysize;
extern unsigned long slpt_scache_waysize;
extern unsigned long slpt_dcache_waysize;

extern unsigned long slpt_icache_waybit;
extern unsigned long slpt_scache_waybit;
extern unsigned long slpt_dcache_waybit;

extern unsigned long slpt_icache_ways;
extern unsigned long slpt_scache_ways;
extern unsigned long slpt_dcache_ways;

#define KSEG1VALUE(val) (*((typeof(val)*)KSEG1ADDR((unsigned long)&(val))))

/* build blast_xxx, blast_xxx_page, blast_xxx_page_indexed */
#define __SLPT_BUILD_BLAST_CACHE(pfx, desc, indexop, hitop, lsize) \
static inline void slpt_blast_##pfx##cache##lsize(void)			\
{									\
	unsigned long start = INDEX_BASE;				\
	unsigned long end = start + KSEG1VALUE(slpt_##desc##_waysize);	\
	unsigned long ws_inc = 1UL << KSEG1VALUE(slpt_##desc##_waybit);	\
	unsigned long ws_end = KSEG1VALUE(slpt_##desc##_ways) <<		\
		KSEG1VALUE(slpt_##desc##_waybit);							\
	unsigned long ws, addr;						\
									\
	__##pfx##flush_prologue						\
									\
	for (ws = 0; ws < ws_end; ws += ws_inc)				\
		for (addr = start; addr < end; addr += lsize * 32)	\
			cache##lsize##_unroll32(addr|ws, indexop);	\
									\
	__##pfx##flush_epilogue						\
}									\
									\
static inline void slpt_blast_##pfx##cache##lsize##_page(unsigned long page)	\
{									\
	unsigned long start = page;					\
	unsigned long end = page + PAGE_SIZE;				\
									\
	__##pfx##flush_prologue						\
									\
	do {								\
		cache##lsize##_unroll32(start, hitop);			\
		start += lsize * 32;					\
	} while (start < end);						\
									\
	__##pfx##flush_epilogue						\
}									\
									\
static inline void slpt_blast_##pfx##cache##lsize##_page_indexed(unsigned long page) \
{									\
	unsigned long indexmask = KSEG1VALUE(slpt_##desc##_waysize) - 1;	\
	unsigned long start = INDEX_BASE + (page & indexmask);		\
	unsigned long end = start + PAGE_SIZE;				\
	unsigned long ws_inc = 1UL << KSEG1VALUE(slpt_##desc##_waybit);	\
	unsigned long ws_end = KSEG1VALUE(slpt_##desc##_ways) <<		\
		KSEG1VALUE(slpt_##desc##_waybit);							\
	unsigned long ws, addr;						\
									\
	__##pfx##flush_prologue						\
									\
	for (ws = 0; ws < ws_end; ws += ws_inc)				\
		for (addr = start; addr < end; addr += lsize * 32)	\
			cache##lsize##_unroll32(addr|ws, indexop);	\
									\
	__##pfx##flush_epilogue						\
}

__SLPT_BUILD_BLAST_CACHE(d, dcache, Index_Writeback_Inv_D, Hit_Writeback_Inv_D, 16)
__SLPT_BUILD_BLAST_CACHE(i, icache, Index_Invalidate_I, Hit_Invalidate_I, 16)
__SLPT_BUILD_BLAST_CACHE(s, scache, Index_Writeback_Inv_SD, Hit_Writeback_Inv_SD, 16)
__SLPT_BUILD_BLAST_CACHE(d, dcache, Index_Writeback_Inv_D, Hit_Writeback_Inv_D, 32)
__SLPT_BUILD_BLAST_CACHE(i, icache, Index_Invalidate_I, Hit_Invalidate_I, 32)
__SLPT_BUILD_BLAST_CACHE(s, scache, Index_Writeback_Inv_SD, Hit_Writeback_Inv_SD, 32)
__SLPT_BUILD_BLAST_CACHE(d, dcache, Index_Writeback_Inv_D, Hit_Writeback_Inv_D, 64)
__SLPT_BUILD_BLAST_CACHE(i, icache, Index_Invalidate_I, Hit_Invalidate_I, 64)
__SLPT_BUILD_BLAST_CACHE(s, scache, Index_Writeback_Inv_SD, Hit_Writeback_Inv_SD, 64)
__SLPT_BUILD_BLAST_CACHE(s, scache, Index_Writeback_Inv_SD, Hit_Writeback_Inv_SD, 128)

__SLPT_BUILD_BLAST_CACHE(inv_d, dcache, Index_Writeback_Inv_D, Hit_Invalidate_D, 16)
__SLPT_BUILD_BLAST_CACHE(inv_d, dcache, Index_Writeback_Inv_D, Hit_Invalidate_D, 32)
__SLPT_BUILD_BLAST_CACHE(inv_s, scache, Index_Writeback_Inv_SD, Hit_Invalidate_SD, 16)
__SLPT_BUILD_BLAST_CACHE(inv_s, scache, Index_Writeback_Inv_SD, Hit_Invalidate_SD, 32)
__SLPT_BUILD_BLAST_CACHE(inv_s, scache, Index_Writeback_Inv_SD, Hit_Invalidate_SD, 64)
__SLPT_BUILD_BLAST_CACHE(inv_s, scache, Index_Writeback_Inv_SD, Hit_Invalidate_SD, 128)

/* build blast_xxx_range, protected_blast_xxx_range */
#define __SLPT_BUILD_BLAST_CACHE_RANGE(pfx, desc, hitop, prot) \
static inline void slpt_##prot##blast_##pfx##cache##_range(unsigned long start, \
						    unsigned long end)	\
{									\
	unsigned long lsize = KSEG1VALUE(slpt_##desc##_line_size);	\
	unsigned long addr = start & ~(lsize - 1);			\
	unsigned long aend = (end - 1) & ~(lsize - 1);			\
									\
	__##pfx##flush_prologue						\
									\
	while (1) {							\
		prot##cache_op(hitop, addr);				\
		if (addr == aend)					\
			break;						\
		addr += lsize;						\
	}								\
									\
	__##pfx##flush_epilogue						\
}

__SLPT_BUILD_BLAST_CACHE_RANGE(d, dcache, Hit_Writeback_Inv_D, protected_)
__SLPT_BUILD_BLAST_CACHE_RANGE(s, scache, Hit_Writeback_Inv_SD, protected_)
__SLPT_BUILD_BLAST_CACHE_RANGE(i, icache, Hit_Invalidate_I, protected_)
__SLPT_BUILD_BLAST_CACHE_RANGE(d, dcache, Hit_Writeback_Inv_D, )
__SLPT_BUILD_BLAST_CACHE_RANGE(s, scache, Hit_Writeback_Inv_SD, )
/* blast_inv_dcache_range */
__SLPT_BUILD_BLAST_CACHE_RANGE(inv_d, dcache, Hit_Invalidate_D, )
__SLPT_BUILD_BLAST_CACHE_RANGE(inv_s, scache, Hit_Invalidate_SD, )

#endif /* _SLPT_CACHE_H_ */
