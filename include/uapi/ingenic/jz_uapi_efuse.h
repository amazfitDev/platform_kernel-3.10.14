#ifndef __JZ_UAPI_EFUSE_H__
#define __JZ_UAPI_EFUSE_H__

struct efuse_wr_info {
	uint32_t seg_addr;
	uint32_t data_length;
	void __user *buf;
};

#define EFUSE_CMD_READ	_IOWR('k', 51, struct efuse_wr_info *)
#define EFUSE_CMD_WRITE	_IOWR('k', 52, struct efuse_wr_info *)

#endif /*__JZ_UAPI_EFUSE_H__*/
