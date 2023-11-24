// HardwareImplementation.java
// $Id$
package ngat.liric;

import java.lang.*;
import java.text.*;
import java.util.*;

import ngat.fits.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.*;
import ngat.liric.command.*;
import ngat.util.logging.*;

/**
 * This class provides some common hardware related routines to move folds, and FITS
 * interface routines needed by many command implementations
 * @version $Revision$
 */
public class HardwareImplementation extends CommandImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");

	/**
	 * This method calls the super-classes method.
	 * @param command The command to be implemented.
	 */
	public void init(COMMAND command)
	{
		super.init(command);
	}
	
	/**
	 * This method is used to calculate how long an implementation of a command is going to take, so that the
	 * client has an idea of how long to wait before it can assume the server has died.
	 * @param command The command to be implemented.
	 * @return The time taken to implement this command, or the time taken before the next acknowledgement
	 * is to be sent.
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		return super.calculateAcknowledgeTime(command);
	}

	/**
	 * This routine performs the generic command implementation.
	 * @param command The command to be implemented.
	 * @return The results of the implementation of this command.
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		return super.processCommand(command);
	}

	/**
	 * This routine tries to move the mirror fold to a certain location, by issuing a MOVE_FOLD command
	 * to the ISS. The position to move the fold to is specified by the liric property file.
	 * If an error occurs the done objects field's are set accordingly.
	 * @param command The command being implemented that made this call to the ISS. This is used
	 * 	for error logging.
	 * @param done A COMMAND_DONE subclass specific to the command being implemented. If an
	 * 	error occurs the relevant fields are filled in with the error.
	 * @return The routine returns a boolean to indicate whether the operation was completed
	 *  	successfully.
	 * @see LiricStatus#getPropertyInteger
	 * @see Liric#sendISSCommand
	 */
	public boolean moveFold(COMMAND command,COMMAND_DONE done)
	{
		INST_TO_ISS_DONE instToISSDone = null;
		MOVE_FOLD moveFold = null;
		int mirrorFoldPosition = 0;

		moveFold = new MOVE_FOLD(command.getId());
		try
		{
			mirrorFoldPosition = status.getPropertyInteger("liric.mirror_fold_position");
		}
		catch(NumberFormatException e)
		{
			mirrorFoldPosition = 0;
			liric.error(this.getClass().getName()+":moveFold:"+
				command.getClass().getName(),e);
			done.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_BASE+1201);
			done.setErrorString("moveFold:"+e);
			done.setSuccessful(false);
			return false;
		}
		moveFold.setMirror_position(mirrorFoldPosition);
		instToISSDone = liric.sendISSCommand(moveFold,serverConnectionThread);
		if(instToISSDone.getSuccessful() == false)
		{
			liric.error(this.getClass().getName()+":moveFold:"+
				command.getClass().getName()+":"+instToISSDone.getErrorString());
			done.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_BASE+1202);
			done.setErrorString(instToISSDone.getErrorString());
			done.setSuccessful(false);		
			return false;
		}
		return true;
	}

	/**
	 * This routine clears the current set of FITS headers. 
	 */
	public void clearFitsHeaders()
	{
	}

	/**
	 * This routine gets a set of FITS header from a config file. The retrieved FITS headers are added to the 
	 * C layer. The "liric.fits.keyword.&lt;n&gt;" properties is queried in ascending order
	 * of &lt;n&gt; to find keywords.
	 * The "liric.fits.value.&lt;keyword&gt;" property contains the value of the keyword.
	 * The value's type is retrieved from the property "liric.fits.value.type.&lt;keyword&gt;", 
	 * which should comtain one of the following values: boolean|float|integer|string.
	 * The addFitsHeader method is then called to actually add the FITS header to the C layer.
	 * @param command The command being implemented that made this call to the ISS. This is used
	 * 	for error logging.
	 * @param commandDone A COMMAND_DONE subclass specific to the command being implemented. If an
	 * 	error occurs the relevant fields are filled in with the error.
	 * @return The routine returns a boolean to indicate whether the operation was completed
	 *  	successfully.
	 * @see #addFitsHeader
	 */
	public boolean setFitsHeaders(COMMAND command,COMMAND_DONE commandDone)
	{
		String keyword = null;
		String typeString = null;
		String valueString = null;
		boolean done;
		double dvalue;
		int index,ivalue;
		boolean bvalue;

		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":setFitsHeaders:Started.");
		index = 0;
		done = false;
		while(done == false)
		{
			liric.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
				   ":setFitsHeaders:Looking for keyword index "+index+" in list.");
			keyword = status.getProperty("liric.fits.keyword."+index);
			if(keyword != null)
			{
				typeString = status.getProperty("liric.fits.value.type."+keyword);
				if(typeString == null)
				{
					liric.error(this.getClass().getName()+
						     ":setFitsHeaders:Failed to get value type for keyword:"+keyword);
					commandDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_BASE+1203);
					commandDone.setErrorString(this.getClass().getName()+
						     ":setFitsHeaders:Failed to get value type for keyword:"+keyword);
					commandDone.setSuccessful(false);
					return false;
				}
				// Add FITS header
				try
				{
					if(typeString.equals("string"))
					{
						valueString = status.getProperty("liric.fits.value."+keyword);
						addFitsHeader(keyword,valueString);
					}
					else if(typeString.equals("integer"))
					{
						Integer iov = null;
						
						ivalue = status.getPropertyInteger("liric.fits.value."+keyword);
						iov = new Integer(ivalue);
						addFitsHeader(keyword,iov);
					}
					else if(typeString.equals("float"))
					{
						Float fov = null;
							
						dvalue = status.getPropertyDouble("liric.fits.value."+keyword);
						fov = new Float(dvalue);
						addFitsHeader(keyword,fov);
					}
					else if(typeString.equals("boolean"))
					{
						Boolean bov = null;
						
						bvalue = status.getPropertyBoolean("liric.fits.value."+keyword);
						bov = new Boolean(bvalue);
						addFitsHeader(keyword,bov);
					}
					else
					{
						liric.error(this.getClass().getName()+
							     ":setFitsHeaders:Unknown value type "+typeString+
							     " for keyword:"+keyword);
						commandDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_BASE+
									1204);
						commandDone.setErrorString(this.getClass().getName()+
									   ":setFitsHeaders:Unknown value type "+
									   typeString+" for keyword:"+keyword);
						commandDone.setSuccessful(false);
						return false;
					}
				}
				catch(Exception e)
				{
					liric.error(this.getClass().getName()+
						     ":setFitsHeaders:Failed to add value for keyword:"+
						     keyword,e);
					commandDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_BASE+1206);
					commandDone.setErrorString(this.getClass().getName()+
								   ":setFitsHeaders:Failed to add value for keyword:"+
								   keyword+":"+e);
					commandDone.setSuccessful(false);
					return false;
				}
				// increment index
				index++;
			}
			else
				done = true;
		}// end while
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":setFitsHeaders:Finished.");
		return true;
	}

	/**
	 * This routine tries to get a set of FITS headers for an exposure, by issuing a GET_FITS command
	 * to the ISS. The results from this command are put into the C layers list of FITS headers by calling
	 * addISSFitsHeaderList.
	 * If an error occurs the done objects field's can be set to record the error.
	 * @param command The command being implemented that made this call to the ISS. This is used
	 * 	for error logging.
	 * @param done A COMMAND_DONE subclass specific to the command being implemented. If an
	 * 	error occurs the relevant fields are filled in with the error.
	 * @return The routine returns a boolean to indicate whether the operation was completed
	 *  	successfully.
	 * @see #addISSFitsHeaderList
	 * @see Liric#sendISSCommand
	 * @see Liric#getStatus
	 * @see LiricStatus#getPropertyInteger
	 */
	public boolean getFitsHeadersFromISS(COMMAND command,COMMAND_DONE done)
	{
		INST_TO_ISS_DONE instToISSDone = null;
		GET_FITS_DONE getFitsDone = null;
		FitsHeaderCardImage cardImage = null;
		Object value = null;
		Vector list = null;
		int orderNumberOffset;

		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":getFitsHeadersFromISS:Started.");
		instToISSDone = liric.sendISSCommand(new GET_FITS(command.getId()),serverConnectionThread);
		if(instToISSDone.getSuccessful() == false)
		{
			liric.error(this.getClass().getName()+":getFitsHeadersFromISS:"+
				     command.getClass().getName()+":"+instToISSDone.getErrorString());
			done.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_BASE+1205);
			done.setErrorString(instToISSDone.getErrorString());
			done.setSuccessful(false);
			return false;
		}
	// Get the returned FITS header information into the FitsHeader object.
		getFitsDone = (GET_FITS_DONE)instToISSDone;
	// extract specific FITS headers and add them to the C layer's list
		list = getFitsDone.getFitsHeader();
		try
		{
			addISSFitsHeaderList(list);
		}
		catch(Exception e)
		{
			liric.error(this.getClass().getName()+
				     ":getFitsHeadersFromISS:addISSFitsHeaderList failed.",e);
			done.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_BASE+1207);
			done.setErrorString(this.getClass().getName()+
					    ":getFitsHeadersFromISS:addISSFitsHeaderList failed:"+e);
			done.setSuccessful(false);
			return false;
		}
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":getFitsHeadersFromISS:finished.");
		return true;
	}

	/**
	 * Try to extract the GET_FITS headers returned from the ISS (RCS),
	 * and pass them onto the C layer.
	 * @param list A Vector of FitsHeaderCardImage instances. These will be passed to the C layer.
	 * @exception Exception Thrown if addFitsHeader fails.
	 * @see #addFitsHeader
	 * @see ngat.fits.FitsHeaderCardImageKeywordComparator
	 * @see ngat.fits.FitsHeaderCardImage
	 */
	protected void addISSFitsHeaderList(List list) throws Exception
	{
		FitsHeaderCardImage cardImage = null;
		String commentString = null;
		String unitsString = null;
		
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":addISSFitsHeaderList:started.");
		// iterate over keywords to copy
		for(int index = 0; index < list.size(); index ++)
		{
			cardImage = (FitsHeaderCardImage)(list.get(index));
			liric.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
				  ":addISSFitsHeaderList:Adding "+cardImage.getKeyword()+" to C layer.");
			addFitsHeader(cardImage.getKeyword(),cardImage.getValue());
			// comment
			commentString = cardImage.getComment();
			if((commentString != null)&&(commentString.length() > 0))
			{
				addFitsHeaderComment(cardImage.getKeyword(),commentString);
			}
			// units
			unitsString = cardImage.getUnits();
			if((unitsString != null)&&(unitsString.length() > 0))
			{
				addFitsHeaderUnits(cardImage.getKeyword(),unitsString);
			}
		}// end for
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+":addISSFitsHeaderList:finished.");
	}

	/**
	 * Method to add the specified FITS header to the C layers list of FITS headers. 
	 * The C layer machine/port specification is retrieved from the 
	 * "liric.c.hostname" and "liric.c.port_number" properties. 
	 * An instance of FitsHeaderAddCommand is used to transmit the data.
	 * @param keyword The FITS headers keyword.
	 * @param value The FITS headers value - an object of class String,Integer,Float,Double,Boolean,Date.
	 * @exception Exception Thrown if the FitsHeaderAddCommand internally errors, or the return code indicates a
	 *            failure.
	 * @see #status
	 * @see #dateFitsFieldToString
	 * @see ngat.liric.LiricStatus#getProperty
	 * @see ngat.liric.LiricStatus#getPropertyInteger
	 * @see ngat.liric.command.FitsHeaderAddCommand
	 * @see ngat.liric.command.FitsHeaderAddCommand#setAddress
	 * @see ngat.liric.command.FitsHeaderAddCommand#setPortNumber
	 * @see ngat.liric.command.FitsHeaderAddCommand#setCommand
	 * @see ngat.liric.command.FitsHeaderAddCommand#getParsedReplyOK
	 * @see ngat.liric.command.FitsHeaderAddCommand#getReturnCode
	 * @see ngat.liric.command.FitsHeaderAddCommand#getParsedReply
	 */
	protected void addFitsHeader(String keyword,Object value) throws Exception
	{
		FitsHeaderAddCommand addCommand = null;
		int portNumber,returnCode;
		String hostname = null;
		String errorString = null;

		if(keyword == null)
		{
			throw new NullPointerException(this.getClass().getName()+":addFitsHeader:keyword was null.");
		}
		if(value == null)
		{
			throw new NullPointerException(this.getClass().getName()+
						       ":addFitsHeader:value was null for keyword:"+keyword);
		}
		addCommand = new FitsHeaderAddCommand();
		// configure C comms
		hostname = status.getProperty("liric.c.hostname");
		portNumber = status.getPropertyInteger("liric.c.port_number");
		addCommand.setAddress(hostname);
		addCommand.setPortNumber(portNumber);
		// set command parameters
		if(value instanceof String)
		{
			liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":addFitsHeader:Adding keyword "+keyword+" with String value "+value+".");
			addCommand.setCommand(keyword,(String)value);
		}
		else if(value instanceof Integer)
		{
			liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":addFitsHeader:Adding keyword "+keyword+" with integer value "+
				   ((Integer)value).intValue()+".");
			addCommand.setCommand(keyword,((Integer)value).intValue());
		}
		else if(value instanceof Float)
		{
			liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":addFitsHeader:Adding keyword "+keyword+" with float value "+
				   ((Float)value).doubleValue()+".");
			addCommand.setCommand(keyword,((Float)value).doubleValue());
		}
		else if(value instanceof Double)
		{
			liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":addFitsHeader:Adding keyword "+keyword+" with double value "+
				   ((Double)value).doubleValue()+".");
		        addCommand.setCommand(keyword,((Double)value).doubleValue());
		}
		else if(value instanceof Boolean)
		{
			liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":addFitsHeader:Adding keyword "+keyword+" with boolean value "+
				   ((Boolean)value).booleanValue()+".");
			addCommand.setCommand(keyword,((Boolean)value).booleanValue());
		}
		else if(value instanceof Date)
		{
			liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
				   ":addFitsHeader:Adding keyword "+keyword+" with date value "+
				   dateFitsFieldToString((Date)value)+".");
			addCommand.setCommand(keyword,dateFitsFieldToString((Date)value));
		}
		else
		{
			throw new IllegalArgumentException(this.getClass().getName()+
							   ":addFitsHeader:value had illegal class:"+
							   value.getClass().getName());
		}
		// actually send the command to the C layer
		addCommand.sendCommand();
		// check the parsed reply
		if(addCommand.getParsedReplyOK() == false)
		{
			returnCode = addCommand.getReturnCode();
			errorString = addCommand.getParsedReply();
			liric.log(Logging.VERBOSITY_TERSE,"addFitsHeader:Command failed with return code "+
				   returnCode+" and error string:"+errorString);
			throw new Exception(this.getClass().getName()+
					    ":addFitsHeader:Command failed with return code "+returnCode+
					    " and error string:"+errorString);
		}
	}

	/**
	 * Method to add a comment to a specified FITS header keyword in the C layers list of
	 * FITS headers. The C layer machine/port specification is retrieved from the "liric.c.hostname"
	 * and "liric.c.port_number" properties. An instance of FitsHeaderAddCommand is used to transmit the data.
	 * @param keyword The FITS headers keyword. This should have previously been added to the C layers list of
	 *        FITS headers.
	 * @param comment The comment to associate with the FITS keyword.
	 * @exception Exception Thrown if the FitsHeaderAddCommand internally errors, or the return code indicates a
	 *            failure.
	 * @see #status
	 * @see ngat.liric.LiricStatus#getProperty
	 * @see ngat.liric.LiricStatus#getPropertyInteger
	 * @see ngat.liric.command.FitsHeaderAddCommand
	 * @see ngat.liric.command.FitsHeaderAddCommand#setAddress
	 * @see ngat.liric.command.FitsHeaderAddCommand#setPortNumber
	 * @see ngat.liric.command.FitsHeaderAddCommand#setCommentCommand
	 * @see ngat.liric.command.FitsHeaderAddCommand#getParsedReplyOK
	 * @see ngat.liric.command.FitsHeaderAddCommand#getReturnCode
	 * @see ngat.liric.command.FitsHeaderAddCommand#getParsedReply
	 */
	protected void addFitsHeaderComment(String keyword,String comment) throws Exception
	{
		FitsHeaderAddCommand addCommand = null;
		int portNumber,returnCode;
		String hostname = null;
		String errorString = null;

		if(keyword == null)
		{
			throw new NullPointerException(this.getClass().getName()+
						       ":addFitsHeaderComment:keyword was null.");
		}
		if(comment == null)
		{
			throw new NullPointerException(this.getClass().getName()+
						       ":addFitsHeaderComment:comment was null for keyword:"+keyword);
		}
		addCommand = new FitsHeaderAddCommand();
		// configure C comms
		hostname = status.getProperty("liric.c.hostname");
		portNumber = status.getPropertyInteger("liric.c.port_number");
		addCommand.setAddress(hostname);
		addCommand.setPortNumber(portNumber);
		// set command parameters
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			  ":addFitsHeaderComment:Adding comment '"+comment+"' to keyword "+keyword+".");
		addCommand.setCommentCommand(keyword,comment);
		// actually send the command to the C layer
		addCommand.sendCommand();
		// check the parsed reply
		if(addCommand.getParsedReplyOK() == false)
		{
			returnCode = addCommand.getReturnCode();
			errorString = addCommand.getParsedReply();
			liric.log(Logging.VERBOSITY_TERSE,"addFitsHeaderComment:Command failed with return code "+
				  returnCode+" and error string:"+errorString);
			throw new Exception(this.getClass().getName()+
					    ":addFitsHeaderComment:Command failed with return code "+returnCode+
					    " and error string:"+errorString);
		}
	}

	/**
	 * Method to add a units string to a specified FITS header keyword in the C layers list of
	 * FITS headers. The C layer machine/port specification is retrieved from the "liric.c.hostname"
	 * and "liric.c.port_number" properties. An instance of FitsHeaderAddCommand is used to transmit the data.
	 * @param keyword The FITS headers keyword. This should have previously been added to the C layers list of
	 *        FITS headers.
	 * @param units The units string to associate with the FITS keyword.
	 * @exception Exception Thrown if the FitsHeaderAddCommand internally errors, or the return code indicates a
	 *            failure.
	 * @see #status
	 * @see ngat.liric.LiricStatus#getProperty
	 * @see ngat.liric.LiricStatus#getPropertyInteger
	 * @see ngat.liric.command.FitsHeaderAddCommand
	 * @see ngat.liric.command.FitsHeaderAddCommand#setAddress
	 * @see ngat.liric.command.FitsHeaderAddCommand#setPortNumber
	 * @see ngat.liric.command.FitsHeaderAddCommand#setUnitsCommand
	 * @see ngat.liric.command.FitsHeaderAddCommand#getParsedReplyOK
	 * @see ngat.liric.command.FitsHeaderAddCommand#getReturnCode
	 * @see ngat.liric.command.FitsHeaderAddCommand#getParsedReply
	 */
	protected void addFitsHeaderUnits(String keyword,String units) throws Exception
	{
		FitsHeaderAddCommand addCommand = null;
		int portNumber,returnCode;
		String errorString = null;
		String hostname = null;

		if(keyword == null)
		{
			throw new NullPointerException(this.getClass().getName()+
						       ":addFitsHeaderUnits:keyword was null.");
		}
		if(units == null)
		{
			throw new NullPointerException(this.getClass().getName()+
						       ":addFitsHeaderUnits:units was null for keyword:"+keyword);
		}
		addCommand = new FitsHeaderAddCommand();
		// configure C comms
		hostname = status.getProperty("liric.c.hostname");
		portNumber = status.getPropertyInteger("liric.c.port_number");
		addCommand.setAddress(hostname);
		addCommand.setPortNumber(portNumber);
		// set command parameters
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			  ":addFitsHeaderUnits:Adding units '"+units+"' to keyword "+keyword+".");
		addCommand.setUnitsCommand(keyword,units);
		// actually send the command to the C layer
		addCommand.sendCommand();
		// check the parsed reply
		if(addCommand.getParsedReplyOK() == false)
		{
			returnCode = addCommand.getReturnCode();
			errorString = addCommand.getParsedReply();
			liric.log(Logging.VERBOSITY_TERSE,"addFitsHeaderUnits:Command failed with return code "+
				  returnCode+" and error string:"+errorString);
			throw new Exception(this.getClass().getName()+
					    ":addFitsHeaderUnits:Command failed with return code "+returnCode+
					    " and error string:"+errorString);
		}
	}

	/**
	 * This routine takes a Date, and formats a string to the correct FITS format for that date and returns it.
	 * The format should be 'CCYY-MM-DDThh:mm:ss[.sss...]'.
	 * @param date The date to return a string for.
	 * @return Returns a String version of the date in the correct new FITS format.
	 */
	private String dateFitsFieldToString(Date date)
	{
		Calendar calendar = Calendar.getInstance();
		NumberFormat numberFormat = NumberFormat.getInstance();

		numberFormat.setMinimumIntegerDigits(2);
		calendar.setTime(date);
		return new String(calendar.get(Calendar.YEAR)+"-"+
			numberFormat.format(calendar.get(Calendar.MONTH)+1)+"-"+
			numberFormat.format(calendar.get(Calendar.DAY_OF_MONTH))+"T"+
			numberFormat.format(calendar.get(Calendar.HOUR_OF_DAY))+":"+
			numberFormat.format(calendar.get(Calendar.MINUTE))+":"+
			numberFormat.format(calendar.get(Calendar.SECOND))+"."+
			calendar.get(Calendar.MILLISECOND));
	}
}
