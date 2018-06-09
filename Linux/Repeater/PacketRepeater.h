#ifndef _PACKET_REPEATER_H
#define _PACKET_REPEATER_H


#include "Shiyong.h"
#include "IpcGuaji.h"


BYTE CheckNALUType(BYTE bNALUHdr);
BYTE CheckNALUNri(BYTE bNALUHdr);

void fake_rtp_recv_fn(void *ctx, int payload_type, unsigned long rtptimestamp, unsigned char *data, int len);


#endif //_PACKET_REPEATER_H
