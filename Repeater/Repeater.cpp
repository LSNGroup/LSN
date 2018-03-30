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



// 唯一的应用程序对象

CWinApp theApp;

using namespace std;



static void UiLoop(void)
{
	char cmd[MAX_PATH];
	int index;
	char cam_id[MAX_PATH];
	char pass[MAX_PATH];
	char temp[MAX_PATH];
	char type[MAX_PATH];
	int ext;
	int num;
	char name[MAX_PATH];

	printf("\n>");

	while(1)
	{
		gets(cmd);

		trim(cmd);

		if (strcmp(cmd, "") == 0 || strcmp(cmd, "h") == 0 || strcmp(cmd, "help") == 0)
		{
			printf("\n");
			printf("h(help)                       帮助\n");
			printf("l                             列出所有已连接的推流端 以及观看端数量\n");
			printf("f <width> <height> <fps>      设置视频格式\n");
			printf("c <cam_id> <password>         连接新的推流端\n");
			printf("d <index>                     断开指定推流端\n");
			printf("s <index>                     视频源切到指定推流端\n");
			printf("v <0/1>                       当前推流端切换摄像头\n");
			printf("o <type> <ext> <num> <name> <pass> <tcp_addr>   转发器online\n");
			printf("x                             退出程序\n");
		}
		else if (strncmp(cmd, "f ", 2) == 0)
		{
			sscanf(cmd, "%s%d%d%d", temp, &g_video_width, &g_video_height, &g_video_fps);
			printf("Set video format: %dx%d fps(%d)...\n", g_video_width, g_video_height, g_video_fps);
		}
		else if (strcmp(cmd, "l") == 0)
		{
			printf("Current video format: %dx%d fps(%d) \n", g_video_width, g_video_height, g_video_fps);
			printf("Current source index: %d \n", g_pShiyong->currentSourceIndex);
			printf("---------------------------------------------------------------\n");
			for (int i = 0; i < MAX_VIEWER_NUM; i++)
			{
			}
			printf("---------------------------------------------------------------\n");
			printf("\nServerNodes=%d, connected_av=%d\n", MAX_SERVER_NUM, GetAvClientsCount());
		}
		else if (strncmp(cmd, "c ", 2) == 0)
		{
			sscanf(cmd, "%s%s%s", temp, cam_id, pass);
			printf("To connect id(%s) with password(%s)...\n", cam_id, pass);
		}
		else if (strncmp(cmd, "d ", 2) == 0)
		{
			sscanf(cmd, "%s%d", temp, &index);
			printf("To disconnect index(%d)...\n", index);
			if (g_pShiyong->viewerArray[index].bUsing == FALSE) {
				printf("ViewerNode not using!\n");
			}
			else {
				g_pShiyong->DisconnectNode(&(g_pShiyong->viewerArray[index]));
			}
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

					//由于电脑推流端（ViewerNode[0]）的T264不会周期性发送SPS、PPS，所以切换时要重新发送一遍！！！
					int t264_index = 0;
					if (index == t264_index)
					{
						//先切断PacketRepeater的转发过程，避免SPS与PPS之间有东西。。。
						g_pShiyong->currentSourceIndex = -1;

						for (int i = 0; i < MAX_SERVER_NUM; i++)
						{
							if (arrServerProcesses[i].m_bAVStarted == FALSE) {
								continue;
							}
							if (arrServerProcesses[i].m_bVideoEnable == FALSE) {
								continue;
							}
							
							int ret = CtrlCmd_Send_FAKERTP_RESP(SOCKET_TYPE_TCP, arrServerProcesses[i].m_fhandle, g_pShiyong->viewerArray[t264_index].m_sps_buff, g_pShiyong->viewerArray[t264_index].m_sps_len);
							if (ret < 0) {
								log_msg("Switch video source: CtrlCmd_Send_FAKERTP_RESP(sps) failed!", LOG_LEVEL_ERROR);
							}
							ret = CtrlCmd_Send_FAKERTP_RESP(SOCKET_TYPE_TCP, arrServerProcesses[i].m_fhandle, g_pShiyong->viewerArray[t264_index].m_pps_buff, g_pShiyong->viewerArray[t264_index].m_pps_len);
							if (ret < 0) {
								log_msg("Switch video source: CtrlCmd_Send_FAKERTP_RESP(pps) failed!", LOG_LEVEL_ERROR);
							}
						}
					}//if (index == t264_index)

					g_pShiyong->currentSourceIndex = index;
				}
			}
		}
		else if (strncmp(cmd, "v ", 2) == 0)
		{
			sscanf(cmd, "%s%d", temp, &index);
			if (g_pShiyong->currentSourceIndex == -1 || g_pShiyong->viewerArray[g_pShiyong->currentSourceIndex].bUsing == FALSE || g_pShiyong->viewerArray[g_pShiyong->currentSourceIndex].bConnected == FALSE) {
				printf("Current ViewerNode not using!\n");
			}
			else {
				printf("推流端(%d) will use camera(%d)...\n", g_pShiyong->currentSourceIndex, index);
				CtrlCmd_AV_SWITCH(g_pShiyong->viewerArray[g_pShiyong->currentSourceIndex].httpOP.m1_use_sock_type,
								g_pShiyong->viewerArray[g_pShiyong->currentSourceIndex].httpOP.m1_use_udt_sock,
								index);
			}
		}
		else if (strncmp(cmd, "o ", 2) == 0)
		{
			sscanf(cmd, "%s%s%d%d%s%s%s", temp, type, &ext, &num, name, pass, g_tcp_address);
			printf("online type(%s)-%d num(%d) name(%s) pass(%s) tcp_addr(%s)...\n", type, ext, num, name, pass, g_tcp_address);

			if (arrServerProcesses != NULL) {
				printf("不能重复上线！\n");
			}
			else if (g_pShiyong->currentSourceIndex == -1 || g_pShiyong->viewerArray[g_pShiyong->currentSourceIndex].bUsing == FALSE) {
				printf("请先连接并选中视频源！\n");
			}
			else {
				strncpy(SERVER_TYPE, type, sizeof(SERVER_TYPE));
				UUID_EXT = ext;
				MAX_SERVER_NUM = num;
				strncpy(NODE_NAME, name, sizeof(NODE_NAME));
				strncpy(CONNECT_PASSWORD, pass, sizeof(CONNECT_PASSWORD));
				
				StartServerProcesses();
			}
		}
		else if (strcmp(cmd, "x") == 0)
		{
			printf("Stop ViewerNode...\n");
			for (int i = 0; i < MAX_VIEWER_NUM; i++)
			{
				g_pShiyong->DisconnectNode(&(g_pShiyong->viewerArray[i]));
			}
			Sleep(15000);
			g_pShiyong->DoExit();

			printf("Stop ServerProcesses...\n");
			for (int i = 0; i < MAX_SERVER_NUM; i++)
			{
				CtrlCmd_Send_END(SOCKET_TYPE_TCP, arrServerProcesses[i].m_fhandle);
			}
			Sleep(1000);
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


		{//////////////////////////////////////////////////////////////


		}//////////////////////////////////////////////////////////////

		UiLoop();


		CtrlCmd_Uninit();

		// use this function to release the UDT library
		UDT::cleanup();

		WSACleanup();

		CloseHandle(hGlobalMutex);
	}

	return nRetCode;
}
