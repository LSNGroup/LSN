#ifndef __USER_CONNECTION_H_
#define __USER_CONNECTION_H_

#include "base_event_handler.h"
#include "base_bin_stream.h"
#include "base_thread_mutex.h"
#include "base_sock_dgram.h"
#include "rudp_stream.h"
#include "rudp_interface.h"
#include "user_adapter.h"

#include "direct_udp.h"
#include "learn_addr.h"


using namespace BASE;


enum RUDPConnState
{
	RUDP_CONN_IDLE,			//平静状态,有可能是未连接
	RUDP_CONN_LISTENING,	//侦听中。。。
	RUDP_CONN_CONNECTING,	//连接中。。。
	RUDP_CONN_CONNECTED,	//已连接状态
	RUDP_CONN_CLOSE,		//关闭
};


class UserConnection : public CEventHandler,
					   public RUDPEventHandler
{
public:
	UserConnection();
	virtual ~UserConnection();

public:
	static UserConnection*	get_the_connection(int32_t rudp_id)
	{
		UserConnection *conn = (UserConnection *)rudp_get_user_ptr(rudp_id);
		return conn;
	};
	RUDPStream&		get_rudp_stream() {return rudp_sock_;};

	int32_t			rudp_accept_event(int32_t rudp_id);
	int32_t			rudp_input_event(int32_t rudp_id);
	int32_t			rudp_output_event(int32_t rudp_id);
	int32_t			rudp_close_event(int32_t rudp_id);
	int32_t			rudp_exception_event(int32_t rudp_id);

public:
	int32_t			open();
	int32_t			listen();
	int32_t			accept();
	int32_t			connect(const Inet_Addr& dst_addr);
	void			close();
	int32_t			recv(uint8_t* buf, uint32_t buf_size);
	int32_t			send(const uint8_t* buf, uint32_t buf_size);
	int32_t			select_recv(uint64_t mili_secs);
	int32_t			select_send(uint64_t mili_secs);
	int32_t			select_accept(uint64_t mili_secs);

	void			set_state(uint8_t state) {state_ = state;};
	uint8_t			get_state() const {return state_;};

	void			set_broken(bool broken) {broken_ = broken;};
	bool			get_broken() const {return broken_;};

	void			set_listen(bool listen) {listen_ = listen;};
	bool			get_listen() const {return listen_;};

	void			set_slave(bool slave) {slave_ = slave;};
	bool			get_slave() const {return slave_;};

	RUDPStream		rudp_sock_;

protected:
	volatile  uint8_t		state_;
	volatile  bool			broken_;
	volatile  bool			listen_;
	int32_t			accepted_rudp_;
	BaseThreadMutex	readMutex_;
	BaseThreadMutex	writeMutex_;
	volatile  bool		need_read_;
	volatile  bool		need_write_;
	volatile  bool		can_read_;
	volatile  bool		can_write_;
	bool			bind_udp_;
	bool			slave_;

public:
	BASE_HANDLER	get_handle() const{return get_udp_handle();};
	void			set_handle(BASE_HANDLER handle){sock_dgram_.set_handler(handle);};

	BASE_HANDLER	get_udp_handle() const;
	bool			is_udp_open() const;
	int32_t			udp_open(const Inet_Addr& local_addr);
	int32_t			udp_open(BASE_HANDLER handler);
	int32_t			udp_close();
	int32_t			udp_send(BinStream& bin_strm, const Inet_Addr& remote_addr);
	void			set_direct_udp_fn(DIRECT_UDP_RECV_FN fn){if (!slave_) direct_fn_ = fn;};
	void			set_direct_udp_fn2(DIRECT_UDP_RECV_FN2 fn2, void *ctx2){if (!slave_) {direct_fn2_ = fn2; direct_ctx2_ = ctx2;}};
	void			set_learn_addr_fn(LEARN_ADDR_FN fn, void *ctx){if (!slave_) {learn_addr_fn_ = fn; learn_addr_ctx_ = ctx;}};
	void			set_learn_addr_fn2(LEARN_ADDR_FN2 fn2, void *ctx2){if (!slave_) {learn_addr_fn2_ = fn2; learn_addr_ctx2_ = ctx2;}};

	virtual int32_t	handle_timeout(const void *act, uint32_t timer_id);
	virtual int32_t	handle_input(BASE_HANDLER handle);
	virtual int32_t handle_output(BASE_HANDLER handle);
	virtual int32_t handle_close(BASE_HANDLER handle, ReactorMask close_mask);
	virtual int32_t handle_exception(BASE_HANDLER handle);

protected:
	uint32_t		timer_id_;
	BinStream		bin_strm_;
	CSockDgram		sock_dgram_;
	UserAdapter		adapter_;
	DIRECT_UDP_RECV_FN		direct_fn_;
	DIRECT_UDP_RECV_FN2		direct_fn2_;
	void	*direct_ctx2_;
	LEARN_ADDR_FN		learn_addr_fn_;
	void	*learn_addr_ctx_;
	LEARN_ADDR_FN2		learn_addr_fn2_;
	void	*learn_addr_ctx2_;
};

#endif /* __USER_CONNECTION_H_ */
