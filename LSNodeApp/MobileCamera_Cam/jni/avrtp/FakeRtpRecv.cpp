#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include "platform.h"
#include "CommonLib.h"
#include "wps_queue.h"
#include "ControlCmd.h"
#include "FakeRtpRecv.h"

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




#define GET_TLV_PACKET_WAIT_TIME	50  /* ms */
#define GET_AUDIO_PACKET_WAIT_TIME	350 /* ms */
#define GET_VIDEO_PACKET_WAIT_TIME	350 /* ms */


static BOOL bInit = FALSE;
static UDPSOCKET udpSock = INVALID_SOCKET;
static SOCKET fhandle = INVALID_SOCKET;
static SOCKET_TYPE type = SOCKET_TYPE_UNKNOWN;
static pthread_t hUdtRecvThread = 0;
static BOOL bQuitUdtRecv = FALSE;
static wps_queue *arrayRecvQueue[NUM_PAYLOAD_TYPE];
static pthread_mutex_t arrayCriticalSec[NUM_PAYLOAD_TYPE];


void RecvUdpPacketCallback(SOCKET my_sock, char *data, int len, sockaddr *his_addr, int addr_len);
void * RecvUdtSocketData(void *lpParameter);


static inline double CurrentTime()
{
	struct timeval t_start;

	gettimeofday(&t_start, NULL);

	uint32_t _sec      = (uint32_t)(t_start.tv_sec);
	uint32_t _microsec = (uint32_t)(t_start.tv_usec);

	return (((double)_sec)+(((double)_microsec)/1000000.0));
}

static void PutIntoRecvPacketQueue(int nQueueIndex, RECV_PACKET_SMALL *pPacket)
{
	WORD wSeqNum = pPacket->wSeqNum;
	wps_queue *temp;
	wps_queue *tail;
	RECV_PACKET_SMALL *p;


	wps_init_que(&pPacket->link, (void *)pPacket);

	//EnterCriticalSection(&(arrayCriticalSec[nQueueIndex]));
	pthread_mutex_lock(&(arrayCriticalSec[nQueueIndex]));

	if (NULL == arrayRecvQueue[nQueueIndex]) {
#if 0//LOG_MSG
		log_msg("FakeRtpRecv: empty queue, enter directly!\n", LOG_LEVEL_DEBUG);
#endif
		arrayRecvQueue[nQueueIndex] = &pPacket->link;
	}
	else {
		//temp = arrayRecvQueue[nQueueIndex];
		//while (temp) {
		//	p = (RECV_PACKET_SMALL *)get_qbody(temp);
		//	if (p->wSeqNum > wSeqNum) {
		//		insert_before(&(arrayRecvQueue[nQueueIndex]), temp, &pPacket->link);
		//		break;
		//	}
		//	else if (p->wSeqNum == wSeqNum) {
		//		free(pPacket);
		//		break;
		//	}
		//	temp = temp->q_forw;
		//}

		tail = arrayRecvQueue[nQueueIndex];

		while (tail->q_forw != NULL) tail = tail->q_forw;

		temp = tail;
		while (temp) {
			p = (RECV_PACKET_SMALL *)get_qbody(temp);
			if ((p->wSeqNum < wSeqNum)
				|| (p->wSeqNum > 0xfc00U && wSeqNum < 0x400U)) /* wSeqNum: 0xffff -> 0 */ {
				wps_insert_que(&(arrayRecvQueue[nQueueIndex]), temp, &pPacket->link);
				goto _OUT;
			}
			else if (p->wSeqNum == wSeqNum) {
#if LOG_MSG
				log_msg("FakeRtpRecv: duplicate packet, drop!\n", LOG_LEVEL_DEBUG);
#endif
				free(pPacket);
				goto _OUT;
			}
			temp = temp->q_back;
#if LOG_MSG
			log_msg("FakeRtpRecv: late packet, haha!\n", LOG_LEVEL_DEBUG);
#endif
			if (pPacket->dfRecvTime != 0)
			{
				pPacket->dfRecvTime -= 0.1f;//迟来的数据包，应该尽快被上层获取！
			}
		}
#if 0//LOG_MSG
		log_msg("FakeRtpRecv: least sequence number, place it in the head!\n", LOG_LEVEL_DEBUG);
#endif
		wps_insert_que(&(arrayRecvQueue[nQueueIndex]), NULL, &pPacket->link);
	}

_OUT:
	//LeaveCriticalSection(&(arrayCriticalSec[nQueueIndex]));
	pthread_mutex_unlock(&(arrayCriticalSec[nQueueIndex]));
}

static void FreeRecvPacketQueue(int nQueueIndex)
{
	wps_queue *temp;
	RECV_PACKET_SMALL *p;
	int count;

	//EnterCriticalSection(&(arrayCriticalSec[nQueueIndex]));
	pthread_mutex_lock(&(arrayCriticalSec[nQueueIndex]));
	count= wps_count_que(arrayRecvQueue[nQueueIndex]);
	while (arrayRecvQueue[nQueueIndex]) {
		temp = arrayRecvQueue[nQueueIndex];
		wps_remove_que(&(arrayRecvQueue[nQueueIndex]), temp);
		p = (RECV_PACKET_SMALL *)get_qbody(temp);
		free(p);
	}
	//LeaveCriticalSection(&(arrayCriticalSec[nQueueIndex]));
	pthread_mutex_unlock(&(arrayCriticalSec[nQueueIndex]));

#ifdef ANDROID_NDK
	__android_log_print(ANDROID_LOG_INFO, "avrtp", "FakeRtpRecv: FreeRecvPacketQueue(%d) = %d nodes\n", nQueueIndex, count);
#else
	printf("FakeRtpRecv: FreeRecvPacketQueue(%d) = %d nodes\n", nQueueIndex, count);
#endif
}

static void learn_remote_addr2(void *ctx2, sockaddr* his_addr, int addr_len)
{
	sockaddr_in *sin = (sockaddr_in *)his_addr;
#ifdef ANDROID_NDK
	__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "learn_remote_addr2() called, %s[%d]\n", inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));
#endif
}

int FakeRtpRecv_init(const T_RTPPARAM *param)
{
	int i;


	if (bInit) {
		return 0;
	}
	bInit = TRUE;

	for (i = 0; i < NUM_PAYLOAD_TYPE; i++) {
		//InitializeCriticalSection(&(arrayCriticalSec[i]));
		pthread_mutex_init(&(arrayCriticalSec[i]), NULL);
		arrayRecvQueue[i] = NULL;
	}


#if LOG_MSG
	log_msg("FakeRtpRecv_init(...)\n", LOG_LEVEL_DEBUG);
#endif


	if (SOCKET_TYPE_UDT == param->type) {
		UDT::register_direct_udp_recv(param->fhandle, RecvUdpPacketCallback);
		UDT::register_learn_remote_addr2(param->fhandle, learn_remote_addr2, NULL);
	}
	udpSock = param->udpSock;
	fhandle = param->fhandle;
	type = param->type;


	if (0 == hUdtRecvThread)
	{
		bQuitUdtRecv = FALSE;
		pthread_create(&hUdtRecvThread, NULL, RecvUdtSocketData, NULL);
	}

	return 0;
}

void FakeRtpRecv_uninit()
{
	int i;


	if (!bInit) {
		return;
	}
	bInit = FALSE;
	usleep(800000);


	if (0 != hUdtRecvThread)
	{
		bQuitUdtRecv = TRUE;
		//pthread_join(hUdtRecvThread, NULL);
		usleep(1500000);
		hUdtRecvThread = 0;
	}

	if (SOCKET_TYPE_UDT == type) {
		UDT::unregister_direct_udp_recv(fhandle);
		UDT::unregister_learn_remote_addr2(fhandle);
	}
	udpSock = INVALID_SOCKET;
	fhandle = INVALID_SOCKET;
	type = SOCKET_TYPE_UNKNOWN;


	for (i = 0; i < NUM_PAYLOAD_TYPE; i++) {
		FreeRecvPacketQueue(i);
		//DeleteCriticalSection(&(arrayCriticalSec[i]));
		pthread_mutex_destroy(&(arrayCriticalSec[i]));
	}

#if LOG_MSG
	log_msg("FakeRtpRecv_uninit()\n", LOG_LEVEL_DEBUG);
#endif
}

int FakeRtpRecv_getpacket(RECV_PACKET_SMALL **pack, BYTE bPayloadType)
{
	if (!bInit) {
		return -1;
	}

	if (pack == NULL) {
		return -1;
	}

	double time = CurrentTime();
	double time_wait;
	int nQueueIndex = bPayloadType;
	wps_queue *temp;
	RECV_PACKET_SMALL *p;
	int ret = 0;

	if (bPayloadType == PAYLOAD_VIDEO) {
		time_wait = GET_VIDEO_PACKET_WAIT_TIME;
	}
	else if (bPayloadType == PAYLOAD_AUDIO) {
		time_wait = GET_AUDIO_PACKET_WAIT_TIME;
	}
	else {
		time_wait = GET_TLV_PACKET_WAIT_TIME;
	}

	//EnterCriticalSection(&(arrayCriticalSec[nQueueIndex]));
	pthread_mutex_lock(&(arrayCriticalSec[nQueueIndex]));

	temp = arrayRecvQueue[nQueueIndex];
	if (temp != NULL) {
		p = (RECV_PACKET_SMALL *)get_qbody(temp);
		if (p->dfRecvTime == 0 || (time - p->dfRecvTime) * (double)1000 >= time_wait) {
			*pack = p;
			wps_remove_que(&(arrayRecvQueue[nQueueIndex]), temp);
			ret = 1;
		}
//		else {
//#if LOG_MSG
//			log_msg("FakeRtpRecv: can not get this time, wait...\n", LOG_LEVEL_DEBUG);
//#endif
//		}
	}

	//LeaveCriticalSection(&(arrayCriticalSec[nQueueIndex]));
	pthread_mutex_unlock(&(arrayCriticalSec[nQueueIndex]));

	return ret;
}

void RecvUdpPacketCallback(SOCKET my_sock, char *data, int len, sockaddr *his_addr, int addr_len)
{
	RECV_PACKET_SMALL *pNewPack;
	BYTE bPayloadType;

	if (len <= RECV_PACKET_SMALL_BUFF_SIZE) {
		pNewPack = (RECV_PACKET_SMALL *)malloc(sizeof(RECV_PACKET_SMALL));
	}
	else if (len <= RECV_PACKET_MEDIUM_BUFF_SIZE) {
		pNewPack = (RECV_PACKET_SMALL *)malloc(sizeof(RECV_PACKET_MEDIUM));
	}
	else if (len <= RECV_PACKET_BIG_BUFF_SIZE) {
		pNewPack = (RECV_PACKET_SMALL *)malloc(sizeof(RECV_PACKET_BIG));
	}
	else {
#if LOG_MSG
		log_msg("RecvUdpPacketCallback: Packet is too big!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", LOG_LEVEL_DEBUG);
#endif
		return;
	}
	if (NULL == pNewPack) {
#if LOG_MSG
		log_msg("RecvUdpPacketCallback: No memory!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", LOG_LEVEL_DEBUG);
#endif
		return;
	}

	memcpy(pNewPack->szData, data, len);
	pNewPack->nDataSize = len;
	pNewPack->wSeqNum = ntohs(pf_get_word(pNewPack->szData + 2));
	pNewPack->bReserved = *((unsigned char *)(pNewPack->szData) + 1);
	pNewPack->dfRecvTime = CurrentTime();

	bPayloadType = pNewPack->szData[0];
	PutIntoRecvPacketQueue(bPayloadType, pNewPack);
}

void * RecvUdtSocketData(void *lpParameter)
{
	BOOL bRecvEnd = FALSE;

	if (fhandle == INVALID_SOCKET) {
		return 0;
	}

	while (!bQuitUdtRecv)
	{
		char buf[6];
		WORD wCmd;
		DWORD copy_len;
		RECV_PACKET_SMALL *pNewPack;
		BYTE bPayloadType;
		int ret;


		if (RecvStream(type, fhandle, buf, 6) < 0)
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
				log_msg("RecvUdtSocketData(): CMD_CODE_END?????????\n", LOG_LEVEL_DEBUG);
			}
			break;
		}

		if (copy_len <= RECV_PACKET_SMALL_BUFF_SIZE) {
			pNewPack = (RECV_PACKET_SMALL *)malloc(sizeof(RECV_PACKET_SMALL));
		}
		else if (copy_len <= RECV_PACKET_MEDIUM_BUFF_SIZE) {
			pNewPack = (RECV_PACKET_SMALL *)malloc(sizeof(RECV_PACKET_MEDIUM));
		}
		else if (copy_len <= RECV_PACKET_BIG_BUFF_SIZE) {
			pNewPack = (RECV_PACKET_SMALL *)malloc(sizeof(RECV_PACKET_BIG));
		}
		else {
			
			for (int i = 0; i < copy_len; i++) {
				RecvStream(type, fhandle, buf, 1);
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


		ret = RecvStream(type, fhandle, (char *)(pNewPack->szData), copy_len);
		if (ret == 0) {
			pNewPack->nDataSize = copy_len;
			pNewPack->wSeqNum = ntohs(pf_get_word(pNewPack->szData + 2));
			pNewPack->bReserved = *((unsigned char *)(pNewPack->szData) + 1);
			pNewPack->dfRecvTime = 0;//CurrentTime() -> 0，可靠传输通道，jitter缓冲时间不需要。

            if (pNewPack->wSeqNum == 0)
            {
                printf("FakeRtpRecv: index=%u, wSeqNum=%u, copy_len=%lu\n", pNewPack->szData[0], pNewPack->wSeqNum, copy_len);
            }
			bPayloadType = pNewPack->szData[0];
			PutIntoRecvPacketQueue(bPayloadType, pNewPack);
		}
		else {
			free(pNewPack);
			break;
		}

	}//while

	if (bRecvEnd == FALSE) {
		CtrlCmd_Recv_AV_END(type, fhandle);
	}

	return 0;
}
