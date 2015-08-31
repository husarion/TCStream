#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdint.h>
#include <pthread.h>

#include "../tcstream.h"
#include "tcutils.h"
#include "TCStream.h"

#if defined(LOG_FUNC) && defined(TCSTREAM_DEBUG)
#define LOG(x,...) LOG_FUNC(x "\r\n", ##__VA_ARGS__)
#define LOG_INFO(x,...) LOG_FUNC(x, ##__VA_ARGS__)
#else
#define LOG(x,...)
#define LOG_INFO(x,...)
#endif

JavaVM* g_vm;
jobject g_obj;
jmethodID g_midRead, g_midWrite;
jbyteArray g_array;
jclass g_clazz;
uint8_t* g_data;

pthread_t runThread;

class Serial : public virtual ITCDeviceStream
{
public:
	// always called from native thread so attach only once
	int read(void* data, int length, int timeout)
	{
		JNIEnv *g_env;
		int getEnvStat = g_vm->GetEnv((void **)&g_env, JNI_VERSION_1_6);


		jobject buffer = g_env->NewDirectByteBuffer(data, length);

		int res = g_env->CallIntMethod(g_obj, g_midRead, buffer, length, timeout);
		// LOG("rr %d", res);

		g_env->DeleteLocalRef(buffer);

		if (g_env->ExceptionCheck())
		{
			g_env->ExceptionDescribe();
		}

		return res;
	}
	int write(const void* data, int length, int timeout)
	{
		JNIEnv *g_env;
		int getEnvStat = g_vm->GetEnv((void **)&g_env, JNI_VERSION_1_6);

		bool attached = false;
		if (getEnvStat == JNI_EDETACHED)
		{
			// std::cout << "GetEnv: not attached" << std::endl;
			// if (g_vm->AttachCurrentThread((void**)&g_env, NULL) != 0)
#ifdef ANDROID_NDK
			if (g_vm->AttachCurrentThread(&g_env, NULL) != 0)
#else
			if (g_vm->AttachCurrentThread((void**)&g_env, NULL) != 0)
#endif
			{
				std::cout << "Failed to attach" << std::endl;
				return -1;
			}
			attached = true;
		}
		else if (getEnvStat == JNI_OK)
		{
			//
		}
		else if (getEnvStat == JNI_EVERSION)
		{
			std::cout << "GetEnv: version not supported" << std::endl;
			return -1;
		}

		jobject buffer = g_env->NewDirectByteBuffer(const_cast<void*>(data), length);

		int res = g_env->CallIntMethod(g_obj, g_midWrite, buffer, length, timeout);
		// printf("rr %d\r\n", res);

		g_env->DeleteLocalRef(buffer);

		if (g_env->ExceptionCheck())
		{
			g_env->ExceptionDescribe();
		}

		if (attached)
			g_vm->DetachCurrentThread();

		return res;
	}
};

Serial serial;
TCStream tcs(serial);

void* runThreadFunc(void*)
{
	JNIEnv *g_env;
	int getEnvStat = g_vm->GetEnv((void **)&g_env, JNI_VERSION_1_6);

	bool attached = false;
	if (getEnvStat == JNI_EDETACHED)
	{
		// std::cout << "GetEnv: not attached" << std::endl;
#ifdef ANDROID_NDK
		if (g_vm->AttachCurrentThread(&g_env, NULL) != 0)
#else
		if (g_vm->AttachCurrentThread((void**)&g_env, NULL) != 0)
#endif
		{
			std::cout << "Failed to attach" << std::endl;
			return 0;
		}
		attached = true;
	}
	else if (getEnvStat == JNI_OK)
	{
		//
	}
	else if (getEnvStat == JNI_EVERSION)
	{
		std::cout << "GetEnv: version not supported" << std::endl;
		return 0;
	}

	tcs.run();

	LOG("detaching");
	if (attached)
		g_vm->DetachCurrentThread();
	LOG("detached");

	return 0;
}
JNIEXPORT void JNICALL Java_utils_TCStream_run(JNIEnv* env, jobject obj, jint packetSize, jint queueSize)
{
	env->GetJavaVM(&g_vm);
	g_obj = env->NewGlobalRef(obj);

	g_clazz = env->GetObjectClass(g_obj);
	if (g_clazz == NULL)
	{
		std::cout << "Failed to find class" << std::endl;
	}

	g_midRead = env->GetMethodID(g_clazz, "onRead", "(Ljava/nio/ByteBuffer;II)I");
	if (g_midRead == NULL)
	{
		std::cout << "Unable to get method1 ref" << std::endl;
	}

	g_midWrite = env->GetMethodID(g_clazz, "onWrite", "(Ljava/nio/ByteBuffer;II)I");
	if (g_midWrite == NULL)
	{

		std::cout << "Unable to get method2 ref" << std::endl;
	}

	tcs.allocateBuffers(packetSize);
	tcs.allocateQueue(queueSize);

	srand(time(0));

	pthread_create(&runThread, NULL, runThreadFunc, 0);
}

JNIEXPORT void JNICALL Java_utils_TCStream_stop(JNIEnv* env, jobject obj)
{
	LOG("tcs.stop()");
	tcs.stop();
	LOG("join");
	pthread_join(runThread, 0);
	LOG("joined");
}

JNIEXPORT void JNICALL Java_utils_TCStream_beginPacket(JNIEnv *, jobject)
{
	tcs.beginPacket();
}
JNIEXPORT jint JNICALL Java_utils_TCStream_write(JNIEnv* env, jobject obj, jobject buffer, jint length)
{
	void* data = env->GetDirectBufferAddress(buffer);
	int res = tcs.write(data, length);
	return res;
}
JNIEXPORT void JNICALL Java_utils_TCStream_endPacket(JNIEnv *, jobject)
{
	tcs.endPacket();
}

JNIEXPORT jint JNICALL Java_utils_TCStream_read(JNIEnv* env, jobject obj, jobject buffer, jint length, jint timeout)
{
	void* data = env->GetDirectBufferAddress(buffer);
	int res = tcs.read(data, length, timeout);
	return res;
}
