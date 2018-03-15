//#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "CommonLib.h"
#include "ControlCmd.h"

#ifdef ANDROID_NDK
#include <android/log.h>
#endif


#ifdef ANDROID_NDK
#define log_msg(msg, lev)  __android_log_print(ANDROID_LOG_INFO, "shdir", msg)
#else
#define log_msg(msg, lev)  printf(msg)
#endif



static BOOL m_bInit = FALSE;
static pthread_mutex_t m_mutexSend;


int  CtrlCmd_Init()
{
	if (m_bInit) {
		return 0;
	}
	m_bInit = TRUE;

	log_msg("CtrlCmd_Init()...\n", LOG_LEVEL_DEBUG);
	pthread_mutex_init(&m_mutexSend, NULL);
	return 0;
}

void CtrlCmd_Uninit()
{
	if (FALSE == m_bInit) {
		return;
	}
	m_bInit = FALSE;

	log_msg("CtrlCmd_Uninit()...\n", LOG_LEVEL_DEBUG);
	pthread_mutex_destroy(&m_mutexSend);
}


//#ifdef JNI_FOR_MOBILECAMERA

int CtrlCmd_Send_FAKERTP_RESP(SOCKET_TYPE type, SOCKET fhandle, BYTE *packet, int len)
{
	int ret;
	char bSendPacket[32];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_FAKERTP_RESP));
	pf_set_dword(bSendPacket + 2, htonl(len));
	
	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6);
	ret += SendStream(type, fhandle, (char *)packet, len);
	if (ret != 0) {
		ret = -1;
	}
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_Send_HELLO_RESP(SOCKET_TYPE type, SOCKET fhandle, BYTE *server_node_id, DWORD server_version, BYTE func_flags, WORD result_code)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_HELLO_RESP));
	pf_set_dword(bSendPacket + 2, htonl(6+4+1+2));
	memcpy(bSendPacket + 6, server_node_id, 6);
	pf_set_dword(bSendPacket + 12, htonl(server_version));
	*((BYTE *)bSendPacket + 16) = func_flags;
	pf_set_word(bSendPacket + 17, htons(result_code));

	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6 + 6 + 4 + 1 + 2);
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_Send_RUN_RESP(SOCKET_TYPE type, SOCKET fhandle, const char *result_str)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];
	int len = strlen(result_str) + 1;

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_RUN_RESP));
	pf_set_dword(bSendPacket + 2, htonl(len));
	
	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6);
	ret += SendStream(type, fhandle, (char *)result_str, len);
	if (ret != 0) {
		ret = -1;
	}
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_Send_MAV_WP_RESP(SOCKET_TYPE type, SOCKET fhandle, WP_ITEM *wp_data, int wp_num)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_MAV_WP_RESP));
	pf_set_dword(bSendPacket + 2, htonl(sizeof(WP_ITEM)*wp_num));
	
	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6);
	for (int i = 0; i < wp_num; i++)
	{
		pf_set_dword(bSendPacket + 0,  htonl(wp_data[i].wpFlags));
		pf_set_dword(bSendPacket + 4,  htonl(wp_data[i].wpType));
		pf_set_dword(bSendPacket + 8,  htonl(wp_data[i].wpLati));
		pf_set_dword(bSendPacket + 12, htonl(wp_data[i].wpLongi));
		pf_set_dword(bSendPacket + 16, htonl(wp_data[i].wpAlti));
		ret += SendStream(type, fhandle, bSendPacket, 20);
	}
	if (ret != 0) {
		ret = -1;
	}
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_Send_MAV_TLV_RESP(SOCKET_TYPE type, SOCKET fhandle, BYTE *tlv_data, int len)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_MAV_TLV_RESP));
	pf_set_dword(bSendPacket + 2, htonl(len));
	
	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6);
	ret += SendStream(type, fhandle, (char *)tlv_data, len);
	if (ret != 0) {
		ret = -1;
	}
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_Send_NULL(SOCKET_TYPE type, SOCKET fhandle)
{
	int ret;
	char bSendPacket[32];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_NULL));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6);
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_Send_END(SOCKET_TYPE type, SOCKET fhandle)
{
	int ret;
	char bSendPacket[32];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_END));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6);
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

void CtrlCmd_Recv_AV_END(SOCKET_TYPE type, SOCKET fhandle)
{
	char buff[6];
	int ret;
	WORD wCommand;
	DWORD dwDataLength;
	
	while (TRUE)
	{
		ret = RecvStream(type, fhandle, buff, 6);
		if (ret != 0) {
			break;
		}
		wCommand = ntohs(pf_get_word(buff+0));
		dwDataLength = ntohl(pf_get_dword(buff+2));
		
		if (CMD_CODE_END == wCommand)
		{
			log_msg("CtrlCmd_Recv_AV_END(): CMD_CODE_END!!!!!!!!\n", LOG_LEVEL_DEBUG);
			break;
		}
		else /* if (CMD_CODE_FAKERTP_RESP == wCommand) */
		{
			if (CMD_CODE_FAKERTP_RESP == wCommand) log_msg("CtrlCmd_Recv_AV_END(): CMD_CODE_FAKERTP_RESP...\n", LOG_LEVEL_DEBUG);
			else                              log_msg("CtrlCmd_Recv_AV_END(): CMD_CODE_???????...\n", LOG_LEVEL_DEBUG);
			
			for (int i = 0; i < dwDataLength; i++) {
				RecvStream(type, fhandle, buff, 1);
			}
		}
	}
}


//#else

int CtrlCmd_HELLO(SOCKET_TYPE type, SOCKET fhandle, BYTE *client_node_id, DWORD client_version, const char *password, BYTE *server_node_id, DWORD *server_version, BYTE *func_flags, WORD *result_code)
{
	char buff[32];
	int ret;
	WORD wCommand;
	DWORD dwDataLength;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_HELLO_REQ));
	pf_set_dword(bSendPacket + 2, htonl(6+4+256));
	memcpy(bSendPacket + 6, client_node_id, 6);
	pf_set_dword(bSendPacket + 12, htonl(client_version));
	strcpy(bSendPacket + 16, password);

	pthread_mutex_lock(&m_mutexSend);

	if (SendStream(type, fhandle, bSendPacket, 6+6+4+256) < 0) {
		pthread_mutex_unlock(&m_mutexSend);
		return -1;
	}

	pthread_mutex_unlock(&m_mutexSend);

	ret = RecvStream(type, fhandle, buff, 6);
	if (ret != 0) {
		return -1;
	}

	wCommand = ntohs(pf_get_word(buff));
	dwDataLength = ntohl(pf_get_dword(buff+2));

	if (wCommand != CMD_CODE_HELLO_RESP || dwDataLength != 13) {
		return -1;
	}

	ret = RecvStream(type, fhandle, buff, 6);
	if (ret != 0) {
		return -1;
	}
	if (memcmp(server_node_id, buff, 6) != 0) {
		return -1;
	}

	ret = RecvStream(type, fhandle, buff, 4);
	if (ret != 0) {
		return -1;
	}
	*server_version = ntohl(pf_get_dword(buff));

	ret = RecvStream(type, fhandle, buff, 1);
	if (ret != 0) {
		return -1;
	}
	*func_flags = buff[0];

	ret = RecvStream(type, fhandle, buff, 2);
	if (ret != 0) {
		return -1;
	}

	*result_code = ntohs(pf_get_word(buff));
	return 0;
}

int CtrlCmd_RUN(SOCKET_TYPE type, SOCKET fhandle, const char *exe_cmd, char *result_buf, int buf_size)
{
	char buff[32];
	int ret;
	WORD wCommand;
	DWORD dwDataLength;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];
	int len = strlen(exe_cmd) + 1;

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_RUN_REQ));
	pf_set_dword(bSendPacket + 2, htonl(len));

	pthread_mutex_lock(&m_mutexSend);
	
	if (SendStream(type, fhandle, bSendPacket, 6) < 0) {
		pthread_mutex_unlock(&m_mutexSend);
		return -1;
	}
	if (SendStream(type, fhandle, (char *)exe_cmd, len) < 0) {
		pthread_mutex_unlock(&m_mutexSend);
		return -1;
	}
	
	pthread_mutex_unlock(&m_mutexSend);

	ret = RecvStream(type, fhandle, buff, 6);
	if (ret != 0) {
		return -1;
	}

	wCommand = ntohs(pf_get_word(buff));
	dwDataLength = ntohl(pf_get_dword(buff+2));

	if (wCommand != CMD_CODE_RUN_RESP) {
		return -1;
	}

	if (dwDataLength > buf_size) {
		for (int i = 0; i < dwDataLength; i++) {
			RecvStream(type, fhandle, buff, 1);
		}
		return -1;
	}

	ret = RecvStream(type, fhandle, result_buf, dwDataLength);
	if (ret != 0) {
		return -1;
	}

	return 0;
}

int CtrlCmd_MAV_WP(SOCKET_TYPE type, SOCKET fhandle, WP_ITEM *wp_array, int *lpNumWP)
{
	char buff[32];
	int ret;
	WORD wCommand;
	DWORD dwDataLength;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	if (wp_array == NULL || lpNumWP == NULL) {
		return -1;
	}

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_MAV_WP_REQ));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(&m_mutexSend);
	
	if (SendStream(type, fhandle, bSendPacket, 6) < 0) {
		pthread_mutex_unlock(&m_mutexSend);
		return -1;
	}
	
	pthread_mutex_unlock(&m_mutexSend);

	ret = RecvStream(type, fhandle, buff, 6);
	if (ret != 0) {
		return -1;
	}

	wCommand = ntohs(pf_get_word(buff));
	dwDataLength = ntohl(pf_get_dword(buff+2));

	if (wCommand != CMD_CODE_MAV_WP_RESP) {
		return -1;
	}

	if (dwDataLength > (*lpNumWP) * sizeof(WP_ITEM)) {
		for (int i = 0; i < dwDataLength; i++) {
			RecvStream(type, fhandle, buff, 1);
		}
		return -1;
	}

	for (int i = 0; i < dwDataLength/sizeof(WP_ITEM); i++)
	{
		ret = RecvStream(type, fhandle, buff, sizeof(DWORD));
		if (ret != 0) {
			return -1;
		}
		wp_array[i].wpFlags = ntohl(pf_get_dword(buff));////

		ret = RecvStream(type, fhandle, buff, sizeof(DWORD));
		if (ret != 0) {
			return -1;
		}
		wp_array[i].wpType = ntohl(pf_get_dword(buff));////

		ret = RecvStream(type, fhandle, buff, sizeof(DWORD));
		if (ret != 0) {
			return -1;
		}
		wp_array[i].wpLati = ntohl(pf_get_dword(buff));////

		ret = RecvStream(type, fhandle, buff, sizeof(DWORD));
		if (ret != 0) {
			return -1;
		}
		wp_array[i].wpLongi = ntohl(pf_get_dword(buff));////

		ret = RecvStream(type, fhandle, buff, sizeof(DWORD));
		if (ret != 0) {
			return -1;
		}
		wp_array[i].wpAlti = ntohl(pf_get_dword(buff));////
	}

	*lpNumWP = dwDataLength/sizeof(WP_ITEM);
	return 0;
}

int CtrlCmd_MAV_TLV(SOCKET_TYPE type, SOCKET fhandle, BYTE *tlv_buf, int *lpLen)
{
	char buff[32];
	int ret;
	WORD wCommand;
	DWORD dwDataLength;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	if (lpLen == NULL) {
		return -1;
	}

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_MAV_TLV_REQ));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(&m_mutexSend);
	
	if (SendStream(type, fhandle, bSendPacket, 6) < 0) {
		pthread_mutex_unlock(&m_mutexSend);
		return -1;
	}
	
	pthread_mutex_unlock(&m_mutexSend);

	ret = RecvStream(type, fhandle, buff, 6);
	if (ret != 0) {
		return -1;
	}

	wCommand = ntohs(pf_get_word(buff));
	dwDataLength = ntohl(pf_get_dword(buff+2));

	if (wCommand != CMD_CODE_MAV_TLV_RESP) {
		return -1;
	}

	if (dwDataLength > *lpLen) {
		for (int i = 0; i < dwDataLength; i++) {
			RecvStream(type, fhandle, buff, 1);
		}
		return -1;
	}

	ret = RecvStream(type, fhandle, (char *)tlv_buf, dwDataLength);
	if (ret != 0) {
		return -1;
	}

	*lpLen = dwDataLength;
	return 0;
}

int CtrlCmd_PROXY(SOCKET_TYPE type, SOCKET fhandle, WORD wTcpPort)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_PROXY_REQ));
	pf_set_dword(bSendPacket + 2, htonl(2));
	pf_set_word(bSendPacket + 6, htons(wTcpPort));

	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6+2);
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_PROXY_DATA(SOCKET_TYPE type, SOCKET fhandle, BYTE *data, int len)
{//proxy data is big!!!
	int ret;
	char bSendPacket[32];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_PROXY_DATA));
	pf_set_dword(bSendPacket + 2, htonl(len));
	
	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6);
	ret += SendStream(type, fhandle, (char *)data, len);
	if (ret != 0) {
		ret = -1;
	}
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_PROXY_END(SOCKET_TYPE type, SOCKET fhandle)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_PROXY_END));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6);
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_ARM(SOCKET_TYPE type, SOCKET fhandle)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_ARM_REQ));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6);
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_DISARM(SOCKET_TYPE type, SOCKET fhandle)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_DISARM_REQ));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6);
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_AV_START(SOCKET_TYPE type, SOCKET fhandle, BYTE flags, BYTE video_size, BYTE video_framerate, DWORD audio_channel, DWORD video_channel)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_AV_START_REQ));
	pf_set_dword(bSendPacket + 2, htonl(3 + 4 + 4));
	*(BYTE *)(bSendPacket + 6) = flags;
	*(BYTE *)(bSendPacket + 7) = video_size;
	*(BYTE *)(bSendPacket + 8) = video_framerate;
	pf_set_dword(bSendPacket + 9, htonl(audio_channel));
	pf_set_dword(bSendPacket + 13, htonl(video_channel));
	
	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6 + 3 + 4 + 4);
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_AV_STOP(SOCKET_TYPE type, SOCKET fhandle)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_AV_STOP_REQ));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6);
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_AV_SWITCH(SOCKET_TYPE type, SOCKET fhandle, DWORD video_channel)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_AV_SWITCH_REQ));
	pf_set_dword(bSendPacket + 2, htonl(4));
	pf_set_dword(bSendPacket + 6, htonl(video_channel));

	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6 + 4);
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_AV_CONTRL(SOCKET_TYPE type, SOCKET fhandle, WORD contrl, DWORD contrl_param)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_AV_CONTRL_REQ));
	pf_set_dword(bSendPacket + 2, htonl(2 + 4));
	pf_set_word(bSendPacket + 6, htons(contrl));
	pf_set_dword(bSendPacket + 8, htonl(contrl_param));

	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6 + 2 + 4);
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_VOICE(SOCKET_TYPE type, SOCKET fhandle, BYTE *data, int len)
{//voice data is big!!!
	int ret;
	char bSendPacket[32];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_VOICE_REQ));
	pf_set_dword(bSendPacket + 2, htonl(len));
	
	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6);
	ret += SendStream(type, fhandle, (char *)data, len);
	if (ret != 0) {
		ret = -1;
	}
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_TEXT(SOCKET_TYPE type, SOCKET fhandle, BYTE *data, int len)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_TEXT_REQ));
	pf_set_dword(bSendPacket + 2, htonl(512));
	memcpy(bSendPacket + 6, data, len);

	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6+512);
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_BYE(SOCKET_TYPE type, SOCKET fhandle)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_BYE_REQ));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6);
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_MAV_START(SOCKET_TYPE type, SOCKET fhandle)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_MAV_START_REQ));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6);
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_MAV_STOP(SOCKET_TYPE type, SOCKET fhandle)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_MAV_STOP_REQ));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6);
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

int CtrlCmd_MAV_GUID(SOCKET_TYPE type, SOCKET fhandle, float lati, float longi, float alti)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_MAV_GUID_REQ));
	pf_set_dword(bSendPacket + 2, htonl(12));
	pf_set_dword(bSendPacket + 6, htonl((int)(lati*1000.0f)));
	pf_set_dword(bSendPacket + 10, htonl((int)(longi*1000.0f)));
	pf_set_dword(bSendPacket + 14, htonl((int)(alti*1000.0f)));

	pthread_mutex_lock(&m_mutexSend);
	ret = SendStream(type, fhandle, bSendPacket, 6+12);
	pthread_mutex_unlock(&m_mutexSend);
	return ret;
}

//#endif
