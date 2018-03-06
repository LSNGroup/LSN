/*************************************************************************************
*filename:	rudp_channel_interface.h
*
*to do:		RUDP BUFFER������ͨ���ӿ�
*Create on: 2013-04
*Author:	zerok
*check list:
*************************************************************************************/
#ifndef __RUDP_CHANNEL_INTERFACE_H_
#define __RUDP_CHANNEL_INTERFACE_H_

#include "revolver/base_typedef.h"
#include "rudp/rudp_packet.h"

BASE_NAMESPACE_BEGIN_DECL

class IRUDPNetChannel
{
public:
	IRUDPNetChannel() {};
	virtual ~IRUDPNetChannel() {};

	//BUFFER���ķ��ͽӿ�
	virtual void send_ack(uint64_t ack_seq_id) = 0;
	virtual void send_nack(uint64_t base_seq_id, const LossIDArray& ids) = 0;
	virtual	void send_data(uint64_t ack_seq_id, uint64_t cur_seq_id, const uint8_t* data, uint16_t data_size, uint64_t now_ts) = 0;

	//BUFFER״̬�ӿ�,BUFFER���Խ������ݷ���ͨ��
	virtual void on_write() = 0;
	//buffer���Խ������ݶ�ȡ
	virtual void on_read() = 0;
	//����ͨ��
	virtual void on_exception() = 0;
};

BASE_NAMESPACE_END_DECL

#endif
/************************************************************************************/

