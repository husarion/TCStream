#ifndef __TCSTREAM_H__
#define __TCSTREAM_H__

#include <stdint.h>
#include <hFramework.h>
using namespace hFramework;

#pragma pack(1)
struct TPacketHeader
{
	uint8_t type;
	uint16_t packetId;
	uint16_t byteIdx;
	uint16_t length;
	uint16_t crc;
};
#pragma pack()

#pragma pack(1)
struct TQueueItem
{
	uint8_t type;
	uint8_t data;

	TQueueItem() { }
	TQueueItem(uint8_t type, uint8_t data) : type(type), data(data) { }
};
#pragma pack()

#define PACKET_HEADER_SIZE (4 + sizeof(TPacketHeader))

#define ERROR_TIMEOUT      -1
#define ERROR_NEWSESSION   -2
#define ERROR_API          -3
#define RESULT_NEWPACKET   -4
#define RESULT_ENDOFPACKET -5

class ITCDeviceStream
{
public:
	virtual int read(void* data, int length, int timeout) = 0;
	virtual int write(const void* data, int length, int timeout) = 0;
};

class TCStream
{
public:
	ITCDeviceStream& stream;
	hMutex readMutex;
	hMutex writeMutex, writeTransferMutex;
	hCondVar writeCondVar;

	TCStream(ITCDeviceStream& stream)
		: stream(stream)
	{
		lastReceivedPacketId = 0;
		sendHeader.packetId = sys.getRandNr();

		droppedStartPacketMakers = 0;
		droppedEndPacketMarkers = 0;
		droppedBytes = 0;
		droppedDataPackets = 0;
		retransmissions = 0;
		repeatedPackets = 0;
		timedoutPackets = 0;
		sentBytes = sentPayloadBytes = sentPackets = 0;
		recvBytes = recvPayloadBytes = recvPackets = 0;
	}

	/*void allocateQueue(int recvQueueSize)
	{
		recvQueue = new Queue<TQueueItem>(recvQueueSize);
	}*/

	void setQueue(hQueue<TQueueItem>& queue)
	{
		recvQueue = &queue;
	}

	/*void allocateBuffers(int packetSize)
	{
		this->packetSize = packetSize;
		inPacketData = (uint8_t*)System::malloc(packetSize + PACKET_HEADER_SIZE);
		outPacketData = (uint8_t*)System::malloc(packetSize + PACKET_HEADER_SIZE);
	}*/

	void setBuffers(int packetSize, uint8_t* inPacketData, uint8_t* outPacketData)
	{
		this->packetSize = packetSize;
		this->inPacketData = inPacketData;
		this->outPacketData = outPacketData;
	}

	void run();
	void stop() { doStop = true; }

	void beginPacket();
	int write(const void* data, int length);
	int endPacket();

	int read(void* data, int length, int timeout);

	void printStats();

private:
	int packetSize;
	bool doStop;

	// receiving
	uint8_t* inPacketData;
	TPacketHeader recvHeader;
	hQueue<TQueueItem>* recvQueue;
	uint16_t lastReceivedPacketId;
	uint16_t lastReceivedPacketByteIdx;
	int lastReceivedByteNum;
	uint8_t lastReceivedPacketType;

	void checkPacket();
	void processPacket();
	bool processIncomingBytes();

	void sendAck();

	// sending
	TPacketHeader sendHeader;
	uint8_t* outPacketData;
	int outIdx;
	uint16_t ackedPacketId, ackedPacketByteIdx;

	int flushPacket(bool lastPacket);
	int sendPacket(TPacketHeader& header);
	int sendDataPacket(TPacketHeader& header);

	// stats
	int sentBytes, sentPayloadBytes, sentPackets;
	int recvBytes, recvPayloadBytes, recvPackets;
	int droppedStartPacketMakers, droppedEndPacketMarkers, droppedBytes, droppedDataPackets;
	int repeatedPackets, retransmissions, timedoutPackets;
};

#endif
