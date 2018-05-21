#include "base_reactor_instance.h"
#include "core/core_packet.h"
#include "rudp_interface.h"
#include "rudp_socket.h"
#include "direct_udp.h"
#include "user_connection.h"

#define MAX_UDP_PACKET 1500


UserConnection::UserConnection()
{
	state_ = RUDP_CONN_IDLE;
	broken_ = false;
	listen_ = false;
	accepted_rudp_ = INVALID_RUDP_HANDLE;
#ifdef WIN32_xxx
	readMutex_.acquire();//在REACTOR线程中获取
	writeMutex_.acquire();//在REACTOR线程中获取
#else
	timer_id_ = 0;
	timer_id_ = REACTOR_INSTANCE()->set_timer(this, NULL, 10);
#endif
	need_read_ = false;
	need_write_ = false;
	can_read_ = false;
	can_write_ = false;
	bind_udp_ = false;
	slave_ = false;
	bin_strm_.resize(MAX_UDP_PACKET);
	direct_fn_ = NULL;
	direct_fn2_ = NULL;
	learn_addr_fn_ = NULL;
	learn_addr_fn2_ = NULL;
}

UserConnection::~UserConnection()
{
	if (state_ != RUDP_CONN_CLOSE) {
		this->close();
	}
}

int32_t UserConnection::open()
{
	if(rudp_sock_.open() != 0)
	{
		return -1;
	}

	rudp_set_user_ptr(rudp_sock_.get_handler(), this);
	rudp_setoption(rudp_sock_.get_handler(), RUDP_NAGLE, 0);

	//绑定一个事件器
	RUDP()->bind_event_handle(rudp_sock_.get_handler(), this);

	state_ = RUDP_CONN_IDLE;
	broken_ = false;
	listen_ = false;
	bind_udp_ = false;
	slave_ = false;
	return rudp_sock_.get_handler();
}

int32_t UserConnection::listen()
{
	RUDP()->attach_listener(this);
	state_ = RUDP_CONN_LISTENING;
	listen_ = true;
	accepted_rudp_ = INVALID_RUDP_HANDLE;
	return 0;
}

int32_t UserConnection::accept()
{
	if (false == listen_) {
		return INVALID_RUDP_HANDLE;
	}
	while (true)
	{
		usleep(100000);
		if (RUDP_CONN_CLOSE == state_) {
			return INVALID_RUDP_HANDLE;
		}
		if (RUDP_CONN_CONNECTED == state_) {
			return accepted_rudp_;
		}
	}
	return INVALID_RUDP_HANDLE;
}

int32_t UserConnection::connect(const Inet_Addr& dst_addr)
{
	if (state_ != RUDP_CONN_IDLE) {
		return -1;
	}
	
	state_ = RUDP_CONN_CONNECTING;
	
	if(rudp_sock_.connect(dst_addr) != 0) {
		state_ = RUDP_CONN_IDLE;
		return -1;
	}
	
	for (int i = 0; i < 150; i++)
	{
		if (RUDP_CONN_CONNECTED == state_) {
			return 0;
		}
		usleep(100000);
	}
	state_ = RUDP_CONN_IDLE;
	return -1;
}

void UserConnection::close()
{
	state_ = RUDP_CONN_CLOSE;
	broken_ = true;
	direct_fn_ = NULL;
	direct_fn2_ = NULL;
	learn_addr_fn_ = NULL;
	learn_addr_fn2_ = NULL;

	timer_id_ = REACTOR_INSTANCE()->set_timer(this, NULL, 10);

	usleep(300000);
	cout << "close() OK!" << endl;////Debug
}

int32_t UserConnection::select_recv(uint64_t mili_secs)
{
	if (broken_) {
		return -1;
	}
	for (uint64_t i = 0; i <= (mili_secs/22); i++ )
	{
		if (RUDP_CONN_CLOSE == state_) {
			return -1;
		}
		if (broken_) {
			return -1;
		}
		if (can_read_) {
			return 1;
		}
		usleep(22000);
	}
	return 0;
}

int32_t UserConnection::recv(uint8_t* buf, uint32_t buf_size)
{
	int32_t ret;
	can_read_ = false;
	while (true)
	{
		if (RUDP_CONN_CONNECTED != state_) {
			return -1;
		}
		if (broken_) {
			return -1;
		}
		need_read_ = true;
		readMutex_.acquire();
		ret = rudp_sock_.recv(buf, buf_size);
		readMutex_.release();
		need_read_ = false;
		if (ret > 0) {
			break;
		}
		if (ret < 0) {
			return -1;
		}
		usleep(55000);
	}
	return ret;
}

int32_t UserConnection::select_send(uint64_t mili_secs)
{
	if (broken_) {
		return -1;
	}
	for (uint64_t i = 0; i <= (mili_secs/22); i++ )
	{
		if (RUDP_CONN_CLOSE == state_) {
			return -1;
		}
		if (broken_) {
			return -1;
		}
		if (can_write_) {
			return 1;
		}
		usleep(22000);
	}
	return 0;
}

int32_t UserConnection::send(const uint8_t* buf, uint32_t buf_size)
{
	int32_t ret;
	can_write_ = false;
	while (true)
	{
		if (RUDP_CONN_CONNECTED != state_) {
			return -1;
		}
		if (broken_) {
			return -1;
		}
		need_write_ = true;
		writeMutex_.acquire();
		ret = rudp_sock_.send(buf, buf_size);
		writeMutex_.release();
		need_write_ = false;
		if (ret > 0) {
			break;
		}
		if (ret < 0) {
			return -1;
		}
		usleep(55000);
	}
	return ret;
}

int32_t UserConnection::select_accept(uint64_t mili_secs)
{
	if (false == listen_) {
		return -1;
	}
	for (uint64_t i = 0; i <= (mili_secs/100); i++ )
	{
		if (RUDP_CONN_CLOSE == state_) {
			return -1;
		}
		if (RUDP_CONN_CONNECTED == state_) {
			return 1;
		}
		usleep(100000);
	}
	return 0;
}

int32_t UserConnection::rudp_accept_event(int32_t rudp_id)
{
	if (RUDP_CONN_LISTENING != state_) {
		return -1;
	}
	
	UserConnection *conn = new UserConnection();
	rudp_set_user_ptr(rudp_id, conn);
	rudp_setoption(rudp_id, RUDP_NAGLE, 0);
	
	RUDPStream& sock = conn->get_rudp_stream();
	sock.set_handler(rudp_id);
	
	int buffer_size = RUDP_SEND_BUFFER;
	sock.set_option(RUDP_SEND_BUFF_SIZE, buffer_size);
	
	Inet_Addr addr(0, 0);
	if (sock.get_local_addr(addr) < 0) {
		conn->set_state(RUDP_CONN_CLOSE);
		delete conn;
		return -1;
	}
	
	if (bind_udp_)
	{
		if (!slave_)
		{
			REACTOR_INSTANCE()->delete_handler(this);//(bind_udp_ && listen_ && accepted_rudp_ != INVALID_HANDLER)条件下在udp_close()时不会执行delete_handler
			if (conn->udp_open(sock_dgram_.get_handler()) < 0) {
				conn->set_state(RUDP_CONN_CLOSE);
				delete conn;
				return -1;
			}
		}
		else {
			conn->set_slave(true);
			if (conn->udp_open(INVALID_HANDLER) < 0) {
				conn->set_state(RUDP_CONN_CLOSE);
				delete conn;
				return -1;
			}
		}
	}
	else
	{
		if (conn->udp_open(addr) < 0) {
			conn->set_state(RUDP_CONN_CLOSE);
			delete conn;
			return -1;
		}
	}
	
	if (!slave_) {
		RUDP()->unattach(&adapter_);
	}
	sock.bind(addr);
	
	//关联HANDLER
	RUDP()->bind_event_handle(sock.get_handler(), conn);
	
	accepted_rudp_ = rudp_id;
	state_ = RUDP_CONN_CONNECTED;
	conn->set_state(RUDP_CONN_CONNECTED);
	conn->set_broken(false);
	conn->set_listen(false);
	return 0;
}

int32_t UserConnection::rudp_input_event(int32_t rudp_id)
{
	can_read_ = true;
	if (need_read_)
	{
		readMutex_.release();
		usleep(2000);
		readMutex_.acquire();
	}
	return 0;
}

int32_t UserConnection::rudp_output_event(int32_t rudp_id)
{
	if(state_ == RUDP_CONN_CONNECTING)
	{
		state_ = RUDP_CONN_CONNECTED;
	}
	can_write_ = true;
	if (need_write_)
	{
		writeMutex_.release();
		usleep(2000);
		writeMutex_.acquire();
	}
	return 0;
}

int32_t UserConnection::rudp_close_event(int32_t rudp_id)
{
	cout << "rudp_close_event(" << rudp_id << ")..." << endl;////Debug
	broken_ = true;
	writeMutex_.release();
	readMutex_.release();
	usleep(2000);
	readMutex_.acquire();
	writeMutex_.acquire();
	return 0;
}

int32_t UserConnection::rudp_exception_event(int32_t rudp_id)
{
	cout << "rudp_exception_event(" << rudp_id << ")..." << endl;////Debug
	broken_ = true;
	writeMutex_.release();
	readMutex_.release();
	usleep(2000);
	readMutex_.acquire();
	writeMutex_.acquire();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

BASE_HANDLER UserConnection::get_udp_handle() const
{
	return sock_dgram_.get_handler();
}

bool UserConnection::is_udp_open() const 
{
	return (get_udp_handle() != INVALID_HANDLER);
}

int32_t UserConnection::udp_open(const Inet_Addr& local_addr)
{
	int32_t ret = sock_dgram_.open(local_addr, true);
	if(ret == 0)
	{
		//设置缓冲区大小
		int32_t buf_size = 2 * 1024 * 1024; //2M
		sock_dgram_.set_option(SOL_SOCKET, SO_RCVBUF, (void *)&buf_size, sizeof(int32_t));
		sock_dgram_.set_option(SOL_SOCKET, SO_SNDBUF, (void *)&buf_size, sizeof(int32_t));

		//本地UDP桥接口
		adapter_.set_user_connection(this);
		adapter_.set_title(0);
		adapter_.set_local_addr(local_addr);
		RUDP()->attach(&adapter_);

		return REACTOR_INSTANCE()->register_handler(this, MASK_READ);
	}
	else
	{
		return -1;
	}
}

int32_t UserConnection::udp_open(BASE_HANDLER handler)
{
	sock_dgram_.set_handler(handler);
	bind_udp_ = true;

	if (slave_) {
		return 0;
	}

#ifdef WIN32 //解决WINSOCK2 的UDP端口ICMP的问题
	int32_t byte_retruned = 0;
	bool new_be = false;  

	int32_t status = WSAIoctl(handler, SIO_UDP_CONNRESET,  
		&new_be, sizeof(new_be), NULL, 0, (LPDWORD)&byte_retruned, NULL, NULL);  
#endif

	//设置异步模式
	set_socket_nonblocking(handler);


	//64K buffer,设置收发缓冲区
	int32_t buf_size = 64 * 1024;
	sock_dgram_.set_option(SOL_SOCKET, SO_RCVBUF, (void *)&buf_size, sizeof(int32_t));
	sock_dgram_.set_option(SOL_SOCKET, SO_SNDBUF, (void *)&buf_size, sizeof(int32_t));


	{
		//设置缓冲区大小
		int32_t buf_size = 2 * 1024 * 1024; //2M
		sock_dgram_.set_option(SOL_SOCKET, SO_RCVBUF, (void *)&buf_size, sizeof(int32_t));
		sock_dgram_.set_option(SOL_SOCKET, SO_SNDBUF, (void *)&buf_size, sizeof(int32_t));

		Inet_Addr local_addr(0, 0);
		sock_dgram_.get_local_addr(local_addr);

		//本地UDP桥接口
		adapter_.set_user_connection(this);
		adapter_.set_title(0);
		adapter_.set_local_addr(local_addr);
		RUDP()->attach(&adapter_);

		return REACTOR_INSTANCE()->register_handler(this, MASK_READ);
	}
}

int32_t UserConnection::udp_close()
{
	if (!(bind_udp_ && listen_ && accepted_rudp_ != INVALID_HANDLER) && !slave_)
	{
		REACTOR_INSTANCE()->delete_handler(this);
	}
	if (!bind_udp_ && !slave_)
	{
		sock_dgram_.close();
	}

	//注销桥接口
	if (!slave_)
	{
		RUDP()->unattach(&adapter_);
	}

	return 0;
}

int32_t UserConnection::udp_send(BinStream& bin_strm, const Inet_Addr& remote_addr)
{
	//if(rand() % 50 == 1)
	//	return 0;

	int32_t rc = sock_dgram_.send(bin_strm.get_rptr(), bin_strm.data_size(), remote_addr);
	if(rc < 0)
	{
		usleep(10000);
		if (RUDP_CONN_CLOSE == state_) {
			return rc;
		}

		if(XEAGAIN == error_no() || XEINPROGRESS == error_no()) //插入一个写事件，防止SOCKET异常
		{
			REACTOR_INSTANCE()->register_handler(this, MASK_WRITE);

			return 0;
		}
		else
		{
			return -1;
		}
	}

	return rc;
}

int32_t UserConnection::handle_timeout(const void *act, uint32_t timer_id)
{
	if(timer_id == timer_id_)
	{
	  if (RUDP_CONN_CLOSE == state_)
	  {
		cout << "handle_timeout()...do close..." << endl;////Debug
		
		int32_t rudp_id = rudp_sock_.get_handler();
		rudp_sock_.close();////这句必须在前面，触发rudp_close_event()
		RUDP()->bind_event_handle(rudp_id, NULL);
		
		if (listen_) {
			RUDP()->attach_listener(NULL);
		}
		
		usleep(5000);//Send FIN...
		udp_close();
		
		cout << "handle_timeout()...close done!" << endl;////Debug
	  }
#ifndef WIN32_xxx
	  else {
		readMutex_.acquire();
		writeMutex_.acquire();
	  }
#endif
	}
	
	return 0;
}

int32_t UserConnection::handle_input(BASE_HANDLER handle)
{
	Inet_Addr remote_addr;
	while(true)
	{
		bin_strm_.rewind(true);
		int32_t rc = sock_dgram_.recv(bin_strm_.get_wptr(), MAX_UDP_PACKET, remote_addr);
		if(rc > 0)
		{
			bool is_DMsg = false;//是否局域网搜索包？
			
			bin_strm_.set_used_size(rc);
			uint8_t packet_type = 0;
			bin_strm_ >> packet_type;

			//提送到ADAPTER INTERFACE
			if(packet_type == 0)
				adapter_.on_data(bin_strm_, remote_addr);
			else if (packet_type == DIRECT_UDP_HEAD_VAL) {
				
				unsigned char bMagic[2] = {0x69, 0x69};//#define DMSG_MAGIC_VALUE	0x6969
				if (0 == memcmp((char *)(bin_strm_.get_data_ptr() + DIRECT_UDP_HEAD_LEN), bMagic, 2)) {
					is_DMsg = true;
				}
				
				if (NULL != direct_fn2_) {
					direct_fn2_(direct_ctx2_, handle, (char *)(bin_strm_.get_data_ptr() + DIRECT_UDP_HEAD_LEN), bin_strm_.data_size() - DIRECT_UDP_HEAD_LEN, (sockaddr*)(remote_addr.get_addr()), sizeof(sockaddr_in));
				}
				else if (NULL != direct_fn_) {
					direct_fn_(handle, (char *)(bin_strm_.get_data_ptr() + DIRECT_UDP_HEAD_LEN), bin_strm_.data_size() - DIRECT_UDP_HEAD_LEN, (sockaddr*)(remote_addr.get_addr()), sizeof(sockaddr_in));
				}
			}
			
			
			Inet_Addr orig_remote_addr;
			if (false == is_DMsg && 0 == rudp_get_peer_addr(rudp_sock_.get_handler(), orig_remote_addr) && orig_remote_addr != remote_addr)
			{
				if (NULL != learn_addr_fn_) {
					learn_addr_fn_(learn_addr_ctx_, (sockaddr*)(remote_addr.get_addr()), sizeof(sockaddr_in));
				}
				if (NULL != learn_addr_fn2_) {
					learn_addr_fn2_(learn_addr_ctx2_, (sockaddr*)(remote_addr.get_addr()), sizeof(sockaddr_in));
				}
			}
			//rudp_socket.cpp中的 RUDPSocket::process() 支持地址学习！！！
		}
		else
		{
			if(XEAGAIN == error_no())
			{
				return 0;
			}
			else
			{
				return -1;
			}
		}
		if (listen_ && state_ == RUDP_CONN_CONNECTED) {
			break;
		}
	}
	return 0;
}

int32_t UserConnection::handle_output(BASE_HANDLER handle)
{
	REACTOR_INSTANCE()->remove_handler(this, MASK_WRITE);
	return 0;
}

int32_t UserConnection::handle_close(BASE_HANDLER handle, ReactorMask close_mask)
{
	return 0;
}

int32_t UserConnection::handle_exception(BASE_HANDLER handle)
{
	return 0;
}
