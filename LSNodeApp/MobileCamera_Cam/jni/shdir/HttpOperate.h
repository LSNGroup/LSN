#ifndef _HTTPOPERATE_H
#define _HTTPOPERATE_H

#include "platform.h"
#include "CommonLib.h"


#if defined(FOR_WL_YKZ)

#define DEFAULT_HTTP_SERVER		"ykz.e2eye.com"
#define DEFAULT_STUN_SERVER		"Unknown" /* 可以用来测试是否可以访问HTTP Server */
#define HTTP_SERVER_PORT		80
#define HTTP_REGISTER_URL		"/cloudctrl/Register.php"
#define HTTP_REGISTER_REFERER	"/cloudctrl/Register.html"
#define HTTP_UNREGISTER_URL		"/cloudctrl/Unregister.php"
#define HTTP_UNREGISTER_REFERER	"/cloudctrl/Unregister.html"
#define HTTP_QUERYLIST_URL		"/cloudctrl/QueryNodeList.php"
#define HTTP_QUERYLIST_REFERER	"/cloudctrl/QueryNodeList.html"
#define HTTP_CONNECT_URL			"/cloudctrl/Connect.php"
#define HTTP_CONNECT_REFERER		"/cloudctrl/Connect.html"
#define HTTP_PROXY_URL				"/cloudctrl/Proxy.php"
#define HTTP_PROXY_REFERER			"/cloudctrl/Proxy.html"
#define HTTP_SENDEMAIL_URL			"/cloudctrl/SendEmail.php"
#define HTTP_SENDEMAIL_REFERER		"/cloudctrl/SendEmail.html"
#define HTTP_PUTLOCATION_URL		"/cloudctrl/PutLocation.php"
#define HTTP_PUTLOCATION_REFERER	"/cloudctrl/PutLocation.html"
#define HTTP_PUTWPLOCATION_URL		"/cloudctrl/PutWPLocation.php"
#define HTTP_PUTWPLOCATION_REFERER	"/cloudctrl/PutWPLocation.html"
#define HTTP_QUERYNOTIFICATIONLIST_URL		"/cloudctrl/QueryNotificationList.php"
#define HTTP_QUERYNOTIFICATIONLIST_REFERER	"/cloudctrl/QueryNotificationList.html"

#elif defined(FOR_51HZ_GUAJI)

#define DEFAULT_HTTP_SERVER		"www.liren100.cn"
#define DEFAULT_STUN_SERVER		"Unknown" /* 可以用来测试是否可以访问HTTP Server */
#define HTTP_SERVER_PORT		80
#define HTTP_REGISTER_URL		"/mayictrl/Register.php"
#define HTTP_REGISTER_REFERER	"/mayictrl/Register.html"
#define HTTP_UNREGISTER_URL		"/mayictrl/Unregister.php"
#define HTTP_UNREGISTER_REFERER	"/mayictrl/Unregister.html"
#define HTTP_QUERYLIST_URL		"/mayictrl/QueryNodeList.php"
#define HTTP_QUERYLIST_REFERER	"/mayictrl/QueryNodeList.html"
#define HTTP_CONNECT_URL			"/mayictrl/Connect.php"
#define HTTP_CONNECT_REFERER		"/mayictrl/Connect.html"
#define HTTP_PROXY_URL				"/mayictrl/Proxy.php"
#define HTTP_PROXY_REFERER			"/mayictrl/Proxy.html"
//#define HTTP_SENDEMAIL_URL			"/mayictrl/SendEmail.php"
//#define HTTP_SENDEMAIL_REFERER		"/mayictrl/SendEmail.html"
//#define HTTP_PUTLOCATION_URL		"/mayictrl/PutLocation.php"
//#define HTTP_PUTLOCATION_REFERER	"/mayictrl/PutLocation.html"
//#define HTTP_PUTWPLOCATION_URL		"/mayictrl/PutWPLocation.php"
//#define HTTP_PUTWPLOCATION_REFERER	"/mayictrl/PutWPLocation.html"
#define HTTP_QUERYNOTIFICATIONLIST_URL		"/mayictrl/QueryNotificationList.php"
#define HTTP_QUERYNOTIFICATIONLIST_REFERER	"/mayictrl/QueryNotificationList.html"

#elif defined(FOR_MAYI_GUAJI)
 /* ... */
#endif


#define POST_RESULT_START		"FLAG5TGB6YHNSTART"
#define POST_RESULT_END			"FLAG5TGB6YHNEND"

#define DEFAULT_REGISTER_PERIOD		10
#define DEFAULT_EXPIRE_PERIOD		25



extern char g0_device_uuid[MAX_LOADSTRING];
extern BYTE g0_node_id[6];
extern char g0_node_name[MAX_LOADSTRING];
extern DWORD g0_version;
extern DWORD g0_ipArray[MAX_ADAPTER_NUM];  /* Network byte order */
extern int g0_ipCount;
extern WORD g0_port;
extern DWORD g0_pub_ip;  /* Network byte order */
extern WORD g0_pub_port;
extern BOOL g0_no_nat;
extern int  g0_nat_type;
extern BOOL g0_is_admin;
extern BOOL g0_is_busy;
extern DWORD g0_audio_channels;
extern DWORD g0_video_channels;
extern char g0_os_info[MAX_LOADSTRING];

extern BOOL g1_is_active;
extern DWORD g1_lowest_version;
extern DWORD g1_admin_lowest_version;
extern char g1_http_server[MAX_LOADSTRING];
extern char g1_stun_server[MAX_LOADSTRING];
extern DWORD g1_http_client_ip;  /* Network byte order */
extern int g1_register_period;  /* Seconds */
extern int g1_expire_period;  /* Seconds */
extern int g1_lowest_level_for_av;
extern int g1_lowest_level_for_vnc;
extern int g1_lowest_level_for_ft;
extern int g1_lowest_level_for_adb;
extern int g1_lowest_level_for_webmoni;
extern int g1_lowest_level_allow_hide;

//#ifdef JNI_FOR_MOBILECAMERA
extern BOOL g1_is_activated;
extern DWORD g1_comments_id;
//#endif

extern BYTE g1_peer_node_id[6];
extern DWORD g1_peer_ip;  /* Network byte order */
extern WORD g1_peer_port;
extern BOOL g1_peer_nonat;
extern int  g1_peer_nattype;
extern DWORD g1_peer_pri_ipArray[MAX_ADAPTER_NUM];  /* Network byte order */
extern int g1_peer_pri_ipCount;
extern WORD g1_peer_pri_port;
extern int g1_wait_time;
extern DWORD g1_use_peer_ip;  /* Network byte order */
extern WORD g1_use_peer_port;
extern UDPSOCKET g1_use_udp_sock;
extern UDTSOCKET g1_use_udt_sock;
extern SOCKET_TYPE g1_use_sock_type;


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


#if defined(FOR_WL_YKZ)

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
//  1: OK.
//
int DoPutLocation(const char *client_charset, const char *client_lang, DWORD put_time, int num, const char *items);


//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK.
//
int DoPutWPLocation(const char *client_charset, const char *client_lang, DWORD cam_id, DWORD put_time, bool wp_ignore, const char *wp_content, const char *loc_item);

#endif


//
// Return Value:
// -1: Error
//  0: Success
//
int DoQueryNotificationList(const char *client_charset, const char *client_lang, NOTIFICATION_ITEM *notificationArray, int *lpCount);


#endif /* _HTTPOPERATE_H */



#if (defined(FOR_51HZ_GUAJI) || defined(FOR_MAYI_GUAJI))
#include "MayiOperate.h"
#endif
