#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "CommonLib.h"
#include "wps_queue.h"
#include "AudioCodec.h"
#include "G729Recv.h"

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



static BOOL m_bStartPlay = FALSE;
static pthread_mutex_t m_secQueue; /* To protect m_pRecvQueue */
static wps_queue *	m_pRecvQueue;
static DWORD	m_ulLastSeqNum;


//
// Return values:
// -1: Old packet
//  0: Normal
//  1: Packet lost
//
static int CheckRtpPacket(BYTE *pcData, int nLen, BOOL bMarker, unsigned short nSeqNum);

static BOOL PutIntoRecvQueue(DWORD rtptimestamp, BYTE *lpData, int nLength);
static void FreeRecvQueue();


static pthread_t g_hRecvG729DataThread = 0;
static BOOL g_bExitRecvG729DataProc	= FALSE;

void *RecvG729Data(void *lpParameter)
{
	int  nPayloadLen;
	WORD wSeqNum;
	DWORD dwTimeStamp;
	BYTE bReserved;

	RECV_PACKET_SMALL *pack;
	int ret;
	unsigned char *buff;


#if LOG_MSG
	log_msg("RecvG729Data thread start...\n", LOG_LEVEL_DEBUG);
#endif

	while (1)
	{
		if (g_bExitRecvG729DataProc) {
			break;
		}

		while ((ret = FakeRtpRecv_getpacket(&pack, PAYLOAD_AUDIO)) > 0)
		{
			// You can examine the data here
			dwTimeStamp  = ntohl(pf_get_dword(pack->szData + 4));
			bReserved    = pack->bReserved;
			wSeqNum      = pack->wSeqNum;
			buff         = pack->szData + 8;
			nPayloadLen  = pack->nDataSize - 8;

			if (CheckRtpPacket(buff,nPayloadLen,true,wSeqNum) != -1)
			{
				unsigned char *tmp = (unsigned char *)malloc(nPayloadLen * AUDIO_MAX_COMPRESSION);
				int nOutBytes = AudioDecoder(bReserved, buff, nPayloadLen, tmp);
				PutIntoRecvQueue(dwTimeStamp, tmp, nOutBytes);// to free(tmp)
			}

			// we don't longer need this packet, so we'll free it
			free(pack);

			if (g_bExitRecvG729DataProc) {
				break;
			}
		}

		if (ret < 0) {
			;//error
		}
		
		usleep(5000);
	}

#if LOG_MSG
	log_msg("RecvG729Data thread exit!\n", LOG_LEVEL_DEBUG);
#endif

	return 0;
}


//
// Return values:
// -1: Old packet
//  0: Normal
//  1: Packet lost
//
static int CheckRtpPacket(BYTE *pcData, int nLen, BOOL bMarker, unsigned short nSeqNum)
{
	BOOL bPacketOld = FALSE;
	BOOL bPacketLost = FALSE;

	if (m_ulLastSeqNum == 0xffffffffUL) {
		m_ulLastSeqNum = nSeqNum;
	}
	else {
		if (nSeqNum <= m_ulLastSeqNum) {
			if (nSeqNum < 0x400U && m_ulLastSeqNum > 0xfc00UL) {
				if (nSeqNum + (0xffffUL - m_ulLastSeqNum) > 0) {
					bPacketLost = TRUE;
				}
			}
			else {
				bPacketOld = TRUE;
			}
		}
		else {
			if (nSeqNum != m_ulLastSeqNum + 1) {
				bPacketLost = TRUE;
			}
			else {
				m_ulLastSeqNum = nSeqNum;
			}
		}
	}

	if (bPacketOld) {
#if LOG_MSG
		char szMsg[256];
		sprintf(szMsg, "##### G729Recv: RTP old packet. m_ulLastSeqNum=%lu, nSeqNum=%u\n", m_ulLastSeqNum, nSeqNum);
		log_msg(szMsg, LOG_LEVEL_DEBUG);
#endif
		m_ulLastSeqNum = nSeqNum;//return -1;
	}

	if (bPacketLost) {
#if LOG_MSG
		char szMsg[256];
		sprintf(szMsg, "!!!!! G729Recv: RTP packet lost. m_ulLastSeqNum=%lu, nSeqNum=%u\n", m_ulLastSeqNum, nSeqNum);
		log_msg(szMsg, LOG_LEVEL_DEBUG);
#endif
		m_ulLastSeqNum = nSeqNum;//return 1;
	}

	return 0;
}

static BOOL PutIntoRecvQueue(DWORD rtptimestamp, BYTE *lpData, int nLength)
{
	RECV_DATA_NODE *p;
	int count;

	// We must protect the memory usage!!!
	//EnterCriticalSection(&m_secQueue);
	pthread_mutex_lock(&m_secQueue);
	count = wps_count_que(m_pRecvQueue);
	//LeaveCriticalSection(&m_secQueue);
	pthread_mutex_unlock(&m_secQueue);
	if (count > G729_RECV_QUEUE_MAX_NODES) {
#if LOG_MSG
		log_msg("G729 decoding queue full!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", LOG_LEVEL_DEBUG);
#endif
		usleep(2*1000);//给播放线程留出CPU
		free(lpData);
		return FALSE;
	}

	p = (RECV_DATA_NODE *)malloc(sizeof(RECV_DATA_NODE));
	if (!p) {
		free(lpData);
		return FALSE;
	}

	wps_init_que(&p->link, (void *)p);
	p->pDataPtr = lpData;
	p->nDataSize = nLength;
	//p->bMarker = TRUE;
	p->rtptimestamp = rtptimestamp;

	//EnterCriticalSection(&m_secQueue);
	pthread_mutex_lock(&m_secQueue);
	wps_append_que(&m_pRecvQueue, &p->link);
	//LeaveCriticalSection(&m_secQueue);
	pthread_mutex_unlock(&m_secQueue);

	return TRUE;
}

static void FreeRecvQueue()
{
	wps_queue *temp;
	RECV_DATA_NODE *p;

	//EnterCriticalSection(&m_secQueue);
	pthread_mutex_lock(&m_secQueue);
	while (m_pRecvQueue) {
		temp = m_pRecvQueue;
		wps_remove_que(&m_pRecvQueue, temp);
		p = (RECV_DATA_NODE *)get_qbody(temp);
		free(p->pDataPtr);
		free(p);
	}
	//LeaveCriticalSection(&m_secQueue);
	pthread_mutex_unlock(&m_secQueue);
}

void AudioRecvStart(int nBasePort, DWORD dwDestIp/* Network byte order */, int nDestPort, UDPSOCKET udpSock, SOCKET fhandle, SOCKET_TYPE type)
{
	if (!m_bStartPlay)
	{
		m_bStartPlay = TRUE;
		
#if LOG_MSG
		log_msg("AudioRecvStart(...)\n", LOG_LEVEL_DEBUG);
#endif
		
		AudioDecoderInit();
		
		//InitializeCriticalSection(&m_secQueue);
		pthread_mutex_init(&m_secQueue, NULL);
		m_pRecvQueue = NULL;
		m_ulLastSeqNum = 0xffffffffUL;
		
		T_RTPPARAM tRtpParam;
		tRtpParam.nBasePort = nBasePort;
		tRtpParam.dwDestIp = dwDestIp;
		tRtpParam.nDestPort = nDestPort;
		tRtpParam.udpSock = udpSock;
		tRtpParam.fhandle = fhandle;
		tRtpParam.type = type;
		FakeRtpRecv_init(&tRtpParam);
		
		g_bExitRecvG729DataProc = FALSE;
		pthread_create(&g_hRecvG729DataThread, NULL, RecvG729Data, NULL);
	}
}

void AudioRecvStop(void)
{
	if (m_bStartPlay)
	{
		m_bStartPlay = FALSE;
		
#if LOG_MSG
		log_msg("AudioRecvStop()\n", LOG_LEVEL_DEBUG);
#endif
		
		if (0 != g_hRecvG729DataThread)
		{
			g_bExitRecvG729DataProc = TRUE;
			//Waiting...
			pthread_join(g_hRecvG729DataThread, NULL);
			g_hRecvG729DataThread = 0;
		}
		
		FakeRtpRecv_uninit();
		
		FreeRecvQueue();
		//DeleteCriticalSection(&m_secQueue);
		pthread_mutex_destroy(&m_secQueue);
	}
}

//
// Return values:
// -1: Error
//  0: No data
//  n: Length of data
//
int AudioRecvGetData(unsigned char *Buff, int bufSize, BOOL *pFull, DWORD *pTimeStamp)
{
	if (NULL == Buff || bufSize < 0 || NULL == pFull || NULL == pTimeStamp)
	{
		return -1;
	}
	
	if (!m_bStartPlay)
	{
		return -1;
	}
	

	int count;
	BYTE *pcData = Buff;
	int  lSize = bufSize;
	int  lDataLength = 0;
	wps_queue *temp;
	RECV_DATA_NODE *p;
	DWORD rtptimestamp = 0;


	//EnterCriticalSection(&m_secQueue);
	pthread_mutex_lock(&m_secQueue);

	if (NULL != m_pRecvQueue)
	{
		count = wps_count_que(m_pRecvQueue);

		if (count > G729_RECV_QUEUE_MAX_NODES*3/4) {
			*pFull = TRUE;
		}
		else {
			*pFull = FALSE;
		}

		if (count > G729_RECV_QUEUE_MAX_NODES/3)
		{
			temp = m_pRecvQueue;
			while (temp && lDataLength < lSize) {

				p = (RECV_DATA_NODE *)get_qbody(temp);

				if ((lDataLength + p->nDataSize) <= lSize) {
					memcpy(pcData + lDataLength, p->pDataPtr, p->nDataSize);
					lDataLength += p->nDataSize;
					if (rtptimestamp == 0)
					{
						rtptimestamp = p->rtptimestamp;
					}
					wps_remove_que(&m_pRecvQueue, temp);
					p = (RECV_DATA_NODE *)get_qbody(temp);
					free(p->pDataPtr);
					free(p);
					temp = m_pRecvQueue;
					continue;
				}
				else {
					break;
				}
				temp = temp->q_forw;
			}//while
		}
	}
	else {
		*pFull = FALSE;
	}

	*pTimeStamp = rtptimestamp;
	
	//LeaveCriticalSection(&m_secQueue);
	pthread_mutex_unlock(&m_secQueue);


	if (lDataLength == 0) {
		usleep(5000);
	}

    return lDataLength;
}
