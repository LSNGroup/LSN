#pragma once

//
// 轻量级加解密算法，就在输入缓冲区原地处理。
//


void set_encdec_key(unsigned char *key, int len);
void encrypt_data(unsigned char *data, int len);
void decrypt_data(unsigned char *data, int len);
