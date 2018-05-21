#ifndef _VIEWER_NODE_H_
#define _VIEWER_NODE_H_



#define MAX_VIEWER_NUM		3

typedef struct _tag_viewer_node {
	BOOL m_bNeedRegister;
	pthread_mutex_t m_secBind;
	HttpOperate httpOP;
	UPNPNAT_MAPPING mapping;
	pthread_t hConnectThread;
	pthread_t hConnectThread2;
	pthread_t hConnectThreadRev;
	int channel_id;
	BOOL from_star;
	BOOL bConnecting;
	BOOL bConnected;
	BOOL bQuitRecvSocketLoop;
	char password[MAX_PATH];
	int nID;
	BOOL bUsing;
} VIEWER_NODE;


extern VIEWER_NODE viewerArray[MAX_VIEWER_NUM];
extern int currentSourceIndex;
extern BOOL is_app_recv_av;

void DisconnectNode(VIEWER_NODE *pViewerNode);
void ReturnViewerNode(VIEWER_NODE *pViewerNode);


#endif /* _VIEWER_NODE_H_ */
