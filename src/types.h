/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-2005        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Original LexiMUD Code                                                 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : types.h                                                        [*]
[*] Usage: Basic data types                                               [*]
\***************************************************************************/

#ifndef __TYPES_H__
#define __TYPES_H__

#include "conf.h"
#include "sysdep.h"

//	Basic Types
typedef unsigned int	UInt32;
typedef void *			Ptr;


//	Lexi Types
typedef short			VNum;

typedef unsigned int	Flags;

//typedef unsigned long long			XFlags;
//typedef	unsigned int	Type;
typedef unsigned long	CRC32;
typedef unsigned long	Hash;
typedef unsigned short	Hash16;

typedef unsigned int	IDNum;


template<typename T>
struct Range
{
	Range(T mn, T mx) : min(mn), max(mx) {}
	
	T				Enforce(T v) { return (v < min ? min : (v > max ? max : v)); }
	
	T				min;
	T				max;
private: Range();
};


enum
{
	Nothing = -1,
	Nowhere = -1,
	Nobody = -1,
	NoID = 0,
	
	NOTHING = -1,
	NOWHERE = -1,
	NOBODY = -1
};



// Shared Strings

//	This structure is purely intended to be an easy way to transfer
//	and return information about time (real or mudwise).
struct TimeInfoData {
	int		minutes, hours, day, month, year;
	int		total_hours;
	int		weekday;
};


class Entity;
class CharData;
class ObjData;
class DescriptorData;
class RoomData;
class TrigData;
class ZoneData;
class Scriptable;
class Event;

typedef void(*ActionCommandFunction) (CharData *ch, char * argument, const char *command, int subcmd);

//CRC32 GetCRC32(const unsigned char *buf, unsigned int len, CRC32 crc = 0);
CRC32 GetStringCRC32(const char *buf, unsigned int len);
CRC32 GetStringCRC32(const char *buf);
Hash GetStringFastHash(const char *buf, unsigned int len, Hash hash = 0);
Hash GetStringFastHash(const char *buf);
Hash16 GetStringFastHash16(const char *buf, unsigned int len, Hash hash = 0);
Hash16 GetStringFastHash16(const char *buf);

#define HASH_SANITY_CHECKS

void ReportHashCollision(Hash hash, const char *lhsName, const char *rhsName);

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE  (!FALSE)
#endif



#endif	// __TYPES_H__

