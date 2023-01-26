/** ADAMEm: Coleco ADAM emulator ********************************************/
/**                                                                        **/
/**                                  Asm.h                                 **/
/**                                                                        **/
/** This file contains miscellaneous MS-DOS assembly routines              **/
/**                                                                        **/
/** Copyright (C) Marcel de Kogel 1996,1997,1998,1999                      **/
/**     You are not allowed to distribute this software commercially       **/
/**     Please, notify me, if you make any changes to this file            **/
/****************************************************************************/

unsigned JoyGetPos (void);           /* low word=X, high word=Y             */
void PutImage_Asm (void);            /* Copy DisplayBuf to screen           */
void StartTimer (void);              /* Initialise timer                    */
unsigned ReadTimer (void);           /* Read timer (resolution=1192380Hz)   */
void RestoreTimer (void);            /* Restore timer to its original state */
void nofunc (void);                  /* Do nothing                          */
void __enable (void);                /* Enable interrupts                   */
void __disable (void);               /* Disable interrupts                  */

