// ConfigNudgematicOffsetSizeCommand.java
// $Id$
package ngat.raptor.command;

import java.io.*;
import java.lang.*;
import java.net.*;
import java.util.*;

import ngat.phase2.RaptorConfig;

/**
 * The "config nudgematic" command is an extension of the Command, and configures the size of the offsets 
 * performed by the nudgematic during multruns.
 * @author Chris Mottram
 * @version $Revision$
 */
public class ConfigNudgematicOffsetSizeCommand extends Command implements Runnable
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");

	/**
	 * Default constructor.
	 * @see Command
	 * @see #commandString
	 */
	public ConfigNudgematicOffsetSizeCommand()
	{
		super();
		commandString = null;
	}

	/**
	 * Constructor.
	 * @param address A string representing the address of the server, i.e. "raptor",
	 *     "localhost"
	 * @param portNumber An integer representing the port number the server is receiving command on.
	 * @see Command
	 * @see Command#setAddress
	 * @see Command#setPortNumber
	 * @exception UnknownHostException Thrown if the address in unknown.
	 */
	public ConfigNudgematicOffsetSizeCommand(String address,int portNumber) throws UnknownHostException
	{
		super();
		super.setAddress(address);
		super.setPortNumber(portNumber);
	}

	/**
	 * Setup the command to send to the server.
	 * @param nudgematicOffetSizeString A string representing the nudgematic offset size selected.
	 *        Should be one of "small" or "large".
	 * @see #commandString
	 */
	public void setCommand(String nudgematicOffetSizeString)
	{
		commandString = new String("config nudgematic "+nudgematicOffetSizeString);
	}
	
	/**
	 * Setup the command to send to the server.
	 * @param nudgematicOffsetSize An integer representing the nudgematic offset size selected.
	 *        Should be one of NUDGEMATIC_OFFSET_SIZE_SMALL / NUDGEMATIC_OFFSET_SIZE_LARGE.
	 * @excetption IllegalArgumentException Thrown if nudgematicOffetSize is not valid.
	 * @see #commandString
	 * @see ngat.phase2.RaptorConfig#NUDGEMATIC_OFFSET_SIZE_SMALL
	 * @see ngat.phase2.RaptorConfig#NUDGEMATIC_OFFSET_SIZE_LARGE
	 */
	public void setCommand(int nudgematicOffsetSize) throws IllegalArgumentException
	{
		String nudgematicOffsetSizeString = null;
		
		if(nudgematicOffsetSize == RaptorConfig.NUDGEMATIC_OFFSET_SIZE_SMALL)
			nudgematicOffsetSizeString  = "small";
		else if(nudgematicOffsetSize == RaptorConfig.NUDGEMATIC_OFFSET_SIZE_LARGE)
			nudgematicOffsetSizeString  = "large";
		else
			throw new IllegalArgumentException(this.getClass().getName()+":setCommand:"+
							   "Illegal offset size number:"+nudgematicOffsetSize);
		commandString = new String("config nudgematic "+nudgematicOffsetSizeString);
	}
	
	/**
	 * Main test program.
	 * @param args The argument list.
	 */
	public static void main(String args[])
	{
		ConfigNudgematicOffsetSizeCommand command = null;
		String hostname = null;
		String nudgematicOffsetSizeString = null;
		int portNumber = 1111;

		if(args.length != 3)
		{
			System.out.println("java ngat.raptor.command.ConfigNudgematicOffsetSizeCommand <hostname> <port number> <small|large>");
			System.exit(1);
		}
		try
		{
			hostname = args[0];
			portNumber = Integer.parseInt(args[1]);
			nudgematicOffsetSizeString = args[2];
			command = new ConfigNudgematicOffsetSizeCommand(hostname,portNumber);
			command.setCommand(nudgematicOffsetSizeString);
			command.run();
			if(command.getRunException() != null)
			{
				System.err.println("ConfigNudgematicOffsetSizeCommand: Command failed.");
				command.getRunException().printStackTrace(System.err);
				System.exit(1);
			}
			System.out.println("Finished:"+command.getCommandFinished());
			System.out.println("Reply Parsed OK:"+command.getParsedReplyOK());
			System.out.println("Return Code:"+command.getReturnCode());
			System.out.println("Reply String:"+command.getParsedReply());
		}
		catch(Exception e)
		{
			e.printStackTrace(System.err);
			System.exit(1);
		}
		System.exit(0);
	}
}
