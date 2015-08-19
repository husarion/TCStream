#ifndef __TCMUTEX_H__
#define __TCMUTEX_H__

#include <pthread.h>

class TCMutex {
public:
	pthread_mutex_t mutex;

	TCMutex()
	{
		pthread_mutex_init(&mutex, NULL);
	}

	void lock()
	{
		pthread_mutex_lock(&mutex);
	}
	void unlock()
	{
		pthread_mutex_unlock(&mutex);
	}
};

#endif
