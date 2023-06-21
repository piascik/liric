// StatusNudgematicStatusCommand.java
// $Id$
package ngat.raptor.command;

import java.io.*;
import java.lang.*;
import java.net.*;
import java.util.*;

/**
 * The "status nudgematic status" command is an extension of Command, and returns whether the nudgematic
 * mechanism is currently in position or moving.
 * @author Chris Mottram
 * @version $Revision$
 */
public class StatusNudgematicStatusCommand extends Command implements Runnable
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * This command returns the string "moving" if the nudgematic was moving.
	 */
	public final static String NUDGEMATIC_MOVING = new String("moving");
	/**
	 * This command returns the string "stopped" if the nudgematic is currently stopped i.e. not moving.
	 */
	public final static String NUDGEMATIC_STOPPED = new String("stopped");
	/**
	 * The command to send to the server.
	 */
	public final static String COMMAND_STRING = new String("status nudgematic status");
	/**
	 * The current nudgematic status, "moving" if the nudgematic was moving, and "stopped" if it is
	 * in stopped.
	 * @see #NUDGEMATIC_MOVING
	 * @see #NUDGEMATIC_STOPPED
	 */
	protected String statusString = null;
	
	/**
	 * Default constructor.
	 * @see Command
	 * @see #commandString
	 * @see #COMMAND_STRING
	 */
	public StatusNudgematicStatusCommand()
	{
		super();
		commandString = COMMAND_STRING;
	}

	/**
	 * Constructor.
	 * @param address A string representing the address of the server, i.e. "raptor",
	 *     "localhost", "192.168.1.34"
	 * @param portNumber An integer representing the port number the server is receiving command on.
	 * @see Command
	 * @see #COMMAND_STRING
	 * @exception UnknownHostException Thrown if the address in unknown.
	 */
	public StatusNudgematicStatusCommand(String address,int portNumber) throws UnknownHostException
	{
		super(address,portNumber,COMMAND_STRING);
	}

	/**
	 * Parse a string returned from the server over the telnet connection.
	 * @exception Exception Thrown if a parse error occurs.
	 * @see #replyString
	 * @see #parsedReplyString
	 * @see #parsedReplyOk
	 * @see #statusString
	 */
	public void parseReplyString() throws Exception
	{
		super.parseReplyString();
		if(parsedReplyOk == false)
		{
			statusString = null;
			return;
		}
		statusString = parsedReplyString;
	}

	/**
	 * Get the current nudgematic status.
	 * @return The current nudgematic status as a string, "moving" or "stopped". 
	 * @see #statusString
	 * @see #NUDGEMATIC_MOVING
	 * @see #NUDGEMATIC_STOPPED
	 */
	public String getNudgematicStatus()
	{
		return statusString;
	}
	
	/**
	 * Return whether the nudgematic is currently moving.
	 * @return Returns true if the statusString equals NUDGEMATIC_MOVING, false if it is null or 
	 *         NUDGEMATIC_STOPPED.
	 * @see #statusString
	 * @see #NUDGEMATIC_MOVING
	 * @see #NUDGEMATIC_STOPPED
	 */
	public boolean nudgematicIsMoving()
	{
		if(statusString == null)
			return false;
		return statusString.equals(NUDGEMATIC_MOVING);
	}
	
	/**
	 * Return whether the nudgematic is currently stopped.
	 * @return Returns true if the statusString equals NUDGEMATIC_STOPPED, false if it is null or 
	 *         NUDGEMATIC_MOVING.
	 * @see #statusString
	 * @see #NUDGEMATIC_MOVING
	 * @see #NUDGEMATIC_STOPPED
	 */
	public boolean nudgematicIsStopped()
	{
		if(statusString == null)
			return false;
		return statusString.equals(NUDGEMATIC_STOPPED);
	}
	
	/**
	 * Main test program.
	 * @param args The argument list.
	 */
	public static void main(String args[])
	{
		StatusNudgematicStatusCommand command = null;
		String hostname = null;
		int portNumber = 8284;

		if(args.length != 2)
		{
			System.out.println("java ngat.raptor.command.StatusNudgematicStatusCommand <hostname> <port number>");
			System.exit(1);
		}
		try
		{
			hostname = args[0];			
			portNumber = Integer.parseInt(args[1]);
			command = new StatusNudgematicStatusCommand(hostname,portNumber);
			command.run();
			if(command.getRunException() != null)
			{
				System.err.println("StatusNudgematicStatusCommand: Command failed.");
				command.getRunException().printStackTrace(System.err);
				System.exit(1);
			}
			System.out.println("Finished:"+command.getCommandFinished());
			System.out.println("Reply Parsed OK:"+command.getParsedReplyOK());
			System.out.println("Nudgematic Status:"+command.getNudgematicStatus());
			System.out.println("Nudgematic Is Moving:"+command.nudgematicIsMoving());
			System.out.println("Nudgematic Is Stopped:"+command.nudgematicIsStopped());
		}
		catch(Exception e)
		{
			e.printStackTrace(System.err);
			System.exit(1);
		}
		System.exit(0);
	}
}
