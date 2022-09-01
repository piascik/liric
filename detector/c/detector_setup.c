/* deetctor_setup.c
** Raptor Ninox-640 Infrared detector library : setup routines.
*/
/**
 * Routines to look after setting up the Raptor Ninox-640 Infrared detector for acquisition.
 * @author Chris Mottram
 * @version $Revision$
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
/* These POSIX prototypes don't work with xclibsc.h: uint is not found 
** #define _POSIX_SOURCE 1*/
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
/*These POSIX prototypes don't work with xclibsc.h: uint is not found 
** #define _POSIX_C_SOURCE 199309L*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log_udp.h"
#include "detector_setup.h"
#include "detector_general.h"
#include "xcliball.h"

/* hash defines */
/**
 * Define which camera we are talking to the xclib way, the first one.
 */
#define UNITS	                  1
/**
 * Define a bitwise definition of which cameras we are talking to, to pass into XCLIB functions.
 */
#define UNITSMAP                  ((1<<UNITS)-1) 

/* data types */
/**
 * Data type holding local data to detector_setup. This consists of the following:
 * <dl>
 * <dt>Units</dt> <dd>A bitwise definition of which cameras we are talking to, this should always be 1 for Raptor.</dd>
 * </dl>
 */
struct Setup_Struct
{
	int Units;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of Setup_Struct that contains local data for this module. This is initialised as follows:
 * <dl>
 * <dt>Units</dt> <dd>1</dd>
 * </dl>
 */
static struct Setup_Struct Setup_Data = 
{
	1
};

/**
 * Variable holding error code of last operation performed.
 */
static int Setup_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see detector_general.html#DETECTOR_GENERAL_ERROR_STRING_LENGTH
 */
static char Setup_Error_String[DETECTOR_GENERAL_ERROR_STRING_LENGTH] = "";
/* internal functions */

/* --------------------------------------------------------
** External Functions
** -------------------------------------------------------- */
/**
 * Routine to initialise the Raptor detector.
 * <ul>
 * <li>Detector_Setup_Open is called with the specified format_file.
 * </ul>
 * @param formatfile The filename of a '.fmt' format file, used to configure the video mode of the detector.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Setup_Error_Number/Setup_Error_String are set.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Detector_Setup_Open
 */
int Detector_Setup_Startup(char *format_filename)
{
	int retval;
	
	Setup_Error_Number = 0;
	if(format_filename == NULL)
	{
		Setup_Error_Number = 3;
		sprintf(Setup_Error_String,"Detector_Setup_Startup:format_file was NULL.");
		return FALSE;
	}
#if LOGGING > 1
	Detector_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				    "Detector_Setup_Startup(format_file=%s):Started.",
				    format_filename);
#endif
	if(!Detector_Setup_Open("","",format_filename))
	{
		/* Setup_Error_Number / Setup_Error_String set in Detector_Setup_Open */
		return FALSE;
	}
	
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Setup_Startup:Finished.");
#endif
	return TRUE;
}

/**
 * Routine to close the connection to the Raptor detector.
 * <ul>
 * <li>Detector_Setup_Close is called.
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Setup_Error_Number/Setup_Error_String are set.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Detector_Setup_Close
 */
int Detector_Setup_Shutdown(void)
{
#if LOGGING > 1
	Detector_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"Detector_Setup_Shutdown:Started.");
#endif
	if(!Detector_Setup_Close())
	{
		/* Setup_Error_Number / Setup_Error_String set in Detector_Setup_Close */
		return FALSE;
	}
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Setup_Shutdown:Finished.");
#endif
	return TRUE;
}

/**
 * Routine to open a connection to the library/driver, using the xclib pxd_PIXCIopen routine.
 * @param driverparms A driver configuration parameter string.
 * @param formatname The video format as a string, We usually set this to a blank string and use the formatfile instead.
 * @param formatfile The filename of a '.fmt' format file, used to configure the video mode of the detector.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Setup_Error_Number/Setup_Error_String are set.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 */
int Detector_Setup_Open(char *driverparms,char *formatname, char *formatfile)
{
	int retval;
	
	Setup_Error_Number = 0;
#if LOGGING > 1
	Detector_General_Log_Format(LOG_VERBOSITY_VERBOSE,
				    "Detector_Setup_Open(driverparms=%s,formatname=%s,formatfile=%s):Started.",
				    driverparms,formatname,formatfile);
#endif
	retval = pxd_PIXCIopen(driverparms,formatname,formatfile);
	if(retval < 0)
	{
		Setup_Error_Number = 1;
		sprintf(Setup_Error_String,"Detector_Setup_Open:pxd_PIXCIopen failed: %s (%d).",
			pxd_mesgErrorCode(retval),retval);
		return FALSE;
	}
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_VERBOSE,"Detector_Setup_Open:Finished.");
#endif
	return TRUE;
}

/**
 * Routine to close the previously opened connection to the library/driver, using the xclib pxd_PIXCIclose routine.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Setup_Error_Number/Setup_Error_String are set.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 */
int Detector_Setup_Close(void)
{
	int retval;
	
	Setup_Error_Number = 0;
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_VERBOSE,"Detector_Setup_Close:Started.");
#endif
	retval = pxd_PIXCIclose();
	if(retval < 0)
	{
		Setup_Error_Number = 2;
		sprintf(Setup_Error_String,"Detector_Setup_Close:pxd_PIXCIclose failed: %s (%d).",
			pxd_mesgErrorCode(retval),retval);
		return FALSE;
	}
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_VERBOSE,"Detector_Setup_Close:Finished.");
#endif
	return TRUE;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Setup_Error_Number
 */
int Detector_Setup_Get_Error_Number(void)
{
	return Setup_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see detector_general.html#Detector_General_Get_Current_Time_String
 */
void Detector_Setup_Error(void)
{
	char time_string[32];

	Detector_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Setup_Error_Number == 0)
		sprintf(Setup_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s Detector_Setup:Error(%d) : %s\n",time_string,Setup_Error_Number,Setup_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see detector_general.html#Detector_General_Get_Current_Time_String
 */
void Detector_Setup_Error_String(char *error_string)
{
	char time_string[32];

	Detector_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Setup_Error_Number == 0)
		sprintf(Setup_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s Detector_Setup:Error(%d) : %s\n",time_string,
		Setup_Error_Number,Setup_Error_String);
}

/* =======================================
**  internal functions 
** ======================================= */
