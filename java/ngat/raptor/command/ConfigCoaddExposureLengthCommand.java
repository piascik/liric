// ConfigCoaddExposureLengthCommand.java
// $Id$
package ngat.raptor.command;

import java.io.*;
import java.lang.*;
import java.net.*;
import java.util.*;

import ngat.phase2.RaptorConfig;

/**
 * The "config coadd_exp_len" command is an extension of the Command, and configures the coadd exposure length
 * used during multruns.
 * @author Chris Mottram
 * @version $Revision$
 */
public class ConfigCoaddExposureLengthCommand extends Command implements Runnable
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
	public ConfigCoaddExposureLengthCommand()
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
	public ConfigCoaddExposureLengthCommand(String address,int portNumber) throws UnknownHostException
	{
		super();
		super.setAddress(address);
		super.setPortNumber(portNumber);
	}

	/**
	 * Setup the command to send to the server.
	 * @param coaddExposureLengthString A string representing the coadd exposure length selected.
	 *        Should be one of "short" or "long".
	 * @see #commandString
	 */
	public void setCommand(String coaddExposureLengthString)
	{
		commandString = new String("config coadd_exp_len "+coaddExposureLengthString);
	}
	
	/**
	 * Setup the command to send to the server.
	 * @param coaddExposureLength An integer representing the coadd exposure length selected.
	 *        Should be either '100' or '1000'.
	 * @exception IllegalArgumentException Thrown if coadd exposure length is not valid.
	 * @see #commandString
	 */
	public void setCommand(int coaddExposureLength) throws IllegalArgumentException
	{
		String coaddExposureLengthString = null;
		
		if(coaddExposureLength == 100)
			coaddExposureLengthString  = "short";
		else if(coaddExposureLength == 1000)
			coaddExposureLengthString  = "long";
		else
			throw new IllegalArgumentException(this.getClass().getName()+":setCommand:"+
							   "Illegal coadd exposure length:"+coaddExposureLength);
		commandString = new String("config coadd_exp_len "+coaddExposureLengthString);
	}
	
	/**
	 * Main test program.
	 * @param args The argument list.
	 */
	public static void main(String args[])
	{
		ConfigCoaddExposureLengthCommand command = null;
		String hostname = null;
		String coaddExposureLengthString = null;
		int portNumber = 1111;

		if(args.length != 3)
		{
			System.out.println("java ngat.raptor.command.ConfigCoaddExposureLengthCommand <hostname> <port number> <short|long>");
			System.exit(1);
		}
		try
		{
			hostname = args[0];
			portNumber = Integer.parseInt(args[1]);
			coaddExposureLengthString = args[2];
			command = new ConfigCoaddExposureLengthCommand(hostname,portNumber);
			command.setCommand(coaddExposureLengthString);
			command.run();
			if(command.getRunException() != null)
			{
				System.err.println("ConfigCoaddExposureLengthCommand: Command failed.");
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
