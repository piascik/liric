/* usb_pio_connection.c 
** USB-PIO IO board communication library : routines to open and close a connection to the board.
*/
/**
 * Routines to open and close a connection to the USB-PIO IO board.
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
/**
 * This hash define is needed before including headers to give us CRTSCTS.
 */
#define _BSD_SOURCE 1
#include <errno.h>   /* Error number definitions */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "log_udp.h"
#include "usb_pio_general.h"
#include "usb_pio_connection.h"

/* hash defines */
/**
 * Length to use for character arrays.
 */
#define STRING_LENGTH                (256)
/**
 * Maximum length of commands/replies sent to the USB-PIO board.
 */
#define COMMAND_STRING_LENGTH        (32)

/**
 * Data type holding local data to usb_pio_connection. This consists of the following:
 * <dl>
 * <dt>Fd</dt> <dd>The file descriptor of the Linux ttyACM device opened to connect with the usb-pio board.</dd>
 * </dl>
 * @see #STRING_LENGTH
 */
struct Connection_Struct
{
	int Fd;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of Connection_Struct that contains local data for this module.
 * This is statically initialised to the following:
 * <dl>
 * <dt>Fd</dt> <dd>-1</dd>
 * </dl>
 * @see #Connection_Struct
 */
static struct Connection_Struct Connection_Data = 
{
	-1
};

/**
 * Variable holding error code of last operation performed.
 */
static int Connection_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see usb_pio_general.html#USB_PIO_GENERAL_ERROR_STRING_LENGTH
 */
static char Connection_Error_String[USB_PIO_GENERAL_ERROR_STRING_LENGTH] = "";

/* internal functions */
static int Connection_Set_Serial_Attributes(void);
static int Connection_Set_Blocking(int blocking);

/* =======================================
**  external functions 
** ======================================= */
/**
 * Open a connection to the USB-PIO board.
 * <ul>
 * <li>If compiled in the mutex is obtained.
 * <li>The open routine is called on the device name. If successfull the filoe descriptor is stored in 
 *     Connection_Data.Fd
 * <li>We set up the serial port attributres of the opened port, by calling Connection_Set_Serial_Attributes.
 * <li>We set up the non-blocking state of the connection, by calling Connection_Set_Blocking.
 * <li>If compiled in the mutex is released.
 * </ul>
 * @param device_name A string containing the filesystem location of the USB serial device to open.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #STRING_LENGTH
 * @see #Connection_Set_Serial_Attributes
 * @see #Connection_Set_Blocking
 * @see #Connection_Data
 * @see #Connection_Error_Number
 * @see #Connection_Error_String
 * @see usb_pio_general.html#USB_PIO_General_Log_Format
 * @see usb_pio_general.html#USB_PIO_General_Mutex_Lock
 * @see usb_pio_general.html#USB_PIO_General_Mutex_Unlock
 */

int USB_PIO_Connection_Open(const char* device_name)
{
	int open_errno,retval;
	
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_TERSE,"USB_PIO_Connection_Open(device_name=%s): Started.",
					device_name);
#endif /* LOGGING */
	/* initialise error number */
	Connection_Error_Number = 0;
#ifdef MUTEXED
	if(!USB_PIO_General_Mutex_Lock())
	{
		Connection_Error_Number = 1;
		sprintf(Connection_Error_String,"USB_PIO_Connection_Open: failed to lock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
	/* open device connection */
	Connection_Data.Fd = open(device_name,O_RDWR | O_NOCTTY | O_SYNC);
	if(Connection_Data.Fd < 0)
	{
		open_errno = errno;
#ifdef MUTEXED
		USB_PIO_General_Mutex_Unlock();
#endif /* MUTEXED */
		Connection_Error_Number = 2;
		sprintf(Connection_Error_String,"USB_PIO_Connection_Open: open(%s) failed with errno %d.",device_name,
			open_errno);
		return FALSE;
	}
	/* set serial attributes */
	if(!Connection_Set_Serial_Attributes())
	{
#ifdef MUTEXED
		USB_PIO_General_Mutex_Unlock();
#endif /* MUTEXED */
		return FALSE;
	}
	/* set non-blocking status */
	if(!Connection_Set_Blocking(FALSE))
	{
#ifdef MUTEXED
		USB_PIO_General_Mutex_Unlock();
#endif /* MUTEXED */
		return FALSE;
	}
#ifdef MUTEXED
	if(!USB_PIO_General_Mutex_Unlock())
	{
		Connection_Error_Number = 3;
		sprintf(Connection_Error_String,"USB_PIO_Connection_Open: failed to unlock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_TERSE,"USB_PIO_Connection_Open: Finished.");
#endif /* LOGGING */
	return TRUE;	
}

/**
 * Close the previously opened connection to the USB-PIO board. 
 * <ul>
 * <li>If compiled in the mutex is obtained.
 * <li>The close routine is called on the file descriptor held in Connection_Data.Fd.
 * <li>If compiled in the mutex is released.
 * </ul>
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Connection_Data
 * @see #Connection_Error_Number
 * @see #Connection_Error_String
 * @see usb_pio_general.html#USB_PIO_General_Log_Format
 * @see usb_pio_general.html#USB_PIO_General_Mutex_Lock
 * @see usb_pio_general.html#USB_PIO_General_Mutex_Unlock
 */
int USB_PIO_Connection_Close(void)
{
	int close_retval, close_errno;
	
#if LOGGING > 0
	USB_PIO_General_Log(LOG_VERBOSITY_TERSE,"USB_PIO_Connection_Close: Started.");
#endif /* LOGGING */
	/* initialise error number */
	Connection_Error_Number = 0;
#ifdef MUTEXED
	if(!USB_PIO_General_Mutex_Lock())
	{
		Connection_Error_Number = 4;
		sprintf(Connection_Error_String,"USB_PIO_Connection_Close: failed to lock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
	close_retval = close(Connection_Data.Fd);
	if(close_retval < 0)
	{
		close_errno = errno;
#ifdef MUTEXED
		USB_PIO_General_Mutex_Unlock();
#endif /* MUTEXED */
		Connection_Error_Number = 5;
		sprintf(Connection_Error_String,"USB_PIO_Connection_Close: close(%d) failed with errno %d.",
			Connection_Data.Fd,close_errno);
		return FALSE;
	}
	Connection_Data.Fd = -1;
#ifdef MUTEXED
	if(!USB_PIO_General_Mutex_Unlock())
	{
		Connection_Error_Number = 6;
		sprintf(Connection_Error_String,"USB_PIO_Connection_Close: failed to unlock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_TERSE,"USB_PIO_Connection_Close: Finished.");
#endif /* LOGGING */
	return TRUE;	
}

/**
 * Routine to send a command string over the opened connection, receive a reply, and potentially check it against
 * an expected reply.
 * <ul>
 * <li>We check the command string is not NULL, and is not too long (COMMAND_STRING_LENGTH).
 * <li>If compiled in the mutex is obtained.
 * <li>We copy the command string to an internal buffer, and terminate it with a carridge return '\r'.
 * <li>We write the command string to the open connection Fd (file descriptor).
 * <li>If the reply_string is NOT NULL, we:
 *     <ul>
 *     <li>We read up to reply_string_length bytes from the open connection Fd to the reply_string.
 *     <li>We check the last character returned in the reply_string was a carridge return '\r', and replace it
 *         with a NULL terminator '\0'.
 *      <li>If the expected_reply_string is NOT NULL, we check the reply_string matches the expected_reply_string.
 *     </ul>
 * <li>If compiled in the mutex is released.
 * </ul>
 * @param command_string A NULL terminated string containing the command to send over the open connection.
 * @param expected_reply_string A NULL terminated string containing the expected reply to the command. This can
 *        be set to NULL, if no reply is expected, or no checking is required.
 * @param reply_string A character array of length reply_string_length. On a successful command invocation, on exit
 *        this string will contain any reply received. This can be passed in as NULL, if no reply is required to be
 *        read.
 * @param reply_string_length The length of the reply_string buffer, in bytes.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #COMMAND_STRING_LENGTH
 * @see #Connection_Data
 * @see #Connection_Error_Number
 * @see #Connection_Error_String
 * @see usb_pio_general.html#USB_PIO_General_Log_Format
 * @see usb_pio_general.html#USB_PIO_General_Mutex_Lock
 * @see usb_pio_general.html#USB_PIO_General_Mutex_Unlock
 */
int USB_PIO_Connection_Command(char *command_string,char *expected_reply_string,
			       char *reply_string,int reply_string_length)
{
	char internal_command_string[COMMAND_STRING_LENGTH];
	int retval,write_errno,read_errno,reply_length;
	
	if(command_string == NULL)
	{
		Connection_Error_Number = 11;
		sprintf(Connection_Error_String,"USB_PIO_Connection_Command: command_string was NULL.");
		return FALSE;
	}
	/* check command string is not too long for internal buffer - allow for '\r' terminator to be added */
	if((strlen(command_string)+1) >= COMMAND_STRING_LENGTH)
	{
		Connection_Error_Number = 12;
		sprintf(Connection_Error_String,
			"USB_PIO_Connection_Command: command_string '%s' was too long (%ld vs %d).",command_string,
			(strlen(command_string)+1),COMMAND_STRING_LENGTH);
		return FALSE;
	}
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_TERSE,"USB_PIO_Connection_Command(%s): Started.",command_string);
#endif /* LOGGING */
	/* initialise error number */
	Connection_Error_Number = 0;
#ifdef MUTEXED
	if(!USB_PIO_General_Mutex_Lock())
	{
		Connection_Error_Number = 13;
		sprintf(Connection_Error_String,"USB_PIO_Connection_Command: failed to lock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
	/* prepare internal command string */
	strcpy(internal_command_string,command_string);
	strcat(internal_command_string,"\r");
	/* write to fd */
	retval = write(Connection_Data.Fd,internal_command_string,strlen(internal_command_string));
	if(retval == -1)
	{
		write_errno = errno;
		Connection_Error_Number = 14;
		sprintf(Connection_Error_String,"USB_PIO_Connection_Command: write failed (%s,%d,%s).",
			command_string,write_errno,strerror(write_errno));
		return FALSE;
		
	}
	/* do we need to get a reply ? */
	if(reply_string != NULL)
	{
		/* read reply */
		reply_length = read(Connection_Data.Fd,reply_string,reply_string_length);
		if(reply_length < 0)
		{
			read_errno = errno;
			Connection_Error_Number = 15;
			sprintf(Connection_Error_String,"USB_PIO_Connection_Command: read failed (%s,%d,%s).",
				command_string,read_errno,strerror(read_errno));
			return FALSE;
		}
		/* remove \r from reply string, and null terminate */
		if(reply_string[reply_length-1] != '\r')
		{
			reply_string[reply_length] = '\0'; /* null terminate full reply for error message */
			Connection_Error_Number = 16;
			sprintf(Connection_Error_String,
				"USB_PIO_Connection_Command: read reply not terminated with a carridge return (command=%s,reply=%s).",
				command_string,reply_string);
			return FALSE;
		}
		reply_string[reply_length-1] = '\0';
#if LOGGING > 3
		USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"USB_PIO_Connection_Command(%s): Reply = '%s'.",
					   command_string,reply_string);
#endif /* LOGGING */
		/* do we need to check the reply_string ? */
		if(expected_reply_string != NULL)
		{
			if(strcmp(reply_string,expected_reply_string) != 0)
			{
				Connection_Error_Number = 17;
				sprintf(Connection_Error_String,
					"USB_PIO_Connection_Command: Unexpected reply (command=%s,reply=%s,expected_reply=%s).",
					command_string,reply_string,expected_reply_string);
				return FALSE;
			}
		}
	}/* end if reply_string != NULL */
#ifdef MUTEXED
	if(!USB_PIO_General_Mutex_Unlock())
	{
		Connection_Error_Number = 18;
		sprintf(Connection_Error_String,"USB_PIO_Connection_Command: failed to unlock mutex.");
		return FALSE;
	}
#endif /* MUTEXED */
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_TERSE,"USB_PIO_Connection_Command: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Connection_Error_Number
 */
int USB_PIO_Connection_Get_Error_Number(void)
{
	return Connection_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Connection_Error_Number
 * @see #Connection_Error_String
 * @see usb_pio_general.html#USB_PIO_General_Get_Current_Time_String
 */
void USB_PIO_Connection_Error(void)
{
	char time_string[32];

	USB_PIO_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Connection_Error_Number == 0)
		sprintf(Connection_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s USB_PIO_Connection:Error(%d) : %s\n",time_string,
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
 * @see usb_pio_general.html#USB_PIO_General_Get_Current_Time_String
 */
void USB_PIO_Connection_Error_String(char *error_string)
{
	char time_string[32];

	USB_PIO_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Connection_Error_Number == 0)
		sprintf(Connection_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s USB_PIO_Connection:Error(%d) : %s\n",time_string,
		Connection_Error_Number,Connection_Error_String);
}

/* =======================================
**  internal functions 
** ======================================= */
/**
 * Setup the serial port attributes of the opened connection to the USB-PIO board.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Connection_Data
 * @see #Connection_Error_Number
 * @see #Connection_Error_String
 * @see usb_pio_general.html#USB_PIO_General_Log_Format
 */
static int Connection_Set_Serial_Attributes(void)
{
	struct termios tty;
	int retval,baudrate;
	
#if LOGGING > 5
	USB_PIO_General_Log_Format(LOG_VERBOSITY_VERBOSE,"Connection_Set_Serial_Attributes: Get attributes.");
#endif /* LOGGING */
	retval = tcgetattr(Connection_Data.Fd, &tty);
	if(retval != 0)
	{
		Connection_Error_Number = 7;
		sprintf(Connection_Error_String,"Connection_Set_Serial_Attributes: tcgetattr failed (%d=%s).",
			errno, strerror(errno));
		return FALSE;
	}
	/* baud rate hardcoded to the maximum allowed at the moment */
	baudrate = __MAX_BAUD;
#if LOGGING > 5
	USB_PIO_General_Log_Format(LOG_VERBOSITY_VERBOSE,"Connection_Set_Serial_Attributes: Set baud rate to %d.",
				   baudrate);
#endif /* LOGGING */
	cfsetospeed(&tty, baudrate);
	cfsetispeed(&tty, baudrate);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     /* 8-bit chars*/
	/* Disable IGNBRK for mismatched baud tests; otherwise receive break as \000 chars */
	tty.c_iflag &= ~IGNBRK; /* disable break processing */

	tty.c_lflag     = 0;    /* no signaling chars, no echo, */
	tty.c_oflag     = 0;    /* no remapping, no delays */
	tty.c_cc[VMIN]  = 0;    /* read doesn't block */
	tty.c_cc[VTIME] = 5;    /* 0.5 seconds read timeout */

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); /* shut off xon/xoff ctrl */
	tty.c_cflag |=  (CLOCAL | CREAD);       /* ignore modem controls, */
	tty.c_cflag &= ~(PARENB | PARODD);      /* shut off parity */
	/* tty.c_cflag |=  parity; parity set to 0 */
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;
#if LOGGING > 5
	USB_PIO_General_Log_Format(LOG_VERBOSITY_VERBOSE,"Connection_Set_Serial_Attributes: Set attributes.");
#endif /* LOGGING */
	retval = tcsetattr(Connection_Data.Fd, TCSANOW, &tty);
	if(retval != 0)
	{
		Connection_Error_Number = 8;
		sprintf(Connection_Error_String,"Connection_Set_Serial_Attributes: tcsetattr failed (%d=%s).",
			errno, strerror(errno));
		return FALSE;
	}
#if LOGGING > 5
	USB_PIO_General_Log_Format(LOG_VERBOSITY_VERBOSE,"Connection_Set_Serial_Attributes: Finished.");
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set the opened connection to the USB-PIO board to blocking or non-blocking.
 * @param blocking A boolean, if true set the connection blocking, otherwise set it non-blocking.
 * @return The routine returns TRUE on success and FALSE if an error occurs.
 * @see #Connection_Data
 * @see #Connection_Error_Number
 * @see #Connection_Error_String
 * @see usb_pio_general.html#USB_PIO_General_Log_Format
 */
static int Connection_Set_Blocking(int blocking)
{
	struct termios tty;
	int retval;
	
	memset(&tty, 0, sizeof tty);
#if LOGGING > 5
	USB_PIO_General_Log_Format(LOG_VERBOSITY_VERBOSE,"Connection_Set_Blocking(%d): Get attributes.",blocking);
#endif /* LOGGING */
	retval = tcgetattr(Connection_Data.Fd, &tty);
	if(retval != 0)
	{
		Connection_Error_Number = 9;
		sprintf(Connection_Error_String,"Connection_Set_Blocking: tcgetattr failed (%d=%s).",
			errno, strerror(errno));
		return FALSE;
	}
	if(blocking)
		tty.c_cc[VMIN]  = 1;
	else
		tty.c_cc[VMIN]  = 0;
	tty.c_cc[VTIME] = 5; /* 0.5 seconds read timeout*/
#if LOGGING > 5
	USB_PIO_General_Log_Format(LOG_VERBOSITY_VERBOSE,"Connection_Set_Blocking: Set attributes.");
#endif /* LOGGING */
	retval = tcsetattr(Connection_Data.Fd, TCSANOW, &tty);
	if(retval != 0)
	{
		Connection_Error_Number = 10;
		sprintf(Connection_Error_String,"Connection_Set_Blocking: tcsetattr failed (%d=%s).",
			errno, strerror(errno));
		return FALSE;
	}
#if LOGGING > 5
	USB_PIO_General_Log_Format(LOG_VERBOSITY_VERBOSE,"Connection_Set_Blocking: Finished.");
#endif /* LOGGING */
	return TRUE;
}
