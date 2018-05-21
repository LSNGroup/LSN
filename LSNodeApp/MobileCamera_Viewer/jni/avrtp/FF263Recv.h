#ifndef _FF263RECV_H
#define _FF263RECV_H


#include "FakeRtpRecv.h"



#ifndef VIDEO_GET_BUFFER_SIZE

#define VIDEO_GET_BUFFER_SIZE		(640*480*4)

typedef enum _tag_video_mediasubtype_enum {
	VIDEO_MEDIASUBTYPE_RGB32X = 0,
	VIDEO_MEDIASUBTYPE_RGB32,
	VIDEO_MEDIASUBTYPE_RGB24,
	VIDEO_MEDIASUBTYPE_RGB565,
	VIDEO_MEDIASUBTYPE_RGB555,
} VIDEO_MEDIASUBTYPE;

#endif



#define FF263_RECV_QUEUE_MAX_NODES	(25)


void FF263RecvStart(int nBasePort, DWORD dwDestIp/* Network byte order */, int nDestPort, UDPSOCKET udpSock, SOCKET fhandle, SOCKET_TYPE type);

void FF263RecvStop(void);

void FF263RecvSetVflip(void);

//
// Return values:
// -1: Error
//  0: No data
//  n: Length of data
//
int FF263RecvGetData(VIDEO_MEDIASUBTYPE videoType, unsigned char *Buff, int bufSize, int *pWidth, int *pHeight, int *pTimePerFrame/* ms */, BOOL *pFull, DWORD *pTimeStamp);


#endif /* _FF263RECV_H */
