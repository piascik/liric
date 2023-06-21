// MULTBIASImplementation.java
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
 * This class provides the implementation for the MULTBIAS command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision$
 * @see ngat.liric.CALIBRATEImplementation
 */
public class MULTBIASImplementation extends CALIBRATEImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");

	/**
	 * Constructor.
	 */
	public MULTBIASImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.MULTBIAS&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.MULTBIAS";
	}

	/**
	 * This method returns the MULTBIAS command's acknowledge time. We multiply the MULTRUN's exposureCount by
	 * 1 second and add the default acknowledge time (getDefaultAcknowledgeTime).
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see LiricTCPServerConnectionThread#getDefaultAcknowledgeTime
	 * @see CALIBRATEImplementation#MILLISECONDS_PER_SECOND
	 * @see #status
	 * @see #serverConnectionThread
	 * @see MULTBIAS#getNumberExposures
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		MULTBIAS multBiasCommand = (MULTBIAS)command;
		ACK acknowledge = null;
		int exposureCount,ackTime=0;

		exposureCount = multBiasCommand.getNumberExposures();
		liric.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:exposureCount = "+exposureCount);
		ackTime = MILLISECONDS_PER_SECOND*exposureCount;
		liric.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:ackTime = "+ackTime);
		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(ackTime+serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the MULTBIAS command. 
	 * <ul>
	 * <li>clearFitsHeaders is called.
	 * <li>setFitsHeaders is called to get some FITS headers from the properties files and add them to the C layers.
	 * <li>getFitsHeadersFromISS is called to gets some FITS headers from the ISS (RCS). 
	 *     These are sent on to the C layer.
	 * <li>We send a Multbias command to the C layer.
	 * <li>The done object is setup. 
	 * </ul>
	 * @see #testAbort
	 * @see ngat.liric.CALIBRATEImplementation#sendMultbiasCommand
	 * @see ngat.liric.CALIBRATEImplementation#filenameCount
	 * @see ngat.liric.CALIBRATEImplementation#multrunNumber
	 * @see ngat.liric.CALIBRATEImplementation#lastFilename
	 * @see ngat.liric.HardwareImplementation#clearFitsHeaders
	 * @see ngat.liric.HardwareImplementation#setFitsHeaders
	 * @see ngat.liric.HardwareImplementation#getFitsHeadersFromISS
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		MULTBIAS multBiasCommand = (MULTBIAS)command;
		MULTBIAS_DONE multBiasDone = new MULTBIAS_DONE(command.getId());
		int exposureCount;
	
		liric.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Started.");
		if(testAbort(multBiasCommand,multBiasDone) == true)
			return multBiasDone;
		// get multbias data
		exposureCount = multBiasCommand.getNumberExposures();
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:exposureCount = "+exposureCount+".");
		// get fits headers
		clearFitsHeaders();
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from properties.");
		if(setFitsHeaders(multBiasCommand,multBiasDone) == false)
			return multBiasDone;
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from ISS.");
		if(getFitsHeadersFromISS(multBiasCommand,multBiasDone) == false)
			return multBiasDone;
		if(testAbort(multBiasCommand,multBiasDone) == true)
			return multBiasDone;
		// call multbias command
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:Starting Multbias.");
		try
		{
			sendMultbiasCommand(exposureCount);
		}
		catch(Exception e )
		{
			liric.error(this.getClass().getName()+":processCommand:sendMultbiasCommand failed:",e);
			multBiasDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_BASE+2600);
			multBiasDone.setErrorString(this.getClass().getName()+
						   ":processCommand:sendMultbiasCommand failed:"+e);
			multBiasDone.setSuccessful(false);
			return multBiasDone;
		}
		// setup return values.
		// setup multbias done
		multBiasDone.setFilename(lastFilename);
		// standard success values
		multBiasDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_NO_ERROR);
		multBiasDone.setErrorString("");
		multBiasDone.setSuccessful(true);
	// return done object.
		liric.log(Logging.VERBOSITY_VERY_TERSE,this.getClass().getName()+":processCommand:finished.");
		return multBiasDone;
	}
}
