import java.lang.Byte;
import java.lang.Override;
import java.lang.Runnable;
import java.lang.System;
import java.lang.Thread;
import java.net.SocketTimeoutException;
import java.util.logging.Logger;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;

public class Example_UDP implements TCStream.ITCDeviceStream
{
  private DatagramSocket socket;

  public void init()
  {
    try
    {
      this.socket = new DatagramSocket(3334, InetAddress.getLocalHost());
    }
    catch (Exception e)
    {
      e.printStackTrace();
    }
  }

  byte[] buffer = new byte[65535];
  int bufferLen = 0, bufferPos = 0;

  public int read(java.nio.ByteBuffer data, int length, int timeout)
  {
    if (bufferLen != 0)
    {
      int toRead = Math.min(length, bufferLen - bufferPos);
      data.put(buffer, bufferPos, toRead);
      bufferPos += toRead;
      if (bufferPos == bufferLen)
      {
        bufferLen = 0;
      }
      return toRead;
    }
    else
    {
      try
      {
        this.socket.setSoTimeout(timeout);
        DatagramPacket e = new DatagramPacket(buffer, buffer.length);
        this.socket.receive(e);
        bufferLen = e.getLength();
        bufferPos = 0;
        return read(data, length, timeout);
      }
      catch (SocketTimeoutException e)
      {
        return 0;
      }
      catch (IOException e)
      {
        return -1;
      }
    }
  }

  public int write(java.nio.ByteBuffer data, int length, int timeout)
  {
    try
    {
      // System.out.println("write " + length);
      byte[] d = new byte[length];
      data.get(d);
      DatagramPacket e = new DatagramPacket(d, 0, length, InetAddress.getByName("localhost"), 3333);
      this.socket.send(e);
      return length;
    }
    catch (IOException e)
    {
      e.printStackTrace();
      return -1;
    }
  }

  public void run()
  {
    long threadId = Thread.currentThread().getId();
    System.out.println("main Thread # " + threadId + " is doing this task");

    init();

    final TCStream obj = new TCStream(this);
    obj.init();
    obj.run();

    new Thread(new Runnable()
    {
      @Override
      public void run()
      {
        for (; ; )
        {
          java.nio.ByteBuffer bb = java.nio.ByteBuffer.allocateDirect(100);
          int r = obj.read(bb, 100, 1000);
          System.out.println("r " + r);
        }
      }
    }).start();

    try
    {
      java.nio.ByteBuffer bb = java.nio.ByteBuffer.allocateDirect(100);
      for (; ; )
      {
        /*bb.rewind();
        obj.beginPacket();
        bb.putChar('a');
        bb.putChar('b');
        bb.putChar('d');
        bb.putChar('e');
        bb.putChar('f');
        obj.write(bb, 5);
        obj.endPacket();*/

        Thread.sleep(100);
      }
    }
    catch (Exception e)
    {
    }
  }

  public static void main(String[] args)
  {
    new Example_UDP().run();
  }

}
