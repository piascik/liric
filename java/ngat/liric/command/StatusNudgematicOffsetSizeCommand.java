// StatusNudgematicOffsetSizeCommand.java
// $Id$
package ngat.liric.command;

import java.io.*;
import java.lang.*;
import java.net.*;
import java.util.*;

/**
 * The "status nudgematic offsetsize" command is an extension of Command, and returns the current size of offsets
 * the nudgematic is using.
 * @author Chris Mottram
 * @version $Revision$
 */
public class StatusNudgematicOffsetSizeCommand extends Command implements Runnable
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * The command to send to the server.
	 */
	public final static String COMMAND_STRING = new String("status nudgematic offsetsize");
	/**
	 * The current nudgematic offset size, 
	 */
	protected String offsetSizeString = null;
	
	/**
	 * Default constructor.
	 * @see Command
	 * @see #commandString
	 * @see #COMMAND_STRING
	 */
	public StatusNudgematicOffsetSizeCommand()
	{
		super();
		commandString = COMMAND_STRING;
	}

	/**
	 * Constructor.
	 * @param address A string representing the address of the server, i.e. "liric",
	 *     "localhost", "192.168.1.34"
	 * @param portNumber An integer representing the port number the server is receiving command on.
	 * @see Command
	 * @see #COMMAND_STRING
	 * @exception UnknownHostException Thrown if the address in unknown.
	 */
	public StatusNudgematicOffsetSizeCommand(String address,int portNumber) throws UnknownHostException
	{
		super(address,portNumber,COMMAND_STRING);
	}

	/**
	 * Parse a string returned from the server over the telnet connection.
	 * @exception Exception Thrown if a parse error occurs.
	 * @see #replyString
	 * @see #parsedReplyString
	 * @see #parsedReplyOk
	 * @see #offsetSizeString
	 */
	public void parseReplyString() throws Exception
	{
		super.parseReplyString();
		if(parsedReplyOk == false)
		{
			offsetSizeString = null;
			return;
		}
		offsetSizeString = parsedReplyString;
	}

	/**
	 * Get the current nudgematic status.
	 * @return The current nudgematic status as a string, "none","small","large" or "UNKNOWN". 
	 * @see #offsetSizeString
	 */
	public String getNudgematicOffsetSize()
	{
		return offsetSizeString;
	}
	
	/**
	 * Main test program.
	 * @param args The argument list.
	 */
	public static void main(String args[])
	{
		StatusNudgematicOffsetSizeCommand command = null;
		String hostname = null;
		int portNumber = 8284;

		if(args.length != 2)
		{
			System.out.println("java ngat.liric.command.StatusNudgematicOffsetSizeCommand <hostname> <port number>");
			System.exit(1);
		}
		try
		{
			hostname = args[0];			
			portNumber = Integer.parseInt(args[1]);
			command = new StatusNudgematicOffsetSizeCommand(hostname,portNumber);
			command.run();
			if(command.getRunException() != null)
			{
				System.err.println("StatusNudgematicOffsetSizeCommand: Command failed.");
				command.getRunException().printStackTrace(System.err);
				System.exit(1);
			}
			System.out.println("Finished:"+command.getCommandFinished());
			System.out.println("Reply Parsed OK:"+command.getParsedReplyOK());
			System.out.println("Nudgematic Offset Size:"+command.getNudgematicOffsetSize());
		}
		catch(Exception e)
		{
			e.printStackTrace(System.err);
			System.exit(1);
		}
		System.exit(0);
	}
}
