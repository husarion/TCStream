#ifndef __TCCONDVAR_H__
#define __TCCONDVAR_H__

#include <hFramework.h>

#include "tcmutex.h"

class TCCondVar
{
	hFramework::hMutex s, x; // TODO: change to semaphores
	volatile bool isWaiting;

public:
	TCCondVar()
	{
		x.give();
		s.take(0);
	}

	// only one thread at a time is calling this
	void notify_one()
	{
		x.take();
		if (isWaiting)
			s.give();
		x.give();
	}
	// only one thread at a time can call this
	bool wait(TCMutex& mutex, int timeout = 0)
	{
		x.take();
		isWaiting = true;
		x.give();

		mutex.unlock();

		bool res = s.take(timeout);

		mutex.lock();

		return res;
	}
};

#endif
