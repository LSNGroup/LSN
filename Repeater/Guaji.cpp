// Guaji.cpp : 实现文件
//

#include "stdafx.h"
#include "Repeater.h"
#include "Guaji.h"

#include "platform.h"
#include "CommonLib.h"
#include "HttpOperate.h"
#include "phpMd5.h"
#include "base64.h"
#include "PacketRepeater.h"
#include "LogMsg.h"



SERVER_PROCESS_NODE *arrServerProcesses = NULL;

char SERVER_TYPE[16];
int UUID_EXT = 0;
int MAX_SERVER_NUM = 0;
char NODE_NAME[MAX_PATH];
char CONNECT_PASSWORD[MAX_PATH];

char g_tcp_address[MAX_PATH] = "127.0.0.1";


static BYTE getServerFuncFlags()
{
	BYTE ret = 0;
	ret |= FUNC_FLAGS_AV;
	ret |= FUNC_FLAGS_SHELL;
	if (strcmp(SERVER_TYPE, "ANYPC") == 0) {
		ret |= FUNC_FLAGS_HASROOT;
	}
	if (g1_is_activated) {
		ret |= FUNC_FLAGS_ACTIVATED;
	}
	return ret;
}

static void OnIpcMsg(SERVER_PROCESS_NODE *pServerPorcess, SOCKET fhandle)
{
	SOCKET_TYPE type = SOCKET_TYPE_TCP;
	int  ret;
	char buff[16*1024];
	WORD wCommand;
	DWORD dwDataLength;
	BYTE node_type;
	BYTE source_node_id[6];
	BYTE dest_node_id[6];
	BYTE object_node_id[6];
	DWORD begin_time,end_time,stream_flow;
	char *temp_ptr;

	ret = RecvStream(type, fhandle, buff, 6);
	if (ret != 0) {
		//closesocket(pServerPorcess->m_fhandle);
		pServerPorcess->m_fhandle = INVALID_SOCKET;
		printf("OnIpcMsg: RecvStream(6) failed!\n");
		return;
	}

	wCommand = ntohs(*(WORD*)buff);
	dwDataLength = ntohl(*(DWORD*)(buff+2));

	switch (wCommand)
	{
			case CMD_CODE_HELLO_REQ:

				ret = RecvStream(type, fhandle, buff, 6);
				if (ret != 0) {
					break;
				}
				//if (memcmp(g1_peer_node_id, buff, 6) != 0) {
				//	break;
				//}

				ret = RecvStream(type, fhandle, buff, 4);
				if (ret != 0) {
					break;
				}

				ret = RecvStream(type, fhandle, buff, 1);
				if (ret != 0) {
					break;
				}
				//(*(BYTE *)buff == 1

				ret = RecvStream(type, fhandle, buff, 256);
				if (ret != 0) {
					break;
				}

				CtrlCmd_Send_HELLO_RESP(type, fhandle, g_pShiyong->device_node_id, g0_version, getServerFuncFlags(), g_pShiyong->device_topo_level, CTRLCMD_RESULT_OK);

				break;

			case CMD_CODE_ARM_REQ:
				printf("if_mc_arm() fd=%ld \n", fhandle);
				//if_mc_arm();
				break;
				
			case CMD_CODE_DISARM_REQ:
				printf("if_mc_disarm() fd=%ld \n", fhandle);
				//if_mc_disarm();
				break;

			case CMD_CODE_AV_START_REQ:
				ret = RecvStream(type, fhandle, buff, 1+1+1+4+4);
				if (ret != 0) {
					break;
				}
				
				pServerPorcess->m_bAVStarted = TRUE;
				pServerPorcess->m_bVideoEnable = ((BYTE)(buff[0]) & AV_FLAGS_VIDEO_ENABLE) != 0;
				pServerPorcess->m_bAudioEnable = ((BYTE)(buff[0]) & AV_FLAGS_AUDIO_ENABLE) != 0;
				pServerPorcess->m_bTLVEnable = FALSE;
				
				if (pServerPorcess->m_bVideoEnable && 
					g_pShiyong->currentSourceIndex != -1 && 
					g_pShiyong->viewerArray[g_pShiyong->currentSourceIndex].m_sps_len > 0 && 
					g_pShiyong->viewerArray[g_pShiyong->currentSourceIndex].m_pps_len > 0)
				{
					int ret = CtrlCmd_Send_FAKERTP_RESP(SOCKET_TYPE_TCP, pServerPorcess->m_fhandle, g_pShiyong->viewerArray[g_pShiyong->currentSourceIndex].m_sps_buff, g_pShiyong->viewerArray[g_pShiyong->currentSourceIndex].m_sps_len);
					if (ret < 0) {
						log_msg("OnIpcMsg: CtrlCmd_Send_FAKERTP_RESP(sps) failed!", LOG_LEVEL_ERROR);
					}
					ret = CtrlCmd_Send_FAKERTP_RESP(SOCKET_TYPE_TCP, pServerPorcess->m_fhandle, g_pShiyong->viewerArray[g_pShiyong->currentSourceIndex].m_pps_buff, g_pShiyong->viewerArray[g_pShiyong->currentSourceIndex].m_pps_len);
					if (ret < 0) {
						log_msg("OnIpcMsg: CtrlCmd_Send_FAKERTP_RESP(pps) failed!", LOG_LEVEL_ERROR);
					}
				}

				break;

			case CMD_CODE_AV_STOP_REQ:
				pServerPorcess->m_bAVStarted = FALSE;
				break;

			case CMD_CODE_AV_SWITCH_REQ:
				ret = RecvStream(type, fhandle, buff, 4);
				if (ret != 0) {
					break;
				}
				//DShowAV_Switch(pServerNode, ntohl(pf_get_dword(buff+0)));
				break;

			case CMD_CODE_AV_CONTRL_REQ:
				ret = RecvStream(type, fhandle, buff, 2+4);
				if (ret != 0) {
					break;
				}
				//DShowAV_Contrl(pServerNode, ntohs(pf_get_word(buff+0)), ntohl(pf_get_dword(buff+2)));
				break;

			case CMD_CODE_IPC_REPORT:

				ret = RecvStream(type, fhandle, buff, 6);
				if (ret != 0) {
					break;
				}
				memcpy(pServerPorcess->m_node_id, buff, 6);

				ret = RecvStream(type, fhandle, buff, 6);
				if (ret != 0) {
					break;
				}

				if (dwDataLength - 12 <= sizeof(pServerPorcess->m_ipcReport)) {
					ret = RecvStream(type, fhandle, pServerPorcess->m_ipcReport, dwDataLength - 12);
				}
				else {
					ret = RecvStream(type, fhandle, pServerPorcess->m_ipcReport, sizeof(pServerPorcess->m_ipcReport));
					pServerPorcess->m_ipcReport[sizeof(pServerPorcess->m_ipcReport) - 1] = '\0';
					for (int i = 0; i < (dwDataLength - 12) - sizeof(pServerPorcess->m_ipcReport); i++)
					{
						RecvStream(type, fhandle, buff, 1);
					}
				}

				break;

			case CMD_CODE_TOPO_REPORT: //RepeaterNode 的 rudp server 向上转发来的

				ret = RecvStream(type, fhandle, buff, 6);
				if (ret != 0) {
					break;
				}
				memcpy(source_node_id, buff, 6);

				ret = RecvStream(type, fhandle, buff, 6);
				if (ret != 0) {
					break;
				}

				temp_ptr = (char *)malloc(dwDataLength - 12);

				ret = RecvStream(type, fhandle, temp_ptr, dwDataLength - 12);
				if (ret != 0) {
					free(temp_ptr);
					break;
				}

				g_pShiyong->UpdateRouteTable(pServerPorcess->m_nIndex, temp_ptr);

				if (g_pShiyong->ShouldDoHttpOP())//Root Node, DoReport()
				{

				}
				else
				{//优先选择Primary通道，向上转发。。。
					for (int i = 0; i < MAX_VIEWER_NUM; i++)
					{
						if (g_pShiyong->viewerArray[i].bUsing == FALSE || g_pShiyong->viewerArray[i].bConnected == FALSE) {
							continue;
						}
						if (g_pShiyong->viewerArray[i].bTopoPrimary == TRUE)
						{
							CtrlCmd_TOPO_REPORT(g_pShiyong->viewerArray[i].httpOP.m1_use_sock_type, g_pShiyong->viewerArray[i].httpOP.m1_use_udt_sock, source_node_id, temp_ptr);
							break;
						}
					}
				}

				free(temp_ptr);

				break;

			case CMD_CODE_TOPO_DROP: //RepeaterNode 的 rudp server 向上转发来的

				ret = RecvStream(type, fhandle, buff, 1);
				if (ret != 0) {
					break;
				}
				node_type = *(BYTE *)buff;

				ret = RecvStream(type, fhandle, buff, 6);
				if (ret != 0) {
					break;
				}
				memcpy(object_node_id, buff, 6);

				g_pShiyong->DropRouteItem(node_type, object_node_id);

				if (g_pShiyong->ShouldDoHttpOP())//Root Node, DoDrop()
				{
					HttpOperate::DoDrop("gbk", "zh", g_pShiyong->device_node_id, (node_type == ROUTE_ITEM_TYPE_VIEWERNODE), object_node_id);
				}
				else
				{//优先选择Primary通道，向上转发。。。
					for (int i = 0; i < MAX_VIEWER_NUM; i++)
					{
						if (g_pShiyong->viewerArray[i].bUsing == FALSE || g_pShiyong->viewerArray[i].bConnected == FALSE) {
							continue;
						}
						if (g_pShiyong->viewerArray[i].bTopoPrimary == TRUE)
						{
							CtrlCmd_TOPO_DROP(g_pShiyong->viewerArray[i].httpOP.m1_use_sock_type, g_pShiyong->viewerArray[i].httpOP.m1_use_udt_sock, node_type, object_node_id);
							break;
						}
					}
				}

				break;

			case CMD_CODE_TOPO_EVALUATE: //RepeaterNode 的 rudp server 向上转发来的

				ret = RecvStream(type, fhandle, buff, 6);
				if (ret != 0) {
					break;
				}
				memcpy(source_node_id, buff, 6);

				ret = RecvStream(type, fhandle, buff, 6);
				if (ret != 0) {
					break;
				}
				memcpy(object_node_id, buff, 6);

				ret = RecvStream(type, fhandle, buff, 4);
				if (ret != 0) {
					break;
				}
				begin_time = ntohl(pf_get_dword(buff));

				ret = RecvStream(type, fhandle, buff, 4);
				if (ret != 0) {
					break;
				}
				end_time = ntohl(pf_get_dword(buff));

				ret = RecvStream(type, fhandle, buff, 4);
				if (ret != 0) {
					break;
				}
				stream_flow = ntohl(pf_get_dword(buff));


				if (g_pShiyong->ShouldDoHttpOP())//Root Node, DoEvaluate()
				{

				}
				else
				{//优先选择Secondary通道，向上转发。。。
					BOOL bFoundSecondary = FALSE;
					for (int i = 0; i < MAX_VIEWER_NUM; i++)
					{
						if (g_pShiyong->viewerArray[i].bUsing == FALSE || g_pShiyong->viewerArray[i].bConnected == FALSE) {
							continue;
						}
						if (g_pShiyong->viewerArray[i].bTopoPrimary == FALSE)
						{
							bFoundSecondary = TRUE;
							CtrlCmd_TOPO_EVALUATE(g_pShiyong->viewerArray[i].httpOP.m1_use_sock_type, g_pShiyong->viewerArray[i].httpOP.m1_use_udt_sock, source_node_id, object_node_id, begin_time, end_time, stream_flow);
							break;
						}
					}
					if (FALSE == bFoundSecondary)
					{
						for (int i = 0; i < MAX_VIEWER_NUM; i++)
						{
							if (g_pShiyong->viewerArray[i].bUsing == FALSE || g_pShiyong->viewerArray[i].bConnected == FALSE) {
								continue;
							}
							//if (g_pShiyong->viewerArray[i].bTopoPrimary == TRUE)
							{
								CtrlCmd_TOPO_EVALUATE(g_pShiyong->viewerArray[i].httpOP.m1_use_sock_type, g_pShiyong->viewerArray[i].httpOP.m1_use_udt_sock, source_node_id, object_node_id, begin_time, end_time, stream_flow);
								break;
							}
						}
					}
				}

				break;

			case CMD_CODE_TOPO_PACKET: //RepeaterNode 的 rudp server 向上转发来的

				break;

			default:
				break;
	}
}

#ifdef WIN32
DWORD WINAPI IpcThreadFn(LPVOID pvThreadParam)
#else
void *IpcThreadFn(void *pvThreadParam)
#endif
{
	SOCKET server;
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);
	int ret;
	int count = 0;
	struct timeval waitval;
	fd_set allset;
	int max_fd;


	server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server == INVALID_SOCKET) {
		printf("IpcThreadFn: TCP socket() failed!\n");
		return 0;
	}

	int timeo = 5000;
	setsockopt(server, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeo, sizeof(timeo));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(IPC_SERVER_BIND_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(server, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		printf("IpcThreadFn: TCP bind() failed!\n");
		closesocket(server);
		server = INVALID_SOCKET;
		return 0;
	}

	listen(server, 10);

	while (count < MAX_SERVER_NUM)
	{
		if (NULL == arrServerProcesses) {
			usleep(3000*1000);
			continue;
		}

		SOCKET fhandle = accept(server, (sockaddr*)&their_addr, &namelen);
		if (INVALID_SOCKET == fhandle) {
			printf("IpcThreadFn: TCP accept() failed!\n");
			usleep(10*1000);
			continue;
		}

		arrServerProcesses[count].m_fhandle = fhandle;
		count += 1;
	}

	printf("IpcThreadFn: OK!!! All ipc clients connected ###########\n");

	while (true)
	{
		FD_ZERO(&allset);
		waitval.tv_sec  = 1;
		waitval.tv_usec = 100000;
		max_fd = 0;
		for (int i = 0; i < MAX_SERVER_NUM; i++)
		{
			if (INVALID_SOCKET == arrServerProcesses[i].m_fhandle) {
				continue;
			}
			FD_SET(arrServerProcesses[i].m_fhandle, &allset);
			if (arrServerProcesses[i].m_fhandle > max_fd) {
				max_fd = arrServerProcesses[i].m_fhandle;
			}
		}
		ret = select(max_fd + 1, &allset, NULL, NULL, &waitval);
		if (ret < 0) {
			printf("IpcThreadFn: select() failed!\n");
			usleep(10*1000);
			continue;
		}
		for (int i = 0; i < MAX_SERVER_NUM; i++)
		{
			if (FD_ISSET(arrServerProcesses[i].m_fhandle, &allset))
			{
				OnIpcMsg(&(arrServerProcesses[i]), arrServerProcesses[i].m_fhandle);
			}
		}
	}

	closesocket(server);
	server = INVALID_SOCKET;
}

void StartServerProcesses()
{
#ifdef WIN32
	DWORD dwThreadID;
	HANDLE hThread = ::CreateThread(NULL,0,IpcThreadFn,NULL,0,&dwThreadID);
	if (hThread == NULL)
#else
	pthread_t hThread;
	int err = pthread_create(&hThread, NULL, IpcThreadFn, NULL);
	if (0 != err)
#endif
	{
		printf("Create IpcThreadFn failed!\n");/* Error */
	}


	if (MAX_SERVER_NUM > FD_SETSIZE)
	{
		printf("MAX_SERVER_NUM <== FD_SETSIZE(%d)\n", FD_SETSIZE);
		MAX_SERVER_NUM = FD_SETSIZE;
	}

	arrServerProcesses = (SERVER_PROCESS_NODE *)malloc(sizeof(SERVER_PROCESS_NODE) * MAX_SERVER_NUM);

	for (int i = 0; i < MAX_SERVER_NUM; i++ )
	{
		printf("ServerProcessNode %d online...\n", i+1);

		arrServerProcesses[i].m_nIndex = i;

		arrServerProcesses[i].m_fhandle = INVALID_SOCKET;

		arrServerProcesses[i].m_bAVStarted = FALSE;

		arrServerProcesses[i].m_bVideoEnable = FALSE;
		arrServerProcesses[i].m_bAudioEnable = FALSE;
		arrServerProcesses[i].m_bTLVEnable = FALSE;

		char szExeCmd[MAX_PATH];
		int p2p_port = FIRST_CONNECT_PORT + i*4;

		_snprintf(szExeCmd, MAX_PATH, "RepeaterNode.exe  %s %d %d %s %s %d %d %d %d %d %s", SERVER_TYPE, UUID_EXT, MAX_SERVER_NUM, NODE_NAME, CONNECT_PASSWORD, p2p_port, IPC_SERVER_BIND_PORT, g_video_width, g_video_height, g_video_fps, g_tcp_address);
		RunExeNoWait(szExeCmd, FALSE);

		usleep(200*1000);//保证RepeaterNode.exe 生成的受控端node_id不重复
	}
}

int GetAvClientsCount()
{
	int count = 0;
	if (NULL == arrServerProcesses) {
		return count;
	}
	for (int i = 0; i < MAX_SERVER_NUM; i++ )
	{
		if (arrServerProcesses[i].m_bAVStarted) {
			count += 1;
		}
	}
	return count;
}
