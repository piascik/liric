/* detector_temperature.c
** Raptor Ninox-640 Infrared detector library : temperature status and control routines.
*/
/**
 * Routines to control and monitor the detector temperature of  the Raptor Ninox-640 Infrared detector.
 * @author Chris Mottram
 * @version $Revision$
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log_udp.h"
#include "detector_general.h"
#include "detector_serial.h"
#include "detector_temperature.h"
#include "xcliball.h"

/* hash defines */
/* data types */
/**
 * Data type holding local data to detector_setup. This consists of the following:
 * <dl>
 * <dt>ADC_Zero_C</dt> <dd>The ADU value the temperature analogue to digital converter returns when
 *                         the temperature is zero degrees centigrade.</dd>
 * <dt>ADC_Forty_C</dt> <dd>The ADU value the temperature analogue to digital converter returns when
 *                          the temperature is forty degrees centigrade.</dd>
 * <dt>ADC_M</dt> <dd>The computed slope/gradient of the ADC (x) to temperature (Y) equation of the line (y=mx+c).</dd>
 * <dt>ADC_C</dt> <dd>The computed intercept of the ADC (x) to temperature (Y) equation of the line (y=mx+c).</dd>
 * <dt>DAC_Zero_C</dt> <dd>The ADU value the temperature digital to analogue converter requires
 *                         for a temperature set-point of zero degrees centigrade.</dd>
 * <dt>DAC_Forty_C</dt> <dd>The ADU value the temperature digital to analogue converter requires
 *                          for a temperature set-point of forty degrees centigrade.</dd>
 * <dt>DAC_M</dt> <dd>The computed slope/gradient of the temperature (x) to DAC (Y) equation of the line (y=mx+c).</dd>
 * <dt>DAC_C</dt> <dd>The computed intercept of the temperature (x) to DAC (Y) equation of the line (y=mx+c).</dd>
 * </dl>
 */
struct Temperature_Struct
{
	int ADC_Zero_C;
	int ADC_Forty_C;
	double ADC_M;
	double ADC_C;
	int DAC_Zero_C;
	int DAC_Forty_C;
	double DAC_M;
	double DAC_C;
};

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id$";
/**
 * The instance of Temperature_Struct that contains local data for this module. This is initialised as follows:
 * <dl>
 * <dt>ADC_Zero_C</dt> <dd>0</dd>
 * <dt>ADC_Forty_C</dt> <dd>0</dd>
 * <dt>ADC_M</dt> <dd>0.0</dd>
 * <dt>ADC_C</dt> <dd>0.0</dd>
 * <dt>DAC_Zero_C</dt> <dd>0</dd>
 * <dt>DAC_Forty_C</dt> <dd>0</dd>
 * <dt>DAC_M</dt> <dd>0.0</dd>
 * <dt>DAC_C</dt> <dd>0.0</dd>
 * </dl>
 */
static struct Temperature_Struct Temperature_Data = 
{
	0,0,0.0,0.0,0,0,0.0,0.0
};
/**
 * Variable holding error code of last operation performed.
 */
static int Temperature_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 * @see detector_general.html#DETECTOR_GENERAL_ERROR_STRING_LENGTH
 */
static char Temperature_Error_String[DETECTOR_GENERAL_ERROR_STRING_LENGTH] = "";

/* internal functions */

/* --------------------------------------------------------
** External Functions
** -------------------------------------------------------- */
/**
 * Initialisation routine for the temperature module.
 * The manuafacturers data supplied as arguments to this routine are stored in Temperature_Data, 
 * and the slope and intercept (M and C)
 * parameters of the y=mx+c line are computed for the ADC (temp = (ADC_M * adc) + ADC_C), 
 * and DAC (dac = (DAC_M * temp) +DAC_C).
 * @param adc_zeroC An integer, the ADU value the temperature analogue to digital converter returns when
 *                  the temperature is zero degrees centigrade.
 * @param adc_fortyC An integer, the ADU value the temperature analogue to digital converter returns when
 *                  the temperature is forty degrees centigrade.
 * @param dac_zeroC An integer, the ADU value the temperature digital to analogue converter requires
 *                   for a temperature set-point of zero degrees centigrade.
 * @param dac_fortyC An integer, the ADU value the temperature digital to analogue converter requires
 *                   for a temperature set-point of forty degrees centigrade.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Temperature_Error_Number/Temperature_Error_String are set.
 * @see #Temperature_Data
 * @see #Temperature_Error_Number
 * @see #Temperature_Error_String
 */
int Detector_Temperature_Initialise(int adc_zeroC,int adc_fortyC,int dac_zeroC,int dac_fortyC)
{
	Temperature_Error_Number = 0;
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Temperature_Initialise:Started.");
#endif
	Temperature_Data.ADC_Zero_C = adc_zeroC;
	Temperature_Data.ADC_Forty_C = adc_fortyC;
	Temperature_Data.DAC_Zero_C = dac_zeroC;
	Temperature_Data.DAC_Forty_C = dac_fortyC;
#if LOGGING > 5
	Detector_General_Log_Format(LOG_VERBOSITY_VERBOSE,
				    "Detector_Temperature_Initialise:adc_zeroC = %d,adc_fortyC = %d,"
				    "dac_zeroC = %d,dac_fortyC = %d.",
				    Temperature_Data.ADC_Zero_C,Temperature_Data.ADC_Forty_C,
				    Temperature_Data.DAC_Zero_C,Temperature_Data.DAC_Forty_C);
#endif
	/* compute m and c for the adc and dac slopes
	** See OWL_640_Cooled_IM_v1_0.pdf, Sec 3.1.2, P10 */
	/* ADC, X is ADC and Y is temperature */
	Temperature_Data.ADC_M = -40.0/(double)((Temperature_Data.ADC_Zero_C-Temperature_Data.ADC_Forty_C));
	Temperature_Data.ADC_C = 40.0-(Temperature_Data.ADC_M*((double)Temperature_Data.ADC_Forty_C));
#if LOGGING > 5
	Detector_General_Log_Format(LOG_VERBOSITY_VERBOSE,
				    "Detector_Temperature_Initialise:y (temp) = (adc * ADC_M %.3f) + ADC_C %.3f .",
				    Temperature_Data.ADC_M,Temperature_Data.ADC_C);
#endif
	/* DAC, X is temperature and Y is DAC */
	Temperature_Data.DAC_M = (double)((Temperature_Data.DAC_Zero_C-Temperature_Data.DAC_Forty_C))/-40.0;
	Temperature_Data.DAC_C = ((double)Temperature_Data.DAC_Forty_C)-(Temperature_Data.DAC_M*40.0);
#if LOGGING > 5
	Detector_General_Log_Format(LOG_VERBOSITY_VERBOSE,
				    "Detector_Temperature_Initialise:y (DAC) = (temp (C) * DAC_M %.3f) + DAC_C %.3f .",
				    Temperature_Data.DAC_M,Temperature_Data.DAC_C);
#endif
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Temperature_Initialise:Finished.");
#endif
	return TRUE;
}

/**
 * Routine to turn the Raptor Ninox 640 fan on or off. 
 * @param onoff A boolean, TRUE to turn the fan on, and FALSE to turn it off.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Temperature_Error_Number/Temperature_Error_String are set.
 * @see #Temperature_Error_Number
 * @see #Temperature_Error_String
 * @see detector_serial.html##DETECTOR_SERIAL_FPGA_CTRL_FAN_ENABLED
 * @see detector_serial.html#Detector_Serial_Command_Get_FPGA_Status
 * @see detector_serial.html#Detector_Serial_Command_Set_FPGA_Control
 */
int Detector_Temperature_Set_Fan(int onoff)
{
	unsigned char ctrl_byte;
	
#if LOGGING > 1
	Detector_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"Detector_Temperature_Set_Fan:Started with fan %d.",onoff);
#endif
	Temperature_Error_Number = 0;
	if(!DETECTOR_IS_BOOLEAN(onoff))
	{
		Temperature_Error_Number = 4;
		sprintf(Temperature_Error_String,"Detector_Temperature_Set_Fan:onoff was not a boolean (%d).",onoff);
		return FALSE;
	}
	/* get current FPGA crtl (status) byte */
	if(!Detector_Serial_Command_Get_FPGA_Status(&ctrl_byte))
	{
		Temperature_Error_Number = 5;
		sprintf(Temperature_Error_String,
			"Detector_Temperature_Set_Fan:Detector_Serial_Command_Get_FPGA_Status failed.");
		return FALSE;
	}
	/* twiddle fan bit as specified */
	if(onoff)
	{
		ctrl_byte |= DETECTOR_SERIAL_FPGA_CTRL_FAN_ENABLED;	
	}
	else
	{
		ctrl_byte &= ~(DETECTOR_SERIAL_FPGA_CTRL_FAN_ENABLED);
	}
	/* write new FPGA crtl byte */
	if(!Detector_Serial_Command_Set_FPGA_Control(ctrl_byte))
	{
		Temperature_Error_Number = 6;
		sprintf(Temperature_Error_String,
			"Detector_Temperature_Set_Fan:Detector_Serial_Command_Set_FPGA_Control failed.");
		return FALSE;
	}
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Temperature_Set_Fan:Finished.");
#endif
	return TRUE;
}

/**
 * Routine to turn the Raptor Ninox 640 TEC (thermo electric cooler) on or off. 
 * @param onoff A boolean, TRUE to turn the TEC on, and FALSE to turn it off.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Temperature_Error_Number/Temperature_Error_String are set.
 * @see #Temperature_Error_Number
 * @see #Temperature_Error_String
 * @see detector_serial.html##DETECTOR_SERIAL_FPGA_CTRL_TEC_ENABLED
 * @see detector_serial.html#Detector_Serial_Command_Get_FPGA_Status
 * @see detector_serial.html#Detector_Serial_Command_Set_FPGA_Control
 */
int Detector_Temperature_Set_TEC(int onoff)
{
	unsigned char ctrl_byte;
	
#if LOGGING > 1
	Detector_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,"Detector_Temperature_Set_TEC:Started with TEC %d.",onoff);
#endif
	Temperature_Error_Number = 0;
	if(!DETECTOR_IS_BOOLEAN(onoff))
	{
		Temperature_Error_Number = 7;
		sprintf(Temperature_Error_String,"Detector_Temperature_Set_TEC:onoff was not a boolean (%d).",onoff);
		return FALSE;
	}
	/* get current FPGA crtl (status) byte */
	if(!Detector_Serial_Command_Get_FPGA_Status(&ctrl_byte))
	{
		Temperature_Error_Number = 8;
		sprintf(Temperature_Error_String,
			"Detector_Temperature_Set_FTEC:Detector_Serial_Command_Get_FPGA_Status failed.");
		return FALSE;
	}
	/* twiddle TEC bit as specified */
	if(onoff)
	{
		ctrl_byte |= DETECTOR_SERIAL_FPGA_CTRL_TEC_ENABLED;	
	}
	else
	{
		ctrl_byte &= ~(DETECTOR_SERIAL_FPGA_CTRL_TEC_ENABLED);
	}
	/* write new FPGA crtl byte */
	if(!Detector_Serial_Command_Set_FPGA_Control(ctrl_byte))
	{
		Temperature_Error_Number = 9;
		sprintf(Temperature_Error_String,
			"Detector_Temperature_Set_Tec:Detector_Serial_Command_Set_FPGA_Control failed.");
		return FALSE;
	}
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Temperature_Set_TEC:Finished.");
#endif
	return TRUE;
}

/**
 * Routine to get the detector temperature. The setup, serial and temperature modules must have previously been
 * initialsed / opened for this call to work.
 * @param detector_temperature_C The address of a double, on return this is filled in with the detector
 *        temperature in degrees centigrade.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Temperature_Error_Number/Temperature_Error_String are set.
 * @see #Temperature_Error_Number
 * @see #Temperature_Error_String
 * @see #Detector_Temperature_ADC_To_Temp
 * @see detector_serial.html#Detector_Serial_Command_Get_Sensor_Temp
 */
int Detector_Temperature_Get(double *detector_temperature_C)
{
	int adc_value;
#if LOGGING > 1
	Detector_General_Log(LOG_VERBOSITY_INTERMEDIATE,"Detector_Temperature_Get:Starteded.");
#endif
	Temperature_Error_Number = 0;
	if(detector_temperature_C == NULL)
	{
		Temperature_Error_Number = 1;
		sprintf(Temperature_Error_String,"Detector_Temperature_Get:detector_temperature_C was NULL.");
		return FALSE;
	}
	/* get adc count from the detector via the serial interface */
	if(!Detector_Serial_Command_Get_Sensor_Temp(&adc_value))
	{
		Temperature_Error_Number = 2;
		sprintf(Temperature_Error_String,
			"Detector_Temperature_Get:Detector_Serial_Command_Get_Sensor_Temp failed.");
		return FALSE;
	}
	/* convert to degrees centigrade */
	if(!Detector_Temperature_ADC_To_Temp(adc_value,detector_temperature_C))
		return FALSE;
#if LOGGING > 1
	Detector_General_Log_Format(LOG_VERBOSITY_INTERMEDIATE,
				    "Detector_Temperature_Get:Finished with temperature %.3f C.",
				    (*detector_temperature_C));
#endif
	return TRUE;
}

/**
 * Get the Sensor's PCB temperature. This routine just wraps Detector_Serial_Command_Get_Sensor_PCB_Temp,
 * which does all the work.
 * @param pcb_temp The address of an double, on return from a successful invocation this will be filled with the PCB
 *                 temperature in degrees centigrade.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Serial_Error_Number/Serial_Error_String are set.
 * @see detector_serial.html#Detector_Serial_Command_Get_Sensor_PCB_Temp
 */
int Detector_Temperature_PCB_Get(double *detector_temperature_C)
{
	return Detector_Serial_Command_Get_Sensor_PCB_Temp(detector_temperature_C);
}

/**
 * Routine to convert an ADC value read from the detector temperature sensor, to a temperature in degrees centigrade.
 * @param adc_value The Analogue to digital converter value read from the camera ahead.
 * @param detector_temperature_C The address of a double. On a successful conversion, on return the temperature
 *        will be returned in degrees centigrade.
 * @return The routine returns TRUE on success and FALSE on failure. 
 *         On failure, Temperature_Error_Number/Temperature_Error_String are set.
 * @see #Temperature_Data
 * @see #Temperature_Error_Number
 * @see #Temperature_Error_String
 */
int Detector_Temperature_ADC_To_Temp(int adc_value,double *detector_temperature_C)
{
	if(detector_temperature_C == NULL)
	{
		Temperature_Error_Number = 3;
		sprintf(Temperature_Error_String,"Detector_Temperature_ADC_To_Temp:detector_temperature_C was NULL.");
		return FALSE;
	}
	(*detector_temperature_C) = (((double)adc_value)*Temperature_Data.ADC_M)+Temperature_Data.ADC_C;
	return TRUE;
}

/**
 * Get the current value of the error number.
 * @return The current value of the error number.
 * @see #Temperature_Error_Number
 */
int Detector_Temperature_Get_Error_Number(void)
{
	return Temperature_Error_Number;
}

/**
 * The error routine that reports any errors occuring in a standard way.
 * @see #Temperature_Error_Number
 * @see #Temperature_Error_String
 * @see detector_general.html#Detector_General_Get_Current_Time_String
 */
void Detector_Temperature_Error(void)
{
	char time_string[32];

	Detector_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Temperature_Error_Number == 0)
		sprintf(Temperature_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"%s Detector_Temperature:Error(%d) : %s\n",time_string,Temperature_Error_Number,
		Temperature_Error_String);
}

/**
 * The error routine that reports any errors occuring in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 * @see #Temperature_Error_Number
 * @see #Temperature_Error_String
 * @see detector_general.html#Detector_General_Get_Current_Time_String
 */
void Detector_Temperature_Error_String(char *error_string)
{
	char time_string[32];

	Detector_General_Get_Current_Time_String(time_string,32);
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Temperature_Error_Number == 0)
		sprintf(Temperature_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"%s Detector_Temperature:Error(%d) : %s\n",time_string,
		Temperature_Error_Number,Temperature_Error_String);
}

/* =======================================
**  internal functions 
** ======================================= */
