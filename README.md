# TCStream
Connectionless, packet-based Transmission Control layer for any stream of bytes

If a stream can expose read() and write() methods, this library can be used to provide reliability for data transmitted over the stream.

Features:
  * Connectionless
  * Packet-based
  * Support for dynamically generated packets
  * Retransmissions
  * Checksums
  * Can be used on devices with low memory (sending packet in chunks)

Assumptions:
  * P2P link
  * Sent bytes always arrive in order
  * Operating system available (mutexes, threads)

Available ports:
  * Linux
  * [RoboCORE](robocore.io)
