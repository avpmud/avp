/* ************************************************************************
*   File: comm.h                                        Part of CircleMUD *
*  Usage: header file: prototypes of public communication functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define NUM_RESERVED_DESCS	8

#define COPYOVER_FILE "copyover.dat"
#define IDENTCOPYOVER_FILE "identcopyover.dat"


/* comm.c */
/*
 * Backward compatibility macros.
 */
void	send_to_players(CharData *ch, const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
void	send_to_all(const char *messg);
void	send_to_char(const char *messg, CharData *ch);
void	send_to_zone(char *messg, ZoneData *zone, Hash ns = 0);
void	send_to_zone_race(char *messg, ZoneData *zone, int race, Hash ns = 0);

void	act(const char *str, int hide_invisible, CharData *ch, ObjData *obj, const void *vict_obj, int type, char *storeBuf = NULL);

void sub_write(char *arg, CharData *ch, bool find_invis, int targets);

#define TO_ROOM		(1 << 0)
#define TO_VICT		(1 << 1)
#define TO_NOTVICT	(1 << 2)
#define TO_CHAR		(1 << 3)
#define TO_ZONE		(1 << 4)
#define TO_GAME		(1 << 5)
#define TO_NOTTRIG	(1 << 7)
#define TO_SLEEP	(1 << 8)	/* to char, even if sleeping */
#define TO_IGNOREINVIS (1 << 9)
#define TO_FULLYVISIBLE_ONLY (1 << 10)
#define TO_TEAM		(1 << 11)
#define TO_SAYBUF	(1 << 12)
#define TO_IGNORABLE	(1 << 13)
#define TO_SCRIPT	(1 << 14)

void	page_string(DescriptorData *d, const char *str);

#define USING_SMALL(d)	((d)->output == (d)->small_outbuf)
#define USING_LARGE(d)  (!USING_SMALL(d))

typedef RETSIGTYPE sigfunc(int);

extern FILE *logfile;	/* Where to send messages from log() */
