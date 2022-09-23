/* raptor_config.h */
#ifndef RAPTOR_CONFIG_H
#define RAPTOR_CONFIG_H

extern int Raptor_Config_Load(char *filename);
extern int Raptor_Config_Shutdown(void);
extern int Raptor_Config_Get_String(char *key, char **value);
extern int Raptor_Config_Get_Character(char *key, char *value);
extern int Raptor_Config_Get_Integer(char *key, int *i);
extern int Raptor_Config_Get_Long(char *key, long *l);
extern int Raptor_Config_Get_Unsigned_Short(char *key,unsigned short *us);
extern int Raptor_Config_Get_Double(char *key, double *d);
extern int Raptor_Config_Get_Float(char *key, float *f);
extern int Raptor_Config_Get_Boolean(char *key, int *boolean);
extern int Raptor_Config_Detector_Is_Enabled(void);
extern int Raptor_Config_Nudgeomatic_Is_Enabled(void);
extern int Raptor_Config_Filter_Wheel_Is_Enabled(void);

#endif
