/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 2007             [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Original LexiMUD Code                                                 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : virtualid.h                                                    [*]
[*] Usage: A zonename:# scheme for referencing in-game assets.            [*]
[*]        Replaces traditional Diku VNums                                [*]
\***************************************************************************/


#ifndef __VIRTUALID_H__
#define __VIRTUALID_H__


#include "types.h"
#include "lexistring.h"

//	VirtualID is a "zonename:category:asset#" scheme
struct VirtualID
{
						VirtualID() : zone(0), ns(0), number(0)   {}
						VirtualID(const char *str, Hash n = 0) : zone(0), ns(0), number(0) { Parse(str, n); }
						VirtualID(VNum vn) : zone(0), ns(0), number(0) { Parse(vn); }
	
	bool				IsValid() const { return zone != 0; }
	
	bool				operator==(const VirtualID &vid) const { return zone == vid.zone && ns == vid.ns && number == vid.number; }
	bool				operator!=(const VirtualID &vid) const { return !(*this == vid); }
	bool				operator<(const VirtualID &vid) const
	{
		return (zone < vid.zone)
			|| ((zone == vid.zone)
				&& ((ns < vid.ns) ||
					((ns == vid.ns) && number < vid.number)));
	}
	bool				operator>(const VirtualID &vid) const
	{
		return (zone > vid.zone)
			|| ((zone == vid.zone)
				&& ((ns > vid.ns) ||
					((ns == vid.ns) && number > vid.number)));
	}
	bool				operator<=(const VirtualID &vid) const { return !(*this > vid); }
	bool				operator>=(const VirtualID &vid) const { return !(*this < vid); }

//	VirtualID &			operator=(const char *str) { Parse(str); return *this; }
//	VirtualID &			operator=(VNum vn) { Parse(vn); return *this; }
	
	void				Clear() { zone = 0; ns = 0; number = 0; }
	
	void				Parse(const char *str, Hash relativeZone = 0);
	void				Parse(VNum vnum);
	Lexi::String		Print(Hash relativeZone = 0) const;

	Hash				zone;		//	Hashed name of zone
	Hash				ns;			//	Hashed namespace within zone
	unsigned int		number;		//	Number of asset in zone:namespace
};

const bool NO_VNUMS = true;
extern bool IsVirtualID(const char *str, bool bNoVnums = false);


namespace VIDSystem
{
	void				AddZoneAlias(ZoneData *zone, Hash alias);
	ZoneData *			GetZoneFromAlias(Hash alias);
	
/*
	extern void			RegisterZoneName(const char *name);
	const char *		GetZoneName(Hash hash);
	
	bool				IsZoneNameRegistered(const char *name);
	bool				IsZoneNameRegistered(Hash hash);
*/
}

namespace VNumLegacy
{
	struct Zone
	{
		Zone() : name(0), base(0) {}
		
		Hash				name;
		VNum				base;
	};
	
	void				RegisterZone(ZoneData *zone, unsigned int znum, VNum top);
	Zone *				FindZone(VNum vnum);
	VNum				GetVirtualNumber(VirtualID vid);
	unsigned int		MaxTopForZone(int zoneNumber);
}


#endif
