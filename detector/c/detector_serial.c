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
/**
 * Serial command byte to read from the system status register.
 */
#define SERIAL_SYSTEM_STATUS_REGISTER_READ    (0x49)
/**
 * Command termintor / successful command acknowledge.
 * See OWL_640_Cooled_IM_v1_0.pdf, Sec 4.2 "ETX/EROOR codes", P20."
 */
#define SERIAL_ETX                            (0x50)
/**
 * Partial command packet received, camera timed out waiting for end of packet. Command not processed
 * See OWL_640_Cooled_IM_v1_0.pdf, Sec 4.2 "ETX/EROOR codes", P20."
 */
#define SERIAL_ETX_SER_TIMEOUT                (0x51)
/**
 * Check sum transmitted by host did not match that calculated for the packet. Command not processed.
 * See OWL_640_Cooled_IM_v1_0.pdf, Sec 4.2 "ETX/EROOR codes", P20."
 */
#define SERIAL_ETX_CK_SUM_ERR                 (0x52)
/**
 * An I2C command has been received from the Host but failed internally in the camera.
 * See OWL_640_Cooled_IM_v1_0.pdf, Sec 4.2 "ETX/EROOR codes", P20."
 */
#define SERIAL_ETX_I2C_ERR                    (0x53)
/**
 * Data was detected on serial line, command not recognized
 * See OWL_640_Cooled_IM_v1_0.pdf, Sec 4.2 "ETX/EROOR codes", P20."
 */
#define SERIAL_ETX_UNKNOWN_CMD                (0x54)
/**
 * Host Command to access the camera EPROM successfully received by camera but not processed as EPROM is busy. I.e.
 * FPGA trying to boot.
 * See OWL_640_Cooled_IM_v1_0.pdf, Sec 4.2 "ETX/EROOR codes", P20."
 */
#define SERIAL_ETX_DONE_LOW                   (0x55)

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
 * Get the system status from the Raptor's serial interface, and parse the results.
 * @param status An unsigned character byte, if not NULL, on return contains the raw status byte.
 * @param checksum_enabled The address on an integer, if not NULL, on return contains a boolean, 
 *        TRUE if checksum's are enabled.
 * @param cmd_ack_enabled The address on an integer, if not NULL, on return contains a boolean, 
 *        TRUE if command acknowledgements are enabled.
 * @param fpga_booted The address on an integer, if not NULL, on return contains a boolean, 
 *        TRUE if the FPGA has booted successfully.
 * @param fpga_in_reset The address on an integer, if not NULL, on return contains a boolean, 
 *        TRUE if the FPGA is held in RESET.
 * @param eprom_comms_enabled The address on an integer, if not NULL, on return contains a boolean, 
 *        TRUE if comms are enabled to the FPGA EPROM.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Serial_Error_Number/Serial_Error_String are set.
 * @see #SERIAL_SYSTEM_STATUS_REGISTER_READ
 * @see #SERIAL_ETX
 * @see #Serial_Error_Number
 * @see #Serial_Error_String
 * @see #Detector_Serial_Compute_Checksum
 * @see #Detector_Serial_Command
 */
int Detector_Serial_Command_Get_System_Status(unsigned char *status,int *checksum_enabled,
						     int *cmd_ack_enabled,int *fpga_booted,int *fpga_in_reset,
						     int *eprom_comms_enabled)
{
	unsigned char command_buffer[16];
	unsigned char reply_buffer[16];
	int command_buffer_length;
	
	Serial_Error_Number = 0;
#if LOGGING > 9
	Detector_General_Log(LOG_VERBOSITY_VERBOSE,"Detector_Serial_Command_Get_System_Status:Started.");
#endif
	/* setup command buffer */
	command_buffer_length = 0;
	command_buffer[command_buffer_length++] = SERIAL_SYSTEM_STATUS_REGISTER_READ;
	command_buffer[command_buffer_length++] = SERIAL_ETX;
	/* add checksum */
	if(!Detector_Serial_Compute_Checksum(command_buffer,&command_buffer_length))
		return FALSE;
	/* send command and get reply. We don't know if checksums/acks are currently enabled, 
	** so only expect the status byte */
	if(!Detector_Serial_Command(command_buffer,command_buffer_length,reply_buffer,1))
		return FALSE;
	if(status != NULL)
	{
		(*status) = reply_buffer[0];
	}
	if(checksum_enabled != NULL)
		(*checksum_enabled) = reply_buffer[0]&(1<<6);
	if(cmd_ack_enabled != NULL)
		(*cmd_ack_enabled) = reply_buffer[0]&(1<<4);
	if(fpga_booted != NULL)
		(*fpga_booted) = reply_buffer[0]&(1<<2);
	if(fpga_in_reset != NULL)
		(*fpga_in_reset) = (reply_buffer[0]&(1<<1)) == 0;
	if(eprom_comms_enabled != NULL)
		(*eprom_comms_enabled) = reply_buffer[0]&(1<<0);
#if LOGGING > 9
	Detector_General_Log(LOG_VERBOSITY_VERBOSE,"Detector_Serial_Command_Get_System_Status:Finished.");
#endif
	return TRUE;
}

/**
 * Low level command to send a Raptor Ninox-640 command over the 
 * camera link's internal serial connection to the camera head, and optionally wait for a reply.
 * The amera link's internal serial connection should have been previously opened/configured 
 * by calling Detector_Serial_Open.
 * <ul>
 * <li>We flush the serial port input and output stream, by calling pxd_serialFlush.
 * <li>We write command_buffer_length bytes of command_buffer to the serial stream by calling pxd_serialWrite.
 * <li>If reply_buffer is NOT NULL, we are expecting a reply, therefore we:
 *     <ul>
 *     <li>Initialise reply_bytes_read to zero and take a timestamp reply_start_time.
 *     <li>Enter a loop while reply_bytes_read is less than the expected_reply_length:
 *         <ul>
 *         <li>Read some serial data into reply_buffer by callling pxd_serialRead.
 *         <li>If we received no bytes, sleep a while (500uS) before trying again.
 *         <li>Take a timestamp, and if the elapsed time between reply_start_time and now is greater than 
 *             Serial_Data.Reply_Timeout_Ms timeout as an error.
 *         </ul>
 *     </ul>
 * <li>
 * </ul>
 * @param command_buffer A previously allocated array of unsigned characters of at least length command_buffer_length,
 *      each character containing a byte to send to the Raptor Ninox-640 camera head. The command can be binary in
 *      nature and the 'string' is not NULL terminated (and may indeed include NULL bytes as command bytes/parameters).
 *      The command buffer should normally terminate with an ETX byte and a checksum byte 
 *      (if checksums have been switched on).
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
int Detector_Serial_Command(unsigned char *command_buffer,int command_buffer_length,
			    unsigned char *reply_buffer,int expected_reply_length)
{
	char print_buffer[DETECTOR_GENERAL_ERROR_STRING_LENGTH];
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
				    Detector_Serial_Print_Command(command_buffer,command_buffer_length,
								  print_buffer,DETECTOR_GENERAL_ERROR_STRING_LENGTH));
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
					    Detector_Serial_Print_Command(reply_buffer,reply_bytes_read,
					      print_buffer,DETECTOR_GENERAL_ERROR_STRING_LENGTH));
#endif
	}/* end if reply_buffer != NULL */
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_VERBOSE,"Detector_Serial_Command:Finished.");
#endif
	return TRUE;
}

/**
 * This routine computes a checksum for the specified command buffer, and adds it to the end of the buffer.
 * The command buffer should already have the ETX terminator byte included.
 * @param buffer An array of unsigned chars, containing command bytes. The array should have memory allocated to it 
 *        at least one byte larger than the inout buffer_length, so the checksum byte can be added to the buffer 
 *        at buffer[(*buffer_length)].
 * @param buffer_length The address of an integer. At the start of this function the integer should contain the 
 *        number of bytes in the buffer. On successful execution of the function, this should have been incremented 
 *        by 1, and a checksum byte appended to the end of the buffer 
 *        (which should have extra memory allocated for this).
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Serial_Error_Number/Serial_Error_String are set.
 * @see #Serial_Error_Number
 * @see #Serial_Error_String
 * @see #Detector_Serial_Print_Command
 * @see detector_general.html#Detector_General_Log
 * @see detector_general.html#Detector_General_Log_Format
 */
int Detector_Serial_Compute_Checksum(unsigned char *buffer,int *buffer_length)
{
	char print_buffer[DETECTOR_GENERAL_ERROR_STRING_LENGTH];
	unsigned char checksum_byte;
	int i;
	
	Serial_Error_Number = 0;
	if(buffer == NULL)
	{
		Serial_Error_Number = 12;
		sprintf(Serial_Error_String,"Detector_Serial_Compute_Checksum:buffer is NULL.");
		return FALSE;	
	}
	if(buffer_length == NULL)
	{
		Serial_Error_Number = 13;
		sprintf(Serial_Error_String,"Detector_Serial_Compute_Checksum:buffer_length is NULL.");
		return FALSE;	
	}
#if LOGGING > 5
	Detector_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
				    "Detector_Serial_Compute_Checksum:Started with buffer '%s'.",
				    Detector_Serial_Print_Command(buffer,(*buffer_length),print_buffer,
								  DETECTOR_GENERAL_ERROR_STRING_LENGTH));
#endif
	/* do exclusive or of buffer contents */
	checksum_byte = 0;
	for(i=0; i < (*buffer_length); i++)
	{
		checksum_byte ^= buffer[i];
	}
	buffer[(*buffer_length)++] = checksum_byte;
#if LOGGING > 5
	Detector_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
				    "Detector_Serial_Compute_Checksum:Finished with buffer '%s'.",
				    Detector_Serial_Print_Command(buffer,(*buffer_length),print_buffer,256));
#endif
	return TRUE;
}

/**
 * Routine to translate a command/reply buffer into printable hex format.
 * @param buffer An unsigned character string containing buffer_length bytes, to print in a hex format.
 * @param buffer_length The number of characters in the buffer.
 * @param string_buffer A string to store the hex string representation of buffer.
 * @param string_buffer_length The allocated length of string_buffer.
 * @return A pointer to string_buffer containing the values in the buffer printed as
 *         '0xNN<spc>[0xNN}...'.
 * @see #Serial_Error_Number
 * @see #Serial_Error_String
 * @see detector_general.html#Detector_General_Log
 * @see detector_general.html#Detector_General_Log_Format
 */
char* Detector_Serial_Print_Command(unsigned char *buffer,int buffer_length,
				    char *string_buffer,int string_buffer_length)
{
	char char_buff[16];
	int i,done;

	done = FALSE;
	i=0;
	strcpy(string_buffer,"");
	while(done == FALSE)
	{
		sprintf(char_buff,"%#02x ",buffer[i]);
		if((strlen(string_buffer)+strlen(char_buff)+1) < string_buffer_length)
		{
			strcat(string_buffer,char_buff);
			i++;
			done = (i == buffer_length);
		}
		else
		{
			if((strlen(string_buffer)+strlen("...")+1) < string_buffer_length)
			{
				strcat(string_buffer,"...");
			}
			else
				strcat(string_buffer+(string_buffer_length-(strlen("...")+1)),"...");
			done = TRUE;
		}
	}
	return string_buffer;
}

/**
 * Routine to parse a string of the form '0xNN [0xNN...]', representing a series of unsigned command bytes, 
 * into a command buffer, suitable for sending over the serial link.
 * @param string_buffer A string containing the bytes to put into the command buffer in the text form '0xNN [0xNN...]'.
 * @param command_buffer A pointer to an unsigned character array (representing a serial of command bytes) 
 *                       to put the parsed data into.
 * @param command_buffer_max_length The allocated length of the command_buffer.
 * @param command_buffer_length The address of an integer, on a successful return the number of parsed bytes put into
 *        the command buffer is returned.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Serial_Error_Number/Serial_Error_String are set.
 * @see #Serial_Error_Number
 * @see #Serial_Error_String
 * @see #Detector_Serial_Print_Command
 * @see detector_general.html#DETECTOR_GENERAL_ERROR_STRING_LENGTH
 * @see detector_general.html#Detector_General_Log
 * @see detector_general.html#Detector_General_Log_Format
 */
int Detector_Serial_Parse_Hex_String(char *string_buffer,unsigned char *command_buffer,int command_buffer_max_length,
				     int *command_buffer_length)
{
	char print_buffer[DETECTOR_GENERAL_ERROR_STRING_LENGTH];
	int done,retval,string_buffer_index,char_count;
	unsigned int value;
	
	Serial_Error_Number = 0;
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_VERY_VERBOSE,"Detector_Serial_Parse_Hex_String:Started.");
#endif
	if(string_buffer == NULL)
	{
		Serial_Error_Number = 6;
		sprintf(Serial_Error_String,"Detector_Serial_Parse_Hex_String:string_buffer is NULL.");
		return FALSE;	
	}
	if(command_buffer == NULL)
	{
		Serial_Error_Number = 7;
		sprintf(Serial_Error_String,"Detector_Serial_Parse_Hex_String:command_buffer is NULL.");
		return FALSE;	
	}
	if(command_buffer_length == NULL)
	{
		Serial_Error_Number = 8;
		sprintf(Serial_Error_String,"Detector_Serial_Parse_Hex_String:command_buffer_length is NULL.");
		return FALSE;	
	}
	(*command_buffer_length) = 0;
	string_buffer_index = 0;
	done = FALSE;
	while(done == FALSE)
	{
		retval = sscanf(string_buffer+string_buffer_index,"%x%n",&value,&char_count);
		if(retval == 0) /* no conversion done, end of string ? */
		{
			done = TRUE;
		}
		else if(retval == 1) /* data parsed */
		{
			if(value > 255)
			{
				Serial_Error_Number = 9;
				sprintf(Serial_Error_String,
					"Detector_Serial_Parse_Hex_String:parse failed:"
					"value too large value = %d (string = %s,string_buffer_index = %d).",
					value,string_buffer,string_buffer_index);
				return FALSE;	
			}
			if((*command_buffer_length) >= command_buffer_max_length)
			{
				Serial_Error_Number = 10;
				sprintf(Serial_Error_String,
					"Detector_Serial_Parse_Hex_String:parse failed:command buffer too short:"
					"%d vs %d, value = %d (string = '%s',string_buffer_index = %d).",
					(*command_buffer_length),command_buffer_max_length,value,
					string_buffer,string_buffer_index);
				return FALSE;	
			}
			command_buffer[(*command_buffer_length)] = value;
#if LOGGING > 9
			Detector_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
						    "Detector_Serial_Parse_Hex_String:command buffer[%d] = %02x (%d).",
						    (*command_buffer_length),command_buffer[(*command_buffer_length)],
						    command_buffer[(*command_buffer_length)]);
#endif
			string_buffer_index += char_count;
			(*command_buffer_length)++;
		}
		else
		{
			if(string_buffer_index = strlen(string_buffer))
			{
				/* we have reached the end of the input string */
				done = TRUE;
			}
			else
			{
				Serial_Error_Number = 11;
				sprintf(Serial_Error_String,"Detector_Serial_Parse_Hex_String:parse failed: "
					"retval = %d (string = %s,string_buffer_index = %d).",
					retval,string_buffer,string_buffer_index);
				return FALSE;
			}
		}
	}
#if LOGGING > 1
	Detector_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,
				    "Detector_Serial_Parse_Hex_String:Finished with '%s' parsed as '%s'.",
				    string_buffer,
				    Detector_Serial_Print_Command(command_buffer,(*command_buffer_length),
								  print_buffer,DETECTOR_GENERAL_ERROR_STRING_LENGTH));
#endif
	return TRUE;
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
