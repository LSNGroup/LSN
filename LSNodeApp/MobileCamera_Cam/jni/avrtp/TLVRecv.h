#ifndef _TLVRECV_H
#define _TLVRECV_H

#include "avrtp_def.h"
#include "FakeRtpRecv.h"




void TLVRecvStart(int nBasePort, DWORD dwDestIp/* Network byte order */, int nDestPort, UDPSOCKET udpSock, SOCKET fhandle, SOCKET_TYPE type);

void TLVRecvStop(void);

void TLVRecvSetPeriod(int miniSec);

double TLVRecvGetData(WORD wType);



#endif /* _TLVRECV_H */
