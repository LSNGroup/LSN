#pragma once

//
// �������ӽ����㷨���������뻺����ԭ�ش���
//


void set_encdec_key(unsigned char *key, int len);
void encrypt_data(unsigned char *data, int len);
void decrypt_data(unsigned char *data, int len);
