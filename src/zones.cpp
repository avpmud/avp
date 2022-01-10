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
[*] File : zones.cp                                                       [*]
[*] Usage: Zone class and support                                         [*]
\***************************************************************************/

#include "types.h"
#include "buffer.h"
#include "utils.h"
#include "zones.h"
#include "weather.h"
#include "characters.h"
#include "rooms.h"
#include "objects.h"
#include "scripts.h"
#include "lexifile.h"
#include "lexiparser.h"
#include "utils.h"
#include "files.h"
#include "constants.h"
#include "find.h"
#include "interpreter.h"
#include "db.h"
#include "virtualid.h"

class IReset
{
public:
	IReset();
	virtual ~IReset();

	enum Type
	{
		Type_Undefined = -1,
		Type_Mobile,
		Type_Object,
		Type_Equip,
		Type_Give,
		Type_Put,
		Type_Door,
		Type_Remove,
		Type_Command,
		Type_Trigger
	};
	
	enum HiveFlag
	{
		IgnoreHived,
		HivedOnly,
		NotHivedOnly
	};

	//	Type
	virtual Type	GetType() = 0;

	//	Load/Save and Creation
	static IReset *	Create(Type type);
	static IReset *	Create(Lexi::Parser &parser);
	
	virtual IReset *Clone() = 0;

	void			Load(Lexi::Parser &parser);
	void			Save(Lexi::OutputParserFile &file);
	
	//	Validation
	virtual bool	IsValid() = 0;
	
	//	Execution
	bool			Execute(bool bLastCommandSucceeded); // execute the reset?

	//	Editor and Display
	virtual Lexi::String	Print() /* = 0*/;

	virtual RoomData *	GetDestination() { return NULL; }
	virtual void	SetDestination(RoomData *) {}

	void			SetRepeat(int num)		{ m_Repeat = num; }
	int				GetRepeat()				{ return m_Repeat; }

	void			SetHiveFlag(HiveFlag num)	{ m_HiveFlag = num; }
	HiveFlag		GetHiveFlag()			{ return m_HiveFlag; }

	void			SetDependent(bool num)	{ m_bDependent = num; }
	bool			IsDependent()			{ return m_bDependent; }

	int				m_LineNum;		//	temp for debugging my sloppy code

	static void		ClearVariables(void);
	
protected:
	//	Internal implementations
	virtual void	DoLoad(Lexi::Parser &parser) = 0;
	virtual void	DoSave(Lexi::OutputParserFile &file) = 0;
	virtual bool	DoExecute(bool bLastCommandSucceeded) = 0; // execute the reset?

private:
	//	All resets need these
 	HiveFlag		m_HiveFlag;
	int				m_Repeat;
	bool			m_bDependent;

protected:
	//	Shared data for resets
	static ObjData *	ms_LastObj;
	static CharData *	ms_LastMob;
};


/*
Mob:
	Room	- target
	Mob		- thing
	MaxRoom, MaxZone, MaxGame

Obj:
	Room	- target
	Obj		- thing
	MaxRoom, MaxGame

Give:
	Object	- thing
	MaxGame

Equip:
	Object	- thing
	Location	- param1
	MaxGame

Put:
	Object	- thing
	Container	- target
	MaxGame

Door:
	Room	- target
	Direction	- param1
	State	- param2
	
Remove:
	Object	- thing
	Room	- target

Trigger:
	Trigger	- thing
	TriggerType	- param1
	Room	- target
	MaxGame

Command:
	String
*/


ZoneMap		zone_table;


ZoneData::ZoneData(const char *tag)
:	m_ZoneNumber(is_number(tag) ? atoi(tag) : -1)	//	NOTE: Check the m_Tag stuff below and in new zone creation when moving to VID
,	m_Tag(tag)
,	m_TagHash(GetStringFastHash(tag))
,	m_WeatherSystem(NULL)
,	m_WeatherParentZone(NULL)
,	m_Name("New Zone")
,	lifespan(0)
,	top(0)
,	m_ResetMode(ZoneData::ResetMode_Always)
,	age(0)
,	m_NumPlayersPresent(0)
{
}


bool ZoneData::IsLight() const
{
	WeatherSystem *weather = GetWeather();
	return !weather || weather->m_Light >= 20;
}


bool ZoneData::IsOwner(CharData *ch) const
{
	return IsOwner(ch->GetID());
}


bool ZoneData::IsBuilder(IDNum id, Hash ns) const
{
	if (IsOwner(id))	return true;
	
	FOREACH_CONST(BuilderList, m_Builders, builder)
	{
		if (builder->id == id)
			return builder->ns.empty() || Lexi::IsInContainer(builder->ns, ns);
	}
	
	return false;
}


bool ZoneData::IsBuilder(CharData *ch, Hash ns) const
{
	return IsBuilder(ch->GetID(), ns);
}


void ZoneData::AddBuilder(IDNum id, Hash ns)
{
	if (IsOwner(id))	return;
	
	FOREACH(BuilderList, m_Builders, builder)
	{
		if (builder->id == id)
		{
			if (builder->ns.empty())
				return;	//	Global edit privs, can't change this!
			
			if (ns)	
			{
				builder->ns.remove(ns);	//	Remove in case it is already present
				builder->ns.push_back(ns);	
			}
			else
				builder->ns.clear();	//	Grant global edit privs
			return;
		}
	}
	
	Builder newBuilder;
	newBuilder.id = id;
	if (ns)	newBuilder.ns.push_back(ns);
	
	m_Builders.push_back(newBuilder);
}


void ZoneData::RemoveBuilder(IDNum id, Hash ns)
{
	if (IsOwner(id))	return;
	
	FOREACH(BuilderList, m_Builders, builder)
	{
		if (builder->id == id)
		{
			if (ns)							//	Removing specific privs
			{
				if (builder->ns.empty())	//	If they have global privs, then we dont remove specific ones
					return;
				
				builder->ns.remove(ns);
			}
			
			if (builder->ns.empty())
			{
				m_Builders.erase(builder);
			}
			
			return;
		}
	}
}


void ZoneData::SetTag(const char *tag)
{
	m_Tag = tag;
	m_TagHash = GetStringFastHash(tag);
}


void ZoneData::AddAlias(const char *tag)
{
	m_Aliases.remove(tag);
	m_Aliases.push_back(tag);
	VIDSystem::AddZoneAlias(this, GetStringFastHash(tag));
}


void ZoneData::AddNamespace(Hash/*16*/ hash, const Lexi::String &str)
{
	NamespaceMap::iterator i = m_Namespaces.find(hash);
	if (i == m_Namespaces.end())	m_Namespaces[hash] = str;
}


const char *ZoneData::GetNamespace(Hash/*16*/ hash) const
{
	NamespaceMap::const_iterator i = m_Namespaces.find(hash);
	return (i != m_Namespaces.end()) ? i->second.c_str() : NULL;
}


void ZoneData::SetWeather(Weather::Climate *climate)
{
	if (!m_WeatherSystem)
	{
		if (GetWeather())	GetWeather()->Unregister(this);
		m_WeatherParentZone = NULL;
		
		m_WeatherSystem = new WeatherSystem;
		m_WeatherSystem->Register(this);
		m_WeatherSystem->Start();
	
		/* Link zones to it! */
		FOREACH(ZoneMap, zone_table, z)
		{
			if ((*z)->m_WeatherParentZone == this)
				m_WeatherSystem->Register(*z);
		}
	}
	
	m_WeatherSystem->m_Climate = *climate;
}


void ZoneData::SetWeather(ZoneData *zone)
{
	if (m_WeatherSystem)	delete m_WeatherSystem;
	else if (GetWeather())	GetWeather()->Unregister(this);
	
	m_WeatherSystem = NULL;
	m_WeatherParentZone = zone;
	
	if (GetWeather())	GetWeather()->Register(this);
}


//	Global Variables
ObjData *IReset::ms_LastObj = NULL;
CharData *IReset::ms_LastMob = NULL;

// External functions
extern void load_otrigger(ObjData *obj);


void ZoneData::Write(Lexi::OutputParserFile &file)
{

	file.WriteString("Name", m_Name, "Undefined");
	file.WriteString("Comment", m_Comment);
	
	file.BeginSection("Reset");
	{
		file.WriteEnum("Mode", m_ResetMode);
		file.WriteInteger("Time", this->lifespan);
	}
	file.EndSection();
	
	if (m_Owner)
	{
		const char *	ownerName = get_name_by_id(m_Owner);
		if (ownerName)	file.WriteString("Owner", Lexi::Format("%u %s", m_Owner, ownerName));
	}
	
	file.BeginSection("Builders");
	int count = 0;
	FOREACH(BuilderList, m_Builders, builder)
	{
		const char *playerName = get_name_by_id(builder->id);
		
		if (!playerName)
			continue;
		
		if (builder->ns.empty())
		{
			file.WriteString(Lexi::Format("Builder %d", ++count).c_str(), playerName);
		}
		else
		{
			FOREACH(Lexi::List<Hash>, builder->ns, ns)
			{
				const char *nsName = GetNamespace(*ns);
				
				if (!nsName)
					continue;
				
				file.WriteString(Lexi::Format("Builder %d", ++count).c_str(), Lexi::Format("%s %s", playerName, nsName).c_str());
			}
		}
	}
	file.EndSection();
	
	file.BeginSection("Legacy");
	if (m_ZoneNumber >= 0)
	{
		file.WriteInteger("Number", m_ZoneNumber);
		file.WriteInteger("Top", this->top);
	}
	file.EndSection();
	
	file.BeginSection("Weather");
	if (m_WeatherSystem)
	{
		BUFFER(buf, MAX_STRING_LENGTH);
		
		Weather::Climate & climate = m_WeatherSystem->m_Climate;
		
		file.WriteEnum("SeasonalPattern", climate.m_SeasonalPattern);
		file.WriteFlags("Flags", climate.m_Flags);
		file.WriteInteger("Energy", climate.m_Energy, 0);
		
		int	numSeasons = Weather::num_seasons[climate.m_SeasonalPattern];
		for (int i = 0; i < numSeasons; ++i)
		{
			file.BeginSection(Lexi::Format("Seasons/%d", i + 1));
			{
				Weather::Climate::Season &season = climate.m_Seasons[i];
				
				file.WriteEnum("Wind", season.m_Wind);
				file.WriteEnum("WindDirection", season.m_WindDirection, dirs);
				file.WriteString("WindVaries", YESNO(season.m_WindVaries), YESNO(0));
				file.WriteEnum("Precipitation", season.m_Precipitation);
				file.WriteEnum("Temperature", season.m_Temperature);
			}
			file.EndSection();
		}
	}
	else if (m_WeatherParentZone)
	{
		file.WriteString("ParentZone", m_WeatherParentZone->GetTag());
	}
	file.EndSection();

	file.BeginSection("Aliases");
	count = 0;
	FOREACH(Lexi::StringList, m_Aliases, iter)
		file.WriteString(Lexi::Format("Alias %d", ++count).c_str(), iter->c_str());
	file.EndSection();
	
	file.WriteFlags("Flags", m_Flags);
	
	int resetNum = 0;
	FOREACH(ResetCommandList, this->cmd, iter)
	{
		file.BeginSection(Lexi::Format("Resets/%d", ++resetNum));
				
#if defined( NEW_ZRESET_TYPES )
		(*iter)->Save(file);
#else
		ResetCommand &ZCMD = *iter;

		ZCMD.line = resetNum;
				
		switch (ZCMD.command)
		{
			case 'M':
				if (!ZCMD.mob || !ZCMD.room)
					break;
				
				file.WriteString("Type", "Mobile");
				file.WriteVirtualID("Mobile", ZCMD.mob->GetVirtualID(), GetHash());
				file.WriteVirtualID("Room", ZCMD.room->GetVirtualID(), GetHash());
				
				if (ZCMD.maxInRoom > 0)	file.WriteInteger("MaxInRoom", ZCMD.maxInRoom);
				if (ZCMD.maxInZone > 0)	file.WriteInteger("MaxInZone", ZCMD.maxInZone);
				if (ZCMD.maxInGame > 0)	file.WriteInteger("MaxInGame", ZCMD.maxInGame);
				
				if (ZCMD.repeat > 1)	file.WriteInteger("Repeat", ZCMD.repeat);
				file.WriteEnum("Hive", ZCMD.hive, hive_types, 0);
				file.WriteString("Dependent", YESNO(ZCMD.if_flag), YESNO(0));
		
				break;
				
			case 'O':
				if (!ZCMD.obj || !ZCMD.room)
					break;
				
				file.WriteString("Type", "Object");
				file.WriteVirtualID("Object", ZCMD.obj->GetVirtualID(), GetHash());
				file.WriteVirtualID("Room", ZCMD.room->GetVirtualID(), GetHash());
				
				if (ZCMD.maxInRoom > 0)	file.WriteInteger("MaxInRoom", ZCMD.maxInRoom);
				if (ZCMD.maxInGame > 0)	file.WriteInteger("MaxInGame", ZCMD.maxInGame);
				
				if (ZCMD.repeat > 1)	file.WriteInteger("Repeat", ZCMD.repeat);
				file.WriteEnum("Hive", ZCMD.hive, hive_types, 0);
				file.WriteString("Dependent", YESNO(ZCMD.if_flag), YESNO(0));
				break;
				
			case 'G':
				if (!ZCMD.obj)
					break;
				
				file.WriteString("Type", "Give");
				file.WriteVirtualID("Object", ZCMD.obj->GetVirtualID(), GetHash());
					
				if (ZCMD.maxInGame > 0)	file.WriteInteger("MaxInGame", ZCMD.maxInGame);
				
				if (ZCMD.repeat > 1)	file.WriteInteger("Repeat", ZCMD.repeat);
				file.WriteEnum("Hive", ZCMD.hive, hive_types, 0);
				file.WriteString("Dependent", YESNO(ZCMD.if_flag), YESNO(0));
				break;

			case 'E':
				if (!ZCMD.obj)
					break;
				
				file.WriteString("Type", "Equip");
				file.WriteVirtualID("Object", ZCMD.obj->GetVirtualID(), GetHash());
				file.WriteEnum("Location", ZCMD.equipLocation, equipment_types);
				
				if (ZCMD.maxInGame > 0)	file.WriteInteger("MaxInGame", ZCMD.maxInGame);
				
				if (ZCMD.repeat > 1)	file.WriteInteger("Repeat", ZCMD.repeat);
				file.WriteEnum("Hive", ZCMD.hive, hive_types, 0);
				file.WriteString("Dependent", YESNO(ZCMD.if_flag), YESNO(0));
				break;

			case 'P':
				if (!ZCMD.obj || !ZCMD.container)
					break;
				
				file.WriteString("Type", "Put");
				file.WriteVirtualID("Object", ZCMD.obj->GetVirtualID(), GetHash());
				file.WriteVirtualID("Container", ZCMD.container->GetVirtualID(), GetHash());
					
				if (ZCMD.maxInGame > 0)	file.WriteInteger("MaxInGame", ZCMD.maxInGame);
				
				if (ZCMD.repeat > 1)	file.WriteInteger("Repeat", ZCMD.repeat);
				file.WriteEnum("Hive", ZCMD.hive, hive_types, 0);
				file.WriteString("Dependent", YESNO(ZCMD.if_flag), YESNO(0));
				break;
				
			case 'D':
				if (!ZCMD.room || (unsigned)ZCMD.direction >= NUM_OF_DIRS)
					break;
				
				file.WriteString("Type", "Door");
				file.WriteVirtualID("Room", ZCMD.room->GetVirtualID(), GetHash());
				file.WriteEnum("Direction", ZCMD.direction, dirs);
				file.WriteEnum("State", ZCMD.doorState);
				
				if (ZCMD.repeat > 1)	file.WriteInteger("Repeat", ZCMD.repeat);
				file.WriteEnum("Hive", ZCMD.hive, hive_types, 0);
				file.WriteString("Dependent", YESNO(ZCMD.if_flag), YESNO(0));
				break;
				
			case 'R':
				if (!ZCMD.room || !ZCMD.obj)
					break;
				
				file.WriteString("Type", "Remove");
				file.WriteVirtualID("Object", ZCMD.obj->GetVirtualID(), GetHash());
				file.WriteVirtualID("Room", ZCMD.room->GetVirtualID(), GetHash());
				
				if (ZCMD.repeat > 1)	file.WriteInteger("Repeat", ZCMD.repeat);
				file.WriteEnum("Hive", ZCMD.hive, hive_types, 0);
				file.WriteString("Dependent", YESNO(ZCMD.if_flag), YESNO(0));
				break;

			case 'T':
				if (!ZCMD.script || (ZCMD.triggerType == WLD_TRIGGER && !ZCMD.room))
					break;
				
				file.WriteString("Type", "Trigger");
				file.WriteVirtualID("Trigger", ZCMD.script->GetVirtualID(), GetHash());
				file.WriteEnum("TriggerType", ZCMD.triggerType, trig_data_types);
				
				if (ZCMD.triggerType == WLD_TRIGGER)	file.WriteVirtualID("Room", ZCMD.room->GetVirtualID(), GetHash());
				
				if (ZCMD.maxInGame > 0)	file.WriteInteger("MaxInGame", ZCMD.maxInGame);
				
				if (ZCMD.repeat > 1)	file.WriteInteger("Repeat", ZCMD.repeat);
				file.WriteEnum("Hive", ZCMD.hive, hive_types, 0);
				file.WriteString("Dependent", YESNO(ZCMD.if_flag), YESNO(0));
				break;

			case 'C':
				if (ZCMD.command_string.empty())
					break;
				
				file.WriteString("Type", "Command");
				file.WriteString("Command", ZCMD.command_string.c_str());
				
				if (ZCMD.repeat > 1)	file.WriteInteger("Repeat", ZCMD.repeat);
				file.WriteEnum("Hive", ZCMD.hive, hive_types, 0);
				file.WriteString("Dependent", YESNO(ZCMD.if_flag), YESNO(0));
				break;
				
			case '*':
				//	Invalid commands are replaced with '*' - Ignore them
				break;
				
			default:
				mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: OLC: z_save_to_disk(): Unknown cmd '%c' - NOT saving", ZCMD.command);
				break;
		}
#endif
		
		file.EndSection();
	}
}

// Brainstorming the loader...
void ZoneData::Parse(Lexi::Parser &parser, const char *tag)
{
	if (zone_table.Find(tag))
	{
		log("Duplicate hash zone found in zone table: adding '%s' overlaps existing '%s'", tag, zone_table.Find(tag)->GetTag());
		exit(0);	
	}
	
	ZoneData *zone = new ZoneData(tag);
	zone_table.insert(zone);

	zone->m_Name				= parser.GetString("Name", "New Zone");
	zone->m_Comment				= parser.GetString("Comment");
	
	zone->m_ResetMode			= parser.GetEnum("Reset/Mode", ResetMode_Never);
	zone->lifespan				= parser.GetInteger("Reset/Time", 30);
	
	zone->m_Owner				= parser.GetInteger("Owner");
	if (zone->m_Owner && !get_name_by_id(zone->m_Owner))	zone->m_Owner = 0;

	BUFFER(buf, MAX_INPUT_LENGTH);
	int numBuilders = parser.NumFields("Builders");
	for (int i = 0; i < numBuilders; ++i)
	{
		const char *str = parser.GetIndexedString("Builders", i);
		
		str = any_one_arg(str, buf);
		
		IDNum id = get_id_by_name(buf);
		
		Hash hash = GetStringFastHash(str);
		if (hash)	zone->AddNamespace(hash, str);
		zone->AddBuilder(id, hash);
	}
	
	int numAliases = parser.NumFields("Aliases");
	for (int i = 0; i < numAliases; ++i)
	{
		const char *alias = parser.GetIndexedString("Aliases", i);
		zone->m_Aliases.push_back(alias);
		VIDSystem::AddZoneAlias(zone, GetStringFastHash(alias));
	}
	
	zone->m_Flags				= parser.GetBitFlags<ZoneFlag>("Flags");

	//	New Zone
	if (parser.DoesSectionExist("Legacy") || parser.DoesFieldExist("Number"))
	{
		//	This is a legacy zone...
		zone->m_ZoneNumber		= parser.GetInteger("Legacy/Number", parser.GetInteger("Number", zone->GetZoneNumber()));
		zone->top				= parser.GetInteger("Legacy/Top", parser.GetInteger("Top", (zone->GetZoneNumber() * 100) + 99)); // handle with care

		VNumLegacy::RegisterZone(zone, zone->GetZoneNumber(), zone->top);
	}
	
	
	if (parser.DoesSectionExist("Weather")
		&& (!parser.DoesFieldExist("Weather/ParentZone") ||
			zone->m_Tag == parser.GetString("Weather/ParentZone")))
	{
		zone->m_WeatherSystem = new WeatherSystem;
		
		Weather::Climate & climate = zone->m_WeatherSystem->m_Climate;
		
		climate.m_Flags				= parser.GetBitFlags<Weather::ClimateBits>("Weather/Flags");
		climate.m_Energy			= parser.GetInteger("Weather/Energy");
		
		climate.m_SeasonalPattern	= parser.GetEnum("Weather/SeasonalPattern", Weather::Seasons::One);
		
		int numClimates = parser.NumSections("Weather/Seasons");
		for (int i = 0; i < numClimates; ++i)
		{
			PARSER_SET_ROOT_INDEXED(parser, "Weather/Seasons", i);
			
			Weather::Climate::Season &season = climate.m_Seasons[i];
			
			season.m_Wind				= parser.GetEnum("Wind", Weather::Wind::Calm);
			season.m_WindDirection		= parser.GetEnum("WindDirection", dirs);
			season.m_WindVaries			= !str_cmp(parser.GetString("WindVaries", "NO"), "YES");
			season.m_Precipitation		= parser.GetEnum("Precipitation", Weather::Precipitation::None);
			season.m_Temperature		= parser.GetEnum("Temperature", Weather::Temperature::Arctic);
		}
		
		zone->m_WeatherSystem->Register(zone);
		zone->m_WeatherSystem->Start();
	}
}


void ZoneData::ParseResets(Lexi::Parser &parser, const char *tag)
{
	ZoneData *zone = zone_table.Find(tag);
	
	if (!zone)
	{
		log("Unable to load zone '%s' for resets", tag);
		exit(0);
	}
	
	if (parser.DoesFieldExist("Weather/ParentZone"))
	{
		zone->m_WeatherParentZone = zone_table.Find(parser.GetString("Weather/ParentZone"));
		if (zone->m_WeatherParentZone == zone)
			zone->m_WeatherParentZone = NULL;
		else if (zone->GetWeather())
			zone->GetWeather()->Register(zone);
	}
		
	int numResets = parser.NumSections("Resets");
	
	if (numResets > 0)
	{
		zone->cmd.reserve(numResets);
	}
	
	for (int i = 0; i < numResets; ++i)
	{
		PARSER_SET_ROOT_INDEXED(parser, "Resets", i);

		// CRITICAL First Data
#if defined( NEW_ZRESET_TYPES )
		IReset *zreset;
		int type = parser.GetEnum("Type", zonecmd_types);

		if (!(zreset = IReset::Create((IReset::Type)type)))
		{
			log("SYSERR: Invalid type #%d from reset #%d in zone %d.", type, i, nr);
			continue;
		}
		
		zreset->Load(parser);
		zreset->m_LineNum = i;			// might be useful for logs
		zone->cmd.push_back(zreset);	// descending order please
#else
		ResetCommand	rc;
		
		rc.command = *parser.GetString("Type");
		
		rc.repeat	= parser.GetInteger("Repeat", 1);
		rc.hive		= parser.GetEnum("Hive", hive_types);
		rc.if_flag	= !str_cmp(parser.GetString("Dependent", "NO"), "YES");
		
		if (rc.command == 'T' || rc.command == 'C')
		{
			rc.repeat = 0;	
		}
		
		bool bValid = true;
		switch (rc.command)
		{
			case 'M':
				//rc.thingVNum	= parser.GetInteger("Mobile", -1);
				//rc.targetVNum	= parser.GetInteger("Room", -1);
				rc.mob			= mob_index.Find(parser.GetVirtualID("Mobile", zone->GetHash()));
				rc.room			= world.Find(parser.GetVirtualID("Room", zone->GetHash()));
				rc.maxInRoom	= parser.GetInteger("MaxInRoom");
				rc.maxInZone	= parser.GetInteger("MaxInZone");
				rc.maxInGame	= parser.GetInteger("MaxInGame");
				rc.repeat = 0;
				bValid = rc.mob && rc.room;
				break;
				
			case 'O':
				//rc.thingVNum	= parser.GetInteger("Object", -1);
				//rc.targetVNum	= parser.GetInteger("Room", -1);
				rc.obj			= obj_index.Find(parser.GetVirtualID("Object", zone->GetHash()));
				rc.room			= world.Find(parser.GetVirtualID("Room", zone->GetHash()));
				rc.maxInRoom	= parser.GetInteger("MaxInRoom");
				rc.maxInGame	= parser.GetInteger("MaxInGame");
				bValid = rc.obj && rc.room;
				break;

			case 'G':
				//rc.thingVNum	= parser.GetInteger("Object", -1);
				rc.obj			= obj_index.Find(parser.GetVirtualID("Object", zone->GetHash()));
				rc.maxInGame	= parser.GetInteger("MaxInGame");
				bValid = rc.obj;
				break;
				
			case 'E':
				//rc.thingVNum	= parser.GetInteger("Object", -1);
				rc.obj			= obj_index.Find(parser.GetVirtualID("Object", zone->GetHash()));
				rc.equipLocation= parser.GetEnum("Location", equipment_types);
				rc.maxInGame	= parser.GetInteger("MaxInGame");
				rc.repeat = 0;
				bValid = rc.obj;
				break;

			case 'P':
				//rc.thingVNum	= parser.GetInteger("Object", -1);
				//rc.targetVNum	= parser.GetInteger("Container", -1);
				rc.obj			= obj_index.Find(parser.GetVirtualID("Object", zone->GetHash()));
				rc.container	= obj_index.Find(parser.GetVirtualID("Container", zone->GetHash()));
				rc.maxInGame	= parser.GetInteger("MaxInGame");
				bValid = rc.obj && rc.container;
				break;
				
			case 'D':
				//rc.targetVNum	= parser.GetInteger("Room", -1);
				rc.room			= world.Find(parser.GetVirtualID("Room", zone->GetHash()));
				rc.direction	= parser.GetEnum<Direction>("Direction", dirs);
				rc.doorState	= parser.GetEnum("State", ResetCommand::DOOR_OPEN);
				rc.repeat = 0;
				bValid = rc.room;
				break;
			
			case 'R':
				//rc.thingVNum	= parser.GetInteger("Object", -1);
				//rc.targetVNum	= parser.GetInteger("Room", -1);
				rc.obj			= obj_index.Find(parser.GetVirtualID("Object", zone->GetHash()));
				rc.room			= world.Find(parser.GetVirtualID("Room", zone->GetHash()));
				bValid = rc.obj && rc.room;
				break;
				
			case 'T':
				//rc.thingVNum	= parser.GetInteger("Trigger", -1);
				rc.script		= trig_index.Find(parser.GetVirtualID("Trigger", zone->GetHash()));
				rc.triggerType	= parser.GetEnum("TriggerType", trig_data_types);
				//rc.targetVNum	= parser.GetInteger("Room", -1);
				if (rc.triggerType == WLD_TRIGGER)
				{
					rc.room			= world.Find(parser.GetVirtualID("Room", zone->GetHash()));
					bValid = rc.room;
				}
				rc.maxInGame	= parser.GetInteger("MaxInGame");
				rc.repeat = 0;
				bValid = bValid && rc.script && (rc.triggerType != -1);
				break;
				
			case 'C':
				rc.command_string = parser.GetString("Command");
				rc.repeat = 0;
				break;

			default:
				log("SYSERR: Unknown reset type \"%s\".", parser.GetString("Type"));
		}
		
		rc.line = i + 1;
		
		if (!bValid)
		{
			mudlogf(NRM, LVL_STAFF, FALSE,	"ZONEERR: Invalid reset command #%d ('%c') in zone '%s'", rc.line, rc.command, tag);
			rc.command = '*';
		}
		else
			zone->cmd.push_back(rc);
#endif
	}
}



ZoneData::ResetCommandListRange ZoneData::GetResetsForRoom(RoomData *room)
{
	ResetCommandList::iterator	b, e;
	
	RoomData *resetRoom = NULL;
	for (b = cmd.begin(); b != cmd.end(); ++b)
	{
#if defined( NEW_ZRESET_TYPES )
		if ((*e)->GetDestination())	resetRoom = (e)->GetDestination();
#else
		ResetCommand &ZCMD = *b;
		switch(ZCMD.command)
    	{
			case 'T':
				if (ZCMD.triggerType != WLD_TRIGGER)
					break;
    		case 'M':
    		case 'O':
    		case 'D':
    		case 'R':
    			resetRoom = ZCMD.room;
				break;
	    }
#endif
	    if (resetRoom == room)
	    {
	    	break;
	    }
	}
	
	for (e = b; e != cmd.end(); ++e)
	{
#if defined( NEW_ZRESET_TYPES )
		if ((*e)->GetDestination())	resetRoom = (e)->GetDestination();
#else
		ResetCommand &ZCMD = *e;
		switch(ZCMD.command)
    	{
			case 'T':
				if (ZCMD.triggerType != WLD_TRIGGER)
					break;
    		case 'M':
    		case 'O':
    		case 'D':
    		case 'R':
    			resetRoom = ZCMD.room;
				break;
	    }
#endif
	    if (resetRoom != room)
	    {
	    	break;
	    }
	}
	
	return ResetCommandListRange(b, e);
}


//	Binary search
ZoneMap::value_type ZoneMap::Find(Hash zone)
{
	if (m_Size == 0)	return NULL;
	if (zone == 0)		return NULL;
	
	int bot = 0;
	int top = m_Size - 1;
	
	do
	{
		int mid = (bot + top) / 2;
		
		value_type i = m_Zones[mid];
		Hash vzone = i->GetHash();
		
		if (vzone == zone)	return i;
		if (bot >= top)		break;
		if (vzone > zone)	top = mid - 1;
		else				bot = mid + 1;
	} while (1);
	
	return NULL;
}


void ZoneMap::insert(value_type i)
{
	if (m_Size == m_Allocated)
	{
		//	Grow: increase alloc size, alloc new, copy old to new, delete old, reassign
		m_Allocated += msGrowSize;
		value_type *newIndex = new value_type[m_Allocated];
		std::copy(m_Zones, m_Zones + m_Size, newIndex);
		delete [] m_Zones;
		m_Zones = newIndex;
	}
	
	//	Mark as dirty if the item being added would fall before the end of the entries
	bool bShouldSort = !empty() && i->GetHash() < back()->GetHash();	

	//	Add new item to end
	m_Zones[m_Size] = i;
	++m_Size;
	
	if (bShouldSort)	Sort();
}


void ZoneMap::Sort()
{
	std::sort(m_Zones, m_Zones + m_Size, SortFunc);
}

bool ZoneMap::SortFunc(value_type lhs, value_type rhs)
{
	return lhs->GetHash() < rhs->GetHash();
}


bool ZoneMap::SortByNameFunc(value_type lhs, value_type rhs)
{
	return str_cmp_numeric(lhs->GetTag(), rhs->GetTag()) < 0;
}



bool IsSameEffectiveZone(RoomData *r1, RoomData *r2)
{
	ZoneData *z1 = r1->GetZone();
	ZoneData *z2 = r2->GetZone();
	
	if (z1 != z2)
		return false;
	
	if (z1->GetFlags().test(ZONE_NS_SUBZONES))
	{
		return r1->GetVirtualID().ns == r2->GetVirtualID().ns;
	}
	
	return true;
}


bool IsSameEffectiveZone(RoomData *r1, ZoneData *z2, Hash ns2)
{
	ZoneData *z1 = r1->GetZone();
	
	if (z1 != z2)
		return false;
	
	if (z1->GetFlags().test(ZONE_NS_SUBZONES))
	{
		return r1->GetVirtualID().ns == ns2;
	}
	
	return true;
}


class ResetLoadMobile : public IReset
{
public:
	//	Type
	virtual IReset::Type GetType() { return IReset::Type_Mobile; }

	//	Load/Save and Creation
	virtual IReset *Clone() { return new ResetLoadMobile(*this); };
	virtual void	DoLoad(Lexi::Parser &parser);
	virtual void	DoSave(Lexi::OutputParserFile &file);
	
	//	Validation
	virtual bool	IsValid();
	
	//	Execution
	virtual bool	DoExecute(bool bLastCommandSucceeded);
	virtual RoomData *	GetDestination() { return m_DestinationRoom; }

//private:
	//	We use a union, once fully loaded we 'parse' it
	//	If we dont mind extra overhead during a reset we could just use vnums and look it up every time
	
	CharacterPrototype *m_MobProto;
	RoomData *			m_DestinationRoom;
	
	int				m_MaxInGame;
	int				m_MaxInZone;
	int				m_MaxInRoom;
};

class ResetLoadObject : public IReset
{
public:
	//	Type
	virtual IReset::Type GetType() { return IReset::Type_Object; }
	
	//	Load/Save and Creation
	virtual IReset *Clone() { return new ResetLoadObject(*this); };
	virtual void	DoLoad(Lexi::Parser &parser);
	virtual void	DoSave(Lexi::OutputParserFile &file);
	
	//	Validation
	virtual bool	IsValid(void);
	
	//	Execution
	virtual bool	DoExecute(bool bLastCommandSucceeded);
	virtual RoomData *	GetDestination() { return m_DestinationRoom; }

//private:
	ObjectPrototype *	m_ObjProto;
	RoomData *			m_DestinationRoom;
	
	int				m_MaxInGame;
	int				m_MaxInZone;
	int				m_MaxInRoom;
};


class ResetEquipMobile : public IReset
{
public:
	//	Type
	virtual IReset::Type GetType() { return IReset::Type_Equip; }
	
	//	Load/Save and Creation
	virtual IReset *Clone() { return new ResetEquipMobile(*this); };
	virtual void	DoLoad(Lexi::Parser &parser);
	virtual void	DoSave(Lexi::OutputParserFile &file);
	
	//	Validation
	virtual bool	IsValid(void);
	
	//	Execution
	virtual bool	DoExecute(bool bLastCommandSucceeded);

//private:
	//	We use a union, once fully loaded we 'parse' it
	//	If we dont mind extra overhead during a reset we could just use vnums and look it up every time
	
	CharacterPrototype *m_MobProto;
	ObjectPrototype *	m_ObjProto;
	RoomData *			m_DestinationRoom;

	int				m_EqPosition;
	int				m_MaxInGame;
};


class ResetGiveObject : public IReset
{
public:
	//	Type
	virtual IReset::Type GetType() { return IReset::Type_Give; }

	//	Load/Save and Creation
	virtual IReset *Clone() { return new ResetGiveObject(*this); };
	virtual void	DoLoad(Lexi::Parser &parser);
	virtual void	DoSave(Lexi::OutputParserFile &file);
	
	//	Validation
	virtual bool	IsValid(void);
	
	//	Execution
	virtual bool	DoExecute(bool bLastCommandSucceeded);


//private:
	ObjectPrototype *m_ObjProto;
	CharacterPrototype *m_MobProto;
	RoomData *	m_DestinationRoom;

	int				m_MaxInGame;
};


class ResetPutObject : public IReset
{
public:
	//	Type
	virtual IReset::Type GetType() { return IReset::Type_Put; }
	
	//	Load/Save and Creation
	virtual IReset *Clone() { return new ResetPutObject(*this); };
	virtual void	DoLoad(Lexi::Parser &parser);
	virtual void	DoSave(Lexi::OutputParserFile &file);

	//	Validation
	virtual bool	IsValid(void);
	
	//	Execution
	virtual bool	DoExecute(bool bLastCommandSucceeded);

//private:
	ObjectPrototype *m_ObjProto;
	ObjectPrototype *m_ContainerProto;
	RoomData *	m_DestinationRoom;
	
	int				m_MaxInGame;
};

class ResetDoor : public IReset
{
public:
	//	Type
	virtual IReset::Type GetType() { return IReset::Type_Door; }
	
	//	Load/Save and Creation
	virtual IReset *Clone() { return new ResetDoor(*this); };
	virtual void	DoLoad(Lexi::Parser &parser);
	virtual void	DoSave(Lexi::OutputParserFile &file);

	//	Validation
	virtual bool	IsValid(void);
	
	//	Execution
	virtual bool	DoExecute(bool bLastCommandSucceeded);
	virtual RoomData *	GetDestination() { return m_DestinationRoom; }

//private:
	RoomData *	m_DestinationRoom;

	ResetCommand::DoorState		m_DoorState;
	int				m_Direction;
};

class ResetRemoveObject : public IReset
{
public:
	//	Type
	virtual IReset::Type GetType() { return IReset::Type_Remove; }
	
	//	Load/Save and Creation
	virtual IReset *Clone() { return new ResetRemoveObject(*this); };
	virtual void	DoLoad(Lexi::Parser &parser);
	virtual void	DoSave(Lexi::OutputParserFile &file);
	
	//	Validation
	virtual bool	IsValid(void);
	
	//	Execution
	virtual bool	DoExecute(bool bLastCommandSucceeded);
	virtual RoomData *	GetDestination() { return m_DestinationRoom; }

//private:
	ObjectPrototype	*m_ObjProto;
	RoomData *	m_DestinationRoom;
};

class ResetMobCommand : public IReset
{
public:
	//	Type
	virtual IReset::Type GetType() { return IReset::Type_Command; }
	
	//	Load/Save and Creation
	virtual IReset *Clone() { return new ResetMobCommand(*this); };
	virtual void	DoLoad(Lexi::Parser &parser);
	virtual void	DoSave(Lexi::OutputParserFile &file);
	
	//	Validation
	virtual bool	IsValid(void);
	
	//	Execution
	virtual bool	DoExecute(bool bLastCommandSucceeded);

//private:
	CharacterPrototype *m_MobProto;
	RoomData *	m_DestinationRoom;
	Lexi::String	m_Command;
};


class ResetLoadTrigger : public IReset
{
public:
	//	Type
	virtual IReset::Type GetType() { return IReset::Type_Trigger; }
	
	//	Load/Save and Creation
	virtual IReset *Clone() { return new ResetLoadTrigger(*this); };
	virtual void	DoLoad(Lexi::Parser &parser);
	virtual void	DoSave(Lexi::OutputParserFile &file);
	
	//	Validation
	virtual bool	IsValid(void);
	
	//	Execution
	virtual bool	DoExecute(bool bLastCommandSucceeded);
	virtual RoomData *	GetDestination() { return m_DestinationRoom; }

//private:
	ScriptPrototype *	m_ScriptProto;
	RoomData *			m_DestinationRoom;

	int				m_TriggerType;
	int				m_MaxInGame;
};



IReset::IReset() : m_LineNum(0), m_HiveFlag(IgnoreHived), m_Repeat(1), m_bDependent(false)
{
}

	
IReset::~IReset()
{
}


bool IReset::Execute(bool bLastCommandSucceeded)
{
	if (!IsValid())
		return false;
	else if (m_bDependent && !bLastCommandSucceeded)
		return false;
	else if ((GetHiveFlag() != IgnoreHived) && GetDestination())
	{
		RoomData *destination = GetDestination();
		
		if (destination)
		{
			if ((GetHiveFlag() == HivedOnly) && !ROOM_FLAGGED(destination, ROOM_HIVED))
				return false;	//	Can't do, not hived
			else if ((GetHiveFlag() == NotHivedOnly) && ROOM_FLAGGED(destination, ROOM_HIVED))
				return false;	//	Can't do, hived
		}
	}
	
	for (int i = m_Repeat; i > 0; --i)
	{
		if (!DoExecute(bLastCommandSucceeded))
			return false;
	}
	
	return true;
}


// [ Start IReset Stuff ]
void IReset::Save(Lexi::OutputParserFile &file)
{
	file.WriteEnum("Type", GetType(), zonecmd_types);
	if (m_Repeat > 1)	file.WriteInteger("Repeat", m_Repeat);
	file.WriteEnum("Hive", m_HiveFlag, hive_types, 0);
	file.WriteString("Dependent", YESNO(IsDependent()), YESNO(0));
	
	DoSave(file);
}

void IReset::Load(Lexi::Parser &parser)
{
	m_Repeat		= parser.GetInteger("Repeat", 1);
	m_HiveFlag		= parser.GetEnum("Hive", hive_types, IgnoreHived);
	m_bDependent	= !str_cmp(parser.GetString("Dependent", "NO"), "YES");
	
	DoLoad(parser);
}


IReset *IReset::Create(IReset::Type type)
{
	switch (type)
	{
		case IReset::Type_Mobile:	return new ResetLoadMobile();
		case IReset::Type_Object:	return new ResetLoadObject();
		case IReset::Type_Equip:	return new ResetEquipMobile();
		case IReset::Type_Give:		return new ResetGiveObject();
		case IReset::Type_Put:		return new ResetPutObject();
		case IReset::Type_Door:		return new ResetDoor();
		case IReset::Type_Remove:	return new ResetRemoveObject();
		case IReset::Type_Command:	return new ResetMobCommand();
		case IReset::Type_Trigger:	return new ResetLoadTrigger();
	}
	
	return NULL;
}

void IReset::ClearVariables(void)
{
	ms_LastObj	= NULL;
	ms_LastMob	= NULL;
}


Lexi::String IReset::Print()
{
	return "";
}
// [ End of IReset ]


// [ ResetLoadMobile Stuff ]
bool ResetLoadMobile::DoExecute(bool bLastCommandSucceeded)
{
	if (  m_DestinationRoom
	&& ( (m_MaxInGame <= 0) || (m_MobProto->m_Count < m_MaxInGame))
	&& ( (m_MaxInRoom <= 0) || (CountSpawnMobsInList(m_MobProto, m_DestinationRoom->people) < m_MaxInRoom))
	&& ( (m_MaxInZone <= 0) || (CountSpawnMobsInList(m_MobProto, m_DestinationRoom->GetZone()->m_Characters) < m_MaxInZone)))
	{
		ms_LastMob = CharData::Create(m_MobProto);
		if (ms_LastMob)
		{
			ms_LastMob->ToRoom(m_DestinationRoom);
			finish_load_mob(ms_LastMob);
			return true;
		}
	}
	return false;
}

void ResetLoadMobile::DoSave(Lexi::OutputParserFile &file)
{
	//	Write out Load Mobile specific stuff
	file.WriteVirtualID("Mobile", m_MobProto->GetVirtualID());
	file.WriteVirtualID("Room", m_DestinationRoom->GetVirtualID());
	
	if (m_MaxInRoom > 0)	file.WriteInteger("MaxInRoom", m_MaxInRoom);
	if (m_MaxInZone > 0)	file.WriteInteger("MaxInZone", m_MaxInZone);
	if (m_MaxInGame > 0)	file.WriteInteger("MaxInGame", m_MaxInGame);
}

void ResetLoadMobile::DoLoad(Lexi::Parser &parser)
{
	m_MobProto			= mob_index.Find(parser.GetString("Mobile"));
	m_DestinationRoom	= world.Find(parser.GetString("Room"));

	m_MaxInRoom			= parser.GetInteger("MaxInRoom");
	m_MaxInZone			= parser.GetInteger("MaxInZone");
	m_MaxInGame			= parser.GetInteger("MaxInGame");
}


bool ResetLoadMobile::IsValid()
{
	// maybe a little excessive to check out of bounds topside
	if (m_MobProto == NULL)			return false;
	if (m_DestinationRoom == NULL)	return false;

	return true;
}


// [ ResetLoadObject Stuff ]
bool ResetLoadObject::DoExecute(bool bLastCommandSucceeded)
{
	if (  m_DestinationRoom
	&& ( (m_MaxInGame <= 0) || (m_ObjProto->m_Count < m_MaxInGame))
	&& ( (m_MaxInRoom <= 0) || (CountSpawnObjectsInList(m_ObjProto, m_DestinationRoom->contents) < m_MaxInRoom))
//	&& ( (m_MaxInZone <= 0) || (count_objs_in_zone(m_ObjRNum, m_DestinationRoom) < m_MaxInZone))
	)
	{
		ms_LastObj = ObjData::Create(m_ObjProto);
		if (ms_LastObj)
		{
			ms_LastObj->ToRoom(m_DestinationRoom);
			load_otrigger(ms_LastObj);
			return true;
		}
	}
	return false;
}

void ResetLoadObject::DoSave(Lexi::OutputParserFile &file)
{
	file.WriteVirtualID("Object", m_ObjProto->GetVirtualID());
	file.WriteVirtualID("Room", m_DestinationRoom->GetVirtualID());

	if (m_MaxInRoom > 0)	file.WriteInteger("MaxInRoom", m_MaxInRoom);
	if (m_MaxInZone > 0)	file.WriteInteger("MaxInZone", m_MaxInZone);
	if (m_MaxInGame > 0)	file.WriteInteger("MaxInGame", m_MaxInGame);
}

void ResetLoadObject::DoLoad(Lexi::Parser &parser)
{
	m_ObjProto			= obj_index.Find(parser.GetString("Object"));
	m_DestinationRoom	= world.Find(parser.GetString("Room"));

	m_MaxInRoom			= parser.GetInteger("MaxInRoom");
	m_MaxInZone			= parser.GetInteger("MaxInZone");
	m_MaxInGame			= parser.GetInteger("MaxInGame");
}

bool ResetLoadObject::IsValid()
{
	if (m_ObjProto == NULL)			return false;
	if (m_DestinationRoom == NULL)	return false;

	return true;
}


// [ ResetEquipMobile Stuff ]
bool ResetEquipMobile::DoExecute(bool bLastCommandSucceeded)
{
	// problem : Equip WHAT mobile?
	// solution: Modify the zedit so that it works like PutObjectInObject, define a MobVNum to insert into
	CharData *mob = NULL;
	void load_otrigger(ObjData *obj);
	int wear_otrigger(ObjData *obj, CharData *actor, int where);


	if (IsDependent() && ms_LastMob && ms_LastMob->GetPrototype() == m_MobProto)
		mob = ms_LastMob;
	else
	{
		FOREACH(CharacterList, m_DestinationRoom->people, iter)
		{
			CharData *person = *iter;
			if (IS_NPC(person) && person->GetPrototype() == m_MobProto)
			{
				mob = person;	//	sue me!
				break;
			}
		}
	}

	if (!mob)
	{
		return false;
	}

	if (m_MaxInGame >= 0 || m_ObjProto->m_Count < m_MaxInGame) {
		if (m_EqPosition < 0 || m_EqPosition >= NUM_WEARS) {
			switch (m_EqPosition) {
				case POS_WIELD_TWO:
				case POS_HOLD_TWO:
				case POS_WIELD:
					if (!mob->equipment[m_EqPosition]) {
						ms_LastObj = ObjData::Create(m_ObjProto);
						equip_char(mob, ms_LastObj, WEAR_HAND_R);
						load_otrigger(ms_LastObj);
						if (!PURGED(ms_LastObj) && !PURGED(mob))
							wear_otrigger(ms_LastObj, mob, WEAR_HAND_R);
						return true;
					}
					break;
				case POS_WIELD_OFF:
				case POS_LIGHT:
				case POS_HOLD:
					if (!mob->equipment[m_EqPosition]) {
						ms_LastObj = ObjData::Create(m_ObjProto);
						equip_char(mob, ms_LastObj, WEAR_HAND_L);
						load_otrigger(ms_LastObj);
						if (!PURGED(ms_LastObj) && !PURGED(mob))
							wear_otrigger(ms_LastObj, mob, WEAR_HAND_L);
						return true;
					}
					break;
				default:
					return false;
			}
		} else if (!mob->equipment[m_EqPosition]) {
			ms_LastObj = ObjData::Create(m_ObjProto);
			equip_char(mob, ms_LastObj, m_EqPosition);
			load_otrigger(ms_LastObj);
			if (!PURGED(ms_LastObj) && !PURGED(mob))
				wear_otrigger(ms_LastObj, mob, m_EqPosition);
			return true;
		}
	}
	return false;
}

void ResetEquipMobile::DoSave(Lexi::OutputParserFile &file)
{
	file.WriteVirtualID("Object", m_ObjProto->GetVirtualID());
	file.WriteVirtualID("Mobile", m_MobProto->GetVirtualID());
	file.WriteVirtualID("Room", m_DestinationRoom->GetVirtualID());
	file.WriteEnum("Location", m_EqPosition, equipment_types);

	if (m_MaxInGame > 0)	file.WriteInteger("MaxInGame", m_MaxInGame);
}

void ResetEquipMobile::DoLoad(Lexi::Parser &parser)
{
	m_ObjProto			= obj_index.Find(parser.GetString("Object"));
	m_MobProto			= mob_index.Find(parser.GetString("Mobile"));
	m_DestinationRoom	= world.Find(parser.GetString("Room"));

	m_EqPosition		= parser.GetEnum("Location", equipment_types);
	m_MaxInGame			= parser.GetInteger("MaxInGame", -1);
}

bool ResetEquipMobile::IsValid()
{
	if (m_ObjProto == NULL)			return false;
	if (m_MobProto == NULL)			return false;
	if (m_DestinationRoom == NULL)	return false;

	return true;
}

// [ ResetGiveObject Stuff ]
bool ResetGiveObject::DoExecute(bool bLastCommandSucceeded)
{
	if (!ms_LastMob)
	{
		mudlogf(BRF, LVL_STAFF, TRUE, "ZONERR: GiveObjToMob reset #%i has no mobile to load to.", this->m_LineNum);
		return bLastCommandSucceeded;
	}

	if (m_MaxInGame <= 0 || m_ObjProto->m_Count < m_MaxInGame)
	{
		ms_LastObj = ObjData::Create(m_ObjProto);
		ms_LastObj->ToChar(ms_LastMob);
		load_otrigger(ms_LastObj);
		return true;
	}
	return false;
}

void ResetGiveObject::DoSave(Lexi::OutputParserFile &file)
{
	file.WriteVirtualID("Object", m_ObjProto->GetVirtualID());
	file.WriteVirtualID("Mobile", m_MobProto->GetVirtualID());
	file.WriteVirtualID("Room", m_DestinationRoom->GetVirtualID());

	if (m_MaxInGame > 0)	file.WriteInteger("MaxInGame", m_MaxInGame);
}

void ResetGiveObject::DoLoad(Lexi::Parser &parser)
{
	m_ObjProto			= obj_index.Find(parser.GetString("Object"));
	m_MobProto			= mob_index.Find(parser.GetString("Mobile"));
	m_DestinationRoom	= world.Find(parser.GetString("Room"));

	m_MaxInGame			= parser.GetInteger("MaxInGame", -1);
}

bool ResetGiveObject::IsValid()
{
	if (m_ObjProto == NULL)			return false;
	if (m_MobProto == NULL)			return false;
	if (m_DestinationRoom == NULL)	return false;

	return true;
}

// [ ResetPutObject Stuff ]
bool ResetPutObject::DoExecute(bool bLastCommandSucceeded)
{
	if (m_MaxInGame <= 0 || m_ObjProto->m_Count < m_MaxInGame)
	{
		ObjData *container = NULL;
		
		if (m_DestinationRoom)	container = get_obj_in_list_num(m_ContainerProto->GetVirtualID(), container->contents);
		if (!container)			container = get_obj_num(m_ContainerProto->GetVirtualID());
		
		if (!container)
		{
			// log it?
			return bLastCommandSucceeded;
		}
		ms_LastObj = ObjData::Create(m_ObjProto);
		ms_LastObj->ToObject(container);
		load_otrigger(ms_LastObj);
		return true;
	}
	return false;
}

void ResetPutObject::DoSave(Lexi::OutputParserFile &file)
{
	file.WriteVirtualID("Object", m_ObjProto->GetVirtualID());
	file.WriteVirtualID("Container", m_ContainerProto->GetVirtualID());
	file.WriteVirtualID("Room", m_DestinationRoom->GetVirtualID());

	if (m_MaxInGame > 0)	file.WriteInteger("MaxInGame", m_MaxInGame);
}

void ResetPutObject::DoLoad(Lexi::Parser &parser)
{
	m_ObjProto			= obj_index.Find(parser.GetString("Object"));
	m_ContainerProto	= obj_index.Find(parser.GetString("Container"));
	m_DestinationRoom	= world.Find(parser.GetString("Room"));

	m_MaxInGame			= parser.GetInteger("MaxInGame", -1);
}

bool ResetPutObject::IsValid()
{
	if (m_ObjProto == NULL)			return false;
	if (m_ContainerProto == NULL)	return false;
	if (m_DestinationRoom == NULL)	return false;

	return true;
}

// [ ResetDoor Stuff ]
bool ResetDoor::DoExecute(bool bLastCommandSucceeded)
{
	RoomExit *exit = m_DestinationRoom->GetExit(m_Direction);

	// validity of direction and destination room done in IsInvalid and DoFinalize, no problem
	if (!exit)
	{
		// logit?
		return false;	// in reset_zones, it would set false, but at end of door reset it sets to true
	}

	switch (m_DoorState)
	{
		case 0:
			exit->GetStates().clear(EX_STATE_CLOSED)
							 .clear(EX_STATE_LOCKED);
			break;
		case 1:
			exit->GetStates().set(EX_STATE_CLOSED)
							 .clear(EX_STATE_LOCKED);
			break;
		case 2:
			exit->GetStates().set(EX_STATE_CLOSED)
							 .set(EX_STATE_LOCKED);
			break;
	}

	return true;
}

void ResetDoor::DoSave(Lexi::OutputParserFile &file)
{
	file.WriteVirtualID("Room", m_DestinationRoom->GetVirtualID());
	file.WriteEnum("State", m_DoorState);
	file.WriteEnum("Direction", m_Direction, dirs);
}

void ResetDoor::DoLoad(Lexi::Parser &parser)
{
	m_DestinationRoom	= world.Find(parser.GetString("Room"));
	m_DoorState			= parser.GetEnum("State", ResetCommand::DOOR_OPEN);
	m_Direction			= parser.GetEnum("Direction", dirs);
}

bool ResetDoor::IsValid()
{
	if (m_DestinationRoom == NULL)						return false;
	if (m_Direction < 0 || m_Direction >= NUM_OF_DIRS)	return false;
	return true;
}

// [ ResetRemoveObject Stuff ]
bool ResetRemoveObject::DoExecute(bool bLastCommandSucceeded)
{
	ms_LastObj = get_obj_in_list_num(m_ObjProto->GetVirtualID(), m_DestinationRoom->contents);
	if (ms_LastObj)
	{
		ms_LastObj->FromRoom();
		ms_LastObj->Extract();
		ms_LastObj = NULL;		// safety first!
	}

	return true;
}

void ResetRemoveObject::DoSave(Lexi::OutputParserFile &file)
{
	file.WriteVirtualID("Object", m_ObjProto->GetVirtualID());
	file.WriteVirtualID("Room", m_DestinationRoom->GetVirtualID());
}

void ResetRemoveObject::DoLoad(Lexi::Parser &parser)
{
	m_ObjProto			= obj_index.Find(parser.GetString("Object"));
	m_DestinationRoom	= world.Find(parser.GetString("Room"));
}

bool ResetRemoveObject::IsValid()
{
	if (m_ObjProto == NULL)			return false;
	if (m_DestinationRoom == NULL)	return false;

	return true;
}

// [ ResetCommand Stuff ]
bool ResetMobCommand::DoExecute(bool bLastCommandSucceeded)
{
	if (ms_LastMob && !m_Command.empty())
	{
		BUFFER(arg, MAX_INPUT_LENGTH);
		strcpy(arg, m_Command.c_str());
		command_interpreter(ms_LastMob, arg);
		return true;
	}
	return false;
}

void ResetMobCommand::DoSave(Lexi::OutputParserFile &file)
{
	file.WriteVirtualID("Mobile", m_MobProto->GetVirtualID());
	file.WriteVirtualID("Room", m_DestinationRoom->GetVirtualID());
	file.WriteString("Command", m_Command.c_str());
}

void ResetMobCommand::DoLoad(Lexi::Parser &parser)
{
	m_MobProto			= mob_index.Find(parser.GetString("Mobile"));
	m_Command			= parser.GetString("Command");
	m_DestinationRoom	= world.Find(parser.GetString("Room"));
}

bool ResetMobCommand::IsValid()
{
	if (m_MobProto == NULL)			return false;
	if (m_DestinationRoom == NULL)	return false;

	return true;
}


// [ ResetLoadTrigger Stuff ]
bool ResetLoadTrigger::DoExecute(bool bLastCommandSucceeded)
{
	Scriptable *sc = NULL;

	if (m_MaxInGame <= 0 || m_ScriptProto->m_Count < m_MaxInGame)
	{
		switch (m_TriggerType)
		{
			case MOB_TRIGGER:
				if (!ms_LastMob)
				{
					return false;
				}
				sc = SCRIPT(ms_LastMob);
				break;
			case OBJ_TRIGGER:
				if (!ms_LastObj)
				{
					return false;
				}
				sc = SCRIPT(ms_LastObj);
				break;
			case WLD_TRIGGER:
				sc = SCRIPT(m_DestinationRoom);
				break;
		} // switch

		bool found = false;
		FOREACH(TrigData::List, TRIGGERS(sc), trig)
		{
			if ((*trig)->GetPrototype() == m_ScriptProto)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			sc->AddTrigger(TrigData::Create(m_ScriptProto));
		}
	}
	return true;
}

void ResetLoadTrigger::DoSave(Lexi::OutputParserFile &file)
{
	file.WriteVirtualID("Trigger", m_ScriptProto->GetVirtualID());
	file.WriteEnum("TriggerType", m_ScriptProto->m_pProto->m_Type, trig_data_types);
	file.WriteVirtualID("Room", m_DestinationRoom->GetVirtualID());

	if (m_MaxInGame > 0)	file.WriteInteger("MaxInGame", m_MaxInGame);
}

void ResetLoadTrigger::DoLoad(Lexi::Parser &parser)
{
	m_ScriptProto		= trig_index.Find(parser.GetString("Trigger"));
	m_TriggerType		= parser.GetEnum("TriggerType", trig_data_types);
	m_DestinationRoom	= world.Find(parser.GetString("Room"));
	m_MaxInGame			= parser.GetInteger("MaxInGame", -1);
}

bool ResetLoadTrigger::IsValid()
{
	if (m_ScriptProto == NULL)		return false;
	if (m_DestinationRoom == NULL)	return false;

	return true;
}
