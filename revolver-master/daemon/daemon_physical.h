#ifndef __DAEMON_PHYSICAL_H
#define __DAEMON_PHYSICAL_H

#include "base_typedef.h"
#include "base_namespace.h"
#include "core_connection.h"

using namespace BASE;

class IPhysicalServer
{
public:
	IPhysicalServer() {};
	virtual ~IPhysicalServer() {};

	//��ȡ�������������Ϣ
	virtual void get_physical_server(uint32_t sid, uint8_t stype, const string& server_ip, CConnection* connection) = 0;
	virtual void on_connection_disconnected(CConnection* connection) = 0;
};

#endif
