/*************************************************************************
*   File: rooms.cp                   Part of Aliens vs Predator: The MUD *
*  Usage: Primary code for rooms                                         *
*************************************************************************/

#include <stdarg.h>

#include "rooms.h"
#include "zones.h"
#include "characters.h"
#include "descriptors.h"
#include "interpreter.h"
#include "utils.h"
#include "buffer.h"
#include "db.h"
#include "extradesc.h"
#include "files.h"
#include "find.h"
#include "comm.h"
#include "constants.h"
#include "idmap.h"
#include "lexifile.h"
#include "lexiparser.h"
#include "olc.h"

RoomMap	world;


RoomData::RoomData(VirtualID v)
:	m_Virtual(v)
,	m_Zone(zone_table.Find(v.zone))
,	m_SectorType(SECT_INSIDE)
,	m_NumLights(0)
,	func(NULL)
{
}


RoomData::RoomData(const RoomData &room)
:	Entity(room)
,	m_Virtual(room.m_Virtual)
,	m_Zone(room.m_Zone)
,	m_pName(room.m_pName)
,	m_pDescription(room.m_pDescription)
,	m_Flags(room.m_Flags)
,	m_SectorType(room.m_SectorType)
,	m_NumLights(0)
,	func(room.func)
,	m_ExtraDescriptions(room.m_ExtraDescriptions)
//,	m_pSearchNode(NULL)
{
	for (int i = 0; i < NUM_OF_DIRS; ++i)
	{
		if (room.m_Exits[i])
			m_Exits[i] = new RoomExit(*room.m_Exits[i]);
	}
}


void RoomData::operator=(const RoomData &room)
{
	if (&room == this)
		return;

	Entity::operator=(room);
	
	m_pName = room.m_pName;
	m_pDescription = room.m_pDescription;
	
	m_Flags = room.m_Flags;
	m_SectorType = room.m_SectorType;
	//	m_NumLight	--	No copy
	//	func 		--	No copy (not editable)

	for (int i = 0; i < NUM_OF_DIRS; ++i)
	{
		m_Exits[i] = room.m_Exits[i] ? new RoomExit(*room.m_Exits[i]) : NULL;
	}
	
	m_ExtraDescriptions = room.m_ExtraDescriptions;
}


RoomData::~RoomData()
{
}


void RoomData::SetVirtualID(VirtualID vid)
{
	m_Virtual = vid;
	m_Zone = zone_table.Find(vid.zone);
}


bool RoomData::IsLight() const
{
	//	Light in the room overrides darkness
	if (m_NumLights > 0)				return true;
	
	//	Dark rooms overrides inside/city/indoors
	if (m_Flags.test(ROOM_DARK))		return false;
	
	//	Inside/city/indoors overrides the sun
	if (m_SectorType == SECT_INSIDE
		|| m_SectorType == SECT_CITY
		|| m_Flags.test(ROOM_INDOORS))	return true;
	
	//	Not part of a zone, assume there is light, otherwise get zone's light level
	return !m_Zone || m_Zone->IsLight();
}


int RoomData::Send(const char *messg, ...)
{
	va_list args;
	DescriptorData *	d;
	
	if (!messg || !*messg || !this || this->people.empty())
		return 0;
 
 	BUFFER(send_buf, MAX_STRING_LENGTH);
 	
	va_start(args, messg);
	int length = vsnprintf(send_buf, MAX_STRING_LENGTH, messg, args);
	va_end(args);
	
	FOREACH(CharacterList, this->people, iter)
	{
		CharData *i = *iter;
		
		d = i->desc;
		if (d && d->GetState()->IsPlaying() && !PLR_FLAGGED(i, PLR_WRITING) && AWAKE(i))
			d->Write(send_buf);
//		else if (!d)
//			act(send_buf, FALSE, i, 0, 0, TO_CHAR);
	}

	return length;
}


RoomExit::RoomExit(void)
:	m_pKeyword("door")
,	room(NULL)
{
}

/*
RoomExit::RoomExit(const RoomExit &dir) :
	exit_info(dir.exit_info),
	key(dir.key),
	ToRoom(dir.ToRoom),
	m_pDescription(dir.m_pDescription),
	m_pKeyword(dir.m_pKeyword)
{
}


RoomExit::~RoomExit(void) {
	if (m_pDescription)		free(m_pDescription);
	FREE(m_pKeyword);
}
*/


/* read direction data */
void RoomExit::Parse(Lexi::Parser &parser, Hash zone)
{
	m_pKeyword		= parser.GetString	("Keywords"		, "door"	);
	m_pDescription	= parser.GetString	("Description"				);
	roomVnum		= parser.GetVirtualID("Destination"	, zone		);
	m_Flags			= parser.GetBitFlags<ExitFlag>("Flags");
	key				= parser.GetVirtualID("Key"			, zone		);
	
	if (!m_pDescription.empty())
	{
		if (m_pDescription[m_pDescription.size() - 1] != '\n')
			m_pDescription += "\n";
	}
}


void RoomExit::Write(Lexi::OutputParserFile &file, Hash zone)
{
	if (ToRoom())		file.WriteVirtualID("Destination", ToRoom()->GetVirtualID(), zone);
	file.WriteString	("Keywords", GetKeyword(), "door");
	file.WriteLongString("Description", m_pDescription.c_str());
	file.WriteFlags		("Flags", m_Flags);
	file.WriteVirtualID	("Key", key, zone);
}


void RoomData::Parse(Lexi::Parser &parser, VirtualID vid)
{
	RoomData *room = new RoomData(vid);
	world.insert(room);
	
//	room->m_Zone				= zone_table.Find(vid.zone);
	
	room->m_pName				= parser.GetString	("Name"			, "A blank room."	);
	room->m_pDescription		= parser.GetString	("Description"	, "  Nothing.\n"	);
	room->m_Flags				= parser.GetBitFlags<RoomFlag>("RoomFlags");
	room->m_SectorType			= parser.GetEnum	("Sector"		, SECT_INSIDE);
	
	//	EXITS
	for (int i = 0; i < NUM_OF_DIRS; ++i)
	{
		Lexi::String	section = Lexi::Format("Exit %s", GetEnumName<Direction>(i));
		
		if (!parser.DoesSectionExist(section))
			continue;
		
		PARSER_SET_ROOT(parser, section);
		
		RoomExit *dir = new RoomExit;
		dir->Parse(parser, vid.zone);
		
		room->m_Exits[i] = dir;
	}
	
	//	EXTRADESCS
	int numExtraDescs = parser.NumSections("ExtraDescs");
	for (int i = 0; i < numExtraDescs; ++i)
	{
		PARSER_SET_ROOT_INDEXED(parser, "ExtraDescs", i);
		
		const char *	keywords		= parser.GetString("Keywords");
		const char *	description		= parser.GetString("Description");
		
		if (!*keywords || !*description)
			continue;
		
		room->m_ExtraDescriptions.push_back(ExtraDesc(keywords, description));
	}
	
	if (room->m_Flags.test(ROOM_STARTHIVED))
		room->m_Flags.set(ROOM_HIVED);
}


void RoomData::Write(Lexi::OutputParserFile &file)
{
	file.WriteVirtualID("Virtual", GetVirtualID(), GetVirtualID().zone);
	
	file.WriteString	("Name", GetName(), "A blank room.");
	file.WriteLongString("Description", GetDescription(), "  Nothing.\n");
	
	static const RoomFlags MAKE_BITSET(unwritableFlags, ROOM_HOUSE, ROOM_HOUSE_CRASH, ROOM_HIVED);
	RoomFlags roomflags = m_Flags;
	roomflags.clear(unwritableFlags);
	file.WriteFlags		("RoomFlags", roomflags /* mFlags & ~unwritableFlags */);
	file.WriteEnum		("Sector", m_SectorType);

	//	Handle exits.
	for (int dir = 0; dir < NUM_OF_DIRS; dir++)
	{
		RoomExit *exit = m_Exits[dir];
		
		if (!exit /*|| (exit->ToRoom() == NULL && exit->m_pDescription.empty())*/)
			continue;
		
		file.BeginSection(Lexi::Format("Exit %s", GetEnumName<Direction>(dir)));
		exit->Write(file, GetVirtualID().zone);
		file.EndSection();
	}
	
	file.BeginSection("ExtraDescs");
	{
		//	Home straight, just deal with extra descriptions.
		int	count = 0;
		FOREACH(ExtraDesc::List, m_ExtraDescriptions, extradesc)
		{
			if (!extradesc->keyword.empty() && !extradesc->description.empty())
			{
				file.BeginSection(Lexi::Format("ExtraDesc %d", ++count));
				{
					file.WriteString("Keywords", extradesc->keyword);
					file.WriteLongString("Description", extradesc->description);
				}
				file.EndSection();
			}
		}
	}
	file.EndSection();
}


RoomData *RoomData::Find(IDNum id)
{
	Entity *e = IDManager::Instance()->Find(id);
	return dynamic_cast<RoomData *>(e);
}


void RoomData::AddLight(unsigned int radius, int dir)
{
	++m_NumLights;
	
	if (radius > 0)
	{
		if (dir == -1)
		{
			for (dir = 0; dir < NUM_OF_DIRS; ++dir)
			{
				RoomExit *exit = m_Exits[dir];
				
				if (!exit || !exit->ToRoom() || exit->GetFlags().test(EX_ISDOOR))
					continue;
				
				exit->ToRoom()->AddLight(radius - 1, dir);
			}
		}
		else
		{
			RoomExit *exit = m_Exits[dir];
			
			if (exit && exit->ToRoom() && !exit->GetFlags().test(EX_ISDOOR))
				exit->ToRoom()->AddLight(radius - 1, dir);
		}
	}
}


void RoomData::RemoveLight(unsigned int radius, int dir)
{
	--m_NumLights;
	
	if (radius > 0)
	{
		if (dir == -1)
		{
			for (dir = 0; dir < NUM_OF_DIRS; ++dir)
			{
				RoomExit *exit = m_Exits[dir];
				
				if (!exit || !exit->ToRoom() || exit->GetFlags().test(EX_ISDOOR))
					continue;
				
				exit->ToRoom()->AddLight(radius - 1, dir);
			}
		}
		else
		{
			RoomExit *exit = m_Exits[dir];
			
			if (exit && exit->ToRoom() && !exit->GetFlags().test(EX_ISDOOR))
				exit->ToRoom()->AddLight(radius - 1, dir);
		}
	}
}


RoomMap::RoomMap()
:	m_Allocated(0)
,	m_Size(0)
,	m_bDirty(false)
,	m_Index(NULL)
{
}


RoomMap::~RoomMap()
{
	delete [] m_Index;
}


bool RoomMap::SortFunc(value_type lhs, value_type rhs)
{
//	if (lhs->GetVirtualID() < rhs->GetVirtualID())	return true;
//	if (lhs->GetVirtualID() == rhs->GetVirtualID())	return lhs->GetID() < rhs->GetID();
//	return false;
	return lhs->GetVirtualID() < rhs->GetVirtualID();
}


//	On benchmarking, this has been shown to be as fast as bsearch()
RoomMap::value_type RoomMap::Find(VirtualID vid)
{
	if (m_Size == 0)		return NULL;
	if (!vid.IsValid())		return NULL;
	
	if (m_bDirty)		Sort();
	
	//	Must be signed or it breaks
	int bot = 0;
	int top = m_Size - 1;

	do
	{
		int mid = (bot + top) / 2;
		
		value_type i = m_Index[mid];
		const VirtualID &ivid = i->GetVirtualID();
		
		if (ivid == vid)	return i;
		if (bot >= top)		break;
		if (ivid > vid)		top = mid - 1;
		else				bot = mid + 1;
	} while (1);

	ZoneData *z = VIDSystem::GetZoneFromAlias(vid.zone);
	if (z && z->GetHash() != vid.zone)
	{
		vid.zone = z->GetHash();
		return Find(vid);
	}
	
	return NULL;
}



RoomMap::value_type RoomMap::Find(const char *name)
{
//	return IsVirtualID(name) ? Find(VirtualID(name)) : NULL;
	return *name ? Find(VirtualID(name)) : NULL;
}


static bool RoomMapLowerBoundFunc(const RoomData *r, Hash zone)
{
	return (r->GetVirtualID().zone < zone);
}

static bool RoomMapUpperBoundFunc(Hash zone, const RoomData *r)
{
	return (zone < r->GetVirtualID().zone);
}


RoomMap::iterator RoomMap::lower_bound(Hash zone)
{
//	if (m_Size == 0)	return m_Index;
	
	if (m_bDirty)		Sort();

	return std::lower_bound(m_Index, m_Index + m_Size, zone, RoomMapLowerBoundFunc);
}


RoomMap::iterator RoomMap::upper_bound(Hash zone)
{
//	if (m_Size == 0)	return m_Index;
	
	if (m_bDirty)		Sort();
	
	return std::upper_bound(m_Index, m_Index + m_Size, zone, RoomMapUpperBoundFunc);
}


void RoomMap::renumber(Hash oldHash, Hash newHash)
{
	if (m_bDirty)		Sort();
	
	for (iterator i = lower_bound(oldHash), e = upper_bound(oldHash); i != e; ++i)
		(*i)->m_Virtual.zone = newHash;
	
	m_bDirty = true;
	Sort();
}


void RoomMap::renumber(RoomData *r, VirtualID newVid)
{
	if (!r)	return;
	
	r->m_Virtual = newVid;
	m_bDirty = true;
	Sort();
}


void RoomMap::insert(value_type i)
{
	IDManager::Instance()->GetAvailableIDNum(i);	//	This necessary?
	IDManager::Instance()->Register(i);
	
	if (m_Size == m_Allocated)
	{
		//	Grow: increase alloc size, alloc new, copy old to new, delete old, reassign
		m_Allocated += msGrowSize;
		value_type *newIndex = new value_type[m_Allocated];
		std::copy(m_Index, m_Index + m_Size, newIndex);
		delete [] m_Index;
		m_Index = newIndex;
	}
	
	//	Mark as dirty if the item being added would fall before the end of the entries
	if (!m_bDirty
		&& m_Size > 0
		&& i->GetVirtualID() < m_Index[m_Size - 1]->GetVirtualID())
	{
		m_bDirty = true;
	}

	//	Add new item to end
	m_Index[m_Size] = i;
	++m_Size;
}


void RoomMap::Sort()
{
	if (!m_bDirty)	return;
	
	std::sort(m_Index, m_Index + m_Size, SortFunc);
	
	m_bDirty = false;
}


/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms(void)
{
	if (world.Find(mortal_start_room) == NULL) {
		log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
		exit(1);
	}
	if (world.Find(immort_start_room) == NULL) {
		log("SYSERR:  Warning: Staff start room does not exist.  Change in config.c.");
	}
	if (world.Find(frozen_start_room) == NULL) {
		log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
	}
}


/* resolve all vnums into rnums in the world */
void renum_world(void) {
	int	door;

	FOREACH(RoomMap, world, iter)
	{
		RoomData *room = *iter;
		
		for (door = 0; door < NUM_OF_DIRS; door++)
		{
			if (room->GetExit(door))
			{
				room->GetExit(door)->room = world.Find(room->GetExit(door)->roomVnum);
			}
		}
	}
}


bool RoomData::CanGo(CharData *ch, int dir, bool ignorePeaceful)
{
	if (ch->InRoom() == NULL)							return false;
	
	RoomExit *exit = Exit::Get(ch, dir);
	
	if (!Exit::IsPassable(exit))			return false;
	
	if (IS_NPC(ch) && !ch->path
		&& (EXIT_FLAGGED(exit, EX_NOMOB)
			|| ROOM_FLAGGED(exit->ToRoom(), ROOM_NOMOB)))	return false;
	
	if (NO_STAFF_HASSLE(ch))						return true;

	if (!ignorePeaceful && AFF_FLAGGED(ch, AFF_NOQUIT)
			&& ROOM_FLAGGED(exit->ToRoom(), ROOM_PEACEFUL)
			&& !ROOM_FLAGGED(ch->InRoom(), ROOM_PEACEFUL))
		return false;

	//	Handle race flags...
	return !((EXIT_FLAGGED(exit, EX_NOHUMAN) && IS_HUMAN(ch)) ||
			(EXIT_FLAGGED(exit, EX_NOSYNTHETIC) && IS_SYNTHETIC(ch)) ||
			(EXIT_FLAGGED(exit, EX_NOPREDATOR) && IS_PREDATOR(ch)) ||
			(EXIT_FLAGGED(exit, EX_NOALIEN) && IS_ALIEN(ch)));
}

#if 0
namespace World
{
	typedef std::vector<RoomData *>	RoomVector;
	typedef std::list<RoomData *>	RoomList;
//	typedef std::map<IDNum, RoomData *>	RoomMap;

//	RoomVector			world;	
	RoomList			freeRooms;

	unsigned int		NumberOfVirtualRooms = 0;
}


void World::AddRoom(RoomData *room)
{
	//	If it's a virtual room, insert into the middle in proper location
	//	Otherwise, add to elsewhere!

	if (world.capacity() == world.size())
		world.reserve(world.size() + 1000);
	
	VirtualID newRoomVNum = room->GetVirtualID();
	
	if (newRoomVNum != -1)
	{
		if (world.size() > 0
			&& (world.back()->GetVirtualID() == -1
				|| world.back()->GetVirtualID() > newRoomVNum))
		{
			RoomVector::iterator iter = world.end(), end;
			
			for (iter = world.begin(), end = world.end(); iter != end; ++iter)
			{
				VirtualID vnum = (*iter)->GetVirtualID();
				if (vnum == -1 || vnum > newRoomVNum)
					break;
			}
			
			world.insert(iter, room);
		}
		else
		{
			world.push_back(room);
		}
		
		IDManager::GetAvailableIDNum(room);
		IDManager::Register(room);
		
		++NumberOfVirtualRooms;
	}
	else
	{
		world.push_back(room);
		
		IDManager::GetAvailableIDNum(room);
		IDManager::Register(room);
	}
	
//	room->SetID(ROOM_ID_BASE + world.size());
}


void World::RemoveRoom(RoomData *room)
{
	freeRooms.push_back(room);
}


RoomData *World::GetFreeRoom()
{
	RoomData *room = NULL;
	
	if (freeRooms.empty())
	{
		room = new RoomData;
		AddRoom(room);
	}
	else
	{
		room = freeRooms.front();
		*room = RoomData;
		freeRooms.pop_front();
	}
	
	return room;
}
#endif





RoomExit * Exit::Get(CharData *ch, unsigned int d)
{
	return Get(ch->InRoom(), d);
}


RoomExit * Exit::Get(ObjData *obj, unsigned int d)
{
	return Get(obj->InRoom(), d);
}


RoomExit *	Exit::Get(RoomData *r, unsigned int d)
{
	if (d < NUM_OF_DIRS)	return r->GetExit(d);
	else					return NULL;
}


RoomExit *	Exit::GetIfValid(CharData *ch, unsigned int d)
{
	return GetIfValid(ch->InRoom(), d);
}


RoomExit *	Exit::GetIfValid(ObjData *obj, unsigned int d)
{
	return GetIfValid(obj->InRoom(), d);
}


RoomExit *	Exit::GetIfValid(RoomData *r, unsigned int d)
{
	RoomExit *rd = Get(r,d);
	return IsValid(rd) ? rd : NULL;
}


const ExitStates MAKE_BITSET(ExitClosedOrBlockedStates, EX_STATE_CLOSED, EX_STATE_BLOCKED, EX_STATE_DISABLED);
const ExitStates MAKE_BITSET(ExitNoMoveStates, EX_STATE_CLOSED, EX_STATE_IMPASSABLE, EX_STATE_BLOCKED, EX_STATE_DISABLED);
const ExitStates MAKE_BITSET(ExitNoVisionStates, EX_STATE_CLOSED, EX_STATE_BLOCKED, EX_STATE_DISABLED);


bool Exit::IsOpen(const RoomExit *d)
{
	return IsValid(d)
		&& !IsDoorClosed(d)
		&& !d->GetStates().test(EX_STATE_BLOCKED);
}


bool Exit::IsDoorClosed(const RoomExit *d)
{
	return IsValid(d)
//		&& d->GetFlags().test(EX_ISDOOR)
		&& d->GetStates().test(EX_STATE_CLOSED)
		&& !d->GetStates().test(EX_STATE_BREACHED);
}


bool Exit::IsPassable(const RoomExit *d)
{
	return IsOpen(d)
		&& !d->GetFlags().test(EX_NOMOVE)
		&& !d->GetStates().test(EX_STATE_IMPASSABLE);
}


bool Exit::IsViewable(const RoomExit *d)
{
	return IsOpen(d);
}


/*
int topNum = 0;
FOREACH_BOUNDED(TYPE, CONTAINER, ZONEHASH, i) {
VirtualID vid = (*i)->GetVirtualID();
if (vid.ns > NS) break;
if (vid.ns == NS) topNum = vid.number + 1;
}
*/
