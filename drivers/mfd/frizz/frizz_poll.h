#ifndef __FRIZZ_POLL_H__
#define __FRIZZ_POLL_H__
#include <linux/poll.h>
extern wait_queue_head_t read_q;
/*!
 * sleep until a condition gets true
 *
 * @param[in] file discripter
 * @param[in] poll table
 * @return result of interrupt
 */
unsigned int frizz_poll_sensor(struct file*, poll_table*);
void init_poll_queue(void);
#endif
