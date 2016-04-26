#ifndef __TCUTILS_H__
#define __TCUTILS_H__

#include <stdint.h>
#include <sys/time.h>
#include <stdio.h>
#include <cstdlib>

#ifdef ANDROID_NDK
#include <android/log.h>
#define TCS_LOG_FUNC(x,...) __android_log_print(2, "tcstream", x , ##__VA_ARGS__)
#else
#define TCS_LOG_FUNC(x,...) fprintf(stderr, x, ##__VA_ARGS__)
#endif

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
