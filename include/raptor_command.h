/* raptor_command.h */
#ifndef RAPTOR_COMMAND_H
#define RAPTOR_COMMAND_H
extern int Raptor_Command_Abort(char *command_string,char **reply_string);
extern int Raptor_Command_Config(char *command_string,char **reply_string);
extern int Raptor_Command_Fits_Header(char *command_string,char **reply_string);
extern int Raptor_Command_Multrun(char *command_string,char **reply_string);
extern int Raptor_Command_Status(char *command_string,char **reply_string);
extern int Raptor_Command_Temperature(char *command_string,char **reply_string);


#endif
