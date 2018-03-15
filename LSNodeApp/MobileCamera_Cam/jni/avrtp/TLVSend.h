#ifndef _TLVSEND_H_
#define _TLVSEND_H_

#include "avrtp_def.h"
#include "FakeRtpSend.h"


void TLVSendStart(int nBasePort, DWORD dwDestIp/* Network byte order */, int nDestPort, UDPSOCKET udpSock, SOCKET fhandle, SOCKET_TYPE type);

void TLVSendStop(void);

void TLVSendSetRedundance(BOOL bValue);



void TLVSendUpdateValue(WORD wType, double dfValue);

//
// Return values:
// -1: Error
//  0: OK
//
int TLVSendData(void);


#endif /* _TLVSEND_H_ */
