/*************************************************************************************
*filename:	rudp_event_handler.h
*
*to do:		RUDP��event handler������Ӧ�ò���¼������ʹ���Ľӿ�
*Create on: 2013-04
*Author:	zerok
*check list:
*************************************************************************************/

#ifndef __RUDP_EVENT_HANDLER_H_
#define __RUDP_EVENT_HANDLER_H_

#define INVALID_RUDP_HANDLE		-1

BASE_NAMESPACE_BEGIN_DECL

class RUDPEventHandler
{
public:
	RUDPEventHandler(){};
	virtual ~RUDPEventHandler(){};

	virtual	int32_t rudp_accept_event(int32_t rudp_id) {return 0;};
	virtual	int32_t	rudp_input_event(int32_t rudp_id) {return 0;};
	virtual int32_t rudp_output_event(int32_t rudp_id) {return 0;};
	virtual	int32_t rudp_close_event(int32_t rudp_id) {return 0;};
	virtual int32_t rudp_exception_event(int32_t rudp_id) {return 0;};
};

BASE_NAMESPACE_END_DECL

#endif
/************************************************************************************/

