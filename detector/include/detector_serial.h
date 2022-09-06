/* detector_serial.h */
#ifndef DETECTOR_SERIAL_H
#define DETECTOR_SERIAL_H

extern int Detector_Serial_Open(void);
extern int Detector_Serial_Command(char *command_buffer,int command_buffer_length,
				   char *reply_buffer,int reply_buffer_length);

extern char* Detector_Serial_Print_Command(char *buffer,int buffer_length);
extern int Detector_Serial_Get_Error_Number(void);
extern void Detector_Serial_Error(void);
extern void Detector_Serial_Error_String(char *error_string);

#endif
