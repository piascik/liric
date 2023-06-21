/* liric_bias_dark.h */
#ifndef LIRIC_BIAS_DARK_H
#define LIRIC_BIAS_DARK_H
#include <time.h> /* struct timespec */

extern int Liric_Bias_Dark_MultBias(int exposure_count,char ***filename_list,int *filename_count);
extern int Liric_Bias_Dark_MultDark(int exposure_length_ms,int exposure_count,
				     char ***filename_list,int *filename_count);
extern int Liric_Bias_Dark_Abort(void);

/* status routines */
extern int Liric_Bias_Dark_In_Progress(void);
extern int Liric_Bias_Dark_Count_Get(void);
extern int Liric_Bias_Dark_Exposure_Index_Get(void);

#endif
