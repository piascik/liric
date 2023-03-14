/* nudgematic_command.c
** Nudgematic mechanism library
*/
/**
 * Routines handling commands to the Nudgematic mechanism.
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

#include <errno.h>   /* Error number definitions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#ifdef MUTEXED
#include <pthread.h>
#endif
#include "log_udp.h"

#include "nudgematic_general.h"
#include "nudgematic_command.h"

/* data types */
/**
 * Data type holding local data to nudgematic_command. This consists of the following:
 * <dl>
 * <dt>Offset_Size</dt> <dd>The current set size of offsets the nudgematic will perform.</dd>
 * </dl>
 * @see #NUDGEMATIC_OFFSET_SIZE_T
 */
struct Command_Struct
{
	NUDGEMATIC_OFFSET_SIZE_T Offset_Size;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of Command_Struct that contains local data for this module.
 * This is statically initialised to the following:
 * <dl>
 * <dt>Offset_Size</dt> <dd>NONE</dd>
 * </dl>
 * @see #Command_Struct
 */
static struct Command_Struct Command_Data = 
{
	NONE
};

/**
 * Variable holding error code of last operation performed.
 */
static int Command_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see nudgematic_general.html#NUDGEMATIC_GENERAL_ERROR_STRING_LENGTH
 */
static char Command_Error_String[NUDGEMATIC_GENERAL_ERROR_STRING_LENGTH] = "";

/* =======================================
**  external functions 
** ======================================= */
/**
 * Routine to move the nudgematic to the specified position.
 * @param position The position to move to.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 */
int Nudgematic_Command_Position_Set(int position)
{
#if LOGGING > 0
	Nudgematic_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"Nudgematic_Command_Position_Set: Started with position %d.",
				      position);
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to get the current position of the nudgematic.
 * @param position The address of an integer, on return containing the current position, or '-1' if the mechanism
 *        is not in a defined position/moving.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 */
int Nudgematic_Command_Position_Get(int *position)
{
	if(position == NULL)
	{
		Command_Error_Number = 1;
		sprintf(Command_Error_String,"Nudgematic_Command_Position_Get:position was NULL.");
		return FALSE;
	}
	(*position) = -1;
#if LOGGING > 0
	Nudgematic_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"Nudgematic_Command_Position_Get: Current position %d.",
				      (*position));
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to set the offset size of the positions of the Nudgematic.
 * @param size The size of the offset to use, either SMALL or LARGE.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Nudgematic_Command_Offset_Size_To_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #NUDGEMATIC_OFFSET_SIZE_T
 */
int Nudgematic_Command_Offset_Size_Set(NUDGEMATIC_OFFSET_SIZE_T size)
{
	if((size != NONE)&&(size != SMALL)&&(size != LARGE))
	{
		Command_Error_Number = 2;
		sprintf(Command_Error_String,"Nudgematic_Command_Offset_Size_Set:Illegal size '%d'.",size);
		return FALSE;
	}
	Command_Data.Offset_Size = size;
#if LOGGING > 0
	Nudgematic_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				      "Nudgematic_Command_Offset_Size_Set: Offset size set to to %d (%s).",
				      size,Nudgematic_Command_Offset_Size_To_String(size));
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to get the current offset size of the positions of the Nudgematic.
 * @param size The address of a UDGEMATIC_OFFSET_SIZE_T, on a successful return should contain 
 *             the size of the offset to use, either SMALL or LARGE.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Nudgematic_Command_Offset_Size_To_String
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #NUDGEMATIC_OFFSET_SIZE_T
 */
int Nudgematic_Command_Offset_Size_Get(NUDGEMATIC_OFFSET_SIZE_T *size)
{
	if(size == NULL)
	{
		Command_Error_Number = 3;
		sprintf(Command_Error_String,"Nudgematic_Command_Offset_Size_Get:size was NULL.");
		return FALSE;
	}
	(*size) = Command_Data.Offset_Size;
#if LOGGING > 0
	Nudgematic_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				      "Nudgematic_Command_Offset_Size_Get: Current offset size %d '%s'.",(*size),
				      Nudgematic_Command_Offset_Size_To_String((*size)));
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to parse the specified string into a valid instance of NUDGEMATIC_OFFSET_SIZE_T.
 * @param size The address of a NUDGEMATIC_OFFSET_SIZE_T, on a successful return should contain 
 *             an offset size, either NONE, SMALL or LARGE.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #NUDGEMATIC_OFFSET_SIZE_T
 */
int Nudgematic_Command_Offset_Size_Parse(char *offset_size_string, NUDGEMATIC_OFFSET_SIZE_T *size)
{
	if(offset_size_string == NULL)
	{
		Command_Error_Number = 4;
		sprintf(Command_Error_String,"Nudgematic_Command_Offset_Size_Parse:offset_size_string was NULL.");
		return FALSE;
	}
	if(size == NULL)
	{
		Command_Error_Number = 5;
		sprintf(Command_Error_String,"Nudgematic_Command_Offset_Size_Parse:size was NULL.");
		return FALSE;
	}
#if LOGGING > 0
	Nudgematic_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				      "Nudgematic_Command_Offset_Size_Parse: Parsing offset size '%s'.",offset_size_string);
#endif /* LOGGING */
	if((strcmp(offset_size_string,"none") == 0)||(strcmp(offset_size_string,"NONE") == 0))
		(*size) = NONE;
	else if((strcmp(offset_size_string,"small") == 0)||(strcmp(offset_size_string,"SMALL") == 0))
		(*size) = SMALL;
	else if((strcmp(offset_size_string,"large") == 0)||(strcmp(offset_size_string,"LARGE") == 0))
		(*size) = LARGE;
	else
	{
		Command_Error_Number = 6;
		sprintf(Command_Error_String,"Nudgematic_Command_Offset_Size_Parse:failed to parse size '%s'.",
			offset_size_string);
		return FALSE;
	}
#if LOGGING > 0
	Nudgematic_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				      "Nudgematic_Command_Offset_Size_Parse: Parsed offset size '%s' to %d.",
				      offset_size_string,(*size));
#endif /* LOGGING */
	return TRUE;
}

/**
 * Routine to convert the specified offset size into a string.
 * @param size The offset size, of type NUDGEMATIC_OFFSET_SIZE_T, to convert.
 * @return The routine returns a string based on the offset size, one of: "SMALL", "LARGE", "NONE", "ERROR".
 */
char *Nudgematic_Command_Offset_Size_To_String(NUDGEMATIC_OFFSET_SIZE_T size)
{
	switch(size)
	{
		case NONE:
			return "NONE";
		case SMALL:
			return "SMALL";
		case LARGE:
			return "LARGE";
		default:
			return "ERROR";
	}
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Command_Error_Number
 */
int Nudgematic_Command_Get_Error_Number(void)
{
	return Command_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see nudgematic_general.html#Nudgematic_General_Get_Current_Time_String
 */
void Nudgematic_Command_Error(void)
{
	char time_string[32];

	Nudgematic_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Command_Error_Number == 0)
		sprintf(Command_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s Nudgematic_Command:Error(%d) : %s\n",time_string,
		Command_Error_Number,Command_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see nudgematic_general.html#Nudgematic_General_Get_Current_Time_String
 */
void Nudgematic_Command_Error_To_String(char *error_string)
{
	char time_string[32];

	Nudgematic_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Command_Error_Number == 0)
		sprintf(Command_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s Nudgematic_Command:Error(%d) : %s\n",time_string,
		Command_Error_Number,Command_Error_String);
}
