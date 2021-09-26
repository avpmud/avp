/***************************************************************************\
[*]    __     __  ___                ___  [*]   LexiMUD: A feature rich   [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ [*]      C++ MUD codebase       [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / [*] All rights reserved         [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  [*] Copyright(C) 1997-1998      [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   [*] Chris Jacobson (FearItself) [*]
[*]        LexiMUD: Feel the Power        [*] fear@technologist.com       [*]
[-]---------------------------------------+-+-----------------------------[-]
[*] File : skillengine.c++                                                [*]
[*] Usage: Defines skill protodata and drives the skill engine            [*]
\***************************************************************************/


#include "skills.h"
#include "characters.h"
#include "utils.h"
#include "buffer.h"
#include "interpreter.h"
#include "find.h"
#include "rooms.h"

const SInt32	NumSkills = 19;
Skill Skills[NumSkills + 1] = {
	{ },
	{ SKILL_BACKSTAB, "backstab", Skill::Combat, Agi, (Target)(CharRoom | NotSelf), Hard,
		{1, 1, 1, 1, 1}},									//	Backstab
	{ SKILL_BASH, "bash", Skill::Combat, (Stat)(Str | Agi), (Target)(CharRoom | FightVict | NotSelf), Easy,
		{1, 1, 1, 1, 1}},									//	Bash
	{ SKILL_HIDE, "hide", Skill::Movement, (Stat)(Agi | Per), Ignore, Medium,
		{1, 1, 1, 1, 1}},									//	Hide
	{ SKILL_KICK, "kick", Skill::Combat, Str, (Target)(CharRoom | FightVict | NotSelf), Light,
		{1, 1, 1, 101, 1}},									//	Kick
	{ SKILL_PICK_LOCK, "pick lock", Skill::General, Agi, Ignore, Hard,
		{1, 1, 1, 1, 1}},									//	Pick Lock
	{ SKILL_PUNCH, "melee", Skill::Combat, Str, Ignore, Medium,
		{1, 1, 1, 1, 1}},									//	Punch (Melee)
	{ SKILL_RESCUE, "rescue", Skill::Combat, (Stat)0, (Target)(CharRoom | NotSelf), Easy,
		{1, 1, 1, 1, 1}},									//	Rescue
	{ SKILL_SNEAK, "sneak", Skill::Movement, (Stat)(Agi | Per), Ignore, Medium,
		{1, 1, 1, 1, 1}},									//	Sneak
	{ SKILL_CIRCLE, "circle", Skill::Combat, (Stat)(Agi | Per | Int ), (Target)(CharRoom | FightVict | NotSelf), VHard,
		{1, 1, 1, 1, 1}},									//	Circle
	{ SKILL_TRACK, "track", Skill::General, (Stat)(Per | Int), (Target)(CharWorld | NotSelf), VHard,
		{1, 1, 1, 1, 1}},									//	Track
	{ SKILL_THROW, "throw", Skill::Combat, (Stat)(Agi | Str), Ignore, Easy,
		{1, 1, 1, 1, 1}},									//	Throw
	{ SKILL_SHOOT, "shoot", Skill::Combat, Agi, Ignore, Medium,
		{1, 1, 1, 1, 1}},									//	Shoot
	{ SKILL_DODGE, "dodge", Skill::Combat, (Stat)(Agi | Per), Ignore, Medium,
		{1, 1, 1, 1, 1}},									//	Dodge
	{ SKILL_WATCH, "watch", Skill::General, Per, Ignore, Light,
		{1, 1, 1, 1, 1}},									//	Watch
	{ SKILL_BITE, "bite", Skill::Combat, Str, (Target)(CharRoom | FightVict | NotSelf), Light,
		{101, 101, 101, 1, 1}},								//	Bite
	{ SKILL_TRAIN, "train", Skill::General, Int, (Target)(CharRoom | NotSelf), XHard,
		{1, 1, 1, 1, 1}},									//	Train
	{ SKILL_COCOON, "cocoon", Skill::General, (Stat)0, (Target)(CharRoom | NotSelf), Routine,
		{101, 101, 101, 1, 101}},							//	Cocoon
	{ SKILL_IDENTIFY, "identify", Skill::General, (Stat)(Per | Int), (Target)(ObjInv | ObjRoom | ObjEquip), Hard,
		{1, 1, 1, 1, 1}},									//	Identify
	{ SKILL_CRITICAL, "critical hit", Skill::Combat, (Stat)(Per | Str | Agi), (Ignore), VHard,
		{1, 1, 1, 1, 1}}									//	Critical Hit
};


/*
const SInt32 SkillBase[] = {
	-25,		//	0
	  5,  10,  15,  20,  25,  30,  35,  40,  45,  50,		//	1-10
	 52,  54,  56,  58,  60,  62,  64,  66,  68,  70,		//	11-20
	 71,  72,  73,  74,  75,  76,  77,  78,  79,  80,		//	21-30
	
	 81,  81,  82,  82,  83,  83,  84,  84,  85,  85,		//	31-40
	 86,  86,  87,  87,  88,  88,  89,  89,  90,  90,		//	41-50
	 91,  91,  92,  92,  93,  93,  94,  94,  95,  95,		//	51-60
	 96,  96,  97,  97,  98,  98,  99,  99, 100, 100,		//	61-70
};
*/

SInt32 Skill::RankScore(SInt32 rank) {
//	return ((rank > 30) ? 80 + ((rank - 30) * .5) : SkillBase[rank]);
	return (rank == 0) ? -25 :
			(rank <= 10 ? rank * 5 : (50 +
			(rank <= 20 ? (rank - 10) * 2 : (20 +
			(rank <= 30 ? (rank - 20) : (10 +
			((rank - 30) / 2)))))));
}

SInt32 Skill::StatBonus(SInt32 stat) {
	if (stat <= 1)						return -25;
	else if (stat == 2)					return -20;
	else if (stat >= 3 && stat <= 4)	return -15;
	else if (stat >= 5 && stat <= 9)	return -10;
	else if (stat >= 10 && stat <= 24)	return -5;
	else if (stat >= 25 && stat <= 74)	return 0;
	else if (stat >= 75 && stat <= 89)	return 5;
	else if (stat >= 90 && stat <= 94)	return 10;
	else if (stat >= 95 && stat <= 97)	return 15;
	else if (stat >= 98 && stat <= 99)	return 20;
/*
	else if (stat == 100)				return 25;
	else if (stat == 101)				return 30;
	else if (stat >= 102)				return 35;
*/
	else return (20 +
			(stat <= 104 ? (stat - 99) * 5 : (25 +
			(stat <= 109 ? (stat - 104) * 4 : (20 +
			(stat <= 114 ? (stat - 109) * 3 : (15 +
			(stat <= 119 ? (stat - 114) * 2 : (10 +
			(stat - 119))))))))));
}


Result Skill::Roll(CharData *ch, SInt32 skill, SInt32 modifier, char *argument, CharData **vict, ObjData **obj)
{
	SInt32	result, bitCount = 0, bonus = 0;
	CharData *	target_ch;
	ObjData *	target_obj;
	bool		target = false;
	char *		buf;
	
	if (argument)	skip_spaces(argument);
	
	if (IS_SET(Skills[skill].target, Ignore))
		target = true;
	else if (argument && (*argument)) {
		buf = get_buffer(MAX_INPUT_LENGTH);
		argument = one_argument(argument, buf);
		if (!target && (IS_SET(Skills[skill].target, CharRoom)))
			if ((target_ch = get_char_room_vis(ch, buf)))
				target = true;
		if (!target && IS_SET(Skills[skill].target, CharWorld))
			if ((target_ch = get_char_vis(ch, buf)))
				target = true;
		if (!target && IS_SET(Skills[skill].target, ObjInv))
			if ((target_obj = get_obj_in_list_vis(ch, buf, ch->carrying)))
				target = true;
		if (!target && IS_SET(Skills[skill].target, ObjEquip))
			if ((target_obj = get_object_in_equip_vis(ch, buf, ch->equipment, &bonus)))
				target = true;
		if (!target && IS_SET(Skills[skill].target, ObjRoom))
			if ((target_obj = get_obj_in_list_vis(ch, buf, world[IN_ROOM(ch)].contents)))
				target = true;
		if (!target && IS_SET(Skills[skill].target, ObjWorld))
			if ((target_obj = get_obj_vis(ch, buf)))
				target = true;
		release_buffer(buf);
	} else {
		if (!target && IS_SET(Skills[skill].target, FightSelf) && FIGHTING(ch)) {
			target_ch = ch;
			target = true;
		}
		if (!target && IS_SET(Skills[skill].target, FightVict) && (target_ch = FIGHTING(ch)))
			target = true;
//		if (!target && IS_SET(Skills[skill].target, CharRoom) &&
//				!IS_SET(Skills[skill].target, NotSelf)) {
//			target_ch = ch;
//			target = true;
//		}
		if (!target && IS_SET(Skills[skill].target, SelfOnly)) {
			target_ch = ch;
			target = true;
		}
		if (!target) {
			ch->Send("You need to specify a target...\n");
			return Ignored;
		}
	}
	if (target) {
		if ((target_ch == ch) && IS_SET(Skills[skill].target, NotSelf)) {
			ch->Send("You can't do that to yourself!\n");
			return Ignored;
		}
		if ((target_ch != ch) && IS_SET(Skills[skill].target, SelfOnly)) {
			ch->Send("You can only do that to yourself.\n");
			return Ignored;
		}
	} else {
		ch->Send("You need to specify a target\n");
		return Ignored;
	}
	
	if (vict)	*vict = target_ch;
	if (obj)	*obj = target_obj;
	
	result = ::Number(1, 100);
	
	if (IS_NPC(ch)) {
		buf = get_buffer(MAX_INPUT_LENGTH);
		one_word(argument, buf);
//		if (argument) do {
//			argument = one_word(argument, buf);
//			skip_spaces(argument);
//		} while (*argument);
		result += is_number(buf) ? atoi(buf) : 0;
		release_buffer(buf);
	} else			result += Skill::RankScore(GET_SKILL(ch, skill));
	
	result += Skills[skill].difficulty;

	if (IS_SET(Skills[skill].stats, Str))	{ bonus += Skill::StatBonus(GET_STR(ch)); bitCount++; }
	if (IS_SET(Skills[skill].stats, Int))	{ bonus += Skill::StatBonus(GET_INT(ch)); bitCount++; }
	if (IS_SET(Skills[skill].stats, Per))	{ bonus += Skill::StatBonus(GET_PER(ch)); bitCount++; }
	if (IS_SET(Skills[skill].stats, Agi))	{ bonus += Skill::StatBonus(GET_AGI(ch)); bitCount++; }
	if (IS_SET(Skills[skill].stats, Con))	{ bonus += Skill::StatBonus(GET_CON(ch)); bitCount++; }
	result += bonus / bitCount;
	
	if (result <= -26)							return Blunder;
	else if (result >= -25 && result <= 4)		return AFailure;
	else if (result >= 5 && result <= 75)		return Failure;
	else if (result >= 76 && result <= 90)		return PSuccess;
	else if (result >= 91 && result <= 110)		return NSuccess;
	else if (result >= 111 && result <= 175)	return Success;
	else										return ASuccess;
//	return Ignored;
}


const char *Skill::Name(SInt32 skill) {
	if (skill < 1)				return (skill == -1) ? "UNUSED" : "UNDEFINED";
	if (skill > NumSkills)		return "UNDEFINED";

	return Skills[skill].name;
}

SInt32 Skill::Number(const char *name) {
	for (SInt32 index = 1; index <= NumSkills; index++) {
		if (is_abbrev(name, Skills[index].name))
			return index;
	}
	return -1;
}
