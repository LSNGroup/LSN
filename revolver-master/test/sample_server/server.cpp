#include "sample_frame.h"

int main(int argc, const char* argv[])
{
	//����һ��������
	CREATE_SAMPLE_FRAME();
	
	//��ʼ�����
	SAMPLE_FRAME()->init();

	//��������
	SAMPLE_FRAME()->start();

	//���̵߳ȴ�����WINDOWS�°�e���˳�����LINUX��kill -41 ����ID �˳�
	SAMPLE_FRAME()->frame_run();

	//��������
	SAMPLE_FRAME()->destroy();

	//���ٷ����ܶ���
	DESTROY_SAMPLE_FRAME();
}
