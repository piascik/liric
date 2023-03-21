/* detector_serial.h */
#ifndef DETECTOR_SERIAL_H
#define DETECTOR_SERIAL_H

/**
 * FPGA control bit, this is 1 if the TEC (thermo-electric cooler) is enabled.
 * Also returned by Getting FPGA status.
 */
#define DETECTOR_SERIAL_FPGA_CTRL_TEC_ENABLED              (1<<0)
/**
 * FPGA control bit, this is 1 if the automatic light level/automatic exposure length is enabled.
 * We want this turned off!
 * Also returned by Getting FPGA status.
 */
#define DETECTOR_SERIAL_FPGA_CTRL_AUTO_EXPOSURE_ENABLED    (1<<1)
/**
 * FPGA control bit, this is 1 if the fan is enabled (OWL 640 cooled variant only).
 * Also returned by Getting FPGA status.
 */
#define DETECTOR_SERIAL_FPGA_CTRL_FAN_ENABLED              (1<<2)
/**
 * FPGA control bit, this is 1 if video signal is inverted.
 * Also returned by Getting FPGA status.
 */
#define DETECTOR_SERIAL_FPGA_CTRL_INVERT_VIDEO_ENABLED     (1<<6)
/**
 * FPGA control bit, this is 1 if the image is horizontally flipped.
 * Also returned by Getting FPGA status.
 */
#define DETECTOR_SERIAL_FPGA_CTRL_HORIZONTAL_FLIP_ENABLED  (1<<7)

extern int Detector_Serial_Initialise(void);

extern int Detector_Serial_Open(void);

extern int Detector_Serial_Command_Get_System_Status(unsigned char *status,int *checksum_enabled,
						     int *cmd_ack_enabled,int *fpga_booted,int *fpga_in_reset,
						     int *eprom_comms_enabled);
extern int Detector_Serial_Command_Set_System_State(int checksum_enable,int cmd_ack_enabled,int reset_fpga,
						    int eprom_comms_enable);
extern int Detector_Serial_Command_Get_Manufacturers_Data(int *serial_number,struct timespec *build_date,
							  char *build_code,int *adc_zeroC,int *adc_fortyC,
							  int *dac_zeroC,int *dac_fortyC);
extern int Detector_Serial_Command_Get_Sensor_Temp(int *adc_value);
extern int Detector_Serial_Command_Get_Sensor_PCB_Temp(double *pcb_temp);
extern int Detector_Serial_Command_Get_TEC_Setpoint(int *dac_value);

extern int Detector_Serial_Command_Get_FPGA_Status(unsigned char *status_byte);
extern int Detector_Serial_Command_Set_FPGA_Control(unsigned char ctrl_byte);

extern int Detector_Serial_Command_Set_TEC_Setpoint(int dac_value);

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
