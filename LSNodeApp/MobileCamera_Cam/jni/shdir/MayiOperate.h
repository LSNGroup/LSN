#ifndef _MAYIOPERATE_H
#define _MAYIOPERATE_H

#include "platform.h"
#include "CommonLib.h"



#define DEFAULT_MAYI_SERVER		"www.liren100.cn"
#define MAYI_SERVER_PORT		80

#define MAYI_KEY_STRING  "51hz"



typedef struct _tag_use_node {
	DWORD id;
	char user_username[MAX_LOADSTRING];
	char guaji_owner_username[MAX_LOADSTRING];
	char platform_type[MAX_LOADSTRING];
	char sub_account[MAX_LOADSTRING];
	char goods_category[MAX_LOADSTRING];
	char shop_name[MAX_LOADSTRING];
	char order_num[MAX_LOADSTRING];
	char order_money[MAX_LOADSTRING];
	char str_buy_time[MAX_LOADSTRING];
	char str_review_time[MAX_LOADSTRING];
	char review_content[1024*6];
	char review_extra[MAX_LOADSTRING];
	char str_ack_time[MAX_LOADSTRING];
	char str_op_status[MAX_LOADSTRING];
} USE_NODE;



extern char g0_mac_addr[MAX_LOADSTRING];
extern char g0_dial_user[MAX_LOADSTRING];
extern char g0_osys_info[MAX_LOADSTRING];
extern char g0_cpu_info[MAX_LOADSTRING];
extern char g0_ram_info[MAX_LOADSTRING];
extern char g0_disk_info[MAX_LOADSTRING];
extern char g0_vidcard_info[MAX_LOADSTRING];
extern char g0_monitor_info[MAX_LOADSTRING];
extern char g0_login_ip[MAX_LOADSTRING];
extern char g0_network_type[MAX_LOADSTRING];
extern int g0_network_delay;




extern char g1_login_result[1024];

extern DWORD g1_user_id;
extern DWORD g1_user_score;
extern char g1_expire_time[64];
extern char g1_last_login_time[64];
extern char g1_last_login_ip[64];

extern char g1_related_info[1024];
extern int  g1_guaji_register_period;
extern int  g1_use_register_period;

extern char g1_def_platform_type[1024];
extern char g1_def_goods_category[1024*8];
extern char g1_def_region[1024*8];


extern DWORD g1_guaji_id;
extern char g1_sub_accounts[MAX_LOADSTRING];
extern int g1_total_used_times;
extern int g1_total_complained_times;
extern char g1_auth_info[MAX_LOADSTRING];
extern char g1_comments_info[MAX_LOADSTRING];
extern int g1_total_guaji_time;
extern BOOL g1_check_adb_device;



void GetLocalGuajiInfo();



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
//  1: OK.
//
int MayiUseAck(const char *client_charset, const char *client_lang, DWORD use_id, DWORD guaji_id, const char *guaji_owner_username);


//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK.
//
int MayiUsePay(const char *client_charset, const char *client_lang, DWORD use_id, DWORD user_id, const char *user_username, DWORD guaji_id, const char *guaji_owner_username);


//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK.
//
int MayiFetchScore(const char *client_charset, const char *client_lang, const char *username, DWORD guaji_id, int *p_user_score, int *p_cash_quota, int *p_jifen_duihuan_jiage);


//
// Return Value:
// -1: Error
//  0: NG, Should stop.
//  1: OK, Should continue.
//
int MayiRegister(const char *client_charset, const char *client_lang, BOOL bIsOnline, DWORD owner_user_id, const char *owner_username, char *resultBuff, int buffSize);


//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK.
//
int MayiUnregister(const char *client_charset, const char *client_lang);


//
// Return Value:
// -1: Error
//  0: Success
//
int MayiQueryUseList(const char *client_charset, const char *client_lang, int user_id, int guaji_id, USE_NODE *nodesArray, int *lpCount, int *lpNum);


#endif /* _MAYIOPERATE_H */
