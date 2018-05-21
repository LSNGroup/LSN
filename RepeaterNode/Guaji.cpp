// Guaji.cpp : 实现文件
//

#include "stdafx.h"
#include "RepeaterNode.h"
#include "Guaji.h"

#include "platform.h"
#include "CommonLib.h"
#include "HttpOperate.h"
#include "phpMd5.h"
#include "base64.h"
#include "LogMsg.h"



MyUPnP  myUPnP;

SOCKET g_fhandle = INVALID_SOCKET;

SERVER_NODE *g_pServerNode = NULL;

char SERVER_TYPE[16];
int UUID_EXT = 0;
int MAX_SERVER_NUM = 0;
char NODE_NAME[MAX_PATH];
char CONNECT_PASSWORD[MAX_PATH];
int  P2P_PORT = FIRST_CONNECT_PORT;

int  IPC_BIND_PORT = 20000;

BYTE g_device_topo_level = 1;
BYTE g_peer_node_id[6];//下级的Viewer Node id
BOOL g_is_topo_primary = FALSE;//是否下级的主通道

int g_video_width  = 0;
int g_video_height = 0;
int g_video_fps = 0;

char g_tcp_address[MAX_PATH];

static void InitVar();
int ControlChannelLoop(SERVER_NODE* pServerNode, SOCKET_TYPE type, SOCKET fhandle);

#ifdef WIN32
DWORD WINAPI WinMainThreadFn(LPVOID pvThreadParam);
DWORD WINAPI CheckingThreadFn(LPVOID pvThreadParam);
DWORD WINAPI UnregisterThreadFn(LPVOID pvThreadParam);
DWORD WINAPI WorkingThreadFn1(LPVOID pvThreadParam);
DWORD WINAPI WorkingThreadFnRev(LPVOID pvThreadParam);
DWORD WINAPI WorkingThreadFn2(LPVOID pvThreadParam);
#else
void *WinMainThreadFn(void *pvThreadParam);
void *CheckingThreadFn(void *pvThreadParam);
void *UnregisterThreadFn(void *pvThreadParam);
void *WorkingThreadFn1(void *pvThreadParam);
void *WorkingThreadFnRev(void *pvThreadParam);
void *WorkingThreadFn2(void *pvThreadParam);
#endif


void StartAnyPC()
{
	g_pServerNode = (SERVER_NODE *)malloc(sizeof(SERVER_NODE));

	{
		g_pServerNode->m_bExit = FALSE;

		g_pServerNode->myHttpOperate.m0_is_admin = FALSE;
		g_pServerNode->myHttpOperate.m0_is_busy = FALSE;
		g_pServerNode->myHttpOperate.m0_p2p_port = P2P_PORT;

		//每个HttpOperate实例有不一样的node_id
		generate_nodeid(g_pServerNode->myHttpOperate.m0_node_id, sizeof(g_pServerNode->myHttpOperate.m0_node_id));

		g_pServerNode->m_bConnected = FALSE;

		g_pServerNode->m_pFRS = NULL;
		g_pServerNode->m_bAVStarted = FALSE;
		g_pServerNode->m_bH264FormatSent = FALSE;

		g_pServerNode->m_bFirstCheckStun = TRUE;
		g_pServerNode->m_bOnlineSuccess = FALSE;
		g_pServerNode->m_bDoConnection1 = TRUE;
		g_pServerNode->m_bDoConnection2 = TRUE;
		g_pServerNode->m_InConnection1 = FALSE;
		g_pServerNode->m_InConnection2 = FALSE;


		BOOL bMappingExists = FALSE;
		g_pServerNode->mapping.description = "";
		g_pServerNode->mapping.protocol = UNAT_UDP;
		g_pServerNode->mapping.externalPort = ((myUPnP.GetLocalIP() & 0xff000000) >> 24) | ((2049 + (65535 - 2049) * (WORD)rand() / (65536)) & 0xffffff00);
		while (UNAT_OK == myUPnP.GetNATSpecificEntry(&(g_pServerNode->mapping), &bMappingExists) && bMappingExists)
		{
			log_msg_f(LOG_LEVEL_INFO, "Find NATPortMapping(%d, rand=%d), retry...\n", g_pServerNode->mapping.externalPort, rand());

			bMappingExists = FALSE;
			g_pServerNode->mapping.description = "";
			g_pServerNode->mapping.protocol = UNAT_UDP;
			g_pServerNode->mapping.externalPort = ((myUPnP.GetLocalIP() & 0xff000000) >> 24) | ((2049 + (65535 - 2049) * (WORD)rand() / (65536)) & 0xffffff00);
		}//找到一个未被占用的外部端口映射，或者路由器UPnP功能不可用


		InitVar();

#ifdef WIN32
		DWORD dwThreadID;
		HANDLE hThread = ::CreateThread(NULL,0,WinMainThreadFn,g_pServerNode,0,&dwThreadID);
		if (hThread == NULL)
#else
		pthread_t hThread;
		int err = pthread_create(&hThread, NULL, WinMainThreadFn, g_pServerNode);
		if (0 != err)
#endif
		{
			log_msg("Create WinMainThreadFn failed!\n", LOG_LEVEL_ERROR);/* Error */
		}
	}
}

void StartDoConnection(SERVER_NODE* pServerNode)
{
	pServerNode->m_bExit = FALSE;

	pServerNode->m_bConnected = FALSE;

	pServerNode->m_bDoConnection1 = TRUE;
	pServerNode->m_bDoConnection2 = TRUE;
}

void StopDoConnection(SERVER_NODE* pServerNode)
{
	if (pServerNode->m_bDoConnection1) {
		log_msg("DoUnregister()...\n", LOG_LEVEL_INFO);
		pServerNode->m_bExit = TRUE;
#ifdef WIN32
		DWORD dwThreadID;
		::CreateThread(NULL,0,UnregisterThreadFn,(void *)pServerNode,0,&dwThreadID);
#else
		pthread_t hTempThread;
		pthread_create(&hTempThread, NULL, UnregisterThreadFn, (void *)pServerNode);
#endif
	}
	pServerNode->m_bDoConnection1 = FALSE;
	pServerNode->m_bDoConnection2 = FALSE;

	DShowAV_Stop(pServerNode);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//
//  0: OK
// -1: NG
int if_get_device_uuid(char *buff, int size)
{
	BOOL bRetry = FALSE;
	DWORD dwResult = NO_ERROR;
	DWORD dwRet;
	ULONG ulOutBufLen = 0;
	IP_ADAPTER_INFO *pAdapterInfo = NULL, *pLoopAdapter = NULL;

////////
	TCHAR szValueData[_MAX_PATH];
	DWORD dwDataLen = _MAX_PATH;
	if (GetSoftwareKeyValue(STRING_REGKEY_NAME_SAVED_UUID,(LPBYTE)szValueData,&dwDataLen) && strlen(szValueData) > 0)
	{
		strncpy(buff, szValueData, size);
		return 0;
	}
////////

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

//
//  0: OK
// -1: NG
int if_read_password(char *buff, int size)//orig pass
{
	strcpy(buff, CONNECT_PASSWORD);
	return 0;
}

//
//  0: OK
// -1: NG
int if_read_nodename(char *buff, int size)
{
	strncpy(buff, NODE_NAME, size);

	char *p;
	p = strchr(buff, '.');
	while (p != NULL)
	{
		*p = ' ';
		p = strchr(buff, '.');
	}
	p = strchr(buff, '-');
	while (p != NULL)
	{
		*p = ' ';
		p = strchr(buff, '-');
	}
	p = strchr(buff, '_');
	while (p != NULL)
	{
		*p = ' ';
		p = strchr(buff, '_');
	}
	p = strchr(buff, '+');
	while (p != NULL)
	{
		*p = ' ';
		p = strchr(buff, '+');
	}
	p = strchr(buff, '&');
	while (p != NULL)
	{
		*p = ' ';
		p = strchr(buff, '&');
	}

	return 0;
}

//
//  0: OK
// -1: NG
int if_read_emailaddr(char *buff, int size)
{
	strcpy(buff, "repeater@lsnet.io");
	return 0;
}

//
//  0: OK
// -1: NG
int if_get_osinfo(char *buff, int size)
{
	GetOsInfo(buff, size);

	char *p;
	p = strchr(buff, '.');
	while (p != NULL)
	{
		*p = ' ';
		p = strchr(buff, '.');
	}
	p = strchr(buff, '-');
	while (p != NULL)
	{
		*p = ' ';
		p = strchr(buff, '-');
	}
	p = strchr(buff, '_');
	while (p != NULL)
	{
		*p = ' ';
		p = strchr(buff, '_');
	}
	p = strchr(buff, '+');
	while (p != NULL)
	{
		*p = ' ';
		p = strchr(buff, '+');
	}
	p = strchr(buff, '&');
	while (p != NULL)
	{
		*p = ' ';
		p = strchr(buff, '&');
	}

	return 0;
}

void if_on_client_connected(SERVER_NODE* pServerNode)
{
	pServerNode->m_bConnected = TRUE;

	if (INVALID_SOCKET != g_fhandle)
	{
		CtrlCmd_ARM(SOCKET_TYPE_TCP, g_fhandle);//借用
	}
}

void if_on_client_disconnected(SERVER_NODE* pServerNode)
{
	pServerNode->m_bConnected = FALSE;

	if (INVALID_SOCKET != g_fhandle)
	{
		CtrlCmd_DISARM(SOCKET_TYPE_TCP, g_fhandle);//借用
	}
}

void if_mc_arm()
{
//被借用
}

void if_mc_disarm()
{
//被借用
}

void if_contrl_system_shutdown()
{
	log_msg("shutdown -s -t 10 -c ...\n", LOG_LEVEL_ERROR);
}

void if_contrl_system_reboot()
{
	log_msg("shutdown -r -t 10 -c ...\n", LOG_LEVEL_ERROR);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void GetLocalInformation(HttpOperate *httpOP)
{
	DWORD ipArrayTemp[MAX_ADAPTER_NUM];  /* Network byte order */
	int ipCountTemp;
	char msg[MAX_LOADSTRING];

	/* Local IP Address */
	ipCountTemp = MAX_ADAPTER_NUM;
	if (GetLocalAddress(ipArrayTemp, &ipCountTemp) != NO_ERROR) {
		log_msg("GetLocalAddress() failed!", LOG_LEVEL_ERROR);
	}
	else {
		httpOP->m0_ipCount = ipCountTemp;
		memcpy(httpOP->m0_ipArray, ipArrayTemp, sizeof(DWORD)*ipCountTemp);
		httpOP->m0_port = httpOP->m0_p2p_port;
		sprintf(msg, "GetLocalAddress: ipCount=%d", httpOP->m0_ipCount);
		log_msg(msg, LOG_LEVEL_DEBUG);
	}

	
	/* Node Name */
	if_read_nodename(g0_node_name, sizeof(g0_node_name));
	
	
	/* OS Info */
	if_get_osinfo(g0_os_info, sizeof(g0_os_info));
}

/* node_id, node_name, version, os_info, http_server, stun_server, udt_port, register_period, expire_period */
static void InitVar()
{
	int ret;

	ret = if_get_device_uuid(g0_device_uuid, sizeof(g0_device_uuid));
	log_msg_f(LOG_LEVEL_DEBUG, "InitVar: if_get_device_uuid() = %s", g0_device_uuid);

	//GetOsInfo(g0_os_info, sizeof(g0_os_info));

	g0_version = MYSELF_VERSION;


	strncpy(g1_http_server, DEFAULT_HTTP_SERVER, sizeof(g1_http_server));
	strncpy(g1_stun_server, DEFAULT_STUN_SERVER, sizeof(g1_stun_server));


	g1_register_period = DEFAULT_REGISTER_PERIOD;  /* Seconds */
	g1_expire_period = DEFAULT_EXPIRE_PERIOD;  /* Seconds */
	
	
	g1_is_activated = true;
	g1_comments_id = 0;
}

#ifdef WIN32
DWORD WINAPI WinMainThreadFn(LPVOID pvThreadParam)
#else
void *WinMainThreadFn(void *pvThreadParam)
#endif
{
	SERVER_NODE* pServerNode = (SERVER_NODE*)pvThreadParam;
	int ret;
	int last_register_time = 0, now_time;
	StunAddress4 mapped;
	BOOL bNoNAT;
	int  nNatType;
	DWORD dwThreadID;
	pthread_t hThread;
	DWORD dwThreadID2;
	pthread_t hThread2;
	char msg[MAX_LOADSTRING];


	SaveSoftwareKeyDwordValue(STRING_REGKEY_NAME_VERSION, (DWORD)MYSELF_VERSION);
	SaveSoftwareKeyDwordValue(STRING_REGKEY_NAME_STOPFLAG, (DWORD)0);

	DWORD dwForceNoNAT = 0;
	if (FALSE == GetSoftwareKeyDwordValue(STRING_REGKEY_NAME_FORCE_NONAT, &dwForceNoNAT)) {
		SaveSoftwareKeyDwordValue(STRING_REGKEY_NAME_FORCE_NONAT, (DWORD)0);
	}


	hThread = ::CreateThread(NULL,0,CheckingThreadFn,(void *)pServerNode,0,&dwThreadID);
	if (hThread == NULL) {
		log_msg("Create CheckingThread failed!", LOG_LEVEL_ERROR);/* Error */
	}


#ifdef WIN32
	hThread2 = ::CreateThread(NULL,0,WorkingThreadFn2,(void *)pServerNode,0,&dwThreadID2);
	if (hThread2 == NULL)
#else
	int err = pthread_create(&hThread2, NULL, WorkingThreadFn2, (void *)pServerNode);
	if (0 != err)
#endif
	{
		log_msg("Create WorkingThread2 failed!", LOG_LEVEL_ERROR);
	}


	while (1)
	{
		now_time = time(NULL);
		if (now_time - last_register_time < g1_register_period) {
			usleep((g1_register_period - (now_time - last_register_time)) * 1000 * 1000);
		}
		last_register_time = time(NULL);

		if (pServerNode->m_InConnection2) {
			continue;
		}

		if (strcmp(g1_stun_server, DEFAULT_STUN_SERVER) == 0) {//等待IPC通道收到Settings
			continue;
		}

		GetLocalInformation(&(pServerNode->myHttpOperate));


		if (pServerNode->m_bDoConnection1)
		{
			/* STUN Information */
			if (strcmp(g1_stun_server, "NONE") == 0)
			{
				ret = CheckStunMyself(g1_http_server, pServerNode->myHttpOperate.m0_p2p_port, &mapped, &bNoNAT, &nNatType, &(pServerNode->myHttpOperate.m0_net_time));
				if (ret == -1) {
				   log_msg("CheckStunMyself() failed!", LOG_LEVEL_ERROR);
				   
				   mapped.addr = ntohl(pServerNode->myHttpOperate.m1_http_client_ip);
				   mapped.port = pServerNode->myHttpOperate.m0_p2p_port;
				   
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
					dwForceNoNAT = 0;
					if (GetSoftwareKeyDwordValue(STRING_REGKEY_NAME_FORCE_NONAT, &dwForceNoNAT) && dwForceNoNAT == 1) {
						  bNoNAT = TRUE;
						  nNatType = StunTypeOpen;
					}

				   
					pServerNode->myHttpOperate.m0_pub_ip = htonl(mapped.addr);
					pServerNode->myHttpOperate.m0_pub_port = mapped.port;
					pServerNode->myHttpOperate.m0_no_nat = bNoNAT;
					pServerNode->myHttpOperate.m0_nat_type = nNatType;
					sprintf(msg, "CheckStunHttp: %d.%d.%d.%d[%d], NoNAT=%d\n", 
						(pServerNode->myHttpOperate.m0_pub_ip & 0x000000ff) >> 0,
						(pServerNode->myHttpOperate.m0_pub_ip & 0x0000ff00) >> 8,
						(pServerNode->myHttpOperate.m0_pub_ip & 0x00ff0000) >> 16,
						(pServerNode->myHttpOperate.m0_pub_ip & 0xff000000) >> 24,
						pServerNode->myHttpOperate.m0_pub_port,  pServerNode->myHttpOperate.m0_no_nat ? 1 : 0);
					log_msg(msg, LOG_LEVEL_DEBUG);
				}
				else {
					pServerNode->myHttpOperate.m0_pub_ip = htonl(mapped.addr);
					pServerNode->myHttpOperate.m0_pub_port = mapped.port;
					pServerNode->myHttpOperate.m0_no_nat = bNoNAT;
					pServerNode->myHttpOperate.m0_nat_type = nNatType;
					sprintf(msg, "CheckStunMyself: %d.%d.%d.%d[%d] NoNAT=%d\n", 
						(pServerNode->myHttpOperate.m0_pub_ip & 0x000000ff) >> 0,
						(pServerNode->myHttpOperate.m0_pub_ip & 0x0000ff00) >> 8,
						(pServerNode->myHttpOperate.m0_pub_ip & 0x00ff0000) >> 16,
						(pServerNode->myHttpOperate.m0_pub_ip & 0xff000000) >> 24,
						pServerNode->myHttpOperate.m0_pub_port,  pServerNode->myHttpOperate.m0_no_nat ? 1 : 0);
					log_msg(msg, LOG_LEVEL_DEBUG);
				}
			}
			else if (pServerNode->m_bFirstCheckStun) {
				ret = CheckStun(g1_stun_server, pServerNode->myHttpOperate.m0_p2p_port, &mapped, &bNoNAT, &nNatType);
				if (ret == -1) {
				   log_msg("CheckStun() failed!", LOG_LEVEL_ERROR);
				   
				   mapped.addr = ntohl(pServerNode->myHttpOperate.m1_http_client_ip);
				   mapped.port = pServerNode->myHttpOperate.m0_p2p_port;
				   
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
					dwForceNoNAT = 0;
					if (GetSoftwareKeyDwordValue(STRING_REGKEY_NAME_FORCE_NONAT, &dwForceNoNAT) && dwForceNoNAT == 1) {
						  bNoNAT = TRUE;
						  nNatType = StunTypeOpen;
					}

				   
					pServerNode->myHttpOperate.m0_pub_ip = htonl(mapped.addr);
					pServerNode->myHttpOperate.m0_pub_port = mapped.port;
					pServerNode->myHttpOperate.m0_no_nat = bNoNAT;
					pServerNode->myHttpOperate.m0_nat_type = nNatType;
					sprintf(msg, "CheckStunHttp: %d.%d.%d.%d[%d], NoNAT=%d\n", 
						(pServerNode->myHttpOperate.m0_pub_ip & 0x000000ff) >> 0,
						(pServerNode->myHttpOperate.m0_pub_ip & 0x0000ff00) >> 8,
						(pServerNode->myHttpOperate.m0_pub_ip & 0x00ff0000) >> 16,
						(pServerNode->myHttpOperate.m0_pub_ip & 0xff000000) >> 24,
						pServerNode->myHttpOperate.m0_pub_port,  pServerNode->myHttpOperate.m0_no_nat ? 1 : 0);
					log_msg(msg, LOG_LEVEL_DEBUG);
				}
				else if (ret == 0) {
					log_msg("CheckStun() blocked!", LOG_LEVEL_WARNING);
					pServerNode->myHttpOperate.m0_pub_ip = 0;
					pServerNode->myHttpOperate.m0_pub_port = 0;
					pServerNode->myHttpOperate.m0_no_nat = FALSE;
					pServerNode->myHttpOperate.m0_nat_type = nNatType;
				}
				else {
					pServerNode->m_bFirstCheckStun = FALSE;
					pServerNode->myHttpOperate.m0_pub_ip = htonl(mapped.addr);
					pServerNode->myHttpOperate.m0_pub_port = mapped.port;
					pServerNode->myHttpOperate.m0_no_nat = bNoNAT;
					pServerNode->myHttpOperate.m0_nat_type = nNatType;
					sprintf(msg, "CheckStun: %d.%d.%d.%d[%d] NoNAT=%d\n", 
						(pServerNode->myHttpOperate.m0_pub_ip & 0x000000ff) >> 0,
						(pServerNode->myHttpOperate.m0_pub_ip & 0x0000ff00) >> 8,
						(pServerNode->myHttpOperate.m0_pub_ip & 0x00ff0000) >> 16,
						(pServerNode->myHttpOperate.m0_pub_ip & 0xff000000) >> 24,
						pServerNode->myHttpOperate.m0_pub_port,  pServerNode->myHttpOperate.m0_no_nat ? 1 : 0);
					log_msg(msg, LOG_LEVEL_DEBUG);
				}
			}
			else {
				ret = CheckStunSimple(g1_stun_server, pServerNode->myHttpOperate.m0_p2p_port, &mapped);
				if (ret == -1) {
					log_msg("CheckStunSimple() failed!", LOG_LEVEL_ERROR);
					mapped.addr = ntohl(pServerNode->myHttpOperate.m1_http_client_ip);
					mapped.port = pServerNode->myHttpOperate.m0_p2p_port;
					pServerNode->myHttpOperate.m0_pub_ip = htonl(mapped.addr);
					pServerNode->myHttpOperate.m0_pub_port = mapped.port;
					pServerNode->m_bFirstCheckStun = TRUE;
				}
				else {
					pServerNode->myHttpOperate.m0_pub_ip = htonl(mapped.addr);
					pServerNode->myHttpOperate.m0_pub_port = mapped.port;
					sprintf(msg, "CheckStunSimple: %d.%d.%d.%d[%d]", 
						(pServerNode->myHttpOperate.m0_pub_ip & 0x000000ff) >> 0,
						(pServerNode->myHttpOperate.m0_pub_ip & 0x0000ff00) >> 8,
						(pServerNode->myHttpOperate.m0_pub_ip & 0x00ff0000) >> 16,
						(pServerNode->myHttpOperate.m0_pub_ip & 0xff000000) >> 24,
						pServerNode->myHttpOperate.m0_pub_port);
					log_msg(msg, LOG_LEVEL_DEBUG);
				}
			}
			
			
			/* 本地路由器UPnP处理*/
			if (FALSE == bNoNAT)
			{
				std::string extIp = "";
				myUPnP.GetNATExternalIp(extIp);
				if (false == extIp.empty() && 0 != pServerNode->myHttpOperate.m0_pub_ip && inet_addr(extIp.c_str()) == pServerNode->myHttpOperate.m0_pub_ip)
				{
					g_pServerNode->mapping.description = "UP2P";
					//g_pServerNode->mapping.protocol = ;
					//g_pServerNode->mapping.externalPort = ;
					g_pServerNode->mapping.internalPort = pServerNode->myHttpOperate.m0_p2p_port - 2;//SECOND_CONNECT_PORT;
					if (UNAT_OK == myUPnP.AddNATPortMapping(&(g_pServerNode->mapping), false))
					{
						pServerNode->m_bFirstCheckStun = FALSE;

						pServerNode->myHttpOperate.m0_pub_ip = inet_addr(extIp.c_str());
						pServerNode->myHttpOperate.m0_pub_port = g_pServerNode->mapping.externalPort;
						pServerNode->myHttpOperate.m0_no_nat = TRUE;
						pServerNode->myHttpOperate.m0_nat_type = StunTypeOpen;

						log_msg_f(LOG_LEVEL_INFO, "UPnP AddPortMapping OK: %s[%d] --> %s[%d]", extIp.c_str(), g_pServerNode->mapping.externalPort, myUPnP.GetLocalIPStr().c_str(), g_pServerNode->mapping.internalPort);
					}
					else
					{
						BOOL bTempExists = FALSE;
						myUPnP.GetNATSpecificEntry(&(g_pServerNode->mapping), &bTempExists);
						if (bTempExists)
						{
							pServerNode->m_bFirstCheckStun = FALSE;

							pServerNode->myHttpOperate.m0_pub_ip = inet_addr(extIp.c_str());
							pServerNode->myHttpOperate.m0_pub_port = g_pServerNode->mapping.externalPort;
							pServerNode->myHttpOperate.m0_no_nat = TRUE;
							pServerNode->myHttpOperate.m0_nat_type = StunTypeOpen;

							log_msg_f(LOG_LEVEL_INFO, "UPnP PortMapping Exists: %s[%d] --> %s[%d]", extIp.c_str(), g_pServerNode->mapping.externalPort, myUPnP.GetLocalIPStr().c_str(), g_pServerNode->mapping.internalPort);
						}
						else {
							pServerNode->myHttpOperate.m0_pub_ip = htonl(mapped.addr);
							pServerNode->myHttpOperate.m0_pub_port = mapped.port;
							pServerNode->myHttpOperate.m0_no_nat = bNoNAT;
							pServerNode->myHttpOperate.m0_nat_type = nNatType;
						}
					}
				}
				else {
					log_msg_f(LOG_LEVEL_INFO, "UPnP PortMapping Skipped: %s --> %s", extIp.c_str(), myUPnP.GetLocalIPStr().c_str());
					pServerNode->myHttpOperate.m0_pub_ip = htonl(mapped.addr);
					pServerNode->myHttpOperate.m0_pub_port = mapped.port;
					pServerNode->myHttpOperate.m0_no_nat = bNoNAT;
					pServerNode->myHttpOperate.m0_nat_type = nNatType;
				}
			}


			//修正公网IP受控端的连接端口问题,2014-6-15
			if (bNoNAT) {
				pServerNode->myHttpOperate.m0_pub_port = pServerNode->myHttpOperate.m0_p2p_port - 2;//SECOND_CONNECT_PORT;
			}

			if (pServerNode->myHttpOperate.m0_no_nat) {
				pServerNode->myHttpOperate.m0_port = SECOND_CONNECT_PORT;//
			}

			
			char szIpcReport[512];
			snprintf(szIpcReport, sizeof(szIpcReport), 
				"%02X-%02X-%02X-%02X-%02X-%02X"//"node_id=%02X-%02X-%02X-%02X-%02X-%02X"
				"|%02X-%02X-%02X-%02X-%02X-%02X"//"&peer_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
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
				pServerNode->myHttpOperate.m0_node_id[0], pServerNode->myHttpOperate.m0_node_id[1], pServerNode->myHttpOperate.m0_node_id[2], pServerNode->myHttpOperate.m0_node_id[3], pServerNode->myHttpOperate.m0_node_id[4], pServerNode->myHttpOperate.m0_node_id[5], 
				g_peer_node_id[0], g_peer_node_id[1], g_peer_node_id[2], g_peer_node_id[3], g_peer_node_id[4], g_peer_node_id[5],
				(pServerNode->m_bConnected ? 1 : 0),
				(pServerNode->m_bAVStarted ? 1 : 0),
				pServerNode->myHttpOperate.MakeIpStr(), pServerNode->myHttpOperate.m0_port, pServerNode->myHttpOperate.MakePubIpStr(), pServerNode->myHttpOperate.m0_pub_port, 
				(pServerNode->myHttpOperate.m0_no_nat ? 1 : 0),
				pServerNode->myHttpOperate.m0_nat_type,
				g0_device_uuid, UrlEncode(g0_node_name).c_str(), g0_version, g0_os_info);

			CtrlCmd_IPC_REPORT(SOCKET_TYPE_TCP, g_fhandle, pServerNode->myHttpOperate.m0_node_id, szIpcReport);

			ret = 3;

			//ret = pServerNode->myHttpOperate.DoRegister2("gbk", "zh");
			//sprintf(msg, "DoRegister2() = %d, comments_id=%ld", ret, g1_comments_id);
			//log_msg(msg, LOG_LEVEL_DEBUG);

			/* 如果DoRegister2()返回正常，则执行一次DoOnline() */
			/*
			if (ret != -1 && ret != 0 && ret != 1 && g1_comments_id > 0) {
				if (FALSE == pServerNode->m_bOnlineSuccess) {
					DoOnline();
					pServerNode->m_bOnlineSuccess = TRUE;
				}
				if_on_register_result(g1_comments_id, g1_is_activated);
			}*/
		}
		else {
			ret = 0;
		}

		if (ret == -1) {
			//if (g_bDoConnection1) OnDailerEvent();//宽带拨号，g_bDoConnection1==TRUE
			pServerNode->m_bFirstCheckStun = TRUE;//可能已切换网络
			continue;
		}
		else if (ret == 0) {
			/* Exit this connection mode. */
			pServerNode->m_bDoConnection1 = FALSE;
			continue;
		}


		/* Connection */
		if (ret == 3) {

			if (stricmp(pServerNode->myHttpOperate.m1_event_type, "Accept") == 0)  /* Accept */
			{
				strcpy(pServerNode->myHttpOperate.m1_event_type, "");

				if (pServerNode->myHttpOperate.m0_no_nat == FALSE)
				{
					pServerNode->m_InConnection1 = TRUE;
#ifdef WIN32
					hThread = ::CreateThread(NULL,0,WorkingThreadFn1,(void *)pServerNode,0,&dwThreadID);
					if (hThread == NULL)
#else
					int err = pthread_create(&hThread, NULL, WorkingThreadFn1, (void *)pServerNode);
					if (0 != err)
#endif
					{
						pServerNode->m_InConnection1 = FALSE;
						log_msg("Create WorkingThreadFn1 failed!", LOG_LEVEL_ERROR);
						continue;
					}

					/* Wait... */
#ifdef WIN32
					::WaitForSingleObject(hThread, INFINITE);
					::CloseHandle(hThread);
#else
					pthread_join(hThread, NULL);
#endif
					log_msg("WorkingThread ends, continue to do Register loop...", LOG_LEVEL_INFO);

					pServerNode->m_InConnection1 = FALSE;
				}
			}
			else if (stricmp(pServerNode->myHttpOperate.m1_event_type, "AcceptRev") == 0)  /* AcceptRev */
			{
				strcpy(pServerNode->myHttpOperate.m1_event_type, "");

				pServerNode->m_InConnection1 = TRUE;
#ifdef WIN32
				hThread = ::CreateThread(NULL,0,WorkingThreadFnRev,(void *)pServerNode,0,&dwThreadID);
				if (hThread == NULL)
#else
				int err = pthread_create(&hThread, NULL, WorkingThreadFnRev, (void *)pServerNode);
				if (0 != err)
#endif
				{
					pServerNode->m_InConnection1 = FALSE;
					log_msg("Create WorkingThreadFnRev failed!", LOG_LEVEL_ERROR);
					continue;
				}

				/* Wait... */
#ifdef WIN32
				::WaitForSingleObject(hThread, INFINITE);
				::CloseHandle(hThread);
#else
				pthread_join(hThread, NULL);
#endif
				log_msg("WorkingThread ends, continue to do Register loop...", LOG_LEVEL_INFO);

				pServerNode->m_InConnection1 = FALSE;
			}
			else if (stricmp(pServerNode->myHttpOperate.m1_event_type, "AcceptProxy") == 0)  /* AcceptProxy */
			{
				strcpy(pServerNode->myHttpOperate.m1_event_type, "");

				log_msg("AcceptProxy, ################...\n", LOG_LEVEL_INFO);
			}
			else if (stricmp(pServerNode->myHttpOperate.m1_event_type, "AcceptProxyTcp") == 0) /* AcceptProxyTcp */
			{
				strcpy(pServerNode->myHttpOperate.m1_event_type, "");

				log_msg("AcceptProxyTcp, ################...\n", LOG_LEVEL_INFO);
			}

			/* 退出连接线程后，重新Register到Http服务器上。*/
		}

	}  /* while (1) */

	if (pServerNode->m_bDoConnection1) {
		log_msg("DoUnregister()...", LOG_LEVEL_INFO);
		pServerNode->m_bExit = TRUE;
#ifdef WIN32
		::CreateThread(NULL,0,UnregisterThreadFn,(void *)pServerNode,0,&dwThreadID);
#else
		pthread_t hTempThread;
		pthread_create(&hTempThread, NULL, UnregisterThreadFn, (void *)pServerNode);
#endif
	}

	// 设置退出标志
	SaveSoftwareKeyDwordValue(STRING_REGKEY_NAME_STOPFLAG, (DWORD)1);
	//log_msg("Stop "UVNC_SERVICE_NAME"...", LOG_LEVEL_INFO);
	//RunExeNoWait("net stop "UVNC_SERVICE_NAME, FALSE);

	log_msg("WinMain: Exit this application!\n", LOG_LEVEL_WARNING);
	return 0;
}

#ifdef WIN32
DWORD WINAPI CheckingThreadFn(LPVOID pvThreadParam)
#else
void *CheckingThreadFn(void *pvThreadParam)
#endif
{
	SERVER_NODE* pServerNode = (SERVER_NODE*)pvThreadParam;
	DWORD dwStopFlag;

	while (1)
	{
		if (GetSoftwareKeyDwordValue(STRING_REGKEY_NAME_STOPFLAG, &dwStopFlag)) {
			if (dwStopFlag == 1)
			{
				Sleep(800);

				if (pServerNode->m_bDoConnection1) {
					log_msg("DoUnregister()...", LOG_LEVEL_INFO);
					pServerNode->m_bExit = TRUE;
#ifdef WIN32
					DWORD dwThreadID;
					::CreateThread(NULL,0,UnregisterThreadFn,(void *)pServerNode,0,&dwThreadID);
#else
					pthread_t hTempThread;
					pthread_create(&hTempThread, NULL, UnregisterThreadFn, (void *)pServerNode);
#endif
				}

				myUPnP.RemoveNATPortMapping(pServerNode->mapping);


				log_msg("CheckingThread: Exit this application!", LOG_LEVEL_WARNING);
				ExitProcess(0);
			}
		}
		Sleep(1000);
	}
	return 0;
}

#ifdef WIN32
DWORD WINAPI UnregisterThreadFn(LPVOID pvThreadParam)
#else
void *UnregisterThreadFn(void *pvThreadParam)
#endif
{
	SERVER_NODE* pServerNode = (SERVER_NODE*)pvThreadParam;
	BYTE is_connected = (pServerNode->m_bExit ? 0 : 1);
	CtrlCmd_TOPO_DROP(SOCKET_TYPE_TCP, g_fhandle, is_connected, ROUTE_ITEM_TYPE_GUAJINODE, pServerNode->myHttpOperate.m0_node_id);
	return 0;
}

#ifdef WIN32
DWORD WINAPI WorkingThreadFn1(LPVOID pvThreadParam)
#else
void *WorkingThreadFn1(void *pvThreadParam)
#endif
{
	SERVER_NODE* pServerNode = (SERVER_NODE*)pvThreadParam;
	UDTSOCKET fhandle;
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);
	DWORD use_ip;
	WORD use_port;
	char msg[MAX_LOADSTRING];
	int ret;
	int i;
	int nRetry;


	log_msg("WorkingThreadFn1()...", LOG_LEVEL_INFO);


	for (i = pServerNode->myHttpOperate.m1_wait_time; i > 0; i--) {
		_snprintf(msg, sizeof(msg), "正在与Peer端同步时间，%d秒。。。\n", i);
		log_msg(msg, LOG_LEVEL_INFO);
		usleep(1000*1000);
	}

	pServerNode->myHttpOperate.m1_use_udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (pServerNode->myHttpOperate.m1_use_udp_sock == INVALID_SOCKET) {
		log_msg("WorkingThreadFn1: UDP socket() failed!\n", LOG_LEVEL_INFO);
		return 0;
	}

	int flag = 1;
    setsockopt(pServerNode->myHttpOperate.m1_use_udp_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&flag, sizeof(flag));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(pServerNode->myHttpOperate.m0_p2p_port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(pServerNode->myHttpOperate.m1_use_udp_sock, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		log_msg("WorkingThreadFn1: UDP bind() failed!\n", LOG_LEVEL_INFO);
		closesocket(pServerNode->myHttpOperate.m1_use_udp_sock);
		pServerNode->myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}

	if (FindOutConnection(pServerNode->myHttpOperate.m1_use_udp_sock, pServerNode->myHttpOperate.m0_node_id, pServerNode->myHttpOperate.m1_peer_node_id,
							pServerNode->myHttpOperate.m1_peer_pri_ipArray, pServerNode->myHttpOperate.m1_peer_pri_ipCount, pServerNode->myHttpOperate.m1_peer_pri_port,
							pServerNode->myHttpOperate.m1_peer_ip, pServerNode->myHttpOperate.m1_peer_port, &use_ip, &use_port) != 0) {
		log_msg("FindOutConnection() failed!", LOG_LEVEL_INFO);
		return 0;  /* 2009-12-11 */
	}
	else {
		sprintf(msg, "FindOutConnection: use ip/port = %d.%d.%d.%d[%d]", 
			(use_ip & 0x000000ff) >> 0,
			(use_ip & 0x0000ff00) >> 8,
			(use_ip & 0x00ff0000) >> 16,
			(use_ip & 0xff000000) >> 24,
			use_port);
		log_msg(msg, LOG_LEVEL_INFO);
	}


	fhandle = UDT::socket(AF_INET, SOCK_STREAM, 0);

	ConfigUdtSocket(fhandle);

	if (UDT::ERROR == UDT::bind(fhandle, pServerNode->myHttpOperate.m1_use_udp_sock))
	{
		log_msg("WorkingThreadFn1: UDT::bind() failed!\n", LOG_LEVEL_INFO);
		UDT::close(fhandle);
		closesocket(pServerNode->myHttpOperate.m1_use_udp_sock);
		pServerNode->myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(use_port);
	their_addr.sin_addr.s_addr = use_ip;
	memset(&(their_addr.sin_zero), '\0', 8);
	nRetry = UDT_CONNECT_TIMES;
	ret = UDT::ERROR;
	while (nRetry-- > 0 && ret == UDT::ERROR) {
		//TRACE("@@@ UDT::connect()...\n");
		ret = UDT::connect(fhandle, (sockaddr*)&their_addr, sizeof(their_addr));
	}
	if (UDT::ERROR == ret)
	{
		log_msg("WorkingThreadFn1: UDT::connect() failed!\n", LOG_LEVEL_INFO);
		UDT::close(fhandle);
		closesocket(pServerNode->myHttpOperate.m1_use_udp_sock);
		pServerNode->myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}

	if (pServerNode->m_bDoConnection1) {
		log_msg("DoUnregister()...", LOG_LEVEL_INFO);
#ifdef WIN32
		DWORD dwThreadID;
		::CreateThread(NULL,0,UnregisterThreadFn,(void *)pServerNode,0,&dwThreadID);
#else
		pthread_t hTempThread;
		pthread_create(&hTempThread, NULL, UnregisterThreadFn, (void *)pServerNode);
#endif
	}

	pServerNode->myHttpOperate.m1_use_peer_ip = their_addr.sin_addr.s_addr;
	pServerNode->myHttpOperate.m1_use_peer_port = ntohs(their_addr.sin_port);
	pServerNode->myHttpOperate.m1_use_udt_sock = fhandle;
	pServerNode->myHttpOperate.m1_use_sock_type = SOCKET_TYPE_UDT;


	if (FALSE == pServerNode->m_InConnection2)
	{
		log_msg("WorkingThreadFn1: Enter ControlChannelLoop...\n", LOG_LEVEL_INFO);
		//============>
		ControlChannelLoop(pServerNode, pServerNode->myHttpOperate.m1_use_sock_type, pServerNode->myHttpOperate.m1_use_udt_sock);
		//============>
		log_msg("WorkingThreadFn1: Leave ControlChannelLoop!\n", LOG_LEVEL_INFO);
	}
	else {
		log_msg("WorkingThreadFn1: ###### g_InConnection2=true, drop this connection!\n", LOG_LEVEL_INFO);
	}


	UDT::close(pServerNode->myHttpOperate.m1_use_udt_sock);
	pServerNode->myHttpOperate.m1_use_udt_sock = INVALID_SOCKET;
	pServerNode->myHttpOperate.m1_use_sock_type = SOCKET_TYPE_UNKNOWN;
	closesocket(pServerNode->myHttpOperate.m1_use_udp_sock);
	pServerNode->myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;

	return 0;
}

#ifdef WIN32
DWORD WINAPI WorkingThreadFnRev(LPVOID pvThreadParam)
#else
void *WorkingThreadFnRev(void *pvThreadParam)
#endif
{
	SERVER_NODE* pServerNode = (SERVER_NODE*)pvThreadParam;
	UDTSOCKET fhandle;
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);
	DWORD use_ip;
	WORD use_port;
	char msg[MAX_LOADSTRING];
	int ret;
	int i;
	int nRetry;


	log_msg("WorkingThreadFnRev()...", LOG_LEVEL_INFO);

	for (i = pServerNode->myHttpOperate.m1_wait_time; i > 0; i--) {
		_snprintf(msg, sizeof(msg), "正在与Peer端同步时间，%d秒。。。", i);
		log_msg(msg, LOG_LEVEL_INFO);
		usleep(1000*1000);
	}

	pServerNode->myHttpOperate.m1_use_udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (pServerNode->myHttpOperate.m1_use_udp_sock == INVALID_SOCKET) {
		log_msg("WorkingThreadFnRev: UDP socket() failed!\n", LOG_LEVEL_INFO);
		return 0;
	}

	int flag = 1;
    setsockopt(pServerNode->myHttpOperate.m1_use_udp_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&flag, sizeof(flag));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(0);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(pServerNode->myHttpOperate.m1_use_udp_sock, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		log_msg("WorkingThreadFnRev: UDP bind() failed!\n", LOG_LEVEL_INFO);
		closesocket(pServerNode->myHttpOperate.m1_use_udp_sock);
		pServerNode->myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}


	fhandle = UDT::socket(AF_INET, SOCK_STREAM, 0);

	ConfigUdtSocket(fhandle);

	if (UDT::ERROR == UDT::bind(fhandle, pServerNode->myHttpOperate.m1_use_udp_sock))
	{
		log_msg("WorkingThreadFnRev: UDT::bind() failed!\n", LOG_LEVEL_INFO);
		UDT::close(fhandle);
		closesocket(pServerNode->myHttpOperate.m1_use_udp_sock);
		pServerNode->myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(pServerNode->myHttpOperate.m1_peer_port);
	their_addr.sin_addr.s_addr = pServerNode->myHttpOperate.m1_peer_ip;
	memset(&(their_addr.sin_zero), '\0', 8);
	nRetry = UDT_CONNECT_TIMES;
	ret = UDT::ERROR;
	while (nRetry-- > 0 && ret == UDT::ERROR) {
		ret = UDT::connect(fhandle, (sockaddr*)&their_addr, sizeof(their_addr));
		
		sprintf(msg, "WorkingThreadFnRev: UDT::connect() use ip/port = %d.%d.%d.%d[%d]", 
			(pServerNode->myHttpOperate.m1_peer_ip & 0x000000ff) >> 0,
			(pServerNode->myHttpOperate.m1_peer_ip & 0x0000ff00) >> 8,
			(pServerNode->myHttpOperate.m1_peer_ip & 0x00ff0000) >> 16,
			(pServerNode->myHttpOperate.m1_peer_ip & 0xff000000) >> 24,
			pServerNode->myHttpOperate.m1_peer_port);
		log_msg(msg, LOG_LEVEL_INFO);
	}
	if (UDT::ERROR == ret)
	{
		log_msg("WorkingThreadFnRev: UDT::connect() failed!\n", LOG_LEVEL_INFO);
		UDT::close(fhandle);
		closesocket(pServerNode->myHttpOperate.m1_use_udp_sock);
		pServerNode->myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}

	if (pServerNode->m_bDoConnection1) {
		log_msg("DoUnregister()...", LOG_LEVEL_INFO);
#ifdef WIN32
		DWORD dwThreadID;
		::CreateThread(NULL,0,UnregisterThreadFn,(void *)pServerNode,0,&dwThreadID);
#else
		pthread_t hTempThread;
		pthread_create(&hTempThread, NULL, UnregisterThreadFn, (void *)pServerNode);
#endif
	}

	pServerNode->myHttpOperate.m1_use_peer_ip = their_addr.sin_addr.s_addr;
	pServerNode->myHttpOperate.m1_use_peer_port = ntohs(their_addr.sin_port);
	pServerNode->myHttpOperate.m1_use_udt_sock = fhandle;
	pServerNode->myHttpOperate.m1_use_sock_type = SOCKET_TYPE_UDT;


	if (FALSE == pServerNode->m_InConnection2)
	{
		log_msg("WorkingThreadFnRev: Enter ControlChannelLoop...\n", LOG_LEVEL_INFO);
		//============>
		ControlChannelLoop(pServerNode, pServerNode->myHttpOperate.m1_use_sock_type, pServerNode->myHttpOperate.m1_use_udt_sock);
		//============>
		log_msg("WorkingThreadFnRev: Leave ControlChannelLoop!\n", LOG_LEVEL_INFO);
	}
	else {
		log_msg("WorkingThreadFnRev: ###### g_InConnection2=true, drop this connection!\n", LOG_LEVEL_INFO);
	}


	UDT::close(pServerNode->myHttpOperate.m1_use_udt_sock);
	pServerNode->myHttpOperate.m1_use_udt_sock = INVALID_SOCKET;
	pServerNode->myHttpOperate.m1_use_sock_type = SOCKET_TYPE_UNKNOWN;
	closesocket(pServerNode->myHttpOperate.m1_use_udp_sock);
	pServerNode->myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;

	return 0;
}

#ifdef WIN32
DWORD WINAPI WorkingThreadFn2(LPVOID pvThreadParam)
#else
void *WorkingThreadFn2(void *pvThreadParam)
#endif
{
	SERVER_NODE* pServerNode = (SERVER_NODE*)pvThreadParam;
	SOCKET udp_sock;
	UDTSOCKET serv;
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);
	UDTSOCKET fhandle;
	DWORD dwThreadID;
	int ret;


	udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udp_sock == INVALID_SOCKET) {
		log_msg("WorkingThreadFn2: UDP socket() failed!\n", LOG_LEVEL_INFO);
		return 0;
	}

	int flag = 1;
    setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&flag, sizeof(flag));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(pServerNode->myHttpOperate.m0_p2p_port - 2);//SECOND_CONNECT_PORT
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(udp_sock, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		log_msg("WorkingThreadFn2: UDP bind() failed!\n", LOG_LEVEL_INFO);
		closesocket(udp_sock);
		return 0;
	}

	while (true)
	{
		usleep(500*1000);  /* 2010-06-24 */

		if (pServerNode->m_InConnection1) {
			continue;
		}

		serv = UDT::socket(AF_INET, SOCK_STREAM, 0);

		ConfigUdtSocket(serv);

		if (UDT::ERROR == UDT::bind(serv, udp_sock))
		{
			log_msg("WorkingThreadFn2: UDT::bind() failed!\n", LOG_LEVEL_INFO);
			UDT::close(serv);

			//////////////////////////////////////////////////////////////////
			closesocket(udp_sock);
			udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&flag, sizeof(flag));
			my_addr.sin_family = AF_INET;
			my_addr.sin_port = htons(pServerNode->myHttpOperate.m0_p2p_port - 2);//SECOND_CONNECT_PORT
			my_addr.sin_addr.s_addr = INADDR_ANY;
			memset(&(my_addr.sin_zero), '\0', 8);
			bind(udp_sock, (sockaddr *)&my_addr, sizeof(my_addr));
			//////////////////////////////////////////////////////////////////

			continue;
		}

		if (UDT::ERROR == UDT::listen(serv, 1))
		{
			log_msg("WorkingThreadFn2: UDT::listen() failed!\n", LOG_LEVEL_INFO);
			UDT::close(serv);
			continue;
		}

#if 0
	///////////////
		struct timeval waitval;
		waitval.tv_sec  = 120;
		waitval.tv_usec = 0;
		ret = UDT::select(serv + 1, serv, NULL, NULL, &waitval);
		if (ret == UDT::ERROR || ret == 0) {
			log_msg("WorkingThreadFn2: UDT::select() failed/timeout!\n", LOG_LEVEL_INFO);
			UDT::close(serv);
			continue;
		}
	//////////////////
#endif

		if ((fhandle = UDT::accept(serv, (sockaddr*)&their_addr, &namelen)) == UDT::INVALID_SOCK) {
			log_msg("WorkingThreadFn2: UDT::accept() failed!\n", LOG_LEVEL_INFO);
			UDT::close(serv);
			continue;
		}
		else {
			log_msg("WorkingThreadFn2: UDT::accept() OK!\n", LOG_LEVEL_INFO);

			UDT::close(serv);

			if (FALSE == pServerNode->m_InConnection1)
			{
				pServerNode->m_InConnection2 = TRUE;

				/* 即将进入连接处理，如果启用服务器连接模式，则使其在服务器列表内不可见。*/
				if (pServerNode->m_bDoConnection1) {
					log_msg("WorkingThreadFn2: DoUnregister()...\n", LOG_LEVEL_INFO);
#ifdef WIN32
					::CreateThread(NULL,0,UnregisterThreadFn,(void *)pServerNode,0,&dwThreadID);
#else
					pthread_t hTempThread;
					pthread_create(&hTempThread, NULL, UnregisterThreadFn, (void *)pServerNode);
#endif
				}


				pServerNode->myHttpOperate.m1_use_udp_sock = udp_sock;
				pServerNode->myHttpOperate.m1_use_peer_ip = their_addr.sin_addr.s_addr;
				pServerNode->myHttpOperate.m1_use_peer_port = ntohs(their_addr.sin_port);
				pServerNode->myHttpOperate.m1_use_udt_sock = fhandle;
				pServerNode->myHttpOperate.m1_use_sock_type = SOCKET_TYPE_UDT;


				log_msg("WorkingThreadFn2: Enter ControlChannelLoop...\n", LOG_LEVEL_INFO);
				//============>
				ControlChannelLoop(pServerNode, pServerNode->myHttpOperate.m1_use_sock_type, pServerNode->myHttpOperate.m1_use_udt_sock);
				//============>
				log_msg("WorkingThreadFn2: Leave ControlChannelLoop!\n", LOG_LEVEL_INFO);


				UDT::close(pServerNode->myHttpOperate.m1_use_udt_sock);
				pServerNode->myHttpOperate.m1_use_udt_sock = INVALID_SOCKET;
				pServerNode->myHttpOperate.m1_use_sock_type = SOCKET_TYPE_UNKNOWN;
				//closesocket(pServerNode->myHttpOperate.m1_use_udp_sock);
				pServerNode->myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;

				pServerNode->m_InConnection2 = FALSE;
			}
			else {
				log_msg("WorkingThreadFn2: ###### g_InConnection1=true, drop this connection!\n", LOG_LEVEL_INFO);
				UDT::close(fhandle);
			}/* if (FALSE == g_InConnection1) */

		}

	}/* while */

	closesocket(udp_sock);

	log_msg("WorkingThreadFn2: Exit WorkingThreadFn2!\n", LOG_LEVEL_INFO);
	return 0;
}


///////////////////////////////////////////////////////////////////////////////

int VideoSendSetMediaType(SERVER_NODE *pServerNode, int width, int height, int fps)
{
	if (FALSE == pServerNode->m_bAVStarted)
	{
		return -1;
	}
	

	int m_nWidth = width;
	int m_nHeight = height;
	long long m_avgFrameTime = 10000000/fps;


	BYTE bSendBuff[17];
	BYTE bNRI  = 3; /* most important */
	BYTE bType = 30; /* use un-defined value */
	int i;

	bSendBuff[0] = (0x00 << 7) | (bNRI << 5) | (bType);
	pf_set_dword(bSendBuff + 1, htonl(m_nWidth));
	pf_set_dword(bSendBuff + 5, htonl(m_nHeight));
	pf_set_dword(bSendBuff + 9, htonl((DWORD)(m_avgFrameTime & 0x00000000ffffffffULL))); // lower 4 bytes
	pf_set_dword(bSendBuff + 13, htonl((DWORD)((m_avgFrameTime & 0xffffffff00000000ULL) >> 32))); // high 4 bytes

	if (pServerNode->m_bVideoReliable)
	{
		FakeRtpSend_sendpacket(pServerNode->m_pFRS, 0, bSendBuff, sizeof(bSendBuff), PAYLOAD_VIDEO, 0, bNRI | FAKERTP_RELIABLE_FLAG);
	}
	else
	{
		for (i = 0; i < 8; i++ ) {
			usleep(20000);
			FakeRtpSend_sendpacket(pServerNode->m_pFRS, 0, bSendBuff, sizeof(bSendBuff), PAYLOAD_VIDEO, 0, bNRI);
		}
	}
	
	pServerNode->m_bH264FormatSent = TRUE;
	return 0;
}

void DShowAV_Start(SERVER_NODE* pServerNode, BYTE flags, BYTE video_size, BYTE video_framerate, DWORD audio_channel, DWORD video_channel)
{
	if (pServerNode->m_bAVStarted) {
		return;
	}
	
	pServerNode->m_bVideoEnable = (0 != (flags & AV_FLAGS_VIDEO_ENABLE));
	pServerNode->m_bAudioEnable = (0 != (flags & AV_FLAGS_AUDIO_ENABLE));
	pServerNode->m_bTLVEnable = FALSE;
	pServerNode->m_bVideoReliable = (0 != (flags & AV_FLAGS_VIDEO_RELIABLE));
	pServerNode->m_bAudioRedundance = (0 != (flags & AV_FLAGS_AUDIO_REDUNDANCE));
	
	if (!(pServerNode->m_bVideoEnable) && !(pServerNode->m_bAudioEnable)) {
		return;
	}
	
	T_RTPPARAM tRtpParam;
	//tRtpParam.nBasePort = nBasePort;
	//tRtpParam.pFRS = NULL;
	tRtpParam.dwDestIp  = pServerNode->myHttpOperate.m1_use_peer_ip;
	tRtpParam.nDestPort = pServerNode->myHttpOperate.m1_use_peer_port;
	tRtpParam.udpSock   = pServerNode->myHttpOperate.m1_use_udp_sock;
	tRtpParam.fhandle   = pServerNode->myHttpOperate.m1_use_udt_sock;
	tRtpParam.ftype     = pServerNode->myHttpOperate.m1_use_sock_type;

	FAKERTPSEND *pFRS = NULL;
	FakeRtpSend_init(&pFRS, &tRtpParam);

	pServerNode->m_pFRS = pFRS;
	pServerNode->m_bAVStarted = TRUE;
	pServerNode->m_bH264FormatSent = FALSE;

	g_is_topo_primary = TRUE;

	if (pServerNode->m_bVideoEnable) {
		VideoSendSetMediaType(pServerNode, g_video_width, g_video_height, g_video_fps);
	}

	//首先VideoSendSetMediaType，然后通知Repeater发送SPS，PPS。。。
	if (INVALID_SOCKET != g_fhandle)
	{
		CtrlCmd_AV_START(SOCKET_TYPE_TCP, g_fhandle, flags, video_size, video_framerate, audio_channel, video_channel);
	}
}

void DShowAV_Stop(SERVER_NODE* pServerNode)
{
	if (INVALID_SOCKET != g_fhandle)
	{
		CtrlCmd_AV_STOP(SOCKET_TYPE_TCP, g_fhandle);
	}

	if (!(pServerNode->m_bAVStarted)) {
		return;
	}
	pServerNode->m_bAVStarted = FALSE;

	g_is_topo_primary = FALSE;

	FakeRtpSend_uninit(pServerNode->m_pFRS);
	pServerNode->m_pFRS = NULL;
}

void DShowAV_Switch(SERVER_NODE* pServerNode, DWORD video_channel)
{
	if (INVALID_SOCKET != g_fhandle)
	{
		CtrlCmd_AV_SWITCH(SOCKET_TYPE_TCP, g_fhandle, video_channel);
	}
}

void DShowAV_Contrl(SERVER_NODE* pServerNode, WORD contrl, DWORD contrl_param)
{
	if (INVALID_SOCKET != g_fhandle)
	{
		CtrlCmd_AV_CONTRL(SOCKET_TYPE_TCP, g_fhandle, contrl, contrl_param);
	}
}


///////////////////////////////////////////////////////////////////////////////

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

/* 为了RecvStream()运行安全，必须保证ControlChannelLoop()同一时间只被一个线程执行。*/
int ControlChannelLoop(SERVER_NODE* pServerNode, SOCKET_TYPE type, SOCKET fhandle)
{
	BOOL bAuthOK = FALSE;
	BOOL bQuitLoop = FALSE;
	char buff[16*1024];
	int ret;
	WORD wCommand;
	DWORD dwDataLength;
	WORD wTemp;
	BYTE is_connected;
	BYTE node_type;
	BYTE source_node_id[6];
	BYTE dest_node_id[6];
	BYTE object_node_id[6];
	DWORD begin_time,end_time,stream_flow;
	char *temp_ptr;
	char msg[MAX_LOADSTRING];
	char szPassEnc[PHP_MD5_OUTPUT_CHARS + 1];
	TCHAR szValueData[_MAX_PATH];
	BOOL bSingleQuit;
	WORD wLocalTcpPort;
	UDTSOCKET fhandle_slave;
	sockaddr_in their_addr;


	//CoInitialize(NULL);
	if_on_client_connected(pServerNode);

	while (FALSE == bQuitLoop && 
			(pServerNode->m_bDoConnection1 || pServerNode->m_bDoConnection2))
	{
		ret = RecvStream(type, fhandle, buff, 6);
		if (ret != 0) {
			log_msg("ControlChannelLoop: RecvStream(6) failed!\n", LOG_LEVEL_ERROR);
			break;
		}

		wCommand = ntohs(*(WORD*)buff);
		dwDataLength = ntohl(*(DWORD*)(buff+2));


#if 1 /* 密码校验成功后才能受理其他命令。*/
		if (FALSE == bAuthOK && CMD_CODE_HELLO_REQ != wCommand) {
			break;
		}
#endif

		switch (wCommand)
		{
			case CMD_CODE_HELLO_REQ:
				ret = RecvStream(type, fhandle, buff, 6);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				//if (memcmp(g1_peer_node_id, buff, 6) != 0) {
				//	bQuitLoop = TRUE;
				//	break;
				//}
				memcpy(g_peer_node_id, buff, 6);

				ret = RecvStream(type, fhandle, buff, 4);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}

				ret = RecvStream(type, fhandle, buff, 1);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				g_is_topo_primary = (*(BYTE *)buff == 1);

				ret = RecvStream(type, fhandle, buff, 256);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}

				if (if_read_password(szValueData, sizeof(szValueData)) == 0 && strcmp(szValueData, "") != 0)
				{
					php_md5(szValueData, szPassEnc, sizeof(szPassEnc));
					if (strcmp(szPassEnc, buff) != 0) {
						CtrlCmd_Send_HELLO_RESP(type, fhandle, pServerNode->myHttpOperate.m0_node_id, g0_version, getServerFuncFlags(), g_device_topo_level, CTRLCMD_RESULT_NG);
						log_msg("ControlChannelLoop: Password failed!\n", LOG_LEVEL_INFO);
						break;
					}
				}
				CtrlCmd_Send_HELLO_RESP(type, fhandle, pServerNode->myHttpOperate.m0_node_id, g0_version, getServerFuncFlags(), g_device_topo_level, CTRLCMD_RESULT_OK);
				log_msg("ControlChannelLoop: Password OK!\n", LOG_LEVEL_INFO);
				bAuthOK = TRUE;

				break;

			case CMD_CODE_RUN_REQ:
				int read_len;
				char *exe_cmd;
				exe_cmd = (char *)malloc(dwDataLength);////////
				ret = RecvStream(type, fhandle, exe_cmd, dwDataLength);
				if (ret != 0) {
					free(exe_cmd);////////
					bQuitLoop = TRUE;
					break;
				}
				log_msg("ControlChannelLoop: exe_cmd=\n", LOG_LEVEL_INFO);
				log_msg(exe_cmd, LOG_LEVEL_INFO);
				//RunExeNoWait(exe_cmd, FALSE);
				free(exe_cmd);////////
				CtrlCmd_Send_RUN_RESP(type, fhandle, "<No stdout>\n");
				break;

			case CMD_CODE_PROXY_REQ:
				ret = RecvStream(type, fhandle, buff, 2);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}

				// Do nothing...

				break;
				
///////////////////////////////////////////////////////////////////////////////
			/* 如果ProxyServer()异常结束，就需要在这里妥善处理 CMD_CODE_PROXY_DATA 和 CMD_CODE_PROXY_END */
			case CMD_CODE_PROXY_DATA:
				log_msg("ControlChannelLoop: Recv CMD_CODE_PROXY_DATA !!!!!!\n", LOG_LEVEL_INFO);
				for (int i = 0; i < dwDataLength; i++) {
					RecvStream(type, fhandle, buff, 1);
				}
				break;
				
			case CMD_CODE_PROXY_END:
				log_msg("ControlChannelLoop: Recv CMD_CODE_PROXY_END !!!!!!\n", LOG_LEVEL_INFO);
				break;
///////////////////////////////////////////////////////////////////////////////
				
			case CMD_CODE_ARM_REQ:
				if_mc_arm();
				break;
				
			case CMD_CODE_DISARM_REQ:
				if_mc_disarm();
				break;

			case CMD_CODE_AV_START_REQ:
				ret = RecvStream(type, fhandle, buff, 1+1+1+4+4);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}

				DShowAV_Start(pServerNode, (BYTE)(buff[0]), (BYTE)(buff[1]), (BYTE)(buff[2]), ntohl(pf_get_dword(buff+3)), ntohl(pf_get_dword(buff+7)) );

				break;

			case CMD_CODE_AV_STOP_REQ:
				DShowAV_Stop(pServerNode);
				CtrlCmd_Send_END(type, fhandle);
				break;

			case CMD_CODE_AV_SWITCH_REQ:
				ret = RecvStream(type, fhandle, buff, 4);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				DShowAV_Switch(pServerNode, ntohl(pf_get_dword(buff+0)));
				break;

			case CMD_CODE_AV_CONTRL_REQ:
				ret = RecvStream(type, fhandle, buff, 2+4);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				DShowAV_Contrl(pServerNode, ntohs(pf_get_word(buff+0)), ntohl(pf_get_dword(buff+2)));
				break;

			case CMD_CODE_VOICE_REQ:
				BYTE *voice_data;
				voice_data = (BYTE *)malloc(dwDataLength);
				ret = RecvStream(type, fhandle, (char *)voice_data, dwDataLength);
				if (ret != 0) {
					free(voice_data);
					bQuitLoop = TRUE;
					break;
				}
				//Play_Voice(voice_data, dwDataLength);
				free(voice_data);
				break;

			case CMD_CODE_TEXT_REQ:
				ret = RecvStream(type, fhandle, buff, 512);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				//if_display_text(buff);
				break;

			case CMD_CODE_NULL:
				//收到保活包，什么也不做
				break;

			case CMD_CODE_TOPO_REPORT:

				ret = RecvStream(type, fhandle, buff, 6);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				memcpy(source_node_id, buff, 6);

				ret = RecvStream(type, fhandle, buff, 6);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}

				temp_ptr = (char *)malloc(dwDataLength - 12);
				ret = RecvStream(type, fhandle, temp_ptr, dwDataLength - 12);
				if (ret != 0) {
					free(temp_ptr);
					bQuitLoop = TRUE;
					break;
				}
				CtrlCmd_TOPO_REPORT(SOCKET_TYPE_TCP, g_fhandle, source_node_id, (const char *)temp_ptr);
				free(temp_ptr);

				g_is_topo_primary = TRUE;

				break;

			case CMD_CODE_TOPO_DROP:

				ret = RecvStream(type, fhandle, buff, 1);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				is_connected = *(BYTE *)buff;

				ret = RecvStream(type, fhandle, buff, 1);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				node_type = *(BYTE *)buff;

				ret = RecvStream(type, fhandle, buff, 6);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				memcpy(object_node_id, buff, 6);

				CtrlCmd_TOPO_DROP(SOCKET_TYPE_TCP, g_fhandle, is_connected, node_type, object_node_id);

				break;

			case CMD_CODE_TOPO_EVALUATE:

				ret = RecvStream(type, fhandle, buff, 6);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				memcpy(source_node_id, buff, 6);

				ret = RecvStream(type, fhandle, buff, 6);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				memcpy(object_node_id, buff, 6);

				ret = RecvStream(type, fhandle, buff, 4);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				begin_time = ntohl(pf_get_dword(buff));

				ret = RecvStream(type, fhandle, buff, 4);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				end_time = ntohl(pf_get_dword(buff));

				ret = RecvStream(type, fhandle, buff, 4);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				stream_flow = ntohl(pf_get_dword(buff));

				CtrlCmd_TOPO_EVALUATE(SOCKET_TYPE_TCP, g_fhandle, source_node_id, object_node_id, begin_time, end_time, stream_flow);

				break;

			case CMD_CODE_TOPO_PACKET:

				break;

			case CMD_CODE_BYE_REQ:
			default:
				bQuitLoop = TRUE;
				break;
		}
	} /* while */

	DShowAV_Stop(pServerNode);
	if_on_client_disconnected(pServerNode);
	//CoUninitialize();
	return 0;
}

