#include <linux/time.h>
#include <linux/ioctl.h>

#ifndef __FRIZZ_HAL_H__
#define __FRIZZ_HAL_H__

#include "frizz_reg.h"

enum{
	ERR_UNKNOW = -1,
	ERR_NOSENSOR = -2,
};

#define IOC_MAGIC '9'

#define FRIZZ_IOCTL_SENSOR   1   /*!< Start Index of Android IOCTL */
#define FRIZZ_IOCTL_MCC      32  /*!< Start Index of MCC IOCTL */
#define FRIZZ_IOCTL_HARDWARE 64  /*!< Start Index of Hardware IOCTL */

typedef struct{
    uint32_t sensor_id;
    uint32_t test_loop;
    uint32_t raw_data[MAX_HWFRIZZ_FIFO_SIZE];
} sensor_info;

/*! @struct sensor_enable_t
 *  @brief
 */
typedef struct {
    int code; /*!< Android Sensor type*/
    int flag; /*!< 0 : disable sensor, 1:enable sensor*/
} sensor_enable_t;

typedef struct {
    uint32_t aflag;  // flag for android sensor
    uint32_t mflag;  // flag for mcc sensor
} sensor_data_changed_t;

typedef struct {
	uint32_t version;
} firmware_version_t;

typedef struct {
    uint32_t counter;
}pedometer_counter_t;

/*! @struct sensor_delay_t
 *  @brief  control sensor delay time
 */

typedef struct {
    int code; /*!< Android Sensor type*/
    int ms;   /*!< millisecond*/
} sensor_delay_t;

/*! @struct sensor_data_t
 *  @brief input sensor data
 */
typedef struct {
    int            code;
    struct timeval time;

    union {
    uint32_t u32_value[6];
	float    f32_value[6];
	uint64_t u64_value;

	struct {
	    float rpos[2];
	    float velo[2];
	    float total_dst;
	    unsigned int count;
	};
    };
} sensor_data_t;

/*! @struct batch_t
 *  @brief set batch mode
 */
typedef struct {
    int fifo_full_flag;
    int code;
    uint64_t period_ns;
    uint64_t timeout_ns;
    uint64_t simulate_timeout_ns;
} batch_t;

/*!< set enable or disable.*/
#define FRIZZ_IOCTL_SENSOR_SET_ENABLE       _IOW(IOC_MAGIC, FRIZZ_IOCTL_SENSOR,     sensor_enable_t*)

/*!< get sensor status. (enable or disable)*/
#define FRIZZ_IOCTL_SENSOR_GET_ENABLE       _IOR(IOC_MAGIC, FRIZZ_IOCTL_SENSOR + 1, sensor_enable_t*)

/*!< set sensor delay time */
#define FRIZZ_IOCTL_SENSOR_SET_DELAY        _IOW(IOC_MAGIC, FRIZZ_IOCTL_SENSOR + 2, sensor_delay_t*)

/*!< get sensor delay time */
#define FRIZZ_IOCTL_SENSOR_GET_DELAY        _IOR(IOC_MAGIC, FRIZZ_IOCTL_SENSOR + 3, sensor_delay_t*)

/*!< get sensor data*/
#define FRIZZ_IOCTL_SENSOR_GET_DATA         _IOR(IOC_MAGIC, FRIZZ_IOCTL_SENSOR + 4, sensor_data_t*)

/*!< return time  that can be stored software fifo.*/
#define FRIZZ_IOCTL_SENSOR_SIMULATE_TIMEOUT _IOR(IOC_MAGIC, FRIZZ_IOCTL_SENSOR + 5, batch_t*)

/*!< set batch mode */
#define FRIZZ_IOCTL_SENSOR_SET_BATCH        _IOW(IOC_MAGIC, FRIZZ_IOCTL_SENSOR + 6, batch_t*)

/*!< flush software fifo in frizz*/
#define FRIZZ_IOCTL_SENSOR_FLUSH_FIFO       _IOW(IOC_MAGIC, FRIZZ_IOCTL_SENSOR + 7, void*)

#define FRIZZ_IOCTL_GET_SENSOR_DATA_CHANGED _IOR(IOC_MAGIC, FRIZZ_IOCTL_SENSOR + 8, sensor_data_changed_t*)

#define FRIZZ_IOCTL_GET_FIRMWARE_VERSION    _IOR(IOC_MAGIC, FRIZZ_IOCTL_SENSOR + 9, firmware_version_t*)

#define FRIZZ_IOCTL_SET_PEDO_COUNTER        _IOW(IOC_MAGIC, FRIZZ_IOCTL_SENSOR + 10, pedometer_counter_t*)
/*!< set enable or disable.*/
#define FRIZZ_IOCTL_MCC_SET_ENABLE       _IOW(IOC_MAGIC, FRIZZ_IOCTL_MCC,     sensor_enable_t*)

/*!< get sensor status. (enable or disable)*/
#define FRIZZ_IOCTL_MCC_GET_ENABLE       _IOR(IOC_MAGIC, FRIZZ_IOCTL_MCC + 1, sensor_enable_t*)

/*!< get sensor data*/
#define FRIZZ_IOCTL_MCC_GET_DATA         _IOR(IOC_MAGIC, FRIZZ_IOCTL_MCC + 2, sensor_data_t*)

/*!< set sensor delay time */
#define FRIZZ_IOCTL_MCC_SET_DELAY        _IOW(IOC_MAGIC, FRIZZ_IOCTL_MCC + 3, sensor_delay_t*)

/*!< get sensor delay time */
#define FRIZZ_IOCTL_MCC_GET_DELAY        _IOR(IOC_MAGIC, FRIZZ_IOCTL_MCC + 4, sensor_delay_t*)

/*!< reset frizz*/
#define FRIZZ_IOCTL_HARDWARE_RESET              _IOW(IOC_MAGIC, FRIZZ_IOCTL_HARDWARE,     void*)

/*!< stall frizz*/
#define FRIZZ_IOCTL_HARDWARE_STALL              _IOW(IOC_MAGIC, FRIZZ_IOCTL_HARDWARE + 1, void*)

/*!< download firmware*/
#define FRIZZ_IOCTL_HARDWARE_DOWNLOAD_FIRMWARE  _IOW(IOC_MAGIC, FRIZZ_IOCTL_HARDWARE + 2, char*)

/*!< enable frizz gpio*/
#define FRIZZ_IOCTL_HARDWARE_ENABLE_GPIO        _IOW(IOC_MAGIC, FRIZZ_IOCTL_HARDWARE + 3, void*)

/*!< fizz work status test*/
#define FRIZZ_IOCTL_FW_TEST                     _IOW(IOC_MAGIC, FRIZZ_IOCTL_HARDWARE + 4, sensor_info*)

#endif
