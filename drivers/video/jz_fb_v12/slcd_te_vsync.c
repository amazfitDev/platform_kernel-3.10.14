#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/interrupt.h>

#include <mach/jzfb.h>

#include "jz_fb.h"
#include "regs.h"
#include "slcd_te_vsync.h"

//#if !defined(CONFIG_SLCDC_CONTINUA)

void request_te_vsync_refresh(struct jzfb *jzfb)
{
	struct slcd_te *te;
	te = &jzfb->slcd_te;
	/* irq spin lock ? */

	//printk(KERN_DEBUG " %s te->refresh_request=%d\n", __FUNCTION__, te->refresh_request);

	if(te->refresh_request == 0) {
		te->refresh_request = 1;
		/* cleare gpio irq pendding */
		enable_irq(te->te_irq_no);
	}

	return ;
}

static irqreturn_t jzfb_te_irq_handler(int irq, void *data)
{
	struct jzfb *jzfb = (struct jzfb *)data;
	struct slcd_te *te;

	te = &jzfb->slcd_te;

	//printk(KERN_DEBUG " %s te->refresh_request=%d\n", __FUNCTION__, te->refresh_request);
	/* irq spin lock ? */
	if (te->refresh_request > 0) {
		jzfb_slcd_restart(jzfb);
		te->refresh_request = 0;
	}


	/* disable te irq */
	disable_irq_nosync(te->te_irq_no);

	return IRQ_HANDLED;
}

int jzfb_te_irq_register(struct jzfb *jzfb)
{
	int ret = 0;
	struct slcd_te *te;
	unsigned int te_irq_no;
	char * irq_name;

#ifdef CONFIG_JZ_MIPI_DSI
	int te_gpio = jzfb->pdata->dsi_pdata->dsi_config.te_gpio;
	int te_irq_level = jzfb->pdata->dsi_pdata->dsi_config.te_irq_level;
#else
	int te_gpio = jzfb->pdata->smart_config.te_gpio;
	int te_irq_level = jzfb->pdata->smart_config.te_irq_level;
#endif

	te = &jzfb->slcd_te;
	te->te_gpio = te_gpio;
	te->te_gpio_level = te_irq_level;
	te->te_irq_no = 0;

	printk(KERN_DEBUG " %s te_gpio=%d te_irq_level=%d\n", __FUNCTION__, te_gpio, te_irq_level);
	printk(" %s te_gpio=%d te_irq_level=%d\n", __FUNCTION__, te_gpio, te_irq_level);

	if (te_gpio && gpio_request_one(te_gpio, GPIOF_DIR_IN, "slcd_te_gpio")) {
		dev_err(jzfb->dev, "gpio request slcd te gpio faile, te_gpio=%d\n", te_gpio);
		return -EBUSY;
	}

	te_irq_no = gpio_to_irq(te_gpio);
	printk(KERN_DEBUG " %s te_irq_no=%d\n", __FUNCTION__, te_irq_no);
	printk(" %s te_irq_no=%d\n", __FUNCTION__, te_irq_no);


	irq_name = (char *) kzalloc(30, GFP_KERNEL);
	sprintf(irq_name, "slcd_te_irq(%d:%d)", te_gpio, te_irq_level);

	if (request_irq(te_irq_no, jzfb_te_irq_handler,
			te_irq_level | IRQF_DISABLED,
			irq_name, jzfb)) {
		dev_err(jzfb->dev,"slcd te request irq failed\n");
		ret = -EINVAL;
		goto err;
	}

	disable_irq_nosync(te_irq_no);

	te->te_irq_no = te_irq_no;
err:

	return ret;
}

//#endif /*!defined(CONFIG_SLCDC_CONTINUA)*/
