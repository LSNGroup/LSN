#include "core_local_info.h"
#include "sample_server.h"
#include "core_event_message.h"
#include "sample_frame.h"
#include "base_reactor_instance.h"
#include "core_server_type.h"
#include "sample_log_macro.h"
#include "sample_msg.h"

using namespace SAMPLE_SERVER;
//30������һ��
#define HEARTBEAT_TIMER 30000

//������Ϣ������
BEGIN_MSGCALL_MAP(SampleServer)
	ON_MSG_CON_INT(TCP_CONNECT_EVENT, &SampleServer::on_connect_event)
	ON_MSG_CON_INT(TCP_CLOSE_EVENT, &SampleServer::on_disconnect_event)
	ON_MSG_CON_INT(SAMPLE_MSGID, &SampleServer::on_sample_msg)
END_MSGCALL_MAP()

SampleServer::SampleServer() : timer_id_(0)
{

}

SampleServer::~SampleServer()
{
}

void SampleServer::init()
{

}

void SampleServer::destroy()
{
	cancel_timer();
	//��������й�������ʵ���Ķ�ʱ��
	clear_timer_events();
}

void SampleServer::on_register(uint32_t sid, uint8_t stype, uint16_t net_type, const Inet_Addr& tel_addr, const Inet_Addr& cnc_addr)
{
	SAM_INFO("on_register sid = " << sid << ", stype = " << GetServerName(stype) << ", tel addr = " << tel_addr << ", cnc addr = " << cnc_addr);

	//������һ���յ��Ǽǽ�������timer id != 0��ʾ�п����Ǳ�������daemond֮��TCP�Ͽ���������������ӣ�����
	if(timer_id_ == 0)
	{
		//�����ַ��Ϣ
		if(CHECK_CNC_NETTYPE(net_type))
			SERVER_PORT = cnc_addr.get_port();
		else
			SERVER_PORT = tel_addr.get_port();

		SERVER_NET_TYPE = net_type;

		SERVER_ID = sid;
		TEL_IPADDR = tel_addr;
		CNC_IPADDR = cnc_addr;

		//�󶨷���˿ڣ�����tcp udp�����˿�
		SAMPLE_FRAME()->bind_port(SERVER_PORT);

		//���������¼�
		set_timer();
	}
}

void SampleServer::on_add_server(uint32_t sid, uint8_t stype, uint16_t net_type, const Inet_Addr& tel_addr, const Inet_Addr& cnc_addr)
{
	SAM_INFO("on add server, sid = " << sid << ", stype = " << GetServerName(stype));
	//�������֪�ķ���Ԫ�����������ڴ˴������������Ĺ���

}

void SampleServer::on_del_server(uint32_t sid, uint8_t stype)
{
	SAM_WARNING("on_del_server sid = " << sid << ", stype = " << GetServerName(stype));

	//�������֪�ķ�����daemond�Ͽ�
}

void SampleServer::on_server_dead(uint32_t sid, uint8_t stype)
{
	SAM_WARNING("on_server_dead, sid = " << sid << ", stype = " << GetServerName(stype));

	//�������֪�ķ���ֹͣ����

	//TODO:�ڴ˴����֪����ֹͣ���У���������
}

int32_t SampleServer::handle_timeout(const void *act, uint32_t timer_id)
{
	if(timer_id_ == timer_id)
	{
		heartbeat();
	}

	return 0;
}

void SampleServer::heartbeat()
{
	//TODO:���������¼�
}

void SampleServer::release_timer_act(const void* act)
{
	//TODO:�ڴ��ͷ�set_timer���������act����
}

void SampleServer::set_timer()
{
	if(timer_id_ == 0)
		timer_id_ = REACTOR_INSTANCE()->set_timer(this, NULL, HEARTBEAT_TIMER);
}

void SampleServer::cancel_timer()
{
	if(timer_id_ > 0)
	{
		const void* act = NULL;
		REACTOR_INSTANCE()->cancel_timer(timer_id_, &act);

		timer_id_ = 0;
	}
}

int32_t SampleServer::on_connect_event(CBasePacket* packet, uint32_t sid, CConnection* connection)
{
	SAM_INFO("on tcp connect event, sid = " << sid << ", stype = " << GetServerName(connection->get_server_type()));

	//TODO:����TCP�Ѿ����ӵ��¼�
	return 0;
}

int32_t SampleServer::on_disconnect_event(CBasePacket* packet, uint32_t sid, CConnection* connection)
{
	SAM_INFO("on disconnect event, sid = " << sid << ", stype = " << GetServerName(connection->get_server_type()));

	//TODO:����TCP���ӵ㿪���¼�
	return 0;
}

int32_t SampleServer::on_sample_msg(CBasePacket* packet, uint32_t sid, CConnection* connection)
{
	SAM_INFO("on sample msg!!");

	CSamplePacket* body = (CSamplePacket *)packet;

	cout << "msg body = " << *body << endl;

	return 0;
}




