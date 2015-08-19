#include <stdlib.h>
#include <unistd.h>
#include <cassert>
#include <cstring>

#include "tcstream.h"
#include "tcutils.h"

uint8_t magic[4] = { 0xaa, 0xbb, 0xcc, 0xdd };

#define PACKET_TYPE_DATA 1
#define PACKET_TYPE_SOP  2 // start of packet
#define PACKET_TYPE_EOP  4 // end of packet
#define PACKET_TYPE_ACK  8

#if defined(LOG_FUNC) && defined(TCSTREAM_DEBUG)
#define LOG(x,...) LOG_FUNC(x "\r\n", ##__VA_ARGS__)
#define LOG_INFO(x,...) LOG_FUNC(x, ##__VA_ARGS__)
#else
#define LOG(x,...)
#define LOG_INFO(x,...)
#endif

#define ERROR_TIMEOUT      -1
#define ERROR_NEWSESSION   -2
#define ERROR_API          -3
#define RESULT_NEWPACKET   -4
#define RESULT_ENDOFPACKET -5

uint16_t crc16_update(uint16_t crc, const void* data, int len);

void printHeader(const char* intro, const TPacketHeader& header)
{
#ifdef TCSTREAM_DEBUG
	const char *t1 = "", *t2 = "", *t3 = "", *t4 = "";
	if (header.type & PACKET_TYPE_SOP) t1 = " SOP";
	if (header.type & PACKET_TYPE_DATA) t2 = " DATA";
	if (header.type & PACKET_TYPE_EOP) t3 = " EOP";
	if (header.type & PACKET_TYPE_ACK) t4 = " ACK";

	LOG("%s type:%s%s%s%s id: %d byteIdx: %d len: %d crc: 0x%04x", intro,
	    t1, t2, t3, t4, header.packetId, header.byteIdx, header.length, header.crc);
#endif
}

void TCStream::run()
{
	int state = 0;
	uint8_t magicQueue[4];
	uint32_t startTime;
	uint16_t idx;
	doStop = false;
	while (!doStop)
	{
		char inData[10];
		int rd = stream.read(inData, sizeof(inData), 20);
		LOG("new data: %d", rd);

		if (rd > 0)
			recvBytes += rd;

		for (int i = 0; i < rd; i++)
		{
			uint8_t b = inData[i];
			// LOG("new data (%d): 0x%02x %d", i, b, b);

			magicQueue[0] = magicQueue[1];
			magicQueue[1] = magicQueue[2];
			magicQueue[2] = magicQueue[3];
			magicQueue[3] = b;

			bool magicMatch =
			  magicQueue[0] == magic[0] &&
			  magicQueue[1] == magic[1] &&
			  magicQueue[2] == magic[2] &&
			  magicQueue[3] == magic[3];

			uint8_t *headerData = (uint8_t*)&recvHeader;

			switch (state)
			{
			case 0:
				if (magicMatch)
				{
					state = 1;
					startTime = TCUtils::getTicks();
					LOG("magic match start %d", startTime);
					idx = 0;
				}
				break;
			case 1:
				headerData[idx++] = b;
				if (idx == sizeof(recvHeader))
				{
					printHeader("RECEIVED PACKET", recvHeader);
					if (recvHeader.length > 0)
					{
						idx = 0;
						state = 2;
					}
					else
					{
						checkPacket();
						state = 0;
					}
				}
				break;
			case 2:
				inPacketData[idx++] = b;
				if (idx == recvHeader.length)
				{
					LOG("got payload");
					checkPacket();
					state = 0;
				}
				else if (idx > packetSize)
				{
					LOG("too long packet");
					state = 0;
				}
				break;
			}
		}

		if (state != 0 && TCUtils::getTicks() - startTime >= 150)
		{
			state = 0;
			LOG("lost packet");
		}
	}
}

// receiving
void TCStream::checkPacket()
{
	uint16_t origCrc = recvHeader.crc;
	recvHeader.crc = 0;

	uint16_t crc = 0;
	crc = crc16_update(crc, (void*)&recvHeader, sizeof(recvHeader));
	crc = crc16_update(crc, inPacketData, recvHeader.length);
	// LOG("calcr: 0x%04x", crc);

	if (origCrc == crc)
	{
		processPacket();
	}
	else
	{
		LOG("bad crc");
	}
}
void TCStream::processPacket()
{
	int type = recvHeader.type;
	bool doSendAck = false;

	if (recvHeader.type == PACKET_TYPE_ACK)
	{
		LOG("process ACK (%d:%d)", recvHeader.packetId, recvHeader.byteIdx);
		writeMutex.lock();
		ackedPacketId = recvHeader.packetId;
		ackedPacketByteIdx = recvHeader.byteIdx;
		writeMutex.unlock();
		writeCondVar.notify_one();
		LOG("ack proceed");
		return;
	}

	if (type & PACKET_TYPE_SOP)
	{
		LOG("process SOP (%d:%d)", recvHeader.packetId, recvHeader.byteIdx);
		if (recvHeader.packetId == lastReceivedPacketId)
		{
			// prevent putting multiple SOP to the queue, ACK will be sent in DATA processing, as SOP always comes with DATA
		}
		else
		{
			LOG("new packet id");
			if (recvQueue->put(TQueueItem(1, 0), 10))
			{
				lastReceivedPacketId = recvHeader.packetId;
				lastReceivedPacketByteIdx = 0;
				lastReceivedByteNum = -1;
				lastReceivedPacketType = PACKET_TYPE_DATA;
			}
			else
			{
				droppedStartPacketMakers++;
				goto end;
			}
		}
	}

	if (type & PACKET_TYPE_DATA)
	{
		LOG("process DATA (%d:%d)", recvHeader.packetId, recvHeader.byteIdx);
		// printf("got packet (%d:%d)\r\n", recvHeader.packetId, recvHeader.byteIdx);

		if (recvHeader.packetId == lastReceivedPacketId)
		{
			// sender didn't receive ACK and retransmitted packet
			if (recvHeader.byteIdx < lastReceivedPacketByteIdx)
			{
				LOG("repeated DATA, dropping, new packet (%d:%d) last packet (%d:%d)",
				    recvHeader.packetId, recvHeader.byteIdx, lastReceivedPacketId, lastReceivedPacketByteIdx);

				repeatedPackets++;

				doSendAck = true;
			}
			// next packet part was received
			else
			{
				LOG("new packet part");

				if (processIncomingBytes())
				{
					recvPackets++;
					lastReceivedPacketByteIdx = recvHeader.byteIdx;
					doSendAck = true;
				}
				else
				{
					droppedDataPackets++;
					goto end;
				}
			}
		}
		// different packet
		else
		{
			// packet with different id was received but SOP was not present, as we cannot do anything interesting
			// in this situation, it is simple dropped
			LOG("unknown %d:%d\r\n", recvHeader.packetId, recvHeader.byteIdx);
		}
	}

	if (type & PACKET_TYPE_EOP)
	{
		LOG("process EOP (%d:%d)", recvHeader.packetId, recvHeader.byteIdx);
		if (recvHeader.packetId == lastReceivedPacketId)
		{
			// sender didn't receive ACK and retransmitted packet
			if (lastReceivedPacketType == PACKET_TYPE_EOP)
			{
				LOG("repeated EOP, dropping, new packet (%d:%d) last packet (%d:%d)",
				    recvHeader.packetId, recvHeader.byteIdx, lastReceivedPacketId, lastReceivedPacketByteIdx);

				repeatedPackets++;

				doSendAck = true;
			}
			// End Of Packet was received
			else
			{
				int received = lastReceivedByteNum + 1;

				int toReceive;
				if (type & PACKET_TYPE_DATA) // DATA + EOP
					toReceive = recvHeader.byteIdx + recvHeader.length;
				else // only EOP
					toReceive = recvHeader.byteIdx; // byteIdx = total payload bytes sent by sender

				LOG("whole packet, received %d, to receive %d", received, toReceive);

				if (received == toReceive)
				{
					if (recvQueue->put(TQueueItem(2, 0), 10))
					{
						recvPackets++;
						lastReceivedPacketByteIdx = recvHeader.byteIdx;
						lastReceivedPacketType = PACKET_TYPE_EOP;
						doSendAck = true;
					}
					else
					{
						droppedEndPacketMarkers++;
						goto end;
					}
				}
				// we received EOP but we didn't receive all DATA packets, so all we can do is to ignore whole packet
				else
				{
					LOG("EOP without DATA, ignoring");
				}
			}
		}
		// different packet
		else
		{
			// packet with different id was received but it is End Of Packet, as we cannot do anything interesting
			// in this situation, it is simple dropped
			LOG("unknown EOP %d:%d\r\n", recvHeader.packetId, recvHeader.byteIdx);
		}
	}

end:
	if (doSendAck)
		sendAck();
}
bool TCStream::processIncomingBytes()
{
	int len = recvHeader.length;

	// try to put data in receive queue, if not all data can be put, return false
	// so sender will resend whole part
	for (int i = 0; i < len; i++)
	{
		int byteNum = recvHeader.byteIdx + i;
		// printf("byteNum %d [%d] [%c]\r\n", byteNum, inPacketData[i], inPacketData[i]);

		// lastReceivedByteNum - num of bytes put in queue for current packet
		if (byteNum > lastReceivedByteNum)
		{
			if (recvQueue->put(TQueueItem(0, inPacketData[i]), 10))
			{
				// printf("p %d %d\r\n", lastReceivedByteNum, inPacketData[i]);
				lastReceivedByteNum++;
				recvPayloadBytes++;
			}
			else
			{
				// printf("asd\r\n");
				droppedBytes++;
				LOG("unable to put all in queue, force other side to resend packet");
				return false; // in order to make other side to resend packet (not all bytes has been placen in queue)
			}
		}
	}

	return true;
}

// sending
void TCStream::beginPacket()
{
	writeTransferMutex.lock();
	sendHeader.type = PACKET_TYPE_SOP;
	sendHeader.packetId++;
	sendHeader.length = 0;
	sendHeader.crc = 0;
	sendHeader.byteIdx = 0;
	outIdx = 0;
}
int TCStream::write(const void* data, int length)
{
	for (int i = 0; i < length; i++)
	{
		outPacketData[PACKET_HEADER_SIZE + outIdx++] = ((uint8_t*)data)[i];
		if (outIdx == packetSize)
		{
			LOG("out buffer full (%d), flushing", outIdx);
			int res = flushPacket(false);
			if (res < 0)
			{
				writeTransferMutex.unlock();
				return res;
			}
		}
	}

	LOG("part saved to buffer");
	return length;
}
int TCStream::endPacket()
{
	if (outIdx > 0)
	{
		LOG("sending remaining in buffer %d", outIdx);
		int res = flushPacket(true);
		if (res < 0)
		{
			writeTransferMutex.unlock();
			return res;
		}
	}
	else
	{
		LOG("sending EOP marker");
		TPacketHeader eopHeader;
		eopHeader.type = PACKET_TYPE_EOP;
		eopHeader.packetId = sendHeader.packetId;
		eopHeader.byteIdx = sendHeader.byteIdx; // bytesIdx = total length
		eopHeader.crc = 0;
		eopHeader.length = 0;
		int res = sendDataPacket(eopHeader);
		if (res < 0)
		{
			writeTransferMutex.unlock();
			return res;
		}
	}

	LOG("whole packet has been sent successfully");

	writeTransferMutex.unlock();
	return 0;
}
int TCStream::flushPacket(bool lastPacket)
{
	LOG("flushPacket() len: %d", outIdx);
	sendHeader.length = outIdx;
	sendHeader.type |= PACKET_TYPE_DATA;
	if (lastPacket)
		sendHeader.type |= PACKET_TYPE_EOP;

	int res = sendDataPacket(sendHeader);

	if (res == 0)
	{
		sendHeader.byteIdx += sendHeader.length;
		sendHeader.type &= ~PACKET_TYPE_SOP;
		outIdx = 0;
		return sendHeader.length;
	}
	else
	{
		return res;
	}
}
int TCStream::sendPacket(TPacketHeader& header)
{
	header.crc = 0;
	uint16_t crc = crc16_update(0, &header, sizeof(header));
	header.crc = crc;

	uint8_t buffer[PACKET_HEADER_SIZE];
	memcpy(buffer, magic, sizeof(magic));
	memcpy(buffer + sizeof(magic), &header, sizeof(header));
	int res = stream.write(buffer, sizeof(buffer), 100);
	if (res < 0)
	{
		LOG("API error %d", res);
		return ERROR_API;
	}
	return 0;
}
int TCStream::sendDataPacket(TPacketHeader& header)
{
	header.crc = 0;
	uint16_t crc = crc16_update(0, &header, sizeof(header));
	crc = crc16_update(crc, outPacketData + PACKET_HEADER_SIZE, header.length);
	header.crc = crc;

	printHeader("SENDING PACKET", header);

	memcpy(outPacketData, magic, sizeof(magic));
	memcpy(outPacketData + sizeof(magic), &header, sizeof(header));

	bool sendok = false;
	uint32_t t = TCUtils::getTicks();
	while (!sendok && TCUtils::getTicks() - t < 1000)
	{
		int res = stream.write(outPacketData, PACKET_HEADER_SIZE + header.length, 100);
		if (res < 0)
		{
			LOG("API error %d", res);
			return ERROR_API;
		}

		bool timedout = false;

		writeMutex.lock();
		while (!timedout && (ackedPacketId != header.packetId || ackedPacketByteIdx != header.byteIdx))
		{
			if (!writeCondVar.wait(writeMutex, 200))
				timedout = true;
		}
		writeMutex.unlock();

		if (timedout)
		{
			// usleep(20000);
			// usleep(1000000);
			LOG("resending");
			retransmissions++;
		}
		else
		{
			sentPayloadBytes += header.length;
			sentPackets++;
			// printf("ela %d\r\n", TCUtils::getTicks()-t);
			LOG("packet has beed ACKed");
			return 0;
		}
	}
	timedoutPackets++;
	return ERROR_TIMEOUT;
}

// interface
int TCStream::read(void* data, int length, int timeout)
{
	assert(timeout >= 0);
	LOG("read(%d)", length);
	readMutex.lock();
	uint8_t *_data = (uint8_t*)data;
	for (int i = 0; i < length; i++)
	{
		TQueueItem item;
		bool r = recvQueue->peek(item, timeout);
		timeout = 0; // timeout is only for first byte
		if (r)
		{
			// item with data
			if (item.type == 0)
			{
				recvQueue->get(item, 0);
				_data[i] = item.data;
				LOG("read(): got byte %d", _data[i]);
			}
			else
			{
				// if item on the front of the queue is a special one (packet start or stop), it cannot be pop,
				// if there are another bytes received during current read() invocation (i > 0). if so,
				// return current received bytes. special item will be returned during subsequent call.
				if (i == 0)
				{
					if (item.type == 1)
					{
						recvQueue->get(item, 0);
						LOG("read(): return new packet");
						readMutex.unlock();
						return RESULT_NEWPACKET;
					}
					else if (item.type == 2)
					{
						recvQueue->get(item, 0);
						LOG("read(): return packet end");
						readMutex.unlock();
						return RESULT_ENDOFPACKET;
					}
				}
				else
				{
					LOG("EOD");
					readMutex.unlock();
					return i;
				}
			}
		}
		else
		{
			LOG("EOD");
			readMutex.unlock();
			return i;
		}
	}
	readMutex.unlock();
	return length;
}

void TCStream::sendAck()
{
	TPacketHeader packet;
	packet.type = PACKET_TYPE_ACK;
	packet.packetId = lastReceivedPacketId;
	packet.byteIdx = lastReceivedPacketByteIdx;
	packet.length = 0;

	LOG("send ack");
	sendPacket(packet);
}

void TCStream::printStats()
{
	LOG_INFO("Receiving:\r\n");
	LOG_INFO("  packets: %d bytes: %d payload bytes: %d\r\n",
	         recvPackets, recvBytes, recvPayloadBytes);
	LOG_INFO("  dropped: START: %3d DATA: %3d EOP: %3d BYTES: %3d\r\n",
	         droppedStartPacketMakers, droppedDataPackets, droppedEndPacketMarkers, droppedBytes);
	LOG_INFO("  repeated: %3d\r\n", repeatedPackets);
	LOG_INFO("Sending:\r\n");
	LOG_INFO("  packets: %d bytes: %d payload bytes: %d\r\n",
	         sentPackets, sentBytes, sentPayloadBytes);
	LOG_INFO("  retransmissions: %d\r\n  timeouts: %d\r\n", retransmissions, timedoutPackets);
}

uint16_t crc16_update(uint16_t crc, const void* data, int len)
{
	int i;

	uint8_t *_data = (uint8_t*)data;

	while (len--)
	{
		crc ^= *_data++;
		for (i = 0; i < 8; ++i)
		{
			if (crc & 1)
				crc = (crc >> 1) ^ 0xA001;
			else
				crc = (crc >> 1);
		}
	}

	return crc;
}
