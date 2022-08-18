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
 * @return The macro returns TRUE if the port type is valid, and FALSE if it is NOT valid.
 * @see #USB_PIO_PORT_TYPE
 */
#define COMMAND_PORT_TYPE_IS_VALID(n) ((n == USB_PIO_PORT_TYPE_OUTPUT)||(n == USB_PIO_PORT_TYPE_INPUT))
/**
 * Macro to determine whether the specified port is valid.
 * @param n The port, should be one of 0 (=A), 1 (=B) or 2 (=C) to be valid.
 * @return The macro returns TRUE if the port is valid, and FALSE if it is NOT valid.
 */
#define COMMAND_PORT_IS_VALID(n) ((n == 0)||(n == 1)||(n == 2))
/**
 * Macro to determine whether the specified input is valid.
 * @param valus The input number, should be between 1 and 8 to be valid (as marked on the board). The bit to be
 *        set when communicating with the board is one less than this i.e. 0..7.
 * @return The macro returns TRUE if the port is valid, and FALSE if it is NOT valid.
 */
#define COMMAND_INPUT_IS_VALID(value)	(((value) > 0)&&((value) < 9))
/**
 * Macro to determine whether the specified output is valid.
 * @param valus The output number, should be between 1 and 8 to be valid (as marked on the board). The bit to be
 *        set when communicating with the board is one less than this i.e. 0..7.
 * @return The macro returns TRUE if the port is valid, and FALSE if it is NOT valid.
 */
#define COMMAND_OUTPUT_IS_VALID(value)	(((value) > 0)&&((value) < 9))

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
/**
 * Set the specified output on the BMCM OR8 I/O board to be on or off.
 * We retieve the current outpus using USB_PIO_Command_Outputs_Get, set or clear the relevant output bit
 * in the outputs, and write the new value back to the board using USB_PIO_Command_Outputs_Set.
 * @param output Which output to modify, from 1..8 (as marked on the board). The bit to be
 *        set when communicating with the board is one less than this i.e. 0..7.
 * @param onoff A boolean, TRUE turns the specified output on, FALSE turns it off.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #COMMAND_OUTPUT_IS_VALID
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #USB_PIO_Command_Outputs_Set
 * @see #USB_PIO_Command_Outputs_Get
 */
int USB_PIO_Command_Output_Set(int output,int onoff)
{
	int output_bit;
	unsigned char outputs;
	
	Command_Error_Number = 0;
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				   "USB_PIO_Command_Output_Set(output=%d,onoff=%d): Started.",output,onoff);
#endif /* LOGGING */
	if(!COMMAND_OUTPUT_IS_VALID(output))
	{
		Command_Error_Number = 5;
		sprintf(Command_Error_String,"USB_PIO_Command_Output_Set: output '%d' is invalid.",output);
		return FALSE;
	}
	if(!USB_PIO_IS_BOOLEAN(onoff))
	{
		Command_Error_Number = 6;
		sprintf(Command_Error_String,"USB_PIO_Command_Output_Set: onoff '%d' is not a boolean.",onoff);
		return FALSE;
	}
	/* get the current outputs */
	if(!USB_PIO_Command_Outputs_Get(&outputs))
		return FALSE;
	/* output_bit is one less than the output number i.e. 1..8 -> 0..7 */
	output_bit = output - 1;
	if(onoff)
	{
		/* turn output_bit on */
		outputs |= (1<<output_bit);
	}
	else
	{
		/* turn output_bit off */
		outputs &= ~(1<<output_bit);
	}
	/* set the new output bits */
	if(!USB_PIO_Command_Outputs_Set(outputs))
		return FALSE;
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				   "USB_PIO_Command_Output_Set(output=%d,onoff=%d): Finished.",output,onoff);
#endif /* LOGGING */
	return TRUE;
}

/**
 * Set the outputs on the BMCM OR8 I/O board
 * @param outputs An unsigned char representing the output bits to turn on. This should be in the range 0..255.
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
	char reply_string[COMMAND_STRING_LENGTH];

	Command_Error_Number = 0;
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				   "USB_PIO_Command_Outputs_Set(outputs=%2.2X): Started.",outputs);
#endif /* LOGGING */
	/* set output port to output */
#if LOGGING > 5
	USB_PIO_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"USB_PIO_Command_Outputs_Set: Set port %d to output %#x.",
				   OUTPUT_PORT,USB_PIO_PORT_TYPE_OUTPUT);
#endif /* LOGGING */
	if(!USB_PIO_Command_Port_Set(OUTPUT_PORT,USB_PIO_PORT_TYPE_OUTPUT))
		return FALSE;
	/* set outputs */
#if LOGGING > 5
	USB_PIO_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"USB_PIO_Command_Outputs_Get: Set output port %d to %2.2X.",
				   OUTPUT_PORT,outputs);
#endif /* LOGGING */
	sprintf(command_string,"@00P%d%2.2X",OUTPUT_PORT,outputs);
	if(!USB_PIO_Connection_Command(command_string,"!00",reply_string,COMMAND_STRING_LENGTH))
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
#if LOGGING > 5
	USB_PIO_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"USB_PIO_Command_Outputs_Get: Set port %d to output %#x.",
				   OUTPUT_PORT,USB_PIO_PORT_TYPE_OUTPUT);
#endif /* LOGGING */
	if(!USB_PIO_Command_Port_Set(OUTPUT_PORT,USB_PIO_PORT_TYPE_OUTPUT))
		return FALSE;
	/* get outputs query command */
#if LOGGING > 5
	USB_PIO_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"USB_PIO_Command_Outputs_Get: Query output port %d.",
				   OUTPUT_PORT);
#endif /* LOGGING */
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

/**
 * Routine to get whether the specified output is on or off.
 * @param output Which output to query, from 1..8 (as marked on the board). The bit to be
 *        set when communicating with the board is one less than this i.e. 0..7.
 * @param onoff The address of an integer, on return from a successful invocation this will contain a boolean,
 *        TRUE if the specified output was on, and FALSE if the specified output was off.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #COMMAND_OUTPUT_IS_VALID
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #USB_PIO_Command_Outputs_Get
 */
int USB_PIO_Command_Output_Get(int output,int *onoff)
{
	int output_bit;
	unsigned char outputs;
	
	Command_Error_Number = 0;
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				   "USB_PIO_Command_Output_Get(output=%d): Started.",output);
#endif /* LOGGING */
	if(onoff == NULL)
	{
		Command_Error_Number = 7;
		sprintf(Command_Error_String,"USB_PIO_Command_Output_Get: onoff was NULL.");
		return FALSE;
	}
	if(!COMMAND_OUTPUT_IS_VALID(output))
	{
		Command_Error_Number = 8;
		sprintf(Command_Error_String,"USB_PIO_Command_Output_Get: output '%d' is invalid.",output);
		return FALSE;
	}
	/* get the current outputs */
	if(!USB_PIO_Command_Outputs_Get(&outputs))
		return FALSE;
	/* output_bit is one less than the output number i.e. 1..8 -> 0..7 */
	output_bit = output - 1;
	/* check whether the specified output is set in the outputs bits, and set onoff accordingly */
	if((outputs&(1<<output_bit)) > 0)
		(*onoff) = TRUE;
	else
		(*onoff) = FALSE;
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				   "USB_PIO_Command_Output_Get(output=%d): Returned.",output,(*onoff));
#endif /* LOGGING */
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
#if LOGGING > 5
	USB_PIO_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"USB_PIO_Command_Inputs_Get: Set port %d to input %#x.",
				   INPUT_PORT,USB_PIO_PORT_TYPE_INPUT);
#endif /* LOGGING */
	if(!USB_PIO_Command_Port_Set(INPUT_PORT,USB_PIO_PORT_TYPE_INPUT))
		return FALSE;
	/* get inputs query command */
#if LOGGING > 5
	USB_PIO_General_Log_Format(LOG_VERBOSITY_VERY_VERBOSE,"USB_PIO_Command_Inputs_Get: Query input port %d.",
				   INPUT_PORT);
#endif /* LOGGING */
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

/**
 * Routine to get whether the specified input is on or off.
 * @param input Which input to query, from 1..8 (as marked on the board). The bit to be
 *        set when communicating with the board is one less than this i.e. 0..7.
 * @param onoff The address of an integer, on return from a successful invocation this will contain a boolean,
 *        TRUE if the specified input was on (high), and FALSE if the specified input was off (low).
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #COMMAND_INPUT_IS_VALID
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see #USB_PIO_Command_Inputs_Get
 */
int USB_PIO_Command_Input_Get(int input,int *onoff)
{
	int input_bit;
	unsigned char inputs;
	
	Command_Error_Number = 0;
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				   "USB_PIO_Command_Input_Get(input=%d): Started.",input);
#endif /* LOGGING */
	if(onoff == NULL)
	{
		Command_Error_Number = 9;
		sprintf(Command_Error_String,"USB_PIO_Command_Input_Get: onoff was NULL.");
		return FALSE;
	}
	if(!COMMAND_INPUT_IS_VALID(input))
	{
		Command_Error_Number = 10;
		sprintf(Command_Error_String,"USB_PIO_Command_Input_Get: input '%d' is invalid.",input);
		return FALSE;
	}
	/* get the current inputs */
	if(!USB_PIO_Command_Inputs_Get(&inputs))
		return FALSE;
	/* input_bit is one less than the input number i.e. 1..8 -> 0..7 */
	input_bit = input - 1;
	/* check whether the specified input is set in the inputs bits, and set onoff accordingly */
	if((inputs&(1<<input_bit)) > 0)
		(*onoff) = TRUE;
	else
		(*onoff) = FALSE;
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				   "USB_PIO_Command_Input_Get(input=%d): Returned.",input,(*onoff));
#endif /* LOGGING */
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
	/* it appears a port set just returns "!00", this is different to the documentation */
	if(!USB_PIO_Connection_Command(command_string,"!00",reply_string,COMMAND_STRING_LENGTH))
		return FALSE;
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				   "USB_PIO_Command_Port_Set(port=%d,port_type=%d): Finished.",port,port_type);
#endif /* LOGGING */
	return TRUE;
}

/**
 * Find out whether the specified USB-PIO port is currently set to be an input or output port. 
 * The BMCM USB-PIO has 3 ports (0..2), the
 * BMCM OR8 I/O board has one port with 8 inputs, and one port with eight outputs. These can be re-configured 
 * by moving ICs U10-U16 around. As delivered the BMCM OR8 I/O board has port A (=0) configured as outputs and
 * port B (=1) configured as inputs.
 * @param port The port to query, port A (=0) or port B (=1), or port C (=2).
 * @param port_type The address of a USB_PIO_PORT_TYPE variable, on a successful invocation this is filled in with the 
 *                  the ports type: input or output.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #USB_PIO_PORT_TYPE
 * @see #COMMAND_STRING_LENGTH
 * @see #COMMAND_PORT_TYPE_IS_VALID
 * @see #COMMAND_PORT_IS_VALID
 * @see #Command_Error_Number
 * @see #Command_Error_String
 * @see usb_pio_connection.html#USB_PIO_Connection_Command
 */
int USB_PIO_Command_Port_Get(int port,enum USB_PIO_PORT_TYPE *port_type)
{
	char command_string[COMMAND_STRING_LENGTH];
	char reply_string[COMMAND_STRING_LENGTH];
	int retval;
	
	Command_Error_Number = 0;
	if(!COMMAND_PORT_IS_VALID(port))
	{
		Command_Error_Number = 11;
		sprintf(Command_Error_String,"USB_PIO_Command_Port_Get: Illegal port %d.",port);
		return FALSE;
	}
	if(port_type == NULL)
	{
		Command_Error_Number = 12;
		sprintf(Command_Error_String,"USB_PIO_Command_Port_Get: port_type was NULL.");
		return FALSE;
	}
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				   "USB_PIO_Command_Port_Get(port=%d): Started.",port);
#endif /* LOGGING */
	/* query port type */
	sprintf(command_string,"@00D%d?",port);
	if(!USB_PIO_Connection_Command(command_string,"!00",reply_string,COMMAND_STRING_LENGTH))
		return FALSE;
	/* parse reply_string - hex representation of the port_type should be in characters 3 and 4 i.e. '!00xx' */
	(*port_type) = strtol( &reply_string[3], NULL, 16);
	if(!COMMAND_PORT_TYPE_IS_VALID((*port_type)))
	{
		Command_Error_Number = 13;
		sprintf(Command_Error_String,"USB_PIO_Command_Port_Get: Parsed Illegal port_type %2.2X from reply '%s'.",
			(*port_type),reply_string);
		return FALSE;
	}
#if LOGGING > 0
	USB_PIO_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				   "USB_PIO_Command_Port_Get(port=%d) returned port_type=%d.",port,(*port_type));
#endif /* LOGGING */
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

