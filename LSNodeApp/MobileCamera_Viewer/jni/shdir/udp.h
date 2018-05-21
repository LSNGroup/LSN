#ifndef udp_h
#define udp_h


#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>


typedef long long Int64;

typedef int Socket;
typedef int SOCKET;

#define closesocket(s) close(s)

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif


#include <errno.h>
inline int getErrno() { return errno; }



/// Open a UDP socket to receive on the given port - if port is 0, pick a a
/// port, if interfaceIp!=0 then use ONLY the interface specified instead of
/// all of them  
Socket
openPort( unsigned short port, unsigned int interfaceIp,
          bool verbose);


/// recive a UDP message 
bool 
getMessage( Socket fd, char* buf, int* len,
            unsigned int* srcIp, unsigned short* srcPort,
            bool verbose);


/// send a UDP message 
bool 
sendMessage( Socket fd, char* msg, int len, 
             unsigned int dstIp, unsigned short dstPort,
             bool verbose);


#endif
