/* liric_main.c */
/**
 * Liric C server main program. This controls the Liric Ninox-640 Infra-red detector,
 * the filter_wheel and the nudgematic offseting mechanism.
 * @author $Author$
 */
#include <signal.h> /* signal handling */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log_udp.h"

#include "command_server.h"

#include "detector_buffer.h"
#include "detector_exposure.h"
#include "detector_fits_filename.h"
#include "detector_general.h"
#include "detector_setup.h"
#include "detector_temperature.h"

#include "filter_wheel_command.h"
#include "filter_wheel_general.h"

#include "nudgematic_connection.h"
#include "nudgematic_general.h"

#include "liric_config.h"
#include "liric_general.h"
#include "liric_fits_header.h"
#include "liric_server.h"

/* internal variables */
/**
 * Revision control system identifier.
 */
static char rcsid[] = "$Id$";

/* internal routines */
static int Liric_Initialise_Signal(void);
static int Liric_Initialise_Logging(void);
static int Liric_Initialise_Mechanisms(void);
static void Liric_Shutdown_Mechanisms(void);
static int Liric_Startup_Detector(void);
static int Liric_Shutdown_Detector(void);
static int Liric_Startup_Nudgematic(void);
static int Liric_Shutdown_Nudgematic(void);
static int Liric_Startup_Filter_Wheel(void);
static int Liric_Shutdown_Filter_Wheel(void);
static int Parse_Arguments(int argc, char *argv[]);
static void Help(void);


/* ------------------------------------------------------------------
** External functions 
** ------------------------------------------------------------------ */
/**
 * Main program.
 * <ul>
 * <li>We parse the command line arguments using Parse_Arguments.
 * <li>We setup signal handling (so the server doesn't crash when a client does) using Liric_Initialise_Signal.
 * <li>We load the configuration file using Liric_Config_Load, using the config file returned by 
 *     Liric_General_Get_Config_Filename.
 * <li>We initialise the logging using Liric_Initialise_Logging.
 * <li>We initialise the mechanisms (detector, nudgematic, filter wheel) using Liric_Initialise_Mechanisms.
 * <li>We intialise the server using Liric_Server_Initialise.
 * <li>We start the server to handle incoming commands with Liric_Server_Start. This routine finishes
 *     when the server/progam is told to terminate.
 * <li>We shutdown the connection to the mechanisms using Liric_Shutdown_Mechanisms.
 * </ul>
 * @param argc The number of arguments to the program.
 * @param argv An array of argument strings.
 * @return This function returns 0 if the program succeeds, and a positive integer if it fails.
 * @see #Parse_Arguments
 * @see #Liric_Initialise_Signal
 * @see #Liric_Config_Load
 * @see #Liric_Initialise_Logging
 * @see #Liric_Initialise_Mechanisms
 * @see #Liric_Server_Initialise
 * @see #Liric_Server_Start
 * @see #Liric_Shutdown_Mechanisms
 * @see liric_general.html#Liric_General_Get_Config_Filename
 * @see liric_general.html#Liric_General_Error
 */
int main(int argc, char *argv[])
{
	int retval;

/* parse arguments */
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP","Parsing Arguments.");
#endif
	if(!Parse_Arguments(argc,argv))
		return 1;
	/* initialise signal handling */
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP",
			   "Liric_Initialise_Signal.");
#endif
	retval = Liric_Initialise_Signal();
	if(retval == FALSE)
	{
		Liric_General_Error("main","liric_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
		return 4;
	}
	/* initialise/load configuration */
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP","Liric_Config_Load.");
#endif
	retval = Liric_Config_Load(Liric_General_Get_Config_Filename());
	if(retval == FALSE)
	{
		Liric_General_Error("main","liric_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
		return 2;
	}
	/* set logging options */
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP",
			       "Liric_Initialise_Logging.");
#endif
	retval = Liric_Initialise_Logging();
	if(retval == FALSE)
	{
		Liric_General_Error("main","liric_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
		return 4;
	}
	/* initialise mechanisms */
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP",
			   "Liric_Initialise_Mechanisms.");
#endif
	retval = Liric_Initialise_Mechanisms();
	if(retval == FALSE)
	{
		Liric_General_Error("main","liric_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
		return 3;
	}
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP",
			       "Liric_Server_Initialise.");
#endif
	retval = Liric_Server_Initialise();
	if(retval == FALSE)
	{
		Liric_General_Error("main","liric_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
		/* shutdown mechanisms */
		Liric_Shutdown_Mechanisms();
		return 4;
	}
	/* start server */
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP",
			       "Liric_Server_Start.");
#endif
	retval = Liric_Server_Start();
	if(retval == FALSE)
	{
		Liric_General_Error("main","liric_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
		/* shutdown mechanisms */
		Liric_Shutdown_Mechanisms();
		return 4;
	}
	/* shutdown */
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP",
			   "Liric_Shutdown_Mechanisms");
#endif
	Liric_Shutdown_Mechanisms();
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","main",LOG_VERBOSITY_VERY_TERSE,"STARTUP",
			   "liric completed.");
#endif
	return 0;
}

/* -----------------------------------------------------------------------------
**      Internal routines
** ----------------------------------------------------------------------------- */
/**
 * Initialise signal handling. Switches off default "Broken pipe" error, so client
 * crashes do NOT kill the Liric control system.
 * Don't use Logging here, this is called pre-logging.
 */
static int Liric_Initialise_Signal(void)
{
	struct sigaction sig_action;

	/* old code
	signal(SIGPIPE, SIG_IGN);
	*/
	sig_action.sa_handler = SIG_IGN;
	sig_action.sa_flags = 0;
	sigemptyset(&sig_action.sa_mask);
	sigaction(SIGPIPE,&sig_action,NULL);
	return TRUE;
}

/**
 * Setup logging. Get directory name from config "logging.directory_name".
 * Get UDP logging config. Setup log handlers for Liric software and subsystems.
 * @return The routine returns TRUE on success and FALSE on failure. Liric_General_Error_Number / 
 *         Liric_General_Error_String are set on failure.
 * @see liric_general.html#Liric_General_Error_Number
 * @see liric_general.html#Liric_General_Error_String
 * @see liric_general.html#Liric_General_Log_Set_Directory
 * @see liric_general.html#Liric_General_Log_Set_Root
 * @see maptor_general.html#Liric_General_Log_Set_Error_Root
 * @see liric_general.html#Liric_General_Log_Set_UDP
 * @see liric_general.html#Liric_General_Add_Log_Handler_Function
 * @see liric_general.html#Liric_General_Log_Handler_Log_Hourly_File
 * @see liric_general.html#Liric_General_Log_Handler_Log_UDP
 * @see liric_general.html#Liric_General_Call_Log_Handlers
 * @see liric_general.html#Liric_General_Call_Log_Handlers_Detector
 * @see liric_general.html#Liric_General_Call_Log_Handlers_Filter_Wheel
 * @see liric_general.html#Liric_General_Call_Log_Handlers_Nudgematic
 * @see liric_general.html#Liric_General_Set_Log_Filter_Function
 * @see liric_general.html#Liric_General_Log_Filter_Level_Absolute
 * @see liric_config.html#Liric_Config_Get_Boolean
 * @see liric_config.html#Liric_Config_Get_Integer
 * @see liric_config.html#Liric_Config_Get_String
 * @see ../detector/cdocs/detector_general.html#Detector_General_Set_Log_Handler_Function
 * @see ../detector/cdocs/detector_general.html#Detector_General_Set_Log_Filter_Function
 * @see ../detector/cdocs/detector_general.html#Detector_General_Log_Filter_Level_Absolute
 * @see ../filter_wheel/cdocs/filter_wheel_general.html#Filter_Wheel_General_Set_Log_Handler_Function
 * @see ../filter_wheel/cdocs/filter_wheel_general.html#Filter_Wheel_General_Set_Log_Filter_Function
 * @see ../filter_wheel/cdocs/filter_wheel_general.html#Filter_Wheel_General_Log_Filter_Level_Absolute
 * @see ../nudgematic/cdocs/nudgematic_general.html#Nudgematic_General_Set_Log_Handler_Function
 * @see ../nudgematic/cdocs/nudgematic_general.html#Nudgematic_General_Set_Log_Filter_Function
 * @see ../nudgematic/cdocs/nudgematic_general.html#Nudgematic_General_Log_Filter_Level_Absolute
 * @see ../../commandserver/cdocs/command_server.html#Command_Server_Set_Log_Handler_Function
 * @see ../../commandserver/cdocs/command_server.html#Command_Server_Set_Log_Filter_Function
 * @see ../../commandserver/cdocs/command_server.html#Command_Server_Log_Filter_Level_Absolute
 */
static int Liric_Initialise_Logging(void)
{
	char *log_directory = NULL;
	char *filename_root = NULL;
	char *hostname = NULL;
	int retval,port_number,active;

	/* don't log yet - not fully setup yet */
	/* log directory */
	if(!Liric_Config_Get_String("logging.directory_name",&log_directory))
	{
		Liric_General_Error_Number = 17;
		sprintf(Liric_General_Error_String,"Liric_Initialise_Logging:"
			"Failed to get logging directory.");
		return FALSE;
	}
	if(!Liric_General_Log_Set_Directory(log_directory))
	{
		if(log_directory != NULL)
			free(log_directory);
		return FALSE;
	}
	if(log_directory != NULL)
		free(log_directory);
	/* log filename root */
	if(!Liric_Config_Get_String("logging.root.log",&filename_root))
	{
		Liric_General_Error_Number = 19;
		sprintf(Liric_General_Error_String,"Liric_Initialise_Logging:"
			"Failed to get log root filename.");
		return FALSE;
	}
	if(!Liric_General_Log_Set_Root(filename_root))
	{
		if(filename_root != NULL)
			free(filename_root);
		return FALSE;
	}
	if(filename_root != NULL)
		free(filename_root);
	/* error filename root */
	if(!Liric_Config_Get_String("logging.root.error",&filename_root))
	{
		Liric_General_Error_Number = 23;
		sprintf(Liric_General_Error_String,"Liric_Initialise_Logging:"
			"Failed to get error root filename.");
		return FALSE;
	}
	if(!Liric_General_Log_Set_Error_Root(filename_root))
	{
		if(filename_root != NULL)
			free(filename_root);
		return FALSE;
	}
	if(filename_root != NULL)
		free(filename_root);
	/* setup log_udp */
	if(!Liric_Config_Get_Boolean("logging.udp.active",&active))
	{
		Liric_General_Error_Number = 20;
		sprintf(Liric_General_Error_String,"Liric_Initialise_Logging:"
			"Failed to get log_udp active.");
		return FALSE;
	}
	if(!Liric_Config_Get_Integer("logging.udp.port_number",&port_number))
	{
		Liric_General_Error_Number = 21;
		sprintf(Liric_General_Error_String,"Liric_Initialise_Logging:"
			"Failed to get log_udp port_number.");
		return FALSE;
	}
	if(!Liric_Config_Get_String("logging.udp.hostname",&hostname))
	{
		Liric_General_Error_Number = 22;
		sprintf(Liric_General_Error_String,"Liric_Initialise_Logging:"
			"Failed to get log_udp hostname.");
		return FALSE;
	}
	if(!Liric_General_Log_Set_UDP(active,hostname,port_number))
	{
		if(hostname != NULL)
			free(hostname);
		return FALSE;
	}
	if(hostname != NULL)
		free(hostname);
	/* Liric */
	Liric_General_Add_Log_Handler_Function(Liric_General_Log_Handler_Log_Hourly_File);
	Liric_General_Add_Log_Handler_Function(Liric_General_Log_Handler_Log_UDP);
	Liric_General_Set_Log_Filter_Function(Liric_General_Log_Filter_Level_Absolute);
	/* Detector */
	Detector_General_Set_Log_Handler_Function(Liric_General_Call_Log_Handlers_Detector);
	Detector_General_Set_Log_Filter_Function(Detector_General_Log_Filter_Level_Absolute);
	/* filter wheel */
	Filter_Wheel_General_Set_Log_Handler_Function(Liric_General_Call_Log_Handlers_Filter_Wheel);
	Filter_Wheel_General_Set_Log_Filter_Function(Filter_Wheel_General_Log_Filter_Level_Absolute);
	/* nudgematic */
	Nudgematic_General_Set_Log_Handler_Function(Liric_General_Call_Log_Handlers_Nudgematic);
	Nudgematic_General_Set_Log_Filter_Function(Nudgematic_General_Log_Filter_Level_Absolute);
	/* setup command server logging */
	Command_Server_Set_Log_Handler_Function(Liric_General_Call_Log_Handlers);
	Command_Server_Set_Log_Filter_Function(Command_Server_Log_Filter_Level_Absolute);
	return TRUE;
}

/**
 * Initialise the liric mechanisms. Calls Liric_Startup_Detector,Liric_Startup_Mudgematic,
 * Liric_Startup_Filter_Wheel.
 * @return The routine returns TRUE on success and FALSE on failure. Liric_General_Error_Number / 
 *         Liric_General_Error_String are set on failure.
 * @see #Liric_Startup_Detector
 * @see #Liric_Startup_Nudgematic
 * @see #Liric_Startup_Filter_Wheel
 * @see liric_general.html#aptor_General_Error_Number
 * @see liric_general.html#Liric_General_Error_String
 * @see liric_general.html#Liric_General_Log
 */
static int Liric_Initialise_Mechanisms(void)
{
	int retval;

#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Initialise_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Started.");
#endif
	/* initialise connection to the Detector */
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Initialise_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Calling Liric_Startup_Detector.");
#endif
	retval = Liric_Startup_Detector();
	if(retval == FALSE)
	{
		return FALSE;
	}
	/* initialise connection to the nudgematic */
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Initialise_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Calling Liric_Startup_Nudgematic.");
#endif
	retval = Liric_Startup_Nudgematic();
	if(retval == FALSE)
	{
		return FALSE;
	}
	/* initialise connection to the filter wheel */
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Initialise_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Calling Liric_Startup_Filter_Wheel.");
#endif
	retval = Liric_Startup_Filter_Wheel();
	if(retval == FALSE)
	{
		return FALSE;
	}
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Initialise_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Finished.");
#endif
	return TRUE;
}

/**
 * Shutdown the liric mechanisms. Calls Liric_Shutdown_Detector,Liric_Shutdown_Nudgematic,
 * Liric_Shutdown_Filter_Wheel.
 * @see #Liric_Shutdown_Detector
 * @see #Liric_Shutdown_Nudgematic
 * @see #Liric_Shutdown_Filter_Wheel
 * @see liric_general.html#Liric_General_Error_Number
 * @see liric_general.html#Liric_General_Error_String
 * @see liric_general.html#Liric_General_Log
 */
static void Liric_Shutdown_Mechanisms(void)
{
	int retval;

#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Shutdown_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Started.");
#endif
	/* shutdown detector */
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Shutdown_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Calling Liric_Shutdown_Detector.");
#endif
	retval = Liric_Shutdown_Detector();
	if(retval == FALSE)
	{
		/* log the error but do not exit, try to shutdown other mechanisms */
		Liric_General_Error("main","liric_main.c","Liric_Shutdown_Mechanisms",LOG_VERBOSITY_VERY_TERSE,
				     "STARTUP");
	}
	/* shutdown nudgematic */
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Shutdown_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Calling Liric_Shutdown_Nudgematic.");
#endif
	retval = Liric_Shutdown_Nudgematic();
	if(retval == FALSE)
	{
		/* log the error but do not exit, try to shutdown other mechanisms */
		Liric_General_Error("main","liric_main.c","Liric_Shutdown_Mechanisms",LOG_VERBOSITY_VERY_TERSE,"STARTUP");
	}
	/* shutdown filter wheel */
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Shutdown_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Calling Liric_Shutdown_Filter_Wheel.");
#endif
	retval = Liric_Shutdown_Filter_Wheel();
	if(retval == FALSE)
	{
		/* log the error but do not exit, try to shutdown other mechanisms */
		Liric_General_Error("main","liric_main.c","Liric_Shutdown_Mechanisms",LOG_VERBOSITY_VERY_TERSE,
				     "STARTUP");
	}
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Shutdown_Mechanisms",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Finished.");
#endif
}

/**
 * If the Detector is enabled for initialisation, initialise the Detector.
 * <ul>
 * <li>Use Liric_Config_Get_Boolean to get "detector.enable" to see whether the Detector is enabled for initialisation.
 * <li>If it is _not_ enabled, log and return success.
 * <li>We call Liric_Config_Get_String with key "detector.format_dir" to get the format directory 
 *     (directory containing '.fmt' files used by the Liric SDK / Detector_Setup_Startup).
 * <li>We call Liric_Config_Get_Integer with key "detector.coadd_exposure_length.long" to get an initial value for 
 *     the coadd exposure length.
 * <li>We generate a format filename to use as a parameter to Detector_Setup_Startup, based on the above config.
 * <li>We call Liric_Config_Get_Boolean to get "detector.fan.enable" to see whether the Detector to turn the detector fan on or off.
 * <li>We call Detector_Setup_Startup to initialise the Detector.
 * <li>We call Detector_Exposure_Set_Coadd_Frame_Exposure_Length to set the coadded exposure length to use for exposures.
 * <li>We call Detector_Temperature_Set_Fan to turn the detector fan on or off.
 * <li>We call Liric_Config_Get_Character to get the instrument code for Liric
 *     with property keyword: "file.fits.instrument_code".
 * <li>We call Liric_Config_Get_String to get the data directory to store generated FITS images in using the
 *     property keyword: "file.fits.path".
 * <li>We call Detector_Fits_Filename_Initialise to initialise FITS filename data and find the current MULTRUN number.
 * <li>We call Liric_Fits_Header_Initialise to initialise FITS header data.
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see liric_config.html#Liric_Config_Get_Integer
 * @see liric_config.html#Liric_Config_Get_Boolean
 * @see liric_config.html#Liric_Config_Get_Character
 * @see liric_config.html#Liric_Config_Get_String
 * @see liric_fits_header.html#Liric_Fits_Header_Initialise
 * @see liric_general.html#Liric_General_Error_Number
 * @see liric_general.html#Liric_General_Error_String
 * @see liric_general.html#Liric_General_Log
 * @see liric_general.html#Liric_General_Log_Format
 * @see ../detector/cdocs/detector_fits_filename.html#Detector_Fits_Filename_Initialise
 * @see ../detector/cdocs/detector_setup.html#Detector_Setup_Startup
 * @see ../detector/cdocs/detector_temperature.html#Detector_Temperature_Set_Fan
 * @see ../detector/cdocs/detector_exposure.html#Detector_Exposure_Set_Coadd_Frame_Exposure_Length
 */
static int Liric_Startup_Detector(void)
{
	int enabled,fan_enabled,coadd_exposure_length;
	char instrument_code;
	char format_filename[256];
	char* data_dir = NULL;
	char* format_dir_string = NULL;
	
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Startup_Detector",LOG_VERBOSITY_TERSE,"STARTUP","Started.");
#endif
	/* do we want to initialise the detector.
	** The C layer always has a detector attached, but if it is unplugged/broken, setting "detector.enable" to FALSE
	** allows the C layer to initialise to enable control of other mechanisms from the C layer. */
	if(!Liric_Config_Get_Boolean("detector.enable",&enabled))
	{
		Liric_General_Error_Number = 1;
		sprintf(Liric_General_Error_String,
			"Liric_Startup_Detector:Failed to get whether the detector is enabled for initialisation.");
		return FALSE;
	}
	/* if we don't want to initialise the detector, just return here. */
	if(enabled == FALSE)
	{
#if LIRIC_DEBUG > 1
		Liric_General_Log("main","liric_main.c","Liric_Startup_Detector",LOG_VERBOSITY_TERSE,"STARTUP",
				   "Finished (Detector NOT enabled).");
#endif
		return TRUE;
	}
	/* diddly we can now replace this next bit with a call to Liric_Command_Initialise_Detector */	 
	/* get the coadd exposure length. */
	if(!Liric_Config_Get_Integer("detector.coadd_exposure_length.long",&coadd_exposure_length))
	{
		Liric_General_Error_Number = 27;
		sprintf(Liric_General_Error_String,
			"Liric_Startup_Detector:Failed to get long coadd exposure length.");
		return FALSE;
	}
	/* get the '.fmt' format directory from config */
	if(!Liric_Config_Get_String("detector.format_dir",&format_dir_string))
	{
		Liric_General_Error_Number = 32;
		sprintf(Liric_General_Error_String,"Liric_Startup_Detector:Failed to get detector format directory.");
		return FALSE;
	}
	sprintf(format_filename,"%s/rap_%dms.fmt",format_dir_string,coadd_exposure_length);	
	if(format_dir_string != NULL)
		free(format_dir_string);
	/* get whether to turn the fan on of off */
	if(!Liric_Config_Get_Boolean("detector.fan.enable",&fan_enabled))
	{
		Liric_General_Error_Number = 15;
		sprintf(Liric_General_Error_String,
			"Liric_Startup_Detector:Failed to get whether the detector fan is enabled for initialisation.");
		return FALSE;
	}
	/* actually do initialisation of the detector library */
#if LIRIC_DEBUG > 1
	Liric_General_Log_Format("main","liric_main.c","Liric_Startup_Detector",LOG_VERBOSITY_TERSE,"STARTUP",
				  "Calling Detector_Setup_Startup with format filename '%s'.",format_filename);
#endif
	if(!Detector_Setup_Startup(format_filename))
	{
		Liric_General_Error_Number = 2;
		sprintf(Liric_General_Error_String,"Liric_Startup_Detector:Detector_Setup_Startup failed.");
		return FALSE;
	}
	/* setup coadd exposure length */
	if(!Detector_Exposure_Set_Coadd_Frame_Exposure_Length(coadd_exposure_length))
	{
		Liric_General_Error_Number = 3;
		sprintf(Liric_General_Error_String,
			"Liric_Startup_Detector:Detector_Exposure_Set_Coadd_Frame_Exposure_Length failed.");
		return FALSE;
	}
#if LIRIC_DEBUG > 1
	Liric_General_Log_Format("main","liric_main.c","Liric_Startup_Detector",LOG_VERBOSITY_VERBOSE,"STARTUP",
				  "Calling Detector_Temperature_Set_Fan with fan enabled '%s'.",fan_enabled ? "True" : "False");
#endif
	/* turn the detector fan on or off */
	if(!Detector_Temperature_Set_Fan(fan_enabled))
	{
		Liric_General_Error_Number = 25;
		sprintf(Liric_General_Error_String,
			"Liric_Startup_Detector:Detector_Temperature_Set_Fan(%d) failed.",fan_enabled);
		return FALSE;
	}
	/* fits filename initialisation */
	if(!Liric_Config_Get_Character("file.fits.instrument_code",&instrument_code))
		return FALSE;
	if(!Liric_Config_Get_String("file.fits.path",&data_dir))
		return FALSE;
#if LIRIC_DEBUG > 1
	Liric_General_Log_Format("main","liric_main.c","Liric_Startup_Detector",LOG_VERBOSITY_TERSE,"STARTUP",
				  "Calling Detector_Fits_Filename_Initialise(%c,%s).",instrument_code,data_dir);
#endif
	if(!Detector_Fits_Filename_Initialise(instrument_code,data_dir))
	{
		Liric_General_Error_Number = 4;
		sprintf(Liric_General_Error_String,
			"Liric_Startup_Detector:Detector_Fits_Filename_Initialise failed.");
		if(data_dir != NULL)
			free(data_dir);
		return FALSE;
	}
	/* free allocated data */
	if(data_dir != NULL)
		free(data_dir);
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Startup_Detector",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Calling Liric_Fits_Header_Initialise.");
#endif
	if(!Liric_Fits_Header_Initialise())
	{
		Liric_General_Error_Number = 5;
		sprintf(Liric_General_Error_String,"Liric_Startup_Detector:Detector_Fits_Header_Initialise failed.");
		return FALSE;
	}
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Startup_Detector",LOG_VERBOSITY_TERSE,"STARTUP","Finished.");
#endif
	return TRUE;
}

/**
 * Shutdown the Detector.
 * <ul>
 * <li>Use Liric_Config_Get_Boolean to get "detector.enable" to see whether the Detector is enabled for initialisation/finislisation.
 * <li>If it is _not_ enabled, log and return success.
 * <li>Call Detector_Setup_Shutdown to shutdown the connection to the detector.
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see liric_config.html#Liric_Config_Get_Boolean
 * @see ../detector/cdocs/detector_setup.html#Detector_Setup_Shutdown
 */
static int Liric_Shutdown_Detector(void)
{
	int enabled;

#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Shutdown_Detector",LOG_VERBOSITY_TERSE,"STARTUP","Started.");
#endif
	/* is the detector active/enabled */
	if(!Liric_Config_Get_Boolean("detector.enable",&enabled))
	{
		Liric_General_Error_Number = 6;
		sprintf(Liric_General_Error_String,
			"Liric_Shutdown_Detector:Failed to get whether detector initialisation is enabled.");
		return FALSE;
	}
	/* if the detector is _not_ active, just return OK here */
	if(enabled == FALSE)
	{
#if LIRIC_DEBUG > 1
		Liric_General_Log("main","liric_main.c","Liric_Shutdown_Detector",LOG_VERBOSITY_TERSE,"STARTUP",
				   "Finished (Detector NOT enabled).");
#endif
		return TRUE;
	}
	/* shutdown the connection */
#if LIRIC_DEBUG > 1
	Liric_General_Log_Format("main","liric_main.c","Liric_Shutdown_Detector",LOG_VERBOSITY_TERSE,"STARTUP",
				  "Calling Detector_Setup_Shutdown.");
#endif
	if(!Detector_Setup_Shutdown())
	{
		Liric_General_Error_Number = 24;
		sprintf(Liric_General_Error_String,"Liric_Shutdown_Detector:Detector_Setup_Shutdown failed.");
		return FALSE;
	}
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Shutdown_Detector",LOG_VERBOSITY_TERSE,"STARTUP","Finished.");
#endif
	return TRUE;
}

/**
 * If the nudgematic is enabled, open a connection to the USB-PIO board.
 * <ul>
 * <li>Use Liric_Config_Get_Boolean to get "nudgematic.enable" to see whether the nudgematic is enabled.
 * <li>If it is _not_ enabled, log and return success.
 * <li>Use Liric_Config_Get_String to get the device name to use for the 
 *     USB-PIO connection ("nudgematic.device_name").
 * <li>Call Nudgematic_Connection_Open to connect to the Nudgematic mechanism via the USB-PIO board.
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see liric_config.html#Liric_Config_Get_Boolean
 * @see liric_config.html#Liric_Config_Get_String
 * @see liric_general.html#Liric_General_Error_Number
 * @see liric_general.html#Liric_General_Error_String
 * @see liric_general.html#Liric_General_Log
 * @see liric_general.html#Liric_General_Log_Format
 * @see ../nudgematic/cdocs/nudgematic_connection.html#Nudgematic_Connection_Open
 */
static int Liric_Startup_Nudgematic(void)
{
	char *device_name = NULL;
	int enabled;

#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Startup_Nudgematic",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Started.");
#endif
	/* is the nudgematic active/enabled */
	if(!Liric_Config_Get_Boolean("nudgematic.enable",&enabled))
	{
		Liric_General_Error_Number = 12;
		sprintf(Liric_General_Error_String,"Liric_Startup_Nudgematic:"
			"Failed to get whether nudgematic is enabled.");
		return FALSE;
	}
	/* if the nudgematic is _not_ active, just return OK here */
	if(enabled == FALSE)
	{
#if LIRIC_DEBUG > 1
		Liric_General_Log("main","liric_main.c","Liric_Startup_Nudgematic",LOG_VERBOSITY_TERSE,"STARTUP",
				   "Finished (nudgematic NOT enabled).");
#endif
		return TRUE;
	}
	/* get device name */
	if(!Liric_Config_Get_String("nudgematic.device_name",&device_name))
	{
		Liric_General_Error_Number = 13;
		sprintf(Liric_General_Error_String,"Liric_Startup_Rotator:"
			"Failed to get nudgematic device_name.");
		return FALSE;
	}
        /* open device */
#if LIRIC_DEBUG > 1
	Liric_General_Log_Format("main","liric_main.c","Liric_Startup_Nudgematic",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Open a connection to the nudgematic using device '%s'.",device_name);
#endif
	if(!Nudgematic_Connection_Open(device_name))
	{
		Liric_General_Error_Number = 14;
		sprintf(Liric_General_Error_String,"Liric_Startup_Nudgematic:"
			"Nudgematic_Connection_Open(%s) failed.",device_name);
		/* free allocated data */
		if(device_name != NULL)
			free(device_name);
		return FALSE;
	}
	/* free allocated data */
	if(device_name != NULL)
		free(device_name);
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","mliric_main.c","Liric_Startup_Nudgematic",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Finished.");
#endif
	return TRUE;
}

/**
 * Shutdown a previously opened connection to the nudgematic.
 * <ul>
 * <li>Use Liric_Config_Get_Boolean to get "nudgematic.enable" to see whether the nudgematic is enabled.
 * <li>If it is _not_ enabled, log and return success.
 * <li>Use Nudgematic_Connection_Close to close the connection to the nudgematic.
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see liric_config.html#Liric_Config_Get_Boolean
 * @see liric_general.html#Liric_General_Error_Number
 * @see liric_general.html#Liric_General_Error_String
 * @see liric_general.html#Liric_General_Log
 * @see ../nudgematic/cdocs/nudgematic_connection.html#Nudgematic_Connection_Close
 */
static int Liric_Shutdown_Nudgematic(void)
{
	int enabled;

#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Shutdown_Rotator",LOG_VERBOSITY_TERSE,"STARTUP","Started.");
#endif
	/* is the nudgematic active/enabled */
	if(!Liric_Config_Get_Boolean("nudgematic.enable",&enabled))
	{
		Liric_General_Error_Number = 16;
		sprintf(Liric_General_Error_String,"Liric_Shutdown_Nudgematic:"
			"Failed to get whether nudgematic is enabled.");
		return FALSE;
	}
	/* if the nudgematic is _not_ active, just return OK here */
	if(enabled == FALSE)
	{
#if LIRIC_DEBUG > 1
		Liric_General_Log("main","liric_main.c","Liric_Shutdown_Nudgematic",LOG_VERBOSITY_TERSE,"STARTUP",
				   "Finished (nudgematic NOT enabled).");
#endif
		return TRUE;
	}
	/* shutdown the connection */
#if LIRIC_DEBUG > 1
	Liric_General_Log_Format("main","liric_main.c","Liric_Shutdown_Nudgematic",LOG_VERBOSITY_TERSE,"STARTUP",
				  "Calling Nudgematic_Connection_Close.");
#endif
	if(!Nudgematic_Connection_Close())
	{
		Liric_General_Error_Number = 18;
		sprintf(Liric_General_Error_String,"Liric_Shutdown_Nudgematic:Nudgematic_Connection_Close failed.");
		return FALSE;
	}
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Shutdown_Rotator",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Finished.");
#endif
	return TRUE;
}

/**
 * If the filter wheel is enabled, open a connection to the filter wheel.
 * <ul>
 * <li>Use Liric_Config_Get_Boolean to get "filter_wheel.enable" to see whether the filter wheel is enabled.
 * <li>If it is _not_ enabled, log and return success.
 * <li>Use Liric_Config_Get_String to get the device name to use for the 
 *     filter wheel connection ("filter_wheel.device_name").
 * <li>Use Filter_Wheel_Command_Open to open a connection to the filter wheel.
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see liric_config.html#Liric_Config_Get_Boolean
 * @see liric_config.html#Liric_Config_Get_String
 * @see liric_general.html#Liric_General_Error_Number
 * @see liric_general.html#Liric_General_Error_String
 * @see liric_general.html#Liric_General_Log
 * @see liric_general.html#Liric_General_Log_Format
 * @see ../filter_wheel/cdocs/filter_wheel_command.html#Filter_Wheel_Command_Open
 */
static int Liric_Startup_Filter_Wheel(void)
{
	char *device_name = NULL;
	int enabled;

#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Startup_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Started.");
#endif
	/* is the filter wheel active/enabled */
	if(!Liric_Config_Get_Boolean("filter_wheel.enable",&enabled))
	{
		Liric_General_Error_Number = 7;
		sprintf(Liric_General_Error_String,"Liric_Startup_Filter_Wheel:"
			"Failed to get whether filter wheel is enabled.");
		return FALSE;
	}
	/* if the filter wheel is _not_ active, just return OK here */
	if(enabled == FALSE)
	{
#if LIRIC_DEBUG > 1
		Liric_General_Log("main","liric_main.c","Liric_Startup_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
				   "Finished (filter wheel NOT enabled).");
#endif
		return TRUE;
	}
	/* get device name */
	if(!Liric_Config_Get_String("filter_wheel.device_name",&device_name))
	{
		Liric_General_Error_Number = 8;
		sprintf(Liric_General_Error_String,"Liric_Startup_Filter_Wheel:"
			"Failed to get filter wheel device_name.");
		return FALSE;
	}
#if LIRIC_DEBUG > 1
	Liric_General_Log_Format("main","liric_main.c","Liric_Startup_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
				  "Opening connection to filter wheel using device '%s'.",device_name);
#endif
	/* open connection to the filter wheel */
	if(!Filter_Wheel_Command_Open(device_name))
	{
		Liric_General_Error_Number = 9;
		sprintf(Liric_General_Error_String,"Liric_Startup_Filter_Wheel:"
			"Filter_Wheel_Command_Open(%s) failed.",device_name);
		/* free allocated data */
		if(device_name != NULL)
			free(device_name);
		return FALSE;
	}
	/* free allocated data */
	if(device_name != NULL)
		free(device_name);
	/* fault 2668. The filter wheel is failing and on startup ends up in position 8 which is impossible.
	** Try driving the filter wheel to a known position after opening a connection to it. */
	/*
#if LIRIC_DEBUG > 1
	Liric_General_Log_Format("main","liric_main.c","Liric_Startup_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
				  "Moving filter wheel to a known position(1).");
#endif
	if(!Filter_Wheel_Command_Move(1))
	{
		Liric_General_Error_Number = 26;
		sprintf(Liric_General_Error_String,"Liric_Startup_Filter_Wheel:Filter_Wheel_Command_Move(1) failed.");
		return FALSE;
	}
	*/
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Startup_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Finished.");
#endif
	return TRUE;
}

/**
 * Shutdown a previously opened connection to the filter wheel.
 * <ul>
 * <li>Use Liric_Config_Get_Boolean to get "filter_wheel.enable" to see whether the filter wheel is enabled.
 * <li>If it is _not_ enabled, log and return success.
 * <li>Use Filter_Wheel_Command_Close to close the connection to the filter wheel.
 * </ul>
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see liric_config.html#Liric_Config_Get_Boolean
 * @see liric_general.html#Liric_General_Error_Number
 * @see liric_general.html#Liric_General_Error_String
 * @see liric_general.html#Liric_General_Log
 * @see ../filter_wheel/cdocs/filter_wheel_command.html#Filter_Wheel_Command_Close
 */
static int Liric_Shutdown_Filter_Wheel(void)
{
	int enabled;

#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Shutdown_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Started.");
#endif
	/* is the filter wheel active/enabled */
	if(!Liric_Config_Get_Boolean("filter_wheel.enable",&enabled))
	{
		Liric_General_Error_Number = 10;
		sprintf(Liric_General_Error_String,"Liric_Shutdown_Filter_Wheel:"
			"Failed to get whether filter wheel is enabled.");
		return FALSE;
	}
	/* if the filter wheel is _not_ active, just return OK here */
	if(enabled == FALSE)
	{
#if LIRIC_DEBUG > 1
		Liric_General_Log("main","liric_main.c","Liric_Shutdown_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
				   "Finished (filter wheel NOT enabled).");
#endif
		return TRUE;
	}
	/* shutdown the connection */
#if LIRIC_DEBUG > 1
	Liric_General_Log_Format("main","liric_main.c","Liric_Shutdown_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
				  "Calling Filter_Wheel_Command_Close.");
#endif
	if(!Filter_Wheel_Command_Close())
	{
		Liric_General_Error_Number = 11;
		sprintf(Liric_General_Error_String,"Liric_Shutdown_Filter_Wheel:Filter_Wheel_Command_Close failed.");
		return FALSE;
	}
#if LIRIC_DEBUG > 1
	Liric_General_Log("main","liric_main.c","Liric_Shutdown_Filter_Wheel",LOG_VERBOSITY_TERSE,"STARTUP",
			   "Finished.");
#endif
	return TRUE;
}

/**
 * Help routine.
 */
static void Help(void)
{
	fprintf(stdout,"Liric:Help.\n");
	fprintf(stdout,"liric [-co[nfig_filename] <filename>]\n");
	fprintf(stdout,"\t[-liric_log_level|-ll <level>\n");
	fprintf(stdout,"\t[-detector_log_level|-ddetll <level>\n");
	fprintf(stdout,"\t[-filter_wheel_log_level|-fwll <level>\n");
	fprintf(stdout,"\t[-nudgematic_log_level|-nll <level>\n");
	fprintf(stdout,"\t[-command_server_log_level|-csll <level>\n");
	fprintf(stdout,"\t<level> is an integer from 1..5.\n");
}

/**
 * Routine to parse command line arguments.
 * @param argc The number of arguments sent to the program.
 * @param argv An array of argument strings.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Help
 * @see liric_general.html#Liric_General_Set_Config_Filename
 * @see liric_general.html#Liric_General_Set_Log_Filter_Level
 * @see ../detector/cdocs/detector_general.html#Detector_General_Set_Log_Filter_Level
 * @see ../filter_wheel/cdocs/filter_wheel_general.html#Filter_Wheel_General_Set_Log_Filter_Level
 * @see ../nudgematic/cdocs/nudgematic_general.html#Nudgematic_General_Set_Log_Filter_Level
 * @see ../../commandserver/cdocs/command_server.html#Command_Server_Set_Log_Filter_Level
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,retval,log_level;

	for(i=1;i<argc;i++)
	{
		if((strcmp(argv[i],"-liric_log_level")==0)||(strcmp(argv[i],"-ll")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&log_level);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing log level %s failed.\n",argv[i+1]);
					return FALSE;
				}
				Liric_General_Set_Log_Filter_Level(log_level);
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:Log Level requires a level.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-detector_log_level")==0)||(strcmp(argv[i],"-detll")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&log_level);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing log level %s failed.\n",argv[i+1]);
					return FALSE;
				}
				Detector_General_Set_Log_Filter_Level(log_level);
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:Log Level requires a level.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-command_server_log_level")==0)||(strcmp(argv[i],"-csll")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&log_level);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing log level %s failed.\n",argv[i+1]);
					return FALSE;
				}
				Command_Server_Set_Log_Filter_Level(log_level);
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:Log Level requires a level.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-config_filename")==0)||(strcmp(argv[i],"-co")==0))
		{
			if((i+1)<argc)
			{
				if(!Liric_General_Set_Config_Filename(argv[i+1]))
				{
					fprintf(stderr,"Parse_Arguments:"
						"Liric_General_Set_Config_Filename failed.\n");
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:config filename required.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-filter_wheel_log_level")==0)||(strcmp(argv[i],"-fwll")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&log_level);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing log level %s failed.\n",argv[i+1]);
					return FALSE;
				}
				Filter_Wheel_General_Set_Log_Filter_Level(log_level);
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:Log Level requires a level.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-help")==0)||(strcmp(argv[i],"-h")==0))
		{
			Help();
			exit(0);
		}
		else if((strcmp(argv[i],"-nudgematic_log_level")==0)||(strcmp(argv[i],"-nll")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&log_level);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing log level %s failed.\n",argv[i+1]);
					return FALSE;
				}
				Nudgematic_General_Set_Log_Filter_Level(log_level);
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:Log Level requires a level.\n");
				return FALSE;
			}
		}
		else
		{
			fprintf(stderr,"Parse_Arguments:argument '%s' not recognized.\n",argv[i]);
			return FALSE;
		}
	}
	return TRUE;
}

