#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "platform.h"
#include "CommonLib.h"
#include "wps_queue.h"
#include "colorspace.h"
#include "FF264Recv.h"

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
static wps_queue *	m_pFuQueue; /* Only accessed by RecvData thread */
static int          m_nFuDataLength;
static DWORD	m_ulLastSeqNum;

static AVCodec *m_avCodec;			      // Codec
static AVCodecContext *m_avCodecContext;  // Codec Context
static AVFrame *m_avFrame;		          // Frame

static long long m_avgFrameTime = 0;
static int m_nWidth = 0;
static int m_nHeight = 0;

static BOOL m_bVflip = FALSE;


static BYTE g_StartCode3[] = {0x00, 0x00, 0x01};
static BYTE g_StartCode4[] = {0x00, 0x00, 0x00, 0x01};


static void OnReceiveRtpPacket(DWORD rtptimestamp, BYTE * pcData, int nLen, BOOL bMarker, unsigned short nSeqNum);
static BYTE CheckNALUType(BYTE bNALUHdr);
static BYTE CheckNALUNri(BYTE bNALUHdr);
static BOOL PutIntoRecvQueue(DWORD rtptimestamp, BYTE *lpData, int nLength);
static void FreeRecvQueue();

static BOOL PutIntoFuQueue(BYTE *lpData, int nLength, BOOL bMarker);
static BOOL MergeFuQueue(DWORD rtptimestamp, BYTE bNALUhdr);
static void FreeFuQueue();

static void FF264DecReceive(DWORD rtptimestamp, BYTE *lpData, int nLength);
static void FF264DecSendSample(DWORD rtptimestamp, AVCodecContext *c, AVFrame *picture);



static pthread_t g_hRecvDataThread = 0;
static BOOL g_bExitRecvDataProc	= FALSE;

void *FF264RecvData(void *lpParameter)
{
	int  nPayloadLen;
	WORD wSeqNum;
	DWORD dwTimeStamp;

	RECV_PACKET_SMALL *pack;
	int ret;
	unsigned char *buff;


#if LOG_MSG
	log_msg("FF264RecvData thread start...\n", LOG_LEVEL_DEBUG);
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
	log_msg("FF264RecvData thread exit!\n", LOG_LEVEL_DEBUG);
#endif

	return 0;
}



static void OnReceiveRtpPacket(DWORD rtptimestamp, BYTE * pcData, int nLen, BOOL bMarker, unsigned short nSeqNum)
{
	BYTE bType = CheckNALUType(pcData[0]);
	int nParsedLen;
	int nNALUsize;
	BYTE bFuHdr;
	BYTE bOrigNALUhdr;
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
		sprintf(szMsg, "##### FF264Recv: RTP old packet. m_ulLastSeqNum=%lu, nSeqNum=%u\n", m_ulLastSeqNum, nSeqNum);
		log_msg(szMsg, LOG_LEVEL_DEBUG);
#endif
		m_ulLastSeqNum = nSeqNum;//return;
	}

	if (bPacketLost) {
#if LOG_MSG
		char szMsg[256];
		sprintf(szMsg, "!!!!! FF264Recv: RTP packet lost. m_ulLastSeqNum=%lu, nSeqNum=%u\n", m_ulLastSeqNum, nSeqNum);
		log_msg(szMsg, LOG_LEVEL_DEBUG);
#endif
		m_ulLastSeqNum = nSeqNum;
		if (m_pFuQueue) {
			FreeFuQueue();
		}
	}

	if (bType == 30) { /* Video Format */
		if (m_pFuQueue) {
			FreeFuQueue();
		}
		if (nLen == 17 && (m_nWidth == 0 || m_nHeight == 0)) {
			m_nWidth   = ntohl( pf_get_dword(pcData + 1) );
			m_nHeight  = ntohl( pf_get_dword(pcData + 5) );
			m_avgFrameTime = (long long)ntohl(pf_get_dword(pcData + 9))  |  ((long long)ntohl(pf_get_dword(pcData + 13)) << 32);
#if LOG_MSG
			char szMsg[256];
			sprintf(szMsg, "Video Format received: width=%ld, height=%ld, m_avgFrameTime=%ld\n", m_nWidth, m_nHeight, m_avgFrameTime);
			log_msg(szMsg, LOG_LEVEL_DEBUG);
#endif
			if (m_avCodecContext != NULL)
		    {
		        avcodec_close(m_avCodecContext);
		        av_free(m_avCodecContext);
		        m_avCodecContext = NULL;
				av_free(m_avFrame);
				m_avFrame = NULL;
		    }
#ifdef ANDROID_NDK
			m_avCodec = avcodec_find_decoder(CODEC_ID_H264);
			m_avCodecContext = avcodec_alloc_context();
			m_avFrame   = avcodec_alloc_frame();
			avcodec_open(m_avCodecContext, m_avCodec);
#else
			m_avCodec = avcodec_find_decoder(CODEC_ID_H264);
			m_avCodecContext = avcodec_alloc_context3(m_avCodec);
			m_avFrame   = avcodec_alloc_frame();
			avcodec_open2(m_avCodecContext, m_avCodec, NULL);
#endif
			if (m_pFuQueue) {
				FreeFuQueue();
			}
		}
	}
	else if (bType >= 1 && bType <= 23) { /* Single NALU per RTP packet */
		if (m_pFuQueue) {
			FreeFuQueue();
		}
		FF264DecReceive(rtptimestamp, pcData, nLen);
	}
	else if (bType == 24) { /* STAP-A */
		if (m_pFuQueue) {
			FreeFuQueue();
		}
		nParsedLen = 1;
		while(nParsedLen < nLen) {
			nNALUsize = ntohs(pf_get_word(pcData + nParsedLen));
			nParsedLen += 2;
			if (nNALUsize > 0) {
				FF264DecReceive(rtptimestamp, pcData + nParsedLen, nNALUsize);
				nParsedLen += nNALUsize;
			}
		}
	}
	else if (bType == 28) { /* FU-A */
		bFuHdr = pcData[1];
		if (bFuHdr & 0x80) {/* Start fragment */
			if (m_pFuQueue) {
				FreeFuQueue();
			}
#if 0//LOG_MSG
			log_msg("FU-A Start.........\n", LOG_LEVEL_DEBUG);
#endif
			PutIntoFuQueue(pcData + 2, nLen - 2, FALSE);
		}
		else if (!(bFuHdr & 0x80) && !(bFuHdr & 0x40)) {
			if (m_pFuQueue) {
#if 0//LOG_MSG
				log_msg("FU-A Middle+++++++++\n", LOG_LEVEL_DEBUG);
#endif
				PutIntoFuQueue(pcData + 2, nLen - 2, FALSE);
			}
		}
		else if (bFuHdr & 0x40) {/* End fragment */
			if (m_pFuQueue) {
#if 0//LOG_MSG
				log_msg("FU-A End !!!!!!!!!!!!\n", LOG_LEVEL_DEBUG);
#endif
				PutIntoFuQueue(pcData + 2, nLen - 2, TRUE);
				bOrigNALUhdr = (pcData[0] & 0x60) | (pcData[1] & 0x1f);
				MergeFuQueue(rtptimestamp, bOrigNALUhdr);
			}
		}
		else {
			;//error
		}
	}
	else {
		;//error
	}
}

static BYTE CheckNALUType(BYTE bNALUHdr)
{
	return ((bNALUHdr & 0x1f) >> 0);
}

static BYTE CheckNALUNri(BYTE bNALUHdr)
{
	return ((bNALUHdr & 0x60) >> 5);
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
	if (count > FF264_RECV_QUEUE_MAX_NODES) {
#if LOG_MSG
		log_msg("FF264 decoding queue full!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", LOG_LEVEL_DEBUG);
#endif
		usleep(2*1000);//给播放线程留出CPU
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

/* FuQueue中每个节点都是原始NALU的负载数据片段 */
static BOOL PutIntoFuQueue(BYTE *lpData, int nLength, BOOL bMarker)
{
	BYTE *pBuf;
	RECV_DATA_NODE *p;

#if 0//LOG_MSG
	log_msg("PutIntoFuQueue\n", LOG_LEVEL_DEBUG);
#endif

	pBuf = (BYTE *)malloc(nLength);
	if (!pBuf) {
		return FALSE;
	}

	p = (RECV_DATA_NODE *)malloc(sizeof(RECV_DATA_NODE));
	if (!p) {
		free(pBuf);
		return FALSE;
	}

	wps_init_que(&p->link, (void *)p);
	memcpy(pBuf, lpData, nLength);
	p->pDataPtr = pBuf;
	p->nDataSize = nLength;
	p->bMarker = bMarker;

	wps_append_que(&m_pFuQueue, &p->link);
	m_nFuDataLength += nLength;
	return TRUE;
}

static BOOL MergeFuQueue(DWORD rtptimestamp, BYTE bNALUhdr)
{
	BYTE *pBuf;
	int   nCopiedLength = 0;
	wps_queue *temp;
	RECV_DATA_NODE *p;

#if 0//LOG_MSG
	log_msg("MergeFuQueue\n", LOG_LEVEL_DEBUG);
#endif

	pBuf = (BYTE *)malloc(m_nFuDataLength + 1);
	if (!pBuf) {
		return FALSE;
	}

	*pBuf = bNALUhdr;

	while (m_pFuQueue) {
		temp = m_pFuQueue;
		wps_remove_que(&m_pFuQueue, temp);
		p = (RECV_DATA_NODE *)get_qbody(temp);

		memcpy(pBuf + 1 + nCopiedLength, p->pDataPtr, p->nDataSize);
		nCopiedLength += p->nDataSize;
		m_nFuDataLength -= p->nDataSize;

		free(p->pDataPtr);
		free(p);
	}
	assert(m_nFuDataLength == 0);

	FF264DecReceive(rtptimestamp, pBuf, nCopiedLength + 1);

	free(pBuf);
	return TRUE;
}

static void FreeFuQueue()
{
	wps_queue *temp;
	RECV_DATA_NODE *p;

	while (m_pFuQueue) {
		temp = m_pFuQueue;
		wps_remove_que(&m_pFuQueue, temp);
		p = (RECV_DATA_NODE *)get_qbody(temp);
		free(p->pDataPtr);
		free(p);
	}
	m_nFuDataLength = 0;

#if LOG_MSG
	log_msg("FreeFuQueue() OK!\n", LOG_LEVEL_DEBUG);
#endif
}

static void FF264DecReceive(DWORD rtptimestamp, BYTE *lpData, int nLength)
{
    BYTE* pSrc;
    int lSrcSize;
    
	if (NULL == m_avCodecContext)
	{
		return;
	}
    
	pSrc = (BYTE *)malloc(nLength + sizeof(g_StartCode4));
	if (NULL == pSrc)
	{
		return;
	}
	
	memcpy(pSrc, g_StartCode4, sizeof(g_StartCode4));
	memcpy(pSrc + sizeof(g_StartCode4), lpData, nLength);
	lSrcSize = nLength + sizeof(g_StartCode4);
	
	
#if 0//LOG_MSG
    char szMsg[MAX_PATH];
    sprintf(szMsg, "FF264DecReceive(%d bytes)...\n", lSrcSize);
	log_msg(szMsg, LOG_LEVEL_DEBUG);
#endif
	
	
	int got_picture, consumed_bytes;
	AVPacket packet = {0};
	packet.data = pSrc;
	packet.size = lSrcSize;
	
	consumed_bytes = avcodec_decode_video2(m_avCodecContext, m_avFrame, &got_picture, &packet);
	if(consumed_bytes > 0 && got_picture)
	{
		FF264DecSendSample(rtptimestamp, m_avCodecContext, m_avFrame);
	}
	
	free(pSrc);
}

static void FF264DecSendSample(DWORD rtptimestamp, AVCodecContext *c, AVFrame *picture)
{
    int	i;
    BYTE *p, *pDst, *pTemp;
	
#if 0//LOG_MSG
	log_msg("FF264DecSendSample()...\n", LOG_LEVEL_DEBUG);
#endif
	
	if (c->width != m_nWidth || c->height != m_nHeight)
	{
		log_msg("FF264DecSendSample():############# Frame size not match!!!\n", LOG_LEVEL_DEBUG);
#ifdef ANDROID_NDK
		__android_log_print(ANDROID_LOG_INFO, "avrtp", "%dx%d -> %dx%d", m_nWidth, m_nHeight, c->width, c->height);
#endif////Debug
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

void FF264RecvStart(int nBasePort, DWORD dwDestIp/* Network byte order */, int nDestPort, UDPSOCKET udpSock, SOCKET fhandle, SOCKET_TYPE type)
{
	if (!m_bStartPlay)
	{
		m_bStartPlay = TRUE;

#if LOG_MSG
		log_msg("FF264RecvStart(...)\n", LOG_LEVEL_DEBUG);
#endif
		
		colorspace_init();
		
		//InitializeCriticalSection(&m_secQueue);
		pthread_mutex_init(&m_secQueue, NULL);
		m_pRecvQueue = NULL;
		m_pFuQueue   = NULL;
		m_nFuDataLength = 0;
		m_ulLastSeqNum = 0xffffffffUL;

		m_avgFrameTime = 0;
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
		m_avCodec = avcodec_find_decoder(CODEC_ID_H264);
		m_avCodecContext = avcodec_alloc_context();
		m_avFrame   = avcodec_alloc_frame();
		avcodec_open(m_avCodecContext, m_avCodec);
#else
		m_avCodec = avcodec_find_decoder(CODEC_ID_H264);
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
		pthread_create(&g_hRecvDataThread, NULL, FF264RecvData, NULL);
	}
}

void FF264RecvStop(void)
{
	if (m_bStartPlay)
	{
		m_bStartPlay = FALSE;
		
#if LOG_MSG
		log_msg("FF264RecvStop()\n", LOG_LEVEL_DEBUG);
#endif

		if (0 != g_hRecvDataThread)
		{
			g_bExitRecvDataProc = TRUE;
			//Waiting...
			pthread_join(g_hRecvDataThread, NULL);
			g_hRecvDataThread = 0;
		}
		
		FakeRtpRecv_uninit();
		
		FreeFuQueue();
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

void FF264RecvSetVflip(void)
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
int FF264RecvGetData(VIDEO_MEDIASUBTYPE videoType, unsigned char *Buff, int bufSize, int *pWidth, int *pHeight, int *pTimePerFrame, BOOL *pFull, DWORD *pTimeStamp)
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

	if (NULL != m_pRecvQueue && wps_count_que(m_pRecvQueue) > FF264_RECV_QUEUE_MAX_NODES*3/4) {
		*pFull = TRUE;
	}
	else {
		*pFull = FALSE;
	}

	if (m_pRecvQueue)
	{
#if 0//LOG_MSG
		char szLogBuff[256];
		sprintf(szLogBuff, "FF264RecvGetData: RecvQueue Nodes = %ld\n", wps_count_que(m_pRecvQueue));
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

