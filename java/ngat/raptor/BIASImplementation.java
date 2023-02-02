// BIASImplementation.java
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
 * This class provides the implementation for the BIAS command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision$
 * @see ngat.raptor.CALIBRATEImplementation
 */
public class BIASImplementation extends CALIBRATEImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");

	/**
	 * Constructor.
	 */
	public BIASImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.BIAS&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.BIAS";
	}

	/**
	 * This method returns the BIAS command's acknowledge time. We allow
	 * 1 second for the bias and add the default acknowledge time (getDefaultAcknowledgeTime).
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see RaptorTCPServerConnectionThread#getDefaultAcknowledgeTime
	 * @see CALIBRATEImplementation#MILLISECONDS_PER_SECOND
	 * @see #status
	 * @see #serverConnectionThread
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		ACK acknowledge = null;
		int ackTime=0;

		ackTime = MILLISECONDS_PER_SECOND;
		raptor.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:ackTime = "+ackTime);
		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(ackTime+serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the BIAS command. 
	 * <ul>
	 * <li>clearFitsHeaders is called.
	 * <li>setFitsHeaders is called to get some FITS headers from the properties files and add them to the C layers.
	 * <li>getFitsHeadersFromISS is called to gets some FITS headers from the ISS (RCS). 
	 *     These are sent on to the C layer.
	 * <li>We send a Multbias command to the C layer, with exposure count 1.
	 * <li>The done object is setup. 
	 * </ul>
	 * @see #testAbort
	 * @see ngat.raptor.CALIBRATEImplementation#sendMultbiasCommand
	 * @see ngat.raptor.CALIBRATEImplementation#filenameCount
	 * @see ngat.raptor.CALIBRATEImplementation#multrunNumber
	 * @see ngat.raptor.CALIBRATEImplementation#lastFilename
	 * @see ngat.raptor.HardwareImplementation#clearFitsHeaders
	 * @see ngat.raptor.HardwareImplementation#setFitsHeaders
	 * @see ngat.raptor.HardwareImplementation#getFitsHeadersFromISS
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		BIAS biasCommand = (BIAS)command;
		BIAS_DONE biasDone = new BIAS_DONE(command.getId());
	
		raptor.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Started.");
		if(testAbort(biasCommand,biasDone) == true)
			return biasDone;
		// get fits headers
		clearFitsHeaders();
		raptor.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from properties.");
		if(setFitsHeaders(biasCommand,biasDone) == false)
			return biasDone;
		raptor.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from ISS.");
		if(getFitsHeadersFromISS(biasCommand,biasDone) == false)
			return biasDone;
		if(testAbort(biasCommand,biasDone) == true)
			return biasDone;
		// call multbias command
		raptor.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:Starting Multbias with exposure count 1.");
		try
		{
			sendMultbiasCommand(1);
		}
		catch(Exception e )
		{
			raptor.error(this.getClass().getName()+":processCommand:sendMultbiasCommand failed:",e);
			biasDone.setErrorNum(RaptorConstants.RAPTOR_ERROR_CODE_BASE+700);
			biasDone.setErrorString(this.getClass().getName()+
						   ":processCommand:sendMultbiasCommand failed:"+e);
			biasDone.setSuccessful(false);
			return biasDone;
		}
		// setup return values.
		// setup multbias done
		biasDone.setFilename(lastFilename);
		// standard success values
		biasDone.setErrorNum(RaptorConstants.RAPTOR_ERROR_CODE_NO_ERROR);
		biasDone.setErrorString("");
		biasDone.setSuccessful(true);
	// return done object.
		raptor.log(Logging.VERBOSITY_VERY_TERSE,this.getClass().getName()+":processCommand:finished.");
		return biasDone;
	}
}
