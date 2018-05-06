#pragma once

#include "platform.h"
#include "CommonLib.h"
#include "ControlCmd.h"
#include "HttpOperate.h"
#include "UPnP.h"

#include "avrtp_def.h"



#define MAX_TOPO_LEVEL			4



#define MAX_ROUTE_ITEM_NUM	2048

typedef struct _tag_topo_route_item {
	int guaji_index;//向下的出口
	BYTE route_item_type;
	BYTE topo_level;
	BYTE node_id[6];
	BYTE owner_node_id[6];
	BOOL is_connected;
	BOOL is_streaming;
	BYTE peer_node_id[6];
	DWORD last_refresh_time;
	union {
		struct { //ip|port|pub_ip|pub_port|no_nat|nat_type
			char ip_str[16*MAX_ADAPTER_NUM];
			char port_str[8];
			char pub_ip_str[16];
			char pub_port_str[8];
			BOOL no_nat;
			int  nat_type;
		}node_nat_info;
		struct {
			int connected_viewer_nodes;
			int unconnected_guaji_nodes;
			int device_free_streams;
			char device_node_str[200];  //device_uuid|node_name|version|os_info
		}device_info;
	}u;
	int nID;
	BOOL bUsing;
} TOPO_ROUTE_ITEM;



#define MAX_CONNECTING_EVENT_NUM	10

typedef struct _tag_connecting_event_item {
	BYTE viewer_owner_node_id[6];
	BYTE viewer_node_id[6];
	BYTE guaji_node_id[6];
	DWORD create_time;
	BOOL switch_after_connected;
	int nID;
	BOOL bUsing;
} CONNECTING_EVENT_ITEM;



#define MAX_VIEWER_NUM		3

typedef struct _tag_viewer_node {
	BOOL bFirstCheckStun;
	pthread_mutex_t localbind_csec;
	HttpOperate httpOP;
	UPNPNAT_MAPPING mapping;
	BOOL bConnecting;
	BOOL bConnected;
	BOOL bTopoPrimary;
	//FAKERTPRECV *m_pFRR;
	//TLV_RECV *m_pTlvRecv;
	BOOL bQuitRecvSocketLoop;
	unsigned char m_sps_buff[1024];//包含8字节头部
	int m_sps_len;
	unsigned char m_pps_buff[1024];//包含8字节头部
	int m_pps_len;

	pthread_t	hConnectThread;
	pthread_t	hConnectThread2;
	pthread_t	hConnectThreadRev;
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
	pthread_t m_hThread;

	VIEWER_NODE viewerArray[MAX_VIEWER_NUM];
	int currentSourceIndex;
	DWORD currentLastMediaTime;
	DWORD timeoutMedia;
	void CalculateTimeoutMedia();
	void SwitchMediaSource(int oldIndex, int newIndex);
	void ReturnViewerNode(VIEWER_NODE *pViewerNode);

	void ConnectNode(int i, char *password);
	void ConnectRevNode(int i, char *password);
	void DisconnectNode(VIEWER_NODE *pViewerNode);
	void UnregisterNode(VIEWER_NODE *pViewerNode);

	DWORD joined_channel_id;
	BYTE joined_node_id[6];
	BYTE device_topo_level;
	BYTE device_node_id[6];
	int device_max_streams;
	CONNECTING_EVENT_ITEM connecting_event_table[MAX_CONNECTING_EVENT_NUM];
	TOPO_ROUTE_ITEM device_route_table[MAX_ROUTE_ITEM_NUM];
	pthread_mutex_t route_table_csec;

	BOOL ShouldDoHttpOP();
	BOOL ShouldDoAdjustAndOptimize();

	int FindConnectingItemViewerOwnerNode(BYTE *node_id);
	int FindConnectingItemViewerNode(BYTE *node_id);
	int FindConnectingItemGuajiNode(BYTE *node_id);

	int FindViewerNode(BYTE *viewer_node_id);
	int FindTopoRouteItem(BYTE *dest_node_id);
	int FindRouteNode(BYTE *node_id);
	int FindRouteNode_NoLock(BYTE *node_id);
	void DropRouteItem(BYTE node_type, BYTE *node_id);
	void CheckTopoRouteTable();
	int FindUncleGuajiNode_NoLock(int father_topo_level, BYTE *father_device_node_id, int *device_free_streams);
	int FindDeviceFreeViewerNode_NoLock(BYTE *device_node_id);
	void AdjustTopoStructure();////
	int FindLevelHeaviestStreamDeviceNode_NoLock(int topo_level);
	int FindTheOtherViewerFatherDevieFreeStreams_NoLock(BYTE *viewer_node_id);
	void OptimizeStreamPath();////
	int UpdateRouteTable(int guajiIndex, const char *report_string);
	int GetConnectedViewerNodes();
	int GetUnconnectedGuajiNodes();
	int GetDeviceFreeStreams();
	int DeviceTopoReport();

	const char *get_public_ip();
	const char *get_node_array();

	int get_route_item_num();

	int last_viewer_num_time;
	int last_viewer_num;
	int get_viewer_grow_rate();

	int get_level_device_num(int topo_level);

	int get_level_max_connections(int topo_level);
	int get_level_current_connections(int topo_level);
	int get_level_max_streams(int topo_level);
	int get_level_current_streams(int topo_level);
};

extern CShiyong* g_pShiyong;

