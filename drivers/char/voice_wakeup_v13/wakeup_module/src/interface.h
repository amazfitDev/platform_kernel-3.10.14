#ifndef __INTERFACE_H__
#define __INTERFACE_H__
#include <tcsm_layout.h>

extern int (*h_handler)(const char *fmt, ...);
#define SLEEP_VOICE_DEBUG

#ifdef SLEEP_VOICE_DEBUG
#define printk	h_handler
#else
#define debug_print(fmt, args...) do{} while(0)
#define printk debug_print
#endif

enum open_mode {
	EARLY_SLEEP = 1,
	DEEP_SLEEP,
	NORMAL_RECORD,
	NORMAL_WAKEUP
};


#define DMIC_IOCTL_SET_SAMPLERATE	0x200
#define DMIC_IOCTL_SET_CHANNEL		0x201


/* same define as kernel */
#define		SLEEP_BUFFER_SIZE	(32 * 1024)
#define		NR_BUFFERS			(8)

struct sleep_buffer {
	unsigned char *buffer[NR_BUFFERS];
	unsigned int nr_buffers;
	unsigned long total_len;
};

#define LOAD_ADDR	0x81f00000 /* 31M */
#define LOAD_SIZE	(256 * 1024)

#ifdef SLEEP_VOICE_DEBUG
#define OFF_TDR         (0x00)
#define OFF_LCR         (0x0C)
#define OFF_LSR         (0x14)

#define LSR_TDRQ        (1 << 5)
#define LSR_TEMT        (1 << 6)

#define UART1_IOBASE    0x10032000
#define UART_IOBASE (UART1_IOBASE + 0xa0000000)
#define TCSM_PCHAR(x)                           \
	*((volatile unsigned int*)(UART_IOBASE+OFF_TDR)) = x;     \
while ((*((volatile unsigned int*)(UART_IOBASE + OFF_LSR)) & (LSR_TDRQ | LSR_TEMT)) != (LSR_TDRQ | LSR_TEMT))

static inline void serial_put_hex(unsigned int x) {
	int i;
	unsigned int d;
	for(i = 7;i >= 0;i--) {
		d = (x  >> (i * 4)) & 0xf;
		if(d < 10) d += '0';
		else d += 'A' - 10;
		TCSM_PCHAR(d);
	}
}
#else
#define TCSM_PCHAR(x)	do {} while(0)
#define serial_put_hex(x) do {} while(0)
#endif



#endif



