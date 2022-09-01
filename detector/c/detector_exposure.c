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
 * <dt>Exposure_Length_Ms</dt> <dd>The overall exposure length for the current exposure, in milliseconds.</dd>
 * <dt>Coadd_Count</dt> <dd>The number of coadds needed (each of length Coadd_Frame_Exposure_Length_Ms), 
 *                      to do the requested exposure length of length Exposure_Length_Ms.</dd>
 * <dt>Exposure_Start_Timestamp</dt> <dd>A timestamp taken at the start of an exposure.</dd>
 * </dl>
 */
struct Exposure_Struct
{
	int Coadd_Frame_Exposure_Length_Ms;
	int Exposure_Length_Ms;
	int Coadd_Count;
	struct timespec Exposure_Start_Timestamp;
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
 * <dt>Exposure_Length_Ms</dt> <dd>0</dd>
 * <dt>Coadd_Count</dt> <dd>0</dd>
 * <dt>Exposure_Start_Timestamp</dt> <dd>{0,0}</dd>
 * </dl>
 */
static struct Exposure_Struct Exposure_Data = 
{
	0,0,0,{0,0}
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
 * Routine to set the exposure length of an individual coadd frame (effectively the frame grabber frame rate). 
 * This is configured in the '.fmt' file supplied when opening the connection to the frame grabber 
 * (Detector_Setup_Startup). The value in the '.fmt' config file must match the value supplied here,
 * if exposures are to be of the right exposure length (have the right number of coadds).
 * @param The exposure length of an individual coadd frame , in milliseconds.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Exposure_Error_Number/Exposure_Error_String are set.
 * @see #Exposure_Data
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 * @see detector_setup.html#Detector_Setup_Startup
 */
int Detector_Exposure_Set_Coadd_Frame_Exposure_Length(int coadd_frame_exposure_length_ms)
{
	if(coadd_frame_exposure_length_ms < 1)
	{
		Exposure_Error_Number = 1;
		sprintf(Exposure_Error_String,
			"Detector_Exposure_Set_Coadd_Frame_Exposure_Length:exposure length was too short:%d.",
			coadd_frame_exposure_length_ms);
		return FALSE;
	}
	Exposure_Data.Coadd_Frame_Exposure_Length_Ms = coadd_frame_exposure_length_ms;
#if LOGGING > 1
	Detector_General_Log_Format(LOG_VERBOSITY_TERSE,
	       "Detector_Exposure_Set_Coadd_Frame_Exposure_Length: Coadd frame exposure length set to %d ms.",
				    coadd_frame_exposure_length_ms);
#endif
	return TRUE;
}

/**
 * Routine to take an individual 'exposure' with the detector. Here 'exposure' means a series of coadd frames, 
 * each of the previously configured Coadd_Frame_Exposure_Length_Ms 
 * (see Detector_Exposure_Set_Coadd_Frame_Exposure_Length). 
 * The number of coadd frames is determined by dividing the exposure_length_ms parameter 
 * by the individual coadd frame exposure length Coadd_Frame_Exposure_Length_Ms. 
 * The resultant mean of the coadds is written to the specified FITS filename.
 * @param exposure_length_ms The overall exposure length in milliseconds, used to determine the number of coadd frames
 *        retrieved from the detector and averaged.
 * @param fits_filename A string containing the FITS image filename to write the read out data into.
 * @see #Exposure_Data
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 * @see detector_general.html#DETECTOR_GENERAL_ONE_MICROSECOND_NS
 * @see detector_setup.html#Detector_Setup_Startup
 */
int Detector_Exposure_Expose(int exposure_length_ms,char* fits_filename)
{
	struct timespec current_time,coadd_start_time,sleep_time;
	pxbuffer_t lastbuf;
	int i,retval;
	
	Exposure_Error_Number = 0;
	if(fits_filename ==NULL)
	{
		Exposure_Error_Number = 2;
		sprintf(Exposure_Error_String,"Detector_Exposure_Expose:FITS filename was NULL.");
		return FALSE;
	}
#if LOGGING > 1
	Detector_General_Log_Format(LOG_VERBOSITY_TERSE,
				    "Detector_Exposure_Expose(exposure_length=%d ms,fits_filename = '%s':Started.",
				    exposure_length_ms,fits_filename);
#endif
	/* range check exposure length / compute coadds */
	Exposure_Data.Exposure_Length_Ms = exposure_length_ms;
	/* may need to revisit this when thinking about bias frames */
	if(Exposure_Data.Coadd_Frame_Exposure_Length_Ms < 1)
	{
		Exposure_Error_Number = 3;
		sprintf(Exposure_Error_String,"Detector_Exposure_Expose:Coadd frame Exposure length %d ms too small.",
			Exposure_Data.Coadd_Frame_Exposure_Length_Ms);
		return FALSE;
	}
	Exposure_Data.Coadd_Count = Exposure_Data.Exposure_Length_Ms / Exposure_Data.Coadd_Frame_Exposure_Length_Ms;
	if(Exposure_Data.Coadd_Count < 1)
	{
		Exposure_Error_Number = 4;
		sprintf(Exposure_Error_String,"Detector_Exposure_Expose:Exposure length %d ms was too short "
			"for this fmt configuration (coadd frame exposure length %d).",
			Exposure_Data.Exposure_Length_Ms,Exposure_Data.Coadd_Frame_Exposure_Length_Ms);
		return FALSE;	
	}
#if LOGGING > 1
	Detector_General_Log_Format(LOG_VERBOSITY_TERSE,
				    "Detector_Exposure_Expose:exposure_length=%d ms has %d coadds each of length %d ms.",
				    Exposure_Data.Exposure_Length_Ms,Exposure_Data.Coadd_Count,
				    Exposure_Data.Coadd_Frame_Exposure_Length_Ms);
#endif
	/* reset coadd image to 0 */
	if(!Detector_Buffer_Initialise_Coadd_Image())
	{
		Exposure_Error_Number = 5;
		sprintf(Exposure_Error_String,"Detector_Exposure_Expose:Failed to initialise coadd image.");
		return FALSE;	
	}
	/* take start of exposure timestamp */
	clock_gettime(CLOCK_REALTIME,&(Exposure_Data.Exposure_Start_Timestamp));
	/* turn on image capture into frame buffers 1 and 2 */
	lastbuf = 0;
	pxd_goLivePair(1,1,2);
	for(i=0; i < Exposure_Data.Coadd_Count; i ++)
	{
		/* get a timestamp for the start of this coadd */
		clock_gettime(CLOCK_REALTIME,&coadd_start_time);
		/* enter a loop until the last capture buffer changes */ 
		while (pxd_capturedBuffer(1)==lastbuf)
		{
			/* sleep a bit (500 us) */
			sleep_time.tv_sec = 0;
			sleep_time.tv_nsec = 500*DETECTOR_GENERAL_ONE_MICROSECOND_NS;
			nanosleep(&sleep_time,&sleep_time);
			/* check for timeout  - make this 10 times the Coadd_Frame_Exposure_Length
			** Note fdifftime works in decimal seconds */
			clock_gettime(CLOCK_REALTIME,&current_time);
			if(fdifftime(current_time,coadd_start_time) >
			   (((double)(Exposure_Data.Coadd_Frame_Exposure_Length_Ms*10))/DETECTOR_GENERAL_ONE_SECOND_MS))
			{
				Exposure_Error_Number = 6;
				sprintf(Exposure_Error_String,
					"Detector_Exposure_Expose:Timed out whilst waiting for a new capture buffer "
					"(%d of %d coadds), coadd frame exposure length %d ms, timeout length %.3f s.",
					i, Exposure_Data.Coadd_Count, Exposure_Data.Coadd_Frame_Exposure_Length_Ms,
			     (((double)(Exposure_Data.Coadd_Frame_Exposure_Length_Ms*10))/DETECTOR_GENERAL_ONE_SECOND_MS));
				pxd_goAbortLive(1);
				return FALSE;
			}
		}/* end while the frame grabber captured buffer is the lastbuf */
		/* update lastbuf */
		lastbuf = pxd_capturedBuffer(1);
		/* copy frame grabber buffer into mono image buffer 
		** Assuming UNITS = 1  here, e.g. 1 detector */
		retval = pxd_readushort(1,lastbuf,0,0,
					Detector_Setup_Get_Sensor_Size_X(),Detector_Setup_Get_Sensor_Size_Y(),
					Detector_Buffer_Get_Mono_Image(),Detector_Buffer_Get_Pixel_Count(),"Grey");
		if(retval < 0)
		{
			Exposure_Error_Number = 7;
			sprintf(Exposure_Error_String,
				"Detector_Exposure_Expose:pxd_readushort failed: '%s' (%d).",
				pxd_mesgErrorCode(retval),retval);
			pxd_goAbortLive(1);
			return FALSE;	
		}
		/* check pxd_readushort read out the whole image */
		if(retval != Detector_Buffer_Get_Pixel_Count())
		{
			Exposure_Error_Number = 8;
			sprintf(Exposure_Error_String,
				"Detector_Exposure_Expose:pxd_readushort read %d of %d pixels.",
				retval,Detector_Buffer_Get_Pixel_Count());
			pxd_goAbortLive(1);
			return FALSE;				
		}
		/* Add mono image buffer to coadd image buffer */
		if(!Detector_Buffer_Add_Mono_To_Coadd_Image())
		{
			pxd_goAbortLive(1);
			Exposure_Error_Number = 9;
			sprintf(Exposure_Error_String,
				"Detector_Exposure_Expose:Failed to copy mono image buffer to coadd image.");
			return FALSE;	
		}
	}/* end for (i) on Coadd_Count */
	/* stop the frame grabber acquiring data */
	pxd_goAbortLive(1);
	/* create mean image from coadds */
	if(!Detector_Buffer_Create_Mean_Image(Exposure_Data.Coadd_Count))
	{
		Exposure_Error_Number = 10;
		sprintf(Exposure_Error_String,
			"Detector_Exposure_Expose:Failed to create mean image from coadd image with %d coadds.",
			Exposure_Data.Coadd_Count);
		return FALSE;	
	}
	/* write FITS image */

	/* diddly */
	
#if LOGGING > 1
	Detector_General_Log_Format(LOG_VERBOSITY_TERSE,
				    "Detector_Exposure_Expose(exposure_length=%d ms,fits_filename = '%s':Finished.",
				    exposure_length_ms,fits_filename);
#endif
	return TRUE;
}
	
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
