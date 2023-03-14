/* nudgematic_connection.c
** Nudgematic mechanism library
*/
/**
 * Routines handling connecting to the Nudgematic mechanism..
 * @author Chris Mottram
 * @version $Revision$
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_C_SOURCE 199309L

#include <errno.h>   /* Error number definitions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#ifdef MUTEXED
#include <pthread.h>
#endif

#include "nudgematic_general.h"
#include "nudgematic_connection.h"

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";

/* --------------------------------------------------------
** External Functions
** -------------------------------------------------------- */
/**
 * Routine to open a connection to the Nudgematic. 
 * @return The routine returns TRUE on success and FALSE on failure.
 */
int Nudgematic_Connection_Open(const char* device_name)
{
	return TRUE;
}

/**
 * Routine to close the connection to the Nudgematic. 
 * @return The routine returns TRUE on success and FALSE on failure.
 */
int Nudgematic_Connection_Close(void)
{
	return TRUE;
}

