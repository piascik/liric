// RaptorTCPServer.java
// $Header$
package ngat.raptor;

import java.lang.*;
import java.io.*;
import java.net.*;

import ngat.net.*;

/**
 * This class extends the TCPServer class for the Raptor application.
 * @author Chris Mottram
 * @version $Revision$
 */
public class RaptorTCPServer extends TCPServer
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * Field holding the instance of Raptor currently executing, so we can pass this to spawned threads.
	 */
	private Raptor raptor = null;

	/**
	 * The constructor.
	 * @param name The name of the server thread.
	 * @param portNumber The port number to wait for connections on.
	 */
	public RaptorTCPServer(String name,int portNumber)
	{
		super(name,portNumber);
	}

	/**
	 * Routine to set this objects pointer to the raptor object.
	 * @param o The raptor object.
	 */
	public void setRaptor(Raptor o)
	{
		this.raptor = o;
	}

	/**
	 * This routine spawns threads to handle connection to the server. This routine
	 * spawns RaptorTCPServerConnectionThread threads.
	 * The routine also sets the new threads priority to higher than normal. This makes the thread
	 * reading it's command a priority so we can quickly determine whether the thread should
	 * continue to execute at a higher priority.
	 * @see RaptorTCPServerConnectionThread
	 */
	public void startConnectionThread(Socket connectionSocket)
	{
		RaptorTCPServerConnectionThread thread = null;

		thread = new RaptorTCPServerConnectionThread(connectionSocket);
		thread.setRaptor(raptor);
		thread.setPriority(raptor.getStatus().getThreadPriorityInterrupt());
		thread.start();
	}
}
