// Shiyong.cpp : ʵ���ļ�
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


#define strProductName   ("Repeater")
#define SetStatusInfo(hWnd, lpStatusInfo)   do {printf(lpStatusInfo);	printf("\n");} while(0)


static void HandleKeepLoop(void *eloop_ctx, void *timeout_ctx)
{
	eloop_register_timeout(2, 0, HandleKeepLoop, NULL, NULL);
}

static void HandleDoUnregister(void *eloop_ctx, void *timeout_ctx)
{
	VIEWER_NODE *pViewerNode = (VIEWER_NODE *)eloop_ctx;
	TRACE("@@@ DoUnregister...\n");
	pViewerNode->httpOP.DoUnregister("gbk", "zh");
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
		   
			SetStatusInfo(pDlgWnd, "����NAT̽��ɹ�! - CheckStunHttp");
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
			SetStatusInfo(pDlgWnd, "����NAT̽��ɹ�!");
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
		   
			SetStatusInfo(pDlgWnd, "����NAT̽��ɹ�! - CheckStunHttp");
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
			SetStatusInfo(pDlgWnd, "����NAT�޷���͸!!!�������绷�����ܷ�����UDPͨ�š�");
			pViewerNode->httpOP.m0_pub_ip = 0;
			pViewerNode->httpOP.m0_pub_port = 0;
			pViewerNode->httpOP.m0_no_nat = FALSE;
			pViewerNode->httpOP.m0_nat_type = nNatType;
		}
		else {
			SetStatusInfo(pDlgWnd, "����NAT̽��ɹ�!");
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


	//��������IP�ܿض˵����Ӷ˿�����,2014-6-15
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

	eloop_register_timeout(2, 0, HandleKeepLoop, NULL, NULL);
	for (int i = 0; i < MAX_VIEWER_NUM; i++)
	{
		eloop_register_timeout(1 + i * 2, 0, HandleRegister, &(g_pShiyong->viewerArray[i]), NULL);
	}

	/* Loop... */
	eloop_run();

	eloop_destroy();
	return 0;
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


	//��鿴�Ƿ��һ��rudp���ӣ���һ��rudp����Ӧ����TopoPrimary
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
					g_pShiyong->ConnectNode(index, REPEATER_PASSWORD);
				}
				else if (stricmp(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "ConnectRev") == 0)
				{
					g_pShiyong->ConnectRevNode(index, REPEATER_PASSWORD);
				}
				else if (stricmp(g_pShiyong->viewerArray[index].httpOP.m1_event_type, "Disconnect") == 0)
				{
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
	SetStatusInfo(pDlg->m_hWnd, _T("�ɹ��������ӡ�����"));
	pViewerNode->bConnected = TRUE;

	//������Ϊ����Ƶ��ء�����
	if (pViewerNode->bTopoPrimary)
	{
		//������Ƶ�ɿ����䣬��Ƶ�����ഫ�䣡����
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

	//Loop...,��ҪCShiyong::DisconnectNode()�����˳�Loop������
	pViewerNode->bQuitRecvSocketLoop = FALSE;
	RecvSocketDataLoop(pViewerNode, pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);

	{
		//CtrlCmd_AV_STOP()����CShiyong::DisconnectNode()�е���
		//CtrlCmd_AV_STOP(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);
	}

	SetStatusInfo(pDlg->m_hWnd, _T("�����Ͽ����ӡ�����"));
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
		SetStatusInfo(pDlg->m_hWnd, "StunType��֧�֣��޷����ӣ�");
		CloseHandle(pViewerNode->hConnectThread);
		pViewerNode->hConnectThread = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}


	//SetStatusInfo(pDlg->m_hWnd, "�����ύ����������Ϣ�����Ժ򡣡���");
	//nRetry = 3;
	//do {
	//	HandleRegister(pViewerNode, NULL);
	//} while (nRetry-- > 0 && pViewerNode->bNeedRegister);


	for (i = pViewerNode->httpOP.m1_wait_time; i > 0; i--) {
		_snprintf(szStatusStr, sizeof(szStatusStr), "�����Ŷӵȴ�������Ӧ��%d�롣����", i);
		SetStatusInfo(pDlg->m_hWnd, szStatusStr);
		Sleep(1000);
	}


	::EnterCriticalSection(&(pViewerNode->localbind_csec));////////


	pViewerNode->httpOP.m1_use_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (pViewerNode->httpOP.m1_use_udp_sock == INVALID_SOCKET) {
		::LeaveCriticalSection(&(pViewerNode->localbind_csec));////////
		SetStatusInfo(pDlg->m_hWnd, "����ʧ�ܣ�(udp socket failed)");
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
		SetStatusInfo(pDlg->m_hWnd, "����ʧ�ܣ�(udp bind failed)");
		closesocket(pViewerNode->httpOP.m1_use_udp_sock);
		pViewerNode->httpOP.m1_use_udp_sock = INVALID_SOCKET;
		CloseHandle(pViewerNode->hConnectThread);
		pViewerNode->hConnectThread = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}


	SetStatusInfo(pDlg->m_hWnd, "���ڴ�ͨ�����ŵ�������");
	if (FindOutConnection(pViewerNode->httpOP.m1_use_udp_sock, pViewerNode->httpOP.m0_node_id, pViewerNode->httpOP.m1_peer_node_id, 
							pViewerNode->httpOP.m1_peer_pri_ipArray, pViewerNode->httpOP.m1_peer_pri_ipCount, pViewerNode->httpOP.m1_peer_pri_port, pViewerNode->httpOP.m1_peer_ip, pViewerNode->httpOP.m1_peer_port,
							&use_ip, &use_port) != 0) {
		::LeaveCriticalSection(&(pViewerNode->localbind_csec));////////
		SetStatusInfo(pDlg->m_hWnd, "����ʧ�ܣ�(FindOutConnection failed)");
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


	/* Ϊ�˽�ʡ����ʱ�䣬�� eloop ��ִ�� DoUnregister  */
	//eloop_register_timeout(3, 0, HandleDoUnregister, pViewerNode, NULL);


	SetStatusInfo(pDlg->m_hWnd, "����ͬ�Է��������ӣ����Ժ򡣡���");

	fhandle = UDT::socket(AF_INET, SOCK_STREAM, 0);

	ConfigUdtSocket(fhandle);

	if (UDT::ERROR == UDT::bind(fhandle, pViewerNode->httpOP.m1_use_udp_sock))
	{
		::LeaveCriticalSection(&(pViewerNode->localbind_csec));////////
		SetStatusInfo(pDlg->m_hWnd, "����ʧ�ܣ�(udt bind failed)");
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
		SetStatusInfo(pDlg->m_hWnd, "��ʱ������״�����ã�ֱ���е�С���ѣ����Ժ����Ի��߲���TCP���ӣ�");
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
		SetStatusInfo(pDlg->m_hWnd, "��ʱ������״�����ã�ֱ���е�С���ѣ����Ժ����Ի��߲���TCP���ӣ�");
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


	SetStatusInfo(pDlg->m_hWnd, _T("�ɹ��������ӣ�������֤�������롣����"));
	ret = CheckPassword(pViewerNode, pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock, 2);
	if (-1 == ret) {
		SetStatusInfo(pDlg->m_hWnd, "��֤����ʱͨ�ų����޷����ӣ�");
	}
	else if (0 == ret)
	{
		SetStatusInfo(pDlg->m_hWnd, "��֤��������޷����ӣ�");
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


	SetStatusInfo(pDlg->m_hWnd, _T("������Է��������ӣ����Ժ򡣡���"));

	pViewerNode->httpOP.m1_use_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (pViewerNode->httpOP.m1_use_udp_sock == INVALID_SOCKET) {
		SetStatusInfo(pDlg->m_hWnd, _T("����ʧ�ܣ�(udp socket failed)"));
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
		SetStatusInfo(pDlg->m_hWnd, _T("����ʧ�ܣ�(udp bind failed)"));
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
		SetStatusInfo(pDlg->m_hWnd, _T("����ʧ�ܣ�(udt bind failed)"));
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
		SetStatusInfo(pDlg->m_hWnd, _T("����ʧ��(udt connect failed)�������ԡ�"));
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


	SetStatusInfo(pDlg->m_hWnd, _T("�ɹ��������ӣ�������֤�������롣����"));
	ret = CheckPassword(pViewerNode, pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock, 2);
	if (-1 == ret) {
		SetStatusInfo(pDlg->m_hWnd, "��֤����ʱͨ�ų����޷����ӣ�");
	}
	else if (0 == ret)
	{
		SetStatusInfo(pDlg->m_hWnd, "��֤��������޷����ӣ�");
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
		_snprintf(szStatusStr, sizeof(szStatusStr), "�����Ŷӵȴ�������Ӧ��%d�롣����", i);
		SetStatusInfo(pDlg->m_hWnd, szStatusStr);
		Sleep(1000);
	}


	SetStatusInfo(pDlg->m_hWnd, "���ڵȴ��Է����������ӣ����Ժ򡣡���");

	udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_sock == INVALID_SOCKET) {
		SetStatusInfo(pDlg->m_hWnd, "����ʧ�ܣ�(udp socket failed)");
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
		SetStatusInfo(pDlg->m_hWnd, "����ʧ�ܣ�(udp bind failed)");
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
		SetStatusInfo(pDlg->m_hWnd, "����ʧ�ܣ�(udt bind failed)");
		UDT::close(serv);
		closesocket(udp_sock);
		CloseHandle(pViewerNode->hConnectThreadRev);
		pViewerNode->hConnectThreadRev = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}

	if (UDT::ERROR == UDT::listen(serv, 1))
	{
		SetStatusInfo(pDlg->m_hWnd, "����ʧ�ܣ�(udt listen failed)");
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
		SetStatusInfo(pDlg->m_hWnd, "��ʱ������״�����ã�ֱ���е�С���ѣ����Ժ����Ի��߲���TCP����");
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
		SetStatusInfo(pDlg->m_hWnd, "��ʱ������״�����ã�ֱ���е�С���ѣ����Ժ����Ի��߲���TCP����");
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


	SetStatusInfo(pDlg->m_hWnd, _T("�ɹ��������ӣ�������֤�������롣����"));
	ret = CheckPassword(pViewerNode, pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock, 2);
	if (-1 == ret) {
		SetStatusInfo(pDlg->m_hWnd, "��֤����ʱͨ�ų����޷����ӣ�");
	}
	else if (0 == ret)
	{
		SetStatusInfo(pDlg->m_hWnd, "��֤��������޷����ӣ�");
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
		
		//ÿ���в�һ����node_id
		Sleep(1100);
		generate_nodeid(viewerArray[i].httpOP.m0_node_id, sizeof(viewerArray[i].httpOP.m0_node_id));

		viewerArray[i].bQuitRecvSocketLoop = TRUE;

		viewerArray[i].m_sps_len = 0;
		viewerArray[i].m_pps_len = 0;
	}
	
	currentSourceIndex = -1;

#if FIRST_LEVEL_REPEATER
	device_topo_level = 0;
#else
	device_topo_level = 1;
#endif
	generate_nodeid(device_node_id, sizeof(device_node_id));

	g_pShiyong = this;

	OnInit();
}

CShiyong::~CShiyong()
{
	for (int i = 0; i < MAX_VIEWER_NUM; i++ )
	{
		::DeleteCriticalSection(&(viewerArray[i].localbind_csec));
	}

	g_pShiyong = NULL;
}

BOOL CShiyong::OnInit()
{
	DWORD dwThreadID;
	HANDLE hThread = ::CreateThread(NULL,0,WorkingThreadFn,(void *)this,0,&dwThreadID);
	if (hThread != NULL) {
		m_hThread = hThread;
	}
	else {
		printf("WorkingThreadFn���̴߳���ʧ�ܣ�\n");
	}


	InitVar();

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
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
	for (int i = 0; i < MAX_ROUTE_ITEM_NUM; i++)
	{
		if (device_route_table[i].bUsing == FALSE) {
			continue;
		}
		if (memcmp(device_route_table[i].node_id, dest_node_id, 6) == 0) {
			return i;
		}
	}
	return -1;
}

void CShiyong::DoExit()
{
	// TODO: �ڴ���������������

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
