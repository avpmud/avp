/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 2005             [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Original LexiMUD Code                                                 [*]
[*] Based on CircleMUD, Copyright 1993-94, and DikuMUD, Copyright 1990-91 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : zones.h                                                        [*]
[*] Usage: Zone class and support                                         [*]
\***************************************************************************/


#ifndef __ZONES_H__
#define __ZONES_H__


//	Comment to revert to old system
//#define NEW_ZRESET_TYPES

#include <vector>

#include "lexistring.h"
#include "lexilist.h"
#include "bitflags.h"
#include "virtualid.h"
#include "characters.h"
#include "objects.h"
#include "scripts.h"
#include "rooms.h"

class CharData;
class WeatherSystem;
namespace Lexi {
	class Parser;
	class OutputParserFile;
}
namespace Weather
{
	class Climate;
}


enum ZoneFlag
{
	ZONE_NS_SUBZONES,
	NUM_ZONE_FLAGS
};


/* structure for the reset commands */
struct ResetCommand
{
	ResetCommand() : command(0), if_flag(0), repeat(0), arg1_hack(0), arg2_hack(0), arg3_hack(0), arg4_hack(0), arg5_hack(0), hive(0), line(0) {}
	
	enum DoorState
	{
		DOOR_OPEN,
		DOOR_CLOSED,
		DOOR_LOCKED,
		DOOR_IMPASSABLE,
		DOOR_BLOCKED,
		DOOR_ENABLED,
		DOOR_DISABLED,
		DOOR_BREACHED,
		DOOR_JAMMED,
		DOOR_REPAIRED,
		DOOR_HIDDEN,
		NUM_DOOR_STATES
	};
	
	char			command;	/* current command							*/

	bool			if_flag;	/* if TRUE: exe only if preceding exe'd		*/
	int				repeat;		/* Number of times to repeat this command	*/

	union
	{
		void *			arg1_hack;
		CharacterPrototype *mob;
		ObjectPrototype *	obj;
		ScriptPrototype *	script;
	};
	
	union
	{
		void *			arg2_hack;
		RoomData *		room;
		ObjectPrototype *	container;
	};
	
	union
	{
		int				arg3_hack;
		int				maxInRoom;
		int				equipLocation;
		Direction		direction;
		int				triggerType;		
	};
	
	union
	{
		int				arg4_hack;
		int				maxInZone;
		DoorState		doorState;
	};
	union
	{
		int				arg5_hack;
		int				maxInGame;		
	};

	int				hive;
	unsigned int	line;		/* line number this command appears on		*/

	Lexi::String	command_string;
	
	/* 
	 *  Commands:              *
	 *  'M': Read a mobile     *
	 *  'O': Read an object    *
	 *  'G': Give obj to mob   *
	 *  'P': Put obj in obj    *
	 *  'G': Obj to char       *
	 *  'E': Obj to char equip *
	 *  'D': Set state of door *
	 */
};

#if defined( NEW_ZRESET_TYPES )
typedef std::vector<IReset *>		ResetCommandList;
#else
typedef std::vector<ResetCommand>	ResetCommandList;
#endif


/* zone definition structure. for the 'zone-table'   */
class ZoneData
{
public:
	typedef Lexi::BitFlags<NUM_ZONE_FLAGS, ZoneFlag> Flags;

						ZoneData(const char *tag);

	const char *		GetTag() const { return m_Tag.c_str(); }
	Hash				GetHash() const { return m_TagHash; }
	int					GetZoneNumber() const { return m_ZoneNumber; }
	
//	const char *		GetFilename() const { return m_Filename.c_str(); }
	const char *		GetName() const { return m_Name.c_str(); }
	const char *		GetComment() const { return m_Comment.c_str(); }
	
	void				SetTag(const char *tag);
	void				AddAlias(const char *tag);
	void				AddNamespace(Hash/*16*/ hash, const Lexi::String &name);
	const char *		GetNamespace(Hash/*16*/ hash) const;
	
	bool				IsOwner(IDNum id) const { return m_Owner == id; }
	bool				IsOwner(CharData *ch) const;
	
	IDNum				GetOwner() const { return m_Owner; }
	void				SetOwner(IDNum owner) { m_Owner = owner; }
	
	struct Builder
	{
		IDNum id;
		Lexi::List<Hash> ns;
	};
	
	typedef Lexi::List<Builder>	BuilderList;
	const BuilderList &	GetBuilders() const { return m_Builders; }
	
	bool				IsBuilder(IDNum id, Hash ns) const;
	bool				IsBuilder(CharData *ch, Hash ns) const;
	
	void				AddBuilder(IDNum id, Hash ns);
	void				RemoveBuilder(IDNum id, Hash ns);
	
	bool				IsLight() const;

//	bool				IsOwnWeatherSystem() const { return m_WeatherSystem; }
	WeatherSystem *		GetWeather() const
	{
		return m_WeatherSystem ? m_WeatherSystem : (m_WeatherParentZone ? m_WeatherParentZone->m_WeatherSystem : NULL);
	}
	void				SetWeather(Weather::Climate *climate = NULL);
	void				SetWeather(ZoneData *zone = NULL);
	ZoneData *			GetWeatherParent() const { return m_WeatherParentZone; }
	
	const Flags &		GetFlags() const { return m_Flags; }
	Flags &				GetFlags() { return m_Flags; }
	
	bool				ArePlayersPresent() { return m_NumPlayersPresent > 0; }

//	static void			Parse(Lexi::Parser &parser, unsigned int nr);
	void				Write(Lexi::OutputParserFile &file);
	static void			Parse(Lexi::Parser &parser, const char *tag);
	static void			ParseResets(Lexi::Parser &parser, const char *tag);
	
	typedef std::pair<ResetCommandList::iterator, ResetCommandList::iterator>	ResetCommandListRange;
	ResetCommandListRange	GetResetsForRoom(RoomData *room);
	
	
private:
	int					m_ZoneNumber;		//	virtual number of this zone
	Lexi::String		m_Tag;
	Hash				m_TagHash;

	Lexi::StringList	m_Aliases;
	typedef std::map<Hash/*16*/, Lexi::String>	NamespaceMap;
	NamespaceMap		m_Namespaces;

	//	Weather system
	WeatherSystem *		m_WeatherSystem;//	A weather system for the zone
	ZoneData *			m_WeatherParentZone;
		
	//	OLC
	IDNum				m_Owner;
	BuilderList			m_Builders;		//	zone builders
	
	Flags				m_Flags;

public:	
	//	Visual Data
	Lexi::String		m_Name;			//	name of this zone
	Lexi::String		m_Comment;
	
	//	
	int					lifespan;		//	how long between resets (minutes)
	unsigned int		top;			//	upper limit for rooms in this zone
	
	/*
	 *  Reset mode:									*
	 *  0: Don't reset, and don't update age.		*
	 *  1: Reset if no PC's are located in zone.	*
	 *  2: Just reset.								*
	 */
	enum ResetMode
	{
		ResetMode_Never,
		ResetMode_Empty,
		ResetMode_Always,
		NUM_RESETMODES
	}					m_ResetMode;	//	conditions for reset (see below)
	ResetCommandList	cmd;			//	command table for reset

	//	Run-time Data
	int					age;			//	current age of this zone (minutes)
	Lexi::List<CharData *>m_Characters;
	int					m_NumPlayersPresent;	//	Number of players present
	
	
private:
						ZoneData();
	ZoneData &			operator=(const ZoneData &zone);
};



class ZoneMap
{
public:
	typedef ZoneData	data_type;
	typedef data_type *	value_type;
	typedef value_type *pointer;
	typedef value_type &reference;
	typedef pointer		iterator;
	typedef const pointer const_iterator;
	
	ZoneMap() : m_Allocated(0), m_Size(0), m_Zones(NULL) {}
	~ZoneMap() { delete [] m_Zones; }
	
	size_t				size() { return m_Size; }
	bool				empty() { return m_Size == 0; }
	
	void				insert(value_type i);
	value_type			Find(Hash zoneTag);
	value_type			Find(const char *zoneName) { return Find(GetStringFastHash(zoneName)); }
	value_type			Get(int i)		{ return m_Zones[i]; }
	
	iterator			begin()			{ return m_Zones; }
	iterator			end()			{ return m_Zones + m_Size; }
	const_iterator		begin() const	{ return m_Zones; }
	const_iterator		end() const 	{ return m_Zones + m_Size; }

	value_type			front()			{ return m_Zones[0]; }
	value_type			back()			{ return m_Zones[m_Size - 1]; }
	
	void				GetSortedByName(std::list<ZoneData *> l);
	
	static bool			SortFunc(value_type lhs, value_type rhs);
	static bool			SortByNameFunc(value_type lhs, value_type rhs);
	
	void				Sort();
	
private:
	
	unsigned int		m_Allocated;
	unsigned int		m_Size;
	value_type *		m_Zones;
	
	static const int	msGrowSize = 128;
		
	ZoneMap(const ZoneMap &);
	ZoneMap &operator=(const ZoneMap &);
};


extern ZoneMap				zone_table;


bool IsSameEffectiveZone(RoomData *r1, RoomData *r2);
bool IsSameEffectiveZone(RoomData *r1, ZoneData *z2, Hash ns2);


#endif
