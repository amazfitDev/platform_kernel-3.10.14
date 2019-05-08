#ifndef __VOICE_WAKEUP_MODULE_H__
#define __VOICE_WAKEUP_MODULE_H__

#include<soc/tcsm_layout.h>


enum open_mode {
	EARLY_SLEEP = 1,
	DEEP_SLEEP,
	NORMAL_RECORD,
	NORMAL_WAKEUP
};



#define		SLEEP_BUFFER_SIZE	(32 * 1024)
#define		NR_BUFFERS			(8)

struct sleep_buffer {
	unsigned char *buffer[NR_BUFFERS];
	unsigned int nr_buffers;
	unsigned long total_len;
};


#define DMIC_IOCTL_SET_SAMPLERATE	0x200

#ifdef CONFIG_JZ_DMIC_WAKEUP_V13
#define DMIC_IOCTL_SET_CHANNEL	0x201
#endif

#ifdef CONFIG_JZ_DMIC_WAKEUP_V13
#define WAKEUP_HANDLER_ADDR	(0x81f00004)
#else
#define WAKEUP_HANDLER_ADDR	(0x8ff00004)
#endif

#define SYS_WAKEUP_OK		(0x1)
#define SYS_WAKEUP_FAILED	(0x2)
#define SYS_NEED_DATA		(0x3)

#define TCSM_BANK5_V		(0xb3427000)
#define TCSM_BUFFER_SIZE	(4096)

#define TCSM_DATA_BUFFER_ADDR	VOICE_TCSM_DATA_BUF
#define TCSM_DATA_BUFFER_SIZE	VOICE_TCSM_DATA_BUF_SIZE

int wakeup_module_open(int mode);

int wakeup_module_close(int mode);

void wakeup_module_cache_prefetch(void);

int wakeup_module_handler(int par);

dma_addr_t wakeup_module_get_dma_address(void);

unsigned char wakeup_module_get_resource_addr(void);

int wakeup_module_ioctl(int cmd, unsigned long args);


int wakeup_module_process_data(void);

int wakeup_module_is_cpu_wakeup_by_dmic(void);


int wakeup_module_set_sleep_buffer(struct sleep_buffer *);

int wakeup_module_get_sleep_process(void);

int wakeup_module_wakeup_enable(int enable);

int wakeup_module_is_wakeup_enabled(void);

int wakeup_module_cpu_should_sleep(void);



#endif
