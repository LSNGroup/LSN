#include "core_server_type.h"
#include "sample_frame.h"
#include "core_local_info.h"
#include "core_message_processor.h"
#include "core_message_map_decl.h"
#include "sample_msg.h"

using namespace SAMPLE_SERVER;

SampleFrame::SampleFrame()
{

}

SampleFrame::~SampleFrame()
{

}

void SampleFrame::on_init()
{
	//���ýڵ�����
	SERVER_TYPE = eSample_Server;

	//����daemon client���
	create_daemon_client(&sample_server_, &config_);

	//����TCP��������
	create_tcp_listener();

	//���ӷ���������״̬ͨ��ӿ�
	attach_server_notify(&sample_server_);

	//���ù�������, add_focus�������Ǳ�������Ҫ��֪�ķ�����������˴���Է����������͹رգ�����ͨ���ض��¼�֪ͨ������������Կ�SampleServer��ʵ��
	//daemon_client_->add_focus(eData_Center);

	//������Ϣ������
	INIT_MSG_PROCESSOR1(&sample_server_);

	//����Ҫ�������ϢȺ��
	LOAD_MESSAGEMAP_DECL(SAMPLE_MSG);
	
	//��ʼ����������
	sample_server_.init();

	//����daemon client
	daemon_client_->init();
}

void SampleFrame::on_destroy()
{
	//������������
	sample_server_.destroy();
}

void SampleFrame::on_start()
{

}

void SampleFrame::on_stop()
{

}
