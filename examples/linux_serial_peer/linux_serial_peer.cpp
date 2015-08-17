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

int txbytes = 0, rxbytes = 0;
int txpkt = 0, rxpkt = 0;

int dropping = 0;
bool doDrop()
{
	if (!dropping)
		return false;
	if ((rand() % 1000) < dropping)
	{
		printf("dropping packet for test\r\n");
		return true;
	}
	else
	{
		return false;
	}
}

class SerialTTY : public virtual ITCDeviceStream
{
public:
	int write_fd = 1, read_fd = 0;

	void init()
	{
		// printf("a1: %d\r\n", r);
	}

	int read(void* data, int length, int timeout)
	{
		fd_set fdset;
		struct timeval timeout_val = {0};
		timeout_val.tv_usec = timeout * 1000;
		FD_ZERO(&fdset);
		FD_SET(read_fd, &fdset);
		select(read_fd + 1, &fdset, NULL, NULL, &timeout_val);

		int res = ::read(read_fd, data, length);
		if (res == -1 && errno == EAGAIN)
			return 0;
		return res;
	}
	int write(const void* data, int length, int timeout)
	{
		return ::write(write_fd, data, length);
	}
};

SerialTTY serialTTY;

int hMain(int argc, char** argv)
{
	srand(time(0));
	TCStream tcs(serialTTY);
	
	tcs.allocateBuffers(50);
	tcs.allocateQueue(1000);

	std::thread th([&tcs]()
	{
		tcs.run();
	});

	std::thread th3([&tcs]()
	{
		for (;;)
		{
			// printf("tx %d %d rx %d %d \r\n", txpkt, txbytes, rxpkt, rxbytes);
			tcs.printStats();
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
			int len = sprintf(d, "[ab %7d abc", num++);
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
		}
	}
	else if (ac == 0)
	{
		int total = 0;
		int rr;
		char d[1024];
		int pos;
		printf("a0\r\n");
		usleep(3000000);
		printf("a\r\n");
		for (;;)
		{
			int len;
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
			}
			if (rr >= 0)
			{
				total += rr;
			}
			usleep(1000);
		}
	}
	else if (ac == 2)
	{
		std::thread thR([&tcs]()
		{
			int total = 0;
			int rr;
			char d[1024];
			int pos;
			for (;;)
			{
				int len;
				// printf("===\r\n");
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

void tty_raw(int fd)
{
	struct termios raw;

	if (tcgetattr(fd, &raw) == -1)
	{
		fprintf(stderr, "tcgetattr");
		exit(1);
	}
	/* input modes - clear indicated ones giving: no break, no CR to NL,
	   no parity check, no strip char, no start/stop output (sic) control */
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

	/* output modes - clear giving: no post processing such as NL to CR+NL */
	raw.c_oflag &= ~(OPOST);

	/* control modes - set 8 bit chars */
	raw.c_cflag |= (CS8);
	/* local modes - clear giving: echoing off, canonical off (no erase with
	   backspace, ^U,...),  no extended functions, no signal chars (^Z,^C) */
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	/* control chars - set return condition: min number of bytes and timer */
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 0; /* immediate - anything       */
	/* put terminal in raw mode after flushing */
	if (tcsetattr(fd, TCSAFLUSH, &raw) < 0)
	{
		fprintf(stderr, "can't set raw mode");
		exit(1);
	}
}

int main(int argc, char** argv)
{
	// int master, slave;
	// char path[64];
	// pid_t pid = openpty(&master, &slave, path, 0, 0);
	// if (pid < 0)
	// {
	// fprintf(stderr, "Unable to allocate ptty\n");
	// return 1;
	// }

	// tty_raw(master);

	// printf("created pty %s\r\n", path);
	// chmod(path, 0666);

	// const char* link = "/dev/ttyVIRTSTM";
	// printf("creating symlink %s\r\n", link);
	// unlink(link);
	// symlink(path, link);

	// link = "/tmp/ttyVIRTSTM";
	// printf("creating symlink %s\r\n", link);
	// unlink(link);
	// symlink(path, link);

	// int flags = fcntl(master, F_GETFL, 0);
	// flags = flags | O_NONBLOCK;
	// fcntl(master, F_SETFL, flags);


	int fd = open("/dev/ttyUSB1", O_RDWR);

	struct termios tty;
	memset(&tty, 0, sizeof tty);
	tcgetattr(fd, &tty);
	cfsetospeed(&tty, B921600);
	cfsetispeed(&tty, B921600);

	tcsetattr(fd, TCSANOW, &tty);

	tty_raw(fd);

	serialTTY.write_fd = fd;
	serialTTY.read_fd = fd;

	hMain(argc, argv);
}
