#ifndef __FRIZZ_MOTION_CODE_REMAP_H_
#define __FRIZZ_MOTION_CODE_REMAP_H_
#include "sensors.h"

enum report_code {
    MOTION_RESET = 0,
    MOTION_STOP = 1,
    MOTION_WALK = 2,
    MOTION_RUN = 3,
    MOTION_SLEEP = 4,
    MOTION_DEEP_SLEEP = 5,
};

enum Activity_code {
	ACTIVITY_DEEP_SLEEP = 0,
	ACTIVITY_SLEEP,
	ACTIVITY_RESET,
	ACTIVITY_WALK,
	ACTIVITY_RUN,
};

enum ISP_code {
	ISP_STOP = 0,
	ISP_RESET,
	ISP_WALK,
	ISP_RUN,
};

struct
{
	int sensor_typ;
	int code_origin;
	int code_remap;
} motion_code_remap[] = {
	{SENSOR_TYPE_ACTIVITY, ACTIVITY_DEEP_SLEEP, MOTION_DEEP_SLEEP},
	{SENSOR_TYPE_ACTIVITY, ACTIVITY_SLEEP,      MOTION_SLEEP},
	{SENSOR_TYPE_ACTIVITY, ACTIVITY_RESET,      MOTION_RESET},
	{SENSOR_TYPE_ACTIVITY, ACTIVITY_WALK,       MOTION_WALK},
	{SENSOR_TYPE_ACTIVITY, ACTIVITY_RUN,        MOTION_RUN},

	{SENSOR_TYPE_ISP,      ISP_STOP,            MOTION_STOP},
	{SENSOR_TYPE_ISP,      ISP_RESET,           MOTION_RESET},
	{SENSOR_TYPE_ISP,      ISP_WALK,            MOTION_WALK},
	{SENSOR_TYPE_ISP,      ISP_RUN,             MOTION_RUN},
};

#define MAX_MOTION_CODE_COUNT (sizeof motion_code_remap / sizeof motion_code_remap[0])

#endif
