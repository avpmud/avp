/***************************************************************************\
[*]    __     __  ___                ___  [*]   LexiMUD: A feature rich   [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ [*]      C++ MUD codebase       [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / [*] All rights reserved         [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  [*] Copyright(C) 1997-1998      [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   [*] Chris Jacobson (FearItself) [*]
[*]        LexiMUD: Feel the Power        [*] fear@technologist.com       [*]
[-]---------------------------------------+-+-----------------------------[-]
[*] File : skills.h                                                       [*]
[*] Usage: Skill classes and enums                                        [*]
\***************************************************************************/

#include "types.h"
#include "character.defs.h"

enum Difficulty {
	Routine		= 30,
	Easy		= 20,
	Light		= 10,
	Medium		= 0,
	Hard		= -10,
	VHard		= -20,
	XHard		= -30,
	Folly		= -50,
	Absurd		= -70,
	Insane		= -100
};

enum Result {
	Ignored = -1,
	Blunder,
	AFailure,
	Failure,
	PSuccess,
	NSuccess,
	Success,
	ASuccess
};

enum Stat {
	Str = (1 << 0),		//	1
	Int = (1 << 1),		//	2
	Per = (1 << 2),		//	4
	Agi = (1 << 3),		//	8
	Con = (1 << 4)		//	16
};


enum Target {
	Ignore		= (1 << 0),		//	IGNORE TARGET
	CharRoom	= (1 << 1),		//	PC/NPC in room
	CharWorld	= (1 << 2),		//	PC/NPC in world
	FightSelf	= (1 << 3),		//	If fighting, and no argument, target is self
	FightVict	= (1 << 4),		//	If fighting, and no argument, target is opponent
	SelfOnly	= (1 << 5),		//	Flag: Self only
	NotSelf		= (1 << 6),		//	Flag: Anybody BUT self
	ObjInv		= (1 << 7),		//	Object in inventory
	ObjRoom		= (1 << 8),		//	Object in room
	ObjWorld	= (1 << 9),		//	Object in world
	ObjEquip	= (1 << 10)		//	Object held
};


class Skill {
public:
	enum { General, Movement, Combat };
	
	static SInt32		RankScore(SInt32 rank);
	static SInt32		StatBonus(SInt32 stat);
	static Result		Roll(CharData *ch, SInt32 skill, SInt32 modifier, char *argument,
								CharData **vict, ObjData **obj);
	static const char *	Name(SInt32 number);
	static SInt32		Number(const char *name);

	SInt32				number;
	const char *		name;
	unsigned int		type;
	Stat				stats;
	Target				target;
	Difficulty			difficulty;
	SInt8				level[NUM_RACES];
};


extern const SInt32	NumSkills;
extern Skill Skills[];


enum {
	TYPE_UNDEFINED = -1,
	SKILL_RESERVED,

	SKILL_BACKSTAB,			//	1
	SKILL_BASH,
	SKILL_HIDE,
	SKILL_KICK,
	SKILL_PICK_LOCK,		//	5
	SKILL_PUNCH,
	SKILL_RESCUE,
	SKILL_SNEAK,
	SKILL_CIRCLE,
	SKILL_TRACK,			//	10
	SKILL_THROW,
	SKILL_SHOOT,
	SKILL_DODGE,
	SKILL_WATCH,
	SKILL_BITE,				//	15
	SKILL_TRAIN,
	SKILL_COCOON,
	SKILL_IDENTIFY,
	SKILL_CRITICAL,
	SKILL_STUN,			//	20

	//	WEAPON ATTACK TYPES
	TYPE_HIT = 300,
	TYPE_STING,
	TYPE_WHIP,
	TYPE_SLASH,
	TYPE_BITE,
	TYPE_BLUDGEON,			//	305
	TYPE_CRUSH,
	TYPE_POUND,
	TYPE_CLAW,
	TYPE_MAUL,
	TYPE_THRASH,			//	310
	TYPE_PIERCE,
	TYPE_BLAST,
	TYPE_PUNCH,
	TYPE_STAB,

	TYPE_SHOOT,			 	//	315
	TYPE_BURN,

	//	MISC ATTACK TYPES
	TYPE_SUFFERING = 399,
	TYPE_EXPLOSION,			//	400
	TYPE_GRAVITY,
	TYPE_ACIDBLOOD,
	TYPE_MISC
};

#define ACID_BURN_TIME		4



/* Attacktypes with grammar */
struct attack_hit_type {
   char	*singular;
   char	*plural;
};


