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
#include "ngat_astro.h"
#include "ngat_astro_mjd.h"
#include "detector_buffer.h"
#include "detector_exposure.h"
#include "detector_fits_filename.h"
#include "detector_fits_header.h"
#include "detector_setup.h"
#include "detector_general.h"
#include "fitsio.h"
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
static int Exposure_Save(char *fits_filename);
static void Exposure_TimeSpec_To_Date_String(struct timespec time,char *time_string);
static void Exposure_TimeSpec_To_Date_Obs_String(struct timespec time,char *time_string);
static void Exposure_TimeSpec_To_UtStart_String(struct timespec time,char *time_string);
static int Exposure_TimeSpec_To_Mjd(struct timespec time,int leap_second_correction,double *mjd);

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
 * All the above is implemented as follows:
 * <ul>
 * <li>We check the fits_filename is not NULL.
 * <li>We compute the number of coadds, and check it is within range.
 * <li>We initialise the coadd image buffer to 0 by calling Detector_Buffer_Initialise_Coadd_Image.
 * <li>We take a timestamp for the start of this 'exposure' and store it in Exposure_Data.Exposure_Start_Timestamp.
 * <li>We initialise last_buffer to 0.
 * <li>We call pxd_goLivePair to start camera 1 saving frames to frame grabber buffers 1 and 2.
 * <li>We enter a for loop over Exposure_Data.Coadd_Count:
 *     <ul>
 *     <li>We get a timestamp for the start of this coadd.
 *     <li>We enter a loop until the last capture buffer changes: while (pxd_capturedBuffer(1) == last_buffer).
 *         <ul>
 *         <li>We sleep for a bit (500 us).
 *         <li>We take a current timestamp.
 *         <li>We check whether the current coadd has taken too long, and time out/abort if this is the case. 
 *             The current timeout is 10 times per coadd frame time (Exposure_Data.Coadd_Frame_Exposure_Length_Ms).
 *         </ul>
 *     <li>We update last_buffer to the last captured buffer pxd_capturedBuffer(1).
 *     <li>We call pxd_readushort to read out last_buffer from the frame grabber and put the image contents 
 *         into the allocated mono image buffer (Detector_Buffer_Get_Mono_Image), which has allocated 
 *         Detector_Buffer_Get_Pixel_Count pixels, reading out the whole image from (0,0) to 
 *         (Detector_Setup_Get_Sensor_Size_X,Detector_Setup_Get_Sensor_Size_Y).
 *     <li>We check pxd_readushort read out the whole image.
 *     <li>We add the mono image buffer to the coadd image buffer by calling Detector_Buffer_Add_Mono_To_Coadd_Image.
 *     </ul>
 * <li>We stop the frame grabber acquiring data, by calling pxd_goAbortLive.
 * <li>We create a mean image from the acquired coadds, by calling Detector_Buffer_Create_Mean_Image.
 * <li>We write the image to a FITS image by calling Exposure_Save.
 * </ul>
 * @param exposure_length_ms The overall exposure length in milliseconds, used to determine the number of coadd frames
 *        retrieved from the detector and averaged.
 * @param fits_filename A string containing the FITS image filename to write the read out data into.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Exposure_Error_Number/Exposure_Error_String are set.
 * @see #Exposure_Data
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 * @see #Exposure_Save
 * @see detector_buffer.html#Detector_Buffer_Initialise_Coadd_Image
 * @see detector_buffer.html#Detector_Buffer_Get_Mono_Image
 * @see detector_buffer.html#Detector_Buffer_Get_Pixel_Count
 * @see detector_buffer.html#Detector_Buffer_Add_Mono_To_Coadd_Image
 * @see detector_buffer.html#Detector_Buffer_Create_Mean_Image
 * @see detector_general.html#DETECTOR_GENERAL_ONE_MICROSECOND_NS
 * @see detector_general.html#DETECTOR_GENERAL_ONE_SECOND_MS
 * @see detector_general.html#Detector_General_Log_Format
 * @see detector_setup.html#Detector_Setup_Startup
 * @see detector_setup.html#Detector_Setup_Get_Sensor_Size_X
 * @see detector_setup.html#Detector_Setup_Get_Sensor_Size_Y
 */
int Detector_Exposure_Expose(int exposure_length_ms,char* fits_filename)
{
	struct timespec current_time,coadd_start_time,sleep_time;
	pxbuffer_t last_buffer;
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
	/* initialise last_buffer */
	last_buffer = 0;
	/* turn on image capture into frame buffers 1 and 2 */
	retval = pxd_goLivePair(1,1,2);
	if(retval < 0)
	{
		Exposure_Error_Number = 11;
		sprintf(Exposure_Error_String,"Detector_Exposure_Expose:pxd_goLivePair failed: '%s' (%d).",
			pxd_mesgErrorCode(retval),retval);
		return FALSE;	
	}
	/* loop over coadds */
	for(i=0; i < Exposure_Data.Coadd_Count; i ++)
	{
#if LOGGING > 1
		Detector_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				    "Detector_Exposure_Expose:Starting coadd %d of %d.",i,Exposure_Data.Coadd_Count);
#endif
		/* get a timestamp for the start of this coadd */
		clock_gettime(CLOCK_REALTIME,&coadd_start_time);
		/* enter a loop until the last capture buffer changes */ 
		while (pxd_capturedBuffer(1) == last_buffer)
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
		}/* end while the frame grabber captured buffer is the last_buffer */
		/* update last_buffer */
		last_buffer = pxd_capturedBuffer(1);
		/* copy frame grabber buffer into mono image buffer 
		** Assuming UNITS = 1  here, e.g. 1 detector */
		retval = pxd_readushort(1,last_buffer,0,0,
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
	retval = pxd_goAbortLive(1);
	if(retval < 0)
	{
		Exposure_Error_Number = 12;
		sprintf(Exposure_Error_String,"Detector_Exposure_Expose:pxd_goAbortLive failed: '%s' (%d).",
			pxd_mesgErrorCode(retval),retval);
		return FALSE;	
	}
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
	if(!Exposure_Save(fits_filename))
	{
		/* Exposure_Error_Number set internally to Exposure_Save */
		return FALSE;
	}
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
/**
 * Routine to save the acquired mean image data to a FITS image, with appropriate headers.
 * <ul>
 * <li>We check the FITS filename was not NULL.
 * <li>We get the image dimensions from Detector_Setup_Get_Sensor_Size_X / Detector_Setup_Get_Sensor_Size_Y.
 * <li>We create a lock file for the FITS filename to be saved, by calling Detector_Fits_Filename_Lock.
 * <li>We create the FITS file by calling fits_create_file.
 * <li>We create an image HDU by calling fits_create_img.
 * <li>We write the mean image data into the FITS file by calling fits_write_img, using Detector_Buffer_Get_Mean_Image
 *     to get the mean image data buffer (of length Detector_Buffer_Get_Pixel_Count).
 * <li>We call Detector_Fits_Header_Write_To_Fits to write the previously configured FITS headers into the file.
 * <li>We call Exposure_TimeSpec_To_Date_String to generate a string value to write into the FITS header for the 
 *     DATE keyword, based on Exposure_Data.Exposure_Start_Timestamp.
 * <li>We call Exposure_TimeSpec_To_Date_Obs_String to generate a string value to write into the FITS header for the 
 *     DATE-OBS keyword, based on Exposure_Data.Exposure_Start_Timestamp.
 * <li>We call Exposure_TimeSpec_To_UtStart_String to generate a string value to write into the FITS header for the 
 *     UTSTART keyword, based on Exposure_Data.Exposure_Start_Timestamp.
 * <li>We call Exposure_TimeSpec_To_Mjd to generate a double value to write into the FITS header for the 
 *     MJD keyword, based on Exposure_Data.Exposure_Start_Timestamp.
 * <li>We compute the exposure length in seconds using Exposure_Data.Coadd_Count and 
 *     Exposure_Data.Coadd_Frame_Exposure_Length_Ms, 
 *     and write the computed value as a double to the EXPTIME FITS keyword.
 * <li>We compute an individual coadd exposure length in seconds using Exposure_Data.Coadd_Frame_Exposure_Length_Ms, 
 *     and write the computed value as a double to the COADDSEC FITS keyword.
 * <li>We write the number of coadds (Exposure_Data.Coadd_Count) to the COADDNUM keyword as an integer.
 * <li>We call fits_close_file to close the FITS file and flush any data to disk.
 * <li>We call Detector_Fits_Filename_UnLock to delete the FITS lock file.
 * </ul>
 * @param fits_filename A string, the FITS image filename to save the data into.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Exposure_Error_Number/Exposure_Error_String are set.
 * @see #Exposure_Data
 * @see #Exposure_Error_Number
 * @see #Exposure_Error_String
 * @see #Exposure_TimeSpec_To_Date_String
 * @see #Exposure_TimeSpec_To_Date_Obs_String
 * @see #Exposure_TimeSpec_To_UtStart_String
 * @see #Exposure_TimeSpec_To_Mjd
 * @see detector_buffer.html#Detector_Buffer_Get_Pixel_Count
 * @see detector_buffer.html#Detector_Buffer_Get_Mean_Image
 * @see detector_fits_filename.html#Detector_Fits_Filename_Lock
 * @see detector_fits_filename.html#Detector_Fits_Filename_UnLock
 * @see detector_fits_header.html#Detector_Fits_Header_Write_To_Fits
 * @see detector_general.html#DETECTOR_GENERAL_ONE_SECOND_MS
 * @see detector_general.html#Detector_General_Log_Format
 * @see detector_setup.html#Detector_Setup_Get_Sensor_Size_X
 * @see detector_setup.html#Detector_Setup_Get_Sensor_Size_Y
 */
static int Exposure_Save(char *fits_filename)
{
	fitsfile *fits_fp = NULL;
	char exposure_start_time_string[64];
	char buff[32]; /* fits_get_errstatus returns 30 chars max */
	long axes[2];
	int status = 0,retval,ivalue,ncols,nrows;
	double exposure_length,mjd;
	
	Exposure_Error_Number = 0;
	if(fits_filename == NULL)
	{
		Exposure_Error_Number = 13;
		sprintf(Exposure_Error_String,"Exposure_Save:fits_filename was NULL.");
		return FALSE;
	}
#if LOGGING > 1
	Detector_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"Exposure_Save:Saving FITS image '%s'.",fits_filename);
#endif
	/* get dimensions */
	ncols = Detector_Setup_Get_Sensor_Size_X();
	nrows = Detector_Setup_Get_Sensor_Size_Y();
	/* create lock file for image to be saved */
	if(!Detector_Fits_Filename_Lock(fits_filename))
	{
		Exposure_Error_Number = 14;
		sprintf(Exposure_Error_String,"Exposure_Save:Failed to create lock file for FITS image '%s'.",fits_filename);
		return FALSE;
	}
	/* open file */
	if(fits_create_file(&fits_fp,fits_filename,&status))
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		Detector_Fits_Filename_UnLock(fits_filename);
		Exposure_Error_Number = 15;
		sprintf(Exposure_Error_String,"Exposure_Save: File create failed(%s,%d,%s).",fits_filename,status,buff);
		return FALSE;
	}
	/* create image block */
	axes[0] = ncols;
	axes[1] = nrows;
	retval = fits_create_img(fits_fp,DOUBLE_IMG,2,axes,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fits_fp,&status);
		Detector_Fits_Filename_UnLock(fits_filename);
		Exposure_Error_Number = 16;
		sprintf(Exposure_Error_String,"Exposure_Save: Create image failed(%s,%d,%s).",fits_filename,status,buff);
		return FALSE;
	}
	/* write the data */
	retval = fits_write_img(fits_fp,TDOUBLE,1,Detector_Buffer_Get_Pixel_Count(),Detector_Buffer_Get_Mean_Image(),
				&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fits_fp,&status);
		Detector_Fits_Filename_UnLock(fits_filename);
		Exposure_Error_Number = 17;
		sprintf(Exposure_Error_String,"Exposure_Save: File write image failed(%s,%d,%s).",fits_filename,status,buff);
		return FALSE;
	}
	/* save FITS headers to filename */
	if(!Detector_Fits_Header_Write_To_Fits(fits_fp))
	{
		fits_close_file(fits_fp,&status);
		Detector_Fits_Filename_UnLock(fits_filename);
		Exposure_Error_Number = 18;
		sprintf(Exposure_Error_String,"Exposure_Save:Detector_Fits_Header_Write_To_Fits failed.");
		return FALSE;
	}
	/* update DATE keyword */
	Exposure_TimeSpec_To_Date_String(Exposure_Data.Exposure_Start_Timestamp,exposure_start_time_string);
	retval = fits_update_key(fits_fp,TSTRING,"DATE",exposure_start_time_string,NULL,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fits_fp,&status);
		Detector_Fits_Filename_UnLock(fits_filename);
		Exposure_Error_Number = 19;
		sprintf(Exposure_Error_String,"Exposure_Save: Updating DATE failed(%s,%d,%s).",fits_filename,status,buff);
		return FALSE;
	}
	/* update DATE-OBS keyword */
	Exposure_TimeSpec_To_Date_Obs_String(Exposure_Data.Exposure_Start_Timestamp,exposure_start_time_string);
	retval = fits_update_key(fits_fp,TSTRING,"DATE-OBS",exposure_start_time_string,NULL,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fits_fp,&status);
		Detector_Fits_Filename_UnLock(fits_filename);
		Exposure_Error_Number = 20;
		sprintf(Exposure_Error_String,"Exposure_Save: Updating DATE-OBS failed(%s,%d,%s).",fits_filename,
			status,buff);
		return FALSE;
	}
	/* update UTSTART keyword */
	Exposure_TimeSpec_To_UtStart_String(Exposure_Data.Exposure_Start_Timestamp,exposure_start_time_string);
	retval = fits_update_key(fits_fp,TSTRING,"UTSTART",exposure_start_time_string,NULL,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fits_fp,&status);
		Detector_Fits_Filename_UnLock(fits_filename);
		Exposure_Error_Number = 21;
		sprintf(Exposure_Error_String,"Exposure_Save: Updating UTSTART failed(%s,%d,%s).",fits_filename,
			status,buff);
		return FALSE;
	}
	/* update MJD keyword */
	/* note leap second correction not implemented yet (always FALSE). */
	if(!Exposure_TimeSpec_To_Mjd(Exposure_Data.Exposure_Start_Timestamp,FALSE,&mjd))
	{
		Detector_Fits_Filename_UnLock(fits_filename);
		return FALSE;
	}
	retval = fits_update_key_fixdbl(fits_fp,"MJD",mjd,6,NULL,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fits_fp,&status);
		Detector_Fits_Filename_UnLock(fits_filename);
		Exposure_Error_Number = 22;
		sprintf(Exposure_Error_String,"Exposure_Save: Updating MJD failed(%.2f,%s,%d,%s).",mjd,fits_filename,
			status,buff);
		return FALSE;
	}
	/* update EXPTIME keyword */
	/* compute exposure length in decimal seconds */
	exposure_length = ((double)(Exposure_Data.Coadd_Count*Exposure_Data.Coadd_Frame_Exposure_Length_Ms))/
		((double)DETECTOR_GENERAL_ONE_SECOND_MS);
	retval = fits_update_key_fixdbl(fits_fp,"EXPTIME",exposure_length,6,NULL,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fits_fp,&status);
		Detector_Fits_Filename_UnLock(fits_filename);
		Exposure_Error_Number = 23;
		sprintf(Exposure_Error_String,"Exposure_Save: Updating exposure length failed(%.2f,%s,%d,%s).",
			exposure_length,fits_filename,status,buff);
		return FALSE;
	}
	/* update COADDSEC keyword:- this is the exposure length of one coadd in decimal seconds */
	exposure_length = ((double)Exposure_Data.Coadd_Frame_Exposure_Length_Ms)/((double)DETECTOR_GENERAL_ONE_SECOND_MS);
	retval = fits_update_key_fixdbl(fits_fp,"COADDSEC",exposure_length,6,NULL,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fits_fp,&status);
		Detector_Fits_Filename_UnLock(fits_filename);
		Exposure_Error_Number = 24;
		sprintf(Exposure_Error_String,"Exposure_Save: Updating coadd exposure length failed(%.2f,%s,%d,%s).",
			exposure_length,fits_filename,status,buff);
		return FALSE;
	}
	/* update COADDNUM keyword */
	retval = fits_update_key(fits_fp,TINT,"COADDNUM",&(Exposure_Data.Coadd_Count),"Number of coadds",&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fits_fp,&status);
		Detector_Fits_Filename_UnLock(fits_filename);
		Exposure_Error_Number = 25;
		sprintf(Exposure_Error_String,"Exposure_Save: Updating number of coadds failed(%d,%s,%d,%s).",
		       Exposure_Data.Coadd_Count,fits_filename,status,buff);
		return FALSE;
	}
	/* ensure data we have written is in the actual data buffer, not CFITSIO's internal buffers */
	/* closing the file ensures this. */ 
	retval = fits_close_file(fits_fp,&status);
	if(retval)
	{
		fits_get_errstatus(status,buff);
		fits_report_error(stderr,status);
		fits_close_file(fits_fp,&status);
		Detector_Fits_Filename_UnLock(fits_filename);
		Exposure_Error_Number = 26;
		sprintf(Exposure_Error_String,"Exposure_Save: File close file failed(%s,%d,%s).",fits_filename,status,buff);
		return FALSE;
	}
	/* remove lock file */
	if(!Detector_Fits_Filename_UnLock(fits_filename))
	{
		Exposure_Error_Number = 27;
		sprintf(Exposure_Error_String,"Exposure_Save:Failed to unlock '%s'.",fits_filename);
		return FALSE;				
	}
#if LOGGING > 1
	Detector_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"Exposure_Save:Finished saving '%s'.",fits_filename);
#endif
	return TRUE;
}

/**
 * Routine to convert a timespec structure to a DATE sytle string to put into a FITS header.
 * This uses gmtime and strftime to format the string. The resultant string is of the form:
 * <b>CCYY-MM-DD</b>, which is equivalent to %Y-%m-%d passed to strftime.
 * @param time The time to convert.
 * @param time_string The string to put the time representation in. The string must be at least
 * 	12 characters long.
 */
static void Exposure_TimeSpec_To_Date_String(struct timespec time,char *time_string)
{
	struct tm *tm_time = NULL;

	tm_time = gmtime(&(time.tv_sec));
	strftime(time_string,12,"%Y-%m-%d",tm_time);
}

/**
 * Routine to convert a timespec structure to a DATE-OBS sytle string to put into a FITS header.
 * This uses gmtime and strftime to format most of the string, and tags the milliseconds on the end.
 * The resultant form of the string is <b>CCYY-MM-DDTHH:MM:SS.sss</b>.
 * @param time The time to convert.
 * @param time_string The string to put the time representation in. The string must be at least
 * 	24 characters long.
 * @see detector_general.html#DETECTOR_GENERAL_ONE_MILLISECOND_NS
 */
static void Exposure_TimeSpec_To_Date_Obs_String(struct timespec time,char *time_string)
{
	struct tm *tm_time = NULL;
	char buff[32];
	int milliseconds;

	tm_time = gmtime(&(time.tv_sec));
	strftime(buff,32,"%Y-%m-%dT%H:%M:%S.",tm_time);
	milliseconds = (((double)time.tv_nsec)/((double)DETECTOR_GENERAL_ONE_MILLISECOND_NS));
	sprintf(time_string,"%s%03d",buff,milliseconds);
}

/**
 * Routine to convert a timespec structure to a UTSTART sytle string to put into a FITS header.
 * This uses gmtime and strftime to format most of the string, and tags the milliseconds on the end.
 * @param time The time to convert.
 * @param time_string The string to put the time representation in. The string must be at least
 * 	14 characters long.
 * @see detector_general.html#DETECTOR_GENERAL_ONE_MILLISECOND_NS
 */
static void Exposure_TimeSpec_To_UtStart_String(struct timespec time,char *time_string)
{
	struct tm *tm_time = NULL;
	char buff[16];
	int milliseconds;

	tm_time = gmtime(&(time.tv_sec));
	strftime(buff,16,"%H:%M:%S.",tm_time);
	milliseconds = (((double)time.tv_nsec)/((double)DETECTOR_GENERAL_ONE_MILLISECOND_NS));
	sprintf(time_string,"%s%03d",buff,milliseconds);
}

/**
 * Routine to convert a timespec structure to a Modified Julian Date (decimal days) to put into a FITS header.
 * This uses NGAT_Astro_Timespec_To_MJD to get the MJD.
 * <p>This routine is still wrong for last second of the leap day, as gmtime will return 1st second of the next day.
 * Also note the passed in leap_second_correction should change at midnight, when the leap second occurs.
 * None of this should really matter, 1 second will not affect the MJD for several decimal places.
 * @param time The time to convert.
 * @param leap_second_correction A number representing whether a leap second will occur. This is normally zero,
 * 	which means no leap second will occur. It can be 1, which means the last minute of the day has 61 seconds,
 *	i.e. there are 86401 seconds in the day. It can be -1,which means the last minute of the day has 59 seconds,
 *	i.e. there are 86399 seconds in the day.
 * @param mjd The address of a double to store the calculated MJD.
 * @return The routine returns TRUE if it succeeded, FALSE if it fails. 
 */
static int Exposure_TimeSpec_To_Mjd(struct timespec time,int leap_second_correction,double *mjd)
{
	int retval;

	retval = NGAT_Astro_Timespec_To_MJD(time,leap_second_correction,mjd);
	if(retval == FALSE)
	{
		Exposure_Error_Number = 28;
		sprintf(Exposure_Error_String,"CCD_Exposure_TimeSpec_To_Mjd:NGAT_Astro_Timespec_To_MJD failed.\n");
		/* concatenate NGAT Astro library error onto Exposure_Error_String */
		NGAT_Astro_Error_String(Exposure_Error_String+strlen(Exposure_Error_String));
		return FALSE;
	}
	return TRUE;
}

