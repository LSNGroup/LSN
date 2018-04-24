#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef ANDROID_NDK
#include <jni.h>
#include "jni_func.h"
#endif

#include "platform.h"
#include "CommonLib.h"
#include "Discovery.h"
#include "ControlCmd.h"
#include "HttpOperate.h"
#include "phpMd5.h"
#include "stun.h"
#include "colorspace.h"
#include "mobcam_if.h"
#include "mobcam_av.h"

#if defined(ANDROID_NDK)
#include <android/log.h>
#else
#define __android_log_print(level, tag, format, ...) printf(format, ## __VA_ARGS__)
#endif


#include "UPnP.h"
static MyUPnP  myUPnP;
static UPNPNAT_MAPPING mapping;

HttpOperate myHttpOperate;

static BOOL g_bFirstCheckStun = TRUE;

BOOL g_bDoConnection1 = FALSE;
BOOL g_bDoConnection2 = FALSE;
BOOL g_InConnection1 = FALSE;
BOOL g_InConnection2 = FALSE;

static char g_client_charset[16] = "utf8";
static char g_client_lang[16] = "zh";
static char g_app_package_name[256];

static pthread_t g_hUnregisterThread = 0;
static pthread_t g_hWorkingThread1 = 0;
static pthread_t g_hWorkingThreadRev = 0;

void *UnregisterThreadFn(void *pvThreadParam);
void *WorkingThreadFn1(void *pvThreadParam);
void *WorkingThreadFnRev(void *pvThreadParam);

static int ControlChannelLoop(SOCKET_TYPE type, SOCKET fhandle);



static void GetLocalInformation()
{
	DWORD ipArrayTemp[MAX_ADAPTER_NUM];  /* Network byte order */
	int ipCountTemp;

	/* Local IP Address */
	ipCountTemp = MAX_ADAPTER_NUM;
	if (GetLocalAddress(ipArrayTemp, &ipCountTemp) != NO_ERROR) {
		__android_log_print(ANDROID_LOG_INFO, "GetLocalInformation", "GetLocalAddress() failed!\n");
	}
	else {
		myHttpOperate.m0_ipCount = ipCountTemp;
		memcpy(myHttpOperate.m0_ipArray, ipArrayTemp, sizeof(DWORD)*ipCountTemp);
		myHttpOperate.m0_port = myHttpOperate.m0_p2p_port;
		__android_log_print(ANDROID_LOG_INFO, "GetLocalInformation", "GetLocalAddress: ipCount=%d\n", myHttpOperate.m0_ipCount);
	}
	
	
	/* Node Name */
	if_read_nodename(g0_node_name, sizeof(g0_node_name));
	
	
	/* OS Info */
	if_get_osinfo(g0_os_info, sizeof(g0_os_info));
}

static void InitVar()
{
	int ret;
	
	ret = if_get_device_uuid(g0_device_uuid, sizeof(g0_device_uuid));
	__android_log_print(ANDROID_LOG_INFO, "InitVar", "if_get_device_uuid() = %s\n", g0_device_uuid);
	
	
	char szNodeId[20];
	if_get_nodeid(szNodeId, sizeof(szNodeId));
	__android_log_print(ANDROID_LOG_INFO, "InitVar", "if_get_nodeid() = %s\n", szNodeId);
	mac_addr(szNodeId, myHttpOperate.m0_node_id, NULL);
	
	
	g0_version = MYSELF_VERSION;


	strncpy(g1_http_server, DEFAULT_HTTP_SERVER, sizeof(g1_http_server));
	strncpy(g1_stun_server, DEFAULT_STUN_SERVER, sizeof(g1_stun_server));
	myHttpOperate.m1_http_client_ip = 0;

	myHttpOperate.m0_no_nat = false;
	myHttpOperate.m0_nat_type = StunTypeUnknown;
	myHttpOperate.m0_pub_ip = 0;
	myHttpOperate.m0_pub_port = 0;

	myHttpOperate.m0_p2p_port = FIRST_CONNECT_PORT;
	myHttpOperate.m0_is_admin = false;
	myHttpOperate.m0_is_busy = false;
	g1_register_period = DEFAULT_REGISTER_PERIOD;  /* Seconds */
	g1_expire_period = DEFAULT_EXPIRE_PERIOD;  /* Seconds */
	
	g1_is_activated = true;
	g1_comments_id = 0;
	
	myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;
	myHttpOperate.m1_use_udt_sock = INVALID_SOCKET;
	myHttpOperate.m1_use_sock_type = SOCKET_TYPE_UNKNOWN;
}

void *NativeMainFunc(void *lpParameter)
{
	int ret;
	StunAddress4 mapped;
	BOOL bNoNAT;
	int  nNatType;


	__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "UdtStartup()\n");


	BOOL bMappingExists = FALSE;
	mapping.description = "";
	mapping.protocol = UNAT_UDP;
	mapping.externalPort = ((myUPnP.GetLocalIP() & 0xff000000) >> 24) | ((2049 + (65535 - 2049) * (WORD)rand() / (65536)) & 0xffffff00);
	while (UNAT_OK == myUPnP.GetNATSpecificEntry(&mapping, &bMappingExists) && bMappingExists)
	{
		__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "Find NATPortMapping(%d, rand=%d), retry...\n", mapping.externalPort, rand());
		bMappingExists = FALSE;
		mapping.description = "";
		mapping.protocol = UNAT_UDP;
		mapping.externalPort = ((myUPnP.GetLocalIP() & 0xff000000) >> 24) | ((2049 + (65535 - 2049) * (WORD)rand() / (65536)) & 0xffffff00);
	}//找到一个未被占用的外部端口映射，或者路由器UPnP功能不可用


	if (g_bDoConnection1)
	{
		GetLocalInformation();

		ret = myHttpOperate.DoRegister1(g_client_charset, g_client_lang);
		__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "DoRegister1(%s, %s) = %d\n", g_client_charset, g_client_lang, ret);
		if (ret < 0) {
			ret = myHttpOperate.DoRegister1(g_client_charset, g_client_lang);
			__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "DoRegister1(%s, %s) = %d\n", g_client_charset, g_client_lang, ret);
		}

		if (ret >= 0)
		{
			if (g_bDoConnection1)
			{
				/* STUN Information */
				if (strcmp(g1_stun_server, "NONE") == 0)
				{
					ret = CheckStunMyself(g1_http_server, myHttpOperate.m0_p2p_port, &mapped, &bNoNAT, &nNatType);
					if (ret == -1) {
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStunMyself() failed!\n");
						
					   mapped.addr = ntohl(myHttpOperate.m1_http_client_ip);
					   mapped.port = myHttpOperate.m0_p2p_port;
					   
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
					   
						myHttpOperate.m0_pub_ip = htonl(mapped.addr);
						myHttpOperate.m0_pub_port = mapped.port;
						myHttpOperate.m0_no_nat = bNoNAT;
						myHttpOperate.m0_nat_type = nNatType;
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStunHttp: %d.%d.%d.%d[%d], NoNAT=%d\n", 
							(myHttpOperate.m0_pub_ip & 0x000000ff) >> 0,
							(myHttpOperate.m0_pub_ip & 0x0000ff00) >> 8,
							(myHttpOperate.m0_pub_ip & 0x00ff0000) >> 16,
							(myHttpOperate.m0_pub_ip & 0xff000000) >> 24,
							myHttpOperate.m0_pub_port,  myHttpOperate.m0_no_nat ? 1 : 0);
					}
					else {
						myHttpOperate.m0_pub_ip = htonl(mapped.addr);
						myHttpOperate.m0_pub_port = mapped.port;
						myHttpOperate.m0_no_nat = bNoNAT;
						myHttpOperate.m0_nat_type = nNatType;
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStunMyself: %d.%d.%d.%d[%d]\n", 
							(myHttpOperate.m0_pub_ip & 0x000000ff) >> 0,
							(myHttpOperate.m0_pub_ip & 0x0000ff00) >> 8,
							(myHttpOperate.m0_pub_ip & 0x00ff0000) >> 16,
							(myHttpOperate.m0_pub_ip & 0xff000000) >> 24,
							myHttpOperate.m0_pub_port);
					}
				}
				else if (g_bFirstCheckStun) {
					ret = CheckStun(g1_stun_server, myHttpOperate.m0_p2p_port, &mapped, &bNoNAT, &nNatType);
					if (ret == -1) {
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStun() failed!\n");
						
					   mapped.addr = ntohl(myHttpOperate.m1_http_client_ip);
					   mapped.port = myHttpOperate.m0_p2p_port;
					   
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
					   
						myHttpOperate.m0_pub_ip = htonl(mapped.addr);
						myHttpOperate.m0_pub_port = mapped.port;
						myHttpOperate.m0_no_nat = bNoNAT;
						myHttpOperate.m0_nat_type = nNatType;
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStunHttp: %d.%d.%d.%d[%d], NoNAT=%d\n", 
							(myHttpOperate.m0_pub_ip & 0x000000ff) >> 0,
							(myHttpOperate.m0_pub_ip & 0x0000ff00) >> 8,
							(myHttpOperate.m0_pub_ip & 0x00ff0000) >> 16,
							(myHttpOperate.m0_pub_ip & 0xff000000) >> 24,
							myHttpOperate.m0_pub_port,  myHttpOperate.m0_no_nat ? 1 : 0);
					}
					else if (ret == 0) {
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStun() blocked!\n");
						myHttpOperate.m0_pub_ip = 0;
						myHttpOperate.m0_pub_port = 0;
						myHttpOperate.m0_no_nat = FALSE;
						myHttpOperate.m0_nat_type = nNatType;
					}
					else {
						g_bFirstCheckStun = FALSE;
						myHttpOperate.m0_pub_ip = htonl(mapped.addr);
						myHttpOperate.m0_pub_port = mapped.port;
						myHttpOperate.m0_no_nat = bNoNAT;
						myHttpOperate.m0_nat_type = nNatType;
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStun: %d.%d.%d.%d[%d]\n", 
							(myHttpOperate.m0_pub_ip & 0x000000ff) >> 0,
							(myHttpOperate.m0_pub_ip & 0x0000ff00) >> 8,
							(myHttpOperate.m0_pub_ip & 0x00ff0000) >> 16,
							(myHttpOperate.m0_pub_ip & 0xff000000) >> 24,
							myHttpOperate.m0_pub_port);
					}
				}
				else {
					ret = CheckStunSimple(g1_stun_server, myHttpOperate.m0_p2p_port, &mapped);
					if (ret == -1) {
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStunSimple() failed!\n");
						mapped.addr = ntohl(myHttpOperate.m1_http_client_ip);
						mapped.port = myHttpOperate.m0_p2p_port;
						myHttpOperate.m0_pub_ip = htonl(mapped.addr);
						myHttpOperate.m0_pub_port = mapped.port;
						g_bFirstCheckStun = TRUE;
					}
					else {
						myHttpOperate.m0_pub_ip = htonl(mapped.addr);
						myHttpOperate.m0_pub_port = mapped.port;
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStunSimple: %d.%d.%d.%d[%d]\n", 
							(myHttpOperate.m0_pub_ip & 0x000000ff) >> 0,
							(myHttpOperate.m0_pub_ip & 0x0000ff00) >> 8,
							(myHttpOperate.m0_pub_ip & 0x00ff0000) >> 16,
							(myHttpOperate.m0_pub_ip & 0xff000000) >> 24,
							myHttpOperate.m0_pub_port);
					}
				}
				
				
				/* 本地路由器UPnP处理*/
				if (TRUE == if_should_do_upnp() && FALSE == bNoNAT)
				{
					char extIp[16];
					strcpy(extIp, "");
					myUPnP.GetNATExternalIp(extIp);
					if (strlen(extIp) > 0 && 0 != myHttpOperate.m0_pub_ip && inet_addr(extIp) == myHttpOperate.m0_pub_ip)
					{
						mapping.description = "UP2P";
						//mapping.protocol = ;
						//mapping.externalPort = ;
						mapping.internalPort = myHttpOperate.m0_p2p_port - 2;//SECOND_CONNECT_PORT;
						if (UNAT_OK == myUPnP.AddNATPortMapping(&mapping, false))
						{
							g_bFirstCheckStun = FALSE;

							myHttpOperate.m0_pub_ip = inet_addr(extIp);
							myHttpOperate.m0_pub_port = mapping.externalPort;
							myHttpOperate.m0_no_nat = TRUE;
							myHttpOperate.m0_nat_type = StunTypeOpen;

							__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "UPnP AddPortMapping OK: %s[%d] --> %s[%d]", extIp, mapping.externalPort, myUPnP.GetLocalIPStr().c_str(), mapping.internalPort);
						}
						else
						{
							BOOL bTempExists = FALSE;
							myUPnP.GetNATSpecificEntry(&mapping, &bTempExists);
							if (bTempExists)
							{
								g_bFirstCheckStun = FALSE;

								myHttpOperate.m0_pub_ip = inet_addr(extIp);
								myHttpOperate.m0_pub_port = mapping.externalPort;
								myHttpOperate.m0_no_nat = TRUE;
								myHttpOperate.m0_nat_type = StunTypeOpen;

								__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "UPnP PortMapping Exists: %s[%d] --> %s[%d]", extIp, mapping.externalPort, myUPnP.GetLocalIPStr().c_str(), mapping.internalPort);
							}
							else {
								myHttpOperate.m0_pub_ip = htonl(mapped.addr);
								myHttpOperate.m0_pub_port = mapped.port;
								myHttpOperate.m0_no_nat = bNoNAT;
								myHttpOperate.m0_nat_type = nNatType;
							}
						}
					}
					else {
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "UPnP PortMapping Skipped: %s --> %s", extIp, myUPnP.GetLocalIPStr().c_str());
						myHttpOperate.m0_pub_ip = htonl(mapped.addr);
						myHttpOperate.m0_pub_port = mapped.port;
						myHttpOperate.m0_no_nat = bNoNAT;
						myHttpOperate.m0_nat_type = nNatType;
					}
				}/* 本地路由器UPnP处理*/
				
				//修正公网IP受控端的连接端口问题,2014-6-15
				if (bNoNAT) {
					myHttpOperate.m0_pub_port = myHttpOperate.m0_p2p_port - 2;//SECOND_CONNECT_PORT;
				}

				if (myHttpOperate.m0_no_nat) {
					myHttpOperate.m0_port = SECOND_CONNECT_PORT;
				}

				int joined_channel_id = 0;
				ret = myHttpOperate.DoPush(g_client_charset, g_client_lang, "Channel Comments", &joined_channel_id);
				__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "DoPush() = %d\n", ret);
				if_on_push_result(ret, joined_channel_id);
				if (ret == 1 && joined_channel_id > 0) {
					ret = 3;//Event
				}

			}
			else {
				ret = 0;
			}
		}//if (ret >= 0)

		if (ret == -1) {
			if_on_register_network_error();
			//if (g_bDoConnection1) OnDailerEvent();//宽带拨号，g_bDoConnection1==TRUE
			g_bFirstCheckStun = TRUE;//可能已切换网络
		}

		/* Connection */
		if (ret == 3) {

			if (strcmp(myHttpOperate.m1_event_type, "Accept") == 0)  /* Accept */
			{
				strcpy(myHttpOperate.m1_event_type, "");

				g_InConnection1 = TRUE;

				//hThread = ::CreateThread(NULL,0,WorkingThreadFn1,NULL,0,&dwThreadID);
				pthread_create(&g_hWorkingThread1, NULL, WorkingThreadFn1, NULL);

				if (g_bDoConnection1) {
					__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "DoUnregister()...\n");
					pthread_create(&g_hUnregisterThread, NULL, UnregisterThreadFn, NULL);
				}

				/* Wait... */
				pthread_join(g_hWorkingThread1, NULL);
				g_hWorkingThread1 = 0;
				__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "WorkingThread ends, continue to do Register loop...\n");

				g_InConnection1 = FALSE;
			}
			else if (strcmp(myHttpOperate.m1_event_type, "AcceptRev") == 0)  /* AcceptRev */
			{
				strcpy(myHttpOperate.m1_event_type, "");

				g_InConnection1 = TRUE;

				//hThread = ::CreateThread(NULL,0,WorkingThreadFnRev,NULL,0,&dwThreadID);
				pthread_create(&g_hWorkingThreadRev, NULL, WorkingThreadFnRev, NULL);

				if (g_bDoConnection1) {
					__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "DoUnregister()...\n");
					pthread_create(&g_hUnregisterThread, NULL, UnregisterThreadFn, NULL);
				}

				/* Wait... */
				pthread_join(g_hWorkingThreadRev, NULL);
				g_hWorkingThreadRev = 0;
				__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "WorkingThread ends, continue to do Register loop...\n");

				g_InConnection1 = FALSE;
			}
			else if (strcmp(myHttpOperate.m1_event_type, "AcceptProxy") == 0)  /* AcceptProxy */
			{
				strcpy(myHttpOperate.m1_event_type, "");
				;
			}
			else if (strcmp(myHttpOperate.m1_event_type, "AcceptProxyTcp") == 0) /* AcceptProxyTcp */
			{
				strcpy(myHttpOperate.m1_event_type, "");
				;
			}
		}//if (ret == 3) {

	}//if (g_bDoConnection1)

	__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "Exit main function!\n");
	return 0;
}

void *UnregisterThreadFn(void *pvThreadParam)
{
	myHttpOperate.DoUnregister(g_client_charset, g_client_lang);
}

void *WorkingThreadFn1(void *pvThreadParam)
{
	UDTSOCKET fhandle;
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);
	DWORD use_ip;
	WORD use_port;
	int ret;
	int nRetry;


	__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1", "WorkingThreadFn1()...\n");

	for (int i = myHttpOperate.m1_wait_time; i > 0; i--) {
		//_snprintf(msg, sizeof(msg), "正在排队等待服务响应，%d秒。。。\n", i);
		//log_msg(msg, LOG_LEVEL_INFO);
		usleep(1000*1000);
	}

	myHttpOperate.m1_use_udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (myHttpOperate.m1_use_udp_sock == INVALID_SOCKET) {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1", "UDP socket() failed!\n");
		return 0;
	}

	int flag = 1;
    setsockopt(myHttpOperate.m1_use_udp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(myHttpOperate.m0_p2p_port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(myHttpOperate.m1_use_udp_sock, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1", "UDP bind() failed!\n");
		closesocket(myHttpOperate.m1_use_udp_sock);
		myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}

	if (FindOutConnection(myHttpOperate.m1_use_udp_sock, myHttpOperate.m0_node_id, myHttpOperate.m1_peer_node_id,
							myHttpOperate.m1_peer_pri_ipArray, myHttpOperate.m1_peer_pri_ipCount, myHttpOperate.m1_peer_pri_port,
							myHttpOperate.m1_peer_ip, myHttpOperate.m1_peer_port, &use_ip, &use_port) != 0) {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1", "FindOutConnection() failed!\n");
		closesocket(myHttpOperate.m1_use_udp_sock);
		myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1", "FindOutConnection: use ip/port = %d.%d.%d.%d[%d]\n", 
			(use_ip & 0x000000ff) >> 0,
			(use_ip & 0x0000ff00) >> 8,
			(use_ip & 0x00ff0000) >> 16,
			(use_ip & 0xff000000) >> 24,
			use_port);
	}


	fhandle = UDT::socket(AF_INET, SOCK_STREAM, 0);

	ConfigUdtSocket(fhandle);

	if (UDT::ERROR == UDT::bind(fhandle, myHttpOperate.m1_use_udp_sock))
	{
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1", "UDT::bind() failed!\n");
		UDT::close(fhandle);
		closesocket(myHttpOperate.m1_use_udp_sock);
		myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;
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
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1", "UDT::connect() failed!\n");
		UDT::close(fhandle);
		closesocket(myHttpOperate.m1_use_udp_sock);
		myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}

	myHttpOperate.m1_use_peer_ip = their_addr.sin_addr.s_addr;
	myHttpOperate.m1_use_peer_port = ntohs(their_addr.sin_port);
	myHttpOperate.m1_use_udt_sock = fhandle;
	myHttpOperate.m1_use_sock_type = SOCKET_TYPE_UDT;


	if (FALSE == g_InConnection2)
	{
				__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1", "Enter ControlChannelLoop...\n");
		//============>
		ControlChannelLoop(myHttpOperate.m1_use_sock_type, myHttpOperate.m1_use_udt_sock);
		//============>
				__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1", "Leave ControlChannelLoop!\n");
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1", "###### g_InConnection2=true, drop this connection!\n");
	}


	UDT::close(myHttpOperate.m1_use_udt_sock);
	myHttpOperate.m1_use_udt_sock = INVALID_SOCKET;
	myHttpOperate.m1_use_sock_type = SOCKET_TYPE_UNKNOWN;
	closesocket(myHttpOperate.m1_use_udp_sock);////????
	myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;

	return 0;
}

void *WorkingThreadFnRev(void *pvThreadParam)
{
	UDTSOCKET fhandle;
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);
	DWORD use_ip;
	WORD use_port;
	int ret;
	int nRetry;


	__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFnRev", "WorkingThreadFnRev()...\n");

	for (int i = myHttpOperate.m1_wait_time; i > 0; i--) {
		//_snprintf(msg, sizeof(msg), "正在排队等待服务响应，%d秒。。。\n", i);
		//log_msg(msg, LOG_LEVEL_INFO);
		usleep(1000*1000);
	}

	myHttpOperate.m1_use_udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (myHttpOperate.m1_use_udp_sock == INVALID_SOCKET) {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFnRev", "UDP socket() failed!\n");
		return 0;
	}

	int flag = 1;
    setsockopt(myHttpOperate.m1_use_udp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(0);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(myHttpOperate.m1_use_udp_sock, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFnRev", "UDP bind() failed!\n");
		closesocket(myHttpOperate.m1_use_udp_sock);
		myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}


	fhandle = UDT::socket(AF_INET, SOCK_STREAM, 0);

	ConfigUdtSocket(fhandle);

	if (UDT::ERROR == UDT::bind(fhandle, myHttpOperate.m1_use_udp_sock))
	{
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFnRev", "UDT::bind() failed!\n");
		UDT::close(fhandle);
		closesocket(myHttpOperate.m1_use_udp_sock);
		myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(myHttpOperate.m1_peer_port);
	their_addr.sin_addr.s_addr = myHttpOperate.m1_peer_ip;
	memset(&(their_addr.sin_zero), '\0', 8);
	nRetry = UDT_CONNECT_TIMES;
	ret = UDT::ERROR;
	while (nRetry-- > 0 && ret == UDT::ERROR) {
#if 1 ////Debug
	__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFnRev", "@@@ UDT::connect()... ip/port: %d.%d.%d.%d[%d]\n", 
		(myHttpOperate.m1_peer_ip & 0x000000ff) >> 0,
		(myHttpOperate.m1_peer_ip & 0x0000ff00) >> 8,
		(myHttpOperate.m1_peer_ip & 0x00ff0000) >> 16,
		(myHttpOperate.m1_peer_ip & 0xff000000) >> 24,
		myHttpOperate.m1_peer_port);
#endif
		ret = UDT::connect(fhandle, (sockaddr*)&their_addr, sizeof(their_addr));
	}
	if (UDT::ERROR == ret)
	{
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFnRev", "UDT::connect() failed!\n");
		UDT::close(fhandle);
		closesocket(myHttpOperate.m1_use_udp_sock);
		myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}

	myHttpOperate.m1_use_peer_ip = their_addr.sin_addr.s_addr;
	myHttpOperate.m1_use_peer_port = ntohs(their_addr.sin_port);
	myHttpOperate.m1_use_udt_sock = fhandle;
	myHttpOperate.m1_use_sock_type = SOCKET_TYPE_UDT;


	if (FALSE == g_InConnection2)
	{
				__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFnRev", "Enter ControlChannelLoop...\n");
		//============>
		ControlChannelLoop(myHttpOperate.m1_use_sock_type, myHttpOperate.m1_use_udt_sock);
		//============>
				__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFnRev", "Leave ControlChannelLoop!\n");
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFnRev", "###### g_InConnection2=true, drop this connection!\n");
	}


	UDT::close(myHttpOperate.m1_use_udt_sock);
	myHttpOperate.m1_use_udt_sock = INVALID_SOCKET;
	myHttpOperate.m1_use_sock_type = SOCKET_TYPE_UNKNOWN;
	closesocket(myHttpOperate.m1_use_udp_sock);////????
	myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;

	return 0;
}

static BOOL hasRoot()
{
	char buff[128];
	BOOL ret = TRUE;
	
	if (0 != access("/system/xbin/su", F_OK)) {
		if (0 != access("/system/bin/su", F_OK)) {
			if (0 != access("/su/bin/su", F_OK)) {
				ret = FALSE;
			}
		}
	}
	
	if (ret) {
		__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "system(su >) start...\n");
		system("su -c \"id\" > /data/local/tmp/root.txt");//wait...
		__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "system(su >) done!!!\n");
		FILE *fp = fopen("/data/local/tmp/root.txt", "r");
		if (NULL != fp) {
			__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "To read /data/local/tmp/root.txt...\n");
			strcpy(buff, "");
			fgets(buff, sizeof(buff), fp);
			__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "Read /data/local/tmp/root.txt: %s\n", buff);
			fclose(fp);
			if (NULL == strstr(buff, "(root)")) {
				ret = FALSE;
			}
			remove("/data/local/tmp/root.txt");
		}
	}
	
	if (ret) {
		__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "system(su >) start...\n");
		system("su -c \"id\" > /sdcard/root.txt");//wait...
		__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "system(su >) done!!!\n");
		FILE *fp = fopen("/sdcard/root.txt", "r");
		if (NULL != fp) {
			__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "To read /sdcard/root.txt...\n");
			strcpy(buff, "");
			fgets(buff, sizeof(buff), fp);
			__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "Read /sdcard/root.txt: %s\n", buff);
			fclose(fp);
			if (NULL == strstr(buff, "(root)")) {
				ret = FALSE;
			}
			remove("/sdcard/root.txt");
		}
	}
	
	__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "hasRoot() = %d\n", ret);
	return ret;
}

static BYTE getServerFuncFlags()
{
	BYTE ret = 0;
	ret |= FUNC_FLAGS_AV;
	ret |= FUNC_FLAGS_SHELL;
	if (hasRoot()) {
		ret |= FUNC_FLAGS_HASROOT;
	}
	if (g1_is_activated) {
		ret |= FUNC_FLAGS_ACTIVATED;
	}
	
	return ret;
}

static void learn_remote_addr(void *ctx, sockaddr* his_addr, int addr_len)
{
	sockaddr_in *sin = (sockaddr_in *)his_addr;
	myHttpOperate.m1_use_peer_port = ntohs(sin->sin_port);
	myHttpOperate.m1_use_peer_ip = sin->sin_addr.s_addr;
	__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "learn_remote_addr() called, %s[%d]\n", inet_ntoa(sin->sin_addr), myHttpOperate.m1_use_peer_port);
}

/* 为了RecvStream()运行安全，必须保证ControlChannelLoop()同一时间只被一个线程执行。*/
static int ControlChannelLoop(SOCKET_TYPE type, SOCKET fhandle)
{
	BOOL bAuthOK = FALSE;
	BOOL bQuitLoop = FALSE;
	char buff[16*1024];
	int ret;
	WORD wCommand;
	DWORD dwDataLength;
	WORD wTemp;
	char szValueData[MAX_LOADSTRING];
	char szPassEnc[PHP_MD5_OUTPUT_CHARS + 1];

	if_on_client_connected();
	if (SOCKET_TYPE_UDT == type) {
		UDT::register_learn_remote_addr(fhandle, learn_remote_addr, NULL);
	}

	while (FALSE == bQuitLoop && 
			(g_bDoConnection1 || g_bDoConnection2) )
	{
		ret = RecvStream(type, fhandle, buff, 6);
		if (ret != 0) {
			__android_log_print(ANDROID_LOG_INFO, "ControlChannelLoop", "RecvStream(6) failed!\n");
			break;
		}

		wCommand = ntohs(pf_get_word(buff+0));
		dwDataLength = ntohl(pf_get_dword(buff+2));


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
				//if (memcmp(myHttpOperate.m1_peer_node_id, buff, 6) != 0) {
				//	bQuitLoop = TRUE;
				//	break;
				//}

				ret = RecvStream(type, fhandle, buff, 4);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}

				ret = RecvStream(type, fhandle, buff, 256);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}

				if (if_read_password(szValueData, sizeof(szValueData)) == 0 && strcmp(szValueData, "") != 0)
				{
					php_md5(szValueData, szPassEnc, sizeof(szPassEnc));
					if (strcmp(szPassEnc, buff) != 0) {
						CtrlCmd_Send_HELLO_RESP(type, fhandle, myHttpOperate.m0_node_id, g0_version, getServerFuncFlags(), 0, CTRLCMD_RESULT_NG);
						__android_log_print(ANDROID_LOG_INFO, "ControlChannelLoop", "Password failed!\n");
						break;
					}
				}
				CtrlCmd_Send_HELLO_RESP(type, fhandle, myHttpOperate.m0_node_id, g0_version, getServerFuncFlags(), 0, CTRLCMD_RESULT_OK);
				__android_log_print(ANDROID_LOG_INFO, "ControlChannelLoop", "Password OK!\n");
				bAuthOK = TRUE;

				break;
				
			case CMD_CODE_RUN_REQ:
				FILE *pipe;
				int read_len;
				char *exe_cmd;
				exe_cmd = (char *)malloc(dwDataLength);////////
				ret = RecvStream(type, fhandle, exe_cmd, dwDataLength);
				if (ret != 0) {
					free(exe_cmd);////////
					bQuitLoop = TRUE;
					break;
				}
				__android_log_print(ANDROID_LOG_INFO, "ControlChannelLoop", "exe_cmd=%s\n", exe_cmd);
				if (strstr(exe_cmd, ">/dev/null") != NULL || strstr(exe_cmd, "> /dev/null") != NULL) {
					{
						system(exe_cmd);//wait...
						free(exe_cmd);////////
						CtrlCmd_Send_RUN_RESP(type, fhandle, "<No stdout>\n");
						break;
					}
				}
				pipe = popen(exe_cmd, "r");
				free(exe_cmd);////////
				if (NULL == pipe) {
					CtrlCmd_Send_RUN_RESP(type, fhandle, "popen() failed!\n");
					break;
				}
				__android_log_print(ANDROID_LOG_INFO, "ControlChannelLoop", "popen() OK!\n");
				read_len = 0;
				memset(buff, 0, sizeof(buff));
				while (fgets(buff + read_len, sizeof(buff) - read_len, pipe) != NULL) {
					read_len = strlen(buff);
					if (read_len >= sizeof(buff)-1) {
						break;
					}
				}
				__android_log_print(ANDROID_LOG_INFO, "ControlChannelLoop", "read_len=%d, buff=\n%s\n", read_len, buff);
				CtrlCmd_Send_RUN_RESP(type, fhandle, buff);
				pclose(pipe);
				__android_log_print(ANDROID_LOG_INFO, "ControlChannelLoop", "pclose() done!\n");
				break;
				
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

				DShowAV_Start((BYTE)(buff[0]), (BYTE)(buff[1]), (BYTE)(buff[2]), ntohl(pf_get_dword(buff+3)), ntohl(pf_get_dword(buff+7)) );

				break;

			case CMD_CODE_AV_STOP_REQ:
				DShowAV_Stop();
				CtrlCmd_Send_END(type, fhandle);
				break;

			case CMD_CODE_AV_SWITCH_REQ:
				ret = RecvStream(type, fhandle, buff, 4);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				DShowAV_Switch(ntohl(pf_get_dword(buff+0)));
				break;

			case CMD_CODE_AV_CONTRL_REQ:
				ret = RecvStream(type, fhandle, buff, 2+4);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				DShowAV_Contrl(ntohs(pf_get_word(buff+0)), ntohl(pf_get_dword(buff+2)));
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
				Play_Voice(voice_data, dwDataLength);
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
				__android_log_print(ANDROID_LOG_INFO, "ControlChannelLoop", "Recv CMD_CODE_NULL !!!!!!\n");
				break;

			case CMD_CODE_BYE_REQ:
				__android_log_print(ANDROID_LOG_INFO, "ControlChannelLoop", "Recv CMD_CODE_BYE_REQ !!!!!!\n");
				bQuitLoop = TRUE;
				break;
			default:
				bQuitLoop = TRUE;
				break;
		}
	} /* while */

	if (SOCKET_TYPE_UDT == type) {
		UDT::unregister_learn_remote_addr(fhandle);
	}
	if_contrl_flash_off();
	DShowAV_Stop();
	if (strncmp(g0_device_uuid, "ANDROID@UAV@", 12) == 0) {
		if_on_mavlink_stop();
	}
	if_on_client_disconnected();
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int StartNativeMain(const char *client_charset, const char *client_lang)
{
	if (NULL != client_charset)  strncpy(g_client_charset, client_charset, sizeof(g_client_charset));
	if (NULL != client_lang)     strncpy(g_client_lang,    client_lang,    sizeof(g_client_lang));
	
	g_bFirstCheckStun = TRUE;
	
	g_InConnection1 = FALSE;
	g_InConnection2 = FALSE;
	
	// use this function to initialize the UDT library
	UdtStartup();
	
	CtrlCmd_Init();
	
	InitVar();
}

void StopNativeMain()
{
	g_bDoConnection1 = FALSE;
	g_bDoConnection2 = FALSE;
	
	myUPnP.RemoveNATPortMapping(mapping);
	
	CtrlCmd_Uninit();
	
	//__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "UdtCleanup()...\n");
	//UdtCleanup();
}

void StartDoConnection()
{
	g_bDoConnection1 = TRUE;
	g_bDoConnection2 = FALSE;
	
	NativeMainFunc(NULL);//Test
}

void StopDoConnection()
{
	if (g_bDoConnection1) {
		__android_log_print(ANDROID_LOG_INFO, "StopDoConnection", "DoUnregister()...\n");
		pthread_create(&g_hUnregisterThread, NULL, UnregisterThreadFn, NULL);
	}
	g_bDoConnection1 = FALSE;
	g_bDoConnection2 = FALSE;
	
	if_contrl_flash_off();
	DShowAV_Stop();
	if (strncmp(g0_device_uuid, "ANDROID@UAV@", 12) == 0) {
		if_on_mavlink_stop();
	}
}

void ForceDisconnect()
{
	__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "ForceDisconnect()...\n");
	
	if (myHttpOperate.m1_use_sock_type == SOCKET_TYPE_UDT)
	{
		UDT::unregister_learn_remote_addr(myHttpOperate.m1_use_udt_sock);
		
		UdtClose(myHttpOperate.m1_use_udt_sock);
		myHttpOperate.m1_use_udt_sock = INVALID_SOCKET;
		myHttpOperate.m1_use_sock_type = SOCKET_TYPE_UNKNOWN;
		UdpClose(myHttpOperate.m1_use_udp_sock);
		myHttpOperate.m1_use_udp_sock = INVALID_SOCKET;
	}
	else if (myHttpOperate.m1_use_sock_type == SOCKET_TYPE_TCP)
	{
		closesocket(myHttpOperate.m1_use_udt_sock);
		myHttpOperate.m1_use_udt_sock = INVALID_SOCKET;
		myHttpOperate.m1_use_sock_type = SOCKET_TYPE_UNKNOWN;
	}
}

void runNativeShellCmd(const char *exe_cmd)
{
	system(exe_cmd);//wait...
}

void *RunCmdThreadFn(void *pvThreadParam)
{
	char *exe_cmd = (char *)pvThreadParam;
	system(exe_cmd);//wait...
	free(pvThreadParam);
}

void runNativeShellCmdNoWait(const char *exe_cmd)
{
	char *param = (char *)malloc(strlen(exe_cmd) + 1);
	strcpy(param, exe_cmd);
	
	pthread_t hRunCmdThread = 0;
	pthread_create(&hRunCmdThread, NULL, RunCmdThreadFn, param);
}


#ifdef ANDROID_NDK

extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(StartNativeMain)
	(JNIEnv* env, jobject thiz, jstring str_client_charset, jstring str_client_lang, jstring str_app_package_name)
{
	int len;
	
	len = (env)->GetStringLength(str_client_charset);
	assert(len < sizeof(g_client_charset));
	(env)->GetStringUTFRegion(str_client_charset, 0, len, g_client_charset);
	g_client_charset[len] = '\0';
	
	len = (env)->GetStringLength(str_client_lang);
	assert(len < sizeof(g_client_lang));
	(env)->GetStringUTFRegion(str_client_lang, 0, len, g_client_lang);
	g_client_lang[len] = '\0';
	
	len = (env)->GetStringLength(str_app_package_name);
	assert(len < sizeof(g_app_package_name));
	(env)->GetStringUTFRegion(str_app_package_name, 0, len, g_app_package_name);
	g_app_package_name[len] = '\0';
	
	return StartNativeMain(NULL, NULL);
}

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(StopNativeMain)
	(JNIEnv* env, jobject thiz)
{
	StopNativeMain();
}

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(StartDoConnection)
	(JNIEnv* env, jobject thiz)
{
	StartDoConnection();
}

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(StopDoConnection)
	(JNIEnv* env, jobject thiz)
{
	StopDoConnection();
}

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(ForceDisconnect)
	(JNIEnv* env, jobject thiz)
{
	ForceDisconnect();
}

extern "C"
jboolean
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(hasRootPermission)
	(JNIEnv* env, jobject thiz)
{
	return (jboolean)hasRoot();
}

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(runNativeShellCmd)
	(JNIEnv* env, jobject thiz, jstring strCmd)
{
	const char *cmd;
	
	cmd = (env)->GetStringUTFChars(strCmd, NULL);
	
	runNativeShellCmd(cmd);
	
	(env)->ReleaseStringUTFChars(strCmd, cmd);
}

extern "C"
void
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(runNativeShellCmdNoWait)
	(JNIEnv* env, jobject thiz, jstring strCmd)
{
	const char *cmd;
	
	cmd = (env)->GetStringUTFChars(strCmd, NULL);
	
	runNativeShellCmdNoWait(cmd);
	
	(env)->ReleaseStringUTFChars(strCmd, cmd);
}


#endif //ifdef ANDROID_NDK
