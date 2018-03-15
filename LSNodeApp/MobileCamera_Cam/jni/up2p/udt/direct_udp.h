#ifndef __DIRECT_UDP_H__
#define __DIRECT_UDP_H__

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#endif


#define DIRECT_UDP_RECV  1  /* Changed by GaoHua */


#ifdef WIN32
   #ifndef __MINGW__
      typedef SOCKET UDPSOCKET;
   #else
      typedef int UDPSOCKET;
   #endif
#else
   typedef int UDPSOCKET;
#endif


#define DIRECT_UDP_HEAD_LEN		4
#define DIRECT_UDP_HEAD_VAL		85/* 'U' */

typedef void (*DIRECT_UDP_RECV_FN)(UDPSOCKET sock, char *data, int len, sockaddr* his_addr, int addr_len);
typedef void (*DIRECT_UDP_RECV_FN2)(void *ctx2, UDPSOCKET sock, char *data, int len, sockaddr* his_addr, int addr_len);


#endif /* __DIRECT_UDP_H__ */
