#ifndef __TCSTREAM_H__
#define __TCSTREAM_H__

#include <stdint.h>

#include "tcqueue.h"
#include "tccondvar.h"
#include "tcutils.h"

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
	TCMutex readMutex;
	TCMutex writeMutex, writeTransferMutex;
	TCCondVar writeCondVar;

	TCStream(ITCDeviceStream& stream)
		: stream(stream)
	{
		lastReceivedPacketId = 0;
		sendHeader.packetId = TCUtils::getRand();

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

	void allocateQueue(int recvQueueSize)
	{
		recvQueue = new TCQueue<TQueueItem>();
		recvQueue->init(recvQueueSize);
	}

	void setQueue(TCQueue<TQueueItem>& queue)
	{
		recvQueue = &queue;
	}

	void allocateBuffers(int packetSize)
	{
		this->packetSize = packetSize;
		inPacketData = (uint8_t*)TCUtils::malloc(packetSize);
		outPacketData = (uint8_t*)TCUtils::malloc(packetSize);
	}

	void setBuffers(int packetSize, uint8_t* inPacketData, uint8_t* outPacketData)
	{
		this->packetSize = packetSize;
		this->inPacketData = inPacketData;
		this->outPacketData = outPacketData;
	}

	void run();

	void beginPacket();
	int write(const void* data, int length);
	int endPacket();

	int read(void* data, int length, int timeout);

	void printStats();

private:
public:

	int packetSize;

	// receiving
	uint8_t* inPacketData;
	TPacketHeader recvHeader;
	TCQueue<TQueueItem>* recvQueue;
	uint16_t lastReceivedPacketId;
	uint16_t lastReceivedPacketByteIdx;
	int lastReceivedByteNum;

	void checkPacket();
	void processPacket();
	bool processIncomingBytes();

	void sendAck();

	// sending
	TPacketHeader sendHeader;
	uint8_t* outPacketData;
	int outIdx;
	uint16_t ackedPacketId, ackedPacketByteIdx;

	int flushPacket();
	int sendPacket(TPacketHeader& header, const void* data, bool waitForAck);

	// stats
	int sentBytes, sentPayloadBytes, sentPackets;
	int recvBytes, recvPayloadBytes, recvPackets;
	int droppedStartPacketMakers, droppedEndPacketMarkers, droppedBytes, droppedDataPackets;
	int repeatedPackets, retransmissions, timedoutPackets;
};

#endif
