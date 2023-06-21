/* liric_config.h */
#ifndef LIRIC_CONFIG_H
#define LIRIC_CONFIG_H

extern int Liric_Config_Load(char *filename);
extern int Liric_Config_Shutdown(void);
extern int Liric_Config_Get_String(char *key, char **value);
extern int Liric_Config_Get_Character(char *key, char *value);
extern int Liric_Config_Get_Integer(char *key, int *i);
extern int Liric_Config_Get_Long(char *key, long *l);
extern int Liric_Config_Get_Unsigned_Short(char *key,unsigned short *us);
extern int Liric_Config_Get_Double(char *key, double *d);
extern int Liric_Config_Get_Float(char *key, float *f);
extern int Liric_Config_Get_Boolean(char *key, int *boolean);
extern int Liric_Config_Detector_Is_Enabled(void);
extern int Liric_Config_Nudgematic_Is_Enabled(void);
extern int Liric_Config_Filter_Wheel_Is_Enabled(void);

#endif
