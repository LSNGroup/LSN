#include "base_reactor_instance.h"
#include "rudp_thread.h"
#include "user_adapter.h"
#include "user_connection.h"
#include "udt.h"


////////////////////////////////////////////////////////////////////////////////

namespace UDT
{

const UDTSOCKET INVALID_SOCK = INVALID_RUDP_HANDLE;
const int ERROR = -1;

int startup()
{
	init_socket();
	
	REACTOR_CREATE();
	REACTOR_INSTANCE()->open_reactor(20000);
	
	init_rudp_socket();
	
	CREATE_RUDP_THREAD();
	RUDP_THREAD()->start();
	
	return 0;
}

int cleanup()
{
	RUDP_THREAD()->terminate();
	
	RUDP()->destroy();
	destroy_rudp_socket();
	
	REACTOR_INSTANCE()->close_reactor();
	
	DESTROY_RUDP_THREAD();
	REACTOR_DESTROY();
	
	destroy_socket();
	
	return 0;
}

UDTSOCKET socket(int af, int type, int protocol)
{
	UserConnection *conn = new UserConnection();
	UDTSOCKET ret = conn->open();
	if (ret == -1) {
		delete conn;
		return INVALID_SOCK;
	}
	return ret;
}

int bind(UDTSOCKET u, const struct sockaddr* name, int namelen)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return -1;
	}
	
	Inet_Addr addr(*(sockaddr_in *)name);
	if (conn->udp_open(addr) < 0) {
		return -1;
	}
	return conn->get_rudp_stream().bind(addr);
}

int bind(UDTSOCKET u, UDPSOCKET udpsock)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return -1;
	}
	
	socklen_t len = sizeof(sockaddr_in);
	Inet_Addr addr;
	sockaddr *local_addr = reinterpret_cast<sockaddr *> (addr.get_addr());
	if(::getsockname(udpsock, local_addr, &len) == -1)
		return -1;
	
	if (conn->udp_open(udpsock) < 0) {
		return -1;
	}
	return conn->get_rudp_stream().bind(addr);
}

int bind2(UDTSOCKET u, UDPSOCKET udpsock)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return -1;
	}
	
	conn->set_slave(true);
	
	socklen_t len = sizeof(sockaddr_in);
	Inet_Addr addr;
	sockaddr *local_addr = reinterpret_cast<sockaddr *> (addr.get_addr());
	if(::getsockname(udpsock, local_addr, &len) == -1)
		return -1;
	
	if (conn->udp_open(INVALID_HANDLER) < 0) {
		return -1;
	}
	return conn->get_rudp_stream().bind(addr);
}

int listen(UDTSOCKET u, int backlog)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return -1;
	}
	
	return conn->listen();
}

UDTSOCKET accept(UDTSOCKET u, struct sockaddr* addr, int* addrlen)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return INVALID_SOCK;
	}
	
	int32_t rudp = conn->accept();///blocking...
	if (rudp != INVALID_SOCK)
	{
		UserConnection* conn2 = UserConnection::get_the_connection(rudp);
		if (conn2 != NULL) {
			Inet_Addr peer_addr(0, 0);
			conn2->get_rudp_stream().get_peer_addr(peer_addr);
			memcpy(addr, peer_addr.get_addr(), sizeof(sockaddr_in));
			*addrlen = sizeof(sockaddr_in);
		}
	}
	return rudp;
}

int connect(UDTSOCKET u, const struct sockaddr* name, int namelen)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return -1;
	}
	
	Inet_Addr dst_addr(*(sockaddr_in *)name);
	return conn->connect(dst_addr);//// blocking n seconds...
}

int close(UDTSOCKET u)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return -1;
	}
	
	conn->close();
	delete conn;
	return 0;
}

int getpeername(UDTSOCKET u, struct sockaddr* name, int* namelen)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return -1;
	}
	
	Inet_Addr peer_addr(0, 0);
	if (conn->get_rudp_stream().get_peer_addr(peer_addr) < 0) {
		return -1;
	}
	memcpy(name, peer_addr.get_addr(), sizeof(sockaddr_in));
	*namelen = sizeof(sockaddr_in);
	return 0;
}

int getsockname(UDTSOCKET u, struct sockaddr* name, int* namelen)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return -1;
	}
	
	Inet_Addr addr(0, 0);
	if (conn->get_rudp_stream().get_local_addr(addr) < 0) {
		return -1;
	}
	memcpy(name, addr.get_addr(), sizeof(sockaddr_in));
	*namelen = sizeof(sockaddr_in);
	return 0;
}

int send(UDTSOCKET u, const char* buf, int len, int flags)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return -1;
	}
	
	return conn->send((const uint8_t*)buf, (uint32_t)len);//// blocking...
}

int recv(UDTSOCKET u, char* buf, int len, int flags)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return -1;
	}
	
	return conn->recv((uint8_t*)buf, (uint32_t)len);//// blocking...
}

int select_read(int nfds, UDTSOCKET readfd, UDTSOCKET writefd, UDTSOCKET exceptfd, const struct timeval* timeout)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)readfd);
	if (conn == NULL) {
		return -1;
	}
	
	uint64_t tv = (timeout->tv_sec * 1000 + timeout->tv_usec / 1000);
	return conn->select_recv(tv);
}

int select_write(int nfds, UDTSOCKET readfd, UDTSOCKET writefd, UDTSOCKET exceptfd, const struct timeval* timeout)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)writefd);
	if (conn == NULL) {
		return -1;
	}
	
	uint64_t tv = (timeout->tv_sec * 1000 + timeout->tv_usec / 1000);
	return conn->select_send(tv);
}

int select(int nfds, UDTSOCKET readfd, UDTSOCKET writefd, UDTSOCKET exceptfd, const struct timeval* timeout)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)readfd);
	if (conn == NULL) {
		return -1;
	}
	
	uint64_t tv = (timeout->tv_sec * 1000 + timeout->tv_usec / 1000);
	return conn->select_accept(tv);
}

int register_direct_udp_recv(UDTSOCKET u, DIRECT_UDP_RECV_FN fn)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return -1;
	}
	conn->set_direct_udp_fn(fn);
	return 0;
}

void unregister_direct_udp_recv(UDTSOCKET u)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return;
	}
	conn->set_direct_udp_fn(NULL);
}

int register_direct_udp_recv2(UDTSOCKET u, DIRECT_UDP_RECV_FN2 fn2, void *ctx2)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return -1;
	}
	conn->set_direct_udp_fn2(fn2, ctx2);
	return 0;
}

void unregister_direct_udp_recv2(UDTSOCKET u)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return;
	}
	conn->set_direct_udp_fn2(NULL, NULL);
}

int register_learn_remote_addr(UDTSOCKET u, LEARN_ADDR_FN fn, void *ctx)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return -1;
	}
	conn->set_learn_addr_fn(fn, ctx);
	return 0;
}

void unregister_learn_remote_addr(UDTSOCKET u)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return;
	}
	conn->set_learn_addr_fn(NULL, NULL);
}

int register_learn_remote_addr2(UDTSOCKET u, LEARN_ADDR_FN2 fn2, void *ctx2)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return -1;
	}
	conn->set_learn_addr_fn2(fn2, ctx2);
	return 0;
}

void unregister_learn_remote_addr2(UDTSOCKET u)
{
	UserConnection* conn = UserConnection::get_the_connection((int32_t)u);
	if (conn == NULL) {
		return;
	}
	conn->set_learn_addr_fn2(NULL, NULL);
}

}// namespace UDT
