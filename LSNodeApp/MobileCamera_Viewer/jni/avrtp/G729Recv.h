#ifndef _G729RECV_H
#define _G729RECV_H


#include "FakeRtpRecv.h"



#define AUDIO_GET_BUFFER_SIZE		(640)
#define G729_RECV_QUEUE_MAX_NODES	(80)




void AudioRecvStart(int nBasePort, DWORD dwDestIp/* Network byte order */, int nDestPort, UDPSOCKET udpSock, SOCKET fhandle, SOCKET_TYPE type);

void AudioRecvStop(void);


//
// Return values:
// -1: Error
//  0: No data
//  n: Length of data
//
int AudioRecvGetData(unsigned char *Buff, int bufSize, BOOL *pFull, DWORD *pTimeStamp);



#endif /* _G729RECV_H */
