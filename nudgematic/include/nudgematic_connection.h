/* nudgematic_connection.h */

#ifndef NUDGEMATIC_CONNECTION_H
#define NUDGEMATIC_CONNECTION_H

extern int Nudgematic_Connection_Open(const char* device_name);
extern int Nudgematic_Connection_Close(void);

extern int Nudgematic_Connection_Get_Error_Number(void);
extern void Nudgematic_Connection_Error(void);
extern void Nudgematic_Connection_Error_To_String(char *error_string);

#endif
