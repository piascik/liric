/* detector_serial.h */
#ifndef DETECTOR_SERIAL_H
#define DETECTOR_SERIAL_H

extern int Detector_Serial_Open(void);
extern int Detector_Serial_Command_Get_System_Status(unsigned char *status,int *checksum_enabled,
						     int *cmd_ack_enabled,int *fpga_booted,int *fpga_in_reset,
						     int *eprom_comms_enabled);
extern int Detector_Serial_Command_Set_System_Status(int cmd_ack_enable,int checksum_enable,int eprom_comms_enable);

extern int Detector_Serial_Command(unsigned char *command_buffer,int command_buffer_length,
				   unsigned char *reply_buffer,int reply_buffer_length);

extern int Detector_Serial_Compute_Checksum(unsigned char *buffer,int *buffer_length);
extern char* Detector_Serial_Print_Command(unsigned char *buffer,int buffer_length,char *string_buffer,int string_buffer_length);
extern int Detector_Serial_Parse_Hex_String(char *string_buffer,unsigned char *command_buffer,int command_buffer_max_length,
				     int *command_buffer_length);
extern int Detector_Serial_Get_Error_Number(void);
extern void Detector_Serial_Error(void);
extern void Detector_Serial_Error_String(char *error_string);

#endif
