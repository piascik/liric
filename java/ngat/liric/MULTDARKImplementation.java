// MULTDARKImplementation.java
// $Id$
package ngat.liric;

import java.lang.*;
import java.text.*;
import java.util.*;

import ngat.fits.*;
import ngat.phase2.*;
import ngat.liric.command.*;
import ngat.message.base.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.*;
import ngat.util.logging.*;

/**
 * This class provides the implementation for the MULTDARK command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision$
 * @see ngat.liric.CALIBRATEImplementation
 */
public class MULTDARKImplementation extends CALIBRATEImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");

	/**
	 * Constructor.
	 */
	public MULTDARKImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.MULTDARK&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.MULTDARK";
	}

	/**
	 * This method returns the MULTDARKs command's acknowledge time. We multiply the MULTDRKS's exposureCount by
	 * it's exposureLength (in milliseconds) and add the default acknowledge time (getDefaultAcknowledgeTime).
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see LiricTCPServerConnectionThread#getDefaultAcknowledgeTime
	 * @see #status
	 * @see #serverConnectionThread
	 * @see MULTDARK#getExposureTime
	 * @see MULTDARK#getNumberExposures
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		MULTDARK multDarkCommand = (MULTDARK)command;
		ACK acknowledge = null;
		int exposureLength,exposureCount,ackTime=0;

		exposureLength = multDarkCommand.getExposureTime();
		exposureCount = multDarkCommand.getNumberExposures();
		liric.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:exposureLength = "+exposureLength+
			   " :exposureCount = "+exposureCount);
		ackTime = exposureLength*exposureCount;
		liric.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:ackTime = "+ackTime);
		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(ackTime+serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the MULTDARK command. 
	 * <ul>
	 * <li>clearFitsHeaders is called.
	 * <li>setFitsHeaders is called to get some FITS headers from the properties files and add them to the C layers.
	 * <li>getFitsHeadersFromISS is called to gets some FITS headers from the ISS (RCS). 
	 *     These are sent on to the C layer.
	 * <li>We send a Multdark command to the C layer.
	 * <li>The done object is setup. 
	 * </ul>
	 * @see #testAbort
	 * @see ngat.liric.CALIBRATEImplementation#sendMultdarkCommand
	 * @see ngat.liric.CALIBRATEImplementation#filenameCount
	 * @see ngat.liric.CALIBRATEImplementation#multrunNumber
	 * @see ngat.liric.CALIBRATEImplementation#lastFilename
	 * @see ngat.liric.HardwareImplementation#clearFitsHeaders
	 * @see ngat.liric.HardwareImplementation#setFitsHeaders
	 * @see ngat.liric.HardwareImplementation#getFitsHeadersFromISS
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		MULTDARK multDarkCommand = (MULTDARK)command;
		MULTDARK_DONE multDarkDone = new MULTDARK_DONE(command.getId());
		int exposureLength,exposureCount;
	
		liric.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Started.");
		if(testAbort(multDarkCommand,multDarkDone) == true)
			return multDarkDone;
		// get multdark data
		exposureLength = multDarkCommand.getExposureTime();
		exposureCount = multDarkCommand.getNumberExposures();
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:exposureLength = "+exposureLength+
			   " :exposureCount = "+exposureCount+".");
		// get fits headers
		clearFitsHeaders();
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from properties.");
		if(setFitsHeaders(multDarkCommand,multDarkDone) == false)
			return multDarkDone;
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from ISS.");
		if(getFitsHeadersFromISS(multDarkCommand,multDarkDone) == false)
			return multDarkDone;
		if(testAbort(multDarkCommand,multDarkDone) == true)
			return multDarkDone;
		// call multdark command
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:Starting Multdark.");
		try
		{
			sendMultdarkCommand(exposureLength,exposureCount);
		}
		catch(Exception e )
		{
			liric.error(this.getClass().getName()+":processCommand:sendMultdarkCommand failed:",e);
			multDarkDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_BASE+2700);
			multDarkDone.setErrorString(this.getClass().getName()+
						   ":processCommand:sendMultdarkCommand failed:"+e);
			multDarkDone.setSuccessful(false);
			return multDarkDone;
		}
		// setup return values.
		// setup multdark done
		multDarkDone.setFilename(lastFilename);
		// standard success values
		multDarkDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_NO_ERROR);
		multDarkDone.setErrorString("");
		multDarkDone.setSuccessful(true);
	// return done object.
		liric.log(Logging.VERBOSITY_VERY_TERSE,this.getClass().getName()+":processCommand:finished.");
		return multDarkDone;
	}
}
