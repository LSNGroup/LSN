#ifndef _PROXYSERVER_H
#define _PROXYSERVER_H


typedef struct _tag_proxy_server_param
{
	BOOL  bSingleQuit;
	SOCKET_TYPE ftype;
	SOCKET fhandle;
	BOOL   bAutoClose;
	WORD wLocalTcpPort;
	DWORD dwTcpAddress;//ÍøÂç×Ö½ÚÐò,Default is 127.0.0.1
} PROXY_SERVER_PARAM;


int DoProxyServer(BOOL *pbSingleQuit, SOCKET_TYPE ftype, SOCKET fhandle, WORD wLocalTcpPort, DWORD dwTcpAddress = INADDR_LOOPBACK);
void ProxyServerStartProxy(SOCKET_TYPE ftype, SOCKET fhandle, BOOL bAutoClose, WORD wLocalTcpPort, DWORD dwTcpAddress = INADDR_LOOPBACK);
void ProxyServerClearQuitFlag();
void ProxyServerAllQuit();


#endif /* _PROXYSERVER_H */
