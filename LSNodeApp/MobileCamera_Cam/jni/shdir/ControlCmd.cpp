//#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "CommonLib.h"
#include "ControlCmd.h"
#ifdef WIN32
#include "LogMsg.h"
#endif

#ifdef ANDROID_NDK
#include <android/log.h>
#define log_msg(msg, lev)  __android_log_print(ANDROID_LOG_INFO, "shdir", msg)
#endif


static BOOL m_bInit = FALSE;
static pthread_mutex_t m_mutexSendArray[2];


static pthread_mutex_t * getMutexSend(SOCKET_TYPE type)
{
	if (type == SOCKET_TYPE_UDT) {
		return &(m_mutexSendArray[0]);
	}
	else // SOCKET_TYPE_TCP
	{
		return &(m_mutexSendArray[1]);
	}
}

int  CtrlCmd_Init()
{
	if (m_bInit) {
		return 0;
	}
	m_bInit = TRUE;

	log_msg("CtrlCmd_Init()...\n", LOG_LEVEL_DEBUG);
	pthread_mutex_init(&(m_mutexSendArray[0]), NULL);
	pthread_mutex_init(&(m_mutexSendArray[1]), NULL);
	return 0;
}

void CtrlCmd_Uninit()
{
	if (FALSE == m_bInit) {
		return;
	}
	m_bInit = FALSE;

	log_msg("CtrlCmd_Uninit()...\n", LOG_LEVEL_DEBUG);
	pthread_mutex_destroy(&(m_mutexSendArray[0]));
	pthread_mutex_destroy(&(m_mutexSendArray[1]));
}

int CtrlCmd_Send_Raw(SOCKET_TYPE type, SOCKET fhandle, BYTE *raw_data, int data_len)
{
	int ret;

	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, (char *)raw_data, data_len);
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

//#ifdef JNI_FOR_MOBILECAMERA

int CtrlCmd_Send_FAKERTP_RESP(SOCKET_TYPE type, SOCKET fhandle, BYTE *packet, int len)
{
	int ret;
	char bSendPacket[32];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_FAKERTP_RESP));
	pf_set_dword(bSendPacket + 2, htonl(len));
	
	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6);
	ret += SendStream(type, fhandle, (char *)packet, len);
	if (ret != 0) {
		ret = -1;
	}
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

int CtrlCmd_Send_FAKERTP_RESP_NOMUTEX(SOCKET_TYPE type, SOCKET fhandle, BYTE *packet, int len)
{
	int ret;
	char bSendPacket[32];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_FAKERTP_RESP));
	pf_set_dword(bSendPacket + 2, htonl(len));
	
	//pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6);
	ret += SendStream(type, fhandle, (char *)packet, len/2);
	ret += SendStream(type, fhandle, (char *)packet + (len/2), len - (len/2));
	if (ret != 0) {
		ret = -1;
	}
	//pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

int CtrlCmd_Send_HELLO_RESP(SOCKET_TYPE type, SOCKET fhandle, BYTE *server_node_id, DWORD server_version, BYTE func_flags, BYTE topo_level, WORD result_code)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_HELLO_RESP));
	pf_set_dword(bSendPacket + 2, htonl(6+4+1+1+2));
	memcpy(bSendPacket + 6, server_node_id, 6);
	pf_set_dword(bSendPacket + 12, htonl(server_version));
	*((BYTE *)bSendPacket + 16) = func_flags;
	*((BYTE *)bSendPacket + 17) = topo_level;
	pf_set_word(bSendPacket + 18, htons(result_code));

	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6 + 6 + 4 + 1 + 1 + 2);
	pthread_mutex_unlock(getMutexSend(type));
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
	
	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6);
	ret += SendStream(type, fhandle, (char *)result_str, len);
	if (ret != 0) {
		ret = -1;
	}
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

int CtrlCmd_Send_NULL(SOCKET_TYPE type, SOCKET fhandle)
{
	int ret;
	char bSendPacket[32];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_NULL));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6);
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

int CtrlCmd_Send_END(SOCKET_TYPE type, SOCKET fhandle)
{
	int ret;
	char bSendPacket[32];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_END));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6);
	pthread_mutex_unlock(getMutexSend(type));
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
				ret = RecvStream(type, fhandle, buff, 1);
				if (ret != 0) {
					break;
				}
			}
		}
	}
}


//#else

int CtrlCmd_HELLO(SOCKET_TYPE type, SOCKET fhandle, BYTE *client_node_id, DWORD client_version, BYTE topo_primary, const char *password, BYTE *server_node_id, DWORD *server_version, BYTE *func_flags, BYTE *topo_level, WORD *result_code)
{
	BYTE bZeroID[6] = {0, 0, 0, 0, 0, 0};
	char buff[32];
	int ret;
	WORD wCommand;
	DWORD dwDataLength;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_HELLO_REQ));
	pf_set_dword(bSendPacket + 2, htonl(6+4+1+256));
	memcpy(bSendPacket + 6, client_node_id, 6);
	pf_set_dword(bSendPacket + 12, htonl(client_version));
	*((BYTE *)bSendPacket + 16) = topo_primary;
	strcpy(bSendPacket + 17, password);

	pthread_mutex_lock(getMutexSend(type));

	if (SendStream(type, fhandle, bSendPacket, 6+6+4+1+256) < 0) {
		pthread_mutex_unlock(getMutexSend(type));
		return -1;
	}

	pthread_mutex_unlock(getMutexSend(type));

	ret = RecvStream(type, fhandle, buff, 6);
	if (ret != 0) {
		return -1;
	}

	wCommand = ntohs(pf_get_word(buff));
	dwDataLength = ntohl(pf_get_dword(buff+2));

	if (wCommand != CMD_CODE_HELLO_RESP || dwDataLength != 14) {//修改协议字段时，不要忘记这个数据长度！！！
		return -1;
	}

	ret = RecvStream(type, fhandle, buff, 6);
	if (ret != 0) {
		return -1;
	}
	if (memcmp(client_node_id, bZeroID, 6) == 0)//client_node_id为零，IPC通道，返回server_node_id
	{
		memcpy(server_node_id, buff, 6);
	}
	else {
		if (memcmp(server_node_id, buff, 6) != 0) {
			return -1;
		}
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

	ret = RecvStream(type, fhandle, buff, 1);
	if (ret != 0) {
		return -1;
	}
	*topo_level = buff[0];

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

	pthread_mutex_lock(getMutexSend(type));
	
	if (SendStream(type, fhandle, bSendPacket, 6) < 0) {
		pthread_mutex_unlock(getMutexSend(type));
		return -1;
	}
	if (SendStream(type, fhandle, (char *)exe_cmd, len) < 0) {
		pthread_mutex_unlock(getMutexSend(type));
		return -1;
	}
	
	pthread_mutex_unlock(getMutexSend(type));

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

int CtrlCmd_IPC_REPORT(SOCKET_TYPE type, SOCKET fhandle, BYTE *source_node_id, const char *report_string)
{
	int ret;
	char bSendPacket[32];
	int len = strlen(report_string) + 1;

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_IPC_REPORT));
	pf_set_dword(bSendPacket + 2, htonl(12 + len));
	memcpy(bSendPacket + 6, source_node_id, 6);
	memset(bSendPacket + 12, 0, 6);
	
	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6 + 12);
	ret += SendStream(type, fhandle, (char *)report_string, len);
	if (ret != 0) {
		ret = -1;
	}
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

int CtrlCmd_TOPO_REPORT(SOCKET_TYPE type, SOCKET fhandle, BYTE *source_node_id, const char *report_string)
{
	int ret;
	char bSendPacket[32];
	int len = strlen(report_string) + 1;

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_TOPO_REPORT));
	pf_set_dword(bSendPacket + 2, htonl(12 + len));
	memcpy(bSendPacket + 6, source_node_id, 6);
	memset(bSendPacket + 12, 0, 6);
	
	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6 + 12);
	ret += SendStream(type, fhandle, (char *)report_string, len);
	if (ret != 0) {
		ret = -1;
	}
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

int CtrlCmd_TOPO_DROP(SOCKET_TYPE type, SOCKET fhandle, BYTE is_connected, BYTE node_type, BYTE *node_id)
{
	int ret;
	char bSendPacket[32];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_TOPO_DROP));
	pf_set_dword(bSendPacket + 2, htonl(1 + 1 + 6));
	*(BYTE *)(bSendPacket + 6) = is_connected;
	*(BYTE *)(bSendPacket + 7) = node_type;
	memcpy(bSendPacket + 8, node_id, 6);
	
	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6+1+1+6);
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

int CtrlCmd_TOPO_EVALUATE(SOCKET_TYPE type, SOCKET fhandle, BYTE *source_node_id, BYTE *object_node_id, DWORD begin_time, DWORD end_time, DWORD stream_flow)
{
	int ret;
	char bSendPacket[64];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_TOPO_EVALUATE));
	pf_set_dword(bSendPacket + 2, htonl(12 + 4 + 4 + 4));
	memcpy(bSendPacket + 6, source_node_id, 6);
	memcpy(bSendPacket + 12, object_node_id, 6);
	pf_set_dword(bSendPacket + 18, htonl(begin_time));
	pf_set_dword(bSendPacket + 22, htonl(end_time));
	pf_set_dword(bSendPacket + 26, htonl(stream_flow));
	
	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6+12+4+4+4);
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

int CtrlCmd_TOPO_EVENT(SOCKET_TYPE type, SOCKET fhandle, BYTE *dest_node_id, const char *event_string)
{
	int ret;
	char bSendPacket[32];
	int len = strlen(event_string) + 1;

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_TOPO_EVENT));
	pf_set_dword(bSendPacket + 2, htonl(12 + len));
	memset(bSendPacket + 6, 0, 6);
	memcpy(bSendPacket + 12, dest_node_id, 6);
	
	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6 + 12);
	ret += SendStream(type, fhandle, (char *)event_string, len);
	if (ret != 0) {
		ret = -1;
	}
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

int CtrlCmd_TOPO_SETTINGS(SOCKET_TYPE type, SOCKET fhandle, BYTE topo_level, const char *settings_string)
{
	int ret;
	char bSendPacket[32];
	int len = strlen(settings_string) + 1;

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_TOPO_SETTINGS));
	pf_set_dword(bSendPacket + 2, htonl(12 + 1 + len));
	memset(bSendPacket + 6, 0, 6);
	memset(bSendPacket + 12, 0xff, 6);
	bSendPacket[18] = (char)topo_level;
	
	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6 + 12 + 1);
	ret += SendStream(type, fhandle, (char *)settings_string, len);
	if (ret != 0) {
		ret = -1;
	}
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

int CtrlCmd_PROXY(SOCKET_TYPE type, SOCKET fhandle, WORD wTcpPort)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_PROXY_REQ));
	pf_set_dword(bSendPacket + 2, htonl(2));
	pf_set_word(bSendPacket + 6, htons(wTcpPort));

	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6+2);
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

int CtrlCmd_PROXY_DATA(SOCKET_TYPE type, SOCKET fhandle, BYTE *data, int len)
{//proxy data is big!!!
	int ret;
	char bSendPacket[32];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_PROXY_DATA));
	pf_set_dword(bSendPacket + 2, htonl(len));
	
	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6);
	ret += SendStream(type, fhandle, (char *)data, len);
	if (ret != 0) {
		ret = -1;
	}
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

int CtrlCmd_PROXY_END(SOCKET_TYPE type, SOCKET fhandle)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_PROXY_END));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6);
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

int CtrlCmd_ARM(SOCKET_TYPE type, SOCKET fhandle)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_ARM_REQ));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6);
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

int CtrlCmd_DISARM(SOCKET_TYPE type, SOCKET fhandle)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_DISARM_REQ));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6);
	pthread_mutex_unlock(getMutexSend(type));
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
	
	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6 + 3 + 4 + 4);
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

int CtrlCmd_AV_STOP(SOCKET_TYPE type, SOCKET fhandle)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_AV_STOP_REQ));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6);
	pthread_mutex_unlock(getMutexSend(type));
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

	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6 + 4);
	pthread_mutex_unlock(getMutexSend(type));
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

	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6 + 2 + 4);
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

int CtrlCmd_VOICE(SOCKET_TYPE type, SOCKET fhandle, BYTE *data, int len)
{//voice data is big!!!
	int ret;
	char bSendPacket[32];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_VOICE_REQ));
	pf_set_dword(bSendPacket + 2, htonl(len));
	
	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6);
	ret += SendStream(type, fhandle, (char *)data, len);
	if (ret != 0) {
		ret = -1;
	}
	pthread_mutex_unlock(getMutexSend(type));
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

	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6+512);
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

int CtrlCmd_BYE(SOCKET_TYPE type, SOCKET fhandle)
{
	int ret;
	char bSendPacket[CTRLCMD_MAX_PACKET_SIZE];

	memset(bSendPacket, 0, sizeof(bSendPacket));
	pf_set_word(bSendPacket + 0, htons(CMD_CODE_BYE_REQ));
	pf_set_dword(bSendPacket + 2, htonl(0));

	pthread_mutex_lock(getMutexSend(type));
	ret = SendStream(type, fhandle, bSendPacket, 6);
	pthread_mutex_unlock(getMutexSend(type));
	return ret;
}

//#endif
