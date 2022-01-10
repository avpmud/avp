/***************************************************************************\
[*]    __     __  ___                ___  [*]   LexiMUD: A feature rich   [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ [*]      C++ MUD codebase       [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / [*] All rights reserved         [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  [*] Copyright(C) 1997-1998      [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   [*] Chris Jacobson (FearItself) [*]
[*]        LexiMUD: Feel the Power        [*] fear@technologist.com       [*]
[-]---------------------------------------+-+-----------------------------[-]
[*] File : skills.c++                                                     [*]
[*] Usage: Skill commands                                                 [*]
\***************************************************************************/


#include "skills.h"
#include "characters.h"
#include "utils.h"
#include "interpreter.h"
#include "objects.h"
#include "comm.h"
#include "event.h"
#include "descriptors.h"
#include "handler.h"
#include "rooms.h"
#include "affects.h"
#include "buffer.h"

extern EVENT(combat_event);
extern char *dirs[];
extern char *dir_text_2[];
extern int rev_dir[];


ACMD(new_backstab) {
	CharData *	vict;
	Result		result;
	
	if (!GET_EQ(ch, WEAR_HAND_R) || GET_OBJ_TYPE(GET_EQ(ch, WEAR_HAND_R)) != ITEM_WEAPON) {
		ch->Send("You need to wield a weapon in your good hand to make it a success.\n");
		return;
	}
	if (GET_OBJ_VAL(GET_EQ(ch, WEAR_HAND_R), 3) != TYPE_PIERCE - TYPE_HIT) {
		ch->Send("Only piercing weapons can be used for backstabbing.\n");
		return;
	}
	
	result = Skill::Roll(ch, SKILL_BACKSTAB, 0, argument, &vict, NULL);
	if (result == Ignored)	return;
	
	if (FIGHTING(vict)) {
		ch->Send("You can't backstab a fighting person -- they're too alert!\n");
		return;
	}
	
	if (MOB_FLAGGED(vict, MOB_AWARE)) {
		act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
		act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
		act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
		combat_event(vict, ch, 0);
	} else switch (result) {
		case Blunder:
			act("You cut yourself in attempting to backstab $N!", FALSE, ch, 0, vict, TO_CHAR);
			act("$n cuts $mself in attempting to backstab you!", FALSE, ch, 0, vict, TO_VICT);
			act("$n cuts $mself in attempting to backstab $N!", FALSE, ch, 0, vict, TO_NOTVICT);
			damage(ch, NULL, 50, TYPE_MISC, 1);
			//	And drop through...
		case AFailure:
		case Failure:		hit (ch, vict, 0, 0);				break;
			
		case PSuccess:		hit (ch, vict, 0, 1);				break;
		case NSuccess:		hit (ch, vict, SKILL_BACKSTAB, 1);	break;
		case Success:
			hit (ch, vict, SKILL_BACKSTAB, 1);
			vict->Send("`yYou are stunned!`n\n");
			GET_POS(vict) = POS_STUNNED;
			break;
		case ASuccess:
			hit (ch, vict, SKILL_BACKSTAB, 2);
			vict->Send("`YYou are stunned!`n\n");
			GET_POS(vict) = POS_STUNNED;
			return;		//	No wait state
	}
	WAIT_STATE(ch, 10 RL_SEC);
}



ACMD(new_bash) {
	CharData *	vict;
	Result		result;
	
	if ((!GET_EQ(ch, WEAR_HAND_R) || GET_OBJ_TYPE(GET_EQ(ch, WEAR_HAND_R)) != ITEM_WEAPON) &&
		(!GET_EQ(ch, WEAR_HAND_L) || GET_OBJ_TYPE(GET_EQ(ch, WEAR_HAND_L)) != ITEM_WEAPON)) {
		send_to_char("You need to wield a weapon to make it a success.\n", ch);
		return;
	}
	
	result = Skill::Roll(ch, SKILL_BASH, 0, argument, &vict, NULL);
	if (result == Ignored)	return;
	
	if (FIGHTING(ch) && (vict != FIGHTING(ch))) {
		act("You are too busy fighting $N!", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}

	if (MOB_FLAGGED(vict, MOB_NOBASH))
		result = Failure;
	
	switch (result) {
		case Blunder:
		case AFailure:
			act("Woops!  You tripped!", FALSE, ch, 0, 0, TO_CHAR);
			act("$n trips and falls!", FALSE, ch, 0, 0, TO_ROOM);
			damage(NULL, ch, result == Blunder? 200 : 50, TYPE_MISC, 1);
		case Failure:
			damage(ch, vict, 0, SKILL_BASH, 1);
			GET_POS(ch) = POS_SITTING;
			break;
		case PSuccess:		//	Simply knocks down
			damage(ch, vict, 1, SKILL_BASH, 1);
		case NSuccess:		//	Does a real hit!
		case Success:		//	Double bash attack!
			if (result != PSuccess)
				hit(ch, vict, SKILL_BASH, result == Success? 2 : 1);
			GET_POS(vict) = POS_SITTING;
			WAIT_STATE(vict, PULSE_VIOLENCE);
			break;
		case ASuccess:
			return;
	}
	WAIT_STATE(ch, 5 RL_SEC);
}


ACMD(new_rescue) {
	CharData *	vict, *tmp_ch;
	Result		result;
	
	result = Skill::Roll(ch, SKILL_RESCUE, 0, argument, &vict, NULL);	
	if (result == Ignore)	return;
	
	if (FIGHTING(ch) == vict) {
		send_to_char("How can you rescue someone you are trying to kill?\n", ch);
		return;
	}
	
	LListIterator<CharData *>	iter(world[IN_ROOM(ch)].people);
	while ((tmp_ch = iter.Next()))
		if (FIGHTING(tmp_ch) == vict)
			break;

	if (!tmp_ch) {
		act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}

	switch (result) {
		case Blunder:
		case AFailure:
		case Failure:
			send_to_char("You fail the rescue!\n", ch);
			if (result == Blunder) {
				send_to_char("You fall flat on your ass too!\n", ch);
				GET_POS(ch) = POS_SITTING;
			}
			break;
		case PSuccess:
		case NSuccess:
		case Success:
		case ASuccess:
			send_to_char("Banzai!  To the rescue...\n", ch);
			act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
			act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

			if (FIGHTING(vict) == tmp_ch)	vict->stop_fighting();
			if (FIGHTING(tmp_ch))			tmp_ch->stop_fighting();
			if (FIGHTING(ch))				ch->stop_fighting();

			set_fighting(ch, tmp_ch);
			set_fighting(tmp_ch, ch);
			
			if (result == NSuccess)			hit(ch, tmp_ch, TYPE_UNDEFINED, 1);
			else if (result >= Success)		combat_event(ch, vict, 0);
			if (result == ASuccess)			return;	//	No wait for absolute successes!
			break;
	}

	WAIT_STATE(vict, 10 RL_SEC);
}


ACMD(new_kick_bite) {
	CharData *	vict;
	Result		result;
	
	result = Skill::Roll(ch, subcmd, 0, argument, &vict, NULL);
	if (result == Ignore)	return;
	
	switch (result) {
		case Blunder:
		case AFailure:
		case Failure:
			damage(ch, vict, 0, subcmd, 1);
			break;
		case PSuccess:
		case NSuccess:
		case Success:
		case ASuccess:
			//	Add 30 to make it a minimum of 5
			damage(ch, vict, (Skill::StatBonus(GET_STR(ch)) + 30) *
					(1 + result - PSuccess), subcmd, 1);
			if (result == ASuccess)	return;		//	No wait
			break;
	}
	WAIT_STATE(ch, 6 RL_SEC);
}


ACMD(new_circle) {
	CharData *	victim;
	Result		result;
	
	result = Skill::Roll(ch, SKILL_CIRCLE, 0, argument, &victim, NULL);
	if (result == Ignore)	return;
	
	switch (result) {
		case Blunder:
			act("$n leaps to the side of $N, stumbles, and falls flat on $s face!", FALSE, ch, 0, victim, TO_NOTVICT);
			act("You try to circle behind $N, but stumble and fall flat on your face!", FALSE, ch, 0, victim, TO_CHAR);
			act("$n manages to fall flat on $s face when $e leaps at you.", FALSE, ch, 0, victim, TO_VICT);			
			GET_POS(victim) = POS_STUNNED;
			WAIT_STATE(ch, 20 RL_SEC);
			return;
		case AFailure:
		case Failure:
			act("$n tries to circle around $N, but $N tracks $s moves.", FALSE, ch, 0, victim, TO_NOTVICT);
			act("You try to circle behind $N, but $E manages to keep up with you.", FALSE, ch, 0, victim, TO_CHAR);
			act("$n tries to circle around you, but you manage to follow $m.", FALSE, ch, 0, victim, TO_VICT);			
			break;
		case PSuccess:
		case NSuccess:
		case Success:
		case ASuccess:
			hit(ch, victim, SKILL_CIRCLE, MIN(1, result - PSuccess));
			if (result >= Success)	GET_POS(victim) = POS_STUNNED;
			if (result == ASuccess)	return;
			break;
	}
	WAIT_STATE(ch, (result <= PSuccess ? 15 : 10) RL_SEC);
}


ACMD(new_sneak) {
	Result		result;
	Affect *	af;
	SInt32		door;
	
	if (AFF_FLAGGED(ch, AFF_SNEAK))
		Affect::FromChar(ch, Affect::Sneak);
	
	result = Skill::Roll(ch, SKILL_SNEAK, 0, argument, NULL, NULL);
	
	switch (result) {
		case Blunder:
			ch->Send("Woops!  You accidentally make a loud noise!");
			for (door = 0; door < NUM_OF_DIRS; door++) {
				if (EXIT(ch, door) && (EXIT(ch, door)->to_room != NOWHERE))
					world[EXIT(ch, door)->to_room].Send("You hear a loud noise %s you!", dir_text_2[rev_dir[door]]);			
			}
			break;
		case PSuccess:
		case NSuccess:
		case Success:
		case ASuccess:
			af = new Affect(Affect::Sneak, 0, APPLY_NONE, AFF_SNEAK);
			af->ToChar(ch, Skill::RankScore(GET_SKILL(ch, SKILL_SNEAK)) / 4);
		case AFailure:	//	Fall through - so if they fail, they THINK they're sneaking.
		case Failure:
			ch->Send("Okay, you'll try to move silently for a while.\n");
			if (result == PSuccess) {
				ch->Send("You're a bit nervous, so you take a breather first.\n");
				WAIT_STATE(ch, 10 RL_SEC);
			}
			break;
	}
}


ACMD(new_hide) {
	Result		result;
	SInt32		door;

	//	Eventually make this take into account lighting and terrain!  Muahahahah...
	result = Skill::Roll(ch, SKILL_HIDE, 0, argument, NULL, NULL);
	
	if (AFF_FLAGGED(ch, AFF_HIDE))
		REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);
	
	switch (result) {
		case Blunder:
			ch->Send("Woops!  You accidentally make a loud noise!");
			for (door = 0; door < NUM_OF_DIRS; door++) {
				if (EXIT(ch, door) && (EXIT(ch, door)->to_room != NOWHERE))
					world[EXIT(ch, door)->to_room].Send("You hear a loud noise %s you!", dir_text_2[rev_dir[door]]);			
			}
			break;
		case PSuccess:
		case NSuccess:
		case Success:
		case ASuccess:
			SET_BIT(AFF_FLAGS(ch), AFF_HIDE);
		case AFailure:	//	Fall through, so if they fail, they THINK they're hiding...
		case Failure:
			send_to_char("You attempt to hide yourself.\n", ch);
			if (result == PSuccess) {
				ch->Send("You have a difficult time finding decent cover, so it will take you a few\n"
						"seconds to hide.\n");
				WAIT_STATE(ch, 10 RL_SEC);
			} else if (result == ASuccess)
				return;
			break;
	}
	WAIT_STATE(ch, 5 RL_SEC);
}


ACMD(new_watch) {
	SInt32		dir;
	char *		arg;
	Result		result;
	
	arg = get_buffer(MAX_INPUT_LENGTH);
	argument = one_argument(argument, arg);
	
	if (!*arg) {
		CHAR_WATCHING(ch) = 0;
		ch->Send("You stop watching.\n");
	} else if ((dir = search_block(arg, dirs, FALSE)) >= 0) {
		ch->Send("You begin watching %s you.\n", dir_text_2[dir]);
		
		result = Skill::Roll(ch, SKILL_WATCH, 0, argument, NULL, NULL);
		if (result == Ignored)	return;

		switch (result) {
			case Blunder:
			case AFailure:
			case Failure:
				break;
			case PSuccess:
			case NSuccess:
			case Success:
			case ASuccess:
				CHAR_WATCHING(ch) = dir + 1;
				break;
		}
	} else
		ch->Send("That's not a valid direction...\n");
	release_buffer(arg);
}
