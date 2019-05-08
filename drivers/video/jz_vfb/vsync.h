#include <linux/wait.h>
#include <linux/sched.h>

struct vsync_wq_t {
	wait_queue_head_t vsync_wq;
	struct task_struct *vsync_thread;
	unsigned int vsync_skip_map;	/* 10 bits width */
	int vsync_skip_ratio;
#define TIMESTAMP_CAP	16
	struct {
		volatile int wp; /* write position */
		int rp;	/* read position */
		u64 value[TIMESTAMP_CAP];
	} timestamp;

} vsync_data;




