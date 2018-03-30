#ifndef _HTTPOPERATE_H
#define _HTTPOPERATE_H

#include "platform.h"
#include "CommonLib.h"


#define DEFAULT_HTTP_SERVER		"ykz.e2eye.com"
#define DEFAULT_STUN_SERVER		"Unknown" /* 可以用来测试是否可以访问HTTP Server */
#define HTTP_SERVER_PORT		80

#define DEFAULT_REGISTER_PERIOD		10
#define DEFAULT_EXPIRE_PERIOD		25



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
	//  0: Should exit.
	//  1: Should stop.
	//  2: OK, continue.
	//
	int DoRegister1(const char *client_charset, const char *client_lang);


	//
	// Return Value:
	// -1: Error
	//  0: Should exit.
	//  1: Should stop.
	//  2: OK, continue.
	//  3: Setup connection
	//
	int DoRegister2(const char *client_charset, const char *client_lang);


	//
	// Return Value:
	// -1: Error
	//  0: NG.
	//  1: OK.
	//
	int DoUnregister(const char *client_charset, const char *client_lang);


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
	//  0: NG.
	//  1: OK.
	//
	int DoProxy(const char *client_charset, const char *client_lang, char *his_node_id_str, BOOL bIsTcp = FALSE, WORD wFuncPort = 0);


	//
	// Return Value:
	// -1: Error
	//  0: NG.
	//  1: OK.
	//
	int DoSendEmail(const char *client_charset, const char *client_lang, const char *to_email, const char *subject, const char *content);


	//
	// Return Value:
	// -1: Error
	//  0: NG.
	//  n: n>0  comments_id
	//
	static int DoFetchNode(const char *client_charset, const char *client_lang, const char *external_app_name, const char *external_app_key, const char *type, const char *region, char passwd_buff[], int buff_size, char desc_buff[], int desc_size);

};


#endif /* _HTTPOPERATE_H */
