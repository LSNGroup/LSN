#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "platform.h"
#include "CommonLib.h"
#include "FakeRtpRecv.h"
#include "TLVRecv.h"

#ifndef ANDROID_NDK
#include <float.h>
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
static pthread_mutex_t m_secQueue; /* To protect TLV values */
static int m_tlv_values[TLV_TYPE_COUNT];
static DWORD	m_ulLastSeqNum;
static int m_miniSecPeriod = 1000;



//
// Return values:
// -1: Old packet
//  0: Normal
//  1: Packet lost
//
static int CheckRtpPacket(BYTE *pcData, int nLen, BOOL bMarker, unsigned short nSeqNum);


static pthread_t g_hRecvTLVDataThread = 0;
static BOOL g_bExitRecvTLVDataProc	= FALSE;

void *RecvTLVData(void *lpParameter)
{
	int  nPayloadLen;
	WORD wSeqNum;
	BYTE bReserved;

	RECV_PACKET_SMALL *pack;
	int ret;
	unsigned char *buff;
	unsigned char *tmp;
	WORD wType;
	WORD wLength;

#if LOG_MSG
	log_msg("RecvTLVData thread start...\n", LOG_LEVEL_DEBUG);
#endif

	while (1)
	{
		if (g_bExitRecvTLVDataProc) {
			break;
		}

		if ((ret = FakeRtpRecv_getpacket(&pack, PAYLOAD_TLV)) > 0)
		{
			// You can examine the data here
			bReserved    = pack->bReserved;
			wSeqNum      = pack->wSeqNum;
			buff         = pack->szData + 8;
			nPayloadLen  = pack->nDataSize - 8;

			if (CheckRtpPacket(buff,nPayloadLen,true,wSeqNum) != -1)
			{
				pthread_mutex_lock(&m_secQueue);////////
				
				tmp = buff;
				while (buff + nPayloadLen > tmp)
				{
					wType = ntohs( pf_get_word(tmp) );
					wLength = ntohs( pf_get_word(tmp+2) );
					if (wType < TLV_TYPE_COUNT && wLength == 4)
					{
						m_tlv_values[wType] = ntohl( pf_get_dword(tmp+4) );
					}
                    else
                    {
#if LOG_MSG
						char szMsg[256];
						sprintf(szMsg, "RecvTLVData: Invalid packet!!! nPayloadLen=%d, wType=%d, wLength=%d\n", nPayloadLen, wType, wLength);
						log_msg(szMsg, LOG_LEVEL_DEBUG);
#endif
                    }
					tmp += 8;
				}
				
				pthread_mutex_unlock(&m_secQueue);////////
			}

			// we don't longer need this packet, so we'll free it
			free(pack);
		}

		if (ret < 0) {
			;//error
		}
		
		usleep(m_miniSecPeriod * 1000);
	}

#if LOG_MSG
	log_msg("RecvTLVData thread exit!\n", LOG_LEVEL_DEBUG);
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
		return -1;
	}

	if (bPacketLost) {
#if LOG_MSG
		log_msg("!!!!!!!!!!!!! RTP packet lost.\n", LOG_LEVEL_DEBUG);
#endif
		return 1;
	}

	return 0;
}


void TLVRecvStart(int nBasePort, DWORD dwDestIp/* Network byte order */, int nDestPort, UDPSOCKET udpSock, SOCKET fhandle, SOCKET_TYPE type)
{
	if (!m_bStartPlay)
	{
		m_bStartPlay = TRUE;
		
#if LOG_MSG
		log_msg("TLVRecvStart(...)\n", LOG_LEVEL_DEBUG);
#endif
		
		//InitializeCriticalSection(&m_secQueue);
		pthread_mutex_init(&m_secQueue, NULL);
		for( int i = 0; i < TLV_TYPE_COUNT; i++)
		{
			m_tlv_values[i] = TLV_INVALID_VALUE;
		}
		m_ulLastSeqNum = 0xffffffffUL;
		
		T_RTPPARAM tRtpParam;
		tRtpParam.nBasePort = nBasePort;
		tRtpParam.dwDestIp = dwDestIp;
		tRtpParam.nDestPort = nDestPort;
		tRtpParam.udpSock = udpSock;
		tRtpParam.fhandle = fhandle;
		tRtpParam.type = type;
		FakeRtpRecv_init(&tRtpParam);
		
		g_bExitRecvTLVDataProc = FALSE;
		pthread_create(&g_hRecvTLVDataThread, NULL, RecvTLVData, NULL);
	}
}


void TLVRecvStop(void)
{
	if (m_bStartPlay)
	{
		m_bStartPlay = FALSE;
		
#if LOG_MSG
		log_msg("TLVRecvStop()\n", LOG_LEVEL_DEBUG);
#endif
		
		if (0 != g_hRecvTLVDataThread)
		{
			g_bExitRecvTLVDataProc = TRUE;
			//Waiting...
			pthread_join(g_hRecvTLVDataThread, NULL);
			g_hRecvTLVDataThread = 0;
		}
		
		FakeRtpRecv_uninit();
		
		//DeleteCriticalSection(&m_secQueue);
		pthread_mutex_destroy(&m_secQueue);
	}
}


void TLVRecvSetPeriod(int miniSec)
{
	m_miniSecPeriod = miniSec;
}


double TLVRecvGetData(WORD wType)
{
	int lValue = TLV_INVALID_VALUE;
	
	if (wType >= TLV_TYPE_COUNT)
	{
		return DBL_MAX;
	}
	
	if (!m_bStartPlay)
	{
		return DBL_MAX;
	}
	
	
	//EnterCriticalSection(&m_secQueue);
	pthread_mutex_lock(&m_secQueue);
	
	lValue = m_tlv_values[wType];
	
	//LeaveCriticalSection(&m_secQueue);
	pthread_mutex_unlock(&m_secQueue);
	
	if (lValue == TLV_INVALID_VALUE) {
		return DBL_MAX;
	}
	else {
		return ((double)lValue / (double)TLV_VALUE_TIMES);
	}
}
