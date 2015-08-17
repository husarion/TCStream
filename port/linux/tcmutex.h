#ifndef __TCMUTEX_H__
#define __TCMUTEX_H__

#include <mutex>

class TCMutex
{
public:
	std::mutex d_mutex;
	
	void lock()
	{
		d_mutex.lock();
	}
	void unlock()
	{
		d_mutex.unlock();
	}
	bool trylock()
	{
		return d_mutex.try_lock();
	}
};

#endif
