#ifndef _FAKERTPRECV_H
#define _FAKERTPRECV_H


#include "avrtp_def.h"



// Return values: 0, -1
int FakeRtpRecv_init(const T_RTPPARAM *param);

void FakeRtpRecv_uninit();

// Return values: 1, 0, -1
// Caller must free() pack
// Non-blocking
int FakeRtpRecv_getpacket(RECV_PACKET_SMALL **pack, BYTE bPayloadType);



#endif /* _FAKERTPRECV_H */
