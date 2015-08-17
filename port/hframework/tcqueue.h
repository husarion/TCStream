#ifndef __TCQUEUE_H__
#define __TCQUEUE_H__

#include <hFramework.h>

template<typename T>
class TCQueue
{
private:
	hFramework::hQueue<T> queue;
public:
	void init(int size)
	{
		queue.init(size);
	}

	bool put(T data, int timeout)
	{
		return queue.sendToBack(data, timeout);
	}
	bool peek(T& data, int timeout)
	{
		return queue.copyFromFront(data, timeout);
	}
	bool get(T& data, int timeout)
	{
		return queue.receive(data, timeout);
	}
};

#endif
