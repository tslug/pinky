import java.lang.*;
import java.io.*;

/**
** This class' purpose is to write specified bits into
** a file.  The bit values are passed in via a string,
** which is composed of "0"'s and "1"'s (ie, "10110").
** Once 8 bits have been accumulated it writes the
** data to the stream.  When the stream is closed,
** the bits which make up the remainder of the last
** byte are set to zero and the stream is flush'ed.
*/

public class BitStream {
	protected byte current;
	protected int bitMask = 0x80;
	protected BufferedOutputStream out;
	protected long count;

	public BitStream(String filename) 
	throws Exception {
		long tmp;
		FileOutputStream fileout;

		fileout = new FileOutputStream(filename);
		out = new BufferedOutputStream(fileout);

		current = 0;
		count = 0;
	}

	public void writeBit(int val) 
	throws IOException {
		if (val != 0) {
			current |= bitMask;
		}
		bitMask >>= 1;
		if (bitMask == 0) {
			bitMask = 0x80;
			out.write(current);
			current=0;
		}
		count++;
	}

	public void writeBits(String bitString, int length) 
	throws IOException, Exception {
		char data[];
		int looper,diff;

		//System.out.print("BITS: " + bitString);
		data = bitString.toCharArray();
		//If the string is shorter than the length we
		//  want to write.. buffer the end with zeros.
		diff = length - bitString.length();
		if (diff < 0) diff=0;
		//System.out.println("  Diff=" + diff);
		for (looper=0; looper<diff; looper++) {
			writeBit(0);
		}

		//read the string one char at a time and write that
		//  bit to the output stream
		for (looper=0; looper<data.length; looper++) {
			switch (data[looper]) {
				case '0':
					writeBit(0);
					break;
				case '1':
					writeBit(1);
					break;
				default:
					throw(new Exception("bitString must only contain '0's and '1's!"));
			}
		}
	}

	public void close() 
	throws IOException {
		if (bitMask != 0x80) {
			out.write(current);
		}
		out.flush();
		out.close();
		//System.out.println("Wrote " + count + " bits.");
	}
}
