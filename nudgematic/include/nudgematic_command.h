/* nudgematic_command.h */

#ifndef NUDGEMATIC_COMMAND_H
#define NUDGEMATIC_COMMAND_H

/**
 * Enumeration type defining the magnitudes of offsets induced by the nudgematic, this can be one of:
 * <ul>
 * <li>UNKNOWN
 * <li>SMALL
 * <li>LARGE
 * </ul>
 */
enum NUDGEMATIC_OFFSET_SIZE_ENUM 
{
	UNKNOWN = 0,
	SMALL = 1,
	LARGE = 2
};

/**
 * Typedef of an Enumeration type defining the magnitudes of offsets induced by the nudgematic.
 * @see #NUDGEMATIC_OFFSET_SIZE_ENUM
 */
typedef enum NUDGEMATIC_OFFSET_SIZE_ENUM NUDGEMATIC_OFFSET_SIZE_T;

extern int Nudgematic_Command_Position_Set(int position);
extern int Nudgematic_Command_Position_Get(int *position);

extern int Nudgematic_Command_Offset_Size_Set(NUDGEMATIC_OFFSET_SIZE_T size);
extern int Nudgematic_Command_Offset_Size_Get(NUDGEMATIC_OFFSET_SIZE_T *size);
extern int Nudgematic_Command_Offset_Size_Parse(char *offset_size_string, NUDGEMATIC_OFFSET_SIZE_T *size);
extern char *Nudgematic_Command_Offset_Size_To_String(NUDGEMATIC_OFFSET_SIZE_T size);

extern int Nudgematic_Command_Get_Error_Number(void);
extern void Nudgematic_Command_Error(void);
extern void Nudgematic_Command_Error_To_String(char *error_string);

#endif
