/* liric_multrun.h */
#ifndef LIRIC_MULTRUN_H
#define LIRIC_MULTRUN_H
#include <time.h> /* struct timespec */

extern int Liric_Multrun(int exposure_length_ms,int exposure_count,int do_standard,
			  char ***filename_list,int *filename_count);
extern int Liric_Multrun_Abort(void);

/* status routines */
extern int Liric_Multrun_In_Progress(void);
extern int Liric_Multrun_Count_Get(void);
extern int Liric_Multrun_Exposure_Index_Get(void);

#endif
