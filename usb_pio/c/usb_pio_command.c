/* usb_pio_command.c 
** USB-PIO IO board communication library : routines to send i/o commands to the board.
*/
/**
 * Routines to send i/o commands to the USB-PIO IO board.
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
#include "log_udp.h"
#include "usb_pio_general.h"
#include "usb_pio_command.h"
#include "usb_pio_connection.h"

/* hash defines */
/**
 * Which BMCM OR8 port has been configured for output.
 */
#define OUTPUT_PORT                  (0)
/**
 * Which BMCM OR8 port has been configured for input.
 */
#define INPUT_PORT                   (1)
/**
 * Maximum length of commands/replies sent to the USB-PIO board.
 */
#define COMMAND_STRING_LENGTH        (32)

/* macros */
/**
 * Macro to determine whether the specified port type is valid.
 * @param n The port type, should be one of USB_PIO_PORT_TYPE_OUTPUT or USB_PIO_PORT_TYPE_INPUT to be valid.
 * @see #USB_PIO_PORT_TYPE
 */
#define COMMAND_PORT_TYPE_IS_VALID(n) ((n == USB_PIO_PORT_TYPE_OUTPUT)||(n == USB_PIO_PORT_TYPE_INPUT))
/**
 * Macro to determine whether the specified port is valid.
 * @param n The port, should be one of 0 (=A), 1 (=B) or 2 (=C) to be valid.
 */
#define COMMAND_PORT_IS_VALID(n) ((n == 0)||(n == 1)||(n == 2))

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";

/**
 * Variable holding error code of last operation performed.
 */
static int Command_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see usb_pio_general.html#USB_PIO_GENERAL_ERROR_STRING_LENGTH
 */
static char Command_Error_String[USB_PIO_GENERAL_ERROR_STRING_LENGTH] = "";

/* =======================================
**  external functions 
** ======================================= */
int USB_PIO_Command_Output_Set(int output,int onoff)
{
	/* diddly */
	return TRUE;
}

/**
 * Set the outputs on the BMCM OR8 I/O board
 * @param outputs An unsigned char representing the output bits to turn on.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #USB_PIO_PORT_TYPE
 * @see #OUTPUT_PORT
 * @see #COMMAND_STRING_LENGTH
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see usb_pio_connection.html#USB_PIO_Connection_Command
 */
int USB_PIO_Command_Outputs_Set(unsigned char outputs)
{
	char command_string[COMMAND_STRING_LENGTH];
	char expected_reply_string[COMMAND_STRING_LENGTH];
	char reply_string[COMMAND_STRING_LENGTH];

	Command_Error_Number = 1;
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				   "USB_PIO_Command_Outputs_Set(outputs=%2.2X): Started.",outputs);
#endif /* LOGGING */
	/* set output port to output */
	if(!USB_PIO_Command_Port_Set(OUTPUT_PORT,USB_PIO_PORT_TYPE_OUTPUT))
		return FALSE;
	/* set outputs */
	sprintf(command_string,"@00P%d%2.2X",OUTPUT_PORT,outputs);
	sprintf(expected_reply_string,"!00%2.2X",outputs);
	if(!USB_PIO_Connection_Command(command_string,expected_reply_string,reply_string,COMMAND_STRING_LENGTH))
		return FALSE;
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				   "USB_PIO_Command_Outputs_Set(outputs=%2.2X): Finished.",outputs);
#endif /* LOGGING */
	return TRUE;
}

/**
 * Get the outputs currently turned on on the BMCM OR8 I/O board.
 * @param outputs The address of an unsigned char, on a successful this is filled in with a bitwise representation
 *        of which outputs are currently turned on.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #USB_PIO_PORT_TYPE
 * @see #OUTPUT_PORT
 * @see #COMMAND_STRING_LENGTH
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see usb_pio_connection.html#USB_PIO_Connection_Command
 */
int USB_PIO_Command_Outputs_Get(unsigned char *outputs)
{
	char command_string[COMMAND_STRING_LENGTH];
	char expected_reply_string[COMMAND_STRING_LENGTH];
	char reply_string[COMMAND_STRING_LENGTH];

	Command_Error_Number = 0;
	if(outputs == NULL)
	{
		Command_Error_Number = 1;
		sprintf(Command_Error_String,"USB_PIO_Command_Outputs_Get: outputs is NULL.");
		return FALSE;
	}
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"USB_PIO_Command_Outputs_Get: Started.");
#endif /* LOGGING */
	/* set output port to output */
	if(!USB_PIO_Command_Port_Set(OUTPUT_PORT,USB_PIO_PORT_TYPE_OUTPUT))
		return FALSE;
	/* set outputs query command */
	sprintf(command_string,"@00P%d?",OUTPUT_PORT);
	strcpy(expected_reply_string,"!00");
	if(!USB_PIO_Connection_Command(command_string,expected_reply_string,reply_string,COMMAND_STRING_LENGTH))
		return FALSE;
	/* parse reply_string - hex representation of the outputs should be in character 3 i.e. '!00x' */
	(*outputs) = strtol( &reply_string[3], NULL, 16);
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				   "USB_PIO_Command_Outputs_Get(outputs=%2.2X): Finished.",(*outputs));
#endif /* LOGGING */
	return TRUE;
}

int USB_PIO_Command_Output_Get(int output,int *onoff)
{
	/* diddly */
	return TRUE;
}

/**
 * Get the inputs from the BMCM OR8 I/O board.
 * @param inputs The address of an unsigned char, on a successful invocation this is filled in with a 
 *        bitwise representation of which inputs are currently reading on (high).
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #USB_PIO_PORT_TYPE
 * @see #INPUT_PORT
 * @see #COMMAND_STRING_LENGTH
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see usb_pio_connection.html#USB_PIO_Connection_Command
 */
int USB_PIO_Command_Inputs_Get(unsigned char *inputs)
{
	char command_string[COMMAND_STRING_LENGTH];
	char expected_reply_string[COMMAND_STRING_LENGTH];
	char reply_string[COMMAND_STRING_LENGTH];

	Command_Error_Number = 0;
	if(inputs == NULL)
	{
		Command_Error_Number = 2;
		sprintf(Command_Error_String,"USB_PIO_Command_Inputs_Get: inputs is NULL.");
		return FALSE;
	}
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"USB_PIO_Command_Inputs_Get: Started.");
#endif /* LOGGING */
	/* set input port to input */
	if(!USB_PIO_Command_Port_Set(INPUT_PORT,USB_PIO_PORT_TYPE_INPUT))
		return FALSE;
	/* set inputs query command */
	sprintf(command_string,"@00P%d?",INPUT_PORT);
	strcpy(expected_reply_string,"!00");
	if(!USB_PIO_Connection_Command(command_string,expected_reply_string,reply_string,COMMAND_STRING_LENGTH))
		return FALSE;
	/* parse reply_string - hex representation of the outputs should be in character 3 i.e. '!00x' */
	(*inputs) = strtol( &reply_string[3], NULL, 16);
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				   "USB_PIO_Command_Inputs_Get(inputs=%2.2X): Finished.",(*inputs));
#endif /* LOGGING */
	return TRUE;
}

int USB_PIO_Command_Input_Get(int input,int *onoff)
{
	/* diddly */
	return TRUE;
}

/**
 * Set the specified USB-PIO port to be an input or output port. The BMCM USB-PIO has 3 ports (0..2), the
 * BMCM OR8 I/O board has one port with 8 inputs, and one port with eight outputs. These can be re-configured 
 * by moving ICs U10-U16 around. As delivered the BMCM OR8 I/O board has port A (=0) configured as outputs and
 * port B (=1) configured as inputs.
 * @param port The port to reconfigure, port A (=0) or port B (=1), or port C (=2).
 * @param port_type Whether to configure the I/O port as an input or output.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #USB_PIO_PORT_TYPE
 * @see #COMMAND_STRING_LENGTH
 * @see #COMMAND_PORT_TYPE_IS_VALID
 * @see #COMMAND_PORT_IS_VALID
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see usb_pio_connection.html#USB_PIO_Connection_Command
 */
int USB_PIO_Command_Port_Set(int port,enum USB_PIO_PORT_TYPE port_type)
{
	char command_string[COMMAND_STRING_LENGTH];
	char expected_reply_string[COMMAND_STRING_LENGTH];
	char reply_string[COMMAND_STRING_LENGTH];
	int retval;
	
	Command_Error_Number = 0;
	if(!COMMAND_PORT_IS_VALID(port))
	{
		Command_Error_Number = 3;
		sprintf(Command_Error_String,"USB_PIO_Command_Port_Set: Illegal port %d.",port);
		return FALSE;
	}
	if(!COMMAND_PORT_TYPE_IS_VALID(port_type))
	{
		Command_Error_Number = 4;
		sprintf(Command_Error_String,"USB_PIO_Command_Port_Set: Illegal port_type %d.",port_type);
		return FALSE;
	}
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				   "USB_PIO_Command_Port_Set(port=%d,port_type=%d): Started.",port,port_type);
#endif /* LOGGING */
	sprintf(command_string,"@00D%d%2.2X",port,port_type);
	sprintf(expected_reply_string,"!00%2.2X",port_type);
	if(!USB_PIO_Connection_Command(command_string,expected_reply_string,reply_string,COMMAND_STRING_LENGTH))
		return FALSE;
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				   "USB_PIO_Command_Port_Set(port=%d,port_type=%d): Finished.",port,port_type);
#endif /* LOGGING */
	return TRUE;
}

int USB_PIO_Command_Port_Get(int port,enum USB_PIO_PORT_TYPE *port_type)
{
	/* diddly */
	return TRUE;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Command_Error_Number
 */
int USB_PIO_Command_Get_Error_Number(void)
{
	return Command_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see usb_pio_general.html#USB_PIO_General_Get_Current_Time_String
 */
extern void USB_PIO_Command_Error(void)
{
	char time_string[32];

	USB_PIO_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Command_Error_Number == 0)
		sprintf(Command_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s USB_PIO_Command:Error(%d) : %s\n",time_string,
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
 * @see usb_pio_general.html#USB_PIO_General_Get_Current_Time_String
 */
extern void USB_PIO_Command_Error_String(char *error_string)
{
	char time_string[32];

	USB_PIO_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Command_Error_Number == 0)
		sprintf(Command_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s USB_PIO_Command:Error(%d) : %s\n",time_string,
		Command_Error_Number,Command_Error_String);
}

