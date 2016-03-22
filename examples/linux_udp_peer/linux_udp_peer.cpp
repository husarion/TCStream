#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pty.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tcstream.h"

#include "UdpSocket.h"

#include <thread>

int dropping = 0;
bool doDrop()
{
	if (!dropping)
		return false;
	if ((rand() % 100) < dropping)
	{
		printf("dropping packet for test\r\n");
		return true;
	}
	else
	{
		return false;
	}
}

class UdpStream : public virtual ITCDeviceStream
{
public:
	UdpSocket socket;
	string ipTo;
	int portFrom;
	int portTo;

	void init()
	{
		socket.init();
		bool r  = socket.bind(portFrom);
		printf("bind: %d\r\n", r);
	}

	uint8_t buffer[65535];
	int bufferLen = 0, bufferPos = 0;
	int read(void* data, int length, int timeout)
	{
		if (bufferLen != 0)
		{
			int toRead = min(length, bufferLen - bufferPos);
			memcpy(data, buffer + bufferPos, toRead);
			bufferPos += toRead;
			if (bufferPos == bufferLen)
			{
				bufferLen = 0;
			}
			return toRead;
		}
		else
		{
			string ip;
			uint16_t port;
			int res = socket.read(ip, port, buffer, sizeof(buffer), timeout);
			// printf("r %d\r\n", res);
			if (res < 0)
				return -1;

			bufferLen = res;
			bufferPos = 0;

			return read(data, length, timeout);
		}

	}
	int write(const void* data, int length, int timeout)
	{
		if (doDrop())
			return length;
		// printf("SOCKET SEND\r\n");
		socket.send(ipTo, portTo, data, length);
		return length;
	}
};

UdpStream udpStream;

void hMain(int argc, char** argv)
{
	srand(time(0));
	udpStream.portFrom = atoi(argv[1]);
	udpStream.ipTo = argv[2];
	udpStream.portTo = atoi(argv[3]);
	udpStream.init();
	TCStream tcs(udpStream);

	tcs.allocateBuffers(5);
	tcs.allocateQueue(10);

	std::thread th([&tcs]()
	{
		tcs.run();
	});

	std::thread th3([&tcs]()
	{
		for (;;)
		{
			// printf("tx %d %d rx %d %d \r\n", txpkt, txbytes, rxpkt, rxbytes);
			// tcs.printStats();
			usleep(1000000);
		}
	});

	int ac = atoi(argv[4]);
	dropping = atoi(argv[5]);

	int res;
	if (ac == 1)
	{
		for (;;)
		{
			// printf("====================================\r\n");

			char d[300];
			static int num = 0;
			// int len = sprintf(d, "[ab %7d abc", num++);
			int len = sprintf(d, "%4d", num++);
			printf("SEND: %s\r\n", d);
			// int len = sprintf(d, "%7d", num++);
			// int len = sprintf(d, "abcdef");
			len++;

			tcs.beginPacket();
			res = tcs.write(d, len);
			res = tcs.endPacket();

			if (res == -1)
				printf("timeout\r\n");

			// usleep(100000000);
			// usleep(1000000);
			usleep(1000000);
		}
	}
	else if (ac == 0)
	{
		int total = 0;
		int rr;
		char d[1024];
		// usleep(3000000);
		for (;;)
		{
			// printf("===\r\n");
			rr = tcs.read(d + total, 1024, 3000);
			printf("-------------- main RES: %d\r\n", rr);
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
				printf("TOTAL RECEVIED %d %s\r\n", total, d);
				usleep(3000000);
			}
			if (rr >= 0)
			{
				total += rr;
			}
		}
	}
	else if (ac == 2)
	{
		std::thread thR([&tcs]()
		{
			int total = 0;
			int rr;
			char d[1024];
			for (;;)
			{
				rr = tcs.read(d + total, 1024, 3000);
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
					printf("TOTAL RECEVIED %d %s\r\n", total, d);
				}
				if (rr >= 0)
				{
					total += rr;
				}
			}
		});
		std::thread thS([&tcs, &ac]()
		{
			for (;;)
			{
				// printf("====================================\r\n");

				char d[100];
				static int num = 0;
				int len = sprintf(d, "[ab %7d abc ac=%d", num++, ac);
				// int len = sprintf(d, "%7d", num++);
				// int len = sprintf(d, "abcdef");
				len++;

				tcs.beginPacket();
				int res = tcs.write(d, len);
				res = tcs.endPacket();

				if (res == -1)
					printf("timeout\r\n");

				usleep(1000 + (rand() % 10000));
				// usleep(200000);
			}
		});

		thR.join();
	}

	th.join();
}

int main(int argc, char** argv)
{
	hMain(argc, argv);
}
