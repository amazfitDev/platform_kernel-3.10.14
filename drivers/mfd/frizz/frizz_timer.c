
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#include "frizz_connection.h"

#include "frizz_workqueue.h"
#include "frizz_debug.h"
//#include "frizz_callback.h"

//#define DEFAULT_INTERVAL (10000 * HZ)
#define DEFAULT_INTERVAL (0.01 * HZ)

/*!< information of fixed-cycle */
static struct timer_list interval_timer;

/*!< update interval of fixed-cycle */
static int expires = DEFAULT_INTERVAL;

/*!< flag of batch mode */
static int batch_flag     = 0;

/*!< batch mode of fifo full flag*/
static int fifo_full_flag = 0;

/*!< notice interval of batch mode */
static uint64_t timeout_ns = 3600000000000LL;

/*!< sensor sampling time of batch mode */
static uint64_t period_ns  = 3600000000000LL;

/*!< count of period_ns*/
static uint64_t counter = 0;

/*!< calculate present timeout_ns */
static uint64_t limit_time = 0;

static void frizz_interval(unsigned long arg)
{
    //struct timeval tv;

    //Don't use semaphore when interrupt context execute.
    //use workqueue.
    workqueue_analysis_fifo();

    if (batch_flag == 1) {

        counter++;
        limit_time = counter * period_ns;
        printk("count %d \n", (int) counter);

        if (fifo_full_flag == 0) {
            if (limit_time >= timeout_ns) {
                counter = 0;
            }
        }
    }

//  DEBUG_PRINT("timer 19\n");

    //do_gettimeofday(&tv);

    //DEBUG_PRINT("time us %ld\n", tv.tv_usec);

    //Setting of update interval.
    //high-load of software interrupt. Have to improve.
    mod_timer(&interval_timer, jiffies + expires);
}

void init_frizz_timer(void) {
    //int ret;

    init_timer(&interval_timer);

    interval_timer.function = frizz_interval;
    interval_timer.data     = 1;//(unsigned long)&interval_timer;
    interval_timer.expires  = jiffies + expires;

//    add_timer(&interval_timer);

    //setup_timer( &interval_timer, frizz_interval, 0 );
    //ret = mod_timer( &interval_timer, jiffies + msecs_to_jiffies(100) );
    //period_ns = expires * 1000000;
}

void update_timeout(uint64_t value) {

    batch_flag = 1;
    if ((value > 1000000) && (value < timeout_ns)) {
        timeout_ns = value;
        DEBUG_PRINT("update batch\n");
    }
}

void update_fifo_full_flag(int flag) {

    fifo_full_flag = flag;

}

 void delete_frizz_timer(void) {

    //releasing of fixed-cycle timer
    del_timer_sync(&interval_timer);
    //int ret;

    //ret = del_timer( &interval_timer );
}

//callback function
void interrupt_timer(unsigned int header, unsigned int sensor_data[]) {

    int interval;

    if ((sensor_data[2] & 0xf) == 0x3) {
        DEBUG_PRINT("FIFO_FULL_NOTIFY\n");

        counter = 0;

    } else {
        interval = sensor_data[1];

        if (((expires * 1000) > interval) && (interval > 0)) {
            expires = ((interval / 1000) * HZ);

            period_ns = interval * 1000000;
        }
    }
}
