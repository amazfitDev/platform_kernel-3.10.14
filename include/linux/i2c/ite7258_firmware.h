#ifndef __ITE7258_FIRMWARE_H__
#define __ITE7258_FIRMWARE_H__

struct ite7258_update_data{
        struct i2c_client *client;
        unsigned int algo_firmware_length;
        unsigned int configurate_firmware_length;
        char *algo_firmware_data;
        char *configurate_firmware_data;
};
#endif
