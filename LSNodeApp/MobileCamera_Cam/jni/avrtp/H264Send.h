#ifndef _H264SEND_H
#define _H264SEND_H



void VideoSendStart(int nBasePort, DWORD dwDestIp/* Network byte order */, int nDestPort, UDPSOCKET udpSock, SOCKET fhandle, SOCKET_TYPE type);

void VideoSendStop(void);

void VideoSendSetReliable(BOOL bValue);

int VideoSendSetMediaType(int width, int height, int fps);

//
// Return values:
// -1: Error
//  0: OK
//
int VideoSendH264Data(DWORD rtptimestamp, const unsigned char *pSourceBuffer, int lSourceSize);


//
// Return values:
// -1: Error
//  0: OK
//
int VideoSendH263Data(DWORD rtptimestamp, const unsigned char *pSourceBuffer, int lSourceSize);



#endif /* _H264SEND_H */
