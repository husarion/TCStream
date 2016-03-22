#include <hFramework.h>
#include <stdio.h>

#include "tcstream.h"

class MySerial : public virtual ITCDeviceStream
{
public:
	void init()
	{
	}

	int read(void* data, int length, int timeout)
	{
		int res = hExt1.serial.read(data, length, timeout);
		// printf("r %d\r\n", res);
		return res;
	}
	int write(const void* data, int length, int timeout)
	{
		return hExt1.serial.write(data, length, timeout);
	}
};

MySerial serial;
TCStream tcs(serial);

#define PACKET_SIZE 50
char d[300];
uint8_t b1[PACKET_SIZE + PACKET_HEADER_SIZE], b2[PACKET_SIZE + PACKET_HEADER_SIZE];

void run()
{
	tcs.run();
}
void run2()
{
	for (;;)
	{
		tcs.beginPacket();
		tcs.write("data", 4);
		tcs.endPacket();

		sys.delay(500);
	}
}
void run3()
{
	for (;;)
	{
		tcs.printStats();
		sys.delay(1000);
	}
}

void dowait(const char *a, int sleep)
{
	uint32_t s =  sys.getRefTime();
	sys.delay(sleep);
	printf("%s waited %d\r\n", a, sys.getRefTime() - s);
}
void hMain()
{
	sys.setLogDev(&Serial);

	serial.init();

	hExt1.serial.init(921600);

	tcs.setBuffers(PACKET_SIZE, b1, b2);
	tcs.allocateQueue(80);

	sys.taskCreate([]()
	{
		tcs.run();
	});

	sys.taskCreate([]()
	{
		// for (;;)
		// {
			// uint8_t inBuffer[64];
			// int r = tcs.read(inBuffer, 64, 200);
			// printf("%d\r\n", r);
		// }
	}, 4);

	sys.taskCreate([]()
	{
		for (;;)
		{
			// dowait("task", 5000);
			// dowait("task", 5000);
			dowait("task4 1000", 1000);

			tcs.beginPacket();
			int res = tcs.write("SAD", 3);
			if (res > 0)
				tcs.endPacket();
		}
	}, 4);
	sys.taskCreate([]()
	{
		for (;;)
		{
			// dowait("task", 5000);
			// dowait("task", 5000);
			dowait("task3 5000", 5000);

			tcs.beginPacket();
			int res = tcs.write("VERYSAD", 7);
			if (res > 0)
				tcs.endPacket();
			// platform.write("a", 1);
			// platform.printf("asd %d %d", num++, sys.getRefTime()/1000);
			// printf("%d\r\n", sys.getRefTime());
		}
	}, 3);

	for (;;)
	{
		sys.delay(200);
	}

	sys.taskCreate(run);
// sys.taskCreate(run2);
	sys.taskCreate(run3);

	for (;;)
	{
		int total = 0;
		int rr;
		for (;;)
		{
			LED1.toggle();
			rr = tcs.read(d + total, 300, 1000);
			// printf("-------------- main RES: %d\r\n", rr);
			if (rr == -1)
			{
				printf("================ TIMEOUT ===========\r\n");
				continue;
			}
			if (rr == -3)
			{
				printf("================ SHORT BUFFER ===========\r\n");
				continue;
			}
			if (rr == -4)
			{
				total = 0;
			}
			if (rr == -5)
			{
				// printf("TOTAL RECEVIED %d %s\r\n", total, d);
			}
			if (rr >= 0)
			{
				total += rr;
			}
		}

		LED1.toggle();
		// sys.delay(100);
	}
}
