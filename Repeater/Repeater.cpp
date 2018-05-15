// Repeater.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "Repeater.h"

#include "Shiyong.h"
#include "Guaji.h"

#include "../ShareDir/platform.h"
#include "../ShareDir/CommonLib.h"
#include "../ShareDir/ControlCmd.h"
#include "../ShareDir/HttpOperate.h"
#include "../ShareDir/LogMsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



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



// 唯一的应用程序对象

CWinApp theApp;

using namespace std;



static void UiLoop(void)
{
	char cmd[MAX_PATH];
	int index;
	char temp[MAX_PATH];


	printf("\n>");

	while(1)
	{
		gets(cmd);

		trim(cmd);

		if (strcmp(cmd, "") == 0 || strcmp(cmd, "h") == 0 || strcmp(cmd, "help") == 0)
		{
			printf("\n");
			printf("h(help)                       帮助\n");
			printf("l                             列出当前状态信息\n");
			printf("f <width> <height> <fps>      设置视频格式\n");
			printf("s <index>                     视频源切换\n");
			printf("x                             退出程序\n");
		}
		else if (strncmp(cmd, "f ", 2) == 0)
		{
			sscanf(cmd, "%s%d%d%d", temp, &g_video_width, &g_video_height, &g_video_fps);
			printf("Set video format: %dx%d fps(%d)...\n", g_video_width, g_video_height, g_video_fps);
		}
		else if (strcmp(cmd, "l") == 0)
		{
			printf("Software Version: 0x%08lx\n", MYSELF_VERSION); 
			printf("Current video format: %dx%d fps(%d) \n", g_video_width, g_video_height, g_video_fps);
			printf("Current source index: %d \n", g_pShiyong->currentSourceIndex);
			printf("Joined channel id: %ld \n", g_pShiyong->get_joined_channel_id());
			printf("Device topo level: %d \n", g_pShiyong->device_topo_level);
			printf("----------------- Viewer Nodes --------------------------------------------\n");
			for (int i = 0; i < MAX_VIEWER_NUM; i++)
			{
				printf("  ViewerNode %d(port%d), bUsing=%d  bConnecting=%d  bConnected=%d  bTopoPrimary=%d  \n", i, g_pShiyong->viewerArray[i].httpOP.m0_p2p_port, (g_pShiyong->viewerArray[i].bUsing ? 1 : 0), (g_pShiyong->viewerArray[i].bConnecting ? 1 : 0), (g_pShiyong->viewerArray[i].bConnected ? 1 : 0), (g_pShiyong->viewerArray[i].bTopoPrimary ? 1 : 0));
			}
			printf("\n");
			printf("----------------- Guaji Nodes ---------------------------------------------\n");
			printf(" max_connections=%d  current_connections=%d  max_streams=%d  current_streams=%d\n", MAX_SERVER_NUM, MAX_SERVER_NUM - g_pShiyong->GetUnconnectedGuajiNodes(), g_pShiyong->device_max_streams, GetAvClientsCount());
		}
		else if (strncmp(cmd, "s ", 2) == 0)
		{
			sscanf(cmd, "%s%d", temp, &index);
			printf("To switch to index(%d)...\n", index);
			if (g_pShiyong->viewerArray[index].bUsing == FALSE || g_pShiyong->viewerArray[index].bConnected == FALSE) {
				printf("ViewerNode not using!\n");
			}
			else {
				if (g_pShiyong->currentSourceIndex != index) {
					g_pShiyong->SwitchMediaSource(g_pShiyong->currentSourceIndex, index);
				}
			}

		}
		else if (strcmp(cmd, "x") == 0)
		{
			printf("Stop ViewerNode...\n");
			for (int i = 0; i < MAX_VIEWER_NUM; i++)
			{
				g_pShiyong->UnregisterNode(&(g_pShiyong->viewerArray[i]));
			}
			for (int i = 0; i < MAX_VIEWER_NUM; i++)
			{
				g_pShiyong->DisconnectNode(&(g_pShiyong->viewerArray[i]));
			}
			g_pShiyong->DoExit();
			printf("Waiting...\n");
			usleep(5000*1000);

			printf("Stop ServerProcesses...\n");
			for (int i = 0; i < MAX_SERVER_NUM; i++)
			{
				CtrlCmd_Send_END(SOCKET_TYPE_TCP, arrServerProcesses[i].m_fhandle);
			}
			usleep(1000*1000);
			printf("Done!!!\n");

			break;
		}

		printf("\n>");
	}
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// 初始化 MFC 并在失败时显示错误
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: 更改错误代码以符合您的需要
		_tprintf(_T("错误: MFC 初始化失败\n"));
		nRetCode = 1;
	}
	else
	{
		// TODO: 在此处为应用程序的行为编写代码。

		GetProgramDir(gszProgramDir, sizeof(gszProgramDir));
		SetCurrentDirectory(gszProgramDir);


		log_msg_init();

		char msg[MAX_PATH];
		sprintf(msg, "CurrentDirectory: %s", gszProgramDir);
		log_msg(msg, LOG_LEVEL_INFO);


		HANDLE hGlobalMutex;
		WSADATA wsaData;

		hGlobalMutex = CreateMutex(NULL, TRUE, "Global\\WL_REPEATER");
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			printf("已经有一个程序实例在运行，不能重复启动!\n");
			return 1;
		}


		if (WSAStartup(0x0202, &wsaData) != 0) {
			printf("WinSock传输库初始化失败，无法启动程序!\n");
			return 1;
		}


		// use this function to initialize the UDT library
		UDT::startup();

		CtrlCmd_Init();

		RunExeNoWait("net stop mpssvc",                  FALSE);
		RunExeNoWait("sc config mpssvc start= disabled", FALSE);


#if FIRST_LEVEL_REPEATER
		printf("\nLSN Star Repeater \nSoftware Version: 0x%08lx\n", MYSELF_VERSION);
#else
		printf("\nLSN Repeater \nSoftware Version: 0x%08lx\n", MYSELF_VERSION);
#endif

		DWORD dwForceNoNAT = 0;
		if (FALSE == GetSoftwareKeyDwordValue(STRING_REGKEY_NAME_FORCE_NONAT, &dwForceNoNAT)) {
			SaveSoftwareKeyDwordValue(STRING_REGKEY_NAME_FORCE_NONAT, (DWORD)0);
		}

		{//////////////////////////////////////////////////////////////
			if (arrServerProcesses != NULL) {
				printf("不能重复上线！\n");
			}
			else {
#if FIRST_LEVEL_REPEATER
				strncpy(SERVER_TYPE, "STAR", sizeof(SERVER_TYPE));
				UUID_EXT = 1;//TREE ROOT MARK
#else
				strncpy(SERVER_TYPE, "TREE", sizeof(SERVER_TYPE));
				UUID_EXT = 0;//TREE ROOT MARK
#endif
				strncpy(NODE_NAME, "GuajiNodeName", sizeof(NODE_NAME));
				strncpy(CONNECT_PASSWORD, "123456", sizeof(CONNECT_PASSWORD));
				strncpy(g_tcp_address, "127.0.0.1", sizeof(g_tcp_address));


				g_pShiyong = new CShiyong();
				g_pShiyong->OnInit();

				for (int i = 0; i < 20; i++)
				{
					usleep(500*1000);
					if (g1_bandwidth_per_stream != BANDWIDTH_PER_STREAM_UNKNOWN) {
						break;
					}
					printf("Waiting for parameter bandwidth_per_stream...\n");
				}
				if (g1_bandwidth_per_stream == BANDWIDTH_PER_STREAM_UNKNOWN) {
					g1_bandwidth_per_stream = BANDWIDTH_PER_STREAM_DEFAULT;
				}
				g_pShiyong->device_max_streams = 2;//测速。。。
				if (strstr(g0_device_uuid, "-1@1") != NULL) {
					MAX_SERVER_NUM = g_pShiyong->device_max_streams;
				}
				else {
					MAX_SERVER_NUM = g_pShiyong->device_max_streams * 2;
				}
				
				//字符串参数，如NODE_NAME，不能包含空格！！！因为RepeaterNode用sscanf()解析参数。
				printf("Online type(%s)-%d num(%d) name(%s) pass(%s) tcp_addr(%s)...\n", SERVER_TYPE, UUID_EXT, MAX_SERVER_NUM, NODE_NAME, CONNECT_PASSWORD, g_tcp_address);

				StartServerProcesses();
			}
		}//////////////////////////////////////////////////////////////

		UiLoop();

		if (NULL != g_pShiyong)
		{
			delete g_pShiyong;
			g_pShiyong = NULL;
		}


		CtrlCmd_Uninit();

		// use this function to release the UDT library
		UDT::cleanup();

		WSACleanup();

		CloseHandle(hGlobalMutex);
	}

	return nRetCode;
}
