// Guaji.cpp : 实现文件
//

//#include "stdafx.h"
#include "Repeater.h"
#include "PacketRepeater.h"
#include "IpcGuaji.h"

#include "platform.h"
#include "CommonLib.h"
#include "HttpOperate.h"
#include "phpMd5.h"
#include "base64.h"
#include "AppSettings.h"
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

static void DoSendAvPacket(SERVER_PROCESS_NODE *pServerPorcess, wps_queue **queue_item)
{
	if (NULL == queue_item || NULL == *queue_item) {
		return;
	}

	wps_queue *next_item = (*queue_item)->q_forw;

	RECV_PACKET_SMALL *p = (RECV_PACKET_SMALL *)get_qbody(*queue_item);
	int ret = CtrlCmd_Send_FAKERTP_RESP(SOCKET_TYPE_TCP, pServerPorcess->m_fhandle, p->szData, p->nDataSize);
	if (ret < 0) {
		log_msg("SendPacketsInQueue: CtrlCmd_Send_FAKERTP_RESP() failed!", LOG_LEVEL_ERROR);
	}

	*queue_item = next_item;
}

static void SendPacketsInQueue(SERVER_PROCESS_NODE *pServerPorcess, DWORD dwAudioSeq, DWORD dwVideoSeq)
{
	wps_queue *temp;
	wps_queue *tail;
	RECV_PACKET_SMALL *p;
	wps_queue *found_video_item = NULL;
	wps_queue *found_audio_item = NULL;
	double dfTimestamp = 0.0f;

	if (NULL == g_pShiyong->arrayRecvQueue[PAYLOAD_VIDEO] || NULL == g_pShiyong->arrayRecvQueue[PAYLOAD_AUDIO])
	{
		log_msg("SendPacketsInQueue: RecvQueue is empty!!!", LOG_LEVEL_INFO);
		return;
	}

	pthread_mutex_lock(&(g_pShiyong->arrayCriticalSec[PAYLOAD_AUDIO]));
	pthread_mutex_lock(&(g_pShiyong->arrayCriticalSec[PAYLOAD_VIDEO]));

	if (dwAudioSeq == 0xffffffffUL && dwVideoSeq == 0xffffffffUL)
	{
		if (NULL != g_pShiyong->arrayRecvQueue[PAYLOAD_VIDEO] && NULL != g_pShiyong->arrayRecvQueue[PAYLOAD_AUDIO])
		{
			tail = g_pShiyong->arrayRecvQueue[PAYLOAD_VIDEO];

			while (tail->q_forw != NULL) tail = tail->q_forw;

			temp = tail;
			while (temp) {
				p = (RECV_PACKET_SMALL *)get_qbody(temp);
				BYTE bType = CheckNALUType(p->szData[8]);
				if (bType == NALU_TYPE_SEQ_SET)//SPS -> PPS -> IDR ->...
				{
					found_video_item = temp;
					dfTimestamp = p->dfRecvTime;
					break;
				}
				temp = temp->q_back;
			}//while (temp)

			if (NULL != found_video_item)
			{
				log_msg("SendPacketsInQueue: Found video SPS packet...", LOG_LEVEL_INFO);

				tail = g_pShiyong->arrayRecvQueue[PAYLOAD_AUDIO];

				while (tail->q_forw != NULL) tail = tail->q_forw;

				temp = tail;
				while (temp) {
					p = (RECV_PACKET_SMALL *)get_qbody(temp);
					if (p->dfRecvTime <= dfTimestamp)
					{
						found_audio_item = temp;
						break;
					}
					temp = temp->q_back;
				}//while (temp)

			}//if (NULL != found_video_item)
			else {
				log_msg("SendPacketsInQueue: Can not find video SPS packet!!!", LOG_LEVEL_INFO);
			}
		}
	}
	else
	{
		temp = g_pShiyong->arrayRecvQueue[PAYLOAD_VIDEO];
		while (temp) {
			p = (RECV_PACKET_SMALL *)get_qbody(temp);
			if (p->wSeqNum == dwVideoSeq)
			{
				found_video_item = temp;
				break;
			}
			temp = temp->q_forw;
		}//while (temp)
		if (NULL != found_video_item) {
			log_msg("SendPacketsInQueue: Found video packet with certain seq...", LOG_LEVEL_INFO);
		}
		else {
			log_msg("SendPacketsInQueue: Can not find video packet with certain seq!!!", LOG_LEVEL_INFO);
		}

		temp = g_pShiyong->arrayRecvQueue[PAYLOAD_AUDIO];
		while (temp) {
			p = (RECV_PACKET_SMALL *)get_qbody(temp);
			if (p->wSeqNum == dwAudioSeq)
			{
				found_audio_item = temp;
				break;
			}
			temp = temp->q_forw;
		}//while (temp)
		if (NULL != found_audio_item) {
			log_msg("SendPacketsInQueue: Found audio packet with certain seq...", LOG_LEVEL_INFO);
		}
		else {
			log_msg("SendPacketsInQueue: Can not find audio packet with certain seq!!!", LOG_LEVEL_INFO);
		}
	}

	pthread_mutex_unlock(&(g_pShiyong->arrayCriticalSec[PAYLOAD_VIDEO]));
	pthread_mutex_unlock(&(g_pShiyong->arrayCriticalSec[PAYLOAD_AUDIO]));


	if (NULL != found_audio_item && NULL != found_video_item)
	{
		while (TRUE)
		{
			int n = MAX_VIDEO_RECV_QUEUE_SIZE / MAX_AUDIO_RECV_QUEUE_SIZE;
			if (n > 5) 	n = 5;
			if (n < 1) 	n = 1;

			pthread_mutex_lock(&(g_pShiyong->arrayCriticalSec[PAYLOAD_AUDIO]));
			DoSendAvPacket(pServerPorcess, &found_audio_item);
			pthread_mutex_unlock(&(g_pShiyong->arrayCriticalSec[PAYLOAD_AUDIO]));

			pthread_mutex_lock(&(g_pShiyong->arrayCriticalSec[PAYLOAD_VIDEO]));
			for (int i = 0; i < n; i++) {
				DoSendAvPacket(pServerPorcess, &found_video_item);
			}
			pthread_mutex_unlock(&(g_pShiyong->arrayCriticalSec[PAYLOAD_VIDEO]));

			if (NULL == found_audio_item && NULL == found_video_item) {
				break;
			}
			usleep(1*1000);
		}
	}
}

static void OnIpcMsg(SERVER_PROCESS_NODE *pServerPorcess, SOCKET fhandle)
{
	SOCKET_TYPE type = SOCKET_TYPE_TCP;
	int  ret;
	char buff[16*1024];
	WORD wCommand;
	DWORD dwDataLength;
	DWORD dwAudioSeq;
	DWORD dwVideoSeq;
	BYTE is_connected;
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
		perror("RecvStream() error:");
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
				//if_mc_arm();
				pServerPorcess->m_bConnected = TRUE;//借用
				printf("GuajiNodes[%d] Connected!\n", pServerPorcess->m_nIndex);
				break;
				
			case CMD_CODE_DISARM_REQ:
				//if_mc_disarm();
				pServerPorcess->m_bConnected = FALSE;//借用
				printf("GuajiNodes[%d] Disconnected!\n", pServerPorcess->m_nIndex);
				break;

			case CMD_CODE_AV_START_REQ:
				ret = RecvStream(type, fhandle, buff, 1+1+1+4+4);
				if (ret != 0) {
					break;
				}

				dwAudioSeq = ntohl(pf_get_dword(buff+3));
				dwVideoSeq = ntohl(pf_get_dword(buff+7));
				
				pServerPorcess->m_bAVStarted = TRUE;
				pServerPorcess->m_bVideoEnable = ((BYTE)(buff[0]) & AV_FLAGS_VIDEO_ENABLE) != 0;
				pServerPorcess->m_bAudioEnable = ((BYTE)(buff[0]) & AV_FLAGS_AUDIO_ENABLE) != 0;
				pServerPorcess->m_bTLVEnable = FALSE;

				SendPacketsInQueue(pServerPorcess, dwAudioSeq, dwVideoSeq);

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

			case CMD_CODE_MAV_START_REQ:
				break;

			case CMD_CODE_MAV_STOP_REQ:
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

				snprintf(pServerPorcess->m_szReport, sizeof(pServerPorcess->m_szReport), 
					"row=%d"//"node_type=%d"
					"|%d"//"&topo_level=%d"
					"|%02X-%02X-%02X-%02X-%02X-%02X"//"&owner_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
					"|"
					,
					ROUTE_ITEM_TYPE_GUAJINODE,
					g_pShiyong->device_topo_level,
					g_pShiyong->device_node_id[0],g_pShiyong->device_node_id[1],g_pShiyong->device_node_id[2],g_pShiyong->device_node_id[3],g_pShiyong->device_node_id[4],g_pShiyong->device_node_id[5]);


				if (dwDataLength - 12 + strlen(pServerPorcess->m_szReport) <= sizeof(pServerPorcess->m_szReport)) {
					ret = RecvStream(type, fhandle, pServerPorcess->m_szReport + strlen(pServerPorcess->m_szReport), dwDataLength - 12);
					if (ret == 0) {
						strcat(pServerPorcess->m_szReport, "\n");
						g_pShiyong->UpdateRouteTable(pServerPorcess->m_nIndex, pServerPorcess->m_szReport);
					}
				}
				else {
					log_msg_f(LOG_LEVEL_ERROR, "OnIpcMsg: m_szReport overflow!!!");
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
				is_connected = *(BYTE *)buff;

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

				if (is_connected == 0) {//真的Exit了
					g_pShiyong->DropRouteItem(node_type, object_node_id);
				}

				if (is_connected != 0 && node_type == ROUTE_ITEM_TYPE_VIEWERNODE)
				{
					int found_item = g_pShiyong->FindConnectingItemViewerNode(object_node_id);
					if (found_item != -1) {//从EventTable中删除。。。
						g_pShiyong->connecting_event_table[found_item].bUsing = FALSE;
						g_pShiyong->connecting_event_table[found_item].nID = -1;
					}
					if (found_item != -1 && g_pShiyong->connecting_event_table[found_item].switch_after_connected) {
						//
						int ret_index = g_pShiyong->FindTopoRouteItem(object_node_id);
						if (-1 != ret_index)
						{
							char szEvent[64];
							snprintf(szEvent, sizeof(szEvent), "%02X-%02X-%02X-%02X-%02X-%02X|0|Switch|00-00-00-00-00-00", 
										object_node_id[0],object_node_id[1],object_node_id[2],object_node_id[3],object_node_id[4],object_node_id[5]);

							log_msg_f(LOG_LEVEL_INFO, "OnIpcMsg: CMD_CODE_TOPO_DROP and Switch, szEvent=%s\n", szEvent);

							int guaji_index = g_pShiyong->device_route_table[ret_index].guaji_index;
							CtrlCmd_TOPO_EVENT(SOCKET_TYPE_TCP, arrServerProcesses[guaji_index].m_fhandle, object_node_id, (const char *)szEvent);
						}
					}
				}

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
							CtrlCmd_TOPO_DROP(g_pShiyong->viewerArray[i].httpOP.m1_use_sock_type, g_pShiyong->viewerArray[i].httpOP.m1_use_udt_sock, is_connected, node_type, object_node_id);
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


				if (strstr(g0_device_uuid, "-1@1") != NULL)//Root Node, DoEvaluate()
				{
					int retIndex = g_pShiyong->FindRouteNode(object_node_id);
					if (retIndex != -1)
					{
						BYTE *pNodeId = g_pShiyong->device_route_table[retIndex].owner_node_id;

						char szTemp[256];
						snprintf(szTemp, sizeof(szTemp), 
							"%02X-%02X-%02X-%02X-%02X-%02X"//source_node_id
							"|%02X-%02X-%02X-%02X-%02X-%02X"//object_device_node_id
							"|%02X-%02X-%02X-%02X-%02X-%02X"//object_guaji_node_id
							"|%lu"//begin_time
							"|%lu"//end_time
							"|%lu"//stream_flow
							,
							source_node_id[0],source_node_id[1],source_node_id[2],source_node_id[3],source_node_id[4],source_node_id[5],
							pNodeId[0],pNodeId[1],pNodeId[2],pNodeId[3],pNodeId[4],pNodeId[5],
							object_node_id[0],object_node_id[1],object_node_id[2],object_node_id[3],object_node_id[4],object_node_id[5],
							begin_time,
							end_time,
							stream_flow);

						if (g_pShiyong->szEvaluateRecordBuff[0] == '\0')
						{
							strcpy(g_pShiyong->szEvaluateRecordBuff, szTemp);
						}
						else if (strlen(g_pShiyong->szEvaluateRecordBuff) + strlen(szTemp) < sizeof(g_pShiyong->szEvaluateRecordBuff) - 2)
						{
							strcat(g_pShiyong->szEvaluateRecordBuff, ";");
							strcat(g_pShiyong->szEvaluateRecordBuff, szTemp);
						}

						if (strlen(g_pShiyong->szEvaluateRecordBuff) > sizeof(g_pShiyong->szEvaluateRecordBuff) - 256)
						{
							ret = HttpOperate::DoEvaluate("gbk", "zh", g_pShiyong->device_node_id, g_pShiyong->szEvaluateRecordBuff);
							log_msg_f(LOG_LEVEL_DEBUG, "DoEvaluate() = %d \n", ret);
							if (ret != -1) {
								strcpy(g_pShiyong->szEvaluateRecordBuff, "");
							}
						}
					}//if (retIndex != -1)
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
	socklen_t namelen = sizeof(their_addr);
	int ret;
	int count = 0;
	struct timeval waitval;
	fd_set allset;
	int max_fd;

	if (NULL == arrServerProcesses) {
		usleep(2000*1000);
	}

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
			usleep(2000*1000);
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
		arrServerProcesses[i].m_nIndex = i;

		arrServerProcesses[i].m_fhandle = INVALID_SOCKET;
		memset(arrServerProcesses[i].m_node_id, 0, 6);
		strcpy(arrServerProcesses[i].m_szReport, "");

		arrServerProcesses[i].m_bConnected = FALSE;
		arrServerProcesses[i].m_bAVStarted = FALSE;

		arrServerProcesses[i].m_bVideoEnable = FALSE;
		arrServerProcesses[i].m_bAudioEnable = FALSE;
		arrServerProcesses[i].m_bTLVEnable = FALSE;
	}


	for (int i = 0; i < MAX_SERVER_NUM; i++ )
	{
		printf("ServerProcessNode %d online...\n", i+1);

		char szExeCmd[MAX_PATH];
		char *argv[15];
		int ai = 0;
		int p2p_port = FIRST_CONNECT_PORT + i*4;

		//snprintf(szExeCmd, MAX_PATH, "RepeaterNode.exe  %s %d %d %s %s %d %d %d %d %d %s", SERVER_TYPE, UUID_EXT, MAX_SERVER_NUM, NODE_NAME, CONNECT_PASSWORD, p2p_port, IPC_SERVER_BIND_PORT, g_video_width, g_video_height, g_video_fps, g_tcp_address);
		//RunExeNoWait(szExeCmd, FALSE);
		
		snprintf(szExeCmd, MAX_PATH, "%s/RepeaterNode", gszProgramDir);
		printf("szExeCmd=%s\n", szExeCmd);
		
		argv[ai++] = szExeCmd;
		argv[ai++] = SERVER_TYPE;
		char str_UUID_EXT[MAX_PATH];       snprintf(str_UUID_EXT, MAX_PATH, "%d", UUID_EXT); argv[ai++] = str_UUID_EXT;
		char str_MAX_SERVER_NUM[MAX_PATH]; snprintf(str_MAX_SERVER_NUM, MAX_PATH, "%d", MAX_SERVER_NUM); argv[ai++] = str_MAX_SERVER_NUM;
		argv[ai++] = NODE_NAME;
		argv[ai++] = CONNECT_PASSWORD;
		char str_p2p_port[MAX_PATH];             snprintf(str_p2p_port, MAX_PATH, "%d", p2p_port); argv[ai++] = str_p2p_port;
		char str_IPC_SERVER_BIND_PORT[MAX_PATH]; snprintf(str_IPC_SERVER_BIND_PORT, MAX_PATH, "%d", IPC_SERVER_BIND_PORT); argv[ai++] = str_IPC_SERVER_BIND_PORT;
		char str_g_video_width[MAX_PATH];        snprintf(str_g_video_width, MAX_PATH, "%d", g_video_width); argv[ai++] = str_g_video_width;
		char str_g_video_height[MAX_PATH];       snprintf(str_g_video_height, MAX_PATH, "%d", g_video_height); argv[ai++] = str_g_video_height;
		char str_g_video_fps[MAX_PATH];          snprintf(str_g_video_fps, MAX_PATH, "%d", g_video_fps); argv[ai++] = str_g_video_fps;
		argv[ai++] = g_tcp_address;
		argv[ai++] = (char *)0;
		
		pid_t pid = fork();
		if (pid < 0) {
			printf("fork() failed!\n");
		}
		else if (pid == 0) {
			//int null = open("/dev/null", O_WRONLY | O_CLOEXEC);
			//dup2(null, STDIN_FILENO);
		    if (execv(szExeCmd, argv) < 0) {
		    	perror("execv() error:");
		    }
		    _exit(0);
		}
		else {
			
		}
		
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
