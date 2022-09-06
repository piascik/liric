/* detector_serial.c
** Raptor Ninox-640 Infrared detector library : serial connection routines.
*/
/**
 * Routines to communicate with the Raptor Ninox-640 Infrared detector over the inbuilt camera link serial interface.
 * Commands to control the detector TEC (thermo-electric cooler) and read the temperature are sent of this interface.
 * @author Chris Mottram
 * @version $Revision$
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log_udp.h"
#include "detector_general.h"
#include "detector_serial.h"
#include "xcliball.h"

/* hash defines */
/**
 * Define which camera we are talking to the xclib way, the first one.
 */
#define UNITS	                  1
/**
 * Define a bitwise definition of which cameras we are talking to, to pass into XCLIB functions.
 */
#define UNITSMAP                  ((1<<UNITS)-1)
/**
 * Default length of time to wait for a reply to a command, in milliseconds.
 */
#define DEFAULT_REPLY_TIMEOUT_MS  (1000)

/* data types */
/**
 * Data type holding local data to detector_setup. This consists of the following:
 * <dl>
 * <dt>Reply_Timeout_Ms</dt> <dd>The number of milliseconds to wait for a reply to a command.</dd>
 * </dl>
 */
struct Serial_Struct
{
	int Reply_Timeout_Ms;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of Serial_Struct that contains local data for this module. This is initialised as follows:
 * <dl>
 * <dt>Reply_Timeout_Ms</dt> <dd>DEFAULT_REPLY_TIMEOUT_MS</dd>
 * </dl>
 * @see #DEFAULT_REPLY_TIMEOUT_MS
 */
static struct Serial_Struct Serial_Data = 
{
	DEFAULT_REPLY_TIMEOUT_MS
};

/**
 * Variable holding error code of last operation performed.
 */
static int Serial_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see detector_general.html#DETECTOR_GENERAL_ERROR_STRING_LENGTH
 */
static char Serial_Error_String[DETECTOR_GENERAL_ERROR_STRING_LENGTH] = "";

/* internal functions */

/* --------------------------------------------------------
** External Functions
** -------------------------------------------------------- */
/**
 * Routine to configure and open the camera-link's internal serial connection to the Raptor Ninox-640 camera head.
 * <ul>
 * <li>We call pxd_serialConfigure to configure the camera-link's internal serial link to 115200 baud, 8 data bits, 
 *     1 stop bit.
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Serial_Error_Number/Serial_Error_String are set.
 * @see #UNITSMAP
 * @see #Serial_Error_Number
 * @see #Serial_Error_String
 * @see #Serial_Data
 */
int Detector_Serial_Open(void)
{
	int retval;
	
	Serial_Error_Number = 0;
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Serial_Open:Started.");
#endif
	/* open serial connection. See OWL_640_Cooled_IM_v1_0.pdf Sec 4 (SERIAL COMMUNICATION):
	** - 115200 baud
	** - 1 start bit
	** - 8 data bits
	** - 1 stop bit
	*/
	retval = pxd_serialConfigure(UNITSMAP,0,115200.0,8,0,1,0,0,0);
	if(retval < 0)
	{
		Serial_Error_Number = 1;
		sprintf(Serial_Error_String,"Detector_Serial_Open:pxd_serialConfigure failed: %s (%d).",
			pxd_mesgErrorCode(retval),retval);
		return FALSE;
	}
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Serial_Open:Finished.");
#endif
	return TRUE;
}

/**
 * Low level command to send a Raptor Ninox-640 command over the 
 * camera link's internal serial connection to the camera head, and optionally wait for a reply.
 * The amera link's internal serial connection should have been previously opened/configured 
 * by calling Detector_Serial_Open.
 * <ul>
 * <li>
 * </ul>
 * @param command_buffer A previously allocated array of characters of at least length command_buffer_length,
 *      each character containing a byte to send to the Raptor Ninox-640 camera head. The command can be binary in
 *      nature and the 'string' is not NULL terminated (and may indeed include NULL bytes as command bytes/parameters).
 * @param command_buffer_length The number of bytes to send to the Raptor Ninox-640 camera head.
 * @param reply_buffer A previously allocated array of characters of at least length expected_reply_length. 
 *                     Any reply read from the camera head will be stored in this arry. 
 *                     The array can be passed in as NULL if no reply is required.
 * @param expected_reply_length The number of bytes to attempt to read as a reply. 
 *                     The reply_buffer must be at least this many bytes long 
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Serial_Error_Number/Serial_Error_String are set.
 * @see #UNITSMAP
 * @see #Detector_Serial_Open
 * @see #Detector_Serial_Print_Command
 * @see #Serial_Error_Number
 * @see #Serial_Error_String
 * @see #Serial_Data
 * @see detector_general.html#DETECTOR_GENERAL_ONE_SECOND_MS
 * @see detector_general.html#Detector_General_Log
 * @see detector_general.html#Detector_General_Log_Format
 */
int Detector_Serial_Command(char *command_buffer,int command_buffer_length,
			    char *reply_buffer,int expected_reply_length)
{
	struct timespec reply_start_time,sleep_time,current_time;
	int reply_bytes_read,retval;
	
	Serial_Error_Number = 0;
	if(command_buffer == NULL)
	{
		Serial_Error_Number = 2;
		sprintf(Serial_Error_String,"Detector_Serial_Command:command_buffer is NULL.");
		return FALSE;	
	}
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_VERBOSE,"Detector_Serial_Command:Started.");
#endif
	/* flush the serial port */
	retval = pxd_serialFlush(UNITSMAP,0,1,1);
	/* write command message */
#if LOGGING > 9
	Detector_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"Detector_Serial_Command:Writing '%s'.",
				    Detector_Serial_Print_Command(command_buffer,command_buffer_length));
#endif
	retval = pxd_serialWrite(UNITSMAP,0,command_buffer,command_buffer_length);
	if(retval < 0)
	{
		Serial_Error_Number = 3;
		sprintf(Serial_Error_String,"Detector_Serial_Command:pxd_serialWrite failed: %s (%d).",
			pxd_mesgErrorCode(retval),retval);
		return FALSE;
	}
	/* should we  read a reply ? */
	if(reply_buffer != NULL)
	{
		reply_bytes_read = 0;
		/* get a timestamp for the start of waiting foor a reply */
		clock_gettime(CLOCK_REALTIME,&reply_start_time);
		while(reply_bytes_read < expected_reply_length)
		{
			/* try to read some serial data */
			retval = pxd_serialRead(UNITSMAP,0,reply_buffer+reply_bytes_read,
						expected_reply_length-reply_bytes_read);
			if(retval < 0)
			{
				Serial_Error_Number = 4;
				sprintf(Serial_Error_String,"Detector_Serial_Command:pxd_serialRead failed: %s (%d).",
					pxd_mesgErrorCode(retval),retval);
				return FALSE;
			}
			/* if no bytes received since the last check, sleep a bit */
			if(retval == 0) 
			{
				/* sleep a bit (500 us) */
				sleep_time.tv_sec = 0;
				sleep_time.tv_nsec = 500*DETECTOR_GENERAL_ONE_MICROSECOND_NS;
				nanosleep(&sleep_time,&sleep_time);
			}
			reply_bytes_read += retval;
			/* check for timeout. Note fdifftime works in decimal seconds */
			clock_gettime(CLOCK_REALTIME,&current_time);
			if(fdifftime(current_time,reply_start_time) >
			   (((double)(Serial_Data.Reply_Timeout_Ms))/DETECTOR_GENERAL_ONE_SECOND_MS))
			{
				Serial_Error_Number = 5;
				sprintf(Serial_Error_String,
				  "Detector_Serial_Command:Timed out waiting for reply after %.3f s (%d of %d bytes read).",
					fdifftime(current_time,reply_start_time),reply_bytes_read,expected_reply_length);
				return FALSE;
			}
		}/* end while not all reply read */
#if LOGGING > 9
		Detector_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"Detector_Serial_Command:Reply was '%s'.",
					    Detector_Serial_Print_Command(reply_buffer,reply_bytes_read));
#endif
	}/* end if reply_buffer != NULL */
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_VERBOSE,"Detector_Serial_Command:Finished.");
#endif
	return TRUE;
}

/**
 * Routine to translate a command/reply buffer into printable hex format.
 * @param buffer A character string containing buffer_length bytes, to print in a hex format.
 * @param buffer_length The number of characters in the buffer.
 * @return A pointer to a string (on the stack for this function call) containing the values in the buffer printed as
 *         '0xNN<spc>[0xNN}...'.
 */
char* Detector_Serial_Print_Command(char *buffer,int buffer_length)
{
	char buff[DETECTOR_GENERAL_ERROR_STRING_LENGTH];
	char char_buff[16];
	int i,done;

	done = FALSE;
	i=0;
	strcpy(buff,"");
	while(done == FALSE)
	{
		sprintf(char_buff,"%#02x ",buffer[i]);
		if((strlen(buff)+strlen(char_buff)+1) < DETECTOR_GENERAL_ERROR_STRING_LENGTH)
		{
			strcat(buff,char_buff);
			i++;
			done = (i == buffer_length);
		}
		else
		{
			if((strlen(buff)+strlen("...")+1) < DETECTOR_GENERAL_ERROR_STRING_LENGTH)
			{
				strcat(buff,"...");
			}
			done = TRUE;
		}
	}
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Serial_Error_Number
 */
int Detector_Serial_Get_Error_Number(void)
{
	return Serial_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Serial_Error_Number
 * @see #Serial_Error_String
 * @see detector_general.html#Detector_General_Get_Current_Time_String
 */
void Detector_Serial_Error(void)
{
	char time_string[32];

	Detector_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Serial_Error_Number == 0)
		sprintf(Serial_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s Detector_Serial:Error(%d) : %s\n",time_string,Serial_Error_Number,Serial_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Serial_Error_Number
 * @see #Serial_Error_String
 * @see detector_general.html#Detector_General_Get_Current_Time_String
 */
void Detector_Serial_Error_String(char *error_string)
{
	char time_string[32];

	Detector_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Serial_Error_Number == 0)
		sprintf(Serial_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s Detector_Serial:Error(%d) : %s\n",time_string,
		Serial_Error_Number,Serial_Error_String);
}

/* =======================================
**  internal functions 
** ======================================= */
