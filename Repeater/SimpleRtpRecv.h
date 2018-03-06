#pragma once

#include "avrtp_def.h"

//data是包含8字节头部的数据。。。
typedef void (*FAKERTPRECV_RECV_FN)(void *ctx, int payload_type, unsigned long rtptimestamp, unsigned char *data, int len);

typedef struct _tag_fakertprecv {
	BOOL bInit;
	UDPSOCKET udpSock;
	SOCKET fhandle;
	SOCKET_TYPE ftype;
	BOOL bQuitUdtRecv;
	FAKERTPRECV_RECV_FN recvFn;
	unsigned long m_audioTimeStamp;
	unsigned long m_videoTimeStamp;
} FAKERTPRECV;



void FakeRtpRecv_setflags(FAKERTPRECV *pFRR, bool tlvEnabled, bool audioEnabled, bool videoEnabled);

void FakeRtpRecv_setfn(FAKERTPRECV *pFRR, FAKERTPRECV_RECV_FN recvFn);

void FakeRtpRecv_setquit(FAKERTPRECV *pFRR);


// Return values: 0, -1
int FakeRtpRecv_init(FAKERTPRECV **ppFRR, const T_RTPPARAM *param);

void FakeRtpRecv_uninit(FAKERTPRECV *pFRR);


//Loop function
DWORD RecvUdtSocketData(FAKERTPRECV *pFRR);
