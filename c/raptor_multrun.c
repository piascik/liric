/* raptor_multrun.c
** Raptor multrun routines
*/
/**
 * Multrun routines for the raptor program.
 * @author Chris Mottram
 * @version $Revision$
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_C_SOURCE 199309L
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log_udp.h"

#include "detector_buffer.h"
#include "detector_command.h"
#include "detector_exposure.h"
#include "detector_fits_filename.h"
#include "detector_fits_header.h"
#include "detector_setup.h"
#include "detector_temperature.h"

#include "filter_wheel_command.h"
#include "filter_wheel_config.h"

#include "nudgematic_command.h"

#include "raptor_config.h"
#include "raptor_fits_header.h"
#include "raptor_general.h"
#include "raptor_multrun.h"

/* hash defines */
/**
 * Length of FITS filename string.
 */
#define MULTRUN_FITS_FILENAME_LENGTH  (256)

/* data types */
/**
 * Data type holding local data to raptor multruns.
 * <dl>
 * <dt>CCD_Temperature</dt> <dd>A copy of the current CCD temperature, taken at the start of a multrun. 
 *                              Used to populate FITS headers.</dd>
 * <dt>Requested_Exposure_Length</dt> <dd>A copy of the per-frame requested exposure length (in seconds) 
 *                                        used to configure the CCD camera. Used to populate FITS headers.</dd>
 * <dt>Image_Index</dt> <dd>Which frame in the multrun we are currently working on.</dd>
 * <dt>Image_Count</dt> <dd>The number of FITS images we are expecting to generate in the current multrun.</dd>
 * <dt>Multrun_Start_Time</dt> <dd>A timestamp taken the first time an exposure was started in the multrun 
 *                              (actually, just before we start waiting for the next image to arrive, 
 *                              the timestamp is only approximate). Used for calculating TELAPSE.</dd>
 * <dt>Exposure_Start_Time</dt> <dd>A timestamp taken the last time an exposure was started in the multrun 
 *                              (actually, just before we start waiting for the next image to arrive, 
 *                              the timestamp is only approximate).</dd>
 * </dl>
 */
struct Multrun_Struct
{
	double CCD_Temperature;
	double Requested_Exposure_Length;
	int Image_Index;
	int Image_Count;
	struct timespec Multrun_Start_Time;
	struct timespec Exposure_Start_Time;
};

/* internal data */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * Multrun Data holding local data to raptor multruns.
 * <dl>
 * <dt>CCD_Temperature</dt>               <dd>0.0</dd>
 * <dt>Requested_Exposure_Length</dt>     <dd>0.0</dd>
 * <dt>Image_Index</dt>                   <dd>0</dd>
 * <dt>Image_Count</dt>                   <dd>0</dd>
 * <dt>Multrun_Start_Time</dt>            <dd>{0,0}</dd>
 * <dt>Exposure_Start_Time</dt>           <dd>{0,0}</dd>
 * </dl>
 * @see #Multrun_Struct
 */
static struct Multrun_Struct Multrun_Data =
{
	0.0,0,0,{0,0},{0,0}
};

/**
 * Is a multrun in progress.
 */
static int Multrun_In_Progress = FALSE;
/**
 * Abort any multrun currently in progress.
 */
static int Moptop_Abort = FALSE;

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
int Raptor_Multrun_Coadd_Exposure_Length_Set(int coadd_exposure_length_ms)
{
	return TRUE;
}

int Raptor_Multrun(int exposure_length_ms,int exposure_count,int do_standard,
			  char ***filename_list,int *filename_count)
{
	return TRUE;
}

int Raptor_Multrun_Abort(void)
{
	return TRUE;
}

int Raptor_Multrun_In_Progress(void)
{
	return Multrun_In_Progress;
}

/**
 * Return the total number of exposures expected to be generated in the current/last multrun.
 * @return The number of images/frames expected.
 * @see #Multrun_Data
 */
int Raptor_Multrun_Count_Get(void)
{
	return Multrun_Data.Image_Count;
}

int Raptor_Multrun_Exposure_Length_Get(void)
{
}

int Raptor_Multrun_Exposure_Start_Time_Get(struct timespec *exposure_start_time)
{
	if(exposure_start_time == NULL)
	{
		Raptor_General_Error_Number = ;
		sprintf(Raptor_General_Error_String,
			"Raptor_Multrun_Exposure_Start_Time_Get:exposure_start_time was NULL.");
		return FALSE;
	}
	(*exposure_start_time) = Multrun_Data.Multrun_Start_Time;
	return TRUE;
}

/**
 * Return which exposure in the multrun we are on.
 * @return The exposure index in the multrun.
 * @see #Multrun_Data
 */
int Raptor_Multrun_Exposure_Index_Get(void)
{
	return Multrun_Data.Image_Index;
}

/**
 * Return the multrun number (in the generated FITS filenames) of this multrun.
 * @return The current multrun number.
 * @see ../detector/cdocs/detector_fits_filename.html#Detector_Fits_Filename_Multrun_Get
 */
int Raptor_Multrun_Multrun_Get(void)
{
	return Detector_Fits_Filename_Multrun_Get();
}

/**
 * Return the run number (in the generated FITS filenames) of this multrun.
 * @return The current run number.
 * @see #Multrun_Data
 */
int Raptor_Multrun_Run_Get(void)
{
}
