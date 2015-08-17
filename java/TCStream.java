import java.lang.Byte;
import java.lang.System;
import java.net.SocketTimeoutException;
import java.util.logging.Logger;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;

public class TCStream
{
  public interface ITCDeviceStream
  {
    int read(java.nio.ByteBuffer data, int length, int timeout);

    int write(java.nio.ByteBuffer data, int length, int timeout);
  }

  private final static Logger logger = Logger.getLogger("tcstream");

  private ITCDeviceStream stream;

  public TCStream(ITCDeviceStream stream)
  {
    this.stream = stream;
  }

  public void init()
  {
  }

  public int onRead(java.nio.ByteBuffer data, int length, int timeout)
  {
    return stream.read(data, length, timeout);
  }

  public int onWrite(java.nio.ByteBuffer data, int length, int timeout)
  {
    return stream.write(data, length, timeout);
  }

  public native void run();

  public native void beginPacket();

  public native int write(java.nio.ByteBuffer data, int length);

  public native void endPacket();

  public native int read(java.nio.ByteBuffer data, int length, int timeout);

  static
  {
    System.loadLibrary("tcstream");
  }
}
