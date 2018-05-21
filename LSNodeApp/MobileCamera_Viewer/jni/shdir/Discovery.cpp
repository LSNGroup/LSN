#include "platform.h"
#include "CommonLib.h"
#include "Discovery.h"

#include <sys/ioctl.h>     
#include <sys/types.h>     
#include <sys/socket.h>     
#include <netinet/in.h>     
#include <netdb.h>     
#include <arpa/inet.h>     
#include <net/if.h>    
//#include <net/if_dl.h>  


void PrepareBroadcastDests(DWORD *broadcastDsts, int *lpDstsCnt)
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
	int copied = 0;


	if (broadcastDsts == NULL || lpDstsCnt == NULL || *lpDstsCnt == 0) {
		return;
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	ifc.ifc_len = BUFFERSIZE;
	ifc.ifc_buf = buffer;
	ioctl(sockfd, SIOCGIFCONF, &ifc);

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

		ifrcopy = *ifr;    
		ioctl(sockfd, SIOCGIFBRDADDR, &ifrcopy);

		sin = (struct sockaddr_in *)&(ifrcopy.ifr_addr);    
		if (copied < *lpDstsCnt  &&  sin->sin_addr.s_addr != inet_addr("127.255.255.255")) {
			broadcastDsts[copied] = sin->sin_addr.s_addr;
			copied++;
		}
	}
	
	close(sockfd); 

	/* Add 255.255.255.255 */
	if (copied < *lpDstsCnt) {
		broadcastDsts[copied] = 0xffffffffUL;
		copied++;
	}

	/* OK, give a total count before return. */
	*lpDstsCnt = copied;

#undef min
#undef max
#undef BUFFERSIZE
}

DWORD MyLibBroadcastUdp(DWORD dwBroadcastIp, WORD wDstPort,
							  BYTE *lpbSbuf, DWORD cbSbuf,
							  BYTE **lpRBufPtrArray, DWORD *lpcntPtr, DWORD dwWaitms, DWORD dwResendTimes)
{
	int tlen,tr;
	int nOptVal;  /* BOOL -> int */
	BYTE *newbuffer;
	DWORD dwResult = NO_ERROR,copied = 0;
	int i;
	char *maxreply = NULL;
	fd_set target_fds;
	SOCKET sock = INVALID_SOCKET;
	struct sockaddr_in src,dest,peer;
	socklen_t peer_len = sizeof(peer);
	struct timeval waitval;     


	if (NULL == lpbSbuf || 0 == cbSbuf) {
		dwResult = -1;
		goto exit;
	}

	maxreply = (char *)malloc(_MAX_UDP_REPLY);
	if (NULL == maxreply) {
		dwResult = -1;
		goto exit;
	}

	sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if (INVALID_SOCKET == sock) {
		dwResult = getErrno();
		goto exit;
	}

#if 0
	//bind any IP and any port
	src.sin_family = AF_INET;
	src.sin_addr.s_addr = 0;
	src.sin_port = htons(0);
	tr = bind(sock, (struct sockaddr *)&src, sizeof(src));
	if (0 != tr) {
		dwResult = getErrno();
		goto exit;
	}
#endif

	//set broadcast option
	nOptVal = TRUE;
	tr = setsockopt(sock,SOL_SOCKET,SO_BROADCAST,(const char*)&nOptVal,sizeof(nOptVal));
	if (0 != tr) {
		dwResult = getErrno();
		goto exit;
	}

	//set the destination ip address
	dest.sin_family = AF_INET;
	dest.sin_port = htons(wDstPort);
	dest.sin_addr.s_addr = dwBroadcastIp;
	
	for (i = 0 ; i < dwResendTimes ; i++) {
		tlen = sendto(sock,(const char*)lpbSbuf,cbSbuf,0,(struct sockaddr *)&dest,sizeof(dest)); //udp is no-connection protocol, send out in any case
		if (tlen < 0) {
			dwResult = getErrno();
			goto exit;
		}
	}

	for(;;)
	{	
		FD_ZERO(&target_fds);
		FD_SET(sock, &target_fds);

		waitval.tv_sec  = dwWaitms / 1000;      
		waitval.tv_usec = (dwWaitms % 1000) * 1000; //translate from milli second to micro second

		tr = select(sock+1, &target_fds, NULL, NULL, &waitval);
		if (0 == tr) { // Time out.
			break;
		} 
		
		memset( maxreply, 0x00, _MAX_UDP_REPLY );
		tlen = recvfrom(sock,maxreply,_MAX_UDP_REPLY,0,(sockaddr *)&peer, &peer_len); 	
		if (tlen < 0) {
			dwResult = getErrno();
			goto exit;
		}

		if (lpRBufPtrArray && lpcntPtr) {
			if (copied < *lpcntPtr) { //there are still empty buffers in lpRBufPtrArray
				//try to find lpBuffer is whether there're already in lpServer
				newbuffer = (BYTE*)malloc(tlen+4+2);
				pf_set_dword(newbuffer + 0, peer.sin_addr.s_addr);
				pf_set_word(newbuffer + 4, peer.sin_port);
				memcpy(newbuffer+6,maxreply,tlen);
				lpRBufPtrArray[copied] = newbuffer;
				copied++;	
			}
			else { // Buffer is full, ready to return.
				break;
			}
		}
	}

	if (lpcntPtr) {
		*lpcntPtr = copied; 
	}
	dwResult = NO_ERROR;
	
exit:

	if (maxreply != NULL) {
		free(maxreply);
	}

	if (INVALID_SOCKET != sock) {  
		closesocket(sock);
	}

	//give the output parameter a error case value
	if (NO_ERROR != dwResult) {
		if (lpRBufPtrArray && lpcntPtr) {
			for (i = 0; i < copied; i++) {
				free(lpRBufPtrArray[i]);
			}
			memset(lpRBufPtrArray, 0, (*lpcntPtr) * sizeof(BYTE *));
			*lpcntPtr = 0;
		}
	}

	return dwResult;
}

BOOL DMsg_CheckSearchReply(BYTE *lpPacket)
{
	if (pf_get_word(lpPacket + 0) != htons(DMSG_MAGIC_VALUE)) {
		return FALSE;
	}

	if (pf_get_word(lpPacket + 2) != htons(DMSG_TYPE_DISCOVERY_RESP)) {
		return FALSE;
	}

	return TRUE;
}

DWORD DMsg_SearchServers(BYTE __g0_node_id[], const char *__g0_node_name, DWORD __g0_version, 
							DWORD *broadcastDsts, int nDstsCnt, ANYPC_NODE *lpServer, DWORD *lpcntServer)
{
	DWORD dwResult = NO_ERROR,dwRet,cntPtr,n,i,j,copied;
	BYTE *bEnumPacket = NULL;
	BYTE *lpBuf,**lpPtrArray;
	DWORD dwBroadcastIp;


	bEnumPacket = (BYTE *)malloc(_MAX_UDP_SEND);

	cntPtr = NODES_PER_PAGE * DEFAULT_RESEND_TIMES;
	lpPtrArray = (BYTE**)malloc(cntPtr * sizeof(BYTE*));
	copied = 0;

	memset(bEnumPacket, 0, _MAX_UDP_SEND);
	memset(bEnumPacket, DIRECT_UDP_HEAD_VAL, DIRECT_UDP_HEAD_LEN);
	pf_set_word(bEnumPacket + DIRECT_UDP_HEAD_LEN, htons(DMSG_MAGIC_VALUE));
	pf_set_word(bEnumPacket + DIRECT_UDP_HEAD_LEN + 2, htons(DMSG_TYPE_DISCOVERY_REQ));
	memcpy(bEnumPacket + DIRECT_UDP_HEAD_LEN + 4, __g0_node_id, 6);
	memcpy(bEnumPacket + DIRECT_UDP_HEAD_LEN + 10, __g0_node_name, 256);
	pf_set_dword(bEnumPacket + DIRECT_UDP_HEAD_LEN + 266, htonl(__g0_version));

	for (n = 0; n < nDstsCnt; n++)
	{
		dwBroadcastIp = broadcastDsts[n];
		cntPtr = NODES_PER_PAGE * DEFAULT_RESEND_TIMES;

		dwRet = MyLibBroadcastUdp(
			dwBroadcastIp,	/*	broadcast IP address */
			SECOND_CONNECT_PORT,
			bEnumPacket,
			DIRECT_UDP_HEAD_LEN + 270,
			lpPtrArray,
			&cntPtr,
			DEFAULT_WAIT_TIME,
			1);
			
		if (NO_ERROR != dwRet) {
			continue;
		}

		//copy the server info to lpServer
		for (i = 0 ; i < cntPtr; i++) {
			lpBuf = lpPtrArray[i];
			DWORD use_peer_ip = pf_get_dword(lpBuf + 0);
			WORD use_peer_port = ntohs(pf_get_word(lpBuf + 4));
			if (copied < *lpcntServer) {
				if (DMsg_CheckSearchReply(lpBuf + 6))
				{						
					//try to find lpBuffer is whether there're already in lpServer
					for (j = 0; j < copied; j++) {
						if (0 == memcmp(lpBuf + 10, lpServer[j].node_id_str,17)) 
							break;
					}
					if (copied == j) {
						if (ParseRowValue((char *)(lpBuf + 10), &(lpServer[copied])) == TRUE) {
							lpServer[copied].bLanNode = TRUE;
							snprintf(lpServer[copied].pub_ip_str, sizeof(lpServer[copied].pub_ip_str),
										"%d.%d.%d.%d", 	
										(use_peer_ip & 0x000000ff) >> 0,
										(use_peer_ip & 0x0000ff00) >> 8,
										(use_peer_ip & 0x00ff0000) >> 16,
										(use_peer_ip & 0xff000000) >> 24);
							snprintf(lpServer[copied].pub_port_str, sizeof(lpServer[copied].pub_port_str), "%d", use_peer_port);
							copied++;
						}
					}
				}
			}
			free(lpBuf);
		}
	}

	*lpcntServer = copied;
	dwResult = NO_ERROR;

	if (bEnumPacket) {
		free(bEnumPacket);
	}

	if(lpPtrArray) {
		free(lpPtrArray);
	}

	return dwResult;
}

