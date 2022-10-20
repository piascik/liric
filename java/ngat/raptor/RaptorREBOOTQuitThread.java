// RaptorREBOOTQuitThread.java
// $Header$
package ngat.raptor;

import java.lang.*;
import java.io.*;

/**
 * This class is a thread that is started when the Raptor is to terminate.
 * A thread is passed in, which must terminate before System.exit is called.
 * This is used in, for instance, the REBOOTImplementation, so that the 
 * REBOOT's DONE mesage is returned to the client before the Raptor is terminated.
 * @author Chris Mottram
 * @version $Revision$
 */
public class RaptorREBOOTQuitThread extends Thread
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id$");
	/**
	 * The Thread, that has to terminatre before this thread calls System.exit
	 */
	private Thread waitThread = null;
	/**
	 * Field holding the instance of the raptor currently executing, used to access error handling routines etc.
	 */
	private Raptor raptor = null;
	/**
	 * The exit value to exit the JVM with. Normally (and by default) 0.
	 */
	private int exitValue = 0;

	/**
	 * The constructor.
	 * @param name The name of the thread.
	 */
	public RaptorREBOOTQuitThread(String name)
	{
		super(name);
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
	 * Method to set a thread, such that this thread will not call System.exit until
	 * that thread has terminated.
	 * @param t The thread to wait for.
	 * @see #waitThread
	 */
	public void setWaitThread(Thread t)
	{
		waitThread = t;
	}

	/**
	 * Method to set the exit value to pass into System.exit.
	 * @param i The exit value to pass into System.exit
	 * @see #exitValue
	 */
	public void setExitValue(int i)
	{
		exitValue = i;
	}

	/**
	 * Run method, called when the thread is started.
	 * If the waitThread is non-null, we try to wait until it has terminated.
	 * System.exit(exitValue) is then called.
	 * @see #waitThread
	 * @see #exitValue
	 */
	public void run()
	{
		if(waitThread != null)
		{
			try
			{
				waitThread.join();
			}
			catch (InterruptedException e)
			{
				raptor.error(this.getClass().getName()+":run:",e);
			}
		}
		System.exit(exitValue);
	}
}
