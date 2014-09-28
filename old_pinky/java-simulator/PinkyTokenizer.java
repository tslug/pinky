import java.lang.*;
import java.util.*;
import java.io.*;


/**
** this class is meant to tokenize the asm file into nice,
** intelligent chunks.  Documentation will most likely be
** fairly parse here..  The only thing you really need to 
** know is that label definitions, opcodes, and arguments 
** are the only things considered to be tokens.  All comments
** are stripped out here as well.
*/
public class PinkyTokenizer {
	/**
	** This class is used to describe and store a line of
	** text from the ASM file.
	*/
	protected class LineNode {
		//These are set when the initial file read is
		//  performed (all other fields are set to 
		//  null or 0...
		public long lineNumber;
		public String text;

		//These are generated when the opcode and arg
		//  list are figured out.
		public Vector parts;
	}

	/**
	** This class is used when communicating with external
	** entities.
	*/
	public class Token {
		long   lineNumber;
		String str;
	}

	protected Vector fileText;
	protected Vector errorMessages;
	protected int commentDepth;	//Used in initial text proccessing pass
	protected long lineCount;


	/**
	** Construct and initialize as normal.
	*/
	public PinkyTokenizer() {
		fileText = null;
		errorMessages = null;
		commentDepth = 0;
		lineCount = 0;
	}

	/**
	** Get rid of extra whitespace and comments.
	*/
	public String cleanString(String str) {
		String result=null;
		char inChars[], outChars[];
		char ch, last;
		int inIndex=0, outIndex=0;

		//This routine cleans a string into a much more
		//  standard package.  All whitespace on either end
		//  is removed, all tabs are converted to spaces,
		//  only one space at a time is allowed, and all
		//  comment standards are supported.  This routine
		//  uses the class member variable called
		//  commentDepth. (i guess java cant do static
		//  vars in functions??)

		if ((str == null)||(str.length()<1)) {
			result = null;
		} else {
			last = ' ';
			inChars = str.toCharArray();
			while (inIndex < inChars.length) {
				ch = inChars[inIndex++];
				//System.out.println("Ch=" + ch + " Last=" + last + " in=" + inIndex + " out=" + outIndex);
				switch (ch) {
					case '/':
						//Check for the "//" case:
						if (last == '/') {
							//Ignore "//" comments if embedded
							//  in a block comment
							if (commentDepth == 0) {
								//back up to the first '/'
								outIndex--;
								//Skip the rest of this line:
								inIndex = inChars.length;
							}
						//check for the "*/" case:
						} else if (last == '*') {
							//Dont output this character sequence
							//  if it is part of a comment.  If it
							//  isn't part of a comment go ahead.
							if (commentDepth > 0) {
								//Decrement our nest variable:
								commentDepth--;
								//We need the lasat character before the
								//  comment, but if there was no last
								//  character make the last character a
								//  space.
								if (outIndex == 0) {
									ch = ' ';
								} else {
									ch = inChars[outIndex - 1];
								}
							} else {
								inChars[outIndex++] = ch;
							}
						//fallback on standard method:
						} else {
							if (commentDepth == 0) {
								inChars[outIndex++]=ch;
							}
						}
						break;
					case '*':
						//Check for the "/*" case:
						if (last == '/') {
							commentDepth++;
							if (commentDepth==1) {
								outIndex--;
							}
						} else {
							if (commentDepth == 0) {
								inChars[outIndex++]=ch;
							}
						}
						break;
					case ';':
					case '#':
						//Various other to-EOL comment chars:
						if (commentDepth == 0) {
							inChars[outIndex] = '\0';
							inIndex = inChars.length;
						}
						break;
					case '\t':
						ch = ' ';
					case ' ':
						if (last == ' ') break;
					default:
						if (commentDepth == 0) {
							inChars[outIndex++]=ch;
						}
						break;
				}
				last = ch;
			}
			//Make sure everything gets null terminated:
			if (outIndex < inChars.length) inChars[outIndex]='\0';

			if (outIndex == 0) {
				//No characters were outputted into the result array
				result = null;
			} else {
				//Make sure the last character isnt a space:
				if (inChars[outIndex - 1] == ' ') inChars[outIndex - 1] = '\0';
				//Allocate an array just big enough for the result:
				if (outIndex == inChars.length) {
					outChars = new char[outIndex + 1];
				} else {
					outChars = new char[outIndex];
				}
				//Copy the result into the new array:
				System.arraycopy(inChars,0,outChars,0,outIndex);
				
				//Set the result to the result char string:
				result=new String(outChars);
			}
		}
		
		return(result);
	}

	/**
	** Add an error to our list.
	*/
	protected void addError(String errorMessage) 
	throws Exception {
		String msg;
		if (errorMessages == null) {
			throw(new Exception("You must call getTokens(filename)!"));
		} else {
			msg = new String(errorMessage);
			errorMessages.addElement(errorMessage);
		}
	}

	/**
	** Read a file, tokenize, etc.
	*/
	protected void readFile(String filename) 
	throws FileNotFoundException, IOException {
		FileReader fileReader;
		BufferedReader reader;
		String newLine;
		long lineNumber=0;
		LineNode ln;

		fileReader = new FileReader(filename);
		reader = new BufferedReader(fileReader);

		while (reader.ready()) {
			newLine = reader.readLine();
			lineNumber++;
			newLine = cleanString(newLine);
			if (newLine != null) {
				ln = new LineNode();
				ln.text = newLine;
				ln.lineNumber = lineNumber;
				lineCount = lineNumber;
				ln.parts = new Vector();
				fileText.addElement(ln);
			}
		}
		reader.close();
		fileReader.close();
	}

	/**
	** Splits a line up into multiple tokens
	*/
	protected void splitLine() 
	throws Exception {
		StringTokenizer strTok;
		String tmpString,partString;
		LineNode ln;
		int looper, size, spaceloc, semiloc;
		int commaloc, tmpint;
		boolean opCodeFound;

		//This routine will split up the entire text line
		//  into an opcode, arguments, and N counter.

		size = fileText.size();
		for (looper=0; looper<size; looper++) {
			ln = (LineNode) fileText.elementAt(looper);
			opCodeFound = false;

			//Start looking at the data and splitting
			//  it up into small sections.. 
			tmpString = ln.text;
			while ((tmpString!=null)&&(tmpString.length() > 0)) {
				semiloc = tmpString.indexOf(":");
				spaceloc = tmpString.indexOf(" ");
				commaloc = tmpString.indexOf(",");
				//System.out.println("Space=" + spaceloc + " Semi=" + semiloc + " Comma=" + commaloc);
				if (opCodeFound) {
					//Break the string apart by semicolons and
					//  commas.
					tmpint=(semiloc >= 0) ? semiloc : commaloc;
					tmpint=(tmpint < commaloc) ? tmpint : commaloc;
				} else {
					//Break the string apart by semicolons and
					//  spaces.
					tmpint=(semiloc >= 0) ? semiloc : spaceloc;
					tmpint=(tmpint < spaceloc) ? tmpint : spaceloc;
					
					if (tmpint == spaceloc) {
						//Ignore spaces after semicolons
						if ((semiloc + 1) != spaceloc) {
							opCodeFound = true;
						}
					} else {
						tmpint++;
					}
				}

				if (tmpint < 0) {
					//Check to see if we have the tail end of the string
					//  which has no delimiters in it:
					if (tmpString.length() > 0) {
						//System.out.println("Part: " + tmpString);
						ln.parts.addElement(tmpString.trim());
						tmpString = null;
					} else {
						addError("Line " + ln.lineNumber + ": Parse error!");
					}
				} else {
					//Grab the interesting section
					if (tmpint == semiloc) {
						partString = tmpString.substring(0,tmpint + 1).trim();
					} else {
						partString = tmpString.substring(0,tmpint).trim();
					}
					//System.out.println("Part: " + partString);
					ln.parts.addElement(partString);

					//Loop again and find the next part
					if (tmpint < tmpString.length()) {
						tmpString = tmpString.substring(tmpint+1, tmpString.length()).trim();
					} else {
						tmpString = null;
					}
					//System.out.println("Remainder: '" + tmpString + "'\n");
				}
			}
		}
	}

	/**
	** Dump some useful debug into to the screen
	*/
	protected void dumpList() {
		String tmpString;
		LineNode ln;
		int looper, size, partLooper;
		//Dumps the text line list (the Vector)
		//  to the screen, showing the partially
		//  parsed data.

		System.out.println("\nCurrent parsed text:");
		size = fileText.size();
		for (looper=0; looper<size; looper++) {
			ln = (LineNode) fileText.elementAt(looper);

			if (ln.parts.size() > 0) {
				System.out.print(ln.lineNumber + ": ");
				for (partLooper=0; partLooper<ln.parts.size(); partLooper++) {
					tmpString = (String) ln.parts.elementAt(partLooper);
					System.out.print("'" + tmpString + "'");
					if (partLooper < (ln.parts.size() - 1)) {
						System.out.print("  ");
					}
				}
			} else {
				System.out.println(ln.lineNumber + ": " + ln.text);
			}
		}
		System.out.println("");

	}

	/**
	** Returns a list of errors encountered during the parse.
	*/
	public String[] getErrors() {
		int looper, count;
		String tmpString;
		String result[];

		count = errorMessages.size();
		if (count > 0) {
			result = new String[count];
			for (looper=0; looper<count; looper++) {
				result[looper] = (String) errorMessages.elementAt(looper);
			}
		} else {
			result = null;
		}
		return(result);
	}

	/**
	** Returns the total number of lines in the file
	*/
	public long getLines() {
		return(lineCount);
	}

	/**
	** Returns a list of the parsed tokens.
	*/
	protected Vector getTokens(String filename) 
	throws Exception {
		String tmpString;
		LineNode ln;
		Token newToken;
		Vector result = new Vector();
		int looper, size, partLooper;

		//This routine jsut creates a stream of tokens for
		//  the assembler to parse.  Basically, all the token
		//  carries with it is a line number.
		fileText = new Vector();
		errorMessages = new Vector();
		commentDepth = 0;

		readFile(filename);
		//dumpList();
		splitLine();

		size = fileText.size();
		for (looper=0; looper<size; looper++) {
			ln = (LineNode) fileText.elementAt(looper);

			for (partLooper=0; partLooper<ln.parts.size(); partLooper++) {
				tmpString = (String) ln.parts.elementAt(partLooper);
				newToken = new Token();
				newToken.lineNumber = ln.lineNumber;
				newToken.str = tmpString.trim();
				result.addElement(newToken);
			}
		}

		fileText = null;
		
		return(result);
	}

	/**
	** used to test this class.
	*/
	public static void main(String args[]) {
		PinkyTokenizer pt;
		Token tok;
		Vector tokens;
		int looper;
		pt = new PinkyTokenizer();

		if (args.length != 1) {
			System.err.println("USAGE: PinkyTokenizer <filename>");
			System.exit(1);
		}

		try {
			tokens = pt.getTokens(args[0]);
			for (looper=0; looper < tokens.size(); looper++) {
				tok = (Token) tokens.elementAt(looper);
				System.out.println("Line " + tok.lineNumber + "  Token: " + tok.str);
			}
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

