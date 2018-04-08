// Shiyong.cpp : 实现文件
//

#include "stdafx.h"
#include "Repeater.h"
#include "Shiyong.h"

#include "platform.h"
#include "CommonLib.h"
#include "HttpOperate.h"
#include "phpMd5.h"
#include "base64.h"
#include "Discovery.h"
#include "Eloop.h"
#include "PacketRepeater.h"


int g_video_width  = 640;
int g_video_height = 480;
int g_video_fps = 25;


CShiyong* g_pShiyong = NULL;

DWORD WINAPI ConnectThreadFn(LPVOID lpParameter);
DWORD WINAPI ConnectThreadFn2(LPVOID lpParameter);
DWORD WINAPI ConnectThreadFnRev(LPVOID lpParameter);

static int lastDoReportTime = 0;
void OnReportSettings(char *settings_str);
void OnReportEvent(char *event_str);


#define strProductName   ("Repeater")
#define SetStatusInfo(hWnd, lpStatusInfo)   do {printf(lpStatusInfo);	printf("\n");} while(0)


static void HandleKeepLoop(void *eloop_ctx, void *timeout_ctx)
{
	g_pShiyong->CheckTopoRouteTable();

	if (g_pShiyong->device_topo_level <= 1)
	{
		int now = time(NULL);
		if (now - lastDoReportTime >= g1_register_period)
		{
			lastDoReportTime = now;
			HttpOperate::DoReport2("gbk", "zh", g_pShiyong->joined_channel_id, g_pShiyong->joined_node_id, 
				g0_device_uuid, g_pShiyong->get_public_ip(), g_pShiyong->device_node_id, 
				g_pShiyong->get_route_item_num(), MAX_ROUTE_ITEM_NUM, 
				g_pShiyong->get_level_max_connections(1), g_pShiyong->get_level_current_connections(1), g_pShiyong->get_level_max_streams(1), g_pShiyong->get_level_current_streams(1),
				g_pShiyong->get_level_max_connections(2), g_pShiyong->get_level_current_connections(2), g_pShiyong->get_level_max_streams(2), g_pShiyong->get_level_current_streams(2),
				g_pShiyong->get_level_max_connections(3), g_pShiyong->get_level_current_connections(3), g_pShiyong->get_level_max_streams(3), g_pShiyong->get_level_current_streams(3),
				g_pShiyong->get_level_max_connections(4), g_pShiyong->get_level_current_connections(4), g_pShiyong->get_level_max_streams(4), g_pShiyong->get_level_current_streams(4),
				g_pShiyong->get_node_array(), OnReportSettings, OnReportEvent);
		}
	}

	eloop_register_timeout(1, 0, HandleKeepLoop, NULL, NULL);
}

static void HandleTopoReport(void *eloop_ctx, void *timeout_ctx)
{
	g_pShiyong->DeviceTopoReport();//非Root节点
}

static void HandleDoUnregister(void *eloop_ctx, void *timeout_ctx)
{
	VIEWER_NODE *pViewerNode = (VIEWER_NODE *)eloop_ctx;
	BYTE node_type = ROUTE_ITEM_TYPE_VIEWERNODE;
	BYTE *object_node_id = pViewerNode->httpOP.m0_node_id;

	g_pShiyong->DropRouteItem(node_type, object_node_id);

	if (g_pShiyong->device_topo_level <= 1)//Root Node, DoDrop()
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
}

static void HandleRegister(void *eloop_ctx, void *timeout_ctx)
{
	VIEWER_NODE *pViewerNode = (VIEWER_NODE *)eloop_ctx;
	int ret;
	StunAddress4 mapped;
	static BOOL bNoNAT;
	static int  nNatType;
	DWORD ipArrayTemp[MAX_ADAPTER_NUM];  /* Network byte order */
	int ipCountTemp;


	::EnterCriticalSection(&(pViewerNode->localbind_csec));////////

	/* STUN Information */
	if (strcmp(g1_stun_server, "NONE") == 0)
	{
		ret = CheckStunMyself(g1_http_server, pViewerNode->httpOP.m0_p2p_port, &mapped, &bNoNAT, &nNatType, &(pViewerNode->httpOP.m0_net_time));
		if (ret == -1) {
		   TRACE("CheckStunMyself: fialed!\n");

		   mapped.addr = ntohl(pViewerNode->httpOP.m1_http_client_ip);
		   mapped.port = pViewerNode->httpOP.m0_p2p_port;
		   
		   Socket s = openPort( 0/*use ephemeral*/, mapped.addr, false);
		   if (s != INVALID_SOCKET)
		   {
		      closesocket(s);
		      bNoNAT = TRUE;
		      nNatType = StunTypeOpen;
		   }
		   else
		   {
		      bNoNAT = FALSE;
		      nNatType = StunTypeIndependentFilter;
		   }
		   
			SetStatusInfo(pDlgWnd, "本端NAT探测成功! - CheckStunHttp");
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
			pViewerNode->httpOP.m0_no_nat = bNoNAT;
			pViewerNode->httpOP.m0_nat_type = nNatType;
			TRACE("CheckStunHttp: %d.%d.%d.%d[%d], NoNAT=%d\n", 
				(pViewerNode->httpOP.m0_pub_ip & 0x000000ff) >> 0,
				(pViewerNode->httpOP.m0_pub_ip & 0x0000ff00) >> 8,
				(pViewerNode->httpOP.m0_pub_ip & 0x00ff0000) >> 16,
				(pViewerNode->httpOP.m0_pub_ip & 0xff000000) >> 24,
				pViewerNode->httpOP.m0_pub_port,  pViewerNode->httpOP.m0_no_nat ? 1 : 0);
		}
		else {
			SetStatusInfo(pDlgWnd, "本端NAT探测成功!");
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
			pViewerNode->httpOP.m0_no_nat = bNoNAT;
			pViewerNode->httpOP.m0_nat_type = nNatType;
			TRACE("CheckStunMyself: %d.%d.%d.%d[%d]\n", 
				(pViewerNode->httpOP.m0_pub_ip & 0x000000ff) >> 0,
				(pViewerNode->httpOP.m0_pub_ip & 0x0000ff00) >> 8,
				(pViewerNode->httpOP.m0_pub_ip & 0x00ff0000) >> 16,
				(pViewerNode->httpOP.m0_pub_ip & 0xff000000) >> 24,
				pViewerNode->httpOP.m0_pub_port);
		}
	}
	else if (pViewerNode->bFirstCheckStun)
	{
		ret = CheckStun(g1_stun_server, pViewerNode->httpOP.m0_p2p_port, &mapped, &bNoNAT, &nNatType);
		if (ret == -1) {
		   TRACE("CheckStun: fialed!\n");

		   mapped.addr = ntohl(pViewerNode->httpOP.m1_http_client_ip);
		   mapped.port = pViewerNode->httpOP.m0_p2p_port;
		   
		   Socket s = openPort( 0/*use ephemeral*/, mapped.addr, false);
		   if (s != INVALID_SOCKET)
		   {
		      closesocket(s);
		      bNoNAT = TRUE;
		      nNatType = StunTypeOpen;
		   }
		   else
		   {
		      bNoNAT = FALSE;
		      nNatType = StunTypeIndependentFilter;
		   }
		   
			SetStatusInfo(pDlgWnd, "本端NAT探测成功! - CheckStunHttp");
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
			pViewerNode->httpOP.m0_no_nat = bNoNAT;
			pViewerNode->httpOP.m0_nat_type = nNatType;
			TRACE("CheckStunHttp: %d.%d.%d.%d[%d], NoNAT=%d\n", 
				(pViewerNode->httpOP.m0_pub_ip & 0x000000ff) >> 0,
				(pViewerNode->httpOP.m0_pub_ip & 0x0000ff00) >> 8,
				(pViewerNode->httpOP.m0_pub_ip & 0x00ff0000) >> 16,
				(pViewerNode->httpOP.m0_pub_ip & 0xff000000) >> 24,
				pViewerNode->httpOP.m0_pub_port,  pViewerNode->httpOP.m0_no_nat ? 1 : 0);
		}
		else if (ret == 0) {
			SetStatusInfo(pDlgWnd, "本端NAT无法穿透!!!您的网络环境可能封锁了UDP通信。");
			pViewerNode->httpOP.m0_pub_ip = 0;
			pViewerNode->httpOP.m0_pub_port = 0;
			pViewerNode->httpOP.m0_no_nat = FALSE;
			pViewerNode->httpOP.m0_nat_type = nNatType;
		}
		else {
			SetStatusInfo(pDlgWnd, "本端NAT探测成功!");
			pViewerNode->bFirstCheckStun = FALSE;
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
			pViewerNode->httpOP.m0_no_nat = bNoNAT;
			pViewerNode->httpOP.m0_nat_type = nNatType;
			TRACE("CheckStun: %d.%d.%d.%d[%d]\n", 
				(pViewerNode->httpOP.m0_pub_ip & 0x000000ff) >> 0,
				(pViewerNode->httpOP.m0_pub_ip & 0x0000ff00) >> 8,
				(pViewerNode->httpOP.m0_pub_ip & 0x00ff0000) >> 16,
				(pViewerNode->httpOP.m0_pub_ip & 0xff000000) >> 24,
				pViewerNode->httpOP.m0_pub_port);
		}
	}
	else {
		TRACE("CheckStunSimple......\n");
		ret = CheckStunSimple(g1_stun_server, pViewerNode->httpOP.m0_p2p_port, &mapped);
		if (ret == -1) {
			TRACE("CheckStunSimple: fialed!\n");
			mapped.addr = ntohl(pViewerNode->httpOP.m1_http_client_ip);
			mapped.port = pViewerNode->httpOP.m0_p2p_port;
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
		}
		else {
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
			TRACE("CheckStunSimple: %d.%d.%d.%d[%d]\n", 
				(pViewerNode->httpOP.m0_pub_ip & 0x000000ff) >> 0,
				(pViewerNode->httpOP.m0_pub_ip & 0x0000ff00) >> 8,
				(pViewerNode->httpOP.m0_pub_ip & 0x00ff0000) >> 16,
				(pViewerNode->httpOP.m0_pub_ip & 0xff000000) >> 24,
				pViewerNode->httpOP.m0_pub_port);
		}
	}


	//修正公网IP受控端的连接端口问题,2014-6-15
	if (bNoNAT) {
		pViewerNode->httpOP.m0_pub_port = pViewerNode->httpOP.m0_p2p_port - 2;//SECOND_CONNECT_PORT
	}


	/* Local IP Address */
	ipCountTemp = MAX_ADAPTER_NUM;
	if (GetLocalAddress(ipArrayTemp, &ipCountTemp) != NO_ERROR) {
		TRACE("GetLocalAddress: fialed!\n");
	}
	else {
		pViewerNode->httpOP.m0_ipCount = ipCountTemp;
		memcpy(pViewerNode->httpOP.m0_ipArray, ipArrayTemp, sizeof(DWORD)*ipCountTemp);
		pViewerNode->httpOP.m0_port = pViewerNode->httpOP.m0_p2p_port;
	}

	if (pViewerNode->httpOP.m0_no_nat) {
		pViewerNode->httpOP.m0_port = SECOND_CONNECT_PORT;
	}

_OUT:
	::LeaveCriticalSection(&(pViewerNode->localbind_csec));////////


	eloop_register_timeout(g1_register_period, 0, HandleRegister, pViewerNode, NULL);
}

DWORD WINAPI WorkingThreadFn(LPVOID pvThreadParam)
{
	eloop_init(NULL);

	eloop_register_timeout(1, 0, HandleKeepLoop, NULL, NULL);
	for (int i = 0; i < MAX_VIEWER_NUM; i++)
	{
		eloop_register_timeout(1 + i * 2, 0, HandleRegister, &(g_pShiyong->viewerArray[i]), NULL);
	}

	/* Loop... */
	eloop_run();

	eloop_destroy();
	return 0;
}

void OnReportSettings(char *settings_str)
{
	ParseTopoSettings((const char *)settings_str);

	for (int i = 0; i < MAX_SERVER_NUM; i++)
	{
		int ret = CtrlCmd_TOPO_SETTINGS(SOCKET_TYPE_TCP, arrServerProcesses[i].m_fhandle, g_pShiyong->device_topo_level, (const char *)settings_str);
		if (ret < 0) {
			continue;
		}
	}
}

void OnReportEvent(char *event_str)
{
	char szDestNodeId[32];
	BYTE dest_node_id[6];
	char *pRecvData = event_str;

	strncpy(szDestNodeId, event_str, 17);
	szDestNodeId[17] = '\0';
	mac_addr(szDestNodeId, dest_node_id, NULL);

	int index = g_pShiyong->FindViewerNode(dest_node_id);
	if (-1 != index)
	{
		g_pShiyong->viewerArray[index].httpOP.ParseEventValue((char *)pRecvData);
		if (stricmp(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "Connect") == 0)
		{
			strcpy(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "");
			g_pShiyong->ConnectNode(index, REPEATER_PASSWORD);
		}
		else if (stricmp(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "ConnectRev") == 0)
		{
			strcpy(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "");
			g_pShiyong->ConnectRevNode(index, REPEATER_PASSWORD);
		}
		else if (stricmp(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "Disconnect") == 0)
		{
			strcpy(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "");
			g_pShiyong->DisconnectNode(&(g_pShiyong->viewerArray[index]));
		}
	}
	else {
		index = g_pShiyong->FindTopoRouteItem(dest_node_id);
		if (-1 != index)
		{
			int guaji_index = g_pShiyong->device_route_table[index].guaji_index;
			CtrlCmd_TOPO_EVENT(SOCKET_TYPE_TCP, arrServerProcesses[guaji_index].m_fhandle, dest_node_id, (const char *)pRecvData);
		}
	}
}

//
// Return values:
// -1: Network error
//  0: NG
//  1: OK
//
int CheckPassword(VIEWER_NODE *pViewerNode, SOCKET_TYPE sock_type, SOCKET fhandle, int nRetry)
{
	DWORD dwServerVersion;
	BYTE bFuncFlags;
	BYTE bTopoLevel;
	WORD wResult;
	char szPassEnc[MAX_PATH];


	//检查看是否第一个rudp连接，第一个rudp连接应该是TopoPrimary
	BOOL hasConnection = FALSE;
	for (int i = 0; i < MAX_VIEWER_NUM; i++)
	{
		if (g_pShiyong->viewerArray[i].bUsing == FALSE) {
			continue;
		}
		if (g_pShiyong->viewerArray[i].bConnected) {
			hasConnection = TRUE;
			break;
		}
	}
	if (hasConnection) {
		pViewerNode->bTopoPrimary = FALSE;
	}
	else {
		pViewerNode->bTopoPrimary = TRUE;
	}

	php_md5(pViewerNode->password, szPassEnc, sizeof(szPassEnc));

	if (CtrlCmd_HELLO(sock_type, fhandle, pViewerNode->httpOP.m0_node_id, g0_version, pViewerNode->bTopoPrimary, szPassEnc, pViewerNode->httpOP.m1_peer_node_id, &dwServerVersion, &bFuncFlags, &bTopoLevel, &wResult) == 0) {
		if (wResult == CTRLCMD_RESULT_OK) {
			//set_encdec_key((unsigned char *)szPassEnc, strlen(szPassEnc));
			//nodeInfo->func_flags = bFuncFlags;
			g_pShiyong->device_topo_level = bTopoLevel + 1;
			return 1;
		}
		else {
			return 0;
		}
	}
	else {
		return -1;
	}
}

static void RecvSocketDataLoop(VIEWER_NODE *pViewerNode, SOCKET_TYPE ftype, SOCKET fhandle)
{
	BOOL bRecvEnd = FALSE;

	if (fhandle == INVALID_SOCKET) {
		return;
	}

	while (FALSE == pViewerNode->bQuitRecvSocketLoop)
	{
		char buf[32];
		WORD wCmd;
		DWORD copy_len;
		BYTE topo_level;
		BYTE dest_node_id[6];
		int index;
		unsigned char *pRecvData;
		int ret;


		if (RecvStream(ftype, fhandle, buf, 6) < 0)
		{
			break;
		}

		wCmd = ntohs(pf_get_word(buf + 0));
		copy_len = ntohl(pf_get_dword(buf + 2));
		//assert(wCmd == CMD_CODE_FAKERTP_RESP && copy_len > 0);

		switch (wCmd)
		{
		case CMD_CODE_FAKERTP_RESP:

			pRecvData = (unsigned char *)malloc(copy_len);
			if (NULL == pRecvData) {
#if LOG_MSG
				log_msg("RecvSocketDataLoop: No memory!\n", LOG_LEVEL_DEBUG);
#endif
				return;
			}


			ret = RecvStream(ftype, fhandle, (char *)pRecvData, copy_len);
			if (ret == 0) {
				fake_rtp_recv_fn(pViewerNode, pRecvData[0], ntohl(pf_get_dword(pRecvData + 4)), pRecvData, copy_len);
				free(pRecvData);
			}
			else {
				free(pRecvData);
				return;
			}

			break;

		case CMD_CODE_END:
			bRecvEnd = TRUE;
#if LOG_MSG
			log_msg("RecvSocketDataLoop: CMD_CODE_END?????????\n", LOG_LEVEL_DEBUG);
#endif
			break;

		case CMD_CODE_TOPO_EVENT:

			if (RecvStream(ftype, fhandle, buf, 6) != 0) {
				return;
			}
			if (RecvStream(ftype, fhandle, buf, 6) != 0) {
				return;
			}
			memcpy(dest_node_id, buf, 6);

			pRecvData = (unsigned char *)malloc(copy_len - 12);
			if (RecvStream(ftype, fhandle, (char *)pRecvData, copy_len - 12) != 0) {
				free(pRecvData);
				return;
			}

			index = g_pShiyong->FindViewerNode(dest_node_id);
			if (-1 != index)
			{
				g_pShiyong->viewerArray[index].httpOP.ParseEventValue((char *)pRecvData);
				if (stricmp(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "Connect") == 0)
				{
					strcpy(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "");
					g_pShiyong->ConnectNode(index, REPEATER_PASSWORD);
				}
				else if (stricmp(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "ConnectRev") == 0)
				{
					strcpy(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "");
					g_pShiyong->ConnectRevNode(index, REPEATER_PASSWORD);
				}
				else if (stricmp(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "Disconnect") == 0)
				{
					strcpy(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "");
					g_pShiyong->DisconnectNode(&(g_pShiyong->viewerArray[index]));
				}
			}
			else {
				index = g_pShiyong->FindTopoRouteItem(dest_node_id);
				if (-1 != index)
				{
					int guaji_index = g_pShiyong->device_route_table[index].guaji_index;
					CtrlCmd_TOPO_EVENT(SOCKET_TYPE_TCP, arrServerProcesses[guaji_index].m_fhandle, dest_node_id, (const char *)pRecvData);
				}
			}

			free(pRecvData);

			break;

		case CMD_CODE_TOPO_SETTINGS:

			if (RecvStream(ftype, fhandle, buf, 6) != 0) {
				return;
			}
			if (RecvStream(ftype, fhandle, buf, 6) != 0) {
				return;
			}

			if (RecvStream(ftype, fhandle, buf, 1) != 0) {
				return;
			}
			topo_level = *(BYTE *)buf;
			if (g_pShiyong->device_topo_level > 1 && g_pShiyong->device_topo_level != topo_level + 1) {
#if LOG_MSG
				log_msg("RecvSocketDataLoop: device_topo_level != topo_level + 1\n", LOG_LEVEL_DEBUG);
#endif
			}
			g_pShiyong->device_topo_level = topo_level + 1;

			pRecvData = (unsigned char *)malloc(copy_len - 12 -1);
			if (RecvStream(ftype, fhandle, (char *)pRecvData, copy_len - 12 -1) != 0) {
				free(pRecvData);
				return;
			}

			ParseTopoSettings((const char *)pRecvData);

			for (int i = 0; i < MAX_SERVER_NUM; i++)
			{
				int ret = CtrlCmd_TOPO_SETTINGS(SOCKET_TYPE_TCP, arrServerProcesses[i].m_fhandle, g_pShiyong->device_topo_level, (const char *)pRecvData);
				if (ret < 0) {
					continue;
				}
			}

			if (g_pShiyong->device_topo_level > 1)
			{
				//随机延迟，避免同一时刻大量TopoReport涌向树根
				srand(timeGetTime());
				eloop_register_timeout(0, (rand() % 800 + 200)*1000, HandleTopoReport, NULL, NULL);
			}

			free(pRecvData);

			break;

		default:
#if LOG_MSG
			log_msg("RecvSocketDataLoop: CMD_CODE_???????...\n", LOG_LEVEL_DEBUG);
#endif
			for (int i = 0; i < copy_len; i++) {
				ret = RecvStream(ftype, fhandle, buf, 1);
				if (ret != 0) {
					return;
				}
			}
			break;
		}

	}//while


	if (bRecvEnd == FALSE) {
#if LOG_MSG
		log_msg("RecvSocketDataLoop: CtrlCmd_Recv_AV_END()...\n", LOG_LEVEL_DEBUG);
#endif
		CtrlCmd_Recv_AV_END(ftype, fhandle);
	}
}

void DoInConnection(CShiyong *pDlg, VIEWER_NODE *pViewerNode, BOOL bProxy = FALSE);
void DoInConnection(CShiyong *pDlg, VIEWER_NODE *pViewerNode, BOOL bProxy)
{
	SetStatusInfo(pDlg->m_hWnd, _T("成功建立连接。。。"));
	pViewerNode->bConnected = TRUE;

	//连接是为了视频监控。。。
	if (pViewerNode->bTopoPrimary)
	{
		//必须视频可靠传输，音频非冗余传输！！！
		BYTE bFlags = AV_FLAGS_VIDEO_ENABLE | AV_FLAGS_AUDIO_ENABLE | AV_FLAGS_VIDEO_RELIABLE | AV_FLAGS_VIDEO_H264 | AV_FLAGS_AUDIO_G729A;
		BYTE bVideoSize = AV_VIDEO_SIZE_VGA;
		if (g_video_width == 640 && g_video_height == 480) {
			bVideoSize = AV_VIDEO_SIZE_VGA;
		}
		else if (g_video_width == 480 && g_video_height == 320) {
			bVideoSize = AV_VIDEO_SIZE_480x320;
		}
		else if (g_video_width == 320 && g_video_height == 240) {
			bVideoSize = AV_VIDEO_SIZE_QVGA;
		}
		BYTE bVideoFrameRate = g_video_fps;
		DWORD audioChannel = 0;
		DWORD videoChannel = 0;
		CtrlCmd_AV_START(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock, bFlags, bVideoSize, bVideoFrameRate, audioChannel, videoChannel);
	}

	//Loop...,需要CShiyong::DisconnectNode()才能退出Loop！！！
	pViewerNode->bQuitRecvSocketLoop = FALSE;
	RecvSocketDataLoop(pViewerNode, pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);

	{
		//CtrlCmd_AV_STOP()将在CShiyong::DisconnectNode()中调用
		//CtrlCmd_AV_STOP(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);
	}

	SetStatusInfo(pDlg->m_hWnd, _T("即将断开连接。。。"));
	pViewerNode->bConnected = FALSE;
}


DWORD WINAPI ConnectThreadFn(LPVOID lpParameter)
{
	VIEWER_NODE *pViewerNode = (VIEWER_NODE *)lpParameter;
	CShiyong *pDlg = g_pShiyong;
	TCHAR szStatusStr[MAX_LOADSTRING];
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);
	UDTSOCKET fhandle;
	DWORD use_ip;
	WORD use_port;
	int ret;
	int i, nRetry;


	if (	StunTypeUnknown == pViewerNode->httpOP.m1_peer_nattype ||
			StunTypeFailure == pViewerNode->httpOP.m1_peer_nattype ||
			StunTypeBlocked == pViewerNode->httpOP.m1_peer_nattype         )
	{
		SetStatusInfo(pDlg->m_hWnd, "StunType不支持，无法连接！");
		CloseHandle(pViewerNode->hConnectThread);
		pViewerNode->hConnectThread = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}


	//SetStatusInfo(pDlg->m_hWnd, "正在提交本机连接信息，请稍候。。。");
	//nRetry = 3;
	//do {
	//	HandleRegister(pViewerNode, NULL);
	//} while (nRetry-- > 0 && pViewerNode->bNeedRegister);


	for (i = pViewerNode->httpOP.m1_wait_time; i > 0; i--) {
		_snprintf(szStatusStr, sizeof(szStatusStr), "正在排队等待服务响应，%d秒。。。", i);
		SetStatusInfo(pDlg->m_hWnd, szStatusStr);
		Sleep(1000);
	}


	::EnterCriticalSection(&(pViewerNode->localbind_csec));////////


	pViewerNode->httpOP.m1_use_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (pViewerNode->httpOP.m1_use_udp_sock == INVALID_SOCKET) {
		::LeaveCriticalSection(&(pViewerNode->localbind_csec));////////
		SetStatusInfo(pDlg->m_hWnd, "连接失败！(udp socket failed)");
		CloseHandle(pViewerNode->hConnectThread);
		pViewerNode->hConnectThread = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}

	int flag = 1;
    setsockopt(pViewerNode->httpOP.m1_use_udp_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&flag, sizeof(flag));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(pViewerNode->httpOP.m0_p2p_port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(pViewerNode->httpOP.m1_use_udp_sock, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		::LeaveCriticalSection(&(pViewerNode->localbind_csec));////////
		SetStatusInfo(pDlg->m_hWnd, "连接失败！(udp bind failed)");
		closesocket(pViewerNode->httpOP.m1_use_udp_sock);
		pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;
		CloseHandle(pViewerNode->hConnectThread);
		pViewerNode->hConnectThread = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}


	SetStatusInfo(pDlg->m_hWnd, "正在打通连接信道。。。");
	if (FindOutConnection(pViewerNode->httpOP.m1_use_udp_sock, pViewerNode->httpOP.m0_node_id, pViewerNode->httpOP.m1_peer_node_id, 
							pViewerNode->httpOP.m1_peer_pri_ipArray, pViewerNode->httpOP.m1_peer_pri_ipCount, pViewerNode->httpOP.m1_peer_pri_port, pViewerNode->httpOP.m1_peer_ip, pViewerNode->httpOP.m1_peer_port,
							&use_ip, &use_port) != 0) {
		::LeaveCriticalSection(&(pViewerNode->localbind_csec));////////
		SetStatusInfo(pDlg->m_hWnd, "连接失败！(FindOutConnection failed)");
		closesocket(pViewerNode->httpOP.m1_use_udp_sock);
		pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;
		CloseHandle(pViewerNode->hConnectThread);
		pViewerNode->hConnectThread = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}
	pViewerNode->httpOP.m1_use_peer_ip = use_ip;
	pViewerNode->httpOP.m1_use_peer_port = use_port;
	TRACE("@@@ Use ip/port: %d.%d.%d.%d[%d]\n", 
		(pViewerNode->httpOP.m1_use_peer_ip & 0x000000ff) >> 0,
		(pViewerNode->httpOP.m1_use_peer_ip & 0x0000ff00) >> 8,
		(pViewerNode->httpOP.m1_use_peer_ip & 0x00ff0000) >> 16,
		(pViewerNode->httpOP.m1_use_peer_ip & 0xff000000) >> 24,
		pViewerNode->httpOP.m1_use_peer_port);


	/* 为了节省连接时间，在 eloop 中执行 DoUnregister  */
	eloop_register_timeout(3, 0, HandleDoUnregister, pViewerNode, NULL);


	SetStatusInfo(pDlg->m_hWnd, "正在同对方建立连接，请稍候。。。");

	fhandle = UDT::socket(AF_INET, SOCK_STREAM, 0);

	ConfigUdtSocket(fhandle);

	if (UDT::ERROR == UDT::bind(fhandle, pViewerNode->httpOP.m1_use_udp_sock))
	{
		::LeaveCriticalSection(&(pViewerNode->localbind_csec));////////
		SetStatusInfo(pDlg->m_hWnd, "连接失败！(udt bind failed)");
		UDT::close(fhandle);
		closesocket(pViewerNode->httpOP.m1_use_udp_sock);
		pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;
		CloseHandle(pViewerNode->hConnectThread);
		pViewerNode->hConnectThread = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}


	UDT::listen(fhandle, 1);


///////////////
	struct timeval waitval;

	waitval.tv_sec  = UDT_ACCEPT_TIME;
	waitval.tv_usec = 0;
	ret = UDT::select(fhandle + 1, fhandle, NULL, NULL, &waitval);
	if (ret == UDT::ERROR || ret == 0) {
		::LeaveCriticalSection(&(pViewerNode->localbind_csec));////////
		SetStatusInfo(pDlg->m_hWnd, "暂时性网络状况不好，直连有点小困难，请稍后重试或者采用TCP连接！");
		UDT::close(fhandle);
		closesocket(pViewerNode->httpOP.m1_use_udp_sock);
		pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;
		CloseHandle(pViewerNode->hConnectThread);
		pViewerNode->hConnectThread = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;

	}
//////////////////


	UDTSOCKET fhandle2;
	if ((fhandle2 = UDT::accept(fhandle, (sockaddr*)&their_addr, &namelen)) == UDT::INVALID_SOCK) {
		::LeaveCriticalSection(&(pViewerNode->localbind_csec));////////
		SetStatusInfo(pDlg->m_hWnd, "暂时性网络状况不好，直连有点小困难，请稍后重试或者采用TCP连接！");
		UDT::close(fhandle);
		closesocket(pViewerNode->httpOP.m1_use_udp_sock);
		pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;
		CloseHandle(pViewerNode->hConnectThread);
		pViewerNode->hConnectThread = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}
	else {
		pViewerNode->httpOP.m1_use_peer_ip = their_addr.sin_addr.s_addr;
		pViewerNode->httpOP.m1_use_peer_port = ntohs(their_addr.sin_port);
		UDT::close(fhandle);
		//ConfigUdtSocket(fhandle2);
	}


	pViewerNode->httpOP.m1_use_udt_sock = fhandle2;
	pViewerNode->httpOP.m1_use_sock_type = SOCKET_TYPE_UDT;


	SetStatusInfo(pDlg->m_hWnd, _T("成功建立连接，正在验证访问密码。。。"));
	ret = CheckPassword(pViewerNode, pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock, 2);
	if (-1 == ret) {
		SetStatusInfo(pDlg->m_hWnd, "验证密码时通信出错，无法连接！");
	}
	else if (0 == ret)
	{
		SetStatusInfo(pDlg->m_hWnd, "认证密码错误，无法连接！");
	}
	else
	{

		DoInConnection(pDlg, pViewerNode);

	}

	CtrlCmd_BYE(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);

	Sleep(3000);

	ret = UDT::close(pViewerNode->httpOP.m1_use_udt_sock);
	pViewerNode->httpOP.m1_use_udt_sock = INVALID_SOCKET;
	pViewerNode->httpOP.m1_use_sock_type = SOCKET_TYPE_UNKNOWN;

	ret = closesocket(pViewerNode->httpOP.m1_use_udp_sock);
	pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;

	::LeaveCriticalSection(&(pViewerNode->localbind_csec));////////
	CloseHandle(pViewerNode->hConnectThread);
	pViewerNode->hConnectThread = NULL;
	pDlg->ReturnViewerNode(pViewerNode);
	return 0;
}

DWORD WINAPI ConnectThreadFn2(LPVOID lpParameter)
{
	VIEWER_NODE *pViewerNode = (VIEWER_NODE *)lpParameter;
	CShiyong *pDlg = g_pShiyong;
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	UDTSOCKET fhandle;
	int ret;
	int nRetry;


	pViewerNode->httpOP.m1_use_peer_ip = pViewerNode->httpOP.m1_peer_ip;
	pViewerNode->httpOP.m1_use_peer_port = pViewerNode->httpOP.m1_peer_port;
	//TRACE("@@@ Use ip/port: %d.%d.%d.%d[%d]\n", 
	//	(pViewerNode->httpOP.m1_use_peer_ip & 0x000000ff) >> 0,
	//	(pViewerNode->httpOP.m1_use_peer_ip & 0x0000ff00) >> 8,
	//	(pViewerNode->httpOP.m1_use_peer_ip & 0x00ff0000) >> 16,
	//	(pViewerNode->httpOP.m1_use_peer_ip & 0xff000000) >> 24,
	//	pViewerNode->httpOP.m1_use_peer_port);


	/* 为了节省连接时间，在 eloop 中执行 DoUnregister  */
	eloop_register_timeout(3, 0, HandleDoUnregister, pViewerNode, NULL);


	SetStatusInfo(pDlg->m_hWnd, _T("正在向对方发起连接，请稍候。。。"));

	pViewerNode->httpOP.m1_use_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (pViewerNode->httpOP.m1_use_udp_sock == INVALID_SOCKET) {
		SetStatusInfo(pDlg->m_hWnd, _T("连接失败！(udp socket failed)"));
		CloseHandle(pViewerNode->hConnectThread2);
		pViewerNode->hConnectThread2 = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}

	int flag = 1;
    setsockopt(pViewerNode->httpOP.m1_use_udp_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&flag, sizeof(flag));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(0);/* bind any port */
	my_addr.sin_addr.s_addr = INADDR_ANY;/* bind any ip */
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(pViewerNode->httpOP.m1_use_udp_sock, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		SetStatusInfo(pDlg->m_hWnd, _T("连接失败！(udp bind failed)"));
		closesocket(pViewerNode->httpOP.m1_use_udp_sock);
		pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;
		CloseHandle(pViewerNode->hConnectThread2);
		pViewerNode->hConnectThread2 = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}


	fhandle = UDT::socket(AF_INET, SOCK_STREAM, 0);

	ConfigUdtSocket(fhandle);

	if (UDT::ERROR == UDT::bind(fhandle, pViewerNode->httpOP.m1_use_udp_sock))
	{
		SetStatusInfo(pDlg->m_hWnd, _T("连接失败！(udt bind failed)"));
		UDT::close(fhandle);
		closesocket(pViewerNode->httpOP.m1_use_udp_sock);
		pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;
		CloseHandle(pViewerNode->hConnectThread2);
		pViewerNode->hConnectThread2 = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(pViewerNode->httpOP.m1_use_peer_port);
	their_addr.sin_addr.s_addr = pViewerNode->httpOP.m1_use_peer_ip;
	memset(&(their_addr.sin_zero), '\0', 8);
	nRetry = UDT_CONNECT_TIMES;
	ret = UDT::ERROR;
	while (nRetry-- > 0 && ret == UDT::ERROR) {
		//TRACE("@@@ UDT::connect()...\n");
		ret = UDT::connect(fhandle, (sockaddr*)&their_addr, sizeof(their_addr));
	}
	if (UDT::ERROR == ret)
	{
		SetStatusInfo(pDlg->m_hWnd, _T("连接失败(udt connect failed)，请重试。"));
		UDT::close(fhandle);
		closesocket(pViewerNode->httpOP.m1_use_udp_sock);
		pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;
		CloseHandle(pViewerNode->hConnectThread2);
		pViewerNode->hConnectThread2 = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}


	pViewerNode->httpOP.m1_use_udt_sock = fhandle;
	pViewerNode->httpOP.m1_use_sock_type = SOCKET_TYPE_UDT;


	SetStatusInfo(pDlg->m_hWnd, _T("成功建立连接，正在验证访问密码。。。"));
	ret = CheckPassword(pViewerNode, pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock, 2);
	if (-1 == ret) {
		SetStatusInfo(pDlg->m_hWnd, "验证密码时通信出错，无法连接！");
	}
	else if (0 == ret)
	{
		SetStatusInfo(pDlg->m_hWnd, "认证密码错误，无法连接！");
	}
	else
	{

		DoInConnection(pDlg, pViewerNode);

	}

	CtrlCmd_BYE(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);

	Sleep(3000);

	ret = UDT::close(pViewerNode->httpOP.m1_use_udt_sock);
	pViewerNode->httpOP.m1_use_udt_sock = INVALID_SOCKET;
	pViewerNode->httpOP.m1_use_sock_type = SOCKET_TYPE_UNKNOWN;

	ret = closesocket(pViewerNode->httpOP.m1_use_udp_sock);
	pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;

	CloseHandle(pViewerNode->hConnectThread2);
	pViewerNode->hConnectThread2 = NULL;
	pDlg->ReturnViewerNode(pViewerNode);
	return 0;
}

DWORD WINAPI ConnectThreadFnRev(LPVOID lpParameter)
{
	VIEWER_NODE *pViewerNode = (VIEWER_NODE *)lpParameter;
	CShiyong *pDlg = g_pShiyong;
	TCHAR szStatusStr[MAX_LOADSTRING];
	int namelen;
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	SOCKET udp_sock;
	UDTSOCKET serv;
	UDTSOCKET fhandle;
	int ret;
	int i, nRetry;


	pViewerNode->httpOP.m1_wait_time -= 5;
	if (pViewerNode->httpOP.m1_wait_time < 0) {
		pViewerNode->httpOP.m1_wait_time = 0;
	}

	for (i = pViewerNode->httpOP.m1_wait_time; i > 0; i--) {
		_snprintf(szStatusStr, sizeof(szStatusStr), "正在排队等待服务响应，%d秒。。。", i);
		SetStatusInfo(pDlg->m_hWnd, szStatusStr);
		Sleep(1000);
	}


	/* 为了节省连接时间，在 eloop 中执行 DoUnregister  */
	eloop_register_timeout(3, 0, HandleDoUnregister, pViewerNode, NULL);


	SetStatusInfo(pDlg->m_hWnd, "正在等待对方发起反向连接，请稍候。。。");

	udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_sock == INVALID_SOCKET) {
		SetStatusInfo(pDlg->m_hWnd, "连接失败！(udp socket failed)");
		CloseHandle(pViewerNode->hConnectThreadRev);
		pViewerNode->hConnectThreadRev = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}

	int flag = 1;
    setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&flag, sizeof(flag));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(pViewerNode->httpOP.m0_p2p_port - 2);//SECOND_CONNECT_PORT
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(udp_sock, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		SetStatusInfo(pDlg->m_hWnd, "连接失败！(udp bind failed)");
		closesocket(udp_sock);
		CloseHandle(pViewerNode->hConnectThreadRev);
		pViewerNode->hConnectThreadRev = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}


	serv = UDT::socket(AF_INET, SOCK_STREAM, 0);

	ConfigUdtSocket(serv);

	if (UDT::ERROR == UDT::bind(serv, udp_sock))
	{
		SetStatusInfo(pDlg->m_hWnd, "连接失败！(udt bind failed)");
		UDT::close(serv);
		closesocket(udp_sock);
		CloseHandle(pViewerNode->hConnectThreadRev);
		pViewerNode->hConnectThreadRev = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}

	if (UDT::ERROR == UDT::listen(serv, 1))
	{
		SetStatusInfo(pDlg->m_hWnd, "连接失败！(udt listen failed)");
		UDT::close(serv);
		closesocket(udp_sock);
		CloseHandle(pViewerNode->hConnectThreadRev);
		pViewerNode->hConnectThreadRev = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}
	
#if 1
	///////////////
	struct timeval waitval;
	waitval.tv_sec  = UDT_ACCEPT_TIME + 5;
	waitval.tv_usec = 0;
	ret = UDT::select(serv + 1, serv, NULL, NULL, &waitval);
	if (ret == UDT::ERROR || ret == 0) {
		SetStatusInfo(pDlg->m_hWnd, "暂时性网络状况不好，直连有点小困难，请稍后重试或者采用TCP连接");
		UDT::close(serv);
		closesocket(udp_sock);
		CloseHandle(pViewerNode->hConnectThreadRev);
		pViewerNode->hConnectThreadRev = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}
	//////////////////
#endif
	
	
	if ((fhandle = UDT::accept(serv, (sockaddr*)&their_addr, &namelen)) == UDT::INVALID_SOCK) {
		SetStatusInfo(pDlg->m_hWnd, "暂时性网络状况不好，直连有点小困难，请稍后重试或者采用TCP连接");
		UDT::close(serv);
		closesocket(udp_sock);
		CloseHandle(pViewerNode->hConnectThreadRev);
		pViewerNode->hConnectThreadRev = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}

	UDT::close(serv);

	pViewerNode->httpOP.m1_use_udp_sock = udp_sock;
	pViewerNode->httpOP.m1_use_peer_ip = their_addr.sin_addr.s_addr;
	pViewerNode->httpOP.m1_use_peer_port = ntohs(their_addr.sin_port);
	pViewerNode->httpOP.m1_use_udt_sock = fhandle;
	pViewerNode->httpOP.m1_use_sock_type = SOCKET_TYPE_UDT;


	SetStatusInfo(pDlg->m_hWnd, _T("成功建立连接，正在验证访问密码。。。"));
	ret = CheckPassword(pViewerNode, pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock, 2);
	if (-1 == ret) {
		SetStatusInfo(pDlg->m_hWnd, "验证密码时通信出错，无法连接！");
	}
	else if (0 == ret)
	{
		SetStatusInfo(pDlg->m_hWnd, "认证密码错误，无法连接！");
	}
	else
	{

		DoInConnection(pDlg, pViewerNode);

	}

	CtrlCmd_BYE(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);

	Sleep(3000);

	ret = UDT::close(pViewerNode->httpOP.m1_use_udt_sock);
	pViewerNode->httpOP.m1_use_udt_sock = INVALID_SOCKET;
	pViewerNode->httpOP.m1_use_sock_type = SOCKET_TYPE_UNKNOWN;

	ret = closesocket(pViewerNode->httpOP.m1_use_udp_sock);
	pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;

	CloseHandle(pViewerNode->hConnectThreadRev);
	pViewerNode->hConnectThreadRev = NULL;
	pDlg->ReturnViewerNode(pViewerNode);
	return 0;
}



static void InitVar()
{
	strncpy(g0_device_uuid, "Viewer Device UUID", sizeof(g0_device_uuid));
	strncpy(g0_node_name,   "Viewer Node Name", sizeof(g0_node_name));
	GetOsInfo(g0_os_info, sizeof(g0_os_info));

	g0_version = MYSELF_VERSION;

	strncpy(g1_http_server, DEFAULT_HTTP_SERVER, sizeof(g1_http_server));
	strncpy(g1_stun_server, DEFAULT_STUN_SERVER, sizeof(g1_stun_server));

	g1_register_period = DEFAULT_REGISTER_PERIOD;  /* Seconds */
	g1_expire_period = DEFAULT_EXPIRE_PERIOD;  /* Seconds */
}



// CShiyong

CShiyong::CShiyong()
{
	m_bQuit = FALSE;
	m_hThread = NULL;


	for (int i = 0; i < MAX_VIEWER_NUM; i++ )
	{
		viewerArray[i].bUsing = FALSE;
		viewerArray[i].nID = -1;
		viewerArray[i].bFirstCheckStun = TRUE;
		::InitializeCriticalSection(&(viewerArray[i].localbind_csec));
		
		viewerArray[i].bConnected = FALSE;
		viewerArray[i].httpOP.m0_is_admin = TRUE;
		viewerArray[i].httpOP.m0_is_busy = FALSE;
		viewerArray[i].httpOP.m0_p2p_port = FIRST_CONNECT_PORT - (i+1)*4;
		
		//每次有不一样的node_id
		Sleep(200);
		generate_nodeid(viewerArray[i].httpOP.m0_node_id, sizeof(viewerArray[i].httpOP.m0_node_id));

		viewerArray[i].bQuitRecvSocketLoop = TRUE;

		viewerArray[i].m_sps_len = 0;
		viewerArray[i].m_pps_len = 0;
	}
	
	currentSourceIndex = -1;

	joined_channel_id = 0;
	memset(joined_node_id, 0, 6);

#if FIRST_LEVEL_REPEATER
	device_topo_level = 0;
#else
	device_topo_level = 1;
#endif
	generate_nodeid(device_node_id, sizeof(device_node_id));

	::InitializeCriticalSection(&route_table_csec);

	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		device_route_table[i].bUsing = FALSE;
		device_route_table[i].nID = -1;
	}

	g_pShiyong = this;

	OnInit();
}

CShiyong::~CShiyong()
{
	for (int i = 0; i < MAX_VIEWER_NUM; i++ )
	{
		::DeleteCriticalSection(&(viewerArray[i].localbind_csec));
	}

	::DeleteCriticalSection(&route_table_csec);

	g_pShiyong = NULL;
}

BOOL CShiyong::OnInit()
{
	InitVar();

	HttpOperate::DoReport1("gbk", "zh", OnReportSettings);


	DWORD dwThreadID;
	HANDLE hThread = ::CreateThread(NULL,0,WorkingThreadFn,(void *)this,0,&dwThreadID);
	if (hThread != NULL) {
		m_hThread = hThread;
	}
	else {
		printf("WorkingThreadFn子线程创建失败！\n");
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CShiyong::ConnectNode(int i, char *password)
{
	if (i < 0 || i >= MAX_VIEWER_NUM) {
		return;
	}

	if (viewerArray[i].bUsing == TRUE) {
		return;
	}

	viewerArray[i].bUsing = TRUE;
	viewerArray[i].nID = i;


	//{{{{--------------------------------------->
	viewerArray[i].hConnectThread = NULL;
	viewerArray[i].hConnectThread2 = NULL;
	viewerArray[i].hConnectThreadRev = NULL;
	viewerArray[i].bQuitRecvSocketLoop = TRUE;
	viewerArray[i].bConnected = FALSE;
	viewerArray[i].bTopoPrimary = FALSE;

	viewerArray[i].m_sps_len = 0;
	viewerArray[i].m_pps_len = 0;

	strncpy(viewerArray[i].password, password, sizeof(viewerArray[i].password));
	//<---------------------------------------}}}}

	//mac_addr(viewerArray[i].anypcNode.node_id_str, viewerArray[i].httpOP.m1_peer_node_id, NULL);


	DWORD dwID = 0;
	if (FALSE == viewerArray[i].httpOP.m1_peer_nonat)
	{
		viewerArray[i].hConnectThread = CreateThread(NULL, 0, ConnectThreadFn, (void *)(&(viewerArray[i])), 0, &dwID);
	}
	else {
		viewerArray[i].hConnectThread2 = CreateThread(NULL, 0, ConnectThreadFn2, (void *)(&(viewerArray[i])), 0, &dwID);
	}
}

void CShiyong::ConnectRevNode(int i, char *password)
{
	if (i < 0 || i >= MAX_VIEWER_NUM) {
		return;
	}

	if (viewerArray[i].bUsing == TRUE) {
		return;
	}

	viewerArray[i].bUsing = TRUE;
	viewerArray[i].nID = i;


	//{{{{--------------------------------------->
	viewerArray[i].hConnectThread = NULL;
	viewerArray[i].hConnectThread2 = NULL;
	viewerArray[i].hConnectThreadRev = NULL;
	viewerArray[i].bQuitRecvSocketLoop = TRUE;
	viewerArray[i].bConnected = FALSE;
	viewerArray[i].bTopoPrimary = FALSE;

	viewerArray[i].m_sps_len = 0;
	viewerArray[i].m_pps_len = 0;

	strncpy(viewerArray[i].password, password, sizeof(viewerArray[i].password));
	//<---------------------------------------}}}}

	//mac_addr(viewerArray[i].anypcNode.node_id_str, viewerArray[i].httpOP.m1_peer_node_id, NULL);


	DWORD dwID = 0;
	{
		viewerArray[i].hConnectThreadRev = CreateThread(NULL, 0, ConnectThreadFnRev, (void *)(&(viewerArray[i])), 0, &dwID);
	}
}

void CShiyong::DisconnectNode(VIEWER_NODE *pViewerNode)
{
	if (pViewerNode->bUsing == FALSE) {
		return;
	}
	if (pViewerNode->bTopoPrimary) {
		CtrlCmd_AV_STOP(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);
	}
	pViewerNode->bQuitRecvSocketLoop = TRUE;
}

void CShiyong::ReturnViewerNode(VIEWER_NODE *pViewerNode)
{
	if (NULL != pViewerNode)
	{
		pViewerNode->bUsing = FALSE;
		pViewerNode->nID = -1;
	}
}

// -1: Not exist
int CShiyong::FindViewerNode(BYTE *viewer_node_id)
{
	for (int i = 0; i < MAX_VIEWER_NUM; i++)
	{
		if (memcmp(viewerArray[i].httpOP.m0_node_id, viewer_node_id, 6) == 0) {
			return i;
		}
	}
	return -1;
}

// -1: Not exist
int CShiyong::FindTopoRouteItem(BYTE *dest_node_id)
{
	::EnterCriticalSection(&route_table_csec);////////
	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (device_route_table[i].bUsing == FALSE) {
			continue;
		}
		if (memcmp(device_route_table[i].node_id, dest_node_id, 6) == 0) {
			::LeaveCriticalSection(&route_table_csec);////////
			return i;
		}
	}
	::LeaveCriticalSection(&route_table_csec);////////
	return -1;
}

int CShiyong::FindRouteNode(BYTE *node_id)
{
	int ret = -1;
	::EnterCriticalSection(&route_table_csec);////////
	ret = FindRouteNode_NoLock(node_id);
	::LeaveCriticalSection(&route_table_csec);////////
	return ret;
}

int CShiyong::FindRouteNode_NoLock(BYTE *node_id)
{
	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (FALSE == device_route_table[i].bUsing) {
			continue;
		}
		if (memcmp(device_route_table[i].node_id, node_id, 6) == 0) {
			return i;
		}
	}
	return -1;
}

void CShiyong::DropRouteItem(BYTE node_type, BYTE *node_id)
{
	::EnterCriticalSection(&route_table_csec);////////

	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (FALSE == device_route_table[i].bUsing) {
			continue;
		}
		if (device_route_table[i].route_item_type == node_type && memcmp(device_route_table[i].node_id, node_id, 6) == 0) {
			device_route_table[i].bUsing = FALSE;
			device_route_table[i].nID = -1;
			break;
		}
	}

	::LeaveCriticalSection(&route_table_csec);////////
}

void CShiyong::CheckTopoRouteTable()
{
	time_t now = time(NULL);

	::EnterCriticalSection(&route_table_csec);////////

	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (FALSE == device_route_table[i].bUsing) {
			continue;
		}
		if (now - device_route_table[i].last_refresh_time >= g1_expire_period) {
			device_route_table[i].bUsing = FALSE;
			device_route_table[i].nID = -1;
		}
	}

	::LeaveCriticalSection(&route_table_csec);////////
}

static BOOL ParseReportNode(char *value, TOPO_ROUTE_ITEM *pRouteItem)
{
	char *p;


	if (!value || !pRouteItem) {
		return FALSE;
	}


	/* node_type */
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	pRouteItem->route_item_type = atol(value);

	if (ROUTE_ITEM_TYPE_VIEWERNODE == pRouteItem->route_item_type || ROUTE_ITEM_TYPE_GUAJINODE == pRouteItem->route_item_type)
	{
		/* topo_level */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		pRouteItem->topo_level = (BYTE)atol(value);

		/* owner_node_id */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		mac_addr(value, pRouteItem->owner_node_id, NULL);

		/* node_id */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		mac_addr(value, pRouteItem->node_id, NULL);

		/* peer_node_id */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		mac_addr(value, pRouteItem->peer_node_id, NULL);

		/* is_busy */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		if (strcmp(value, "1") == 0) {
			pRouteItem->is_busy = TRUE;
		}
		else {
			pRouteItem->is_busy = FALSE;
		}

		/* is_streaming */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		if (strcmp(value, "1") == 0) {
			pRouteItem->is_streaming = TRUE;
		}
		else {
			pRouteItem->is_streaming = FALSE;
		}

		/* ip */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		strncpy(pRouteItem->u.node_nat_info.ip_str, value, sizeof(pRouteItem->u.node_nat_info.ip_str));

		/* port */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		strncpy(pRouteItem->u.node_nat_info.port_str, value, sizeof(pRouteItem->u.node_nat_info.port_str));

		/* pub_ip */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		strncpy(pRouteItem->u.node_nat_info.pub_ip_str, value, sizeof(pRouteItem->u.node_nat_info.pub_ip_str));

		/* pub_port */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		strncpy(pRouteItem->u.node_nat_info.pub_port_str, value, sizeof(pRouteItem->u.node_nat_info.pub_port_str));

		/* NO_NAT */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		if (strcmp(value, "1") == 0) {
			pRouteItem->u.node_nat_info.no_nat = TRUE;
		}
		else {
			pRouteItem->u.node_nat_info.no_nat = FALSE;
		}

		/* NAT_TYPE */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		pRouteItem->u.node_nat_info.nat_type = atoi(value);

		/* device_uuid ... os_info */
		value = p + 1;
		p = strchr(value, '|');
		if (p != NULL) {
			*p = '\0';  /* Last field */
		}
#if 0//不要把联合体u里面的node_nat_info覆盖了！！！
		strncpy(pRouteItem->u.device_node_str, value, sizeof(pRouteItem->u.device_node_str));
#endif
	}
	else if (ROUTE_ITEM_TYPE_DEVICENODE == pRouteItem->route_item_type)
	{
		/* topo_level */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		pRouteItem->topo_level = (BYTE)atol(value);

		/* node_id */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		mac_addr(value, pRouteItem->node_id, NULL);

		/* device_uuid ... os_info */
		value = p + 1;
		p = strchr(value, '|');
		if (p != NULL) {
			*p = '\0';  /* Last field */
		}
		strncpy(pRouteItem->u.device_node_str, value, sizeof(pRouteItem->u.device_node_str));
	}

	return TRUE;
}

int CShiyong::UpdateRouteTable(int guajiIndex, char *report_string)
{
	char *start = report_string;
	char name[32];
	char value[1024];
	char *next_start = NULL;
	TOPO_ROUTE_ITEM temp_route_item;
	int ret = -1;

	while(TRUE) {

		if (ParseLine(start, name, sizeof(name), value, sizeof(value), &next_start) == FALSE) {
			return -1;
		}

		if (strcmp(name, "row") == 0) {
			ParseReportNode(value, &temp_route_item);
			temp_route_item.guaji_index = guajiIndex;
			temp_route_item.last_refresh_time = time(NULL);
			int index = FindTopoRouteItem(temp_route_item.node_id);
			::EnterCriticalSection(&route_table_csec);////////
			if (-1 == index)
			{
				int i;
				for (i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
				{
					if (FALSE == device_route_table[i].bUsing) {
						break;
					}
				}
				if (i < MAX_ROUTE_ITEM_NUM) {
					device_route_table[i] = temp_route_item;
					device_route_table[i].bUsing = TRUE;
					device_route_table[i].nID = i;
				}
				else {
					printf("对不起，没有空闲的TOPO_ROUTE_ITEM!\n");
				}
			}
			else {
				device_route_table[index] = temp_route_item;
				device_route_table[index].bUsing = TRUE;
				device_route_table[index].nID = index;
			}
			::LeaveCriticalSection(&route_table_csec);////////
		}

		if (next_start == NULL) {
			break;
		}
		start = next_start;
	}

	return ret;
}

int CShiyong::DeviceTopoReport()
{
	int i;
	char szTemp[512];
	int buf_len = (MAX_SERVER_NUM + MAX_VIEWER_NUM + 1) * 512;
	char *report_string = (char *)malloc(buf_len);
	strcpy(report_string, "");
	{//Device Node
		snprintf(szTemp, sizeof(szTemp), 
			"%d"//"node_type=%d"
			"|%d"//"&topo_level=%d"
			"|%02X-%02X-%02X-%02X-%02X-%02X"//"      device_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
			"|%s"//"&device_uuid=%s"
			"|%s"//"&node_name=%s"
			"|%ld"//"&version=%ld"
			"|%s"//"&os_info=%s"
			,
			ROUTE_ITEM_TYPE_DEVICENODE,
			device_topo_level,
			device_node_id[0],device_node_id[1],device_node_id[2],device_node_id[3],device_node_id[4],device_node_id[5],
			g0_device_uuid, UrlEncode(g0_node_name).c_str(), g0_version, g0_os_info);

		strcat(report_string, "row=");
		strcat(report_string, szTemp);
		strcat(report_string, "\n");
	}

	for (i = 0; i < MAX_SERVER_NUM; i++)
	{
		snprintf(szTemp, sizeof(szTemp), 
			"%d"//"node_type=%d"
			"|%d"//"&topo_level=%d"
			"|%02X-%02X-%02X-%02X-%02X-%02X"//"owner_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
			"|%s"
			,
			ROUTE_ITEM_TYPE_GUAJINODE,
			device_topo_level,
			device_node_id[0],device_node_id[1],device_node_id[2],device_node_id[3],device_node_id[4],device_node_id[5],
			arrServerProcesses[i].m_ipcReport);

		strcat(report_string, "row=");
		strcat(report_string, szTemp);
		strcat(report_string, "\n");
	}

	for (i = 0; i < MAX_VIEWER_NUM; i++)
	{
		snprintf(szTemp, sizeof(szTemp), 
			"%d"//"node_type=%d"
			"|%d"//"&topo_level=%d"
			"|%02X-%02X-%02X-%02X-%02X-%02X"//"owner_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
			"|%02X-%02X-%02X-%02X-%02X-%02X"//"      node_id=%02X-%02X-%02X-%02X-%02X-%02X"
			"|%02X-%02X-%02X-%02X-%02X-%02X"//" peer_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
			"|%d"//"&is_busy=%d"
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
			ROUTE_ITEM_TYPE_VIEWERNODE,
			device_topo_level,
			device_node_id[0],device_node_id[1],device_node_id[2],device_node_id[3],device_node_id[4],device_node_id[5],
			viewerArray[i].httpOP.m0_node_id[0], viewerArray[i].httpOP.m0_node_id[1], viewerArray[i].httpOP.m0_node_id[2], viewerArray[i].httpOP.m0_node_id[3], viewerArray[i].httpOP.m0_node_id[4], viewerArray[i].httpOP.m0_node_id[5], 
			viewerArray[i].httpOP.m1_peer_node_id[0], viewerArray[i].httpOP.m1_peer_node_id[1], viewerArray[i].httpOP.m1_peer_node_id[2], viewerArray[i].httpOP.m1_peer_node_id[3], viewerArray[i].httpOP.m1_peer_node_id[4], viewerArray[i].httpOP.m1_peer_node_id[5],
			(viewerArray[i].bConnected ? 1 : 0),
			(viewerArray[i].bTopoPrimary ? 1 : 0),
			viewerArray[i].httpOP.MakeIpStr(), viewerArray[i].httpOP.m0_port, viewerArray[i].httpOP.MakePubIpStr(), viewerArray[i].httpOP.m0_pub_port, 
			(viewerArray[i].httpOP.m0_no_nat ? 1 : 0),
			viewerArray[i].httpOP.m0_nat_type,
			g0_device_uuid, UrlEncode(g0_node_name).c_str(), g0_version, g0_os_info);

		strcat(report_string, "row=");
		strcat(report_string, szTemp);
		strcat(report_string, "\n");
	}

	//优先选择Primary通道，向上转发。。。
	for (i = 0; i < MAX_VIEWER_NUM; i++)
	{
		if (viewerArray[i].bUsing == FALSE || viewerArray[i].bConnected == FALSE) {
			continue;
		}
		if (viewerArray[i].bTopoPrimary == TRUE)
		{
			CtrlCmd_TOPO_REPORT(viewerArray[i].httpOP.m1_use_sock_type, viewerArray[i].httpOP.m1_use_udt_sock, device_node_id, report_string);
			break;
		}
	}

	free(report_string);
	return 0;
}

const char * CShiyong::get_public_ip()
{
	static char s_public_ip[16];
	struct in_addr inaddr;

	strcpy(s_public_ip, "");

	for (int i = 0; i < MAX_VIEWER_NUM; i++)
	{
		if (viewerArray[i].httpOP.m0_pub_ip != 0) {
			inaddr.s_addr = viewerArray[i].httpOP.m0_pub_ip;
			strcpy(s_public_ip, inet_ntoa(inaddr));
			break;
		}
	}
	return s_public_ip;
}

const char * CShiyong::get_node_array()
{
	static char s_node_array[1024*32];

	strcpy(s_node_array, "");

	::EnterCriticalSection(&route_table_csec);////////

	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (FALSE == device_route_table[i].bUsing) {
			continue;
		}
		if (ROUTE_ITEM_TYPE_VIEWERNODE != device_route_table[i].route_item_type && ROUTE_ITEM_TYPE_GUAJINODE != device_route_table[i].route_item_type) {
			continue;
		}
		if (TRUE == device_route_table[i].is_busy || TRUE == device_route_table[i].is_streaming) {
			continue;
		}
		int is_admin = (device_route_table[i].route_item_type == ROUTE_ITEM_TYPE_VIEWERNODE) ? 1 : 0;
		int device_node_index = FindRouteNode_NoLock(device_route_table[i].owner_node_id);

		char szTemp[1024*2];
		snprintf(szTemp, sizeof(szTemp), 
			"%d"//"&topo_level=%d"
			"|%02X-%02X-%02X-%02X-%02X-%02X"//"      node_id=%02X-%02X-%02X-%02X-%02X-%02X"
			"|%d"//"&is_admin=%d"
			"|%d"//"&is_busy=%d"
			"|%d"//"&is_streaming=%d"
			"|%s"//"&ip"
			"|%s"//"&port"
			"|%s"//"&pub_ip"
			"|%s"//"&pub_port"
			"|%d"//"&no_nat=%d"
			"|%d"//"&nat_type=%d"
			"|%s"//"&device_uuid|node_name|version|os_info"
			,
			device_route_table[i].topo_level,
			device_route_table[i].node_id[0],device_route_table[i].node_id[1],device_route_table[i].node_id[2],device_route_table[i].node_id[3],device_route_table[i].node_id[4],device_route_table[i].node_id[5],
			is_admin,
			(device_route_table[i].is_busy ? 1 : 0),
			(device_route_table[i].is_streaming ? 1 : 0),
			device_route_table[i].u.node_nat_info.ip_str, device_route_table[i].u.node_nat_info.port_str, 
			device_route_table[i].u.node_nat_info.pub_ip_str, device_route_table[i].u.node_nat_info.pub_port_str, 
			(device_route_table[i].u.node_nat_info.no_nat ? 1 : 0),
			device_route_table[i].u.node_nat_info.nat_type,
			(device_node_index == -1 ? "" : device_route_table[device_node_index].u.device_node_str));

		if (strlen(s_node_array) > 0) {
			strcat(s_node_array, ";");
		}
		if (strlen(s_node_array) + strlen(szTemp) < sizeof(s_node_array)) {
			strcat(s_node_array, szTemp);
		}
		else {
			printf("s_node_array string buff overflow!!!\n");
			break;
		}
	}

	::LeaveCriticalSection(&route_table_csec);////////

	return s_node_array;
}

int CShiyong::get_route_item_num()
{
	int count = 0;

	::EnterCriticalSection(&route_table_csec);////////

	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (FALSE == device_route_table[i].bUsing) {
			continue;
		}
		count += 1;
	}

	::LeaveCriticalSection(&route_table_csec);////////

	return count;
}

int CShiyong::get_level_max_connections(int topo_level)
{
	time_t now = time(NULL);
	int count = 0;

	::EnterCriticalSection(&route_table_csec);////////

	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (FALSE == device_route_table[i].bUsing) {
			continue;
		}
		if (ROUTE_ITEM_TYPE_GUAJINODE != device_route_table[i].route_item_type) {
			continue;
		}
		if (topo_level != device_route_table[i].topo_level) {
			continue;
		}
		if (now - device_route_table[i].last_refresh_time < g1_expire_period) {
			count += 1;
		}
	}

	::LeaveCriticalSection(&route_table_csec);////////

	return count;
}

int CShiyong::get_level_current_connections(int topo_level)
{
	time_t now = time(NULL);
	int count = 0;

	::EnterCriticalSection(&route_table_csec);////////

	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (FALSE == device_route_table[i].bUsing) {
			continue;
		}
		if (ROUTE_ITEM_TYPE_GUAJINODE != device_route_table[i].route_item_type) {
			continue;
		}
		if (topo_level != device_route_table[i].topo_level) {
			continue;
		}
		if (device_route_table[i].is_busy && now - device_route_table[i].last_refresh_time < g1_expire_period) {
			count += 1;
		}
	}

	::LeaveCriticalSection(&route_table_csec);////////

	return count;
}

int CShiyong::get_level_max_streams(int topo_level)
{
	return get_level_max_connections(topo_level);
}

int CShiyong::get_level_current_streams(int topo_level)
{
	time_t now = time(NULL);
	int count = 0;

	::EnterCriticalSection(&route_table_csec);////////

	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (FALSE == device_route_table[i].bUsing) {
			continue;
		}
		if (ROUTE_ITEM_TYPE_GUAJINODE != device_route_table[i].route_item_type) {
			continue;
		}
		if (topo_level != device_route_table[i].topo_level) {
			continue;
		}
		if (device_route_table[i].is_busy && device_route_table[i].is_streaming && now - device_route_table[i].last_refresh_time < g1_expire_period) {
			count += 1;
		}
	}

	::LeaveCriticalSection(&route_table_csec);////////

	return count;
}

void CShiyong::DoExit()
{
	// TODO: 在此添加命令处理程序代码

	if (m_bQuit)
	{
		return;
	}
	m_bQuit = TRUE;


	for (int i = 0; i < MAX_VIEWER_NUM; i++)
	{
		if (NULL != viewerArray[i].hConnectThread) {
			::TerminateThread(viewerArray[i].hConnectThread, 0);
		}

		if (NULL != viewerArray[i].hConnectThread2) {
			::TerminateThread(viewerArray[i].hConnectThread2, 0);
		}

		if (NULL != viewerArray[i].hConnectThreadRev) {
			::TerminateThread(viewerArray[i].hConnectThreadRev, 0);
		}
	}


	//::EnterCriticalSection(&m_localbind_csec);////////
	//::LeaveCriticalSection(&m_localbind_csec);////////

	if (m_hThread != NULL) {
		eloop_terminate();
		::WaitForSingleObject(m_hThread, INFINITE);
		::CloseHandle(m_hThread);
		m_hThread = NULL;
	}
}


///////////////////////////////////////////////////////

CShiyong  the_shiyong;
