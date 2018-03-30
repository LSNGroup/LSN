#include "stdafx.h"
#include "atlbase.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "CommonLib.h"


#define LOG_MSG  1

#if LOG_MSG
#include "LogMsg.h"
#endif


extern void GetSoftwareKeyName(LPTSTR szKey, DWORD dwLen);



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

void trim(char *str)
{
     trim_left(str);
	 trim_right(str);
}


static inline unsigned __int64 CalculateMicroseconds(unsigned __int64 performancecount,unsigned __int64 performancefrequency)
{
	unsigned __int64 f = performancefrequency;
	unsigned __int64 a = performancecount;
	unsigned __int64 b = a/f;
	unsigned __int64 c = a%f; // a = b*f+c => (a*1000000)/f = b*1000000+(c*1000000)/f

	return b*1000000ui64+(c*1000000ui64)/f;
}

static inline DWORD CurrentTime()
{
	static int inited = 0;
	static unsigned __int64 microseconds, initmicroseconds;
	static LARGE_INTEGER performancefrequency;

	unsigned __int64 emulate_microseconds, microdiff;
	SYSTEMTIME systemtime;
	FILETIME filetime;

	LARGE_INTEGER performancecount;

	QueryPerformanceCounter(&performancecount);
    
	if(!inited){
		inited = 1;
		QueryPerformanceFrequency(&performancefrequency);
		GetSystemTime(&systemtime);
		SystemTimeToFileTime(&systemtime,&filetime);
		microseconds = ( ((unsigned __int64)(filetime.dwHighDateTime) << 32) + (unsigned __int64)(filetime.dwLowDateTime) ) / 10ui64;
		microseconds-= 11644473600000000ui64; // EPOCH
		initmicroseconds = CalculateMicroseconds(performancecount.QuadPart, performancefrequency.QuadPart);
	}
    
	emulate_microseconds = CalculateMicroseconds(performancecount.QuadPart, performancefrequency.QuadPart);

	microdiff = emulate_microseconds - initmicroseconds;


	DWORD _milisec      = (DWORD)((microseconds + microdiff) / 1000);

	return _milisec;
}

int mac_addr(char *lpMacStr, LPBYTE lpMacAddr, UINT *lpAddrLen)
{
	int nMacBytes[6];
	int i,nScanned;

	if(!lpMacStr || !lpMacAddr) {
		return -1;
	}

	ZeroMemory(nMacBytes,sizeof(nMacBytes));
	nScanned = sscanf(lpMacStr,"%x-%x-%x-%x-%x-%x", nMacBytes, nMacBytes + 1, nMacBytes + 2, 
		nMacBytes + 3, nMacBytes + 4, nMacBytes + 5);

	if (nScanned != 6) {
		memset(nMacBytes, 0, sizeof(nMacBytes));
		nScanned = sscanf(lpMacStr,"%x:%x:%x:%x:%x:%x", nMacBytes, nMacBytes + 1, nMacBytes + 2, 
			nMacBytes + 3, nMacBytes + 4, nMacBytes + 5);
	}

	if (nScanned != 6) {
		memset(nMacBytes, 0, sizeof(nMacBytes));
		nScanned = sscanf(lpMacStr,"%x_%x_%x_%x_%x_%x", nMacBytes, nMacBytes + 1, nMacBytes + 2, 
			nMacBytes + 3, nMacBytes + 4, nMacBytes + 5);
	}

	if (nScanned != 6)
		return -1;

	for(i = 0; i < 6; i++)
	{
		if( nMacBytes[i] < 0 || nMacBytes[i] > 255 )
			return -1;
		lpMacAddr[i] = nMacBytes[i];
	}

	if (lpAddrLen)
		*lpAddrLen = 6;

	return 0;
}

void generate_nodeid(BYTE *bNodeId, int nBuffLen)
{
	int i;
	unsigned int temp;
	BYTE bClientID[6];
	BYTE bZeroID[6] = {0, 0, 0, 0, 0, 0};
	BYTE bSuperID[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	
	do {
		temp = (unsigned int)time(NULL);
#if _SXDEF_LITTLE_ENDIAN
		bClientID[0] = (temp & 0x000000ff) >> 0;
		bClientID[1] = (temp & 0x0000ff00) >> 8;
		bClientID[2] = (temp & 0x00ff0000) >> 16;
		bClientID[3] = (temp & 0xff000000) >> 24;
#else
#error ERROR: Please check here!
#endif
		srand(temp);
		for (i = 4; i < 6; i++) {
			bClientID[i] = (BYTE)(rand());
		}
	} while (memcmp(bClientID, bZeroID, 6) == 0 || memcmp(bClientID, bSuperID, 6) == 0);
	/* To avoid all-zero ID and Super ID */
	
	if (nBuffLen >= 6) {
		memcpy(bNodeId, bClientID, 6);
	}
	else {
		memcpy(bNodeId, bClientID, nBuffLen);
	}
}

BOOL GetSoftwareKeyValue(LPCTSTR szValueName, LPBYTE szValueData, DWORD* lpdwLen)
{
	BOOL bResult = FALSE;
	LONG lRet;
	char szKey[MAX_KEY_NAME];
	HKEY hKey;

	if(NULL == szValueName || NULL == szValueData || NULL == lpdwLen)
		goto exit;

	GetSoftwareKeyName(szKey,MAX_KEY_NAME);
	lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szKey,0,KEY_ALL_ACCESS,&hKey);
	if(ERROR_SUCCESS != lRet)
		goto exit;

	lRet = RegQueryValueEx(hKey,szValueName,NULL,NULL,szValueData,lpdwLen);
	if(ERROR_SUCCESS != lRet)
	{
		RegCloseKey(hKey);
		goto exit;
	}

	RegCloseKey(hKey);	
	bResult = TRUE;

exit:
	return bResult;
}

BOOL SaveSoftwareKeyValue(LPCTSTR szValueName, LPCTSTR szValueData)
{
	BOOL bResult = FALSE;
	LONG lRet;
	char szKey[MAX_KEY_NAME];
	HKEY hKey;

	if(NULL == szValueName || NULL == szValueData) {
		goto exit;
	}

	GetSoftwareKeyName(szKey,MAX_KEY_NAME);
	lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szKey,0,KEY_ALL_ACCESS,&hKey);
	if(ERROR_SUCCESS != lRet) {
		lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE,szKey,0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,NULL);
		if(ERROR_SUCCESS != lRet) {
			goto exit;
		}
	}
	lRet = RegSetValueEx(hKey,szValueName,0,REG_SZ,(LPBYTE)szValueData,lstrlen(szValueData)+1);
	if(ERROR_SUCCESS != lRet) {
		RegCloseKey(hKey);	
		goto exit;
	}

	RegCloseKey(hKey);	
	bResult = TRUE;

exit:
	return bResult;
}

BOOL GetSoftwareKeyDwordValue(LPCTSTR szValueName, DWORD *lpdwValue)
{
	char szValueData[16];
	DWORD dwDataLen = 16, cntScaned;
	BOOL bResult = FALSE;

	if(NULL == lpdwValue) {
		goto exit;
	}

	if(GetSoftwareKeyValue(szValueName,(LPBYTE)szValueData,&dwDataLen) && dwDataLen > 0) {
		cntScaned = sscanf(szValueData,"%d",lpdwValue);
		if(cntScaned > 0) { 
			bResult = TRUE;
		}
	}

exit:
	return bResult;
}

BOOL SaveSoftwareKeyDwordValue(LPCTSTR szValueName, DWORD dwValue)
{
	char szValueData[16];

	sprintf(szValueData, "%ld", dwValue);
	return SaveSoftwareKeyValue(szValueName, szValueData);
}

BOOL do_regcheck(HKEY hKey, const char *lpSubKey)
{
	LONG ret;
	HKEY hMainKey=NULL;
	ret=::RegOpenKeyEx(hKey,lpSubKey, 0, KEY_ALL_ACCESS, &hMainKey);
	if(ret!=ERROR_SUCCESS)
	{
		if(hMainKey!=NULL)
			::RegCloseKey(hMainKey);
		return FALSE;
	}
	else
		return TRUE;
}

BOOL do_regread(HKEY hKey, const char *lpSubKey, const char *lpValueName, LPDWORD lpType, LPBYTE lpData, LPDWORD lpSize)
{
	LONG ret;
	HKEY hMainKey=NULL;
	
	ret=::RegOpenKeyEx(hKey,lpSubKey, 0, KEY_ALL_ACCESS, &hMainKey);
	if(ret!=ERROR_SUCCESS)
	{
		if(hMainKey!=NULL)
			::RegCloseKey(hMainKey);
		return FALSE;
	}
	ret=::RegQueryValueEx(hMainKey,lpValueName,0,lpType,lpData,lpSize);
	::RegCloseKey(hMainKey);
	if(ret!=ERROR_SUCCESS)
		return FALSE;
    else 
		return TRUE;
}

BOOL do_regwrite(HKEY hKey, const char *lpSubKey, const char *lpValueName, DWORD Type, const BYTE *lpData, DWORD Size)
{
	LONG ret;
	HKEY hMainKey=NULL;
	DWORD dwDis;
	
	ret=::RegCreateKeyEx(hKey, lpSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hMainKey, &dwDis);
	if(ret!=ERROR_SUCCESS)
	{
		if(hMainKey!=NULL)
			::RegCloseKey(hMainKey);
		return FALSE;
	}
	ret=::RegSetValueEx(hMainKey,lpValueName,0,Type,lpData,Size);
	::RegCloseKey(hMainKey);
	if(ret!=ERROR_SUCCESS)
		return FALSE;
    else 
		return TRUE;
}

BOOL do_regdelete(HKEY hKey, const char *lpSubKey, const char *lpValueName)
{
	LONG ret;
	HKEY hMainKey=NULL;
	
	ret=::RegOpenKeyEx(hKey,lpSubKey, 0, KEY_ALL_ACCESS, &hMainKey);
	if(ret!=ERROR_SUCCESS)
	{
		if(hMainKey!=NULL)
			::RegCloseKey(hMainKey);
		return FALSE;
	}
	ret=::RegDeleteValue(hMainKey,lpValueName);
	::RegCloseKey(hMainKey);
	if(ret!=ERROR_SUCCESS)
		return FALSE;
    else 
		return TRUE;
}

int RecvStreamUdt(UDTSOCKET fhandle, char *buff, int len)
{
	int ret;
	int remain = len;

	while (remain > 0) {
		ret = UDT::recv(fhandle, buff + (len - remain), remain, 0);
		if (ret > 0) {
			remain -= ret;
		}
		else if (ret == 0) {
			//printf("Debug: fhandle Recv timeout!\n");
			;
		}
		else {
			//printf("Debug: fhandle Connection shutdown!\n");
			return -1;
		}
	}
	return 0;
}

int SendStreamUdt(UDTSOCKET fhandle, char *data, int len)
{
	int ret;
	int remain = len;

	while (remain > 0) {
		ret = UDT::send(fhandle, data + (len - remain), remain, 0);
		if (ret > 0) {
			remain -= ret;
		}
		else if (ret == 0) {
			//printf("Debug: fhandle Send timeout!\n");
			;
		}
		else {
			//printf("Debug: fhandle Connection shutdown!\n");
			return -1;
		}
	}
	return 0;
}

int RecvStreamTcp(SOCKET sock, char *buff, int len)
{
	int ret;
	int remain = len;
   fd_set target_fds;
   fd_set except_fds;
   struct timeval waitval;

	while (remain > 0) {

		FD_ZERO(&target_fds);
		FD_SET(sock, &target_fds);
		FD_ZERO(&except_fds);
		FD_SET(sock, &except_fds);
		waitval.tv_sec  = 1;
		waitval.tv_usec = 0;
		if (select(sock+1, &target_fds, NULL, &except_fds, &waitval) < 0 || FD_ISSET(sock, &except_fds)) {
			return -1;
		}
		else if (!FD_ISSET(sock, &target_fds)) {
			continue;
		}

		ret = recv(sock, buff + (len - remain), remain, 0);
		if (ret > 0) {
			remain -= ret;
		}
		else {
#if LOG_MSG
			int err = getErrno();
			char szMsg[MAX_PATH];
			sprintf(szMsg, "RecvStreamTcp: recv()=%d, getErrno()=%d", ret, err);
			log_msg(szMsg, LOG_LEVEL_ERROR);
#endif
			if (err != ETIMEDOUT)
			{
				log_msg("RecvStreamTcp: getErrno() != ETIMEDOUT", LOG_LEVEL_ERROR);////Debug
				return -1;
			}
			Sleep(1);
		}
	}
	return 0;
}

int SendStreamTcp(SOCKET sock, char *data, int len)
{
	int ret;
	int remain = len;

	while (remain > 0) {
		ret = send(sock, data + (len - remain), remain, 0);
		if (ret > 0) {
			remain -= ret;
		}
		else {
#if LOG_MSG
			int err = getErrno();
			char szMsg[MAX_PATH];
			sprintf(szMsg, "SendStreamTcp: send()=%d, getErrno()=%d", ret, err);
			log_msg(szMsg, LOG_LEVEL_ERROR);
#endif
			if (err != ETIMEDOUT)
			{
				log_msg("SendStreamTcp: getErrno() != ETIMEDOUT", LOG_LEVEL_ERROR);////Debug
				return -1;
			}
			Sleep(1);
		}
	}
	return 0;
}

//
// Return Value:
// -1: Error
//  0: Success
//
int RecvStream(SOCKET_TYPE type, SOCKET sock, char *buff, int len)
{
	if (SOCKET_TYPE_TCP == type)
	{
		return RecvStreamTcp(sock, buff, len);
	}
	else if (SOCKET_TYPE_UDT == type)
	{
		return RecvStreamUdt(sock, buff, len);
	}
	else {
		return -1;
	}
}

//
// Return Value:
// -1: Error
//  0: Success
//
int SendStream(SOCKET_TYPE type, SOCKET sock, char *data, int len)
{
	if (SOCKET_TYPE_TCP == type)
	{
		return SendStreamTcp(sock, data, len);
	}
	else if (SOCKET_TYPE_UDT == type)
	{
		return SendStreamUdt(sock, data, len);
	}
	else {
		return -1;
	}
}

//
// Return Value:
// -1: Error
//  0: Success.
//
int FindOutConnection(SOCKET sock, BYTE __g0_node_id[], BYTE __g1_peer_node_id[],
	DWORD __g1_peer_pri_ipArray[], int __g1_peer_pri_ipCount, WORD __g1_peer_pri_port,
	DWORD __g1_peer_ip, WORD __g1_peer_port,
	DWORD *use_ip, WORD *use_port)
{
#define INTRANET_RECV_WAIT_MS  800
#define REPLY_BUFF_SIZE        64

	int tlen,tr;
	int i,j;
	char buff[REPLY_BUFF_SIZE];
	struct sockaddr_in dest,his;
	socklen_t addr_len;
	fd_set target_fds;
	struct timeval waitval;
	int start_time;
#if 1 /* 2009-12-06 */
	BOOL bHisAddr = FALSE;
#endif


	if (INVALID_SOCKET == sock) {
		return -1;
	}

	start_time = time(NULL);
	while ((time(NULL) - start_time) < (20*INTRANET_RECV_WAIT_MS/1000+1))
	{
		if (FALSE == bHisAddr)
		{
			/* 优先尝试建立内网P2P连接 */
			for (i = 0; i < __g1_peer_pri_ipCount; i++) {
				dest.sin_family = AF_INET;
				dest.sin_addr.s_addr = __g1_peer_pri_ipArray[i];
				dest.sin_port = htons(__g1_peer_pri_port);
				buff[0] = 0x01;
				memcpy(buff + 1, __g0_node_id, 6);
				sendto(sock,buff,7,0,(struct sockaddr *)&dest,sizeof(dest)); //udp is no-connection protocol, send out in any case
				Sleep(20);
			}

			/* 然后尝试用公网IP连接 */
			dest.sin_family = AF_INET;
			dest.sin_addr.s_addr = __g1_peer_ip;
			dest.sin_port = htons(__g1_peer_port);
			buff[0] = 0x01;
			memcpy(buff + 1, __g0_node_id, 6);
			sendto(sock,buff,7,0,(struct sockaddr *)&dest,sizeof(dest)); //udp is no-connection protocol, send out in any case
		}
		else {
			dest.sin_family = AF_INET;
			dest.sin_addr.s_addr = *use_ip;
			dest.sin_port = htons(*use_port);
			buff[0] = 0x02;
			memcpy(buff + 1, __g0_node_id, 6);
			memcpy(buff + 7, __g1_peer_node_id, 6);
			for (j = 0; j < 2; j++) {
				sendto(sock,buff,13,0,(struct sockaddr *)&dest,sizeof(dest)); //udp is no-connection protocol, send out in any case
				Sleep(20);
			}
		}


		FD_ZERO(&target_fds);
		FD_SET(sock, &target_fds);
		
		waitval.tv_sec  = INTRANET_RECV_WAIT_MS / 1000;
		waitval.tv_usec = (INTRANET_RECV_WAIT_MS % 1000) * 1000; //translate from milli second to micro second
		
		tr = select(sock+1, &target_fds, NULL, NULL, &waitval);
		if (!FD_ISSET(sock, &target_fds)) { // Time out.
			continue;
		}

		memset(buff, 0, sizeof(buff));
		addr_len = sizeof(his);
		tlen = recvfrom(sock, buff, sizeof(buff), 0, (struct sockaddr *)&his, &addr_len);
		if (tlen <= 0) {
			continue;
		}

		if (buff[0] == 0x01 && tlen == 7) {
			if (memcmp(buff + 1, __g1_peer_node_id, 6) == 0) {
				dest.sin_family = his.sin_family;
				dest.sin_addr.s_addr = his.sin_addr.s_addr;
				dest.sin_port = his.sin_port;
				buff[0] = 0x02;
				memcpy(buff + 1, __g0_node_id, 6);
				memcpy(buff + 7, __g1_peer_node_id, 6);
				for (j = 0; j < 3; j++) {
					sendto(sock,buff,13,0,(struct sockaddr *)&dest,sizeof(dest)); //udp is no-connection protocol, send out in any case
					Sleep(50);
				}
#if 1 /* 2009-12-06 */
				if (FALSE == bHisAddr) {
					bHisAddr = TRUE;
					*use_ip = his.sin_addr.s_addr;
					*use_port = ntohs(his.sin_port);
				}
#endif
			}
		}
		else if (buff[0] == 0x02 && tlen == 13) {
			if (memcmp(buff + 1, __g1_peer_node_id, 6) == 0 && memcmp(buff + 7, __g0_node_id, 6) == 0) {
				dest.sin_family = his.sin_family;
				dest.sin_addr.s_addr = his.sin_addr.s_addr;
				dest.sin_port = his.sin_port;
				buff[0] = 0x02;
				memcpy(buff + 1, __g0_node_id, 6);
				memcpy(buff + 7, __g1_peer_node_id, 6);
				for (j = 0; j < 8; j++) {
					sendto(sock,buff,13,0,(struct sockaddr *)&dest,sizeof(dest)); //udp is no-connection protocol, send out in any case
					Sleep(50);
				}
				*use_ip = his.sin_addr.s_addr;
				*use_port = ntohs(his.sin_port);
				return 0;
			}
		}
		
		/* UDT/rudp packet come!!! */
		if (tlen != 7 && tlen != 13) {
			*use_ip = his.sin_addr.s_addr;
			*use_port = ntohs(his.sin_port);
			return 0;
		}
	}

	return -1;

#undef INTRANET_RECV_WAIT_MS
#undef REPLY_BUFF_SIZE
}

//
// Return Value:
// -1: Error
//  0: Success.
//
int WaitForProxyReady(SOCKET sock, BYTE __g0_node_id[], DWORD __g1_peer_ip, WORD __g1_peer_port)
{
#define INTRANET_RECV_WAIT_MS  800
#define REPLY_BUFF_SIZE        64

	char buff[REPLY_BUFF_SIZE];
	int tr, tlen;
	struct sockaddr_in  dest,his;
	socklen_t addr_len;
	fd_set target_fds;
	struct timeval waitval;
	int start_time;


	start_time = time(NULL);
	while ((time(NULL) - start_time) < 35)
	{
		dest.sin_family = AF_INET;
		dest.sin_addr.s_addr = __g1_peer_ip;  /* Proxy ip */
		dest.sin_port = htons(__g1_peer_port);/* Proxy port */
		memset(buff, '0', 16);
		sprintf(buff + 16, "%02X-%02X-%02X-%02X-%02X-%02X", __g0_node_id[0], __g0_node_id[1], __g0_node_id[2], __g0_node_id[3], __g0_node_id[4], __g0_node_id[5]); 
		sendto(sock,buff,16+17,0,(struct sockaddr *)&dest,sizeof(dest)); //udp is no-connection protocol, send out in any case
	

		FD_ZERO(&target_fds);
		FD_SET(sock, &target_fds);
		
		waitval.tv_sec  = INTRANET_RECV_WAIT_MS / 1000;
		waitval.tv_usec = (INTRANET_RECV_WAIT_MS % 1000) * 1000; //translate from milli second to micro second
		
		tr = select(sock+1, &target_fds, NULL, NULL, &waitval);
		if (!FD_ISSET(sock, &target_fds)) { // Time out.
			continue;
		}

		memset(buff, 0, sizeof(buff));
		addr_len = sizeof(his);
		tlen = recvfrom(sock, buff, sizeof(buff), 0, (struct sockaddr *)&his, &addr_len);
		if (tlen <= 0) {
			continue;
		}

		buff[tlen] = '\0';
		if (strstr(buff, "OK")) {
			return 0;
		}

	}

	return -1;

#undef INTRANET_RECV_WAIT_MS
#undef REPLY_BUFF_SIZE
}

//
// Return Value:
// None
//
void EndProxy(SOCKET udp_sock, DWORD dest_ip, WORD dest_port)
{
	char buff[16];
	struct sockaddr_in  dest;
	int i;

	if (INVALID_SOCKET == udp_sock || 0 == dest_ip || 0 == dest_port) {
		return;
	}

	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = dest_ip;  /* Proxy ip */
	dest.sin_port = htons(dest_port);/* Proxy port */
	memset(buff, '1', 16);
	for (i = 0; i < 5; i++) {
		sendto(udp_sock,buff,16,0,(struct sockaddr *)&dest,sizeof(dest)); //udp is no-connection protocol, send out in any case
		Sleep(20);
	}
}

BOOL ParseRowValue(char *value, ANYPC_NODE *lpNode)
{
	char *p;


	if (!value || !lpNode) {
		return FALSE;
	}


	/* node_id */
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->node_id_str, value, sizeof(lpNode->node_id_str));


	/* node_name */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->node_name, value, sizeof(lpNode->node_name));////UrlDecode
	if (strcmp(lpNode->node_name, "NONE") == 0) {
		strcpy(lpNode->node_name, "");
	}


	/* version */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	lpNode->version = atol(value);


	/* ip */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->ip_str, value, sizeof(lpNode->ip_str));


	/* port */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->port_str, value, sizeof(lpNode->port_str));


	/* pub_ip */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->pub_ip_str, value, sizeof(lpNode->pub_ip_str));


	/* pub_port */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->pub_port_str, value, sizeof(lpNode->pub_port_str));


	/* no_nat */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	if (strcmp(value, "1") == 0) {
		lpNode->no_nat = TRUE;
	}
	else {
		lpNode->no_nat = FALSE;
	}


	/* nat_type */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	lpNode->nat_type = atol(value);


	/* is_admin */

	/* is_busy */

	/* audio_channels */

	/* video_channels */


	/* os_info */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->os_info, value, sizeof(lpNode->os_info));////UrlDecode
	if (strcmp(lpNode->os_info, "NONE") == 0) {
		strcpy(lpNode->os_info, "");
	}


	/* device_uuid */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->device_uuid, value, sizeof(lpNode->device_uuid));////UrlDecode


	/* comments_id */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	lpNode->comments_id = atol(value);


	/* location */
	value = p + 1;
	p = strchr(value, '|');
	if (p != NULL) { /* Last field */
		*p = '\0';
	}
	strncpy(lpNode->location, value, sizeof(lpNode->location));
	if (strcmp(lpNode->location, "NONE") == 0) {
		strcpy(lpNode->location, "");
	}

	return TRUE;
}

DWORD RunExeWait(LPTSTR szCmdLine, BOOL bShowWnd)
{
	BOOL bRet;
	DWORD dwExitCode;
	PROCESS_INFORMATION processInfo;
	STARTUPINFO startupInfo;
	DWORD dwCreationFlags;

	ZeroMemory(&startupInfo,sizeof(startupInfo));
	startupInfo.cb = sizeof(STARTUPINFO);

	if (bShowWnd) {
		startupInfo.wShowWindow = SW_SHOWNORMAL;
		dwCreationFlags = 0;
	}
	else {
		startupInfo.wShowWindow = SW_HIDE;
		dwCreationFlags = CREATE_NO_WINDOW;
	}

	bRet = ::CreateProcess(
		NULL,		/*	1: lpApplicationName */
		szCmdLine,	/*	2: lpCommandLine */
		NULL,		/*  3: */
		NULL,		/*  4: */
		FALSE,		/*  5: */
		dwCreationFlags,	/*  6: CREATE_NO_WINDOW */
		NULL,		/*  7: */
		NULL,		/*  8: */
		&startupInfo,		/*  9: lpStartupInfo */
		&processInfo			/*  10: */
		);

	if (!bRet) {
		return GetLastError();
	}

	while(1) {
		GetExitCodeProcess(processInfo.hProcess,&dwExitCode);
		if (STILL_ACTIVE != dwExitCode) 
			break;
		Sleep(200);
	}

	::CloseHandle(processInfo.hThread);
	::CloseHandle(processInfo.hProcess);
	return NO_ERROR;
}

DWORD RunExeNoWait(LPTSTR szCmdLine, BOOL bShowWnd)
{
	BOOL bRet;
	PROCESS_INFORMATION processInfo;
	STARTUPINFO startupInfo;
	DWORD dwCreationFlags;

	ZeroMemory(&startupInfo,sizeof(startupInfo));
	startupInfo.cb = sizeof(STARTUPINFO);

	if (bShowWnd) {
		startupInfo.wShowWindow = SW_SHOWNORMAL;
		dwCreationFlags = 0;
	}
	else {
		startupInfo.wShowWindow = SW_HIDE;
		dwCreationFlags = CREATE_NO_WINDOW;
	}

	bRet = ::CreateProcess(
		NULL,		/*	1: lpApplicationName */
		szCmdLine,	/*	2: lpCommandLine */
		NULL,		/*  3: */
		NULL,		/*  4: */
		FALSE,		/*  5: */
		dwCreationFlags,	/*  6: CREATE_NO_WINDOW */
		NULL,		/*  7: */
		NULL,		/*  8: */
		&startupInfo,		/*  9: lpStartupInfo */
		&processInfo			/*  10: */
		);

	if (!bRet) {
		return GetLastError();
	}

	::CloseHandle(processInfo.hThread);
	::CloseHandle(processInfo.hProcess);
	return NO_ERROR;
}

DWORD GetAdapters(IP_ADAPTER_INFO *lpAdapter, LPDWORD lpcntAdapter)
{
	DWORD dwResult = NO_ERROR;
	DWORD dwRet;
	int nCount = 0;
	ULONG ulOutBufLen = 0;
	IP_ADAPTER_INFO *pAdapterInfo = NULL, *pLoopAdapter = NULL;

	if (NULL == lpAdapter || NULL == lpcntAdapter) {
		dwResult = ERROR_INVALID_PARAMETER;	
		goto exit;
	}

	dwRet = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
	if (NO_ERROR != dwRet && ERROR_BUFFER_OVERFLOW != dwRet) {
		dwResult = dwRet;
		goto exit;
	}

	pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
	dwRet = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);	/* call second */
	if (NO_ERROR !=  dwRet) {
		dwResult = dwRet;
		goto exit;
	}

	pLoopAdapter = pAdapterInfo;
	while (pLoopAdapter != NULL) {
		/* 接口类型为： 有线接口，无线接口，拨号网络接口 */
		if ((MIB_IF_TYPE_ETHERNET == pLoopAdapter->Type || 71 == pLoopAdapter->Type || MIB_IF_TYPE_PPP == pLoopAdapter->Type || MIB_IF_TYPE_SLIP == pLoopAdapter->Type) 
					&& inet_addr(pLoopAdapter->IpAddressList.IpAddress.String) != 0 
					&& (DWORD)nCount < *lpcntAdapter) {
			lpAdapter[nCount] = *pLoopAdapter;
			nCount ++;
		}
		pLoopAdapter = pLoopAdapter->Next;
	}
	*lpcntAdapter = nCount;

exit:
	if (pAdapterInfo) {
		free(pAdapterInfo);
	}
	return dwResult;
}

DWORD GetLocalAddress(DWORD addresses[], int *lpCount)
{
	DWORD dwResult = NO_ERROR;
	DWORD dwRet;
	int nCount = 0;
	ULONG ulOutBufLen = 0;
	IP_ADAPTER_INFO *pAdapterInfo = NULL, *pLoopAdapter = NULL;
	DWORD dwIp;

	dwRet = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
	if (NO_ERROR != dwRet && ERROR_BUFFER_OVERFLOW != dwRet) {
		dwResult = dwRet;
		goto exit;
	}

	pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
	dwRet = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);	/* call second */
	if (NO_ERROR !=  dwRet) {
		dwResult = dwRet;
		goto exit;
	}

	pLoopAdapter = pAdapterInfo;
	while (pLoopAdapter != NULL) {
		/* 接口类型为： 有线接口，无线接口，拨号网络接口 */
		if ((MIB_IF_TYPE_ETHERNET == pLoopAdapter->Type || 71 == pLoopAdapter->Type || MIB_IF_TYPE_PPP == pLoopAdapter->Type || MIB_IF_TYPE_SLIP == pLoopAdapter->Type) 
					&& nCount < *lpCount) {
			dwIp = inet_addr(pLoopAdapter->IpAddressList.IpAddress.String);
			if (dwIp != 0) {
				addresses[nCount] = dwIp;
				nCount ++;
			}
		}
		pLoopAdapter = pLoopAdapter->Next;
	}
	*lpCount = nCount;

exit:
	if (pAdapterInfo) {
		free(pAdapterInfo);
	}
	return dwResult;
}

int GetAudioVideoInfo(BOOL *lpAudio, BOOL *lpVideo)
{

	*lpAudio = FALSE;
	*lpVideo = FALSE;
	return 0;
}

int GetDisplayInfo(WORD *lpX, WORD *lpY, WORD *lpBits)
{
	HDC screenDC;
	DEVMODE   dm;   
	dm.dmSize=sizeof(DEVMODE);

	*lpX = 0;
	*lpY = 0;
	*lpBits = 0;

	if (::EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&dm) == TRUE) {
		*lpX = dm.dmPelsWidth;
		*lpY = dm.dmPelsHeight;
		*lpBits = dm.dmBitsPerPel;
	}
	else {
		screenDC = CreateDC("DISPLAY", NULL, NULL, NULL);
		*lpX = GetDeviceCaps(screenDC, HORZRES);
		*lpY = GetDeviceCaps(screenDC, VERTRES);
		*lpBits = GetDeviceCaps(screenDC, BITSPIXEL);
		DeleteDC(screenDC);
	}

	if (*lpX < 10 || *lpX > 10000 || *lpY < 10 || *lpY > 10000) {
		*lpX = GetSystemMetrics(SM_CXSCREEN);
		*lpY = GetSystemMetrics(SM_CYSCREEN);
	}

	return 0;
}

int CheckStun(LPTSTR lpStunServer, WORD wSrcPort, StunAddress4 *lpMappedResult, BOOL *lpNoNAT, int *lpNatType)
{
	NatType stype;
	StunAddress4 stunServerAddr;
	StunAddress4 sAddr;
	bool presPort=false;
	bool hairpin=false;

	sAddr.addr=0;
	sAddr.port=wSrcPort;

	stunServerAddr.addr=0;
	if (stunParseServerName(lpStunServer, stunServerAddr) != true 
			|| stunServerAddr.addr == 0)//这里会指定默认端口号3478
	{
		return -1;
	}

	stype = stunNatType(stunServerAddr, false, &presPort, &hairpin, 
								  wSrcPort, &sAddr);

	if (0 == sAddr.addr || 0 == sAddr.port) {
		stype = StunTypeUnknown;
	}

	lpMappedResult->addr = sAddr.addr;
	lpMappedResult->port = sAddr.port;
	*lpNoNAT = FALSE;
	*lpNatType = stype;

	 switch (stype)
	 {
		case StunTypeFailure:
		   //cout << "Some stun error detetecting NAT type";
		   return -1;
		   break;
		case StunTypeUnknown:
		   //cout << "Some unknown type error detetecting NAT type";
		   return -1;
		   break;
		case StunTypeIndependentFilter:
		   //cout << "Independent Mapping, Independent Filter";
		   return 1;
		   break;
		case StunTypeDependentFilter:
		   //cout << "Independedt Mapping, Address Dependendent Filter";
		   return 1;
		   break;
		case StunTypePortDependedFilter:
		   //cout << "Indepndent Mapping, Port Dependent Filter";
		   return 1;
		   break;
		case StunTypeDependentMapping:
		   //cout << "Dependent Mapping";
		   return 1;
		   break;
		case StunTypeOpen:
		   //cout << "Open";
			*lpNoNAT = TRUE;
			return 1;
		   break;
		case StunTypeFirewall:
		   //cout << "Firewall";
			*lpNoNAT = TRUE;
		   return 1;
		   break;
		case StunTypeBlocked:
		   //cout << "Blocked or could not reach STUN server";
		   return 0;
		   break;
		default:
			return -1;
			break;
	 }
}

int CheckStunSimple(LPTSTR lpStunServer, WORD wSrcPort, StunAddress4 *lpMappedResult)
{
	StunAddress4 stunServerAddr;
	StunAddress4 sAddr;


	sAddr.addr=0;
	sAddr.port=wSrcPort;

	stunServerAddr.addr=0;
	if (stunParseServerName(lpStunServer, stunServerAddr) != true 
			|| stunServerAddr.addr == 0)//这里会指定默认端口号3478
	{
		return -1;
	}

	stunTest(stunServerAddr, 1, false, &sAddr);

	if (sAddr.addr == 0 || sAddr.port == 0) {
		return -1;
	}

	lpMappedResult->addr = sAddr.addr;
	lpMappedResult->port = sAddr.port;
	return 0;
}

//
// Return Value:
// -1: Error
//  0: Success
//
int CheckStunMyself(LPTSTR lpStunServer, WORD wSrcPort, StunAddress4 *lpMappedResult, BOOL *lpNoNAT, int *lpNatType, DWORD *lpTime)
{
	StunAddress4 stunServerAddr;
	
	stunServerAddr.addr=0;
	if (stunParseServerName(lpStunServer, stunServerAddr) != true 
			|| stunServerAddr.addr == 0)//这里会指定默认端口号3478
	{
		return -1;
	}
	
	struct sockaddr_in my_addr;
    struct sockaddr_in addr;
    int sock;
    fd_set target_fds;
    struct timeval waitval;
    int tr;
    char buff[512];
    int len = sizeof(addr);
    int n;
	
    if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return -1;
    }
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(wSrcPort);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
    {
    	closesocket(sock);
        return -1;
    }
	
	for (int i = 0; i < 5; i++)
	{
	    addr.sin_family = AF_INET;
	    addr.sin_port = htons(FIRST_CONNECT_PORT);//这里MyStun服务器端口是固定的
	    addr.sin_addr.s_addr = htonl(stunServerAddr.addr);
	    
		*(DWORD *)buff = my_addr.sin_addr.s_addr;//???
		*(WORD *)(buff + 4) = my_addr.sin_port;
		*(DWORD *)(buff + 6) = CurrentTime();
		n = sendto(sock, buff, 10, 0, (struct sockaddr *)&addr, sizeof(addr));
		if (n == 10)
		{
			FD_ZERO(&target_fds);
			FD_SET(sock, &target_fds);
			
			waitval.tv_sec  = 0;
			waitval.tv_usec = 100 * 1000;
			
			tr = select(sock+1, &target_fds, NULL, NULL, &waitval);
			if (0 == tr) { // Time out.
				continue;
			}
			
			len = sizeof(addr);
			n = recvfrom(sock, buff, sizeof(buff), 0, (struct sockaddr*)&addr, &len);
			if (n == 10)
			{
				lpMappedResult->addr = ntohl(*(DWORD *)buff);
				lpMappedResult->port = ntohs(*(WORD *)(buff + 4));

				DWORD ts = *(DWORD *)(buff + 6);
				ts = (CurrentTime() - ts);
				if (ts > 0 && ts < 15000) {
					*lpTime = (DWORD)((double)(*lpTime) * 0.6f + (double)ts * 0.4f);
				}

				closesocket(sock);
				
				Socket s = openPort( 0/*use ephemeral*/, lpMappedResult->addr, false);
				if (s != INVALID_SOCKET)
				{
					closesocket(s);
					*lpNoNAT = TRUE;
					*lpNatType = StunTypeOpen;
				}
				else
				{
					*lpNoNAT = FALSE;
					*lpNatType = StunTypeIndependentFilter;
				}
				
				return 0;
			}
		}
		Sleep(20);
	}
	closesocket(sock);
	return -1;
}


/* Actually, this function is used to get the UNI-ID from Windows Registry! */
void GetViewerNodeId(LPTSTR lpNodeId, int nBuffLen)
{
	TCHAR szValueData[_MAX_PATH];
	DWORD dwDataLen = _MAX_PATH;

	if (GetSoftwareKeyValue(STRING_REGKEY_NAME_VIEWERNODEID,(LPBYTE)szValueData,&dwDataLen))
	{
		strncpy(lpNodeId, szValueData, nBuffLen);
	}
	else {
		BYTE bNodeId[6];
		generate_nodeid(bNodeId, sizeof(bNodeId));
		snprintf(lpNodeId, nBuffLen, "%02X-%02X-%02X-%02X-%02X-%02X",
			bNodeId[0], bNodeId[1], bNodeId[2], bNodeId[3], bNodeId[4], bNodeId[5]);
		SaveSoftwareKeyValue(STRING_REGKEY_NAME_VIEWERNODEID, lpNodeId);
	}
}

BOOL GetOsInfo(LPTSTR lpInfoBuff, int BuffSize)
{
#define BUFSIZE 80

   OSVERSIONINFOEX osvi;
   BOOL bOsVersionInfoEx;
   char temp[_MAX_PATH];


   strcpy(lpInfoBuff, "");


   // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
   // If that fails, try using the OSVERSIONINFO structure.

   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

   if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
   {
      osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
      if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) ) 
         return FALSE;
   }

   switch (osvi.dwPlatformId)
   {
      // Test for the Windows NT product family.
      case VER_PLATFORM_WIN32_NT:

      // Test for the specific product.
      if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion >= 2 )
         strcat (lpInfoBuff, "Windows 8, ");

      else if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1 )
         strcat (lpInfoBuff, "Windows 7, ");

      else if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0 )
         strcat (lpInfoBuff, "Windows Vista, ");

      else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
         strcat (lpInfoBuff, "Windows Server 2003, ");

      else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
         strcat (lpInfoBuff, "Windows XP, ");

      else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
         strcat (lpInfoBuff, "Windows 2000, ");

      else if ( osvi.dwMajorVersion <= 4 )
         strcat (lpInfoBuff, "Windows NT, ");

      // Test for specific product on Windows NT 4.0 SP6 and later.
      if( bOsVersionInfoEx )
      {
         // Test for the workstation type.
         if ( osvi.wProductType == VER_NT_WORKSTATION )
         {
            if( osvi.dwMajorVersion == 4 )
               strcat (lpInfoBuff, "Workstation 4.0 " );
            else if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
               strcat (lpInfoBuff, "Home Edition " );
            else strcat (lpInfoBuff, "Professional " );
         }
            
         // Test for the server type.
         else if ( osvi.wProductType == VER_NT_SERVER || 
                   osvi.wProductType == VER_NT_DOMAIN_CONTROLLER )
         {
            if(osvi.dwMajorVersion==5 && osvi.dwMinorVersion==2)
            {
               if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                  strcat (lpInfoBuff, "Datacenter Edition " );
               else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                  strcat (lpInfoBuff, "Enterprise Edition " );
               else if ( osvi.wSuiteMask == VER_SUITE_BLADE )
                  strcat (lpInfoBuff, "Web Edition " );
               else strcat (lpInfoBuff, "Standard Edition " );
            }
            else if(osvi.dwMajorVersion==5 && osvi.dwMinorVersion==0)
            {
               if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                  strcat (lpInfoBuff, "Datacenter Server " );
               else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                  strcat (lpInfoBuff, "Advanced Server " );
               else strcat (lpInfoBuff, "Server " );
            }
            else  // Windows NT 4.0 
            {
               if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                  strcat (lpInfoBuff, "Server 4.0, Enterprise Edition " );
               else strcat (lpInfoBuff, "Server 4.0 " );
            }
         }
      }
      // Test for specific product on Windows NT 4.0 SP5 and earlier
      else  
      {
         HKEY hKey;
         char szProductType[BUFSIZE];
         DWORD dwBufLen=BUFSIZE;
         LONG lRet;

         lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
            0, KEY_QUERY_VALUE, &hKey );
         if( lRet != ERROR_SUCCESS )
            return FALSE;

         lRet = RegQueryValueEx( hKey, "ProductType", NULL, NULL,
            (LPBYTE) szProductType, &dwBufLen);
         if( (lRet != ERROR_SUCCESS) || (dwBufLen > BUFSIZE) )
            return FALSE;

         RegCloseKey( hKey );

         if ( lstrcmpi( "WINNT", szProductType) == 0 )
            strcat (lpInfoBuff, "Workstation " );
         if ( lstrcmpi( "LANMANNT", szProductType) == 0 )
            strcat (lpInfoBuff, "Server " );
         if ( lstrcmpi( "SERVERNT", szProductType) == 0 )
            strcat (lpInfoBuff, "Advanced Server " );
		 sprintf(temp, "%d.%d ", osvi.dwMajorVersion, osvi.dwMinorVersion );
		 strcat (lpInfoBuff, temp);

      }

      // Display service pack (if any) and build number.

      if( osvi.dwMajorVersion == 4 && 
          lstrcmpi( osvi.szCSDVersion, "SP6" ) == 0 )
      { 
         HKEY hKey;
         LONG lRet;

         // Test for SP6 versus SP6a.
         lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
  "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009",
            0, KEY_QUERY_VALUE, &hKey );
		 if( lRet == ERROR_SUCCESS ) {
            sprintf(temp, "SP6a (Build %d)", 
					osvi.dwBuildNumber & 0xFFFF );
			strcat (lpInfoBuff, temp);
		}
         else // Windows NT 4.0 prior to SP6a
         {
            sprintf(temp, "%s (Build %d)",
               osvi.szCSDVersion,
               osvi.dwBuildNumber & 0xFFFF);
			strcat (lpInfoBuff, temp);
         }

         RegCloseKey( hKey );
      }
      else // not Windows NT 4.0 
      {
         sprintf(temp, "%s (Build %d)",
            osvi.szCSDVersion,
            osvi.dwBuildNumber & 0xFFFF);
		 strcat (lpInfoBuff, temp);
      }

      break;

      // Test for the Windows Me/98/95.
      case VER_PLATFORM_WIN32_WINDOWS:

      if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
      {
          strcat (lpInfoBuff, "Windows 95 ");
          if (osvi.szCSDVersion[1]=='C' || osvi.szCSDVersion[1]=='B')
             strcat (lpInfoBuff, "OSR2 " );
      } 

      if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
      {
          strcat (lpInfoBuff, "Windows 98 ");
          if ( osvi.szCSDVersion[1] == 'A' )
             strcat (lpInfoBuff, "SE " );
      } 

      if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
      {
          strcat (lpInfoBuff, "Windows ME");
      } 
      break;

      case VER_PLATFORM_WIN32s:

      strcat (lpInfoBuff, "Win32s");
      break;
   }
   
   
	LANGID   lId=GetSystemDefaultLangID();
	switch(lId)
	{ 
	case   0x0804://Chinese   (PRC)
	case   0x1004://Chinese   (SG)
		strcat (lpInfoBuff, " -- CHS");
	break;

	case   0x0C04://Chinese   (HK)
	case   0x1404://Chinese   (Macau SAR) 
	case   0x0404://Chinese   (Taiwan) 
		strcat (lpInfoBuff, " -- CHT");
	break;

	default:
		strcat (lpInfoBuff, " -- Other");
	break;
	}

   return TRUE;

#undef BUFSIZE

}


static unsigned int utf8_decode( char *s, unsigned int *pi )
{
    unsigned int c;
    int i = *pi;
    /* one digit utf-8 */
    if ((s[i] & 128)== 0 ) {
        c = (unsigned int) s[i];
        i += 1;
    } else if ((s[i] & 224)== 192 ) { /* 110xxxxx & 111xxxxx == 110xxxxx */
        c = (( (unsigned int) s[i] & 31 ) << 6) +
            ( (unsigned int) s[i+1] & 63 );
        i += 2;
    } else if ((s[i] & 240)== 224 ) { /* 1110xxxx & 1111xxxx == 1110xxxx */
        c = ( ( (unsigned int) s[i] & 15 ) << 12 ) +
            ( ( (unsigned int) s[i+1] & 63 ) << 6 ) +
            ( (unsigned int) s[i+2] & 63 );
        i += 3;
    } else if ((s[i] & 248)== 240 ) { /* 11110xxx & 11111xxx == 11110xxx */
        c =  ( ( (unsigned int) s[i] & 7 ) << 18 ) +
            ( ( (unsigned int) s[i+1] & 63 ) << 12 ) +
            ( ( (unsigned int) s[i+2] & 63 ) << 6 ) +
            ( (unsigned int) s[i+3] & 63 );
        i+= 4;
    } else if ((s[i] & 252)== 248 ) { /* 111110xx & 111111xx == 111110xx */
        c = ( ( (unsigned int) s[i] & 3 ) << 24 ) +
            ( ( (unsigned int) s[i+1] & 63 ) << 18 ) +
            ( ( (unsigned int) s[i+2] & 63 ) << 12 ) +
            ( ( (unsigned int) s[i+3] & 63 ) << 6 ) +
            ( (unsigned int) s[i+4] & 63 );
        i += 5;
    } else if ((s[i] & 254)== 252 ) { /* 1111110x & 1111111x == 1111110x */
        c = ( ( (unsigned int) s[i] & 1 ) << 30 ) +
            ( ( (unsigned int) s[i+1] & 63 ) << 24 ) +
            ( ( (unsigned int) s[i+2] & 63 ) << 18 ) +
            ( ( (unsigned int) s[i+3] & 63 ) << 12 ) +
            ( ( (unsigned int) s[i+4] & 63 ) << 6 ) +
            ( (unsigned int) s[i+5] & 63 );
        i += 6;
    } else {
        c = '?';
        i++;
    }
    *pi = i;
    return c;
}

std::string UrlEncode(const std::string& src)
{
    static    char hex[] = "0123456789ABCDEF";
    std::string dst;
    
    for (size_t i = 0; i < src.size(); i++)
    {
        unsigned char ch = src[i];
        if (isalnum(ch))
        {
            dst += ch;
        }
        else
            if (src[i] == ' ')
            {
                dst += '+';
            }
            else if (src[i] == '-')
            {
                dst += '-';
            }
            else if (src[i] == '_')
            {
                dst += '_';
            }
            else if (src[i] == '.')
            {
                dst += '.';
            }
            else
            {
                unsigned char c = static_cast<unsigned char>(src[i]);
                dst += '%';
                dst += hex[c / 16];
                dst += hex[c % 16];
            }
    }
    return dst;
}

std::string UrlDecode(const std::string& src)
{
    std::string dst, dsturl;

    int srclen = src.size();

    for (size_t i = 0; i < (size_t)srclen; i++)
    {
        if (src[i] == '%')
        {
            if(isxdigit(src[i + 1]) && isxdigit(src[i + 2]))
            {
                char c1 = src[++i];
                char c2 = src[++i];
                c1 = c1 - 48 - ((c1 >= 'A') ? 7 : 0) - ((c1 >= 'a') ? 32 : 0);
                c2 = c2 - 48 - ((c2 >= 'A') ? 7 : 0) - ((c2 >= 'a') ? 32 : 0);
                dst += (unsigned char)(c1 * 16 + c2);
            }
        }
        else
            if (src[i] == '+')
            {
                dst += ' ';
            }
            else
            {
                dst += src[i];
            }
    }

    int len = dst.size();
    
    for(unsigned int pos = 0; pos < (unsigned int)len;)
    {
        unsigned int nvalue = utf8_decode((char *)dst.c_str(), &pos);
        dsturl += (unsigned char)nvalue;
    }

    return dsturl;
}

// 注释：多字节包括GBK和UTF-8  
int GBK2UTF8(char *szGbk,char *szUtf8,int Len)  
{  
    // 先将多字节GBK（CP_ACP或ANSI）转换成宽字符UTF-16  
    // 得到转换后，所需要的内存字符数  
    int n = MultiByteToWideChar(CP_ACP,0,szGbk,-1,NULL,0);  
    // 字符数乘以 sizeof(WCHAR) 得到字节数  
    WCHAR *str1 = new WCHAR[n + 1];  
    // 转换  
    MultiByteToWideChar(CP_ACP,  // MultiByte的代码页Code Page  
        0,            //附加标志，与音标有关  
        szGbk,        // 输入的GBK字符串  
        -1,           // 输入字符串长度，-1表示由函数内部计算  
        str1,         // 输出  
        n             // 输出所需分配的内存  
        );  
  
    // 再将宽字符（UTF-16）转换多字节（UTF-8）  
    n = WideCharToMultiByte(CP_UTF8, 0, str1, -1, NULL, 0, NULL, NULL);  
    if (n > Len)  
    {  
        delete[]str1;  
        return -1;  
    }  
    WideCharToMultiByte(CP_UTF8, 0, str1, -1, szUtf8, n, NULL, NULL);  
    delete[]str1;  
    str1 = NULL;  
  
    return 0;  
}

//UTF-8 GBK  
int UTF82GBK(char *szUtf8,char *szGbk,int Len)  
{  
    int n = MultiByteToWideChar(CP_UTF8, 0, szUtf8, -1, NULL, 0);  
    WCHAR * wszGBK = new WCHAR[n + 1];  
    memset(wszGBK, 0, sizeof(WCHAR) * (n+1));  
    MultiByteToWideChar(CP_UTF8, 0,szUtf8,-1, wszGBK, n);  
  
    n = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);  
    if (n > Len)  
    {  
        delete[]wszGBK;  
        return -1;  
    }  
  
    WideCharToMultiByte(CP_ACP,0, wszGBK, -1, szGbk, n, NULL, NULL);  
  
    delete[]wszGBK;  
    wszGBK = NULL;  
  
    return 0;  
}

void ConfigUdtSocket(UDTSOCKET fhandle)
{
//	int mss = 1096;
//	UDT::setsockopt(fhandle, 0, UDT_MSS, &mss, sizeof(int));
}

void CopyTextToClipboard(const char *txt)
{
	HGLOBAL hClip;
	if (OpenClipboard(NULL))
	{
		EmptyClipboard();

		hClip = GlobalAlloc(GMEM_MOVEABLE, strlen(txt)+1);
		char *buff = (char*)GlobalLock(hClip);
		strcpy(buff, txt);
		GlobalUnlock(hClip);
		SetClipboardData(CF_TEXT, hClip);

		CloseClipboard();
	}
}
