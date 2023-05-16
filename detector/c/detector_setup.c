/* detector_setup.c
** Raptor Ninox-640 Infrared detector library : setup routines.
*/
/**
 * Routines to look after setting up the Raptor Ninox-640 Infrared detector for acquisition.
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
#include "detector_serial.h"
#include "detector_setup.h"
#include "detector_temperature.h"
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
 * <li>Is_Open</dt> <dd>Boolean, determines whether a connection to the detector has been previously successfully been opened.
 * <dt>Size_X</dt> <dd>The size of the frame grabber image in the X direction, in pixels.</dd>
 * <dt>Size_Y</dt> <dd>The size of the frame grabber image in the Y direction, in pixels.</dd>
 * </dl>
 */
struct Setup_Struct
{
	int Is_Open;
	int Size_X;
	int Size_Y;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of Setup_Struct that contains local data for this module. This is initialised as follows:
 * <dl>
 * <li>Is_Open</dt> <dd>FALSE</dd>
 * <dt>Size_X</dt> <dd>0</dd>
 * <dt>Size_Y</dt> <dd>0</dd>
 * </dl>
 */
static struct Setup_Struct Setup_Data = 
{
	FALSE,0,0
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
static int Setup_Get_Dimensions(int *x_size,int *y_size);

/* --------------------------------------------------------
** External Functions
** -------------------------------------------------------- */
/**
 * Routine to initialise the Raptor detector.
 * <ul>
 * <li>Initialise turn_fan_on to TRUE.
 * <li>If the connection to the library has been previously opened (Setup_Data.Is_Open): 
 *     <ul>
 *     <li>We retrieve the current fpga status byte using Detector_Serial_Command_Get_FPGA_Status.
 *     <li>We extract whether the fan is currently turned on or off 
 *         (is the DETECTOR_SERIAL_FPGA_CTRL_FAN_ENABLED bit set?), and set turn_fan_on accordingly.
 *     <li>We close the connection to the library by calling Detector_Setup_Shutdown.
 *     </ul>
 * <li>Detector_Setup_Open is called with the specified format_file.
 * <li>We log some information from the frame grabber by calling pxd_infoMemsize, pxd_imageZdim, pxd_infoUnits.
 * <li>We call Setup_Get_Dimensions to get the frame grabbers image dimensions from the frame grabber. We
 *     store the returned image dimensions in the setup data (Size_X/Size_Y).
 * <li>We log some more information from the frame grabber by calling pxd_imageCdim / pxd_imageBdim.
 * <li>We initialise the detector library's buffers by calling Detector_Buffer_Allocate. Detector_Buffer_Allocate is written such that
 *     the buffers will only be freed/reallocated, if the size dimensions have changed (or Detector_Buffer_Allocate has not been called before).
 * <li>We initialise the internal serial link to the detector by calling Detector_Serial_Initialise.
 * <li>Setup_Data.Is_Open is set to TRUE as the connection to the detector is now open.
 * <li>We use the previously initialsed / extracted fan status to turn the fan off if necessary by calling 
 *     Detector_Temperature_Set_Fan with turn_fan_on. Detector_Setup_Open reinitialises the fan to on 
 *     (with the format files we are currently using), and this call allows us to retain the fan status (on or off) 
 *     over a call to Detector_Setup_Startup.
 * </ul>
 * @param formatfile The filename of a '.fmt' format file, used to configure the video mode of the detector.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Setup_Error_Number/Setup_Error_String are set.
 * @see #UNITSMAP
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Setup_Data
 * @see #Detector_Setup_Open
 * @see #Detector_Setup_Shutdown
 * @see #Setup_Get_Dimensions
 * @see detector_serial.html#DETECTOR_SERIAL_FPGA_CTRL_FAN_ENABLED
 * @see detector_buffer.html#Detector_Buffer_Allocate
 * @see detector_serial.html#Detector_Serial_Initialise
 * @see detector_serial.html#Detector_Serial_Command_Get_FPGA_Status
 * @see detector_temperature.html#Detector_Temperature_Set_Fan
 * @see detector_general.html#Detector_General_Log
 * @see detector_general.html#Detector_General_Log_Format
 */
int Detector_Setup_Startup(char *format_filename)
{
	unsigned char fpga_status;
	int retval,turn_fan_on;
	
	Setup_Error_Number = 0;
	/* default to turning the fan on (by default an open on out format files will do this) */
	turn_fan_on = TRUE;
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
	/* if a connection to the library is already open, close it */
	if(Setup_Data.Is_Open)
	{
		/* get current fpga status and extract current fan setting. */
		if(Detector_Serial_Command_Get_FPGA_Status(&fpga_status))
		{
			turn_fan_on = ((fpga_status & DETECTOR_SERIAL_FPGA_CTRL_FAN_ENABLED) > 0);
#if LOGGING > 1
			Detector_General_Log_Format(LOG_VERBOSITY_VERBOSE,
						    "Detector_Setup_Startup:FPGA Status was %#02x,turn_fan_on = %d.",
						    fpga_status,turn_fan_on);
#endif
		}
		else 
		{
			/* if getting the fpga status failed, log it and try and continue anyway. */
			Detector_General_Error();
			turn_fan_on = TRUE;
		}
		/* shutdown connection */
#if LOGGING > 1
		Detector_General_Log(LOG_VERBOSITY_VERBOSE,
				     "Detector_Setup_Startup:Shutdown previously opened connection.");
#endif
		if(!Detector_Setup_Shutdown())
		{
			/* if the shutdown failed, log it and try and continue anyway. */
			Detector_General_Error();
		}
	}
	/* open connection to the library */
	if(!Detector_Setup_Open("","",format_filename))
	{
		/* Setup_Error_Number / Setup_Error_String set in Detector_Setup_Open */
		return FALSE;
	}
	/* get some information from the frame grabber */
#if LOGGING > 1
	Detector_General_Log_Format(LOG_VERBOSITY_VERBOSE,"Detector_Setup_Startup:Frame buffer memory size %ld bytes.",
				    pxd_infoMemsize(UNITSMAP));
	Detector_General_Log_Format(LOG_VERBOSITY_VERBOSE,"Detector_Setup_Startup:Image frame buffers: %d.",
				    pxd_imageZdim());
	Detector_General_Log_Format(LOG_VERBOSITY_VERBOSE,"Detector_Setup_Startup:Number of boards: %d.",
				    pxd_infoUnits());
#endif
	/* get image dimensions */
	if(!Setup_Get_Dimensions(&(Setup_Data.Size_X),&(Setup_Data.Size_Y)))
	{
		/* Setup_Error_Number / Setup_Error_String set in Setup_Get_Dimensions */
		return FALSE;
	}
	/* more information from the frame grabber logged */
#if LOGGING > 1
	Detector_General_Log_Format(LOG_VERBOSITY_VERBOSE,"Detector_Setup_Startup:Image dimensions (x=%d,y=%d).",
				    Setup_Data.Size_X,Setup_Data.Size_Y);
	Detector_General_Log_Format(LOG_VERBOSITY_VERBOSE,"Detector_Setup_Startup:Colours = %d.",
				    pxd_imageCdim());
	Detector_General_Log_Format(LOG_VERBOSITY_VERBOSE,"Detector_Setup_Startup:Bits per pixel = %d.",
				    pxd_imageCdim()*pxd_imageBdim());
#endif
	/* allocate image buffers. Note this now automatically frees and re-allocates buffers, only if the sizes have changed,
	** if the sizes are the same and the buffers are non-null nothing is changed. */
	if(!Detector_Buffer_Allocate(Setup_Data.Size_X,Setup_Data.Size_Y))
	{
		Setup_Error_Number = 8;
		sprintf(Setup_Error_String,
			"Detector_Setup_Startup:Detector_Buffer_Allocate(size_x = %d,size_y = %d) failed.",
			Setup_Data.Size_X,Setup_Data.Size_Y);
		return FALSE;
	}
	/* open connection to and initialise the internal serial link */
	if(!Detector_Serial_Initialise())
	{
		Setup_Error_Number = 9;
		sprintf(Setup_Error_String,"Detector_Setup_Startup:Detector_Serial_Initialise failed.");
		return FALSE;
	}
	/* we have now finished initialing the detector */
	Setup_Data.Is_Open = TRUE;
	/* Detector_Setup_Open will have turned the fan back on (for our format files).
	** If it was previously turned off, turn it off again */
	if(!Detector_Temperature_Set_Fan(turn_fan_on))
	{
		/* if reseting the fan failed, log it and try and continue anyway. */
		Detector_General_Error();
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
 * <li>Setup_Data.Is_Open is set to FALSE as the connection to the detector is closed.
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Setup_Error_Number/Setup_Error_String are set.
 * @see #Setup_Data
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 * @see #Detector_Setup_Close
 * @see detector_general.html#Detector_General_Log
 * @see detector_general.html#Detector_General_Log_Format
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
	/* we have now finished closing the connection to the detector */
	Setup_Data.Is_Open = FALSE;
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
 * @see detector_general.html#Detector_General_Log
 * @see detector_general.html#Detector_General_Log_Format
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
		sprintf(Setup_Error_String,"Detector_Setup_Open:pxd_PIXCIopen(formatfile='%s') failed: %s (%d).",formatfile,
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
 * @see detector_general.html#Detector_General_Log
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
 * Get the frame grabber's image data size in x, in pixels. Detector_Setup_Startup must have previously been called
 * to open a connection to the frame grabber, and retrieve the detector's image dimensions.
 * @return An integer, the image data size in x, in pixels, from Setup_Data.Size_X.
 * @see #Setup_Data
 * @see #Detector_Setup_Startup
 */
int Detector_Setup_Get_Sensor_Size_X(void)
{
	return Setup_Data.Size_X;
}

/**
 * Get the frame grabber's image data size in y, in pixels. Detector_Setup_Startup must have previously been called
 * to open a connection to the frame grabber, and retrieve the detector's image dimensions.
 * @return An integer, the image data size in y, in pixels, from Setup_Data.Size_Y.
 * @see #Setup_Data
 * @see #Detector_Setup_Startup
 */
int Detector_Setup_Get_Sensor_Size_Y(void)
{
	return Setup_Data.Size_Y;
}

/**
 * Get the frame grabber's image data size in pixels. Detector_Setup_Startup must have previously been called
 * to open a connection to the frame grabber, and retrieve the detector's image dimensions.
 * @return An integer, the image data size in pixels.
 * @see #Detector_Setup_Get_Sensor_Size_X
 * @see #Detector_Setup_Get_Sensor_Size_Y
 * @see #Detector_Setup_Startup
 */
int Detector_Setup_Get_Image_Size_Pixels(void)
{
	return Detector_Setup_Get_Sensor_Size_X()*Detector_Setup_Get_Sensor_Size_Y();
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
/**
 * Get the frame grabber image dimensions from the frame grabber. A connection to the frame grabber must have 
 * previously been opened. pxd_imageXdim and pxd_imageYdim are used to retrieve the size of the image.
 * @param x_size The address of an integer to store the returned x size of the image.
 * @param y_size The address of an integer to store the returned y size of the image.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Setup_Error_Number/Setup_Error_String are set.
 * @see #Setup_Error_Number
 * @see #Setup_Error_String
 */
static int Setup_Get_Dimensions(int *x_size,int *y_size)
{
	Setup_Error_Number = 0;
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_VERBOSE,"Setup_Get_Dimensions:Started.");
#endif
	if(x_size == NULL)
	{
		Setup_Error_Number = 4;
		sprintf(Setup_Error_String,"Setup_Get_Dimensions:x_size was NULL.");
		return FALSE;
	}
	if(y_size == NULL)
	{
		Setup_Error_Number = 5;
		sprintf(Setup_Error_String,"Setup_Get_Dimensions:y_size was NULL.");
		return FALSE;
	}
	(*x_size) = pxd_imageXdim();
	if((*x_size) == 0) /* library has not been opened */
	{
		Setup_Error_Number = 6;
		sprintf(Setup_Error_String,"Setup_Get_Dimensions:x_size was 0:library not open.");
		return FALSE;
	}
	(*y_size) = pxd_imageYdim();
	if((*y_size) == 0) /* library has not been opened */
	{
		Setup_Error_Number = 7;
		sprintf(Setup_Error_String,"Setup_Get_Dimensions:y_size was 0:library not open.");
		return FALSE;
	}
#if LOGGING > 1
	Detector_General_Log_Format(LOG_VERBOSITY_VERBOSE,"Setup_Get_Dimensions:x_size = %d, y_size = %d.",
				    (*x_size),(*y_size));
	Detector_General_Log(LOG_VERBOSITY_VERBOSE,"Setup_Get_Dimensions:Finished.");
#endif
	return TRUE;
}
