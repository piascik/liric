/* detector_test_exposure.c */
/**
 * Test program to take an exposure with the Raptor Ninox-640 and save the resulatant image in a FITS file.
 * @author Chris Mottram
 * @version $Id$
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "log_udp.h"

#include "detector_buffer.h"
#include "detector_exposure.h"
#include "detector_fits_filename.h"
#include "detector_fits_header.h"
#include "detector_setup.h"
#include "detector_general.h"

/* data types */

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
/**
 * The default directory containing the '.fmt' format files to use to configure the detector.
 */
#define DEFAULT_FITS_DIRECTORY               ("/icc/tmp/")

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
 * The exposure length to use for the exposure, in milliseconds.
 */
static int Exposure_Length_Ms;
/**
 * The exposure length to use for each individual coadd, in milliseconds.
 * This also determines the '.fmt' to configure the detector with.
 * @see #DEFAULT_COADD_FRAME_EXPOSURE_LENGTH
 */
static int Coadd_Frame_Exposure_Length_Ms = DEFAULT_COADD_FRAME_EXPOSURE_LENGTH;
/**
 * The filename to save the read out data into.
 * @see #STRING_LENGTH
 */
static char FITS_Filename[STRING_LENGTH] = "";
/**
 * The directory containing the '.fmt' format files to use to configure the detector.
 * @see #STRING_LENGTH
 * @see #DEFAULT_FMT_DIRECTORY
 */
static char FMT_Directory[STRING_LENGTH] = DEFAULT_FMT_DIRECTORY;
/**
 * A directory to save the read out FITS data into. This is only used to construct a FITS filename 
 * if FITS_Filename is not explicitly set.
 * @see #STRING_LENGTH
 * @see #DEFAULT_FITS_DIRECTORY
 */
static char FITS_Directory[STRING_LENGTH] = DEFAULT_FITS_DIRECTORY;

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
 * @see #Exposure_Length_Ms
 * @see #FITS_Directory
 * @see #FITS_Filename
 * @see ../cdocs/detector_exposure.html#Detector_Exposure_Set_Coadd_Frame_Exposure_Length
 * @see ../cdocs/detector_exposure.html#Detector_Exposure_Expose
 * @see ../cdocs/detector_fits_filename.html#Detector_Fits_Filename_Initialise
 * @see ../cdocs/detector_fits_filename.html#DETECTOR_FITS_FILENAME_DEFAULT_INSTRUMENT_CODE
 * @see ../cdocs/detector_fits_filename.html#Detector_Fits_Filename_Next_Multrun
 * @see ../cdocs/detector_fits_filename.html#Detector_Fits_Filename_Next_Run
 * @see ../cdocs/detector_fits_filename.html#Detector_Fits_Filename_Get_Filename
 * @see ../cdocs/detector_fits_filename.html#DETECTOR_FITS_FILENAME_EXPOSURE_TYPE
 * @see ../cdocs/detector_fits_filename.html#DETECTOR_FITS_FILENAME_PIPELINE_FLAG
 * @see ../cdocs/detector_fits_header.html#Detector_Fits_Header_Initialise
 * @see ../cdocs/detector_general.html#Detector_General_Set_Log_Filter_Level
 * @see ../cdocs/detector_general.html#Detector_General_Set_Log_Filter_Function
 * @see ../cdocs/detector_general.html#Detector_General_Log_Filter_Level_Absolute
 * @see ../cdocs/detector_general.html#Detector_General_Set_Log_Handler_Function
 * @see ../cdocs/detector_general.html#Detector_General_Log_Handler_Stdout
 * @see ../cdocs/detector_general.html#Detector_General_Error
 * @see ../cdocs/detector_setup.html#Detector_Setup_Startup
 * @see ../cdocs/detector_setup.html#Detector_Setup_Shutdown
 */
int main(int argc, char *argv[])
{
	char format_filename[STRING_LENGTH];
	
	/* parse arguments */
	fprintf(stdout,"detector_test_exposure : Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	Detector_General_Set_Log_Filter_Level(Log_Level);
	Detector_General_Set_Log_Filter_Function(Detector_General_Log_Filter_Level_Absolute);
	Detector_General_Set_Log_Handler_Function(Detector_General_Log_Handler_Stdout);
	/* create format filename and setup detector */
	fprintf(stdout,"detector_test_exposure : Initialising Detector.\n");
	sprintf(format_filename,"%s/rap_%dms.fmt",FMT_Directory,Coadd_Frame_Exposure_Length_Ms);
	if(!Detector_Setup_Startup(format_filename))
	{
		Detector_General_Error();
		return 2;
	}
	/* create a FITS filename, if one does not already exist */
	if(strcmp(FITS_Filename,"") == 0)
	{
		if(!Detector_Fits_Filename_Initialise(DETECTOR_FITS_FILENAME_DEFAULT_INSTRUMENT_CODE,FITS_Directory))
		{
			Detector_General_Error();
			return 4;
		}
		if(!Detector_Fits_Filename_Next_Multrun())
		{
			Detector_General_Error();
			return 5;
		}
		if(!Detector_Fits_Filename_Next_Run())
		{
			Detector_General_Error();
			return 6;
		}
		if(!Detector_Fits_Filename_Get_Filename(DETECTOR_FITS_FILENAME_EXPOSURE_TYPE_EXPOSURE,
							DETECTOR_FITS_FILENAME_PIPELINE_FLAG_UNREDUCED,
							FITS_Filename,STRING_LENGTH))
		{
			Detector_General_Error();
			return 7;
		}
	}
	/* initialise fits headers */
	if(!Detector_Fits_Header_Initialise())
	{
		Detector_General_Error();
		return 8;
	}
	/* setup coadd exposure length */
	if(!Detector_Exposure_Set_Coadd_Frame_Exposure_Length(Coadd_Frame_Exposure_Length_Ms))
	{
		Detector_General_Error();
		return 9;
	}
	/* do exposure */
	if(!Detector_Exposure_Expose(Exposure_Length_Ms,FITS_Filename))
	{
		Detector_General_Error();
		return 10;
	}		
	/* shutdown connection to the detector */
	if(!Detector_Setup_Shutdown())
	{
		Detector_General_Error();
		return 3;
	}
	fprintf(stdout,"detector_test_exposure : Finished exposure of length %d ms, saved in '%s'.\n",
		Exposure_Length_Ms,FITS_Filename);		
	return 0;
}

/* ------------------------------------------------------------------
**          Internal functions 
** ------------------------------------------------------------------ */
/**
 * Routine to parse command line arguments.
 * @param argc The number of arguments sent to the program.
 * @param argv An array of argument strings.
 * @see #Coadd_Frame_Exposure_Length_Ms
 * @see #Exposure_Length_Ms
 * @see #FITS_Directory
 * @see #FITS_Filename
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
		else if((strcmp(argv[i],"-e")==0)||(strcmp(argv[i],"-exposure_length")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Exposure_Length_Ms);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Failed to parse exposure length %s.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:-exposure_length requires an exposure length in milliseconds.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-fits_dir")==0)||(strcmp(argv[i],"-fits_directory")==0))
		{
			if((i+1)<argc)
			{
				strncpy(FITS_Directory,argv[i+1],STRING_LENGTH-1);
				FITS_Directory[STRING_LENGTH-1] = '\0';
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:fits_directory requires a directory path name.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-fits_file")==0)||(strcmp(argv[i],"-fits_filename")==0))
		{
			if((i+1)<argc)
			{
				strncpy(FITS_Filename,argv[i+1],STRING_LENGTH-1);
				FITS_Filename[STRING_LENGTH-1] = '\0';
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:fits_filenasme requires a file name.\n");
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
	fprintf(stdout,"Detector Test Exposure:Help.\n");
	fprintf(stdout,"This program takes a series of coadd frames to create an individual exposure using the Raptor Ninox-640 IR detector.\n");
	fprintf(stdout,"detector_test_exposure -e[posure_length] <ms> [-coadd[_exposure_length] <ms>]\n");
	fprintf(stdout,"\t[-fmt[_directory] <dir>][-fits_dir[ectory] <dir>][-fits_file[name] <filename>]\n");
	fprintf(stdout,"\t[-help][-l[og_level <0..5>].\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"The FITS image to save the data into can specified as a filename (-fits_filename),\n");
	fprintf(stdout,"or automatically created in LT format by specifying a directory(-fits_directory).\n");
	fprintf(stdout,"The exposure length of an individual coadd is specified in milliseconds (-coadd_exposure_length),\n");
	fprintf(stdout,"this defaults to %d, a valid '.fmt' file for that exposure length must exist \n",
		DEFAULT_COADD_FRAME_EXPOSURE_LENGTH);
	fprintf(stdout,"in the directory specified by -fmt_directory (default '%s')\n",DEFAULT_FMT_DIRECTORY);
}
