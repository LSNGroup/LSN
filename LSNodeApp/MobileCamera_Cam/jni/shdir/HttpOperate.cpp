//#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "CommonLib.h"
#include "phpMd5.h"
#include "TWSocket.h"
#include "HttpOperate.h"

#ifdef ANDROID_NDK
#include <android/log.h>
#endif


char g0_device_uuid[MAX_LOADSTRING];
BYTE g0_node_id[6];
char g0_node_name[MAX_LOADSTRING];
DWORD g0_version;
DWORD g0_ipArray[MAX_ADAPTER_NUM];  /* Network byte order */
int g0_ipCount;
WORD g0_port;
DWORD g0_pub_ip;  /* Network byte order */
WORD g0_pub_port;
BOOL g0_no_nat;
int  g0_nat_type;
BOOL g0_is_admin;
BOOL g0_is_busy;
DWORD g0_audio_channels;
DWORD g0_video_channels;
char g0_os_info[MAX_LOADSTRING];

BOOL g1_is_active;
DWORD g1_lowest_version;
DWORD g1_admin_lowest_version;
char g1_http_server[MAX_LOADSTRING];
char g1_stun_server[MAX_LOADSTRING];
DWORD g1_http_client_ip;  /* Network byte order */
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

BYTE g1_peer_node_id[6];
DWORD g1_peer_ip;  /* Network byte order */
WORD g1_peer_port;
BOOL g1_peer_nonat;
int  g1_peer_nattype;
DWORD g1_peer_pri_ipArray[MAX_ADAPTER_NUM];  /* Network byte order */
int g1_peer_pri_ipCount;
WORD g1_peer_pri_port;
int g1_wait_time;
DWORD g1_use_peer_ip;  /* Network byte order */
WORD g1_use_peer_port;
UDPSOCKET g1_use_udp_sock = INVALID_SOCKET;
UDTSOCKET g1_use_udt_sock = INVALID_SOCKET;
SOCKET_TYPE g1_use_sock_type = SOCKET_TYPE_UNKNOWN;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static char szPostFormat[] = 
"POST %s HTTP/1.0\r\n"  // 1.1 -> 1.0 关闭chunked编码
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


const char *MakeIpStr()
{
	static char szIpStr[16*MAX_ADAPTER_NUM];
	struct in_addr inaddr;
	int i;

	strcpy(szIpStr, "");

	if (g0_ipCount > 0) {
		inaddr.s_addr = g0_ipArray[0];
		strcat(szIpStr, inet_ntoa(inaddr));
		for (i = 1; i < g0_ipCount; i++) {
			strcat(szIpStr, "-");
			inaddr.s_addr = g0_ipArray[i];
			strcat(szIpStr, inet_ntoa(inaddr));
		}
	}

	return szIpStr;
}


const char *MakePubIpStr()
{
	static char szPubIpStr[16];
	struct in_addr inaddr;

	inaddr.s_addr = g0_pub_ip;
	strcpy(szPubIpStr, inet_ntoa(inaddr));
	return szPubIpStr;
}


static BOOL ParseIpValue(char *value)
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
			g1_peer_pri_ipArray[nCount] = inet_addr(value);
			nCount += 1;
		}
		if (bLastOne) {
			break;
		}
		value = p + 1;
	}
	g1_peer_pri_ipCount = nCount;
	return TRUE;
}


/* event=Accept|3F-1A-CD-90-4B-67|61.126.78.32|23745|0|5|192.168.1.101-192.168.110.103|3478 */
/* event=AcceptProxy|3F-1A-CD-90-4B-67|1|61.126.78.32|23745 */
static BOOL ParseEventValue(char *value)
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
		mac_addr(node_id_str, g1_peer_node_id, NULL);

		/* IP */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		ip_str = value;
		g1_peer_ip = inet_addr(ip_str);

		/* PORT */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		port_str = value;
		g1_peer_port = atoi(port_str);

		/* NO_NAT */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		no_nat_str = value;
		if (strcmp(no_nat_str, "1") == 0) {
			g1_peer_nonat = TRUE;
		}
		else {
			g1_peer_nonat = FALSE;
		}

		/* NAT_TYPE */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		nat_type_str = value;
		g1_peer_nattype = atoi(nat_type_str);

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
		g1_peer_pri_port = atoi(priv_port_str);

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
		mac_addr(node_id_str, g1_peer_node_id, NULL);

		/* IP */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		ip_str = value;
		g1_peer_ip = inet_addr(ip_str);

		/* PORT */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		port_str = value;
		g1_peer_port = atoi(port_str);

		/* NO_NAT */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		no_nat_str = value;
		if (strcmp(no_nat_str, "1") == 0) {
			g1_peer_nonat = TRUE;
		}
		else {
			g1_peer_nonat = FALSE;
		}

		/* NAT_TYPE */
		value = p + 1;
		p = strchr(value, '|');
		if (p == NULL) {
			return FALSE;
		}
		*p = '\0';
		nat_type_str = value;
		g1_peer_nattype = atoi(nat_type_str);

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
		g1_peer_pri_port = atoi(priv_port_str);


		/* 作为区别Accept类型的标志 */
		g1_peer_pri_ipCount = 0;
		g1_peer_pri_port = atoi(priv_port_str);//SECOND_CONNECT_PORT

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
		mac_addr(node_id_str, g1_peer_node_id, NULL);

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
		g1_peer_ip = inet_addr(ip_str);

		/* Proxy PORT */
		value = p + 1;
		p = strchr(value, '|');
		if (p != NULL) {
			*p = '\0';  /* Last field */
		}
		port_str = value;
		g1_peer_port = atoi(port_str);


		/* 作为区别Accept类型的标志 */
		g1_peer_pri_ipCount = 0;
		g1_peer_pri_port = 0;

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
		mac_addr(node_id_str, g1_peer_node_id, NULL);

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
		g1_peer_ip = inet_addr(ip_str);

		/* Proxy PORT */
		value = p + 1;
		p = strchr(value, '|');
		if (p != NULL) {
			*p = '\0';  /* Last field */
		}
		port_str = value;
		g1_peer_port = atoi(port_str);


		/* 作为区别Accept类型的标志 */
		g1_peer_pri_ipCount = 0;
		g1_peer_pri_port = func_port;//FIRST_CONNECT_PORT

	}

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
//  0: Should exit.
//  1: Should stop.
//  2: OK, continue.
//
int DoRegister1(const char *client_charset, const char *client_lang)
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
//#ifdef JNI_FOR_MOBILECAMERA
			else if (strcmp(name, "lowest_version") == 0) {
				g1_lowest_version = atol(value);
				if (g1_lowest_version > g0_version) {
					bUpgradeVersion = TRUE;
				}
			}
//#else
			else if (strcmp(name, "admin_lowest_version") == 0) {
				g1_admin_lowest_version = atol(value);
				if (g1_admin_lowest_version > g0_version) {
					bUpgradeVersion = TRUE;
				}
			}
//#endif
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
			else if (strcmp(name, "http_client_ip") == 0) {
				DWORD temp_ip = inet_addr(value);
				if (temp_ip != 0) {
					g1_http_client_ip = temp_ip;
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
		//DoUpgradeVersion();
		//result = 0;  /* Exit */
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
int DoRegister2(const char *client_charset, const char *client_lang)
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
		g0_node_id[0], g0_node_id[1], g0_node_id[2], g0_node_id[3], g0_node_id[4], g0_node_id[5], 
		UrlEncode(g0_node_name).c_str(), g0_version, MakeIpStr(), g0_port, MakePubIpStr(), g0_pub_port, 
		(g0_no_nat ? 1 : 0),
		g0_nat_type,
		(g0_is_admin ? 1 : 0),
		(g0_is_busy ? 1 : 0),
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
#ifdef ANDROID_NDK ////Debug
	__android_log_print(ANDROID_LOG_INFO, "RecvHttpResponse", "DoRegister2()...szPost=\r\n%s", szPost);
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
			else if (strcmp(name, "lowest_version") == 0) {
				g1_lowest_version = atol(value);
				if (g1_lowest_version > g0_version) {
					bUpgradeVersion = TRUE;
				}
			}
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
			else if (strcmp(name, "http_client_ip") == 0) {
				DWORD temp_ip = inet_addr(value);
				if (temp_ip != 0) {
					g1_http_client_ip = temp_ip;
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
		//DoUpgradeVersion();
		//result = 0;  /* Exit */
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
int DoUnregister(const char *client_charset, const char *client_lang)
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
					g0_node_id[0], g0_node_id[1], g0_node_id[2], g0_node_id[3], g0_node_id[4], g0_node_id[5], 
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
int DoQueryList(const char *client_charset, const char *client_lang, const char *request_nodes, ANYPC_NODE *nodesArray, int *lpCount, int *lpNum)
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
			g0_node_id[0], g0_node_id[1], g0_node_id[2], g0_node_id[3], g0_node_id[4], g0_node_id[5],
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
static BOOL ParseHisInfoValue(char *value)
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
	mac_addr(node_id_str, g1_peer_node_id, NULL);


	/* IP */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	ip_str = value;
	g1_peer_ip = inet_addr(ip_str);


	/* PORT */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	port_str = value;
	g1_peer_port = atol(port_str);


	/* NO_NAT */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	no_nat_str = value;
	if (strcmp(no_nat_str, "1") == 0) {
		g1_peer_nonat = TRUE;
	}
	else {
		g1_peer_nonat = FALSE;
	}


	/* NAT_TYPE */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	nat_type_str = value;
	g1_peer_nattype = atol(nat_type_str);


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
	g1_peer_pri_port = atol(priv_port_str);


	/* WAIT TIME */
	value = p + 1;
	p = strchr(value, '|');
	if (p != NULL) {
		*p = '\0';  /* Last field */
	}
	wait_time_str = value;
	g1_wait_time = atol(wait_time_str);

	return TRUE;
}


//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK1.
//  2: OK2.
//
int DoConnect(const char *client_charset, const char *client_lang, char *his_node_id_str)
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
					g0_node_id[0], g0_node_id[1], g0_node_id[2], g0_node_id[3], g0_node_id[4], g0_node_id[5],
					UrlEncode(g0_node_name).c_str(), g0_version, MakeIpStr(), g0_port, MakePubIpStr(), g0_pub_port, 
					(g0_no_nat ? 1 : 0),
					g0_nat_type,
					g0_node_id[0], g0_node_id[1], g0_node_id[2], g0_node_id[3], g0_node_id[4], g0_node_id[5],
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
int DoProxy(const char *client_charset, const char *client_lang, char *his_node_id_str, BOOL bIsTcp, WORD wFuncPort)
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
					g0_node_id[0], g0_node_id[1], g0_node_id[2], g0_node_id[3], g0_node_id[4], g0_node_id[5],
					(bIsTcp ? 1 : 0),
					wFuncPort,
					g0_node_id[0], g0_node_id[1], g0_node_id[2], g0_node_id[3], g0_node_id[4], g0_node_id[5],
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
			g1_peer_ip = inet_addr(value);
		}
		else if (strcmp(name, "proxy_port") == 0) {
			g1_peer_port = atol(value);
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


#if defined(FOR_WL_YKZ)

//
// Return Value:
// -1: Error
//  0: NG.
//  1: OK.
//
int DoSendEmail(const char *client_charset, const char *client_lang, const char *to_email, const char *subject, const char *content)
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
					g0_node_id[0], g0_node_id[1], g0_node_id[2], g0_node_id[3], g0_node_id[4], g0_node_id[5],
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
//  1: OK.
//
int DoPutLocation(const char *client_charset, const char *client_lang, DWORD put_time, int num, const char *items)
{
#define POST_BUFF_SIZE	(10*1024)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+1];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[8*1024];
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


	snprintf(szPostBody, sizeof(szPostBody), "sender_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
												"&sender_cam_id=%ld"
												"&put_time=%d"
												"&num=%d"
												"&items=%s"
												"&client_charset=%s"
												"&client_lang=%s",
					g0_node_id[0], g0_node_id[1], g0_node_id[2], g0_node_id[3], g0_node_id[4], g0_node_id[5],
					g1_comments_id,
					put_time,
					num,
					items,
					client_charset, client_lang);
	
	php_md5(szPostBody, szKey1, sizeof(szKey1));
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);
	
	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										HTTP_PUTLOCATION_URL,
										g1_http_server, HTTP_PUTLOCATION_REFERER,
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
int DoPutWPLocation(const char *client_charset, const char *client_lang, DWORD cam_id, DWORD put_time, bool wp_ignore, const char *wp_content, const char *loc_item)
{
#define POST_BUFF_SIZE	(10*1024)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+1];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[8*1024];
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


	snprintf(szPostBody, sizeof(szPostBody), "sender_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
												"&cam_id=%ld"
												"&put_time=%ld"
												"&wp_ignore=%d"
												"&wp_content=%s"
												"&loc_item=%s"
												"&client_charset=%s"
												"&client_lang=%s",
					g0_node_id[0], g0_node_id[1], g0_node_id[2], g0_node_id[3], g0_node_id[4], g0_node_id[5],
					cam_id,
					put_time,
					(wp_ignore ? 1 : 0),
					wp_content,
					loc_item,
					client_charset, client_lang);
	
	php_md5(szPostBody, szKey1, sizeof(szKey1));
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);
	
	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										HTTP_PUTWPLOCATION_URL,
										g1_http_server, HTTP_PUTWPLOCATION_REFERER,
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

#endif


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
int DoQueryNotificationList(const char *client_charset, const char *client_lang, NOTIFICATION_ITEM *notificationArray, int *lpCount)
{
	TWSocket sockClient;
	char szPostBody[256];
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

	sockClient.SetSockSendBufferSize(sizeof(szPost));
	sockClient.SetSockRecvBufferSize(sizeof(szPost));


	snprintf(szPostBody, sizeof(szPostBody), 
			"sender_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
			"&client_charset=%s"
			"&client_lang=%s",
			g0_node_id[0], g0_node_id[1], g0_node_id[2], g0_node_id[3], g0_node_id[4], g0_node_id[5],
			client_charset, client_lang);

	snprintf(szPost, sizeof(szPost), szPostFormat, 
										HTTP_QUERYNOTIFICATIONLIST_URL,
										g1_http_server, HTTP_QUERYNOTIFICATIONLIST_REFERER,
										g1_http_server,
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


#if (defined(FOR_51HZ_GUAJI) || defined(FOR_MAYI_GUAJI))
#include "MayiOperate.cpp"
#endif
