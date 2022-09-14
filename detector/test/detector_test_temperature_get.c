/* detector_test_temperature_get.c */
/**
 * Test program to test retrieving the sensor temperature from the Raptor Ninox-640 camera head.
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
#include "detector_temperature.h"

/* hash defines */
/**
 * Length of some of the strings used in this program.
 */
#define STRING_LENGTH                       (256)
/**
 * The default exposure length to use for each individual coadd, in milliseconds.
 * This also determines the '.fmt' to configure the detector with.
 */
#define DEFAULT_COADD_FRAME_EXPOSURE_LENGTH (1000)
/**
 * The default directory containing the '.fmt' format files to use to configure the detector.
 */
#define DEFAULT_FMT_DIRECTORY               ("/icc/bin/raptor/fmt")

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
 * The exposure length to use for each individual coadd, in milliseconds.
 * This also determines the '.fmt' to configure the detector with.
 * @see #DEFAULT_COADD_FRAME_EXPOSURE_LENGTH
 */
static int Coadd_Frame_Exposure_Length_Ms = DEFAULT_COADD_FRAME_EXPOSURE_LENGTH;

/**
 * The directory containing the '.fmt' format files to use to configure the detector.
 * @see #STRING_LENGTH
 * @see #DEFAULT_FMT_DIRECTORY
 */
static char FMT_Directory[STRING_LENGTH] = DEFAULT_FMT_DIRECTORY;

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
 * @see #Log_Level
 * @see #FMT_Directory
 * @see #Coadd_Frame_Exposure_Length_Ms
 * @see ../cdocs/detector_general.html#Detector_General_Set_Log_Filter_Level
 * @see ../cdocs/detector_general.html#Detector_General_Set_Log_Filter_Function
 * @see ../cdocs/detector_general.html#Detector_General_Log_Filter_Level_Absolute
 * @see ../cdocs/detector_general.html#Detector_General_Set_Log_Handler_Function
 * @see ../cdocs/detector_general.html#Detector_General_Log_Handler_Stdout
 * @see ../cdocs/detector_general.html#Detector_General_Error
 * @see ../cdocs/detector_setup.html#Detector_Setup_Open
 * @see ../cdocs/detector_setup.html#Detector_Setup_Close
 * @see ../cdocs/detector_serial.html#Detector_Serial_Initialise
 * @see ../cdocs/detector_temperature.html#Detector_Temperature_Get
 */
int main(int argc, char *argv[])
{
	char format_filename[STRING_LENGTH];
	double temperature;
	
	/* parse arguments */
	fprintf(stdout,"detector_test_temperature_get : Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	Detector_General_Set_Log_Filter_Level(Log_Level);
	Detector_General_Set_Log_Filter_Function(Detector_General_Log_Filter_Level_Absolute);
	Detector_General_Set_Log_Handler_Function(Detector_General_Log_Handler_Stdout);
	/* create format filename and setup connection to the detector/ XCLIB library */
	fprintf(stdout,"detector_test_temperature_get : Initialising Detector library.\n");
	sprintf(format_filename,"%s/rap_%dms.fmt",FMT_Directory,Coadd_Frame_Exposure_Length_Ms);
	if(!Detector_Setup_Open("","",format_filename))
	{
		Detector_General_Error();
		return 3;
	}
	/* initialise the serial connection. The library connection has to be already open to do this */
	if(!Detector_Serial_Initialise())
	{
		Detector_General_Error();
		Detector_Setup_Close();
		return 3;
	}
	/* get the detector temperature. We must have already initialised the serial device, which causes the 
	** manufacturers data to be read (including ADC/DAC calibration values), and the temperature software
	** to be initialised. */
	if(!Detector_Temperature_Get(&temperature))
	{
		Detector_General_Error();
		Detector_Setup_Close();
		return 3;
	}		
	fprintf(stdout,"detector_test_temperature_get : Sensor temperature is %.3f degrees Centigrade.\n",temperature);
	/* close the connection to the serial port/ XCLIB library */
	if(!Detector_Setup_Close())
	{
		Detector_General_Error();
		return 3;
	}
	fprintf(stdout,"detector_test_temperature_get : Finished.\n");		
	return 0;
}

/* ------------------------------------------------------------------
**          Internal functions 
** ------------------------------------------------------------------ */
/**
 * Routine to parse command line arguments.
 * @param argc The number of arguments sent to the program.
 * @param argv An array of argument strings.
 * @see #STRING_LENGTH
 * @see #Coadd_Frame_Exposure_Length_Ms
 * @see #FMT_Directory
 * @see #Log_Level
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,retval;

	for(i=1;i<argc;i++)
	{
		if((strcmp(argv[i],"-coadd")==0)||(strcmp(argv[i],"-coadd_exposure_length")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Coadd_Frame_Exposure_Length_Ms);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Failed to parse coadd exposure length %s.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:-coadd_exposure_length requires an exposure length in milliseconds (for which a valid .fmt file exists).\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-fmt")==0)||(strcmp(argv[i],"-fmt_directory")==0))
		{
			if((i+1)<argc)
			{
				strncpy(FMT_Directory,argv[i+1],STRING_LENGTH-1);
				FMT_Directory[STRING_LENGTH-1] = '\0';
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:fmt_directory requires a directory path name.\n");
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
	fprintf(stdout,"Detector Test Getting the sensor temperature:Help.\n");
	fprintf(stdout,"This program tests retrieving the sensor temperature from the Raptor Ninox-640 camera head.\n");
	fprintf(stdout,"detector_test_temperature_get [-coadd[_exposure_length] <ms>][-fmt[_directory] <dir>]\n");
	fprintf(stdout,"\t[-help][-l[og_level <0..5>].\n");
	fprintf(stdout,"The exposure length of an individual coadd is specified in milliseconds (-coadd_exposure_length),\n");
	fprintf(stdout,"this defaults to %d, a valid '.fmt' file for that exposure length must exist \n",
		DEFAULT_COADD_FRAME_EXPOSURE_LENGTH);
	fprintf(stdout,"The -coadd_exposure_length / -fmt_directory arguments are needed to construct a valid '.fmt. filename, which is needed to open a connection to the XCLIB library.\n");
}

