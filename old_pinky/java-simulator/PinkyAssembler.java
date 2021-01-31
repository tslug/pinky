import java.lang.*;
import java.util.*;
import java.io.*;

import ParseInstruction;
import ParseNode;
import PinkyTokenizer;
import BitStream;

/**
** This is the meat of the pinky assembler.  I originally designed
** this class so that it could be included into another program
** without too many headaches, but somewhere along the line laziness
** set in and I started outputting stuff to standard out.  Oh well..
** I'll fix it when I get some spare cycles.  Oh yeah, this class
** parses pinky ASM and produces bit-binaries! :)
*/
public class PinkyAssembler {
	/**
	** This is the structure I use to hold onto
	** symbol table information.
	*/
	protected class SymbolNode {
		public String text;
		
		//this is the actual bit address of the label
		public long address;
	}

	/**
	** This is an ordered list of the actual instructions.
	** The arguments to these functions are attached to the
	** "nodes" in this list, as are the labels.
	*/
	protected Vector instructions;

	/**
	** This is an error message storage facility.  This keeps all
	** error messages away from the screen for use when including
	** this assembler as part of a larger mechanism.  I should do
	** the same thing with the standard out...
	*/
	protected Vector errorMessages;

	/**
	** This keeps track of all symbols defined, and holds onto
	** their associated bit addresses.
	*/
	protected SymbolTable symbols;

	/**
	** Construct stuff.. general stuff here.
	*/
	public PinkyAssembler() {
		instructions = new Vector();
		errorMessages = new Vector();
		symbols = new SymbolTable();
	}

	/**
	** Adds an error string to the end of the error list. Used
	** to automatically include other information (which was
	** later stripperd back out).  Probably not too usefull
	** for anything but abstraction now. :P
	*/
	protected void addError(String errorMessage) {
		errorMessages.addElement(errorMessage);
	}

	/**
	** This method dumps out a list of the instructions in their
	** current state, even if that state is that of being half-
	** parsed. 
	*/
	public void printInstructions() {
		int size,looper;
		ParseInstruction inst;
		size = instructions.size();
		for (looper=0; looper<size; looper++) {
			inst = (ParseInstruction) instructions.elementAt(looper);
			System.out.println(inst.toString());
		}
		
	}

	/**
	** Prints out all labels and their bit addresses to the screen. Good
	** debug info! :)
	*/
	public void printLabels() {
		System.out.println("==== LABELS ==============================");
		symbols.dump();
		System.out.println("");
	}

	/**
	** This method prints all errors generated to the screen.
	** If no errors were encountered it returns 'false'. 'true'
	** otherwise.
	*/
	public boolean printErrors() {
		int looper, count;
		String tmpString;
		boolean result;

		count = errorMessages.size();
		if (count > 0) {
			System.out.println("ASSEMBLE ERRORS:");
			for (looper=0; looper<count; looper++) {
				tmpString = (String) errorMessages.elementAt(looper);
				System.err.println(tmpString);
			}
			result = true;
		} else {
			result = false;
		}
		return(result);
	}

	/**
	** This method will take in any binary value and calculate
	** the minimum number of bits needed to store it.  In other
	** words it shifts the bits until it gets a 0 result. :)
	*/
	protected int calculateBestN(long value) {
		long tmpval = value;
		int n=0;
		while (tmpval > 0) {
			tmpval >>= 1;
			n++;
		}
		if (n <= 0) n=1;
		return(n);
	}

	/**
	** This method builds the initial symbol table, setting all
	** symbols to their worst-case scenario.  It cant be smart 
	** about it since it doesnt know how the labels interact yet.
	*/
	protected void buildSymbolTable(Vector tokens) {
		PinkyTokenizer.Token tok;
		int size,looper,idx;
		String tmpString;

		size=tokens.size();
		for (looper=0; looper<size; looper++) {
			tok = (PinkyTokenizer.Token) tokens.elementAt(looper);
			idx = tok.str.indexOf(":");
			if (idx >= 0) {
				tmpString=tok.str.substring(0,idx);
				symbols.setSymbol(tmpString, 0);
			}
		}
	}

	/**
	** The first "smart" pass! This method recognizes that the worst
	** possible N is the value of the last program address (assuming
	** that jumping out of local code is not possible).
	*/
	protected void buildInitialSymbolGuesses() {
		String tmpString;
		ParseInstruction inst;
		Vector labels;
		int size, looper, labelLooper;
		long tmpval,progptr=0;
		

		size = instructions.size();
		for (looper=0; looper<size; looper++) {
			inst = (ParseInstruction) instructions.elementAt(looper);
			//grab all labels and assign this program pointer to them
			labels = inst.getLabels();
			if (labels != null) {
				for (labelLooper=0; labelLooper<labels.size(); labelLooper++) {
					tmpString = (String) labels.elementAt(labelLooper);
					symbols.setSymbol(tmpString,progptr);
				}
			}
			//increment the program counter by the wort case size:
			tmpval = inst.getLength();
			progptr = progptr + tmpval;
		}
	}

	/**
	** The "nifty" portion of the assembler.  This method looks sequentially
	** through all the instructions checking to see whether or not the value
	** of N currently stored is bigger than it needs to be to address the
	** value of its arguments.  If it is too big, it reduces the value of N
	** to the most optimal size and starts the process over again.  This
	** makes sure that changes later in the instruction set will still influence
	** the first instructions.  Can anyone else thing of a more mathematical/
	** elegant way to do this?  For small pieces of code, the pass count can
	** get pretty high (though it still runs fast).
	*/
	protected boolean optimizationPass() {
		ParseInstruction inst;
		int size, looper;
		long tmpval, oldN, newN, progptr=0;
		long oldLength, newLength;
		boolean result=false;
		

		size = instructions.size();
		for (looper=0; (looper<size)&&(result==false); looper++) {
			inst = (ParseInstruction) instructions.elementAt(looper);
			//dont change final values!
			if (!inst.isNFinal()) {
				oldN = inst.getN();
				oldLength = inst.getLength();
				tmpval = inst.getMaxArgument();
				newN = calculateBestN(tmpval);
				if (newN < oldN) {
					result = true;
					inst.setN(newN);
					newLength = inst.getLength();
					tmpval = oldLength - newLength;
					//System.out.println("Reducing instruction on line " + inst.getLine() + " by >> " + tmpval + " << bits");
					symbols.recalculate(progptr + 1, -tmpval);
				} else if (newN > oldN) {
					//should this ever happen? I dont think so, but this is a safety.
					System.out.println("Old N was smaller than new N!!!");
				}
			}
			progptr += inst.getLength();
		}
		return(result);
	}

	/**
	** This writes the binary to disk.  When the program gets here it
	** is done being nifty and gets back to nitty-gritty.
	*/
	public void writeToDisk(String binaryFile) {
		BitStream bits;
		ParseInstruction inst;
		int size, looper;

		try {
			bits = new BitStream(binaryFile);
			size = instructions.size();
			for (looper=0; looper<size; looper++) {
				inst = (ParseInstruction) instructions.elementAt(looper);
				inst.writeBits(bits);
			}
			bits.close();
		} catch (Exception e) {
			System.out.println("ERROR! Exception while writing binary!");
			e.printStackTrace();
		}
	}

	/**
	** This method will first use the pinky tokenizer to break up
	** a file into tokens.  It will then try to parse these tokens
	** into the assembly-ish form, with instructions and arguments.
	** If this is successful it will continue to the optimization
	** stage, where the N values will be calculated, and then finally
	** write the program out to disk.
	*/
	public void assemble(String filename, String binaryFile) 
	throws Exception {
		String errors[], tmpString;
		PinkyTokenizer pt;
		PinkyTokenizer.Token tok;
		ParseInstruction newInstruction;
		ParseArgument newArgument;
		Vector tokens;
		Vector labels;
		int looper, expectingArgument=0, idx;
		boolean thereWereErrors;

		//Tokenize the sucker:
		System.out.println("Parsing assembly file...");
		pt = new PinkyTokenizer();
		tokens = pt.getTokens(filename);
		errors = pt.getErrors();
		labels = new Vector();
		newInstruction = null;
		pt = null;

		//If there were any errorsm print them and stop.
		if (errors != null) {
			System.out.println("ERRORS:");
			for (looper=0; looper<errors.length; looper++) {
				System.out.println(errors[looper]);
			}
		} else {
			//Build the barebones, worst case symbol table.
			buildSymbolTable(tokens);

			//build the instruction tree:
			for (looper=0; looper<tokens.size(); looper++) {
				tok = (PinkyTokenizer.Token) tokens.elementAt(looper);
				idx = tok.str.indexOf(":");
				if (idx >= 0) {
					//This is a symbol.. add to our current list to be
					//  passed into the next argument.
					//Oh yeah.. remove the semicolon..
					tmpString = tok.str.substring(0,idx);
					labels.addElement(tmpString);
				} else {
					//are we collecting arguments for an instruction that we
					//  previously read in?
					if (expectingArgument > 0) {
						try {
							newArgument = new ParseArgument(tok.lineNumber, tok.str, symbols);
							newArgument.addLabels(labels);
							labels = new Vector();
							newInstruction.addArgument(newArgument);
						} catch (Exception e) {
							addError("Line " + tok.lineNumber + ": " + e.toString());
						}
						expectingArgument--;
					} else {
						if (newInstruction != null) {
							instructions.addElement(newInstruction);
						}
						try {
							newInstruction = new ParseInstruction(tok.lineNumber, tok.str);
							newInstruction.addLabels(labels);
							labels = new Vector();
							expectingArgument = newInstruction.getArgumentCount();
						} catch (Exception e) {
							addError("Line " + tok.lineNumber + ": " + e.toString());
						}
					}
				}
			}
			//Did we run out of tokens while waiting for arguments?
			if (expectingArgument > 0) {
				addError("Abrupt end of file!");
			} else {
				if (newInstruction != null) {
					instructions.addElement(newInstruction);
				}
			}

			//printInstructions();

			//display any errors:
			thereWereErrors = printErrors();
			if (!thereWereErrors) {
				//No errors, so lets optimize this sucker!
				System.out.println("Parse complete!");
				System.out.println("Calculating N's...");
				buildInitialSymbolGuesses();
				//printLabels();

				looper=0;
				while(optimizationPass()) {
					looper++;
					//printLabels();
					//printInstructions();
				}
				System.out.println("Calculations complete! " + looper + " passes were required.");
				printLabels();
				printInstructions();

				writeToDisk(binaryFile);
			}
		}
	}

	/**
	** This is a barebones interface to the assembler that needs some
	** beef added to it.  I wanted to get this thing out there without
	** all the bells and whistles because someone else may be able to
	** use the binary for their simulator, and I wont have too much
	** time to play pinky for a few days.
	*/
	public static void main(String args[]) {
		PinkyAssembler pa;
		pa = new PinkyAssembler();

		if (args.length != 1) {
			System.err.println("USAGE: PinkyAssembler <filename>");
			System.exit(1);
		}

		try {
			pa.assemble(args[0],args[0] + ".bin");
		} catch (FileNotFoundException fnfe) {
			System.err.println("Input file '" + args[0] + "' not found!");
		} catch (IOException ioe) {
			System.err.println("I/O Exception while parsing input file!");
		} catch (Exception e) {
			e.printStackTrace();
		}

		System.exit(0);
	}
}

