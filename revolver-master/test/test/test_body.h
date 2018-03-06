#ifndef __BASE_TEST_H
#define __BASE_TEST_H

//�����߳���
int test_guard(); 

//����BaseTimerValue
void test_timer_value();

//���Զ����
void test_pool();

//���Զ�ʱ��
void test_time_node();
void test_timer_queue();

//���Ա��Ķ����BinStream
void test_packet();

//����BLOCK BUFFERģ��
void test_block_buffer();

//���Ե���
void test_singleton();

//����Inet_Addr
void test_ip_addr();
//����UDP

int test_udp();

//����TCP
void test_tcp();

//�����߳�
void test_thread();

//������Ϣ����
void test_queue();

//������־ϵͳ
void test_log();
void test_single_log();
//һЩ�����Ĳ���
void test_hex_string();
void test_fork();
void test_as_socket();
void test_set();
void test_cache_buffer();
void test_cache_buffer2();

//����MD5
void test_md5();

//����base file�Ķ�д
void test_base_file();

//����JSONģ��
void test_json();

//����SilConnHash_Tģ��
void test_conn_hash();

//���Է���ڵ�ѡȡ�㷨ģ��
void test_node_load();
#endif

