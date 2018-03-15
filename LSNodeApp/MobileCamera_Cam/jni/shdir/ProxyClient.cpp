//#include "stdafx.h"
#include "platform.h"
#include "CommonLib.h"
#include "udt.h"
#include "udp.h"
#include "ControlCmd.h"
#include "HttpOperate.h"
#include "ProxyClient.h"


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

void ProxyClientClearQuitFlag()
{
	bQuit = FALSE;
}

void ProxyClientAllQuit()
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
	DataLoop1(loop_param->sock, loop_param->ftype, loop_param->fhandle, loop_param->pbSingleQuit);
	return 0;
}

int DoProxyClient(BOOL *pbSingleQuit, SOCKET_TYPE ftype, SOCKET fhandle, WORD wLocalTcpPort)
{
	SOCKET serv;
	sockaddr_in my_addr;
	sockaddr_in their_addr;
	socklen_t namelen = sizeof(their_addr);
	SOCKET sock;
	int ret;


	serv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(wLocalTcpPort);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);

	if (bind(serv, (sockaddr*)&my_addr, sizeof(my_addr)) != 0)
	{
		closesocket(serv);
		return -1;
	}


	listen(serv, 1);


//////////////////////////////////
   fd_set target_fds;
   fd_set except_fds;
   struct timeval waitval;

	FD_ZERO(&target_fds);
	FD_SET(serv, &target_fds);
	FD_ZERO(&except_fds);
	FD_SET(serv, &except_fds);
	waitval.tv_sec  = 15;       /* Wait user operation */
	waitval.tv_usec = 0;
	if (select(serv+1, &target_fds, NULL, &except_fds, &waitval) < 0 
				|| FD_ISSET(serv, &except_fds) 
				|| !FD_ISSET(serv, &target_fds)) {
		closesocket(serv);
		return -1;
	}
//////////////////////////////////


	if ((sock = accept(serv, (sockaddr*)&their_addr, &namelen)) == INVALID_SOCKET) {
		closesocket(serv);
		return -1;
	}
	closesocket(serv);



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
	pthread_t tid;
	int err = pthread_create(&tid, NULL, ProxyWorkingThreadFn, (void *)&param);
	if (0 != err)
#endif
	{
		closesocket(sock);
		return -1;
	}

	DataLoop2(ftype, fhandle, sock, pbSingleQuit);

	//Waiting...
#ifdef WIN32
	::WaitForSingleObject(hThread,  INFINITE);
	::CloseHandle(hThread);
#else
	pthread_join(tid, NULL);
#endif

	closesocket(sock);
	return 0;
}

#ifdef WIN32
DWORD WINAPI ProxyClientThreadFn(LPVOID pvThreadParam)
#else
void *ProxyClientThreadFn(void *pvThreadParam)
#endif
{
	PROXY_CLIENT_PARAM *p = (PROXY_CLIENT_PARAM *)pvThreadParam;

	DoProxyClient(&(p->bSingleQuit), p->ftype, p->fhandle, p->wLocalTcpPort);
	
	
	//让受控端退出Proxy循环
	for (int i = 0; i < 3; i++)
	{
		CtrlCmd_PROXY_END(p->ftype, p->fhandle);
	}
	
	//读完控制端ControlChannel的接收数据，直到超时
	if (SOCKET_TYPE_TCP == p->ftype)
	{
		fd_set target_fds;
		fd_set except_fds;
		struct timeval waitval;
		char tmp_buff[1024];
		int ret;
		
		while (TRUE)
		{
			FD_ZERO(&target_fds);
			FD_SET(p->fhandle, &target_fds);
			FD_ZERO(&except_fds);
			FD_SET(p->fhandle, &except_fds);
			waitval.tv_sec  = 1;
			waitval.tv_usec = 0;
			if (select(p->fhandle + 1, &target_fds, NULL, &except_fds, &waitval) < 0 
						|| FD_ISSET(p->fhandle, &except_fds) 
						|| !FD_ISSET(p->fhandle, &target_fds)) {
				break;
			}
			
			ret = recv(p->fhandle, tmp_buff, sizeof(tmp_buff), 0);
			if (ret <= 0) {
				break;
			}
		}
	}
	else if (SOCKET_TYPE_UDT == p->ftype)
	{
		// Slave socket. Do nothing.
	}
	
	
	if (p->bAutoClose)
	{
		if (SOCKET_TYPE_UDT == p->ftype) {
			UDT::close(p->fhandle);
		}
		else if (SOCKET_TYPE_TCP == p->ftype) {
			closesocket(p->fhandle);
		}
	}
	log_msg("ProxyClient(): done!!!\n", LOG_LEVEL_INFO);

	free(pvThreadParam);
	
	return 0;
}

void ProxyClientStartProxy(SOCKET_TYPE ftype, SOCKET fhandle, BOOL bAutoClose, WORD wLocalTcpPort)
{
	PROXY_CLIENT_PARAM *p;
	p = (PROXY_CLIENT_PARAM *)malloc(sizeof(PROXY_CLIENT_PARAM));
	p->bSingleQuit = FALSE;
	p->ftype = ftype;
	p->fhandle = fhandle;
	p->bAutoClose = bAutoClose;
	p->wLocalTcpPort = wLocalTcpPort;
	
#ifdef WIN32
	DWORD dwThreadID;
	HANDLE hThread = ::CreateThread(NULL,0,ProxyClientThreadFn,p,0,&dwThreadID);
	::CloseHandle(hThread);
#else
	pthread_t tid;
	pthread_create(&tid, NULL, ProxyClientThreadFn, p);
	pthread_detach(tid);
#endif
	// ---->>
}

void ProxyClientStartSlave(WORD wLocalTcpPort)
{
	int ret;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);

	if (g1_use_sock_type != SOCKET_TYPE_UDT) {
		return;
	}

	UDTSOCKET serv_slave = UDT::socket(AF_INET, SOCK_STREAM, 0);

	ConfigUdtSocket(serv_slave);

	if (UDT::ERROR == UDT::bind2(serv_slave, g1_use_udp_sock))
	{
		UDT::close(serv_slave);
		return;
	}


	UDT::listen(serv_slave, 1);


///////////////
	struct timeval waitval;
	waitval.tv_sec  = UDT_ACCEPT_TIME;
	waitval.tv_usec = 0;
	ret = UDT::select(serv_slave + 1, serv_slave, NULL, NULL, &waitval);
	if (ret == UDT::ERROR || ret == 0) {
		UDT::close(serv_slave);
		return;
	}
//////////////////


	UDTSOCKET fhandle_slave;
	if ((fhandle_slave = UDT::accept(serv_slave, (sockaddr*)&their_addr, &namelen)) == UDT::INVALID_SOCK) {
		UDT::close(serv_slave);
		return;
	}
	else {
		UDT::close(serv_slave);
		//ConfigUdtSocket(fhandle2);
	}

	ProxyClientStartProxy(SOCKET_TYPE_UDT, fhandle_slave, TRUE, wLocalTcpPort);
}
