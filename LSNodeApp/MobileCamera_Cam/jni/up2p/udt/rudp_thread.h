#ifndef __RUDP_THREAD_H_
#define __RUDP_THREAD_H_

#include "base_namespace.h"
#include "base_typedef.h"
#include "base_thread.h"
#include "base_singleton.h"

using namespace BASE;

class RudpThread : public CThread
{
public:
	RudpThread();
	virtual ~RudpThread();

	void execute();
};

#define CREATE_RUDP_THREAD	CSingleton<RudpThread>::instance
#define RUDP_THREAD			CSingleton<RudpThread>::instance
#define DESTROY_RUDP_THREAD	CSingleton<RudpThread>::destroy

#endif /* __RUDP_THREAD_H_ */

