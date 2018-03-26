#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "CommonLib.h"
#include "phpMd5.h"
#include "base64.h"
#include "TWSocket.h"
#include "HttpOperate.h"
#include "LogMsg.h"

#ifdef ANDROID_NDK
#include <android/log.h>
#endif


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
#define HTTP_FETCHNODE_URL			"/cloudctrl/FetchNode.php"
#define HTTP_FETCHNODE_REFERER		"/cloudctrl/FetchNode.html"

#define POST_RESULT_START		"FLAG5TGB6YHNSTART"
#define POST_RESULT_END			"FLAG5TGB6YHNEND"



char g0_device_uuid[MAX_LOADSTRING];
char g0_node_name[MAX_LOADSTRING];
DWORD g0_version;
DWORD g0_audio_channels;
DWORD g0_video_channels;
char g0_os_info[MAX_LOADSTRING];

BOOL g1_is_active;
DWORD g1_lowest_version;
DWORD g1_admin_lowest_version;
char g1_http_server[MAX_LOADSTRING];
char g1_stun_server[MAX_LOADSTRING];
char g1_mayi_server[MAX_LOADSTRING];
int g1_register_period;  /* Seconds */
int g1_expire_period;  /* Seconds */
int g1_lowest_level_for_av;
int g1_lowest_level_for_vnc;
int g1_lowest_level_for_ft;
int g1_lowest_level_for_adb;
int g1_lowest_level_for_webmoni;
int g1_lowest_level_allow_hide;

//#ifdef JNI_FOR_MOBILECAMERA
BOOL g1_is_activated = TRUE;
DWORD g1_comments_id = 0;
//#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static char szPostFormat[] = 
"POST %s HTTP/1.0\r\n"  // 1.1 -> 1.0 关闭chunked编码
"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg,  */*\r\n"
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
	while ((time(NULL) - start_time) < 60 && nRecvd < nSize)
	{
		ret = sock->WaitRecv(lpBuff + nRecvd, nSize - 1 - nRecvd, 800);// Wait recv
		if (ret > 0) {
			nRecvd += ret;
			lpBuff[nRecvd] = '\0';
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
//  0: Success.
//
static int DoUpgradeVersion(DWORD new_version)
{
	return 0;
}

DWORD dwThreadID_Upgrade;
HANDLE hThread_Upgrade = NULL;

DWORD WINAPI UpgradeThreadFn(LPVOID pvThreadParam)
{
	DoUpgradeVersion(g1_lowest_version);
	CloseHandle(hThread_Upgrade);
	hThread_Upgrade = NULL;
	return 0;
}

//
// Return Value:
// -1: Error
//  0: Success
//
int ParseTopoSettings(const char *settings_string)
{
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

HttpOperate::HttpOperate(BOOL is_admin, WORD p2p_port)
{
	m0_p2p_port = p2p_port;

	m0_is_admin = is_admin;
	m0_is_busy = 0;

	//每个HttpOperate实例有不一样的node_id
	generate_nodeid(m0_node_id, sizeof(m0_node_id));

	m0_pub_ip = 0;
	m0_pub_port = 0;
	m0_no_nat = FALSE;
	m0_nat_type = StunTypeUnknown;
	m0_net_time = 50;

	m1_http_client_ip = 0;

	m1_use_udp_sock = INVALID_SOCKET;
	m1_use_udt_sock = INVALID_SOCKET;
	m1_use_sock_type = SOCKET_TYPE_UNKNOWN;

}


HttpOperate::~HttpOperate()
{
	
}


const char *HttpOperate::MakeIpStr()
{
	static char szIpStr[16*MAX_ADAPTER_NUM];
	struct in_addr inaddr;
	int i;

	strcpy(szIpStr, "");

	if (m0_ipCount > 0) {
		inaddr.s_addr = m0_ipArray[0];
		strcat(szIpStr, inet_ntoa(inaddr));
		for (i = 1; i < m0_ipCount; i++) {
			strcat(szIpStr, "-");
			inaddr.s_addr = m0_ipArray[i];
			strcat(szIpStr, inet_ntoa(inaddr));
		}
	}

	return szIpStr;
}


const char *HttpOperate::MakePubIpStr()
{
	static char szPubIpStr[16];
	struct in_addr inaddr;

	inaddr.s_addr = m0_pub_ip;
	strcpy(szPubIpStr, inet_ntoa(inaddr));
	return szPubIpStr;
}


BOOL HttpOperate::ParseIpValue(char *value)
{
	char *p;
	int nCount = 0;
	BOOL bLastOne = FALSE;

	if (!value) {
		return FALSE;
	}

	while (*value != '\0') {

		p = strchr(value, '-');
		if (p != NULL) {
			*p = '\0';
		}
		else {
			bLastOne = TRUE;
		}
		if (inet_addr(value) != 0 && nCount < MAX_ADAPTER_NUM) {
			m1_peer_pri_ipArray[nCount] = inet_addr(value);
			nCount += 1;
		}
		if (bLastOne) {
			break;
		}
		value = p + 1;
	}
	m1_peer_pri_ipCount = nCount;
	return TRUE;
}


/* event=Accept|3F-1A-CD-90-4B-67|61.126.78.32|23745|0|5|192.168.1.101-192.168.110.103|3478 */
/* event=AcceptProxy|3F-1A-CD-90-4B-67|1|61.126.78.32|23745 */
BOOL HttpOperate::ParseEventValue(char *value)
{
	char *p;
	char *type_str, *node_id_str, *ip_str, *port_str, *no_nat_str, *nat_type_str, *priv_ip_str, *priv_port_str;
	int func_port;

	if (!value) {
		return FALSE;
	}


	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	type_str = value;
	if (strcmp(type_str, "Accept") == 0)
	{

		/* NODE ID */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		node_id_str = value;
		mac_addr(node_id_str, m1_peer_node_id, NULL);

		/* IP */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		ip_str = value;
		m1_peer_ip = inet_addr(ip_str);

		/* PORT */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		port_str = value;
		m1_peer_port = atoi(port_str);

		/* NO_NAT */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		no_nat_str = value;
		if (strcmp(no_nat_str, "1") == 0) {
			m1_peer_nonat = TRUE;
		}
		else {
			m1_peer_nonat = FALSE;
		}

		/* NAT_TYPE */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		nat_type_str = value;
		m1_peer_nattype = atoi(nat_type_str);

		/* PRIVATE IP */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		priv_ip_str = value;
		ParseIpValue(priv_ip_str);

		/* PRIVATE PORT */
		value = p + 1;
		p = strchr(value, '|');
		if (p != NULL) {
			*p = '\0';  /* Last field */
		}
		priv_port_str = value;
		m1_peer_pri_port = atoi(priv_port_str);

	}
	else if (strcmp(type_str, "AcceptRev") == 0)
	{

		/* NODE ID */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		node_id_str = value;
		mac_addr(node_id_str, m1_peer_node_id, NULL);

		/* IP */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		ip_str = value;
		m1_peer_ip = inet_addr(ip_str);

		/* PORT */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		port_str = value;
		m1_peer_port = atoi(port_str);

		/* NO_NAT */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		no_nat_str = value;
		if (strcmp(no_nat_str, "1") == 0) {
			m1_peer_nonat = TRUE;
		}
		else {
			m1_peer_nonat = FALSE;
		}

		/* NAT_TYPE */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		nat_type_str = value;
		m1_peer_nattype = atoi(nat_type_str);

		/* PRIVATE IP */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		priv_ip_str = value;
		ParseIpValue(priv_ip_str);

		/* PRIVATE PORT */
		value = p + 1;
		p = strchr(value, '|');
		if (p != NULL) {
			*p = '\0';  /* Last field */
		}
		priv_port_str = value;
		m1_peer_pri_port = atoi(priv_port_str);

	}
	else if (strcmp(type_str, "AcceptProxy") == 0)
	{

		/* NODE ID */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		node_id_str = value;
		mac_addr(node_id_str, m1_peer_node_id, NULL);

		/* Is Server Proxy */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';

		/* Proxy IP */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		ip_str = value;
		m1_peer_ip = inet_addr(ip_str);

		/* Proxy PORT */
		value = p + 1;
		p = strchr(value, '|');
		if (p != NULL) {
			*p = '\0';  /* Last field */
		}
		port_str = value;
		m1_peer_port = atoi(port_str);

	}
	else if (strcmp(type_str, "AcceptProxyTcp") == 0)
	{

		/* NODE ID */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		node_id_str = value;
		mac_addr(node_id_str, m1_peer_node_id, NULL);

		/* Is Server Proxy */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';

		/* Func Port */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		func_port = atoi(value);

		/* Proxy IP */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		ip_str = value;
		m1_peer_ip = inet_addr(ip_str);

		/* Proxy PORT */
		value = p + 1;
		p = strchr(value, '|');
		if (p != NULL) {
			*p = '\0';  /* Last field */
		}
		port_str = value;
		m1_peer_port = atoi(port_str);

	}

	strncpy(m1_event_type, type_str, sizeof(m1_event_type));

	return TRUE;
}


//
// Return Value:
// -1: Error
//  0: Should exit.
//  1: Should stop.
//  2: OK, continue.
//
int HttpOperate::DoRegister1(const char *client_charset, const char *client_lang)
{
#define POST_BUFF_SIZE  (1024*2)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+1];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[512];
	char *szPost;
	char *start, *end;
	int result = 2;
	char name[32];
	char value[1024];
	char *next_start = NULL;
	BOOL bUpgradeVersion = FALSE;
	//TCHAR wszTemp[MAX_PATH];


	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_http_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, HTTP_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			 "settings_only=1"
			 "&client_charset=%s"
			 "&client_lang=%s",
			 client_charset, client_lang);
	
	php_md5(szPostBody, szKey1, sizeof(szKey1));
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);
	
	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										HTTP_REGISTER_URL,
										g1_http_server, HTTP_REGISTER_REFERER,
										g1_http_server,
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
			if (strcmp(name, "is_active") == 0) {
				if (strcmp(value, "0") == 0) {
					g1_is_active = FALSE;
					result = 0;  /* Exit */
					break;
				}
				else {
					g1_is_active = TRUE;
				}
			}
#if !(ANYPC_ADMIN)
			else if (strcmp(name, "lowest_version") == 0) {
				g1_lowest_version = atol(value);
				if (g1_lowest_version > g0_version) {
					bUpgradeVersion = TRUE;
				}
			}
#else
			else if (strcmp(name, "admin_lowest_version") == 0) {
				g1_admin_lowest_version = atol(value);
				if (g1_admin_lowest_version > g0_version) {
					bUpgradeVersion = TRUE;
				}
			}
#endif
			else if (strcmp(name, "http_server") == 0) {
				strncpy(value, UrlDecode(value).c_str(), sizeof(value));
				if (strcmp(value, g1_http_server) != 0) {
					strncpy(g1_http_server, value, sizeof(g1_http_server));
					//MultiByteToWideChar(CP_OEMCP,0,g1_http_server,-1,wszTemp,MAX_PATH);
					//SaveSoftwareKeyValue(_T(STRING_REGKEY_NAME_HTTPSERVER), wszTemp);
					dwHttpServerIp = 0;  /* 重新解析PHP服务器地址 */
					//break;
				}
			}
			else if (strcmp(name, "stun_server") == 0) {
				strncpy(value, UrlDecode(value).c_str(), sizeof(value));
				if (strcmp(value, g1_stun_server) != 0) {
					strncpy(g1_stun_server, value, sizeof(g1_stun_server));
				}
			}
			else if (strcmp(name, "mayi_server") == 0) {
				strncpy(value, UrlDecode(value).c_str(), sizeof(value));
				if (strcmp(value, g1_mayi_server) != 0) {
					strncpy(g1_mayi_server, value, sizeof(g1_mayi_server));
				}
			}
			else if (strcmp(name, "http_client_ip") == 0) {
				DWORD temp_ip = inet_addr(value);
				if (temp_ip != 0) {
					m1_http_client_ip = temp_ip;
				}
			}
			else if (strcmp(name, "register_period") == 0) {
				g1_register_period = atol(value);
			}
			else if (strcmp(name, "expire_period") == 0) {
				g1_expire_period = atol(value);
			}
			else if (strcmp(name, "lowest_level_for_av") == 0) {
				g1_lowest_level_for_av = atoi(value);
			}
			else if (strcmp(name, "lowest_level_for_vnc") == 0) {
				g1_lowest_level_for_vnc = atoi(value);
			}
			else if (strcmp(name, "lowest_level_for_ft") == 0) {
				g1_lowest_level_for_ft = atoi(value);
			}
			else if (strcmp(name, "lowest_level_for_adb") == 0) {
				g1_lowest_level_for_adb = atoi(value);
			}
			else if (strcmp(name, "lowest_level_for_webmoni") == 0) {
				g1_lowest_level_for_webmoni = atoi(value);
			}
			else if (strcmp(name, "lowest_level_allow_hide") == 0) {
				g1_lowest_level_allow_hide = atoi(value);
			}
		}


		if (next_start == NULL) {
			break;
		}
		start = next_start;
	}


	sockClient.ShutDown();
	sockClient.CloseSocket();

	if (bUpgradeVersion) {
		if (NULL == hThread_Upgrade) {
			hThread_Upgrade = ::CreateThread(NULL,0,UpgradeThreadFn,NULL,0,&dwThreadID_Upgrade);
		}
		result = 2;
	}

	free(szPost);
	return result;

#undef POST_BUFF_SIZE
}


//
// Return Value:
// -1: Error
//  0: Should exit.
//  1: Should stop.
//  2: OK, continue.
//  3: Setup connection
//
int HttpOperate::DoRegister2(const char *client_charset, const char *client_lang)
{
#define POST_BUFF_SIZE  (1024*4)
	
	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+1];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024*2];
	char *szPost;
	char *start, *end;
	int result = 2;
	char name[256];
	char value[1024*2];
	char *next_start = NULL;
	BOOL bUpgradeVersion = FALSE;


	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_http_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, HTTP_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	szPost = (char *)malloc(POST_BUFF_SIZE);


	/* 若修改此处，记得一并修改 AnyPC.cpp RecvUdpDiscoveryRequest()！！！ */
	snprintf(szPostBody, sizeof(szPostBody), 
		"settings_only=0"
		"&device_uuid=%s"
		"&node_id=%02X-%02X-%02X-%02X-%02X-%02X"
		"&node_name=%s"
		"&version=%ld"
		"&ip=%s"
		"&port=%d"
		"&pub_ip=%s"
		"&pub_port=%d"
		"&no_nat=%d"
		"&nat_type=%d"
		"&is_admin=%d"
		"&is_busy=%d"
		"&audio_channels=%d"
		"&video_channels=%d"
		"&os_info=%s"
		"&client_charset=%s"
		"&client_lang=%s",
		g0_device_uuid,
		m0_node_id[0], m0_node_id[1], m0_node_id[2], m0_node_id[3], m0_node_id[4], m0_node_id[5], 
		UrlEncode(g0_node_name).c_str(), g0_version, MakeIpStr(), m0_port, MakePubIpStr(), m0_pub_port, 
		(m0_no_nat ? 1 : 0),
		m0_nat_type,
		(m0_is_admin ? 1 : 0),
		(m0_is_busy ? 1 : 0),
		g0_audio_channels,
		g0_video_channels,
		g0_os_info,
		client_charset, client_lang);
	
	php_md5(szPostBody, szKey1, sizeof(szKey1));
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);
	
	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										HTTP_REGISTER_URL,
										g1_http_server, HTTP_REGISTER_REFERER,
										g1_http_server,
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
			if (strcmp(name, "is_active") == 0) {
				if (strcmp(value, "0") == 0) {
					g1_is_active = FALSE;
					result = 0;  /* Exit */
					break;
				}
				else {
					g1_is_active = TRUE;
				}
			}
#if !(ANYPC_ADMIN)
			else if (strcmp(name, "lowest_version") == 0) {
				g1_lowest_version = atol(value);
				if (g1_lowest_version > g0_version) {
					bUpgradeVersion = TRUE;
				}
			}
#endif
			else if (strcmp(name, "http_server") == 0) {
				strncpy(value, UrlDecode(value).c_str(), sizeof(value));
				if (strcmp(value, g1_http_server) != 0) {
					strncpy(g1_http_server, value, sizeof(g1_http_server));
					//SaveSoftwareKeyValue(STRING_REGKEY_NAME_HTTPSERVER, g1_http_server);
					dwHttpServerIp = 0;  /* 重新解析PHP服务器地址 */
					//break;
				}
			}
			else if (strcmp(name, "stun_server") == 0) {
				strncpy(value, UrlDecode(value).c_str(), sizeof(value));
				if (strcmp(value, g1_stun_server) != 0) {
					strncpy(g1_stun_server, value, sizeof(g1_stun_server));
				}
			}
			else if (strcmp(name, "mayi_server") == 0) {
				strncpy(value, UrlDecode(value).c_str(), sizeof(value));
				if (strcmp(value, g1_mayi_server) != 0) {
					strncpy(g1_mayi_server, value, sizeof(g1_mayi_server));
				}
			}
			else if (strcmp(name, "http_client_ip") == 0) {
				DWORD temp_ip = inet_addr(value);
				if (temp_ip != 0) {
					m1_http_client_ip = temp_ip;
				}
			}
			else if (strcmp(name, "register_period") == 0) {
				g1_register_period = atoi(value);
			}
			else if (strcmp(name, "expire_period") == 0) {
				g1_expire_period = atoi(value);
			}
			else if (strcmp(name, "lowest_level_for_av") == 0) {
				g1_lowest_level_for_av = atoi(value);
			}
			else if (strcmp(name, "lowest_level_for_vnc") == 0) {
				g1_lowest_level_for_vnc = atoi(value);
			}
			else if (strcmp(name, "lowest_level_for_ft") == 0) {
				g1_lowest_level_for_ft = atoi(value);
			}
			else if (strcmp(name, "lowest_level_for_adb") == 0) {
				g1_lowest_level_for_adb = atoi(value);
			}
			else if (strcmp(name, "lowest_level_for_webmoni") == 0) {
				g1_lowest_level_for_webmoni = atoi(value);
			}
			else if (strcmp(name, "lowest_level_allow_hide") == 0) {
				g1_lowest_level_allow_hide = atoi(value);
			}
			else if (strcmp(name, "is_activated") == 0) {
				if (strcmp(value, "1") == 0) {
					g1_is_activated = TRUE;
				}
				else {
					g1_is_activated = FALSE;
				}
			}
			else if (strcmp(name, "comments_id") == 0) {
				g1_comments_id = atoi(value);
			}
			else if (strcmp(name, "event") == 0) {
				ParseEventValue(value);
				result = 3; /* Connection */
			}
		}


		if (next_start == NULL) {
			break;
		}
		start = next_start;
	}


	sockClient.ShutDown();
	sockClient.CloseSocket();

	if (bUpgradeVersion) {
		if (NULL == hThread_Upgrade) {
			hThread_Upgrade = ::CreateThread(NULL,0,UpgradeThreadFn,NULL,0,&dwThreadID_Upgrade);
		}
		result = 2;
	}

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
int HttpOperate::DoUnregister(const char *client_charset, const char *client_lang)
{
#define POST_BUFF_SIZE  (1024*2)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+1];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[512];
	char *szPost;
	char *start, *end;
	int result = 0;
	char name[32];
	char value[1024];
	char *next_start = NULL;


	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_http_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, HTTP_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), "node_id=%02X-%02X-%02X-%02X-%02X-%02X"
												"&client_charset=%s"
												"&client_lang=%s",
					m0_node_id[0], m0_node_id[1], m0_node_id[2], m0_node_id[3], m0_node_id[4], m0_node_id[5], 
					client_charset, client_lang);
	
	php_md5(szPostBody, szKey1, sizeof(szKey1));
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);
	
	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										HTTP_UNREGISTER_URL,
										g1_http_server, HTTP_UNREGISTER_REFERER,
										g1_http_server,
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

		if (strcmp(name, "result_code") == 0) {
			if (strcmp(value, "OK") == 0) {
				result = 1;
				break;
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
int HttpOperate::DoQueryList(const char *client_charset, const char *client_lang, const char *request_nodes, ANYPC_NODE *nodesArray, int *lpCount, int *lpNum)
{
#define POST_BUFF_SIZE	(1024*NODES_PER_PAGE)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+1];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024*2];//for request_nodes
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
		if (TWSocket::GetIPAddr(g1_http_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, HTTP_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	sockClient.SetSockSendBufferSize(POST_BUFF_SIZE);
	sockClient.SetSockRecvBufferSize(POST_BUFF_SIZE);

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), 
			"sender_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
			"&request_nodes=%s"
			"&client_charset=%s"
			"&client_lang=%s",
			m0_node_id[0], m0_node_id[1], m0_node_id[2], m0_node_id[3], m0_node_id[4], m0_node_id[5],
			request_nodes,
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);

	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										HTTP_QUERYLIST_URL,
										g1_http_server, HTTP_QUERYLIST_REFERER,
										g1_http_server,
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
					if (ParseRowValue(value, &(nodesArray[nCount])) == FALSE) {
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


/* his_info=3F-1A-CD-90-4B-67|68.123.62.234|32168|0|5|192.168.111.102-192.168.110.1|3478|5 */
BOOL HttpOperate::ParseHisInfoValue(char *value)
{
	char *p;
	char *node_id_str, *ip_str, *port_str, *no_nat_str, *nat_type_str, *priv_ip_str, *priv_port_str, *wait_time_str;

	if (!value) {
		return FALSE;
	}


	/* NODE ID */
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	node_id_str = value;
	mac_addr(node_id_str, m1_peer_node_id, NULL);


	/* IP */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	ip_str = value;
	m1_peer_ip = inet_addr(ip_str);


	/* PORT */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	port_str = value;
	m1_peer_port = atol(port_str);


	/* NO_NAT */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	no_nat_str = value;
	if (strcmp(no_nat_str, "1") == 0) {
		m1_peer_nonat = TRUE;
	}
	else {
		m1_peer_nonat = FALSE;
	}


	/* NAT_TYPE */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	nat_type_str = value;
	m1_peer_nattype = atol(nat_type_str);


	/* PRIVATE IP */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	priv_ip_str = value;
	ParseIpValue(priv_ip_str);

	
	/* PRIVATE PORT */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	priv_port_str = value;
	m1_peer_pri_port = atol(priv_port_str);


	/* WAIT TIME */
	value = p + 1;
	p = strchr(value, '|');
	if (p != NULL) {
		*p = '\0';  /* Last field */
	}
	wait_time_str = value;
	m1_wait_time = atol(wait_time_str);

	return TRUE;
}


//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK1.
//  2: OK2.
//
int HttpOperate::DoConnect(const char *client_charset, const char *client_lang, char *his_node_id_str)
{
#define POST_BUFF_SIZE	(2*1024)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+1];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024];
	char *szPost;
	char *start, *end;
	int result = 0;
	char name[32];
	char value[1024];
	char *next_start = NULL;


	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_http_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, HTTP_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}

	szPost = (char *)malloc(POST_BUFF_SIZE);


	snprintf(szPostBody, sizeof(szPostBody), "sender_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
												"&node_name=%s"
												"&version=%ld"
												"&ip=%s"
												"&port=%d"
												"&pub_ip=%s"
												"&pub_port=%d"
												"&no_nat=%d"
												"&nat_type=%d"
												"&my_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
												"&his_node_id=%s"
												"&client_charset=%s"
												"&client_lang=%s",
					m0_node_id[0], m0_node_id[1], m0_node_id[2], m0_node_id[3], m0_node_id[4], m0_node_id[5],
					UrlEncode(g0_node_name).c_str(), g0_version, MakeIpStr(), m0_port, MakePubIpStr(), m0_pub_port, 
					(m0_no_nat ? 1 : 0),
					m0_nat_type,
					m0_node_id[0], m0_node_id[1], m0_node_id[2], m0_node_id[3], m0_node_id[4], m0_node_id[5],
					his_node_id_str, 
					client_charset, client_lang);
	
	php_md5(szPostBody, szKey1, sizeof(szKey1));
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);
	
	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										HTTP_CONNECT_URL,
										g1_http_server, HTTP_CONNECT_REFERER,
										g1_http_server,
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

		if (strcmp(name, "his_info") == 0) {
			if (ParseHisInfoValue(value) == FALSE) {
				sockClient.CloseSocket();
				free(szPost);
				return -1;
			}
		}
		else if (strcmp(name, "result_code") == 0) {
			if (strcmp(value, "OK1") == 0) {
				result = 1;
			}
			else if (strcmp(value, "OK2") == 0) {
				result = 2;
			}
			else {
				break;
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
int HttpOperate::DoProxy(const char *client_charset, const char *client_lang, char *his_node_id_str, BOOL bIsTcp, WORD wFuncPort)
{
#define POST_BUFF_SIZE	(2*1024)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+1];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024];
	char *szPost;
	char *start, *end;
	int result = 0;
	BOOL bIsServerProxy;
	int ret;
	char name[32];
	char value[1024];
	char *next_start = NULL;


	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_http_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, HTTP_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}


	szPost = (char *)malloc(POST_BUFF_SIZE);



	snprintf(szPostBody, sizeof(szPostBody), "sender_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
												"&proxy_proto=%d"
												"&func_port=%d"
												"&need_applet=0"
												"&my_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
												"&his_node_id=%s"
												"&client_charset=%s"
												"&client_lang=%s",
					m0_node_id[0], m0_node_id[1], m0_node_id[2], m0_node_id[3], m0_node_id[4], m0_node_id[5],
					(bIsTcp ? 1 : 0),
					wFuncPort,
					m0_node_id[0], m0_node_id[1], m0_node_id[2], m0_node_id[3], m0_node_id[4], m0_node_id[5],
					his_node_id_str, 
					client_charset, client_lang);
	
	php_md5(szPostBody, szKey1, sizeof(szKey1));
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);
	
	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										HTTP_PROXY_URL,
										g1_http_server, HTTP_PROXY_REFERER,
										g1_http_server,
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

		if (strcmp(name, "result_code") == 0) {
			if (strcmp(value, "OK") == 0) {
				result = 1;
			}
			else {
				break;
			}
		}
		else if (strcmp(name, "is_server_proxy") == 0) {
			if (strcmp(value, "1") == 0) {
				bIsServerProxy = TRUE;
			}
			else {
				bIsServerProxy = FALSE;
			}
		}
		else if (strcmp(name, "proxy_ip") == 0) {
			m1_peer_ip = inet_addr(value);
		}
		else if (strcmp(name, "proxy_port") == 0) {
			m1_peer_port = atol(value);
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
int HttpOperate::DoSendEmail(const char *client_charset, const char *client_lang, const char *to_email, const char *subject, const char *content)
{
#define POST_BUFF_SIZE	(6*1024)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+1];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[4*1024];
	char *szPost;
	char *start, *end;
	int result = 0;
	int ret;
	char name[32];
	char value[1024];
	char *next_start = NULL;


#ifdef ANDROID_NDK ////Debug
	__android_log_print(ANDROID_LOG_INFO, "DoSendEmail", "[%s, %s], to_email=%s", client_charset, client_lang, to_email);
	__android_log_print(ANDROID_LOG_INFO, "DoSendEmail", "subject=%s", subject);
	__android_log_print(ANDROID_LOG_INFO, "DoSendEmail", "content=%s", content);
#endif

	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_http_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, HTTP_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}


	szPost = (char *)malloc(POST_BUFF_SIZE);



	snprintf(szPostBody, sizeof(szPostBody), "sender_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
												"&sender_cam_id=%ld"
												"&to_email=%s"
												"&subject=%s"
												"&content=%s"
												"&client_charset=%s"
												"&client_lang=%s",
					m0_node_id[0], m0_node_id[1], m0_node_id[2], m0_node_id[3], m0_node_id[4], m0_node_id[5],
					g1_comments_id,
					to_email,
					UrlEncode(subject).c_str(),
					UrlEncode(content).c_str(),
					client_charset, client_lang);
	
	php_md5(szPostBody, szKey1, sizeof(szKey1));
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);
	
	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										HTTP_SENDEMAIL_URL,
										g1_http_server, HTTP_SENDEMAIL_REFERER,
										g1_http_server,
										strlen(szPostBody),
										szPostBody);

#ifdef ANDROID_NDK ////Debug
	__android_log_print(ANDROID_LOG_INFO, "DoSendEmail", "%s", szPost);
#endif

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

		if (strcmp(name, "result_code") == 0) {
			if (strcmp(value, "OK") == 0) {
				result = 1;
			}
			else {
				break;
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
//  n: n>0  comments_id
//
int HttpOperate::DoFetchNode(const char *client_charset, const char *client_lang, const char *external_app_name, const char *external_app_key, const char *type, const char *region, char passwd_buff[], int buff_size, char desc_buff[], int desc_size)
{
#define POST_BUFF_SIZE	(2*1024)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+1];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024];
	char *szPost;
	char *start, *end;
	int result = 0;
	int ret;
	char name[32];
	char value[1024];
	char *next_start = NULL;


	if (dwHttpServerIp == 0) {
		if (TWSocket::GetIPAddr(g1_http_server, szIpAddr, sizeof(szIpAddr)) == 0) {
			return -1;
		}
		dwHttpServerIp = inet_addr(szIpAddr);
	}

	if (sockClient.Create() < 0) {
		return -1;
	}

	if (sockClient.Connect(szIpAddr, HTTP_SERVER_PORT) != 0) {
		dwHttpServerIp = 0;
		sockClient.CloseSocket();
		return -1;
	}


	szPost = (char *)malloc(POST_BUFF_SIZE);



	snprintf(szPostBody, sizeof(szPostBody), "external_app_name=%s"
												"&external_app_key=%s"
												"&type=%s"
												"&region=%s"
												"&client_charset=%s"
												"&client_lang=%s",
					external_app_name, external_app_key, type, region, 
					client_charset, client_lang);
	
	php_md5(szPostBody, szKey1, sizeof(szKey1));
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);
	
	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										HTTP_FETCHNODE_URL,
										g1_http_server, HTTP_FETCHNODE_REFERER,
										g1_http_server,
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

		if (strcmp(name, "comments_id") == 0) {
			result = atoi(value);
		}
		else if (strcmp(name, "passwd") == 0) {
			char szPasswd[MAX_PATH];
			strncpy(szPasswd, value + 2, sizeof(szPasswd));
			strncpy(passwd_buff, Base64::decode(szPasswd).c_str(), buff_size);
		}
		else if (strcmp(name, "description") == 0) {
			strncpy(desc_buff, value, desc_size);
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
