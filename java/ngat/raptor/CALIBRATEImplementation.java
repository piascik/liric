// CALIBRATEImplementation.java
// $Id$
package ngat.raptor;

import java.lang.*;
import java.text.*;
import java.util.*;

import ngat.fits.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.*;
import ngat.raptor.command.*;
import ngat.util.logging.*;

/**
 * This class provides common methods to send bias and dark commands to the C layer,
 * as need by several Bias and Dark calibration implementations.
 * @version $Revision$
 * @see ngat.raptor.HardwareImplementation
 */
public class CALIBRATEImplementation extends HardwareImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * The number of milliseconds in one second.
	 */
	public final static int MILLISECONDS_PER_SECOND = 1000;
	/**
	 * Return value from C layer: how many FITS files were produced.
	 */
	protected int filenameCount;
	/**
	 * Return value from C layer: The multrun number of the FITS files.
	 */
	protected int multrunNumber;
	/**
	 * Return value from C layer: The last FITS filename produced.
	 */
	protected String lastFilename;

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
	 * Send a multdark command to the C layer. On success, parseSuccessfulReply is called to extract
	 * data from the reply.
	 * @param exposureLength The total exposure length of each frame in the multrun, in milliseconds
	 *        (itself consisting of the average of a series of frames of the coadd exposure length).
	 * @param exposureCount The number of exposures to do in the multdark.
	 * @exception Exception Thrown if an error occurs.
	 * @see #parseSuccessfulReply
	 * @see ngat.raptor.command.MultdarkCommand
	 * @see ngat.raptor.command.MultdarkCommand#setAddress
	 * @see ngat.raptor.command.MultdarkCommand#setPortNumber
	 * @see ngat.raptor.command.MultdarkCommand#setCommand
	 * @see ngat.raptor.command.MultdarkCommand#sendCommand
	 * @see ngat.raptor.command.MultdarkCommand#getParsedReplyOK
	 * @see ngat.raptor.command.MultdarkCommand#getReturnCode
	 * @see ngat.raptor.command.MultdarkCommand#getParsedReply
	 */
	protected void sendMultdarkCommand(int exposureLength,int exposureCount) throws Exception
	{
		MultdarkCommand command = null;
		int portNumber,returnCode;
		String hostname = null;
		String errorString = null;

		raptor.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultdarkCommand:exposure length = "+exposureLength+
			   ":exposure count = "+exposureCount+".");
		command = new MultdarkCommand();
		// configure C comms
		hostname = status.getProperty("raptor.c.hostname");
		portNumber = status.getPropertyInteger("raptor.c.port_number");
		command.setAddress(hostname);
		command.setPortNumber(portNumber);
		raptor.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultdarkCommand:hostname = "+hostname+
			   " :port number = "+portNumber+".");
		// set command parameters
		command.setCommand(exposureLength,exposureCount);
		// actually send the command to the C layer
		command.sendCommand();
		// check the parsed reply
		if(command.getParsedReplyOK() == false)
		{
			returnCode = command.getReturnCode();
			errorString = command.getParsedReply();
			raptor.log(Logging.VERBOSITY_TERSE,
				   "sendMultdarkCommand:multdark command failed with return code "+
				   returnCode+" and error string:"+errorString);
			throw new Exception(this.getClass().getName()+
					    ":sendMultdarkCommand:Command failed with return code "+
					    returnCode+" and error string:"+errorString);
		}
		// extract data from successful reply.
		parseSuccessfulReply(command.getParsedReply());
		raptor.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultdarkCommand:finished.");
	}

	/**
	 * Send a multbias command to the C layer. On success, parseSuccessfulReply is called to extract
	 * data from the reply.
	 * @param exposureCount The number of bias frames to do in the multbias.
	 * @exception Exception Thrown if an error occurs.
	 * @see #parseSuccessfulReply
	 * @see ngat.raptor.command.MultbiasCommand
	 * @see ngat.raptor.command.MultbiasCommand#setAddress
	 * @see ngat.raptor.command.MultbiasCommand#setPortNumber
	 * @see ngat.raptor.command.MultbiasCommand#setCommand
	 * @see ngat.raptor.command.MultbiasCommand#sendCommand
	 * @see ngat.raptor.command.MultbiasCommand#getParsedReplyOK
	 * @see ngat.raptor.command.MultbiasCommand#getReturnCode
	 * @see ngat.raptor.command.MultbiasCommand#getParsedReply
	 */
	protected void sendMultbiasCommand(int exposureCount) throws Exception
	{
		MultbiasCommand command = null;
		int portNumber,returnCode;
		String hostname = null;
		String errorString = null;

		raptor.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultbiasCommand:exposure count = "+exposureCount+".");
		command = new MultbiasCommand();
		// configure C comms
		hostname = status.getProperty("raptor.c.hostname");
		portNumber = status.getPropertyInteger("raptor.c.port_number");
		command.setAddress(hostname);
		command.setPortNumber(portNumber);
		raptor.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultbiasCommand:hostname = "+hostname+
			   " :port number = "+portNumber+".");
		// set command parameters
		command.setCommand(exposureCount);
		// actually send the command to the C layer
		command.sendCommand();
		// check the parsed reply
		if(command.getParsedReplyOK() == false)
		{
			returnCode = command.getReturnCode();
			errorString = command.getParsedReply();
			raptor.log(Logging.VERBOSITY_TERSE,
				   "sendMultbiasCommand:multbias command failed with return code "+
				   returnCode+" and error string:"+errorString);
			throw new Exception(this.getClass().getName()+
					    ":sendMultbiasCommand:Command failed with return code "+
					    returnCode+" and error string:"+errorString);
		}
		// extract data from successful reply.
		parseSuccessfulReply(command.getParsedReply());
		raptor.log(Logging.VERBOSITY_INTERMEDIATE,"sendMultbiasCommand:finished.");
	}

	/**
	 * Parse the successful reply string from the Multbias/Multdark command.
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
		
		raptor.log(Logging.VERBOSITY_VERBOSE,"parseSuccessfulReply:started.");
		tokenIndex = 0;
		while(st.hasMoreTokens())
		{
			// get next token 
			token = st.nextToken();
			raptor.log(Logging.VERBOSITY_VERY_VERBOSE,"parseSuccessfulReply:token "+
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
				raptor.log(Logging.VERBOSITY_VERBOSE,
					   "parseSuccessfulReply:unknown token index "+
					   tokenIndex+" = "+token+".");
			}
			// increment index
			tokenIndex++;
		}
		raptor.log(Logging.VERBOSITY_VERBOSE,"parseSuccessfulReply:finished.");
	}	
}

