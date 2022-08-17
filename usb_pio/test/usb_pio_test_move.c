/* usb_pio_test_move.c */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "log_udp.h"
#include "usb_pio_command.h"
#include "usb_pio_connection.h"
#include "usb_pio_general.h"
/**
 * Test program using the USB-PIO BMCM OR8 I/O card. This turns on a specified output, 
 * waits for a specified input to go high, and then turns off the output.
 */
/**
 * Length of some of the strings used in this program.
 */
#define STRING_LENGTH        (256)
/**
 * Verbosity log level : initialised to LOG_VERBOSITY_VERY_VERBOSE.
 */
static int Log_Level = LOG_VERBOSITY_VERY_VERBOSE;
/**
 * The USB device to connect to.
 * @see #STRING_LENGTH
 */
static char Device_Name[STRING_LENGTH];
/**
 * Which output to turn on.
 */
static int Output = 0;
/**
 * Which output to read.
 */
static int Input = 0;
/**
 * How long to wait for the input to turn on, before timing out, in decimal seconds.
 */
static double Timeout = 60.0;
/**
 * Time to sleep each time round the loop whilst waiting for the input to be high, in nanoseconds
 */
static int Sleep_Time = 1000000;

/* internal functions */
static int Parse_Arguments(int argc, char *argv[]);
static void Help(void);

/* ------------------------------------------------------------------
**          External functions 
** ------------------------------------------------------------------ */

/**
 * Main program.
 * @param argc The number of arguments to the program.
 * @param argv An array of argument strings.
 * @see #Parse_Arguments
 * @see #Device_Name
 * @see #Input
 * @see #Output
 * @see #Sleep_Time
 * @see #Timeout
 * @see #Log_Level
 * @see ../cdocs/usb_pio_general.html#USB_PIO_General_Set_Log_Filter_Level
 * @see ../cdocs/usb_pio_general.html#USB_PIO_General_Set_Log_Filter_Function
 * @see ../cdocs/usb_pio_general.html#USB_PIO_General_Log_Filter_Level_Absolute
 * @see ../cdocs/usb_pio_general.html#USB_PIO_General_Set_Log_Handler_Function
 * @see ../cdocs/usb_pio_general.html#USB_PIO_General_Log_Handler_Stdout
 * @see ../cdocs/usb_pio_general.html#USB_PIO_Log
 * @see ../cdocs/usb_pio_general.html#USB_PIO_General_Error
 * @see ../cdocs/usb_pio_connection.html#USB_PIO_Connection_Open
 * @see ../cdocs/usb_pio_connection.html#USB_PIO_Connection_Close
 * @see ../cdocs/usb_pio_command.html#USB_PIO_Command_Output_Set
 */
int main(int argc, char *argv[])
{
	struct timespec loop_start_time,current_time,sleep_time;
	int done,onoff;
	
       	/* parse arguments */
	fprintf(stdout,"usb_pio_test_move : Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	if((Output < 1)||(Output > 8))
	{
		fprintf(stderr, "usb_pio_test_move : Output not specified / out of range (%d).\n",Output);
		return 2;
	}
	if((Input < 1)||(Input > 8))
	{
		fprintf(stderr, "usb_pio_test_move : Input not specified / out of range (%d).\n",Input);
		return 2;
	}
	USB_PIO_General_Set_Log_Filter_Level(Log_Level);
	USB_PIO_General_Set_Log_Filter_Function(USB_PIO_General_Log_Filter_Level_Absolute);
	USB_PIO_General_Set_Log_Handler_Function(USB_PIO_General_Log_Handler_Stdout);
         /* open device */
	USB_PIO_General_Log(LOG_VERBOSITY_TERSE,"usb_pio_test_move : Connecting to controller.");
	if(!USB_PIO_Connection_Open(Device_Name))
	{
		USB_PIO_General_Error();
		return 3;
	}
	/* turn on output */
	USB_PIO_General_Log_Format(LOG_VERBOSITY_TERSE,"usb_pio_test_move : Turning on output %d.",Output);
	if(!USB_PIO_Command_Output_Set(Output,TRUE))
	{
		USB_PIO_General_Error();
		return 4;
	}
	/* wait for input to be high */
	done = FALSE;
	clock_gettime(CLOCK_REALTIME,&loop_start_time);
	clock_gettime(CLOCK_REALTIME,&current_time);
	while(done == FALSE)
	{
		/* get input */
		if(!USB_PIO_Command_Input_Get(Input,&onoff))
		{
			USB_PIO_General_Error();
			return 5;
		}
		/* is it on yet? */
		if(onoff)
		{
			USB_PIO_General_Log(LOG_VERBOSITY_TERSE,"usb_pio_test_move : Input %d is high.");
			done = TRUE;
		}
		else
		{
			/* check for timeout */
			clock_gettime(CLOCK_REALTIME,&current_time);
			if(fdifftime(current_time,loop_start_time) > Timeout)
			{
				USB_PIO_General_Log_Format(LOG_VERBOSITY_TERSE,
			        "usb_pio_test_move : Timed out after %.2f seconds waiting for Input %d to be high.",
						    fdifftime(current_time,loop_start_time),Input);
				
				done = TRUE;
			}
		}
		/* wait a bit (Sleep_Time ns) before retrying */
		sleep_time.tv_sec = 0;
		sleep_time.tv_nsec = Sleep_Time;
		nanosleep(&sleep_time,&sleep_time);
	}
	/* turn off output */
	USB_PIO_General_Log_Format(LOG_VERBOSITY_TERSE,"usb_pio_test_move : Turning off output %d.",Output);
	if(!USB_PIO_Command_Output_Set(Output,FALSE))
	{
		USB_PIO_General_Error();
		return 4;
	}
	fprintf(stdout,"usb_pio_test_move:Closing connection.\n");
	USB_PIO_Connection_Close();
	return 0;
}

/* ------------------------------------------------------------------
**          Internal functions 
** ------------------------------------------------------------------ */
/**
 * Routine to parse command line arguments.
 * @param argc The number of arguments sent to the program.
 * @param argv An array of argument strings.
 * @see #Device_Name
 * @see #Log_Level
 * @see #Output
 * @see #Input
 * @see #Timeout
 * @see #Sleep_Time
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,retval;

	for(i=1;i<argc;i++)
	{
		if((strcmp(argv[i],"-d")==0)||(strcmp(argv[i],"-device_name")==0))
		{
			if((i+1)<argc)
			{
				strncpy(Device_Name,argv[i+1],STRING_LENGTH-1);
				Device_Name[STRING_LENGTH-1] = '\0';
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:device_name requires a USB device name.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-help")==0))
		{
			Help();
			return FALSE;
		}
		else if((strcmp(argv[i],"-i")==0)||(strcmp(argv[i],"-input")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Input);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Failed to parse input %s.\n",argv[i+1]);
					return FALSE;
				}
				if((Input < 1)||(Input > 8))
				{
					fprintf(stderr,"Parse_Arguments: input %s out of range 1..8.\n",argv[i+1]);
					return FALSE;
					
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:-input requires a number 1..8.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-l")==0)||(strcmp(argv[i],"-log_level")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Log_Level);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Failed to parse log level %s.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:-log_level requires a number 0..5.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-o")==0)||(strcmp(argv[i],"-output")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Output);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Failed to parse output %s.\n",argv[i+1]);
					return FALSE;
				}
				if((Output < 1)||(Output > 8))
				{
					fprintf(stderr,"Parse_Arguments: output %s out of range 1..8.\n",argv[i+1]);
					return FALSE;
					
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:-output requires a number 1..8.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-s")==0)||(strcmp(argv[i],"-sleep_time")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Sleep_Time);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Failed to parse sleep time %s.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:-sleep_time requires a time length in nanoseconds.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-t")==0)||(strcmp(argv[i],"-timeout")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%lf",&Timeout);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Failed to parse timeout %s.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:-timeout requires a time length in seconds.\n");
				return FALSE;
			}
		}
		else
		{
			fprintf(stderr,"Parse_Arguments:argument '%s' not recognized.\n",argv[i]);
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * Help routine.
 */
static void Help(void)
{
	fprintf(stdout,"USB PIO Test Move:Help.\n");
	fprintf(stdout,"This turns on a specified output, waits for a specified input to go high, and then turns off the output, using the USB-PIO BMCM OR8 I/O board. \n");
	fprintf(stdout,"The program waits for timeout seconds (default 60) before timing out.\n");
	fprintf(stdout,"Each time round the loop the program sleep for sleep_time nanoseconds.\n");
	fprintf(stdout,"usb_pio_test_move -d[evice_name] <USB device> -o[utput <1..8> -i[nput <1..8> [-help]\n");
	fprintf(stdout,"\t[-l[og_level] <0..5>] [-s[leep_time] <nanosecs>] [-t[imeout] <secs>].\n");
}
