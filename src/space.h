#ifndef __SPACE_H__
#define __SPACE_H__

#include "entity.h"
#include "character.defs.h"
#include "lexilist.h"

#define flabs(i)	((i) < 0 ? -(i) : (i))
#define UMIN(a, b)	((a) < (b) ? (a) : (b))
#define UMAX(a, b)	((a) > (b) ? (a) : (b))
#define URANGE(a, b, c)	((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))

#define DIR_NORTH		0
#define DIR_EAST		1
#define DIR_SOUTH		2
#define DIR_WEST		3
#define DIR_UP			4
#define DIR_DOWN		5

#define TO_SHIP		(1 << 0)
#define TO_COCKPIT	(1 << 1)
#define TO_GUNSEAT	(1 << 2)
#define TO_PILOTSEAT	(1 << 3)
#define TO_TURRETS	(1 << 4)
#define TO_HANGERS	(1 << 5)

#define UPGRADE_SHIELD		0
#define UPGRADE_ARMOR		1
#define UPGRADE_GROUND		2
#define UPGRADE_AIR		3
#define UPGRADE_INTERCEPTORS	4

#define RTYPE_PILOT		1
#define RTYPE_BRIDGE		2
#define RTYPE_COPILOT		3
#define RTYPE_TURRET		4
#define RTYPE_HANGER		5
#define RTYPE_GARAGE		6
#define RTYPE_ENGINE		7
#define RTYPE_LOUNGE		8
#define RTYPE_ENTRANCE		9
#define RTYPE_COCKPIT		10
#define RTYPE_MEDICAL		11
#define RTYPE_TROOPHOLD		12
#define RTYPE_DEPLOYMENT	13

#define DAM_ION			0
#define DAM_LASER		1
#define DAM_MISSILE		2
#define DAM_GRENADE		3
#define DAM_COLLISION		4

#define SHIP_LIST		SHIP_DIR "ships.lst"
#define SPACE_LIST		SPACE_DIR "space.lst"
#define STELLAROBJ_LIST		STELLAROBJ_DIR "objects.lst"

#define SHIP_DIR		"space/ships/"
#define SPACE_DIR		"space/systems/"
#define STELLAROBJ_DIR		"space/objects/"
#define STARSYSTEM_FILE		SPACE_DIR "space.lst"

#define SHP_OBJ_USENET		1213
#define SHP_OBJ_EXITHATCH	-1
#define SHP_OBJ_VIEWSCREEN	-1
#define SHP_OBJ_PILOTCHAIR	-1
#define SHP_OBJ_GUNNERCHAIR	-1
#define SHP_OBJ_MEDICAL_BED	-1
#define SHP_OBJ_ENGINE		-1
#define SHP_OBJ_GUNTURRET	-1
#define SHP_OBJ_INTERCEPTOR	-1

#define OBJ_VNUM_TOOLKIT	1218
#define OBJ_VNUM_LENS		1218
#define OBJ_VNUM_MIRROR		1218
#define OBJ_VNUM_BATTERY	1218
#define OBJ_VNUM_PLASTIC	1218
#define OBJ_VNUM_NEOSTEEL	1218
#define OBJ_VNUM_CIRCUIT	1218
#define OBJ_VNUM_SUPERCONDUCTOR	1218

#define ROOM_SHIP_LIMBO		5

#define SHP_FLAGS(ship)		((ship)->flags)
#define SHP_FLAGGED(ship, flag)	(IS_SET(SHP_FLAGS(ship), (flag)))
#define STR_FLAGS(sys)		((sys)->flags)
#define STR_FLAGGED(sys, flag)	(IS_SET(STR_FLAGS(sys), (flag)))
#define STO_FLAGS(obj)		((obj)->flags)
#define STO_FLAGGED(obj, flag)	(IS_SET(STO_FLAGS(obj), (flag)))
#define MODEL_FLAGS(ship, flag)	(IS_SET(model[(ship)->model].flags, (flag)))
#define MODEL_NAME(ship)	(model[(ship)->model].name)

// Ship flags
#define SHIP_AUTOPILOT		(1 << 0)	// a
#define SHIP_AUTOTRACK		(1 << 1)	// b
#define SHIP_REGEN		(1 << 2)	// c
#define SHIP_SABOTAGE		(1 << 3)	// d
#define SHIP_LIFESUPPORT	(1 << 4)	// e
#define SHIP_CLOAKER		(1 << 5)	// f
#define SHIP_CLOAKED		(1 << 6)	// g
#define SHIP_DETECTOR		(1 << 7)	// h
#define SHIP_WEBBER		(1 << 8)	// i
#define SHIP_WEBBED		(1 << 9)	// j
#define SHIP_IRRADIATOR		(1 << 10)	// k
#define SHIP_IRRADIATED		(1 << 11)	// l
#define SHIP_MASSCLOAK		(1 << 12)	// m
#define SHIP_EXOTIC		(1 << 13)	// n
// last bit
#define SHIP_CUSTOM_ZONE	(1 << 30)	// E
#define SHIP_DISABLED		(1 << 31)	// F

// Stellar Object Flags
#define OBJECT_HIDDEN		(1 << 0)	// a
#define OBJECT_NOSAVE		(1 << 31)	// F

// StarSystem Flags
#define STAR_HIDDEN		(1 << 0)	// a
#define STAR_NOJUMP		(1 << 1)	// b

enum ship_types {
	PLAYER_SHIP,	MOB_SHIP
};

typedef enum {	SHIP_DOCKED, SHIP_READY, SHIP_BUSY, SHIP_BUSY_2,
		SHIP_BUSY_3, SHIP_REFUEL, SHIP_LAUNCH, SHIP_LAUNCH_2,
		SHIP_LAND, SHIP_LAND_2, SHIP_HYPERSPACE,
		SHIP_FLYING, SHIP_DESTROY } static_ship_states;

typedef enum {	MISSILE_READY, MISSILE_FIRED, MISSILE_RELOAD,
		MISSILE_RELOAD_2, MISSILE_DAMAGED } missile_states;

typedef enum {	SPACECRAFT, SPACE_STATION, AIRCRAFT, BOAT, SUBMARINE,
		LAND_VEHICLE } ship_classes;

typedef enum {	CORSAIR,	WRAITH,		SCOUT,		INFWRAITH,
		SHUTTLE,	DROPSHIP,	OVERLORD,	VALKYRIE,
		ARBITER,	SCIENCEVESSEL,	QUEEN,		GUARDIAN,
		INFBATTLESHIP,	BATTLECRUISER,	CARRIER,	INTERCEPTOR,
		SIEGE_TANK,	REAVER,		ATV,
		CUSTOM_SHIP,	MAX_SHIP_MODEL } ship_models;

typedef enum {	FIGHTER_SHIP,	MIDSIZE_SHIP,	LARGE_SHIP,
		CAPITAL_SHIP,	SHIP_PLATFORM } ship_sizes;

typedef enum {	MOB_PATROL = 0, MOB_DESTROYER = 6, MOB_CRUISER = 8,
		MOB_BATTLESHIP = 10 } mob_ship_models;

enum Habitat {
    AshWorld,		// 0
    Badlands,		// 1
    Desert,		// 2
    Ice,		// 3
    Installation,	// 4
    Jungle,		// 5
    Twilight,		// 6
    Uninhabitable	// 7
};

enum StellarObjectType {
    Star,
    Base,
    Planet,
    Meteor,
    Nebula,
    Derelict,
    Wormhole,
    SpacePlatform,
    Gate,
    Moon
};

enum ShipState {
    Docked,
    Ready,
    Busy,
    Refueling,
    Launching,
    Landing,
    HyperSpace,
    Flying
};

#define MAX_PLANET	100
#define MAX_SHIP	1000
#define MAX_SHIP_ROOMS	25

class Turret;
class Hanger;
class MissileData;


typedef Lexi::List<class ShipData *>	ShipList;
typedef Lexi::List<class StellarObject *>	StellarObjectList;

struct model_type {
    char *	name;
    char *	race;
    int	size;
    int	shipclass;
    Flags	flags;
    int	hyperspeed;
    int	maxspeed;
    int	primdam;
    int	cooldown;
    int	secondam;
    UInt32	mcooldown;
    int	manuever;
    int	energy;
    int	shield;
    int	hull;
    int	price;
    int	armor;
    int	rooms;
};

#define LASER_DAMAGED	-1
#define LASER_READY	0


class StarSystem {
public:
    ShipList				ships;
    Lexi::List<MissileData *>missiles;
    Lexi::List<StellarObject *>stellarObjects;

    char *			name;
    char *			filename;
    Flags			flags;

    void			Update(void);
    bool			CanJump(ShipData *ship);
};


class Turret {
public:
    RoomData *	room;
    ShipData *	target;
    int		laserstate;
};

class Hanger {
public:
    RoomData *	room;
    unsigned short	capacity;
    bool        bayopen;
    bool	CanHold(ShipData *ship);
};

// Modifiable Stats (Modules)
class ShipMods {
public:
    short		armor;
    short		damage;

    short speed;
    short manuever;
    short hyperspeed;

    short shield;
    short reactor;
};

class ShipData : public Entity {
public:
    ShipData(void);
    ~ShipData(void);

    Lexi::List<RoomData *>	rooms;
    Event::List			m_Events;
    Lexi::List<Turret *>turrets;
    Lexi::List<Hanger *>hangers;
    Lexi::List<RoomData *>	garages;
    ShipList			fighters;	// deployed, not built!

    Lexi::List<ObjData *>	modules;
						
	RoomData *			InRoom()					{ return m_InRoom; }
	void				SetInRoom(RoomData * room)	{ m_InRoom = room; }

private:
    RoomData *		m_InRoom;

public:
    RoomData *			pilotseat;
    RoomData *			gunseat;
    RoomData *			viewscreen;
    RoomData *			engine;
    RoomData *			entrance;
    RoomData *			deployment;
    ShipData *			target;
    ShipData *			following;
    ShipData *			mothership;
    StarSystem *		currjump;
    StarSystem *		starsystem;
    char *			filename;
    char *			name;
    char *			home;
    char *			description[MAX_SHIP_ROOMS];
    char *			owner;
    char *			pilot;
    char *			copilot;
    char *			dest;
    int			age;
    int			type;
    int			size;
    int			shipclass;
    int			model;
    int			hyperspeed;
    int			hyperdistance;
    int			maxspeed;
    int			speed;
    int			state;
    int			recall;
    int			primdam;
    int			secondam;
    int			primstate;
    int			secondstate;
    int			manuever;
    int			interceptors;
    unsigned short	armor;
    unsigned short	upgrades[5];
    bool			bayopen;
    bool			hatchopen;
    float			vx, vy, vz;
    float			hx, hy, hz;
    float			jx, jy, jz;
    float			ox, oy, oz;
    int			sensor;
    int			energy;
    int			maxenergy;
    int			shield;
    int			maxshield;
    int			tempshield;
    int			hull;
    int			maxhull;
    RoomData *		location;
    RoomData *		lastdoc;
    RoomData *		shipyard;
    long			collision;
    Flags			flags;
    RNum			clan;

//  Timers
    void			AddTimer(int time, ActionCommandFunction func, int newstate);
    void			ExtractTimer(void);

    Event *			ship_event[6];	// movement, primary, secondary
    Event *			timer_event;
    ActionCommandFunction	timer_func;
    int			substate;
    char *			dest_buf;
    char *			dest_buf_2;

//  Utility functions
    bool			CanSee(ShipData *target);
    bool			CanSee(StellarObject *target);
    bool			IsCloaked(void);

    void			Move(void);
    void			Update(void);

    void			Reset(void);
    void			Appear(void);
    void			Extract(void);
    void			CreateRooms(void);
    void			EventCleanup(void);

    void			SetTarget(ShipData *target);

    void			Land(char *argument);
    void			Launch(void);
    bool			Damage(CharData *ch, int dam, int type);
    void			Destroy(CharData *ch = NULL);
    bool			AutoPilot(void);

    double			Distance(ShipData *target);
    double			Distance(StellarObject *target);
    double			Distance(PlanetData *planet);
    double			JumpDistance(StellarObject *target);
    bool			SafeDistance(void);

    bool			load(FILE *fl);
    void			save(void);
    void			addsave(void);
    void			remsave(void);

    bool			ToRoom(RoomData * room);
    bool			ToRoom(UInt32 vnum);
    void			FromRoom(void);
    void			ToSystem(StarSystem *starsystem);
    void			FromSystem(void);

    int			Skill(void);
    Relation			GetRelation(ShipData *target);
    StellarObject *		NearestObject(void);

//  Comm Funcs
    void			Computer(char *txt, ...);
};

class MissileData {
public:
    StarSystem *	starsystem;
    UInt32		target;
    UInt32		fired_by;
    UInt32		fired_from;
    char *		name;
    int			age;
    int			speed;
    int			damage;
    float		mx, my, mz;

    void		Move(void);
    double		Distance(ShipData *target);
};

class StellarObject {
public:
    StellarObject(StellarObjectType objectType = SpacePlatform);
    ~StellarObject(void);

    char *		name;
    char *		filename;
    VNum		clan;
    VNum		zone;
    float		size;
    StellarObjectType	type;
    Habitat		habitat;
    int		gravity;
    float		x, y, z;
    float		dx, dy, dz;	// destination coords for gates
    StarSystem *	starsystem;

    Flags		flags;
    UInt32		timer;
    Event::List	events;
    StarSystem *	connects;

    void		Update(void);
    void		Extract(void);

protected:
    void		Init(void);
};

void MoveShips(void);
void UpdateSpace(void);

extern ShipList			ship_list;
extern ShipList			purged_ships;
extern StellarObjectList	stellar_objects;

ShipData *ship_from_room(RoomData *room);
ShipData *ship_from_turret(RoomData *room);
ShipData *ship_from_engine(RoomData *room);
ShipData *ship_from_cockpit(RoomData *room);
ShipData *ship_in_room(RoomData *room, char *name);
ShipData *ship_from_entrance(RoomData *room);
ShipData *get_ship_here(char *name, StarSystem *starsystem);

bool autofly(ShipData *ship);
bool check_owner(CharData *ch, ShipData *ship);
bool check_pilot(CharData *ch, ShipData *ship);
void echo_to_room(RoomData *room, char *argument);
void echo_to_ship(Flags type, ShipData *ship, char *argument, ...) __attribute__ ((format (printf, 3, 4)));
void echo_to_system(ShipData *ship, ShipData *ignore, char *argument, ...) __attribute__ ((format (printf, 3, 4)));
void echo_to_starsystem(StarSystem *system, char *argument, ...) __attribute__ ((format (printf, 2, 3)));

void AlterHull(ShipData *ship, int amount);
void AlterShield(ShipData *ship, int amount);
void AlterEnergy(ShipData *ship, int amount);

#endif
