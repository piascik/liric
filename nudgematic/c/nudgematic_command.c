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
#include "nudgematic_connection.h"

/* hash defines */
/**
 * Index used in array for variables pertaining to the vertical cam of the nudgematic. 
 */
#define NUDGEMATIC_VERTICAL          (0)
/**
 * Index used in array for variables pertaining to the horizontal cam of the nudgematic. 
 */
#define NUDGEMATIC_HORIZONTAL        (1)
/**
 * The number of cams/motors in the nudgematic. 
 */
#define NUDGEMATIC_CAM_COUNT         (2)
/**
 * How many offsets are in NUDGEMATIC_OFFSET_SIZE_ENUM  (there is actually 3, as NONE is an offset).
 * @see #NUDGEMATIC_OFFSET_SIZE_ENUM 
 */
#define NUDGEMATIC_OFFSET_SIZE_COUNT (3)
/**
 * How many offset positions there are per offset size.
 */
#define NUDGEMATIC_POSITION_COUNT    (9)
/**
 * The length of some internal strings.
 */
#define STRING_LENGTH                (256)

/* data types */
/**
 * Data type holding local data to nudgematic_command. This consists of the following:
 * <dl>
 * <dt>Offset_Size</dt> <dd>The current set size of offsets the nudgematic will perform.</dd>
 * <dt>Target_Position</dt> <dd>The postion (0..9) we are currently trying to attain.</dd>
 * </dl>
 * @see #NUDGEMATIC_OFFSET_SIZE_T
 */
struct Command_Struct
{
	NUDGEMATIC_OFFSET_SIZE_T Offset_Size;
	int Target_Position;
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
 * <dt>Target_Position</dt> <dd>-1</dd>
 * </dl>
 * @see #Command_Struct
 */
static struct Command_Struct Command_Data = 
{
	NONE,-1
};

/**
 * List of characters representing commands to retrieve the current status (where the cam is) for each cam.
 * @see #NUDGEMATIC_CAM_COUNT
 * @see #NUDGEMATIC_VERTICAL
 * @see #NUDGEMATIC_HORIZONTAL
 */
static char Where_Command_List[NUDGEMATIC_CAM_COUNT] = {'w','W'};
/**
 * List of move commands to send, for each cam, for each offset size, for each position.
 * @see #NUDGEMATIC_POSITION_COUNT
 * @see #NUDGEMATIC_OFFSET_SIZE_COUNT
 * @see #NUDGEMATIC_CAM_COUNT
 */
static char Move_Command_List[NUDGEMATIC_POSITION_COUNT][NUDGEMATIC_OFFSET_SIZE_COUNT][NUDGEMATIC_CAM_COUNT] =
{     /* NONE(V,H),SMALL(V,H),LARGE(V,H) */
	{{'c','C'},{'c','C'},{'c','C'}}, /* centre */
	{{'c','C'},{'b','B'},{'a','A'}}, /* top left */
	{{'c','C'},{'d','D'},{'e','E'}}, /* bottom right */
	{{'c','C'},{'b','D'},{'a','E'}}, /* top right */
	{{'c','C'},{'d','B'},{'e','A'}}, /* bottom left */
	{{'c','C'},{'c','B'},{'c','A'}}, /* centre left */
	{{'c','C'},{'c','D'},{'c','E'}}, /* centre right */
	{{'c','C'},{'b','C'},{'a','C'}}, /* top centre */
	{{'c','C'},{'d','C'},{'e','C'}}  /* bottom centre */
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
 * @see #NUDGEMATIC_CAM_COUNT
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #Command_Data
 * @see #Move_Command_List
 * @see #Where_Command_List
 * @see nudgematic_connection.html#Nudgematic_Connection_Send_Command
 */
int Nudgematic_Command_Position_Set(int position)
{
	char command_string[STRING_LENGTH];
	char reply_string[STRING_LENGTH];
	char horizontal_cam_command, vertical_cam_command;
	int cam;
	int done[NUDGEMATIC_CAM_COUNT];
	
#if LOGGING > 0
	Nudgematic_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				      "Nudgematic_Command_Position_Set: Started with position %d and offset size '%s'.",
				      position,Nudgematic_Command_Offset_Size_To_String(Command_Data.Offset_Size));
#endif /* LOGGING */
	if((position < 0)||(position >= NUDGEMATIC_POSITION_COUNT))
	{
		Command_Error_Number = 7;
		sprintf(Command_Error_String,"Nudgematic_Command_Position_Set:position %d was out of range (0,%d).",
			position,NUDGEMATIC_POSITION_COUNT);
		return FALSE;		
	}
	Command_Data.Target_Position = position;
	horizontal_cam_command = Move_Command_List[Command_Data.Target_Position][Command_Data.Offset_Size][NUDGEMATIC_HORIZONTAL];
	vertical_cam_command = Move_Command_List[Command_Data.Target_Position][Command_Data.Offset_Size][NUDGEMATIC_VERTICAL];
#if LOGGING > 0
	Nudgematic_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				      "Nudgematic_Command_Position_Set: Position %d and Offset Size '%s' maps to "
				      "horizontal cam command '%c' and vertical cam command '%c'.",
				      Command_Data.Target_Position,Nudgematic_Command_Offset_Size_To_String(Command_Data.Offset_Size),
				      horizontal_cam_command,vertical_cam_command);
#endif /* LOGGING */
	/* move horizontal cam */
	command_string[0] = horizontal_cam_command;
	command_string[1] = '\0';
	if(!Nudgematic_Connection_Send_Command(command_string,reply_string,STRING_LENGTH))
	{
		Command_Error_Number = 8;
		sprintf(Command_Error_String,"Nudgematic_Command_Position_Set:Failed to send horizontal command string '%s'.",
			command_string);
		return FALSE;		
	}
	/* diddly parse reply string */
	/* move vertical cam */
	command_string[0] = vertical_cam_command;
	command_string[1] = '\0';
	if(!Nudgematic_Connection_Send_Command(command_string,reply_string,STRING_LENGTH))
	{
		Command_Error_Number = 9;
		sprintf(Command_Error_String,"Nudgematic_Command_Position_Set:Failed to send vertical command string '%s'.",
			command_string);
		return FALSE;		
	}
	/* diddly parse reply string */
	/* monitor for completion of move */
	for(cam = NUDGEMATIC_VERTICAL; cam < NUDGEMATIC_CAM_COUNT; cam++)
	{
		done[cam] = FALSE;
	}
	while((done[NUDGEMATIC_VERTICAL] == FALSE)&&(done[NUDGEMATIC_HORIZONTAL] == FALSE))
	{
		for(cam = NUDGEMATIC_VERTICAL; cam < NUDGEMATIC_CAM_COUNT; cam++)
		{
			command_string[0] = Where_Command_List[cam];
			command_string[1] = '\0';
			if(!Nudgematic_Connection_Send_Command(command_string,reply_string,STRING_LENGTH))
			{
				Command_Error_Number = 10;
				sprintf(Command_Error_String,"Nudgematic_Command_Position_Set:Failed to send where command string '%s'.",
					command_string);
				return FALSE;		
			}
			/* diddly parse reply string */
		}/* end for on cam */
	}/* end while not done[] */
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

/* =======================================
**  internal functions 
** ======================================= */
/**
 * Parse a reply string from the nudgematic. These are usually in the form: '&lt;pc&gt; &lt;adu&gt; &lt;pe&gt; &lt;nc&gt; &lt;t&gt;' where
 * <dl>
 * <dt>pc</dt> <dd>Position character, one of 'a','b','c','d','e','A','B','C','D','E','w','W'</dd>
 * <dt>adu</dt> <dd>An integer, the ADU of the position potentiometer for this cam.</dd>
 * <dt>pe</dt> <dd>The error (number of ADUs) between the attained position and it's ideal position.</dd>
 * <dt>nc</dt> <dd>The number of nudges used to attain the position.</dd>
 * <dt>t</dt> <dd>How long it took to attain the position, in milliseconds.</dd>
 * </dl>
 * @param reply_string The reply received from the Arduino.
 * @param position_char The address of a character, on return from the routine the position character.
 * @param position_adu The address of an integer, on return from the routine the relevant potentiometer ADUS (proxy for angle).
 * @param position_error The address of an integer, on return from the routine error (number of ADUs) between the 
 *                       attained position and it's ideal position.
 * @param nudges The address of an integer, on return from the routine the number of nudges used to attain the position.
 * @param time_ms The address of an integer, on return how long it took to attain the position, in milliseconds.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 */
static int Command_Parse_Reply_String(char *reply_string,char *position_char,int *position_adu,int *position_error,int *nudges,int *time_ms)
{
	int retval;
	
	if(reply_string == NULL)
	{
		Command_Error_Number = 11;
		sprintf(Command_Error_String,"Command_Parse_Reply_String:reply_string was NULL.");
		return FALSE;		
	}
	if(position_char == NULL)
	{
		Command_Error_Number = 12;
		sprintf(Command_Error_String,"Command_Parse_Reply_String:position_char was NULL.");
		return FALSE;		
	}
	if(position_adu == NULL)
	{
		Command_Error_Number = 13;
		sprintf(Command_Error_String,"Command_Parse_Reply_String:position_adu was NULL.");
		return FALSE;		
	}
	if(position_error == NULL)
	{
		Command_Error_Number = 14;
		sprintf(Command_Error_String,"Command_Parse_Reply_String:position_error was NULL.");
		return FALSE;		
	}
	if(nudges == NULL)
	{
		Command_Error_Number = 15;
		sprintf(Command_Error_String,"Command_Parse_Reply_String:nudges was NULL.");
		return FALSE;		
	}
	if(time_ms == NULL)
	{
		Command_Error_Number = 16;
		sprintf(Command_Error_String,"Command_Parse_Reply_String:time_ms was NULL.");
		return FALSE;		
	}
	retval = sscanf(reply_string,"%c %d %d %d %d",position_char,position_adu,position_error,nudges,time_ms);
	if(retval != 5)
	{
		Command_Error_Number = 17;
		sprintf(Command_Error_String,"Command_Parse_Reply_String:Failed to parse reply_string '%s' (%d).",reply_string,retval);
		return FALSE;		
	}
	return TRUE;
}
