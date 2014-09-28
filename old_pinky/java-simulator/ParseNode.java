import java.lang.*;
import java.util.Vector;

/**
** A small class to eliminate some redundant code between
** the instructions and the arguments.
*/
public class ParseNode {
	protected long lineNumber;
	protected String text;
	protected Vector labels;

	/**
	** construct this sucker! :P
	*/
	public ParseNode(long line, String instructionText) {
		//Set up some of the basics
		text = instructionText;
		lineNumber = line;
		labels = new Vector();
	}

	/**
	** Returns the line number this object is found on
	*/
	public long getLine() {
		return(lineNumber);
	}

	/**
	** Prepends a label to the beginning of this object.
	** In practice these labels are assigned the value
	** of the current program counter before the object
	** is parsed.
	*/
	public void addLabel(String label) {
		labels.addElement(label);
	}

	/**
	** Same thing, but a bit more useful.
	*/
	public void addLabels(Vector lotsOfLabels) {
		int looper,size;
		size = lotsOfLabels.size();
		for (looper=0; looper<size; looper++) {
			labels.addElement(lotsOfLabels.elementAt(looper));
		}
	}

	/**
	** Returns a vector of all the labels 'attached' to this
	** object.
	*/
	public Vector getLabels() {
		return(labels);
	}

	/**
	** A useless debug dump.  I always extend this method
	** to make it more useful.
	*/
	public String toString() {
		String result;
		result = text;
		return(result);
	}
}
