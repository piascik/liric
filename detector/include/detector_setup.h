/* detector_setup.h */
#ifndef DETECTOR_SETUP_H
#define DETECTOR_SETUP_H

extern int Detector_Setup_Startup(char *format_filename);
extern int Detector_Setup_Shutdown(void);
extern int Detector_Setup_Dimensions(void);

extern int Detector_Setup_Get_Sensor_Width(void);
extern int Detector_Setup_Get_Sensor_Height(void);
extern int Detector_Setup_Get_Image_Size_Bytes(void);

extern int Detector_Setup_Get_Error_Number(void);
extern void Detector_Setup_Error(void);
extern void Detector_Setup_Error_String(char *error_string);

#endif
