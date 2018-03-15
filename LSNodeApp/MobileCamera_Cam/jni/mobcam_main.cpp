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
#include "ProxyServer.h"
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
MyUPnP  myUPnP;
UPNPNAT_MAPPING mapping;


static pthread_t g_hNativeMainThread = 0;
static BOOL g_bQuitNativeMain = FALSE;


static BOOL g_bFirstCheckStun = TRUE;
static BOOL g_bOnlineSuccess = FALSE;

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
static pthread_t g_hWorkingThread1_Proxy = 0;
static pthread_t g_hWorkingThread1_ProxyTcp = 0;
static pthread_t g_hWorkingThread2 = 0;

void *UnregisterThreadFn(void *pvThreadParam);
void *WorkingThreadFn1(void *pvThreadParam);
void *WorkingThreadFnRev(void *pvThreadParam);
void *WorkingThreadFn1_Proxy(void *pvThreadParam);
void *WorkingThreadFn1_ProxyTcp(void *pvThreadParam);
void *WorkingThreadFn2(void *pvThreadParam);

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
		g0_ipCount = ipCountTemp;
		memcpy(g0_ipArray, ipArrayTemp, sizeof(DWORD)*ipCountTemp);
		g0_port = FIRST_CONNECT_PORT;
		__android_log_print(ANDROID_LOG_INFO, "GetLocalInformation", "GetLocalAddress: ipCount=%d\n", g0_ipCount);
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
	if (strncmp(g0_device_uuid, "ANDROID@YKZ@", 12) == 0) {
		if (strcmp(g0_device_uuid + 12 + 17, "@0") != 0) {
			__android_log_print(ANDROID_LOG_INFO, "InitVar", "if_get_device_uuid() invalid!!!\n");
			strcpy(g0_device_uuid, "");
		}
	}
	else if (strncmp(g0_device_uuid, "ANDROID@ROB@", 12) == 0) {
		if (strcmp(g0_device_uuid + 12 + 17, "@1") != 0) {
			__android_log_print(ANDROID_LOG_INFO, "InitVar", "if_get_device_uuid() invalid!!!\n");
			strcpy(g0_device_uuid, "");
		}
	}
	else if (strncmp(g0_device_uuid, "ANDROID@UAV@", 12) == 0) {
		if (strcmp(g0_device_uuid + 12 + 17, "@1") != 0) {
			__android_log_print(ANDROID_LOG_INFO, "InitVar", "if_get_device_uuid() invalid!!!\n");
			strcpy(g0_device_uuid, "");
		}
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, "InitVar", "if_get_device_uuid() invalid!!!\n");
		strcpy(g0_device_uuid, "");
	}
	
	
	char szNodeId[20];
	if_get_nodeid(szNodeId, sizeof(szNodeId));
	__android_log_print(ANDROID_LOG_INFO, "InitVar", "if_get_nodeid() = %s\n", szNodeId);
	mac_addr(szNodeId, g0_node_id, NULL);
	
	
	g0_version = MYSELF_VERSION;


	strncpy(g1_http_server, DEFAULT_HTTP_SERVER, sizeof(g1_http_server));
	strncpy(g1_stun_server, DEFAULT_STUN_SERVER, sizeof(g1_stun_server));
	g1_http_client_ip = 0;

	g0_no_nat = false;
	g0_nat_type = StunTypeUnknown;
	g0_pub_ip = 0;
	g0_pub_port = 0;

	g0_is_admin = false;
	g0_is_busy = false;
	g0_audio_channels = if_get_audio_channels();
	g0_video_channels = if_get_video_channels();
	g1_register_period = DEFAULT_REGISTER_PERIOD;  /* Seconds */
	g1_expire_period = DEFAULT_EXPIRE_PERIOD;  /* Seconds */
	
	g1_lowest_level_for_av = 0;
	g1_lowest_level_for_vnc = 0;
	g1_lowest_level_for_ft = 0;
	g1_lowest_level_for_adb = 0;
	g1_lowest_level_for_webmoni = 0;
	g1_lowest_level_allow_hide = 0;
	
	g1_is_activated = true;
	g1_comments_id = 0;
	
	g1_use_udp_sock = INVALID_SOCKET;
	g1_use_udt_sock = INVALID_SOCKET;
	g1_use_sock_type = SOCKET_TYPE_UNKNOWN;
}

void *NativeMainFunc(void *lpParameter)
{
	int ret;
	int last_register_time = 0, now_time;
	BOOL bShouldStop = TRUE;
	StunAddress4 mapped;
	BOOL bNoNAT;
	int  nNatType;


	__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "UdtStartup()\n");

	// use this function to initialize the UDT library
	UdtStartup();

	InitVar();


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


    pthread_create(&g_hWorkingThread2, NULL, WorkingThreadFn2, NULL);


	while (FALSE == g_bQuitNativeMain)
	{
		now_time = time(NULL);
		if (now_time - last_register_time < g1_register_period) {
			usleep((g1_register_period - (now_time - last_register_time)) * 1000000);
		}
		last_register_time = time(NULL);

		if (g_InConnection2) {
			continue;
		}

		GetLocalInformation();


		if (bShouldStop && g_bDoConnection1)
		{
			ret = DoRegister1(g_client_charset, g_client_lang);
			__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "DoRegister1(%s, %s) = %d\n", g_client_charset, g_client_lang, ret);
			//如果DoRegister1成功，下一轮可以立即DoRegister2
			if (ret == 2) {
				last_register_time = 0;
			}
		}
		else
		{
			if (g_bDoConnection1)
			{
				/* STUN Information */
				if (strcmp(g1_stun_server, "NONE") == 0)
				{
					ret = CheckStunMyself(g1_http_server, FIRST_CONNECT_PORT, &mapped, &bNoNAT, &nNatType);
					if (ret == -1) {
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStunMyself() failed!\n");
						
					   mapped.addr = ntohl(g1_http_client_ip);
					   mapped.port = FIRST_CONNECT_PORT;
					   
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
					   
						g0_pub_ip = htonl(mapped.addr);
						g0_pub_port = mapped.port;
						g0_no_nat = bNoNAT;
						g0_nat_type = nNatType;
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStunHttp: %d.%d.%d.%d[%d], NoNAT=%d\n", 
							(g0_pub_ip & 0x000000ff) >> 0,
							(g0_pub_ip & 0x0000ff00) >> 8,
							(g0_pub_ip & 0x00ff0000) >> 16,
							(g0_pub_ip & 0xff000000) >> 24,
							g0_pub_port,  g0_no_nat ? 1 : 0);
					}
					else {
						g0_pub_ip = htonl(mapped.addr);
						g0_pub_port = mapped.port;
						g0_no_nat = bNoNAT;
						g0_nat_type = nNatType;
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStunMyself: %d.%d.%d.%d[%d]\n", 
							(g0_pub_ip & 0x000000ff) >> 0,
							(g0_pub_ip & 0x0000ff00) >> 8,
							(g0_pub_ip & 0x00ff0000) >> 16,
							(g0_pub_ip & 0xff000000) >> 24,
							g0_pub_port);
					}
				}
				else if (g_bFirstCheckStun) {
					ret = CheckStun(g1_stun_server, FIRST_CONNECT_PORT, &mapped, &bNoNAT, &nNatType);
					if (ret == -1) {
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStun() failed!\n");
						
					   mapped.addr = ntohl(g1_http_client_ip);
					   mapped.port = FIRST_CONNECT_PORT;
					   
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
					   
						g0_pub_ip = htonl(mapped.addr);
						g0_pub_port = mapped.port;
						g0_no_nat = bNoNAT;
						g0_nat_type = nNatType;
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStunHttp: %d.%d.%d.%d[%d], NoNAT=%d\n", 
							(g0_pub_ip & 0x000000ff) >> 0,
							(g0_pub_ip & 0x0000ff00) >> 8,
							(g0_pub_ip & 0x00ff0000) >> 16,
							(g0_pub_ip & 0xff000000) >> 24,
							g0_pub_port,  g0_no_nat ? 1 : 0);
					}
					else if (ret == 0) {
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStun() blocked!\n");
						g0_pub_ip = 0;
						g0_pub_port = 0;
						g0_no_nat = FALSE;
						g0_nat_type = nNatType;
					}
					else {
						g_bFirstCheckStun = FALSE;
						g0_pub_ip = htonl(mapped.addr);
						g0_pub_port = mapped.port;
						g0_no_nat = bNoNAT;
						g0_nat_type = nNatType;
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStun: %d.%d.%d.%d[%d]\n", 
							(g0_pub_ip & 0x000000ff) >> 0,
							(g0_pub_ip & 0x0000ff00) >> 8,
							(g0_pub_ip & 0x00ff0000) >> 16,
							(g0_pub_ip & 0xff000000) >> 24,
							g0_pub_port);
					}
				}
				else {
					ret = CheckStunSimple(g1_stun_server, FIRST_CONNECT_PORT, &mapped);
					if (ret == -1) {
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStunSimple() failed!\n");
						mapped.addr = ntohl(g1_http_client_ip);
						mapped.port = FIRST_CONNECT_PORT;
						g0_pub_ip = htonl(mapped.addr);
						g0_pub_port = mapped.port;
						g_bFirstCheckStun = TRUE;
					}
					else {
						g0_pub_ip = htonl(mapped.addr);
						g0_pub_port = mapped.port;
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "CheckStunSimple: %d.%d.%d.%d[%d]\n", 
							(g0_pub_ip & 0x000000ff) >> 0,
							(g0_pub_ip & 0x0000ff00) >> 8,
							(g0_pub_ip & 0x00ff0000) >> 16,
							(g0_pub_ip & 0xff000000) >> 24,
							g0_pub_port);
					}
				}
				
				
				/* 本地路由器UPnP处理*/
				if (TRUE == if_should_do_upnp() && FALSE == bNoNAT)
				{
					char extIp[16];
					strcpy(extIp, "");
					myUPnP.GetNATExternalIp(extIp);
					if (strlen(extIp) > 0 && 0 != g0_pub_ip && inet_addr(extIp) == g0_pub_ip)
					{
						mapping.description = "UP2P";
						//mapping.protocol = ;
						//mapping.externalPort = ;
						mapping.internalPort = SECOND_CONNECT_PORT;
						if (UNAT_OK == myUPnP.AddNATPortMapping(&mapping, false))
						{
							g_bFirstCheckStun = FALSE;

							g0_pub_ip = inet_addr(extIp);
							g0_pub_port = mapping.externalPort;
							g0_no_nat = TRUE;
							g0_nat_type = StunTypeOpen;

							__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "UPnP AddPortMapping OK: %s[%d] --> %s[%d]", extIp, mapping.externalPort, myUPnP.GetLocalIPStr().c_str(), mapping.internalPort);
						}
						else
						{
							BOOL bTempExists = FALSE;
							myUPnP.GetNATSpecificEntry(&mapping, &bTempExists);
							if (bTempExists)
							{
								g_bFirstCheckStun = FALSE;

								g0_pub_ip = inet_addr(extIp);
								g0_pub_port = mapping.externalPort;
								g0_no_nat = TRUE;
								g0_nat_type = StunTypeOpen;

								__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "UPnP PortMapping Exists: %s[%d] --> %s[%d]", extIp, mapping.externalPort, myUPnP.GetLocalIPStr().c_str(), mapping.internalPort);
							}
							else {
								g0_pub_ip = htonl(mapped.addr);
								g0_pub_port = mapped.port;
								g0_no_nat = bNoNAT;
								g0_nat_type = nNatType;
							}
						}
					}
					else {
						__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "UPnP PortMapping Skipped: %s --> %s", extIp, myUPnP.GetLocalIPStr().c_str());
						g0_pub_ip = htonl(mapped.addr);
						g0_pub_port = mapped.port;
						g0_no_nat = bNoNAT;
						g0_nat_type = nNatType;
					}
				}
				
				//修正公网IP受控端的连接端口问题,2014-6-15
				if (bNoNAT) {
					g0_pub_port = SECOND_CONNECT_PORT;
				}

				if (g0_no_nat) {
					g0_port = SECOND_CONNECT_PORT;
				}

				ret = DoRegister2(g_client_charset, g_client_lang);
				__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "DoRegister2() = %d\n", ret);
				/* 如果DoRegister2()返回正常，则执行一次DoOnline() */
				if (ret != -1 && ret != 0 && ret != 1 && g1_comments_id > 0) {
					if (FALSE == g_bOnlineSuccess) {
						//DoOnline();
						g_bOnlineSuccess = TRUE;
					}
					if_on_register_result(g1_comments_id, g1_is_activated, g1_is_activated || 0 == g1_lowest_level_allow_hide);
				}

			}
			else {
				ret = 0;
			}
		}

		if (ret == -1) {
			if_on_register_network_error();
			//if (g_bDoConnection1) OnDailerEvent();//宽带拨号，g_bDoConnection1==TRUE
			g_bFirstCheckStun = TRUE;//可能已切换网络
			continue;
		}
		else if (ret == 0) {
			/* Exit this connection mode. */
			g_bDoConnection1 = FALSE;
			continue;
		}
		else if (ret == 1) {
			bShouldStop = TRUE;
			continue;
		}

		bShouldStop = FALSE;

		/* Connection */
		if (ret == 3) {

			if (g1_peer_pri_ipCount != 0 && g1_peer_pri_port != 0)  /* Accept */
			{
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
			else if (g1_peer_pri_ipCount == 0 && g1_peer_pri_port == SECOND_CONNECT_PORT)  /* AcceptRev */
			{
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
			else if (g1_peer_pri_ipCount == 0 && g1_peer_pri_port == 0)  /* AcceptProxy */
			{
				g_InConnection1 = TRUE;

				//hThread = ::CreateThread(NULL,0,WorkingThreadFn1_Proxy,NULL,0,&dwThreadID);
				pthread_create(&g_hWorkingThread1_Proxy, NULL, WorkingThreadFn1_Proxy, NULL);


				if (g_bDoConnection1) {
					__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "DoUnregister()...\n");
					pthread_create(&g_hUnregisterThread, NULL, UnregisterThreadFn, NULL);
				}

				/* Wait... */
				pthread_join(g_hWorkingThread1_Proxy, NULL);
				g_hWorkingThread1_Proxy = 0;
				__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "WorkingThread ends, continue to do Register loop...\n");

				g_InConnection1 = FALSE;
			}
			else if (g1_peer_pri_ipCount == 0 && g1_peer_pri_port != 0) /* AcceptProxyTcp */
			{
				g_InConnection1 = TRUE;

				//hThread = ::CreateThread(NULL,0,WorkingThreadFn1_ProxyTcp,NULL,0,&dwThreadID);
				pthread_create(&g_hWorkingThread1_ProxyTcp, NULL, WorkingThreadFn1_ProxyTcp, NULL);


				if (g_bDoConnection1) {
					__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "DoUnregister()...\n");
					pthread_create(&g_hUnregisterThread, NULL, UnregisterThreadFn, NULL);
				}

				/* Wait... */
				pthread_join(g_hWorkingThread1_ProxyTcp, NULL);
				g_hWorkingThread1_ProxyTcp = 0;
				__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "WorkingThread ends, continue to do Register loop...\n");

				g_InConnection1 = FALSE;
			}

			/* 退出连接线程后，重新Register到Http服务器上。*/
		}

	}  /* while (1) */

	if (g_bDoConnection1) {
		__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "DoUnregister()...\n");
		pthread_create(&g_hUnregisterThread, NULL, UnregisterThreadFn, NULL);
	}

	if (0 != g_hWorkingThread2)
	{
		//Waiting...
		__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "pthread_join(WorkingThread2)...\n");
		pthread_join(g_hWorkingThread2, NULL);
		g_hWorkingThread2 = 0;
	}

	//__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "UdtCleanup()...\n");
	//UdtCleanup();

	__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "Exit main function!\n");
	return 0;
}

void *UnregisterThreadFn(void *pvThreadParam)
{
	DoUnregister(g_client_charset, g_client_lang);
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

	g1_use_udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (g1_use_udp_sock == INVALID_SOCKET) {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1", "UDP socket() failed!\n");
		return 0;
	}

	int flag = 1;
    setsockopt(g1_use_udp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(FIRST_CONNECT_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(g1_use_udp_sock, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1", "UDP bind() failed!\n");
		closesocket(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}

	if (FindOutConnection(g1_use_udp_sock, g0_node_id, g1_peer_node_id,
							g1_peer_pri_ipArray, g1_peer_pri_ipCount, g1_peer_pri_port,
							g1_peer_ip, g1_peer_port, &use_ip, &use_port) != 0) {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1", "FindOutConnection() failed!\n");
		closesocket(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
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

	if (UDT::ERROR == UDT::bind(fhandle, g1_use_udp_sock))
	{
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1", "UDT::bind() failed!\n");
		UDT::close(fhandle);
		closesocket(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
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
		closesocket(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}

	g1_use_peer_ip = their_addr.sin_addr.s_addr;
	g1_use_peer_port = ntohs(their_addr.sin_port);
	g1_use_udt_sock = fhandle;
	g1_use_sock_type = SOCKET_TYPE_UDT;


	if (FALSE == g_InConnection2)
	{
				__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1", "Enter ControlChannelLoop...\n");
		//============>
		ControlChannelLoop(g1_use_sock_type, g1_use_udt_sock);
		//============>
				__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1", "Leave ControlChannelLoop!\n");
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1", "###### g_InConnection2=true, drop this connection!\n");
	}


	UDT::close(g1_use_udt_sock);
	g1_use_udt_sock = INVALID_SOCKET;
	g1_use_sock_type = SOCKET_TYPE_UNKNOWN;
	closesocket(g1_use_udp_sock);////????
	g1_use_udp_sock = INVALID_SOCKET;

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

	g1_use_udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (g1_use_udp_sock == INVALID_SOCKET) {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFnRev", "UDP socket() failed!\n");
		return 0;
	}

	int flag = 1;
    setsockopt(g1_use_udp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(0);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(g1_use_udp_sock, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFnRev", "UDP bind() failed!\n");
		closesocket(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}


	fhandle = UDT::socket(AF_INET, SOCK_STREAM, 0);

	ConfigUdtSocket(fhandle);

	if (UDT::ERROR == UDT::bind(fhandle, g1_use_udp_sock))
	{
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFnRev", "UDT::bind() failed!\n");
		UDT::close(fhandle);
		closesocket(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(g1_peer_port);
	their_addr.sin_addr.s_addr = g1_peer_ip;
	memset(&(their_addr.sin_zero), '\0', 8);
	nRetry = UDT_CONNECT_TIMES;
	ret = UDT::ERROR;
	while (nRetry-- > 0 && ret == UDT::ERROR) {
#if 1 ////Debug
	__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFnRev", "@@@ UDT::connect()... ip/port: %d.%d.%d.%d[%d]\n", 
		(g1_peer_ip & 0x000000ff) >> 0,
		(g1_peer_ip & 0x0000ff00) >> 8,
		(g1_peer_ip & 0x00ff0000) >> 16,
		(g1_peer_ip & 0xff000000) >> 24,
		g1_peer_port);
#endif
		ret = UDT::connect(fhandle, (sockaddr*)&their_addr, sizeof(their_addr));
	}
	if (UDT::ERROR == ret)
	{
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFnRev", "UDT::connect() failed!\n");
		UDT::close(fhandle);
		closesocket(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}

	g1_use_peer_ip = their_addr.sin_addr.s_addr;
	g1_use_peer_port = ntohs(their_addr.sin_port);
	g1_use_udt_sock = fhandle;
	g1_use_sock_type = SOCKET_TYPE_UDT;


	if (FALSE == g_InConnection2)
	{
				__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFnRev", "Enter ControlChannelLoop...\n");
		//============>
		ControlChannelLoop(g1_use_sock_type, g1_use_udt_sock);
		//============>
				__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFnRev", "Leave ControlChannelLoop!\n");
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFnRev", "###### g_InConnection2=true, drop this connection!\n");
	}


	UDT::close(g1_use_udt_sock);
	g1_use_udt_sock = INVALID_SOCKET;
	g1_use_sock_type = SOCKET_TYPE_UNKNOWN;
	closesocket(g1_use_udp_sock);////????
	g1_use_udp_sock = INVALID_SOCKET;

	return 0;
}

void *WorkingThreadFn1_Proxy(void *pvThreadParam)
{
	UDTSOCKET fhandle;
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);
	int ret;
	int nRetry;


	__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_Proxy", "WorkingThreadFn1_Proxy()...\n");

	g1_use_udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (g1_use_udp_sock == INVALID_SOCKET) {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_Proxy", "UDP socket() failed!\n");
		return 0;
	}

	int flag = 1;
    setsockopt(g1_use_udp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(FIRST_CONNECT_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(g1_use_udp_sock, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_Proxy", "UDP bind() failed!\n");
		closesocket(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}

	if (WaitForProxyReady(g1_use_udp_sock, g0_node_id, g1_peer_ip, g1_peer_port) < 0) {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_Proxy", "WaitForProxyReady() failed!\n");
		closesocket(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}

	__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_Proxy", "WaitForProxyReady() OK!\n");


	fhandle = UDT::socket(AF_INET, SOCK_STREAM, 0);

	ConfigUdtSocket(fhandle);

	if (UDT::ERROR == UDT::bind(fhandle, g1_use_udp_sock))
	{
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_Proxy", "UDT::bind() failed!\n");
		UDT::close(fhandle);
		closesocket(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(g1_peer_port);
	their_addr.sin_addr.s_addr = g1_peer_ip;
	memset(&(their_addr.sin_zero), '\0', 8);
	nRetry = UDT_CONNECT_TIMES;
	ret = UDT::ERROR;
	while (nRetry-- > 0 && ret == UDT::ERROR) {
		//TRACE("@@@ UDT::connect()...\n");
		ret = UDT::connect(fhandle, (sockaddr*)&their_addr, sizeof(their_addr));
	}
	if (UDT::ERROR == ret)
	{
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_Proxy", "UDT::connect() failed!\n");
		UDT::close(fhandle);
		closesocket(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
		return 0;
	}

	g1_use_peer_ip = their_addr.sin_addr.s_addr;
	g1_use_peer_port = ntohs(their_addr.sin_port);
	g1_use_udt_sock = fhandle;
	g1_use_sock_type = SOCKET_TYPE_UDT;


	if (FALSE == g_InConnection2)
	{
				__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_Proxy", "Enter ControlChannelLoop...\n");
		//============>
		ControlChannelLoop(g1_use_sock_type, g1_use_udt_sock);
		//============>
				__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_Proxy", "Leave ControlChannelLoop!\n");
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_Proxy", "###### g_InConnection2=true, drop this connection!\n");
	}

	EndProxy(g1_use_udp_sock, g1_peer_ip, g1_peer_port);


	UDT::close(g1_use_udt_sock);
	g1_use_udt_sock = INVALID_SOCKET;
	g1_use_sock_type = SOCKET_TYPE_UNKNOWN;
	closesocket(g1_use_udp_sock);////????
	g1_use_udp_sock = INVALID_SOCKET;

	return 0;
}

void *WorkingThreadFn1_ProxyTcp(void *pvThreadParam)
{
	SOCKET fhandle;
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);
	int ret;
	int nRetry;


	__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_ProxyTcp", "WorkingThreadFn1_ProxyTcp()...\n");

	fhandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fhandle == INVALID_SOCKET) {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_ProxyTcp", "TCP socket() failed!\n");
		return 0;
	}

	struct timeval timeo = {5, 0};
	setsockopt(fhandle, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(0);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(fhandle, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_ProxyTcp", "TCP bind() failed!\n");
		closesocket(fhandle);
		fhandle = INVALID_SOCKET;
		return 0;
	}


	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(g1_peer_port);
	their_addr.sin_addr.s_addr = g1_peer_ip;
	memset(&(their_addr.sin_zero), '\0', 8);
	nRetry = 2;
	ret = -1;
	while (nRetry-- > 0 && ret < 0) {
		//TRACE("@@@ TCP::connect()...\n");
		ret = connect(fhandle, (sockaddr*)&their_addr, sizeof(their_addr));
	}
	if (ret < 0)
	{
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_ProxyTcp", "TCP::connect() failed!\n");
		closesocket(fhandle);
		fhandle = INVALID_SOCKET;
		return 0;
	}

	g1_use_peer_ip = their_addr.sin_addr.s_addr;
	g1_use_peer_port = ntohs(their_addr.sin_port);
	g1_use_udt_sock = fhandle;
	g1_use_sock_type = SOCKET_TYPE_TCP;

	__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_ProxyTcp", "TCP::connect() OK, use ip/port = %d.%d.%d.%d[%d]\n", 
		(g1_use_peer_ip & 0x000000ff) >> 0,
		(g1_use_peer_ip & 0x0000ff00) >> 8,
		(g1_use_peer_ip & 0x00ff0000) >> 16,
		(g1_use_peer_ip & 0xff000000) >> 24,
		g1_use_peer_port);


	if (FALSE == g_InConnection2)
	{
				__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_ProxyTcp", "Enter ControlChannelLoop...\n");
		//============>
		ControlChannelLoop(g1_use_sock_type, g1_use_udt_sock);
		//============>
				__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_ProxyTcp", "Leave ControlChannelLoop!\n");
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn1_ProxyTcp", "###### g_InConnection2=true, drop this connection!\n");
	}


	closesocket(g1_use_udt_sock);
	g1_use_udt_sock = INVALID_SOCKET;
	g1_use_sock_type = SOCKET_TYPE_UNKNOWN;
	//closesocket(g1_use_udp_sock);
	g1_use_udp_sock = INVALID_SOCKET;

	return 0;
}

void RecvUdpDiscoveryRequest(SOCKET my_sock, char *data, int len, sockaddr *his_addr, int addr_len)
{
	if (FALSE == g_bDoConnection2) {
		return;
	}
	
	/* 正在连接时，不响应检索包。*/
	if (g_InConnection1 || g_InConnection2) {
		return;
	}

	if (len < 270) {
		return;
	}

	WORD wMagic = ntohs(pf_get_word(data + 0));
	if (wMagic != DMSG_MAGIC_VALUE) {
		return;
	}

	WORD wType = ntohs(pf_get_word(data + 2));
	if (wType != DMSG_TYPE_DISCOVERY_REQ) {
		return;
	}

	BYTE client_node_id[6];
	char client_node_name[256];
	DWORD client_version;

	memcpy(client_node_id, data + 4, 6);
	memcpy(client_node_name, data + 10, 256);
	client_version = ntohl(pf_get_dword(data + 266));


	char send_buff[1024*2];
	char row_value[1024*2];

	snprintf(row_value, sizeof(row_value), "%02X-%02X-%02X-%02X-%02X-%02X|%s|%ld|%s|%d|%s|%d|%d|%d|%d|%d|%d|%d|%s|%s|%d|%s", 
		g0_node_id[0], g0_node_id[1], g0_node_id[2], g0_node_id[3], g0_node_id[4], g0_node_id[5], 
		g0_node_name, 
		g0_version, 
		"0.0.0.0", 
		0/*g0_port*/, 
		"0.0.0.0", 
		0/*g0_pub_port*/, 
		1 /*(g0_no_nat ? 1 : 0)*/, 
		StunTypeOpen /*g0_nat_type*/,
		(g0_is_admin ? 1 : 0),
		(g0_is_busy ? 1 : 0),
		g0_audio_channels,
		g0_video_channels,
		g0_os_info, 
		g0_device_uuid, 
		g1_comments_id,/*comments_id*/
		ANYPC_LOCAL_LAN);

	pf_set_word(send_buff + 0, htons(DMSG_MAGIC_VALUE));
	pf_set_word(send_buff + 2, htons(DMSG_TYPE_DISCOVERY_RESP));
	memcpy(send_buff + 4, row_value, strlen(row_value) + 1);

	int ret = sendto(my_sock, send_buff, 4 + strlen(row_value) + 1, 0, his_addr, addr_len);
	if (ret <= 0) {
		__android_log_print(ANDROID_LOG_INFO, "RecvUdpDiscoveryRequest", "sendto() failed!\n");
	}
	else {
		__android_log_print(ANDROID_LOG_INFO, "RecvUdpDiscoveryRequest", "Send response OK!\n");
		__android_log_print(ANDROID_LOG_INFO, "RecvUdpDiscoveryRequest", row_value);
	}
}

void *WorkingThreadFn2(void *pvThreadParam)
{
	SOCKET udp_sock;
	UDTSOCKET serv;
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);
	UDTSOCKET fhandle;
	int ret;


	udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udp_sock == INVALID_SOCKET) {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn2", "UDP socket() failed!\n");
		return 0;
	}

	int flag = 1;
    setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(SECOND_CONNECT_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);
	if (bind(udp_sock, (sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
		__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn2", "UDP bind() failed!\n");
		closesocket(udp_sock);
		return 0;
	}

	while (FALSE == g_bQuitNativeMain)
	{
		usleep(500000);  /* 2010-06-24 */

		serv = UDT::socket(AF_INET, SOCK_STREAM, 0);
		UDT::register_direct_udp_recv(serv, RecvUdpDiscoveryRequest);
		
		ConfigUdtSocket(serv);

		if (UDT::ERROR == UDT::bind(serv, udp_sock))
		{
			__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn2", "UDT::bind() failed!\n");
			UDT::close(serv);

			//////////////////////////////////////////////////////////////////
			closesocket(udp_sock);
			udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
			my_addr.sin_family = AF_INET;
			my_addr.sin_port = htons(SECOND_CONNECT_PORT);
			my_addr.sin_addr.s_addr = INADDR_ANY;
			memset(&(my_addr.sin_zero), '\0', 8);
			bind(udp_sock, (sockaddr *)&my_addr, sizeof(my_addr));
			//////////////////////////////////////////////////////////////////

			continue;
		}

		if (UDT::ERROR == UDT::listen(serv, 1))
		{
			__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn2", "UDT::listen() failed!\n");
			UDT::close(serv);
			continue;
		}

#if 1
	///////////////
		struct timeval waitval;
		waitval.tv_sec  = 60;
		waitval.tv_usec = 0;
		ret = UDT::select(serv + 1, serv, NULL, NULL, &waitval);
		if (ret == UDT::ERROR || ret == 0) {
			__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn2", "UDT::select() failed/timeout!\n");
			UDT::close(serv);
			continue;
		}
	//////////////////
#endif

		if ((fhandle = UDT::accept(serv, (sockaddr*)&their_addr, &namelen)) == UDT::INVALID_SOCK) {
			__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn2", "UDT::accept() failed!\n");
			UDT::close(serv);
			continue;
		}
		else {
			__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn2", "UDT::accept() OK!\n");

			UDT::close(serv);

			if (FALSE == g_InConnection1)
			{
				g_InConnection2 = TRUE;

				/* 即将进入连接处理，如果启用服务器连接模式，则使其在服务器列表内不可见。*/
				if (g_bDoConnection1) {
					__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn2", "DoUnregister()...\n");
					pthread_create(&g_hUnregisterThread, NULL, UnregisterThreadFn, NULL);
				}


				g1_use_udp_sock = udp_sock;
				g1_use_peer_ip = their_addr.sin_addr.s_addr;
				g1_use_peer_port = ntohs(their_addr.sin_port);
				g1_use_udt_sock = fhandle;
				g1_use_sock_type = SOCKET_TYPE_UDT;


						__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn2", "Enter ControlChannelLoop...\n");
				//============>
				ControlChannelLoop(g1_use_sock_type, g1_use_udt_sock);
				//============>
						__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn2", "Leave ControlChannelLoop!\n");


				UDT::close(g1_use_udt_sock);
				g1_use_udt_sock = INVALID_SOCKET;
				g1_use_sock_type = SOCKET_TYPE_UNKNOWN;
				//closesocket(g1_use_udp_sock);////????!!!!
				g1_use_udp_sock = INVALID_SOCKET;

				g_InConnection2 = FALSE;
			}
			else {
				__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn2", "###### g_InConnection1=true, drop this connection!\n");
				UDT::close(fhandle);
			}/* if (FALSE == g_InConnection1) */

		}

	}/* while */

	closesocket(udp_sock);

	__android_log_print(ANDROID_LOG_INFO, "WorkingThreadFn2", "Exit WorkingThreadFn2!\n");
	return 0;
}

static int killVncServer()
{
  char unix_13132[] = "unix_13132";
  char buffer[] = "~KILL|";
  int sock, n;
  unsigned int length;
  struct sockaddr_un server;

  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) perror("socket() failed!");

  bzero(&server, sizeof(server));
  server.sun_family = AF_UNIX;
  server.sun_path[0] = '\0';
  strncpy(server.sun_path + 1, unix_13132, sizeof(server.sun_path) - 1);
  length = offsetof(struct sockaddr_un, sun_path) + 1 + strlen(unix_13132);

  if (connect(sock, (struct sockaddr *)&server, length) < 0) {
	close(sock);
	return -1;
  }
  n = send(sock, buffer, strlen(buffer) + 1, 0);
  if (n < 0) perror("send() failed!");

  close(sock);
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
	ret |= FUNC_FLAGS_VNC;
	ret |= FUNC_FLAGS_FT;
	ret |= FUNC_FLAGS_ADB;
	ret |= FUNC_FLAGS_SHELL;
	if (hasRoot()) {
		ret |= FUNC_FLAGS_HASROOT;
	}
	if (g1_is_activated) {
		ret |= FUNC_FLAGS_ACTIVATED;
	}
	
	//ID过期时，功能限制
	if (!g1_is_activated && g1_lowest_level_for_av > 0) {
		ret &= ~FUNC_FLAGS_AV;
	}
	if (!g1_is_activated && g1_lowest_level_for_vnc > 0) {
		ret &= ~FUNC_FLAGS_VNC;
	}
	if (!g1_is_activated && g1_lowest_level_for_ft > 0) {
		ret &= ~FUNC_FLAGS_FT;
	}
	if (!g1_is_activated && g1_lowest_level_for_adb > 0) {
		ret &= ~FUNC_FLAGS_ADB;
	}
	return ret;
}

static void learn_remote_addr(void *ctx, sockaddr* his_addr, int addr_len)
{
	sockaddr_in *sin = (sockaddr_in *)his_addr;
	g1_use_peer_port = ntohs(sin->sin_port);
	g1_use_peer_ip = sin->sin_addr.s_addr;
	__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "learn_remote_addr() called, %s[%d]\n", inet_ntoa(sin->sin_addr), g1_use_peer_port);
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
	BOOL bSingleQuit;
	WORD wLocalTcpPort;
	UDTSOCKET fhandle_slave;
	sockaddr_in their_addr;

	if_on_client_connected();
	if (SOCKET_TYPE_UDT == type) {
		UDT::register_learn_remote_addr(fhandle, learn_remote_addr, NULL);
	}
	ProxyServerClearQuitFlag();

	while (FALSE == bQuitLoop && 
			(g_bDoConnection1 || g_bDoConnection2) && 
			FALSE == g_bQuitNativeMain)
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
				//if (memcmp(g1_peer_node_id, buff, 6) != 0) {
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
						CtrlCmd_Send_HELLO_RESP(type, fhandle, g0_node_id, g0_version, getServerFuncFlags(), CTRLCMD_RESULT_NG);
						__android_log_print(ANDROID_LOG_INFO, "ControlChannelLoop", "Password failed!\n");
						break;
					}
				}
				CtrlCmd_Send_HELLO_RESP(type, fhandle, g0_node_id, g0_version, getServerFuncFlags(), CTRLCMD_RESULT_OK);
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
				
			case CMD_CODE_PROXY_REQ:
				ret = RecvStream(type, fhandle, buff, 2);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				
				bSingleQuit = FALSE;
				wLocalTcpPort = ntohs(pf_get_word(buff));
				
#if defined(FOR_WL_YKZ)
				if (SOCKET_TYPE_TCP == type)
#elif (defined(FOR_51HZ_GUAJI) || defined(FOR_MAYI_GUAJI))
				if (TRUE)//挂机端，都是本线程内阻塞。。。
#endif
				{
					//Blocking...
					DoProxyServer(&bSingleQuit, type, fhandle, wLocalTcpPort);
				}
				else if (SOCKET_TYPE_UDT == type)
				{
					fhandle_slave = UDT::socket(AF_INET, SOCK_STREAM, 0);
					
					ConfigUdtSocket(fhandle_slave);
					
					if (UDT::ERROR == UDT::bind2(fhandle_slave, g1_use_udp_sock))
					{
						UDT::close(fhandle_slave);
						break;
					}
					
					their_addr.sin_family = AF_INET;
					their_addr.sin_port = htons(g1_use_peer_port);
					their_addr.sin_addr.s_addr = g1_use_peer_ip;
					memset(&(their_addr.sin_zero), '\0', 8);
					ret = UDT::connect(fhandle_slave, (sockaddr*)&their_addr, sizeof(their_addr));
					if (UDT::ERROR == ret)
					{
						UDT::close(fhandle_slave);
						break;
					}
					
					
					ProxyServerStartProxy(SOCKET_TYPE_UDT, fhandle_slave, TRUE, wLocalTcpPort);
				}
				
				break;
				
///////////////////////////////////////////////////////////////////////////////
			/* 如果ProxyServer()异常结束，就需要在这里妥善处理 CMD_CODE_PROXY_DATA 和 CMD_CODE_PROXY_END */
			case CMD_CODE_PROXY_DATA:
				__android_log_print(ANDROID_LOG_INFO, "ControlChannelLoop", "Recv CMD_CODE_PROXY_DATA dwDataLength=%lu !!!!!!\n", dwDataLength);
				for (int i = 0; i < dwDataLength; i++) {
					RecvStream(type, fhandle, buff, 1);
				}
				break;
				
			case CMD_CODE_PROXY_END:
				__android_log_print(ANDROID_LOG_INFO, "ControlChannelLoop", "Recv CMD_CODE_PROXY_END !!!!!!\n");
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

			case CMD_CODE_MAV_START_REQ:
				if (strncmp(g0_device_uuid, "ANDROID@UAV@", 12) == 0) {
					if_on_mavlink_start();
				}
				break;

			case CMD_CODE_MAV_STOP_REQ:
				if (strncmp(g0_device_uuid, "ANDROID@UAV@", 12) == 0) {
					if_on_mavlink_stop();
				}
				break;

			case CMD_CODE_MAV_GUID_REQ:
				float lati,longi,alti;
				ret = RecvStream(type, fhandle, buff, 12);
				if (ret != 0) {
					bQuitLoop = TRUE;
					break;
				}
				lati  = (float)ntohl(pf_get_dword(buff+0)) / 1000.0f;
				longi = (float)ntohl(pf_get_dword(buff+4)) / 1000.0f;
				alti  = (float)ntohl(pf_get_dword(buff+8)) / 1000.0f;
				if (strncmp(g0_device_uuid, "ANDROID@UAV@", 12) == 0) {
					if_on_mavlink_guid(lati, longi, alti);
				}
				break;

			case CMD_CODE_MAV_WP_REQ:
				WP_ITEM *wpItems;
				int wpNum;
				wpItems = NULL;
				wpNum = 0;
				if (strncmp(g0_device_uuid, "ANDROID@UAV@", 12) == 0) {
					if_get_wp_data(&wpItems, &wpNum);
				}
				CtrlCmd_Send_MAV_WP_RESP(type, fhandle, wpItems, wpNum);
				if (wpItems != NULL) {
					free(wpItems);
				}
				break;

			case CMD_CODE_MAV_TLV_REQ:
				unsigned char *tlvBuff;
				int tlvLen;
				tlvBuff = NULL;
				tlvLen = 0;
				if (strncmp(g0_device_uuid, "ANDROID@UAV@", 12) == 0) {
					if_get_tlv_data(&tlvBuff, &tlvLen);
				}
				CtrlCmd_Send_MAV_TLV_RESP(type, fhandle, tlvBuff, tlvLen);
				if (tlvBuff != NULL) {
					free(tlvBuff);
				}
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
	ProxyServerAllQuit();
	killVncServer();
	if_on_client_disconnected();
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int StartNativeMain(const char *client_charset, const char *client_lang)
{
	CtrlCmd_Init();
	
	if (NULL != client_charset)  strncpy(g_client_charset, client_charset, sizeof(g_client_charset));
	if (NULL != client_lang)     strncpy(g_client_lang,    client_lang,    sizeof(g_client_lang));
	
	g_bFirstCheckStun = TRUE;
	g_bOnlineSuccess = FALSE;
	
	g_InConnection1 = FALSE;
	g_InConnection2 = FALSE;
	
	g_bQuitNativeMain = FALSE;
    return pthread_create(&g_hNativeMainThread, NULL, NativeMainFunc, NULL);//若成功则返回0，否则返回出错编号 
}

void StopNativeMain()
{
	g_bDoConnection1 = FALSE;
	g_bDoConnection2 = FALSE;
	
	myUPnP.RemoveNATPortMapping(mapping);
	
	if (0 != g_hNativeMainThread)
	{
		g_bQuitNativeMain = TRUE;
		//Waiting...
		pthread_join(g_hNativeMainThread, NULL);
		g_hNativeMainThread = 0;
	}
	
	CtrlCmd_Uninit();
}

void StartDoConnection()
{
	g_bDoConnection1 = TRUE;
	g_bDoConnection2 = TRUE;
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
	ProxyServerAllQuit();
	killVncServer();
}

void ForceDisconnect()
{
	__android_log_print(ANDROID_LOG_INFO, "NativeMainFunc", "ForceDisconnect()...\n");
	
	if (g1_use_sock_type == SOCKET_TYPE_UDT)
	{
		UDT::unregister_learn_remote_addr(g1_use_udt_sock);
		
		UdtClose(g1_use_udt_sock);
		g1_use_udt_sock = INVALID_SOCKET;
		g1_use_sock_type = SOCKET_TYPE_UNKNOWN;
		UdpClose(g1_use_udp_sock);
		g1_use_udp_sock = INVALID_SOCKET;
	}
	else if (g1_use_sock_type == SOCKET_TYPE_TCP)
	{
		closesocket(g1_use_udt_sock);
		g1_use_udt_sock = INVALID_SOCKET;
		g1_use_sock_type = SOCKET_TYPE_UNKNOWN;
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

#if defined(FOR_WL_YKZ)
extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(NativeSendEmail)
	(JNIEnv* env, jobject thiz, jstring strToEmail, jstring strSubject, jstring strContent)
{
	int ret;
	const char *toemail;
	const char *subject;
	const char *content;
	
	toemail = (env)->GetStringUTFChars(strToEmail, NULL);
	subject = (env)->GetStringUTFChars(strSubject, NULL);
	content = (env)->GetStringUTFChars(strContent, NULL);
	
	ret = DoSendEmail(g_client_charset, g_client_lang, toemail, subject, content);
	
	(env)->ReleaseStringUTFChars(strToEmail, toemail);
	(env)->ReleaseStringUTFChars(strSubject, subject);
	(env)->ReleaseStringUTFChars(strContent, content);
    return ret;
}
#endif

#if defined(FOR_WL_YKZ)
extern "C"
jint
MAKE_JNI_FUNC_NAME_FOR_MobileCameraActivity(NativePutLocation)
	(JNIEnv* env, jobject thiz, jint put_time, jint num, jstring strItems)
{
	int ret;
	const char *items;
	
	items = (env)->GetStringUTFChars(strItems, NULL);
	
	ret = DoPutLocation(g_client_charset, g_client_lang, put_time, num, items);
	
	(env)->ReleaseStringUTFChars(strItems, items);
    return ret;
}
#endif

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


#if (defined(FOR_51HZ_GUAJI) || defined(FOR_MAYI_GUAJI))
#include "mayi_jni.inl"
#endif


#endif //ifdef ANDROID_NDK
