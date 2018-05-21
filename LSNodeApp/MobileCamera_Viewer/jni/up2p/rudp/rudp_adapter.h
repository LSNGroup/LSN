/*************************************************************************************
*filename:	rudp_adapter.h
*
*to do:		RUDP�ķ����Žӿڣ�һ��������UDP�����ݷ��Ͷ���
*Create on: 2013-04
*Author:	zerok
*check list:
*************************************************************************************/

#ifndef __RUDP_ADAPTER_H_
#define __RUDP_ADAPTER_H_

#include "revolver/base_typedef.h"
#include "revolver/base_namespace.h"
#include "revolver/base_bin_stream.h"
#include "revolver/base_inet_addr.h"

//RUDP�������ӿ�
BASE_NAMESPACE_BEGIN_DECL

class IRUDPAdapter
{
public:
	IRUDPAdapter(){index_ = 0xff;};
	virtual ~IRUDPAdapter() {};

	uint8_t				get_index() const {return index_;};
	void				set_index(uint8_t index) {index_ = index;};

	uint8_t				get_title() const {return title_;};
	void				set_title(uint8_t title) {title_ = title;};

	const Inet_Addr&	get_local_addr() const {return local_addr_;};
	void				set_local_addr(const Inet_Addr& addr) {local_addr_ = addr;};

	void				on_data(BinStream& strm, const Inet_Addr& remote_addr);
	virtual void		send(BinStream& strm, const Inet_Addr& remote_addr) = 0;

protected:
	uint8_t				index_;
	uint8_t				title_;
	Inet_Addr			local_addr_;
};

BASE_NAMESPACE_END_DECL

#endif
/************************************************************************************/

