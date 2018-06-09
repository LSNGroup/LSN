#ifndef _IPC_GUAJI_H
#define _IPC_GUAJI_H


#include "platform.h"
#include "CommonLib.h"
#include "ControlCmd.h"
#include "HttpOperate.h"


#define IPC_SERVER_BIND_PORT  20000


typedef struct _tag_server_process_node {

	SOCKET m_fhandle;

	BYTE m_node_id[6];
	char m_szReport[512];//row=node_type|topo_level|...\n

	BOOL m_bConnected;//rudp client connected
	BOOL m_bAVStarted;
	
	BOOL m_bVideoEnable;
	BOOL m_bAudioEnable;
	BOOL m_bTLVEnable;

	int m_nIndex;
} SERVER_PROCESS_NODE;




extern SERVER_PROCESS_NODE *arrServerProcesses;

extern char SERVER_TYPE[16];
extern int UUID_EXT;
extern int MAX_SERVER_NUM;
extern char NODE_NAME[MAX_PATH];
extern char CONNECT_PASSWORD[MAX_PATH];

extern char g_tcp_address[MAX_PATH];


void StartServerProcesses();

int GetAvClientsCount();


#endif //_IPC_GUAJI_H