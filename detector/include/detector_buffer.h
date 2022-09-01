/* detector_buffer.h */
#ifndef DETECTOR_BUFFER_H
#define DETECTOR_BUFFER_H

extern int Detector_Buffer_Allocate(int size_x,int size_y);
extern int Detector_Buffer_Free(void);

extern int Detector_Buffer_Initialise_Coadd_Image(void);
extern int Detector_Buffer_Add_Mono_To_Coadd_Image(void);
extern int Detector_Buffer_Create_Mean_Image(int coadds);

extern unsigned short* Detector_Buffer_Get_Mono_Image(void);
extern double* Detector_Buffer_Get_Mean_Image(void);
extern int Detector_Buffer_Get_Size_X(void);
extern int Detector_Buffer_Get_Size_Y(void);
extern int Detector_Buffer_Get_Pixel_Count(void);

extern int Detector_Buffer_Get_Error_Number(void);
extern void Detector_Buffer_Error(void);
extern void Detector_Buffer_Error_String(char *error_string);

#endif
