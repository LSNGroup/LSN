#pragma once

#include "platform.h"
#include "CommonLib.h"
#include "ControlCmd.h"
#include "HttpOperate.h"

#include "avrtp_def.h"



#define MAX_TOPO_LEVEL			4
#define BANDWIDTH_PER_STREAM	260 // KB/s



#define MAX_ROUTE_ITEM_NUM	2048

typedef struct _tag_topo_route_item {
	BYTE guaji_node_id[6];//向下的出口
	BYTE route_item_type;
	BYTE topo_level;
	BYTE node_id[6];
	BYTE owner_node_id[6];
	BOOL is_busy;
	BOOL is_streaming;
	BYTE peer_node_id[6];
	DWORD last_refresh_time;
	union { 
		char  guaji_node_str[256];  //ip|port|pub_ip|pub_port|no_nat|nat_type
		char viewer_node_str[256];  //
		char device_node_str[256];  //device_uuid|node_name|version|os_info
	}u; 
} TOPO_ROUTE_ITEM;



#define MAX_VIEWER_NUM		5

typedef struct _tag_viewer_node {
	BOOL bFirstCheckStun;
	BOOL bNeedRegister;
	CRITICAL_SECTION localbind_csec;
	ANYPC_NODE anypcNode;
	HttpOperate httpOP;
	//FAKERTPRECV *m_pFRR;
	//TLV_RECV *m_pTlvRecv;
	BOOL bQuitRecvSocketLoop;
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

	int device_topo_level;
	BYTE device_node_id[6];
};

extern CShiyong* g_pShiyong;

