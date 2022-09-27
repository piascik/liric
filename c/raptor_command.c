/* raptor_command.c
** Raptor command routines
*/
/**
 * Command routines for Raptor.
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
/**
 * Add more fields to struct tm (tm_tm_zone).
 */
#define _BSD_SOURCE
#include <time.h>
#include <unistd.h>

#include "log_udp.h"

#include "detector_exposure.h"
#include "detector_fits_filename.h"
#include "detector_fits_header.h"
#include "detector_general.h"
#include "detector_setup.h"
#include "detector_temperature.h"

#include "command_server.h"

#include "filter_wheel_command.h"
#include "filter_wheel_config.h"
#include "filter_wheel_general.h"

#include "raptor_config.h"
#include "raptor_fits_header.h"
/* diddly #include "raptor_multrun.h"*/
#include "raptor_general.h"
#include "raptor_server.h"

/* hash defines */
/**
 * Timezone offset for 1 hour.
 */
#define TIMEZONE_OFFSET_HOUR (3600)
/**
 * Timezone offset for BST.
 */
#define TIMEZONE_OFFSET_BST  (TIMEZONE_OFFSET_HOUR)

/* internal data */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";

/* internal functions */
static int Command_Parse_Date(char *time_string,int *time_secs);

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * Handle a command of the form: "abort".
 * <ul>
 * <li>We abort any running multrun's by calling Raptor_Multrun_Abort.
 * <li>We check the returned values from the aborts to see if they failed, ang log/return an error is this is the case.
 * <li>Otherwise we set the reply_string to a successful message.
 * </ul>
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see raptor_general.html#Raptor_General_Log
 * @see raptor_general.html#Raptor_General_Error_Number
 * @see raptor_general.html#Raptor_General_Error_String
 * @see raptor_general.html#Raptor_General_Add_String
 * @see raptor_multrun.html#Raptor_Multrun_Abort
 */
int Raptor_Command_Abort(char *command_string,char **reply_string)
{
	int multrun_abort_retval, bias_dark_abort_retval;
#if RAPTOR_DEBUG > 1
	Raptor_General_Log("command","raptor_command.c","Raptor_Command_Abort",LOG_VERBOSITY_TERSE,
			   "COMMAND","started.");
#endif
	/* abort multrun */
#if RAPTOR_DEBUG > 5
	Raptor_General_Log("command","raptor_command.c","Raptor_Command_Abort",LOG_VERBOSITY_INTERMEDIATE,
			   "COMMAND","Aborting multrun.");
#endif
	/* diddly multrun_abort_retval = Raptor_Multrun_Abort();*/
	/* check to see if there were problems with the aborts */
	/* diddly
	if((multrun_abort_retval == FALSE))
	{
		Raptor_General_Error("command","raptor_command.c","Raptor_Command_Abort",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
		Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Abort",
					  LOG_VERBOSITY_TERSE,"COMMAND","Failed to abort multrun (%d) or bias/dark (%d) command.",
					  multrun_abort_retval,bias_dark_abort_retval);
#endif
		if(!Raptor_General_Add_String(reply_string,"1 Failed to abort multrun/bias/dark command."))
			return FALSE;
		return TRUE;
	}
	*/
	if(!Raptor_General_Add_String(reply_string,"0 Multrun/Bias/Dark aborted."))
		return FALSE;
#if RAPTOR_DEBUG > 1
	Raptor_General_Log("command","raptor_command.c","Raptor_Command_Abort",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;
}

/**
 * Handle config commands of the forms:
 * <ul>
 * <li>"config coadd_exp_len <short|long>"
 * <li>"config filter <filtername>"
 * <li>"config nudgematic <small|large>"
 * </ul>
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see raptor_config.html#Raptor_Config_Nudgematic_Is_Enabled
 * @see raptor_config.html#Raptor_Config_Filter_Wheel_Is_Enabled
 * @see raptor_general.html#Raptor_General_Log
 * @see raptor_general.html#Raptor_General_Error_Number
 * @see raptor_general.html#Raptor_General_Error_String
 * @see raptor_general.html#Raptor_General_Add_String
 * @see raptor_general.html#Raptor_General_Add_Integer_To_String
 * @see raptor_multrun.html#Raptor_Multrun_Coadd_Exposure_Length_Set
 * @see ../filter_wheel/cdocs/filter_wheel_config.html#Filter_Wheel_Config_Name_To_Position
 * @see ../filter_wheel/cdocs/filter_wheel_command.html#Filter_Wheel_Command_Move
 */
int Raptor_Command_Config(char *command_string,char **reply_string)
{
	int retval,bin,parameter_index,filter_position;
	double camera_exposure_length;
	char filter_string[32];
	char coadd_exposure_length_string[32];
	char nudgematic_offset_size_string[32];
	char sub_config_command_string[16];

#if RAPTOR_DEBUG > 1
	Raptor_General_Log("command","raptor_command.c","Raptor_Command_Config",LOG_VERBOSITY_TERSE,
			   "COMMAND","started.");
#endif
	/* parse command */
	retval = sscanf(command_string,"config %15s %n",sub_config_command_string,&parameter_index);
	if(retval != 1)
	{
		Raptor_General_Error_Number = 501;
		sprintf(Raptor_General_Error_String,"Raptor_Command_Config:"
			"Failed to parse command %s (%d).",command_string,retval);
		Raptor_General_Error("command","raptor_command.c","Raptor_Command_Config",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
		Raptor_General_Log("command","raptor_command.c","Raptor_Command_Config",
				       LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
		if(!Raptor_General_Add_String(reply_string,"1 Failed to parse config command."))
			return FALSE;
		return TRUE;
	}
#if RAPTOR_DEBUG > 9
	Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Config",LOG_VERBOSITY_VERY_VERBOSE,
				  "COMMAND","Sub config command string: %s, parameter index %d.",
				  sub_config_command_string,parameter_index);
#endif
        if(strcmp(sub_config_command_string,"coadd_exp_len") == 0)
	{
		retval = sscanf(command_string+parameter_index,"%31s",coadd_exposure_length_string);
		if(retval != 1)
		{
			Raptor_General_Error_Number = 502;
			sprintf(Raptor_General_Error_String,"Raptor_Command_Config:"
				"Failed to parse command %s (%d).",command_string,retval);
			Raptor_General_Error("command","raptor_command.c","Raptor_Command_Config",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
			Raptor_General_Log("command","raptor_command.c","Raptor_Command_Config",
					   LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
			if(!Raptor_General_Add_String(reply_string,"1 Failed to parse config coadd_exp_len command."))
				return FALSE;
			return TRUE;
		}
		/* do something with parsed coadd_exposure_length_string */
		if(strcmp(coadd_exposure_length_string,"short") == 0)
		{
			/* diddly */
		}
		else if(strcmp(coadd_exposure_length_string,"long") == 0)
		{
			/* diddly */
		}
		else
		{
			Raptor_General_Error_Number = 506;
			sprintf(Raptor_General_Error_String,"Raptor_Command_Config:"
				"Illegal coadd exposure length: '%s'.",coadd_exposure_length_string);
			Raptor_General_Error("command","raptor_command.c","Raptor_Command_Config",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
			Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Config",
						  LOG_VERBOSITY_TERSE,"COMMAND",
						  "finished (Illegal coadd exposure length: '%s').",
						  coadd_exposure_length_string);
#endif
			if(!Raptor_General_Add_String(reply_string,"1 Illegal coadd exposure length:"))
				return FALSE;
			if(!Raptor_General_Add_String(reply_string,coadd_exposure_length_string))
				return FALSE;
			return TRUE;
		}
		if(!Raptor_General_Add_String(reply_string,"0 Coadd exposure length set to:"))
			return FALSE;
		if(!Raptor_General_Add_String(reply_string,coadd_exposure_length_string))
			return FALSE;
	}
	else if(strcmp(sub_config_command_string,"filter") == 0)
	{
		/* copy rest of command as filter name - filter names have spaces in them! */
		strncpy(filter_string,command_string+parameter_index,31);
#if RAPTOR_DEBUG > 5
		Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Config",
					  LOG_VERBOSITY_VERBOSE,"COMMAND","Setting Filter to: %s.",filter_string);
#endif
		if(Raptor_Config_Filter_Wheel_Is_Enabled())
		{
			/* string to position conversion */
			if(!Filter_Wheel_Config_Name_To_Position(filter_string,&filter_position))
			{
				Raptor_General_Error_Number = 503;
				sprintf(Raptor_General_Error_String,"Raptor_Command_Config:"
					"Failed to convert filter name '%s' to a valid filter position.",filter_string);
				Raptor_General_Error("command","raptor_command.c","Raptor_Command_Config",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
				Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Config",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to convert filter name '%s' to a valid filter position.",
							  filter_string);
#endif
				if(!Raptor_General_Add_String(reply_string,"1 Failed to convert filter name:"))
					return FALSE;
				if(!Raptor_General_Add_String(reply_string,filter_string))
					return FALSE;
				return TRUE;
			}
#if RAPTOR_DEBUG > 9
			Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Config",
					  LOG_VERBOSITY_VERY_VERBOSE,"COMMAND","Filter position: %d.",filter_position);
#endif
			/* actually move filter wheel */
			if(!Filter_Wheel_Command_Move(filter_position))
			{
				Raptor_General_Error_Number = 504;
				sprintf(Raptor_General_Error_String,"Raptor_Command_Config:"
					"Failed to move filter wheel to filter '%s', position %d.",
					filter_string,filter_position);
				Raptor_General_Error("command","raptor_command.c","Raptor_Command_Config",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
				Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Config",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to move filter wheel to filter '%s', position %d.",
							  filter_string,filter_position);
#endif
				if(!Raptor_General_Add_String(reply_string,"1 Failed to move filter wheel to filter:"))
					return FALSE;
				if(!Raptor_General_Add_String(reply_string,filter_string))
					return FALSE;
				return TRUE;
			}
			/* success */
			if(!Raptor_General_Add_String(reply_string,"0 Filter wheel moved to position:"))
				return FALSE;
			if(!Raptor_General_Add_String(reply_string,filter_string))
				return FALSE;
		}
		else /* filter wheel is not enabled */
		{
			/* success */
			if(!Raptor_General_Add_String(reply_string,"0 Filter Wheel not enabled."))
				return FALSE;
		}
	}
	else if(strcmp(sub_config_command_string,"nudgematic") == 0)
	{
		retval = sscanf(command_string+parameter_index,"%31s",nudgematic_offset_size_string);
		if(retval != 1)
		{
			Raptor_General_Error_Number = 533;
			sprintf(Raptor_General_Error_String,"Raptor_Command_Config:"
				"Failed to parse command %s (%d).",command_string,retval);
			Raptor_General_Error("command","raptor_command.c","Raptor_Command_Config",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
			Raptor_General_Log("command","raptor_command.c","Raptor_Command_Config",
					   LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
			if(!Raptor_General_Add_String(reply_string,"1 Failed to parse config nudgematic command."))
				return FALSE;
			return TRUE;
		}
#if RAPTOR_DEBUG > 5
		Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Config",
					  LOG_VERBOSITY_VERBOSE,"COMMAND","Setting nudgematic offset size to: %s.",
					  nudgematic_offset_size_string);
#endif
		/* cache the nudgematic offset size setting in the multrun data for FITS header generation */
		/*Raptor_Multrun_Nudgematic_Offset_Size_Set(nudgematic_offset_size_string);*/
		if(strcmp(nudgematic_offset_size_string,"small") == 0)
		{
			/* diddly */
		}
		else if(strcmp(nudgematic_offset_size_string,"large") == 0)
		{
			/* diddly */
		}
		else
		{
			Raptor_General_Error_Number = 534;
			sprintf(Raptor_General_Error_String,"Raptor_Command_Config:Unknown nudgematic offset size %s.",
				nudgematic_offset_size_string);
			Raptor_General_Error("command","raptor_command.c","Raptor_Command_Config",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
			Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Config",
						  LOG_VERBOSITY_TERSE,"COMMAND",
						  "finished (Unknown nudgematic offset size %s).",
						  nudgematic_offset_size_string);
#endif
			if(!Raptor_General_Add_String(reply_string,"1 Failed to parse config nudgematic command:"))
				return FALSE;
			if(!Raptor_General_Add_String(reply_string,command_string))
				return FALSE;
			return TRUE;
		}
		/* configure nudgematic if enabled */
		if(Raptor_Config_Nudgematic_Is_Enabled())
		{
			/* diddly */
		} /* end if Raptor_Config_Nudgematic_Is_Enabled */
		if(!Raptor_General_Add_String(reply_string,"0 Config nudgematic completed."))
			return FALSE;
	}
	else
	{
		if(!Raptor_General_Add_String(reply_string,"1 Unknown config sub-command:"))
			return FALSE;
		if(!Raptor_General_Add_String(reply_string,sub_config_command_string))
			return FALSE;
	}
#if RAPTOR_DEBUG > 1
	Raptor_General_Log("command","raptor_command.c","Raptor_Command_Config",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;
}

/**
 * Implementation of FITS Header commands.
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see raptor_general.html#Raptor_General_Log
 * @see raptor_general.html#Raptor_General_Error_Number
 * @see raptor_general.html#Raptor_General_Error_String
 * @see raptor_general.html#Raptor_General_Add_String
 * @see raptor_fits_header.html#Raptor_Fits_Header_Logical_Add
 * @see raptor_fits_header.html#Raptor_Fits_Header_Float_Add
 * @see raptor_fits_header.html#Raptor_Fits_Header_Integer_Add
 * @see raptor_fits_header.html#Raptor_Fits_Header_String_Add
 * @see raptor_fits_header.html#Raptor_Fits_Header_Clear
 * @see raptor_fits_header.html#Raptor_Fits_Header_Delete
 */
int Raptor_Command_Fits_Header(char *command_string,char **reply_string)
{
	char operation_string[8];
	char keyword_string[13];
	char type_string[8];
	char value_string[80];
	int retval,command_string_index,ivalue,value_index;
	double dvalue;

	/* parse command to retrieve operation*/
	retval = sscanf(command_string,"fitsheader %6s %n",operation_string,&command_string_index);
	if((retval != 1)&&(retval != 2)) /* sscanf isn't sure whether %n increments returned value! */
	{
		Raptor_General_Error_Number = 517;
		sprintf(Raptor_General_Error_String,"Raptor_Command_Fits_Header:"
			"Failed to parse command %s (%d).",command_string,retval);
#if RAPTOR_DEBUG > 1
		Raptor_General_Log("command","raptor_command.c","Raptor_Command_Fits_Header",
				       LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
		if(!Raptor_General_Add_String(reply_string,"1 Failed to parse fitsheader command."))
			return FALSE;
		return TRUE;
	}
	/* do operation */
	if(strncmp(operation_string,"add",3) == 0)
	{
		retval = sscanf(command_string+command_string_index,"%12s %7s %n",keyword_string,type_string,
				&value_index);
		if((retval != 3)&&(retval != 2)) /* %n may or may not increment retval*/
		{
			Raptor_General_Error_Number = 518;
			sprintf(Raptor_General_Error_String,"Raptor_Command_Fits_Header:"
				"Failed to parse add command %s (%d).",command_string,retval);
			Raptor_General_Error("command","raptor_command.c","Raptor_Command_Fits_Header",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
			Raptor_General_Log("command","raptor_command.c","Raptor_Command_Fits_Header",
					   LOG_VERBOSITY_TERSE,"COMMAND","finished (add command parse failed).");
#endif
			if(!Raptor_General_Add_String(reply_string,"1 Failed to parse fitsheader add command."))
				return FALSE;
			return TRUE;
		}
		strncpy(value_string,command_string+command_string_index+value_index,79);
		value_string[79] = '\0';
		if(strncmp(type_string,"boolean",7)==0)
		{
			/* parse value */
			if(strncmp(value_string,"true",4) == 0)
				ivalue = TRUE;
			else if(strncmp(value_string,"false",5) == 0)
				ivalue = FALSE;
			else
			{
				Raptor_General_Error_Number = 519;
				sprintf(Raptor_General_Error_String,"Raptor_Command_Fits_Header:"
					"Add boolean command had unknown value %s.",value_string);
				Raptor_General_Error("command","raptor_command.c","Raptor_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
				Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Add boolean command had unknown value %s.",value_string);
#endif
				if(!Raptor_General_Add_String(reply_string,
							   "1 Failed to parse fitsheader add boolean command value."))
					return FALSE;
				return TRUE;
			}
			/* do operation */
			if(!Raptor_Fits_Header_Logical_Add(keyword_string,ivalue,NULL))
			{
				Raptor_General_Error("command","raptor_command.c","Raptor_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
				Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to add boolean to FITS header.");
#endif
				if(!Raptor_General_Add_String(reply_string,"1 Failed to add boolean fits header."))
					return FALSE;
				return TRUE;
			}
		}
		else if(strncmp(type_string,"float",5)==0)
		{
			/* parse value */
			retval = sscanf(value_string,"%lf",&dvalue);
			if(retval != 1)
			{
				Raptor_General_Error_Number = 520;
				sprintf(Raptor_General_Error_String,"Raptor_Command_Fits_Header:"
					"Add float command had unknown value %s.",value_string);
				Raptor_General_Error("command","raptor_command.c","Raptor_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
				Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Add float command had unknown value %s.",value_string);
#endif
				if(!Raptor_General_Add_String(reply_string,
							   "1 Failed to parse fitsheader add float command value."))
					return FALSE;
				return TRUE;
			}
			/* do operation */
			if(!Raptor_Fits_Header_Float_Add(keyword_string,dvalue,NULL))
			{
				Raptor_General_Error("command","raptor_command.c","Raptor_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
				Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to add float to FITS header.");
#endif
				if(!Raptor_General_Add_String(reply_string,"1 Failed to add float fits header."))
					return FALSE;
				return TRUE;
			}
		}
		else if(strncmp(type_string,"integer",7)==0)
		{
			/* parse value */
			retval = sscanf(value_string,"%d",&ivalue);
			if(retval != 1)
			{
				Raptor_General_Error_Number = 521;
				sprintf(Raptor_General_Error_String,"Raptor_Command_Fits_Header:"
					"Add integer command had unknown value %s.",value_string);
				Raptor_General_Error("command","raptor_command.c","Raptor_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
				Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Add integer command had unknown value %s.",value_string);
#endif
				if(!Raptor_General_Add_String(reply_string,
							   "1 Failed to parse fitsheader add integer command value."))
					return FALSE;
				return TRUE;
			}
			/* do operation */
			if(!Raptor_Fits_Header_Integer_Add(keyword_string,ivalue,NULL))
			{
				Raptor_General_Error("command","raptor_command.c","Raptor_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
				Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to add integer to FITS header.");
#endif
				if(!Raptor_General_Add_String(reply_string,"1 Failed to add integer fits header."))
					return FALSE;
				return TRUE;
			}
		}
		else if(strncmp(type_string,"string",6)==0)
		{
			/* do operation */
			if(!Raptor_Fits_Header_String_Add(keyword_string,value_string,NULL))
			{
				Raptor_General_Error("command","raptor_command.c","Raptor_Command_Fits_Header",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
				Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Fits_Header",
							  LOG_VERBOSITY_TERSE,"COMMAND",
							  "Failed to add string to FITS header.");
#endif
				if(!Raptor_General_Add_String(reply_string,"1 Failed to add string fits header."))
					return FALSE;
				return TRUE;
			}
		}
		else
		{
			Raptor_General_Error_Number = 522;
			sprintf(Raptor_General_Error_String,"Raptor_Command_Fits_Header:"
				"Add command had unknown type %s.",type_string);
			Raptor_General_Error("command","raptor_command.c","Raptor_Command_Fits_Header",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
			Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Fits_Header",
				       LOG_VERBOSITY_TERSE,"COMMAND","Add command had unknown type %s.",type_string);
#endif
			if(!Raptor_General_Add_String(reply_string,"1 Failed to parse fitsheader add command type."))
				return FALSE;
			return TRUE;
		}
	}
	else if(strncmp(operation_string,"delete",6) == 0)
	{
		retval = sscanf(command_string+command_string_index,"%12s",keyword_string);
		if(retval != 1)
		{
			Raptor_General_Error_Number = 523;
			sprintf(Raptor_General_Error_String,"Raptor_Command_Fits_Header:"
				"Failed to parse delete command %s (%d).",command_string,retval);
			Raptor_General_Error("command","raptor_command.c","Raptor_Command_Fits_Header",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
			Raptor_General_Log("command","raptor_command.c","Raptor_Command_Fits_Header",
					   LOG_VERBOSITY_TERSE,"COMMAND","finished (delete command parse failed).");
#endif
			if(!Raptor_General_Add_String(reply_string,"1 Failed to parse fitsheader delete command."))
				return FALSE;
			return TRUE;
		}
		/* do delete */
		if(!Raptor_Fits_Header_Delete(keyword_string))
		{
			Raptor_General_Error("command","raptor_command.c","Raptor_Command_Fits_Header",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
			Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Fits_Header",
						  LOG_VERBOSITY_TERSE,"COMMAND",
						  "Failed to delete FITS header with keyword '%s'.",keyword_string);
#endif
			if(!Raptor_General_Add_String(reply_string,"1 Failed to delete fits header."))
				return FALSE;
			return TRUE;
		}
	}
	else if(strncmp(operation_string,"clear",5) == 0)
	{
		/* do clear */
		if(!Raptor_Fits_Header_Clear())
		{
			Raptor_General_Error("command","raptor_command.c","Raptor_Command_Fits_Header",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
			Raptor_General_Log("command","raptor_command.c","Raptor_Command_Fits_Header",
					   LOG_VERBOSITY_TERSE,"COMMAND","Failed to clear FITS header.");
#endif
			if(!Raptor_General_Add_String(reply_string,"1 Failed to clear fits header."))
				return FALSE;
			return TRUE;
		}
	}
	else
	{
		Raptor_General_Error_Number = 524;
		sprintf(Raptor_General_Error_String,"Raptor_Command_Fits_Header:"
			"Unknown operation %s:Failed to parse command %s.",operation_string,command_string);
		Raptor_General_Error("command","raptor_command.c","Raptor_Command_Fits_Header",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
		Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Fits_Header",
					  LOG_VERBOSITY_TERSE,
					  "COMMAND","Unknown operation %s:Failed to parse command %s.",
					  operation_string,command_string);
#endif
		if(!Raptor_General_Add_String(reply_string,"1 Failed to parse fitsheader command: Unknown operation."))
			return FALSE;
		return TRUE;
	}
	if(!Raptor_General_Add_String(reply_string,"0 FITS Header command succeeded."))
		return FALSE;
#if RAPTOR_DEBUG > 1
	Raptor_General_Log("command","raptor_command.c","Raptor_Command_Fits_Header",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;
}

/**
 * Handle a command of the form: "multrun <length> <count> <standard>".
 * <ul>
 * <li>The multrun command is parsed to get the exposure length, count and standard (true|false) values.
 * <li>We call Raptor_Multrun to take the multrun images.
 * <li>The reply string is constructed of the form "0 <filename count> <multrun number> <last FITS filename>".
 * <li>We log the returned filenames.
 * <li>We free the returned filenames.
 * </ul>
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see raptor_general.html#Raptor_General_Log
 * @see raptor_general.html#Raptor_General_Error_Number
 * @see raptor_general.html#Raptor_General_Error_String
 * @see raptor_general.html#Raptor_General_Add_String
 * @see raptor_multrun.html#Raptor_Multrun
 * @see ../detector/cdocs/detector_fits_filename.html#Detector_Fits_Filename_Multrun_Get
 * @see ../detector/cdocs/detector_fits_filename.html#Detector_Fits_Filename_List_Free
 */
int Raptor_Command_Multrun(char *command_string,char **reply_string)
{
	struct timespec start_time = {0L,0L};
	char **filename_list = NULL;
	char standard_string[8];
	char count_string[16];
	int i,retval,exposure_length,exposure_count,filename_count,do_standard,multrun_number;

#if RAPTOR_DEBUG > 1
	Raptor_General_Log("command","raptor_command.c","Raptor_Command_Multrun",LOG_VERBOSITY_TERSE,
			   "COMMAND","started.");
#endif
	/* parse command */
	retval = sscanf(command_string,"multrun %d %d %7s",&exposure_length,&exposure_count,standard_string);
	if(retval != 3)
	{
		Raptor_General_Error_Number = 505;
		sprintf(Raptor_General_Error_String,"Raptor_Command_Multrun:"
			"Failed to parse command %s (%d).",command_string,retval);
		Raptor_General_Error("command","raptor_command.c","Raptor_Command_Multrun",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
		Raptor_General_Log("command","raptor_command.c","Raptor_Command_Multrun",
				       LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
		if(!Raptor_General_Add_String(reply_string,"1 Failed to parse multrun command."))
			return FALSE;
		return TRUE;
	}
	/* parse standard string */
	if(strcmp(standard_string,"true") == 0)
		do_standard = TRUE;
	else if(strcmp(standard_string,"false") == 0)
		do_standard = FALSE;
	else
	{
		Raptor_General_Error_Number = 539;
		sprintf(Raptor_General_Error_String,"Raptor_Command_Multrun:Illegal standard value '%s'.",
			standard_string);
		Raptor_General_Error("command","raptor_command.c","Raptor_Command_Multrun",
				     LOG_VERBOSITY_TERSE,"COMMAND");
		if(!Raptor_General_Add_String(reply_string,"1 Multrun failed:Illegal standard value."))
			return FALSE;
		return TRUE;
	}
	/* do multrun */
	/* diddly
	retval = Raptor_Multrun(exposure_length,exposure_count,do_standard,&filename_list,&filename_count);
	if(retval == FALSE)
	{
		Raptor_General_Error("command","raptor_command.c","Raptor_Command_Multrun",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
		Raptor_General_Log("command","raptor_command.c","Raptor_Command_Multrun",
				   LOG_VERBOSITY_TERSE,"COMMAND","Multrun failed.");
#endif
		if(!Raptor_General_Add_String(reply_string,"1 Multrun failed."))
			return FALSE;
		return TRUE;
	}
	*/
	/* success */
	if(!Raptor_General_Add_String(reply_string,"0 "))
	{
		Detector_Fits_Filename_List_Free(&filename_list,&filename_count);
		return FALSE;
	}
	/* add number of FITS images */
	sprintf(count_string,"%d ",filename_count);
	if(!Raptor_General_Add_String(reply_string,count_string))
	{
		Detector_Fits_Filename_List_Free(&filename_list,&filename_count);
		return FALSE;
	}
	/* get multrun number */
	multrun_number = Detector_Fits_Filename_Multrun_Get();
	sprintf(count_string,"%d ",multrun_number);
	if(!Raptor_General_Add_String(reply_string,count_string))
	{
		Detector_Fits_Filename_List_Free(&filename_list,&filename_count);
		return FALSE;
	}
	/* add last filename */
	if(filename_count > 0)
	{
		if(!Raptor_General_Add_String(reply_string,filename_list[filename_count-1]))
		{
			Detector_Fits_Filename_List_Free(&filename_list,&filename_count);
			return FALSE;
		}
	}
	else
	{
		if(!Raptor_General_Add_String(reply_string,"none"))
		{
			Detector_Fits_Filename_List_Free(&filename_list,&filename_count);
			return FALSE;
		}
	}
	/* log filenames returned */
	for(i=0; i < filename_count; i++)
	{
#if RAPTOR_DEBUG > 8
		Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Multrun",
					  LOG_VERBOSITY_VERY_VERBOSE,"COMMAND","Filename %d : %s",i,filename_list[i]);
#endif
	}
	if(!Detector_Fits_Filename_List_Free(&filename_list,&filename_count))
	{
		Raptor_General_Error_Number = 510;
		sprintf(Raptor_General_Error_String,"Raptor_Command_Multrun:Detector_Fits_Filename_List_Free failed.");
		Raptor_General_Error("command","raptor_command.c","Raptor_Command_Multrun",
				     LOG_VERBOSITY_TERSE,"COMMAND");
		if(!Raptor_General_Add_String(reply_string,"1 Multrun failed (freeing filename list)."))
			return FALSE;
		return TRUE;
	}
#if RAPTOR_DEBUG > 1
	Raptor_General_Log("command","raptor_command.c","Raptor_Command_Multrun",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;
}

/**
 * Handle a status command. Possible forms: 
 * <ul>
 * <li>status temperature [get|status]
 * <li>status filterwheel [filter|position|status]
 * <li>status nudgematic [position|status]
 * <li>status exposure [status|count|length|start_time]
 * <li>status exposure [index|multrun|run]
 * </ul>
 * <ul>
 * <li>The status command is parsed to retrieve the subsystem (1st parameter).
 * <li>Based on the subsystem, further parsing occurs.
 * <li>The relevant status is retrieved, and a suitable reply constructed.
 * </ul>
 * @param command_string The command. This is not changed during this routine.
 * @param reply_string The address of a pointer to allocate and set the reply string.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see raptor_config.html#Raptor_Config_Filter_Wheel_Is_Enabled
 * @see raptor_general.html#Raptor_General_Log
 * @see raptor_general.html#Raptor_General_Error_Number
 * @see raptor_general.html#Raptor_General_Error_String
 * @see raptor_general.html#Raptor_General_Add_String
 * @see raptor_general.html#Raptor_General_Get_Time_String
 * @see raptor_general.html#Raptor_General_Get_Current_Time_String
 * @see raptor_multrun.html#Raptor_Multrun_In_Progress
 * @see raptor_multrun.html#Raptor_Multrun_Count_Get
 * @see raptor_multrun.html#Raptor_Multrun_Exposure_Length_Get
 * @see raptor_multrun.html#Raptor_Multrun_Exposure_Start_Time_Get
 * @see raptor_multrun.html#Raptor_Multrun_Exposure_Index_Get
 * @see raptor_multrun.html#Raptor_Multrun_Multrun_Get
 * @see raptor_multrun.html#Raptor_Multrun_Run_Get
 * @see ../detector/cdocs/detector_fits_filename.html#Detector_Fits_Filename_Multrun_Get
 * @see ../detector/cdocs/detector_fits_filename.html#Detector_Fits_Filename_Run_Get
 * @see ../detector/cdocs/detector_temperature.html#Detector_Temperature_Get
 * @see ../detector/cdocs/detector_temperature.html#Detector_Temperature_PCB_Get
 * @see ../filter_wheel/cdocs/filter_wheel_command.html#Filter_Wheel_Command_Get_Position
 */
int Raptor_Command_Status(char *command_string,char **reply_string)
{
	struct timespec status_time;
	char time_string[32];
	char return_string[128];
	char subsystem_string[32];
	char get_set_string[16];
	char key_string[64];
	char temperature_status_string[32];
	char filter_name_string[32];
	char *camera_name_string = NULL;
	int retval,command_string_index,ivalue,filter_wheel_position;
	double temperature;

	/* parse command */
	retval = sscanf(command_string,"status %31s %n",subsystem_string,&command_string_index);
	if((retval != 1)&&(retval != 2)) /* sscanf isn't sure whether %n increments returned value! */
	{
		Raptor_General_Error_Number = 511;
		sprintf(Raptor_General_Error_String,"aptor_Command_Status:"
			"Failed to parse command %s (%d).",command_string,retval);
		Raptor_General_Error("command","raptor_command.c","Raptor_Command_Status",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
		Raptor_General_Log("command","raptor_command.c","Raptor_Command_Status",
				       LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
		if(!Raptor_General_Add_String(reply_string,"1 Failed to parse status command."))
			return FALSE;
		return TRUE;
	}
	
	/* initialise return string */
	strcpy(return_string,"0 ");
	/* parse subsystem */
	if(strncmp(subsystem_string,"exposure",8) == 0)
	{
		/* diddly
		if(strncmp(command_string+command_string_index,"status",6)==0)
		{
			if(Raptor_Multrun_In_Progress())
				strcat(return_string,"true");
			else
				strcat(return_string,"false");
		}
		else if(strncmp(command_string+command_string_index,"count",5)==0)
		{
			ivalue = Raptor_Multrun_Count_Get();
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else if(strncmp(command_string+command_string_index,"length",6)==0)
		{
			ivalue = Raptor_Multrun_Exposure_Length_Get();
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else if(strncmp(command_string+command_string_index,"start_time",10)==0)
		{
			Raptor_Multrun_Exposure_Start_Time_Get(&status_time);
			Raptor_General_Get_Time_String(status_time,time_string,31);
			sprintf(return_string+strlen(return_string),"%s",time_string);
		}
		else if(strncmp(command_string+command_string_index,"index",5)==0)
		{
			ivalue = Raptor_Multrun_Exposure_Index_Get();
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else if(strncmp(command_string+command_string_index,"multrun",7)==0)
		{
			ivalue = Raptor_Multrun_Multrun_Get();
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else if(strncmp(command_string+command_string_index,"run",3)==0)
		{
			ivalue = Raptor_Multrun_Run_Get();
			sprintf(return_string+strlen(return_string),"%d",ivalue);
		}
		else
		{
			Raptor_General_Error_Number = 512;
			sprintf(Raptor_General_Error_String,"Raptor_Command_Status:"
				"Failed to parse exposure status command %s.",command_string+command_string_index);
			RaptorGeneral_Error("command","raptor_command.c","Raptor_Command_Status",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
			Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Status",
						  LOG_VERBOSITY_TERSE,"COMMAND",
						  "Failed to parse exposure status command %s.",
						  command_string+command_string_index);
#endif
			if(!Raptor_General_Add_String(reply_string,"1 Failed to parse exposure status command."))
				return FALSE;
			return TRUE;
		}
		*/
		/* diddly */
		if(!Raptor_General_Add_String(reply_string,"1 exposure status not implemented yet."))
			return FALSE;
		return TRUE;
	}
	else if(strncmp(subsystem_string,"filterwheel",11) == 0)
	{
		if(Raptor_Config_Filter_Wheel_Is_Enabled())
		{
			if(!Filter_Wheel_Command_Get_Position(&filter_wheel_position))
			{
				Raptor_General_Error_Number = 509;
				sprintf(Raptor_General_Error_String,"Raptor_Command_Status:"
					"Failed to get filter wheel position.");
				Raptor_General_Error("command","raptor_command.c","Raptor_Command_Status",
						     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
				Raptor_General_Log("command","raptor_command.c","Raptor_Command_Status",
						   LOG_VERBOSITY_TERSE,"COMMAND","Failed to get filter wheel position.");
#endif
				if(!Raptor_General_Add_String(reply_string,"1 Failed to get filter wheel position."))
					return FALSE;
				return TRUE;
			}/* end if filter wheel is enabled */
		}
		else
		{
#if RAPTOR_DEBUG > 5
			Raptor_General_Log("command","raptor_command.c","Raptor_Command_Status",
					   LOG_VERBOSITY_VERBOSE,"COMMAND",
					   "Raptor filter wheel is NOT enabled, faking filter wheel position to 0 (moving).");
#endif
			/* we pretend the filter wheel is moving when it is not enabled */
			filter_wheel_position = 0;
		}
		if(strncmp(command_string+command_string_index,"filter",6)==0)
		{
			if(filter_wheel_position == 0) /* moving */
			{
				strcpy(filter_name_string,"moving");
			}
			else
			{
				if(!Filter_Wheel_Config_Position_To_Name(filter_wheel_position,filter_name_string))
				{
					Raptor_General_Error_Number = 514;
					sprintf(Raptor_General_Error_String,"Raptor_Command_Status:"
						"Failed to get filter wheel filter name from position %d.",
						filter_wheel_position);
					Raptor_General_Error("command","raptor_command.c","Raptor_Command_Status",
							     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
					Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Status",
								  LOG_VERBOSITY_TERSE,"COMMAND",
						  "Failed to get filter wheel filter name from position %d.",
								  filter_wheel_position);
#endif
					if(!Raptor_General_Add_String(reply_string,
							"1 Failed to get filter wheel filter name from position:"))
						return FALSE;
					if(!Raptor_General_Add_Integer_To_String(reply_string,filter_wheel_position))
						return FALSE;
					return TRUE;
				}
			}
			strcat(return_string,filter_name_string);
		}
		else if(strncmp(command_string+command_string_index,"position",8)==0)
		{
			sprintf(return_string+strlen(return_string),"%d",filter_wheel_position);
		}
		else if(strncmp(command_string+command_string_index,"status",6)==0)
		{
			if(filter_wheel_position == 0)/* moving */
				strcat(return_string,"moving");
			else
				strcat(return_string,"in_position");
		}
		else
		{
			Raptor_General_Error_Number = 525;
			sprintf(Raptor_General_Error_String,"Raptor_Command_Status:"
				"Failed to parse filterwheel command %s.",command_string+command_string_index);
			Raptor_General_Error("command","raptor_command.c","Raptor_Command_Status",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
			Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Status",
						  LOG_VERBOSITY_TERSE,"COMMAND",
						  "Failed to parse filterwheel command %s.",
						  command_string+command_string_index);
#endif
			if(!Raptor_General_Add_String(reply_string,"1 Failed to parse filterwheel status command."))
				return FALSE;
			return TRUE;
		}
	}
	else if(strncmp(subsystem_string,"nudgematic",7) == 0)
	{
		/* diddly
		if(strncmp(command_string+command_string_index,"position",8)==0)
		{
			diddly;
			sprintf(return_string+strlen(return_string),"%.2f",rotator_position);			
		}
		else if(strncmp(command_string+command_string_index,"status",6)==0)
		{
			diddly;
		}
		else
		{
			Raptor_General_Error_Number = 543;
			sprintf(Raptor_General_Error_String,"Raptor_Command_Status:"
				"Failed to parse nudgematic command %s.",command_string+command_string_index);
			Raptor_General_Error("command","raptor_command.c","Raptor_Command_Status",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
			Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Status",
						  LOG_VERBOSITY_TERSE,"COMMAND",
						  "Failed to parse status nudgematic command %s.",
						  command_string+command_string_index);
#endif
			if(!Raptor_General_Add_String(reply_string,"1 Failed to parse status nudgematic command."))
				return FALSE;
			return TRUE;
		}
		*/
		if(!Raptor_General_Add_String(reply_string,"1 status nudgematic not implemented yet."))
			return FALSE;
		return TRUE;
	}
	else if(strncmp(subsystem_string,"temperature",11) == 0)
	{
		retval = sscanf(command_string,"status temperature %15s",get_set_string);
		if(retval != 1)
		{
			Raptor_General_Error_Number = 526;
			sprintf(Raptor_General_Error_String,"Raptor_Command_Status:"
				"Failed to parse command %s (%d).",command_string,retval);
			Raptor_General_Error("command","raptor_command.c","Raptor_Command_Status",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
			Raptor_General_Log("command","raptor_command.c","Raptor_Command_Status",
					   LOG_VERBOSITY_TERSE,"COMMAND","finished (command parse failed).");
#endif
			if(!Raptor_General_Add_String(reply_string,
						      "1 Failed to parse status temperature ."))
				return FALSE;
			return TRUE;
		}
		/* check subcommand */
		if(strncmp(get_set_string,"get",3)==0)
		{
			/* diddly
			if((Raptor_Multrun_In_Progress() == FALSE))
			{
			*/
				if(!Detector_Temperature_Get(&temperature))
				{
					Raptor_General_Error_Number = 513;
					sprintf(Raptor_General_Error_String,"Raptor_Command_Status:"
						"Failed to get temperature.");
					Raptor_General_Error("command","raptor_command.c","Raptor_Command_Status",
							     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
					Raptor_General_Log("command","raptor_command.c","Raptor_Command_Status",
							   LOG_VERBOSITY_TERSE,"COMMAND","Failed to get temperature.");
#endif
					if(!Raptor_General_Add_String(reply_string,"1 Failed to get temperature."))
						return FALSE;
					return TRUE;
				}
				Raptor_General_Get_Current_Time_String(time_string,31);
				/* diddly
			}
			else
			{
				Detector_Temperature_Get_Cached_Temperature(&temperature,&status_time);
				Raptor_General_Get_Time_String(status_time,time_string,31);
			}
				*/
			sprintf(return_string+strlen(return_string),"%s %.2f",time_string,temperature);
		}
		else if(strncmp(get_set_string,"pcb",6)==0)
		{
			/* diddly 
			if((Raptor_Multrun_In_Progress() == FALSE))
			{
			*/
				if(!Detector_Temperature_PCB_Get(&temperature))
				{
					Raptor_General_Error_Number = 507;
					sprintf(Raptor_General_Error_String,"Raptor_Command_Status:"
						"Failed to get PCB temperature.");
					Raptor_General_Error("command","raptor_command.c","Raptor_Command_Status",
							     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
					Raptor_General_Log("command","raptor_command.c","Raptor_Command_Status",
							   LOG_VERBOSITY_TERSE,"COMMAND","Failed to get PCB temperature.");
#endif
					if(!Raptor_General_Add_String(reply_string,"1 Failed to get PCB temperature."))
						return FALSE;
					return TRUE;
				}
				Raptor_General_Get_Current_Time_String(time_string,31);
				/* diddly
			}
			else
			{
				Detector_Temperature_Get_Cached_PCB_Temperature(&temperature,&status_time);
				Raptor_General_Get_Time_String(status_time,time_string,31);
			}
				*/
			sprintf(return_string+strlen(return_string),"%s %.2f",time_string,temperature);
		}
		else
		{
			Raptor_General_Error_Number = 515;
			sprintf(Raptor_General_Error_String,"Raptor_Command_Status:"
				"Failed to parse temperature command %s from %d.",command_string,command_string_index);
			Raptor_General_Error("command","raptor_command.c","Raptor_Command_Status",
					     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
			Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Status",
						  LOG_VERBOSITY_TERSE,"COMMAND",
						  "Failed to parse temperature command %s from %d.",
						  command_string,command_string_index);
#endif
			if(!Raptor_General_Add_String(reply_string,"1 Failed to parse temperature status command."))
				return FALSE;
			return TRUE;
		}
	}
	else
	{
		Raptor_General_Error_Number = 516;
		sprintf(Raptor_General_Error_String,"Raptor_Command_Status:"
			"Unknown subsystem %s:Failed to parse command %s.",subsystem_string,command_string);
		Raptor_General_Error("command","raptor_command.c","Raptor_Command_Status",
				     LOG_VERBOSITY_TERSE,"COMMAND");
#if RAPTOR_DEBUG > 1
		Raptor_General_Log_Format("command","raptor_command.c","Raptor_Command_Status",LOG_VERBOSITY_TERSE,
					  "COMMAND","Unknown subsystem %s:Failed to parse command %s.",
					  subsystem_string,command_string);
#endif
		if(!Raptor_General_Add_String(reply_string,"1 Failed to parse status command."))
			return FALSE;
		return TRUE;
	}
	/* success */
	if(!Raptor_General_Add_String(reply_string,return_string))
		return FALSE;
#if RAPTOR_DEBUG > 1
	Raptor_General_Log("command","raptor_command.c","Raptor_Command_Status",LOG_VERBOSITY_TERSE,
			   "COMMAND","finished.");
#endif
	return TRUE;
}

/* ----------------------------------------------------------------------------
** 		internal functions 
** ---------------------------------------------------------------------------- */
/**
 * Parse a date of the form "2007-05-03T07:38:48.099 UTC" into number of seconds since 1970 (unix time).
 * @param time_string The string.
 * @param time_secs The address of an integer to store the number of seconds.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #TIMEZONE_OFFSET_BST
 */
static int Command_Parse_Date(char *time_string,int *time_secs)
{
	struct tm time_data;
	int year,month,day,hours,minutes,retval;
	double seconds;
	char timezone_string[16];
	time_t time_in_secs;

	/* check parameters */
	if(time_string == NULL)
	{
		Raptor_General_Error_Number = 528;
		sprintf(Raptor_General_Error_String,"Command_Parse_Date:time_string was NULL.");
		return FALSE;
	}
	if(time_secs == NULL)
	{
		Raptor_General_Error_Number = 529;
		sprintf(Raptor_General_Error_String,"Command_Parse_Date:time_secs was NULL.");
		return FALSE;
	}
#if RAPTOR_DEBUG > 9
	Raptor_General_Log_Format("command","raptor_command.c","Command_Parse_Date",LOG_VERBOSITY_TERSE,
				  "COMMAND","Parsing date/time '%s'.",time_string);
#endif
	/* parse time_string into fields */
	strcpy(timezone_string,"UTC");
	retval = sscanf(time_string,"%d-%d-%d T %d:%d:%lf %15s",&year,&month,&day,
			&hours,&minutes,&seconds,timezone_string);
	if(retval < 6)
	{
		Raptor_General_Error_Number = 530;
		sprintf(Raptor_General_Error_String,
			"Command_Parse_Date:Failed to parse '%s', only parsed %d fields: year=%d,month=%d,day=%d,"
			"hour=%d,minute=%d,second=%.2f,timezone_string=%s.",time_string,retval,year,month,day,
			hours,minutes,seconds,timezone_string);
		return FALSE;
	}
#if RAPTOR_DEBUG > 9
	Raptor_General_Log_Format("command","raptor_command.c","Command_Parse_Date",LOG_VERBOSITY_TERSE,"COMMAND",
			    "Date/time '%s' has year=%d,month=%d,day=%d,hour=%d,minute=%d,seconds=%.2lf,timezone=%s.",
				  time_string,year,month,day,hours,minutes,seconds,timezone_string);
#endif
	/* construct tm */
	time_data.tm_year  = year-1900; /* years since 1900 */
	time_data.tm_mon = month-1; /* 0..11 */
	time_data.tm_mday  = day; /* 1..31 */
	time_data.tm_hour  = hours; /* 0..23 */
	time_data.tm_min   = minutes;
	time_data.tm_sec   = seconds;
	time_data.tm_wday  = 0;
	time_data.tm_yday  = 0;
	time_data.tm_isdst = 0;
	/* BSD extension stuff */
	/*
	time_data.tm_gmtoff = 0;
	time_data.tm_zone = strdup(timezone_string);
	*/
	/* create time in UTC */
	time_in_secs = mktime(&time_data);
	if(time_in_secs < 0)
	{
		Raptor_General_Error_Number = 532;
		sprintf(Raptor_General_Error_String,"Command_Parse_Date:mktime failed.");
		return FALSE;
	}
	(*time_secs) = (int)time_in_secs;
	if(strcmp(timezone_string,"UTC") == 0)
	{
		/* do nothing */
		(*time_secs) = (*time_secs);
	}
	else if(strcmp(timezone_string,"GMT") == 0)
	{
		/* do nothing */
		(*time_secs) = (*time_secs);
	}
	else if(strcmp(timezone_string,"BST") == 0)
	{
		(*time_secs) += TIMEZONE_OFFSET_BST;
	}
	else
	{
		Raptor_General_Error_Number = 531;
		sprintf(Raptor_General_Error_String,"Command_Parse_Date:Unknown timezone '%s'.",timezone_string);
		return FALSE;
	}
	return TRUE;
}
