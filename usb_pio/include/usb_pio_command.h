/* usb_pio_command.h */

#ifndef USB_PIO_COMMAND_H
#define USB_PIO_COMMAND_H

/**
 * Enum denoting whether a specified I/O port is configured for INPUT or OUTPUT.
 * <ul>
 * <li><b>USB_PIO_PORT_TYPE_OUTPUT</b> Configure the port for output only.
 * <li><b>USB_PIO_PORT_TYPE_INPUT</b> Configure the port for input only.
 * </ul>
 */
enum USB_PIO_PORT_TYPE
{
	USB_PIO_PORT_TYPE_OUTPUT = 0x00,
	USB_PIO_PORT_TYPE_INPUT = 0xff
};

extern int USB_PIO_Command_Output_Set(int output,int onoff);
extern int USB_PIO_Command_Outputs_Set(unsigned char outputs);
extern int USB_PIO_Command_Outputs_Get(unsigned char *outputs);
extern int USB_PIO_Command_Output_Get(int output,int *onoff);
extern int USB_PIO_Command_Inputs_Get(unsigned char *inputs);
extern int USB_PIO_Command_Input_Get(int input,int *onoff);

extern int USB_PIO_Command_Port_Set(int port,enum USB_PIO_PORT_TYPE port_type);
extern int USB_PIO_Command_Port_Get(int port,enum USB_PIO_PORT_TYPE *port_type);

extern int USB_PIO_Command_Get_Error_Number(void);
extern void USB_PIO_Command_Error(void);
extern void USB_PIO_Command_Error_String(char *error_string);

#endif
