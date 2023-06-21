/* liric_command.h */
#ifndef LIRIC_COMMAND_H
#define LIRIC_COMMAND_H
extern int Liric_Command_Abort(char *command_string,char **reply_string);
extern int Liric_Command_Config(char *command_string,char **reply_string);
extern int Liric_Command_Fan(char *command_string,char **reply_string);
extern int Liric_Command_Fits_Header(char *command_string,char **reply_string);
extern int Liric_Command_Multrun(char *command_string,char **reply_string);
extern int Liric_Command_MultBias(char *command_string,char **reply_string);
extern int Liric_Command_MultDark(char *command_string,char **reply_string);
extern int Liric_Command_Status(char *command_string,char **reply_string);
extern int Liric_Command_Temperature(char *command_string,char **reply_string);

extern int Liric_Command_Initialise_Detector(char *coadd_exposure_length_string);

#endif
