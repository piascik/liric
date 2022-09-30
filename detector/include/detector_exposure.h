/* detector_exposure.h */
#ifndef DETECTOR_EXPOSURE_H
#define DETECTOR_EXPOSURE_H

#include <time.h>

extern int Detector_Exposure_Set_Coadd_Frame_Exposure_Length(int coadd_frame_exposure_length_ms);
extern int Detector_Exposure_Expose(int exposure_length_ms,char* fits_filename);

extern int Detector_Exposure_Coadd_Frame_Exposure_Length_Get(void);
extern int Detector_Exposure_Exposure_Length_Get(void);
extern int Detector_Exposure_Coadd_Count_Get(void);
extern struct timespec Detector_Exposure_Start_Time_Get(void);

extern int Detector_Exposure_Get_Error_Number(void);
extern void Detector_Exposure_Error(void);
extern void Detector_Exposure_Error_String(char *error_string);

#endif
