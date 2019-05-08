#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/gpio.h>

#include "frizz_workqueue.h"
#include "frizz_debug.h"
#include "frizz_gpio.h"

static atomic_t v = ATOMIC_INIT(0);

int frizz_irq, frizz_reset, frizz_wakeup;
static int irq_number = 0;
static char *irq_button = "irq_button";
static char *reset_button = "reset_button";
static char *wakeup_button = "wakeup_button";

extern unsigned int slpt_onoff;
void set_wakeup_gpio(int mode);
//this counter is for debug, for maybe the frizz_connection can not get
//respone, and we should know if the frizz had responed yet.
int irq_counter = 0;

static irqreturn_t frizz_isr(int irq, void *device_instance)
{
	irq_counter += 1;
    workqueue_analysis_fifo();
    return IRQ_HANDLED;
}

void delete_frizz_gpio(void)
{
    free_irq(irq_number, irq_button);
}

int init_frizz_gpio(struct frizz_platform_data* frizz_pdata)
{
    int ret = 0;

	frizz_irq = frizz_pdata->irq_gpio;

    irq_number = gpio_to_irq(frizz_irq);

    printk("frizz interrupt gpio is %d\n", frizz_irq);
    gpio_request(frizz_irq, irq_button);
    gpio_direction_input(frizz_irq);

    frizz_reset = frizz_pdata->reset_gpio;
	printk("frizz reset gpio is %d \n", frizz_reset);
	ret = gpio_request(frizz_reset, reset_button);
	if(ret)
	{
		printk("frizz request failed (%d) \n", ret);
	}

	gpio_direction_output(frizz_reset, 0);
	mdelay(2);
	gpio_direction_output(frizz_reset, 1);

	frizz_wakeup = frizz_pdata->wakeup_frizz_gpio;
	printk("frizz wakeup gpio is %d \n", frizz_wakeup);
	ret = gpio_request(frizz_wakeup, wakeup_button);
	gpio_direction_output(frizz_wakeup, 1);

    return ret;
}
void enable_irq_gpio(void)
{
    int irq_error = 0;
    irq_error = request_irq(irq_number, frizz_isr, IRQF_TRIGGER_FALLING, "frizz", irq_button);

    if(!slpt_onoff) {
        //if slpt off then enable the irq wake, or do not enable
        enable_irq_wake(irq_number);
    }
}
//when use the low power mode,
//REMEBER: one keep_frizz_wakeup stand with one release_frizz_wakeup.
int keep_frizz_wakeup()
{
	atomic_inc(&v);
	if(atomic_read(&v) >= 1)
		set_wakeup_gpio(1);
 //when wakeup the frizz need to delay 3ms then to read the data
 //but if the frizz has been wakeup, just leave it to save the time.
	if(atomic_read(&v) == 1)
		msleep(3);
	return 0;
}
int release_frizz_wakeup()
{
	atomic_dec(&v);
	if(atomic_read(&v) < 0) {
		dump_stack();
		kprint("Frizz, the atomic value is not designed to be lower than Zero, check it!");
	}
	if(atomic_read(&v) <= 0)
	{
		atomic_set(&v, 0);
		set_wakeup_gpio(0);
	}
	return 0;
}
void set_wakeup_gpio(int mode)
{
	gpio_direction_output(frizz_wakeup, mode);
}
