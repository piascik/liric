/* raptor_multrun.h */
#ifndef RAPTOR_MULTRUN_H
#define RAPTOR_MULTRUN_H
#include <time.h> /* struct timespec */

extern int Raptor_Multrun(int exposure_length_ms,int exposure_count,int do_standard,
			  char ***filename_list,int *filename_count);
extern int Raptor_Multrun_Abort(void);

/* status routines */
extern int Raptor_Multrun_In_Progress(void);
extern int Raptor_Multrun_Count_Get(void);
extern int Raptor_Multrun_Exposure_Index_Get(void);

#endif
