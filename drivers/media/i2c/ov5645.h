#ifndef _OV5645_H_
#define _OV5645_H_
#ifdef CONFIG_MIPI_CAMERA_BYPASS_MODE
int ov5645_read(struct i2c_client *client, unsigned short reg, unsigned char *value);
#else
int ov5645_read(struct v4l2_subdev *sd, unsigned short reg, unsigned char *value);
#endif
#endif
