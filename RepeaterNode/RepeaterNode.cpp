// RepeaterNode.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "RepeaterNode.h"

#include "Guaji.h"

#include "../ShareDir/platform.h"
#include "../ShareDir/CommonLib.h"
#include "../ShareDir/ControlCmd.h"
#include "../ShareDir/HttpOperate.h"
#include "../ShareDir/UPnP.h"
#include "../ShareDir/LogMsg.h"


typedef enum _tag_audio_codec_type {
	AUDIO_CODEC_G721 = 1,
	AUDIO_CODEC_G723_24,
	AUDIO_CODEC_G723_40,
	AUDIO_CODEC_G729A,
} AUDIO_CODEC_TYPE;



TCHAR gszProgramName[MAX_PATH] = "LSN_REPEATER";
TCHAR gszProgramDir[MAX_PATH] = "";


void GetSoftwareKeyName(LPTSTR szKey, DWORD dwLen)
{
	if(NULL == szKey || 0 == dwLen)
		return;

	_snprintf(szKey,dwLen,"Software\\%s", "LSN_REPEATER");
}

static void GetProgramDir(LPTSTR szDirectory, int nMaxChars)
{
	::GetModuleFileName(NULL, szDirectory, nMaxChars);

    int i;
	int len = strlen(szDirectory);

	for (i = len-1; i >= 0; i--) {
		if (szDirectory[i] == _T('\\') || szDirectory[i] == _T('/')) {
			szDirectory[i] = _T('\0');
			break;
		}
	}
}

static void on_fake_rtp_payload(int payload_type, DWORD timestamp, WORD seq, unsigned char *data, int len)
{
	if (g_pServerNode->m_bConnected == FALSE || g_pServerNode->m_bAVStarted == FALSE || g_pServerNode->m_pFRS == NULL) {
		return;
	}

	BYTE bSendNri = 0;

	if (payload_type == PAYLOAD_TLV && g_pServerNode->m_bTLVEnable) {

		FakeRtpSend_sendpacket(g_pServerNode->m_pFRS, 0, seq, data, len, payload_type, 0, bSendNri);

	}
	else if (payload_type == PAYLOAD_AUDIO && g_pServerNode->m_bAudioEnable) {

		if (g_pServerNode->m_bAudioRedundance) {
			bSendNri = 2;
		}

		bSendNri |= FAKERTP_RELIABLE_FLAG;//LSN转发器的要求！！！

		FakeRtpSend_sendpacket(g_pServerNode->m_pFRS, timestamp, seq, data, len, payload_type, AUDIO_CODEC_G729A, bSendNri);

	}
	else if (payload_type == PAYLOAD_VIDEO && g_pServerNode->m_bVideoEnable) {

		if (g_pServerNode->m_bVideoReliable) {
			bSendNri |= FAKERTP_RELIABLE_FLAG;
		}

		FakeRtpSend_sendpacket(g_pServerNode->m_pFRS, timestamp, seq, data, len, payload_type, 0, bSendNri);
	}
}

static void record_fake_rtp_payload(FILE *fp_record, int payload_type, DWORD timestamp, unsigned char *data, int len)
{
	
}

static void generate_record_filename(char *buff)
{
	time_t timep;
	struct tm *p;


	time(&timep);
	p=localtime(&timep);

	srand((unsigned int)timep);
	sprintf(buff, ".\\AvRecord\\%04d-%02d-%02d_%02d-%02d-%02d_%02X%02X.stream", (1900 + p->tm_year), (1 + p->tm_mon),  p->tm_mday,
		p->tm_hour, p->tm_min, p->tm_sec, (BYTE)rand(), (BYTE)rand());
}

static int DoIpcReportInConnection()
{
	int ret = 0;
	char szIpcReport[512];
	snprintf(szIpcReport, sizeof(szIpcReport), 
		"%02X-%02X-%02X-%02X-%02X-%02X"//"node_id=%02X-%02X-%02X-%02X-%02X-%02X"
		"|%02X-%02X-%02X-%02X-%02X-%02X"//"&peer_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
		"|%d"//"&is_connected=%d"
		"|%d"//"&is_streaming=%d"
		"|%s"//"&ip=%s"
		"|%d"//"&port=%d"
		"|%s"//"&pub_ip=%s"
		"|%d"//"&pub_port=%d"
		"|%d"//"&no_nat=%d"
		"|%d"//"&nat_type=%d"
		"|%s"//"&device_uuid=%s"
		"|%s"//"&node_name=%s"
		"|%ld"//"&version=%ld"
		"|%s"//"&os_info=%s"
		,
		g_pServerNode->myHttpOperate.m0_node_id[0], g_pServerNode->myHttpOperate.m0_node_id[1], g_pServerNode->myHttpOperate.m0_node_id[2], g_pServerNode->myHttpOperate.m0_node_id[3], g_pServerNode->myHttpOperate.m0_node_id[4], g_pServerNode->myHttpOperate.m0_node_id[5], 
		g_peer_node_id[0], g_peer_node_id[1], g_peer_node_id[2], g_peer_node_id[3], g_peer_node_id[4], g_peer_node_id[5],
		(g_pServerNode->m_bConnected ? 1 : 0),
		(g_pServerNode->m_bAVStarted ? 1 : 0),
		g_pServerNode->myHttpOperate.MakeIpStr(), g_pServerNode->myHttpOperate.m0_port, g_pServerNode->myHttpOperate.MakePubIpStr(), g_pServerNode->myHttpOperate.m0_pub_port, 
		(g_pServerNode->myHttpOperate.m0_no_nat ? 1 : 0),
		g_pServerNode->myHttpOperate.m0_nat_type,
		g0_device_uuid, UrlEncode(g0_node_name).c_str(), g0_version, g0_os_info);

	ret = CtrlCmd_IPC_REPORT(SOCKET_TYPE_TCP, g_fhandle, g_pServerNode->myHttpOperate.m0_node_id, szIpcReport);

	return ret;
}


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	//UNREFERENCED_PARAMETER(lpCmdLine);

	if (strlen(lpCmdLine) < 10) {
		return -1;
	}

	sscanf(lpCmdLine, "%s%d%d%s%s%d%d%d%d%d%s", SERVER_TYPE, &UUID_EXT, &MAX_SERVER_NUM, NODE_NAME, CONNECT_PASSWORD, &P2P_PORT, &IPC_BIND_PORT, &g_video_width, &g_video_height, &g_video_fps, g_tcp_address);


	GetProgramDir(gszProgramDir, sizeof(gszProgramDir));
	SetCurrentDirectory(gszProgramDir);

	char log_file[MAX_PATH];
	_snprintf(log_file, MAX_PATH, "./LogMsg/LogMsg_%d.txt", P2P_PORT);
	log_msg_init(log_file);

	char msg[MAX_PATH];
	sprintf(msg, "CurrentDirectory: %s", gszProgramDir);
	log_msg(msg, LOG_LEVEL_INFO);

	if (g_video_width <= 0 || g_video_height <= 0 || g_video_fps <= 0) {
		log_msg("CmdLine parse failed!\n", LOG_LEVEL_ERROR);
		return -1;
	}


	WSADATA wsaData;
	if (WSAStartup(0x0202, &wsaData) != 0) {
		log_msg("WinSock传输库初始化失败，无法启动程序!\n", LOG_LEVEL_ERROR);
		return -1;
	}

	// use this function to initialize the UDT library
	UDT::startup();

	CtrlCmd_Init();


	FILE *fp_record = NULL;
	if (FIRST_CONNECT_PORT == P2P_PORT) {
		char record_file[MAX_PATH];
		generate_record_filename(record_file);
		fp_record = fopen(record_file, "wb");
	}


	{//////////////////////////////////////////////////////////////
	SOCKET fhandle;
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);
	int ret;

	fhandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(0);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(fhandle, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		closesocket(fhandle);
		fhandle = INVALID_SOCKET;
		goto _OUT;
	}

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(IPC_BIND_PORT);
	their_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(&(their_addr.sin_zero), '\0', 8);
	if (connect(fhandle, (sockaddr*)&their_addr, sizeof(their_addr)) < 0)
	{
		closesocket(fhandle);
		fhandle = INVALID_SOCKET;
		goto _OUT;
	}
	g_fhandle = fhandle;


/*
	BYTE bZeroID[6] = {0, 0, 0, 0, 0, 0};
	DWORD server_version;
	BYTE func_flags;
	WORD result_code;
	ret = CtrlCmd_HELLO(SOCKET_TYPE_TCP, g_fhandle, bZeroID, g0_version, 1, "password", g_device_node_id, &server_version, &func_flags, &g_device_topo_level, &result_code);
	log_msg_f(LOG_LEVEL_DEBUG, "CtrlCmd_HELLO: ret=%d, device_topo_level=%d, result_code=%d", ret, g_device_topo_level, result_code);
	if (ret < 0)
	{
		closesocket(g_fhandle);
		g_fhandle = INVALID_SOCKET;
		goto _OUT;
	}
*/

	StartAnyPC();


	while (true)
	{
		char buf[32];
		WORD wCmd;
		DWORD copy_len;
		BYTE bPayloadType;
		WORD wSeq;
		DWORD dwTimeStamp;

		if (RecvStream(SOCKET_TYPE_TCP, g_fhandle, buf, 6) < 0)
		{
			log_msg("RecvStream() != 6, Exit Process...", LOG_LEVEL_INFO);
			break;
		}

		wCmd = ntohs(pf_get_word(buf + 0));
		copy_len = ntohl(pf_get_dword(buf + 2));
		log_msg_f(LOG_LEVEL_DEBUG, "IPC Recv: wCmd=0x%x, dwDataLength=%d...", wCmd, copy_len);

		if (wCmd == CMD_CODE_END)
		{
			log_msg("wCmd == CMD_CODE_END...", LOG_LEVEL_INFO);
			if (g_pServerNode->m_bConnected == TRUE) {
				CtrlCmd_Send_END(g_pServerNode->myHttpOperate.m1_use_sock_type, g_pServerNode->myHttpOperate.m1_use_udt_sock);
			}
		}
		else if (wCmd == CMD_CODE_FAKERTP_RESP && copy_len != 0)
		{
			unsigned char *pPack = (unsigned char *)malloc(copy_len);////////
			ret = RecvStream(SOCKET_TYPE_TCP, g_fhandle, (char *)(pPack), copy_len);
			if (ret == 0) {
				bPayloadType = pPack[0];
				wSeq = ntohs(pf_get_word(pPack + 2));
				dwTimeStamp = ntohl(pf_get_dword(pPack + 4));
				on_fake_rtp_payload(bPayloadType, dwTimeStamp, wSeq, pPack + 8, copy_len - 8);

				//第一个RepeaterNode负责录制Av。。。
				if (FIRST_CONNECT_PORT == P2P_PORT) {
					record_fake_rtp_payload(fp_record, bPayloadType, dwTimeStamp, pPack + 8, copy_len - 8);
				}
			}
			else {
				log_msg("RecvStream() != copy_len", LOG_LEVEL_INFO);
			}
			free(pPack);////////
		}
		else if (wCmd == CMD_CODE_TOPO_EVENT && copy_len != 0)
		{
			if (RecvStream(SOCKET_TYPE_TCP, g_fhandle, buf, 6) != 0) {
				break;
			}
			if (RecvStream(SOCKET_TYPE_TCP, g_fhandle, buf, 6) != 0) {
				break;
			}

			unsigned char *pRecvData = (unsigned char *)malloc(copy_len - 12);
			if (RecvStream(SOCKET_TYPE_TCP, g_fhandle, (char *)pRecvData, copy_len - 12) != 0) {
				free(pRecvData);
				break;
			}

			// buf: dest_node_id
			if (memcmp(buf, g_pServerNode->myHttpOperate.m0_node_id, 6) == 0)
			{
				log_msg_f(LOG_LEVEL_DEBUG, "GuajiNode event=%s\n", (char *)pRecvData);
				g_pServerNode->myHttpOperate.ParseEventValue((char *)pRecvData);
			}
			else {
				if (strcmp(SERVER_TYPE, "STAR") == 0) {
					;//切断向下传播！！！
				} 
				else if (strcmp(SERVER_TYPE, "TREE") == 0 && g_device_topo_level == 4)
				{
					;//切断向下传播！！！
				} else
				if (g_pServerNode->m_bConnected == TRUE) {
					CtrlCmd_TOPO_EVENT(g_pServerNode->myHttpOperate.m1_use_sock_type, g_pServerNode->myHttpOperate.m1_use_udt_sock, (BYTE *)buf, (const char *)pRecvData);;
				}
			}
			free(pRecvData);
		}
		else if (wCmd == CMD_CODE_TOPO_SETTINGS && copy_len != 0)
		{
			if (RecvStream(SOCKET_TYPE_TCP, g_fhandle, buf, 6) != 0) {
				break;
			}
			if (RecvStream(SOCKET_TYPE_TCP, g_fhandle, buf, 6) != 0) {
				break;
			}

			if (RecvStream(SOCKET_TYPE_TCP, g_fhandle, buf, 1) != 0) {
				break;
			}
			BYTE topo_level = *(BYTE *)buf;
			g_device_topo_level = topo_level;

			unsigned char *pRecvData = (unsigned char *)malloc(copy_len - 12 -1);
			if (RecvStream(SOCKET_TYPE_TCP, g_fhandle, (char *)pRecvData, copy_len - 12 -1) != 0) {
				free(pRecvData);
				break;
			}

			g_pServerNode->myHttpOperate.ParseTopoSettings((const char *)pRecvData);

			if (strcmp(SERVER_TYPE, "STAR") == 0) {
				;//切断向下传播！！！
			}
			else if (strcmp(SERVER_TYPE, "TREE") == 0 && g_device_topo_level == 4)
			{
				;//切断向下传播！！！
			} else
			if (g_pServerNode->m_bConnected == TRUE && g_is_topo_primary) {
				CtrlCmd_TOPO_SETTINGS(g_pServerNode->myHttpOperate.m1_use_sock_type, g_pServerNode->myHttpOperate.m1_use_udt_sock, g_device_topo_level, (const char *)pRecvData);;
			}
			free(pRecvData);

			//每隔一定周期g1_register_period，会收到一个CMD_CODE_TOPO_SETTINGS消息
			if (g_pServerNode->m_bConnected == TRUE) {
				DoIpcReportInConnection();
			}
		}
		else
		{
			sprintf(msg, "Unknow wCmd(0x%x), read...", wCmd);
			log_msg(msg, LOG_LEVEL_INFO);
			//if (copy_len > (DWORD)(1024*1024)) {
			//	break;
			//}
			for (int i = 0; i < copy_len; i++) {
				RecvStream(SOCKET_TYPE_TCP, g_fhandle, buf, 1);
				usleep(10*1000);
			}
			log_msg("Unknow wCmd, read done!", LOG_LEVEL_INFO);
			continue;
		}
	}

	closesocket(g_fhandle);
	g_fhandle = INVALID_SOCKET;
	}//////////////////////////////////////////////////////////////


_OUT:

	if (FIRST_CONNECT_PORT == P2P_PORT) {
		if (NULL != fp_record)  fclose(fp_record);
	}

	myUPnP.RemoveNATPortMapping(g_pServerNode->mapping);

	CtrlCmd_Uninit();

	// use this function to release the UDT library
	UDT::cleanup();

	WSACleanup();

	return 0;
}

