// LiricTCPClientConnectionThread.java
// $Header$
package ngat.liric;

import java.lang.*;
import java.io.*;
import java.net.*;
import java.util.Date;

import ngat.net.*;
import ngat.message.base.*;

/**
 * The LiricTCPClientConnectionThread extends TCPClientConnectionThread. 
 * It implements the generic ISS/DP(RT) instrument command protocol with multiple acknowledgements. 
 * The instrument starts one of these threads each time
 * it wishes to send a message to the ISS/DP(RT).
 * @author Chris Mottram
 * @version $Revision$
 */
public class LiricTCPClientConnectionThread extends TCPClientConnectionThreadMA
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * The commandThread was spawned by the Liric to deal with a Liric command request. 
	 * As part of the running of
	 * the commandThread, this client connection thread was created. We need to know the server thread so
	 * that we can pass back any acknowledge times from the ISS/DpRt back to the Liric client (ISS/IcsGUI etc).
	 */
	private LiricTCPServerConnectionThread commandThread = null;
	/**
	 * The Liric object.
	 */
	private Liric liric = null;

	/**
	 * A constructor for this class. Currently just calls the parent class's constructor.
	 * @param address The internet address to send this command to.
	 * @param portNumber The port number to send this command to.
	 * @param c The command to send to the specified address.
	 * @param ct The Liric command thread, the implementation of which spawned this command.
	 */
	public LiricTCPClientConnectionThread(InetAddress address,int portNumber,COMMAND c,
					       LiricTCPServerConnectionThread ct)
	{
		super(address,portNumber,c);
		commandThread = ct;
	}

	/**
	 * Routine to set this objects pointer to the liric object.
	 * @param o The liric object.
	 */
	public void setLiric(Liric o)
	{
		this.liric = o;
	}

	/**
	 * This routine processes the acknowledge object returned by the server. It
	 * prints out a message, giving the time to completion if the acknowledge was not null.
	 * It sends the acknowledgement to the Liric client for this sub-command of the command,
	 * so that Liric's client does not time out if,say, a zero is returned.
	 * @see LiricTCPServerConnectionThread#sendAcknowledge
	 * @see #commandThread
	 */
	protected void processAcknowledge()
	{
		if(acknowledge == null)
		{
			liric.error(this.getClass().getName()+":processAcknowledge:"+
				command.getClass().getName()+":acknowledge was null.");
			return;
		}
	// send acknowledge to Liric client.
		try
		{
			commandThread.sendAcknowledge(acknowledge);
		}
		catch(IOException e)
		{
			liric.error(this.getClass().getName()+":processAcknowledge:"+
				     command.getClass().getName()+":sending acknowledge to client failed:",e);
		}
	}

	/**
	 * This routine processes the done object returned by the server. 
	 * It prints out the basic return values in done.
	 */
	protected void processDone()
	{
		ACK acknowledge = null;

		if(done == null)
		{
			liric.error(this.getClass().getName()+":processDone:"+
				     command.getClass().getName()+":done was null.");
			return;
		}
	// construct an acknowledgement to sent to the Liric client to tell it how long to keep waiting
	// it currently returns the time the Liric origianally asked for to complete this command
	// This is because the Liric assumed zero time for all sub-commands.
		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(commandThread.getAcknowledgeTime());
		try
		{
			commandThread.sendAcknowledge(acknowledge);
		}
		catch(IOException e)
		{
			liric.error(this.getClass().getName()+":processDone:"+
				     command.getClass().getName()+":sending acknowledge to client failed:",e);
		}
	}
}
