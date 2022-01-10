/*************************************************************************
*   File: rooms.h                    Part of Aliens vs Predator: The MUD *
*  Usage: Header file for room data                                      *
*************************************************************************/

#ifndef __ROOMS_H__
#define __ROOMS_H__

#include "types.h"
#include "lexistring.h"
#include "lexilist.h"
#include "bitflags.h"
#include "entity.h"
#include "virtualid.h"
#include "races.h"
#include "internal.defs.h"
#include "ManagedPtr.h"
#include "extradesc.h"

class RoomData;
class RoomExit;
class SearchNode;


namespace Lexi
{
	class Parser;
	class OutputParserFile;
}



/* The cardinal directions: used as index to room_data.m_Exits[] */
enum Direction
{
	INVALID_DIRECTION = -1,
	NORTH		= 0,
	EAST		= 1,
	SOUTH		= 2,
	WEST		= 3,
	UP			= 4,
	DOWN		= 5,
	NUM_OF_DIRS
};


/* Room flags: used in room_data.room_flags */

enum RoomFlag
{
	ROOM_DARK,
	ROOM_DEATH,
	ROOM_NOMOB,
	ROOM_INDOORS,
	ROOM_PEACEFUL,
	ROOM_SOUNDPROOF,
	ROOM_BLACKOUT,
	ROOM_NOTRACK,
	ROOM_PARSE,
	ROOM_TUNNEL,
	ROOM_PRIVATE,
	ROOM_STAFFROOM,
	ROOM_ADMINROOM,
	ROOM_NOPK,
	ROOM_SPECIAL_MISSION,
	ROOM_VEHICLE,
	ROOM_NOLITTER,
	ROOM_GRAVITY,
	ROOM_VACUUM,
	ROOM_NORECALL,
	ROOM_NOHIVE,
	ROOM_STARTHIVED,
#define NUM_OLC_ROOM_FLAGS (ROOM_STARTHIVED + 1)
	//	Private Bits
	ROOM_HOUSE,
	ROOM_HOUSE_CRASH,
	ROOM_HIVED,
	ROOM_DELETED,
	NUM_ROOM_FLAGS
};
typedef Lexi::BitFlags<NUM_ROOM_FLAGS, RoomFlag> RoomFlags;


enum ExitFlag
{
	EX_ISDOOR,		//	Exit has a door
	EX_PICKPROOF,	//	Lock can't be picked
	EX_HIDDEN,		//	Exit is hidden
	EX_MUTABLE,		//	Exit states can be modified by any script
	EX_NOSHOOT,		//	No shooting through the exit
	EX_NOMOVE,		//	No moving through the exit
	EX_NOMOB,		//	No mobs may move through the exit
	EX_NOVEHICLES,	//	No vehicles
	EX_NOHUMAN,		//	No humans
	EX_NOSYNTHETIC,	//	No synthetics
	EX_NOPREDATOR,	//	No predators
	EX_NOALIEN,		//	No aliens
	NUM_EXIT_FLAGS
};
typedef Lexi::BitFlags<NUM_EXIT_FLAGS, ExitFlag> ExitFlags;


enum ExitState
{
	EX_STATE_CLOSED,	//	Door is closed
	EX_STATE_LOCKED,	//	Door is locked
	EX_STATE_IMPASSABLE,//	Exit has become impassable
	EX_STATE_BLOCKED,
	EX_STATE_DISABLED,	//	Exit acts as if it is non-existant
	EX_STATE_BREACHED,	//	Door is broken open and cannot be closed
	EX_STATE_JAMMED,	//	Door is broken closed and cannot be opened
	EX_STATE_HIDDEN,
	NUM_EXIT_STATES
};
typedef Lexi::BitFlags<NUM_EXIT_STATES, ExitState> ExitStates;
//extern const RoomExitStates RoomExitClosedOrBlockedStates; //MAKE_BITSET(RoomExitNoMoveStates, EX_STATE_CLOSED, EX_STATE_BLOCKED, EX_STATE_DISABLED);
//extern const RoomExitStates RoomExitNoMoveStates; //MAKE_BITSET(RoomExitNoMoveStates, EX_STATE_CLOSED, EX_STATE_IMPASSABLE, EX_STATE_BLOCKED, EX_STATE_DISABLED);
//extern const RoomExitStates RoomExitNoVisionStates; //MAKE_BITSET(RoomExitNoVisionStates, EX_STATE_CLOSED, EX_STATE_BLOCKED, EX_STATE_DISABLED);

/* Sector types: used in room_data.sector_type */
enum RoomSector
{
	SECT_INSIDE,		//	Indoors
	SECT_CITY,			//	In a city
	SECT_ROAD,			//	On a road
	SECT_FIELD,			//	In a field
	SECT_FOREST,		//	In a forest
	SECT_BEACH,
	SECT_DESERT,
	SECT_JUNGLE,
	SECT_HILLS,			//	In the hills
	SECT_SNOW,
	SECT_MOUNTAIN,		//	On a mountain
	SECT_WATER_SWIM,	//	Swimmable water
	SECT_WATER_NOSWIM,	//	Water - need a boat
	SECT_UNDERWATER,	//	Underwater
	SECT_FLYING,		//	Wheee!
	SECT_SPACE,			//	Guess...
	SECT_DEEPSPACE,		//	Guess again!
	
	NUM_SECTORS
};


//	Room Exits
class RoomExit
{
public:
	friend class REdit;
	friend class RoomExitEditor;
	
						RoomExit();
//						RoomExit(const RoomExit &dir);
//						~RoomExit();
	
	const char *		GetKeyword() const { return m_pKeyword.c_str(); }
	const char *		GetDescription() const { return m_pDescription.c_str(); }
	
	void				SetKeyword(const char *keyword) { m_pKeyword = keyword; }
	void				SetDescription(const char *description) { m_pDescription = description; }
	
	RoomData *			ToRoom() const { return room; }
	
	const ExitFlags &	GetFlags() const { return m_Flags; }
	
	ExitStates &		GetStates() { return m_States; }
	const ExitStates &	GetStates() const { return m_States; }

	void				Parse(Lexi::Parser &parser, Hash zone);
	void				Write(Lexi::OutputParserFile &file, Hash zone);

private:
	Lexi::String		m_pDescription;
	Lexi::String		m_pKeyword;

	ExitFlags			m_Flags;			//	Exit info
	ExitStates			m_States;
	
public:
//	RaceFlags			m_RaceRestriction;
	VirtualID			key;					//	Key's number (-1 for no key)
	RoomData *			room;
	VirtualID			roomVnum;			//	This is temporary until exits are resolved	
};


//	Class for Rooms
class RoomData : public Entity
{
public:
						RoomData(VirtualID v = VirtualID());
						RoomData(const RoomData &room);
						~RoomData();
	
	static RoomData *	Find(IDNum i);
	
	static void			Parse(Lexi::Parser &parser, VirtualID nr);
	void				Write(Lexi::OutputParserFile &file);

	//	Entity
	virtual Entity::Type	GetEntityType() { return Entity::TypeRoom; }

	const VirtualID &	GetVirtualID() const { return m_Virtual; }
	void				SetVirtualID(VirtualID vid);

	const char *		GetName() const { return m_pName.c_str(); }
	const char *		GetDescription() const { return m_pDescription.c_str(); }

	void				SetName(const char *name) { m_pName = name; }
	void				SetDescription(const char *pDescription) { m_pDescription = pDescription; }

	RoomFlags &			GetFlags() { return m_Flags; }
	const RoomFlags &	GetFlags() const { return m_Flags; }
	
	RoomExit *			GetExit(unsigned int i) { /*assert(i < NUM_OF_DIRS);*/ return m_Exits[i]; }
	const RoomExit *	GetExit(unsigned int i) const { /*assert(i < NUM_OF_DIRS);*/ return m_Exits[i]; }
	
	void				SetExit(unsigned int i, RoomExit *ex) { /*assert(i < NUM_OF_DIRS);*/ m_Exits[i] = ex; }
	
	ZoneData *			GetZone() const { return m_Zone; }
	
	int					GetLight() const { return m_NumLights; }
	void				AddLight(unsigned int radius = 0, int dir = -1);
	void				RemoveLight(unsigned int radius = 0, int dir = -1);
	
	RoomSector			GetSector() const { return m_SectorType; }
	void				SetSector(RoomSector sector) { m_SectorType = sector; }

	int					Send(const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
	
	static bool			CanGo(CharData *ch, int door, bool ignorePeaceful = false);
	
	bool				IsLight() const;
	
	
private:
	friend class RoomMap;
	friend class REdit;
	friend class RoomEditor;
	
	VirtualID			m_Virtual;
	ZoneData *			m_Zone;					//	Room zone (for resetting)
	
	Lexi::String		m_pName;
	Lexi::String		m_pDescription;

	RoomFlags			m_Flags;
	RoomSector			m_SectorType;			//	sector type (move/hide)
	
	int					m_NumLights;		//	Number of lightsources in room

	ManagedPtr<RoomExit>m_Exits[NUM_OF_DIRS];	//	Directions
	
	void				operator=(const RoomData &room);

public:
	SPECIAL(*func);

	Lexi::List<ObjData *>	contents;				//	List of items in room
	Lexi::List<CharData *>	people;					//	List of characters in room
//	Lexi::List<ShipData *>	ships;					//	List of ships in room
	
	ExtraDesc::List			m_ExtraDescriptions;		//	for examine/look
	
public:
//	SearchNode *		m_pSearchNode;
};


class RoomMap
{
public:
	typedef RoomData	data_type;
	typedef data_type *	value_type;
	typedef value_type *pointer;
	typedef value_type &reference;
	typedef pointer		iterator;
	typedef const pointer const_iterator;
	
	RoomMap();
	~RoomMap();
	
//	size_t				GetNumberOfVirtualRooms() { return m_NumberOfVirtualRooms; }
	size_t				size() { return m_Size; }
	bool				empty() { return m_Size == 0; }
	
	void				insert(value_type i);
	value_type			Find(VirtualID vid);
	value_type			Find(const char *arg);
	value_type			Get(size_t i)		{ return i < m_Size ? m_Index[i] : NULL; }
		
	value_type			operator[] (size_t i) { return Get(i); }
	
	iterator			begin()			{ return m_Index; }
	iterator			end()			{ return m_Index + m_Size; }
	const_iterator		begin() const	{ return m_Index; }
	const_iterator		end() const 	{ return m_Index + m_Size; }

	iterator			lower_bound(Hash zone);
	iterator			upper_bound(Hash zone);
		
	void				renumber(Hash old, Hash newHash);
	void				renumber(RoomData *r, VirtualID newVid);

private:
	unsigned int		m_Allocated;
	unsigned int		m_Size;
//	unsigned int		m_NumberOfVirtualRooms;
	bool				m_bDirty;
	value_type *		m_Index;
	
	static const int	msGrowSize = 512;
	
	void				Sort();
	static bool			SortFunc(value_type lhs, value_type rhs);
	
	RoomMap(const RoomMap &);
	RoomMap &operator=(const RoomMap &);
};


namespace World
{
	//	Room Management
//	void				AddRoom(RoomData *room);
//	void				RemoveRoom(RoomData *room);
	RoomData *			GetFreeRoom();
	
//	RoomData *			GetRoomByIndex(unsigned int i);
	
//	unsigned int		GetNumberOfVirtualRooms();
//	unsigned int		GetNumberOfRooms();
	unsigned int		GetNumberOfDynamicRooms();
	RoomData *			GetDynamicRoomByIndex(unsigned int i);
}

extern RoomMap		world;


//	Global data pertaining to rooms
extern VirtualID	mortal_start_room;
extern VirtualID	immort_start_room;
extern VirtualID	frozen_start_room;

//	Public functions in rooms.cp
void check_start_rooms(void);
void renum_world(void);


class REdit {
public:
	static void			parse(DescriptorData * d, const char *arg);
	static void			setup_new(DescriptorData *d);
	static void			setup_existing(DescriptorData *d, RoomData *room);
	static void			save_to_disk(ZoneData *zone);
	static void			save_internally(DescriptorData *d);
	static void			disp_extradesc_menu(DescriptorData * d);
	static void			disp_extradesc(DescriptorData * d);
	static void			disp_exit_menu(DescriptorData * d);
	static void			disp_exit_flag_menu(DescriptorData * d);
	static void			disp_flag_menu(DescriptorData * d);
	static void			disp_sector_menu(DescriptorData * d);
	static void			disp_menu(DescriptorData * d);
};




namespace Exit
{
	RoomExit *			Get(CharData *ch, unsigned int d);
	RoomExit *			Get(ObjData *obj, unsigned int d);
	RoomExit *			Get(RoomData *r, unsigned int d);
	
	RoomExit *			GetIfValid(ObjData *ch, unsigned int d);
	RoomExit *			GetIfValid(CharData *obj, unsigned int d);
	RoomExit *			GetIfValid(RoomData *r, unsigned int d);
	
	
	inline bool			IsValid(const RoomExit *e) { return e && e->ToRoom() && !e->GetStates().test(EX_STATE_DISABLED) ? e : NULL; }

	bool				IsOpen(const RoomExit *e);
	inline bool			IsOpen(CharData *ch, unsigned int d) { return IsOpen(Get(ch,d)); }
	inline bool			IsOpen(ObjData *obj, unsigned int d) { return IsOpen(Get(obj,d)); }
	inline bool			IsOpen(RoomData *r, unsigned int d) { return IsOpen(Get(r,d)); }
	
	bool				IsDoorClosed(const RoomExit *e);
	inline bool			IsDoorClosed(CharData *ch, unsigned int d) { return IsDoorClosed(Get(ch,d)); }
	inline bool			IsDoorClosed(ObjData *obj, unsigned int d) { return IsDoorClosed(Get(obj,d)); }
	inline bool			IsDoorClosed(RoomData *r, unsigned int d) { return IsDoorClosed(Get(r,d)); }
	
	bool				IsPassable(const RoomExit *e);
	inline bool			IsPassable(CharData *ch, unsigned int d) { return IsPassable(Get(ch,d)); }
	inline bool			IsPassable(ObjData *obj, unsigned int d) { return IsPassable(Get(obj,d)); }
	inline bool			IsPassable(RoomData *r, unsigned int d) { return IsPassable(Get(r,d)); }
	
	
	bool				IsViewable(const RoomExit *e);
	inline bool			IsViewable(CharData *ch, unsigned int d) { return IsViewable(Get(ch,d)); }
	inline bool			IsViewable(ObjData *obj, unsigned int d) { return IsViewable(Get(obj,d)); }
	inline bool			IsViewable(RoomData *r, unsigned int d) { return IsViewable(Get(r,d)); }
}


#endif

