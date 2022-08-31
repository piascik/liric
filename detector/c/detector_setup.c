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
