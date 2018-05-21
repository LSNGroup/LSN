/*************************************************************************************
*filename:	rudp_send_buffer.h
*
*to do:		����RUDP�ķ��ͻ���BUFFER�ͷ��ʹ���
*Create on: 2013-04
*Author:	zerok
*check list:
*************************************************************************************/

#ifndef __RUDP_SEND_BUFFER_H_
#define __RUDP_SEND_BUFFER_H_

#include "rudp/rudp_channel_interface.h"
#include "rudp/rudp_segment.h"

#include <map>
#include <list>

BASE_NAMESPACE_BEGIN_DECL

typedef map<uint64_t, RUDPSendSegment*> SendWindowMap;
typedef list<RUDPSendSegment*> SendDataList;

class RUDPCCCObject;

class RUDPSendBuffer
{
public:
	RUDPSendBuffer();
	virtual ~RUDPSendBuffer();
	
	void				reset();
	//�������ݽӿ�
	int32_t				send(const uint8_t* data, int32_t data_size);
	//ACK����
	void				on_ack(uint64_t ack_seq);
	//NACK����
	void				on_nack(uint64_t base_seq, const LossIDArray& loss_ids);
	//��ʱ���ӿ�
	void				on_timer(uint64_t now_ts);

	void				check_buffer();

public:
	uint64_t			get_buffer_seq() {return buffer_seq_;};
	//����NAGLE�㷨	
	void				set_nagle(bool nagle = true){nagle_ = nagle;};
	bool				get_nagle() const {return nagle_;};
	//���÷��ͻ������Ĵ�С
	void				set_buffer_size(int32_t buffer_size){buffer_size_ = buffer_size;};
	int32_t				get_buffer_size() const {return buffer_size_;};

	void				set_net_channel(IRUDPNetChannel *channel) {net_channel_ = channel;};
	void				set_ccc(RUDPCCCObject* ccc) {ccc_ = ccc;};

	uint32_t			get_bandwidth();
	int32_t				get_buffer_data_size() const {return buffer_data_size_;};

	void				clear_loss();
protected:
	//��ͼ����
	void				attempt_send(uint64_t now_timer);
	uint64_t			last_attempt_send_ts;

protected:
	IRUDPNetChannel*	net_channel_;

	//���ڷ��͵�����Ƭ
	SendWindowMap		send_window_;
	//���ڷ��͵ı��ĵĶ�������
	LossIDSet			loss_set_;
	//�ȴ����͵�����Ƭ
	SendDataList		send_data_;

	//���ͻ������Ĵ�С
	int32_t				buffer_size_;
	//��ǰ�������ݵĴ�С
	int32_t				buffer_data_size_;
	//��ǰBUFFER������SEQ
	uint64_t			buffer_seq_;
	//��ǰWINDOW������SEQ
	uint64_t			cwnd_max_seq_;
	//���ն�����SEQ
	uint64_t			dest_max_seq_;
	//�ٶȿ�����
	RUDPCCCObject*		ccc_;
	//�Ƿ�����NAGLE�㷨
	bool				nagle_;

	uint32_t			bandwidth_;
	uint64_t			bandwidth_ts_;
};

BASE_NAMESPACE_END_DECL

#endif

/************************************************************************************/


