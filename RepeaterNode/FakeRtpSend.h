#ifndef _FAKERTPSEND_H
#define _FAKERTPSEND_H


#include "avrtp_def.h"


#define FAKERTP_RELIABLE_FLAG  0x80
#define FAKERTP_RELIABLE_MASK  0x7f


typedef struct _tag_fakertpsend {
	BOOL bInit;
	UDPSOCKET udpSock;
	SOCKET fhandle;
	SOCKET_TYPE ftype;
	DWORD dstip;
	WORD  dstport;
	WORD arraySequenceNumber[NUM_PAYLOAD_TYPE];
	CRITICAL_SECTION m_mutexRtpSend;//pthread_mutex_t m_mutexRtpSend;
} FAKERTPSEND;





// Return values: 0, -1
int FakeRtpSend_init(FAKERTPSEND **ppFRS, const T_RTPPARAM *param);

void FakeRtpSend_uninit(FAKERTPSEND *pFRS);

// Return values: 0, -1
int FakeRtpSend_sendpacket(FAKERTPSEND *pFRS, unsigned long rtptimestamp, const void *data, size_t len, BYTE bPayloadType, BYTE bReserved, BYTE bNri);



#endif /* _FAKERTPSEND_H */
