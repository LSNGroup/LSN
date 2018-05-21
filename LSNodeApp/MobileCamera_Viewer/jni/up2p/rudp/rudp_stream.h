/*************************************************************************************
*filename:	rudp_stream.h
*
*to do:		����һ��RUDP�������ƶ���
*Create on: 2013-05
*Author:	zerok
*check list:
*************************************************************************************/
#ifndef __RUDP_STREAM_H_
#define __RUDP_STREAM_H_

#include "revolver/base_namespace.h"
#include "revolver/base_inet_addr.h"

#define RUDP_SEND_BUFFER (2 * 1024 * 1024)

BASE_NAMESPACE_BEGIN_DECL
class RUDPStream 
{
public:
	RUDPStream();
	virtual ~RUDPStream();

	int32_t			get_handler() const;
	void			set_handler(int32_t handle);

	int32_t			open(const Inet_Addr& local_addr);
	int32_t			open();
	int32_t			bind(const Inet_Addr& local_addr);
	int32_t			close();
	int32_t			force_close();

	int32_t			connect(const Inet_Addr& remote_addr);

	bool			isopen() const;

	//��������
	int32_t			set_option(int32_t op_type, int32_t op_value) const;

	//��ȡSOCKET�󶨵ĵ�ַ
	int32_t			get_local_addr (Inet_Addr& local_addr) const;
	int32_t			get_peer_addr(Inet_Addr& remote_addr) const;

	int32_t			recv(uint8_t* buf, uint32_t buf_size);
	int32_t			send(const uint8_t* buf, uint32_t buf_size);

protected:
	int32_t			handler_;
};

BASE_NAMESPACE_END_DECL
#endif
/************************************************************************************/

