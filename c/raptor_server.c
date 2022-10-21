/* raptor_server.c
** Raptor server routines
*/
/**
 * Command Server routines for the raptor program.
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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log_udp.h"

#include "command_server.h"

#include "raptor_config.h"
#include "raptor_general.h"
#include "raptor_command.h"
#include "raptor_server.h"

/* internal data */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The server context to use for this server.
 * @see ../command_server/cdocs/command_server.html#Command_Server_Server_Context_T
 */
static Command_Server_Server_Context_T Command_Server_Context = NULL;
/**
 * Command server port number.
 */
static unsigned short Command_Server_Port_Number = 1234;

/* internal functions */
static void Server_Connection_Callback(Command_Server_Handle_T connection_handle);
static int Send_Reply(Command_Server_Handle_T connection_handle,char *reply_message);

/* ----------------------------------------------------------------------------
** 		external functions 
** ---------------------------------------------------------------------------- */
/**
 * Raptor server initialisation routine. Assumes Raptor_Config_Load has previously been called
 * to load the configuration file.
 * It loads the unsigned short with key "command.server.port_number" into the Command_Server_Port_Number variable
 * for use in Raptor_Server_Start.
 * @return The routine returns TRUE if successfull, and FALSE if an error occurs.
 * @see #Raptor_Server_Start
 * @see #Command_Server_Port_Number
 * @see raptor_general.html#Raptor_General_Log
 * @see raptor_general.html#Raptor_General_Log_Format
 * @see raptor_general.html#Raptor_General_Error_Number
 * @see raptor_general.html#Raptor_General_Error_String
 * @see raptor_config.html#Raptor_Config_Get_Unsigned_Short
 */
int Raptor_Server_Initialise(void)
{
	int retval;

#if RAPTOR_DEBUG > 1
	Raptor_General_Log_Format("server","raptor_server.c","Raptor_Server_Initialise",
				  LOG_VERBOSITY_TERSE,"SERVER","started.");
#endif
	/* get port number from config */
	retval = Raptor_Config_Get_Unsigned_Short("command.server.port_number",&Command_Server_Port_Number);
	if(retval == FALSE)
	{
		Raptor_General_Error_Number = 200;
		sprintf(Raptor_General_Error_String,"Failed to find port number in config file.");
		return FALSE;
	}
#if RAPTOR_DEBUG > 1
	Raptor_General_Log_Format("server","raptor_server.c","Raptor_Server_Initialise",
				  LOG_VERBOSITY_TERSE,"SERVER","finished.");
#endif
	return TRUE;
}

/**
 * Raptor server start routine.
 * This routine starts the server. It does not return until the server is stopped using Raptor_Server_Stop.
 * @return The routine returns TRUE if successfull, and FALSE if an error occurs.
 * @see #Command_Server_Port_Number
 * @see #Server_Connection_Callback
 * @see #Command_Server_Context
 * @see raptor_general.html#Raptor_General_Log
 * @see raptor_general.html#Raptor_General_Log_Format
 * @see raptor_general.html#Raptor_General_Error_Number
 * @see raptor_general.html#Raptor_General_Error_String
 * @see ../command_server/cdocs/command_server.html#Command_Server_Start_Server
 */
int Raptor_Server_Start(void)
{
	int retval;

#if RAPTOR_DEBUG > 1
	Raptor_General_Log("server","raptor_server.c","Raptor_Server_Start",
			   LOG_VERBOSITY_VERY_TERSE,"SERVER","started.");
#endif
#if RAPTOR_DEBUG > 2
	Raptor_General_Log_Format("server","raptor_server.c","Raptor_Server_Start",
				      LOG_VERBOSITY_VERY_TERSE,"SERVER",
				      "Starting multi-threaded server on port %hu.",Command_Server_Port_Number);
#endif
	retval = Command_Server_Start_Server(&Command_Server_Port_Number,Server_Connection_Callback,
					     &Command_Server_Context);
	if(retval == FALSE)
	{
		Raptor_General_Error_Number = 201;
		sprintf(Raptor_General_Error_String,"Raptor_Server_Start:"
			"Command_Server_Start_Server returned FALSE.");
		return FALSE;
	}
#if RAPTOR_DEBUG > 1
	Raptor_General_Log("server","raptor_server.c","Raptor_Server_Start",
			   LOG_VERBOSITY_VERY_TERSE,"SERVER","finished.");
#endif
	return TRUE;
}

/**
 * Raptor server stop routine.
 * @return The routine returns TRUE if successfull, and FALSE if an error occurs.
 * @see #Command_Server_Context
 * @see raptor_general.html#Raptor_General_Log
 * @see raptor_general.html#Raptor_General_Log_Format
 * @see ../command_server/cdocs/command_server.html#Command_Server_Close_Server
 */
int Raptor_Server_Stop(void)
{
	int retval;
#if RAPTOR_DEBUG > 1
	Raptor_General_Log_Format("server","raptor_server.c","Raptor_Server_Stop",
				      LOG_VERBOSITY_VERY_TERSE,"SERVER","started.");
#endif
	retval = Command_Server_Close_Server(&Command_Server_Context);
	if(retval == FALSE)
	{
		Raptor_General_Error_Number = 202;
		sprintf(Raptor_General_Error_String,"Raptor_Server_Stop:"
			"Command_Server_Close_Server returned FALSE.");
		return FALSE;
	}
#if RAPTOR_DEBUG > 1
	Raptor_General_Log_Format("server","raptor_server.c","Raptor_Server_Stop",
				  LOG_VERBOSITY_VERY_TERSE,"SERVER","finished.");
#endif
	return TRUE;
}

/* ----------------------------------------------------------------------------
** 		internal functions 
** ---------------------------------------------------------------------------- */
/**
 * Server connection thread, invoked whenever a new command comes in.
 * @param connection_handle Connection handle for this thread.
 * @see #Send_Reply
 * @see #Raptor_Server_Stop
 * @see raptor_command.html#Raptor_Command_Abort
 * @see raptor_command.html#Raptor_Command_Config
 * @see raptor_command.html#Raptor_Command_Fits_Header
 * @see raptor_command.html#Raptor_Command_Multrun
 * @see raptor_command.html#Raptor_Command_MultBias
 * @see raptor_command.html#Raptor_Command_MultDark
 * @see raptor_command.html#Raptor_Command_Status
 * @see raptor_general.html#Raptor_General_Error_Number
 * @see raptor_general.html#Raptor_General_Error_String
 * @see raptor_general.html#Raptor_General_Log_Format
 * @see raptor_general.html#Raptor_General_Thread_Priority_Set_Normal
 * @see raptor_general.html#Raptor_General_Thread_Priority_Set_Exposure
 * @see ../command_server/cdocs/command_server.html#Command_Server_Read_Message
 */
static void Server_Connection_Callback(Command_Server_Handle_T connection_handle)
{
	void *buffer_ptr = NULL;
	size_t buffer_length = 0;
	char *reply_string = NULL;
	char *client_message = NULL;
	int retval;
	int seconds,i;

	/* get message from client */
	retval = Command_Server_Read_Message(connection_handle, &client_message);
	if(retval == FALSE)
	{
		Raptor_General_Error_Number = 203;
		sprintf(Raptor_General_Error_String,"Raptor_Server_Connection_Callback:"
			"Failed to read message.");
		Raptor_General_Error("server","raptor_server.c","Raptor_Server_Connection_Callback",
					 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		return;
	}
#if RAPTOR_DEBUG > 1
	Raptor_General_Log_Format("server","raptor_server.c","Raptor_Server_Connection_Callback",
				      LOG_VERBOSITY_VERY_TERSE,"SERVER","received '%s'",client_message);
#endif
	/* do something with message */
	if(strncmp(client_message,"abort",5) == 0)
	{
#if RAPTOR_DEBUG > 1
		Raptor_General_Log("server","raptor_server.c","Raptor_Server_Connection_Callback",
				       LOG_VERBOSITY_VERY_TERSE,"SERVER","abort detected.");
#endif
		/* exposure thread priority */
		if(!Raptor_General_Thread_Priority_Set_Exposure())
		{
			Raptor_General_Error("server","raptor_server.c",
						 "Raptor_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		retval = Raptor_Command_Abort(client_message,&reply_string);
		if(retval == TRUE)
		{
			retval = Send_Reply(connection_handle,reply_string);
			if(reply_string != NULL)
				free(reply_string);
			if(retval == FALSE)
			{
				Raptor_General_Error("server","raptor_server.c",
							 "Raptor_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
		else
		{
			Raptor_General_Error("server","raptor_server.c",
						 "Raptor_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			retval = Send_Reply(connection_handle, "1 Raptor_Command_Abort failed.");
			if(retval == FALSE)
			{
				Raptor_General_Error("server","raptor_server.c",
						     "Raptor_Server_Connection_Callback",
						     LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
	}
	else if(strncmp(client_message,"config",6) == 0)
	{
#if RAPTOR_DEBUG > 1
		Raptor_General_Log("server","raptor_server.c","Raptor_Server_Connection_Callback",
				       LOG_VERBOSITY_VERY_TERSE,"SERVER","config detected.");
#endif
		/* normal thread priority */
		if(!Raptor_General_Thread_Priority_Set_Normal())
		{
			Raptor_General_Error("server","raptor_server.c",
						 "Raptor_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		retval = Raptor_Command_Config(client_message,&reply_string);
		if(retval == TRUE)
		{
			retval = Send_Reply(connection_handle,reply_string);
			if(reply_string != NULL)
				free(reply_string);
			if(retval == FALSE)
			{
				Raptor_General_Error("server","raptor_server.c",
							 "Raptor_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
		else
		{
			Raptor_General_Error("server","raptor_server.c",
					     "Raptor_Server_Connection_Callback",
					     LOG_VERBOSITY_VERY_TERSE,"SERVER");
			retval = Send_Reply(connection_handle, "1 Raptor_Command_Config failed.");
			if(retval == FALSE)
			{
				Raptor_General_Error("server","raptor_server.c",
						     "Raptor_Server_Connection_Callback",
						     LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
	}
	else if(strncmp(client_message,"fitsheader",10) == 0)
	{
#if RAPTOR_DEBUG > 1
		Raptor_General_Log("server","raptor_server.c","Raptor_Server_Connection_Callback",
				       LOG_VERBOSITY_VERY_TERSE,"SERVER","fitsheader detected.");
#endif
		/* normal thread priority */
		if(!Raptor_General_Thread_Priority_Set_Normal())
		{
			Raptor_General_Error("server","raptor_server.c",
					     "Raptor_Server_Connection_Callback",
					     LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		retval = Raptor_Command_Fits_Header(client_message,&reply_string);
		if(retval == TRUE)
		{
			retval = Send_Reply(connection_handle,reply_string);
			if(reply_string != NULL)
				free(reply_string);
			if(retval == FALSE)
			{
				Raptor_General_Error("server","raptor_server.c",
						     "Raptor_Server_Connection_Callback",
						     LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
		else
		{
			Raptor_General_Error("server","raptor_server.c",
						 "Raptor_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			retval = Send_Reply(connection_handle, "1 Raptor_Command_Fits_Header failed.");
			if(retval == FALSE)
			{
				Raptor_General_Error("server","raptor_server.c",
						     "Raptor_Server_Connection_Callback",
						     LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
	}
	else if(strcmp(client_message, "help") == 0)
	{
#if RAPTOR_DEBUG > 1
		Raptor_General_Log("server","raptor_server.c","Raptor_Server_Connection_Callback",
				   LOG_VERBOSITY_VERY_TERSE,"SERVER","help detected.");
#endif
		/* normal thread priority */
		if(!Raptor_General_Thread_Priority_Set_Normal())
		{
			Raptor_General_Error("server","raptor_server.c",
						 "Raptor_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		Send_Reply(connection_handle, "help:\n"
			   "\tabort\n"
			   "\tconfig filter <filter_name>\n"
			   "\tconfig coadd_exp_len <short|long>\n"
			   "\tconfig nudgematic <small|large>\n"
			   "\tfitsheader add <keyword> <boolean|float|integer|string|comment|units> <value>\n"
			   "\tfitsheader delete <keyword>\n"
			   "\tfitsheader clear\n"
			   "\thelp\n"
			   "\tmultbias <count>\n"
			   "\tmultdark <length> <count>\n"
			   "\tmultrun <length> <count> <standard>\n"
			   "\tstatus [name|identification|fits_instrument_code]\n"
			   "\tstatus temperature [get|pcb]\n"
			   "\tstatus filterwheel [filter|position|status]\n"
			   "\tstatus nudgematic [position|status]\n"
			   "\tstatus exposure [status|count|length|start_time]\n"
			   "\tstatus exposure [index|multrun|run]\n"
			   "\tshutdown\n");
	}
	else if(strncmp(client_message,"multbias",8) == 0)
	{
#if RAPTOR_DEBUG > 1
		Raptor_General_Log("server","raptor_server.c","Raptor_Server_Connection_Callback",
				       LOG_VERBOSITY_VERY_TERSE,"SERVER","multbias detected.");
#endif
		/* exposure thread priority */
		if(!Raptor_General_Thread_Priority_Set_Exposure())
		{
			Raptor_General_Error("server","raptor_server.c",
						 "Raptor_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		/* diddly
		retval = Raptor_Command_MultBias(client_message,&reply_string);
		if(retval == TRUE)
		{
			retval = Send_Reply(connection_handle,reply_string);
			if(reply_string != NULL)
				free(reply_string);
			if(retval == FALSE)
			{
				Raptor_General_Error("server","raptor_server.c",
							 "Raptor_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
		else
		{
			Raptor_General_Error("server","raptor_server.c",
						 "Raptor_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			retval = Send_Reply(connection_handle, "1 Raptor_Command_MultBias failed.");
			if(retval == FALSE)
			{
				Raptor_General_Error("server","raptor_server.c",
							 "Raptor_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
		*/
	}
	else if(strncmp(client_message,"multdark",8) == 0)
	{
#if RAPTOR_DEBUG > 1
		Raptor_General_Log("server","raptor_server.c","Raptor_Server_Connection_Callback",
				       LOG_VERBOSITY_VERY_TERSE,"SERVER","multdark detected.");
#endif
		/* exposure thread priority */
		if(!Raptor_General_Thread_Priority_Set_Exposure())
		{
			Raptor_General_Error("server","raptor_server.c",
						 "Raptor_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		/* diddly
		retval = Raptor_Command_MultDark(client_message,&reply_string);
		if(retval == TRUE)
		{
			retval = Send_Reply(connection_handle,reply_string);
			if(reply_string != NULL)
				free(reply_string);
			if(retval == FALSE)
			{
				Raptor_General_Error("server","raptor_server.c",
							 "Raptor_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
		else
		{
			Raptor_General_Error("server","raptor_server.c",
						 "Raptor_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			retval = Send_Reply(connection_handle, "1 Raptor_Command_MultDark failed.");
			if(retval == FALSE)
			{
				Raptor_General_Error("server","raptor_server.c",
							 "Raptor_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
		*/
	}
	else if(strncmp(client_message,"multrun",7) == 0)
	{
#if RAPTOR_DEBUG > 1
		Raptor_General_Log("server","raptor_server.c","Raptor_Server_Connection_Callback",
				       LOG_VERBOSITY_VERY_TERSE,"SERVER","multrun detected.");
#endif
		/* exposure thread priority */
		if(!Raptor_General_Thread_Priority_Set_Exposure())
		{
			Raptor_General_Error("server","raptor_server.c",
						 "Raptor_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		retval = Raptor_Command_Multrun(client_message,&reply_string);
		if(retval == TRUE)
		{
			retval = Send_Reply(connection_handle,reply_string);
			if(reply_string != NULL)
				free(reply_string);
			if(retval == FALSE)
			{
				Raptor_General_Error("server","raptor_server.c",
							 "Raptor_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
		else
		{
			Raptor_General_Error("server","raptor_server.c",
						 "Raptor_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			retval = Send_Reply(connection_handle, "1 Raptor_Command_Multrun failed.");
			if(retval == FALSE)
			{
				Raptor_General_Error("server","raptor_server.c",
							 "Raptor_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
	}
	else if(strncmp(client_message,"status",6) == 0)
	{
#if RAPTOR_DEBUG > 1
		Raptor_General_Log("server","raptor_server.c","Raptor_Server_Connection_Callback",
				       LOG_VERBOSITY_VERY_TERSE,"SERVER","status detected.");
#endif
		/* normal thread priority */
		if(!Raptor_General_Thread_Priority_Set_Normal())
		{
			Raptor_General_Error("server","raptor_server.c","Raptor_Server_Connection_Callback",
			                     LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		retval = Raptor_Command_Status(client_message,&reply_string);
		if(retval == TRUE)
		{
			retval = Send_Reply(connection_handle,reply_string);
			if(reply_string != NULL)
				free(reply_string);
			if(retval == FALSE)
			{
				Raptor_General_Error("server","raptor_server.c",
							 "Raptor_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
		else
		{
			Raptor_General_Error("server","raptor_server.c",
						 "Raptor_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			retval = Send_Reply(connection_handle, "1 Raptor_Command_Status failed.");
			if(retval == FALSE)
			{
				Raptor_General_Error("server","raptor_server.c",
							 "Raptor_Server_Connection_Callback",
							 LOG_VERBOSITY_VERY_TERSE,"SERVER");
			}
		}
	}
	else if(strcmp(client_message, "shutdown") == 0)
	{
#if RAPTOR_DEBUG > 1
		Raptor_General_Log("server","raptor_server.c","Raptor_Server_Connection_Callback",
				       LOG_VERBOSITY_VERY_TERSE,"SERVER","shutdown detected:about to stop.");
#endif
		/* normal thread priority */
		if(!Raptor_General_Thread_Priority_Set_Normal())
		{
			Raptor_General_Error("server","raptor_server.c",
						 "Raptor_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
		retval = Send_Reply(connection_handle, "0 ok");
		if(retval == FALSE)
			Raptor_General_Error("server","raptor_server.c",
						 "Raptor_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		retval = Raptor_Server_Stop();
		if(retval == FALSE)
		{
			Raptor_General_Error("server","raptor_server.c",
						 "Raptor_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
	}
	else
	{
#if RAPTOR_DEBUG > 1
		Raptor_General_Log_Format("server","raptor_server.c","Raptor_Server_Connection_Callback",
					      LOG_VERBOSITY_VERY_TERSE,"SERVER","message unknown: '%s'\n",
					      client_message);
#endif
		retval = Send_Reply(connection_handle, "1 failed message unknown");
		if(retval == FALSE)
		{
			Raptor_General_Error("server","raptor_server.c",
						 "Raptor_Server_Connection_Callback",
						 LOG_VERBOSITY_VERY_TERSE,"SERVER");
		}
	}
	/* free message */
	free(client_message);
}

/**
 * Send a message back to the client.
 * @param connection_handle The command server connection handle for this thread.
 * @param reply_message The message to send.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see raptor_general.html#Raptor_General_Error_Number
 * @see raptor_general.html#Raptor_General_Error_String
 * @see raptor_general.html#Raptor_General_Log_Format
 * @see ../command_server/cdocs/command_server.html#Command_Server_Write_Message
 */
static int Send_Reply(Command_Server_Handle_T connection_handle,char *reply_message)
{
	int retval;

	/* send something back to the client */
#if RAPTOR_DEBUG > 5
	Raptor_General_Log_Format("server","raptor_server.c","Send_Reply",LOG_VERBOSITY_TERSE,"SERVER",
				      "about to send '%.80s'...",reply_message);
#endif
	retval = Command_Server_Write_Message(connection_handle, reply_message);
	if(retval == FALSE)
	{
		Raptor_General_Error_Number = 204;
		sprintf(Raptor_General_Error_String,"Send_Reply:"
			"Writing message to connection failed.");
		return FALSE;
	}
#if RAPTOR_DEBUG > 5
	Raptor_General_Log_Format("server","raptor_server.c","Send_Reply",LOG_VERBOSITY_TERSE,"SERVER",
				      "sent '%.80s'...",reply_message);
#endif
	return TRUE;
}

