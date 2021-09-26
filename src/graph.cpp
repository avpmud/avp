/* ************************************************************************
*   File: graph.c                                       Part of CircleMUD *
*  Usage: various graph algorithms                                        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


/*
 * You can define or not define TRACK_THOUGH_DOORS, depending on whether
 * or not you want track to find paths which lead through closed or
 * hidden doors.
 */
#define TRACK_THROUGH_DOORS	0




#include "structs.h"
#include "scripts.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "db.h"
#include "spells.h"
#include "constants.h"


/* Locals */
int find_first_step(RoomData *room, char *target);
ACMD(do_track);

/* Utility macros */

/************************************************************************
*  Functions and Commands which use the above fns		        *
************************************************************************/

ACMD(do_track) {
	CharData *vict;
	int dir;
	
	/* The character must have the track skill. */
//	if (!GET_SKILL(ch, SKILL_TRACK)) {
//		send_to_char("You have no idea how.\n", ch);
//		return;
//	}

	BUFFER(arg, MAX_INPUT_LENGTH);
	one_argument(argument, arg);
	
	if (!*arg)
		send_to_char("Whom are you trying to track?\n", ch);
	else if (!IS_STAFF(ch))
		ch->Send("You can't track.\n");
//	else if (AFF_FLAGGED(vict, AFF_NOTRACK))
//		send_to_char("You sense no trail.\n", ch);
//	else if (Number(0, 101) >= GET_SKILL(ch, SKILL_TRACK))
	else if (0) {
		/* 101 is a complete failure, no matter what the proficiency. */
		/* Find a random direction. */
		int x, found = 0;
		for (x=0; x < NUM_OF_DIRS && !found; x++)
			found = RoomData::CanGo(ch, x);
		if (!found)
			send_to_char("You are in a closed room.\n", ch);
		else {
			do {
				dir = Number(0, NUM_OF_DIRS - 1);
			} while (!RoomData::CanGo(ch, dir));
			ch->Send("You sense a trail %s from here!\n", dirs[dir]);
		}
	} else if ((dir = find_first_step(ch->InRoom(), arg)) == -1) {
		if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
			send_to_char("No one is around by that name.\n", ch);
		else if (IN_ROOM(ch) == IN_ROOM(vict))
			send_to_char("You're already in the same room!!\n", ch);
		else
			act("You can't sense a trail to $M from here.\n", TRUE, ch, 0, vict, TO_CHAR);
	} else
		ch->Send("You sense a trail %s from here!\n", dirs[dir]);
}
