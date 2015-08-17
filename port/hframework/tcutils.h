#ifndef __TCUTILS_H__
#define __TCUTILS_H__

#include <stdint.h>

#include <hFramework.h>

// #define LOG(x,...) sys.log(x "\r\n", ##__VA_ARGS__)
#define LOG_INFO(x,...) sys.log(x, ##__VA_ARGS__)

class TCUtils
{
public:
	static uint32_t getTicks()
	{
		return sys.getRefTime();
	}
	static uint32_t getRand()
	{
		return sys.getRandNr();
	}
	static void* malloc(int size)
	{
		return 0;
	}
};

#endif
