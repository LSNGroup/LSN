#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "platform.h"
#include "CommonLib.h"
#include "wps_queue.h"
#include "ControlCmd.h"
#include "FakeRtpSend.h"

#ifdef ANDROID_NDK
#include <android/log.h>
#endif


#define LOG_MSG  1

#if LOG_MSG
#ifdef ANDROID_NDK
#define log_msg(msg, lev)  __android_log_print(ANDROID_LOG_INFO, "avrtp", msg)
#else
#define log_msg(msg, lev)  printf(msg)
#endif
#endif



static BOOL bInit = FALSE;
static UDPSOCKET udpSock = INVALID_SOCKET;
static SOCKET fhandle = INVALID_SOCKET;
static SOCKET_TYPE ftype;
static DWORD dstip;
static WORD  dstport;
static WORD arraySequenceNumber[NUM_PAYLOAD_TYPE];


static void learn_remote_addr2(void *ctx2, sockaddr* his_addr, int addr_len)
{
	sockaddr_in *sin = (sockaddr_in *)his_addr;
	dstport = ntohs(sin->sin_port);
	dstip = sin->sin_addr.s_addr;
#ifdef ANDROID_NDK
	__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "learn_remote_addr2() called, %s[%d]\n", inet_ntoa(sin->sin_addr), dstport);
#endif
}

int FakeRtpSend_init(const T_RTPPARAM *param)
{
	int i;


	if (bInit) {
		return 0;
	}
	bInit = TRUE;

	log_msg("FakeRtpSend_init()...\n", LOG_LEVEL_DEBUG);


	srand((unsigned int)time(NULL));
	for (i = 0; i < NUM_PAYLOAD_TYPE; i++) {
		arraySequenceNumber[i] = (WORD)rand();
	}


	dstip = param->dwDestIp;
	dstport = param->nDestPort;
	udpSock = param->udpSock;
	fhandle = param->fhandle;
	ftype = param->type;

	if (SOCKET_TYPE_UDT == ftype) {
		UDT::register_learn_remote_addr2(fhandle, learn_remote_addr2, NULL);
	}

	return 0;
}

void FakeRtpSend_uninit()
{
	int i;


	if (!bInit) {
		return;
	}
	bInit = FALSE;
	usleep(800000);

	log_msg("FakeRtpSend_uninit()...\n", LOG_LEVEL_DEBUG);

	if (SOCKET_TYPE_UDT == ftype) {
		UDT::unregister_learn_remote_addr2(fhandle);
	}

	fhandle = INVALID_SOCKET;
	udpSock = INVALID_SOCKET;
	dstip = 0;
	dstport = 0;

	for (i = 0; i < NUM_PAYLOAD_TYPE; i++) {
		arraySequenceNumber[i] = 0;
	}
}

int FakeRtpSend_sendpacket(DWORD rtptimestamp, const void *data, size_t len, BYTE bPayloadType, BYTE bReserved, BYTE bNri)
{
	if (!bInit) {
		return -1;
	}

	unsigned char *szSendBuff = (unsigned char *)malloc(len + DIRECT_UDP_HEAD_LEN + 8);

	sockaddr_in dst;
	int i,n;
	int ret;

	if ((bNri & FAKERTP_RELIABLE_FLAG) == 0)
	{
		if (udpSock == INVALID_SOCKET) {
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
		pf_set_word(szSendBuff + DIRECT_UDP_HEAD_LEN + 2, htons(arraySequenceNumber[bPayloadType]));
		pf_set_dword(szSendBuff + DIRECT_UDP_HEAD_LEN + 4, htonl(rtptimestamp));
		memcpy(szSendBuff + DIRECT_UDP_HEAD_LEN + 8, data, len);

		if (arraySequenceNumber[bPayloadType] == 0xffffU) {
			arraySequenceNumber[bPayloadType] = 0;
		}
		else {
			arraySequenceNumber[bPayloadType] += 1;
		}


		dst.sin_family = AF_INET;
		dst.sin_port = htons(dstport);
		dst.sin_addr.s_addr = dstip;
		memset(&(dst.sin_zero), '\0', 8);


		for (i = 0; i < n; i++) {
			ret = sendto(udpSock, (char *)szSendBuff, len + DIRECT_UDP_HEAD_LEN + 8, 0, (sockaddr *)&dst, sizeof(dst));
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
		pf_set_word(szSendBuff + 2, htons(arraySequenceNumber[bPayloadType]));
		pf_set_dword(szSendBuff + 4, htonl(rtptimestamp));
		memcpy(szSendBuff + 8, data, len);

		if (arraySequenceNumber[bPayloadType] == 0xffffU) {
			arraySequenceNumber[bPayloadType] = 0;
		}
		else {
			arraySequenceNumber[bPayloadType] += 1;
		}

		//pthread_mutex_lock(&m_mutexRtpSend);  //在ControlCmd中Packet层次加锁
		ret = CtrlCmd_Send_FAKERTP_RESP(ftype, fhandle, szSendBuff, len + 8);
		//pthread_mutex_unlock(&m_mutexRtpSend);//在ControlCmd中Packet层次加锁
	}

    free(szSendBuff);
	return ret;
}
