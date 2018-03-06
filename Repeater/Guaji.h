#pragma once

#include "platform.h"
#include "CommonLib.h"
#include "ControlCmd.h"
#include "HttpOperate.h"


#define IPC_SERVER_BIND_PORT  20000


typedef struct _tag_server_process_node {

	SOCKET m_fhandle;

	BOOL m_bAVStarted;
	
	BOOL m_bVideoEnable;
	BOOL m_bAudioEnable;
	BOOL m_bTLVEnable;

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
