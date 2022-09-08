/* detector_test_get_system_status.c */
/**
 * Test program to use the XCLIB serial interface to the Raptor Ninox-640 to query it's system status.
 * @author Chris Mottram
 * @version $Id$
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "log_udp.h"

#include "detector_setup.h"
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
 * @see ../cdocs/detector_general.html#Detector_General_Set_Log_Filter_Level
 * @see ../cdocs/detector_general.html#Detector_General_Set_Log_Filter_Function
 * @see ../cdocs/detector_general.html#Detector_General_Log_Filter_Level_Absolute
 * @see ../cdocs/detector_general.html#Detector_General_Set_Log_Handler_Function
 * @see ../cdocs/detector_general.html#Detector_General_Log_Handler_Stdout
 * @see ../cdocs/detector_general.html#Detector_General_Error
 * @see ../cdocs/detector_serial.html#Detector_Serial_Open
 * @see ../cdocs/detector_serial.html#Detector_Serial_Command_Get_System_Status
 */
int main(int argc, char *argv[])
{
	unsigned char status;
	int checksum_enabled,cmd_ack_enabled,fpga_booted,fpga_in_reset,eprom_comms_enabled;
	
	/* parse arguments */
	fprintf(stdout,"detector_test_get_system_status : Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	Detector_General_Set_Log_Filter_Level(Log_Level);
	Detector_General_Set_Log_Filter_Function(Detector_General_Log_Filter_Level_Absolute);
	Detector_General_Set_Log_Handler_Function(Detector_General_Log_Handler_Stdout);
	/* open a connection
	** Just try opening a connection to the serial API of XCLIB, can we do this without a Detector_Setup_Open?  */
	if(!Detector_Serial_Open())
	{
		Detector_General_Error();
		return 3;
	}
	if(!Detector_Serial_Command_Get_System_Status(&status,&checksum_enabled,&cmd_ack_enabled,&fpga_booted,&fpga_in_reset,&eprom_comms_enabled))
	{
		Detector_General_Error();
		return 4;
	}
	fprintf(stdout,"System status byte: %02x.\n",status);
	fprintf(stdout,"Checksum enabled: %s.\n",checksum_enabled ? "TRUE" : "FALSE");
	fprintf(stdout,"Command ACK enabled: %s.\n",cmd_ack_enabled ? "TRUE" : "FALSE");
	fprintf(stdout,"FPGA booted: %s.\n",fpga_booted ? "TRUE" : "FALSE");
	fprintf(stdout,"FPGA in RESET: %s.\n",fpga_in_reset ? "TRUE" : "FALSE");
	fprintf(stdout,"EPROM comms enabled: %s.\n",eprom_comms_enabled ? "TRUE" : "FALSE");
	/* How do we close the connection to the serial port? */
	/*
	if(!Detector_Serial_Shutdown())
	{
		Detector_General_Error();
		return 3;
	}
	*/
	fprintf(stdout,"detector_test_get_system_status : Finished.\n");		
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
	fprintf(stdout,"Detector Test Get System Status:Help.\n");
	fprintf(stdout,"This program uses the XCLIB serial interface to the Raptor Ninox-640 to query it's system status.\n");
	fprintf(stdout,"detector_test_get_system_status [-help][-l[og_level <0..5>].\n");
}

