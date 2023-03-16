/* nudgematic_send_command.c */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "log_udp.h"
#include "nudgematic_general.h"
#include "nudgematic_connection.h"

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
 * The Arduino Mega USB device to connect to.
 * @see #STRING_LENGTH
 */
static char Device_Name[STRING_LENGTH];
/**
 * The command string to send to the device.
 * @see #STRING_LENGTH
 */
static char Command[STRING_LENGTH];

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
 * @see #STRING_LENGTH
 * @see #Parse_Arguments
 * @see #Device_Name
 * @see #Command
 * @see #Log_Level
 * @see ../cdocs/nudgematic_general.html#Nudgematic_General_Set_Log_Filter_Level
 * @see ../cdocs/nudgematic_general.html#Nudgematic_General_Set_Log_Filter_Function
 * @see ../cdocs/nudgematic_general.html#Nudgematic_General_Log_Filter_Level_Absolute
 * @see ../cdocs/nudgematic_general.html#Nudgematic_General_Set_Log_Handler_Function
 * @see ../cdocs/nudgematic_general.html#Nudgematic_General_Log_Handler_Stdout
 * @see ../cdocs/nudgematic_connection.html#Nudgematic_Connection_Open
 * @see ../cdocs/nudgematic_connection.html#Nudgematic_Connection_Close
 * @see ../cdocs/nudgematic_connection.html#Nudgematic_Connection_Write
 * @see ../cdocs/nudgematic_connection.html#Nudgematic_Connection_Read_Line
 */
int main(int argc, char *argv[])
{
	char reply_string[STRING_LENGTH];
	int retval,bytes_read;
	
       	/* parse arguments */
	fprintf(stdout,"nudgematic_send_command : Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	/* setup logging */
	Nudgematic_General_Set_Log_Filter_Level(Log_Level);
	Nudgematic_General_Set_Log_Filter_Function(Nudgematic_General_Log_Filter_Level_Absolute);
	Nudgematic_General_Set_Log_Handler_Function(Nudgematic_General_Log_Handler_Stdout);
         /* open device */
	Nudgematic_General_Log(LOG_VERBOSITY_TERSE,"nudgematic_send_command : Connecting to controller.");
	if(!Nudgematic_Connection_Open(Device_Name))
	{
		Nudgematic_General_Error();
		return 2;
	}
	if(strlen(Command) < 1)
	{
		fprintf(stderr, "nudgematic_send_command : No command specified.\n");
		return 3;
	}
	/* send command to the arduino */
	Nudgematic_General_Log_Format(LOG_VERBOSITY_TERSE,"nudgematic_send_command : Sending command '%s'.",
				      Command);
	if(!Nudgematic_Connection_Write(Command,strlen(Command)))
	{
		Nudgematic_General_Error();
		Nudgematic_Connection_Close();
		return 4;
	}
	/* read a reply */
	if(!Nudgematic_Connection_Read_Line(reply_string,STRING_LENGTH,&bytes_read))
	{
		Nudgematic_General_Error();
		Nudgematic_Connection_Close();
		return 5;
	}
	Nudgematic_General_Log_Format(LOG_VERBOSITY_TERSE,"nudgematic_send_command : Reply '%s'.",
				      reply_string);
	/* close connection */
	Nudgematic_General_Log(LOG_VERBOSITY_TERSE,"nudgematic_send_command:Closing connection.");
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
 * @see #Command
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,retval;

	for(i=1;i<argc;i++)
	{
		if((strcmp(argv[i],"-c")==0)||(strcmp(argv[i],"-command")==0))
		{
			if((i+1)<argc)
			{
				strncpy(Command,argv[i+1],STRING_LENGTH-1);
				Command[STRING_LENGTH-1] = '\0';
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:command requires a string.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-d")==0)||(strcmp(argv[i],"-device_name")==0))
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
	fprintf(stdout,"Nudgematic Send Command:Help.\n");
	fprintf(stdout,"nudgematic_send_command -d[evice_name] <USB device> -c[ommand] <string> [-help]\n");
	fprintf(stdout,"\t[-l[og_level] <0..5>].\n");
}
