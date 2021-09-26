/* ************************************************************************
*   File: act.offensive.c                               Part of CircleMUD *
*  Usage: player-level commands of an offensive nature                    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "structs.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "db.h"
#include "spells.h"
#include "help.h"
#include "event.h"
#include "constants.h"
#include "affects.h"

/* extern variables */
extern int pk_allowed;

/* extern functions */
void AlterMove(CharData *ch, int amount);
bool kill_valid(CharData *ch, CharData *vict);

/* Daniel Houghton's revised external functions */
void mob_reaction(CharData *ch, CharData *vict, int dir);
CharData * scan_target(CharData *ch, char *arg, int range, int &dir, int *distance_result, bool isGun);
int fire_missile(CharData *ch, char *arg1, ObjData *missile, int range, int dir);
void spray_missile(CharData *ch, ObjData *missile, int range, int dir, int rounds);
void check_killer(CharData * ch, CharData * vict);

/* prototypes */
ACMD(do_assist);
ACMD(do_hit);
ACMD(do_kill);
ACMD(do_backstab);
ACMD(do_order);
ACMD(do_flee);
ACMD(do_bash);
ACMD(do_rescue);
ACMD(do_kick_bite);
ACMD(do_shoot);
ACMD(do_throw);
ACMD(do_circle);
ACMD(do_pkill);
ACMD(do_stun);
ACMD(do_spray);
ACMD(do_mode);
ACMD(do_retreat);


ACMD(do_assist) {
	CharData *helpee, *opponent;
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	if (FIGHTING(ch)) {
		send_to_char("You're already fighting!  How can you assist someone else?\n", ch);
		return;
	}
	
	one_argument(argument, arg);

	if (!*arg)
		send_to_char("Whom do you wish to assist?\n", ch);
	else if (!(helpee = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
		send_to_char(NOPERSON, ch);
	else if (helpee == ch)
		send_to_char("You can't help yourself any more than this!\n", ch);
	else {
		opponent = FIGHTING(helpee);
		
		if (!opponent)
		{
			FOREACH(CharacterList, ch->InRoom()->people, iter)
			{
				if (FIGHTING(*iter) == helpee)
				{
					opponent = *iter;
					break;
				}
			}
		}

		if (!opponent)
			act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
		else if (ch->CanSee(opponent) == VISIBLE_NONE)
			act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
		else {
			send_to_char("You join the fight!\n", ch);
			act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
			act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
			
			initiate_combat(ch, opponent, NULL);
		}
	}
}


ACMD(do_hit) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	CharData *vict;
	
	one_argument(argument, arg);

	if (!*arg)
		send_to_char("Hit who?\n", ch);
	else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		send_to_char("They don't seem to be here.\n", ch);
//		WAIT_STATE(ch, 2 RL_SEC);
	} else if (vict == ch) {
		send_to_char("You hit yourself...OUCH!\n", ch);
		act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
	} else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->m_Following == vict))
		act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
	//else if (AFF_FLAGGED(ch, AFF_INVISIBLE_FLAGS))
	//	send_to_char("You can't fight while invisible.  It is not honorable!\n", ch);
	else {
		start_combat(ch, vict, StartCombat_Normal);
	}
}



ACMD(do_kill) {
	CharData *vict;
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	if (!NO_STAFF_HASSLE(ch) || IS_NPC(ch)) {
		do_hit(ch, argument, "hit", subcmd);
		return;
	}
	
	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Kill who?\n", ch);
	} else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
		send_to_char("They aren't here.\n", ch);
	else if (ch == vict)
		send_to_char("Your mother would be so sad...\n", ch);
	else {
		act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, vict, TO_CHAR);
		act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
		act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);
		vict->AbortTimer();
		vict->die(ch);
	}
}



#if 0
ACMD(do_backstab) {
	CharData *vict;
	BUFFER(buf, MAX_INPUT_LENGTH);
	
	one_argument(argument, buf);
	
	vict = get_char_vis(ch, buf, FIND_CHAR_ROOM);
	
	if (!vict)
		send_to_char("Backstab who?\n", ch);
	else if (vict == ch)
		send_to_char("How can you sneak up on yourself?\n", ch);
	else if (!GET_EQ(ch, WEAR_HAND_R) || GET_OBJ_TYPE(GET_EQ(ch, WEAR_HAND_R)) != ITEM_WEAPON)
		send_to_char("You need to wield a weapon in your good hand to make it a success.\n", ch);
	else if (GET_EQ(ch, WEAR_HAND_R)->GetValue(3) != TYPE_PIERCE - TYPE_HIT)
		send_to_char("Only piercing weapons can be used for backstabbing.\n", ch);
	else if (FIGHTING(vict))
		send_to_char("You can't backstab a fighting person -- they're too alert!\n", ch);
	else if (AFF_FLAGGED(ch, AFF_INVISIBLE_FLAGS))
		send_to_char("It is dishonorable to backstab someone while invisible!\n", ch);
	else if (MOB_FLAGGED(vict, MOB_AWARE) && !FIGHTING(vict))
	{
		act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
		act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
		act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
		
		initiate_combat(vict, ch, NULL);
		WAIT_STATE(ch, 10 RL_SEC);
	}
	else
		ch->Send("Backstab is temporarily disabled.\n");
	
	return;	//	Disabled
	
#if 0
	percent = Number(1, 101);	/* 101% is a complete failure */
//	prob = GET_SKILL(ch, SKILL_BACKSTAB);
	if (IS_NPC(ch)) {
		one_argument(argument, buf);
		prob = atoi(buf);
	}// else
//		prob = GET_SKILL(ch, SKILL_BACKSTAB);

	if (AWAKE(vict) && (percent > prob))
		damage(ch, vict, 0, TYPE_BACKSTAB, 1);
	else
		hit(ch, vict, ch->GetWeapon(), TYPE_BACKSTAB, 1);
	WAIT_STATE(ch, 10 RL_SEC);
#endif
}
#endif


ACMD(do_order) {
	int found = FALSE;
	CharData *vict;
	BUFFER(name, MAX_INPUT_LENGTH);
	BUFFER(message, MAX_INPUT_LENGTH);

	half_chop(argument, name, message);

	if (!*name || !*message)
		send_to_char("Order who to do what?\n", ch);
	else if (!(vict = get_char_vis(ch, name, FIND_CHAR_ROOM)) && !is_abbrev(name, "followers"))
		send_to_char("That person isn't here.\n", ch);
	else if (ch == vict)
		send_to_char("You obviously suffer from skitzofrenia.\n", ch);

	else {
		if (AFF_FLAGGED(ch, AFF_CHARM)) {
			send_to_char("Your superior would not aprove of you giving orders.\n", ch);
		} else if (vict) {
			act("$N orders you to '$t'", FALSE, vict, (ObjData *)message, ch, TO_CHAR);
			act("$n gives $N an order.", FALSE, ch, 0, vict, TO_NOTVICT);

			if ((vict->m_Following != ch) || !AFF_FLAGGED(vict, AFF_CHARM))
				act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
			else {
				send_to_char(OK, ch);
				command_interpreter(vict, message);
			}
		} else {			/* This is order "followers" */
			act("$n issues the order '$t'.", FALSE, ch, (ObjData *)message, vict, TO_ROOM);

			RoomData *org_room = ch->InRoom();
			
			FOREACH(CharacterList, ch->m_Followers, iter)
			{
				CharData *follower = *iter;
				
				if ((org_room == follower->InRoom()) && AFF_FLAGGED(follower, AFF_CHARM)) {
					found = TRUE;
					command_interpreter(follower, message);
				}
			}
			if (found)	send_to_char(OK, ch);
			else		send_to_char("Nobody here is a loyal subject of yours!\n", ch);
		}
	}
}


ACMD(do_flee) {
	int	chosenDir = -1;
	int i, attempt;
	
	skip_spaces(argument);
	
	if (GET_POS(ch) < POS_FIGHTING) {
		ch->Send("You are in pretty bad shape, unable to flee!\n");
		return;
	}
	
	if (Event::FindEvent(ch->m_Events, GravityEvent::Type)) {
		ch->Send("You are falling, and can't grab anything!\n");
		return;
	}
	
	if (GET_MOVE(ch) >= 5)
	{
		for (i = 0; i < /*4*/ 6; i++) {
			attempt = Number(0, NUM_OF_DIRS - 1);	/* Select a random direction */
			if (RoomData::CanGo(ch, attempt) && !ROOM_FLAGGED(Exit::Get(ch, attempt)->ToRoom(), ROOM_GRAVITY) &&
/*					!EXIT(ch, attempt)->ToRoom->GetFlags().test(ROOM_DEATH)*/
					Number(1,2) == 1) {
				RoomData *	oldRoom = ch->InRoom();
				act("$n panics, and attempts to flee!", FALSE, ch, 0, 0, TO_ROOM);
				AlterMove(ch, -5);
				
				if (do_simple_move(ch, attempt, MOVE_FLEEING) && oldRoom != ch->InRoom()) {
					send_to_char("You flee head over heels.\n", ch);
					FOREACH(CharacterList, oldRoom->people, iter)
					{
						CharData *vict = *iter;
						if (FIGHTING(vict) == ch)
						{
							vict->redirect_fighting();
						
							if (!FIGHTING(vict))
								Affect::FlagsFromChar(vict, AFF_NOSHOOT);
						}
					}
					if (FIGHTING(ch))
					{
						ch->stop_fighting();
							
						(new Affect("FLEE", 0, 0, AFF_DIDFLEE))->Join(ch, 3 RL_SEC);
					}
				} else {
					break;
				}
				return;
			}
		}
	}
	
	WAIT_STATE(ch, 2 RL_SEC);
	act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
	send_to_char("PANIC!  You couldn't escape!\n", ch);
}


ACMD(do_retreat)
{
	int	dir;
	int numFollowers;
	
	skip_spaces(argument);
	
	numFollowers = 0;
	if (AFF_FLAGGED(ch, AFF_GROUP))
	{
		FOREACH(CharacterList, ch->m_Followers, iter)
		{
			CharData *follower = *iter;
			
			if (AFF_FLAGGED(follower, AFF_GROUP)
				&& IN_ROOM(follower) == IN_ROOM(ch)
				&& !PRF_FLAGGED(follower, PRF_NOAUTOFOLLOW))
			{
				++numFollowers;
			}
		}
	}
	
	if (numFollowers == 0)
		ch->Send("You aren't a group leader!\n");
	else if (GET_POS(ch) < POS_FIGHTING)
		ch->Send("You are in pretty bad shape, unable to retreat!\n");
	else if (Event::FindEvent(ch->m_Events, GravityEvent::Type))
		ch->Send("You are falling, and unable to retreat!\n");
	else if ((dir = search_block(argument, dirs, FALSE)) == -1)
		ch->Send("What direction?\n");
	else if (GET_MOVE(ch) < 20)
		ch->Send("You are too exhausted to make a retreat!\n");
	else
	{
		CharData *vict;
		
		AlterMove(ch, -10);
		
		if (RoomData::CanGo(ch, dir) && !ROOM_FLAGGED(Exit::Get(ch, dir)->ToRoom(), ROOM_GRAVITY) &&
				/* KNO check */
				((SkillRoll((GET_EFFECTIVE_KNO(ch) + GET_EFFECTIVE_PER(ch)) * 2 / 3) - MAX(numFollowers, 3)) > 0)) {
			ch->Send("You order a retreat %sward!", dirs[dir]);
			act("$n orders a retreat $Tward!", TRUE, ch, 0, dirs[dir], TO_ROOM);
			
			FOREACH(CharacterList, ch->m_Followers, iter)
			{
				CharData *follower = *iter;

				if (AFF_FLAGGED(follower, AFF_GROUP)
					&& IN_ROOM(follower) == IN_ROOM(ch)
					&& !PRF_FLAGGED(follower, PRF_NOAUTOFOLLOW))
				{
					vict = FIGHTING(follower);
					if (do_simple_move(follower, dir, 0))
					{
						follower->Send("You retreat %s.\n", dirs[dir]);
						if (vict) {
							if (FIGHTING(vict) == follower)
								vict->redirect_fighting();
#if !ROAMING_COMBAT
							follower->stop_fighting();
#endif

							(new Affect("FLEE", 0, 0, AFF_DIDFLEE))->Join(follower, 3 RL_SEC);
						}
					} else {
						act("$n tries to retreat, but can't!", TRUE, follower, 0, 0, TO_ROOM);
					}
				}
			}
			
			vict = FIGHTING(ch);
			if (do_simple_move(ch, dir, 0))
			{
				ch->Send("You retreat %s.\n", dirs[dir]);
				if (vict)
				{
					if (FIGHTING(vict) == ch)
						vict->redirect_fighting();
#if !ROAMING_COMBAT
					ch->stop_fighting();
#endif

					(new Affect("FLEE", 0, 0, AFF_DIDFLEE))->Join(ch, 3 RL_SEC);
				}
			} else {
				act("$n tries to retreat, but can't!", TRUE, ch, 0, 0, TO_ROOM);
			}
		}
		else
		{
			act("$n orders a retreat, but the group is unable to disengage!", TRUE, ch, 0, 0, TO_ROOM);
			send_to_char("You retreat order was in vain.\n", ch);
		}


		WAIT_STATE(ch, 3 RL_SEC);
	}
}

#if 0
ACMD(do_bash) {
	CharData *vict;
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	one_argument(argument, arg);

	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		if (FIGHTING(ch) && (IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))) {
			vict = FIGHTING(ch);
		} else {
			send_to_char("Bash who?\n", ch);
			return;
		}
	}
	
	if (vict == ch)
		send_to_char("Aren't we funny today...\n", ch);
	else if (!ch->GetWeapon())
		send_to_char("You need to wield a weapon to make it a success.\n", ch);
	else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && !NO_STAFF_HASSLE(ch))
		ch->Send("Not in a safe room...\n");
	else if (!kill_valid(ch, vict))
	{
		if (!PRF_FLAGGED(ch, PRF_PK))
			send_to_char("You must turn on PK first!  Type \"pkill\" for more information.\n", ch);
		else if (!PRF_FLAGGED(vict, PRF_PK))
			act("You cannot kill $N because $E has not chosen to participate in PK.", FALSE, ch, 0, vict, TO_CHAR);
		else
			send_to_char("You can't player-kill here.\n", ch);
	}
	else {
		int percent = Number(1, 101);	/* 101% is a complete failure */
		int prob = 0;
	//	prob = GET_SKILL(ch, SKILL_BASH);
		if (IS_NPC(ch)) {
			one_argument(argument, arg);
			prob = atoi(arg);
		} //else
//			prob = GET_SKILL(ch, SKILL_BASH);

		if (MOB_FLAGGED(vict, MOB_NOBASH))
			percent = 101;
		if (percent > prob) {
			damage(ch, vict, weapon, 0, TYPE_BASH, DAMAGE_UNKNOWN, 1);
			GET_POS(ch) = POS_SITTING;
		} else {
			//	There's a bug in the two lines below.  We could fail bash and they sit.
			damage(ch, vict, 1, TYPE_BASH, DAMAGE_UNKNOWN, 1);
			GET_POS(vict) = POS_STUNNED;
	//		WAIT_STATE(vict, PULSE_VIOLENCE * 2);
		}
		WAIT_STATE(ch, PULSE_VIOLENCE * 2);
	}
}
#endif

ACMD(do_rescue) {
	CharData *vict;
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	one_argument(argument, arg);

	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
		send_to_char("Whom do you want to rescue?\n", ch);
	else if (vict == ch)
		send_to_char("What about fleeing instead?\n", ch);
	else if (FIGHTING(ch) == vict)
		send_to_char("How can you rescue someone you are trying to kill?\n", ch);
	else if (!kill_valid(ch, FIGHTING(vict)))
	{
		if (!PRF_FLAGGED(ch, PRF_PK))
			send_to_char("You must turn on PK first!  Type \"pkill\" for more information.\n", ch);
		else if (!PRF_FLAGGED(vict, PRF_PK))
			act("You cannot kill $N because $E has not chosen to participate in PK.", FALSE, ch, 0, vict, TO_CHAR);
		else
			send_to_char("You can't player-kill here.\n", ch);
	}
	else
	{
		CharData *tmp_ch = NULL;
		FOREACH(CharacterList, ch->InRoom()->people, iter)
		{
			if (FIGHTING(*iter) == vict)
			{
				tmp_ch = *iter;
				break;
			}
		}
		

		if (!tmp_ch)
			act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
		else
		{
			send_to_char("Banzai!  To the rescue...\n", ch);
			act("You are rescued by $N!", FALSE, vict, 0, ch, TO_CHAR);
			act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

/*			if (FIGHTING(vict) == tmp_ch)
				vict->stop_fighting();
			if (FIGHTING(tmp_ch))
				tmp_ch->stop_fighting();
			if (FIGHTING(ch))
				ch->stop_fighting();

			set_fighting(ch, tmp_ch);
			set_fighting(tmp_ch, ch);
*/

			if (!FIGHTING(ch))
			{
				ch->AddOrReplaceEvent(new CombatEvent(1, ch));
				set_fighting(ch, tmp_ch);
			}

			FIGHTING(tmp_ch) = ch;
			check_killer(ch, tmp_ch);
//			WAIT_STATE(vict, PULSE_VIOLENCE * 2);
		}
	}
}

#if 0
ACMD(do_kick_bite) {
	CharData *vict;
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	one_argument(argument, arg);

	if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		if (FIGHTING(ch)) {
			vict = FIGHTING(ch);
		} else {
			if (subcmd == SCMD_KICK)
				send_to_char("Kick who?\n", ch);
			else
				send_to_char("Bite who?\n", ch);
			return;
		}
	}
	
	if (vict == ch) {
		send_to_char("Aren't we funny today...\n", ch);
		return;
	}
	int percent = ((10 - /*(GET_AC(vict) / 10)*/ 5) * 2) + Number(1, 101);
	/* 101% is a complete failure */

	int prob = 0;
	if (IS_NPC(ch)) {
		one_argument(argument, arg);
		prob = atoi(arg);
	} //else
//		prob = GET_SKILL(ch, subcmd);
//	prob = GET_SKILL(ch, subcmd);

	if (percent > prob) {
		damage(ch, vict, 0, subcmd, DAMAGE_UNKNOWN, 1);
	} else
		damage(ch, vict, (ch->GetLevel() * 5) + 100, subcmd, DAMAGE_UNKNOWN, 1);
	WAIT_STATE(ch, 6 RL_SEC);
}
#endif

ACMD(do_shoot) {
	ObjData *gun;
	int range, dir = -1, fired;
	
	if (GET_POS(ch) == POS_FIGHTING) {
		send_to_char("You are too busy fighting!\n", ch);
		return;
	}
	
	if (!ch->InRoom() || ROOM_FLAGGED(ch->InRoom(), ROOM_PEACEFUL)) {
		send_to_char("But it is so peaceful here...\n", ch);
		return;
	}
	
	gun = ch->GetGun();

	if (!gun) {
		send_to_char("You aren't wielding a fireable weapon!\n", ch);
		return;
	}

	if (GET_GUN_AMMO_TYPE(gun) != -1 && GET_GUN_AMMO(gun) <= 0) {
		send_to_char("*CLICK* Weapon isn't loaded!\n", ch);
		return;
	}

	if (!(range = GET_GUN_RANGE(gun))) {
		send_to_char("This is not a ranged weapon!\n", ch);
		return;
	}
	
	if (OBJ_FLAGGED(gun, ITEM_NOSHOOT)) {
		send_to_char("This weapon is unable to be used for aimed firing!\n", ch);
		return;
	}
	
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_GRAVITY)) {
		send_to_char("You are too busy trying to keep from falling!\n", ch);
		return;
	}

	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	
	argument = two_arguments(argument, arg1, arg2);

	if (!*arg1)
	{
		if (!FIGHTING(ch))
			send_to_char("You should try: shoot <someone> [<direction>]\n", ch);
		else
			ch->stop_fighting();
	}
//	else if (IS_DARK(IN_ROOM(ch)))
//		send_to_char("You can't see that far.\n", ch);
	else if (*arg2 && (dir = search_block(arg2, dirs, FALSE)) < 0)
		send_to_char("What direction?\n", ch);
	else if (*arg2 && !Exit::IsOpen(IN_ROOM(ch), dir))
		send_to_char("Something blocks the way!\n", ch);
	else if (*arg2 && EXIT_FLAGGED(Exit::Get(ch, dir), EX_NOSHOOT))
		send_to_char("It seems as if something is in the way!\n", ch);
	else if (AFF_FLAGGED(ch, AFF_NOSHOOT) /*&& FIGHTING(ch)*/)
	{
#if ROAMING_COMBAT // && ENABLE_ROAMING_COMBAT
		if (OBJ_FLAGGED(gun, ITEM_AUTOSNIPE))
		{
			CharData *vict = scan_target(ch, arg1, range, dir, NULL, true);
			if (!vict)
				send_to_char("Can't find your target!\n", ch);
			else if (FIGHTING(ch) == vict || (FIGHTING(ch) && isname(arg1, FIGHTING(ch)->GetAlias())))
				act("You are already fighting $N.", FALSE, ch, 0, FIGHTING(ch), TO_CHAR);
			else if (FIGHTING(ch))
			{
				check_killer(ch, vict);
				act("You change your target to $N.", FALSE, ch, 0, vict, TO_CHAR);
				act("$n changes $s target to $N!", FALSE, ch, 0, vict, TO_ROOM);
				FIGHTING(ch) = vict;
			}
			else
			{
				act("You start shooting at $N.", FALSE, ch, 0, vict, TO_CHAR);
				ch->AddOrReplaceEvent(new CombatEvent(1, ch, true));
				set_fighting(ch, vict);
			}
		}
#endif
	}
	else
	{
		if (range > 8)
			range = 8;
		if (range < 1)
			range = 1;
		
/*		if (dir != -1)
			fired = fire_missile(ch, arg1, gun, range, dir);	//	NOTE: no skill used
		else {
			for (dir = 0; dir < NUM_OF_DIRS; dir++) {
				if ((fired = fire_missile(ch, arg1, gun, range, dir)))	//	NOTE: no skill used
					break;
			}
		}
*/
		fired = fire_missile(ch, arg1, gun, range, dir);

		if (fired == -3)send_to_char("Can't find your target!\n", ch);
//		else			WAIT_STATE(ch, 1 RL_SEC);
	}
}


ACMD(do_throw) {
/* sample format: throw monkey east
   this would throw a throwable or grenade object wielded
   into the room 1 east of the pc's current room. The chance
   to hit the monkey would be calculated based on the pc's skill.
   if the wielded object is a grenade then it does not 'hit' for
   damage, it is merely dropped into the room. (the timer is set
   with the 'pull pin' command.) */

	ObjData *missile;
	int dir, range = 1;
	char *searchArg;
	int fired;
	
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && !NO_STAFF_HASSLE(ch))
	{
		ch->Send("But its so peaceful here...");
		return;
	}
	
/* only two types of throwing objects: 
   ITEM_THROW - knives, stones, etc
   ITEM_GRENADE - calls tick_grenades.c . */

	missile = GET_EQ(ch, WEAR_HAND_L);
	if (!missile || ((GET_OBJ_TYPE(missile) != ITEM_THROW) &&
			(GET_OBJ_TYPE(missile) != ITEM_GRENADE) &&
			(GET_OBJ_TYPE(missile) != ITEM_BOOMERANG)))
		missile = GET_EQ(ch, WEAR_HAND_R);

	if (!missile || ((GET_OBJ_TYPE(missile) != ITEM_THROW) &&
			(GET_OBJ_TYPE(missile) != ITEM_GRENADE) &&
			(GET_OBJ_TYPE(missile) != ITEM_BOOMERANG))) { 
		send_to_char("You should hold a throwing weapon first!\n", ch);
		return;
	}

	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	
	two_arguments(argument, arg1, arg2);
	
	if (!str_cmp(arg1, "into") || !str_cmp(arg2, "out"))
	{
		range = 0;
		searchArg = arg1;
	}
	else if (GET_OBJ_TYPE(missile) == ITEM_GRENADE)
	{
		range = *arg2 ? RANGE(atoi(arg2), 1, 5) : 1;
		searchArg = arg1;
	}
	else
	{
		range = missile->GetValue(OBJVAL_THROWABLE_RANGE);
		searchArg = arg2;
	}
	
#define DIR_OUT		6
#define DIR_INTO	7

	ObjData *hatch = NULL;
	ObjData *vehicle = NULL;
	RoomData *vehicle_room = NULL;
	if (*searchArg)
	{
		if (!str_cmp(searchArg, "into"))
		{
			dir = DIR_INTO;
			
			vehicle = get_obj_in_list_vis(ch, arg2, ch->InRoom()->contents);
			
			if (!vehicle)
			{
				act("Throw it into what?  You see no such thing.", TRUE, ch, NULL, NULL, TO_CHAR);
				return;
			}
			else if (GET_OBJ_TYPE(vehicle) != ITEM_VEHICLE || IS_MOUNTABLE(vehicle))
			{
				act("You cannot throw anything into $p.", TRUE, ch, vehicle, NULL, TO_CHAR);
				return;
			}
			else if ((vehicle_room = world.Find(vehicle->GetVIDValue(OBJVIDVAL_VEHICLE_ENTRY))) == NULL)
			{
				send_to_char("Serious problem: Vehicle has no insides!\n", ch);
				return;
			}
		}
		else if (!str_cmp(searchArg, "out"))
		{
			dir = DIR_OUT;
			
			hatch = get_obj_in_list_type(ITEM_VEHICLE_HATCH, ch->InRoom()->contents);
			if (hatch)
			{
				vehicle = find_vehicle_by_vid(hatch->GetVIDValue(OBJVIDVAL_VEHICLEITEM_VEHICLE));
				if (!vehicle || vehicle->InRoom() == NULL) {
					send_to_char("The hatch seems to lead to nowhere...\n", ch);
					return;
				}
			}
			else
			{
				send_to_char("Throw it out?  How?\n", ch);
				return;
			}
		}
		else
			dir = search_block(searchArg, dirs, FALSE);
	}
	else
		dir = -1;
	
	if (dir == -1 && GET_OBJ_TYPE(missile) == ITEM_GRENADE)
		send_to_char("You should try: throw <direction> [<distance>]\n", ch);
	else if (dir != -1 && dir < DIR_OUT && !Exit::IsViewable(IN_ROOM(ch), dir))
		send_to_char("Something blocks the way!\n", ch);
	else if (dir != -1 && dir < DIR_OUT && EXIT_FLAGGED(Exit::Get(ch, dir), EX_NOSHOOT))
		send_to_char("It seems as if something is in the way!\n", ch);
	else if (GET_OBJ_TYPE(missile) == ITEM_GRENADE)
	{
		if (range > 8)
			range = 8;
		if (range < 1)
			range = 1;
		
		RoomData *	room;
		int			distance = 0;
		
	
		if (dir == DIR_INTO)
		{
			act("You throw $p into $P!", FALSE, ch, missile, vehicle, TO_CHAR);
			act("$n throws $p into $P.", FALSE, ch, missile, vehicle, TO_ROOM);
			
			room = vehicle_room;
		}
		else if (dir == DIR_OUT)
		{
			act("You throw $p out of $P!", FALSE, ch, missile, vehicle, TO_CHAR);
			act("$n throws $p out of $P.", FALSE, ch, missile, vehicle, TO_ROOM);

			room = IN_ROOM(vehicle);
		}
		else
		{
			act("You throw $p $T!", FALSE, ch, missile, dirs[dir], TO_CHAR);
			act("$n throws $p $T.", FALSE, ch, missile, dirs[dir], TO_ROOM);
			
			room = IN_ROOM(ch);
		
			for (distance = 0; distance < range; distance++) {
				if (Exit::IsOpen(room, dir) && !EXIT_FLAGGED(Exit::Get(room, dir), EX_NOSHOOT)) {
	//				if ((skill / (distance + 1)) < Number(1, 101)) {
	//					if (distance < 1) {
	//						act("The $p slips out of your hand and falls to the floor!", FALSE, ch, missile, 0, TO_CHAR);
	//						act("$n accidentally drops $p at his feet!", FALSE, ch, missile, 0, TO_ROOM);
	//					}
	//					break;
	//				} else
					if (distance && !room->people.empty())
						act("$p flies by from the $T!", FALSE, room->people.front(), missile, dir_text_the[rev_dir[dir]], TO_CHAR | TO_ROOM);
					room = Exit::Get(room, dir)->ToRoom();
				} else
					break;
			}
		}
		
		unequip_char(ch, missile->WornOn())->ToRoom(room);
		
		if (dir == DIR_OUT)
			act("$p falls out of $P.", FALSE, 0, missile, vehicle, TO_ROOM);
		else if (dir == DIR_INTO)
			act("$p flies into $P and lands on the floor.", FALSE, 0, missile, vehicle, TO_ROOM);
		else if (distance > 0)
			act("$p flies in from the $T.", FALSE, 0, missile, dir_text_the[rev_dir[dir]], TO_ROOM);
		
		if (Event::FindEvent(missile->m_Events, GrenadeExplosionEvent::Type))
		{
			if (!IS_NPC(ch)) {
				(new Affect("COMBAT", 0, APPLY_NONE, AFF_NOQUIT))->Join(ch, 10 RL_SEC);
			}
			
			if (!ROOM_FLAGGED(room, ROOM_PEACEFUL))
			{
				FOREACH(CharacterList, room->people, iter)
				{
					CharData *vict = *iter;
					if (!IS_NPC(vict)) {
						(new Affect("COMBAT", 0, APPLY_NONE, AFF_NOQUIT))->Join(vict, 20 RL_SEC);
						break;
					}
				}
			}
		}
	}
	else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_GRAVITY))
		send_to_char("You are too busy trying to keep from falling!\n", ch);
	else if (AFF_FLAGGED(ch, AFF_NOSHOOT))
		;
	else if (FIGHTING(ch))
		send_to_char("You are too busy fighting!\n", ch);
	else
	{
		if (range > 8)
			range = 8;
		if (range < 1)
			range = 1;
		
		fired = fire_missile(ch, arg1, missile, range, dir);
		
		if (fired == -3)send_to_char("Can't find your target!\n", ch);
	}
	
}


ACMD(do_circle) {
	CharData *victim = NULL;
	int prob, percent = 0;
	BUFFER(arg, MAX_INPUT_LENGTH);

	argument = one_argument(argument, arg);
	
	if (!(victim = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
		if (FIGHTING(ch))
			victim = FIGHTING(ch);
	}
	
	if (IS_NPC(ch)) {
		one_argument(argument, arg);
		prob = atoi(arg);
	} else
		prob = 0 /*GET_SKILL(ch, SKILL_CIRCLE)*/;
	
	if (!prob)
		send_to_char("You aren't quite skilled enough to pull off a stunt like that...", ch);
	else if (victim == ch)
		send_to_char("Are you stupid or something?", ch);
	else if (victim) {
		percent = Number(1, 100);
//		percent += GET_PER(victim);		// PER to notice
//		percent += GET_AGI(victim);		// DEX to react
//		percent += GET_INT(victim) / 2;	// INT to not fall for it
//		percent -= GET_AGI(ch);			// DEX to try
//		percent -= GET_INT(victim) / 2;	// INT to outwit
		if (percent <= prob) {
//			act("$n circles around $N, confusing the hell out of $M.", FALSE, ch, 0, victim, TO_NOTVICT);
//			act("You circle behind $N, confusing $M, and getting a chance to strike!", FALSE, ch, 0, victim, TO_CHAR);
//			act("$n leaps to the $t, and you lose track of $m.", FALSE, ch, (ObjData *)(Number(0,1) ? "right" : "left"), victim, TO_VICT);
			hit(ch, victim, ch->GetWeapon(), TYPE_CIRCLE, 2);
			if (percent < 0)	GET_POS(victim) = POS_STUNNED;
//		} else if (percent >= 100 && (Number(1,100) > GET_AGI(ch))) {
//			act("$n leaps to the side of $N, stumbles, and falls flat on $s face!", FALSE, ch, 0, victim, TO_NOTVICT);
//			act("You try to circle behind $N, but stumble and fall flat on your face!", FALSE, ch, 0, victim, TO_CHAR);
//			act("$n manages to fall flat on $s face when $e leaps at you.", FALSE, ch, 0, victim, TO_VICT);			
			GET_POS(victim) = POS_STUNNED;
		} else {
			act("$n tries to circle around $N, but $N tracks $s moves.", FALSE, ch, 0, victim, TO_NOTVICT);
			act("You try to circle behind $N, but $E manages to keep up with you.", FALSE, ch, 0, victim, TO_CHAR);
			act("$n tries to circle around you, but you manage to follow $m.", FALSE, ch, 0, victim, TO_VICT);			
		}
		WAIT_STATE(ch, 10 RL_SEC);
	} else
		act("Circle whom?", TRUE, ch, 0, 0, TO_CHAR);
}


ACMD(do_pkill) {
	HelpManager::Entry *el = HelpManager::Instance()->Find("PKILL");
	
	if (!el)
		return;
	
	if (IS_NPC(ch)) {
		send_to_char("You're a MOB, you can already kill players!", ch);
		return;
	}

	BUFFER(arg, MAX_STRING_LENGTH);
	
	strcpy(arg, el->m_Entry.c_str());
	strcat(arg, "\n");
	if (!str_cmp(argument, "yes")) {
		SET_BIT(PRF_FLAGS(ch), PRF_PK);
		strcat(arg, "`RYou are now a player killer!`n");
	} else {
		strcat(arg, "To begin pkilling, type \"`RPKILL YES`n\".\n"
					"Remember, there is no turning back!`n");
	}
	page_string(ch->desc, arg);
}

/*
ACMD(do_stun) {
	CharData *vic;
	int prob = 0;	//IS_NPC(ch) ? atoi(argument) : GET_SKILL(ch, SKILL_STUN);

	if (!IS_STAFF(ch))
		ch->Send("Huh?!?\n");
	else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
		ch->Send("Not in a safe room...\n");
	else if (GET_MOVE(ch) < 50)
		ch->Send("You are too exhausted to do that right now.\n");
	else if (Number(1, 101) > prob) {
		ch->Send("You try to whail, but it comes out wrong.\n");
		act("$n tries to whail, but it comes out wrong.", FALSE, ch, 0, 0, TO_ROOM);
		AlterMove(ch, -50);
	} else {
		ch->Send("You whail loudly, filling the room with a horrible sound!\n");
		act("$n whails loudly, filling the room with a horrible sound!", FALSE, ch, 0, 0, TO_ROOM);
	
		START_ITER(iter, vic, CharData *, ch->InRoom()->people) {
			if (vic != ch && !IS_STAFF(vic) && !AFF_FLAGGED(vic, AFF_WHAILPROOF)) {
				vic->Send("You grab your ears in pain!\n"); 
				act("$n grabs $s ears in pain!\n", TRUE, vic, 0, 0, TO_ROOM);
				WAIT_STATE(vic, ((ch->GetLevel() / 20) + 1) RL_SEC);
				GET_POS(vic) = POS_STUNNED;
			}
		}
		AlterMove(ch, -50);
		WAIT_STATE(ch, 10);
	}
}
*/


ACMD(do_spray) {
	ObjData *gun;
	int dir, rounds;
	
	if (GET_POS(ch) == POS_FIGHTING) {
		send_to_char("You are too busy fighting!\n", ch);
		return;
	}
	
	if (ch->InRoom() == NULL || ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && !NO_STAFF_HASSLE(ch)) {
		send_to_char("But it is so peaceful here...\n", ch);
		return;
	}
	
	gun = ch->GetGun();
	
	if (!gun)
	{
		send_to_char("You aren't wielding a fireable weapon!\n", ch);
		return;
	}
	
	if (GET_GUN_AMMO_TYPE(gun) != -1 && GET_GUN_AMMO(gun) <= 0) {
		send_to_char("*CLICK* Weapon isn't loaded!\n", ch);
		return;
	}
	
	if (GET_GUN_RANGE(gun) <= 0) {
		send_to_char("This is not a ranged weapon!\n", ch);
		return;
	}
	
	if (!OBJ_FLAGGED(gun, ITEM_SPRAY)) {
		send_to_char("This weapon is useless at unaimed firing!\n", ch);
		return;
	}
	
	BUFFER(arg1, MAX_INPUT_LENGTH);

	argument = one_argument(argument, arg1);

	if (!*arg1)
		ch->Send("You should try: spray <direction>\n");
//	else if (IS_DARK(IN_ROOM(ch)))
//		send_to_char("You can't see that far.\n", ch);
	else if ((dir = search_block(arg1, dirs, FALSE)) < 0)
		send_to_char("What direction?\n", ch);
	else if (!Exit::IsOpen(ch, dir))
		send_to_char("Something blocks the way!\n", ch);
	else if (EXIT_FLAGGED(Exit::Get(ch, dir), EX_NOSHOOT))
		send_to_char("It seems as if something is in the way!\n", ch);
	else if (AFF_FLAGGED(ch, AFF_NOSHOOT))
		;
	else
	{
		//	2*rate, if they have the ammo for it.  Otherwise empty the magazine
		rounds = GET_GUN_RATE(gun) * 2;
		if (GET_GUN_AMMO_TYPE(gun) != -1)
			rounds = MIN((int)GET_GUN_AMMO(gun), rounds);
		
		spray_missile(ch, gun, GET_GUN_RANGE(gun), dir, rounds);	//	NOTE: no skill used
		
//		WAIT_STATE(ch, 1 RL_SEC);
	}
}


ACMD(do_mode)
{
	int newmode;
	
	if (!*argument)
		ch->Send("You are fighting %sly.\n", combat_modes[GET_COMBAT_MODE(ch)]);	
	else if ((newmode = search_block(argument, combat_modes, false)) == -1)
		ch->Send("Invalid combat mode.  You may fight DEFENSIVE, NORMAL, or OFFENSIVE.\n");
	else if (newmode == GET_COMBAT_MODE(ch))
		ch->Send("You are already fighting %sly.\n", combat_modes[GET_COMBAT_MODE(ch)]);
	else
	{
		if (newmode == COMBAT_MODE_NORMAL)	ch->Send("You stop fighting %sly.\n", combat_modes[GET_COMBAT_MODE(ch)]);
		else								ch->Send("You begin fighting %sly.\n", combat_modes[newmode]);
		GET_COMBAT_MODE(ch) = (CombatMode)newmode;
	}
}
