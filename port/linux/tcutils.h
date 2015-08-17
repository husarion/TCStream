#ifndef __TCUTILS_H__
#define __TCUTILS_H__

#include <stdint.h>
#include <sys/time.h>

// #define LOG(x,...) printf(x "\r\n", ##__VA_ARGS__)
#define LOG_INFO(x,...) printf(x, ##__VA_ARGS__)

class TCUtils
{
public:
	static uint32_t getTicks()
	{
		timeval tv;
		gettimeofday(&tv, 0);
		uint32_t val = tv.tv_sec * 1000 + tv.tv_usec / 1000;
		return val;
	}
	static uint32_t getRand()
	{
		return rand();
	}
	static void* malloc(int size)
	{
		return ::malloc(size);
	}
};

#endif
