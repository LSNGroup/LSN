#ifndef _PROXYSERVER_H
#define _PROXYSERVER_H


typedef struct _tag_proxy_server_param
{
	BOOL  bSingleQuit;
	SOCKET_TYPE ftype;
	SOCKET fhandle;
	BOOL   bAutoClose;
	WORD wLocalTcpPort;
} PROXY_SERVER_PARAM;


int DoProxyServer(BOOL *pbSingleQuit, SOCKET_TYPE ftype, SOCKET fhandle, WORD wLocalTcpPort);
void ProxyServerStartProxy(SOCKET_TYPE ftype, SOCKET fhandle, BOOL bAutoClose, WORD wLocalTcpPort);
void ProxyServerClearQuitFlag();
void ProxyServerAllQuit();


#endif /* _PROXYSERVER_H */
