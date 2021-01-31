import java.lang.*;
import java.util.Enumeration;
import java.util.Hashtable;

/**
** This class is responsible for storing the values
** assigned to the labels, and for making adjustments
** to them as new N values are calculated.  It is soooo
** nice to just say "I want a hashtable"!
*/
public class SymbolTable {
	/**
	** The meat.  I love Sun!
	*/
	protected Hashtable symbols;

	/**
	** Contruct and init the hashtable
	*/
	public SymbolTable() {
		symbols = new Hashtable();
	}

	/**
	** Returns 'true' is the specified label is defined.
	*/
	public boolean isDefined(String symbol) {
		boolean result;
		Long tmpLong;

		tmpLong = (Long) symbols.get(symbol);
		if (tmpLong == null) result = false;
		else result = true;

		return(result);
	}

	/**
	** Define the specified label to have the speciofied
	** value.  If a previous value existed it is returned,
	** otherwise a 0 is returned. FIX: Would returning a 
	** -1 break anything?
	*/
	public long setSymbol(String symbol, long newValue) {
		long result;
		Long tmpLong;

		tmpLong = (Long) symbols.put(symbol,new Long(newValue));
		if (tmpLong == null) {
			result = 0;
		} else {
			result = tmpLong.longValue();
		}

		return(result);
	}

	/**
	** Returns the value associated with the specified
	** symbol, or throws an exception if the symbol isnt
	** defined.
	*/
	public long getSymbol(String symbol) 
	throws Exception {
		long result;
		Long tmpLong;

		tmpLong = (Long) symbols.get(symbol);
		if (tmpLong == null) {
			throw(new Exception("No such symbol"));
		} else {
			result = tmpLong.longValue();
		}

		return(result);
	}

	/**
	** Adds a specified value to a symbol.
	*/
	public void addToSymbol(String symbol, long value) 
	throws Exception {
		long tmp;
		Long tmpLong;

		tmpLong = (Long) symbols.get(symbol);
		if (tmpLong == null) {
			throw(new Exception("No such symbol"));
		} else {
			tmp = tmpLong.longValue();
			tmp += value;
			tmpLong = new Long(tmp);
			symbols.put(symbol, tmpLong);
		}
	}

	/**
	** Recalculates all symbols defined to be at or after the
	** specified start address. This is used when shifting the
	** label addresses upon a change of an N value.
	*/
	public void recalculate(long startAddress, long change) {
		Enumeration keys,values;
		Long tmpLong;
		String tmpString;
		long tmp;

		keys = symbols.keys();
		values = symbols.elements();
		while (keys.hasMoreElements()) {
			tmpString = (String) keys.nextElement();
			tmpLong = (Long) values.nextElement();
			tmp = tmpLong.longValue();
			if (tmp >= startAddress) {
				tmp += change;
				setSymbol(tmpString, tmp);
			}
		}
	}

	/**
	** Dump the symbols to the screen for debug purposes.
	*/
	public void dump() {
		Enumeration keys,values;
		Long tmpLong;
		String tmpString;
		keys = symbols.keys();
		values = symbols.elements();

		while (keys.hasMoreElements()) {
			tmpString = (String) keys.nextElement();
			tmpLong = (Long) values.nextElement();
			System.out.println(tmpString + ": " + tmpLong.toString());
		}
	}
}
