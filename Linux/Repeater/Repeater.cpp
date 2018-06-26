// Repeater.cpp : 定义控制台应用程序的入口点。
//

//#include "stdafx.h"
#include "Repeater.h"

#include "Shiyong.h"
#include "IpcGuaji.h"

#include "../ShareDir/platform.h"
#include "../ShareDir/CommonLib.h"
#include "../ShareDir/ControlCmd.h"
#include "../ShareDir/HttpOperate.h"
#include "../ShareDir/AppSettings.h"
#include "../ShareDir/LogMsg.h"

#include <pthread.h>
#include <unistd.h>
#include <signal.h>


char gszProgramName[MAX_PATH] = "LSN_REPEATER";
char gszProgramDir[MAX_PATH] = "";


static void GetProgramDir(char current_absolute_path[], int max_size)
{
	//获取当前程序绝对路径
	int cnt = readlink("/proc/self/exe", current_absolute_path, max_size);
	if (cnt < 0 || cnt >= max_size)
	{
	   	getcwd(current_absolute_path, max_size);
	    return;
	}
	current_absolute_path[cnt] = '\0';//重要！！！
	
	//获取当前目录绝对路径，即去掉程序名
	int i;
	for (i = cnt; i >= 0; --i)
	{
	    if (current_absolute_path[i] == '/')
	    {
	        current_absolute_path[i+1] = '\0';
	        break;
	    }
	}
}

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
			g_pShiyong->m_bQuit = TRUE;
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

			if (g1_system_debug_flags != 0)
			{
				char log_info[MAX_PATH];
				snprintf(log_info, sizeof(log_info), "设备下线！！！%s，%s，x", g0_device_uuid, g0_node_name);
				HttpOperate::DoLogRecord("gbk", "zh", g_pShiyong->device_node_id, g0_version, HTTP_LOG_TYPE_DEVICE_OFFLINE, log_info);
			}

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
	
	printf("\nUiLoop exit.\n");
}

int main(int argc, char **argv)
{
	sigset_t signal_mask;
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask, SIGPIPE);
	int rc = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
	if (rc != 0) {
		printf("Block SIGPIPE failed.\n");
	}
	
	//if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: 在此处为应用程序的行为编写代码。

		GetProgramDir(gszProgramDir, sizeof(gszProgramDir));
		chdir(gszProgramDir);


		log_msg_init();

		char msg[MAX_PATH];
		sprintf(msg, "CurrentDirectory: %s", gszProgramDir);
		log_msg(msg, LOG_LEVEL_INFO);


		// use this function to initialize the UDT library
		UDT::startup();

		CtrlCmd_Init();

		//RunExeNoWait("net stop mpssvc",                  FALSE);
		//RunExeNoWait("sc config mpssvc start= disabled", FALSE);


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

				if (g1_system_debug_flags != 0)
				{
					char log_info[MAX_PATH];
					snprintf(log_info, sizeof(log_info), "设备上线。。。%s，%s", g0_device_uuid, g0_node_name);
					HttpOperate::DoLogRecord("gbk", "zh", g_pShiyong->device_node_id, g0_version, HTTP_LOG_TYPE_DEVICE_ONLINE, log_info);
				}

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
#if FIRST_LEVEL_REPEATER
				g_pShiyong->device_max_streams = 5;//测速。。。
#else
				g_pShiyong->device_max_streams = 2;//测速。。。
#endif
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
	}
	
	printf("\nmain() exit.\n");
	return 0;
}
