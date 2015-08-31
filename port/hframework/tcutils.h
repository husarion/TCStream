#ifndef __TCUTILS_H__
#define __TCUTILS_H__

#include <stdint.h>

#include <hFramework.h>

#define TCS_LOG_FUNC(x,...) sys.log(x, ##__VA_ARGS__)

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
