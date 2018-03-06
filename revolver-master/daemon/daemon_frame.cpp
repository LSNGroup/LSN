#include "daemon_frame.h"
#include "core_daemon_msg.h"
#include "core_local_info.h"
#include "core_server_type.h"
#include "core_message_processor.h"

#define DAEMON_SERVER_CNC_IP	"daemon.5211game.com"
#define DAEMON_SERVER_TEL_IP	"daemon.5211game.com"
#define DAEMON_SERVER_PORT		7600

CDaemonFrame::CDaemonFrame() : db_(&daemon_)
{

}

CDaemonFrame::~CDaemonFrame()
{

}

void CDaemonFrame::on_init()
{
	//����DAEMON�ڵ��ȫ����Ϣ
	SERVER_ID = 1;
	SERVER_TYPE = eDaemon_Server;
	//Ĭ������Ϊ˫��
	SERVER_NET_TYPE = 0x00ff;

	SERVER_PORT = DAEMON_SERVER_PORT;
	TEL_IPADDR.set_ip(DAEMON_SERVER_CNC_IP); //����ֱ��������
	TEL_IPADDR.set_port(SERVER_PORT);
	CNC_IPADDR.set_ip(DAEMON_SERVER_TEL_IP);
	CNC_IPADDR.set_port(SERVER_PORT);

	//����һ��TCP������
	create_tcp_listener();
	create_udp();
	create_dc_client();

	//����DAEMON��Ϣ����ӳ��
	INIT_MSG_PROCESSOR1(&daemon_);

	LOAD_MESSAGEMAP_DECL(DAEMON);
}

void CDaemonFrame::on_destroy()
{

}

void CDaemonFrame::on_start()
{
	//ֱ�Ӱ�һ������˿�
	bind_port(SERVER_PORT);

	daemon_.init(&db_);
}

void CDaemonFrame::on_stop()
{
	daemon_.destroy();
}



