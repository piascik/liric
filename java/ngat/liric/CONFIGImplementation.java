// CONFIGImplementation.java
// $Id$
package ngat.liric;

import java.lang.*;
import ngat.liric.command.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.CONFIG;
import ngat.message.ISS_INST.CONFIG_DONE;
import ngat.message.ISS_INST.OFFSET_FOCUS;
import ngat.message.ISS_INST.OFFSET_FOCUS_DONE;
import ngat.message.ISS_INST.INST_TO_ISS_DONE;
import ngat.phase2.*;
import ngat.util.logging.*;

/**
 * This class provides the implementation for the CONFIG command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision$
 */
public class CONFIGImplementation extends CommandImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * Constructor. 
	 */
	public CONFIGImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.CONFIG&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.CONFIG";
	}

	/**
	 * This method gets the CONFIG command's acknowledge time.
	 * This method returns an ACK with timeToComplete set to the &quot;liric.config.acknowledge_time &quot;
	 * held in the Liric configuration file. 
	 * If this cannot be found/is not a valid number the default acknowledge time is used instead.
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set to a time (in milliseconds).
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see LiricTCPServerConnectionThread#getDefaultAcknowledgeTime
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		ACK acknowledge = null;
		int timeToComplete = 0;

		acknowledge = new ACK(command.getId());
		try
		{
			timeToComplete += liric.getStatus().getPropertyInteger("liric.config.acknowledge_time");
		}
		catch(NumberFormatException e)
		{
			liric.error(this.getClass().getName()+":calculateAcknowledgeTime:"+e);
			timeToComplete += serverConnectionThread.getDefaultAcknowledgeTime();
		}
		acknowledge.setTimeToComplete(timeToComplete);
		return acknowledge;
	}

	/**
	 * This method implements the CONFIG command. 
	 * <ul>
	 * <li>The command is casted.
	 * <li>The DONE message is created.
	 * <li>The config is checked to ensure it isn't null and is of the right class.
	 * <li>sendConfigFilterCommand is called with the extracted filter data to send a C layer Config command.
	 * <li>sendConfigNudgematicOffsetSizeCommand is called with the offset size data to send a 
	 *     C layer Config command.
	 * <li>sendConfigCoaddExposureLengthCommand is called with the coadd exposure length data to send a 
	 *     C layer Config command.
	 * <li>We test for command abort.
	 * <li>We calculate the focus offset from "liric.focus.offset", and call setFocusOffset to tell the RCS/TCS
	 *     the focus offset required.
	 * <li>We increment the config Id.
	 * <li>We save the config name in the Liric status instance for future reference.
	 * <li>We return success.
	 * </ul>
	 * @see #testAbort
	 * @see #setFocusOffset
	 * @see #liric
	 * @see #status
	 * @see #sendConfigFilterCommand
	 * @see #sendConfigNudgematicOffsetSizeCommand
	 * @see #sendConfigCoaddExposureLengthCommand
	 * @see ngat.liric.Liric#getStatus
	 * @see ngat.liric.LiricStatus#incConfigId
	 * @see ngat.liric.LiricStatus#setConfigName
	 * @see ngat.phase2.LiricConfig
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		CONFIG configCommand = null;
		LiricConfig config = null;
		CONFIG_DONE configDone = null;
		String configName = null;
		float focusOffset;

		liric.log(Logging.VERBOSITY_VERY_TERSE,"CONFIGImplementation:processCommand:Started.");
	// test contents of command.
		configCommand = (CONFIG)command;
		configDone = new CONFIG_DONE(command.getId());
		if(testAbort(configCommand,configDone) == true)
			return configDone;
		if(configCommand.getConfig() == null)
		{
			liric.error(this.getClass().getName()+":processCommand:"+command+":Config was null.");
			configDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_BASE+800);
			configDone.setErrorString(":Config was null.");
			configDone.setSuccessful(false);
			return configDone;
		}
		if((configCommand.getConfig() instanceof LiricConfig) == false)
		{
			liric.error(this.getClass().getName()+":processCommand:"+
				command+":Config has wrong class:"+
				configCommand.getConfig().getClass().getName());
			configDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_BASE+801);
			configDone.setErrorString(":Config has wrong class:"+
				configCommand.getConfig().getClass().getName());
			configDone.setSuccessful(false);
			return configDone;
		}
	// test abort
		if(testAbort(configCommand,configDone) == true)
			return configDone;
	// get config from configCommand.
		config = (LiricConfig)configCommand.getConfig();
	// get configuration Id - used later
		configName = config.getId();
		liric.log(Logging.VERBOSITY_VERY_TERSE,"Command:"+
			   configCommand.getClass().getName()+
			   "\n\t:id = "+configName+
			   "\n\t:Filter = "+config.getFilterName()+
			   "\n\t:Nudgematic Offset Size = "+config.getNudgematicOffsetSize()+
			   "\n\t:Coadd Exposure Length = "+config.getCoaddExposureLength()+".");

		// send config commands to C layers
		try
		{
			sendConfigFilterCommand(config.getFilterName());
			sendConfigNudgematicOffsetSizeCommand(config.getNudgematicOffsetSize());
			sendConfigCoaddExposureLengthCommand(config.getCoaddExposureLength());
		}
		catch(Exception e)
		{
			liric.error(this.getClass().getName()+":processCommand:"+command,e);
			configDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_BASE+804);
			configDone.setErrorString(e.toString());
			configDone.setSuccessful(false);
			return configDone;
		}
	// test abort
		if(testAbort(configCommand,configDone) == true)
			return configDone;
	// Issue ISS OFFSET_FOCUS commmand. 
		try
		{
			focusOffset = status.getPropertyFloat("liric.focus.offset");
			liric.log(Logging.VERBOSITY_VERY_TERSE,"Command:"+
				   configCommand.getClass().getName()+":focus offset = "+focusOffset+".");
		}
		catch(NumberFormatException e)
		{
			liric.error(this.getClass().getName()+":processCommand:"+command,e);
			configDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_BASE+806);
			configDone.setErrorString(e.toString());
			configDone.setSuccessful(false);
			return configDone;
		}
		if(setFocusOffset(configCommand.getId(),focusOffset,configDone) == false)
			return configDone;
	// Increment unique config ID.
	// This is queried when saving FITS headers to get the CONFIGID value.
		try
		{
			status.incConfigId();
		}
		catch(Exception e)
		{
			liric.error(this.getClass().getName()+":processCommand:"+
				command+":Incrementing configuration ID:"+e.toString());
			configDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_BASE+809);
			configDone.setErrorString("Incrementing configuration ID:"+e.toString());
			configDone.setSuccessful(false);
			return configDone;
		}
	// Store name of configuration used in status object
	// This is queried when saving FITS headers to get the CONFNAME value.
		status.setConfigName(configName);
	// setup return object.
		configDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_NO_ERROR);
		configDone.setErrorString("");
		configDone.setSuccessful(true);
		liric.log(Logging.VERBOSITY_VERY_TERSE,"CONFIGImplementation:processCommand:Finished.");
	// return done object.
		return configDone;
	}

	/**
	 * Send the extracted filter config data onto the C layer.
	 * @param filterName The name of the filter to be selected (put into the beam).
	 * @exception Exception Thrown if an error occurs.
	 * @see ngat.liric.command.ConfigFilterCommand
	 * @see ngat.liric.command.ConfigFilterCommand#setAddress
	 * @see ngat.liric.command.ConfigFilterCommand#setPortNumber
	 * @see ngat.liric.command.ConfigFilterCommand#setCommand
	 * @see ngat.liric.command.ConfigFilterCommand#sendCommand
	 * @see ngat.liric.command.ConfigFilterCommand#getParsedReplyOK
	 * @see ngat.liric.command.ConfigFilterCommand#getReturnCode
	 * @see ngat.liric.command.ConfigFilterCommand#getParsedReply
	 */
	protected void sendConfigFilterCommand(String filterName) throws Exception
	{
		ConfigFilterCommand command = null;
		int portNumber,returnCode;
		String hostname = null;
		String errorString = null;

		liric.log(Logging.VERBOSITY_INTERMEDIATE,"sendConfigFilterCommand:filter name = "+filterName+".");
		command = new ConfigFilterCommand();
		// configure C comms
		hostname = status.getProperty("liric.c.hostname");
		portNumber = status.getPropertyInteger("liric.c.port_number");
		command.setAddress(hostname);
		command.setPortNumber(portNumber);
		liric.log(Logging.VERBOSITY_INTERMEDIATE,"sendConfigFilterCommand:hostname = "+hostname+
			   " :port number = "+portNumber+".");
		// set command parameters
		command.setCommand(filterName);
		// actually send the command to the C layer
		command.sendCommand();
		// check the parsed reply
		if(command.getParsedReplyOK() == false)
		{
			returnCode = command.getReturnCode();
			errorString = command.getParsedReply();
			liric.log(Logging.VERBOSITY_TERSE,
				   "sendConfigFilterCommand:config command failed with return code "+
				   returnCode+" and error string:"+errorString);
			throw new Exception(this.getClass().getName()+
					    ":sendConfigFilterCommand:Command failed with return code "+
					    returnCode+" and error string:"+errorString);
		}
		liric.log(Logging.VERBOSITY_INTERMEDIATE,"sendConfigFilterCommand:finished.");
	}

	/**
	 * Send the extracted nudgematic offset size config data onto the C layer.
	 * @param nudgematicOffsetSize The nudgematic offset size as an integer, one of:
	 *        NUDGEMATIC_OFFSET_SIZE_NONE / NUDGEMATIC_OFFSET_SIZE_SMALL / NUDGEMATIC_OFFSET_SIZE_LARGE.
	 * @exception Exception Thrown if an error occurs.
	 * @see ngat.liric.command.ConfigNudgematicOffsetSizeCommand
	 * @see ngat.liric.command.ConfigNudgematicOffsetSizeCommand#setAddress
	 * @see ngat.liric.command.ConfigNudgematicOffsetSizeCommand#setPortNumber
	 * @see ngat.liric.command.ConfigNudgematicOffsetSizeCommand#setCommand
	 * @see ngat.liric.command.ConfigNudgematicOffsetSizeCommand#sendCommand
	 * @see ngat.liric.command.ConfigNudgematicOffsetSizeCommand#getParsedReplyOK
	 * @see ngat.liric.command.ConfigNudgematicOffsetSizeCommand#getReturnCode
	 * @see ngat.liric.command.ConfigNudgematicOffsetSizeCommand#getParsedReply
	 * @see ngat.phase2.LiricConfig#NUDGEMATIC_OFFSET_SIZE_NONE
	 * @see ngat.phase2.LiricConfig#NUDGEMATIC_OFFSET_SIZE_SMALL
	 * @see ngat.phase2.LiricConfig#NUDGEMATIC_OFFSET_SIZE_LARGE
	 */
	protected void sendConfigNudgematicOffsetSizeCommand(int nudgematicOffsetSize) throws Exception
	{
		ConfigNudgematicOffsetSizeCommand command = null;
		int portNumber,returnCode;
		String hostname = null;
		String errorString = null;

		liric.log(Logging.VERBOSITY_INTERMEDIATE,
			   "sendConfigNudgematicOffsetSizeCommand:nudgematic offset size = "+
			   nudgematicOffsetSize+".");
		command = new ConfigNudgematicOffsetSizeCommand();
		// configure C comms
		hostname = status.getProperty("liric.c.hostname");
		portNumber = status.getPropertyInteger("liric.c.port_number");
		command.setAddress(hostname);
		command.setPortNumber(portNumber);
		liric.log(Logging.VERBOSITY_INTERMEDIATE,"sendConfigNudgematicOffsetSizeCommand:hostname = "+hostname+
			   " :port number = "+portNumber+".");
		// set command parameters
		command.setCommand(nudgematicOffsetSize);
		// actually send the command to the C layer
		command.sendCommand();
		// check the parsed reply
		if(command.getParsedReplyOK() == false)
		{
			returnCode = command.getReturnCode();
			errorString = command.getParsedReply();
			liric.log(Logging.VERBOSITY_TERSE,
				   "sendConfigNudgematicOffsetSizeCommand:config command failed with return code "+
				   returnCode+" and error string:"+errorString);
			throw new Exception(this.getClass().getName()+
					    ":sendConfigNudgematicOffsetSizeCommand:Command failed with return code "+
					    returnCode+" and error string:"+errorString);
		}
		liric.log(Logging.VERBOSITY_INTERMEDIATE,"sendConfigNudgematicOffsetSizeCommand:finished.");
	}

	/**
	 * Send the extracted coadd exposure length config data onto the C layer.
	 * @param coaddExposureLength The coadd exposure length as an integer in milliseconds.
	 * @exception Exception Thrown if an error occurs.
	 * @see ngat.liric.command.ConfigCoaddExposureLengthCommand
	 * @see ngat.liric.command.ConfigCoaddExposureLengthCommand#setAddress
	 * @see ngat.liric.command.ConfigCoaddExposureLengthCommand#setPortNumber
	 * @see ngat.liric.command.ConfigCoaddExposureLengthCommand#setCommand
	 * @see ngat.liric.command.ConfigCoaddExposureLengthCommand#sendCommand
	 * @see ngat.liric.command.ConfigCoaddExposureLengthCommand#getParsedReplyOK
	 * @see ngat.liric.command.ConfigCoaddExposureLengthCommand#getReturnCode
	 * @see ngat.liric.command.ConfigCoaddExposureLengthCommand#getParsedReply
	 */
	protected void sendConfigCoaddExposureLengthCommand(int coaddExposureLength) throws Exception
	{
		ConfigCoaddExposureLengthCommand command = null;
		int portNumber,returnCode;
		String hostname = null;
		String errorString = null;

		liric.log(Logging.VERBOSITY_INTERMEDIATE,
			   "sendConfigCoaddExposureLengthCommand:coadd exposure length = "+
			   coaddExposureLength+".");
		command = new ConfigCoaddExposureLengthCommand();
		// configure C comms
		hostname = status.getProperty("liric.c.hostname");
		portNumber = status.getPropertyInteger("liric.c.port_number");
		command.setAddress(hostname);
		command.setPortNumber(portNumber);
		liric.log(Logging.VERBOSITY_INTERMEDIATE,"sendConfigCoaddExposureLengthCommand:hostname = "+hostname+
			   " :port number = "+portNumber+".");
		// set command parameters
		command.setCommand(coaddExposureLength);
		// actually send the command to the C layer
		command.sendCommand();
		// check the parsed reply
		if(command.getParsedReplyOK() == false)
		{
			returnCode = command.getReturnCode();
			errorString = command.getParsedReply();
			liric.log(Logging.VERBOSITY_TERSE,
				   "sendConfigCoaddExposureLengthCommand:config command failed with return code "+
				   returnCode+" and error string:"+errorString);
			throw new Exception(this.getClass().getName()+
					    ":sendConfigCoaddExposureLengthCommand:Command failed with return code "+
					    returnCode+" and error string:"+errorString);
		}
		liric.log(Logging.VERBOSITY_INTERMEDIATE,"sendConfigCoaddExposureLengthCommand:finished.");
	}

	/**
	 * Routine to set the telescope focus offset, due to the filters selected. Sends a OFFSET_FOCUS command to
	 * the ISS. The OFFSET_FOCUS sent is the offset of Liric's focus from the nominal telescope focus.
	 * @param id The Id is used as the OFFSET_FOCUS command's id.
	 * @param focusOffset The focus offset needed.
	 * @param configDone The instance of CONFIG_DONE. This is filled in with an error message if the
	 * 	OFFSET_FOCUS fails.
	 * @return The method returns true if the telescope attained the focus offset, otherwise false is
	 * 	returned an telFocusDone is filled in with an error message.
	 */
	private boolean setFocusOffset(String id,float focusOffset,CONFIG_DONE configDone)
	{
		OFFSET_FOCUS focusOffsetCommand = null;
		INST_TO_ISS_DONE instToISSDone = null;
		String filterIdName = null;
		String filterTypeString = null;

		focusOffsetCommand = new OFFSET_FOCUS(id);
	// set the commands focus offset
		focusOffsetCommand.setFocusOffset(focusOffset);
		instToISSDone = liric.sendISSCommand(focusOffsetCommand,serverConnectionThread);
		if(instToISSDone.getSuccessful() == false)
		{
			liric.error(this.getClass().getName()+":focusOffset failed:"+focusOffset+":"+
				     instToISSDone.getErrorString());
			configDone.setErrorNum(LiricConstants.LIRIC_ERROR_CODE_BASE+805);
			configDone.setErrorString(instToISSDone.getErrorString());
			configDone.setSuccessful(false);
			return false;
		}
		return true;
	}
}
