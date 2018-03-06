#ifndef __UDT_H__
#define __UDT_H__


#include <fstream>
#include <set>
#include <string>
#include <vector>

#include "direct_udp.h"
#include "learn_addr.h"


////////////////////////////////////////////////////////////////////////////////

//if compiling on VC6.0 or pre-WindowsXP systems
//use -DLEGACY_WIN32

//if compiling with MinGW, it only works on XP or above
//use -D_WIN32_WINNT=0x0501


#ifdef WIN32
   #ifndef __MINGW__
      #ifdef UDT_EXPORTS
         #define UDT_API __declspec(dllexport)
      #else
         #define UDT_API __declspec(dllimport)
      #endif
   #else
      #define UDT_API
   #endif
#else
   #define UDT_API
#endif


typedef int UDTSOCKET;

typedef std::set<UDTSOCKET> ud_set;
#define UD_CLR(u, uset) ((uset)->erase(u))
#define UD_ISSET(u, uset) ((uset)->find(u) != (uset)->end())
#define UD_SET(u, uset) ((uset)->insert(u))
#define UD_ZERO(uset) ((uset)->clear())

////////////////////////////////////////////////////////////////////////////////


namespace UDT
{

typedef ud_set UDSET;

UDT_API extern const UDTSOCKET INVALID_SOCK;
#undef ERROR
UDT_API extern const int ERROR;

UDT_API int startup();
UDT_API int cleanup();
UDT_API UDTSOCKET socket(int af, int type, int protocol);
UDT_API int bind(UDTSOCKET u, const struct sockaddr* name, int namelen);
UDT_API int bind(UDTSOCKET u, UDPSOCKET udpsock);
UDT_API int bind2(UDTSOCKET u, UDPSOCKET udpsock);//Slave udt socket, 其生命期必须基于一个master udt socket，且不能进行 Direct Udp 操作。
UDT_API int listen(UDTSOCKET u, int backlog);
UDT_API UDTSOCKET accept(UDTSOCKET u, struct sockaddr* addr, int* addrlen);
UDT_API int connect(UDTSOCKET u, const struct sockaddr* name, int namelen);
UDT_API int close(UDTSOCKET u);
UDT_API int getpeername(UDTSOCKET u, struct sockaddr* name, int* namelen);
UDT_API int getsockname(UDTSOCKET u, struct sockaddr* name, int* namelen);
//UDT_API int getsockopt(UDTSOCKET u, int level, SOCKOPT optname, void* optval, int* optlen);
//UDT_API int setsockopt(UDTSOCKET u, int level, SOCKOPT optname, const void* optval, int optlen);
UDT_API int send(UDTSOCKET u, const char* buf, int len, int flags);
UDT_API int recv(UDTSOCKET u, char* buf, int len, int flags);
//UDT_API int sendmsg(UDTSOCKET u, const char* buf, int len, int ttl = -1, bool inorder = false);
//UDT_API int recvmsg(UDTSOCKET u, char* buf, int len);
//UDT_API int64_t sendfile(UDTSOCKET u, std::fstream& ifs, int64_t offset, int64_t size, int block = 364000);
//UDT_API int64_t recvfile(UDTSOCKET u, std::fstream& ofs, int64_t offset, int64_t size, int block = 7280000);
UDT_API int select(int nfds, UDTSOCKET readfd, UDTSOCKET writefd, UDTSOCKET exceptfd, const struct timeval* timeout);  /* Changed by GaoHua */
UDT_API int select_read(int nfds, UDTSOCKET readfd, UDTSOCKET writefd, UDTSOCKET exceptfd, const struct timeval* timeout);  /* Changed by GaoHua */
UDT_API int select_write(int nfds, UDTSOCKET readfd, UDTSOCKET writefd, UDTSOCKET exceptfd, const struct timeval* timeout);  /* Changed by GaoHua */
//UDT_API int selectEx(const std::vector<UDTSOCKET>& fds, std::vector<UDTSOCKET>* readfds, std::vector<UDTSOCKET>* writefds, std::vector<UDTSOCKET>* exceptfds, int64_t msTimeOut);
//UDT_API ERRORINFO& getlasterror();
//UDT_API int perfmon(UDTSOCKET u, TRACEINFO* perf, bool clear = true);
UDT_API int register_direct_udp_recv(UDTSOCKET sock, DIRECT_UDP_RECV_FN fn);
UDT_API void unregister_direct_udp_recv(UDTSOCKET sock);
UDT_API int register_direct_udp_recv2(UDTSOCKET sock, DIRECT_UDP_RECV_FN2 fn2, void *ctx2);
UDT_API void unregister_direct_udp_recv2(UDTSOCKET sock);
UDT_API int register_learn_remote_addr(UDTSOCKET sock, LEARN_ADDR_FN fn, void *ctx);
UDT_API void unregister_learn_remote_addr(UDTSOCKET sock);
UDT_API int register_learn_remote_addr2(UDTSOCKET sock, LEARN_ADDR_FN2 fn2, void *ctx2);
UDT_API void unregister_learn_remote_addr2(UDTSOCKET sock);
}//namespace UDT

#endif //__UDT_H__
