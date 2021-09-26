/*************************************************************************
*   File: files.h                    Part of Aliens vs Predator: The MUD *
*  Usage: Header for file handling code                                  *
*************************************************************************/

#ifndef __FILES_H__
#define __FILES_H__

#include "lexistring.h"


extern Lexi::String	fread_lexistring(FILE *fl, char *error, const char *filename);
extern Lexi::String	fread_action(FILE * fl, int nr);

extern void tar_restore_file(const char *filename);

extern Flags asciiflag_conv(const char *flag);


#endif
