#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "CommonLib.h"
#include "phpMd5.h"
#include "TWSocket.h"
#include "HttpOperate.h"
#include "MayiOperate.h"
#include "LogMsg.h"

#ifdef ANDROID_NDK
#include <android/log.h>
#endif


#define MAYI_LOGINUSER_URL		"/toukuizhe/LoginUser.php"
#define MAYI_LOGINUSER_REFERER	"/toukuizhe/LoginUser.html"
#define MAYI_LOGOUTUSER_URL		"/toukuizhe/LogoutUser.php"
#define MAYI_LOGOUTUSER_REFERER	"/toukuizhe/LogoutUser.html"
#define MAYI_ADDNODE_URL		"/toukuizhe/AddNode.php"
#define MAYI_ADDNODE_REFERER	"/toukuizhe/AddNode.html"
#define MAYI_DELNODE_URL		"/toukuizhe/DelNode.php"
#define MAYI_DELNODE_REFERER	"/toukuizhe/DelNode.html"
#define MAYI_USESTART_URL		"/toukuizhe/UseStart.php"
#define MAYI_USESTART_REFERER	"/toukuizhe/UseStart.html"
#define MAYI_USEREFRESH_URL		"/toukuizhe/UseRefresh.php"
#define MAYI_USEREFRESH_REFERER	"/toukuizhe/UseRefresh.html"
#define MAYI_USEEND_URL			"/toukuizhe/UseEnd.php"
#define MAYI_USEEND_REFERER		"/toukuizhe/UseEnd.html"
#define MAYI_COMPLAIN_URL		"/mayihudong/Complain.php"
#define MAYI_COMPLAIN_REFERER	"/mayihudong/Complain.html"
#define MAYI_REGISTER_URL		"/mayihudong/GuajiRegister.php"
#define MAYI_REGISTER_REFERER	"/mayihudong/GuajiRegister.html"
#define MAYI_UNREGISTER_URL		"/mayihudong/GuajiUnregister.php"
#define MAYI_UNREGISTER_REFERER	"/mayihudong/GuajiUnregister.html"
#define MAYI_QUERYIDS_URL				"/toukuizhe/QueryIds.php"
#define MAYI_QUERYIDS_REFERER			"/toukuizhe/QueryIds.html"
#define MAYI_QUERYGUAJILIST_URL				"/mayihudong/QueryGuajiList.php"
#define MAYI_QUERYGUAJILIST_REFERER			"/mayihudong/QueryGuajiList.html"
#define MAYI_SEARCHGUAJILIST_URL				"/mayihudong/SearchGuajiList.php"
#define MAYI_SEARCHGUAJILIST_REFERER			"/mayihudong/SearchGuajiList.html"
#define MAYI_QUERYNOTIFICATIONLIST_URL		"/toukuizhe/QueryNotificationList.php"
#define MAYI_QUERYNOTIFICATIONLIST_REFERER	"/toukuizhe/QueryNotificationList.html"

#define POST_RESULT_START		"FLAG5TGB6YHNSTART"
#define POST_RESULT_END			"FLAG5TGB6YHNEND"



char g1_login_key[PHP_MD5_OUTPUT_CHARS+1];
char g1_login_result[1024] = "µÇÂ¼³É¹¦£¡";

DWORD g1_user_id = 0;
DWORD g1_user_score = 0;
BOOL g1_user_activated = FALSE;
char g1_expire_time[64];
char g1_last_login_time[64];
char g1_last_login_ip[64];

char g1_nickname[MAX_LOADSTRING];

BOOL g1_vip_filter_nodes = FALSE;
int  g1_guaji_register_period = 15;
int  g1_use_register_period = 15;

char g1_def_security_hole[1024*8];
char g1_def_dashang_content[1024*8];
char g1_def_region[1024*8];



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static char szPostFormat[] = 
"POST %s HTTP/1.0\r\n"
"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*\r\n"
"Referer: http://%s%s\r\n"
"Accept-Language: zh-cn\r\n"
"Content-Type: application/x-www-form-urlencoded\r\n"
"User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 2.0.50727)\r\n"
"Host: %s\r\n"
"Content-Length: %d\r\n"
"Connection: Keep-Alive\r\n"
"Cache-Control: no-cache\r\n"
"\r\n"
"%s";


static DWORD dwHttpServerIp = 0;
static char szIpAddr[16];


// trim ' ' and '\t' and '\r' and '\n'
static void trim_left(char *str)
{
	int i;

	if( str==NULL )
		return;
	for(i=0; i<(int)strlen(str); i++)
	{
		if( str[i]!=' ' && str[i]!='\t' && str[i]!='\r' && str[i]!='\n' )
			break;
	}
	if( i!=0 )
	{
		strcpy(str,str+i);
	}
}

// trim ' ' and '\t' and '\r' and '\n'
static void trim_right(char *str)
{
    int i;

	if( str==NULL )
		return;
	for(i=strlen(str)-1; i>-1; i--)
	{
		if( str[i]!=' ' && str[i]!='\t' && str[i]!='\r' && str[i]!='\n')
			break;
		str[i] = '\0';
	}
}

static void trim(char *str)
{
     trim_left(str);
	 trim_right(str);
}

static BOOL ParseLine(char *start, char *name, int name_size, char *value, int value_size, char **next)
{
	char *p;

	if (!start || !name || !value || !next) {
		return FALSE;
	}

	strcpy(name, "");
	strcpy(value, "");
	*next = NULL;

	while (*start==' ' || *start=='\t' || *start=='\r' || *start=='\n')
	{
		start++;
	}

	if (*start == '\0') {
		return TRUE;
	}

	p = strchr(start, '\n');
	if (p == NULL) {
		p = start + strlen(start);
		*next = NULL;
	}
	else {
		*p = '\0';
		*next = p + 1;
	}

	p = strchr(start, '=');
	if (p == NULL) {
		return FALSE;
	}

	*p = '\0';
	p +=1;

	trim(start);
	trim(p);

	strncpy(name, start, name_size);
	strncpy(value, p, value_size);
	return TRUE;
}

//
// Return Value:
// -1: Error
//  0: Success.
//
static int RecvHttpResponse(TWSocket *sock, char *lpBuff, int nSize, char **pstart, char **pend)
{
	int start_time;
	int nRecvd = 0;
	int ret;


	if (sock == NULL || lpBuff == NULL || pstart == NULL || pend == NULL) {
		return -1;
	}

	start_time = time(NULL);
	while ((time(NULL) - start_time) < 30 && nRecvd < nSize)
	{
		ret = sock->WaitRecv(lpBuff + nRecvd, nSize - 1 - nRecvd, 800);// Wait recv
		if (ret > 0) {
			nRecvd += ret;
			lpBuff[nRecvd] = '\0';

			if (NULL != strstr(lpBuff, "Transfer-Encoding: chunked")) {
				log_msg("Error: Http server use Chunked-Encoding!", LOG_LEVEL_ERROR);
			}

#ifdef ANDROID_NDK ////Debug
			__android_log_print(ANDROID_LOG_INFO, "RecvHttpResponse", "%s", lpBuff);
#endif
			*pstart = strstr(lpBuff, POST_RESULT_START);
			*pend = strstr(lpBuff, POST_RESULT_END);
			if (*pstart != NULL && *pend != NULL) {
				*pstart += strlen(POST_RESULT_START);
				**pend = '\0';
				return 0;
			}
		}
		else 
		{
			if (ret == -1/*WSAETIMEDOUT*/)
			{
#ifdef ANDROID_NDK ////Debug
				__android_log_print(ANDROID_LOG_INFO, "RecvHttpResponse", "ret = %d, error=%d [%s]", ret, errno, strerror(errno));
#endif
				break;
			}
			else {
#ifdef ANDROID_NDK ////Debug
				__android_log_print(ANDROID_LOG_INFO, "RecvHttpResponse", "ret = %d, want=%d", ret, nSize - 1 - nRecvd);
#endif
#ifdef WIN32
				Sleep(800);
#else
				usleep(800000);
#endif
			}
		}
	}

	return -1;
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

	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_mayi_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, MAYI_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);

	srand(time(NULL));
	int login_rand = rand();
	snprintf(szPostBody, sizeof(szPostBody), "%s%s%ld", username, password, login_rand);
	php_md5(szPostBody, g1_login_key, sizeof(g1_login_key));

	snprintf(szPostBody, sizeof(szPostBody), 
			"username=%s"
			"&password=%s"
			"&version=%ld"
			"&login_rand=%ld"
			"&client_charset=%s"
			"&client_lang=%s",
			username,password,g0_version,login_rand,
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_LOGINUSER_URL,
										g1_mayi_server, MAYI_LOGINUSER_REFERER,
										g1_mayi_server,
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
			else if (strcmp(name, "user_activated") == 0) {
				g1_user_activated = atol(value);
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

			else if (strcmp(name, "nickname") == 0) {
				strncpy(g1_nickname, value, sizeof(g1_nickname));
			}

			else if (strcmp(name, "vip_filter_nodes") == 0) {
				g1_vip_filter_nodes = atol(value);
			}
			else if (strcmp(name, "guaji_register_period") == 0) {
				g1_guaji_register_period = atol(value);
			}
			else if (strcmp(name, "use_register_period") == 0) {
				g1_use_register_period = atol(value);
			}

			else if (strcmp(name, "def_security_hole") == 0) {
				strncpy(g1_def_security_hole, value, sizeof(g1_def_security_hole));
			}
			else if (strcmp(name, "def_dashang_content") == 0) {
				strncpy(g1_def_dashang_content, value, sizeof(g1_def_dashang_content));
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
//  1: OK
//
int MayiLogoutUser(const char *client_charset, const char *client_lang, DWORD user_id, const char *login_key)
{
#define POST_BUFF_SIZE	(1024*4)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+16];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024*2];
	char *szPost;
	char *start, *end;
	int result = 0; /* NG */
	char name[32];
	char value[1024*2];
	char *next_start = NULL;


	if (login_key == NULL) {
		return -1;
	}

	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_mayi_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, MAYI_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			"user_id=%ld"
			"&login_key=%s"
			"&client_charset=%s"
			"&client_lang=%s",
			user_id,login_key,
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_LOGOUTUSER_URL,
										g1_mayi_server, MAYI_LOGOUTUSER_REFERER,
										g1_mayi_server,
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
//  1: OK
//
int MayiAddNode(const char *client_charset, const char *client_lang, DWORD user_id, DWORD node_id, const char *login_key, char *resultBuff, int buffSize)
{
#define POST_BUFF_SIZE	(1024*4)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+16];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024*2];
	char *szPost;
	char *start, *end;
	int result = 0; /* NG */
	char name[32];
	char value[1024*2];
	char *next_start = NULL;


	if (login_key == NULL || resultBuff == NULL) {
		return -1;
	}

	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_mayi_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, MAYI_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			"user_id=%ld"
			"&node_id=%ld"
			"&login_key=%s"
			"&client_charset=%s"
			"&client_lang=%s",
			user_id,node_id,login_key,
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_ADDNODE_URL,
										g1_mayi_server, MAYI_ADDNODE_REFERER,
										g1_mayi_server,
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
//  1: OK
//
int MayiDelNode(const char *client_charset, const char *client_lang, DWORD user_id, DWORD node_id, const char *login_key, char *resultBuff, int buffSize)
{
#define POST_BUFF_SIZE	(1024*4)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+16];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024*2];
	char *szPost;
	char *start, *end;
	int result = 0; /* NG */
	char name[32];
	char value[1024*2];
	char *next_start = NULL;


	if (login_key == NULL || resultBuff == NULL) {
		return -1;
	}

	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_mayi_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, MAYI_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			"user_id=%ld"
			"&node_id=%ld"
			"&login_key=%s"
			"&client_charset=%s"
			"&client_lang=%s",
			user_id,node_id,login_key,
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_DELNODE_URL,
										g1_mayi_server, MAYI_DELNODE_REFERER,
										g1_mayi_server,
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
int MayiComplain(const char *client_charset, const char *client_lang, DWORD user_id, const char *user_username, DWORD guaji_id, const char *guaji_owner_username, const char *complain_content)
{
#define POST_BUFF_SIZE	(1024*8)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+16];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024*6];
	char *szPost;
	char *start, *end;
	int result = 0; /* NG */
	char name[32];
	char value[1024];
	char *next_start = NULL;


	if (user_username == NULL || guaji_owner_username == NULL || complain_content == NULL) {
		return -1;
	}

	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_mayi_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, MAYI_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			"user_id=%ld"
			"&user_username=%s"
			"&guaji_id=%ld"
			"&guaji_owner_username=%s"
			"&complain_content=%s"
			"&client_charset=%s"
			"&client_lang=%s",
			user_id,user_username,guaji_id,guaji_owner_username,UrlEncode(complain_content).c_str(),
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_COMPLAIN_URL,
										g1_mayi_server, MAYI_COMPLAIN_REFERER,
										g1_mayi_server,
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
//  n: n>0, use_id
//
int MayiUseStart(const char *client_charset, const char *client_lang, DWORD user_id, const char *user_username, DWORD node_id, DWORD guaji_id, const char *guaji_name, const char *login_key, char *resultBuff, int buffSize)
{
#define POST_BUFF_SIZE	(1024*4)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+16];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024*2];
	char *szPost;
	char *start, *end;
	int result = 0; /* NG */
	char name[32];
	char value[1024*2];
	char *next_start = NULL;


	if (user_username == NULL || guaji_name == NULL || login_key == NULL || resultBuff == NULL) {
		return -1;
	}

	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_mayi_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, MAYI_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			"user_id=%ld"
			"&user_username=%s"
			"&node_id=%ld"
			"&guaji_id=%ld"
			"&guaji_name=%s"
			"&login_key=%s"
			"&client_charset=%s"
			"&client_lang=%s",
			user_id,user_username,node_id,guaji_id,UrlEncode(guaji_name).c_str(),
			login_key,
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_USESTART_URL,
										g1_mayi_server, MAYI_USESTART_REFERER,
										g1_mayi_server,
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
					//result = 1;
				}
			}
			else if (strcmp(name, "result_str") == 0) {
				strncpy(resultBuff, value, buffSize);
			}
			else if (strcmp(name, "use_id") == 0) {
				result = atol(value);
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
int MayiUseRefresh(const char *client_charset, const char *client_lang, DWORD use_id)
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


	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_mayi_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, MAYI_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			"use_id=%ld"
			"&client_charset=%s"
			"&client_lang=%s",
			use_id,
			client_charset, client_lang);


	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_USEREFRESH_URL,
										g1_mayi_server, MAYI_USEREFRESH_REFERER,
										g1_mayi_server,
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
int MayiUseEnd(const char *client_charset, const char *client_lang, DWORD use_id, DWORD user_id, const char *user_username, DWORD node_id,  DWORD guaji_id, const char *review_content)
{
#define POST_BUFF_SIZE	(1024*8)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+16];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024*6];
	char *szPost;
	char *start, *end;
	int result = 0; /* NG */
	char name[32];
	char value[1024];
	char *next_start = NULL;


	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_mayi_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, MAYI_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
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
			"&node_id=%ld"
			"&guaji_id=%ld"
			"&review_content=%s"
			"&client_charset=%s"
			"&client_lang=%s",
			use_id,
			user_id,user_username,node_id,guaji_id,
			UrlEncode(review_content).c_str(),
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_USEEND_URL,
										g1_mayi_server, MAYI_USEEND_REFERER,
										g1_mayi_server,
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
//  0: Success
//
int MayiQueryIds(const char *client_charset, const char *client_lang, int user_id, int page_offset, int page_rows, char *ids_buff, int buff_size)
{
#define POST_BUFF_SIZE	(1024*4)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+16];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024];
	char *szPost;
	char *start, *end;
	int result = -1;
	char name[32];
	char value[4*1024];
	char *next_start = NULL;


	if (ids_buff == NULL) {
		return -1;
	}

	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_mayi_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, MAYI_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			"user_id=%d"
			"&page_offset=%d"
			"&page_rows=%d"
			"&client_charset=%s"
			"&client_lang=%s",
			user_id,page_offset,page_rows,
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_QUERYIDS_URL,
										g1_mayi_server, MAYI_QUERYIDS_REFERER,
										g1_mayi_server,
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
			if (strcmp(name, "request_nodes") == 0) {
				strncpy(ids_buff, value, buff_size);
				result = 0;
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
	return 0;

#undef POST_BUFF_SIZE
}


BOOL ParseGuajiRowValue(char *value, GUAJI_NODE *lpNode)
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


	/* region */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->region, value, sizeof(lpNode->region));////UrlDecode


	/* total_used_times */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	lpNode->total_used_times = atol(value);


	/* total_dashang_score */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	lpNode->total_dashang_score = atol(value);


	/* comments_info */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->comments_info, value, sizeof(lpNode->comments_info));////UrlDecode


	/* anypc_node_id */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->anypc_node_id, value, sizeof(lpNode->anypc_node_id));////UrlDecode


	/* is_busy */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	if (strcmp(value, "0") == 0 || strcmp(value, "2") == 0) {
		lpNode->is_busy = TRUE;
	}
	else {
		lpNode->is_busy = FALSE;
	}


	/* network_type */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->network_type, value, sizeof(lpNode->network_type));////UrlDecode


	/* network_delay */
	value = p + 1;
	p = strchr(value, '|');
	if (p != NULL) { /* Last field */
		*p = '\0';
	}
	lpNode->network_delay = atol(value);


	return TRUE;
}

//
// Return Value:
// -1: Error
//  0: Success
//
int MayiQueryGuajiList(const char *client_charset, const char *client_lang, int user_id, int page_offset, int page_rows, GUAJI_NODE *nodesArray, int *lpCount, int *lpNum)
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
	char value[1024];
	char *next_start = NULL;
	int num;
	int nCount = 0;


	if (nodesArray == NULL || lpCount == NULL || *lpCount < 0) {
		return -1;
	}

	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_mayi_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, MAYI_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			"user_id=%d"
			"&page_offset=%d"
			"&page_rows=%d"
			"&client_charset=%s"
			"&client_lang=%s",
			user_id,page_offset,page_rows,
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_QUERYGUAJILIST_URL,
										g1_mayi_server, MAYI_QUERYGUAJILIST_REFERER,
										g1_mayi_server,
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
					if (ParseGuajiRowValue(value, &(nodesArray[nCount])) == FALSE) {
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


//
// Return Value:
// -1: Error
//  0: Success
//
int MayiSearchGuajiList(const char *client_charset, const char *client_lang, int user_id, const char *search_username, const char *search_subaccount, GUAJI_NODE *nodesArray, int *lpCount, int *lpNum)
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
	char value[1024];
	char *next_start = NULL;
	int num;
	int nCount = 0;


	if (nodesArray == NULL || lpCount == NULL || *lpCount < 0) {
		return -1;
	}

	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_mayi_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, MAYI_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			"user_id=%d"
			"&search_username=%s"
			"&search_subaccount=%s"
			"&client_charset=%s"
			"&client_lang=%s",
			user_id,search_username,UrlEncode(search_subaccount).c_str(),
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										MAYI_SEARCHGUAJILIST_URL,
										g1_mayi_server, MAYI_SEARCHGUAJILIST_REFERER,
										g1_mayi_server,
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
					if (ParseGuajiRowValue(value, &(nodesArray[nCount])) == FALSE) {
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


/* row=text|link|type */
static BOOL ParseNotificationRowValue(char *value, NOTIFICATION_ITEM *lpItem)
{
	char *p;

	if (!value) {
		return FALSE;
	}


	/* Text */
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpItem->text, value, sizeof(lpItem->text));


	/* Link */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpItem->link, value, sizeof(lpItem->link));


	/* Type */
	value = p + 1;
	p = strchr(value, '|');
	if (p != NULL) {
		*p = '\0';  /* Last field */
	}
	lpItem->type = atoi(value);

	return TRUE;
}


//
// Return Value:
// -1: Error
//  0: Success
//
int MayiQueryNotificationList(NOTIFICATION_ITEM *notificationArray, int *lpCount)
{
	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+16];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024];
	char szPost[1024*16];
	char *start, *end;
	int result = -1;
	char name[256];
	char value[1024*2];
	char *next_start = NULL;
	int num;
	int nCount = 0;


	if (notificationArray == NULL || lpCount == NULL) {
		return -1;
	}

	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_mayi_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, MAYI_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(sizeof(szPost));
	sockClient.SetSockRecvBufferSize(sizeof(szPost));


	_snprintf(szPostBody, sizeof(szPostBody), 
			"sender_node_id=%02X-%02X-%02X-%02X-%02X-%02X", 
			0, 0, 0, 0, 0, 0);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	strcat(szKey1, MAYI_KEY_STRING);
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);


	_snprintf(szPost, sizeof(szPost), szPostFormat, 
										MAYI_QUERYNOTIFICATIONLIST_URL,
										g1_mayi_server, MAYI_QUERYNOTIFICATIONLIST_REFERER,
										g1_mayi_server,
										strlen(szPostBody),
										szPostBody);

	if (sockClient.Send_Stream(szPost, strlen(szPost)) < 0) {
		sockClient.CloseSocket();
		return -1;
	}


	if (RecvHttpResponse(&sockClient, szPost, sizeof(szPost), &start, &end) < 0) {
		sockClient.CloseSocket();
		return -1;
	}


	while(1) {

		if (ParseLine(start, name, sizeof(name), value, sizeof(value), &next_start) == FALSE) {
			sockClient.CloseSocket();
			return -1;
		}


		if (strcmp(value, "") != 0)
		{
			if (strcmp(name, "num") == 0) {
				num = atoi(value);
				if (num == 0) {
					break;
				}
			}
			else if (strcmp(name, "row") == 0) {
				if (nCount < *lpCount) {
					if (ParseNotificationRowValue(value, &(notificationArray[nCount])) == FALSE) {
						sockClient.CloseSocket();
						return -1;
					}
					nCount += 1;
				}
				else {
					break;
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
	return 0;
}
