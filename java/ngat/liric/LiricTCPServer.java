// LiricTCPServer.java
// $Header$
package ngat.liric;

import java.lang.*;
import java.io.*;
import java.net.*;

import ngat.net.*;

/**
 * This class extends the TCPServer class for the Liric application.
 * @author Chris Mottram
 * @version $Revision$
 */
public class LiricTCPServer extends TCPServer
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * Field holding the instance of Liric currently executing, so we can pass this to spawned threads.
	 */
	private Liric liric = null;

	/**
	 * The constructor.
	 * @param name The name of the server thread.
	 * @param portNumber The port number to wait for connections on.
	 */
	public LiricTCPServer(String name,int portNumber)
	{
		super(name,portNumber);
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
	 * This routine spawns threads to handle connection to the server. This routine
	 * spawns LiricTCPServerConnectionThread threads.
	 * The routine also sets the new threads priority to higher than normal. This makes the thread
	 * reading it's command a priority so we can quickly determine whether the thread should
	 * continue to execute at a higher priority.
	 * @see LiricTCPServerConnectionThread
	 */
	public void startConnectionThread(Socket connectionSocket)
	{
		LiricTCPServerConnectionThread thread = null;

		thread = new LiricTCPServerConnectionThread(connectionSocket);
		thread.setLiric(liric);
		thread.setPriority(liric.getStatus().getThreadPriorityInterrupt());
		thread.start();
	}
}
