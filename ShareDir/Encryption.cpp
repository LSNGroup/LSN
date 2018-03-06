#include "stdafx.h"
#include "Encryption.h"


static unsigned char the_key = 0; /* 8位全零，异或都不翻转 */

void set_encdec_key(unsigned char *key, int len)
{
	int i;
	unsigned short temp = 0;

	for (i = 0; i < len; i++) {
		temp += key[i];
	}

	the_key =(unsigned char)(temp & 0x00ff) | (unsigned char)((temp & 0xff00) >> 8);

	if (0 == the_key) {
		the_key = 0xff;
	}
}

void encrypt_data(unsigned char *data, int len)
{
	int i;
	unsigned char tmp1, tmp2;

	/* 对每个字节，先调换0,1位和6,7位的位置，然后整个字节与the_key按位异或。*/
	for(i = 0; i < len; i++) {
		tmp1 = data[i] & 0x03;
		tmp2 = data[i] & 0xC0;
		data[i] = (data[i] & 0x3C) | (tmp1 << 6) | (tmp2 >> 6);
		data[i] = data[i] ^ the_key;
	}
}

void decrypt_data(unsigned char *data, int len)
{
	int i;
	unsigned char tmp1, tmp2;

	/* 对每个字节，先与the_key按位异或，然后调换0,1位和6,7位的位置。*/
	for(i = 0; i < len; i++) {
		data[i] = data[i] ^ the_key;
		tmp1 = data[i] & 0x03;
		tmp2 = data[i] & 0xC0;
		data[i] = (data[i] & 0x3C) | (tmp1 << 6) | (tmp2 >> 6);
	}
}

