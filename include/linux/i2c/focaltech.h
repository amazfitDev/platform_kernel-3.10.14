#ifndef __LINUX_FTS_TS_H__
#define __LINUX_FTS_TS_H__


#define SCREEN_MAX_X						320
#define SCREEN_MAX_Y						480

#define FTS_REG_FW_VER						0xA6
#define PRESS_MAX							255
#define FTS_REG_POINT_RATE					0x88

#define FOCALTECH_TS_NAME					"focaltech_ts"
#define FOCALTECH_TS_ADDR					0x38

/*register address*/
#define FT_REG_CHIP_ID						0xA3 //chip ID
#define FT_REG_FW_VER						0xA6 //FW  version
#define FT_REG_VENDOR_ID					0xA8 // TP vendor ID
#define TPD_MAX_POINTS_2					2
#define TPD_MAX_POINTS_5					5
#define TPD_MAXPOINTS_10					10
#define AUTO_CLB_NEED						1
#define AUTO_CLB_NONEED						0

struct Upgrade_Info {
	u8  CHIP_ID;
	u8  FTS_NAME[20];
	u8  TPD_MAX_POINTS;
	u8  AUTO_CLB;
	u16 delay_aa;			/*delay of write FT_UPGRADE_AA */
	u16 delay_55;			/*delay of write FT_UPGRADE_55 */
	u8  upgrade_id_1;		/*upgrade id 1 */
	u8  upgrade_id_2;		/*upgrade id 2 */
	u16 delay_readid;		/*delay of read id */
	u16 delay_earse_flash;	/*delay of earse flash*/
};

struct fts_platform_data {
	const char *vdd_name;
	int irq_gpio_number;
	int reset_gpio_number;
	int virtualkeys[12];
	int TP_MAX_X;
	int TP_MAX_Y;
};

extern struct Upgrade_Info fts_updateinfo_curr;
//int FTS_i2c_Read(struct i2c_client *client, char *writebuf, int writelen,char *readbuf, int readlen);
//int FTS_i2c_Write(struct i2c_client *client, char *writebuf, int writelen);

enum fts_regs {
	FTS_REG_THGROUP					= 0x80,
	FTS_REG_THPEAK					= 0x81,
	FTS_REG_THCAL					= 0x82,
	FTS_REG_THWATER					= 0x83,
	FTS_REG_THTEMP					= 0x84,
	FTS_REG_THDIFF					= 0x85,
	FTS_REG_CTRL					= 0x86,
	FTS_REG_TIMEENTERMONITOR		= 0x87,
	FTS_REG_PERIODACTIVE			= 0x88,
	FTS_REG_PERIODMONITOR			= 0x89,
	FTS_REG_HEIGHT_B				= 0x8a,
	FTS_REG_MAX_FRAME				= 0x8b,
	FTS_REG_DIST_MOVE				= 0x8c,
	FTS_REG_DIST_POINT				= 0x8d,
	FTS_REG_FEG_FRAME				= 0x8e,
	FTS_REG_SINGLE_CLICK_OFFSET		= 0x8f,
	FTS_REG_DOUBLE_CLICK_TIME_MIN	= 0x90,
	FTS_REG_SINGLE_CLICK_TIME		= 0x91,
	FTS_REG_LEFT_RIGHT_OFFSET		= 0x92,
	FTS_REG_UP_DOWN_OFFSET			= 0x93,
	FTS_REG_DISTANCE_LEFT_RIGHT		= 0x94,
	FTS_REG_DISTANCE_UP_DOWN		= 0x95,
	FTS_REG_ZOOM_DIS_SQR			= 0x96,
	FTS_REG_RADIAN_VALUE			=0x97,
	FTS_REG_MAX_X_HIGH				= 0x98,
	FTS_REG_MAX_X_LOW				= 0x99,
	FTS_REG_MAX_Y_HIGH				= 0x9a,
	FTS_REG_MAX_Y_LOW				= 0x9b,
	FTS_REG_K_X_HIGH				= 0x9c,
	FTS_REG_K_X_LOW					= 0x9d,
	FTS_REG_K_Y_HIGH				= 0x9e,
	FTS_REG_K_Y_LOW					= 0x9f,
	FTS_REG_AUTO_CLB_MODE			= 0xa0,
	FTS_REG_LIB_VERSION_H			= 0xa1,
	FTS_REG_LIB_VERSION_L			= 0xa2,
	FTS_REG_CIPHER					= 0xa3,
	FTS_REG_MODE					= 0xa4,
	FTS_REG_PMODE					= 0xa5, /* Power Consume Mode */
	FTS_REG_FIRMID					= 0xa6,
	FTS_REG_STATE					= 0xa7,
	FTS_REG_FT5201ID				= 0xa8,
	FTS_REG_ERR						= 0xa9,
	FTS_REG_CLB						= 0xaa,
};

//FTS_REG_PMODE
#define PMODE_ACTIVE				0x00
#define PMODE_MONITOR				0x01
#define PMODE_STANDBY				0x02
#define PMODE_HIBERNATE				0x03

#define FTS_TS_DBG
#ifdef FTS_TS_DBG
#define DBG(fmt, args...) 			printk("[FTS]" fmt, ## args)
#else
#define DBG(fmt, args...) 			do{}while(0)
#endif

#ifndef ABS_MT_TOUCH_MAJOR
#define ABS_MT_TOUCH_MAJOR	0x30	/* touching ellipse */
#define ABS_MT_TOUCH_MINOR	0x31	/* (omit if circular) */
#define ABS_MT_WIDTH_MAJOR	0x32	/* approaching ellipse */
#define ABS_MT_WIDTH_MINOR	0x33	/* (omit if circular) */
#define ABS_MT_ORIENTATION	0x34	/* Ellipse orientation */
#define ABS_MT_POSITION_X	0x35	/* Center X ellipse position */
#define ABS_MT_POSITION_Y	0x36	/* Center Y ellipse position */
#define ABS_MT_TOOL_TYPE	0x37	/* Type of touching device */
#define ABS_MT_BLOB_ID		0x38	/* Group set of pkts as blob */
#define ABS_MT_PRESSURE		0x3a	/* Pressure on contact area */
#define ABS_MT_TRACKING_ID      0x39	/* Unique ID of initiated contact */
#endif /* ABS_MT_TOUCH_MAJOR */

#endif
