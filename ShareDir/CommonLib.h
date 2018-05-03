#pragma once

#include "udt.h"
#include "udp.h"
#include "stun.h"



#define MAX_LOADSTRING			256
#define MAX_KEY_NAME			512
#define MAX_ADAPTER_NUM			3



#define MYSELF_VERSION			0x02050400  /* x.x.x.0 */
#define NODES_PER_PAGE			34
#define MAX_NOTIFICATION_NUM	15
#define UDT_CONNECT_TIMES		1
#define UDT_ACCEPT_TIME			15  /* seconds */
#define FIRST_CONNECT_PORT		3478
#define SECOND_CONNECT_PORT		3476

#define VNC_SERVER_PORT			5901
#define WEBMONI_SERVER_PORT		22
#define ADB_SERVER_PORT			5555
#define LOCAL_PORT_BASE			10000

#define REPEATER_PASSWORD		"repeaterpassword"


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


typedef struct _tag_av_param_ {
	BYTE bFlags;
	BYTE bVideoMode;
	BYTE bVideoSize;
	BYTE bVideoFrameRate;
	int audioInChannel;
	int videoInChannel;
	BOOL bAutoSwitch;
	int  nSwitchTime;
	BOOL bRecord;
} AV_PARAM;




void trim(char *str);


DWORD get_system_milliseconds();


//
// Return Value:
// -1: Error
//  0: Success
//
int mac_addr(char *lpMacStr, LPBYTE lpMacAddr, UINT *lpAddrLen);


void generate_nodeid(BYTE *bNodeId, int nBuffLen);



BOOL GetSoftwareKeyValue(LPCTSTR szValueName, LPBYTE szValueData, DWORD* lpdwLen);
BOOL SaveSoftwareKeyValue(LPCTSTR szValueName, LPCTSTR szValueData);
BOOL GetSoftwareKeyDwordValue(LPCTSTR szValueName, DWORD *lpdwValue);
BOOL SaveSoftwareKeyDwordValue(LPCTSTR szValueName, DWORD dwValue);

BOOL do_regcheck(HKEY hKey, const char *lpSubKey);
BOOL do_regread(HKEY hKey, const char *lpSubKey, const char *lpValueName, LPDWORD lpType, LPBYTE lpData, LPDWORD lpSize);
BOOL do_regwrite(HKEY hKey, const char *lpSubKey, const char *lpValueName, DWORD Type, const BYTE *lpData, DWORD Size);
BOOL do_regdelete(HKEY hKey, const char *lpSubKey, const char *lpValueName);




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


DWORD RunExeWait(LPTSTR szCmdLine, BOOL bShowWnd);
DWORD RunExeNoWait(LPTSTR szCmdLine, BOOL bShowWnd);


DWORD GetAdapters(IP_ADAPTER_INFO *lpAdapter, LPDWORD lpcntAdapter);
DWORD GetLocalAddress(DWORD addresses[], int *lpCount);


//
// Return Value:
// -1: Error
//  0: Success
//  
int GetAudioVideoInfo(BOOL *lpAudio, BOOL *lpVideo);


//
// Return Value:
// -1: Error
//  0: Success
//  
int GetDisplayInfo(WORD *lpX, WORD *lpY, WORD *lpBits);


//
// Return Value:
// -1: Error
//  0: Stun success, can't work.
//  1: Stun success, can work.
//
int CheckStun(LPTSTR lpStunServer, WORD wSrcPort, StunAddress4 *lpMappedResult, BOOL *lpNoNAT, int *lpNatType);

//
// Return Value:
// -1: Error
//  0: Success
//
int CheckStunSimple(LPTSTR lpStunServer, WORD wSrcPort, StunAddress4 *lpMappedResult);

//
// Return Value:
// -1: Error
//  0: Success
//
int CheckStunMyself(LPTSTR lpStunServer, WORD wSrcPort, StunAddress4 *lpMappedResult, BOOL *lpNoNAT, int *lpNatType, DWORD *lpTime);


void GetViewerNodeId(LPTSTR lpNodeId, int nBuffLen);

BOOL GetOsInfo(LPTSTR lpInfoBuff, int BuffSize);



std::string UrlEncode(const std::string& src);
std::string UrlDecode(const std::string& src);


// 注释：多字节包括GBK和UTF-8  
int GBK2UTF8(char *szGbk,char *szUtf8,int Len);

//UTF-8 GBK  
int UTF82GBK(char *szUtf8,char *szGbk,int Len);


void ConfigUdtSocket(UDTSOCKET fhandle);


void CopyTextToClipboard(const char *txt);


///////////////////////////////////////////////////////////////////

#define pthread_mutex_t				CRITICAL_SECTION
#define pthread_mutex_init(m, n)	InitializeCriticalSection(m)
#define pthread_mutex_destroy(m)	DeleteCriticalSection(m)
#define pthread_mutex_lock(m)		EnterCriticalSection(m)
#define pthread_mutex_unlock(m)		LeaveCriticalSection(m)

#define pthread_t					HANDLE

#define usleep(u)					Sleep((u)/1000)
