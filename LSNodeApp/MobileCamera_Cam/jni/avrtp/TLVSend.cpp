#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "platform.h"
#include "CommonLib.h"
#include "FakeRtpSend.h"
#include "TLVSend.h"

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
static BYTE m_bTLVNri = 0;

static int m_tlv_values[TLV_TYPE_COUNT];


void TLVSendStart(int nBasePort, DWORD dwDestIp/* Network byte order */, int nDestPort, UDPSOCKET udpSock, SOCKET fhandle, SOCKET_TYPE type)
{
	if (!m_bStarted)
	{
		m_bStarted = TRUE;

#if LOG_MSG
		log_msg("TLVSendStart(...)\n", LOG_LEVEL_DEBUG);
#endif
		m_bTLVNri = 0;
		
		for( int i = 0; i < TLV_TYPE_COUNT; i++)
		{
			m_tlv_values[i] = TLV_INVALID_VALUE;
		}
		
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

void TLVSendStop(void)
{
	if (m_bStarted)
	{
		m_bStarted = FALSE;
		
#if LOG_MSG
		log_msg("TLVSendStop()\n", LOG_LEVEL_DEBUG);
#endif
		
		FakeRtpSend_uninit();
	}
}

void TLVSendSetRedundance(BOOL bValue)
{
	if (bValue) {
		m_bTLVNri = 2;
	}
	else {
		m_bTLVNri = 0;
	}
}

void TLVSendUpdateValue(WORD wType, double dfValue)
{
	if (wType < TLV_TYPE_COUNT)
	{
		m_tlv_values[wType] = (int)(dfValue * (double)TLV_VALUE_TIMES);
	}
}

//
// Return values:
// -1: Error
//  0: OK
//
int TLVSendData(void)
{
	int i;
	int ret;
	unsigned char *pSourceBuffer = NULL;
	int lSourceSize = 0;
	
	if (!m_bStarted)
	{
		return -1;
	}
	
	
	pSourceBuffer = (unsigned char *)malloc(8 * TLV_TYPE_COUNT);
	
	for (i = 0; i < TLV_TYPE_COUNT; i++)
	{
		if (m_tlv_values[i] != TLV_INVALID_VALUE)
		{
			pf_set_word(pSourceBuffer + lSourceSize, htons(i));
			pf_set_word(pSourceBuffer + lSourceSize + 2, htons(4));
			pf_set_dword(pSourceBuffer + lSourceSize + 4, htonl(m_tlv_values[i]));
			lSourceSize += 8;
		}
	}
	
	if (0 == lSourceSize)
	{
		free(pSourceBuffer);
		return 0;
	}
	
	/* ²Î¿´ AnyPC, DShowAV.cpp, set_audio_record_buffer(pAudioSrc, 160*4, 6), 40ms, 40bytes */
	ret = FakeRtpSend_sendpacket(0, pSourceBuffer, lSourceSize, PAYLOAD_TLV, 0, m_bTLVNri);
	
	free(pSourceBuffer);
	return ret;
}
