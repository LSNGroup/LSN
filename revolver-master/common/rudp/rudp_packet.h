/*************************************************************************************
*filename:	rudp_packet.h
*
*to do:		RUDPЭ�鶨��
*Create on: 2013-04
*Author:	zerok
*check list:
*************************************************************************************/

#ifndef __RUDP_PACKET_H_
#define __RUDP_PACKET_H_

#include "revolver/base_packet.h"
#include "revolver/base_bin_stream.h"
#include "revolver/object_pool.h"

#include <vector>

using namespace std;

BASE_NAMESPACE_BEGIN_DECL

#define PARSE_RUDP_MESSAGE(strm, T, body, info) \
	T body;\
	try{\
	strm >> body;\
}\
	catch(...){\
	RUDP_ERROR(info);\
	return; \
}

//���ӱ���Э��
#define RUDP_SYN				0x10		//������������
#define RUDP_SYN2				0x11		//�������ӷ��ذ�
#define RUDP_SYN_ACK			0x02		//SYN2��ACK
#define RUDP_FIN				0x13		//��������ر�
#define RUDP_FIN2				0x14		//�رշ��ذ�
#define RUDP_KEEPALIVE			0x15		//������
#define RUDP_KEEPALIVE_ACK		0x16		//�������ذ�

//����Э��
#define RUDP_DATA				0x20		//�ɿ�����
#define RUDP_DATA_ACK			0x23		//�ɿ�����ȷ��
#define RUDP_DATA_NACK			0X24		//����ȷ��

//���������
#define ERROR_SYN_STATE			0x01
#define SYSTEM_SYN_ERROR		0X02

class RUDPHeadPacket : public CBasePacket
{
public:
	RUDPHeadPacket(){};
	virtual ~RUDPHeadPacket(){};

protected:
	virtual void Pack(BinStream& strm) const
	{
		strm << msg_id_ << remote_rudp_id_ << check_sum_;
	};

	virtual void UnPack(BinStream& strm)
	{
		strm >> msg_id_ >> remote_rudp_id_ >> check_sum_;
	};

	virtual void Print(std::ostream& os)const
	{
		os << "msg id = " << ", remote rudp id = " <<remote_rudp_id_ << "local sum = " << check_sum_ << std::endl;
	};

public:
	uint8_t		msg_id_;
	uint16_t	check_sum_;
	int32_t		remote_rudp_id_; //-1��ʾ�Ƿ�
};

class RUDPSynPacket : public CBasePacket
{
public:
	RUDPSynPacket(){};
	virtual ~RUDPSynPacket(){};

protected:
	virtual void Pack(BinStream& strm) const
	{
		strm << version_ << max_segment_size_ << local_rudp_id_ << start_seq_ << local_ts_;
	};

	virtual void UnPack(BinStream& strm)
	{
		strm >> version_ >> max_segment_size_ >> local_rudp_id_ >> start_seq_ >> local_ts_;
	};

	virtual void Print(std::ostream& os)const
	{
		os << "version = " << version_ << ", max_segment_size = " << max_segment_size_ \
			<< ", local rudp id = " << local_rudp_id_ << ", start seq = " << local_rudp_id_\
			<< ", local ts = " << local_ts_ << std::endl;
	}

public:
	uint8_t			version_;				//�汾
	uint16_t		max_segment_size_;		//����ߴ�
	int32_t			local_rudp_id_;			//����RUDP ����ID,����socket id
	uint64_t		start_seq_;				//��ʼ���
	uint64_t		local_ts_;				//����ʱ���
};	


class RUDPSyn2Packet : public CBasePacket
{
public:
	RUDPSyn2Packet()
	{
	};

	virtual ~RUDPSyn2Packet()
	{
	};

protected:
	virtual void Pack(BinStream& strm) const
	{
		strm << version_ << syn2_result_ << max_segment_size_ << local_rudp_id_ << start_seq_ << local_ts_ << remote_ts_;
	};

	virtual void UnPack(BinStream& strm)
	{
		strm >> version_>> syn2_result_ >> max_segment_size_ >> local_rudp_id_ >> start_seq_ >> local_ts_ >> remote_ts_;
	};

	virtual void Print(std::ostream& os)const
	{
		os << "version = " << version_ << ", max_segment_size = " << max_segment_size_ \
			<< ", syn2_result_ = " << syn2_result_ \
			<< ", local rudp id = " << local_rudp_id_ << ", start seq = " << local_rudp_id_ \
			<< ", local ts = " << local_ts_ << ", remote ts = " << remote_ts_ << std::endl;
	}
public:
	uint8_t			version_;			//�汾
	uint8_t			syn2_result_;		//���ֽ��
	uint16_t		max_segment_size_;	//��������ߴ�
	int32_t			local_rudp_id_;		//����rudp ����ID
	uint64_t		start_seq_;			//������ʼ���
	uint64_t		local_ts_;			//����ʱ���
	uint64_t		remote_ts_;			//Զ��ʱ���, RUDPSynPacket.local_ts_
};

class RUDPSyn2AckPacket : public CBasePacket
{
public:
	RUDPSyn2AckPacket()
	{
	};

	virtual ~RUDPSyn2AckPacket()
	{
	}

protected:
	virtual void Pack(BinStream& strm) const
	{
		strm << result_ << remote_ts_;
	};

	virtual void UnPack(BinStream& strm)
	{
		strm >> result_ >> remote_ts_;
	};

	virtual void Print(std::ostream& os)const
	{
		os << "result = " << (uint16_t)result_  << ", remote ts = " << remote_ts_ << std::endl;
	}

public:
	uint8_t			result_;
	uint64_t		remote_ts_;	//Զ��ʱ�����RUDPSyn2Packet.local_ts_
};

class RDUPKeepLive : public CBasePacket
{
public:
	RDUPKeepLive(){};
	virtual ~RDUPKeepLive(){};

protected:
	virtual void Pack(BinStream& strm) const
	{
		strm << timestamp_;
	};

	virtual void UnPack(BinStream& strm)
	{
		strm >> timestamp_;
	};

	virtual void Print(std::ostream& os)const
	{
		os << "timestamp = " << timestamp_ << std::endl;
	};

public:
	uint64_t	timestamp_;		//ʱ�������Ҫ���ڼ���RTT
};

class RUDPData : public CBasePacket
{
public:
	RUDPData() {};
	virtual ~RUDPData(){};

protected:
	virtual void Pack(BinStream& strm) const
	{
		strm << ack_seq_id_ << cur_seq_id_;// << data_;
	};

	virtual void UnPack(BinStream& strm)
	{
		strm >> ack_seq_id_ >> cur_seq_id_;// >> data_;
	};

	virtual void Print(std::ostream& os)const
	{
		os << "ack seq id = " << ack_seq_id_ << ",cur seq id = " << cur_seq_id_ << std::endl;
	};

public:
	//��ǰ���ܶ��������SEQ
	uint64_t	ack_seq_id_;
	//��ǰ���ݿ��SEQ
	uint64_t	cur_seq_id_;

	//string		data_;
};

class RUDPDataAck : public CBasePacket
{
public:
	RUDPDataAck(){};
	virtual ~RUDPDataAck(){};

protected:
	virtual void Pack(BinStream& strm) const
	{
		strm << ack_seq_id_;
	};

	virtual void UnPack(BinStream& strm)
	{
		strm >> ack_seq_id_ ;
	};

	virtual void Print(std::ostream& os)const
	{
		os << "ack seq id = " << ack_seq_id_ << std::endl;
	};

public:
	//��ǰ���ܶ��������SEQ
	uint64_t	ack_seq_id_;
};

typedef vector<uint16_t>	LossIDArray;

class RUDPDataNack : public CBasePacket
{
public:
	RUDPDataNack() {};
	virtual ~RUDPDataNack() {};

protected:
		virtual void Pack(BinStream& strm) const
	{
		strm << base_seq_;

		uint16_t array_size = loss_ids_.size();
		strm << array_size;
		for(uint16_t i = 0; i < array_size; ++i)
		{
			strm << loss_ids_[i];
		}
	};

	virtual void UnPack(BinStream& strm)
	{
		strm >> base_seq_ ;

		uint16_t array_size;
		strm >> array_size;
		loss_ids_.resize(array_size);
		for(uint16_t i = 0; i < array_size; ++i)
		{
			strm >> loss_ids_[i];
		}
	};

	virtual void Print(std::ostream& os)const
	{
		os << "base seq id = " << base_seq_ << "loss ids =";
		for(uint16_t i = 0; i < loss_ids_.size(); ++i)
		{
			os << " " << loss_ids_[i];
		}
	};

public:
	uint64_t		base_seq_;	//������ʼID
	LossIDArray		loss_ids_; //�������У����base seq�ľ��롣����ͨ��loss ids[i]��base_seq��ӵõ����������
};

BASE_NAMESPACE_END_DECL

#endif
/************************************************************************************/



