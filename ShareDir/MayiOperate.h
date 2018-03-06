#ifndef _MAYIOPERATE_H
#define _MAYIOPERATE_H

#include "platform.h"
#include "CommonLib.h"
#include "phpMd5.h"



#define DEFAULT_MAYI_SERVER		"tkz.tech007.tech"
#define MAYI_SERVER_PORT		80

#define MAYI_KEY_STRING  "tkz"



typedef struct _tag_notification_item {
	TCHAR text[MAX_LOADSTRING];
	TCHAR link[MAX_LOADSTRING];
	BYTE  type;
} NOTIFICATION_ITEM;

typedef struct _tag_guaji_node {
	DWORD id;
	char guaji_name[MAX_LOADSTRING];
	char guaji_type[MAX_LOADSTRING];
	char region[MAX_LOADSTRING];
	int total_used_times;
	int total_dashang_score;
	char predetermined_time[MAX_LOADSTRING];
	char comments_info[MAX_LOADSTRING];
	char owner_node_id[MAX_LOADSTRING];
	char control_node_id[MAX_LOADSTRING];
	char anypc_node_id[MAX_LOADSTRING];
	BOOL is_busy;//不在线 或者 已连接
	char network_type[MAX_LOADSTRING];
	int  network_delay;
} GUAJI_NODE;



extern char g1_login_key[PHP_MD5_OUTPUT_CHARS+1];
extern char g1_login_result[1024];

extern DWORD g1_user_id;
extern DWORD g1_user_score;
extern BOOL g1_user_activated;
extern char g1_expire_time[64];
extern char g1_last_login_time[64];
extern char g1_last_login_ip[64];

extern char g1_nickname[MAX_LOADSTRING];

extern BOOL g1_vip_filter_nodes;
extern int  g1_guaji_register_period;
extern int  g1_use_register_period;

extern char g1_def_security_hole[1024*8];
extern char g1_def_dashang_content[1024*8];
extern char g1_def_region[1024*8];




//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK.
//
int MayiLoginUser(const char *client_charset, const char *client_lang, const char *username, const char *password);


//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK
//
int MayiLogoutUser(const char *client_charset, const char *client_lang, DWORD user_id, const char *login_key);


//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK
//
int MayiAddNode(const char *client_charset, const char *client_lang, DWORD user_id, DWORD node_id, const char *login_key, char *resultBuff, int buffSize);


//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK
//
int MayiDelNode(const char *client_charset, const char *client_lang, DWORD user_id, DWORD node_id, const char *login_key, char *resultBuff, int buffSize);


//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK.
//
int MayiComplain(const char *client_charset, const char *client_lang, DWORD user_id, const char *user_username, DWORD guaji_id, const char *guaji_owner_username, const char *complain_content);


//
// Return Value:
// -1: Error
//  0: NG.
//  n: n>0, use_id
//
int MayiUseStart(const char *client_charset, const char *client_lang, DWORD user_id, const char *user_username, DWORD node_id,  DWORD guaji_id, const char *guaji_name, const char *login_key, char *resultBuff, int buffSize);


//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK.
//
int MayiUseRefresh(const char *client_charset, const char *client_lang, DWORD use_id);


//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK.
//
int MayiUseEnd(const char *client_charset, const char *client_lang, DWORD use_id, DWORD user_id, const char *user_username, DWORD node_id, DWORD guaji_id, const char *review_content);


//
// Return Value:
// -1: Error
//  0: Success
//
int MayiQueryIds(const char *client_charset, const char *client_lang, int user_id, int page_offset, int page_rows, char *ids_buff, int buff_size);


//
// Return Value:
// -1: Error
//  0: Success
//
int MayiQueryGuajiList(const char *client_charset, const char *client_lang, int user_id, int page_offset, int page_rows, GUAJI_NODE *nodesArray, int *lpCount, int *lpNum);


//
// Return Value:
// -1: Error
//  0: Success
//
int MayiSearchGuajiList(const char *client_charset, const char *client_lang, int user_id, const char *search_username, const char *search_subaccount, GUAJI_NODE *nodesArray, int *lpCount, int *lpNum);


//
// Return Value:
// -1: Error
//  0: Success
//
int MayiQueryNotificationList(NOTIFICATION_ITEM *notificationArray, int *lpCount);


#endif /* _MAYIOPERATE_H */
