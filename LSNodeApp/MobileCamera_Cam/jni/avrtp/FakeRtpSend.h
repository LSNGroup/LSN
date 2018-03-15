#ifndef _FAKERTPSEND_H
#define _FAKERTPSEND_H


#include "avrtp_def.h"


#define FAKERTP_RELIABLE_FLAG  0x80
#define FAKERTP_RELIABLE_MASK  0x7f



// Return values: 0, -1
int FakeRtpSend_init(const T_RTPPARAM *param);

void FakeRtpSend_uninit();

// Return values: 0, -1
int FakeRtpSend_sendpacket(DWORD rtptimestamp, const void *data, size_t len, BYTE bPayloadType, BYTE bReserved, BYTE bNri);



#endif /* _FAKERTPSEND_H */
