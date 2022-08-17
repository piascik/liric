/* usb_pio_test_get_output.c */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "log_udp.h"
#include "usb_pio_command.h"
#include "usb_pio_connection.h"
#include "usb_pio_general.h"
/**
 * Test program to get the current state of the specified output from the USB-PIO BMCM OR8 I/O card.
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
 * An integer specifying which output to return the value of, between 1..8. The output_bit sent to the 
 * BMCM USB-PIO controller is one less than this.
 */
static int Output = 0;
static int Parse_Arguments(int argc, char *argv[]);
static void Help(void);

/* ------------------------------------------------------------------
**          External functions 
** ------------------------------------------------------------------ */

/**
 * Main program.
 * @param argc The number of arguments to the program.
 * @param argv An array of argument strings.
 * @return The program returns 0 if the output is off, 1 if the output is on, and greater than 1 if an error occurs.
 * @see #Parse_Arguments
 * @see #Device_Name
 * @see #Log_Level
 * @see #Output
 * @see ../cdocs/usb_pio_general.html#USB_PIO_General_Set_Log_Filter_Level
 * @see ../cdocs/usb_pio_general.html#USB_PIO_General_Set_Log_Filter_Function
 * @see ../cdocs/usb_pio_general.html#USB_PIO_General_Log_Filter_Level_Absolute
 * @see ../cdocs/usb_pio_general.html#USB_PIO_General_Set_Log_Handler_Function
 * @see ../cdocs/usb_pio_general.html#USB_PIO_General_Log_Handler_Stdout
 * @see ../cdocs/usb_pio_general.html#USB_PIO_Log
 * @see ../cdocs/usb_pio_general.html#USB_PIO_General_Error
 * @see ../cdocs/usb_pio_connection.html#USB_PIO_Connection_Open
 * @see ../cdocs/usb_pio_connection.html#USB_PIO_Connection_Close
 * @see ../cdocs/usb_pio_command.html#USB_PIO_Command_Output_Get
 */
int main(int argc, char *argv[])
{
	int on_off;
	
	/* parse arguments */
	fprintf(stdout,"usb_pio_test_get_output : Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 2;
	if((Output < 1)||(Output > 8))
	{
		fprintf(stderr,"usb_pio_test_get_output : No output specified or output out of range (%d).\n",Output);
		return 3;
	}
	USB_PIO_General_Set_Log_Filter_Level(Log_Level);
	USB_PIO_General_Set_Log_Filter_Function(USB_PIO_General_Log_Filter_Level_Absolute);
	USB_PIO_General_Set_Log_Handler_Function(USB_PIO_General_Log_Handler_Stdout);
         /* open device */
	USB_PIO_General_Log(LOG_VERBOSITY_TERSE,"usb_pio_test_get_output : Connecting to controller.");
	if(!USB_PIO_Connection_Open(Device_Name))
	{
		USB_PIO_General_Error();
		return 4;
	}
	/* query output */
	if(!USB_PIO_Command_Output_Get(Output,&on_off))
	{
		USB_PIO_General_Error();
		return 5;
	}
	/* print result */
	if(on_off)
		fprintf(stdout,"usb_pio_test_get_output:Output %d was on.\n",Output);
	else
		fprintf(stdout,"usb_pio_test_get_output:Output %d was off.\n",Output);
	fprintf(stdout,"usb_pio_test_get_output:Closing connection.\n");
	USB_PIO_Connection_Close();
	return on_off;
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
				i++;
				if((Output < 1)||(Output > 8))
				{
					fprintf(stderr,"Parse_Arguments:Output %d out of range 1..8.\n",Output);
					return FALSE;
				}
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:output requires an output number 1..8.\n");
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
	fprintf(stdout,"Test Getting the current state of a USB PIO Output:Help.\n");
	fprintf(stdout,"This program gets the current output state of the specified output from the USB-PIO BMCM OR8 I/O board.\n");
	fprintf(stdout,"usb_pio_test_get_output -d[evice_name] <USB device> -o[utput] <1..8> [-help]\n");
	fprintf(stdout,"\t[-l[og_level] <0..5>].\n");
}
