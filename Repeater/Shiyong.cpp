// Shiyong.cpp : 实现文件
//

#include "stdafx.h"
#include "Repeater.h"

#include "platform.h"
#include "CommonLib.h"
#include "HttpOperate.h"
#include "phpMd5.h"
#include "UPnP.h"
#include "base64.h"
#include "Discovery.h"
#include "Eloop.h"
#include "LogMsg.h"
#include "PacketRepeater.h"
#include "Shiyong.h"


#define LOG_MSG  1


int g_video_width  = 640;
int g_video_height = 480;
int g_video_fps = 25;


CShiyong* g_pShiyong = NULL;

static MyUPnP  myUPnP;

#ifdef WIN32
DWORD WINAPI ConnectThreadFn(LPVOID lpParameter);
DWORD WINAPI ConnectThreadFn2(LPVOID lpParameter);
DWORD WINAPI ConnectThreadFnRev(LPVOID lpParameter);
#else
void *ConnectThreadFn(void *lpParameter);
void *ConnectThreadFn2(void *lpParameter);
void *ConnectThreadFnRev(void *lpParameter);
#endif

static int lastAdjustTime = 0;
static int lastOptimizeTime = 0;
static int lastDoReportTime = 0;
void OnReportSettings(char *settings_str);
void OnReportEvent(char *event_str);


#define strProductName   ("Repeater")
#define SetStatusInfo(hWnd, lpStatusInfo)   do {printf(lpStatusInfo);	printf("\n");} while(0)


static void HandleKeepLoop(void *eloop_ctx, void *timeout_ctx)
{
	g_pShiyong->CheckTopoRouteTable();//清理已过期未更新的路由表项

	if (g_pShiyong->ShouldDoAdjustAndOptimize())
	{
		int now = time(NULL);
		if (now - lastAdjustTime >= 15)
		{
			lastAdjustTime = now;
			g_pShiyong->AdjustTopoStructure();//自动调整单元树内拓扑结构
		}
	}

	if (g_pShiyong->ShouldDoAdjustAndOptimize())
	{
		int now = time(NULL);
		if (now - lastOptimizeTime >= 10)
		{
			lastOptimizeTime = now;
			g_pShiyong->OptimizeStreamPath();//自动调整单元树内Stream负载均衡
		}
	}

	if (g_pShiyong->ShouldDoHttpOP())
	{
		int now = time(NULL);
		if (now - lastDoReportTime >= g1_register_period)
		{
			lastDoReportTime = now;
			int ret = HttpOperate::DoReport2("gbk", "zh", g_pShiyong->GetConnectedViewerNodes() > 0, 
				g_pShiyong->get_level_device_num(1) + g_pShiyong->get_level_device_num(2) + g_pShiyong->get_level_device_num(3) + g_pShiyong->get_level_device_num(4),
				g_pShiyong->get_viewer_grow_rate(),
				g0_device_uuid, g_pShiyong->get_public_ip(), g_pShiyong->device_node_id, 
				g_pShiyong->get_route_item_num(), MAX_ROUTE_ITEM_NUM, 
				g_pShiyong->get_level_max_connections(1), g_pShiyong->get_level_current_connections(1), g_pShiyong->get_level_max_streams(1), g_pShiyong->get_level_current_streams(1),
				g_pShiyong->get_level_max_connections(2), g_pShiyong->get_level_current_connections(2), g_pShiyong->get_level_max_streams(2), g_pShiyong->get_level_current_streams(2),
				g_pShiyong->get_level_max_connections(3), g_pShiyong->get_level_current_connections(3), g_pShiyong->get_level_max_streams(3), g_pShiyong->get_level_current_streams(3),
				g_pShiyong->get_level_max_connections(4), g_pShiyong->get_level_current_connections(4), g_pShiyong->get_level_max_streams(4), g_pShiyong->get_level_current_streams(4),
				g_pShiyong->get_node_array(), OnReportSettings, OnReportEvent);
			
			log_msg_f(LOG_LEVEL_DEBUG, "DoReport2()=%d\n", ret);
		}
	}

	eloop_register_timeout(1, 0, HandleKeepLoop, NULL, NULL);
}

static void HandleTopoReport(void *eloop_ctx, void *timeout_ctx)
{
	g_pShiyong->DeviceTopoReport();//非Root节点
}

static void HandleMediaStreamingChech(void *eloop_ctx, void *timeout_ctx)
{
	if (g_pShiyong->timeoutMedia > 0 && g_pShiyong->currentLastMediaTime > 0)
	{
		DWORD nowTime = get_system_milliseconds();
		if (nowTime - g_pShiyong->currentLastMediaTime > g_pShiyong->timeoutMedia) {

			//尝试找到另一个已连接的ViewNode
			int i;
			for (i = 0; i < MAX_VIEWER_NUM; i++)
			{
				if (g_pShiyong->viewerArray[i].bUsing == FALSE || g_pShiyong->viewerArray[i].bConnected == FALSE) {
					continue;
				}
				if (g_pShiyong->viewerArray[i].bTopoPrimary == FALSE && g_pShiyong->currentSourceIndex != i) {
					break;
				}
			}
			if (i < MAX_VIEWER_NUM)//找到一个！切换。。。
			{
				int oldIndex = g_pShiyong->currentSourceIndex;
				g_pShiyong->SwitchMediaSource(g_pShiyong->currentSourceIndex, i);
				g_pShiyong->DisconnectNode(&(g_pShiyong->viewerArray[oldIndex]));
			}

		}
	}
	eloop_register_timeout(0, 50000, HandleMediaStreamingChech, NULL, NULL);
}

static void HandleDoUnregister(void *eloop_ctx, void *timeout_ctx)
{
	BYTE is_connected = (BYTE)timeout_ctx;
	VIEWER_NODE *pViewerNode = (VIEWER_NODE *)eloop_ctx;
	BYTE node_type = ROUTE_ITEM_TYPE_VIEWERNODE;
	BYTE *object_node_id = pViewerNode->httpOP.m0_node_id;

	//连接成功，不要DropRouteItem
	if (is_connected == 0) {
		g_pShiyong->DropRouteItem(node_type, object_node_id);
	}

	if (g_pShiyong->ShouldDoHttpOP())//Root Node, DoDrop()
	{
		int ret = HttpOperate::DoDrop("gbk", "zh", g_pShiyong->device_node_id, (node_type == ROUTE_ITEM_TYPE_VIEWERNODE), object_node_id);
		log_msg_f(LOG_LEVEL_DEBUG, "DoDrop()=%d\n", ret);
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
}

static void StunRegister(VIEWER_NODE *pViewerNode)
{
	int ret;
	StunAddress4 mapped;
	static BOOL bNoNAT;
	static int  nNatType;
	DWORD ipArrayTemp[MAX_ADAPTER_NUM];  /* Network byte order */
	int ipCountTemp;


	pthread_mutex_lock(&(pViewerNode->localbind_csec));////////

	/* STUN Information */
	if (strcmp(g1_stun_server, "NONE") == 0)
	{
		ret = CheckStunMyself(g1_http_server, pViewerNode->httpOP.m0_p2p_port, &mapped, &bNoNAT, &nNatType, &(pViewerNode->httpOP.m0_net_time));
		if (ret == -1) {
		   log_msg_f(LOG_LEVEL_ERROR, "CheckStunMyself: fialed!\n");

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
			DWORD dwForceNoNAT = 0;
			if (GetSoftwareKeyDwordValue(STRING_REGKEY_NAME_FORCE_NONAT, &dwForceNoNAT) && dwForceNoNAT == 1) {
				  bNoNAT = TRUE;
				  nNatType = StunTypeOpen;
			}

		   
			log_msg_f(LOG_LEVEL_INFO, "本端NAT探测成功! - CheckStunHttp");
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
			pViewerNode->httpOP.m0_no_nat = bNoNAT;
			pViewerNode->httpOP.m0_nat_type = nNatType;
			log_msg_f(LOG_LEVEL_DEBUG, "CheckStunHttp: %d.%d.%d.%d[%d], NoNAT=%d\n", 
				(pViewerNode->httpOP.m0_pub_ip & 0x000000ff) >> 0,
				(pViewerNode->httpOP.m0_pub_ip & 0x0000ff00) >> 8,
				(pViewerNode->httpOP.m0_pub_ip & 0x00ff0000) >> 16,
				(pViewerNode->httpOP.m0_pub_ip & 0xff000000) >> 24,
				pViewerNode->httpOP.m0_pub_port,  pViewerNode->httpOP.m0_no_nat ? 1 : 0);
		}
		else {
			log_msg_f(LOG_LEVEL_INFO, "本端NAT探测成功! - CheckStunMyself");
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
			pViewerNode->httpOP.m0_no_nat = bNoNAT;
			pViewerNode->httpOP.m0_nat_type = nNatType;
			log_msg_f(LOG_LEVEL_DEBUG, "CheckStunMyself: %d.%d.%d.%d[%d], NoNAT=%d\n", 
				(pViewerNode->httpOP.m0_pub_ip & 0x000000ff) >> 0,
				(pViewerNode->httpOP.m0_pub_ip & 0x0000ff00) >> 8,
				(pViewerNode->httpOP.m0_pub_ip & 0x00ff0000) >> 16,
				(pViewerNode->httpOP.m0_pub_ip & 0xff000000) >> 24,
				pViewerNode->httpOP.m0_pub_port,
				(bNoNAT ? 1 : 0));
		}
	}
	else if (pViewerNode->bFirstCheckStun)
	{
		ret = CheckStun(g1_stun_server, pViewerNode->httpOP.m0_p2p_port, &mapped, &bNoNAT, &nNatType);
		if (ret == -1) {
		   log_msg_f(LOG_LEVEL_ERROR, "CheckStun: fialed!\n");

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
			DWORD dwForceNoNAT = 0;
			if (GetSoftwareKeyDwordValue(STRING_REGKEY_NAME_FORCE_NONAT, &dwForceNoNAT) && dwForceNoNAT == 1) {
				  bNoNAT = TRUE;
				  nNatType = StunTypeOpen;
			}

			log_msg_f(LOG_LEVEL_INFO, "本端NAT探测成功! - CheckStunHttp");
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
			pViewerNode->httpOP.m0_no_nat = bNoNAT;
			pViewerNode->httpOP.m0_nat_type = nNatType;
			log_msg_f(LOG_LEVEL_DEBUG, "CheckStunHttp: %d.%d.%d.%d[%d], NoNAT=%d\n", 
				(pViewerNode->httpOP.m0_pub_ip & 0x000000ff) >> 0,
				(pViewerNode->httpOP.m0_pub_ip & 0x0000ff00) >> 8,
				(pViewerNode->httpOP.m0_pub_ip & 0x00ff0000) >> 16,
				(pViewerNode->httpOP.m0_pub_ip & 0xff000000) >> 24,
				pViewerNode->httpOP.m0_pub_port,  pViewerNode->httpOP.m0_no_nat ? 1 : 0);
		}
		else if (ret == 0) {
			log_msg_f(LOG_LEVEL_WARNING, "本端NAT无法穿透!!!您的网络环境可能封锁了UDP通信。");
			pViewerNode->httpOP.m0_pub_ip = 0;
			pViewerNode->httpOP.m0_pub_port = 0;
			pViewerNode->httpOP.m0_no_nat = FALSE;
			pViewerNode->httpOP.m0_nat_type = nNatType;
		}
		else {
			log_msg_f(LOG_LEVEL_INFO, "本端NAT探测成功! - CheckStun");
			pViewerNode->bFirstCheckStun = FALSE;
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
			pViewerNode->httpOP.m0_no_nat = bNoNAT;
			pViewerNode->httpOP.m0_nat_type = nNatType;
			log_msg_f(LOG_LEVEL_DEBUG, "CheckStun: %d.%d.%d.%d[%d] NoNAT=%d\n", 
				(pViewerNode->httpOP.m0_pub_ip & 0x000000ff) >> 0,
				(pViewerNode->httpOP.m0_pub_ip & 0x0000ff00) >> 8,
				(pViewerNode->httpOP.m0_pub_ip & 0x00ff0000) >> 16,
				(pViewerNode->httpOP.m0_pub_ip & 0xff000000) >> 24,
				pViewerNode->httpOP.m0_pub_port,
				(bNoNAT ? 1 : 0));
		}
	}
	else {
		TRACE("CheckStunSimple......\n");
		ret = CheckStunSimple(g1_stun_server, pViewerNode->httpOP.m0_p2p_port, &mapped);
		if (ret == -1) {
			log_msg_f(LOG_LEVEL_ERROR, "CheckStunSimple: fialed!\n");
			mapped.addr = ntohl(pViewerNode->httpOP.m1_http_client_ip);
			mapped.port = pViewerNode->httpOP.m0_p2p_port;
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
		}
		else {
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
			log_msg_f(LOG_LEVEL_DEBUG, "CheckStunSimple: %d.%d.%d.%d[%d]\n", 
				(pViewerNode->httpOP.m0_pub_ip & 0x000000ff) >> 0,
				(pViewerNode->httpOP.m0_pub_ip & 0x0000ff00) >> 8,
				(pViewerNode->httpOP.m0_pub_ip & 0x00ff0000) >> 16,
				(pViewerNode->httpOP.m0_pub_ip & 0xff000000) >> 24,
				pViewerNode->httpOP.m0_pub_port);
		}
	}


	/* 本地路由器UPnP处理*/
	if (FALSE == bNoNAT)
	{
		std::string extIp = "";
		myUPnP.GetNATExternalIp(extIp);
		if (false == extIp.empty() && 0 != pViewerNode->httpOP.m0_pub_ip && inet_addr(extIp.c_str()) == pViewerNode->httpOP.m0_pub_ip)
		{
			pViewerNode->mapping.description = "UP2P";
			//pViewerNode->mapping.protocol = ;
			//pViewerNode->mapping.externalPort = ;
			pViewerNode->mapping.internalPort = pViewerNode->httpOP.m0_p2p_port - 2;//SECOND_CONNECT_PORT
			if (UNAT_OK == myUPnP.AddNATPortMapping(&(pViewerNode->mapping), false))
			{
				pViewerNode->bFirstCheckStun = FALSE;
				
				pViewerNode->httpOP.m0_pub_ip = inet_addr(extIp.c_str());
				pViewerNode->httpOP.m0_pub_port = pViewerNode->mapping.externalPort;
				pViewerNode->httpOP.m0_no_nat = TRUE;
				pViewerNode->httpOP.m0_nat_type = StunTypeOpen;
				
				log_msg_f(LOG_LEVEL_INFO, "UPnP AddPortMapping OK: %s[%d] --> %s[%d]\n", extIp.c_str(), pViewerNode->mapping.externalPort, myUPnP.GetLocalIPStr().c_str(), pViewerNode->mapping.internalPort);
			}
			else
			{
				BOOL bTempExists = FALSE;
				myUPnP.GetNATSpecificEntry(&(pViewerNode->mapping), &bTempExists);
				if (bTempExists)
				{
					pViewerNode->bFirstCheckStun = FALSE;
					
					pViewerNode->httpOP.m0_pub_ip = inet_addr(extIp.c_str());
					pViewerNode->httpOP.m0_pub_port = pViewerNode->mapping.externalPort;
					pViewerNode->httpOP.m0_no_nat = TRUE;
					pViewerNode->httpOP.m0_nat_type = StunTypeOpen;
					
					log_msg_f(LOG_LEVEL_INFO, "UPnP PortMapping Exists: %s[%d] --> %s[%d]\n", extIp.c_str(), pViewerNode->mapping.externalPort, myUPnP.GetLocalIPStr().c_str(), pViewerNode->mapping.internalPort);
				}
				else {
					pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
					pViewerNode->httpOP.m0_pub_port = mapped.port;
					pViewerNode->httpOP.m0_no_nat = bNoNAT;
					pViewerNode->httpOP.m0_nat_type = nNatType;
				}
			}
		}
		else {
			log_msg_f(LOG_LEVEL_INFO, "UPnP PortMapping Skipped: %s --> %s \n", extIp.c_str(), myUPnP.GetLocalIPStr().c_str());
			pViewerNode->httpOP.m0_pub_ip = htonl(mapped.addr);
			pViewerNode->httpOP.m0_pub_port = mapped.port;
			pViewerNode->httpOP.m0_no_nat = bNoNAT;
			pViewerNode->httpOP.m0_nat_type = nNatType;
		}
	}


	//修正公网IP受控端的连接端口问题,2014-6-15
	if (bNoNAT) {
		pViewerNode->httpOP.m0_pub_port = pViewerNode->httpOP.m0_p2p_port - 2;//SECOND_CONNECT_PORT
	}


	/* Local IP Address */
	ipCountTemp = MAX_ADAPTER_NUM;
	if (GetLocalAddress(ipArrayTemp, &ipCountTemp) != NO_ERROR) {
		log_msg_f(LOG_LEVEL_ERROR, "GetLocalAddress: fialed!\n");
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
	pthread_mutex_unlock(&(pViewerNode->localbind_csec));////////
}

#ifdef WIN32
DWORD WINAPI StunRegisterThreadFn(LPVOID pvThreadParam)
#else
void *StunRegisterThreadFn(void *pvThreadParam)
#endif
{
	while(g_pShiyong->m_bQuit == FALSE)
	{
		for (int i = 0; i < MAX_VIEWER_NUM; i++)
		{
			StunRegister(&(g_pShiyong->viewerArray[i]));
		}

		usleep(g1_register_period * 1000 * 1000);
	}
	return 0;
}

#ifdef WIN32
DWORD WINAPI WorkingThreadFn(LPVOID pvThreadParam)
#else
void *WorkingThreadFn(void *pvThreadParam)
#endif
{
	eloop_init(NULL);

	//8秒钟，等待所有ViewerNode完成Stun探测
	eloop_register_timeout(8, 0, HandleKeepLoop, NULL, NULL);

	eloop_register_timeout(8, 50000, HandleMediaStreamingChech, NULL, NULL);

	/* Loop... */
	eloop_run();

	eloop_destroy();
	return 0;
}

void OnReportSettings(char *settings_str)
{
	for (int i = 0; i < MAX_VIEWER_NUM; i++)
	{
		g_pShiyong->viewerArray[i].httpOP.ParseTopoSettings((const char *)settings_str);
	}

	if (NULL != strstr(g0_device_uuid, "STAR") || NULL != strstr(g0_device_uuid, "-1@1"))//STAR或者TREE Root
	{
		if (g_pShiyong->get_joined_channel_id() == 0)
		{
			log_msg_f(LOG_LEVEL_WARNING, "OnReportSettings: joined_channel_id => 0 , Disconnect all viewer nodes...\n");
			g_pShiyong->currentSourceIndex = -1;//避免DisconnectNode时切换
			for (int i = 0; i < MAX_VIEWER_NUM; i++)
			{
				g_pShiyong->DisconnectNode(&(g_pShiyong->viewerArray[i]));
			}
		}
	}

	for (int i = 0; i < MAX_SERVER_NUM; i++)
	{
		if (arrServerProcesses == NULL) {
			log_msg_f(LOG_LEVEL_WARNING, "OnReportSettings: Guaji processes not started!\n");
			break;
		}
		int ret = CtrlCmd_TOPO_SETTINGS(SOCKET_TYPE_TCP, arrServerProcesses[i].m_fhandle, g_pShiyong->device_topo_level, (const char *)settings_str);
		if (ret < 0) {
			continue;
		}
	}
}

void OnReportEvent(char *event_str)
{//event_str是value内容，不包含"event="
	char szDestNodeId[32];
	BYTE dest_node_id[6];
	char *pRecvData = event_str;

	printf(                   "OnReportEvent: event=%s\n", event_str);
	log_msg_f(LOG_LEVEL_INFO, "OnReportEvent: event=%s\n", event_str);

	strncpy(szDestNodeId, event_str, 17);
	szDestNodeId[17] = '\0';
	mac_addr(szDestNodeId, dest_node_id, NULL);

	if (-1 != g_pShiyong->FindConnectingItemViewerNode(dest_node_id) || -1 != g_pShiyong->FindConnectingItemGuajiNode(dest_node_id)) {
		log_msg_f(LOG_LEVEL_INFO, "OnReportEvent: in connecting_event_table, skip! dest_node_id=%02X-%02X-%02X-%02X-%02X-%02X\n", dest_node_id[0], dest_node_id[1], dest_node_id[2], dest_node_id[3], dest_node_id[4], dest_node_id[5]);
		return;
	}

	int index = g_pShiyong->FindViewerNode(dest_node_id);
	if (-1 != index)
	{
		g_pShiyong->viewerArray[index].httpOP.ParseEventValue((char *)pRecvData);
		if (stricmp(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "Connect") == 0
			 && FALSE == g_pShiyong->viewerArray[index].bConnecting && FALSE == g_pShiyong->viewerArray[index].bConnected)
		{
			strcpy(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "");
			g_pShiyong->ConnectNode(index, REPEATER_PASSWORD);
		}
		else if (stricmp(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "ConnectRev") == 0
			 && FALSE == g_pShiyong->viewerArray[index].bConnecting && FALSE == g_pShiyong->viewerArray[index].bConnected)
		{
			strcpy(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "");
			g_pShiyong->ConnectRevNode(index, REPEATER_PASSWORD);
		}
		else if (stricmp(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "Disconnect") == 0
			 && TRUE == g_pShiyong->viewerArray[index].bConnected)
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
		g_pShiyong->currentSourceIndex = pViewerNode->nID;
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
		char *pTemp;
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
				g_pShiyong->currentLastMediaTime = get_system_milliseconds();
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
				printf(                   "CMD_CODE_TOPO_EVENT: event=%s\n", (char *)pRecvData);
				log_msg_f(LOG_LEVEL_INFO, "CMD_CODE_TOPO_EVENT: event=%s\n", (char *)pRecvData);
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
				else if (stricmp(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "Switch") == 0)
				{
					strcpy(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "");
					if (g_pShiyong->currentSourceIndex != index) {
						g_pShiyong->SwitchMediaSource(g_pShiyong->currentSourceIndex, index);
					}
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

			//通过rudp传递来CMD_CODE_TOPO_SETTINGS，"http_client_ip"已经失效了！！！
			pTemp = strstr((char *)pRecvData, "http_client_ip"); 
			if (pTemp) {
				pTemp[0] = 'f';
				pTemp[1] = 'a';
				pTemp[2] = 'k';
				pTemp[3] = 'e';
			}
			pViewerNode->httpOP.ParseTopoSettings((const char *)pRecvData);

			g_pShiyong->CalculateTimeoutMedia();

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
				eloop_cancel_timeout(HandleTopoReport, NULL, NULL);
				eloop_register_timeout(0, (rand() % 800 + 200)*1000, HandleTopoReport, NULL, NULL);
			}

			free(pRecvData);

			break;

		default:
#if LOG_MSG
			log_msg_f(LOG_LEVEL_DEBUG, "RecvSocketDataLoop: CMD_CODE_???=0x%x, dwDataLength=%d\n", wCmd, copy_len);
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
	/* 为了节省连接时间，在 eloop 中执行 DoUnregister  */
	eloop_register_timeout(1, 0, HandleDoUnregister, pViewerNode, (void *)1/*is_connected*/);

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

#ifdef WIN32
DWORD WINAPI ConnectThreadFn(LPVOID lpParameter)
#else
void *ConnectThreadFn(void *lpParameter)
#endif
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
		_snprintf(szStatusStr, sizeof(szStatusStr), "正在同Peer端同步时间，%d秒。。。", i);
		SetStatusInfo(pDlg->m_hWnd, szStatusStr);
		usleep(1000*1000);
	}


	pthread_mutex_lock(&(pViewerNode->localbind_csec));////////


	pViewerNode->httpOP.m1_use_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (pViewerNode->httpOP.m1_use_udp_sock == INVALID_SOCKET) {
		pthread_mutex_unlock(&(pViewerNode->localbind_csec));////////
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
		pthread_mutex_unlock(&(pViewerNode->localbind_csec));////////
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
		pthread_mutex_unlock(&(pViewerNode->localbind_csec));////////
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


	SetStatusInfo(pDlg->m_hWnd, "正在同对方建立连接，请稍候。。。");

	fhandle = UDT::socket(AF_INET, SOCK_STREAM, 0);

	ConfigUdtSocket(fhandle);

	if (UDT::ERROR == UDT::bind(fhandle, pViewerNode->httpOP.m1_use_udp_sock))
	{
		pthread_mutex_unlock(&(pViewerNode->localbind_csec));////////
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
		pthread_mutex_unlock(&(pViewerNode->localbind_csec));////////
		SetStatusInfo(pDlg->m_hWnd, "暂时性网络状况不好，P2P直连有点小困难，稍后重试。。。");
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
		pthread_mutex_unlock(&(pViewerNode->localbind_csec));////////
		SetStatusInfo(pDlg->m_hWnd, "暂时性网络状况不好，P2P直连有点小困难，稍后重试。。。");
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

	usleep(3000*1000);

	ret = UDT::close(pViewerNode->httpOP.m1_use_udt_sock);
	pViewerNode->httpOP.m1_use_udt_sock = INVALID_SOCKET;
	pViewerNode->httpOP.m1_use_sock_type = SOCKET_TYPE_UNKNOWN;

	ret = closesocket(pViewerNode->httpOP.m1_use_udp_sock);
	pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;

	pthread_mutex_unlock(&(pViewerNode->localbind_csec));////////
	CloseHandle(pViewerNode->hConnectThread);
	pViewerNode->hConnectThread = NULL;
	pDlg->ReturnViewerNode(pViewerNode);
	return 0;
}

#ifdef WIN32
DWORD WINAPI ConnectThreadFn2(LPVOID lpParameter)
#else
void *ConnectThreadFn2(void *lpParameter)
#endif
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

	usleep(3000*1000);

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

#ifdef WIN32
DWORD WINAPI ConnectThreadFnRev(LPVOID lpParameter)
#else
void *ConnectThreadFnRev(void *lpParameter)
#endif
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
		_snprintf(szStatusStr, sizeof(szStatusStr), "正在同Peer端同步时间，%d秒。。。", i);
		SetStatusInfo(pDlg->m_hWnd, szStatusStr);
		usleep(1000*1000);
	}


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
		SetStatusInfo(pDlg->m_hWnd, "暂时性网络状况不好，P2P直连有点小困难，稍后重试。。。");
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
		SetStatusInfo(pDlg->m_hWnd, "暂时性网络状况不好，P2P直连有点小困难，稍后重试。。。");
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

	usleep(3000*1000);

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


//
//  0: OK
// -1: NG
static int get_device_uuid(char *buff, int size)
{
	BOOL bRetry = FALSE;
	DWORD dwResult = NO_ERROR;
	DWORD dwRet;
	ULONG ulOutBufLen = 0;
	IP_ADAPTER_INFO *pAdapterInfo = NULL, *pLoopAdapter = NULL;


	TCHAR szValueData[_MAX_PATH];
	DWORD dwDataLen = _MAX_PATH;
	if (GetSoftwareKeyValue(STRING_REGKEY_NAME_SAVED_UUID,(LPBYTE)szValueData,&dwDataLen) && strlen(szValueData) > 0)
	{
		strncpy(buff, szValueData, size);
		return 0;
	}

	dwRet = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
	if (NO_ERROR != dwRet && ERROR_BUFFER_OVERFLOW != dwRet) {
		dwResult = dwRet;
		goto exit;
	}

	pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);

_RETRY:
	dwRet = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);	/* call second */
	if (NO_ERROR !=  dwRet) {
		dwResult = dwRet;
		goto exit;
	}

	pLoopAdapter = pAdapterInfo;
	while (pLoopAdapter != NULL) {
		/* 接口类型为： 有线接口（无线接口，拨号网络接口）*/
		if (MIB_IF_TYPE_ETHERNET == pLoopAdapter->Type) {
			if (bRetry) {
				break;
			}
			else
			{
				if (NULL == strstr(pLoopAdapter->Description, "Wireless") && NULL == strstr(pLoopAdapter->Description, "USB")) {
					break;
				}
			}
		}
		
		pLoopAdapter = pLoopAdapter->Next;
	}

	if (pLoopAdapter != NULL) {
		_snprintf(buff, size, "WINDOWS@%s@%02X:%02X:%02X:%02X:%02X:%02X-%d@%d", 
			SERVER_TYPE,
			pLoopAdapter->Address[0], pLoopAdapter->Address[1], pLoopAdapter->Address[2], 
			pLoopAdapter->Address[3], pLoopAdapter->Address[4], pLoopAdapter->Address[5],
			UUID_EXT, UUID_EXT);
		//保存到注册表
		SaveSoftwareKeyValue(STRING_REGKEY_NAME_SAVED_UUID, buff);
	}
	else {
		if (bRetry) {
			dwResult = ERROR_ACCESS_DENIED;
		}
		else {
			bRetry = TRUE;
			goto _RETRY;
		}
	}

exit:
	if (pAdapterInfo) {
		free(pAdapterInfo);
	}
	return (dwResult == NO_ERROR ? 0 : -1);
}

static void InitVar()
{
	get_device_uuid(g0_device_uuid, sizeof(g0_device_uuid));

	strncpy(g0_node_name,   "DeviceNodeName", sizeof(g0_node_name));
	GetOsInfo(g0_os_info, sizeof(g0_os_info));

	g0_version = MYSELF_VERSION;

	strncpy(g1_http_server, DEFAULT_HTTP_SERVER, sizeof(g1_http_server));
	strncpy(g1_stun_server, DEFAULT_STUN_SERVER, sizeof(g1_stun_server));

	g1_register_period = DEFAULT_REGISTER_PERIOD;  /* Seconds */
	g1_expire_period = DEFAULT_EXPIRE_PERIOD;  /* Seconds */

	g1_bandwidth_per_stream = BANDWIDTH_PER_STREAM_UNKNOWN;
}



// CShiyong

CShiyong::CShiyong()
{
	log_msg("CShiyong()...\n", LOG_LEVEL_DEBUG);

	m_bQuit = FALSE;
	m_hThread = NULL;

	bool hasUPnP = myUPnP.Search();

	for (int i = 0; i < MAX_VIEWER_NUM; i++ )
	{
		viewerArray[i].bUsing = FALSE;
		viewerArray[i].nID = -1;
		viewerArray[i].bFirstCheckStun = TRUE;
		pthread_mutex_init(&(viewerArray[i].localbind_csec), NULL);
		
		viewerArray[i].bTopoPrimary = FALSE;
		viewerArray[i].bConnecting = FALSE;
		viewerArray[i].bConnected = FALSE;
		viewerArray[i].httpOP.m0_is_admin = TRUE;
		viewerArray[i].httpOP.m0_is_busy = FALSE;
		viewerArray[i].httpOP.m0_p2p_port = FIRST_CONNECT_PORT - (i+1)*4;
		
		//每次有不一样的node_id
		usleep(200*1000);
		generate_nodeid(viewerArray[i].httpOP.m0_node_id, sizeof(viewerArray[i].httpOP.m0_node_id));

		viewerArray[i].bQuitRecvSocketLoop = TRUE;

		viewerArray[i].m_sps_len = 0;
		viewerArray[i].m_pps_len = 0;


		BOOL bMappingExists = FALSE;
		viewerArray[i].mapping.description = "";
		viewerArray[i].mapping.protocol = UNAT_UDP;
		viewerArray[i].mapping.externalPort = ((myUPnP.GetLocalIP() & 0xff000000) >> 24) | ((2049 + (65535 - 2049) * (WORD)rand() / (65536)) & 0xffffff00);
		while (hasUPnP && UNAT_OK == myUPnP.GetNATSpecificEntry(&(viewerArray[i].mapping), &bMappingExists) && bMappingExists)
		{
			//printf("Find NATPortMapping(%s), retry...\n", mapping.description.c_str());
			bMappingExists = FALSE;
			viewerArray[i].mapping.description = "";
			viewerArray[i].mapping.protocol = UNAT_UDP;
			viewerArray[i].mapping.externalPort = ((myUPnP.GetLocalIP() & 0xff000000) >> 24) | ((2049 + (65535 - 2049) * (WORD)rand() / (65536)) & 0xffffff00);
		}//找到一个未被占用的外部端口映射，或者路由器UPnP功能不可用
	}
	
	currentSourceIndex = -1;
	currentLastMediaTime = 0;
	timeoutMedia = 0;

	last_viewer_num_time = 0;
	last_viewer_num = 0;

#if FIRST_LEVEL_REPEATER
	device_topo_level = 0;
#else
	device_topo_level = 1;
#endif
	//确保device_node_id与前面的最后一个viewer_node_id不雷同！！！
	usleep(300*1000);
	generate_nodeid(device_node_id, sizeof(device_node_id));

	pthread_mutex_init(&route_table_csec, NULL);

	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		device_route_table[i].bUsing = FALSE;
		device_route_table[i].nID = -1;
	}

	for (int i = 0; i < MAX_CONNECTING_EVENT_NUM; i++)
	{
		connecting_event_table[i].bUsing = FALSE;
		connecting_event_table[i].nID = -1;
	}

	//g_pShiyong = this;

	//OnInit();
}

CShiyong::~CShiyong()
{
	for (int i = 0; i < MAX_VIEWER_NUM; i++ )
	{
		pthread_mutex_destroy(&(viewerArray[i].localbind_csec));

		myUPnP.RemoveNATPortMapping(viewerArray[i].mapping);
	}

	pthread_mutex_destroy(&route_table_csec);

	//g_pShiyong = NULL;
}

BOOL CShiyong::OnInit()
{
	InitVar();

	int ret = HttpOperate::DoReport1("gbk", "zh", OnReportSettings);
	log_msg_f(LOG_LEVEL_DEBUG, "DoReport1()=%d\n", ret);

#ifdef WIN32
	DWORD dwThreadID;
	HANDLE hThread = ::CreateThread(NULL,0,WorkingThreadFn,(void *)this,0,&dwThreadID);
	if (hThread != NULL)
#else
	pthread_t hThread;
	int err = pthread_create(&hThread, NULL, WorkingThreadFn, this);
	if (0 == err)
#endif
	{
		m_hThread = hThread;
	}
	else {
		log_msg_f(LOG_LEVEL_ERROR, "WorkingThreadFn子线程创建失败！\n");
	}

#ifdef WIN32
	hThread = ::CreateThread(NULL,0,StunRegisterThreadFn,(void *)this,0,&dwThreadID);
	if (hThread != NULL)
#else
	err = pthread_create(&hThread, NULL, StunRegisterThreadFn, this);
	if (0 == err)
#endif
	{
	}
	else {
		log_msg_f(LOG_LEVEL_ERROR, "StunRegisterThreadFn子线程创建失败！\n");
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CShiyong::ConnectNode(int i, char *password)
{
	log_msg_f(LOG_LEVEL_DEBUG, "ConnectNode(%d)...\n", i);
	if (m_bQuit) {
		return;
	}

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
	viewerArray[i].bConnecting = TRUE;
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
#ifdef WIN32
		viewerArray[i].hConnectThread = CreateThread(NULL, 0, ConnectThreadFn, (void *)(&(viewerArray[i])), 0, &dwID);
#else
		pthread_create(&(viewerArray[i].hConnectThread), NULL, ConnectThreadFn, (void *)(&(viewerArray[i])));
#endif
	}
	else {
#ifdef WIN32
		viewerArray[i].hConnectThread2 = CreateThread(NULL, 0, ConnectThreadFn2, (void *)(&(viewerArray[i])), 0, &dwID);
#else
		pthread_create(&(viewerArray[i].hConnectThread2), NULL, ConnectThreadFn2, (void *)(&(viewerArray[i])));
#endif
	}
}

void CShiyong::ConnectRevNode(int i, char *password)
{
	log_msg_f(LOG_LEVEL_DEBUG, "ConnectRevNode(%d)...\n", i);
	if (m_bQuit) {
		return;
	}

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
	viewerArray[i].bConnecting = TRUE;
	viewerArray[i].bConnected = FALSE;
	viewerArray[i].bTopoPrimary = FALSE;

	viewerArray[i].m_sps_len = 0;
	viewerArray[i].m_pps_len = 0;

	strncpy(viewerArray[i].password, password, sizeof(viewerArray[i].password));
	//<---------------------------------------}}}}

	//mac_addr(viewerArray[i].anypcNode.node_id_str, viewerArray[i].httpOP.m1_peer_node_id, NULL);


	DWORD dwID = 0;
	{
#ifdef WIN32
		viewerArray[i].hConnectThreadRev = CreateThread(NULL, 0, ConnectThreadFnRev, (void *)(&(viewerArray[i])), 0, &dwID);
#else
		pthread_create(&(viewerArray[i].hConnectThreadRev), NULL, ConnectThreadFnRev, (void *)(&(viewerArray[i])));
#endif
	}
}

void CShiyong::DisconnectNode(VIEWER_NODE *pViewerNode)
{
	log_msg_f(LOG_LEVEL_DEBUG, "DisconnectNode(%d)...\n", pViewerNode->nID);

	if (pViewerNode->bUsing == FALSE) {
		return;
	}
	//if (pViewerNode->bTopoPrimary) {
	//	CtrlCmd_AV_STOP(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);
	//}
	if (g_pShiyong->currentSourceIndex == pViewerNode->nID) {

		//尝试找到另一个已连接的ViewNode
		int i;
		for (i = 0; i < MAX_VIEWER_NUM; i++)
		{
			if (g_pShiyong->viewerArray[i].bUsing == FALSE || g_pShiyong->viewerArray[i].bConnected == FALSE) {
				continue;
			}
			if (g_pShiyong->viewerArray[i].bTopoPrimary == FALSE && g_pShiyong->currentSourceIndex != i) {
				break;
			}
		}
		if (i < MAX_VIEWER_NUM)//找到一个！切换。。。
		{
			g_pShiyong->SwitchMediaSource(g_pShiyong->currentSourceIndex, i);
		}
		else {
			CtrlCmd_AV_STOP(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);
		}
	}

	pViewerNode->bQuitRecvSocketLoop = TRUE;
}

void CShiyong::ReturnViewerNode(VIEWER_NODE *pViewerNode)
{
	if (NULL != pViewerNode)
	{
		pViewerNode->bTopoPrimary = FALSE;
		pViewerNode->bConnected = FALSE;
		pViewerNode->bConnecting = FALSE;
		pViewerNode->bUsing = FALSE;
		pViewerNode->nID = -1;
	}
}

void CShiyong::CalculateTimeoutMedia()
{
	if (device_topo_level <= 2) {
		timeoutMedia = 0;
	}
	else if (device_topo_level == 3) {
		timeoutMedia = g1_stream_timeout_l3;
	}
	else if (device_topo_level == 4) {
		timeoutMedia = g1_stream_timeout_l3 + g1_stream_timeout_step;
	}
	else {// Viewer App
		timeoutMedia = g1_stream_timeout_l3 + g1_stream_timeout_step + g1_stream_timeout_step;
	}
}

void CShiyong::SwitchMediaSource(int oldIndex, int newIndex)
{
	log_msg_f(LOG_LEVEL_DEBUG, "SwitchMediaSource(%d => %d)...\n", oldIndex, newIndex);

	if (oldIndex < 0 || oldIndex >= MAX_VIEWER_NUM)	{
		return;
	}
	if (newIndex < 0 || newIndex >= MAX_VIEWER_NUM)	{
		return;
	}
	if (oldIndex == newIndex)	{
		return;
	}

	//切换进行中。。。避免下次Check又重复切换操作
	currentLastMediaTime = 0;

	viewerArray[oldIndex].bTopoPrimary = FALSE;
	viewerArray[newIndex].bTopoPrimary = TRUE;
	currentSourceIndex = newIndex;

	//if (viewerArray[newIndex].bTopoPrimary)
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
		CtrlCmd_AV_START(viewerArray[newIndex].httpOP.m1_use_sock_type, viewerArray[newIndex].httpOP.m1_use_udt_sock, bFlags, bVideoSize, bVideoFrameRate, audioChannel, videoChannel);
	}

	//if (viewerArray[oldIndex].bTopoPrimary == FALSE)
	{
		CtrlCmd_AV_STOP(viewerArray[oldIndex].httpOP.m1_use_sock_type, viewerArray[oldIndex].httpOP.m1_use_udt_sock);
	}
}

void CShiyong::UnregisterNode(VIEWER_NODE *pViewerNode)
{
	HandleDoUnregister(pViewerNode, (void *)0/*is_connected*/);
}

BOOL CShiyong::ShouldDoHttpOP()
{
	//一级转发器
	if (device_topo_level == 0) {
		return TRUE;
	}

	BOOL bConnected = FALSE;
	for (int i = 0; i < MAX_VIEWER_NUM; i++)
	{
		if (viewerArray[i].bConnected) {
			bConnected = TRUE;
			break;
		}
	}
	if (bConnected == FALSE)
	{
		device_topo_level = 1;
		return TRUE;
	}

	//已连接，看device_topo_level
	if (device_topo_level == 1) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

BOOL CShiyong::ShouldDoAdjustAndOptimize()
{
	//一级转发器
	if (device_topo_level == 0) {
		return FALSE;
	}

	BOOL bConnected = FALSE;
	for (int i = 0; i < MAX_VIEWER_NUM; i++)
	{
		if (viewerArray[i].bConnected) {
			bConnected = TRUE;
			break;
		}
	}
	if (bConnected == FALSE)
	{
		device_topo_level = 1;
		return TRUE;
	}

	//已连接，看device_topo_level
	if (device_topo_level == 1) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

// -1: Not exist
int CShiyong::FindConnectingItemViewerOwnerNode(BYTE *node_id)
{
	DWORD now = time(NULL);

	for (int i = 0; i < MAX_CONNECTING_EVENT_NUM; i++)
	{
		if (FALSE == connecting_event_table[i].bUsing) {
			continue;
		}
		if (now - connecting_event_table[i].create_time > 60) {
			connecting_event_table[i].bUsing = FALSE;
			connecting_event_table[i].nID = -1;
			continue;
		}
		if (memcmp(connecting_event_table[i].viewer_owner_node_id, node_id, 6) == 0) {
			return i;
		}
	}
	return -1;
}

// -1: Not exist
int CShiyong::FindConnectingItemViewerNode(BYTE *node_id)
{
	DWORD now = time(NULL);

	for (int i = 0; i < MAX_CONNECTING_EVENT_NUM; i++)
	{
		if (FALSE == connecting_event_table[i].bUsing) {
			continue;
		}
		if (now - connecting_event_table[i].create_time > 60) {
			connecting_event_table[i].bUsing = FALSE;
			connecting_event_table[i].nID = -1;
			continue;
		}
		if (memcmp(connecting_event_table[i].viewer_node_id, node_id, 6) == 0) {
			return i;
		}
	}
	return -1;
}

// -1: Not exist
int CShiyong::FindConnectingItemGuajiNode(BYTE *node_id)
{
	DWORD now = time(NULL);

	for (int i = 0; i < MAX_CONNECTING_EVENT_NUM; i++)
	{
		if (FALSE == connecting_event_table[i].bUsing) {
			continue;
		}
		if (now - connecting_event_table[i].create_time > 60) {
			connecting_event_table[i].bUsing = FALSE;
			connecting_event_table[i].nID = -1;
			continue;
		}
		if (memcmp(connecting_event_table[i].guaji_node_id, node_id, 6) == 0) {
			return i;
		}
	}
	return -1;
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
	pthread_mutex_lock(&route_table_csec);////////
	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (device_route_table[i].bUsing == FALSE) {
			continue;
		}
		if (memcmp(device_route_table[i].node_id, dest_node_id, 6) == 0) {
			pthread_mutex_unlock(&route_table_csec);////////
			return i;
		}
	}
	pthread_mutex_unlock(&route_table_csec);////////
	return -1;
}

int CShiyong::FindRouteNode(BYTE *node_id)
{
	int ret = -1;
	pthread_mutex_lock(&route_table_csec);////////
	ret = FindRouteNode_NoLock(node_id);
	pthread_mutex_unlock(&route_table_csec);////////
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
	pthread_mutex_lock(&route_table_csec);////////

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

	pthread_mutex_unlock(&route_table_csec);////////
}

void CShiyong::CheckTopoRouteTable()
{
	time_t now = time(NULL);

	pthread_mutex_lock(&route_table_csec);////////

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

	pthread_mutex_unlock(&route_table_csec);////////
}

//-1: Not found
int CShiyong::FindUncleGuajiNode_NoLock(int father_topo_level, BYTE *father_device_node_id, int *device_free_streams)
{
	int device_index = -1;
	int max_unconnected_guaji_nodes = 0;

	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (FALSE == device_route_table[i].bUsing) {
			continue;
		}
		if (ROUTE_ITEM_TYPE_DEVICENODE != device_route_table[i].route_item_type) {
			continue;
		}
		if (father_topo_level != device_route_table[i].topo_level) {
			continue;
		}
		if (memcmp(device_route_table[i].node_id, father_device_node_id, 6) == 0) {
			continue;
		}

		if (device_route_table[i].u.device_info.unconnected_guaji_nodes > max_unconnected_guaji_nodes) {
			max_unconnected_guaji_nodes = device_route_table[i].u.device_info.unconnected_guaji_nodes;
			device_index = i;
		}
	}

	if (0 == max_unconnected_guaji_nodes) {
		return -1;
	}

	if (NULL != device_free_streams) {
		*device_free_streams = device_route_table[device_index].u.device_info.device_free_streams;
	}

	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (FALSE == device_route_table[i].bUsing) {
			continue;
		}
		if (ROUTE_ITEM_TYPE_GUAJINODE != device_route_table[i].route_item_type) {
			continue;
		}
		if (FALSE != device_route_table[i].is_connected) {
			continue;
		}
		if (memcmp(device_route_table[i].owner_node_id, device_route_table[device_index].node_id, 6) != 0) {
			continue;
		}
		if (FindConnectingItemGuajiNode(device_route_table[i].node_id) == -1) {
			return i;
		}
	}
	return -1;
}

//-1: Not found
int CShiyong::FindDeviceFreeViewerNode_NoLock(BYTE *device_node_id)
{
	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (FALSE == device_route_table[i].bUsing) {
			continue;
		}
		if (ROUTE_ITEM_TYPE_VIEWERNODE != device_route_table[i].route_item_type) {
			continue;
		}
		if (FALSE != device_route_table[i].is_connected) {
			continue;
		}
		if (memcmp(device_route_table[i].owner_node_id, device_node_id, 6) != 0) {
			continue;
		}
		if (FindConnectingItemViewerNode(device_route_table[i].node_id) == -1) {
			return i;
		}
	}
	return -1;
}

void CShiyong::AdjustTopoStructure()
{
	pthread_mutex_lock(&route_table_csec);////////

	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (FALSE == device_route_table[i].bUsing) {
			continue;
		}
		if (ROUTE_ITEM_TYPE_DEVICENODE != device_route_table[i].route_item_type) {
			continue;
		}
		if (device_route_table[i].topo_level < 3) {
			continue;
		}
		if (FindConnectingItemViewerOwnerNode(device_route_table[i].node_id) != -1) {
			continue;
		}
		if (device_route_table[i].u.device_info.connected_viewer_nodes < 2)
		{//找到一个L3或L4的device node，只有一个向上的连接。
			int ret_index;
			int ret_index2;
			BYTE viewer_node_id[6];
			BYTE viewer_peer_node_id[6];
			memset(viewer_peer_node_id, 0, 6);

			for (int j = 0; j < MAX_ROUTE_ITEM_NUM; j++)
			{
				if (FALSE == device_route_table[j].bUsing) {
					continue;
				}
				if (ROUTE_ITEM_TYPE_VIEWERNODE != device_route_table[j].route_item_type) {
					continue;
				}
				if (TRUE != device_route_table[j].is_connected) {
					continue;
				}
				if (memcmp(device_route_table[j].owner_node_id, device_route_table[i].node_id, 6) == 0) {
					memcpy(viewer_node_id, device_route_table[j].node_id, 6);
					memcpy(viewer_peer_node_id, device_route_table[j].peer_node_id, 6);
					break;
				}
			}

			if (viewer_peer_node_id[0] == 0 && viewer_peer_node_id[1] == 0 && viewer_peer_node_id[2] == 0 && viewer_peer_node_id[3] == 0 && viewer_peer_node_id[4] == 0 && viewer_peer_node_id[5] == 0) {
				continue;
			}

			ret_index = FindRouteNode_NoLock(viewer_peer_node_id);
			if (ret_index == -1 || device_route_table[ret_index].route_item_type != ROUTE_ITEM_TYPE_GUAJINODE) {
				continue;
			}

			int uncle_device_free_streams = 0;
			ret_index2 = FindUncleGuajiNode_NoLock(device_route_table[i].topo_level - 1, device_route_table[ret_index].owner_node_id, &uncle_device_free_streams);
			ret_index = FindDeviceFreeViewerNode_NoLock(device_route_table[i].node_id);
			if (ret_index2 == -1 || ret_index == -1) {
				continue;
			}

			/////////////////////////////////////////////////////////////////////////////////////////////
			int ii;
			for (ii = 0; ii < MAX_CONNECTING_EVENT_NUM; ii++)
			{
				if (FALSE == connecting_event_table[ii].bUsing) {
					break;
				}
			}
			if (ii == MAX_CONNECTING_EVENT_NUM) {
				printf("对不起，没有空闲的CONNECTING_EVENT_ITEM!\n");
				break;//没必要检查其他的Device Node了！！！
			}
			connecting_event_table[ii].nID = ii;
			connecting_event_table[ii].bUsing = TRUE;
			connecting_event_table[ii].create_time = time(NULL);
			memcpy(connecting_event_table[ii].viewer_owner_node_id, device_route_table[i].node_id, 6);
			memcpy(connecting_event_table[ii].viewer_node_id, device_route_table[ret_index].node_id, 6);
			memcpy(connecting_event_table[ii].guaji_node_id, device_route_table[ret_index2].node_id, 6);
			connecting_event_table[ii].switch_after_connected = (uncle_device_free_streams >= 3 ? TRUE : FALSE);
			/////////////////////////////////////////////////////////////////////////////////////////////

			HttpOperate::DoDrop("gbk", "zh", device_node_id, TRUE,  connecting_event_table[ii].viewer_node_id);
			HttpOperate::DoDrop("gbk", "zh", device_node_id, FALSE, connecting_event_table[ii].guaji_node_id);

			//生成2个Event，向下发送。。。
			BYTE *viewer = &(device_route_table[ret_index].node_id[0]);
			BYTE *gauji = &(device_route_table[ret_index2].node_id[0]);
			int guaji_index;
			char szEvent[256];//the_node_id   wait_time  type     peer_node_id  pub_ip  pub_port  no_nat  nat_type  ip  port 
			if (device_route_table[ret_index2].u.node_nat_info.no_nat || device_route_table[ret_index].u.node_nat_info.no_nat == FALSE)
			{
				snprintf(szEvent, sizeof(szEvent), "%02X-%02X-%02X-%02X-%02X-%02X|0|Connect|%02X-%02X-%02X-%02X-%02X-%02X|%s|%s|%d|%d|%s|%s", 
										viewer[0],viewer[1],viewer[2],viewer[3],viewer[4],viewer[5],
										gauji[0], gauji[1], gauji[2], gauji[3], gauji[4], gauji[5],
										device_route_table[ret_index2].u.node_nat_info.pub_ip_str,
										device_route_table[ret_index2].u.node_nat_info.pub_port_str,
										(device_route_table[ret_index2].u.node_nat_info.no_nat ? 1 : 0),
										device_route_table[ret_index2].u.node_nat_info.nat_type,
										device_route_table[ret_index2].u.node_nat_info.ip_str,
										device_route_table[ret_index2].u.node_nat_info.port_str);
				guaji_index = device_route_table[ret_index].guaji_index;
				CtrlCmd_TOPO_EVENT(SOCKET_TYPE_TCP, arrServerProcesses[guaji_index].m_fhandle, viewer, (const char *)szEvent);

				snprintf(szEvent, sizeof(szEvent), "%02X-%02X-%02X-%02X-%02X-%02X|0|Accept|%02X-%02X-%02X-%02X-%02X-%02X|%s|%s|%d|%d|%s|%s", 
										gauji[0],gauji[1],gauji[2],gauji[3],gauji[4],gauji[5],
										viewer[0], viewer[1], viewer[2], viewer[3], viewer[4], viewer[5],
										device_route_table[ret_index].u.node_nat_info.pub_ip_str,
										device_route_table[ret_index].u.node_nat_info.pub_port_str,
										(device_route_table[ret_index].u.node_nat_info.no_nat ? 1 : 0),
										device_route_table[ret_index].u.node_nat_info.nat_type,
										device_route_table[ret_index].u.node_nat_info.ip_str,
										device_route_table[ret_index].u.node_nat_info.port_str);
				guaji_index = device_route_table[ret_index2].guaji_index;
				CtrlCmd_TOPO_EVENT(SOCKET_TYPE_TCP, arrServerProcesses[guaji_index].m_fhandle, gauji, (const char *)szEvent);
			}
			else {
				snprintf(szEvent, sizeof(szEvent), "%02X-%02X-%02X-%02X-%02X-%02X|0|ConnectRev|%02X-%02X-%02X-%02X-%02X-%02X|%s|%s|%d|%d|%s|%s", 
										viewer[0],viewer[1],viewer[2],viewer[3],viewer[4],viewer[5],
										gauji[0], gauji[1], gauji[2], gauji[3], gauji[4], gauji[5],
										device_route_table[ret_index2].u.node_nat_info.pub_ip_str,
										device_route_table[ret_index2].u.node_nat_info.pub_port_str,
										(device_route_table[ret_index2].u.node_nat_info.no_nat ? 1 : 0),
										device_route_table[ret_index2].u.node_nat_info.nat_type,
										device_route_table[ret_index2].u.node_nat_info.ip_str,
										device_route_table[ret_index2].u.node_nat_info.port_str);
				guaji_index = device_route_table[ret_index].guaji_index;
				CtrlCmd_TOPO_EVENT(SOCKET_TYPE_TCP, arrServerProcesses[guaji_index].m_fhandle, viewer, (const char *)szEvent);

				snprintf(szEvent, sizeof(szEvent), "%02X-%02X-%02X-%02X-%02X-%02X|0|AcceptRev|%02X-%02X-%02X-%02X-%02X-%02X|%s|%s|%d|%d|%s|%s", 
										gauji[0],gauji[1],gauji[2],gauji[3],gauji[4],gauji[5],
										viewer[0], viewer[1], viewer[2], viewer[3], viewer[4], viewer[5],
										device_route_table[ret_index].u.node_nat_info.pub_ip_str,
										device_route_table[ret_index].u.node_nat_info.pub_port_str,
										(device_route_table[ret_index].u.node_nat_info.no_nat ? 1 : 0),
										device_route_table[ret_index].u.node_nat_info.nat_type,
										device_route_table[ret_index].u.node_nat_info.ip_str,
										device_route_table[ret_index].u.node_nat_info.port_str);
				guaji_index = device_route_table[ret_index2].guaji_index;
				CtrlCmd_TOPO_EVENT(SOCKET_TYPE_TCP, arrServerProcesses[guaji_index].m_fhandle, gauji, (const char *)szEvent);
			}

		}//找到一个L3或L4的device node，只有一个向上的连接。
	}

	pthread_mutex_unlock(&route_table_csec);////////
}

//查找指定Level的，stream最拥挤的那个device node. 要求device_free_streams<0
//-1: Not found
int CShiyong::FindLevelHeaviestStreamDeviceNode_NoLock(int topo_level)
{
	int device_index = -1;
	int min_device_free_streams = 0;

	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (FALSE == device_route_table[i].bUsing) {
			continue;
		}
		if (ROUTE_ITEM_TYPE_DEVICENODE != device_route_table[i].route_item_type) {
			continue;
		}
		if (topo_level != device_route_table[i].topo_level) {
			continue;
		}

		if (device_route_table[i].u.device_info.device_free_streams < min_device_free_streams) {
			min_device_free_streams = device_route_table[i].u.device_info.device_free_streams;
			device_index = i;
		}
	}

	if (0 == min_device_free_streams) {
		return -1;
	}
	else {
		return device_index;
	}
}

//对于给定的ViewerNode，找出其另一个兄弟ViewerNode所连接的上级DeviceNode的device_free_streams
//如果找不到，或者另一个兄弟ViewerNode未连接，则返回device_free_streams = -10000
int CShiyong::FindTheOtherViewerFatherDevieFreeStreams_NoLock(BYTE *viewer_node_id)
{
	int i;
	int ret = -10000;

	BYTE owner_node_id[6];
	int tmp_index = FindRouteNode_NoLock(viewer_node_id);
	if (tmp_index == -1) {
		return ret;
	}
	memcpy(owner_node_id, device_route_table[tmp_index].owner_node_id, 6);

	BYTE guaji_node_id[6];
	for (i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (FALSE == device_route_table[i].bUsing) {
			continue;
		}
		if (ROUTE_ITEM_TYPE_VIEWERNODE != device_route_table[i].route_item_type) {
			continue;
		}
		if (memcmp(device_route_table[i].node_id, viewer_node_id, 6) == 0) {
			continue;
		}
		if (memcmp(device_route_table[i].owner_node_id, owner_node_id, 6) != 0) {
			continue;
		}
		if (device_route_table[i].is_connected) {
			memcpy(guaji_node_id, device_route_table[i].peer_node_id, 6);
			break;
		}
	}
	if (i == MAX_ROUTE_ITEM_NUM) {
		return ret;
	}

	BYTE device_node_id[6];
	tmp_index = FindRouteNode_NoLock(guaji_node_id);
	if (tmp_index == -1) {
		return ret;
	}
	memcpy(device_node_id, device_route_table[tmp_index].owner_node_id, 6);

	tmp_index = FindRouteNode_NoLock(device_node_id);
	if (tmp_index == -1) {
		return ret;
	}
	return device_route_table[tmp_index].u.device_info.device_free_streams;
}

void CShiyong::OptimizeStreamPath()
{
	pthread_mutex_lock(&route_table_csec);////////

	for (int level = 2; level <= 3; level++)
	{
		int device_index = FindLevelHeaviestStreamDeviceNode_NoLock(level);
		if (device_index == -1) {
			continue;
		}

		int max_device_free_streams = -10000;
		BYTE viewer_node_id[6];
		for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
		{
			if (FALSE == device_route_table[i].bUsing) {
				continue;
			}
			if (ROUTE_ITEM_TYPE_GUAJINODE != device_route_table[i].route_item_type) {
				continue;
			}
			if (memcmp(device_route_table[i].owner_node_id, device_route_table[device_index].node_id, 6) != 0) {
				continue;
			}
			if (TRUE != device_route_table[i].is_streaming) {
				continue;
			}

			int device_free_streams = FindTheOtherViewerFatherDevieFreeStreams_NoLock(device_route_table[i].peer_node_id);
			if (device_free_streams > max_device_free_streams) {
				max_device_free_streams = device_free_streams;
				memcpy(viewer_node_id, device_route_table[i].peer_node_id, 6);
			}
		}

		if (max_device_free_streams >= 2) //这里根据实际测试情况来，避免超调！！！
		{
			int ret_index = FindTopoRouteItem(viewer_node_id);
			if (-1 != ret_index)
			{
				char szEvent[64];
				snprintf(szEvent, sizeof(szEvent), "%02X-%02X-%02X-%02X-%02X-%02X|0|Switch|00-00-00-00-00-00", 
							viewer_node_id[0],viewer_node_id[1],viewer_node_id[2],viewer_node_id[3],viewer_node_id[4],viewer_node_id[5]);

				int guaji_index = device_route_table[ret_index].guaji_index;
				CtrlCmd_TOPO_EVENT(SOCKET_TYPE_TCP, arrServerProcesses[guaji_index].m_fhandle, viewer_node_id, (const char *)szEvent);
			}
		}
	}//for(level)

	pthread_mutex_unlock(&route_table_csec);////////
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

		/* is_connected */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		if (strcmp(value, "1") == 0) {
			pRouteItem->is_connected = TRUE;
		}
		else {
			pRouteItem->is_connected = FALSE;
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
		//if (p != NULL) {
		//	*p = '\0';  /* Last field */
		//}
#if 0//不要把联合体u里面的node_nat_info覆盖了！！！
		strncpy(pRouteItem->u.device_info.device_node_str, value, sizeof(pRouteItem->u.device_info.device_node_str));
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

		/* connected_viewer_nodes */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		pRouteItem->u.device_info.connected_viewer_nodes = atol(value);

		/* unconnected_guaji_nodes */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		pRouteItem->u.device_info.unconnected_guaji_nodes = atol(value);

		/* device_free_streams */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		pRouteItem->u.device_info.device_free_streams = atol(value);

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
		//if (p != NULL) {
		//	*p = '\0';  /* Last field */
		//}
		strncpy(pRouteItem->u.device_info.device_node_str, value, sizeof(pRouteItem->u.device_info.device_node_str));
	}

	return TRUE;
}

int CShiyong::UpdateRouteTable(int guajiIndex, const char *report_string)
{
	char *tmp_string;
	char *start;
	char name[32];
	char value[1024];
	char *next_start = NULL;
	TOPO_ROUTE_ITEM temp_route_item;
	int ret = -1;

	tmp_string = (char *)malloc(strlen(report_string) + 1);
	strcpy(tmp_string, report_string);
	start = tmp_string;

	while(TRUE) {

		if (ParseLine(start, name, sizeof(name), value, sizeof(value), &next_start) == FALSE) {
			free(tmp_string);
			return -1;
		}

		if (strcmp(name, "row") == 0) {
			ParseReportNode(value, &temp_route_item);
			temp_route_item.guaji_index = guajiIndex;
			temp_route_item.last_refresh_time = time(NULL);
			int index = FindTopoRouteItem(temp_route_item.node_id);
			pthread_mutex_lock(&route_table_csec);////////
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
					log_msg_f(LOG_LEVEL_ERROR, "对不起，没有空闲的TOPO_ROUTE_ITEM!\n");
				}
			}
			else {
				device_route_table[index] = temp_route_item;
				device_route_table[index].bUsing = TRUE;
				device_route_table[index].nID = index;
			}
			pthread_mutex_unlock(&route_table_csec);////////
		}

		if (next_start == NULL) {
			break;
		}
		start = next_start;
	}

	free(tmp_string);
	return ret;
}

int CShiyong::GetConnectedViewerNodes()
{
	int nConnectedViewer = 0;

	for (int i = 0; i < MAX_VIEWER_NUM; i++)
	{
		if (viewerArray[i].bConnected) {
			nConnectedViewer += 1;
		}
	}

	return nConnectedViewer;
}

int CShiyong::GetUnconnectedGuajiNodes()
{
	int nConnectedGuajiNodes = 0;

	for (int i = 0; i < MAX_SERVER_NUM; i++)
	{
		if (arrServerProcesses[i].m_bConnected) {
			nConnectedGuajiNodes += 1;
		}
	}

	return (MAX_SERVER_NUM - nConnectedGuajiNodes);
}

int CShiyong::GetDeviceFreeStreams()
{
	int nCurrentStreams = 0;

	for (int i = 0; i < MAX_SERVER_NUM; i++)
	{
		if (arrServerProcesses[i].m_bAVStarted) {
			nCurrentStreams += 1;
		}
	}

	return (device_max_streams - nCurrentStreams);
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
			"|%d"//"&connected_viewer_nodes=%d"
			"|%d"//"&unconnected_guaji_nodes=%d"
			"|%d"//"&device_free_streams=%d"
			"|%02X-%02X-%02X-%02X-%02X-%02X"//"      device_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
			"|%s"//"&device_uuid=%s"
			"|%s"//"&node_name=%s"
			"|%ld"//"&version=%ld"
			"|%s"//"&os_info=%s"
			,
			ROUTE_ITEM_TYPE_DEVICENODE,
			device_topo_level,
			GetConnectedViewerNodes(),
			GetUnconnectedGuajiNodes(),
			GetDeviceFreeStreams(),
			device_node_id[0],device_node_id[1],device_node_id[2],device_node_id[3],device_node_id[4],device_node_id[5],
			g0_device_uuid, UrlEncode(g0_node_name).c_str(), g0_version, g0_os_info);

		strcat(report_string, "row=");
		strcat(report_string, szTemp);
		strcat(report_string, "\n");
	}

	for (i = 0; i < MAX_SERVER_NUM; i++)
	{
		strcat(report_string, arrServerProcesses[i].m_szReport);
	}

	for (i = 0; i < MAX_VIEWER_NUM; i++)
	{
		snprintf(szTemp, sizeof(szTemp), 
			"%d"//"node_type=%d"
			"|%d"//"&topo_level=%d"
			"|%02X-%02X-%02X-%02X-%02X-%02X"//"owner_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
			"|%02X-%02X-%02X-%02X-%02X-%02X"//"      node_id=%02X-%02X-%02X-%02X-%02X-%02X"
			"|%02X-%02X-%02X-%02X-%02X-%02X"//" peer_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
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
			log_msg_f(LOG_LEVEL_DEBUG, "CtrlCmd_TOPO_REPORT ==> viewerArray[%d], report_string=\n%s\n", i, report_string);
			CtrlCmd_TOPO_REPORT(viewerArray[i].httpOP.m1_use_sock_type, viewerArray[i].httpOP.m1_use_udt_sock, device_node_id, report_string);
			break;
		}
	}

	free(report_string);
	return 0;
}

int CShiyong::get_joined_channel_id()
{
	return g1_joined_channel_id;
}

const char * CShiyong::get_public_ip()
{
	static char s_public_ip[16];
	struct in_addr inaddr;

	strcpy(s_public_ip, "");

	int i;
	for (i = 0; i < MAX_VIEWER_NUM; i++)
	{
		if (viewerArray[i].httpOP.m0_pub_ip != 0) {
			inaddr.s_addr = viewerArray[i].httpOP.m0_pub_ip;
			strcpy(s_public_ip, inet_ntoa(inaddr));
			break;
		}
	}
	if (i == MAX_VIEWER_NUM) {
		log_msg_f(LOG_LEVEL_ERROR, "get_public_ip: No viewer has valid m0_pub_ip!\n");
	}
	return s_public_ip;
}

const char * CShiyong::get_node_array()
{
	static char s_node_array[1024*32];

	strcpy(s_node_array, "");

	//首先处理本DeviceNode的ViewerNode。。。
	for (int i = 0; i < MAX_VIEWER_NUM; i++)
	{
		if (viewerArray[i].bConnecting || viewerArray[i].bConnected) {
			continue;
		}
		if (FindConnectingItemViewerNode(viewerArray[i].httpOP.m0_node_id) != -1) {
			continue;
		}
		char szTemp[1024*2];
		snprintf(szTemp, sizeof(szTemp), 
			"%d"//"&topo_level=%d"
			"|%d"//"&device_free_streams=%d"
			"|%02X-%02X-%02X-%02X-%02X-%02X"//"      node_id=%02X-%02X-%02X-%02X-%02X-%02X"
			"|%d"//"&is_admin=%d"
			"|%d"//"&is_busy=%d"
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
			device_topo_level,
			GetDeviceFreeStreams(),
			viewerArray[i].httpOP.m0_node_id[0], viewerArray[i].httpOP.m0_node_id[1], viewerArray[i].httpOP.m0_node_id[2], viewerArray[i].httpOP.m0_node_id[3], viewerArray[i].httpOP.m0_node_id[4], viewerArray[i].httpOP.m0_node_id[5], 
			1, 0,
			viewerArray[i].httpOP.MakeIpStr(), viewerArray[i].httpOP.m0_port, viewerArray[i].httpOP.MakePubIpStr(), viewerArray[i].httpOP.m0_pub_port, 
			(viewerArray[i].httpOP.m0_no_nat ? 1 : 0),
			viewerArray[i].httpOP.m0_nat_type,
			g0_device_uuid, UrlEncode(g0_node_name).c_str(), g0_version, g0_os_info);

		if (s_node_array[0] !=  '\0') {
			strcat(s_node_array, ";");
		}
		if (strlen(s_node_array) + strlen(szTemp) < sizeof(s_node_array)) {
			strcat(s_node_array, szTemp);
		}
		else {
			log_msg_f(LOG_LEVEL_ERROR, "s_node_array string buff overflow!!!\n");
			break;
		}
	}

	pthread_mutex_lock(&route_table_csec);////////

	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (FALSE == device_route_table[i].bUsing) {
			continue;
		}
		if (ROUTE_ITEM_TYPE_VIEWERNODE != device_route_table[i].route_item_type && ROUTE_ITEM_TYPE_GUAJINODE != device_route_table[i].route_item_type) {
			continue;
		}
		if (TRUE == device_route_table[i].is_connected || TRUE == device_route_table[i].is_streaming) {
			continue;
		}

		if (ROUTE_ITEM_TYPE_VIEWERNODE == device_route_table[i].route_item_type && FindConnectingItemViewerNode(device_route_table[i].node_id) != -1) {
			continue;
		}
		if (ROUTE_ITEM_TYPE_GUAJINODE == device_route_table[i].route_item_type && FindConnectingItemGuajiNode(device_route_table[i].node_id) != -1) {
			continue;
		}

		int is_admin = (device_route_table[i].route_item_type == ROUTE_ITEM_TYPE_VIEWERNODE) ? 1 : 0;
		int device_free_streams;
		char szDeviceInfo[200];
		int device_node_index = FindRouteNode_NoLock(device_route_table[i].owner_node_id);
		if (device_node_index == -1)
		{
			if (memcmp(device_route_table[i].owner_node_id, device_node_id, 6) == 0)
			{
				device_free_streams = this->GetDeviceFreeStreams();

				snprintf(szDeviceInfo, sizeof(szDeviceInfo), 
					"%s"//"&device_uuid=%s"
					"|%s"//"&node_name=%s"
					"|%ld"//"&version=%ld"
					"|%s"//"&os_info=%s"
					,
					g0_device_uuid, UrlEncode(g0_node_name).c_str(), g0_version, g0_os_info);
			} 
			else {
				device_free_streams = 0;
				strcpy(szDeviceInfo, "");
			}
		}
		else {
			device_free_streams = device_route_table[device_node_index].u.device_info.device_free_streams;
			strncpy(szDeviceInfo, device_route_table[device_node_index].u.device_info.device_node_str, sizeof(szDeviceInfo));
		}

		char szTemp[1024*2];
		snprintf(szTemp, sizeof(szTemp), 
			"%d"//"&topo_level=%d"
			"|%d"//"&device_free_streams=%d"
			"|%02X-%02X-%02X-%02X-%02X-%02X"//"      node_id=%02X-%02X-%02X-%02X-%02X-%02X"
			"|%d"//"&is_admin=%d"
			"|%d"//"&is_busy=%d"
			"|%s"//"&ip"
			"|%s"//"&port"
			"|%s"//"&pub_ip"
			"|%s"//"&pub_port"
			"|%d"//"&no_nat=%d"
			"|%d"//"&nat_type=%d"
			"|%s"//"&device_uuid|node_name|version|os_info"
			,
			device_route_table[i].topo_level,
			device_free_streams,
			device_route_table[i].node_id[0],device_route_table[i].node_id[1],device_route_table[i].node_id[2],device_route_table[i].node_id[3],device_route_table[i].node_id[4],device_route_table[i].node_id[5],
			is_admin,
			(0),
			device_route_table[i].u.node_nat_info.ip_str, device_route_table[i].u.node_nat_info.port_str, 
			device_route_table[i].u.node_nat_info.pub_ip_str, device_route_table[i].u.node_nat_info.pub_port_str, 
			(device_route_table[i].u.node_nat_info.no_nat ? 1 : 0),
			device_route_table[i].u.node_nat_info.nat_type,
			szDeviceInfo);

		if (s_node_array[0] !=  '\0') {
			strcat(s_node_array, ";");
		}
		if (strlen(s_node_array) + strlen(szTemp) < sizeof(s_node_array)) {
			strcat(s_node_array, szTemp);
		}
		else {
			log_msg_f(LOG_LEVEL_ERROR, "s_node_array string buff overflow!!!\n");
			break;
		}
	}

	pthread_mutex_unlock(&route_table_csec);////////

	log_msg_f(LOG_LEVEL_DEBUG, "s_node_array=%s\n", s_node_array);
	return s_node_array;
}

//获取当前device的路由表条目数量
int CShiyong::get_route_item_num()
{
	int count = 0;

	pthread_mutex_lock(&route_table_csec);////////

	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (FALSE == device_route_table[i].bUsing) {
			continue;
		}
		count += 1;
	}

	pthread_mutex_unlock(&route_table_csec);////////

	return count;
}

//获取当前树的Viewer连接数变化率
int CShiyong::get_viewer_grow_rate()
{
	int ret;
	int now = time(NULL);
	int current_viewer_num = get_level_current_streams(4);
	if (last_viewer_num_time != 0) {
		ret = (current_viewer_num - last_viewer_num)*10000/(now - last_viewer_num_time);
	}
	else {
		ret = 0;
	}
	last_viewer_num_time = now;
	last_viewer_num = current_viewer_num;
	return ret;
}

//获取树中指定topo级别的设备总数
int CShiyong::get_level_device_num(int topo_level)
{
	time_t now = time(NULL);
	int count = 0;

	//本device node并不在device_route_table中！！！
	if (topo_level == 1) {
		return 1;
	}

	pthread_mutex_lock(&route_table_csec);////////

	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (FALSE == device_route_table[i].bUsing) {
			continue;
		}
		if (ROUTE_ITEM_TYPE_DEVICENODE != device_route_table[i].route_item_type) {
			continue;
		}
		if (topo_level != device_route_table[i].topo_level) {
			continue;
		}
		if (now - device_route_table[i].last_refresh_time < g1_expire_period) {
			count += 1;
		}
	}

	pthread_mutex_unlock(&route_table_csec);////////

	return count;
}

int CShiyong::get_level_max_connections(int topo_level)
{
	time_t now = time(NULL);
	int count = 0;

	pthread_mutex_lock(&route_table_csec);////////

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

	pthread_mutex_unlock(&route_table_csec);////////

	return count;
}

int CShiyong::get_level_current_connections(int topo_level)
{
	time_t now = time(NULL);
	int count = 0;

	pthread_mutex_lock(&route_table_csec);////////

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
		if (device_route_table[i].is_connected && now - device_route_table[i].last_refresh_time < g1_expire_period) {
			count += 1;
		}
	}

	pthread_mutex_unlock(&route_table_csec);////////

	return count;
}

int CShiyong::get_level_max_streams(int topo_level)
{
	//每个device上，max_connections=max_streams*2
	int ret = get_level_max_connections(topo_level) / 2;
	int device_num = get_level_device_num(topo_level);
	if (ret < device_num * 2) {
		ret = device_num * 2;
	}
	return ret;
}

int CShiyong::get_level_current_streams(int topo_level)
{
	time_t now = time(NULL);
	int count = 0;

	pthread_mutex_lock(&route_table_csec);////////

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
		if (device_route_table[i].is_connected && device_route_table[i].is_streaming && now - device_route_table[i].last_refresh_time < g1_expire_period) {
			count += 1;
		}
	}

	pthread_mutex_unlock(&route_table_csec);////////

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


	//pthread_mutex_lock(&m_localbind_csec);////////
	//pthread_mutex_unlock(&m_localbind_csec);////////

	if (m_hThread != NULL) {
		eloop_terminate();
#ifdef WIN32
		::WaitForSingleObject(m_hThread, INFINITE);
		::CloseHandle(m_hThread);
#else
		pthread_join(m_hThread, NULL);
#endif
		m_hThread = NULL;
	}
}

