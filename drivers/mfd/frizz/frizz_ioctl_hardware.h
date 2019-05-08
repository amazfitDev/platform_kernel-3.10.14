#ifndef __FRIZZ_IOCTL_HARDWARE_H__
#define __FRIZZ_IOCTL_HARDWARE_H__
int frizz_ioctl_hardware_reset(void);
int frizz_ioctl_hardware_download_firmware(char *name);
int frizz_download_firmware(unsigned int g_chip_orientation);
#endif
