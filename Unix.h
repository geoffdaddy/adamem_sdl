/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                 Unix.h                                 **/
/**                                                                        **/
/** This file contains general Unix routines prototypes used by both the   **/
/** SVGALib and the X-Windows versions of ADAMEm                           **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

typedef struct
{
 int buttons;
 int x;
 int y;
} joypos_t;

int InitJoystick (int mode);
void TrashJoystick (void);
void ReadJoystick (joypos_t *jp);

int ReadTimer (void);
