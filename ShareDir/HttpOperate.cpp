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
#ifdef WIN32
#include "LogMsg.h"
#endif

#ifdef ANDROID_NDK
#include <android/log.h>
#define log_msg(msg, lev)  __android_log_print(ANDROID_LOG_INFO, "shdir", msg)
#endif


#define HTTP_REGISTER_URL		"/lsnctrl/Register.php"
#define HTTP_REGISTER_REFERER	"/lsnctrl/Register.html"
#define HTTP_UNREGISTER_URL		"/lsnctrl/Unregister.php"
#define HTTP_UNREGISTER_REFERER	"/lsnctrl/Unregister.html"
#define HTTP_QUERYCHANNELS_URL		"/lsnctrl/QueryChannels.php"
#define HTTP_QUERYCHANNELS_REFERER	"/lsnctrl/QueryChannels.html"
#define HTTP_PUSH_URL				"/lsnctrl/Push.php"
#define HTTP_PUSH_REFERER			"/lsnctrl/Push.html"
#define HTTP_PULL_URL				"/lsnctrl/Pull.php"
#define HTTP_PULL_REFERER			"/lsnctrl/Pull.html"
#define HTTP_REPORT_URL				"/lsnctrl/Report.php"
#define HTTP_REPORT_REFERER			"/lsnctrl/Report.html"
#define HTTP_DROP_URL				"/lsnctrl/Drop.php"
#define HTTP_DROP_REFERER			"/lsnctrl/Drop.html"
#define HTTP_EVALUATE_URL			"/lsnctrl/Evaluate.php"
#define HTTP_EVALUATE_REFERER		"/lsnctrl/Evaluate.html"

#define POST_RESULT_START		"FLAG5TGB6YHNSTART"
#define POST_RESULT_END			"FLAG5TGB6YHNEND"



char g0_device_uuid[MAX_LOADSTRING];
char g0_node_name[MAX_LOADSTRING];
DWORD g0_version;
char g0_os_info[MAX_LOADSTRING];

BOOL g1_is_active;
DWORD g1_lowest_version;
char g1_http_server[MAX_LOADSTRING];
char g1_stun_server[MAX_LOADSTRING];
char g1_measure_server[MAX_LOADSTRING];
int g1_register_period;  /* Seconds */
int g1_expire_period;  /* Seconds */

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


BOOL ParseLine(char *start, char *name, int name_size, char *value, int value_size, char **next)
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
				usleep(800000);
			}
		}
	}

	return -1;
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


//	event,  /* event=3F-1A-CD-90-4B-67|6|Accept    |4F-1A-CD-90-66-88|61.126.78.32|23745|0|5|192.168.1.101-192.168.110.103|3478 */
//          /* event=3F-1A-CD-90-4B-67|6|AcceptRev |4F-1A-CD-90-66-88|61.126.78.32|23745|0|5|192.168.1.101-192.168.110.103|3478 */
//          /* event=3F-1A-CD-90-4B-67|6|Connect   |4F-1A-CD-90-66-88|61.126.78.32|23745|0|5|192.168.1.101-192.168.110.103|3478 */
//          /* event=3F-1A-CD-90-4B-67|6|ConnectRev|4F-1A-CD-90-66-88|61.126.78.32|23745|0|5|192.168.1.101-192.168.110.103|3478 */
//          /* event=3F-1A-CD-90-4B-67|6|Disconnect|4F-1A-CD-90-66-88 */
//                   the_node_id   wait_time  type     peer_node_id       pub_ip  pub_port  no_nat  nat_type  ip  port 
BOOL HttpOperate::ParseEventValue(char *value)
{
	char *p;
	char *the_node_id_str, *wait_time_str, *type_str, *node_id_str, *ip_str, *port_str, *no_nat_str, *nat_type_str, *priv_ip_str, *priv_port_str;
	int func_port;

	if (NULL == value) {
		return FALSE;
	}

	/* THE NODE ID */
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	the_node_id_str = value;
	BYTE the_node_id[6];
	mac_addr(the_node_id_str, the_node_id, NULL);
	if (memcmp(the_node_id, m0_node_id, 6) != 0) {
		return FALSE;
	}

	/* WAIT SECONDS */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	wait_time_str = value;
	m1_wait_time  = atoi(wait_time_str);

	/* TYPE STR */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	type_str = value;
	if (strcmp(type_str, "Accept") == 0 || strcmp(type_str, "AcceptRev") == 0 || strcmp(type_str, "Connect") == 0 || strcmp(type_str, "ConnectRev") == 0)
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
	else if (strcmp(type_str, "Disconnect") == 0)
	{

		/* NODE ID */
		value = p + 1;
		p = strchr(value, '|');
		if (p != NULL) {
			*p = '\0';  /* Last field */
		}
		node_id_str = value;
		mac_addr(node_id_str, m1_peer_node_id, NULL);
	}
	else {
		return FALSE;
	}

	strncpy(m1_event_type, type_str, sizeof(m1_event_type));
	return TRUE;
}


/* his_info=3F-1A-CD-90-4B-67|4|68.123.62.234|32168|0|5|192.168.111.102-192.168.110.1|3478|5 */
BOOL HttpOperate::ParseHisInfoValue(char *value)
{
	char *p;
	char *node_id_str, *topo_level_str, *ip_str, *port_str, *no_nat_str, *nat_type_str, *priv_ip_str, *priv_port_str, *wait_time_str;

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


	/* TOPO LEVEL */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	topo_level_str = value;
	m1_peer_topo_level = atol(topo_level_str);


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
//  0: Success
//
int HttpOperate::ParseTopoSettings(const char *settings_string)
{
	char local_buff[1024*2];
	char *start;
	char name[32];
	char value[1024];
	char *next_start = NULL;
	BOOL bUpgradeVersion = FALSE;
	int result;


	if (NULL == settings_string) {
		return -1;
	}
	strncpy(local_buff, settings_string, sizeof(local_buff));
	start = local_buff;

	while(1) {

		if (ParseLine(start, name, sizeof(name), value, sizeof(value), &next_start) == FALSE) {
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
			else if (strcmp(name, "measure_server") == 0) {
				strncpy(value, UrlDecode(value).c_str(), sizeof(value));
				if (strcmp(value, g1_measure_server) != 0) {
					strncpy(g1_measure_server, value, sizeof(g1_measure_server));
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
		}


		if (next_start == NULL) {
			break;
		}
		start = next_start;
	}
	return 0;
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
			else if (strcmp(name, "measure_server") == 0) {
				strncpy(value, UrlDecode(value).c_str(), sizeof(value));
				if (strcmp(value, g1_measure_server) != 0) {
					strncpy(g1_measure_server, value, sizeof(g1_measure_server));
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
		}


		if (next_start == NULL) {
			break;
		}
		start = next_start;
	}


	sockClient.ShutDown();
	sockClient.CloseSocket();

	if (bUpgradeVersion) {
		//if (NULL == hThread_Upgrade) {
		//	hThread_Upgrade = ::CreateThread(NULL,0,UpgradeThreadFn,NULL,0,&dwThreadID_Upgrade);
		//}
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
int HttpOperate::DoQueryChannels(const char *client_charset, const char *client_lang, int page_offset, int page_rows, CHANNEL_NODE *nodesArray, int *lpCount, int *lpNum)
{
#define POST_BUFF_SIZE	(1024*NODES_PER_PAGE)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+1];
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
			"page_offset=%d"
			"&page_rows=%d"
			"&client_charset=%s"
			"&client_lang=%s",
			page_offset,
			page_rows,
			client_charset, client_lang);

	php_md5(szPostBody, szKey1, sizeof(szKey1));
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);

	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										HTTP_QUERYCHANNELS_URL,
										g1_http_server, HTTP_QUERYCHANNELS_REFERER,
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
					if (ParseChannelRowValue(value, &(nodesArray[nCount])) == FALSE) {
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
//  0: NG.
//  1: OK.event
//
int HttpOperate::DoPush(const char *client_charset, const char *client_lang, const char *channel_comments, int *joined_channel_id)
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
												"&channel_comments=%s"
												"&client_charset=%s"
												"&client_lang=%s",
					m0_node_id[0], m0_node_id[1], m0_node_id[2], m0_node_id[3], m0_node_id[4], m0_node_id[5],
					UrlEncode(g0_node_name).c_str(), g0_version, MakeIpStr(), m0_port, MakePubIpStr(), m0_pub_port, 
					(m0_no_nat ? 1 : 0),
					m0_nat_type,
					UrlEncode(channel_comments).c_str(),
					client_charset, client_lang);
	
	php_md5(szPostBody, szKey1, sizeof(szKey1));
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);
	
	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										HTTP_PUSH_URL,
										g1_http_server, HTTP_PUSH_REFERER,
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

		if (strcmp(name, "event") == 0) {
			if (ParseEventValue(value) == FALSE) {
				sockClient.CloseSocket();
				free(szPost);
				return -1;
			}
			else {
				result = 1;
			}
		}
		else if (strcmp(name, "joined_channel_id") == 0) {
			if (joined_channel_id != NULL) {
				*joined_channel_id = atol(value);
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
//  1: OK.his_info
//
int HttpOperate::DoPull(const char *client_charset, const char *client_lang, int channel_id, BOOL isFromStar)
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
												"&channel_id=%d"
												"&from_star=%d"
												"&client_charset=%s"
												"&client_lang=%s",
					m0_node_id[0], m0_node_id[1], m0_node_id[2], m0_node_id[3], m0_node_id[4], m0_node_id[5],
					UrlEncode(g0_node_name).c_str(), g0_version, MakeIpStr(), m0_port, MakePubIpStr(), m0_pub_port, 
					(m0_no_nat ? 1 : 0),
					m0_nat_type,
					channel_id,
					(isFromStar ? 1 : 0),
					client_charset, client_lang);
	
	php_md5(szPostBody, szKey1, sizeof(szKey1));
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);
	
	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										HTTP_PULL_URL,
										g1_http_server, HTTP_PULL_REFERER,
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
			else {
				result = 1;
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
//  0: Should stop.
//  1: OK.
//  2: settings
//  3: settings,event
int HttpOperate::DoReport1(const char *client_charset, const char *client_lang, ON_REPORT_SETTINGS_FN on_settings_fn)
{
#define POST_BUFF_SIZE  (1024*4)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+1];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024*4];
	char *szPost;
	char *start, *end;
	int result = 1;
	char name[32];
	char value[1024];
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
										HTTP_REPORT_URL,
										g1_http_server, HTTP_REPORT_REFERER,
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


	strcpy(szPostBody, "");

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
					result = 0;  /* stop */
					break;
				}
				strcat(szPostBody, name);
				strcat(szPostBody, "=");
				strcat(szPostBody, value);
				strcat(szPostBody, "\n");
			}
			else if (strcmp(name, "lowest_version") == 0) {
				strcat(szPostBody, name);
				strcat(szPostBody, "=");
				strcat(szPostBody, value);
				strcat(szPostBody, "\n");
			}
			else if (strcmp(name, "http_server") == 0) {
				strcat(szPostBody, name);
				strcat(szPostBody, "=");
				strcat(szPostBody, value);
				strcat(szPostBody, "\n");
			}
			else if (strcmp(name, "stun_server") == 0) {
				strcat(szPostBody, name);
				strcat(szPostBody, "=");
				strcat(szPostBody, value);
				strcat(szPostBody, "\n");
			}
			else if (strcmp(name, "measure_server") == 0) {
				strcat(szPostBody, name);
				strcat(szPostBody, "=");
				strcat(szPostBody, value);
				strcat(szPostBody, "\n");
			}
			else if (strcmp(name, "http_client_ip") == 0) {
				strcat(szPostBody, name);
				strcat(szPostBody, "=");
				strcat(szPostBody, value);
				strcat(szPostBody, "\n");
			}
			else if (strcmp(name, "register_period") == 0) {
				strcat(szPostBody, name);
				strcat(szPostBody, "=");
				strcat(szPostBody, value);
				strcat(szPostBody, "\n");
			}
			else if (strcmp(name, "expire_period") == 0) {
				strcat(szPostBody, name);
				strcat(szPostBody, "=");
				strcat(szPostBody, value);
				strcat(szPostBody, "\n");
			}
		}


		if (next_start == NULL) {
			break;
		}
		start = next_start;
	}


	sockClient.ShutDown();
	sockClient.CloseSocket();

	if (NULL != on_settings_fn && strlen(szPostBody) > 0)
	{
		result = 2;
		on_settings_fn(szPostBody);
	}

	if (bUpgradeVersion) {
		//if (NULL == hThread_Upgrade) {
		//	hThread_Upgrade = ::CreateThread(NULL,0,UpgradeThreadFn,NULL,0,&dwThreadID_Upgrade);
		//}
		result = 2;
	}

	free(szPost);
	return result;

#undef POST_BUFF_SIZE
}


//
// Return Value:
// -1: Error
//  0: Should stop.
//  1: OK.
//  2: settings
//  3: settings,event
int HttpOperate::DoReport2(const char *client_charset, const char *client_lang, 
	DWORD joined_channel_id, BYTE joined_node_id[6], int device_node_num, int viewer_grow_rate,
	const char *root_device_uuid, const char *root_public_ip, BYTE device_node_id[6],
	int route_item_num, int route_item_max,
	int level_1_max_connections, int level_1_current_connections, int level_1_max_streams, int level_1_current_streams,
	int level_2_max_connections, int level_2_current_connections, int level_2_max_streams, int level_2_current_streams,
	int level_3_max_connections, int level_3_current_connections, int level_3_max_streams, int level_3_current_streams,
	int level_4_max_connections, int level_4_current_connections, int level_4_max_streams, int level_4_current_streams,
	const char *node_array, //待连接Node集合的String数据(,;)
	ON_REPORT_SETTINGS_FN on_settings_fn, ON_REPORT_EVENT_FN on_event_fn)
{
#define POST_BUFF_SIZE  (1024*32)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+1];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[1024*32];
	char *szPost;
	char *start, *end;
	int result = 1;
	char name[32];
	char value[1024];
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


	snprintf(szPostBody, sizeof(szPostBody), 
			 "settings_only=0"
			 "&joined_channel_id=%ld"
			 "&joined_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
			 "&device_node_num=%d"
			 "&viewer_grow_rate=%d"
			 "&root_device_uuid=%s"
			 "&root_public_ip=%s"
			 "&device_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
			 "&route_item_num=%d"
			 "&route_item_max=%d"
			 "&level_1_max_connections=%d"
			 "&level_1_current_connections=%d"
			 "&level_1_max_streams=%d"
			 "&level_1_current_streams=%d"
			 "&level_2_max_connections=%d"
			 "&level_2_current_connections=%d"
			 "&level_2_max_streams=%d"
			 "&level_2_current_streams=%d"
			 "&level_3_max_connections=%d"
			 "&level_3_current_connections=%d"
			 "&level_3_max_streams=%d"
			 "&level_3_current_streams=%d"
			 "&level_4_max_connections=%d"
			 "&level_4_current_connections=%d"
			 "&level_4_max_streams=%d"
			 "&level_4_current_streams=%d"
			 "&node_array=%s"
			 "&client_charset=%s"
			 "&client_lang=%s",
			joined_channel_id, 
			joined_node_id[0],joined_node_id[1],joined_node_id[2],joined_node_id[3],joined_node_id[4],joined_node_id[5],
			device_node_num, viewer_grow_rate,
			root_device_uuid, root_public_ip, 
			device_node_id[0],device_node_id[1],device_node_id[2],device_node_id[3],device_node_id[4],device_node_id[5],
			route_item_num, route_item_max,
			level_1_max_connections, level_1_current_connections, level_1_max_streams, level_1_current_streams,
			level_2_max_connections, level_2_current_connections, level_2_max_streams, level_2_current_streams,
			level_3_max_connections, level_3_current_connections, level_3_max_streams, level_3_current_streams,
			level_4_max_connections, level_4_current_connections, level_4_max_streams, level_4_current_streams,
			UrlEncode(node_array).c_str(),
			client_charset, client_lang);
	
	php_md5(szPostBody, szKey1, sizeof(szKey1));
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);
	
	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										HTTP_REPORT_URL,
										g1_http_server, HTTP_REPORT_REFERER,
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


	strcpy(szPostBody, "");

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
					result = 0;  /* Stop */
					break;
				}
				strcat(szPostBody, name);
				strcat(szPostBody, "=");
				strcat(szPostBody, value);
				strcat(szPostBody, "\n");
			}
			else if (strcmp(name, "lowest_version") == 0) {
				strcat(szPostBody, name);
				strcat(szPostBody, "=");
				strcat(szPostBody, value);
				strcat(szPostBody, "\n");
			}
			else if (strcmp(name, "http_server") == 0) {
				strcat(szPostBody, name);
				strcat(szPostBody, "=");
				strcat(szPostBody, value);
				strcat(szPostBody, "\n");
			}
			else if (strcmp(name, "stun_server") == 0) {
				strcat(szPostBody, name);
				strcat(szPostBody, "=");
				strcat(szPostBody, value);
				strcat(szPostBody, "\n");
			}
			else if (strcmp(name, "measure_server") == 0) {
				strcat(szPostBody, name);
				strcat(szPostBody, "=");
				strcat(szPostBody, value);
				strcat(szPostBody, "\n");
			}
			else if (strcmp(name, "http_client_ip") == 0) {
				strcat(szPostBody, name);
				strcat(szPostBody, "=");
				strcat(szPostBody, value);
				strcat(szPostBody, "\n");
			}
			else if (strcmp(name, "register_period") == 0) {
				strcat(szPostBody, name);
				strcat(szPostBody, "=");
				strcat(szPostBody, value);
				strcat(szPostBody, "\n");
			}
			else if (strcmp(name, "expire_period") == 0) {
				strcat(szPostBody, name);
				strcat(szPostBody, "=");
				strcat(szPostBody, value);
				strcat(szPostBody, "\n");
			}
			else if (strcmp(name, "is_activated") == 0) {

			}
			else if (strcmp(name, "comments_id") == 0) {

			}
			else if (strcmp(name, "event") == 0) {
				if (NULL != on_event_fn)
				{
					result = 3;
					on_event_fn(value);
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

	if (NULL != on_settings_fn && strlen(szPostBody) > 0)
	{
		result = 2;
		on_settings_fn(szPostBody);
	}

	if (bUpgradeVersion) {
		//if (NULL == hThread_Upgrade) {
		//	hThread_Upgrade = ::CreateThread(NULL,0,UpgradeThreadFn,NULL,0,&dwThreadID_Upgrade);
		//}
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
int HttpOperate::DoDrop(const char *client_charset, const char *client_lang, BYTE sender_node_id[6], BOOL is_admin, BYTE node_id[6])
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



	snprintf(szPostBody, sizeof(szPostBody), "sender_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
												"&is_admin=%d"
												"&node_id=%02X-%02X-%02X-%02X-%02X-%02X"
												"&client_charset=%s"
												"&client_lang=%s",
					sender_node_id[0], sender_node_id[1], sender_node_id[2], sender_node_id[3], sender_node_id[4], sender_node_id[5],
					(is_admin ? 1 : 0),
					node_id[0], node_id[1], node_id[2], node_id[3], node_id[4], node_id[5],
					client_charset, client_lang);
	
	php_md5(szPostBody, szKey1, sizeof(szKey1));
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);
	
	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										HTTP_DROP_URL,
										g1_http_server, HTTP_DROP_REFERER,
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
int HttpOperate::DoEvaluate(const char *client_charset, const char *client_lang, BYTE sender_node_id[6], const char *record_array)
{
#define POST_BUFF_SIZE	(32*1024)

	TWSocket sockClient;
	char szKey1[PHP_MD5_OUTPUT_CHARS+1];
	char szKey2[PHP_MD5_OUTPUT_CHARS+1];
	char szPostBody[32*1024];
	char *szPost;
	char *start, *end;
	int result = 0;
	int ret;
	char name[32];
	char value[1024];
	char *next_start = NULL;


	if (record_array == NULL) {
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


	szPost = (char *)malloc(POST_BUFF_SIZE);



	snprintf(szPostBody, sizeof(szPostBody), "sender_node_id=%02X-%02X-%02X-%02X-%02X-%02X"
												"&record_array=%s"
												"&client_charset=%s"
												"&client_lang=%s",
					sender_node_id[0], sender_node_id[1], sender_node_id[2], sender_node_id[3], sender_node_id[4], sender_node_id[5],
					UrlEncode(record_array).c_str(),
					client_charset, client_lang);
	
	php_md5(szPostBody, szKey1, sizeof(szKey1));
	php_md5(szKey1, szKey2, sizeof(szKey2));
	strcat(szPostBody, "&session_key=");
	strcat(szPostBody, szKey2);
	
	snprintf(szPost, POST_BUFF_SIZE, szPostFormat, 
										HTTP_EVALUATE_URL,
										g1_http_server, HTTP_EVALUATE_REFERER,
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
