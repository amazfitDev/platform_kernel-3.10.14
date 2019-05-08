#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include "frizz_debug.h"
#include "frizz_file_id_list.h"
extern struct list_head head;
extern struct mutex close_lock;
wait_queue_head_t read_q;
void init_poll_queue(void)
{
	init_waitqueue_head(&read_q);
}

unsigned int frizz_poll_sensor(struct file* file, poll_table* wait)
{

    struct file_id_node *node = (struct file_id_node*)(file->private_data);
	node->poll_condition = 0;
	wait_event_interruptible(read_q, (node->poll_condition != 0));
    return POLLIN | POLLRDNORM;
}

void interrupt_poll(void)
{
	struct list_head *plist;
	struct file_id_node *node = NULL;
	mutex_lock(&close_lock);
	list_for_each(plist,&head)
	{
		node = list_entry(plist, struct file_id_node, list);
		node->poll_condition = 1;
	}
	wake_up_interruptible_all(&(read_q));
	mutex_unlock(&close_lock);
}
