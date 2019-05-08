/*
 * sec_test.c - Ingenic security driver test app
 *
 * Copyright (C) 2015 Ingenic Semiconductor Co., Ltd.
 * Author: liu yang <king.lyang@ingenic.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>
#include "sec_test.h"


/*#define DEBUG*/
unsigned int aes_key[4] = {
0x2b7e1516, 0x28aed2a6, 0xabf71588,0x09cf4f3c,
};

//private rsa key
unsigned int pri[31] = {
	0xceafa9ff,0x141ff8ce,0x60561ce5,0x0aae64c1,
	0xf4764119,0xd24d7a92,0xf70410e0,0x82353580,
	0x445b51ff,0xf1d9122a,0xdf93b265,0x72e0af52,
	0xc1ee840f,0x7b394271,0xcdc887d1,0x1c607a70,
	0x9788522d,0x2e86b6dd,0x18368422,0xc3fa9bf2,
	0x3bdb00e0,0xbf3b2318,0x8b909508,0x3e183c6d,
	0x0c8e6da9,0x867a60d6,0x3b03dbf7,0xdd3fbebb,
	0xdc242725,0x47ddadf7,0x00055a22
};
//public rsa key
unsigned int pub[31] = {
	0xa38c4687,0x2f50aba9,0x3f965db7,0x26dbaa19,
	0xf09100dc,0x375ffb69,0xe9e04423,0xc3b195c7,
	0xa3a1219c,0xef52a678,0xeab83c63,0x8abe57ac,
	0xc5c35078,0x00715d34,0x991eb92e,0x385358dc,
	0x11095124,0x48f91362,0x70d55294,0x127efd5a,
	0x2dae840b,0xf21b265f,0xabb2c475,0x0b3cfd76,
	0xaa9ddcb0,0x7acdc068,0x6f4462b4,0x4401a4ba,
	0xca41132f,0xc5186338,0x003bf8c4
};
//n
unsigned int nnn[31] = {
	0x7a76f999,0xe7735950,0x637a3bad,0x847c7f7c,
	0x1d6b366e,0xffa6c061,0x50be0662,0xa36c6407,
	0x4b7b7a40,0x6427e05f,0x5626f5d4,0x251e034c,
	0x00e5886a,0xd5a4354c,0x8231d1f4,0x2d22e366,
	0x89378d0c,0x7a468f7a,0x92d5957b,0xb52ffd79,
	0x70196a83,0x07bdfb37,0x7532ecc7,0xb198e640,
	0x1bf74468,0x2a5b065e,0xa6a7d107,0x2b04ea51,
	0x892d04cd,0x41415b3b,0x006f0ec4
};
unsigned int nku[62] = {
	//n
	0x7a76f999,0xe7735950,0x637a3bad,0x847c7f7c,
	0x1d6b366e,0xffa6c061,0x50be0662,0xa36c6407,
	0x4b7b7a40,0x6427e05f,0x5626f5d4,0x251e034c,
	0x00e5886a,0xd5a4354c,0x8231d1f4,0x2d22e366,
	0x89378d0c,0x7a468f7a,0x92d5957b,0xb52ffd79,
	0x70196a83,0x07bdfb37,0x7532ecc7,0xb198e640,
	0x1bf74468,0x2a5b065e,0xa6a7d107,0x2b04ea51,
	0x892d04cd,0x41415b3b,0x006f0ec4,
	//pub
	0xa38c4687,0x2f50aba9,0x3f965db7,0x26dbaa19,
	0xf09100dc,0x375ffb69,0xe9e04423,0xc3b195c7,
	0xa3a1219c,0xef52a678,0xeab83c63,0x8abe57ac,
	0xc5c35078,0x00715d34,0x991eb92e,0x385358dc,
	0x11095124,0x48f91362,0x70d55294,0x127efd5a,
	0x2dae840b,0xf21b265f,0xabb2c475,0x0b3cfd76,
	0xaa9ddcb0,0x7acdc068,0x6f4462b4,0x4401a4ba,
	0xca41132f,0xc5186338,0x003bf8c4,
};
unsigned int nkr[62] = {
	//n
	0x7a76f999,0xe7735950,0x637a3bad,0x847c7f7c,
	0x1d6b366e,0xffa6c061,0x50be0662,0xa36c6407,
	0x4b7b7a40,0x6427e05f,0x5626f5d4,0x251e034c,
	0x00e5886a,0xd5a4354c,0x8231d1f4,0x2d22e366,
	0x89378d0c,0x7a468f7a,0x92d5957b,0xb52ffd79,
	0x70196a83,0x07bdfb37,0x7532ecc7,0xb198e640,
	0x1bf74468,0x2a5b065e,0xa6a7d107,0x2b04ea51,
	0x892d04cd,0x41415b3b,0x006f0ec4,
	//pri
	0xceafa9ff,0x141ff8ce,0x60561ce5,0x0aae64c1,
	0xf4764119,0xd24d7a92,0xf70410e0,0x82353580,
	0x445b51ff,0xf1d9122a,0xdf93b265,0x72e0af52,
	0xc1ee840f,0x7b394271,0xcdc887d1,0x1c607a70,
	0x9788522d,0x2e86b6dd,0x18368422,0xc3fa9bf2,
	0x3bdb00e0,0xbf3b2318,0x8b909508,0x3e183c6d,
	0x0c8e6da9,0x867a60d6,0x3b03dbf7,0xdd3fbebb,
	0xdc242725,0x47ddadf7,0x00055a22,
};
/**/
unsigned int user_key[9] =
{
	0x40004, /* bit[31:16] = oldkey_len,bit[15:0] = newkey_len */
	0x2b7e1516, 0x28aed2a6, 0xabf71588,0x09cf4f3c,/*old_key*/
	0x2b7e1516, 0x28aed2a6, 0xabf71588,0x09cf4f3c,/*new_key*/
};

unsigned int user_key2[9] =
{	0x40004,
	0x2b7e1516, 0x28aed2a6, 0xabf71588,0x09cf4f3c,/*old_key*/
	0x1,0x2,0x3,0x4,
};


unsigned int in_tmp[4] = {0x3243f6a8, 0x885a308d, 0x313198a2, 0xe0370734,};
unsigned int out_tmp[4] = {0};
unsigned int expect_output[4] ={0x3925841d, 0x02dc09fb, 0xdc118597, 0x196a0b32,};
unsigned int tmp_out_key[31] = {0};

unsigned int aes_key_out[31] = {0};
int fd=0;

int setup_aes_key(int fd, unsigned int *key, int len, unsigned int *nku_kr, int init_mode)
{
	int ret = 0;
	struct change_key_param key_param;
	key_param.len = len;
	key_param.rsa_enc_data = key;
	key_param.n_ku_kr = nku_kr;
	key_param.init_mode = init_mode;

	ret = ioctl(fd, SECURITY_INTERNAL_CHANGE_KEY, &key_param);
	if(ret < 0) {
		printf("ERROR! in ioctl SECURITY_INTERNAL_CHANGE_KEY, ret = %d\n",ret);
		return -1;
	}

	return 0;
}

int do_aes(int fd, unsigned int *input, unsigned int in_len, unsigned int *output, unsigned int mode)
{
	int ret  = 0;

	struct aes_param aes;
	aes.crypt = mode;
	aes.len = in_len;
	aes.input = input;
	aes.output = output;
	aes.iv = NULL;
	aes.tc = aes.len/4;
#ifdef DEBUG
	printf("aes.crypt		= %d\n",aes.crypt);
	printf("aes.in_len		= %d\n",aes.len);
	printf("aes.input		= %x\n",aes.input);
	printf("aes.output		= %x\n",aes.output);
	printf("aes.iv			= %x\n",aes.iv);
	printf("aes.tc			= %x\n",aes.tc);
#endif
	ret = ioctl(fd, SECURITY_INTERNAL_AES, &aes);
	if (ret < 0) {
		printf("ERROR! in ioctl SECURITY_INTERNAL_AES, ret = %d\n",ret);
		return -1;
	}
	return 0;
}
int do_rsa(unsigned int fd, unsigned int orig_aes_len, unsigned int * rsa_key, unsigned int * n, unsigned int * input, unsigned int *output,unsigned int mode)
{
	int ret = 0;
	struct rsa_param rsa_p;
	if(mode){
		rsa_p.in_len = orig_aes_len;
		rsa_p.out_len = 31;
	} else {
		rsa_p.in_len = 31;
		rsa_p.out_len = orig_aes_len;
	}
	rsa_p.key_len = 31;
	rsa_p.n_len = 31;
	/*rsa_p.out_len = 31;*/

	rsa_p.input = input;
	rsa_p.key = rsa_key;
	rsa_p.n = n;
	rsa_p.output = output;
	rsa_p.mode = mode;
#ifdef DEBUG
		printf("ly-test ---> rsa_p.input_len = %d\n",rsa_p.in_len);
		printf("ly-test ---> rsa_p.key_len	= %d\n",rsa_p.key_len);
		printf("ly-test ---> rsa_p.n_len	= %d\n",rsa_p.n_len);
		printf("ly-test ---> rsa_p.out_len	= %d\n",rsa_p.out_len);
		printf("ly-test ---> rsa_p.input	= %x\n",(unsigned int)rsa_p.input);
		printf("ly-test ---> rsa_p.key		= %x\n",(unsigned int)rsa_p.key);
		printf("ly-test ---> rsa_p.n		= %x\n",(unsigned int)rsa_p.n);
		printf("ly-test ---> rsa_p.output	= %x\n",(unsigned int)rsa_p.output);
#endif
	ret = ioctl(fd, SECURITY_RSA, &rsa_p);//private key encode
	if(ret < 0) {
		printf("ERROR! in ioctl SECURITY_RSA, ret = %d\n",ret);
		return -1;
	}
	return 0;
}

int cmp_data(unsigned int *src, unsigned int *dst, unsigned int len)
{
	unsigned int *start = src;
	unsigned int *end_src = src + len;
	while(start < end_src)
	{
		if(*start++ != *dst++) {
			printf("cmp data error: src:%08x, dst:%08x\n", *(start-1), *(dst-1));
			return (start - src);
		}
	}
	return 0;
}


int rsa_encypt_aes_key()
{
	int ret = 0;
	/*encrypt aes_key*/
	int len =sizeof(user_key)/sizeof(int);
	ret = do_rsa(fd, len, pub, nnn, user_key, tmp_out_key, 1);/*1:encrypt 0:decrypt*/
	if(ret != 0) {
		printf("error!\n");
		return -1;
	}

#ifdef DEBUG
	/*decrypt aes_key for debug*/
	unsigned int tmp_encrypt_key[31];
	int i;
	for(i=0;i<31;i++)
	{
		tmp_encrypt_key[i] = tmp_out_key[i];
	}
	ret = do_rsa(fd, len, pri, nnn, tmp_encrypt_key, aes_key_out,0);
	if(ret != 0) {
		printf("error!\n");
		return -1;
	}

	if(cmp_data(aes_key_out,user_key,len)!=0){
		printf("rsa_encrypt_aes_key error!\n");
		return -1;
	}else {
		printf("rsa_encrypt_aes_key ok!\n");
	}

#endif

	printf("rsa_encrypt_aes_key ok!\n");
	return 0;

}

int change_aes_key()
{
	int ret = 0;
	int aes_key_len = sizeof(tmp_out_key)/sizeof(int);
	ret = setup_aes_key(fd, tmp_out_key, aes_key_len, nkr,  1);/*1: init 0: change */
	if(ret != 0){
		printf("setup_aes_key error!\n");
		return -1;
	}

#ifdef DEBUG//aes_key [change aes_key]

	unsigned int orig_aes_len = sizeof(user_key2)/sizeof(int);
	ret = do_rsa(fd, orig_aes_len, pub, nnn, &user_key2, tmp_out_key,1);/*1:encrypt 0:decrypt*/
	if(ret != 0) {
		printf("error!\n");
		return -1;
	}

	ret = setup_aes_key(fd, tmp_out_key, aes_key_len, nkr, 0);/*1: init 0: change */
	if(ret != 0){
		printf("error!\n");
		return -1;
	}
#endif
	printf("change_aes_key ok!\n");
	return 0;
}

int aes_encrypt_decrypt()
{
	int data_len = sizeof(in_tmp)/sizeof(int);
	int ret =0;

/*start encrypt data*/
	ret = do_aes(fd, in_tmp, data_len, out_tmp, 0); /* 0:aes encode 1:aes decode*/
	if(ret != 0) {
		printf("error!\n");
		return -1;
	}
#ifdef DEBUG
	int i = 0;
	for(i=0; i < data_len; i++) {
		printf("----------------> in_tmp[%d] = 0x%x\n",i,in_tmp[i]);
	}
	for(i=0; i < data_len; i++) {
		printf("-----encode ------> out_tmp[%d] = 0x%x\n",i,out_tmp[i]);
	}
	for(i=0; i < data_len; i++) {
		printf("-----encode ------> expect_output[%d] = 0x%x\n",i,expect_output[i]);
	}
#endif
	ret = cmp_data(out_tmp,expect_output,data_len);
	if(ret != 0){
		printf("aes_encode error!\n");
		return -1;
	} else {
		printf("aes_encrypt ok!\n");
	}

/*end encrypt data*/

/*start decrypt data*/
	ret = do_aes(fd, expect_output, data_len, out_tmp, 1); /* 0:aes encode 1:aes decode*/
	if(ret != 0) {
		printf("error!\n");
		return -1;
	}
#ifdef DEBUG
	for(i=0; i < data_len; i++) {
		printf("-------decode------> out_tmp[%d] = 0x%x\n",i,out_tmp[i]);
	}
	for(i=0; i < data_len; i++) {
		printf("-------decode------> expect output[%d] = 0x%x\n",i,in_tmp[i]);
	}
#endif
	ret = cmp_data(out_tmp,in_tmp,data_len);
	if(ret != 0){
		printf("aes decrypt error!\n");
		return -1;
	}else {
		printf("aes_decrypt ok!\n");
	}

/*end decrypt data*/
	return 0;
}

int main()
{
/*
usage:
	   init...
	   1. user set AES_KEY.
	   2. user Encrypt AES_KEY by RSA Algorithm.
	   3. setup encrypted AES_KEY to cpu.
	   3. user Encrypt or DECrypt input data by AES Algorithm.

	   change key...
	   1. user set new AES_KEY.
	   2. user Encrypt AES_KEY by RSA Algorithm.[copy to user].
	   3. setup encrypted AES_KEY to cpu.
*/
	int ret = 0;
	fd = open(DRV_NAME,O_RDWR);
	if(fd < 0) {
		printf("error! Can't open jz_security node, err = %d \n", -errno);
		return -errno;
	}

	ret = rsa_encypt_aes_key();
	if(ret != 0){
		printf("error! in rsa_encrypt_aes_key.[%s,%d]\n",__func__,__LINE__);
	}

	ret=change_aes_key();
	if(ret != 0){
		printf("error! in change aes_key.[%s,%d]\n",__func__,__LINE__);
	}

	ret = aes_encrypt_decrypt();
	if(ret != 0){
		printf("error! in aes_encrypt.[%s,%d]\n",__func__,__LINE__);
	}

	close(fd);
	fd=-1;
	return 0;
}
