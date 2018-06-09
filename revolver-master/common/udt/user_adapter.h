#ifndef __USER_ADAPTER_H_
#define __USER_ADAPTER_H_

#include "rudp_adapter.h"

class UserConnection;


class UserAdapter : public IRUDPAdapter
{
public:
	UserAdapter();
	virtual ~UserAdapter();

	void				set_user_connection(UserConnection *user_connection) {user_connection_ = user_connection;};

	virtual void		send(BinStream& strm, const Inet_Addr& remote_addr);

private:
	UserConnection *user_connection_;
};

#endif /* __USER_ADAPTER_H_ */
