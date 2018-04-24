#include "platform.h"
#include "CommonLib.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>
//#include <net/if_dl.h>

#ifdef ANDROID_NDK
#include <android/log.h>
#define log_msg(msg, lev)  __android_log_print(ANDROID_LOG_INFO, "shdir", msg)
#endif


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

int mac_addr(const char *lpMacStr, BYTE *lpMacAddr, int *lpAddrLen)
{
	int nMacBytes[6];
	int i,nScanned;

	if(!lpMacStr || !lpMacAddr) {
		return -1;
	}

	memset(nMacBytes, 0, sizeof(nMacBytes));
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

	if (NULL != lpAddrLen)
		*lpAddrLen = 6;

	return 0;
}

static inline unsigned int get_seed()
{
	struct timeval t_start;

	gettimeofday(&t_start, NULL);
	uint32_t _microsec = (uint32_t)(t_start.tv_usec);

	return _microsec;
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
		srand(get_seed());
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
			int errNum = getErrno();
			if (errNum != EAGAIN && errNum != EINTR)////#define ETIMEDOUT 110 /* Connection timed out */ 
			{
				return -1;
			}
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
			int errNum = getErrno();
			if (errNum != EAGAIN && errNum != EINTR)////#define ETIMEDOUT 110 /* Connection timed out */ 
			{
				return -1;
			}
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
				usleep(20000);
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
				usleep(20000);
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
					usleep(50000);
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
					usleep(50000);
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
		usleep(20000);
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
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	if (strcmp(value, "1") == 0) {
		lpNode->is_admin = TRUE;
	}
	else {
		lpNode->is_admin = FALSE;
	}


	/* is_busy */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	if (strcmp(value, "1") == 0) {
		lpNode->is_busy = TRUE;
	}
	else {
		lpNode->is_busy = FALSE;
	}


	/* audio_channels */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	lpNode->audio_channels = atol(value);


	/* video_channels */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	lpNode->video_channels = atol(value);


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

BOOL ParseChannelRowValue(char *value, CHANNEL_NODE *lpNode)
{
	char *p;


	if (!value || !lpNode) {
		return FALSE;
	}


	/* joined_channel_id */
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	lpNode->channel_id = atol(value);


	/* device_uuid */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->device_uuid, value, sizeof(lpNode->device_uuid));////UrlDecode


	/* device_node_id */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->node_id_str, value, sizeof(lpNode->node_id_str));


	/* comments */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->channel_comments, value, sizeof(lpNode->channel_comments));


	/* public_ip */
	value = p + 1;
	p = strchr(value, '|');
	if (p == NULL) {
		return FALSE;
	}
	*p = '\0';
	strncpy(lpNode->pub_ip_str, value, sizeof(lpNode->pub_ip_str));


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

int CheckStun(char *lpStunServer, WORD wSrcPort, StunAddress4 *lpMappedResult, BOOL *lpNoNAT, int *lpNatType)
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
			|| stunServerAddr.addr == 0)
	{
#ifdef ANDROID_NDK ////Debug
		__android_log_print(ANDROID_LOG_INFO, "CheckStun", "stunParseServerName() failed!");
#endif
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
			#ifdef ANDROID_NDK ////Debug
			__android_log_print(ANDROID_LOG_INFO, "CheckStun", "Some stun error detetecting NAT type");
			#endif
		   return -1;
		   break;
		case StunTypeUnknown:
			#ifdef ANDROID_NDK ////Debug
			__android_log_print(ANDROID_LOG_INFO, "CheckStun", "Some unknown type error detetecting NAT type");
			#endif
		   return -1;
		   break;
		case StunTypeIndependentFilter:
		    #ifdef ANDROID_NDK ////Debug
			__android_log_print(ANDROID_LOG_INFO, "CheckStun", "Independent Mapping, Independent Filter");
			#endif
		   return 1;
		   break;
		case StunTypeDependentFilter:
		    #ifdef ANDROID_NDK ////Debug
			__android_log_print(ANDROID_LOG_INFO, "CheckStun", "Independedt Mapping, Address Dependendent Filter");
			#endif
		   return 1;
		   break;
		case StunTypePortDependedFilter:
		    #ifdef ANDROID_NDK ////Debug
			__android_log_print(ANDROID_LOG_INFO, "CheckStun", "Independent Mapping, Port Dependent Filter");
			#endif
		   return 1;
		   break;
		case StunTypeDependentMapping:
		    #ifdef ANDROID_NDK ////Debug
			__android_log_print(ANDROID_LOG_INFO, "CheckStun", "Dependent Mapping");
			#endif
		   return 1;
		   break;
		case StunTypeOpen:
		    #ifdef ANDROID_NDK ////Debug
			__android_log_print(ANDROID_LOG_INFO, "CheckStun", "Open");
			#endif
			*lpNoNAT = TRUE;
			return 1;
		   break;
		case StunTypeFirewall:
			#ifdef ANDROID_NDK ////Debug
			__android_log_print(ANDROID_LOG_INFO, "CheckStun", "Firewall");
			#endif
			*lpNoNAT = TRUE;
		   return 1; /* 0 -> 1 2009-12-06 */
		   break;
		case StunTypeBlocked:
			#ifdef ANDROID_NDK ////Debug
			__android_log_print(ANDROID_LOG_INFO, "CheckStun", "Blocked or could not reach STUN server");
			#endif
		   return 0;
		   break;
		default:
			#ifdef ANDROID_NDK ////Debug
			__android_log_print(ANDROID_LOG_INFO, "CheckStun", "switch default ????");
			#endif
			return -1;
			break;
	 }
}

int CheckStunSimple(char *lpStunServer, WORD wSrcPort, StunAddress4 *lpMappedResult)
{
	StunAddress4 stunServerAddr;
	StunAddress4 sAddr;


	sAddr.addr=0;
	sAddr.port=wSrcPort;

	stunServerAddr.addr=0;
	if (stunParseServerName(lpStunServer, stunServerAddr) != true 
			|| stunServerAddr.addr == 0)
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
int CheckStunMyself(char *lpStunServer, WORD wSrcPort, StunAddress4 *lpMappedResult, BOOL *lpNoNAT, int *lpNatType)
{
	StunAddress4 stunServerAddr;
	
	stunServerAddr.addr=0;
	if (stunParseServerName(lpStunServer, stunServerAddr) != true 
			|| stunServerAddr.addr == 0)
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
    socklen_t len = sizeof(addr);
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
    	close(sock);
        return -1;
    }
	
	for (int i = 0; i < 5; i++)
	{
	    addr.sin_family = AF_INET;
	    addr.sin_port = htons(wSrcPort);
	    addr.sin_addr.s_addr = htonl(stunServerAddr.addr);
	    
		pf_set_dword(buff, my_addr.sin_addr.s_addr);//???
		pf_set_word(buff + 4, my_addr.sin_port);
		n = sendto(sock, buff, 6, 0, (struct sockaddr *)&addr, sizeof(addr));
		if (n == 6)
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
			if (n == 6)
			{
				lpMappedResult->addr = ntohl(pf_get_dword(buff));
				lpMappedResult->port = ntohs(pf_get_word(buff + 4));
				close(sock);
				
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
		usleep(20000);
	}
	close(sock);
	return -1;
}


DWORD GetLocalAddress(DWORD addresses[], int *lpCount)
{
#define min(a,b)    ((a) < (b) ? (a) : (b))
#define max(a,b)    ((a) > (b) ? (a) : (b))
#define BUFFERSIZE  4096

#ifndef ANDROID_NDK
	int                 len;
#endif
	char                buffer[BUFFERSIZE], *ptr, lastname[IFNAMSIZ], *cptr;
	struct ifconf       ifc;
	struct ifreq        *ifr, ifrcopy;
	struct sockaddr_in  *sin;
	int sockfd;
	int nextAddr = 0;


	if (NULL == addresses || NULL == lpCount) {
		return -1;
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);    
	if (sockfd < 0)    
	{    
		//perror("socket failed");    
		return -1;    
	}    


	ifc.ifc_len = BUFFERSIZE;
	ifc.ifc_buf = buffer;    
	if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0)    
	{    
		//perror("ioctl error");    
		close(sockfd); 
		return -1;    
	}    

	lastname[0] = '\0';    

	for (ptr = buffer; ptr < buffer + ifc.ifc_len; )    
	{    
		ifr = (struct ifreq *)ptr;    
#ifndef ANDROID_NDK
		len = max(sizeof(struct sockaddr), ifr->ifr_addr.sa_len);    
		ptr += sizeof(ifr->ifr_name) + len;  // for next one in buffer     
#else
		ptr = (char *)(ifr + 1);
#endif
		if (ifr->ifr_addr.sa_family != AF_INET)    
		{    
			continue;   // ignore if not desired address family     
		}    

		if ((cptr = (char *)strchr(ifr->ifr_name, ':')) != NULL)    
		{    
			*cptr = '\0';      // replace colon will null     
		}    

		if (strncmp("lo", ifr->ifr_name, 2) == 0)    
		{    
			continue;   /* Skip loopback interface */   
		}    

		if (strncmp(lastname, ifr->ifr_name, IFNAMSIZ) == 0)    
		{    
			continue;   /* already processed this interface */   
		}    

		memcpy(lastname, ifr->ifr_name, IFNAMSIZ);    

		ifrcopy = *ifr;    
		ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy);    
		if ((ifrcopy.ifr_flags & IFF_UP) == 0)    
		{    
			continue;   // ignore if interface not up     
		}    

		sin = (struct sockaddr_in *)&ifr->ifr_addr;    
		if (nextAddr < *lpCount  &&  sin->sin_addr.s_addr != inet_addr("127.0.0.1")) {
			addresses[nextAddr] = sin->sin_addr.s_addr;
			nextAddr++;
		}
	}    
	
	close(sockfd); 
	*lpCount = nextAddr;
	return NO_ERROR;

#undef min
#undef max
#undef BUFFERSIZE
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

    for (size_t i = 0; i < srclen; i++)
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
    
    for(unsigned int pos = 0; pos < len;)
    {
        unsigned int nvalue = utf8_decode((char *)dst.c_str(), &pos);
        dsturl += (unsigned char)nvalue;
    }

    return dsturl;
}

void ConfigUdtSocket(UDTSOCKET fhandle)
{
	//int mss = 1096;
	//UDT::setsockopt(fhandle, 0, UDT_MSS, &mss, sizeof(int));
}

UDPSOCKET UdpOpen(DWORD dwIpAddr, int nPort)
{
	SOCKET sock = INVALID_SOCKET;
	struct sockaddr_in src;
	int tr;
	
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == sock) {
		return INVALID_SOCKET;
	}
	
	int flag = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    
	//bind IP and port
	src.sin_family = AF_INET;
	src.sin_addr.s_addr = dwIpAddr;
	src.sin_port = htons(nPort);
	tr = bind(sock, (struct sockaddr *)&src, sizeof(src));
	if (0 != tr) {
		closesocket(sock);
		return INVALID_SOCKET;
	}

	return sock;
}

int UdpClose(UDPSOCKET udpSock)
{
	return closesocket(udpSock);
}

int UdtStartup()
{
	return UDT::startup();
}

int UdtCleanup()
{
	return UDT::cleanup();
}

UDTSOCKET UdtOpen(DWORD dwIpAddr, int nPort)
{
	SOCKET sock = INVALID_SOCKET;
	struct sockaddr_in src;
	int tr;
	
	sock = UDT::socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == sock) {
		return INVALID_SOCKET;
	}
	
	ConfigUdtSocket(sock);

	//bind IP and port
	src.sin_family = AF_INET;
	src.sin_addr.s_addr = dwIpAddr;
	src.sin_port = htons(nPort);
	tr = UDT::bind(sock, (struct sockaddr *)&src, sizeof(src));
	if (UDT::ERROR == tr) {
		UDT::close(sock);
		return INVALID_SOCKET;
	}

	return sock;
}

UDTSOCKET UdtOpenEx(UDPSOCKET udpSock)
{
	SOCKET sock = INVALID_SOCKET;
	int tr;
	
	sock = UDT::socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == sock) {
		return INVALID_SOCKET;
	}
	
	ConfigUdtSocket(sock);

	tr = UDT::bind(sock, udpSock);
	if (UDT::ERROR == tr) {
		UDT::close(sock);
		return INVALID_SOCKET;
	}

	return sock;
}

int UdtClose(UDTSOCKET udtSock)
{
	return UDT::close(udtSock);
}

int UdtConnect(UDTSOCKET udtSock, DWORD dwIpAddr, int nPort)
{
	sockaddr_in addr;
	
	if (INVALID_SOCKET == udtSock) {
		return -1;
	}
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(nPort);
	addr.sin_addr.s_addr = dwIpAddr;
	memset(&(addr.sin_zero), '\0', 8);
	return UDT::connect(udtSock, (sockaddr*)&addr, sizeof(addr));
}

UDTSOCKET UdtWaitConnect(UDTSOCKET serv, DWORD *lpIpAddr, WORD *lpPort)
{
	if (INVALID_SOCKET == serv) {
		return INVALID_SOCKET;
	}
	
	if (UDT::ERROR == UDT::listen(serv, 1))
	{
		return INVALID_SOCKET;
	}
	
	int ret;
	struct timeval waitval;
	waitval.tv_sec  = UDT_ACCEPT_TIME;
	waitval.tv_usec = 0;
	ret = UDT::select(serv + 1, serv, NULL, NULL, &waitval);
	if (ret == UDT::ERROR || ret == 0) {
		return INVALID_SOCKET;
	}
	
	UDTSOCKET fhandle;
	sockaddr_in their_addr;
	int namelen = sizeof(their_addr);
	if ((fhandle = UDT::accept(serv, (sockaddr*)&their_addr, &namelen)) == UDT::INVALID_SOCK) {
		return INVALID_SOCKET;
	}
	
	if (NULL != lpIpAddr) {
		*lpIpAddr = their_addr.sin_addr.s_addr;
	}
	if (NULL != lpPort) {
		*lpPort = ntohs(their_addr.sin_port);
	}
	return fhandle;
}

int UdtRecv(UDTSOCKET udtSock, char *buff, int buffSize)
{
	if (INVALID_SOCKET == udtSock) {
		return -1;
	}
	
	return UDT::recv(udtSock, buff, buffSize, 0);
}

int UdtSend(UDTSOCKET udtSock, const char *data, int dataLen)
{
	if (INVALID_SOCKET == udtSock) {
		return -1;
	}
	
	return UDT::send(udtSock, data, dataLen, 0);
}

