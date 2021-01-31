import java.lang.*;
import java.util.Vector;

import ParseNode;
import ParseArgument;
import BitStream;

/**
** This class is designed to parse a text string in an
** effort to figure out what op code is meant to be.  It
** should also provide the monolithic part of the assembler
** with information such as how many arguments.
*/
public class ParseInstruction extends ParseNode {
	protected static final int OPCODE_MOVE = 1;
	protected static final int OPCODE_ARM = 2;
	protected static final int OPCODE_WAIT = 3;
	protected static final int OPCODE_DATA = 4;
	protected static final int MAX_N = 63;
	protected boolean isFinalN;
	protected long n;
	protected Vector arguments;
	protected int opcode, argumentCount;

	/**
	** Constructs the class (obviously) and parses the text
	** to figure out what instruction it is.
	*/
	protected ParseInstruction(long line, String instructionText) 
	throws Exception {
		super(line, instructionText);
		//Set up some of the basics
		labels = new Vector();
		n = MAX_N;
		argumentCount = 0;
		arguments = new Vector();
		readOpcode(instructionText);
	}

	/**
	**  This method does the meat of the work (which must
	**  be fish or chicken, since the code isnt too heavy)
	**  when it comes to figuring out what opcode is what.
	*/
	protected void readOpcode(String fromHere) 
	throws Exception {
		String tmpOp, tmpN;
		int chIndex, tmpint;
		Integer tmpInteger;

		text = new String(fromHere);
		chIndex = fromHere.indexOf('.');
		if (chIndex >= 0) {
			//n is set manually.. extract and use it.
			tmpN = text.substring(chIndex + 1, text.length());
			tmpOp = text.substring(0,chIndex);
		} else {
			tmpN = null;
			tmpOp = fromHere;
		}
		tmpOp = tmpOp.toUpperCase();

		if (tmpOp.equals("MV")) {
			opcode = OPCODE_MOVE;
			argumentCount = 3;
		} else if (tmpOp.equals("ARM")) {
			opcode = OPCODE_ARM;
			argumentCount = 2;
		} else if (tmpOp.equals("WAIT")) {
			opcode = OPCODE_WAIT;
			argumentCount = 0;
			setN(0);
			setNFinal(true);
		} else if (tmpOp.equals("DATA")) {
			//This really isnt an opcode, but it
			//  almost behaves like one grammatically.
			opcode = OPCODE_DATA;
			argumentCount = 1;
		} else {
			throw(new Exception("Invalid opcode"));
		}

		text = tmpOp;

		//Manually set N:
		if (tmpN != null) {
			try {
				tmpInteger = new Integer(tmpN);
				tmpint = tmpInteger.intValue();
				if ((tmpint > 63) || (tmpint < 0)){
					throw(new Exception("N must be a number from 0 to 63"));
				}
				setN(tmpint);
				setNFinal(true);
			} catch (NumberFormatException nfe) {
				throw(new Exception("Manually set N must be an integer from 0 to 63"));
			}
		}
	}

	/**
	** Returns the number of arguments expected after this
	** instruction.
	*/
	public int getArgumentCount() {
		return(argumentCount);
	}

	/**
	**  Returns the total calculated length of the opcode
	**  and its arguments. (NOTE: this may be a guess as
	**  N is being calculated).
	*/
	public long getLength() {
		ParseArgument arg;
		long result;
		switch (opcode) {
			case OPCODE_MOVE:
				result = 8;
				break;
			case OPCODE_ARM:
				result = 8;
				break;
			case OPCODE_WAIT:
				result = 2;
				break;
			case OPCODE_DATA:
				result = 0;
				break;
			default:
				result = 0;
				System.out.println("getLength(): Unknown opcode?!");
				break;
		}
		result += (argumentCount * n);
		return(result);
	}

	/**
	** Returns the current value of N
	*/
	public long getN() {
		return(n);
	}

	/**
	** Sets the current value of N
	*/
	public void setN(long newN) {
		n = newN;
	}

	/**
	** Returns true if N is either manually set or if the
	** "instruction" is DATA.  In both of these cases the
	** N value should not be changed algorithmically.
	*/
	public boolean isNFinal() {
		return(isFinalN);
	}

	/**
	** Sets the boolean determining the status of the N var.
	*/
	public void setNFinal(boolean newFinal) {
		isFinalN = newFinal;
	}

	/**
	** Adds the specified argument to this instruction's 
	** argument list.  GOTCHA: This has no precautions for
	** adding too many or too few arguments.  It relies
	** on the assembler itself to extract the argument
	** count manually from the above routines.
	*/
	public void addArgument(ParseArgument newArg) 
	throws Exception {
		if (opcode == OPCODE_DATA) {
			if (newArg.usesSymbols()) {
				throw(new Exception("Data length cannot use symbols!"));
			}
			//The operand is the length of the data
			setN(newArg.getValue());
			setNFinal(true);
		}
		arguments.addElement(newArg);
	}

	/**
	** Returns the length of the largest argument in the
	** list.  This is used when calculating new values of
	** N.
	*/
	public long getMaxArgument() {
		ParseArgument arg;
		long tmp,result = 0;
		int size,looper;

		size = arguments.size();
		for (looper=0; looper<size; looper++) {
			arg = (ParseArgument) arguments.elementAt(looper);
			tmp = arg.getValue();
			if (tmp > result) result = tmp;
		}
		return(result);
	}

	/**
	** Writes the opcode and its arguments (if applicable)
	** to the bitstream.
	*/
	public void writeBits(BitStream bitStream) 
	throws Exception {
		ParseArgument arg;
		boolean writeArgs=false;
		int looper;
		String tmpString;
		//System.out.println("");
		switch (opcode) {
			case OPCODE_MOVE:
				bitStream.writeBits("00",2);
				tmpString = Long.toBinaryString(n);
				bitStream.writeBits(tmpString,6);
				writeArgs = true;
				break;
			case OPCODE_ARM:
				bitStream.writeBits("10",2);
				tmpString = Long.toBinaryString(n);
				bitStream.writeBits(tmpString,6);
				writeArgs = true;
				break;
			case OPCODE_WAIT:
				bitStream.writeBits("11",2);
				break;
			case OPCODE_DATA:
				for (looper=0; looper<n; looper++) {
					bitStream.writeBit(0);
				}
				break;
		}
		if (writeArgs) {
			for (looper=0; looper<arguments.size(); looper++) {
				arg = (ParseArgument) arguments.elementAt(looper);
				arg.writeBits(bitStream,(int)n);
			}
		}
	}

	/**
	** A simple debug dump to make sure evrything is working as
	** planned (which it is of course! :)
	*/
	public String toString() {
		ParseArgument arg;
		String result;
		int size, looper;

		result = "Opcode: " + text + "  N=" + n + "  ArgCount=" + argumentCount;
		size = arguments.size();
		for (looper=0; looper<size; looper++) {
			arg = (ParseArgument)arguments.elementAt(looper);
			result += " ARG" + looper + "='" + arg.toString() + "'(" + arg.getValue() + ")";
		}

		return(result);
	}
}
