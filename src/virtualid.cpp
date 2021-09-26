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


#include "virtualid.h"
#include <map>

#include "zones.h"
#include "interpreter.h"
#include "buffer.h"


namespace VIDSystem
{
	typedef std::map<Hash, Lexi::String>	ZoneNameMap;
	ZoneNameMap			registeredZoneNames;
}


void VirtualID::Parse(const char *str, Hash relativeZone)
{
	Clear();	//	Initialize to invalid
	
	skip_spaces(str);
	
	if (!*str)	return;
	
	if (is_number(str))	//	Classic VNum; is_number will return false on a #:
	{
		//	Get classic VNum
		Parse(atoi(str));
		return;
	}
	
	const char *p = str;
	
	bool	bUnknownVID = (*str == '#');
	if (bUnknownVID)		++str;
	
	while (isalnum(*str))	++str;
	
	if (*str != ':' || (!isalnum(str[1]) && str[1] != '#'))	return;	//	No ':#'?  Not a VirtualID
	
	//	The zonename starts at p and ends at str-1
	//	If the string is empty, it's a relative #
	if (str == p)			zone = relativeZone;
	else if (bUnknownVID)	zone = strtoul(p + 1, NULL, 16);
	else					zone = GetStringFastHash(p, str - p);

	++str;	//	Skip the :
	
	if (isalpha(*str))
	{
		p = str;
		
		while (isalnum(*str)
			|| (*str == ':' && isalpha(str[1])))	++str;
		
		if (*str != ':' || !isdigit(str[1]))
		{
			zone = 0;
			return;	//	No ':#'?  Not a VirtualID	
		}

		ns = GetStringFastHash/*16*/(p, str - p);
		
		ZoneData *z = zone_table.Find(zone);
		if (!z)	z = VIDSystem::GetZoneFromAlias(zone);
		if (z)	z->AddNamespace(ns, Lexi::String(p, str-p));	//	Safe to use before zones are loaded!
		
		++str;	//	Skip the :
	}
	else if (*str == '#')
	{
		++str;
		
		p = str;
		
		while (isdigit(*str))	++str;
		
		if (*str != ':' || !isdigit(str[1]))
		{
			zone = 0;
			return;	//	No ':#'?  Not a VirtualID	
		}
		
		ns = strtoul(p, NULL, 16);
		
		++str;	//	Skip the :
	}
	
	number = strtoul(str, /*const_cast<char **>(&str)*/ NULL, 0);
};


void VirtualID::Parse(VNum vnum)
{
	Clear();	//	Initialize to invalid
	
	//	If a negative number, immediately exit
	if (vnum < 0)	return;

	VNumLegacy::Zone *zn = VNumLegacy::FindZone(vnum);	//	Determine old zone #
	
	zone		= zn->name;
	number		= vnum - zn->base;	// Offset
}


Lexi::String VirtualID::Print(Hash relativeZone) const
{
	BUFFER(buf, MAX_STRING_LENGTH);
	
	if (!IsValid())	return "<NONE>";
	
	const char *zoneName = "";
	const char *nsName = NULL;
 	
	if (zone != relativeZone || ns)
	{
	 	ZoneData *z = zone_table.Find(zone);
		if (!z)	z = VIDSystem::GetZoneFromAlias(zone);
		if (zone != relativeZone)	zoneName = z ? z->GetTag() : NULL;
		nsName = ns && z ? z->GetNamespace(ns) : NULL;
	}
	
	char *p = buf;
	if (zoneName)	p += sprintf(p, "%s:", zoneName);
	else			p += sprintf(p, "#%X:", (unsigned int)zone);
	
	if (ns)
	{
		if (nsName)	p += sprintf(p, "%s:", nsName);
		else		p += sprintf(p, "#%hX:", (unsigned int)ns);
	}
	
	sprintf(p, "%u", number);
	
	return buf;
}


bool IsVirtualID(const char *str, bool bNoVnums)
{
	skip_spaces(str);

	if (!bNoVnums && is_number(str))	//	Classic VNum; is_number will return false on a #:
	{
		//	Get classic VNum
		return atoi(str) >= 0;
	}
	
	const char *p = str;
	
	if (*str == '#')		++str;
	while (isalnum(*str))	++str;
	if (*str++ != ':')		return false;
	
	if (isalpha(*str))
	{
		while (isalnum(*str)
			|| (*str == ':' && isalpha(str[1])))	++str;
		if (*str++ != ':')	return false;
	}
	else if (*str == '#')
	{
		++str;
		while (isxdigit(*str))	++str;
		if (*str++ != ':')	return false;
	}
	
	return isdigit(*str);
}


namespace VIDSystem
{
	typedef std::map<Hash, ZoneData *>	AliasMap;
	
	static AliasMap &GetAliases()
	{
		static AliasMap sAliases;
		return sAliases;
	}
}


void VIDSystem::AddZoneAlias(ZoneData * zone, Hash alias)
{
	AliasMap &aliases = GetAliases();
	
	aliases[alias] = zone;
}


ZoneData *VIDSystem::GetZoneFromAlias(Hash alias)
{
	AliasMap &aliases = GetAliases();
	
	std::map<Hash, ZoneData *>::iterator i = aliases.find(alias);
	
	if (i != aliases.end())	return i->second;
	
	return 0;
}


#if 0
void VIDSystem::RegisterZoneName(const char *name)
{
	Hash hash = GetStringFastHash(name);
	
	registeredZoneNames[hash] = name;
}


bool VIDSystem::IsZoneNameRegistered(const char *name)
{
	return registeredZoneNames.find(GetStringFastHash(name)) != registeredZoneNames.end();
}


bool VIDSystem::IsZoneNameRegistered(Hash hash)
{
	return registeredZoneNames.find(hash) != registeredZoneNames.end();
}



const char *VIDSystem::GetZoneName(Hash hash)
{
	ZoneNameMap::iterator iter = registeredZoneNames.find(hash);
	if (iter == registeredZoneNames.end())	return NULL;
	
	return iter->second.c_str();
}
#endif


namespace VNumLegacy
{
	Zone			registeredZones[327];
}


void VNumLegacy::RegisterZone(ZoneData *zone, unsigned int znum, VNum top)
{
	Zone	zoneEntry;
	
	zoneEntry.name = zone->GetHash();
	zoneEntry.base = znum * 100;

	for (unsigned int i = znum, t = top / 100; i <= t; ++i)
	{
		registeredZones[i] = zoneEntry;
	}
}


VNumLegacy::Zone * VNumLegacy::FindZone(VNum vnum)
{
	if (vnum < 0)	return NULL;
	
	return &registeredZones[vnum / 100];
}


VNum VNumLegacy::GetVirtualNumber(VirtualID vid)
{
	if (!vid.IsValid() || vid.number > 32767)	return -1;
	
	ZoneData *	z = zone_table.Find(vid.zone);
	if (!z || z->GetZoneNumber() < 0)	return -1;
	
	unsigned int vidNum = vid.number + (z->GetZoneNumber() * 100);
	
	if (vidNum > z->top)				return -1;
	
	return vidNum;
}


unsigned int VNumLegacy::MaxTopForZone(int zoneNumber)
{
	if (zoneNumber < 0 || zoneNumber >= 327)	return 0;
	
	unsigned int cap = (zoneNumber * 100) + 99;
	for (Hash legacyZone = registeredZones[zoneNumber].name;
		   registeredZones[zoneNumber].name == legacyZone
		|| registeredZones[zoneNumber].name == 0;
		++zoneNumber)
	{
		cap = (zoneNumber * 100) + 99;
	}
	
	return cap;
}
