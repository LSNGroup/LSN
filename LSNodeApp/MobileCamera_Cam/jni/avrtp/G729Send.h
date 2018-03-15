#ifndef _G729SEND_H_
#define _G729SEND_H_



void AudioSendStart(int nBasePort, DWORD dwDestIp/* Network byte order */, int nDestPort, UDPSOCKET udpSock, SOCKET fhandle, SOCKET_TYPE type);

void AudioSendStop(void);

void AudioSendSetCodec(BYTE bCodec);

void AudioSendSetRedundance(BOOL bValue);

//
// Return values:
// -1: Error
//  0: OK
//
int AudioSendData(DWORD rtptimestamp, const unsigned char *pSourceBuffer, int lSourceSize);



#endif /* _G729SEND_H_ */
