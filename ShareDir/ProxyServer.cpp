#include "stdafx.h"
#include "platform.h"
#include "CommonLib.h"
#include "udt.h"
#include "udp.h"
#include "ControlCmd.h"
#include "ProxyServer.h"


#ifdef ANDROID_NDK
#include <android/log.h>
#endif


#ifdef ANDROID_NDK
#define log_msg(msg, lev)  __android_log_print(ANDROID_LOG_INFO, "Proxy", msg)
#else
#define log_msg(msg, lev)  printf(msg)
#endif


typedef struct _loop_param_tag {
	SOCKET_TYPE ftype;
	SOCKET fhandle;
	SOCKET sock;
	BOOL  *pbSingleQuit;
} LOOP_PARAM;


static BOOL bQuit;

void ProxyServerClearQuitFlag()
{
	bQuit = FALSE;
}

void ProxyServerAllQuit()
{
	bQuit = TRUE;
}


#include "ProxyCommon.inl"


#ifdef WIN32
static DWORD WINAPI ProxyWorkingThreadFn(LPVOID pvThreadParam)
#else
static void *ProxyWorkingThreadFn(void *pvThreadParam)
#endif
{
	LOOP_PARAM *loop_param = (LOOP_PARAM *)pvThreadParam;
	DataLoop2(loop_param->ftype, loop_param->fhandle, loop_param->sock, loop_param->pbSingleQuit);
	return 0;
}


int DoProxyServer(BOOL *pbSingleQuit, SOCKET_TYPE ftype, SOCKET fhandle, WORD wLocalTcpPort, DWORD dwTcpAddress)
{
	SOCKET sock;
	sockaddr_in addr;
	int addrlen = sizeof(addr);
	int ret;


	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


	addr.sin_family = AF_INET;
	addr.sin_port = htons(wLocalTcpPort);
	addr.sin_addr.s_addr = dwTcpAddress;
	memset(&(addr.sin_zero), '\0', 8);


	if (connect(sock, (sockaddr*)&addr, addrlen) != 0) {
	  if (connect(sock, (sockaddr*)&addr, addrlen) != 0) {
		closesocket(sock);
		log_msg("Proxy(): connect local tcp failed!", LOG_LEVEL_INFO);
		return -1;
	  }
	}
	log_msg("ProxyServer: OK, start proxy...\n", LOG_LEVEL_INFO);

	*pbSingleQuit = FALSE;

	LOOP_PARAM param;
	param.pbSingleQuit = pbSingleQuit;
	param.ftype = ftype;
	param.fhandle = fhandle;
	param.sock = sock;

#ifdef WIN32
	DWORD dwThreadID;
	HANDLE hThread = ::CreateThread(NULL,0,ProxyWorkingThreadFn,&param,0,&dwThreadID);
	if (hThread == NULL)
#else
	pthread_t hThread;
	int err = pthread_create(&hThread, NULL, ProxyWorkingThreadFn, &param);
	if (0 != err)
#endif
	{
		closesocket(sock);
		return -1;
	}

	DataLoop1(sock, ftype, fhandle, pbSingleQuit);


	//Waiting...蚂蚁互动软件，必须在VncServer退出时，立即退出这个Proxy函数！！！
#if 0
#ifdef WIN32
	::WaitForSingleObject(hThread,  INFINITE);
	::CloseHandle(hThread);
#else
	pthread_join(hThread, NULL);
#endif
#endif

	closesocket(sock);
	return 0;
}

#ifdef WIN32
DWORD WINAPI ProxyServerThreadFn(LPVOID pvThreadParam)
#else
void *ProxyServerThreadFn(void *pvThreadParam)
#endif
{
	PROXY_SERVER_PARAM *p = (PROXY_SERVER_PARAM *)pvThreadParam;

	DoProxyServer(&(p->bSingleQuit), p->ftype, p->fhandle, p->wLocalTcpPort, p->dwTcpAddress);
	if (p->bAutoClose)
	{
		if (SOCKET_TYPE_UDT == p->ftype) {
			UDT::close(p->fhandle);
		}
		else if (SOCKET_TYPE_TCP == p->ftype) {
			closesocket(p->fhandle);
		}
	}
	log_msg("ProxyServer(): done!!!\n", LOG_LEVEL_INFO);

	free(pvThreadParam);
	
	return 0;
}

void ProxyServerStartProxy(SOCKET_TYPE ftype, SOCKET fhandle, BOOL bAutoClose, WORD wLocalTcpPort, DWORD dwTcpAddress)
{
	PROXY_SERVER_PARAM *p;
	p = (PROXY_SERVER_PARAM *)malloc(sizeof(PROXY_SERVER_PARAM));
	p->bSingleQuit = FALSE;
	p->ftype = ftype;
	p->fhandle = fhandle;
	p->bAutoClose = bAutoClose;
	p->wLocalTcpPort = wLocalTcpPort;
	p->dwTcpAddress = dwTcpAddress;
	
#ifdef WIN32
	DWORD dwThreadID;
	HANDLE hThread = ::CreateThread(NULL,0,ProxyServerThreadFn,p,0,&dwThreadID);
	::CloseHandle(hThread);
#else
	pthread_t tid;
	pthread_create(&tid, NULL, ProxyServerThreadFn, p);
	pthread_detach(tid);
#endif
	// ---->>
}
