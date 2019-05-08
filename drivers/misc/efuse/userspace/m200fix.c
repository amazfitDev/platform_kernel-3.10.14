#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "m200fix.h"

int main(void)
{
	struct efuse_wr_info arg;
	int fd_efuse;
	int ret = 0, i;
	unsigned int *fixcode_rd;
	unsigned int *trim_buf;
	unsigned int *trim_buf_x;
	int ok_cnt = 0, error = 0;

	printf("sizeof(fixcode) = %ld\n", sizeof(fixcode));
	fixcode_rd = (unsigned int *)malloc(sizeof(fixcode));
	trim_buf = (unsigned int *)malloc(sizeof(trim0));
	trim_buf_x = (unsigned int *)malloc(2*sizeof(trim2));

	fd_efuse = open(EFUSE_PATH , O_RDWR);
	if (fd_efuse < 0) {
		printf("Efuse error no %s device\n", EFUSE_PATH);
		ret = -ENODEV;
		return -1;
	}

	arg.seg_addr = FIXCODE_START_ADDR;
	arg.data_length = sizeof(fixcode);
	arg.buf = (void *)fixcode;
	ret = ioctl(fd_efuse, EFUSE_CMD_WRITE, &arg);
	if (ret < 0) {
		free(fixcode_rd);
		close(fd_efuse);
		printf("write efuse error %s\n", strerror(errno));
		return -1;
	}

	debug("read from efuse write :\n");
	arg.seg_addr = FIXCODE_START_ADDR;
	arg.data_length = sizeof(fixcode);
	arg.buf = (void *)fixcode_rd;
	ret = ioctl(fd_efuse, EFUSE_CMD_READ, &arg);
	if (ret) {
		printf("write read error %s\n", strerror(errno));
		free(fixcode_rd);
		close(fd_efuse);
		return -1;
	}

	debug("-------------------------\n");
	for (i = 0; i < sizeof(fixcode)/sizeof(unsigned int); i++) {
		if (fixcode[i] != fixcode_rd[i]) {
			printf("efuse write error : i = %d\n", i);
			printf("fixcode_rd[i] = %x", fixcode_rd[i]);
			printf("fixcode[i]   = %x\n", fixcode[i]);
		}
	}
	debug("-------------------------\n");
#if 0
	arg.seg_addr = TRIM0_START_ADDR;
	arg.data_length = sizeof(trim0);
	arg.buf = (void *)trim0;
	ret = ioctl(fd_efuse, EFUSE_CMD_WRITE, &arg);
	if (ret < 0) {
		free(fixcode_rd);
		close(fd_efuse);
		printf("write efuse error %s\n", strerror(errno));
		return -1;
	}
	debug("read from efuse write :\n");
	arg.seg_addr = TRIM0_START_ADDR;
	arg.data_length = sizeof(trim0);
	arg.buf = (void *)trim_buf;
	ret = ioctl(fd_efuse, EFUSE_CMD_READ, &arg);
	if (ret) {
		printf("write read error %s\n", strerror(errno));
		free(fixcode_rd);
		close(fd_efuse);
		return -1;
	}
	printf("trim0_buf %x\n", trim_buf[0]);

	arg.seg_addr = TRIM1_START_ADDR;
	arg.data_length = sizeof(trim1);
	arg.buf = (void *)trim1;
	ret = ioctl(fd_efuse, EFUSE_CMD_WRITE, &arg);
	if (ret < 0) {
		free(fixcode_rd);
		close(fd_efuse);
		printf("write efuse error %s\n", strerror(errno));
		return -1;
	}
	debug("read from efuse write :\n");
	arg.seg_addr = TRIM1_START_ADDR;
	arg.data_length = sizeof(trim1);
	arg.buf = (void *)trim_buf;
	ret = ioctl(fd_efuse, EFUSE_CMD_READ, &arg);
	if (ret) {
		printf("write read error %s\n", strerror(errno));
		free(fixcode_rd);
		close(fd_efuse);
		return -1;
	}
	printf("trim1_buf %x\n", trim_buf[0]);

	arg.seg_addr = TRIM2_START_ADDR;
	arg.data_length = sizeof(trim2);
	arg.buf = (void *)trim2;
	ret = ioctl(fd_efuse, EFUSE_CMD_WRITE, &arg);
	if (ret < 0) {
		free(fixcode_rd);
		close(fd_efuse);
		printf("write efuse error %s\n", strerror(errno));
		return -1;
	}
	arg.seg_addr = TRIM2_START_ADDR;
	arg.data_length = sizeof(trim2);
	arg.buf = (void *)trim_buf;
	ret = ioctl(fd_efuse, EFUSE_CMD_READ, &arg);
	if (ret) {
		printf("write read error %s\n", strerror(errno));
		free(fixcode_rd);
		close(fd_efuse);
		return -1;
	}
	printf("trim2_buf %x\n", trim_buf[0]);


	debug("read from efuse write :\n");
	arg.seg_addr = 0x232;
	arg.data_length = 2 * sizeof(trim0);
	arg.buf = (void *)trim_buf_x;
	ret = ioctl(fd_efuse, EFUSE_CMD_READ, &arg);
	if (ret) {
		printf("write read error %s\n", strerror(errno));
		free(fixcode_rd);
		close(fd_efuse);
		return -1;
	}
	printf("trim_buf0 0x%08x\n", trim_buf_x[0]);
	printf("trim_buf1 0x%08x\n", trim_buf_x[1]);
#endif
	free(fixcode_rd);
	close(fd_efuse);
	return 0;
}
