#ifndef __TCMUTEX_H__
#define __TCMUTEX_H__

#include <hFramework.h>

class TCMutex
{
public:
	hFramework::hMutex mutex;
	
	void lock()
	{
		mutex.take();
	}
	void unlock()
	{
		mutex.give();
	}
};

#endif
