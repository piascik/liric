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
/**
 * Conversion from degrees centigrade to Kelvin.
 */
#define CENTIGRADE_TO_KELVIN          (273.15)

/* data types */
/**
 * Data type holding local data to raptor multruns.
 * <dl>
 * <dt>CCD_Temperature</dt> <dd>A copy of the current CCD temperature, taken at the start of a multrun. 
 *                              Used to populate FITS headers. In degrees centigrade.</dd>
 * <dt>Image_Index</dt> <dd>Which frame in the multrun we are currently working on.</dd>
 * <dt>Image_Count</dt> <dd>The number of FITS images we are expecting to generate in the current multrun.</dd>
 * <dt>Multrun_Start_Time</dt> <dd>A timestamp taken the first time an exposure was started in the multrun.</dd>
 * </dl>
 */
struct Multrun_Struct
{
	double CCD_Temperature;
	int Image_Index;
	int Image_Count;
	struct timespec Multrun_Start_Time;
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
 * <dt>Image_Index</dt>                   <dd>0</dd>
 * <dt>Image_Count</dt>                   <dd>0</dd>
 * <dt>Multrun_Start_Time</dt>            <dd>{0,0}</dd>
 * </dl>
 * @see #Multrun_Struct
 */
static struct Multrun_Struct Multrun_Data =
{
	0.0,0,0,{0,0}
};

/**
 * Is a multrun in progress.
 */
static int Multrun_In_Progress = FALSE;
/**
 * Abort any multrun currently in progress.
 */
static int Moptop_Abort = FALSE;

/* internal function declarations */
static int Multrun_Fits_Headers_Set(int exposure_count,int do_standard);
static int Multrun_Exposure_Fits_Headers_Set(void);

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * Routine to perform a multrun.
 * <ul>
 * <li>We check the input arguments are valid.
 * <li>We initialise the internal variables.
 * <li>We retrieve the multrun flipping configuration fron config ("raptor.multrun.image.flip.[x|y]" and configure
 *     the detector exposure code appropriately (Detector_Exposure_Flip_Set).
 * <li>We call Detector_Fits_Filename_Next_Multrun to generate FITS filenames for a new Multrun.
 * <li>We figure out the DETECTOR_FITS_FILENAME_EXPOSURE_TYPE to use, based on the do_standard flag.
 * <li>We call Multrun_Fits_Headers_Set to make any per-multrun FITS header changes here.
 * <li>We take a multrun start timestamp.
 * <li>We enter a for loop, looping Multrun_Data.Image_Index over Multrun_Data.Image_Count.
 *     <ul>
 *     <li>We check Moptop_Abort to see if the multrun has been aborted by another command thread.
 *     <li>If the nudgematic is enabled (Raptor_Config_Nudgematic_Is_Enabled), 
 *         we move the nudgematic by calling Nudgematic_Command_Position_Set.
 *     <li>We call Detector_Fits_Filename_Next_Run to increment the run number in the FITS filename generation code.
 *     <li>We call Detector_Fits_Filename_Get_Filename to generate a suitable FITS image filename.
 *     <li>We check Moptop_Abort to see if the multrun has been aborted by another command thread.
 *     <li>We call Multrun_Exposure_Fits_Headers_Set to make any per-exposure FITS header changes here.
 *     <li>We call Detector_Exposure_Expose to take the image (a series of coadds) and save it to the FITS image filename.
 *     <li>We call Detector_Fits_Filename_List_Add to add the new FITS image filename to the return list of filenames.
 *     <li>We increment, and potentially reset the nudgematic position to use for the next exposure in the multrun.
 *     </ul>
 * <li>We set Multrun_In_Progress to FALSE, to indicate we have finished the Multrun.
 * </ul>
 * @param exposure_length_ms The exposure length of an individual frame in the multrun (itself consisting of a number
 *        of coadds) in milliseconds.
 * @param exposure_count The number of exposure to perform in the multrun.
 * @param do_standard A boolean, TRUE if the multrun is a standard, and FALSE if it is not. This effacts the resultant
 *        FITS image filenames.
 * @param filename_list The address of a list of strings, on a successful return from this routine an allocated list 
 *        of strings will be returned (of length exposure_count), each string containing a FITS image filename
 *        of one frame/exposure in the multrun. This list will need freeing.
 * @param filename_count The address of an integer, on a successful return from this routine contains the
 *        number of filenames in filename_list.
 * @return The routine returns TRUE on sucess and FALSE on failure. On failure, Raptor_General_Error_Number and
 *         Raptor_General_Error_String should be set.
 * @see #Moptop_Abort
 * @see #Multrun_In_Progress
 * @see #Multrun_Data
 * @see #Multrun_Fits_Headers_Set
 * @see #Multrun_Exposure_Fits_Headers_Set
 * @see raptor_config.html#Raptor_Config_Get_Boolean
 * @see raptor_config.html#Raptor_Config_Nudgematic_Is_Enabled
 * @see raptor_general.html#RAPTOR_GENERAL_IS_BOOLEAN
 * @see raptor_general.html#Raptor_General_Error_Number
 * @see raptor_general.html#Raptor_General_Error_String
 * @see raptor_general.html#Raptor_General_Log
 * @see raptor_general.html#Raptor_General_Log_Format
 * @see ../nudgematic/cdocs/nudgematic_command.html#NUDGEMATIC_POSITION_COUNT
 * @see ../nudgematic/cdocs/nudgematic_command.html#Nudgematic_Command_Position_Set
 * @see ../detector/cdocs/detector_exposure.html#Detector_Exposure_Flip_Set
 * @see ../detector/cdocs/detector_exposure.html#Detector_Exposure_Expose
 * @see ../detector/cdocs/detector_fits_filename.html#DETECTOR_FITS_FILENAME_PIPELINE_FLAG
 * @see ../detector/cdocs/detector_fits_filename.html#DETECTOR_FITS_FILENAME_EXPOSURE_TYPE
 * @see ../detector/cdocs/detector_fits_filename.html#Detector_Fits_Filename_Next_Multrun
 * @see ../detector/cdocs/detector_fits_filename.html#Detector_Fits_Filename_Next_Run
 * @see ../detector/cdocs/detector_fits_filename.html#Detector_Fits_Filename_Get_Filename
 * @see ../detector/cdocs/detector_fits_filename.html#Detector_Fits_Filename_List_Add
 */
int Raptor_Multrun(int exposure_length_ms,int exposure_count,int do_standard,
		   char ***filename_list,int *filename_count)
{
	char fits_filename[256];
	enum DETECTOR_FITS_FILENAME_EXPOSURE_TYPE fits_filename_exposure_type;
	int nudgematic_position_index = 0;
	int flip_x,flip_y;
	
	/* check arguments */
	if(exposure_length_ms < 1)
	{
		Raptor_General_Error_Number = 600;
		sprintf(Raptor_General_Error_String,
			"Raptor_Multrun:exposure length was too short (%d).",exposure_length_ms);
		return FALSE;
	}
	if(exposure_count < 1)
	{
		Raptor_General_Error_Number = 601;
		sprintf(Raptor_General_Error_String,
			"Raptor_Multrun:exposure count was too small (%d).",exposure_count);
		return FALSE;
	}
	if(!RAPTOR_GENERAL_IS_BOOLEAN(do_standard))
	{
		Raptor_General_Error_Number = 602;
		sprintf(Raptor_General_Error_String,
			"Raptor_Multrun:do_standard was not a valid boolean (%d).",do_standard);
		return FALSE;
	}
	if(filename_list == NULL)	
	{
		Raptor_General_Error_Number = 603;
		sprintf(Raptor_General_Error_String,"Raptor_Multrun:filename_list was NULL.");
		return FALSE;
	}
	if(filename_count == NULL)	
	{
		Raptor_General_Error_Number = 604;
		sprintf(Raptor_General_Error_String,"Raptor_Multrun:filename_count was NULL.");
		return FALSE;
	}
#if RAPTOR_DEBUG > 1
	Raptor_General_Log_Format("multrun","raptor_multrun.c","Raptor_Multrun",LOG_VERBOSITY_TERSE,
				  "MULTRUN","Started with exposure_length %d ms, exposure count %d.",
				  exposure_length_ms,exposure_count);
#endif
	/* initialise internal variables */
	Multrun_In_Progress = TRUE;
	Moptop_Abort = FALSE;
	Multrun_Data.Image_Count = exposure_count;
	nudgematic_position_index = 0;
	(*filename_list) = NULL;
	(*filename_count) = 0;
	/* configure flipping of output image */
	if(!Raptor_Config_Get_Boolean("raptor.multrun.image.flip.x",&flip_x))
		return FALSE;		
	if(!Raptor_Config_Get_Boolean("raptor.multrun.image.flip.y",&flip_y))
		return FALSE;		
	Detector_Exposure_Flip_Set(flip_x,flip_y);
	/* intialise FITS filenames for new multrun*/
	if(!Detector_Fits_Filename_Next_Multrun())
	{
		Multrun_In_Progress = FALSE;
		Raptor_General_Error_Number = 605;
		sprintf(Raptor_General_Error_String,"Raptor_Multrun:Failed to initialise FITS filename multrun.");
		return FALSE;
	}
	if(do_standard)
		fits_filename_exposure_type = DETECTOR_FITS_FILENAME_EXPOSURE_TYPE_STANDARD;
	else
		fits_filename_exposure_type = DETECTOR_FITS_FILENAME_EXPOSURE_TYPE_EXPOSURE;
	/* do any per-multrun FITS header changes here */
	if(!Multrun_Fits_Headers_Set(exposure_count,do_standard))
	{
		Multrun_In_Progress = FALSE;
		return FALSE;
	}
	/* take a multrun start timestamp */
	clock_gettime(CLOCK_REALTIME,&(Multrun_Data.Multrun_Start_Time));
	/* start multrun for loop */
	for(Multrun_Data.Image_Index = 0; Multrun_Data.Image_Index < Multrun_Data.Image_Count;
	    Multrun_Data.Image_Index++)
	{
		/* check for aborts */
		if(Moptop_Abort)
		{
			Multrun_In_Progress = FALSE;
			Raptor_General_Error_Number = 606;
			sprintf(Raptor_General_Error_String,"Raptor_Multrun:Aborted.");
			return FALSE;
		}
		/* move to next nudgematic position */
		if(Raptor_Config_Nudgematic_Is_Enabled())
		{
			if(!Nudgematic_Command_Position_Set(nudgematic_position_index))
			{
				Multrun_In_Progress = FALSE;
				Raptor_General_Error_Number = 607;
				sprintf(Raptor_General_Error_String,
					"Raptor_Multrun:Failed to move Nudgematic to position %d.",
					nudgematic_position_index);
				return FALSE;
			}
		}
		/* generate new FITS image filename */
		if(!Detector_Fits_Filename_Next_Run())
		{
			Multrun_In_Progress = FALSE;
			Raptor_General_Error_Number = 608;
			sprintf(Raptor_General_Error_String,"Raptor_Multrun:Failed to generate next FITS filename run number.");
			return FALSE;
		}
		if(!Detector_Fits_Filename_Get_Filename(fits_filename_exposure_type,
							DETECTOR_FITS_FILENAME_PIPELINE_FLAG_UNREDUCED,
							fits_filename,256))
		{
			Multrun_In_Progress = FALSE;
			Raptor_General_Error_Number = 609;
			sprintf(Raptor_General_Error_String,"Raptor_Multrun:Failed to generate next FITS filename.");
			return FALSE;
		}
		/* check for aborts */
		if(Moptop_Abort)
		{
			Multrun_In_Progress = FALSE;
			Raptor_General_Error_Number = 610;
			sprintf(Raptor_General_Error_String,"Raptor_Multrun:Aborted.");
			return FALSE;
		}
		/* do any per-multrun frame FITS header changes here */
		if(!Multrun_Exposure_Fits_Headers_Set())
		{
			Multrun_In_Progress = FALSE;
			return FALSE;
		}
		/* take an exposure */
		if(!Detector_Exposure_Expose(exposure_length_ms,fits_filename))
		{
			Multrun_In_Progress = FALSE;
			Raptor_General_Error_Number = 611;
			sprintf(Raptor_General_Error_String,
				"Raptor_Multrun:Failed to take exposure %d of %d ms with filename '%s'.",
				Multrun_Data.Image_Index,exposure_length_ms,fits_filename);
			return FALSE;
		}
		/* add fits image to list */
		if(!Detector_Fits_Filename_List_Add(fits_filename,filename_list,filename_count))
		{
			Multrun_In_Progress = FALSE;
			Raptor_General_Error_Number = 612;
			sprintf(Raptor_General_Error_String,"Raptor_Multrun:Failed to add filename '%s' to list of length %d.",
				fits_filename,(*filename_count));
			return FALSE;
		}
		/* increment nudgematic position index */
		nudgematic_position_index++;
		if(nudgematic_position_index == NUDGEMATIC_POSITION_COUNT)
			nudgematic_position_index = 0;
	}/* end for on Multrun_Data.Image_Index */
	/* we have finished the multrun */
	Multrun_In_Progress = FALSE;
#if RAPTOR_DEBUG > 1
	Raptor_General_Log("multrun","raptor_multrun.c","Raptor_Multrun",LOG_VERBOSITY_TERSE,"MULTRUN",
			   "Finished.");
#endif		
	return TRUE;
}

/**
 * Routine to abort an in-progress multrun.
 * @return The routine nominally returns TRUE on success and FALSE on failure. 
 *         However, it currently always succeeds (returns TRUE).
 * @see #Moptop_Abort
 */
int Raptor_Multrun_Abort(void)
{
	Moptop_Abort = TRUE;
	return TRUE;
}

/**
 * Return whether we are currently performing a multrun or not.
 * @return A boolean, TRUE if a multrun is in progress and FALSE if it is not in progress..
 * @see #Multrun_In_Progress
 */
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

/**
 * Return which exposure in the multrun we are on.
 * @return The exposure index in the multrun.
 * @see #Multrun_Data
 */
int Raptor_Multrun_Exposure_Index_Get(void)
{
	return Multrun_Data.Image_Index;
}

/* ----------------------------------------------------------------------------
** 		internal functions 
** ---------------------------------------------------------------------------- */

/**
 * Routine to collect and insert FITS headers pertaining to the whole multrun.
 * @param exposure_count The number of exposure to perform in the multrun.
 * @param do_standard A boolean, TRUE if the multrun is a standard, and FALSE if it is not. 
 * @return The routine returns TRUE on sucess and FALSE on failure. On failure, Raptor_General_Error_Number and
 *         Raptor_General_Error_String should be set.
 * @see #CENTIGRADE_TO_KELVIN
 * @see #Multrun_Data
 * @see raptor_config.html#Raptor_Config_Filter_Wheel_Is_Enabled
 * @see raptor_fits_header.html#Raptor_Fits_Header_Integer_Add
 * @see raptor_fits_header.html#Raptor_Fits_Header_Logical_Add
 * @see raptor_fits_header.html#Raptor_Fits_Header_String_Add
 * @see raptor_general.html#RAPTOR_GENERAL_IS_BOOLEAN
 * @see raptor_general.html#Raptor_General_Error_Number
 * @see raptor_general.html#Raptor_General_Error_String
 * @see raptor_general.html#Raptor_General_Log
 * @see raptor_general.html#Raptor_General_Log_Format
 * @see ../detector/cdocs/detector_fits_filename.html#Detector_Fits_Filename_Multrun_Get
 * @see ../detector/cdocs/detector_setup.html#Detector_Setup_Get_Sensor_Size_X
 * @see ../detector/cdocs/detector_setup.html#Detector_Setup_Get_Sensor_Size_Y
 * @see ../filter_wheel/cdocs/filter_wheel_command.html#Filter_Wheel_Command_Get_Position
 * @see ../filter_wheel/cdocs/filter_wheel_config.html#Filter_Wheel_Config_Position_To_Name
 * @see ../filter_wheel/cdocs/filter_wheel_config.html#Filter_Wheel_Config_Position_To_Id
 */
static int Multrun_Fits_Headers_Set(int exposure_count,int do_standard)
{
	char filter_name_string[32];
	double temperature;
	int filter_wheel_position,retval;
		
	if(exposure_count < 1)
	{
		Raptor_General_Error_Number = 613;
		sprintf(Raptor_General_Error_String,
			"Multrun_Fits_Headers_Set:exposure count was too small (%d).",exposure_count);
		return FALSE;
	}
	if(!RAPTOR_GENERAL_IS_BOOLEAN(do_standard))
	{
		Raptor_General_Error_Number = 614;
		sprintf(Raptor_General_Error_String,
			"Multrun_Fits_Headers_Set:do_standard was not a valid boolean (%d).",do_standard);
		return FALSE;
	}
	/* OBSTYPE */
	if(do_standard)
		retval = Raptor_Fits_Header_String_Add("OBSTYPE","STANDARD",NULL);
	else
		retval = Raptor_Fits_Header_String_Add("OBSTYPE","EXPOSE",NULL);
	if(retval == FALSE)
		return FALSE;
	/* filter wheel position keywords */
	if(Raptor_Config_Filter_Wheel_Is_Enabled())
	{
		if(!Filter_Wheel_Command_Get_Position(&filter_wheel_position))
		{
			Raptor_General_Error_Number = 615;
			sprintf(Raptor_General_Error_String,"Multrun_Fits_Headers_Set:"
					"Failed to get filter wheel position.");
			return FALSE;
			
		}
		/* FILTER1 */
		if(!Filter_Wheel_Config_Position_To_Name(filter_wheel_position,filter_name_string))
		{
			Raptor_General_Error_Number = 616;
			sprintf(Raptor_General_Error_String,"Multrun_Fits_Headers_Set:"
				"Failed to get filter wheel name from position %d.",filter_wheel_position);
			return FALSE;
		}
		if(!Raptor_Fits_Header_String_Add("FILTER1",filter_name_string,NULL))
			return FALSE;
		/* FILTERI1 */
		if(!Filter_Wheel_Config_Position_To_Id(filter_wheel_position,filter_name_string))
		{
			Raptor_General_Error_Number = 617;
			sprintf(Raptor_General_Error_String,"Multrun_Fits_Headers_Set:"
				"Failed to get filter wheel Id from position %d.",filter_wheel_position);
			return FALSE;
		}
		if(!Raptor_Fits_Header_String_Add("FILTERI1",filter_name_string,NULL))
			return FALSE;
	}
	else
	{
		/* FILTER1 */
		if(!Raptor_Fits_Header_String_Add("FILTER1","UNKNOWN",NULL))
			return FALSE;
		/* FILTERI1 */
		if(!Raptor_Fits_Header_String_Add("FILTERI1","UNKNOWN",NULL))
			return FALSE;
	}
	/* RUNNUM */
	if(!Raptor_Fits_Header_Integer_Add("RUNNUM",Detector_Fits_Filename_Multrun_Get(),"Number of Multrun"))
		return FALSE;
	/* EXPTOTAL */
	if(!Raptor_Fits_Header_Integer_Add("EXPTOTAL",Multrun_Data.Image_Count,
					   "Total number of exposures within Multrun"))
		return FALSE;
	/* CONFIGID diddly TODO in Java layer */
	/* CONFNAME diddly TODO in Java layer */
	/* CCDSTEMP */
	if(!Detector_Temperature_Get_TEC_Setpoint(&temperature))
	{
		Raptor_General_Error_Number = 618;
		sprintf(Raptor_General_Error_String,"Multrun_Fits_Headers_Set:Failed to get TEC set-point.");
		return FALSE;
	}
	if(!Raptor_Fits_Header_Float_Add("CCDSTEMP",temperature+CENTIGRADE_TO_KELVIN,"[Kelvin] Required temperature."))
		return FALSE;	
	/* CCDATEMP */
	if(!Detector_Temperature_Get(&(Multrun_Data.CCD_Temperature)))
	{
		Raptor_General_Error_Number = 619;
		sprintf(Raptor_General_Error_String,"Multrun_Fits_Headers_Set:Failed to get detector temperature.");
		return FALSE;
	}
	if(!Raptor_Fits_Header_Float_Add("CCDATEMP",Multrun_Data.CCD_Temperature+CENTIGRADE_TO_KELVIN,
					 "[Kelvin] Actual temperature."))
		return FALSE;	
	/* DETECTOR diddly TODO in Java layer */
	/* CCDXBIN */
	if(!Raptor_Fits_Header_Integer_Add("CCDXBIN",1,"X binning factor"))
		return FALSE;
	/* CCDYBIN */
	if(!Raptor_Fits_Header_Integer_Add("CCDYBIN",1,"Y binning factor"))
		return FALSE;
	/* CCDWMODE */
	if(!Raptor_Fits_Header_Logical_Add("CCDWMODE",FALSE,"Using a Window (always false for Raptor)"))
		return FALSE;
	/* CCDXIMSI */
	if(!Raptor_Fits_Header_Integer_Add("CCDXIMSI",Detector_Setup_Get_Sensor_Size_X(),"[pixels] X image size"))
		return FALSE;
	/* CCDYIMSI */
	if(!Raptor_Fits_Header_Integer_Add("CCDYIMSI",Detector_Setup_Get_Sensor_Size_Y(),"[pixels] Y image size"))
		return FALSE;
	/* CCDWXOFF */
	if(!Raptor_Fits_Header_Integer_Add("CCDWXOFF",0,"[pixels] X window offset"))
		return FALSE;
	/* CCDWYOFF */
	if(!Raptor_Fits_Header_Integer_Add("CCDWYOFF",0,"[pixels] Y window offset"))
		return FALSE;
	/* CCDWXSIZ */
	if(!Raptor_Fits_Header_Integer_Add("CCDWXSIZ",Detector_Setup_Get_Sensor_Size_X(),"[pixels] X window size"))
		return FALSE;
	/* CCDWYSIZ */
	if(!Raptor_Fits_Header_Integer_Add("CCDWYSIZ",Detector_Setup_Get_Sensor_Size_Y(),"[pixels] Y window size"))
		return FALSE;
	return TRUE;
}

/**
 * Routine to collect and insert FITS headers pertaining to the current exposure in the multrun.
 * @return The routine returns TRUE on sucess and FALSE on failure. On failure, Raptor_General_Error_Number and
 *         Raptor_General_Error_String should be set.
 * @see raptor_fits_header.html#Raptor_Fits_Header_Integer_Add
 * @see raptor_general.html#Raptor_General_Error_Number
 * @see raptor_general.html#Raptor_General_Error_String
 * @see raptor_general.html#Raptor_General_Log
 * @see raptor_general.html#Raptor_General_Log_Format
 * @see ../detector/cdocs/detector_fits_filename.html#Detector_Fits_Filename_Run_Get
 */
static int Multrun_Exposure_Fits_Headers_Set(void)
{
	/* EXPNUM */
	if(!Raptor_Fits_Header_Integer_Add("EXPNUM",Detector_Fits_Filename_Run_Get(),"Number of exposure within Multrun"))
		return FALSE;
	return TRUE;
}
