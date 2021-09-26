/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 2008             [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Original LexiMUD Code                                                 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : races.h                                                        [*]
[*] Usage: Races and related code                                         [*]
\***************************************************************************/

#ifndef __RACES_H_
#define __RACES_H_

#include "bitflags.h"

namespace Lexi
{
	class Parser;
	class OutputParserFile;
}

enum
{
	MAX_RACES_SUPPORTED = 32
};

typedef Lexi::BitFlags<MAX_RACES_SUPPORTED> RaceFlags;


//	TEMPORARY
enum Race {
	RACE_UNDEFINED	= -1,
	RACE_HUMAN,
	RACE_SYNTHETIC,
	RACE_PREDATOR,
	RACE_ALIEN,
	RACE_OTHER,
	NUM_RACES
};
extern Race factions[NUM_RACES];

Race ParseRace(char arg);
Race ParseRace(const char *arg);
RaceFlags GetRaceFlag(char arg);
RaceFlags GetRaceFlag(const char *arg);

Lexi::String GetDisplayRaceRestrictionFlags(RaceFlags flags);


#endif // __RACES_H__
