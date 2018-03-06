// RepeaterNode.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "RepeaterNode.h"

#include "Guaji.h"

#include "../ShareDir/platform.h"
#include "../ShareDir/CommonLib.h"
#include "../ShareDir/ControlCmd.h"
#include "../ShareDir/HttpOperate.h"
#include "../ShareDir/LogMsg.h"


typedef enum _tag_audio_codec_type {
	AUDIO_CODEC_G721 = 1,
	AUDIO_CODEC_G723_24,
	AUDIO_CODEC_G723_40,
	AUDIO_CODEC_G729A,
} AUDIO_CODEC_TYPE;



TCHAR gszProgramName[MAX_PATH] = "WL_REPEATER";
TCHAR gszProgramDir[MAX_PATH] = "";


void GetSoftwareKeyName(LPTSTR szKey, DWORD dwLen)
{
	if(NULL == szKey || 0 == dwLen)
		return;

	_snprintf(szKey,dwLen,"Software\\%s", "WL_REPEATER");
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

static unsigned long get_rtp_timestamp()//milliseconds
{
	unsigned long ret = GetTickCount();
	return ret;
}

static void on_fake_rtp_payload(int payload_type, unsigned char *data, int len)
{
	if (g_pServerNode->m_bConnected == FALSE || g_pServerNode->m_bAVStarted == FALSE || g_pServerNode->m_pFRS == NULL) {
		return;
	}

	BYTE bSendNri = 0;

	if (payload_type == PAYLOAD_TLV && g_pServerNode->m_bTLVEnable) {

		FakeRtpSend_sendpacket(g_pServerNode->m_pFRS, 0, data, len, payload_type, 0, bSendNri);

	}
	else if (payload_type == PAYLOAD_AUDIO && g_pServerNode->m_bAudioEnable) {

		if (g_pServerNode->m_bAudioRedundance) {
			bSendNri = 2;
		}

		FakeRtpSend_sendpacket(g_pServerNode->m_pFRS, get_rtp_timestamp(), data, len, payload_type, AUDIO_CODEC_G729A, bSendNri);

	}
	else if (payload_type == PAYLOAD_VIDEO && g_pServerNode->m_bVideoEnable) {

		if (g_pServerNode->m_bVideoReliable) {
			bSendNri |= FAKERTP_RELIABLE_FLAG;
		}

		FakeRtpSend_sendpacket(g_pServerNode->m_pFRS, get_rtp_timestamp(), data, len, payload_type, 0, bSendNri);
	}
}

static void record_fake_rtp_payload(FILE *fp_record, int payload_type, unsigned char *data, int len)
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


	StartAnyPC();


	while (true)
	{
		char buf[6];
		WORD wCmd;
		DWORD copy_len;
		BYTE bPayloadType;

		if (RecvStream(SOCKET_TYPE_TCP, g_fhandle, buf, 6) < 0)
		{
			log_msg("RecvStream() != 6, Exit Process...", LOG_LEVEL_INFO);
			break;
		}

		wCmd = ntohs(pf_get_word(buf + 0));
		copy_len = ntohl(pf_get_dword(buf + 2));
		if (wCmd == CMD_CODE_END)
		{
			log_msg("wCmd == CMD_CODE_END, Exit Process...", LOG_LEVEL_INFO);
			break;
		}
		else if (wCmd != CMD_CODE_FAKERTP_RESP || copy_len == 0)
		{
			sprintf(msg, "wCmd(0x%x) != CMD_CODE_FAKERTP_RESP || copy_len(%ld) == 0, read...", wCmd, copy_len);
			log_msg(msg, LOG_LEVEL_INFO);
			//if (copy_len > (DWORD)(1024*1024)) {
			//	break;
			//}
			for (int i = 0; i < copy_len; i++) {
				RecvStream(SOCKET_TYPE_TCP, g_fhandle, buf, 1);
				Sleep(10);
			}
			log_msg("wCmd != CMD_CODE_FAKERTP_RESP || copy_len == 0, read done!", LOG_LEVEL_INFO);
			continue;
		}

		unsigned char *pPack = (unsigned char *)malloc(copy_len);////////
		ret = RecvStream(SOCKET_TYPE_TCP, g_fhandle, (char *)(pPack), copy_len);
		if (ret == 0) {
			bPayloadType = pPack[0];
			on_fake_rtp_payload(bPayloadType, pPack + 8, copy_len - 8);

			//第一个RepeaterNode负责录制Av。。。
			if (FIRST_CONNECT_PORT == P2P_PORT) {
				record_fake_rtp_payload(fp_record, bPayloadType, pPack + 8, copy_len - 8);
			}
		}
		else {
			log_msg("RecvStream() != copy_len", LOG_LEVEL_INFO);
		}
		free(pPack);////////
	}

	closesocket(g_fhandle);
	g_fhandle = INVALID_SOCKET;
	}//////////////////////////////////////////////////////////////


_OUT:

	if (FIRST_CONNECT_PORT == P2P_PORT) {
		if (NULL != fp_record)  fclose(fp_record);
	}


	CtrlCmd_Uninit();

	// use this function to release the UDT library
	UDT::cleanup();

	WSACleanup();

	return 0;
}

