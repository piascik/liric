/* detector_setup.h */
#ifndef DETECTOR_SETUP_H
#define DETECTOR_SETUP_H

extern int Detector_Setup_Startup(char *format_filename);
extern int Detector_Setup_Shutdown(void);

extern int Detector_Setup_Open(char *driverparms,char *formatname, char *formatfile);
extern int Detector_Setup_Close(void);

extern int Detector_Setup_Get_Sensor_Size_X(void);
extern int Detector_Setup_Get_Sensor_Size_Y(void);
extern int Detector_Setup_Get_Image_Size_Pixels(void);

extern int Detector_Setup_Get_Error_Number(void);
extern void Detector_Setup_Error(void);
extern void Detector_Setup_Error_String(char *error_string);

#endif
