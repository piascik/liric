// MULTRUNImplementation.java
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
 * This class provides the implementation for the MULTRUN command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision$
 * @see ngat.liric.HardwareImplementation
 */
public class MULTRUNImplementation extends HardwareImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * Return value from C layer: how many FITS files were produced.
	 */
	int filenameCount;
	/**
	 * Return value from C layer: The multrun number of the FITS files.
	 */
	int multrunNumber;
	/**
	 * Return value from C layer: The last FITS filename produced.
	 */
	String lastFilename;

	/**
	 * Constructor.
	 */
	public MULTRUNImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.MULTRUN&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.MULTRUN";
	}

	/**
	 * This method returns the MULTRUN command's acknowledge time. 
         * <ul>
         * <li>We retrieve the exposureLength and exposureCount from the multrunCommand parameters.
         * <li>We retrieve the coaddExposureLength from the Liric status object (where the last CONFIG command implementation cached it).
         * <li>If coaddExposureLength is zero, it has not been set by a previous config. The multrun should fail, but we reset the
	 *     coaddExposureLength to 100 so we don't get a division by zero in this method.
         * <li>We retrieve a coaddReadoutOverhead from the property file  key "liric.coadd.readout.overhead".
         * <li>We retrieve a nudgematicOverhead from the property file  key "liric.nudgematic.overhead".
         * <li>We calculate the total number of coadds over the multrun (exposureLength/coaddExposureLength)*exposureCount.
         * <li>We calculate the total  coadd readout overhead (coaddReadoutOverhead*coaddCount).
         * <li>We calclulate the total nudgematic overhead (nudgematicOverhead*exposureCount).
         * <li>We calculate the total exposure length  (exposureLength*exposureCount);
         * <li>The ack time is set to the total exposure length plus the total coadd readout overhead plus the total nudgematic overhead.
         * </ul>
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see LiricTCPServerConnectionThread#getDefaultAcknowledgeTime
	 * @see #status
	 * @see #serverConnectionThread
	 * @see MULTRUN#getExposureTime
	 * @see MULTRUN#getNumberExposures
	 * @see LiricStatus#getCoaddExposureLength
	 * @see LiricStatus#getPropertyInteger
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		MULTRUN multRunCommand = (MULTRUN)command;
		ACK acknowledge = null;
		int coaddCount,coaddExposureLength,totalExposureLength,exposureLength,exposureCount,ackTime=0;
		int coaddReadoutOverhead,nudgematicOverhead,totalCoaddOverhead,totalNudgematicOverhead;

		exposureLength = multRunCommand.getExposureTime();
		exposureCount = multRunCommand.getNumberExposures();
		coaddExposureLength = status.getCoaddExposureLength();
		// if the coaddExposureLength has not been set by a previous config the multrun command should fail.
		// Nevertheless, set coaddExposureLength to some sensible default to prevent division by zero in this method.
		if(coaddExposureLength == 0)
			coaddExposureLength = 100;
		liric.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:exposureLength = "+exposureLength+
			   " :exposureCount = "+exposureCount+" :coaddExposureLength = "+coaddExposureLength);
		coaddReadoutOverhead = status.getPropertyInteger("liric.coadd.readout.overhead");
		nudgematicOverhead = status.getPropertyInteger("liric.nudgematic.overhead");
		liric.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:coaddReadoutOverhead = "+coaddReadoutOverhead+
			   " :nudgematicOverhead = "+nudgematicOverhead+".");
		coaddCount = (exposureLength/coaddExposureLength)*exposureCount;
		totalCoaddOverhead = coaddReadoutOverhead*coaddCount;
		liric.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:coaddCount = "+coaddCount+
			   " :totalCoaddOverhead = "+totalCoaddOverhead+".");
		totalNudgematicOverhead = nudgematicOverhead*exposureCount;
		totalExposureLength = exposureLength*exposureCount;
		liric.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:totalNudgematicOverhead = "+totalNudgematicOverhead+
			   " :totalExposureLength = "+totalExposureLength+".");
		ackTime = totalExposureLength+totalCoaddOverhead+totalNudgematicOverhead;
		liric.log(Logging.VERBOSITY_VERBOSE,this.getClass().getName()+
			   ":calculateAcknowledgeTime:ackTime = "+ackTime);
		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(ackTime+serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the MULTRUN command. 
	 * <ul>
	 * <li>It moves the fold mirror to the correct location.
	 * <li>clearFitsHeaders is called.
	 * <li>setFitsHeaders is called to get some FITS headers from the properties files and add them to the C layers.
	 * <li>getFitsHeadersFromISS is called to gets some FITS headers from the ISS (RCS). 
	 *     These are sent on to the C layer.
	 * <li>We send a Multrun command to the C layer.
	 * <li>The done object is setup. 
	 * </ul>
	 * @see #testAbort
	 * @see #sendMultrunCommand
	 * @see #filenameCount
	 * @see #multrunNumber
	 * @see #lastFilename
	 * @see ngat.liric.HardwareImplementation#moveFold
	 * @see ngat.liric.HardwareImplementation#clearFitsHeaders
	 * @see ngat.liric.HardwareImplementation#setFitsHeaders
	 * @see ngat.liric.HardwareImplementation#getFitsHeadersFromISS
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		MULTRUN multRunCommand = (MULTRUN)command;
		MULTRUN_DONE multRunDone = new MULTRUN_DONE(command.getId());
		int exposureLength,exposureCount;
		boolean standard;
	
		liric.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Started.");
		if(testAbort(multRunCommand,multRunDone) == true)
			return multRunDone;
	// move the fold mirror to the correct location
		liric.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Moving fold.");
		if(moveFold(multRunCommand,multRunDone) == false)
			return multRunDone;
		if(testAbort(multRunCommand,multRunDone) == true)
			return multRunDone;
		// get multrun data
		exposureLength = multRunCommand.getExposureTime();
		exposureCount = multRunCommand.getNumberExposures();
		standard = multRunCommand.getStandard();
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:exposureLength = "+exposureLength+
			   " :exposureCount = "+exposureCount+" :standard = "+standard+".");
		// get fits headers
		clearFitsHeaders();
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from properties.");
		if(setFitsHeaders(multRunCommand,multRunDone) == false)
			return multRunDone;
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:getting FITS headers from ISS.");
		if(getFitsHeadersFromISS(multRunCommand,multRunDone) == false)
			return multRunDone;
		if(testAbort(multRunCommand,multRunDone) == true)
			return multRunDone;
		// call multrun command
		liric.log(Logging.VERBOSITY_INTERMEDIATE,this.getClass().getName()+
			   ":processCommand:Starting Multrun.");
		try
		{
			sendMultrunCommand(exposureLength,exposureCount,standard);
		}
		catch(Exception e )
		{
			liric.error(this.getClass().getName()+":processCommand:sendMultrunCommand failed:",e);
			multRunDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_BASE+902);
			multRunDone.setErrorString(this.getClass().getName()+
						   ":processCommand:sendMultrunCommand failed:"+e);
			multRunDone.setSuccessful(false);
			return multRunDone;
		}
		// setup return values.
		// setup multrun done
		multRunDone.setFilename(lastFilename);
		// set to some blank value
		multRunDone.setSeeing(0.0f);
		multRunDone.setCounts(0.0f);
		multRunDone.setXpix(0.0f);
		multRunDone.setYpix(0.0f);
		multRunDone.setPhotometricity(0.0f);
		multRunDone.setSkyBrightness(0.0f);
		multRunDone.setSaturation(false);
		// standard success values
		multRunDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_NO_ERROR);
		multRunDone.setErrorString("");
		multRunDone.setSuccessful(true);
	// return done object.
		liric.log(Logging.VERBOSITY_VERY_TERSE,this.getClass().getName()+":processCommand:finished.");
		return multRunDone;
	}
	
	/**
	 * Send a multrun command to the C layer. On success, parseSuccessfulReply is called to extract
	 * data from the reply.
	 * @param exposureLength The total exposure length of each frame in the multrun, in milliseconds
	 *        (itself consisting of the average of a series of frames of the coadd exposure length).
	 * @param exposureCount The number of exposures to do in the multrun.
	 * @param standard A boolean, true if the observation is of a standard, false if it is not.
	 * @exception Exception Thrown if an error occurs.
	 * @see #parseSuccessfulReply
	 * @see ngat.liric.command.MultrunCommand
	 * @see ngat.liric.command.MultrunCommand#setAddress
	 * @see ngat.liric.command.MultrunCommand#setPortNumber
	 * @see ngat.liric.command.MultrunCommand#setCommand
	 * @see ngat.liric.command.MultrunCommand#sendCommand
	 * @see ngat.liric.command.MultrunCommand#getParsedReplyOK
	 * @see ngat.liric.command.MultrunCommand#getReturnCode
	 * @see ngat.liric.command.MultrunCommand#getParsedReply
	 */
	protected void sendMultrunCommand(int exposureLength,int exposureCount,boolean standard) throws Exception
	{
		MultrunCommand command = null;
		int portNumber,returnCode;
		String hostname = null;
		String errorString = null;

		liric.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultrunCommand:exposure length = "+exposureLength+
			   ":exposure count = "+exposureCount+":standard = "+standard+".");
		command = new MultrunCommand();
		// configure C comms
		hostname = status.getProperty("liric.c.hostname");
		portNumber = status.getPropertyInteger("liric.c.port_number");
		command.setAddress(hostname);
		command.setPortNumber(portNumber);
		liric.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultrunCommand:hostname = "+hostname+
			   " :port number = "+portNumber+".");
		// set command parameters
		command.setCommand(exposureLength,exposureCount,standard);
		// actually send the command to the C layer
		command.sendCommand();
		// check the parsed reply
		if(command.getParsedReplyOK() == false)
		{
			returnCode = command.getReturnCode();
			errorString = command.getParsedReply();
			liric.log(Logging.VERBOSITY_TERSE,
				   "sendMultrunCommand:multrun command failed with return code "+
				   returnCode+" and error string:"+errorString);
			throw new Exception(this.getClass().getName()+
					    ":sendMultrunCommand:Command failed with return code "+
					    returnCode+" and error string:"+errorString);
		}
		// extract data from successful reply.
		parseSuccessfulReply(command.getParsedReply());
		liric.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultrunCommand:finished.");
	}

	/**
	 * Parse the successful reply string from the Multrun command.
	 * Currently should be of the form:
	 * "&lt;filename count&gt; &lt;multrun number&gt; &lt;last FITS filename&gt;".
	 * The preceeding '0' denoting success should have already been stripped off.
	 * @param replyString The reply string.
	 * @exception NumberFormatException Thrown if parsing the filename count or multrun number fails.
	 * @see #filenameCount
	 * @see #multrunNumber
	 * @see #lastFilename
	 */
	protected void parseSuccessfulReply(String replyString) throws NumberFormatException
	{
		String token = null;
		StringTokenizer st = new StringTokenizer(replyString," ");
		int tokenIndex;
		
		liric.log(Logging.VERBOSITY_VERBOSE,"parseSuccessfulReply:started.");
		tokenIndex = 0;
		while(st.hasMoreTokens())
		{
			// get next token 
			token = st.nextToken();
			liric.log(Logging.VERBOSITY_VERY_VERBOSE,"parseSuccessfulReply:token "+
				   tokenIndex+" = "+token+".");
			if(tokenIndex == 0)
			{
				filenameCount = Integer.parseInt(token);
			}
			else if(tokenIndex == 1)
			{
				multrunNumber = Integer.parseInt(token);
			}
			else if(tokenIndex == 2)
			{
				lastFilename = token;
			}
			else
			{
				liric.log(Logging.VERBOSITY_VERBOSE,
					   "parseSuccessfulReply:unknown token index "+
					   tokenIndex+" = "+token+".");
			}
			// increment index
			tokenIndex++;
		}
		liric.log(Logging.VERBOSITY_VERBOSE,"parseSuccessfulReply:finished.");
	}
}
