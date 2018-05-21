#ifndef __RUDP_ACCEPTOR_H_
#define __RUDP_ACCEPTOR_H_

#include "base_typedef.h"
#include "base_namespace.h"
#include "base_bin_stream.h"

#include <vector>
using namespace std;

BASE_NAMESPACE_BEGIN_DECL
class IRUDPAdapter;

typedef vector<IRUDPAdapter *>		RUDPAdapterArray;

//RUDP����������,��Ҫ������������֮����ı��ĺͷ��ͱ��ر���
class RUDPAcceptor
{
public:
	RUDPAcceptor();
	virtual ~RUDPAcceptor();

	int32_t			open();
	void			close();

	void			attach(IRUDPAdapter* adapter);
	IRUDPAdapter*	get_adapter(uint8_t index);
	uint8_t			get_index_by_addr(const Inet_Addr& local_addr);

	void			process_data(IRUDPAdapter* adapter, BinStream& strm, const Inet_Addr& remote_addr);

	void			send(uint8_t index, BinStream& strm, const Inet_Addr& remote_addr);
	
protected:
	RUDPAdapterArray adapters_array_;
};

BASE_NAMESPACE_END_DECL

#endif

