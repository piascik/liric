/* detector_test_hex_parsing.c */
/**
 * Test program to test Detector_Serial_Parse_Hex_String and Detector_Serial_Print_Command.
 * @author Chris Mottram
 * @version $Id$
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "log_udp.h"

#include "detector_general.h"
#include "detector_serial.h"

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * Verbosity log level : initialised to LOG_VERBOSITY_VERY_VERBOSE.
 */
static int Log_Level = LOG_VERBOSITY_VERY_VERBOSE;
/**
 * The hex string to parse.
 */
static char *Hex_String = NULL;

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
 * @see #Log_Level
 * @see #Hex_String
 * @see ../cdocs/detector_general.html#Detector_General_Set_Log_Filter_Level
 * @see ../cdocs/detector_general.html#Detector_General_Set_Log_Filter_Function
 * @see ../cdocs/detector_general.html#Detector_General_Log_Filter_Level_Absolute
 * @see ../cdocs/detector_general.html#Detector_General_Set_Log_Handler_Function
 * @see ../cdocs/detector_general.html#Detector_General_Log_Handler_Stdout
 * @see ../cdocs/detector_general.html#Detector_General_Error
 * @see ../cdocs/detector_serial.html#Detector_Serial_Parse_Hex_String
 * @see ../cdocs/detector_serial.html#Detector_Serial_Print_Command
 */
int main(int argc, char *argv[])
{
	char print_buffer[256];
	char command_buffer[256];
	int command_buffer_length;
	
	/* parse arguments */
	fprintf(stdout,"detector_test_hex_parsing : Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	Detector_General_Set_Log_Filter_Level(Log_Level);
	Detector_General_Set_Log_Filter_Function(Detector_General_Log_Filter_Level_Absolute);
	Detector_General_Set_Log_Handler_Function(Detector_General_Log_Handler_Stdout);
	if(Hex_String == NULL)
	{
		fprintf(stdout,"detector_test_hex_parsing : No input hex string specified.\n");
		return 2;
	}
	if(!Detector_Serial_Parse_Hex_String(Hex_String,command_buffer,256,&command_buffer_length))
	{
		Detector_General_Error();
		return 3;
	}
	fprintf(stdout,"detector_test_hex_parsing: Parsed '%s' as '%s'.\n",Hex_String,
		Detector_Serial_Print_Command(command_buffer,command_buffer_length,print_buffer,256));
	if(command_buffer_length < (256-1))
	{
		if(!Detector_Serial_Compute_Checksum(command_buffer,&command_buffer_length))
		{
			Detector_General_Error();
			return 4;
		}
		fprintf(stdout,"detector_test_hex_parsing: Command buffer after adding checksum: '%s'.\n",
			Detector_Serial_Print_Command(command_buffer,command_buffer_length,print_buffer,256));
	}
	return 0;
}
/* ------------------------------------------------------------------
**          Internal functions 
** ------------------------------------------------------------------ */
/**
 * Routine to parse command line arguments.
 * @param argc The number of arguments sent to the program.
 * @param argv An array of argument strings.
 * @see #Log_Level
 * @see #Help
 * @see #Hex_String
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,retval;

	for(i=1;i<argc;i++)
	{
		if((strcmp(argv[i],"-help")==0))
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
		else
		{
			if(Hex_String == NULL)
			{
				Hex_String = argv[i];
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:hex string already specified: argument '%s' not recognized.\n",argv[i]);
				return FALSE;
			}
		}
	}
	return TRUE;
}

/**
 * Help routine.
 */
static void Help(void)
{
	fprintf(stdout,"Detector Test Hex Parsing:Help.\n");
	fprintf(stdout,"This program tests parsing a Hex string into a command/reply series of bytes, and testing printing of the parsed data.\n");
	fprintf(stdout,"detector_test_hex_parsing [-help][-l[og_level <0..5>] <input string>.\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"Where <input string> is of the form '0xNN [0xNN...]\n");
}
