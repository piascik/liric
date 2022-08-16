/* usb_pio_test_set_outputs.c */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "log_udp.h"
#include "usb_pio_command.h"
#include "usb_pio_connection.h"
#include "usb_pio_general.h"
/**
 * Test program to set the digital outputs using the USB-PIO BMCM OR8 I/O card.
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
 * A unsigned character, each bit representing an outpt. If the bit is set, the output is turned on.
 * This variable is set in Parse_Arguments and then used to control the digital outputs.
 */
static unsigned char Outputs;
/**
 * A boolean, initialised to FALSE, and set TRUE when a new set of outputs are set in Parse_Arguments.
 */
static int Outputs_Set = FALSE;
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
 * @see #Outputs
 * @see #Outputs_Set
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
 * @see ../cdocs/usb_pio_command.html#USB_PIO_Command_Outputs_Set
 */
int main(int argc, char *argv[])
{
       	/* parse arguments */
	fprintf(stdout,"usb_pio_test_set_outputs : Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	if(Outputs_Set == FALSE)
	{
		fprintf(stderr, "usb_pio_test_set_outputs : Please set which outputs to turn on.\n");
		return 2;
	}
	USB_PIO_General_Set_Log_Filter_Level(Log_Level);
	USB_PIO_General_Set_Log_Filter_Function(USB_PIO_General_Log_Filter_Level_Absolute);
	USB_PIO_General_Set_Log_Handler_Function(USB_PIO_General_Log_Handler_Stdout);
         /* open device */
	USB_PIO_General_Log(LOG_VERBOSITY_TERSE,"usb_pio_test_set_outputs : Connecting to controller.");
	if(!USB_PIO_Connection_Open(Device_Name))
	{
		USB_PIO_General_Error();
		return 3;
	}
	if(!USB_PIO_Command_Outputs_Set(Outputs))
	{
		USB_PIO_General_Error();
		return 4;
	}
	fprintf(stdout,"usb_pio_test_set_outputs:Closing connection.\n");
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
 * @see #Outputs
 * @see #Outputs_Set
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,retval,output_i;

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
		else if((strcmp(argv[i],"-o")==0)||(strcmp(argv[i],"-outputs")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&output_i);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Failed to parse outputs %s.\n",argv[i+1]);
					return FALSE;
				}
				if((output_i < 0)||(output_i > 255))
				{
					fprintf(stderr,"Parse_Arguments: outputs %s out of range 0..255.\n",argv[i+1]);
					return FALSE;
					
				}
				Outputs = (unsigned char)output_i;
				Outputs_Set = TRUE;
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:-outputs requires a number 0..255.\n");
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
	fprintf(stdout,"Test USB PIO Set Outputs:Help.\n");
	fprintf(stdout,"This program sets the digital outputs on the USB-PIO BMCM OR8 I/O board.\n");
	fprintf(stdout,"usb_pio_test_get_outputs -d[evice_name] <USB device> [-help]\n");
	fprintf(stdout,"\t[-l[og_level <0..5>][-o[utputs <0..255>].\n");
}
