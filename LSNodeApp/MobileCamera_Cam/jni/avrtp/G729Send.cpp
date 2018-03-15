#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "CommonLib.h"
#include "FakeRtpSend.h"
#include "AudioCodec.h"
#include "G729Send.h"

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
static BYTE m_bCodec = AUDIO_CODEC_G729A;
static BYTE m_bAudioNri = 0;


void AudioSendStart(int nBasePort, DWORD dwDestIp/* Network byte order */, int nDestPort, UDPSOCKET udpSock, SOCKET fhandle, SOCKET_TYPE type)
{
	if (!m_bStarted)
	{
		m_bStarted = TRUE;

#if LOG_MSG
		log_msg("AudioSendStart(...)\n", LOG_LEVEL_DEBUG);
#endif
		m_bAudioNri = 0;
		
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

void AudioSendStop(void)
{
	if (m_bStarted)
	{
		m_bStarted = FALSE;
		
#if LOG_MSG
		log_msg("AudioSendStop()\n", LOG_LEVEL_DEBUG);
#endif
		
		FakeRtpSend_uninit();
	}
}

void AudioSendSetCodec(BYTE bCodec)
{
	m_bCodec = bCodec;
}

void AudioSendSetRedundance(BOOL bValue)
{
	if (bValue) {
		m_bAudioNri = 2;
	}
	else {
		m_bAudioNri = 0;
	}
}

//
// Return values:
// -1: Error
//  0: OK
//
int AudioSendData(DWORD rtptimestamp, const unsigned char *pSourceBuffer, int lSourceSize)
{
	if (!m_bStarted)
	{
		return -1;
	}
	
	if (NULL == pSourceBuffer)
	{
		return -1;
	}
	
	/* ²Î¿´ AnyPC, DShowAV.cpp, set_audio_record_buffer(pAudioSrc, 160*4, 6), 40ms, 40bytes */
	return FakeRtpSend_sendpacket(rtptimestamp, pSourceBuffer, lSourceSize, PAYLOAD_AUDIO, m_bCodec, m_bAudioNri);
}
