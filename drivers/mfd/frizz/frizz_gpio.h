#ifndef __FRIZZ_GPIO_H__
#define __FRIZZ_GPIO_H__
#include <linux/i2c/frizz.h>
/*!
 * initialize gpio process
 *
 * @return 0=sucess, otherwise 0=fail
 */

int init_frizz_gpio(struct frizz_platform_data*);

/*!
 * delete gpio process
 */
void delete_frizz_gpio(void);

int keep_frizz_wakeup(void);
int release_frizz_wakeup(void);

void set_wakeup_gpio(int mode);

void enable_irq_gpio(void);

extern int frizz_irq;
#endif
