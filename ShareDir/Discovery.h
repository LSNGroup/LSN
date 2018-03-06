#ifndef _DISCOVERY_H
#define _DISCOVERY_H



#define ANYPC_LOCAL_LAN			"[LOCAL_LAN]"



#define DMSG_MAGIC_VALUE			0x6969

#define DMSG_TYPE_DISCOVERY_REQ		0x0001
#define DMSG_TYPE_DISCOVERY_RESP	0x0002


#define DEFAULT_WAIT_TIME				400
#define DEFAULT_RESEND_TIMES			2 // how many times to resend the searching
#define DEFAULT_RETRY_TIMES				4

#define _MAX_UDP_SEND		(1024)
#define _MAX_UDP_REPLY		(1024*2)

#define MAX_BROADCAST_DST_NUM	6


/* 
MSG Format:

| DMSG_MAGIC_VALUE | DMSG_TYPE_DISCOVERY_REQ | node_id (6 bytes) | node_name (256 bytes) | version (4 bytes) |

| DMSG_MAGIC_VALUE | DMSG_TYPE_DISCOVERY_RESP | Node Row Value |

*/



void PrepareBroadcastDests(IP_ADAPTER_INFO *interfaces, DWORD cntInterface, LPDWORD broadcastDsts, int *lpDstsCnt);
DWORD MyLibBroadcastUdp(DWORD dwBroadcastIp, WORD wDstPort,
							  BYTE *lpbSbuf, DWORD cbSbuf,
							  BYTE **lpRBufPtrArray, DWORD *lpcntPtr, DWORD dwWaitms, DWORD dwResendTimes);
BOOL DMsg_CheckSearchReply(BYTE *lpPacket);
DWORD DMsg_SearchServers(BYTE __g0_node_id[], const char *__g0_node_name, DWORD __g0_version, 
							DWORD *broadcastDsts, int nDstsCnt, ANYPC_NODE *lpServer, DWORD *lpcntServer);

#endif /* _DISCOVERY_H */
