#ifndef __SEC_TEST_H
#define __SEC_TEST_H
#define DRV_NAME "/dev/jz-security"

#define SECURITY_INTERNAL_CHANGE_KEY  (0xffff0010)
#define SECURITY_INTERNAL_AES         (0xffff0020)
#define SECURITY_RSA                  (0xffff0030)

struct rsa_aes_packet {
	unsigned short oklen; //old security key len in words
	unsigned short nklen;// new security key len in words
	unsigned	int * okey;// old security key
	unsigned int * nkey;// new security key
};

struct change_key_param {
	int len;			//rsa_enc_data len in bytes.
	int *rsa_enc_data;  //okey_len,nkey_len,okey,nkey
	int *n_ku_kr;
	int init_mode;//init key 1;init,0:not init
};

struct aes_param {
	unsigned int crypt;
	unsigned int *input;
	unsigned int *output;
	unsigned int *iv;
	unsigned int tc;
	unsigned int len; /*word len*/
};

struct rsa_param {
	unsigned int in_len;
	unsigned int key_len;
	unsigned int n_len;
	unsigned int out_len;

	unsigned int *input;
	unsigned int *key;      /*KU*/
	unsigned int *n;        /*N*/
	unsigned int *output;
	unsigned int mode;
};

#endif /* MAIN_H */
