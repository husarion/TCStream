#ifndef __TCCONDVAR_H__
#define __TCCONDVAR_H__

#include <condition_variable>

#include "tcmutex.h"

class TCCondVar
{
	std::condition_variable_any d_condition;

public:
	void notify_one()
	{
		d_condition.notify_one();
	}
	bool wait(TCMutex& mutex, int timeout = 0)
	{
		return d_condition.wait_for(mutex.d_mutex, std::chrono::milliseconds(timeout)) == std::cv_status::no_timeout;
	}
};

#endif
