#ifndef _AVRTP_DEF_H
#define _AVRTP_DEF_H


#include "udp.h"
#include "udt.h"
#include "wps_queue.h"


typedef enum _tag_rtp_payload_type {
	PAYLOAD_TLV   = 0, //0, type-length-value
	PAYLOAD_AUDIO = 1, //1
	PAYLOAD_VIDEO = 2, //2
} RTP_PAYLOAD_TYPE;

#define NUM_PAYLOAD_TYPE	3



/* TLV Types */
#define TLV_TYPE_BATTERY1_REMAIN	0x0000
#define TLV_TYPE_BATTERY2_REMAIN	0x0001
#define TLV_TYPE_TEMP				0x0002
#define TLV_TYPE_HUMI				0x0003
#define TLV_TYPE_MQX				0x0004
#define TLV_TYPE_SIGNAL_STRENGTH	0x0005
#define TLV_TYPE_GPS_COUNT		0x0006
#define TLV_TYPE_GPS_LONG		0x0007
#define TLV_TYPE_GPS_LATI		0x0008
#define TLV_TYPE_GPS_ALTI		0x0009
#define TLV_TYPE_TOTAL_TIME		0x000A
#define TLV_TYPE_AIR_SPEED		0x000B
#define TLV_TYPE_GND_SPEED		0x000C
#define TLV_TYPE_DIST			0x000D //from home
#define TLV_TYPE_HEIGHT			0x000E //相对高度，height
#define TLV_TYPE_CLIMB_RATE		0x000F
#define TLV_TYPE_BATTERY1_VOLTAGE	0x0010
#define TLV_TYPE_BATTERY1_CURRENT	0x0011
#define TLV_TYPE_ORIE_X			0x0012
#define TLV_TYPE_ORIE_Y			0x0013
#define TLV_TYPE_ORIE_Z			0x0014
#define TLV_TYPE_USER_A			0x0015
#define TLV_TYPE_USER_B			0x0016
#define TLV_TYPE_USER_C			0x0017
#define TLV_TYPE_OOO			0x0018
#define TLV_TYPE_RC1			0x0019
#define TLV_TYPE_RC2			0x001A
#define TLV_TYPE_RC3			0x001B
#define TLV_TYPE_RC4			0x001C
#define TLV_TYPE_COUNT		0x001D

#define TLV_INVALID_VALUE	(2147483647)
#define TLV_VALUE_TIMES		(100000)



#define RECV_PACKET_SMALL_BUFF_SIZE   (1500)
#define RECV_PACKET_MEDIUM_BUFF_SIZE  (1024*8)
#define RECV_PACKET_BIG_BUFF_SIZE     (1024*24)

typedef struct _tag_recv_packet_small {
	wps_queue link;
	double dfRecvTime;  /* xx.yy seconds */
	WORD wSeqNum;
	BYTE bReserved;
	int nDataSize;      /* header length + payload length */
	unsigned char szData[RECV_PACKET_SMALL_BUFF_SIZE]; /* include 8 bytes header */
} RECV_PACKET_SMALL;

typedef struct _tag_recv_packet_medium {
	wps_queue link;
	double dfRecvTime;  /* xx.yy seconds */
	WORD wSeqNum;
	BYTE bReserved;
	int nDataSize;      /* header length + payload length */
	unsigned char szData[RECV_PACKET_MEDIUM_BUFF_SIZE]; /* include 8 bytes header */
} RECV_PACKET_MEDIUM;

typedef struct _tag_recv_packet_big {
	wps_queue link;
	double dfRecvTime;  /* xx.yy seconds */
	WORD wSeqNum;
	BYTE bReserved;
	int nDataSize;      /* header length + payload length */
	unsigned char szData[RECV_PACKET_BIG_BUFF_SIZE]; /* include 8 bytes header */
} RECV_PACKET_BIG;

/*
	| 1B PayloadType |  1B Reserved  |  2B SeqNum   | 4B TimeStamp

*/



#ifndef _DEFINE_RECV_DATA_NODE
#define _DEFINE_RECV_DATA_NODE

typedef struct _tag_recv_data_node {
	wps_queue link;
	unsigned char *pDataPtr;
	int nDataSize;
	BOOL bMarker;
	DWORD rtptimestamp;
} RECV_DATA_NODE;

#endif /* _DEFINE_RECV_DATA_NODE */



#ifndef _DEFINE_T_RTPPARAM
#define _DEFINE_T_RTPPARAM

typedef struct _tag_t_rtpparam {
	int nBasePort;
	DWORD dwDestIp; /* Network byte order */
	int nDestPort;
	UDPSOCKET udpSock;
	SOCKET fhandle;
	SOCKET_TYPE type;
} T_RTPPARAM;

#endif /* _DEFINE_T_RTPPARAM */



typedef enum _tag_nalu_type_enum {
	NALU_TYPE_UNDEFINED = 0,
	NALU_TYPE_SLICE_NOPART = 1,//1
	NALU_TYPE_SLICE_PARTA,     //2
	NALU_TYPE_SLICE_PARTB,     //3
	NALU_TYPE_SLICE_PARTC,     //4
	NALU_TYPE_SLICE_IDR,  //5
	NALU_TYPE_SEI,        //6
	NALU_TYPE_SEQ_SET,    //7
	NALU_TYPE_PIC_SET,    //8
	NALU_TYPE_ACD,        //9
	NALU_TYPE_END_SEQ,    //10
	NALU_TYPE_END_STREAM, //11
	NALU_TYPE_FILTER,     //12
} NALU_TYPE_ENUM;



#endif /* _AVRTP_DEF_H */
