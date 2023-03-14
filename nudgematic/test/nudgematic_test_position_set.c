/* nudgematic_test_position_set.c */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "log_udp.h"
#include "nudgematic_general.h"
#include "nudgematic_connection.h"
#include "nudgematic_command.h"

/**
 * Test program to move the nudgematic mechanism to a specified position.
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
 * The USB-PIO BMCM OR8 I/O card USB device to connect to.
 * @see #STRING_LENGTH
 */
static char Device_Name[STRING_LENGTH];
/**
 * The position to move the nudgematic to.
 */
static int Position = -1;
/**
 * Whether the nudgematic positions are small offsets or large offsets.
 * @see ../cdocs/nudgematic_command.html#NUDGEMATIC_OFFSET_SIZE_T
 */
static NUDGEMATIC_OFFSET_SIZE_T Offset_Size = SMALL;


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
 * @see #Position
 * @see #Offset_Size
 * @see #Log_Level
 * @see ../cdocs/nudgematic_general.html#Nudgematic_General_Set_Log_Filter_Level
 * @see ../cdocs/nudgematic_general.html#Nudgematic_General_Set_Log_Filter_Function
 * @see ../cdocs/nudgematic_general.html#Nudgematic_General_Log_Filter_Level_Absolute
 * @see ../cdocs/nudgematic_general.html#Nudgematic_General_Set_Log_Handler_Function
 * @see ../cdocs/nudgematic_general.html#Nudgematic_General_Log_Handler_Stdout
 * @see ../cdocs/nudgematic_connection.html#Nudgematic_Connection_Open
 * @see ../cdocs/nudgematic_connection.html#Nudgematic_Connection_Close
 * @see ../cdocs/nudgematic_command.html#Nudgematic_Command_Offset_Size_Set
 * @see ../cdocs/nudgematic_command.html#Nudgematic_Command_Offset_Size_To_String
 * @see ../cdocs/nudgematic_command.html#Nudgematic_Command_Position_Set
 */
int main(int argc, char *argv[])
{
	
       	/* parse arguments */
	fprintf(stdout,"nudgematic_test_position_set : Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	/* setup logging */
	Nudgematic_General_Set_Log_Filter_Level(Log_Level);
	Nudgematic_General_Set_Log_Filter_Function(Nudgematic_General_Log_Filter_Level_Absolute);
	Nudgematic_General_Set_Log_Handler_Function(Nudgematic_General_Log_Handler_Stdout);
         /* open device */
	Nudgematic_General_Log(LOG_VERBOSITY_TERSE,"nudgematic_test_position_set : Connecting to controller.");
	if(!Nudgematic_Connection_Open(Device_Name))
	{
		Nudgematic_General_Error();
		return 2;
	}
	/* set offset size */
	Nudgematic_General_Log_Format(LOG_VERBOSITY_TERSE,"nudgematic_test_position_set : Setting offset size to %s.",
				      Nudgematic_Command_Offset_Size_To_String(Offset_Size));
	if(!Nudgematic_Command_Offset_Size_Set(Offset_Size))
	{
		Nudgematic_General_Error();
		return 3;
	}
	/* move to position */
	Nudgematic_General_Log_Format(LOG_VERBOSITY_TERSE,"nudgematic_test_position_set : Moving to position %d.",
				      Position);
	if(!Nudgematic_Command_Position_Set(Position))
	{
		Nudgematic_General_Error();
		return 4;
	}
	/* close connection */
	Nudgematic_General_Log(LOG_VERBOSITY_TERSE,"nudgematic_test_position_set:Closing connection.");
	if(!Nudgematic_Connection_Close())
	{
		Nudgematic_General_Error();
		return 5;
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
 * @see #Device_Name
 * @see #Log_Level
 * @see #Position
 * @see #Offset_Size
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
		else if((strcmp(argv[i],"-o")==0)||(strcmp(argv[i],"-offset_size")==0))
		{
			if((i+1)<argc)
			{
				if(!Nudgematic_Command_Offset_Size_Parse(argv[i+1],&Offset_Size))
				{
					fprintf(stderr,"Parse_Arguments:Failed to parse offset_size %s.\n",argv[i+1]);
					Nudgematic_General_Error();
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:-offset_size requires a size: 'small', 'large' or 'none'.\n");
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
				fprintf(stderr,"Parse_Arguments:-position requires a number 0..8.\n");
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
	fprintf(stdout,"Nudgematic Test Move to a position:Help.\n");
	fprintf(stdout,"nudgematic_test_position_set -d[evice_name] <USB device> -o[ffset_size <small|large|none> -p[osition] <0..8> [-help]\n");
	fprintf(stdout,"\t[-l[og_level] <0..5>].\n");
}
