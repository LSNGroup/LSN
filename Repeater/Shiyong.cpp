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
	//CShiyongDlg *pDlg = g_pMainDlg1;
	//HWND pDlgWnd = pDlg->m_hWnd;
	int ret;
	StunAddress4 mapped;
	static BOOL bNoNAT;
	static int  nNatType;
	DWORD ipArrayTemp[MAX_ADAPTER_NUM];  /* Network byte order */
	int ipCountTemp;


	ret = pViewerNode->httpOP.DoRegister1("gbk", "zh");
	TRACE("DoRegister1...ret=%d\n", ret);

	if (ret == -1) {
		pViewerNode->bNeedRegister = TRUE;
		return;
	}
	else if (ret == 0) {
		/* Exit this applicantion. */
		//::PostMessage(pDlgWnd, WM_REGISTER_EXIT, 0, 0);
		pViewerNode->bNeedRegister = TRUE;
		return;
	}
	else if (ret == 1) {
		pViewerNode->bNeedRegister = TRUE;
		return;
	}


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

	pViewerNode->bNeedRegister = FALSE;


_OUT:
	::LeaveCriticalSection(&(pViewerNode->localbind_csec));////////
	return;
}

DWORD WINAPI WorkingThreadFn(LPVOID pvThreadParam)
{
	eloop_init(NULL);

	eloop_register_timeout(2, 0, HandleKeepLoop, NULL, NULL);

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
int CheckPassword(VIEWER_NODE *pViewerNode, SOCKET_TYPE sock_type, SOCKET fhandle, ANYPC_NODE *nodeInfo, int nRetry)
{
	BYTE bTopoPrimary = 1;
	DWORD dwServerVersion;
	BYTE bFuncFlags;
	BYTE bTopoLevel;
	WORD wResult;
	char szPassEnc[MAX_PATH];


	php_md5(pViewerNode->password, szPassEnc, sizeof(szPassEnc));

	if (CtrlCmd_HELLO(sock_type, fhandle, pViewerNode->httpOP.m0_node_id, g0_version, bTopoPrimary, szPassEnc, pViewerNode->httpOP.m1_peer_node_id, &dwServerVersion, &bFuncFlags, &bTopoLevel, &wResult) == 0) {
		if (wResult == CTRLCMD_RESULT_OK) {
			//set_encdec_key((unsigned char *)szPassEnc, strlen(szPassEnc));
			nodeInfo->func_flags = bFuncFlags;
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
				if (arrServerProcesses[i].m_bPeerConnected == FALSE) {
					continue;
				}
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
	//连接是为了视频监控。。。
	//if (pDlg->m_bConnectAv == TRUE)
	{
		SetStatusInfo(pDlg->m_hWnd, _T("成功建立连接，正在接收音视频。。。"));

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


		//Loop...,需要CtrlCmd_AV_STOP()才能退出Loop！！！
		pViewerNode->bQuitRecvSocketLoop = FALSE;
		RecvSocketDataLoop(pViewerNode, pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);


		//CtrlCmd_AV_STOP()将在CShiyong::DisconnectNode()中调用
		//CtrlCmd_AV_STOP(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);
	}
	SetStatusInfo(pDlg->m_hWnd, _T("即将断开连接。。。"));
}


DWORD WINAPI ConnectThreadFn(LPVOID lpParameter)
{
	VIEWER_NODE *pViewerNode = (VIEWER_NODE *)lpParameter;
	CShiyong *pDlg = g_pShiyong;
	TCHAR szNodeIdStr[MAX_LOADSTRING];
	TCHAR szPubIpStr[16];
	TCHAR szStatusStr[MAX_LOADSTRING];
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);
	UDTSOCKET fhandle;
	DWORD use_ip;
	WORD use_port;
	int ret;
	int i, nRetry;


	//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(FALSE);
	//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(FALSE);
	//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(FALSE);


	if (	StunTypeUnknown == pViewerNode->anypcNode.nat_type ||
			StunTypeFailure == pViewerNode->anypcNode.nat_type ||
			StunTypeBlocked == pViewerNode->anypcNode.nat_type         )
	{
		SetStatusInfo(pDlg->m_hWnd, "StunType不支持，无法连接！");
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
		CloseHandle(pViewerNode->hConnectThread);
		pViewerNode->hConnectThread = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}


	SetStatusInfo(pDlg->m_hWnd, "正在提交本机连接信息，请稍候。。。");
	nRetry = 3;
	do {
		HandleRegister(pViewerNode, NULL);
	} while (nRetry-- > 0 && pViewerNode->bNeedRegister);
	if (pViewerNode->bNeedRegister) {
		SetStatusInfo(pDlg->m_hWnd, "提交连接信息失败！");
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
		CloseHandle(pViewerNode->hConnectThread);
		pViewerNode->hConnectThread = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}


	::EnterCriticalSection(&(pViewerNode->localbind_csec));////////

	strncpy(szNodeIdStr, pViewerNode->anypcNode.node_id_str, sizeof(szNodeIdStr));
	SetStatusInfo(pDlg->m_hWnd, "正在获取对方连接信息。。。");
	nRetry = 3;
	ret = -1;
	while (nRetry-- > 0 && ret == -1) {
		ret = pViewerNode->httpOP.DoConnect("gbk", "zh", szNodeIdStr);
	}
	if (ret < 1) {
		::LeaveCriticalSection(&(pViewerNode->localbind_csec));////////
		SetStatusInfo(pDlg->m_hWnd, "获取对方连接信息失败！");
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
		CloseHandle(pViewerNode->hConnectThread);
		pViewerNode->hConnectThread = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}

	if (ret == 2) {
		pViewerNode->httpOP.m1_wait_time -= 5;
		if (pViewerNode->httpOP.m1_wait_time < 0) {
			pViewerNode->httpOP.m1_wait_time = 0;
		}
	}

	//if (pViewerNode->httpOP.m1_wait_time > 30) {
	//	pDlg->ShowNotificationBox("提示：升级ID后可以快速获得服务响应，避免长时间排队等待。");
	//}

	for (i = pViewerNode->httpOP.m1_wait_time; i > 0; i--) {
		_snprintf(szStatusStr, sizeof(szStatusStr), "正在排队等待服务响应，%d秒。。。", i);
		SetStatusInfo(pDlg->m_hWnd, szStatusStr);
		Sleep(1000);
	}

	if (ret == 2) {
		::LeaveCriticalSection(&(pViewerNode->localbind_csec));////////
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
		
		DWORD dwID = 0;
		pViewerNode->hConnectThreadRev = CreateThread(NULL, 0, ConnectThreadFnRev, pViewerNode, 0, &dwID);
		WaitForSingleObject(pViewerNode->hConnectThreadRev, INFINITE);
		CloseHandle(pViewerNode->hConnectThreadRev);
		pViewerNode->hConnectThreadRev = NULL;
		
		CloseHandle(pViewerNode->hConnectThread);
		pViewerNode->hConnectThread = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}


	pViewerNode->httpOP.m1_use_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (pViewerNode->httpOP.m1_use_udp_sock == INVALID_SOCKET) {
		::LeaveCriticalSection(&(pViewerNode->localbind_csec));////////
		SetStatusInfo(pDlg->m_hWnd, "连接失败！(udp socket failed)");
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
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
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
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
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
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
	//eloop_register_timeout(3, 0, HandleDoUnregister, pViewerNode, NULL);


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
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
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
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
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
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
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
	ret = CheckPassword(pViewerNode, pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock, &(pViewerNode->anypcNode), 2);
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
	//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
	//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
	//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
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


	//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(FALSE);
	//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(FALSE);
	//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(FALSE);


	pViewerNode->httpOP.m1_use_peer_ip = inet_addr(pViewerNode->anypcNode.pub_ip_str);
	pViewerNode->httpOP.m1_use_peer_port = atol(pViewerNode->anypcNode.pub_port_str);
	//if (FALSE == pViewerNode->anypcNode.bLanNode) {
	//	pViewerNode->httpOP.m1_use_peer_port = pViewerNode->httpOP.m0_p2p_port - 2;//SECOND_CONNECT_PORT
	//}
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
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
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
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
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
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
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
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
		CloseHandle(pViewerNode->hConnectThread2);
		pViewerNode->hConnectThread2 = NULL;
		pDlg->ReturnViewerNode(pViewerNode);
		return -1;
	}


	pViewerNode->httpOP.m1_use_udt_sock = fhandle;
	pViewerNode->httpOP.m1_use_sock_type = SOCKET_TYPE_UDT;


	SetStatusInfo(pDlg->m_hWnd, _T("成功建立连接，正在验证访问密码。。。"));
	ret = CheckPassword(pViewerNode, pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock, &(pViewerNode->anypcNode), 2);
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

	//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
	//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
	//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
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


	//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(FALSE);
	//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(FALSE);
	//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(FALSE);

	SetStatusInfo(pDlg->m_hWnd, "正在等待对方发起反向连接，请稍候。。。");

	udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_sock == INVALID_SOCKET) {
		SetStatusInfo(pDlg->m_hWnd, "连接失败！(udp socket failed)");
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
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
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
		return -1;
	}


	serv = UDT::socket(AF_INET, SOCK_STREAM, 0);

	ConfigUdtSocket(serv);

	if (UDT::ERROR == UDT::bind(serv, udp_sock))
	{
		SetStatusInfo(pDlg->m_hWnd, "连接失败！(udt bind failed)");
		UDT::close(serv);
		closesocket(udp_sock);
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
		return -1;
	}

	if (UDT::ERROR == UDT::listen(serv, 1))
	{
		SetStatusInfo(pDlg->m_hWnd, "连接失败！(udt listen failed)");
		UDT::close(serv);
		closesocket(udp_sock);
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
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
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
		return -1;
	}
	//////////////////
#endif
	
	
	if ((fhandle = UDT::accept(serv, (sockaddr*)&their_addr, &namelen)) == UDT::INVALID_SOCK) {
		SetStatusInfo(pDlg->m_hWnd, "暂时性网络状况不好，直连有点小困难，请稍后重试或者采用TCP连接");
		UDT::close(serv);
		closesocket(udp_sock);
		//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
		//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
		return -1;
	}

	UDT::close(serv);

	pViewerNode->httpOP.m1_use_udp_sock = udp_sock;
	pViewerNode->httpOP.m1_use_peer_ip = their_addr.sin_addr.s_addr;
	pViewerNode->httpOP.m1_use_peer_port = ntohs(their_addr.sin_port);
	pViewerNode->httpOP.m1_use_udt_sock = fhandle;
	pViewerNode->httpOP.m1_use_sock_type = SOCKET_TYPE_UDT;


	SetStatusInfo(pDlg->m_hWnd, _T("成功建立连接，正在验证访问密码。。。"));
	ret = CheckPassword(pViewerNode, pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock, &(pViewerNode->anypcNode), 2);
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

	//pDlg->GetDlgItem(IDC_BUTTON_USE_RECORD)->EnableWindow(TRUE);
	//pDlg->GetDlgItem(IDC_LIST_REGION)->EnableWindow(TRUE);
	//pDlg->GetDlgItem(IDC_LIST_MAIN)->EnableWindow(TRUE);
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

	g0_audio_channels = 0;
	g0_video_channels = 0;
	g1_register_period = DEFAULT_REGISTER_PERIOD;  /* Seconds */
	g1_expire_period = DEFAULT_EXPIRE_PERIOD;  /* Seconds */

	g1_lowest_level_for_av = 0;
	g1_lowest_level_for_vnc = 0;
	g1_lowest_level_for_ft = 0;
	g1_lowest_level_for_adb = 0;
	g1_lowest_level_for_webmoni = 0;
	g1_lowest_level_allow_hide = 0;
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
		viewerArray[i].bNeedRegister = TRUE;
		::InitializeCriticalSection(&(viewerArray[i].localbind_csec));
		
		viewerArray[i].httpOP.m0_is_admin = TRUE;
		viewerArray[i].httpOP.m0_is_busy = FALSE;
		viewerArray[i].httpOP.m0_p2p_port = FIRST_CONNECT_PORT - (i+1)*4;
		
		viewerArray[i].bQuitRecvSocketLoop = TRUE;

		viewerArray[i].m_sps_len = 0;
		viewerArray[i].m_pps_len = 0;
	}
	
	currentSourceIndex = -1;

	device_topo_level = 1;
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
		printf("WorkingThreadFn子线程创建失败！\n");
	}


	InitVar();

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CShiyong::ConnectNode(char *anypc_node_id, char *password)
{
	int i;
	//{{{{--------------------------------------->
	for (i = 0; i < MAX_VIEWER_NUM; i++ ) {
		if (viewerArray[i].bUsing == FALSE) {
			break;
		}
	}

	if (i == MAX_VIEWER_NUM) { // no free node
		printf("对不起，没有空闲的VIEWER_NODE!\n");
		return;
	}

	viewerArray[i].bUsing = TRUE;
	viewerArray[i].nID = i;
	//<---------------------------------------}}}}



	ANYPC_NODE anypcNode;
	int cntServer = 1;
	int nNum;
	int ret = viewerArray[i].httpOP.DoQueryList("gbk", "zh", anypc_node_id, &anypcNode, &cntServer, &nNum);
	if (ret == -1 || cntServer != 1) {
		printf("无法获取该受控端的P2P连接信息!\n");
		//{{{{--------------------------------------->
		viewerArray[i].bUsing = FALSE;
		viewerArray[i].nID = -1;
		//<---------------------------------------}}}}
		return;
	}
	anypcNode.bLanNode = FALSE;


	if (strcmp(anypcNode.location, "") == 0) {
		printf("该受控端的P2P连接信息无效!\n");
		//{{{{--------------------------------------->
		viewerArray[i].bUsing = FALSE;
		viewerArray[i].nID = -1;
		//<---------------------------------------}}}}
		return;
	}


	//{{{{--------------------------------------->
	viewerArray[i].hConnectThread = NULL;
	viewerArray[i].hConnectThread2 = NULL;
	viewerArray[i].hConnectThreadRev = NULL;
	viewerArray[i].bFirstCheckStun = TRUE;
	viewerArray[i].bNeedRegister = TRUE;
	viewerArray[i].anypcNode = anypcNode;
	viewerArray[i].bQuitRecvSocketLoop = TRUE;

	viewerArray[i].m_sps_len = 0;
	viewerArray[i].m_pps_len = 0;

	strncpy(viewerArray[i].password, password, sizeof(viewerArray[i].password));
	//<---------------------------------------}}}}


	//每次有不一样的node_id
	generate_nodeid(viewerArray[i].httpOP.m0_node_id, sizeof(viewerArray[i].httpOP.m0_node_id));

	mac_addr(viewerArray[i].anypcNode.node_id_str, viewerArray[i].httpOP.m1_peer_node_id, NULL);


	DWORD dwID = 0;
	if ((FALSE == viewerArray[i].anypcNode.bLanNode) 
		&& (FALSE == viewerArray[i].anypcNode.no_nat))
	{
		viewerArray[i].hConnectThread = CreateThread(NULL, 0, ConnectThreadFn, (void *)(&(viewerArray[i])), 0, &dwID);
	}
	else {
		viewerArray[i].hConnectThread2 = CreateThread(NULL, 0, ConnectThreadFn2, (void *)(&(viewerArray[i])), 0, &dwID);
	}
}

void CShiyong::DisconnectNode(VIEWER_NODE *pViewerNode)
{
	if (pViewerNode->bUsing == FALSE || pViewerNode->bQuitRecvSocketLoop == TRUE) {
		return;
	}
	CtrlCmd_AV_STOP(pViewerNode->httpOP.m1_use_sock_type, pViewerNode->httpOP.m1_use_udt_sock);
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
