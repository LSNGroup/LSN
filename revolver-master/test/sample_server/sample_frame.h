/*************************************************************************************
*filename:	sample_frame.h
*
*to do:		����һ�����׵ķ���ʹ��core���IFrame�ӿڣ�������Ϊ���Ӳο�
*Create on: 2012-05
*Author:	zerok
*check list:
*************************************************************************************/
#ifndef __SAMPLE_FRAME_H
#define __SAMPLE_FRAME_H

#include "core_frame.h"
#include "base_singleton.h"
#include "daemon_config.h"
#include "sample_server.h"

//ʹ��revolver�������ռ�
using namespace BASE;

class SampleFrame : public ICoreFrame
{
public:
	SampleFrame();
	~SampleFrame();

	//�����ʼ���¼�����core���Frameģ�鴥��
	virtual void on_init();
	//�������¼�����core���Frameģ�鴥��
	virtual void on_destroy();
	//����������¼�
	virtual void on_start();
	//�����ֹͣ�¼�
	virtual void on_stop();

protected:
	//��������,����ʵ����Ϣ���䡢�������
	SampleServer	sample_server_;

	//��������ýӿڣ���Ҫ��¼��һ��ȷ����sid, server type����Ϣ����JSON��ʽ���浽�ļ���
	CDaemonConfig	config_;
};

#define CREATE_SAMPLE_FRAME		CSingleton<SampleFrame>::instance
#define SAMPLE_FRAME			CSingleton<SampleFrame>::instance
#define DESTROY_SAMPLE_FRAME	CSingleton<SampleFrame>::destroy

#endif
/************************************************************************************/

