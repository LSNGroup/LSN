#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "platform.h"
#include "CommonLib.h"
#include "ControlCmd.h"
#include "SimpleRtpRecv.h"


#define xmalloc(s) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (s))
#define xfree(p)   HeapFree(GetProcessHeap(), 0, (p))


#define LOG_MSG  1

#if LOG_MSG
#include "LogMsg.h"
#endif


void RecvUdpPacketCallback(void *ctx, SOCKET my_sock, char *data, int len, sockaddr *his_addr, int addr_len);


#define MIN_VALUE(a,b)  (((a)<(b))?(a):(b))
#define RTP_TIMESTAMP_UNKNOWN  0xffffffffUL


void FakeRtpRecv_setflags(FAKERTPRECV *pFRR, bool tlvEnabled, bool audioEnabled, bool videoEnabled)
{
	pFRR->m_audioTimeStamp = audioEnabled ? 0 : RTP_TIMESTAMP_UNKNOWN;
	pFRR->m_videoTimeStamp = videoEnabled ? 0 : RTP_TIMESTAMP_UNKNOWN;
}

void FakeRtpRecv_setfn(FAKERTPRECV *pFRR, FAKERTPRECV_RECV_FN recvFn)
{
	pFRR->recvFn = recvFn;
}

void FakeRtpRecv_setquit(FAKERTPRECV *pFRR)
{
	pFRR->bQuitUdtRecv = TRUE;
}

static void PutIntoRecvPacketQueue(FAKERTPRECV *pFRR, int payloadType, unsigned long rtptimestamp, RECV_PACKET_SMALL *pPacket)
{
	if (NULL != pFRR->recvFn) {
		pFRR->recvFn((void *)pFRR, payloadType, rtptimestamp, pPacket->szData, pPacket->nDataSize);
	}
	xfree(pPacket);
}

int FakeRtpRecv_init(FAKERTPRECV **ppFRR, const T_RTPPARAM *param)
{
	FAKERTPRECV *pFRR = (FAKERTPRECV *)xmalloc(sizeof(FAKERTPRECV));
	pFRR->bInit = TRUE;

#if LOG_MSG
	log_msg("FakeRtpRecv_init(...)", LOG_LEVEL_DEBUG);
#endif


	if (SOCKET_TYPE_UDT == param->ftype) {
		UDT::register_direct_udp_recv2(param->fhandle, RecvUdpPacketCallback, (void *)pFRR);
	}
	pFRR->udpSock = param->udpSock;
	pFRR->fhandle = param->fhandle;
	pFRR->ftype = param->ftype;

	pFRR->recvFn = NULL;

	//if (NULL == pFRR->hUdtRecvThread)
	{
		pFRR->bQuitUdtRecv = FALSE;
	}

	*ppFRR = pFRR;
	return 0;
}

void FakeRtpRecv_uninit(FAKERTPRECV *pFRR)
{
	if (FALSE == pFRR->bInit) {
		return;
	}
	pFRR->bInit = FALSE;
	Sleep(800);

	//if (NULL != pFRR->hUdtRecvThread)
	{
		pFRR->bQuitUdtRecv = TRUE;
	}

	if (SOCKET_TYPE_UDT == pFRR->ftype) {
		UDT::unregister_direct_udp_recv2(pFRR->fhandle);
	}
	pFRR->udpSock = INVALID_SOCKET;
	pFRR->fhandle = INVALID_SOCKET;
	pFRR->ftype = SOCKET_TYPE_UNKNOWN;

	xfree(pFRR);

#if LOG_MSG
	log_msg("FakeRtpRecv_uninit()", LOG_LEVEL_DEBUG);
#endif
}

void RecvUdpPacketCallback(void *ctx, SOCKET my_sock, char *data, int len, sockaddr *his_addr, int addr_len)
{
	FAKERTPRECV *pFRR = (FAKERTPRECV *)ctx;
	RECV_PACKET_SMALL *pNewPack;
	BYTE bPayloadType;

	if (len <= RECV_PACKET_SMALL_BUFF_SIZE) {
		pNewPack = (RECV_PACKET_SMALL *)xmalloc(sizeof(RECV_PACKET_SMALL));
	}
	else if (len <= RECV_PACKET_MEDIUM_BUFF_SIZE) {
		pNewPack = (RECV_PACKET_SMALL *)xmalloc(sizeof(RECV_PACKET_MEDIUM));
	}
	else if (len <= RECV_PACKET_BIG_BUFF_SIZE) {
		pNewPack = (RECV_PACKET_SMALL *)xmalloc(sizeof(RECV_PACKET_BIG));
	}
	else {
#if LOG_MSG
		log_msg("RecvUdpPacketCallback: Packet is too big!!!!!!!!!!!!!!!!!!!!!!!!!!!!", LOG_LEVEL_DEBUG);
#endif
		return;
	}
	if (NULL == pNewPack) {
#if LOG_MSG
		log_msg("RecvUdpPacketCallback: No memory!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!", LOG_LEVEL_DEBUG);
#endif
		return;
	}

	memcpy(pNewPack->szData, data, len);
	pNewPack->nDataSize = len;
	pNewPack->wSeqNum = ntohs(pf_get_word(pNewPack->szData + 2));
	pNewPack->bReserved = *((unsigned char *)(pNewPack->szData) + 1);
	pNewPack->dfRecvTime = 0;

	bPayloadType = pNewPack->szData[0];
	PutIntoRecvPacketQueue(pFRR, bPayloadType, ntohl(pf_get_dword(pNewPack->szData + 4)), pNewPack);
}

DWORD RecvUdtSocketData(FAKERTPRECV *pFRR)
{
	//FAKERTPRECV *pFRR = (FAKERTPRECV *)lpParameter;
	BOOL bRecvEnd = FALSE;

	if (pFRR->fhandle == INVALID_SOCKET) {
		return 0;
	}

	while (FALSE == pFRR->bQuitUdtRecv)
	{
		char buf[6];
		WORD wCmd;
		DWORD copy_len;
		RECV_PACKET_SMALL *pNewPack;
		BYTE bPayloadType;
		int ret;


		if (RecvStream(pFRR->ftype, pFRR->fhandle, buf, 6) < 0)
		{
			break;
		}

		wCmd = ntohs(pf_get_word(buf + 0));
		copy_len = ntohl(pf_get_dword(buf + 2));
		//assert(wCmd == CMD_CODE_FAKERTP_RESP && copy_len > 0);
		if (wCmd != CMD_CODE_FAKERTP_RESP || copy_len == 0)
		{
			if (wCmd == CMD_CODE_END) {
				bRecvEnd = TRUE;
#if LOG_MSG
				log_msg("RecvUdtSocketData(): CMD_CODE_END?????????\n", LOG_LEVEL_DEBUG);
#endif
			}
			break;
		}

		if (copy_len <= RECV_PACKET_SMALL_BUFF_SIZE) {
			pNewPack = (RECV_PACKET_SMALL *)xmalloc(sizeof(RECV_PACKET_SMALL));
		}
		else if (copy_len <= RECV_PACKET_MEDIUM_BUFF_SIZE) {
			pNewPack = (RECV_PACKET_SMALL *)xmalloc(sizeof(RECV_PACKET_MEDIUM));
		}
		else if (copy_len <= RECV_PACKET_BIG_BUFF_SIZE) {
			pNewPack = (RECV_PACKET_SMALL *)xmalloc(sizeof(RECV_PACKET_BIG));
		}
		else {
			
			for (int i = 0; i < copy_len; i++) {
				RecvStream(pFRR->ftype, pFRR->fhandle, buf, 1);
			}
			
#if LOG_MSG
			log_msg("RecvUdtSocketData: Packet is too big!\n", LOG_LEVEL_DEBUG);
#endif
			continue;
		}
		if (NULL == pNewPack) {
#if LOG_MSG
			log_msg("RecvUdtSocketData: No memory!\n", LOG_LEVEL_DEBUG);
#endif
			break;
		}


		ret = RecvStream(pFRR->ftype, pFRR->fhandle, (char *)(pNewPack->szData), copy_len);
		if (ret == 0) {
			pNewPack->nDataSize = copy_len;
			pNewPack->wSeqNum = ntohs(pf_get_word(pNewPack->szData + 2));
			pNewPack->bReserved = *((unsigned char *)(pNewPack->szData) + 1);
			pNewPack->dfRecvTime = 0;

			bPayloadType = pNewPack->szData[0];
			PutIntoRecvPacketQueue(pFRR, bPayloadType, ntohl(pf_get_dword(pNewPack->szData + 4)), pNewPack);
		}
		else {
			xfree(pNewPack);
			break;
		}

	}//while

	if (bRecvEnd == FALSE) {
#if LOG_MSG
		log_msg("RecvUdtSocketData: CtrlCmd_Recv_AV_END()...\n", LOG_LEVEL_DEBUG);
#endif
		CtrlCmd_Recv_AV_END(pFRR->ftype, pFRR->fhandle);
	}

	return 0;
}
