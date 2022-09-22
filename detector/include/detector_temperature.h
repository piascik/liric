/* detector_temperature.h */
#ifndef DETECTOR_TEMPERATURE_H
#define DETECTOR_TEMPERATURE_H
extern int Detector_Temperature_Initialise(int adc_zeroC,int adc_fortyC,int dac_zeroC,int dac_fortyC);
extern int Detector_Temperature_Set_Fan(int onoff);
extern int Detector_Temperature_Set_TEC(int onoff);
extern int Detector_Temperature_Get(double *detector_temperature_C);
extern int Detector_Temperature_PCB_Get(double *detector_temperature_C);
extern int Detector_Temperature_Get_TEC_Setpoint(double *setpoint_temperature_C);

extern int Detector_Temperature_ADC_To_Temp(int adc_value,double *detector_temperature_C);
extern int Detector_Temperature_DAC_To_Temp(int dac_value,double *temperature_C);

extern int Detector_Temperature_Get_Error_Number(void);
extern void Detector_Temperature_Error(void);
extern void Detector_Temperature_Error_String(char *error_string);

#endif
