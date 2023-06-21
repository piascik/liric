// DARKImplementation.java
// $Id$
package ngat.raptor;

import java.lang.*;
import java.text.*;
import java.util.*;

import ngat.fits.*;
import ngat.phase2.*;
import ngat.raptor.command.*;
import ngat.message.base.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.*;
import ngat.util.logging.*;

/**
 * This class provides the implementation for the DARK command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision$
 * @see ngat.raptor.CALIBRATEImplementation
 */
public class DARKImplementation extends CALIBRATEImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");

	/**
	 * Constructor.
	 */
	public DARKImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.DARK&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.DARK";
	}

	/**
	 * This method returns the DARK command's acknowledge time. We add the DARKS's exposureLength (in milliseconds)
	 * to the default acknowledge time (getDefaultAcknowledgeTime).
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see RaptorTCPServerConnectionThread#getDefaultAcknowledgeTime
	 * @see #status
	 * @see #serverConnectionThread
	 * @see DARK#getExposureTime
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		DARK darkCommand = (DARK)command;
		ACK acknowledge = null;
		int exposureLength,ackTime=0;

		exposureLength = darkCommand.getExposureTime();
		raptor.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:exposureLength = "+exposureLength);
		ackTime = exposureLength;
		raptor.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:ackTime = "+ackTime);
		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(ackTime+serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the DARK command. 
	 * <ul>
	 * <li>clearFitsHeaders is called.
	 * <li>setFitsHeaders is called to get some FITS headers from the properties files and add them to the C layers.
	 * <li>getFitsHeadersFromISS is called to gets some FITS headers from the ISS (RCS). 
	 *     These are sent on to the C layer.
	 * <li>We send a Multdark command to the C layer, with an exposure count of 1.
	 * <li>The done object is setup. 
	 * </ul>
	 * @see #testAbort
	 * @see ngat.raptor.CALIBRATEImplementation#sendMultdarkCommand
	 * @see ngat.raptor.CALIBRATEImplementation#filenameCount
	 * @see ngat.raptor.CALIBRATEImplementation#multrunNumber
	 * @see ngat.raptor.CALIBRATEImplementation#lastFilename
	 * @see ngat.raptor.HardwareImplementation#clearFitsHeaders
	 * @see ngat.raptor.HardwareImplementation#setFitsHeaders
	 * @see ngat.raptor.HardwareImplementation#getFitsHeadersFromISS
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		DARK darkCommand = (DARK)command;
	        DARK_DONE darkDone = new DARK_DONE(command.getId());
		int exposureLength;
	
		raptor.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Started.");
		if(testAbort(darkCommand,darkDone) == true)
			return darkDone;
		// get dark data
		exposureLength = darkCommand.getExposureTime();
		raptor.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:exposureLength = "+exposureLength+".");
		// get fits headers
		clearFitsHeaders();
		raptor.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from properties.");
		if(setFitsHeaders(darkCommand,darkDone) == false)
			return darkDone;
		raptor.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from ISS.");
		if(getFitsHeadersFromISS(darkCommand,darkDone) == false)
			return darkDone;
		if(testAbort(darkCommand,darkDone) == true)
			return darkDone;
		// call multdark command
		raptor.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:Starting Multdark with an exposure count of 1.");
		try
		{
			sendMultdarkCommand(exposureLength,1);
		}
		catch(Exception e )
		{
			raptor.error(this.getClass().getName()+":processCommand:sendMultdarkCommand failed:",e);
			darkDone.setErrorNum(RaptorConstants.RAPTOR_ERROR_CODE_BASE+600);
			darkDone.setErrorString(this.getClass().getName()+
						   ":processCommand:sendMultdarkCommand failed:"+e);
			darkDone.setSuccessful(false);
			return darkDone;
		}
		// setup return values.
		// setup multdark done
		darkDone.setFilename(lastFilename);
		// standard success values
		darkDone.setErrorNum(RaptorConstants.RAPTOR_ERROR_CODE_NO_ERROR);
		darkDone.setErrorString("");
		darkDone.setSuccessful(true);
	// return done object.
		raptor.log(Logging.VERBOSITY_VERY_TERSE,this.getClass().getName()+":processCommand:finished.");
		return darkDone;
	}
}
