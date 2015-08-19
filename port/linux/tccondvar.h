#ifndef __TCCONDVAR_H__
#define __TCCONDVAR_H__

#include <pthread.h>
#include <sys/time.h>

#include "tcmutex.h"

#include <stdio.h>
class TCCondVar {
	pthread_cond_t cond;

public:
	TCCondVar()
	{
		pthread_cond_init(&cond, NULL);
	}

	void notify_one()
	{
		pthread_cond_signal(&cond);
	}
	bool wait(TCMutex& mutex, int timeout = 0)
	{
		struct timespec timeToWait;
		struct timeval now;
		int rt;
		gettimeofday(&now, NULL);

		int usec = (now.tv_usec + 1000UL * timeout);
		int sec = usec / 1000000;
		usec -= sec * 1000000;
		timeToWait.tv_sec = now.tv_sec + sec;
		timeToWait.tv_nsec = usec * 1000UL;

		rt = pthread_cond_timedwait(&cond, &mutex.mutex, &timeToWait);

		return rt == 0;
	}
};

#endif
