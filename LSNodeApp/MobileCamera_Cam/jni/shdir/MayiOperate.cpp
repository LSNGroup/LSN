//#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "CommonLib.h"
#include "phpMd5.h"
#include "TWSocket.h"
#include "HttpOperate.h"
#include "MayiOperate.h"

#ifdef ANDROID_NDK
#include <android/log.h>
#endif


#define MAYI_LOGINUSER_URL		"/huzhu/LoginUser.php"
#define MAYI_LOGINUSER_REFERER	"/huzhu/LoginUser.html"
#define MAYI_USESTART_URL		"/huzhu/UseStart.php"
#define MAYI_USESTART_REFERER	"/huzhu/UseStart.html"
#define MAYI_USEREFRESH_URL		"/huzhu/UseRefresh.php"
#define MAYI_USEREFRESH_REFERER	"/huzhu/UseRefresh.html"
#define MAYI_USEEND_URL			"/huzhu/UseEnd.php"
#define MAYI_USEEND_REFERER		"/huzhu/UseEnd.html"
#define MAYI_USEACK_URL			"/huzhu/UseAck.php"
#define MAYI_USEACK_REFERER		"/huzhu/UseAck.html"
#define MAYI_USEPAY_URL			"/huzhu/UsePay.php"
#define MAYI_USEPAY_REFERER		"/huzhu/UsePay.html"
#define MAYI_FETCHSCORE_URL			"/huzhu/FetchScore.php"
#define MAYI_FETCHSCORE_REFERER		"/huzhu/FetchScore.html"
#define MAYI_COMPLAIN_URL		"/huzhu/Complain.php"
#define MAYI_COMPLAIN_REFERER	"/huzhu/Complain.html"
#define MAYI_REGISTER_URL		"/huzhu/GuajiRegister.php"
#define MAYI_REGISTER_REFERER	"/huzhu/GuajiRegister.html"
#define MAYI_UNREGISTER_URL		"/huzhu/GuajiUnregister.php"
#define MAYI_UNREGISTER_REFERER	"/huzhu/GuajiUnregister.html"
#define MAYI_QUERYGUAJILIST_URL				"/huzhu/QueryGuajiList.php"
#define MAYI_QUERYGUAJILIST_REFERER			"/huzhu/QueryGuajiList.html"
#define MAYI_SEARCHGUAJILIST_URL				"/huzhu/SearchGuajiList.php"
#define MAYI_SEARCHGUAJILIST_REFERER			"/huzhu/SearchGuajiList.html"
#define MAYI_QUERYUSELIST_URL				"/huzhu/QueryUseList.php"
#define MAYI_QUERYUSELIST_REFERER			"/huzhu/QueryUseList.html"
#define MAYI_QUERYNOTIFICATIONLIST_URL		"/huzhu/QueryNotificationList.php"
#define MAYI_QUERYNOTIFICATIONLIST_REFERER	"/huzhu/QueryNotificationList.html"


char g0_mac_addr[MAX_LOADSTRING];
char g0_dial_user[MAX_LOADSTRING];
char g0_osys_info[MAX_LOADSTRING];
char g0_cpu_info[MAX_LOADSTRING];
char g0_ram_info[MAX_LOADSTRING];
char g0_disk_info[MAX_LOADSTRING];
char g0_vidcard_info[MAX_LOADSTRING];
char g0_monitor_info[MAX_LOADSTRING];
char g0_login_ip[MAX_LOADSTRING];
char g0_network_type[MAX_LOADSTRING];
int g0_network_delay;




char g1_login_result[1024] = "登录成功！";

DWORD g1_user_id = 0;
DWORD g1_user_score = 0;
char g1_expire_time[64];
char g1_last_login_time[64];
char g1_last_login_ip[64];

char g1_related_info[1024];//用户关联的店铺集合
int  g1_guaji_register_period = 15;
int  g1_use_register_period = 15;

char g1_def_platform_type[1024];
char g1_def_goods_category[1024*8];
char g1_def_region[1024*8];


DWORD g1_guaji_id = 0;
char g1_sub_accounts[MAX_LOADSTRING];
int g1_total_used_times;
int g1_total_complained_times;
char g1_auth_info[MAX_LOADSTRING];
char g1_comments_info[MAX_LOADSTRING];
int g1_total_guaji_time;
BOOL g1_check_adb_device = FALSE;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static DWORD dwMayiServerIp = 0;
static char szMayiIpAddr[16];


void GetLocalGuajiInfo()
{
	extern const char *MakePubIpStr();

	strncpy(g0_mac_addr, g0_device_uuid + 12, 17);

	strncpy(g0_dial_user, "通过局域网", sizeof(g0_dial_user));

	strncpy(g0_osys_info, g0_os_info, sizeof(g0_osys_info));

	snprintf(g0_cpu_info, sizeof(g0_cpu_info), "...");

	snprintf(g0_ram_info, sizeof(g0_ram_info), "...");

	snprintf(g0_disk_info, sizeof(g0_disk_info), "...");

	snprintf(g0_vidcard_info, sizeof(g0_vidcard_info), "...");

	snprintf(g0_monitor_info, sizeof(g0_monitor_info), "...");


	strncpy(g0_login_ip, MakePubIpStr(), sizeof(g0_login_ip));

	srand(time(NULL));
	g0_network_delay = 50 + (rand() % 50);
}



//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK.
//
int MayiLoginUser(const char *client_charset, const char *client_lang, const char *username, const char *password)
{
#define POST_BUFF_SIZE	(1024*16)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+16];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024];
	char *szPost;
	char *start, *end;
	int result = 0; /* NG */
	char name[32];
	char value[1024*8];
	char *next_start = NULL;


	if (username == NULL || password == NULL) {
		return -1;
	}

	if (dwMayiServerIp == 0) {
		if (TWSocket::GetIPAddr(DEFAULT_MAYI_SERVER, szMayiIpAddr, sizeof(szMayiIpAddr)) == 0) {
			return -1;
		}
		dwMayiServerIp = inet_addr(szMayiIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szMayiIpAddr, MAYI_SERVER_PORT) != 0) {
		dwMayiServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			"username=%s"
			"&password=%s"
			"&client_charset=%s"
			"&client_lang=%s",
			username,password,
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_LOGINUSER_URL,
										DEFAULT_MAYI_SERVER, MAYI_LOGINUSER_REFERER,
										DEFAULT_MAYI_SERVER,
										strlen(szPostBody),
										szPostBody);

	if (sockClient.Send_Stream(szPost, strlen(szPost)) < 0) {
		sockClient.CloseSocket();
		free(szPost);
		return -1;
	}


	if (RecvHttpResponse(&sockClient, szPost, POST_BUFF_SIZE, &start, &end) < 0) {
		sockClient.CloseSocket();
		free(szPost);
		return -1;
	}


	while(1) {

		if (ParseLine(start, name, sizeof(name), value, sizeof(value), &next_start) == FALSE) {
			sockClient.CloseSocket();
			free(szPost);
			return -1;
		}


		if (strcmp(value, "") != 0)
		{
			if (strcmp(name, "result_code") == 0) {
				if (strcmp(value, "OK") == 0) {
					result = 1;
				}
			}
			else if (strcmp(name, "result_str") == 0) {
				strncpy(g1_login_result, value, sizeof(g1_login_result));
			}

			else if (strcmp(name, "user_id") == 0) {
				g1_user_id = atol(value);
			}
			else if (strcmp(name, "user_score") == 0) {
				g1_user_score = atol(value);
			}
			else if (strcmp(name, "expire_time") == 0) {
				strncpy(g1_expire_time, value, sizeof(g1_expire_time));
			}
			else if (strcmp(name, "last_login_time") == 0) {
				strncpy(g1_last_login_time, value, sizeof(g1_last_login_time));
			}
			else if (strcmp(name, "last_login_ip") == 0) {
				strncpy(g1_last_login_ip, value, sizeof(g1_last_login_ip));
			}

			else if (strcmp(name, "related_info") == 0) {
				strncpy(g1_related_info, value, sizeof(g1_related_info));
			}
			else if (strcmp(name, "guaji_register_period") == 0) {
				g1_guaji_register_period = atol(value);
			}
			else if (strcmp(name, "use_register_period") == 0) {
				g1_use_register_period = atol(value);
			}

			else if (strcmp(name, "def_platform_type") == 0) {
				strncpy(g1_def_platform_type, value, sizeof(g1_def_platform_type));
			}
			else if (strcmp(name, "def_goods_category") == 0) {
				strncpy(g1_def_goods_category, value, sizeof(g1_def_goods_category));
			}
			else if (strcmp(name, "def_region") == 0) {
				strncpy(g1_def_region, value, sizeof(g1_def_region));
			}
		}


		if (next_start == NULL) {
			break;
		}
		start = next_start;
	}


	sockClient.ShutDown();
	sockClient.CloseSocket();

	free(szPost);
	return result;

#undef POST_BUFF_SIZE
}


//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK.
//
int MayiUseAck(const char *client_charset, const char *client_lang, DWORD use_id, DWORD guaji_id, const char *guaji_owner_username)
{
#define POST_BUFF_SIZE	(1024)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+16];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024];
	char *szPost;
	char *start, *end;
	int result = 0; /* NG */
	char name[32];
	char value[1024];
	char *next_start = NULL;


	if (dwMayiServerIp == 0) {
		if (TWSocket::GetIPAddr(DEFAULT_MAYI_SERVER, szMayiIpAddr, sizeof(szMayiIpAddr)) == 0) {
			return -1;
		}
		dwMayiServerIp = inet_addr(szMayiIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szMayiIpAddr, MAYI_SERVER_PORT) != 0) {
		dwMayiServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			"use_id=%ld"
			"&guaji_id=%ld"
			"&guaji_owner_username=%s"
			"&client_charset=%s"
			"&client_lang=%s",
			use_id,
			guaji_id,guaji_owner_username,
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_USEACK_URL,
										DEFAULT_MAYI_SERVER, MAYI_USEACK_REFERER,
										DEFAULT_MAYI_SERVER,
										strlen(szPostBody),
										szPostBody);

	if (sockClient.Send_Stream(szPost, strlen(szPost)) < 0) {
		sockClient.CloseSocket();
		free(szPost);
		return -1;
	}


	if (RecvHttpResponse(&sockClient, szPost, POST_BUFF_SIZE, &start, &end) < 0) {
		sockClient.CloseSocket();
		free(szPost);
		return -1;
	}


	while(1) {

		if (ParseLine(start, name, sizeof(name), value, sizeof(value), &next_start) == FALSE) {
			sockClient.CloseSocket();
			free(szPost);
			return -1;
		}


		if (strcmp(value, "") != 0)
		{
			if (strcmp(name, "result_code") == 0) {
				if (strcmp(value, "OK") == 0) {
					result = 1;
				}
			}
		}


		if (next_start == NULL) {
			break;
		}
		start = next_start;
	}


	sockClient.ShutDown();
	sockClient.CloseSocket();

	free(szPost);
	return result;

#undef POST_BUFF_SIZE
}


//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK.
//
int MayiUsePay(const char *client_charset, const char *client_lang, DWORD use_id, DWORD user_id, const char *user_username, DWORD guaji_id, const char *guaji_owner_username)
{
#define POST_BUFF_SIZE	(1024)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+16];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024];
	char *szPost;
	char *start, *end;
	int result = 0; /* NG */
	char name[32];
	char value[1024];
	char *next_start = NULL;


	if (dwMayiServerIp == 0) {
		if (TWSocket::GetIPAddr(DEFAULT_MAYI_SERVER, szMayiIpAddr, sizeof(szMayiIpAddr)) == 0) {
			return -1;
		}
		dwMayiServerIp = inet_addr(szMayiIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szMayiIpAddr, MAYI_SERVER_PORT) != 0) {
		dwMayiServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			"use_id=%ld"
			"&user_id=%ld"
			"&user_username=%s"
			"&guaji_id=%ld"
			"&guaji_owner_username=%s"
			"&client_charset=%s"
			"&client_lang=%s",
			use_id,
			user_id,user_username,
			guaji_id,guaji_owner_username,
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_USEPAY_URL,
										DEFAULT_MAYI_SERVER, MAYI_USEPAY_REFERER,
										DEFAULT_MAYI_SERVER,
										strlen(szPostBody),
										szPostBody);

	if (sockClient.Send_Stream(szPost, strlen(szPost)) < 0) {
		sockClient.CloseSocket();
		free(szPost);
		return -1;
	}


	if (RecvHttpResponse(&sockClient, szPost, POST_BUFF_SIZE, &start, &end) < 0) {
		sockClient.CloseSocket();
		free(szPost);
		return -1;
	}


	while(1) {

		if (ParseLine(start, name, sizeof(name), value, sizeof(value), &next_start) == FALSE) {
			sockClient.CloseSocket();
			free(szPost);
			return -1;
		}


		if (strcmp(value, "") != 0)
		{
			if (strcmp(name, "result_code") == 0) {
				if (strcmp(value, "OK") == 0) {
					result = 1;
				}
			}
		}


		if (next_start == NULL) {
			break;
		}
		start = next_start;
	}


	sockClient.ShutDown();
	sockClient.CloseSocket();

	free(szPost);
	return result;

#undef POST_BUFF_SIZE
}


//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK.
//
int MayiFetchScore(const char *client_charset, const char *client_lang, const char *username, DWORD guaji_id, int *p_user_score, int *p_cash_quota, int *p_jifen_duihuan_jiage)
{
#define POST_BUFF_SIZE	(1024)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+16];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024];
	char *szPost;
	char *start, *end;
	int result = 0; /* NG */
	char name[32];
	char value[1024];
	char *next_start = NULL;


	if (dwMayiServerIp == 0) {
		if (TWSocket::GetIPAddr(DEFAULT_MAYI_SERVER, szMayiIpAddr, sizeof(szMayiIpAddr)) == 0) {
			return -1;
		}
		dwMayiServerIp = inet_addr(szMayiIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szMayiIpAddr, MAYI_SERVER_PORT) != 0) {
		dwMayiServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			"username=%s"
			"&guaji_id=%ld"
			"&client_charset=%s"
			"&client_lang=%s",
			username,
			guaji_id,
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_FETCHSCORE_URL,
										DEFAULT_MAYI_SERVER, MAYI_FETCHSCORE_REFERER,
										DEFAULT_MAYI_SERVER,
										strlen(szPostBody),
										szPostBody);

	if (sockClient.Send_Stream(szPost, strlen(szPost)) < 0) {
		sockClient.CloseSocket();
		free(szPost);
		return -1;
	}


	if (RecvHttpResponse(&sockClient, szPost, POST_BUFF_SIZE, &start, &end) < 0) {
		sockClient.CloseSocket();
		free(szPost);
		return -1;
	}


	while(1) {

		if (ParseLine(start, name, sizeof(name), value, sizeof(value), &next_start) == FALSE) {
			sockClient.CloseSocket();
			free(szPost);
			return -1;
		}


		if (strcmp(value, "") != 0)
		{
			if (strcmp(name, "result_code") == 0) {
				if (strcmp(value, "OK") == 0) {
					result = 1;
				}
			}
			else if (strcmp(name, "user_score") == 0) {
				if (p_user_score != NULL) {
					*p_user_score = atol(value);
				}
			}
			else if (strcmp(name, "cash_quota") == 0) {
				if (p_cash_quota != NULL) {
					*p_cash_quota = atol(value);
				}
			}
			else if (strcmp(name, "jifen_duihuan_jiage") == 0) {
				if (p_jifen_duihuan_jiage != NULL) {
					*p_jifen_duihuan_jiage = atol(value);
				}
			}
		}


		if (next_start == NULL) {
			break;
		}
		start = next_start;
	}


	sockClient.ShutDown();
	sockClient.CloseSocket();

	free(szPost);
	return result;

#undef POST_BUFF_SIZE
}


//
// Return Value:
// -1: Error
//  0: NG, Should stop.
//  1: OK, Should continue.
//
int MayiRegister(const char *client_charset, const char *client_lang, BOOL bIsOnline, DWORD owner_user_id, const char *owner_username, char *resultBuff, int buffSize)
{
#define POST_BUFF_SIZE	(1024*4)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+16];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024*4];
	char *szPost;
	char *start, *end;
	int result = 0; /* NG */
	char name[32];
	char value[1024];
	char *next_start = NULL;


	if (dwMayiServerIp == 0) {
		if (TWSocket::GetIPAddr(DEFAULT_MAYI_SERVER, szMayiIpAddr, sizeof(szMayiIpAddr)) == 0) {
			return -1;
		}
		dwMayiServerIp = inet_addr(szMayiIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szMayiIpAddr, MAYI_SERVER_PORT) != 0) {
		dwMayiServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			"is_online=%d"
			"&node_id=%ld"
			"&owner_user_id=%ld"
			"&owner_username=%s"
			"&mac_addr=%s"
			"&dial_user=%s"
			"&os_info=%s"
			"&cpu_info=%s"
			"&ram_info=%s"
			"&disk_info=%s"
			"&vidcard_info=%s"
			"&monitor_info=%s"
			"&login_ip=%s"
			//"&network_type=%s"
			"&network_delay=%d"
			"&client_charset=%s"
			"&client_lang=%s",
			(bIsOnline ? 1 : 0),
			g1_comments_id,
			owner_user_id,owner_username,
			g0_mac_addr,
			UrlEncode(g0_dial_user).c_str(),
			UrlEncode(g0_osys_info).c_str(),
			UrlEncode(g0_cpu_info).c_str(),
			UrlEncode(g0_ram_info).c_str(),
			UrlEncode(g0_disk_info).c_str(),
			UrlEncode(g0_vidcard_info).c_str(),
			UrlEncode(g0_monitor_info).c_str(),
			g0_login_ip,
			//UrlEncode(g0_network_type).c_str(),
			g0_network_delay,
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_REGISTER_URL,
										DEFAULT_MAYI_SERVER, MAYI_REGISTER_REFERER,
										DEFAULT_MAYI_SERVER,
										strlen(szPostBody),
										szPostBody);

	if (sockClient.Send_Stream(szPost, strlen(szPost)) < 0) {
		sockClient.CloseSocket();
		free(szPost);
		return -1;
	}


	if (RecvHttpResponse(&sockClient, szPost, POST_BUFF_SIZE, &start, &end) < 0) {
		sockClient.CloseSocket();
		free(szPost);
		return -1;
	}


	while(1) {

		if (ParseLine(start, name, sizeof(name), value, sizeof(value), &next_start) == FALSE) {
			sockClient.CloseSocket();
			free(szPost);
			return -1;
		}


		if (strcmp(value, "") != 0)
		{
			if (strcmp(name, "result_code") == 0) {
				if (strcmp(value, "OK") == 0) {
					result = 1;
				}
			}
			else if (strcmp(name, "result_str") == 0) {
				strncpy(resultBuff, value, buffSize);
			}
			else if (strcmp(name, "guaji_id") == 0) {
				g1_guaji_id = atol(value);
			}
			else if (strcmp(name, "sub_accounts") == 0) {
				strncpy(g1_sub_accounts, value, sizeof(g1_sub_accounts));
			}
			else if (strcmp(name, "total_used_times") == 0) {
				g1_total_used_times = atol(value);
			}
			else if (strcmp(name, "total_complained_times") == 0) {
				g1_total_complained_times = atol(value);
			}
			else if (strcmp(name, "auth_info") == 0) {
				strncpy(g1_auth_info, value, sizeof(g1_auth_info));
			}
			else if (strcmp(name, "comments_info") == 0) {
				strncpy(g1_comments_info, value, sizeof(g1_comments_info));
			}
			else if (strcmp(name, "total_guaji_time") == 0) {
				g1_total_guaji_time = atol(value);
			}
			else if (strcmp(name, "check_adb_device") == 0) {
				g1_check_adb_device = (atol(value) == 1);
			}
			else if (strcmp(name, "user_score") == 0) {
				g1_user_score = atol(value);//覆盖登录时得到的值，不断更新
			}
		}


		if (next_start == NULL) {
			break;
		}
		start = next_start;
	}


	sockClient.ShutDown();
	sockClient.CloseSocket();

	free(szPost);
	return result;

#undef POST_BUFF_SIZE
}


//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK.
//
int MayiUnregister(const char *client_charset, const char *client_lang)
{
#define POST_BUFF_SIZE	(1024)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+16];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024];
	char *szPost;
	char *start, *end;
	int result = 0; /* NG */
	char name[32];
	char value[1024];
	char *next_start = NULL;


	if (dwMayiServerIp == 0) {
		if (TWSocket::GetIPAddr(DEFAULT_MAYI_SERVER, szMayiIpAddr, sizeof(szMayiIpAddr)) == 0) {
			return -1;
		}
		dwMayiServerIp = inet_addr(szMayiIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szMayiIpAddr, MAYI_SERVER_PORT) != 0) {
		dwMayiServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			"guaji_id=%ld"
			"&client_charset=%s"
			"&client_lang=%s",
			g1_guaji_id,
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_UNREGISTER_URL,
										DEFAULT_MAYI_SERVER, MAYI_UNREGISTER_REFERER,
										DEFAULT_MAYI_SERVER,
										strlen(szPostBody),
										szPostBody);

	if (sockClient.Send_Stream(szPost, strlen(szPost)) < 0) {
		sockClient.CloseSocket();
		free(szPost);
		return -1;
	}


	if (RecvHttpResponse(&sockClient, szPost, POST_BUFF_SIZE, &start, &end) < 0) {
		sockClient.CloseSocket();
		free(szPost);
		return -1;
	}


	while(1) {

		if (ParseLine(start, name, sizeof(name), value, sizeof(value), &next_start) == FALSE) {
			sockClient.CloseSocket();
			free(szPost);
			return -1;
		}


		if (strcmp(value, "") != 0)
		{
			if (strcmp(name, "result_code") == 0) {
				if (strcmp(value, "OK") == 0) {
					result = 1;
				}
			}
		}


		if (next_start == NULL) {
			break;
		}
		start = next_start;
	}


	sockClient.ShutDown();
	sockClient.CloseSocket();

	free(szPost);
	return result;

#undef POST_BUFF_SIZE
}


BOOL ParseUseRowValue(char *value, USE_NODE *lpNode)
{
	char *p;


	if (!value || !lpNode) {
		return FALSE;
	}


	/* id */
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	lpNode->id = atol(value);


	/* user_username */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->user_username, value, sizeof(lpNode->user_username));////UrlDecode


	/* guaji_owner_username */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->guaji_owner_username, value, sizeof(lpNode->guaji_owner_username));////UrlDecode


	/* platform_type */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->platform_type, value, sizeof(lpNode->platform_type));////UrlDecode


	/* sub_account */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->sub_account, value, sizeof(lpNode->sub_account));////UrlDecode


	/* goods_category */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->goods_category, value, sizeof(lpNode->goods_category));////UrlDecode


	/* shop_name */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->shop_name, value, sizeof(lpNode->shop_name));////UrlDecode


	/* order_num */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->order_num, value, sizeof(lpNode->order_num));////UrlDecode


	/* order_money */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->order_money, value, sizeof(lpNode->order_money));////UrlDecode


	/* str_buy_time */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->str_buy_time, value, sizeof(lpNode->str_buy_time));////UrlDecode


	/* str_review_time */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->str_review_time, value, sizeof(lpNode->str_review_time));////UrlDecode


	/* review_content */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->review_content, value, sizeof(lpNode->review_content));////UrlDecode


	/* review_extra */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->review_extra, value, sizeof(lpNode->review_extra));////UrlDecode


	/* str_ack_time */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->str_ack_time, value, sizeof(lpNode->str_ack_time));////UrlDecode


	/* str_op_status */
	value = p + 1;
	p = strchr(value, '|');
	if (p != NULL) { /* Last field */
		*p = '\0';
	}
	strncpy(lpNode->str_op_status, value, sizeof(lpNode->str_op_status));////UrlDecode


	return TRUE;
}

//
// Return Value:
// -1: Error
//  0: Success
//
int MayiQueryUseList(const char *client_charset, const char *client_lang, int user_id, int guaji_id, USE_NODE *nodesArray, int *lpCount, int *lpNum)
{
#define POST_BUFF_SIZE	(1024*NODES_PER_PAGE)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+16];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024];
	char *szPost;
	char *start, *end;
	int result = -1;
	char name[32];
	char value[1024*4];
	char *next_start = NULL;
	int num;
	int nCount = 0;


	if (nodesArray == NULL || lpCount == NULL || *lpCount < 0) {
		return -1;
	}

	if (dwMayiServerIp == 0) {
		if (TWSocket::GetIPAddr(DEFAULT_MAYI_SERVER, szMayiIpAddr, sizeof(szMayiIpAddr)) == 0) {
			return -1;
		}
		dwMayiServerIp = inet_addr(szMayiIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szMayiIpAddr, MAYI_SERVER_PORT) != 0) {
		dwMayiServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			"user_id=%d"
			"&guaji_id=%d"
			"&client_charset=%s"
			"&client_lang=%s",
			user_id,guaji_id,
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_QUERYUSELIST_URL,
										DEFAULT_MAYI_SERVER, MAYI_QUERYUSELIST_REFERER,
										DEFAULT_MAYI_SERVER,
										strlen(szPostBody),
										szPostBody);

	if (sockClient.Send_Stream(szPost, strlen(szPost)) < 0) {
		sockClient.CloseSocket();
		free(szPost);
		return -1;
	}


	if (RecvHttpResponse(&sockClient, szPost, POST_BUFF_SIZE, &start, &end) < 0) {
		sockClient.CloseSocket();
		free(szPost);
		return -1;
	}


	while(1) {

		if (ParseLine(start, name, sizeof(name), value, sizeof(value), &next_start) == FALSE) {
			sockClient.CloseSocket();
			free(szPost);
			return -1;
		}


		if (strcmp(value, "") != 0)
		{
			if (strcmp(name, "num") == 0) {
				num = atol(value);
				if (lpNum) {
					*lpNum = num;
				}
				if (num == 0) {
					break;
				}
			}
			else if (strcmp(name, "row") == 0) {
				if (nCount < *lpCount) {
					if (ParseUseRowValue(value, &(nodesArray[nCount])) == FALSE) {
						sockClient.CloseSocket();
						free(szPost);
						return -1;
					}
					nCount += 1;
				}
			}
		}


		if (next_start == NULL) {
			break;
		}
		start = next_start;
	}


	sockClient.ShutDown();
	sockClient.CloseSocket();

	*lpCount = nCount;
	free(szPost);
	return 0;

#undef POST_BUFF_SIZE
}
