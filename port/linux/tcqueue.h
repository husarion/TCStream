#ifndef __TCQUEUE_H__
#define __TCQUEUE_H__

#include <queue>

#include "tccondvar.h"

template<typename T>
class TCQueue
{
private:
	TCMutex mutex;
	TCCondVar condVarGet, condVarPut;
	std::queue<T> queue;
	uint32_t size;
public:
	void init(int size)
	{
		this->size = size;
	}

	bool put(const T& data, int timeout)
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

		queue.push(data);

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

		data = queue.front();

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

		data = queue.front();
		queue.pop();

		mutex.unlock();
		condVarPut.notify_one();
		return true;
	}
};

#endif
