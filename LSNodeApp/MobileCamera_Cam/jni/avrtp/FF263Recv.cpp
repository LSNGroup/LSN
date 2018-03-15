#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "platform.h"
#include "CommonLib.h"
#include "wps_queue.h"
#include "colorspace.h"
#include "FF263Recv.h"

#ifdef ANDROID_NDK
extern "C" {
	#include "avcodec.h"
}
#else
extern "C" {
	#include "libavutil/avutil.h"
	#include "libavcodec/avcodec.h"
	#include "libavformat/avformat.h"
	#include "libswscale/swscale.h"
}
#endif


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

static AVCodec *m_avCodec;			      // Codec
static AVCodecContext *m_avCodecContext;  // Codec Context
static AVFrame *m_avFrame;		          // Frame

static long long m_avgFrameTime = 0;
static int m_nWidth = 0;
static int m_nHeight = 0;

static BOOL m_bVflip = FALSE;


static void OnReceiveRtpPacket(DWORD rtptimestamp, BYTE * pcData, int nLen, BOOL bMarker, unsigned short nSeqNum);
static BOOL PutIntoRecvQueue(DWORD rtptimestamp, BYTE *lpData, int nLength);
static void FreeRecvQueue();

static void FF263DecReceive(DWORD rtptimestamp, BYTE *lpData, int nLength);
static void FF263DecSendSample(DWORD rtptimestamp, AVCodecContext *c, AVFrame *picture);



static pthread_t g_hRecvDataThread = 0;
static BOOL g_bExitRecvDataProc	= FALSE;

void *FF263RecvData(void *lpParameter)
{
	int  nPayloadLen;
	WORD wSeqNum;
	DWORD dwTimeStamp;

	RECV_PACKET_SMALL *pack;
	int ret;
	unsigned char *buff;


#if LOG_MSG
	log_msg("FF263RecvData thread start...\n", LOG_LEVEL_DEBUG);
#endif

	while (1)
	{
		if (g_bExitRecvDataProc) {
			break;
		}

		while ((ret = FakeRtpRecv_getpacket(&pack, PAYLOAD_VIDEO)) > 0)
		{
			// You can examine the data here
			dwTimeStamp  = ntohl(pf_get_dword(pack->szData + 4));
			wSeqNum      = pack->wSeqNum;
			buff         = pack->szData + 8;
			nPayloadLen  = pack->nDataSize - 8;

			OnReceiveRtpPacket(dwTimeStamp,buff,nPayloadLen,true,wSeqNum);

			// we don't longer need the packet, so
			// we'll delete it
			free(pack);

			if (g_bExitRecvDataProc) {
				break;
			}
		}

		if (ret < 0) {
			;//error
		}

		usleep(5000);
	}

#if LOG_MSG
	log_msg("FF263RecvData thread exit!\n", LOG_LEVEL_DEBUG);
#endif

	return 0;
}



static void OnReceiveRtpPacket(DWORD rtptimestamp, BYTE * pcData, int nLen, BOOL bMarker, unsigned short nSeqNum)
{
	BOOL bPacketOld = FALSE;
	BOOL bPacketLost = FALSE;

	if (!m_bStartPlay) {
		return;
	}

	if (m_ulLastSeqNum == 0xffffffffUL) {
		m_ulLastSeqNum = nSeqNum;
	}
	else {
		if (nSeqNum <= m_ulLastSeqNum) {
			if (nSeqNum < 0x200U && m_ulLastSeqNum > 0xfe00UL) {
				if (nSeqNum + (0xffffUL - m_ulLastSeqNum) > 0) {
					bPacketLost = TRUE;
				}
				m_ulLastSeqNum = nSeqNum;
			}
			else {
				bPacketOld = TRUE;
			}
		}
		else {
			if (nSeqNum != m_ulLastSeqNum + 1) {
				bPacketLost = TRUE;
			}
			m_ulLastSeqNum = nSeqNum;
		}
	}

	if (bPacketOld) {
#if LOG_MSG
		log_msg("############# RTP old packet.\n", LOG_LEVEL_DEBUG);
#endif
		return;
	}

	if (bPacketLost) {
#if LOG_MSG
		log_msg("!!!!!!!!!!!!! RTP packet lost.\n", LOG_LEVEL_DEBUG);
#endif
	}

	FF263DecReceive(rtptimestamp, pcData, nLen);
}

/* RecvQueue中每个节点都是一个YV12帧。*/
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
	if (count > FF263_RECV_QUEUE_MAX_NODES) {
#if LOG_MSG
		log_msg("FF263 decoding queue full!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", LOG_LEVEL_DEBUG);
#endif
		free(lpData);
		return FALSE;
	}

	p = (RECV_DATA_NODE *)malloc(sizeof(RECV_DATA_NODE));
	if (NULL == p) {
		free(lpData);
		return FALSE;
	}

	wps_init_que(&p->link, (void *)p);
	p->pDataPtr = lpData;
	p->nDataSize = nLength;
	p->bMarker = TRUE;
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

#if LOG_MSG
	log_msg("FreeRecvQueue() OK!\n", LOG_LEVEL_DEBUG);
#endif
}

static void FF263DecReceive(DWORD rtptimestamp, BYTE *lpData, int nLength)
{
#if LOG_MSG
    char szMsg[MAX_PATH];
    sprintf(szMsg, "FF263DecReceive(%d bytes)...\n", nLength);
	log_msg(szMsg, LOG_LEVEL_DEBUG);
#endif
	
	
	int got_picture, consumed_bytes;
	AVPacket packet = {0};
	packet.data = lpData;
	packet.size = nLength;
	
	consumed_bytes = avcodec_decode_video2(m_avCodecContext, m_avFrame, &got_picture, &packet);
	if(consumed_bytes > 0 && got_picture)
	{
		FF263DecSendSample(rtptimestamp, m_avCodecContext, m_avFrame);
	}
}

static void FF263DecSendSample(DWORD rtptimestamp, AVCodecContext *c, AVFrame *picture)
{
    int	i;
    BYTE *p, *pDst, *pTemp;
	
#if 0//LOG_MSG
	log_msg("FF263DecSendSample()...\n", LOG_LEVEL_DEBUG);
#endif
	
	if (c->width != m_nWidth || c->height != m_nHeight)
	{
		m_nWidth  = c->width;
		m_nHeight = c->height;
	}
	
	pDst = (BYTE *)malloc(c->width * c->height + (c->width * c->height >> 1));
	if (NULL == pDst)
	{
		return;
	}
	
	pTemp = pDst;
    //p = frame->Y[0];

	for(i=0; i<c->height; i++) {
		memcpy(pTemp, picture->data[0] + i * picture->linesize[0], c->width);
		pTemp += c->width;
	}
	for(i=0; i<c->height/2; i++) {
		memcpy(pTemp, picture->data[2] + i * picture->linesize[2], c->width/2);
		pTemp += c->width/2;
	}
	for(i=0; i<c->height/2; i++) {
		memcpy(pTemp, picture->data[1] + i * picture->linesize[1], c->width/2);
		pTemp += c->width/2;
	}
	
	PutIntoRecvQueue(rtptimestamp, pDst, c->width * c->height + (c->width * c->height >> 1));// to free(pDst);
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void FF263RecvStart(int nBasePort, DWORD dwDestIp/* Network byte order */, int nDestPort, UDPSOCKET udpSock, SOCKET fhandle, SOCKET_TYPE type)
{
	if (!m_bStartPlay)
	{
		m_bStartPlay = TRUE;

#if LOG_MSG
		log_msg("FF263RecvStart(...)\n", LOG_LEVEL_DEBUG);
#endif
		
		colorspace_init();
		
		//InitializeCriticalSection(&m_secQueue);
		pthread_mutex_init(&m_secQueue, NULL);
		m_pRecvQueue = NULL;
		m_ulLastSeqNum = 0xffffffffUL;

		m_avgFrameTime = 1000000;//Fake Value
		m_nWidth = 0;
		m_nHeight = 0;
		
		m_bVflip = FALSE;
		
		
#ifdef ANDROID_NDK
		avcodec_init(); 
		avcodec_register_all();
#else
		av_register_all();
#endif
		
#ifdef ANDROID_NDK
		m_avCodec = avcodec_find_decoder(CODEC_ID_H263);
		m_avCodecContext = avcodec_alloc_context();
		m_avFrame   = avcodec_alloc_frame();
		avcodec_open(m_avCodecContext, m_avCodec);
#else
		m_avCodec = avcodec_find_decoder(CODEC_ID_H263);
		m_avCodecContext = avcodec_alloc_context3(m_avCodec);
		m_avFrame   = avcodec_alloc_frame();
		avcodec_open2(m_avCodecContext, m_avCodec, NULL);
#endif
		
		
		T_RTPPARAM tRtpParam;
		tRtpParam.nBasePort = nBasePort;
		tRtpParam.dwDestIp = dwDestIp;
		tRtpParam.nDestPort = nDestPort;
		tRtpParam.udpSock = udpSock;
		tRtpParam.fhandle = fhandle;
		tRtpParam.type = type;
		FakeRtpRecv_init(&tRtpParam);
		
		g_bExitRecvDataProc = FALSE;
		pthread_create(&g_hRecvDataThread, NULL, FF263RecvData, NULL);
	}
}

void FF263RecvStop(void)
{
	if (m_bStartPlay)
	{
		m_bStartPlay = FALSE;
		
#if LOG_MSG
		log_msg("FF263RecvStop()\n", LOG_LEVEL_DEBUG);
#endif

		if (0 != g_hRecvDataThread)
		{
			g_bExitRecvDataProc = TRUE;
			//Waiting...
			pthread_join(g_hRecvDataThread, NULL);
			g_hRecvDataThread = 0;
		}
		
		FakeRtpRecv_uninit();
		
		FreeRecvQueue();
		//DeleteCriticalSection(&m_secQueue);
		pthread_mutex_destroy(&m_secQueue);
		
		if (m_avCodecContext != NULL)
	    {
	        avcodec_close(m_avCodecContext);
	        av_free(m_avCodecContext);
	        m_avCodecContext = NULL;
			av_free(m_avFrame);
			m_avFrame = NULL;
	    }
	}
}

void FF263RecvSetVflip(void)
{
	if (m_bStartPlay)
	{
		m_bVflip = !m_bVflip;
	}
}

//
// Return values:
// -1: Error
//  0: No data
//  n: Length of data
//
int FF263RecvGetData(VIDEO_MEDIASUBTYPE videoType, unsigned char *Buff, int bufSize, int *pWidth, int *pHeight, int *pTimePerFrame, BOOL *pFull, DWORD *pTimeStamp)
{
	if (NULL == Buff || bufSize < 0 || NULL == pWidth || NULL == pHeight || NULL == pTimePerFrame || NULL == pFull || NULL == pTimeStamp)
	{
		return -1;
	}
	
	if (!m_bStartPlay)
	{
		return -1;
	}
	
	
	*pWidth  = m_nWidth;
	*pHeight = m_nHeight;
	*pTimePerFrame = (int)(m_avgFrameTime/10000); /* -> ms */
	
	if (0 == m_nWidth || 0 == m_nHeight)
	{
		usleep(5000);
		return 0;//Wait...
	}
	
	
	int  lDataLength = 0;
	wps_queue *temp;
	RECV_DATA_NODE *p;
	
	//EnterCriticalSection(&m_secQueue);
	pthread_mutex_lock(&m_secQueue);

	if (NULL != m_pRecvQueue && wps_count_que(m_pRecvQueue) > FF263_RECV_QUEUE_MAX_NODES*3/4) {
		*pFull = TRUE;
	}
	else {
		*pFull = FALSE;
	}

	if (m_pRecvQueue)
	{
#if 0//LOG_MSG
		char szLogBuff[256];
		sprintf(szLogBuff, "FF263RecvGetData: RecvQueue Nodes = %ld\n", wps_count_que(m_pRecvQueue));
		log_msg(szLogBuff, LOG_LEVEL_DEBUG);
#endif

		temp = m_pRecvQueue;
		p = (RECV_DATA_NODE *)get_qbody(temp);

		int plane = m_nWidth * m_nHeight;
		BYTE *pSrc = p->pDataPtr;
		BYTE *pDst = Buff;
		
		if (videoType == VIDEO_MEDIASUBTYPE_RGB32X) {
			lDataLength = m_nWidth * m_nHeight * 4;
			if (bufSize >= lDataLength) 
			{
				yv12_to_rgba_c(pDst, m_nWidth * 4, 
								pSrc, /* Y */
								pSrc + plane + (plane >> 2), /* V */ 
								pSrc + plane, /* U */ 
								m_nWidth, (m_nWidth >> 1), 
								m_nWidth, m_nHeight, m_bVflip);
			}
		}
		else if (videoType == VIDEO_MEDIASUBTYPE_RGB32) {
			lDataLength = m_nWidth * m_nHeight * 4;
			if (bufSize >= lDataLength) 
			{
				yv12_to_bgra_c(pDst, m_nWidth * 4, 
							   pSrc, /* Y */
							   pSrc + plane + (plane >> 2), /* V */ 
							   pSrc + plane, /* U */ 
							   m_nWidth, (m_nWidth >> 1), 
							   m_nWidth, m_nHeight, m_bVflip);
			}
		}
		else if (videoType == VIDEO_MEDIASUBTYPE_RGB24) {
			lDataLength = m_nWidth * m_nHeight * 3;
			if (bufSize >= lDataLength) 
			{
				yv12_to_bgr_c(pDst, m_nWidth * 3, 
								pSrc, /* Y */
								pSrc + plane + (plane >> 2), /* V */ 
								pSrc + plane, /* U */ 
								m_nWidth, (m_nWidth >> 1), 
								m_nWidth, m_nHeight, m_bVflip);
			}
		}
		else if (videoType == VIDEO_MEDIASUBTYPE_RGB565) {
			lDataLength = m_nWidth * m_nHeight * 2;
			if (bufSize >= lDataLength) 
			{
				yv12_to_rgb565_c(pDst, m_nWidth * 2, 
								pSrc, /* Y */
								pSrc + plane + (plane >> 2), /* V */ 
								pSrc + plane, /* U */ 
								m_nWidth, (m_nWidth >> 1), 
								m_nWidth, m_nHeight, m_bVflip);
			}
		}
		else if (videoType == VIDEO_MEDIASUBTYPE_RGB555) {
			lDataLength = m_nWidth * m_nHeight * 2;
			if (bufSize >= lDataLength) 
			{
				yv12_to_rgb555_c(pDst, m_nWidth * 2, 
								pSrc, /* Y */
								pSrc + plane + (plane >> 2), /* V */ 
								pSrc + plane, /* U */ 
								m_nWidth, (m_nWidth >> 1), 
								m_nWidth, m_nHeight, m_bVflip);
			}
		}
		
		*pTimeStamp = p->rtptimestamp;
		
		wps_remove_que(&m_pRecvQueue, temp);
		p = (RECV_DATA_NODE *)get_qbody(temp);
		free(p->pDataPtr);
		free(p);
	}

	//LeaveCriticalSection(&m_secQueue);
	pthread_mutex_unlock(&m_secQueue);


	if (lDataLength == 0) {
		usleep(5000);
	}
	
	return lDataLength;
}
