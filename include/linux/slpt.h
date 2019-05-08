/*
 *  Copyright (C) 2014 Fighter Sun <wanmyqawdr@126.com>
 *  Copyright (C) 2014 Wu Jiao <jwu@ingenic.cn>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef SLPT_H_
#define SLPT_H_

#include <linux/firmware.h>
#include <linux/list.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/ioctl.h>
#include <linux/suspend.h>
#include <linux/slpt_cache.h>

/* Define pm_entry function argument */
#define SLPTARG 'S'
#define SLPTARG_LOAD_RESETDLL _IO(SLPTARG, 0x1)

struct slpt_task {
	const char *name;
	const char *bin_desc;
	const struct firmware *task_fw;

	void *load_at_addr;
	void *run_at_addr;
	void *init_addr;
	void *exit_addr;
	void *run_every_time_addr;
	void *cache_prefetch_addr;

	size_t size;

	struct {
		unsigned int is_loaded:1;
		unsigned int is_registered:1;
		unsigned int is_inited:1;
	} state;

	struct kobject kobj;
	struct kobject kobj_res;
	struct kobject kobj_method;
	struct attribute_group group;

	struct list_head link;		/* link of task */
	struct list_head res_handlers;	/* handlers of resource */
	struct list_head method_handlers;	/* handlers of resource */
};

extern int slpt_register_task(struct slpt_task *task, int init, bool need_request_firmware);
extern void slpt_unregister_task(struct slpt_task *task);
extern struct slpt_task *name_to_slpt_task(const char *name);
extern int slpt_init_task(struct slpt_task *task);
extern int slpt_exit_task(struct slpt_task *task);
extern int slpt_run_task(struct slpt_task *task);
extern struct slpt_task *slpt_get_cur(void);
extern void slpt_enable_task(struct slpt_task *task, bool enable);

/* slpt_map.c */
extern void slpt_alloc_maped_memory(void);
extern void slpt_map_reserve_mem(void);
extern void slpt_unmap_reserve_mem(void);
extern void slpt_register_vm_area_early(void);

/* slpt_cache.c */
extern int slpt_cache_init(void);

/* slpt_method.c */
extern int slpt_register_method_kobj(struct slpt_task *task);
extern int slpt_register_method(struct slpt_task *task, const char *name, unsigned long addr);

enum slpt_res_type {
	SLPT_RES_FUNCTION = 0,
	SLPT_RES_INT,
	SLPT_RES_MEM,
	SLPT_RES_DIR,

	/* keep last */
	SLPT_RES_MAX,
};

struct slpt_app_res {
	const char *name;
	unsigned int type;
	void *addr;
	unsigned int length;
};

struct slpt_res {
	struct slpt_app_res res;
	struct slpt_task *task;
	struct kobject kobj;
	struct list_head link;
	struct list_head handlers;
	unsigned int create_sys;
};

extern unsigned long slpt_app_get_api(const char *name, struct slpt_task *task);

extern struct slpt_app_res *slpt_app_register_res(struct slpt_app_res *res, struct slpt_task *task);
extern void slpt_app_unregister_res(struct slpt_app_res *res, struct slpt_task *task);
extern struct slpt_app_res *name_to_slpt_app_res(const char *name, struct slpt_task *task);

extern struct slpt_res *slpt_register_res(struct slpt_app_res *res, int create_sys, struct slpt_task *task);
extern void slpt_unregister_res(struct slpt_res *res);
extern struct slpt_res *name_to_slpt_res(const char *name, struct slpt_task* task);

extern int jz4775_slpt_pm_enter(suspend_state_t state);

#define slpt_attr(_name) \
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = __stringify(_name),	\
		.mode = 0666,			\
	},					\
	.show	= _name##_show,			\
	.store	= _name##_store,		\
}

struct slpt_key {
	int id;
	const char *name;
	unsigned long val;
};

/* if you have a new key, add it's id in enum slpt_key_id,
 * and add the key in drivers/slpt/slpt_key.c
 */
enum slpt_key_id {
	SLPT_K_POWER_STATE = 0,
	SLPT_K_CHARGER_GPIO,
	SLPT_K_CHARGER_LEVEL,
	SLPT_K_LOW_BATTERY_WARN_VOL,
	SLPT_K_BATTERY_VOLTAGE,
	SLPT_K_BATTERY_LOW_VOLTAGE,
	SLPT_K_BATTERY_CAPACITY,
	SLPT_K_RESERVE_MEM_ADDR,
	SLPT_K_GO_KERNEL,
	SLPT_K_VOICE_TRIGGER_STATE,
	SLPT_K_FRIZZ_PEDO,
	SLPT_K_FRIZZ_GESTURE,
	SLPT_K_SLEEP_MOTION,
	SLPT_K_SLEEP_MOTION_ENABLE,
	SLPT_K_IOCTL,
	SLPT_K_FB_ON,
	SLPT_K_SAMPLE_ADC_FOR_KERNEL,

	/* keep last */
	SLPT_K_NUMS,
};

#define SLPT_V_LOW_POWER_SHUTDOWN "low-power-shutdown"
#define SLPT_V_POWER_NORMAL "power-normal"

extern struct slpt_key slpt_key_list[SLPT_K_NUMS];

#ifdef CONFIG_SLPT
/* general way */
extern int slpt_get_key_id(const char *name, unsigned long **val_addr);
extern int slpt_set_key_by_id(unsigned int id, unsigned long val);
extern int slpt_get_key_by_id(unsigned int id, unsigned long *val);

/* other way */
extern struct slpt_key *slpt_find_key(const char *name);
extern int slpt_set_key_by_name(const char *name, unsigned long val);
extern int slpt_get_key_by_name(const char *name, unsigned long *val);

static __always_inline void slpt_set_key_no_cached(unsigned int id, unsigned long val) {
	struct slpt_key *list =  (void *)KSEG1ADDR((unsigned long)slpt_key_list);

	if (id >= SLPT_K_NUMS)
		return ;

	list[id].val = val;
}

static __always_inline void slpt_get_key_no_cached(unsigned int id, unsigned long *val) {
	struct slpt_key *list = (void *)KSEG1ADDR((unsigned long)slpt_key_list);

	if (id >= SLPT_K_NUMS)
		return ;

	*val = list[id].val;
}

#else  /* CONFIG_SLPT */

static inline int slpt_get_key_id(const char *name, unsigned long **val_addr)
{
	return 0;
}
static inline int slpt_set_key_by_id(unsigned int id, unsigned long val)
{
	return 0;
}
static inline int slpt_get_key_by_id(unsigned int id, unsigned long *val)
{
	return 0;
}

static inline struct slpt_key *slpt_find_key(const char *name)
{
	struct slpt_key* sk = 0;
	return sk;
}

static inline int slpt_set_key_by_name(const char *name, unsigned long val)
{
	return 0;
}
static inline int slpt_get_key_by_name(const char *name, unsigned long *val)
{
	return 0;
}

static __always_inline void slpt_set_key_no_cached(unsigned int id, unsigned long val) {

}

static __always_inline void slpt_get_key_no_cached(unsigned int id, unsigned long *val) {

}
#endif  /* CONFIG_SLPT */

#define SLPT_SET_KEY(id, val) slpt_set_key_by_id(id, (unsigned long)(val))
#define SLPT_GET_KEY(id, val)                            \
    do {                                                 \
        unsigned long __tmp_val = 0;                     \
        slpt_get_key_by_id(id, &__tmp_val);	             \
        *(val) = (typeof(*(val))) __tmp_val;             \
                                                         \
    } while (0)

/*
 * power state
 */
static inline void slpt_set_power_state(const char *state) {
	SLPT_SET_KEY(SLPT_K_POWER_STATE, state);
}

static inline const char *slpt_get_power_state(void) {
	const char *state;

	SLPT_GET_KEY(SLPT_K_POWER_STATE, &state);
	return state;
}

/*
 * charger gpio
 */
static inline void slpt_set_charger_gpio(int gpio) {
	SLPT_SET_KEY(SLPT_K_CHARGER_GPIO, gpio);
}

static inline int slpt_get_charger_gpio(void) {
	int gpio;

	SLPT_GET_KEY(SLPT_K_CHARGER_GPIO, &gpio);
	return gpio;
}

/*
 * charger gpio level
 */
static inline void slpt_set_charger_level(int level) {
	SLPT_SET_KEY(SLPT_K_CHARGER_LEVEL, level);
}

static inline int slpt_get_charger_level(void) {
	int level;

	SLPT_GET_KEY(SLPT_K_CHARGER_LEVEL, &level);
	return level;
}

/*
 * battery is in "slpt_battery.h"
 */

/*
 * slpt want go back to kernel or not
 */
static inline unsigned long slpt_want_go_kernel(void) {
	unsigned long go_kernel = 1111;

	slpt_get_key_no_cached(SLPT_K_GO_KERNEL, &go_kernel);
	return go_kernel;
}

/*
 * voice trigger is enabled
 */
static inline long slpt_get_voice_trigger_state(void) {
	unsigned long state;

	slpt_get_key_no_cached(SLPT_K_VOICE_TRIGGER_STATE, &state);
	return state;
}

static inline void slpt_set_voice_trigger_state(unsigned long state) {
	slpt_set_key_no_cached(SLPT_K_VOICE_TRIGGER_STATE, state);
}

/*
 * slpt ioctl
 */
static inline void *slpt_get_ioctl(void) {
	void *address;

	SLPT_GET_KEY(SLPT_K_IOCTL, &address);
	return address;
}

/*
 * fb on state: to notify slpt fb's state
 */
static inline int slpt_get_fb_on(void) {
	int fb_on;

	SLPT_GET_KEY(SLPT_K_FB_ON, &fb_on);
	return fb_on;
}

static inline void slpt_set_fb_on(int fb_on) {
	SLPT_SET_KEY(SLPT_K_FB_ON, fb_on);
}

/*
 * sample adc for kernel
 */
static inline int slpt_get_sample_adc_for_kernel(void) {
	int yes_or_no;

	SLPT_GET_KEY(SLPT_K_SAMPLE_ADC_FOR_KERNEL, &yes_or_no);
	return yes_or_no;
}

static inline void slpt_set_sample_adc_for_kernel(int yes_or_no) {
	SLPT_SET_KEY(SLPT_K_SAMPLE_ADC_FOR_KERNEL, yes_or_no);
}

#define SLPT_LIMIT_SIZE (6 * 1024 * 1024)
#ifdef CONFIG_SLPT_MAP_TO_KSEG2
#define SLPT_RESERVE_ADDR 0xc0000000
extern volatile char slpt_reserve_mem[SLPT_LIMIT_SIZE];
#else
#define SLPT_RESERVE_ADDR (0x80000000 + 250 * 1024 * 1024)
#endif

extern struct kobject *slpt_res_kobj;
extern struct kobject *slpt_configs_kobj;
extern struct kobj_type slpt_kobj_ktype;
extern struct slpt_task *slpt_select;
extern unsigned int slpt_task_is_enabled;
extern int (*slpt_save_pm_enter_func)(suspend_state_t state);

extern void slpt_set_suspend_ops(struct platform_suspend_ops *ops);
extern int slpt_suspend_in_kernel(suspend_state_t state);
extern int slpt_printf(const char *fmt, ...);
extern int fb_is_always_on(void);
extern void wlan_pw_en_disable(void);

extern void test_mem_no_printk(void);

extern int __init slpt_configs_init(void);
extern int __exit slpt_configs_exit(void);

#ifdef CONFIG_SLPT_MAP_TO_KSEG2
static __always_inline void slpt_setup_tlb(unsigned int addr_fr0, unsigned int addr_fr1, unsigned int map_addr)
{

	unsigned int pagemask = 0x001fe000;    /* 1MB */
	/*                              cached  D:allow-W   V:valid    G */
	unsigned int entrylo0 = (addr_fr0 >> 6) | ( (1 << 30) + (6 << 3) + (1 << 2) + (1 << 1) + 1); /*Data Entry*/
	unsigned int entrylo1 = (addr_fr1 >> 6) | ( (1 << 30) + (6 << 3) + (1 << 2) + (1 << 1) + 1);
	unsigned int entryhi =  map_addr; /* Tag Entry */
	volatile int i;
	volatile unsigned int temp;

	temp = __read_32bit_c0_register($12, 0);
	temp |= (1<<2);
	/* temp &= ~(1<<2); */
	__write_32bit_c0_register($12, 0, temp);

	write_c0_pagemask(pagemask);
	write_c0_wired(0);

	/* 6M Byte*/
	for(i = 0; i < 3; i++)
	{
		asm (
				"mtc0 %0, $0\n\t"    /* write Index */

				"ssnop\n\t"
				"ssnop\n\t"
				"ssnop\n\t"
				"ssnop\n\t"
				"ssnop\n\t"
				"ssnop\n\t"
				"ssnop\n\t"
				"ssnop\n\t"
				"mtc0 %1, $5\n\t"    /* write pagemask */
				"mtc0 %2, $10\n\t"    /* write entryhi */
				"mtc0 %3, $2\n\t"    /* write entrylo0 */
				"mtc0 %4, $3\n\t"    /* write entrylo1 */
				"ssnop\n\t"
				"ssnop\n\t"
				"ssnop\n\t"
				"ssnop\n\t"
				"ssnop\n\t"
				"ssnop\n\t"
				"ssnop\n\t"
				"ssnop\n\t"
				"tlbwi    \n\t"        /* TLB indexed write */
				: : "Jr" (i), "r" (pagemask), "r" (entryhi),
				"r" (entrylo0), "r" (entrylo1)
			);

		entryhi +=  0x00200000;    /* 1MB */
		entrylo0 += (0x00200000 >> 6);
		entrylo1 += (0x00200000 >> 6);
	}
}

static __always_inline void slpt_cache_prefetch(void) {
	int i;
	volatile unsigned int *addr = (unsigned int *)SLPT_RESERVE_ADDR;
	volatile unsigned int a;

	for(i=0; i< SLPT_LIMIT_SIZE / 32; i++) {
		a = *(addr + i * 8);
	}
}

static __always_inline void slpt_memset(unsigned int *dst, unsigned int c, unsigned int size) {
	unsigned int i;

	c = c & 0xff;
	c = (c << 24 ) + (c << 16) + (c << 8) + c;

	for (i = 0; i < size / 4; ++i) {
		dst[i] = c;
	}
}

static __always_inline void slpt_task_run_everytime(int need_to_go_kernel) {
	void (*volatile run)(int);
	unsigned int addr = (unsigned int)virt_to_phys(slpt_reserve_mem);
	unsigned int *task_is_enabled = (void *)KSEG1ADDR((unsigned long) (&slpt_task_is_enabled));

	if (*task_is_enabled) {
		slpt_setup_tlb(addr, addr + 1 * 1024 * 1024, SLPT_RESERVE_ADDR);
		run = (void (*)(int))KSEG1VALUE(slpt_select)->run_every_time_addr;
		run(need_to_go_kernel);

		slpt_blast_dcache32();
		slpt_blast_icache32();
		__sync();
		__fast_iob();
	}
	return;
}

extern void slpt_task_init_everytime(void);
extern void slpt_cache_prefetch_ops(void);

#else
static __always_inline void slpt_task_run_everytime(int need_to_go_kernel) {

}

static __always_inline void slpt_task_init_everytime(void) {

}

static __always_inline void slpt_cache_prefetch_ops(void) {

}
#endif

#ifdef CONFIG_SLPT
static inline int slpt_is_enabled(void) {
	return !!slpt_task_is_enabled;
}
#else
static inline int slpt_is_enabled(void) {
	return 0;
}
#endif

#endif /* SLPT_H_ */
