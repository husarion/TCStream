#ifndef __TCQUEUE_H__
#define __TCQUEUE_H__

#include <mutex>
#include <condition_variable>
#include <deque>
#include <chrono>

#include "tccondvar.h"

template<typename T>
class TCQueue
{
private:
	TCMutex mutex;
	TCCondVar condVarGet, condVarPut;
	std::deque<T> queue;
	uint32_t size;
public:
	void init(int size)
	{
		this->size = size;
	}

	bool put(T data, int timeout)
	{
		mutex.lock();
		while (queue.size() >= size)
		{
			if (!condVarPut.wait(mutex, timeout))
			{
				mutex.unlock();
				return false;
			}
		}

		queue.push_back(data);

		mutex.unlock();
		condVarGet.notify_one();
		return true;
	}
	bool peek(T& data, int timeout)
	{
		mutex.lock();
		while (queue.empty())
		{
			if (!condVarGet.wait(mutex, timeout))
			{
				mutex.unlock();
				return false;
			}
		}

		T rc(std::move(queue.front()));
		data = rc;

		mutex.unlock();
		return true;
	}
	bool get(T& data, int timeout)
	{
		mutex.lock();
		while (queue.empty())
		{
			if (!condVarGet.wait(mutex, timeout))
			{
				mutex.unlock();
				return false;
			}
		}

		T rc(std::move(queue.front()));
		data = rc;
		queue.pop_front();

		mutex.unlock();
		condVarPut.notify_one();
		return true;
	}
};

#endif
