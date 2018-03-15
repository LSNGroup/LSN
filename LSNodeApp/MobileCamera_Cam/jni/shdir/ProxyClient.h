#ifndef _PROXYCLIENT_H
#define _PROXYCLIENT_H


typedef struct _tag_proxy_client_param
{
	BOOL  bSingleQuit;
	SOCKET_TYPE ftype;
	SOCKET fhandle;
	BOOL   bAutoClose;
	WORD wLocalTcpPort;
} PROXY_CLIENT_PARAM;


int DoProxyClient(BOOL *pbSingleQuit, SOCKET_TYPE ftype, SOCKET fhandle, WORD wLocalTcpPort);
void ProxyClientStartProxy(SOCKET_TYPE ftype, SOCKET fhandle, BOOL bAutoClose, WORD wLocalTcpPort);
void ProxyClientStartSlave(WORD wLocalTcpPort);
void ProxyClientClearQuitFlag();
void ProxyClientAllQuit();


#endif /* _PROXYCLIENT_H */
