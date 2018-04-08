#ifndef _HTTPOPERATE_H
#define _HTTPOPERATE_H

#include "platform.h"
#include "CommonLib.h"


#define DEFAULT_HTTP_SERVER		"www.e2eye.com"
#define DEFAULT_STUN_SERVER		"Unknown" /* 可以用来测试是否可以访问HTTP Server */
#define HTTP_SERVER_PORT		80

#define DEFAULT_REGISTER_PERIOD		10
#define DEFAULT_EXPIRE_PERIOD		25


typedef void (*ON_REPORT_SETTINGS_FN)(char *settings_str);
typedef void (*ON_REPORT_EVENT_FN)(char *event_str);


extern char g0_device_uuid[MAX_LOADSTRING];
extern char g0_node_name[MAX_LOADSTRING];
extern DWORD g0_version;
extern char g0_os_info[MAX_LOADSTRING];

extern BOOL g1_is_active;
extern DWORD g1_lowest_version;
extern char g1_http_server[MAX_LOADSTRING];
extern char g1_stun_server[MAX_LOADSTRING];
extern char g1_measure_server[MAX_LOADSTRING];
extern int g1_register_period;  /* Seconds */
extern int g1_expire_period;  /* Seconds */

//#ifdef JNI_FOR_MOBILECAMERA
extern BOOL g1_is_activated;
extern DWORD g1_comments_id;
//#endif


BOOL ParseLine(char *start, char *name, int name_size, char *value, int value_size, char **next);

int ParseTopoSettings(const char *settings_string);


class HttpOperate
{
public:
	HttpOperate(BOOL is_admin = TRUE, WORD p2p_port = FIRST_CONNECT_PORT);
	virtual ~HttpOperate();

public:
	BYTE m0_node_id[6];
	BOOL m0_is_admin;
	BOOL m0_is_busy;

	WORD m0_p2p_port;
//{{{{<------------------------------------------------------
	DWORD m0_ipArray[MAX_ADAPTER_NUM];  /* Network byte order */
	int m0_ipCount;
	WORD m0_port;
	DWORD m0_pub_ip;  /* Network byte order */
	WORD m0_pub_port;
	BOOL m0_no_nat;
	int  m0_nat_type;
	DWORD m0_net_time;
//}}}}<------------------------------------------------------
	DWORD m1_http_client_ip;  /* Network byte order */  ////<------------------------
	
	BYTE m1_peer_node_id[6];
	DWORD m1_peer_ip;  /* Network byte order */
	WORD m1_peer_port;
	BOOL m1_peer_nonat;
	int  m1_peer_nattype;
	DWORD m1_peer_pri_ipArray[MAX_ADAPTER_NUM];  /* Network byte order */
	int m1_peer_pri_ipCount;
	WORD m1_peer_pri_port;
	int m1_wait_time;
	DWORD m1_use_peer_ip;  /* Network byte order */
	WORD m1_use_peer_port;
	UDPSOCKET m1_use_udp_sock;
	UDTSOCKET m1_use_udt_sock;
	SOCKET_TYPE m1_use_sock_type;
	char m1_event_type[32];

public:
	
	const char *MakeIpStr();
	const char *MakePubIpStr();
	BOOL ParseIpValue(char *value);
	BOOL ParseEventValue(char *value);
	BOOL ParseHisInfoValue(char *value);
	

	//
	// Return Value:
	// -1: Error
	//  0: Success
	//
	int DoQueryList(const char *client_charset, const char *client_lang, const char *request_nodes, ANYPC_NODE *nodesArray, int *lpCount, int *lpNum);


	//
	// Return Value:
	// -1: Error
	//  0: NG.
	//  1: OK1.
	//  2: OK2.
	//
	int DoConnect(const char *client_charset, const char *client_lang, char *his_node_id_str);


	//
	// Return Value:
	// -1: Error
	//  0: Should stop.
	//  1: OK.
	//  2: settings
	//  3: settings,event
	static int DoReport1(const char *client_charset, const char *client_lang, ON_REPORT_SETTINGS_FN on_settings_fn);


	//
	// Return Value:
	// -1: Error
	//  0: Should stop.
	//  1: OK.
	//  2: settings
	//  3: settings,event
	static int DoReport2(const char *client_charset, const char *client_lang, 
		DWORD joined_channel_id, BYTE joined_node_id[6],
		const char *root_device_uuid, const char *root_public_ip, BYTE device_node_id[6],
		int route_item_num, int route_item_max,
		int level_1_max_connections, int level_1_current_connections, int level_1_max_streams, int level_1_current_streams,
		int level_2_max_connections, int level_2_current_connections, int level_2_max_streams, int level_2_current_streams,
		int level_3_max_connections, int level_3_current_connections, int level_3_max_streams, int level_3_current_streams,
		int level_4_max_connections, int level_4_current_connections, int level_4_max_streams, int level_4_current_streams,
		const char *node_array, //待连接Node集合的String数据(,;)
		ON_REPORT_SETTINGS_FN on_settings_fn, ON_REPORT_EVENT_FN on_event_fn);


	//
	// Return Value:
	// -1: Error
	//  0: NG.
	//  1: OK.
	//
	static int DoDrop(const char *client_charset, const char *client_lang, BYTE sender_node_id[6], BOOL is_admin, BYTE node_id[6]);


	//
	// Return Value:
	// -1: Error
	//  0: NG.
	//  1: OK.
	//
	static int DoEvaluate(const char *client_charset, const char *client_lang, BYTE sender_node_id[6], const char *record_array);
};


#endif /* _HTTPOPERATE_H */
