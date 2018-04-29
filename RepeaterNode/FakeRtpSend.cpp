#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "platform.h"
#include "CommonLib.h"
#include "wps_queue.h"
#include "ControlCmd.h"
#include "FakeRtpSend.h"
#include "LogMsg.h"

#ifdef ANDROID_NDK
#include <android/log.h>
#endif


#define LOG_MSG  1



static void learn_remote_addr2(void *ctx2, sockaddr* his_addr, int addr_len)
{
	FAKERTPSEND *pFRS = (FAKERTPSEND *)ctx2;
	sockaddr_in *sin = (sockaddr_in *)his_addr;
	pFRS->dstport = ntohs(sin->sin_port);
	pFRS->dstip = sin->sin_addr.s_addr;
#ifdef LOG_MSG
	log_msg("NativeMainFunc: learn_remote_addr2() called...\n", LOG_LEVEL_DEBUG);
#endif
}

int FakeRtpSend_init(FAKERTPSEND **ppFRS, const T_RTPPARAM *param)
{
	int i;

	FAKERTPSEND *pFRS = (FAKERTPSEND *)malloc(sizeof(FAKERTPSEND));
	pFRS->bInit = TRUE;

	log_msg("FakeRtpSend_init()...\n", LOG_LEVEL_DEBUG);


	srand((unsigned int)time(NULL));
	for (i = 0; i < NUM_PAYLOAD_TYPE; i++) {
		pFRS->arraySequenceNumber[i] = (WORD)rand();
	}


	pFRS->dstip = param->dwDestIp;
	pFRS->dstport = param->nDestPort;
	pFRS->udpSock = param->udpSock;
	pFRS->fhandle = param->fhandle;
	pFRS->ftype = param->ftype;

	if (SOCKET_TYPE_UDT == pFRS->ftype) {
		UDT::register_learn_remote_addr2(pFRS->fhandle, learn_remote_addr2, (void *)pFRS);
	}

	pthread_mutex_init(&(pFRS->m_mutexRtpSend), NULL);

	*ppFRS = pFRS;
	return 0;
}

void FakeRtpSend_uninit(FAKERTPSEND *pFRS)
{
	int i;


	if (FALSE == pFRS->bInit) {
		return;
	}
	pFRS->bInit = FALSE;
	usleep(800000);

	log_msg("FakeRtpSend_uninit()...\n", LOG_LEVEL_DEBUG);

	if (SOCKET_TYPE_UDT == pFRS->ftype) {
		UDT::unregister_learn_remote_addr2(pFRS->fhandle);
	}

	pFRS->fhandle = INVALID_SOCKET;
	pFRS->udpSock = INVALID_SOCKET;
	pFRS->dstip = 0;
	pFRS->dstport = 0;

	for (i = 0; i < NUM_PAYLOAD_TYPE; i++) {
		pFRS->arraySequenceNumber[i] = 0;
	}
	
	pthread_mutex_destroy(&(pFRS->m_mutexRtpSend));
	
	free(pFRS);
}

int FakeRtpSend_sendpacket(FAKERTPSEND *pFRS, unsigned long rtptimestamp, const void *data, size_t len, BYTE bPayloadType, BYTE bReserved, BYTE bNri)
{
	if (FALSE == pFRS->bInit) {
		return -1;
	}

	unsigned char *szSendBuff = (unsigned char *)malloc(len + DIRECT_UDP_HEAD_LEN + 8);

	sockaddr_in dst;
	int i,n;
	int ret;

	if ((bNri & FAKERTP_RELIABLE_FLAG) == 0)
	{
		if (pFRS->udpSock == INVALID_SOCKET) {
			bNri |= FAKERTP_RELIABLE_FLAG;
		}
	}

	switch (bNri) {
		case 0:
			n = 1;
			break;
		case 1:
			n = 1;
			break;
		case 2:
			n = 2;//pRtpSendHandle->set_AudioNri(2);/* 发2遍 */
			break;
		case 3:
			n = 3;
			break;
		default:
			if ((bNri & FAKERTP_RELIABLE_FLAG) != 0)
			{
				n = 0;
			}
			else {
				n = 1;
			}
			break;
	}


	if (n > 0) // udp socket
	{
		memset(szSendBuff, DIRECT_UDP_HEAD_VAL, DIRECT_UDP_HEAD_LEN);
		*(szSendBuff + DIRECT_UDP_HEAD_LEN) = bPayloadType;
		*(szSendBuff + DIRECT_UDP_HEAD_LEN + 1) = bReserved;
		pf_set_word(szSendBuff + DIRECT_UDP_HEAD_LEN + 2, htons(pFRS->arraySequenceNumber[bPayloadType]));
		pf_set_dword(szSendBuff + DIRECT_UDP_HEAD_LEN + 4, htonl(rtptimestamp));
		memcpy(szSendBuff + DIRECT_UDP_HEAD_LEN + 8, data, len);

		if (pFRS->arraySequenceNumber[bPayloadType] == 0xffffU) {
			pFRS->arraySequenceNumber[bPayloadType] = 0;
		}
		else {
			pFRS->arraySequenceNumber[bPayloadType] += 1;
		}


		dst.sin_family = AF_INET;
		dst.sin_port = htons(pFRS->dstport);
		dst.sin_addr.s_addr = pFRS->dstip;
		memset(&(dst.sin_zero), '\0', 8);


		for (i = 0; i < n; i++) {
			ret = sendto(pFRS->udpSock, (char *)szSendBuff, len + DIRECT_UDP_HEAD_LEN + 8, 0, (sockaddr *)&dst, sizeof(dst));
			if (ret < len + DIRECT_UDP_HEAD_LEN + 8) {
				log_msg("FakeRtpSend sendto(udpSock) failed!\n", LOG_LEVEL_ERROR);
			}
			if (n > 2 && (i % 2) == 1) {
				usleep(10000);
			}
		}
	}
	else { // udt,tcp socket
		*(szSendBuff + 0) = bPayloadType;
		*(szSendBuff + 1) = bReserved;
		pf_set_word(szSendBuff + 2, htons(pFRS->arraySequenceNumber[bPayloadType]));
		pf_set_dword(szSendBuff + 4, htonl(rtptimestamp));
		memcpy(szSendBuff + 8, data, len);

		if (pFRS->arraySequenceNumber[bPayloadType] == 0xffffU) {
			pFRS->arraySequenceNumber[bPayloadType] = 0;
		}
		else {
			pFRS->arraySequenceNumber[bPayloadType] += 1;
		}

		pthread_mutex_lock(&(pFRS->m_mutexRtpSend));  //原来是在ControlCmd中Packet层次加锁
		ret = CtrlCmd_Send_FAKERTP_RESP_NOMUTEX(pFRS->ftype, pFRS->fhandle, szSendBuff, len + 8);
		pthread_mutex_unlock(&(pFRS->m_mutexRtpSend));//原来是在ControlCmd中Packet层次加锁
	}

    free(szSendBuff);
	return ret;
}
