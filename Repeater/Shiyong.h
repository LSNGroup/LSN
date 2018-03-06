#pragma once

#include "platform.h"
#include "CommonLib.h"
#include "ControlCmd.h"
#include "HttpOperate.h"

#include "avrtp_def.h"
#include "SimpleRtpRecv.h"



#define MAX_VIEWER_NUM		5

typedef struct _tag_viewer_node {
	BOOL bFirstCheckStun;
	BOOL bNeedRegister;
	CRITICAL_SECTION localbind_csec;
	ANYPC_NODE anypcNode;
	HttpOperate httpOP;
	FAKERTPRECV *m_pFRR;
	//TLV_RECV *m_pTlvRecv;
	unsigned char m_sps_buff[1024];//包含8字节头部
	int m_sps_len;
	unsigned char m_pps_buff[1024];//包含8字节头部
	int m_pps_len;

	HANDLE	hConnectThread;
	HANDLE	hConnectThread2;
	HANDLE	hConnectThreadRev;
	char password[MAX_PATH];
	int nID;
	BOOL bUsing;
} VIEWER_NODE;



extern int g_video_width;
extern int g_video_height;
extern int g_video_fps;



class CShiyong
{
public:
	CShiyong();   // 标准构造函数
	virtual ~CShiyong();

	BOOL OnInit();
	void DoExit();

public:
	BOOL m_bQuit;
	HANDLE m_hThread;

	VIEWER_NODE viewerArray[MAX_VIEWER_NUM];
	int currentSourceIndex;
	void ReturnViewerNode(VIEWER_NODE *pViewerNode);

	void ConnectNode(char *anypc_node_id, char *password);
	void DisconnectNode(VIEWER_NODE *pViewerNode);
};

extern CShiyong* g_pShiyong;

