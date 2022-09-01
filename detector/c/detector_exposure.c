/* detector_exposure.c
** Raptor Ninox-640 Infrared detector library : exposure routines.
*/
/**
 * Routines to look after acquiring data (exposures) using the Raptor Ninox-640 Infrared detector.
 * @author Chris Mottram
 * @version $Revision$
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log_udp.h"
#include "detector_buffer.h"
#include "detector_exposure.h"
#include "detector_setup.h"
#include "detector_general.h"
#include "xcliball.h"

/* data types */
/**
 * Data type holding local data to detector_exposure. This consists of the following:
 * <dl>
 * <dt>Coadd_Frame_Exposure_Length_Ms</dt> <dd>The exposure length of an individual coadd 
 *     within an individual exposure, in ms. This value is determined by the '.fmt' file used to setup the 
 *     connection to the frame grabber. An individual exposure for this detector, 
 *     is determined to be a number of coadds, where each coadd has an exposure length of 
 *     Coadd_Frame_Exposure_Length_Ms, so 
 *     the number of coadds =  the 'exposure length' / Coadd_Frame_Exposure_Length_Ms.</dd>
 * </dl>
 */
struct Exposure_Struct
{
	int Coadd_Frame_Exposure_Length_Ms;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of Exposure_Struct that contains local data for this module. This is initialised as follows:
 * <dl>
 * <dt>Coadd_Frame_Exposure_Length_Ms</dt> <dd>0</dd>
 * </dl>
 */
static struct Exposure_Struct Exposure_Data = 
{
	0
};

/**
 * Variable holding error code of last operation performed.
 */
static int Exposure_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see detector_general.html#DETECTOR_GENERAL_ERROR_STRING_LENGTH
 */
static char Exposure_Error_String[DETECTOR_GENERAL_ERROR_STRING_LENGTH] = "";

/* internal functions */

/* --------------------------------------------------------
** External Functions
** -------------------------------------------------------- */
/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Exposure_Error_Number
 */
int Detector_Exposure_Get_Error_Number(void)
{
	return Exposure_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 * @see detector_general.html#Detector_General_Get_Current_Time_String
 */
void Detector_Exposure_Error(void)
{
	char time_string[32];

	Detector_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Exposure_Error_Number == 0)
		sprintf(Exposure_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s Detector_Exposure:Error(%d) : %s\n",time_string,Exposure_Error_Number,Exposure_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 * @see detector_general.html#Detector_General_Get_Current_Time_String
 */
void Detector_Exposure_Error_String(char *error_string)
{
	char time_string[32];

	Detector_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Exposure_Error_Number == 0)
		sprintf(Exposure_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s Detector_Exposure:Error(%d) : %s\n",time_string,
		Exposure_Error_Number,Exposure_Error_String);
}

/* =======================================
**  internal functions 
** ======================================= */
