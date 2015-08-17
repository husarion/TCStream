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
uint8_t b1[PACKET_SIZE], b2[PACKET_SIZE];

void run()
{
	tcs.run();
}
void run2()
{
	for(;;)
	{
		tcs.beginPacket();
		tcs.write("data", 4);
		tcs.endPacket();

		sys.delay(500);
	}
}
void run3()
{
	for(;;)
	{
		tcs.printStats();
		sys.delay(1000);
	}
}

void hMain()
{
	sys.setLogDev(&Serial);

	serial.init();

	hExt1.serial.init(921600);

	tcs.setBuffers(PACKET_SIZE, b1, b2);
	tcs.allocateQueue(50);

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
