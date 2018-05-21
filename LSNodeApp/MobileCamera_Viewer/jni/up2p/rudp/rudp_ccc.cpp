#include "rudp/rudp_ccc.h"
#include "revolver/base_timer_value.h"
#include "rudp/rudp_log_macro.h"

#include <math.h>

BASE_NAMESPACE_BEGIN_DECL
//默认窗口大小
#define DEFAULT_CWND_SIZE	16
//最小窗口
#define MIN_CWND_SIZE		8

//默认RTT
#define DEFAULT_RTT			50
//最小RTT
#define MIN_RTT				5
//最大RTT
#define MAX_RTT				1000

RUDPCCCObject::RUDPCCCObject()
{
	reset();
}

RUDPCCCObject::~RUDPCCCObject()
{
}

void RUDPCCCObject::init(uint64_t last_ack_id)
{
	reset();

	last_ack_id_ = last_ack_id;
	prev_on_ts_ = CBaseTimeValue::get_time_value().msec();
}

void RUDPCCCObject::reset()
{
	max_cwnd_ = DEFAULT_CWND_SIZE;

	snd_cwnd_ = MIN_CWND_SIZE;
	rtt_ = DEFAULT_RTT;
	rtt_var_ = DEFAULT_RTT / 2;

	last_ack_id_ = 0;
	prev_on_ts_ = 0;

	slow_start_ = true;
	loss_flag_ = false;
	
	rtt_first_ = true;

	resend_count_ = 0;
	print_count_ = 0;
}

void RUDPCCCObject::on_ack(uint64_t ack_seq)
{
	if(ack_seq <= last_ack_id_)
	{
		return;
	}

	if(slow_start_) //慢启动窗口决策
	{
		snd_cwnd_ = snd_cwnd_ * 2;

		if(snd_cwnd_ >= max_cwnd_/2)
		{
			slow_start_ = false;
			RUDP_INFO("max_cwnd/2: ccc stop slow_start, snd_cwnd = " << snd_cwnd_);

			snd_cwnd_ = max_cwnd_/2;
			snd_cwnd_ = core_max(MIN_CWND_SIZE, snd_cwnd_);
		}
	}
	else //平衡状态
	{
		if(loss_flag_) //出现丢包，不做加速
			loss_flag_ = false;
		else //加速
		{
			snd_cwnd_ += 1;
			if (snd_cwnd_ < max_cwnd_/2) {
				slow_start_ = true;
			}
		}
	}

	last_ack_id_ = ack_seq;
}

void RUDPCCCObject::on_loss(uint64_t base_seq, const LossIDArray& loss_ids)
{
	if(slow_start_) //取消慢启动
	{
		slow_start_ = false;
		RUDP_INFO("loss: ccc stop slow_start, snd_cwnd = " << snd_cwnd_);
	}

	//重新计算新的窗口
	if(loss_ids.size() > 0 && last_ack_id_ < base_seq + loss_ids[loss_ids.size() - 1])
	{
		if (false == loss_flag_) {
			max_cwnd_ = (max_cwnd_ * 3 + snd_cwnd_) / 4;
			RUDP_DEBUG("loss: max_cwnd_ =" << max_cwnd_ << ",snd_cwnd_ =" << snd_cwnd_);
		}

		snd_cwnd_ = (uint32_t)ceil(snd_cwnd_ / 1.5);
		snd_cwnd_ = core_max(MIN_CWND_SIZE, snd_cwnd_);
	}

	loss_flag_ = true;
}

void RUDPCCCObject::on_timer(uint64_t now_ts)
{
	uint32_t delay = 100;
	if(rtt_  > 10)
		delay = 10 * rtt_;
	if(delay > 2000)
		delay = 2000;

	if(now_ts >= prev_on_ts_ + delay) //10个RTT决策一次
	{
		print_count_ ++;
		if(print_count_ % 10 == 0)
		{
			RUDP_DEBUG("send window size = " << snd_cwnd_ << ",rtt = " << rtt_ << ",rtt_var = " << rtt_var_ << ",resend = " << resend_count_);
		}

		if(slow_start_)
		{
			;
		}
		else
		{
			if(resend_count_ > core_max(snd_cwnd_ / 2, 8)) //重发次数超过了容忍的次数，降低发送窗口
			{
				snd_cwnd_ = (uint32_t)ceil(snd_cwnd_ / 1.125);
				snd_cwnd_ = core_max(MIN_CWND_SIZE, snd_cwnd_);
			}

			resend_count_ = 0;
		}

		prev_on_ts_ = now_ts;
	} 
}

void RUDPCCCObject::set_rtt(uint32_t keep_live_rtt)
{
	if (keep_live_rtt > MAX_RTT) {
		return;
	}

	//计算rtt和rtt修正
	if(rtt_first_)
	{
		rtt_first_ = false;
		rtt_ = keep_live_rtt;
		rtt_var_ = rtt_ / 2;
	}
	else //参考了tcp的rtt计算
	{
		rtt_var_ = (rtt_var_ * 3 + core_abs(rtt_, keep_live_rtt)) / 4;
		rtt_ = (7 * rtt_ + keep_live_rtt) / 8;
	}

	rtt_ = core_max(MIN_RTT, rtt_);
	rtt_ = core_min(MAX_RTT, rtt_);
	rtt_var_ = core_max(MIN_RTT/2, rtt_var_);
	rtt_var_ = core_min(MAX_RTT/2, rtt_var_);
}

BASE_NAMESPACE_END_DECL
