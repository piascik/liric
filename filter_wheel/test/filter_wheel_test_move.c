/* filter_wheel_test_move.c
*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "log_udp.h"
#include "filter_wheel_command.h"
#include "filter_wheel_general.h"

/**
 * Test program to try and move the Starlight Express filter wheel to the specified position.
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
 * The position to move the filter wheel to.
 */
static int Position;

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
 * @see #Log_Level
 * @see #Position
 * @see ../cdocs/filter_wheel_general.html#Filter_Wheel_General_Set_Log_Filter_Level
 * @see ../cdocs/filter_wheel_general.html#Filter_Wheel_General_Set_Log_Filter_Function
 * @see ../cdocs/filter_wheel_general.html#Filter_Wheel_General_Log_Filter_Level_Absolute
 * @see ../cdocs/filter_wheel_general.html#Filter_Wheel_General_Set_Log_Handler_Function
 * @see ../cdocs/filter_wheel_general.html#Filter_Wheel_General_Log_Handler_Stdout
 * @see ../cdocs/filter_wheel_general.html#Filter_Wheel_General_Log
 * @see ../cdocs/filter_wheel_general.html#Filter_Wheel_General_Error
 * @see ../cdocs/filter_wheel_command.html#Filter_Wheel_Command_Open
 * @see ../cdocs/filter_wheel_command.html#Filter_Wheel_Command_Close
 * @see ../cdocs/filter_wheel_command.html#Filter_Wheel_Command_Move
 */
int main(int argc, char *argv[])
{

	/* parse arguments */
	fprintf(stdout,"test_filter_wheel_move : Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	Filter_Wheel_General_Set_Log_Filter_Level(Log_Level);
	Filter_Wheel_General_Set_Log_Filter_Function(Filter_Wheel_General_Log_Filter_Level_Absolute);
	Filter_Wheel_General_Set_Log_Handler_Function(Filter_Wheel_General_Log_Handler_Stdout);
        /* open device */
	Filter_Wheel_General_Log(LOG_VERBOSITY_TERSE,"test_filter_wheel_move : Connecting to controller.");
	if(!Filter_Wheel_Command_Open(Device_Name))
	{
		Filter_Wheel_General_Error();
		return 2;
	}
	fprintf(stdout,"test_filter_wheel_move:Moving filter wheel to position %d.\n",Position);
	if(!Filter_Wheel_Command_Move(Position))
	{
		Filter_Wheel_General_Error();
		return 3;

	}
	fprintf(stdout,"test_filter_wheel_move:Closing connection.\n");
	Filter_Wheel_Command_Close();
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
 * @see #Position
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
		else if((strcmp(argv[i],"-p")==0)||(strcmp(argv[i],"-position")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Position);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Failed to parse position %s.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:-position requires a filter wheel position (1..5).\n");
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
	fprintf(stdout,"Test Filter Wheel Move:Help.\n");
	fprintf(stdout,"This program tries to move the Starlight Express filter wheel to the specified position.\n");
	fprintf(stdout,"filter_wheel_test_move -d[evice_name] <USB device> -p[osition] <1..5> [-help]\n");
	fprintf(stdout,"\t[-l[og_level <0..5>].\n");
}
