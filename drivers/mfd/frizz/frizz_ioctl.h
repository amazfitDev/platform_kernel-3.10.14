#ifndef __FRIZZ_IOCTL_H__
#define __FRIZZ_IOCTL_H__
#include "frizz_file_id_list.h"

#define GESTURE_SHAKE_HAND    (1 << 0)
#define GESTURE_RAISE_HAND    (1 << 1)
#define GESTURE_LET_HAND_DOWN (1 << 2)
#define GESTURE_TURN_WRIST    (1 << 3)

#define GESTURE_SHAKE_HAND_SILENCE ((~GESTURE_SHAKE_HAND) | \
                                    (GESTURE_RAISE_HAND) | \
                                    (GESTURE_LET_HAND_DOWN) | \
                                    (GESTURE_TURN_WRIST))
#define GESTURE_REPORT_ALL (GESTURE_SHAKE_HAND | \
                            GESTURE_LET_HAND_DOWN | \
                            GESTURE_RAISE_HAND | \
                            GESTURE_TURN_WRIST)

#define PEDO_NOT_REPORT       0
#define PEDO_REPORT_NORMAL    1
/*!
 * enable frizz gpio interrupt
 *
 * @return 0=sucess, otherwise 0=fail
 */
int frizz_ioctl_enable_gpio_interrupt(void);

int frizz_ioctl_sensor_get_version(void);

/*!
 * IOCTL of Android Sensor type
 *
 * @param[in] ioctl command
 * @param[in] struct data
 * @return 0=sucess, otherwise 0=fail
 */
int frizz_ioctl_sensor( struct file_id_node*, unsigned int, unsigned long);

/*!
 * IOCTL of MCC Sensor type
 *
 * @param[in] ioctl command
 * @param[in] struct data
 * @return 0=sucess, otherwise 0=fail
 */
int frizz_ioctl_mcc(unsigned int, unsigned long);

/*!
 * IOCTL of frizz hardware
 *
 * @param[in] ioctl command
 * @param[in] struct data
 * @return 0=sucess, otherwise 0=fail
 */
int frizz_ioctl_hardware(unsigned int, unsigned long);

/*!
 * Download firmware in frizz driver.
 *
 * @param[in] file name (absolute path)
 * @return 0=sucess, otherwise 0=fail
 */
int frizz_ioctl_hardware_download_firmware(char*);

/*!
 * Stall frizz in frizz driver.
 *
 * @return 0=sucess, otherwise 0=fail
 */
int frizz_ioctl_hardware_stall(void);

/*!
 * Reset frizz in frizz driver.
 *
 * @return 0=sucess, otherwise 0=fail
 */
int frizz_ioctl_hardware_reset(void);

int set_pedo_interval(int interval);
/*!
 * Test whether frizz and sensor work normally?
 */
int frizz_fw_command_test(uint32_t sensor_id, uint32_t test_loop, unsigned long arg);

int set_gesture_state(unsigned int gesture);
int init_g_chip_orientation(unsigned int position);
int right_hand_wear(unsigned int isrighthand);
int set_fall_parameter(void);
/*!< This macro copies data from user area. */
#define COPY_FROM_USER(type)                                                    \
	int copy_from_user_ ## type(unsigned int cmd, void* arg, type *data){       \
	if (copy_from_user(data, (int __user *)arg,  sizeof(*data))) {              \
	    return -EFAULT;                                                         \
	}                                                                           \
	return 0;                                                                   \
    }

#define COPY_TO_USER(type)                                                                  \
	int copy_to_user_ ## type(unsigned int cmd, void* arg, type *data, rwlock_t lock){      \
	read_lock(&lock);                                                                       \
	if (copy_to_user((int __user *)arg, data, sizeof(*data))) {                             \
	    goto Error;                                                                         \
	}                                                                                       \
	read_unlock(&lock);                                                                     \
	return 0;                                                                               \
Error:                                                                                      \
	read_unlock(&lock);                                                                     \
	return -EFAULT;                                                                         \
    }

#endif

