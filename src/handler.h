/* ************************************************************************
*   File: handler.h                                     Part of CircleMUD *
*  Usage: header file: prototypes of handling and utility functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


/* utility */
bool		isname(const char *str, const char *namelist);
char *		fname(const char *namelist);
int			get_number(char *name);
bool		silly_isname(const char *str, const char *namelist);
int			split_string(char *str, char *sep, char **argv);

/* ******* characters ********* */

int is_same_group(CharData *ach, CharData *bch );


/* prototypes from crash save system */

void	Crash_listrent(CharData *ch, const char *name);
void	Crash_load(CharData *ch);
void	Crash_crashsave(CharData *ch);
void	Crash_rentsave(CharData *ch);
void	Crash_save_all(void);

/* prototypes from fight.c */
enum
{
	Hit_Normal					= 0,
	Hit_NoMissMessages 			= (1 << 0),
	Hit_IgnoreRange				= (1 << 1),
	Hit_NoTargetRedirection		= (1 << 2),
	
	Hit_Spraying				= Hit_IgnoreRange
};

enum
{
	StartCombat_Normal			= 0,
	StartCombat_NoMessages		= 1 << 0,
	StartCombat_NoTargetChange	= 1 << 1
};


void	start_combat(CharData *ch, CharData *vict, Flags flags);
void	set_fighting(CharData *ch, CharData *victim);
int		hit(CharData *ch, CharData *victim, ObjData *missile, int type, unsigned int times, unsigned int range = 0, Direction dir = NORTH, Flags flags = 0);
int		initiate_combat(CharData *ch, CharData *victim, ObjData *weapon, bool force = false);
void	forget(CharData *ch, CharData *victim);
void	remember(CharData *ch, CharData *victim);
int damage(CharData * ch, CharData * victim, ObjData *weapon, int dam, int attacktype, int damagetype, int times);
int	skill_message(int dam, CharData *ch, CharData *vict, ObjData *weapon, int attacktype, bool ranged);
