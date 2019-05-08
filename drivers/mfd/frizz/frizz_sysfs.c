#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/list.h>
#include "frizz_ioctl_hardware.h"
#include "frizz_ioctl.h"
#include "frizz_file_id_list.h"
#include "sensors.h"
#include "frizz_gpio.h"

static char *HARDWARE_PATH = "/etc/from.bin";
//the version is read in the frizz_connection.c , calls by driver_init & hardware_download
extern struct list_head head;
extern unsigned long Firmware_version;

extern sensor_data_changed_t record_sensor_list;
static char* Id2Name(int ID);

extern unsigned int pedo_interval;
extern unsigned int slpt_onoff;

unsigned int hand_right = 0;

static ssize_t burn_hardware(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned long int cmd;
	if(strict_strtoul(buf, 10, &cmd))
		return -EINVAL;
	if(cmd > 0)
	{
		frizz_ioctl_hardware_download_firmware(HARDWARE_PATH);
	}
	return count;
}

static ssize_t get_version(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	unsigned long version = Firmware_version;
	return sprintf(buf, "%ld\n", version);
}

static ssize_t reset_hardware(struct kobject *kobj, struct kobj_attribute *attr,const char *buf, size_t count)
{
	unsigned long int cmd;
	struct list_head *plist;
	struct file_id_node *node = NULL;
	if(strict_strtoul(buf, 10, &cmd))
		return -EINVAL;
	if(cmd > 0)
	{
		list_for_each(plist,&head)
		{
			node = list_entry(plist, struct file_id_node, list);
			node->sdc.mflag = 0;
			node->sdc.aflag = 0;

		}
		frizz_ioctl_hardware_reset();
	}
	return count;
}

static ssize_t set_g_chip_orientation(struct kobject *kobj, struct kobj_attribute *attr,const char *buf, size_t count)
{
	unsigned long int cmd;
	if(strict_strtoul(buf, 10, &cmd))
		return -EINVAL;
	if(cmd > 7 || cmd < 0)
		return -EINVAL;
	keep_frizz_wakeup();
	init_g_chip_orientation(cmd);
	release_frizz_wakeup();
	return count;
}

static ssize_t set_right_hand_wear(struct kobject *kobj, struct kobj_attribute *attr,const char *buf, size_t count)
{
	unsigned long int cmd;
	int ret = -1;
	if(strict_strtoul(buf, 10, &cmd))
		return -EINVAL;
	if(cmd > 1 || cmd < 0)
		return -EINVAL;
	keep_frizz_wakeup();
	ret = right_hand_wear(cmd);
	release_frizz_wakeup();
	if(ret < 0) {
		hand_right = -ENODEV;
		return ret;
	}

	hand_right = cmd;
	return count;
}

static ssize_t get_right_hand_wear(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", hand_right);
}
//this call is not saft if the sensor_list is not the static one.
//but the designed have make it, if you wanna use it , be careful or add a lock
static ssize_t get_sensors_list(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int i;
	//malloc
	char *sensors = 0;

	//40 is the magic, for each sensor-name length.
	sensors = (char *)kzalloc(40 * HUB_MANAGER, GFP_KERNEL);
	if(!sensors) {
		return  sprintf(buf, "%s\n", "Get list Error, try Again.");;
	}
	for(i = 0; i < 32; i++) {
		if((1 << i) & record_sensor_list.aflag)
			strcat(sensors, Id2Name(i));
	}
	for(i = 0; i < 32; i++) {
		if((1 << i) & record_sensor_list.mflag)
			strcat(sensors, Id2Name(i + 31));
	}
	i = sprintf(buf, "%s", sensors);
	kfree(sensors);
	return i;
}

static ssize_t set_hardware_path(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	if(count <= 1)
		return -EINVAL;
	sscanf(buf, "%s", HARDWARE_PATH);
	return count;
}
static ssize_t set_pedo_interval_when_sleep(struct kobject *kobj, struct kobj_attribute *attr,const char *buf, size_t count)
{
	unsigned long int interval;
	if(strict_strtoul(buf, 10, &interval))
		return -EINVAL;
	pedo_interval = interval;
	return count;
}
static ssize_t set_slpt_onoff(struct kobject *kobj, struct kobj_attribute *attr,const char *buf, size_t count)
{
	unsigned long int cmd;
	if(strict_strtoul(buf, 10, &cmd))
		return -EINVAL;
	slpt_onoff = cmd;
	return count;
}

static struct kobj_attribute attr_burn_hardware = __ATTR(burn_hardware, 0222, NULL, burn_hardware);
static struct kobj_attribute attr_get_version = __ATTR(get_version, 0444, get_version, NULL);
static struct kobj_attribute attr_reset_hardware = __ATTR(reset_hardware, 0222, NULL, reset_hardware);
static struct kobj_attribute attr_set_hardware_path = __ATTR(set_hardware_path, 0222, NULL, set_hardware_path);
static struct kobj_attribute attr_get_sensors_list = __ATTR(get_sensors_list, 0444, get_sensors_list, NULL);
static struct kobj_attribute attr_set_pedo_interval_when_sleep = __ATTR(set_pedo_interval_when_sleep, 0222, NULL, set_pedo_interval_when_sleep);
static struct kobj_attribute attr_set_slpt_onoff = __ATTR(set_slpt_onoff, 0222, NULL, set_slpt_onoff);
static struct kobj_attribute attr_set_g_chip_orientation =
                                                   __ATTR(set_g_chip_orientation, 0222, NULL, set_g_chip_orientation);
static struct kobj_attribute attr_set_right_hand_wear = __ATTR(set_right_hand_wear, 0666, get_right_hand_wear, set_right_hand_wear);

static struct attribute *k_sysfs_interface[] = {
	&attr_burn_hardware.attr,
	&attr_get_version.attr,
	&attr_reset_hardware.attr,
	&attr_set_hardware_path.attr,
	&attr_get_sensors_list.attr,
	&attr_set_pedo_interval_when_sleep.attr,
	&attr_set_slpt_onoff.attr,
	&attr_set_g_chip_orientation.attr,
	&attr_set_right_hand_wear.attr,
	NULL,
};

static struct attribute_group interface_group = {
	.attrs = k_sysfs_interface,
};

int create_sysfs_interface(void)
{
	struct kobject *kobj = NULL;
	int ret = 0;
	kobj = kobject_create_and_add("frizz", NULL);
	if(!kobj)
	{
		printk("frizz creat sysfs-interface failed\n");
		return -1;
	}
	ret = sysfs_create_group(kobj, &interface_group);
	if(ret)
	{
		printk("frizz sysfs create group failed\n");
		kobject_put(kobj);
		sysfs_remove_group(kobj, &interface_group);
		return -1;
	}
	printk("frizz create sysfs interface successed!!\n");
	return 0;

}




static char* Id2Name(int ID)
{
	switch(ID){

		case  SENSOR_TYPE_ACCELEROMETER:
			return "ACCELEROMETER\n";
		case  SENSOR_TYPE_MAGNETIC_FIELD:
			return "MAGNETIC_FIELD\n";
		case  SENSOR_TYPE_ORIENTATION:
			return "ORIENTATION\n";
		case  SENSOR_TYPE_GYROSCOPE:
			return "GYROSCOPE\n";
		case  SENSOR_TYPE_LIGHT:
			return "LIGHT\n";
		case  SENSOR_TYPE_PRESSURE:
			return "PRESSURE\n";
		case  SENSOR_TYPE_TEMPERATURE:
			return "TEMPERATURE\n";
		case  SENSOR_TYPE_PROXIMITY:
			return "PROXIMITY\n";
		case  SENSOR_TYPE_GRAVITY:
			return "GRAVITY\n";
		case  SENSOR_TYPE_LINEAR_ACCELERATION:
			return "LINEAR_ACCELERATION\n";
		case  SENSOR_TYPE_ROTATION_VECTOR:
			return "ROTATION_VECTOR\n";
		case  SENSOR_TYPE_RELATIVE_HUMIDITY:
			return "RELATIVE_HUMIDITY\n";
		case  SENSOR_TYPE_AMBIENT_TEMPERATURE:
			return "AMBIENT_TEMPERATURE\n";
		case  SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
			return "MAGNETIC_FIELD_UNCALIBRATED\n";
		case  SENSOR_TYPE_GAME_ROTATION_VECTOR:
			return "GAME_ROTATION_VECTOR\n";
		case  SENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
			return "GYROSCOPE_UNCALIBRATED\n";
		case  SENSOR_TYPE_SIGNIFICANT_MOTION:
			return "SIGNIFICANT_MOTION\n";
		case  SENSOR_TYPE_STEP_DETECTOR:
			return "STEP_DETECTOR\n";
		case  SENSOR_TYPE_STEP_COUNTER:
			return "STEP_COUNTER\n";
		case  SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR:
			return "GEOMAGNETIC_ROTATION_VECTOR\n";

		case  SENSOR_TYPE_PDR:
			return "PDR\n";
		case  SENSOR_TYPE_MAGNET:
			return "MAGNET\n";
		case  SENSOR_TYPE_GYRO:
			return "GYRO\n";
		case  SENSOR_TYPE_ACCEL_POWER:
			return "ACCEL_POWER\n";
		case  SENSOR_TYPE_ACCEL_LPF:
			return "ACCEL_LPF\n";
		case  SENSOR_TYPE_ACCEL_LINEAR:
			return "ACCEL_LINEAR\n";
		case  SENSOR_TYPE_MAGNET_PARAMETER:
			return "MAGNET_PARAMETER\n";
		case  SENSOR_TYPE_MAGNET_CALIB_SOFT:
			return "MAGNET_CALIB_SOFT\n";
		case  SENSOR_TYPE_MAGNET_LPF:
			return "MAGNET_LPF\n";
		case  SENSOR_TYPE_GYRO_LPF:
			return "GYRO_LPF\n";
		case  SENSOR_TYPE_DIRECTION:
			return "DIRECTION\n";
		case  SENSOR_TYPE_POSTURE:
			return "POSTURE\n";
		case  SENSOR_TYPE_ROTATION_MATRIX:
			return "ROTATION_MATRIX\n";
		case  SENSOR_TYPE_VELOCITY:
			return "VELOCITY\n";
		case  SENSOR_TYPE_RELATIVE_POSITION:
			return "RELATIVE_POSITION\n";
		case  SENSOR_TYPE_MIGRATION_LENGTH:
			return "MIGRATION_LENGTH\n";
		case  SENSOR_TYPE_CYCLIC_TIMER:
			return "CYCLIC_TIMER\n";
		case  SENSOR_TYPE_DEBUG_QUEUE_IN:
			return "DEBUG_QUEUE_IN\n";
		case  SENSOR_TYPE_DEBUG_STD_IN:
			return "DEBUG_STD_IN\n";
		case  SENSOR_TYPE_ISP:
			return "ISP\n";
		case  SENSOR_TYPE_ACCEL_FALL_DOWN:
			return "ACCEL_FALL_DOWN\n";
		case  SENSOR_TYPE_ACCEL_POS_DET:
			return "ACCEL_POS_DET\n";
		case  SENSOR_TYPE_PDR_GEOFENCING:
			return "PDR_GEOFENCING\n";
		case  SENSOR_TYPE_GESTURE:
			return "GESTURE\n";
		case SENSOR_TYPE_ACTIVITY:
			return "SLEEPMOTION\n";
		default:
			printk("Frizz sysfs, id = %d \n", ID);
			return "unknow sensor type\n";
	}
}
