#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "platform.h"
#include "CommonLib.h"
#include "wps_queue.h"
#include "FakeRtpSend.h"
#include "H264Send.h"

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


static BOOL m_bStarted = FALSE;
static BOOL m_bVideoReliable;

static long long m_avgFrameTime = 0;
static int m_nWidth = 0;
static int m_nHeight = 0;


static 	void ReceiveNALU(DWORD rtptimestamp, BYTE *pSourceBuffer, int lSourceSize);
static 	BYTE CheckNALUType(BYTE bNALUHdr);
static 	BYTE CheckNALUNri(BYTE bNALUHdr);
static 	BOOL SendCurrent(DWORD rtptimestamp, BYTE *lpData, int nLength);
static 	BOOL SendCurrentFrag(DWORD rtptimestamp, BYTE *lpData, int nLength);


#define TRANSPORT_LAYER_MSS		(512 - 8 - 1)

static BYTE g_StartCode3[] = {0x00, 0x00, 0x01};
static BYTE g_StartCode4[] = {0x00, 0x00, 0x00, 0x01};


static void ReceiveNALU(DWORD rtptimestamp, BYTE *pSourceBuffer, int lSourceSize)
{
	/* NALU without StartCode */

	assert(pSourceBuffer && lSourceSize > 0);

	//BYTE bTemp = CheckNALUNri(pSourceBuffer[0]);
	//if (bTemp == 0) {
	//	return;
	//}

#if LOG_MSG
	char szLogBuf[MAX_PATH];
	sprintf(szLogBuf, "To send NALU: Length=%d, Type=%d Nri=%d\n", lSourceSize, CheckNALUType(pSourceBuffer[0]), CheckNALUNri(pSourceBuffer[0]));
	log_msg(szLogBuf, LOG_LEVEL_DEBUG);
#endif

	if (lSourceSize > TRANSPORT_LAYER_MSS &&
			CheckNALUType(pSourceBuffer[0]) <= 23) {   // 1-23  Single NAL unit packet per H.264
		// 当前的 分片
		SendCurrentFrag(rtptimestamp, pSourceBuffer, lSourceSize);
	}
	else {
		// 当前的 打成一个包发送
		SendCurrent(rtptimestamp, pSourceBuffer, lSourceSize);
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

static BOOL SendCurrent(DWORD rtptimestamp, BYTE *lpData, int nLength)
{
	assert(lpData && nLength > 0);

#if 0//LOG_MSG
	char szLogBuf[MAX_PATH];
	sprintf(szLogBuf, "SendCurrent: Length=%d\n", nLength);
	log_msg(szLogBuf, LOG_LEVEL_DEBUG);
#endif

	BYTE bOrigNRI = CheckNALUNri(lpData[0]);
	BYTE bOrigType = CheckNALUType(lpData[0]);
	BYTE bSendNri;

	if (bOrigType != NALU_TYPE_SLICE_NOPART && 
			bOrigType != NALU_TYPE_SLICE_PARTA && 
			bOrigType != NALU_TYPE_SLICE_PARTB && 
			bOrigType != NALU_TYPE_SLICE_PARTC && 
			bOrigType != NALU_TYPE_SLICE_IDR &&
			bOrigType != 28/*FU-A*/ &&
			bOrigType != 29/*FU-B*/) {
		bSendNri = 3;
	}
	else {
		bSendNri = 0;
	}

	if (m_bVideoReliable) {
		bSendNri |= FAKERTP_RELIABLE_FLAG;
	}

	if (FakeRtpSend_sendpacket(rtptimestamp, lpData, nLength, PAYLOAD_VIDEO, 0, bSendNri) < 0) {
		return FALSE;
	}
	else {
		return TRUE;
	}
}

static BOOL SendCurrentFrag(DWORD rtptimestamp, BYTE *lpData, int nLength)
{
	assert(lpData && nLength > 0);


#if 0//LOG_MSG
	char szLogBuf[MAX_PATH];
	sprintf(szLogBuf, "SendCurrentFrag: Length=%d\n", nLength);
	log_msg(szLogBuf, LOG_LEVEL_DEBUG);
#endif

	BYTE bOrigNRI = CheckNALUNri(lpData[0]);
	BYTE bOrigType = CheckNALUType(lpData[0]);
	BYTE bType = 28; /* FU-A */

	BYTE bSendBuff[TRANSPORT_LAYER_MSS];
	BYTE bSendNri;

	if (bOrigType != NALU_TYPE_SLICE_NOPART && 
			bOrigType != NALU_TYPE_SLICE_PARTA && 
			bOrigType != NALU_TYPE_SLICE_PARTB && 
			bOrigType != NALU_TYPE_SLICE_PARTC && 
			bOrigType != NALU_TYPE_SLICE_IDR &&
			bOrigType != 28/*FU-A*/ &&
			bOrigType != 29/*FU-B*/) {
		bSendNri = 3;
	}
	else {
		bSendNri = 0;
	}

	if (m_bVideoReliable) {
		bSendNri |= FAKERTP_RELIABLE_FLAG;
	}

	lpData += 1;
	nLength -= 1;

	bSendBuff[0] = (0x00 << 7) | (bOrigNRI << 5) | (bType);
	bSendBuff[1] = (0x01 << 7) | (0x00 << 6) | (bOrigType);
	memcpy(bSendBuff + 2, lpData, sizeof(bSendBuff) - 2);
	FakeRtpSend_sendpacket(rtptimestamp, bSendBuff, sizeof(bSendBuff), PAYLOAD_VIDEO, 0, bSendNri);

	lpData += sizeof(bSendBuff) - 2;
	nLength -= sizeof(bSendBuff) - 2;


	while(nLength > sizeof(bSendBuff) - 2) {

		bSendBuff[0] = (0x00 << 7) | (bOrigNRI << 5) | (bType);
		bSendBuff[1] = (0x00 << 7) | (0x00 << 6) | (bOrigType);
		memcpy(bSendBuff + 2, lpData, sizeof(bSendBuff) - 2);
		FakeRtpSend_sendpacket(rtptimestamp, bSendBuff, sizeof(bSendBuff), PAYLOAD_VIDEO, 0, bSendNri);

		lpData += sizeof(bSendBuff) - 2;
		nLength -= sizeof(bSendBuff) - 2;
	}


	bSendBuff[0] = (0x00 << 7) | (bOrigNRI << 5) | (bType);
	bSendBuff[1] = (0x00 << 7) | (0x01 << 6) | (bOrigType);
	memcpy(bSendBuff + 2, lpData, nLength);
	FakeRtpSend_sendpacket(rtptimestamp, bSendBuff, nLength + 2, PAYLOAD_VIDEO, 0, bSendNri);

	//lpData += nLength;
	//nLength -= nLength;


	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VideoSendStart(int nBasePort, DWORD dwDestIp/* Network byte order */, int nDestPort, UDPSOCKET udpSock, SOCKET fhandle, SOCKET_TYPE type)
{
	if (!m_bStarted)
	{
		m_bStarted = TRUE;

#if LOG_MSG
		log_msg("VideoSendStart(...)\n", LOG_LEVEL_DEBUG);
#endif
		m_bVideoReliable = TRUE;
		
		m_avgFrameTime = 0;
		m_nWidth = 0;
		m_nHeight = 0;
		
		T_RTPPARAM tRtpParam;
		tRtpParam.nBasePort = nBasePort;
		tRtpParam.dwDestIp = dwDestIp;
		tRtpParam.nDestPort = nDestPort;
		tRtpParam.udpSock = udpSock;
		tRtpParam.fhandle = fhandle;
		tRtpParam.type = type;
		FakeRtpSend_init(&tRtpParam);
	}
}

void VideoSendStop(void)
{
	if (m_bStarted)
	{
		m_bStarted = FALSE;
		
#if LOG_MSG
		log_msg("VideoSendStop()\n", LOG_LEVEL_DEBUG);
#endif
		
		FakeRtpSend_uninit();
	}
}

void VideoSendSetReliable(BOOL bValue)
{
	m_bVideoReliable = bValue;
}

int VideoSendSetMediaType(int width, int height, int fps)
{
	if (!m_bStarted)
	{
		return -1;
	}
	
#if LOG_MSG
	log_msg("SetMediaType\n", LOG_LEVEL_DEBUG);
#endif

	m_nWidth = width;
	m_nHeight = height;
	m_avgFrameTime = 10000000/fps;


	BYTE bSendBuff[17];
	BYTE bNRI  = 3; /* most important */
	BYTE bType = 30; /* use un-defined value */
	int i;

	bSendBuff[0] = (0x00 << 7) | (bNRI << 5) | (bType);
	pf_set_dword(bSendBuff + 1, htonl(m_nWidth));
	pf_set_dword(bSendBuff + 5, htonl(m_nHeight));
	pf_set_dword(bSendBuff + 9, htonl((DWORD)(m_avgFrameTime & 0x00000000ffffffffULL))); // lower 4 bytes
	pf_set_dword(bSendBuff + 13, htonl((DWORD)((m_avgFrameTime & 0xffffffff00000000ULL) >> 32))); // high 4 bytes

	if (m_bVideoReliable)
	{
		FakeRtpSend_sendpacket(0, bSendBuff, sizeof(bSendBuff), PAYLOAD_VIDEO, 0, bNRI | FAKERTP_RELIABLE_FLAG);
	}
	else
	{
		for (i = 0; i < 8; i++ ) {
			usleep(20000);
			FakeRtpSend_sendpacket(0, bSendBuff, sizeof(bSendBuff), PAYLOAD_VIDEO, 0, bNRI);
		}
	}
	
	return 0;
}

//
// Return values:
// -1: Error
//  0: OK
//
int VideoSendH264Data(DWORD rtptimestamp, const unsigned char *pSourceBuffer, int lSourceSize)
{
	if (!m_bStarted)
	{
		return -1;
	}
	
	//if (m_nWidth == 0 || m_nHeight == 0)
	//{
	//	return -1;
	//}
	
	if (NULL == pSourceBuffer)
	{
		return -1;
	}
	
	if (lSourceSize >= 4 && memcmp(pSourceBuffer, g_StartCode4, 4) == 0) {
		pSourceBuffer += 4;
		lSourceSize -= 4;
	}
	else if (lSourceSize >= 3 && memcmp(pSourceBuffer, g_StartCode3, 3) == 0) {
		pSourceBuffer += 3;
		lSourceSize -= 3;
	}
	
	if (lSourceSize == 0) {
		return 0;
	}
	
	ReceiveNALU(rtptimestamp, (BYTE *)pSourceBuffer, lSourceSize);
	return 0;
}

//
// Return values:
// -1: Error
//  0: OK
//
int VideoSendH263Data(DWORD rtptimestamp, const unsigned char *pSourceBuffer, int lSourceSize)
{
	BYTE bSendNri = 0;
	
	if (!m_bStarted)
	{
		return -1;
	}
	
	if (NULL == pSourceBuffer)
	{
		return -1;
	}
	
	if (m_bVideoReliable) {
		bSendNri |= FAKERTP_RELIABLE_FLAG;
	}
	
	return FakeRtpSend_sendpacket(rtptimestamp, pSourceBuffer, lSourceSize, PAYLOAD_VIDEO, 0, bSendNri);
}
