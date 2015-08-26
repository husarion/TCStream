package utils;

import java.util.logging.Logger;

public class TCStream {
	public interface ITCDeviceStream {
		int read(java.nio.ByteBuffer data, int length, int timeout);

		int write(java.nio.ByteBuffer data, int length, int timeout);
	}

	private final static Logger logger = Logger.getLogger("tcstream");

	private ITCDeviceStream stream;

	public TCStream(ITCDeviceStream stream) {
		this.stream = stream;
	}

	public void init() {
	}

	// for native use
	public int onRead(java.nio.ByteBuffer data, int length, int timeout) {
		return stream.read(data, length, timeout);
	}

	// for native use
	public int onWrite(java.nio.ByteBuffer data, int length, int timeout) {
		return stream.write(data, length, timeout);
	}

	public native void run(int packetSize, int queueSize);

	public native void stop();

	public native void beginPacket();

	public native int write(java.nio.ByteBuffer data, int length);

	public native void endPacket();

	public native int read(java.nio.ByteBuffer data, int length, int timeout);

	static {
		System.loadLibrary("tcstream");
	}
}
