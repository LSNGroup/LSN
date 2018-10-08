#ifndef _COMMONLIB_H
#define _COMMONLIB_H

#include "udt.h"
#include "udp.h"
#include "stun.h"


/* For AppSettings */
#define STRING_REGKEY_NAME_VIEWERNODEID  "ViewerNodeId"

#define STRING_REGKEY_NAME_VERSION				"Version"
#define STRING_REGKEY_NAME_STOPFLAG				"StopFlag"

#define STRING_REGKEY_NAME_SAVED_UUID		"SavedUUID"
#define STRING_REGKEY_NAME_FORCE_NONAT		"ForceNoNAT"

#define STRING_REGKEY_NAME_PHONENUM			"PhoneNum"


#define _MAX_PATH				512
#define MAX_PATH				512
#define MAX_LOADSTRING			256
#define MAX_ADAPTER_NUM			2

#define _T(str)		(str)
#define stricmp(s1, s2)			strcasecmp((s1), (s2))


#define MYSELF_VERSION			0x02060200  /* x.x.x.0 */
#define FIRST_CONNECT_PORT		3478
#define SECOND_CONNECT_PORT		3476

#define NODES_PER_PAGE			50
#define MAX_NOTIFICATION_NUM	15
#define UDT_CONNECT_TIMES		1
#define UDT_ACCEPT_TIME			15  /* seconds */

#define VNC_SERVER_PORT			5901
#define WEBMONI_SERVER_PORT		22
#define ADB_SERVER_PORT			5555
#define LOCAL_PORT_BASE			10000

#define REPEATER_PASSWORD		"123456"


typedef struct _tag_anypc_node {
	BOOL bLanNode;  /* LAN_NODE_SUPPORT */
	char node_id_str[MAX_LOADSTRING];
	char node_name[MAX_LOADSTRING];
	DWORD version;
	char ip_str[16*MAX_ADAPTER_NUM];
	char port_str[8];
	char pub_ip_str[16];
	char pub_port_str[8];
	BOOL no_nat;
	int  nat_type;
	BOOL is_admin;
	BOOL is_busy;
	DWORD audio_channels;
	DWORD video_channels;
	char os_info[MAX_LOADSTRING];
	BYTE func_flags;////
	char device_uuid[MAX_LOADSTRING];
	DWORD comments_id;
	char location[MAX_LOADSTRING];
} ANYPC_NODE;


typedef struct _tag_channel_node {
	DWORD channel_id;
	char channel_comments[MAX_LOADSTRING];
	char device_uuid[MAX_LOADSTRING];
	char node_id_str[MAX_LOADSTRING];
	char pub_ip_str[16];
	char location[MAX_LOADSTRING];
} CHANNEL_NODE;


typedef struct _tag_notification_item {
	char text[MAX_LOADSTRING];
	char link[MAX_LOADSTRING];
	BYTE  type;
} NOTIFICATION_ITEM;


typedef struct _tag_av_param_ {
	BYTE bFlags;
	BYTE bVideoSize;
	BYTE bVideoFrameRate;
	int audioInChannel;
	int videoInChannel;
	BOOL bAutoSwitch;
	int  nSwitchTime;
	BOOL bRecord;
} AV_PARAM;


typedef struct _tag_wp_item_ {
	DWORD wpFlags;
	DWORD wpType;
	DWORD wpLati; // lati*100000
	DWORD wpLongi;// longi*100000
	DWORD wpAlti; // alti*100000
} WP_ITEM;

#define WP_ITEM_FLAG_INVALID	0x00000001


void trim(char *str);


DWORD get_system_milliseconds();


//
// Return Value:
// -1: Error
//  0: Success
//
int mac_addr(const char *lpMacStr, BYTE *lpMacAddr, int *lpAddrLen);


void generate_nodeid(BYTE *bNodeId, int nBuffLen);



typedef enum _tag_socket_type {
	SOCKET_TYPE_UNKNOWN = 0, //0
	SOCKET_TYPE_UDT = 1, //1
	SOCKET_TYPE_TCP = 2, //2
} SOCKET_TYPE;


//
// Return Value:
// -1: Error
//  0: Success
//
int RecvStreamUdt(UDTSOCKET fhandle, char *buff, int len);


//
// Return Value:
// -1: Error
//  0: Success
//
int SendStreamUdt(UDTSOCKET fhandle, char *data, int len);


//
// Return Value:
// -1: Error
//  0: Success
//
int RecvStreamTcp(SOCKET sock, char *buff, int len);


//
// Return Value:
// -1: Error
//  0: Success
//
int SendStreamTcp(SOCKET sock, char *data, int len);


//
// Return Value:
// -1: Error
//  0: Success
//
int RecvStream(SOCKET_TYPE type, SOCKET sock, char *buff, int len);


//
// Return Value:
// -1: Error
//  0: Success
//
int SendStream(SOCKET_TYPE type, SOCKET sock, char *data, int len);



//
// Return Value:
// -1: Error
//  0: Success.
//
int FindOutConnection(SOCKET sock, BYTE __g0_node_id[], BYTE __g1_peer_node_id[],
	DWORD __g1_peer_pri_ipArray[], int __g1_peer_pri_ipCount, WORD __g1_peer_pri_port,
	DWORD __g1_peer_ip, WORD __g1_peer_port,
	DWORD *use_ip, WORD *use_port);


//
// Return Value:
// -1: Error
//  0: Success.
//
int WaitForProxyReady(SOCKET sock, BYTE __g0_node_id[], DWORD __g1_peer_ip, WORD __g1_peer_port);


//
// Return Value:
// None
//
void EndProxy(SOCKET udp_sock, DWORD dest_ip, WORD dest_port);



BOOL ParseRowValue(char *value, ANYPC_NODE *lpNode);


BOOL ParseChannelRowValue(char *value, CHANNEL_NODE *lpNode);


//
// Return Value:
// -1: Error
//  0: Stun success, can't work.
//  1: Stun success, can work.
//
int CheckStun(char *lpStunServer, WORD wSrcPort, StunAddress4 *lpMappedResult, BOOL *lpNoNAT, int *lpNatType);

//
// Return Value:
// -1: Error
//  0: Success
//
int CheckStunSimple(char *lpStunServer, WORD wSrcPort, StunAddress4 *lpMappedResult);

//
// Return Value:
// -1: Error
//  0: Success
//
int CheckStunMyself(char *lpStunServer, WORD wSrcPort, StunAddress4 *lpMappedResult, BOOL *lpNoNAT, int *lpNatType, DWORD *lpTime);


DWORD GetLocalAddress(DWORD addresses[], int *lpCount);


BOOL GetOsInfo(char *lpInfoBuff, int BuffSize);


void ConfigUdtSocket(UDTSOCKET fhandle);


/* ANSI version */
std::string UrlEncode(const std::string& src);
std::string UrlDecode(const std::string& src);


/* UDP socket */
UDPSOCKET UdpOpen(DWORD dwIpAddr/* Network byte order */, int nPort/* Host byte order */);
int UdpClose(UDPSOCKET udpSock);


/* UDT socket */
int UdtStartup();
int UdtCleanup();
UDTSOCKET UdtOpen(DWORD dwIpAddr/* Network byte order */, int nPort/* Host byte order */);
UDTSOCKET UdtOpenEx(UDPSOCKET udpSock);
int UdtClose(UDTSOCKET udtSock);
int UdtConnect(UDTSOCKET udtSock, DWORD dwIpAddr/* Network byte order */, int nPort/* Host byte order */);
UDTSOCKET UdtWaitConnect(UDTSOCKET serv, DWORD *lpIpAddr/* Network byte order */, WORD *lpPort/* Host byte order */);
int UdtRecv(UDTSOCKET udtSock, char *buff, int buffSize);
int UdtSend(UDTSOCKET udtSock, const char *data, int dataLen);


#endif /* _COMMONLIB_H */
