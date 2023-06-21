// ABORTImplementation.java
// $Id$
package ngat.liric;

import java.lang.*;
import java.text.*;
import java.util.*;

import ngat.liric.command.*;
import ngat.message.base.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.*;
import ngat.util.logging.*;

/**
 * This class provides the implementation for the ABORT command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision$
 */
public class ABORTImplementation extends CommandImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");

	/**
	 * Constructor.
	 */
	public ABORTImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.ABORT&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.ABORT";
	}

	/**
	 * This method gets the ABORT command's acknowledge time. This takes the default acknowledge time to implement.
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see LiricTCPServerConnectionThread#getDefaultAcknowledgeTime
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		ACK acknowledge = null;

		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the ABORT command. 
	 * <ul>
	 * <li>We call sendAbortCommand to send an "abort" command to the C layer.
	 * <li>We get the currently running thread from the status object.
	 * <li>If the currently running thread is non-null, we call setAbortProcessCommand to tell the
	 *     Java thread to abort itself at a suitable point.
	 * <li>We set up a successful ABORT_DONE to return.
	 * </ul>
	 * @param command The abort command.
	 * @return An object of class ABORT_DONE is returned.
	 * @see #sendAbortCommand
	 * @see #status
	 * @see LiricStatus#getCurrentThread
	 * @see LiricTCPServerConnectionThread
	 * @see LiricTCPServerConnectionThread#setAbortProcessCommand
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		ABORT_DONE abortDone = new ABORT_DONE(command.getId());
		LiricTCPServerConnectionThread thread = null;
		String cLayerHostname = null;
		int cLayerPortNumber;

		liric.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Started.");
		try
		{
			sendAbortCommand();
		}
		catch(Exception e)
		{
			liric.error(this.getClass().getName()+":Aborting exposure failed:",e);
			abortDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_BASE+2400);
			abortDone.setErrorString(e.toString());
			abortDone.setSuccessful(false);
			return abortDone;
		}
	// tell the thread itself to abort at a suitable point
		liric.log(Logging.VERBOSITY_TERSE,this.getClass().getName()+":processCommand:Tell thread to abort.");
		thread = (LiricTCPServerConnectionThread)status.getCurrentThread();
		if(thread != null)
			thread.setAbortProcessCommand();
	// return done object.
		liric.log(Logging.VERBOSITY_VERY_TERSE,"Command:"+command.getClass().getName()+
			  ":Abort command completed.");
		abortDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_NO_ERROR);
		abortDone.setErrorString("");
		abortDone.setSuccessful(true);
		return abortDone;
	}
	
	/**
	 * Send an "abort" command to the C layer.
	 * @exception Exception Thrown if an error occurs.
	 * @see #status
	 * @see ngat.liric.command.AbortCommand
	 * @see ngat.liric.command.AbortCommand#setAddress
	 * @see ngat.liric.command.AbortCommand#setPortNumber
	 * @see ngat.liric.command.AbortCommand#setCommand
	 * @see ngat.liric.command.AbortCommand#sendCommand
	 * @see ngat.liric.command.AbortCommand#getParsedReplyOK
	 * @see ngat.liric.command.AbortCommand#getReturnCode
	 * @see ngat.liric.command.AbortCommand#getParsedReply
	 */
	protected void sendAbortCommand() throws Exception
	{
		AbortCommand command = null;
		int portNumber,returnCode;
		String hostname = null;
		String errorString = null;

		liric.log(Logging.VERBOSITY_INTERMEDIATE,"sendAbortCommand:Started.");
		hostname = status.getProperty("liric.c.hostname");
		portNumber = status.getPropertyInteger("liric.c.port_number");
		command = new AbortCommand();
		// configure C comms
		command.setAddress(hostname);
		command.setPortNumber(portNumber);
		liric.log(Logging.VERBOSITY_INTERMEDIATE,"sendAbortCommand:hostname = "+hostname+
			  " :port number = "+portNumber+".");
		// actually send the command to the C layer
		command.sendCommand();
		// check the parsed reply
		if(command.getParsedReplyOK() == false)
		{
			returnCode = command.getReturnCode();
			errorString = command.getParsedReply();
			liric.log(Logging.VERBOSITY_TERSE,
				   "sendAbortCommand:abort command failed with return code "+
				   returnCode+" and error string:"+errorString);
			throw new Exception(this.getClass().getName()+
					    ":sendAbortCommand:Command failed with return code "+returnCode+
					    " and error string:"+errorString);
		}
		liric.log(Logging.VERBOSITY_INTERMEDIATE,"sendAbortCommand:Finished.");
	}
}
