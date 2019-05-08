#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/interrupt.h>

#include <mach/jzfb.h>

#include "jz_fb.h"
#include "regs.h"
#include "slcd_te_vsync.h"

//#if !defined(CONFIG_SLCDC_CONTINUA)
extern int dump_gpio_regs(char *buffer, int port);

#if 1
static int dump_gpio_regs_l(void)
{
	char buffer[2048];
	int len;

	memset((void*)&buffer[0], 0, 2048);
	len = dump_gpio_regs(&buffer[0], -1);
	printk(KERN_DEBUG " %s() len: %d\n", __FUNCTION__, len);
	printk(KERN_DEBUG " %s\n", &buffer[0]);

	return 0;
}
#else
#define  dump_gpio_regs_l()			\
	do {					\
	} while (0);

#endif

void request_te_vsync_refresh(struct jzfb *jzfb)
{
	struct slcd_te *te;
	te = &jzfb->slcd_te;
	/* irq spin lock ? */

	printk(KERN_DEBUG " %s te->refresh_request=%d\n", __FUNCTION__, te->refresh_request);

	if(te->refresh_request == 0) {
		te->refresh_request = 1;
		dump_gpio_regs_l();
/*
<7>[   14.382440]  request_te_vsync_refresh te->refresh_request=0
<7>[   14.382519]  dump_gpio_regs_l() len: 
<7>[   14.382539]  PIN	FLG	INT		MASK		PAT1		PAT0
<7>[   14.382546] 0x41ff60f4	0x00000000	0x40002000	0x2903df0f	0x6903fe0a	0x80fc00f4
<7>[   14.382555] 0xa0000000	0x00000000	0x00000000	0xf030003f	0xf03001bf	0x00000000
<7>[   14.382565] 0x08090400	0x08000000	0x08010000	0x0eeefbff	0x0f35ffff	0x08080000
<7>[   14.382574] 0xf28a0000	0x00000000	0x00040000	0x1bfaffff	0x3ffeffff	0xc0040000
<7>[   14.382583] 0xc0000008	0x00000000	0x00000400	0x30f00004	0x30f00400	0xc0000409
<7>[   14.382593] 0x0000004c	0x00000000	0x00000000	0x0000fc00	0x0000fccf	0x000000c0
*/

#if 0
		/* cleare gpio irq pendding */
		/*
		  PC Address base of GPIO Port C: 0x10010200
		  PxFLGC: PxBase + 0x58
		*/
#define REG_GPIO_PC_FLG *(volatile unsigned long *)((unsigned long)0xB0010200 + 0X50)
#define REG_GPIO_PC_FLGC *(volatile unsigned long *)((unsigned long)0xB0010200 + 0X58)

		printk(KERN_DEBUG " %s REG_GPIO_PC_FLG: 0x%08x\n", __FUNCTION__, REG_GPIO_PC_FLG);
		REG_GPIO_PC_FLGC = 1<<27; /* PC_27( 91) */
		printk(KERN_DEBUG " %s REG_GPIO_PC_FLG: 0x%08x\n", __FUNCTION__, REG_GPIO_PC_FLG);
#endif

		//disable_irq(te->te_irq_no);
		enable_irq(te->te_irq_no);
		//printk(KERN_DEBUG " %s te->refresh_request\n", __FUNCTION__);
		dump_gpio_regs_l();
/*
<7>[   14.298312]  request_te_vsync_refresh te->refresh_request
<7>[   14.298364]  dump_gpio_regs_l() len: 
<7>[   14.298380]  PIN	FLG	INT		MASK		PAT1		PAT0
<7>[   14.298386] 0x413f6054	0x00000000	0x40002000	0x2903df0f	0x6903fe0a	0x80fc00f4
<7>[   14.298396] 0xa0000000	0x00000000	0x00000000	0xf030003f	0xf03001bf	0x00000000
<7>[   14.298405] 0x08090400	0x00000000	0x08010000	0x0eeefbff	0x0f35ffff	0x08080000
<7>[   14.298414] 0xf28a0000	0x00000000	0x00040000	0x1bfaffff	0x3ffeffff	0xc0040000
<7>[   14.298424] 0xc0000008	0x00000000	0x00000400	0x30f00004	0x30f00400	0xc0000409
<7>[   14.298433] 0x0000004c	0x00000000	0x00000000	0x0000fc00	0x0000fccf	0x000000c0
*/

	}
}

static irqreturn_t jzfb_te_irq_handler(int irq, void *data)
{
	struct jzfb *jzfb = (struct jzfb *)data;
	struct slcd_te *te;

	te = &jzfb->slcd_te;

	printk(KERN_DEBUG " %s te->refresh_request=%d\n", __FUNCTION__, te->refresh_request);
	/* irq spin lock ? */
	if (te->refresh_request > 0) {
		jzfb_slcd_restart(jzfb);
		te->refresh_request = 0;
	}


	dump_gpio_regs_l();

	/* disable te irq */
	disable_irq_nosync(te->te_irq_no);

	//dump_gpio_regs_l();
/*
<7>[   14.382623]  jzfb_te_irq_handler te->refresh_request=1
<7>[   14.382685]  dump_gpio_regs_l() len: 
<7>[   14.382702]  PIN	FLG	INT		MASK		PAT1		PAT0
<7>[   14.382709] 0x41ff60f4	0x00000000	0x40002000	0x2903df0f	0x6903fe0a	0x80fc00f4
<7>[   14.382718] 0xa0000000	0x00000000	0x00000000	0xf030003f	0xf03001bf	0x00000000
<7>[   14.382727] 0x08090400	0x00000000	0x08010000	0x0eeefbff	0x0f35ffff	0x08080000
<7>[   14.382737] 0xf28a0000	0x00000000	0x00040000	0x1bfaffff	0x3ffeffff	0xc0040000
<7>[   14.382746] 0xc000000a	0x00000000	0x00000400	0x30f00004	0x30f00400	0xc0000409
<7>[   14.382755] 0x0000004c	0x00000000	0x00000000	0x0000fc00	0x0000fccf	0x000000c0
*/
	return IRQ_HANDLED;
}

static irqreturn_t jzfb_te_irq_thread_handler(int irq, void *data)
{
	struct jzfb *jzfb = (struct jzfb *)data;
	struct slcd_te *te;

	te = &jzfb->slcd_te;

	printk(KERN_DEBUG " %s te->refresh_request=%d\n", __FUNCTION__, te->refresh_request);

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

	dump_gpio_regs_l();

	if (te_gpio && gpio_request_one(te_gpio, GPIOF_DIR_IN, "slcd_te_gpio")) {
		dev_err(jzfb->dev, "gpio request slcd te gpio faile, te_gpio=%d\n", te_gpio);
		return -EBUSY;
	}

	te_irq_no = gpio_to_irq(te_gpio);
	printk(KERN_DEBUG " %s te_irq_no=%d\n", __FUNCTION__, te_irq_no);


	irq_name = (char *) kzalloc(30, GFP_KERNEL);
	sprintf(irq_name, "slcd_te_irq(%d:%d)", te_gpio, te_irq_level);

	dump_gpio_regs_l();

#if 1
	if (request_irq(te_irq_no, jzfb_te_irq_handler,
			te_irq_level | IRQF_DISABLED,
			irq_name, jzfb)) {
		dev_err(jzfb->dev,"slcd te request irq failed\n");
		ret = -EINVAL;
		goto err;
	}

#else
	/* thread irq */
	if (request_threaded_irq(te_irq_no, jzfb_te_irq_handler, jzfb_te_irq_thread_handler,
				 te_irq_level | IRQF_DISABLED | IRQF_ONESHOT,
				 irq_name, jzfb)) {
		dev_err(jzfb->dev,"slcd te request irq failed\n");
		ret = -EINVAL;
		goto err;
	}
#endif

	dump_gpio_regs_l();

	//disable_irq_nosync(te_irq_no);

	te->te_irq_no = te_irq_no;
err:

	return ret;
}

//#endif /*!defined(CONFIG_SLCDC_CONTINUA)*/
