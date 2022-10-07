/* detector_buffer.c
** Raptor Ninox-640 Infrared detector library : image buffer handling routines.
*/
/**
 * Routines to look after the image buffers used when reading out from the Raptor Ninox-640 Infrared detector.
 * @author Chris Mottram
 * @version $Revision$
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log_udp.h"
#include "detector_buffer.h"
#include "detector_general.h"

/* data types */
/**
 * Data type holding local data to detector_buffer. This consists of the following:
 * <dl>
 * <dt>Size_X</dt> <dd>The size of the buffer image in the X direction, in pixels.</dd>
 * <dt>Size_Y</dt> <dd>The size of the buffer image in the Y direction, in pixels.</dd>
 * <dt>Mono_Image</dt> <dd>A pointer to an allocated block of unsigned short memory,
 *                     of size Size_X * Size_Y * sizeof(unsigned short) bytes. 
 *                     Used for storing an individual readout from the detector.</dd>
 * <dt>Coadd_Image</dt> <dd>A pointer to an allocated block of integer memory,
 *                      of size Size_X * Size_Y * sizeof(int) bytes.
 *                      Used for storing a number of individual readouts added together.</dd>
 * <dt>Mean_Image</dt> <dd>A pointer to an allocated block of double floating point memory,
 *                     of size Size_X * Size_Y * sizeof(double) bytes.
 *                     Used for storing the arithmetic mean of the coadds.</dd>
 * </dl>
 */
struct Buffer_Struct
{
	int Size_X;
	int Size_Y;
	unsigned short *Mono_Image;
	int *Coadd_Image;
	double *Mean_Image;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of Buffer_Struct that contains local data for this module. This is initialised as follows:
 * <dl>
 * <dt>Size_X</dt> <dd>0</dd>
 * <dt>Size_Y</dt> <dd>0</dd>
 * <dt>Mono_Image</dt> <dd>NULL</dd>
 * <dt>Coadd_Image</dt> <dd>NULL</dd>
 * <dt>Mean_Image</dt> <dd>NULL</dd>
 * </dl>
 */
static struct Buffer_Struct Buffer_Data = 
{
	0,0,NULL,NULL,NULL
};

/**
 * Variable holding error code of last operation performed.
 */
static int Buffer_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see detector_general.html#DETECTOR_GENERAL_ERROR_STRING_LENGTH
 */
static char Buffer_Error_String[DETECTOR_GENERAL_ERROR_STRING_LENGTH] = "";
/* --------------------------------------------------------
** External Functions
** -------------------------------------------------------- */
/**
 * Create the image buffers to the specified dimensions. 
 * <ul>
 * <li>We check if the new size is the same as the old size, and all the buffers are already allocated (non NULL)
 *     and if so make no changes and return success.
 * <li>We call Detector_Buffer_Free to ensure any previous memory allocations are freed correctly.
 * <li>We allocate new buffers, using size_x and size_y to determine the buffer size (in pixels).
 * </ul>
 * @param size_x The X size of the image, in pixels (should be greater than 0).
 * @param size_y The Y size of the image, in pixels (should be greater than 0).
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Buffer_Error_Number/Buffer_Error_String are set.
 * @see #Buffer_Data
 * @see #Buffer_Error_Number
 * @see #Buffer_Error_String
 * @see #Detector_Buffer_Free
 * @see detector_general.html#Detector_General_Log
 * @see detector_general.html#Detector_General_Log_Format
 */
int Detector_Buffer_Allocate(int size_x,int size_y)
{
	int retval;
	
	Buffer_Error_Number = 0;
#if LOGGING > 1
	Detector_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				    "Detector_Buffer_Allocate(size_x = %d,size_y = %d):Started.",size_x,size_y);
#endif
	if(size_x <= 0)
	{
		Buffer_Error_Number = 1;
		sprintf(Buffer_Error_String,"Detector_Buffer_Allocate:size_x too small (%d).",size_x);
		return FALSE;
	}
	if(size_y <= 0)
	{
		Buffer_Error_Number = 2;
		sprintf(Buffer_Error_String,"Detector_Buffer_Allocate:size_y too small (%d).",size_y);
		return FALSE;
	}
	/* check - if the new size is the same as the old size, and all buffers are already allocated, 
	** we don't need to do anything */
	if((Buffer_Data.Size_X == size_x)&&(Buffer_Data.Size_Y == size_y)&&(Buffer_Data.Mono_Image != NULL)&&
	   (Buffer_Data.Coadd_Image != NULL)&&(Buffer_Data.Mean_Image != NULL))
	{
#if LOGGING > 1
		Detector_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
					    "Detector_Buffer_Allocate:New size is identical to the old size (%d,%d) "
					    "and buffers are already allocated.",size_x,size_y);
#endif
		return TRUE;
	}
	/* free any previously allocated data */
	if(!Detector_Buffer_Free())
		return FALSE;
	/* save dimensions */
	Buffer_Data.Size_X = size_x;
	Buffer_Data.Size_Y = size_y;
	/* allocate mono image */
	Buffer_Data.Mono_Image = (unsigned short *)malloc(Buffer_Data.Size_X*Buffer_Data.Size_Y*sizeof(unsigned short));
	if(Buffer_Data.Mono_Image == NULL)
	{
		Buffer_Error_Number = 3;
		sprintf(Buffer_Error_String,"Detector_Buffer_Allocate:Failed to allocate Mono_Image (%d,%d).",
			Buffer_Data.Size_X,Buffer_Data.Size_Y);
		return FALSE;
	}
	/* allocate coadd image */
	Buffer_Data.Coadd_Image = (int *)malloc(Buffer_Data.Size_X*Buffer_Data.Size_Y*sizeof(int));
	if(Buffer_Data.Coadd_Image == NULL)
	{
		free(Buffer_Data.Mono_Image);
		Buffer_Data.Mono_Image = NULL;
		Buffer_Error_Number = 4;
		sprintf(Buffer_Error_String,"Detector_Buffer_Allocate:Failed to allocate Coadd_Image (%d,%d).",
			Buffer_Data.Size_X,Buffer_Data.Size_Y);
		return FALSE;
	}
	/* allocate mean image */
	Buffer_Data.Mean_Image = (double *)malloc(Buffer_Data.Size_X*Buffer_Data.Size_Y*sizeof(double));
	if(Buffer_Data.Mean_Image == NULL)
	{
		free(Buffer_Data.Mono_Image);
		Buffer_Data.Mono_Image = NULL;
		free(Buffer_Data.Coadd_Image);
		Buffer_Data.Coadd_Image = NULL;
		Buffer_Error_Number = 5;
		sprintf(Buffer_Error_String,"Detector_Buffer_Allocate:Failed to allocate Mean_Image (%d,%d).",
			Buffer_Data.Size_X,Buffer_Data.Size_Y);
		return FALSE;
	}
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Buffer_Allocate:Finished.");
#endif
	return TRUE;
}

/**
 * Free an previously allocated image buffer memory.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Buffer_Error_Number/Buffer_Error_String are set.
 * @see #Buffer_Data
 * @see #Buffer_Error_Number
 * @see #Buffer_Error_String
 * @see detector_general.html#Detector_General_Log
 */
int Detector_Buffer_Free(void)
{
	Buffer_Error_Number = 0;
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Buffer_Free:Started.");
#endif
	/* mono image */
	if(Buffer_Data.Mono_Image != NULL)
		free(Buffer_Data.Mono_Image);
	Buffer_Data.Mono_Image = NULL;
	/* coadd image */
	if(Buffer_Data.Coadd_Image != NULL)
		free(Buffer_Data.Coadd_Image);
	Buffer_Data.Coadd_Image = NULL;
	/* mean image */
	if(Buffer_Data.Mean_Image != NULL)
		free(Buffer_Data.Mean_Image);
	Buffer_Data.Mean_Image = NULL;
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Buffer_Free:Finished.");
#endif
	return TRUE;
}

/**
 * Initialise the coadd image buffer pixels to 0.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Buffer_Error_Number/Buffer_Error_String are set.
 * @see #Buffer_Data
 * @see #Buffer_Error_Number
 * @see #Buffer_Error_String
 * @see detector_general.html#Detector_General_Log
 */
int Detector_Buffer_Initialise_Coadd_Image(void)
{
	int i,pixel_count;
	
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Buffer_Initialise_Coadd_Image:Started.");
#endif
	if(Buffer_Data.Coadd_Image == NULL)
	{
		Buffer_Error_Number = 6;
		sprintf(Buffer_Error_String,"Detector_Buffer_Initialise_Coadd_Image:Coadd Image was NULL.");
		return FALSE;
	}
	pixel_count = Buffer_Data.Size_X*Buffer_Data.Size_Y;
	for(i=0; i < pixel_count; i++)
	{
		Buffer_Data.Coadd_Image[i] = 0;
	}
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Buffer_Initialise_Coadd_Image:Finished.");
#endif
	return TRUE;
}

/**
 * Routine to add the current pixel values in the mono image to the current pixel values in the coadd image, 
 * increasing the pixels values in the coadd image appropriately.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Buffer_Error_Number/Buffer_Error_String are set.
 * @see #Buffer_Data
 * @see #Buffer_Error_Number
 * @see #Buffer_Error_String
 * @see detector_general.html#Detector_General_Log
 */
int Detector_Buffer_Add_Mono_To_Coadd_Image(void)
{
	int i,pixel_count;
	
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Buffer_Add_Mono_To_Coadd_Image:Started.");
#endif
	if(Buffer_Data.Mono_Image == NULL)
	{
		Buffer_Error_Number = 7;
		sprintf(Buffer_Error_String,"Detector_Buffer_Add_Mono_To_Coadd_Image:Mono Image was NULL.");
		return FALSE;
	}
	if(Buffer_Data.Coadd_Image == NULL)
	{
		Buffer_Error_Number = 8;
		sprintf(Buffer_Error_String,"Detector_Buffer_Add_Mono_To_Coadd_Image:Coadd Image was NULL.");
		return FALSE;
	}
	pixel_count = Buffer_Data.Size_X*Buffer_Data.Size_Y;
	for(i=0; i < pixel_count; i++)
	{
		Buffer_Data.Coadd_Image[i] += Buffer_Data.Mono_Image[i];
	}
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Buffer_Add_Mono_To_Coadd_Image:Finished.");
#endif
	return TRUE;
}

/**
 * Flip the coadd image data in the X direction.
 * @see #Buffer_Data
 * @see #Buffer_Error_Number
 * @see #Buffer_Error_String
 * @see detector_general.html#Detector_General_Log
 */
void Detector_Buffer_Coadd_Flip_X(void)
{
	int x,y;
        int tempval;

#if LOGGING > 5
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,
			     "Detector_Buffer_Coadd_Flip_X:Started flipping coadd image in X.");
#endif
	if(Buffer_Data.Coadd_Image == NULL)
	{
		Buffer_Error_Number = 12;
		sprintf(Buffer_Error_String,"Detector_Buffer_Coadd_Flip_X:Coadd Image was NULL.");
		Detector_General_Error();
		return;
	}
	/* for each row */
	for(y=0;y<Buffer_Data.Size_Y;y++)
	{
		/* for the first half of the columns.
		** Note the middle column will be missed, this is OK as it
		** does not need to be flipped if it is in the middle */
		for(x=0;x<(Buffer_Data.Size_X/2);x++)
		{
			/* Copy Buffer_Data.Coadd_Image[x,y] to tempval */
			tempval = *(Buffer_Data.Coadd_Image+(y*Buffer_Data.Size_X)+x);
			/* Copy Buffer_Data.Coadd_Image[Buffer_Data.Size_X-(x+1),y] to Buffer_Data.Coadd_Image[x,y] */
			*(Buffer_Data.Coadd_Image+(y*Buffer_Data.Size_X)+x) = *(Buffer_Data.Coadd_Image+
			       (y*Buffer_Data.Size_X)+(Buffer_Data.Size_X-(x+1)));
			/* Copy tempval to Buffer_Data.Coadd_Image[Buffer_Data.Size_X-(x+1),y] */
			*(Buffer_Data.Coadd_Image+(y*Buffer_Data.Size_X)+(Buffer_Data.Size_X-(x+1))) = tempval;
		}
	}
#if LOGGING > 5
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Buffer_Coadd_Flip_X:Finished.");
#endif
}

/**
 * Flip the coadd image data in the Y direction.
 * @see #Buffer_Data
 * @see #Buffer_Error_Number
 * @see #Buffer_Error_String
 * @see detector_general.html#Detector_General_Log
 */
void Detector_Buffer_Coadd_Flip_Y(void)
{
	int x,y;
        int tempval;

#if LOGGING > 5
	Detector_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				    "Detector_Buffer_Coadd_Flip_Y:Started flipping coadd image in Y.");
#endif
	if(Buffer_Data.Coadd_Image == NULL)
	{
		Buffer_Error_Number = 13;
		sprintf(Buffer_Error_String,"Detector_Buffer_Coadd_Flip_Y:Coadd Image was NULL.");
		Detector_General_Error();
		return;
	}
	/* for the first half of the rows.
	** Note the middle row will be missed, this is OK as it
	** does not need to be flipped if it is in the middle */
	for(y=0;y<(Buffer_Data.Size_Y/2);y++)
	{
		/* for each column */
		for(x=0;x<Buffer_Data.Size_X;x++)
		{
			/* Copy Buffer_Data.Coadd_Image[x,y] to tempval */
			tempval = *(Buffer_Data.Coadd_Image+(y*Buffer_Data.Size_X)+x);
			/* Copy Buffer_Data.Coadd_Image[x,Buffer_Data.Size_Y-(y+1)] to Buffer_Data.Coadd_Image[x,y] */
			*(Buffer_Data.Coadd_Image+(y*Buffer_Data.Size_X)+x) = *(Buffer_Data.Coadd_Image+
				     (((Buffer_Data.Size_Y-(y+1))*Buffer_Data.Size_X)+x));
			/* Copy tempval to Buffer_Data.Coadd_Image[x,Buffer_Data.Size_Y-(y+1)] */
			*(Buffer_Data.Coadd_Image+(((Buffer_Data.Size_Y-(y+1))*Buffer_Data.Size_X)+x)) = tempval;
		}
	}
#if LOGGING > 5
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Buffer_Coadd_Flip_Y:Finished.");
#endif
}

/**
 * This routine creates a mean image of the coadd image, by taking each coadd pixel value and dividing
 * it by the number of coadds used to create the Coadd_Image.
 * @param coadds The number of coadds in the Coadd_Image.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Buffer_Error_Number/Buffer_Error_String are set.
 * @see #Buffer_Data
 * @see #Buffer_Error_Number
 * @see #Buffer_Error_String
 * @see detector_general.html#Detector_General_Log
 */
int Detector_Buffer_Create_Mean_Image(int coadds)
{
	int i,pixel_count;
	
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Buffer_Create_Mean_Image:Started.");
#endif
	if(coadds < 1)
	{
		Buffer_Error_Number = 9;
		sprintf(Buffer_Error_String,"Detector_Buffer_Create_Mean_Image:number of coadds too small (%d).",
			coadds);
		return FALSE;
	}
	if(Buffer_Data.Coadd_Image == NULL)
	{
		Buffer_Error_Number = 10;
		sprintf(Buffer_Error_String,"Detector_Buffer_Create_Mean_Image:Coadd Image was NULL.");
		return FALSE;
	}
	if(Buffer_Data.Mean_Image == NULL)
	{
		Buffer_Error_Number = 11;
		sprintf(Buffer_Error_String,"Detector_Buffer_Create_Mean_Image:Mean Image was NULL.");
		return FALSE;
	}
	pixel_count = Buffer_Data.Size_X*Buffer_Data.Size_Y;
	for(i=0; i < pixel_count; i++)
	{
		Buffer_Data.Mean_Image[i] = ((double)Buffer_Data.Coadd_Image[i])/coadds;
	}
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Buffer_Create_Mean_Image:Finished.");
#endif
	return TRUE;
}

/**
 * Return a pointer to the previously allocated unsigned short image buffer. Detector_Buffer_Allocate should have
 * been called previously to allocate memory for this buffer.
 * @return An unsigned short pointer to the previously allocated unsigned short image buffer.
 * @see #Buffer_Data
 */
unsigned short* Detector_Buffer_Get_Mono_Image(void)
{
	return Buffer_Data.Mono_Image;
}

/**
 * Return a pointer to the previously allocated double floating point image buffer. Detector_Buffer_Allocate should have
 * been called previously to allocate memory for this buffer.
 * @return A double pointer to the previously allocated mean image buffer.
 * @see #Buffer_Data
 */
double* Detector_Buffer_Get_Mean_Image(void)
{
	return Buffer_Data.Mean_Image;
}

/**
 * Return the x size in pixels of the image buffers.
 * @return An integer, the number of pixels in x. 
 * @see #Buffer_Data
 */
int Detector_Buffer_Get_Size_X(void)
{
	return Buffer_Data.Size_X;
}

/**
 * Return the y size in pixels of the image buffers.
 * @return An integer, the number of pixels in y. 
 * @see #Buffer_Data
 */
int Detector_Buffer_Get_Size_Y(void)
{
	return Buffer_Data.Size_Y;
}

/**
 * Return the number of pixels of allocated memory in the Mono / Coadd / Mean image buffers.
 * @return An integer, the number of pixels, computed by multiplying the x size and y size together. 
 * @see #Buffer_Data
 */
int Detector_Buffer_Get_Pixel_Count(void)
{
	return Buffer_Data.Size_X*Buffer_Data.Size_Y;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Buffer_Error_Number
 */
int Detector_Buffer_Get_Error_Number(void)
{
	return Buffer_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Buffer_Error_Number
 * @see #Buffer_Error_String
 * @see detector_general.html#Detector_General_Get_Current_Time_String
 */
void Detector_Buffer_Error(void)
{
	char time_string[32];

	Detector_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Buffer_Error_Number == 0)
		sprintf(Buffer_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s Detector_Buffer:Error(%d) : %s\n",time_string,Buffer_Error_Number,Buffer_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Buffer_Error_Number
 * @see #Buffer_Error_String
 * @see detector_general.html#Detector_General_Get_Current_Time_String
 */
void Detector_Buffer_Error_String(char *error_string)
{
	char time_string[32];

	Detector_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Buffer_Error_Number == 0)
		sprintf(Buffer_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s Detector_Buffer:Error(%d) : %s\n",time_string,
		Buffer_Error_Number,Buffer_Error_String);
}

/* =======================================
**  internal functions 
** ======================================= */
