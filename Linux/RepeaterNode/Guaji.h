#ifndef _GUAJI_H
#define _GUAJI_H


#include "platform.h"
#include "CommonLib.h"
#include "ControlCmd.h"
#include "HttpOperate.h"
#include "UPnP.h"

#include "FakeRtpSend.h"



typedef struct _tag_server_node {

	BOOL m_bExit;

	HttpOperate myHttpOperate;
	UPNPNAT_MAPPING mapping;
	
	BOOL m_bConnected;

	BOOL m_bFirstCheckStun;
	BOOL m_bOnlineSuccess;
	BOOL m_bDoConnection1;
	BOOL m_bDoConnection2;
	BOOL m_InConnection1;
	BOOL m_InConnection2;

	FAKERTPSEND *m_pFRS;
	//TLV_SEND *m_pTlvSend;
	
	BOOL m_bAVStarted;
	BOOL m_bH264FormatSent;
	
	BOOL m_bVideoEnable;
	BOOL m_bAudioEnable;
	BOOL m_bTLVEnable;
	BOOL m_bVideoReliable;
	BOOL m_bAudioRedundance;
} SERVER_NODE;


extern MyUPnP  myUPnP;

extern SOCKET g_fhandle;

extern SERVER_NODE *g_pServerNode;

extern char SERVER_TYPE[16];
extern int UUID_EXT;
extern int MAX_SERVER_NUM;
extern char NODE_NAME[MAX_PATH];
extern char CONNECT_PASSWORD[MAX_PATH];
extern int  P2P_PORT;

extern int  IPC_BIND_PORT;

extern BYTE g_device_topo_level;
extern BYTE g_peer_node_id[6];
extern BOOL g_is_topo_primary;

extern int g_video_width;
extern int g_video_height;
extern int g_video_fps;

extern char g_tcp_address[MAX_PATH];


void StartAnyPC();//Ä¬ÈÏÊÇ StartDoConnection();

void StartDoConnection(SERVER_NODE* pServerNode);
void StopDoConnection(SERVER_NODE* pServerNode);

void DShowAV_Start(SERVER_NODE* pServerNode, BYTE flags, BYTE video_size, BYTE video_framerate, DWORD audio_channel, DWORD video_channel);
void DShowAV_Stop(SERVER_NODE* pServerNode);
void DShowAV_Switch(SERVER_NODE* pServerNode, DWORD video_channel);
void DShowAV_Contrl(SERVER_NODE* pServerNode, WORD contrl, DWORD contrl_param);


#endif //_GUAJI_H
