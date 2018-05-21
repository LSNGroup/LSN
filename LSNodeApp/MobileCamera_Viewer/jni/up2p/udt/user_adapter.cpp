#include "user_adapter.h"
#include "user_connection.h"

UserAdapter::UserAdapter()
{
	user_connection_ = NULL;
}

UserAdapter::~UserAdapter()
{

}

void UserAdapter::send(BinStream& strm, const Inet_Addr& remote_addr)
{
	if(NULL != user_connection_ && user_connection_->is_udp_open())
		user_connection_->udp_send(strm, remote_addr);
}
