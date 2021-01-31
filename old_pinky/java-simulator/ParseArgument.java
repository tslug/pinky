import java.lang.*;
import java.util.*;

import SymbolTable;
import ParseNode;
import BitStream;

/**
** The ParseArgument class was designed originally to handle
** the parsing, verification, and calculation of the arguments
** passed into the MV and ARM instructions.  It's purpose is
** to essentially provide a mechanism (utilizing the symbol
** table) of converting simple math, constants, and labels
** into solid addresses.
*/
public class ParseArgument extends ParseNode {
	/**
	** The operations vector stores (in order)
	** the math operations contained within
	** the argument and the label names involved
	** in these operations.  All constants are
	** folded into a single value (all math ops
	** are commutative right now).
	*/
	protected Vector operations;

	/**
	** This is just a pointer to the symbol table, for use
	** in address lookups.
	*/
	protected SymbolTable symbols;

	/**
	** This variable stores the folded constant values.
	*/
	long constant;

	/**
	** Create an instance on an argument with the specified text.  The
	** text will be parsed, and if any error is found an exception will
	** be thrown during construction.
	*/
	public ParseArgument(long lineNumber, String argText, SymbolTable table) 
	throws Exception {
		super(lineNumber, argText);
		operations = new Vector();
		symbols = table;
		constant=0;
		checkIt();
	}

	/**
	** Returns 'true' if the argument contains references to the
	** symbol table, and 'false' if otherwise.
	*/
	public boolean usesSymbols() {
		boolean result;
		if (operations.size() > 0) {
			result = true;
		} else {
			result = false;
		}
		return(result);
	}

	/**
	** This method checks the argument text and parses it,
	** verifying its validity and folding constants together
	** as it proceeds.
	*/
	protected void checkIt() throws Exception {
		StringTokenizer strTok;
		String tok, tmpString;
		boolean done;
		Integer tmpInteger;
		int tmpint;
		long tmplong;

		constant = 0;

		strTok = new StringTokenizer(text," \t+-",true);
		while (strTok.hasMoreElements()) {
			tok = strTok.nextToken();

			//done is set to true in the next section
			//  if the token requires either a text
			//  to number conversion or a symbol table
			//  lookup.
			done = false;

			//A quick check to see if this is a math op, but
			//  we must also check to see if it is a 1 digit
			//  number or label.
			if (tok.length() == 1) {
				done = true;

				//This may be an operator
				if (tok.equals("+")) {
					try {
						//If the last element was a math op, we have a
						//  problem!
						tmpString = (String) operations.lastElement();
						if ( (tmpString.equals("+")) || (tmpString.equals("-")) ) {
							throw(new Exception("Operand missing"));
						}

						//We got past the exception, so we are good to go.
						operations.addElement(tok);
					} catch (NoSuchElementException nsee) {
						//ignore it.  This is the first token in
						//  the text, which is a gratuitous '+'. :)
					}
				} else if (tok.equals("-")) {
					try {
						//Same song, different verse.. 
						tmpString = (String) operations.lastElement();
						if ( (tmpString.equals("+")) || (tmpString.equals("-")) ) {
							throw(new Exception("Operand missing"));
						}
						operations.addElement(tok);
					} catch (NoSuchElementException nsee) {
						//Ignore it.  The next token will check this one
						//  and compensate in an appropriate manner.
					}
				} else if ((tok.equals(" "))||(tok.equals("\t"))) {
					//Ignore these characters
				} else {
					//This should be a 1 digit number or symbol, so
					//   make sure we pass this through the next section.
					done = false;
				}
			}
			if (!done) {
				//This must either be an integer or a symbol in
				//  the symbol table
				try {
					//perform some reduction (ie, combine all constants together)
					
					tmpInteger = new Integer(tok);
					tmpint = tmpInteger.intValue();

					//find out what operator is before us, if any
					try {
						tmpString = (String)operations.lastElement();
						if (tmpString.equals("+")) {
							constant += tmpint;
							operations.removeElement(tmpString);
						} else if (tmpString.equals("-")) {
							constant -= tmpint;
							operations.removeElement(tmpString);
						} else {
							throw(new Exception("Operator missing"));
						}
					} catch (NoSuchElementException nsee) {
						constant = tmpint;
					}

				} catch (NumberFormatException nfe) {
					//This better be a symbol then! btw.. I love Java! (Though beer is better)
					if (symbols.isDefined(tok)) {
						try {
							tmpString = (String)operations.lastElement();
							if (tmpString.equals("+")||tmpString.equals("-")) {
								operations.addElement(tok);
							} else {
								throw(new Exception("Operator missing!"));
							}
						} catch (NoSuchElementException nsee) {
							operations.addElement(tok);
						}
					} else {
						throw(new Exception("Undefined Symbol '" + tok + "'"));
					}
				}

			}
		}
		//Make sure that the last element was not an operator
		try {
			tmpString = (String)operations.lastElement();
			if (tmpString.equals("+") || tmpString.equals("-")) {
				throw(new Exception("Operand expected!"));
			}
		} catch (NoSuchElementException nsee) {
			//ignore..
		}

		/* Debug information:
		System.out.print("Operations: " + constant);
		for (tmpint=0; tmpint<operations.size(); tmpint++) {
			tmpString = (String) operations.elementAt(tmpint);
			System.out.print(" " + tmpString);
		}
		System.out.println("");
		*/
	}

	/**
	** This routine will run through the operations and symbol table
	** in an effort to obtain a single address.
	*/
	public long getValue() {
		String tmpString;
		long result,tmpresult;
		int size, looper;
		char op=' ';

		//Start result with the value of the foldeed constants.
		result = constant;

		size = operations.size();
		for (looper=0; looper<size; looper++) {
			//Grab a token
			tmpString = (String) operations.elementAt(looper);

			//Is this token an operator?
			if (tmpString.equals("+") || tmpString.equals("-")) {
				//save the operator into a temp variable
				op = tmpString.charAt(0);
			} else {
				//This is a label.. Look it up and operate on
				//  it via the saved operator if available.
				try {
					tmpresult = symbols.getSymbol(tmpString);
					switch (op) {
						case ' ':
						case '+':
							result += tmpresult;
							break;
						case '-':
							result -= tmpresult;
							break;
						default:
							System.out.println("Default case");
							break;
					}
				} catch (Exception e) {
					System.out.println("Oops!");
					e.printStackTrace();
				}
			}
		}
		return(result);
	}

	/**
	**  Write the argument out to disk.  In application, the
	**  length of the argument should be exactly the same as
	** the value of N for its corresponding instruction.
	*/
	public void writeBits(BitStream bitStream, int length) 
	throws Exception {
		String tmpString;

		tmpString = Long.toBinaryString(getValue());
		//System.out.print("(" + getValue() + ")");

		bitStream.writeBits(tmpString, length);
	}
}
