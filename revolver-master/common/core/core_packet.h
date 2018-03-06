/*************************************************************************************
*filename:	core_packet.h
*
*to do:		����ײ���Ϣ������,��Ҫ����������ͷ��Ϣ����ϢID��
*Create on: 2012-05
*Author:	zerok
*check list:
*************************************************************************************/
#ifndef __CORE_PACKET_H
#define __CORE_PACKET_H

#include "revolver/base_packet.h"
#include "revolver/base_bin_stream.h"
#include "revolver/object_pool.h"

#include <string>
using namespace std;

BASE_NAMESPACE_BEGIN_DECL

extern ObjectPool<BinStream, STREAM_POOL_SIZE>	STREAMPOOL;

#define GAIN_BINSTREAM(x) \
	BinStream* x = STREAMPOOL.pop_obj();\
	x->rewind(true)


#define RETURN_BINSTREAM(x) \
	if(x != NULL) \
	{\
		x->rewind(true);\
		x->reduce();\
		STREAMPOOL.push_obj(x);\
	}

typedef enum PacketClass
{
	CORE_HANDSHAKE,
	CORE_REQUEST,
	CORE_PING,
	CORE_MEDIA_SHELL,
	CORE_ZLIB,
	CORE_PONG
}PacketClass;
//���徻����Դ�ı��ģ���Ҫ����
class CCorePacket : public CBasePacket
{
public:
	CCorePacket()
	{
		server_id_		= 0;
		server_type_	= 0;
		msg_id_			= 0;
		msg_type_		= CORE_REQUEST;
		body_ptr_		= NULL;
	};

	~CCorePacket()
	{

	}

	virtual void release_self()
	{
		delete this;
	}

	CCorePacket& operator=(const CCorePacket& packet)
	{
		server_id_		= packet.server_id_;
		server_type_	= packet.server_type_;
		msg_id_			= packet.msg_id_;
		msg_type_		= packet.msg_type_;
		body_ptr_		= packet.body_ptr_;

		return *this;
	}

	void		set_body(CBasePacket& packet);

protected:
	//������뺯��
	virtual void	Pack(BinStream& strm) const;
	
	//���뺯��
	virtual void	UnPack(BinStream& strm);

	virtual void	Print(std::ostream& os) const
	{
		os << "Core Packet, {"\
			<< "server_id = " << server_id_ \
			<<", server_type = " << (uint16_t)server_type_ \
			<<", msg_id_ = " << msg_id_ \
			<< ", msg_type = " << (uint16_t) msg_type_\
			<<"}" << std::endl;
	}
	
public:
	uint32_t		server_id_;		//������ID
	uint8_t			server_type_;	//����������,0��ʾ�ͻ���
	uint32_t		msg_id_;		//��ϢID
	uint8_t			msg_type_;		//��Ϣ���ͣ����������PING PONG��Ϣ��������Ϣ��Ӧ�ò���Ϣ��
	CBasePacket*	body_ptr_;		//��Ϣ����
};

class HandShakeBody : public CBasePacket
{
public:
	HandShakeBody()
	{

	};

	~HandShakeBody()
	{

	};

	virtual void release_self()
	{
		delete this;
	};

protected:
	//������뺯��
	virtual void	Pack(BinStream& strm) const
	{
		strm << digest_data;
	};

	//���뺯��
	virtual void	UnPack(BinStream& strm)
	{
		strm >> digest_data;
	};

public:
	string digest_data;	//ժҪ��Ϣ
};

#define INIT_CORE_REQUEST(p, MSGID)\
	CCorePacket p;\
	p.msg_id_ = MSGID;\
	p.msg_type_ = CORE_REQUEST;\
	p.server_id_ = SERVER_ID;\
	p.server_type_ = SERVER_TYPE

#define INIT_CORE_PING(p)\
	CCorePacket p;\
	p.msg_id_ = 0;\
	p.msg_type_ = CORE_PING;\
	p.server_id_ = SERVER_ID;\
	p.server_type_ = SERVER_TYPE

#define INIT_CORE_PONG(p)\
	CCorePacket p;\
	p.msg_id_ = 0;\
	p.msg_type_ = CORE_PONG;\
	p.server_id_ = SERVER_ID;\
	p.server_type_ = SERVER_TYPE

#define INIT_CORE_HANDSHAKE(p)\
	CCorePacket p;\
	p.msg_id_ = 0;\
	p.msg_type_ = CORE_HANDSHAKE;\
	p.server_id_ = SERVER_ID;\
	p.server_type_ = SERVER_TYPE

BASE_NAMESPACE_END_DECL

#endif
/************************************************************************************/

