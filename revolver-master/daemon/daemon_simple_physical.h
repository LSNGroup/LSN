/*************************************************************************************
*filename:	daemon_simple_physical.h
*
*to do:		����һ���򵥵����������������࣬�������ʽ��ϵͳ�����Լ̳�IPhysicalServer
			��ʵ��һ���ͺ�̨���ݿ���ʺʹ洢���������Ժܷ���������Ԫ�������
*Create on: 2013-08
*Author:	zerok
*check list:
*************************************************************************************/
#ifndef __DAEMON_SIMPLE_PHYSICAL_H
#define __DAEMON_SIMPLE_PHYSICAL_H

#include "daemon_physical.h"
#include "daemon_json.h"

#include <map>
using namespace std;
using namespace BASE;

//���������Ϣ
typedef struct tagPhysicalServerInfo
{
	//���ŵ�ַ,�������ͨ����ַΪ0.0.0.0
	string		tel_addr;
	//��ͨ��ַ,����ǵ��ţ���ַΪ0.0.0.0
	string		cnc_addr;
	//�������ͣ���λ1�ֽڱ�ʾ����������0x00Ϊ������0x01Ϊ�Ϻ�����λһ�ֽڱ�ʾ��Ӫ�̣����磺0x00��ʾ���ţ�0x01��ʾ��ͨ��.......0xff��ʾ˫��
	uint16_t	net_type;
}PhysicalServerInfo;

//����Ԫ��Ϣ
typedef struct tagDaemonServerInfo
{
	//����ID�������ظ�
	uint32_t	sid;
	//�������ͣ��μ�core_server_type.h
	uint8_t		stype;
	//��������
	uint16_t	net_type;

	//���ŵ�ַ,�������ͨ����ַΪ0.0.0.0
	string		tel_addr;
	//��ͨ��ַ,����ǵ��ţ���ַΪ0.0.0.0
	string		cnc_addr;
}DaemonServerInfo;

typedef map<string, PhysicalServerInfo>		PhysicalServerInfoMap;
typedef map<uint32_t, DaemonServerInfo>		DaemonServerInfoMap;

class CDaemonServer;
//ģ��һ���򵥵�DB�������Ԫ��Ϣ������
class DaemonServerDB : public IPhysicalServer
{
public:
	DaemonServerDB(CDaemonServer* server);
	~DaemonServerDB();

	void					get_physical_server(uint32_t sid, uint8_t stype, const string& server_ip, CConnection* connection);
	void					on_connection_disconnected(CConnection* connection);

protected:
	void					init();
	void					destroy();

	//��������ݿ�洢�Ļ������������ݿ�ű�ʵ������2������
	ServerInfoJson			create_server(uint32_t sid, uint8_t stype, const string& server_ip);
	uint32_t				max_sid();
protected:
	CDaemonServer*			server_;
	//TODO:��������ݿ�洢���뽫��Щphysical_server_map_����Ϣ���������ݿ���
	PhysicalServerInfoMap	physical_server_map_;
	//TODO:��������ݿ�洢���뽫server_info_map_��������Ϣ���浽���ݿ�
	DaemonServerInfoMap		server_info_map_;
};
#endif
/************************************************************************************/

