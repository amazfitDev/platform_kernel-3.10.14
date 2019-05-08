//#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "frizz_hal_if.h"
#include "frizz_sensor_list.c"
#include <errno.h>
#include <stdint.h>

void mdelay(int x) {
	int i = 0;
	while (x--) {
		for (i = 0; i < 40000; i++)
			;
	}
}

int main(int argc, char **argv) {
	int fd;
	int cmd;

	fd = open("/dev/frizz", O_RDWR);
	if (fd < 0) {
		perror("open error\n");
		return -1;
	}

	int sensor_id = 0;
	int test_loop = 0;
	int val = 0;
	int err = 0;
	sensor_info test_sensor_info = { 0 };
	sensor_enable_t sensor_enable = { 0 };
	sensor_data_t sensor_data = { 0 };
	sensor_delay_t sensor_delay = { 0 };
	sensor_data_changed_t change;
	firmware_version_t v;
	v.version = 0;

	while (1) {

		printf("=============================================================\n"
				"NOTE:\n"
				"1, download frizz firmware.(/data/from.bin)\n"
				"2, check frizz booted and send command, receive sensor data. (test mode)\n"
				"3, get sensor data. (get uint32_t data mode)\n"
				"4, get sensor data. (get float data mode)\n"
				"5, enable or disable sensor.\n"
				"6, get the changed bit\n"
				"7, get firmware_version\n"
				"8, quit this test program.\n"
			  "=============================================================\n"
			  "Please select:");
		scanf("%d%*c", &cmd);
		switch (cmd) {
		case 1:
			ioctl(fd, FRIZZ_IOCTL_HARDWARE_DOWNLOAD_FIRMWARE, "/data/from.bin");
			printf("done.\n");
			break;

		case 2:
			printf("please input the sensor_id and test_loop:");
			scanf("%x%d%*c", &sensor_id, &val);
			test_sensor_info.sensor_id = sensor_id;
			test_sensor_info.test_loop = 1;
			while (val--) {
				err = ioctl(fd, FRIZZ_IOCTL_FW_TEST, &test_sensor_info);
				if (err < 0) {
					printf("error !!!\n");
					break;
				}
				mdelay(100);
			}
			printf("done.\n");
			break;

		case 3:
		case 4:
			if (cmd == 3) {
				printf("please input the sensor_id and test_loop:(get uint32_t data mode)");
			} else {
				printf("please input the sensor_id and test_loop:(get float data mode)");
			}

			scanf("%x%d%*c", &sensor_id, &val);

			sensor_enable.code = convert_id_to_code(sensor_id);
			sensor_enable.flag = 1;
			sensor_data.code = convert_id_to_code(sensor_id);
			sensor_delay.code = convert_id_to_code(sensor_id);

			err = ioctl(fd, FRIZZ_IOCTL_SENSOR_SET_ENABLE, &sensor_enable);
			if (err < 0) {
				printf("enable 0x%x error !!!\n", sensor_id);
				break;
			}

			sensor_delay.ms = 500;
			err = ioctl(fd, FRIZZ_IOCTL_SENSOR_SET_DELAY, &sensor_delay);
			if (err < 0) {
				printf("delay error !!!\n");
				break;
			}

			while (val--) {
				err = ioctl(fd, FRIZZ_IOCTL_SENSOR_GET_DATA, &sensor_data);
				if (err < 0) {
					printf("get error !!!\n");
					break;
				}
				if (cmd == 3) {
					printf("get: time: %d, (uint32_t)data[0]: %d, data[1]: %d\n",
							sensor_data.time, sensor_data.u32_value[0],sensor_data.u32_value[1]);
				} else {
					printf("get: time: %d, (float)data[0]: %f, data[1]: %f\n",
							sensor_data.time, sensor_data.f32_value[0],sensor_data.f32_value[1]);
				}
				mdelay(500);
			}

			sensor_enable.flag = 0;
			err = ioctl(fd, FRIZZ_IOCTL_SENSOR_SET_ENABLE, &sensor_enable);
			if (err < 0) {
				printf("enable 0x%x error !!!\n", sensor_id);
				break;
			}
			printf("done.\n");
			break;

		case 5:
			printf("please input the sensor_id and enable or not(enable:1, disable:0):");
			scanf("%x%d%*c", &sensor_id, &val);
			sensor_enable.code = convert_id_to_code(sensor_id);
			sensor_enable.flag = val;
			err = ioctl(fd, FRIZZ_IOCTL_SENSOR_SET_ENABLE, &sensor_enable);
			if (err < 0) {
				printf("error !!!\n");
				break;
			}
			printf("done.\n");
			break;

		case 6:
			memset(&change, 0, sizeof(sensor_data_changed_t));

			err = ioctl(fd, FRIZZ_IOCTL_GET_SENSOR_DATA_CHANGED, &change);
			if (err < 0) {
				printf("error !!!\n");
				break;
			}
			printf("get change: aflag: 0x%x, mflag: 0x%x\n", change.aflag,
					change.mflag);
			break;

		case 7:
			ioctl(fd, FRIZZ_IOCTL_GET_FIRMWARE_VERSION, &v);
			printf("get firmware_version: %d\n", v.version);
			break;

		case 8:
			close(fd);
			printf("done.\n");
			return 0;
			break;
		default:
			perror("Cannot find the command\n");
			break;
		}
	}

	return 0;
}
