#include "base_reactor_instance.h"
#include "rudp_thread.h"

RudpThread::RudpThread()
{

}

RudpThread::~RudpThread()
{

}

void RudpThread::execute()
{
	while(!get_terminated())
	{
#ifdef WIN32
		__try
		{
#endif
			REACTOR_INSTANCE()->event_loop();
#ifdef WIN32
		}
		__except(1)
		{
			ExitProcess(0);
		}
#endif
	}

	REACTOR_INSTANCE()->stop_event_loop();

	clear_thread();
}

