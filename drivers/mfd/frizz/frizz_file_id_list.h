#ifndef __FRIZZ_FILE_ID_LIST_H__
#define __FRIZZ_FILE_ID_LIST_H__
#include "frizz_hal_if.h"
#include <linux/list.h>
#include <linux/wait.h>
struct file_id_node
{
	sensor_data_changed_t sdc;
	struct list_head list;
	int poll_condition;
};

#endif
