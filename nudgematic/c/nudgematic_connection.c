/* nudgematic_connection.c
** Nudgematic mechanism library
*/
/**
 * Routines handling connecting to the Nudgematic mechanism..
 * @author Chris Mottram
 * @version $Revision$
 */
/**
 * Define Default Source to get BSD prototypes, including cfmakeraw.
 */
/*#define _BSD_SOURCE*/
#define _DEFAULT_SOURCE
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_C_SOURCE 199309L

#include <errno.h>   /* Error number definitions */
#include <fcntl.h>   /* File control definitions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h> /* POSIX terminal control definitions */
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#ifdef MUTEXED
#include <pthread.h>
#endif

#include "log_udp.h"

#include "nudgematic_general.h"
#include "nudgematic_connection.h"

/* internal data structures */
/**
 * Structure holding information on the conenction to the Arduino Mega.
 * <dl>
 * <dt>Serial_Fd</dt> <dd>The serial file descriptor for communications with the Arduino Mega.</dd>
 * <dt>Serial_Options_Saved</dt> <dd>The saved set of serial options.</dd>
 * <dt>Serial_Options</dt> <dd>The set of serial options configured.</dd>
 * <dt></dt> <dd></dd>
 * </dl>
 */
struct Nudgematic_Connection_Struct
{
	int Serial_Fd;
	struct termios Serial_Options_Saved;
	struct termios Serial_Options;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * Instance of the structure holding information on the conenction to the Arduino Mega.
 * @see #Nudgematic_Connection_Struct
 */
static struct Nudgematic_Connection_Struct Nudgematic_Connection_Data;

/**
 * Variable holding error code of last operation performed.
 */
static int Connection_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see nudgematic_general.html#NUDGEMATIC_GENERAL_ERROR_STRING_LENGTH
 */
static char Connection_Error_String[NUDGEMATIC_GENERAL_ERROR_STRING_LENGTH] = "";

/* --------------------------------------------------------
** External Functions
** -------------------------------------------------------- */
/**
 * Routine to open a connection to the Nudgematic. 
 * @param device_name A string representing the serial device filename presented by the plugged in Arduino Mega 
 *                    (e.g. "/dev/ttyACM0").
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Nudgematic_Connection_Data
 * @see #Connection_Error_Number
 * @see #Connection_Error_String
 * @see nudgematic_general.html#Nudgematic_General_Log_Format
 */
int Nudgematic_Connection_Open(const char* device_name)
{
	int open_errno,retval;

	if(device_name == NULL)
	{
		Connection_Error_Number = 1;
		sprintf(Connection_Error_String,"Nudgematic_Connection_Open:device_name was NULL.");
		return FALSE;
	}
#if LOGGING > 0
	Nudgematic_General_Log_Format(LOG_VERBOSITY_TERSE,
				      "Nudgematic_Connection_Open: Opening connection todevice name '%s'.",device_name);
#endif /* LOGGING */
	/* open the serial connection, read/write, not a controlling terminal, don't wait for DCD signal line. */
	Nudgematic_Connection_Data.Serial_Fd = open(device_name, O_RDWR | O_NOCTTY | O_NDELAY);
	if(Nudgematic_Connection_Data.Serial_Fd < 0)
	{
		open_errno = errno;
		Connection_Error_Number = 2;
		sprintf(Connection_Error_String,"Nudgematic_Connection_Open:Device '%s' failed to open (%d).",
			device_name,open_errno);
		return FALSE;
		
	}
#if LOGGING > 0
	Nudgematic_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
				      "Nudgematic_Connection_Open: Using Serial FD %d.",Nudgematic_Connection_Data.Serial_Fd);
#endif /* LOGGING */
	/* make non-blocking */
	retval = fcntl(Nudgematic_Connection_Data.Serial_Fd, F_SETFL, FNDELAY);
	if(retval != 0)
	{
		open_errno = errno;
		close(Nudgematic_Connection_Data.Serial_Fd);
		Connection_Error_Number = 3;
		sprintf(Connection_Error_String,"Nudgematic_Connection_Open:fcntl failed (%d).",open_errno);
		return FALSE;
	}
	retval = fcntl(Nudgematic_Connection_Data.Serial_Fd, F_SETFL, FASYNC);
	if(retval != 0)
	{
		open_errno = errno;
		close(Nudgematic_Connection_Data.Serial_Fd);
		Connection_Error_Number = 4;
		sprintf(Connection_Error_String,"Nudgematic_Connection_Open: fcntl failed (%d).",open_errno);
		return FALSE;
	}
	/* get current serial options */
	retval = tcgetattr(Nudgematic_Connection_Data.Serial_Fd,&(Nudgematic_Connection_Data.Serial_Options_Saved));
	if(retval != 0)
	{
		open_errno = errno;
		Connection_Error_Number = 5;
		sprintf(Connection_Error_String,"Nudgematic_Connection_Open: tcgetattr failed (%d).",open_errno);
		return FALSE;
	}
	/* initialise new serial options */
	bzero(&(Nudgematic_Connection_Data.Serial_Options), sizeof(Nudgematic_Connection_Data.Serial_Options));
	/* set control flags and baud rate */
	Nudgematic_Connection_Data.Serial_Options.c_cflag |= B115200 | CS8 | CLOCAL | CREAD;
#ifdef CNEW_RTSCTS
	Nudgematic_Connection_Data.Serial_Options.c_cflag &= ~CNEW_RTSCTS;/* disable flow control */
#endif
#ifdef CRTSCTS
	Nudgematic_Connection_Data.Serial_Options.c_cflag &= ~CRTSCTS;/* disable flow control */
#endif
	/* select raw input, not line input. */
	Nudgematic_Connection_Data.Serial_Options.c_lflag = 0;
	/* ignore parity errors */
	Nudgematic_Connection_Data.Serial_Options.c_iflag = IGNPAR;
	/* set raw output */
	Nudgematic_Connection_Data.Serial_Options.c_oflag = 0;
	/* blocking read until 0 char arrives */
	Nudgematic_Connection_Data.Serial_Options.c_cc[VMIN]=0;
	Nudgematic_Connection_Data.Serial_Options.c_cc[VTIME]=0;	
#if LOGGING > 9
	Nudgematic_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
	      "Nudgematic_Connection_Open:New Attr:Input:%#x,Output:%#x,Local:%#x,Control:%#x,Min:%c,Time:%c.",
		       Nudgematic_Connection_Data.Serial_Options.c_iflag,Nudgematic_Connection_Data.Serial_Options.c_oflag,
		       Nudgematic_Connection_Data.Serial_Options.c_lflag,Nudgematic_Connection_Data.Serial_Options.c_cflag,
		       Nudgematic_Connection_Data.Serial_Options.c_cc[VMIN],Nudgematic_Connection_Data.Serial_Options.c_cc[VTIME]);
#endif /* LOGGING */
 	/* set new options */
#if LOGGING > 5
	Nudgematic_General_Log(LOG_VERBOSITY_VERY_VERBOSE,"Nudgematic_Connection_Open:Setting serial options.");
#endif /* LOGGING */
	tcflush(Nudgematic_Connection_Data.Serial_Fd, TCIFLUSH);
	retval = tcsetattr(Nudgematic_Connection_Data.Serial_Fd,TCSANOW,&(Nudgematic_Connection_Data.Serial_Options));
	if(retval != 0)
	{
		open_errno = errno;
		Connection_Error_Number = 6;
		sprintf(Connection_Error_String,"Nudgematic_Connection_Open: tcsetattr failed (%d).",open_errno);
		return FALSE;
	}
	/* re-get current serial options to see what was set */
#if LOGGING > 5
	Nudgematic_General_Log(LOG_VERBOSITY_VERY_VERBOSE,"Nudgematic_Connection_Open:Re-Getting new serial options.");
#endif /* LOGGING */
	retval = tcgetattr(Nudgematic_Connection_Data.Serial_Fd,&(Nudgematic_Connection_Data.Serial_Options));
	if(retval != 0)
	{
		open_errno = errno;
		Connection_Error_Number = 7;
		sprintf(Connection_Error_String,"Nudgematic_Connection_Open: re-get tcgetattr failed (%d).",open_errno);
		return FALSE;
	}
#if LOGGING > 5
	Nudgematic_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
	      "Nudgematic_Connection_Open:New Get Attr:Input:%#x,Output:%#x,Local:%#x,Control:%#x,Min:%c,Time:%c.",
		       Nudgematic_Connection_Data.Serial_Options.c_iflag,Nudgematic_Connection_Data.Serial_Options.c_oflag,
		       Nudgematic_Connection_Data.Serial_Options.c_lflag,Nudgematic_Connection_Data.Serial_Options.c_cflag,
		       Nudgematic_Connection_Data.Serial_Options.c_cc[VMIN],Nudgematic_Connection_Data.Serial_Options.c_cc[VTIME]);
#endif /* LOGGING */
	/* clean I & O device */
	tcflush(Nudgematic_Connection_Data.Serial_Fd,TCIOFLUSH);
#if LOGGING > 0
	Nudgematic_General_Log(LOG_VERBOSITY_TERSE,"Nudgematic_Connection_Open:Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to close the connection to the Nudgematic. 
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Nudgematic_Connection_Data
 * @see #Connection_Error_Number
 * @see #Connection_Error_String
 * @see nudgematic_general.html#Nudgematic_General_Log_Format
 */
int Nudgematic_Connection_Close(void)
{
	int retval,close_errno;

#if LOGGING > 0
	Nudgematic_General_Log(LOG_VERBOSITY_TERSE,"Nudgematic_Connection_Close:Started.");
#endif /* LOGGING */
#if LOGGING > 5
	Nudgematic_General_Log(LOG_VERBOSITY_VERY_VERBOSE,"Nudgematic_Connection_Close:Flushing serial line.");
#endif /* LOGGING */
	tcflush(Nudgematic_Connection_Data.Serial_Fd, TCIFLUSH);
#if LOGGING > 5
	Nudgematic_General_Log(LOG_VERBOSITY_VERY_VERBOSE,"Nudgematic_Connection_Close:Resetting serial options.");
#endif /* LOGGING */
	retval = tcsetattr(Nudgematic_Connection_Data.Serial_Fd,TCSANOW,&(Nudgematic_Connection_Data.Serial_Options_Saved));
	if(retval != 0)
	{
		close_errno = errno;
		Connection_Error_Number = 8;
		sprintf(Connection_Error_String,"Nudgematic_Connection_Close: tcsetattr failed (%d).",close_errno);
		return FALSE;
	}
#if LOGGING > 1
	Nudgematic_General_Log(LOG_VERBOSITY_VERY_VERBOSE,"Nudgematic_Connection_Close:Closing file descriptor.");
#endif /* LOGGING */
	retval = close(Nudgematic_Connection_Data.Serial_Fd);
	if(retval < 0)
	{
		close_errno = errno;
		Connection_Error_Number = 9;
		sprintf(Connection_Error_String,"Nudgematic_Connection_Close: close failed (%d,%d,%d).",Nudgematic_Connection_Data.Serial_Fd,retval,close_errno);
		return FALSE;
	}
	return TRUE;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Connection_Error_Number
 */
int Nudgematic_Connection_Get_Error_Number(void)
{
	return Connection_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Connection_Error_Number
 * @see #Connection_Error_String
 * @see nudgematic_general.html#Nudgematic_General_Get_Current_Time_String
 */
void Nudgematic_Connection_Error(void)
{
	char time_string[32];

	Nudgematic_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Connection_Error_Number == 0)
		sprintf(Connection_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s Nudgematic_Connection:Error(%d) : %s\n",time_string,
		Connection_Error_Number,Connection_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Connection_Error_Number
 * @see #Connection_Error_String
 * @see nudgematic_general.html#Nudgematic_General_Get_Current_Time_String
 */
void Nudgematic_Connection_Error_To_String(char *error_string)
{
	char time_string[32];

	Nudgematic_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Connection_Error_Number == 0)
		sprintf(Connection_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s Nudgematic_Connection:Error(%d) : %s\n",time_string,
		Connection_Error_Number,Connection_Error_String);
}
