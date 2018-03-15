#ifndef __LEARN_ADDR_H__
#define __LEARN_ADDR_H__

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#endif


typedef void (*LEARN_ADDR_FN)(void *ctx, sockaddr* his_addr, int addr_len);
typedef void (*LEARN_ADDR_FN2)(void *ctx2, sockaddr* his_addr, int addr_len);


#endif /* __LEARN_ADDR_H__ */
