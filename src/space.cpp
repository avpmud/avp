#include <math.h>

#include "utils.h"
#include "db.h"
#include "olc.h"
#include "comm.h"
#include "files.h"
#include "clans.h"
#include "buffer.h"
#include "structs.h"
#include "handler.h"
#include "staffcmds.h"
#include "characters.h"
#include "interpreter.h"
#include "find.h"
#include "spells.h"
#include "event.h"
#include "constants.h"
#include "lexifile.h"


ShipList				ship_list;
ShipList				ship_saves;
ShipList				purged_ships;
Lexi::List<MissileData *>missile_list;
Lexi::List<StarSystem *>		starsystem_list;
StellarObjectList		stellar_objects;

// extern functions
ACMD(do_olddrive);


int ship_gain(ShipData *ship, int type);
void CheckShipRates(ShipData *ship);
void AlterShields(ShipData *ship, int amount);
void move_ships(void);
void recharge_ships(void);
void public_update(void);
void space_update(void);
bool ship_was_in_range(ShipData *ship, ShipData *target);
ShipData *ship_from_pilotseat(RoomData * room);
StarSystem *starsystem_from_room(RoomData * room);
void SaveShips(void);
void save_starsystem(StarSystem *starsystem);
void load_space(void);
RoomData * AssignRoom(ShipData *ship, VNum vnum);
void load_ships(void);
void bridge_doors(RoomData * from, RoomData * to, int dir);
void bridge_rooms(RoomData * from, RoomData * to, int dir);
void make_room(ShipData *ship, RoomData * rnum, int type);
RoomData * make_ship_room(ShipData *ship);
ACMD(do_openhatch);
ACMD(do_boardship);
ACMD(do_land);
ACMD(do_launch);
ACMD(do_gate);
ACMD(do_radar);
ACMD(do_accelerate);
ACMD(do_course);
ACMD(do_shipstat);
ACMD(do_calculate);
ACMD(do_hyperspace);
ACMD(do_scanplanet);
ACMD(do_scanship);
ACMD(do_followship);
ACMD(do_target);
ACMD(do_gen_autoship);
ACMD(do_shipcloak);
ACMD(do_baydoors);
ACMD(do_buyship);
StellarObject *get_stellarobject(char *name);
bool perform_stedit(CharData *ch, StellarObject *object, int mode, char *val_arg);
ACMD(do_orbitalinsertion);
ACMD(do_shieldmatrix);
ACMD(do_disruptionweb);
ACMD(do_wormhole);
ACMD(do_foldspace);
ACMD(do_jumpvector);
ACMD(do_remote);
ACMD(do_drive);
ACMD(do_deploy);
ACMD(do_interceptor);
ACMD(do_ping);
ACMD(do_setship);
ACMD(do_stedit);
ACMD(do_rempilot);
ACMD(do_addpilot);
ACMD(do_tranship);
ACMD(do_shipupgrade);
ACMD(do_repairship);
ACMD(do_shipfire);
ACMD(do_shiprecall);
ACMD(do_shiplist); 
ACMD(do_sylist);
ACMD(do_stlist);
ACMD(do_svlist);

long find_race_bitvector(char arg);
void announce(const char *buf);
bool is_online(char *argument);
void AlterMove(CharData *ch, int amount);
void tag_argument(char *argument, char *tag);
void group_gain(CharData * ch, CharData * victim, int attacktype);
int str_prefix(const char *astr, const char *bstr);
void calc_reward(CharData *ch, CharData *victim, int people, int groupLevel);

// local functions
void		fread_planet				(PlanetData *planet, FILE *fp);
void		write_ship_list				(void);
void		fread_starsystem		(StarSystem *starsystem, FILE *fp);
void		write_starsystem_list		(void);

ShipData *make_interceptor		(ShipData *mothership);
void		remove_interceptors		(ShipData *ship);
void		deploy_interceptors		(ShipData *ship);
void		destroy_interceptors		(ShipData *ship, CharData *killer);

void		new_missile				(ShipData *ship, ShipData *target, CharData *ch);
void		echo_to_room				(RoomData * room, char *argument);
void		extract_missile				(MissileData *missile);
void		space_conrol_update		(void);
void		VehicleMove				(ShipData *vehicle, RoomData * to);
bool		load_ship_file				(char *shipfile);
bool		load_starsystem				(char *starsystemfile);
bool		load_planet_file		(char *planetfile);

void		LoadStellarObjects		(void);
bool		LoadStellarObject		(char *file);
void		fread_stellarObject		(StellarObject *object, FILE *fp);

bool		autofly						(ShipData *ship);
bool		has_comlink				(CharData *ch);
bool		is_facing				(ShipData *ship, ShipData *target);

int		ships_owned				(CharData *ch, bool clanbuy);
int		ship_proto_value		(int type);

StarSystem *starsystem_from_name(char *name);

ShipData *make_ship(int type);
ShipData *make_mob_ship(StellarObject *object, int model);

ShipData *get_ship(char *name);
ShipData *get_ship_here(char *name, StarSystem *starsystem);


#define STYPE_BAYDOORS	1
#define STYPE_CLOAKING	2
#define STYPE_WEBBING	3


char *StellarObjectFlags[] = {
	"HIDDEN",
	"Unused1",
	"Unused2",
	"Unused3",
	"Unused4",
	"Unused5",
	"Unused6",
	"Unused7",
	"Unused8",
	"Unused9",
	"Unused10",
	"Unused11",
	"Unused12",
	"Unused13",
	"Unused14",
	"Unused15",
	"Unused16",
	"Unused17",
	"Unused18",
	"Unused19",
	"Unused20",
	"Unused21",
	"Unused22",
	"Unused23",
	"Unused24",
	"Unused25",
	"Unused26",
	"Unused27",
	"Unused28",
	"Unused29",
	"Unused30",
	"NOSAVE",
	"\n"
};

char *StarSystemFlags[] = {
	"HIDDEN",
	"!JUMP",
	"\n"
};

char *HabitatTypes[] = {
	"Ash World",		// 0
	"Badlands",				// 1
	"Desert",				// 2
	"Ice",				// 3
	"Installation",		// 4
	"Jungle",				// 5
	"Twilight",				// 6
	"Uninhabitable",		// 7
	"\n"
};

char *StellarObjectTypes[] = {
	"Star",
	"Base",
	"Planet",
	"Meteor",
	"Nebula",
	"Derelict",
	"Wormhole",
	"Space Platform",
	"Gate",
	"Moon",
	"\n"
};

const char *ship_states[] = {
	"Docked",
	"Ready",
	"Busy",
	"Busy2",
	"Busy3",
	"Refueling",
	"Launching",
	"Launching2",
	"Landing",
	"Landing2",
	"Hyperspace",
	"Flying",
	"Destroyed",
	"\n"
};

const struct model_type model_list[MAX_SHIP_MODEL+1] = {
//		 name,		racebits, class
//		hsp, rsp, grn, air, mvr, energ, shd, hul, prc, a, rm;
//		FIGHTER_SHIP		SKILL_STARFIGHTER		4000
//		MIDSIZE_SHIP		SKILL_TRANSPORTS		6000
//		LARGE_SHIP		SKILL_FRIGATES				8000
//		CAPITAL_SHIP		SKILL_CAPITALSHIPS		35000

	{ "Corsair",				"pd",		FIGHTER_SHIP,		SPACECRAFT,
		SHIP_WEBBER,
		  1, 255,   5,  1 RL_SEC,   0, 0, 255,  4000,  80, 100, 250, 1,  1
	},

	{ "Wraith",						"t",		FIGHTER_SHIP,		SPACECRAFT,
		SHIP_CLOAKER,
		  1, 150,   8,  3 RL_SEC,  20, 3 RL_SEC, 150,  4000,   0, 120, 250, 0,  1
	},

	{ "Scout",						"pd",		FIGHTER_SHIP,		SPACECRAFT,
		0,
		  1, 150,   8,  3 RL_SEC,  28, 3 RL_SEC, 150,  4000, 100, 150, 450, 0,  1
	},

	{ "Infested Wraith",		"zi",		FIGHTER_SHIP,		SPACECRAFT,
		SHIP_REGEN | SHIP_CLOAKER,
		  1, 150, 8, 3 RL_SEC, 20, 2 RL_SEC, 150, 4000, 0, 120, 250, 0, 1
	},

	{ "Shuttle",				"pd",		MIDSIZE_SHIP,		SPACECRAFT,
		0,
		150,  75,   0,   0,  0,  0,  75,  6000,  60,  80, 200, 1,  5
	},

	{ "Dropship",				"ti",		MIDSIZE_SHIP,		SPACECRAFT,
		0,
		100,  70,   0,   0,   0,  0, 70,  6000,   0, 150, 200, 1,  5
	},

	{ "Overlord",				"z",		MIDSIZE_SHIP,		SPACECRAFT,
		SHIP_REGEN | SHIP_DETECTOR,
		100,  70,   0,   0,  0, 0,  70,  6000,   0, 200, 100, 0,  1
	},

	{ "Valkyrie",				"ti",		LARGE_SHIP,		SPACECRAFT,
		0,
		 50, 100,   0,   0,  6, 1 RL_SEC, 100,  8000,   0, 200, 375, 2,  1
	},

	{ "Arbiter",				"pd",		LARGE_SHIP,		SPACECRAFT,
		SHIP_MASSCLOAK,
		200,  50,  10,   4 RL_SEC,  0,  0,  50,  8000, 150, 200, 450, 1, 10
	},

	{ "Science Vessel",				"t",		LARGE_SHIP,		SPACECRAFT,
		SHIP_DETECTOR | SHIP_IRRADIATOR,
		200,  50,   0,   0,  0,  0,  50,  8000,   0, 200, 325, 1, 8
	},

	{ "Queen",						"z",		LARGE_SHIP,		SPACECRAFT,
		SHIP_REGEN,
		200,  50,   0,   0,  0,  0,  50,  8000,   0, 120, 200, 0, 1
	},

	{ "Guardian",				"z",		LARGE_SHIP,		SPACECRAFT,
		SHIP_REGEN,
		150,  30,  20, 2 RL_SEC,  0,  0,   30, 8000, 0, 150, 350, 2, 1
	},

	{ "Behemoth Battleship",		"zi",		CAPITAL_SHIP,		SPACECRAFT,
		SHIP_REGEN,
		255,  40,  30,   5 RL_SEC, 260, 180 RL_SEC,  40, 40000,  0, 500, 750, 3, 12
	},

	{ "Battlecruiser",				"t",		CAPITAL_SHIP,		SPACECRAFT,
		0,
		250,  35,  25,   4 RL_SEC, 260, 180 RL_SEC,  35, 35000,  0, 500, 700, 3, 24
	},

	{ "Carrier",				"pd",		CAPITAL_SHIP,		SPACECRAFT,
		0,
		250,  35,   0,   0 RL_SEC,   0,   0 RL_SEC,  35, 35000, 100, 300, 600, 4, 24
	},

	{ "Interceptor",				"",		FIGHTER_SHIP,		SPACECRAFT,
		0,
		  0, 175,   6,   2 RL_SEC,   0,   0 RL_SEC, 100,  1000,  40,  40,   1, 0,  1
	},

	{ "Arclite Siege Tank",		"ti",		FIGHTER_SHIP,		LAND_VEHICLE,
		0,
		  0,  10,  30,   6 RL_SEC,  70,   6 RL_SEC,  50,  1000,   0, 100, 250, 1, 1
	},

	{ "Reaver",						"p",		FIGHTER_SHIP,		LAND_VEHICLE,
		0,
		  0,  10, 125,   6 RL_SEC,   0,   6 RL_SEC,  35,  1000,  80, 100, 300, 0, 1
	},

	{ "All Terrain Vehicle",		"tpzi",		FIGHTER_SHIP,		LAND_VEHICLE,
		0,
		  0,  10,   0,   0 RL_SEC,   0,   0 RL_SEC, 100,  1000,   0, 200, 100, 0, 4
	},

	{ "Custom Ship",				"",		CAPITAL_SHIP,		SPACECRAFT,
		0,
		255, 255,  30,   1 RL_SEC, 30, 1 RL_SEC, 255, 35000, 350, 500,   0, 0, 0
	},

	{ NULL,						"",		CAPITAL_SHIP,		SPACECRAFT,
		0,
		  0,  0,   0,   0,   0,   0,	 0,   0,   0,   0, 0,  0, 0
	}
};




class ShipEvent : public Event
{
public:
						ShipEvent(unsigned int when, IDNum ship) :
							Event(when), m_Ship(ship)
						{}
						
protected:
	IDNum				m_Ship;
};


class ShipRegenEvent : public ShipEvent
{
public:
						ShipRegenEvent(unsigned int when, IDNum ship, int type) :
							ShipEvent(when, ship), m_Type(type)
						{}
	
	static const char *	Type;
	virtual const char *GetType() const { return Type; }
//	virtual void		UpdateFrom(const Event *event);

private:
	virtual unsigned int	Run();
	
	int					m_Type;
};


class ShipTimerEvent : public ShipEvent
{
public:
						ShipTimerEvent(unsigned int when, IDNum ship, int type) :
							ShipEvent(when, ship), m_Type(type)
						{}
	
	static const char *	Type;
	virtual const char *GetType() const { return Type; }

	virtual int			GetTimerType() const { return m_Type; }
	
private:
	virtual unsigned int	Run();
	
	int					m_Type;
};


class VehicleCooldownEvent : public ShipEvent
{
public:
						VehicleCooldownEvent(unsigned int when, IDNum ship) :
							ShipEvent(when, ship)
						{}
	
	static const char *	Type;
	virtual const char *GetType() const { return Type; }

private:
	virtual unsigned int	Run();
};


class MissileCooldownEvent : public ShipEvent
{
public:
						MissileCooldownEvent(unsigned int when, IDNum ship) :
							ShipEvent(when, ship)
						{}
	
	static const char *	Type;
	virtual const char *GetType() const { return Type; }

private:
	virtual unsigned int	Run();
};


class DisruptionWebEvent : public ShipEvent
{
public:
						DisruptionWebEvent(unsigned int when, IDNum ship) :
							ShipEvent(when, ship)
						{}
	
	static const char *	Type;
	virtual const char *GetType() const { return Type; }

private:
	virtual unsigned int	Run();
};


class ShieldMatrixEvent : public ShipEvent
{
public:
						ShieldMatrixEvent(unsigned int when, IDNum ship) :
							ShipEvent(when, ship)
						{}
	
	static const char *	Type;
	virtual const char *GetType() const { return Type; }

private:
	virtual unsigned int	Run();
};


class WormholeCollapseEvent : public Event
{
public:
						WormholeCollapseEvent(unsigned int when, StellarObject *object) :
							Event(when), m_Object(object)
						{}
	
	static const char *	Type;
	virtual const char *GetType() const { return Type; }

private:
	virtual unsigned int	Run();
	
	StellarObject *		m_Object;
};


const char *ShipRegenEvent::Type = "ShipRegen";
const char *ShipTimerEvent::Type = "ShipTimer";
const char *VehicleCooldownEvent::Type = "VehicleCooldown";
const char *MissileCooldownEvent::Type = "MissileCooldown";
const char *DisruptionWebEvent::Type = "DisruptionWeb";
const char *ShieldMatrixEvent::Type = "ShieldMatrix";
const char *WormholeCollapseEvent::Type = "WormholeCollapse";



bool Hanger::CanHold(ShipData *ship) {
	int count;
	count = ship->size + 1;
	FOREACH(ShipList, this->room->ships, tship)
		if (ship != *tship)		// just in case..
			count += (*tship)->size + 1;

	if (this->capacity >= count)
		return true;

	return false;
}

bool ShipData::SafeDistance(void) {
	bool result = true;

	if (this->starsystem) {
		FOREACH(ShipList, this->starsystem->ships, iter)
		{
			ShipData *target = *iter;
			if (target == this)		continue;
			if (target->target == this && target->Distance(this) < 500)
				result = false;
		}
	}
	return result;
}

double ShipData::Distance(ShipData *target) {
	float x = this->vx - target->vx,
		y = this->vy - target->vy, z = this->vz - target->vz;
	return sqrt(x*x + y*y + z*z);
}

double ShipData::Distance(StellarObject *target) {
	float x = this->vx - target->x,
		y = this->vy - target->y, z = this->vz - target->z;
	return sqrt(x*x + y*y + z*z);
}

double ShipData::JumpDistance(StellarObject *target) {
	float x = this->jx - target->x,
		y = this->jy - target->y, z = this->jz - target->z;
	return sqrt(x*x + y*y + z*z);
}

int ShipData::Skill(void) {
/*
	switch (this->size) {
		case FIGHTER_SHIP:		return SKILL_STARFIGHTERS;
		case MIDSIZE_SHIP:		return SKILL_TRANSPORTS;
		case LARGE_SHIP:		return SKILL_FRIGATES;
		case CAPITAL_SHIP:
		case SHIP_PLATFORM:
		default:
			break;
	}
	return SKILL_CAPITALSHIPS;
*/
	return 0;
}

Relation ShipData::GetRelation(ShipData *target)
{
	Clan * chClan, *tarClan;
	int friends = 0, enemies = 0;

	chClan = Clan::GetClan(this->clan);
	tarClan = Clan::GetClan(target->clan);

	if (chClan && tarClan && Clan::GetRelation(chClan->GetRealNumber(), tarClan->GetRealNumber()) == RELATION_ENEMY)
		return RELATION_ENEMY;

	if (chClan)
	{
		FOREACH(Lexi::List<RoomData *>, target->rooms, room)
		{
			FOREACH(CharacterList, (*room)->people, iter)
			{
				CharData *rch = *iter;
				if (!IS_NPC(rch) && GET_CLAN(rch))
				{
					if (Clan::GetRelation(chClan->GetRealNumber(), GET_CLAN(rch)->GetRealNumber()) == RELATION_ENEMY)
						enemies++;
					else if (chClan == tarClan)
						friends++;
				}
			}
		}
	}

	if (enemies > friends)
		return RELATION_ENEMY;

	if (str_cmp(this->owner, target->owner))
		return RELATION_NEUTRAL;

	return RELATION_FRIEND;		// Friendly
}

void StarSystem::Update(void)
{
	FOREACH(ShipList, this->ships, ship)
		(*ship)->Update();
}

bool StarSystem::CanJump(ShipData *ship) {
	if (ship->starsystem == this)
		return true;

	return !STR_FLAGGED(this, STAR_NOJUMP);
}

StellarObject::StellarObject(StellarObjectType objectType) {
	this->Init();

	this->clan = -1;
	this->zone = -1;
	this->type = objectType;
	this->habitat = Installation;
}

ShipData::ShipData(void)
{
    this->deployment = NULL;
    this->mothership = NULL;
    this->currjump = NULL;
    this->filename = NULL;
    this->name = NULL;
    this->home = NULL;
    for (int i = 0; i < MAX_SHIP_ROOMS; ++i)
    	this->description[i] = NULL;
    this->owner = NULL;
    this->pilot = NULL;
    this->copilot = NULL;
    this->dest = NULL;
    
    this->age = 0;
    this->type = 0;
    this->size = 0;
    this->shipclass = 0;
    this->model = 0;
    this->hyperspeed = 0;
    this->hyperdistance = 0;
    this->maxspeed = 0;
    this->speed = 0;
    this->primdam = 0;
    this->secondam = 0;
    this->primstate = 0;
    this->secondstate = 0;
    this->manuever = 0;
    this->interceptors = 0;
    this->armor = 0;
    for (int i = 0; i < 5; ++i)
    	this->upgrades[i] = 0;
    this->bayopen = false;
    this->hatchopen = false;
    this->vx = this->vy = this->vz = 0;
    this->hx = this->hy = this->hz = 0;
    this->jx = this->jy = this->jz = 0;
    this->ox = this->oy = this->oz = 0;
    this->sensor = 0;
    this->energy = 0;
    this->maxenergy = 0;
    this->shield = 0;
    this->maxshield = 0;
    this->tempshield = 0;
    this->hull = 0;
    this->maxhull = 0;
    this->location = NULL;
    this->lastdoc = NULL;
    this->shipyard = NULL;
    this->collision = 0;
    this->flags = 0;
    this->clan = -1;

    for (int i = 0; i < 6; ++i)
    	this->ship_event[i] = NULL;	// movement, primary, secondary
    this->timer_event = NULL;
    this->timer_func = NULL;
    this->substate = 0;
    this->dest_buf = NULL;
    this->dest_buf_2 = NULL;

	m_InRoom = NULL;

	this->state				= SHIP_DOCKED;
	this->target		= NULL;
	this->following		= NULL;
	this->starsystem		= NULL;

	this->recall		= -1;		// need this

	this->engine		= NULL;
	this->gunseat		= NULL;
	this->entrance		= NULL;
	this->pilotseat		= NULL;
	this->viewscreen		= NULL;
}

StellarObject::~StellarObject(void) {
	if (this->name)					free(this->name);
	if (this->filename)				free(this->filename);
}

ShipData::~ShipData(void) {
	if (this->name)				free(this->name);
	if (this->home)				free(this->home);
	if (this->dest)				free(this->dest);
	if (this->owner)				free(this->owner);
	if (this->pilot)				free(this->pilot);
	if (this->copilot)				free(this->copilot);
	if (this->filename)				free(this->filename);

	for (int i = 0; i < MAX_SHIP_ROOMS; i++) {
		if (this->description[i]) {
			free(this->description[i]);
			this->description[i] = NULL;
		}
	}

	if (this->dest_buf)				FREE(this->dest_buf);
	if (this->dest_buf_2)		FREE(this->dest_buf_2);
}

void StellarObject::Init(void) {
	memset(this, 0, sizeof(*this));

	this->connects = NULL;
	this->starsystem = NULL;
}

void StellarObject::Extract(void)
{
	if (this->starsystem) {
		this->starsystem->stellarObjects.remove(this);
		this->starsystem = NULL;
	}

	while (!this->events.empty())
	{
		Event *event = this->events.front();
		this->events.pop_front();
		event->Cancel();
	}

	stellar_objects.remove(this);
	delete this;
}

void ShipData::EventCleanup(void) {
	int i;
	
	while (!m_Events.empty())
	{
		Event *event = m_Events.front();
		m_Events.pop_front();
		event->Cancel();
	}

	for (i = 0; i < 6; i++) {
		if (this->ship_event[i]) {
			this->ship_event[i]->Cancel();
			this->ship_event[i] = NULL;
		}
	}
	ExtractTimer();
}

void ShipData::Extract(void) {
	if (IsPurged())
		return;

	if (IN_ROOM(this))		this->FromRoom();
	if (this->starsystem)	this->FromSystem();

	this->remsave();
	this->EventCleanup();

	ship_list.remove(this);
	purged_ships.push_back(this);  
	SetPurged();
}

void ShipData::ExtractTimer(void) {
	if (this->timer_event) {
		this->timer_event->Cancel();
		this->timer_event = NULL;
	}
	this->timer_func = NULL;

	if (this->dest_buf)				FREE(this->dest_buf);
	if (this->dest_buf_2)		FREE(this->dest_buf_2);
}

StellarObject *ShipData::NearestObject(void)
{
	int count = 0;
	double dist, nearDist;
	StellarObject *object = NULL;

	if (!this->starsystem)
		return NULL;

	FOREACH(StellarObjectList, this->starsystem->stellarObjects, iter)
	{
		StellarObject *locObject = *iter;
		
		if (count++ == 0)
			nearDist = this->Distance(locObject) + 1;

		if ((dist = this->Distance(locObject)) < nearDist) {
			nearDist = dist;
			object = locObject;
		}
	}
	return object;
}

void ShipData::SetTarget(ShipData *target) {
	FOREACH(ShipList, this->fighters, fighter)
		(*fighter)->target = target;

	FOREACH(Lexi::List<Turret *>, this->turrets, turret)
	{
		if ((*turret)->target && this->target == (*turret)->target)
			(*turret)->target = target;		// why not?
	}
	this->target = target;
}

bool ShipData::IsCloaked(void)
{
	if (SHP_FLAGGED(this, SHIP_CLOAKED))
		return true;		// yup

	if (SHP_FLAGGED(this, SHIP_MASSCLOAK))
		return false;		// haha arbiters can't cloak other arbiters

	FOREACH(ShipList, this->starsystem->ships, iter)
	{
		ShipData *area = *iter;
		// if for some reason the mass-cloaker is cloaked, it doesn't
		// get the benefit of cloaking nearby friendlies...
		if (area == this || SHP_FLAGGED(area, SHIP_CLOAKED))
			continue;

		if (!SHP_FLAGGED(area, SHIP_MASSCLOAK))
			continue;		// don't bother with nons

		if (area->clan != this->clan || str_cmp(this->owner, area->owner))
			continue;

		if (this->Distance(area) <= 1000)		// within range, cloak it!
			return true;
	}
	return false;
}

bool ShipData::CanSee(ShipData *target)
{
	if (!this || !target)
		return false;

	if (this == target)
		return true;

	FOREACH(ShipList, this->starsystem->ships, iter)
	{
		ShipData *area = *iter;
		
		if (area == this || area == target || area->IsCloaked())
			continue;

		if (!SHP_FLAGGED(area, SHIP_DETECTOR))		// don't waste time with nons
			continue;

		if (this->GetRelation(area) != RELATION_FRIEND)
			continue;

		// We want 'area' to be the center point and illuminate all 'targets'
		// within 1000au of itself to all 'friendlies' within 1000au of itself
		// A.K.A spotlight affect
		if (this->Distance(area) <= 1000 && area->Distance(target) <= 1000)
			return true;
	}

	if (target->IsCloaked()) {
		if (SHP_FLAGGED(this, SHIP_DETECTOR) && this->Distance(target) <= 1000)
			return true;
		else
			return false;
	}
	return true;
}

bool ShipData::CanSee(StellarObject *target) {
	if (STO_FLAGGED(target, OBJECT_HIDDEN) && this->Distance(target) > 4000)
		return false;

	return true;
}

char *ground_weap_pers[] = {
	"neutron flares",				// corsair
	"burst lasers",				// wraith
	"photon blasters",				// scout
	"burst lasers",				// infested wraith
	"lasers",						// shuttle
	"lasers",						// dropship
	"lasers",						// overlord
	"lasers",						// valkyrie
	"disruptor cannon",				// arbiter
	"lasers",						// science vessel
	"lasers",						// queen
	"plasma spore",				// guardian
	"laser batteries",				// behemoth
	"laser batteries",				// battlecruiser
	"lasers",						// carrier
	"photon blasters",				// interceptor
	"twin 80mm cannons",		// siege tank
	"scarab",						// reaver
	"lasers",						// atv
	"lasers",						// custom
	"\n"
};

char *ground_weap_othr[] = {
	"A neutron flare",						// corsair
	"Burst laserfire",						// wraith
	"A photon blaster",						// scout
	"Burst laserfire",						// inf wraith
	"Laserfire",						// shuttle
	"Laserfire",						// dropship
	"Laserfire",						// overlord
	"Laserfire",						// valkyrie
	"A phase disruptor cannon",				// arbiter
	"Laserfire",						// science vessel
	"Laserfire",						// queen
	"A plasma spore",						// guardian
	"A laser battery",						// inf battleship
	"A laser battery",						// battlecruiser
	"Laserfire",						// carrier
	"A photon blaster",						// interceptor
	"A twin 80mm cannon",				// siege tank
	"A scarab",								// reaver
	"Laserfire",						// atv
	"Laserfire",						// custom
	"\n"
};

char *ground_weap[] = {
	"Neutron Flare",				// corsair
	"25mm Burst Laser",				// wraith
	"Dual Photon Blasters",		// scout
	"25mm Burst Laser",				// infested wraith
	"None",						// shuttle
	"None",						// dropship
	"None",						// overlord
	"None",						// valkyrie
	"Phase Disruptor Cannon",		// arbiter
	"None",						// science vessel
	"None",						// queen
	"Plasma Spores",				// guardian
	"Laser Batteries",				// behemoth battlecruiser
	"Laser Batteries",				// battlecruiser
	"None",						// carrier
	"Photon Blasters",				// interceptor
	"Twin 80mm Cannon",				// siege tank
	"Scarabs",						// reaver
	"None",						// atv
	"Lasers",						// custom
	"\n"
};

char *air_weap[] = {
	"None",						// corsair
	"Gemini Missile",				// wraith
	"Anti-Matter Missile",		// scout
	"Infested Gemini Missile",		// infested wraith
	"None",						// shuttle
	"None",						// dropship
	"None",						// overlord
	"H.A.L.O Cluster Rocket",		// valkyrie
	"None",						// arbiter
	"None",						// science vessel
	"None",						// queen
	"None",						// guardian
	"Yamato Missile",				// behemoth battlecruiser
	"Yamato Missile",				// battlecruiser
	"None",						// carrier
	"None",						// interceptor
	"120mm Shock Cannon",		// siege tank
	"None",						// reaver
	"None",						// atv
	"Missile",						// custom
	"\n"
};

void ShipData::Appear(void) {
	Flags oldBits = SHP_FLAGS(this) & (SHIP_CLOAKED);

	REMOVE_BIT(SHP_FLAGS(this), SHIP_CLOAKED);
	if (oldBits & SHIP_CLOAKED) {
		echo_to_ship(TO_COCKPIT, this, "`YThe ship becomes visible.`n");
		echo_to_system(this, NULL, "Space shimmers as %s slowly appears.", this->name);
	}
}

#define REGEN_HULL				0
#define REGEN_SHIELDS				1
#define REGEN_ENERGY				2
#define MOVE_SHIP				3
#define PRIMARY_COOLDOWN		4
#define MISSILE_COOLDOWN		5

#define ROOM_SECT(room)				(room->sector_type)

int ship_gain(ShipData *ship, int type) {
	float gain;

	switch (type) {
		case REGEN_HULL:
			gain = 1500 / ((5 - ship->size) * 19.5);

			if (SHP_FLAGGED(ship, SHIP_DISABLED))
				gain *= 3.5;
			break;
		case REGEN_SHIELDS:
			gain = 1500 / ((5 - ship->size) * 19.5);

			if (SHP_FLAGGED(ship, SHIP_DISABLED))
				gain *= 3.5;
			break;
		case REGEN_ENERGY:
			gain = 1000 / ((4 - ship->size) * 19.5);

			if (SHP_FLAGGED(ship, SHIP_DISABLED))		// drains quick!
				gain /= 3.5;
			break;
	}

	switch (ship->state) {
		case SHIP_DOCKED:		gain /= 3;		break;
		case SHIP_READY:		gain /= 1.5;		break;
		case SHIP_HYPERSPACE:		gain *= 1.5;		break;
		default:								break;
	}

	if (SHP_FLAGGED(ship, SHIP_CLOAKED))
		gain *= 1.5;		// fuck ship_ready

	if (SHP_FLAGGED(ship, SHIP_EXOTIC))
		gain /= 5;

	return MAX(1, (int)gain);
}


unsigned int ShipRegenEvent::Run()
{
	int gain;
	ShipData *ship = find_ship(m_Ship);

	if (!ship || ship->IsPurged())
		return 0;		// fucked

	switch (m_Type) {
		case REGEN_HULL:
			if (!SHP_FLAGGED(ship, SHIP_REGEN))
				break;

			ship->hull = MIN(ship->hull + 1, ship->maxhull);

			if (ship->hull < ship->maxhull)
				return ((gain = ship_gain(ship, REGEN_HULL)) ? gain : 1);
			break;
		case REGEN_SHIELDS:
			ship->shield = MIN(ship->shield + 1, ship->maxshield);

			if (ship->shield < ship->maxshield)
				return ((gain = ship_gain(ship, REGEN_SHIELDS)) ? gain : 1);
			break;
		case REGEN_ENERGY:
			if (SHP_FLAGGED(ship, SHIP_DISABLED))		// drains reserve
				ship->energy = URANGE(0, ship->energy - (int)((18.3 + (4 - ship->size)) * 6.2), ship->maxenergy);
			else
				ship->energy = MIN(ship->energy + 1, ship->maxenergy);

			if (ship->energy < ship->maxenergy)
				return ((gain = ship_gain(ship, REGEN_ENERGY)) ? gain : 1);
			break;
		case MOVE_SHIP:
			if (SHP_FLAGGED(ship, SHIP_DISABLED))
				break;
			break;
		case PRIMARY_COOLDOWN:
			break;
		case MISSILE_COOLDOWN:
			break;
		default:
			log("SYSERR:  Unknown ship_event type %d", m_Type);
	}
	ship->ship_event[m_Type] = NULL;
	return 0;
}

void CheckShipRates(ShipData *ship) {
	int gain, time, type;

	for (type = REGEN_HULL; type <= MISSILE_COOLDOWN; type++) {
		switch (type) {
			case REGEN_HULL:
				if (ship->hull >= ship->maxhull)		continue;
				break;
			case REGEN_SHIELDS:
				if (ship->shield >= ship->maxshield)		continue;
				break;
			case REGEN_ENERGY:
				if (ship->speed)						continue;
				if (ship->energy >= ship->maxenergy)		continue;
				break;
			default:
				continue;
		}
		time = ((gain = ship_gain(ship, type)) ? gain : 1);

		if (!ship->ship_event[type] || (time < ship->ship_event[type]->GetTimeRemaining())) {
			if (ship->ship_event[type])
				ship->ship_event[type]->Cancel();

			ship->ship_event[type] = new ShipRegenEvent(time, ship->GetID(), type);
		}
	}
}

void AlterHull(ShipData *ship, int amount) {
	int time, gain;

	ship->hull = MIN(ship->hull - amount, ship->maxhull);

	if (Number(0, 100) == 0 && ship->secondstate != MISSILE_DAMAGED) {
		echo_to_ship(TO_GUNSEAT, ship, "`r`fMissile Launcher DAMAGED!`n");
		ship->secondstate = MISSILE_DAMAGED;
	}

	if (Number(0, 100) == 0 && ship->primstate != LASER_DAMAGED) {
		echo_to_ship(TO_GUNSEAT, ship, "`r`fPrimary weapon DAMAGED!`n");
		ship->primstate = LASER_DAMAGED;
	}

	if (Number(0, 100) == 0 && !SHP_FLAGGED(ship, SHIP_DISABLED)) {
		if (ship->size >= LARGE_SHIP && Number(0, 1))
			return;

		echo_to_ship(TO_COCKPIT, ship, "`r`fShip Drive DAMAGED!`n");
		echo_to_system(ship, NULL, "`y%s is disabled and begins to drift.`n", ship->name);
		SET_BIT(SHP_FLAGS(ship), SHIP_DISABLED);
	}

	if (SHP_FLAGGED(ship, SHIP_DISABLED))
		return;		// disabled wont regen?

	if (ship->hull < ship->maxhull && !ship->ship_event[REGEN_HULL]) {
		time = ((gain = ship_gain(ship, REGEN_HULL)) ? gain : 1);
		ship->ship_event[REGEN_HULL] = new ShipRegenEvent(time, ship->GetID(), REGEN_HULL);
	}
}

void AlterShields(ShipData *ship, int amount) {
	int time, gain;

	ship->shield = URANGE(0, ship->shield - amount, ship->maxshield);

	if (ship->shield < 1)
		echo_to_ship(TO_GUNSEAT, ship, "`rShields down...`n");

	if (ship->shield < ship->maxshield && !ship->ship_event[REGEN_SHIELDS]) {
		time = ((gain = ship_gain(ship, REGEN_SHIELDS)) ? gain : 1);
		ship->ship_event[REGEN_SHIELDS] = new ShipRegenEvent(time, ship->GetID(), REGEN_SHIELDS);
	}
}

void AlterEnergy(ShipData *ship, int amount) {
	int time, gain;

	ship->energy = MIN(ship->energy - amount, ship->maxenergy);

	if (ship->energy < 200 && SHP_FLAGGED(ship, SHIP_CLOAKED))
		ship->Appear();

	if (ship->energy < ship->maxenergy && !ship->ship_event[REGEN_ENERGY]) {
		time = ((gain = ship_gain(ship, REGEN_ENERGY)) ? gain : 1);
		ship->ship_event[REGEN_ENERGY] = new ShipRegenEvent(time, ship->GetID(), REGEN_ENERGY);
	}
}

void VehicleMove(ShipData *vehicle, RoomData * to) {
	vehicle->addsave();
	vehicle->FromRoom();
	vehicle->ToRoom(to);
}

void move_ships(void) {
	bool ch_found = FALSE;
	float dx, dy, dz, change;

	FOREACH(Lexi::List<MissileData *>, missile_list, missile)
		(*missile)->Move();		// ouch missiles move before ships!

	FOREACH(ShipList, ship_list, iter)
	{
		ShipData *ship = *iter;
		
		if (!ship->starsystem)
			continue;

		if (SHP_FLAGGED(ship, SHIP_DISABLED))
			ship->speed = 0;		// no engine, stop already

		if (ship->speed > 0) {
			change = sqrt(ship->hx * ship->hx + ship->hy * ship->hy + ship->hz * ship->hz);

			if (change > 0) {
				dx = ship->hx / change;
				dy = ship->hy / change;
				dz = ship->hz / change;
				ship->vx += (dx * ship->speed / 5);
				ship->vy += (dy * ship->speed / 5);
				ship->vz += (dz * ship->speed / 5);
			}
		}

		if (autofly(ship))
			continue;

		// gravity be suckin'
		FOREACH(StellarObjectList, ship->starsystem->stellarObjects, iter)
		{
			StellarObject *object = *iter;
			
			if (ship->vx != object->x)
				ship->vx -= URANGE(-3, object->gravity / (ship->vx - object->x) / 2, 3);
			if (ship->vy != object->y)
				ship->vy -= URANGE(-3, object->gravity / (ship->vy - object->y) / 2, 3);
			if (ship->vz != object->z)
				ship->vz -= URANGE(-3, object->gravity / (ship->vz - object->z) / 2, 3);

			if (object->type == Star && ship->Distance(object) < 10) {
				echo_to_ship(TO_COCKPIT, ship, "`R`fYou fly directly into the sun.`n");
				echo_to_system(ship, NULL, "`y%s flies directly into %s!`n", ship->name, object->name);
				ship->Destroy();
				return;
			}

			if (ship->speed > 0 && object->type == Wormhole && ship->Distance(object) < 10) {
				if (object->connects == NULL)
					continue;

				echo_to_ship(TO_COCKPIT, ship, "`YYou fly into the center of the %s.`n", object->name);
				echo_to_system(ship, NULL, "`Y%s flies into the %s.`n", ship->name, object->name);
				ship->FromSystem();
				ship->ToSystem(object->connects);
				ship->vx += Number(-100, 100) + 5;
				ship->vy += Number(-100, 100) + 5; // scoot it away from hole
				ship->vz += Number(-100, 100) + 5; // add five incase of zeroes
				echo_to_system(ship, NULL, "`Y%s flies out of the %s.`n", ship->name, object->name);
				break;
			}

			// shouldn't have to check for != Wormhole, above should catch it
			if (ship->speed > 0 && object->type != Star && ship->Distance(object) < 10) {
				echo_to_ship(TO_COCKPIT, ship, "`YYou begin orbiting %s.`n", object->name);
				echo_to_system(ship, NULL, "`y%s begins orbiting %s.`n", ship->name, object->name);
				ship->speed = 0;
			}
		}

		if (ship->collision) {
			echo_to_ship(TO_COCKPIT, ship, "`W`fYou have collided with another ship!`n");
			if (ship->Damage(NULL, ship->collision, DAM_COLLISION))
				echo_to_ship(TO_SHIP, ship, "`RA loud explosion shakes the ship violently!`n");
			ship->collision = 0;
		}
	}
}

void recharge_ships(void) {
	VehicleCooldownEvent *event;

	FOREACH(ShipList, ship_list, iter)
	{
		ShipData *ship = *iter;
		
		FOREACH(Lexi::List<Turret *>, ship->turrets, iter)
		{
			Turret *turret = *iter;
			
			if (turret->target && !ship->CanSee(turret->target))
				turret->target = NULL;

			if (turret->laserstate > 0) {
				AlterEnergy(ship, turret->laserstate * 5);
				turret->laserstate = 0;
			}
		}

		if (ship->primstate > 0) {
			AlterEnergy(ship, ship->primstate * 5);
			ship->primstate = 0;
		}

		if (autofly(ship) && ship->starsystem) {
			if (ship->target)
				deploy_interceptors(ship);

			if (ship->target && ship->primstate != LASER_DAMAGED) {
				float chance = 100;
				ShipData *target = ship->target;

				if (ship->state != SHIP_HYPERSPACE && ship->energy > 25
				&& ship->starsystem == target->starsystem
				&& ship->Distance(target) <= 500 && ship->primdam > 0
				&& !(event = (VehicleCooldownEvent *)Event::FindEvent(ship->m_Events, VehicleCooldownEvent::Type))
				&& (ship->shipclass == SPACE_STATION || is_facing(ship, target))) {
					chance += target->model * 10;
					chance -= target->manuever / 10;
					chance -= target->speed / 20;
					chance -= (flabs(target->vx - ship->vx) / 70);
					chance -= (flabs(target->vy - ship->vy) / 70);
					chance -= (flabs(target->vz - ship->vz) / 70);
					chance = URANGE(10, chance, 90);

					char *buf1 = get_buffer(MAX_INPUT_LENGTH);
					char *buf2 = get_buffer(MAX_INPUT_LENGTH);
					sprintf(buf1, "%s", ground_weap_pers[ship->model]);
					sprintf(buf2, "%s", ground_weap_othr[ship->model]);

					if (Number(0, 100) > chance) {
						echo_to_ship(TO_COCKPIT, target, "`y%s fires at you but misses.`n", ship->name);
						echo_to_system(target, ship, "`y%s from %s barely misses %s.`n", buf2, ship->name, target->name);
					} else {
 						echo_to_system(target, ship, "`y%s from %s hits %s.`n", buf2, ship->name, target->name);
						echo_to_ship(TO_COCKPIT, ship, "`yYour %s hits %s.`n", buf1, target->name);
						echo_to_ship(TO_COCKPIT, target, "`rYou are hit by %s from %s!`n", buf1, ship->name);
						echo_to_ship(TO_SHIP, target, "`RA small explosion vibrates through the ship.`n");
						if (autofly(target) && !target->target) {
								target->SetTarget(ship);
								deploy_interceptors(target);
						}
						target->Damage(NULL, ship->primdam + ship->upgrades[UPGRADE_GROUND], DAM_LASER);
					}
					ship->primstate++;
					ship->m_Events.push_front(new VehicleCooldownEvent(model_list[ship->model].cooldown, ship->GetID()));
					release_buffer(buf1);
					release_buffer(buf2);
				}
			}
		}
	}
}

void public_update(void) {
	char *buf;
	StellarObject *planet;

	FOREACH(ShipList, ship_list, iter)
	{
		ShipData *ship = *iter;
		
		if (str_cmp("Public", ship->owner) || ship->shipclass >= SPACE_STATION)
			continue;

		if (ship->starsystem && ship->recall == -1 && ship->age >= 5)
		{
			bool found = false;

			FOREACH(Lexi::List<RoomData *>, ship->rooms, room)
			{
				if (!(*room)->people.empty())
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				if ((planet = ship->shipyard->zone->planet) == NULL)
					continue;
				if (planet->starsystem == NULL)
					continue;

				buf = get_buffer(MAX_STRING_LENGTH);

				one_argument(planet->name, buf);
				sprintf(buf, "%s %s", buf, ship->shipyard->GetName());

				ship->jx		= planet->x + Number(-10, 10);
				ship->jy		= planet->y + Number(-10, 10);
				ship->jz		= planet->z + Number(-10, 10);
				ship->dest		= str_dup(buf);

				ship->hull		= ship->maxhull;
				ship->shield		= ship->maxshield;
				ship->energy		= ship->maxenergy;
				ship->currjump		= planet->starsystem;
				ship->speed		= ship->maxspeed;

				ship->hyperdistance		= Number(400, 600);
				ship->age				= 0;
				ship->state				= SHIP_READY;
				ship->primstate				= LASER_READY;
				ship->secondstate		= MISSILE_READY;
				ship->recall				= SHIP_HYPERSPACE;
				SET_BIT(SHP_FLAGS(ship), SHIP_AUTOPILOT);
				REMOVE_BIT(SHP_FLAGS(ship), SHIP_DISABLED | SHIP_SABOTAGE);
				release_buffer(buf);
				continue;
			}
		}

		if (IN_ROOM(ship) && ship->location == ship->shipyard)
			continue;

		if (ship->age >= 5) {
			if ((planet = ship->shipyard->zone->planet) == NULL)
				continue;
			if (planet->starsystem == NULL)
				continue;
			if (ship->state != SHIP_DOCKED)
				continue;

			buf = get_buffer(MAX_STRING_LENGTH);

			if (ship->hatchopen) {
				ship->hatchopen = FALSE;
				sprintf(buf, "`YThe hatch on %s closes.`n", ship->name);
				echo_to_room(IN_ROOM(ship), buf);
				echo_to_room(ship->entrance, "`YThe hatch slides shut.`n");
			}
			one_argument(planet->name, buf);
			sprintf(buf, "%s %s", buf, ship->shipyard->GetName());

			ship->jx				= planet->x + Number(-10, 10);
			ship->jy				= planet->y + Number(-10, 10);
			ship->jz				= planet->z + Number(-10, 10);
			ship->dest				= str_dup(buf);

			ship->hull				= ship->maxhull;
			ship->shield		= ship->maxshield;
			ship->energy		= ship->maxenergy;
			ship->currjump		= planet->starsystem;
			ship->speed				= ship->maxspeed;

			ship->hyperdistance = Number(450, 600);

			ship->age				= 0;
			ship->state		= SHIP_LAUNCH;
			ship->primstate		= LASER_READY;
			ship->secondstate		= MISSILE_READY;
			ship->recall		= SHIP_LAUNCH;
			SET_BIT(SHP_FLAGS(ship), SHIP_AUTOPILOT);
			REMOVE_BIT(SHP_FLAGS(ship), SHIP_DISABLED);
			echo_to_ship(TO_COCKPIT, ship, "`GLaunch sequence initiated.`n");
			echo_to_ship(TO_COCKPIT, ship, "`GAutopilot engaged, returning home.`n");
			echo_to_ship(TO_SHIP, ship, "`YThe ship hums as it lifts off the ground.`n");
			sprintf(buf, "`Y%s begins to launch.`n", ship->name);
			echo_to_room(IN_ROOM(ship), buf);
			release_buffer(buf);
			continue;
		}
		ship->age++;
	}
}

void MoveShips(void)
{
	FOREACH(ShipList, ship_list, ship)
		(*ship)->Move();

	FOREACH(Lexi::List<MissileData *>, missile_list, missile)
		(*missile)->Move();
}

void UpdateSpace(void)
{
	FOREACH(ShipList, ship_list, ship)
		(*ship)->Update();

	FOREACH(StellarObjectList, stellar_objects, object)
		(*object)->Update();
}

void space_update(void)
{
	FOREACH(StellarObjectList, stellar_objects, object)
		(*object)->Update();

	FOREACH(ShipList, ship_list, iter)
	{
		ShipData *ship = *iter;
		
		if (ship->state == SHIP_DESTROY) {
			ship->Destroy(NULL);
			continue;
		}

		if (ship->state == SHIP_HYPERSPACE) {
			ship->hyperdistance -= ship->hyperspeed * 2;

			if (ship->hyperdistance <= 0) {
				ship->ToSystem(ship->currjump);
				if (ship->starsystem == NULL)
					echo_to_ship(TO_COCKPIT, ship, "`RShip lost in hyperspace. Make new calculations.`n");
				else {
					echo_to_ship(TO_PILOTSEAT, ship, "`YHyperjump complete.`n");
					echo_to_ship(TO_SHIP | TO_HANGERS, ship, "`YThe ship lurches slightly as it comes out of hyperspace.`n");
					echo_to_system(ship, NULL, "`Y%s enters the starsystem at %.0f %.0f %.0f`n", ship->name, ship->vx, ship->vy, ship->vz);
					ship->state = SHIP_READY;
					FREE(ship->home);
					ship->home = str_dup(ship->starsystem->name);
				}
			} else
				echo_to_ship(TO_PILOTSEAT, ship, "`YRemaining jump distance: `W%d`n", ship->hyperdistance);
		}

		if (ship->state == SHIP_BUSY_3) {
			echo_to_ship(TO_PILOTSEAT, ship, "`YManuever complete.`n");
			ship->state = SHIP_READY;
		}
		if (ship->state == SHIP_BUSY_2)
			ship->state = SHIP_BUSY_3;
		if (ship->state == SHIP_BUSY)
			ship->state = SHIP_BUSY_2;

		if (ship->state == SHIP_LAND_2)
			ship->Land(ship->dest);
		if (ship->state == SHIP_LAND)
			ship->state = SHIP_LAND_2;

		if (ship->state == SHIP_LAUNCH_2)
			ship->Launch();
		if (ship->state == SHIP_LAUNCH)
			ship->state = SHIP_LAUNCH_2;


		if (ship->starsystem && ship->speed > 0)
			echo_to_ship(TO_PILOTSEAT, ship, "`BSpeed: `C%d  `BCoords: `C%.0f %.0f %.0f`n%s", ship->speed, ship->vx, ship->vy, ship->vz, ship->IsCloaked() ? "  `B(`Wcloaked`B)`n" : "");

		if (ship->following && (!ship->starsystem || !ship->CanSee(ship->following)
		|| ship->starsystem != ship->following->starsystem))
			ship->following = NULL;

		if (ship->target) {
			if (!ship->starsystem || !ship->CanSee(ship->target)
			|| ship->starsystem != ship->target->starsystem) {
				ship->SetTarget(NULL);
				remove_interceptors(ship);
			} else {
				echo_to_ship(TO_COCKPIT, ship, "`BTarget: `C%s %s%d  `BCoords: `C%.0f %.0f %.0f (%0.2fk)  `BSpeed: `C%d  `BHull: `C%d`n",
					ship->target->shipclass == SPACE_STATION ?
					"Space Station" :
					model_list[ship->target->model].name,
					ship->target->type == MOB_SHIP ? "m" : "",
					abs(atoi(ship->target->filename)),
					ship->target->vx, ship->target->vy, ship->target->vz,
					(float)(ship->Distance(ship->target) / 1000),
					ship->target->speed, ship->target->hull);
			}
		}

		if (0 && ship->starsystem) {
			int too_close = ship->speed + 50;

			FOREACH(ShipList, ship->starsystem->ships, iter)
			{
				ShipData *target = *iter;
				int target_too_close = too_close + target->speed;
				if (target != ship &&
				flabs(ship->vx - target->vx) < target_too_close &&
				flabs(ship->vy - target->vy) < target_too_close &&
				flabs(ship->vz - target->vz) < target_too_close)
					echo_to_ship(TO_PILOTSEAT, ship, "`RProximity alert: %s  %.0f %.0f %.0f`n", SHPS(target, ship), target->vx, target->vy, target->vz);
			}
		}

		if (ship->energy < 100 && ship->starsystem)
			echo_to_ship(TO_PILOTSEAT, ship, "`RWarning: Ship fuel low.`n");
	}

	FOREACH(ShipList, ship_list, iter)
	{
		ShipData *ship = *iter;
		
		if (SHP_FLAGGED(ship, SHIP_AUTOTRACK) && (ship->following || ship->target)
			&& ship->shipclass < SPACE_STATION)
		{
			ShipData *target = ship->target ? ship->target : ship->following;
			int too_close = ship->speed + 10;
			int target_too_close = too_close + target->speed;

			if (0 && ship != target && ship->state == SHIP_READY &&
			flabs(ship->vx - target->vx) < target_too_close &&
			flabs(ship->vy - target->vy) < target_too_close &&
			flabs(ship->vz - target->vz) < target_too_close) {
				ship->hx = 0 - (target->vx - ship->vx);
				ship->hy = 0 - (target->vy - ship->vy);
				ship->hz = 0 - (target->vz - ship->vz);
				AlterEnergy(ship, ship->speed / 2);
				echo_to_ship(TO_PILOTSEAT, ship, "`RAutotrack: Evading to avoid collision!`n");
				if (ship->manuever > 100)
					ship->state = SHIP_BUSY_3;
				else if (ship->manuever > 50)
					ship->state = SHIP_BUSY_2;
				else
					ship->state = SHIP_BUSY;
			} else if (!is_facing(ship, target)) {
				ship->hx = target->vx - ship->vx;
				ship->hy = target->vy - ship->vy;
				ship->hz = target->vz - ship->vz;
				echo_to_ship(TO_PILOTSEAT, ship, "`BAutotracking target ... setting new course.`n");
				if (ship->manuever > 100)
					ship->state = SHIP_BUSY_3;
				else if (ship->manuever > 50)
					ship->state = SHIP_BUSY_2;
				else
					ship->state = SHIP_BUSY;
			}
		}

		if (autofly(ship)) {
			if (ship->starsystem) {
				if (ship->state == SHIP_READY && ship->recall == SHIP_LAND) {
					echo_to_ship(TO_COCKPIT, ship, "`GAutopilot: Landing sequence initiated.`n");
					ship->state = SHIP_LAND;
					ship->recall = -1;
				}

				if (ship->state == SHIP_READY && ship->recall == SHIP_LAND_2)
					ship->recall = SHIP_LAND;

				if (ship->currjump != NULL
				&& ship->recall == SHIP_HYPERSPACE
				&& ship->state == SHIP_READY) {
					echo_to_system(ship, NULL, "`Y%s disapears from your scanner.`n", ship->name);
					ship->FromSystem();
					ship->state = SHIP_HYPERSPACE;
					echo_to_ship(TO_COCKPIT, ship, "`GAutopilot: Calculating hyperspace coordinates.`n");
					echo_to_ship(TO_SHIP, ship, "`YThe ship lurches slightly as it makes the jump to lightspeed.`n");
					echo_to_ship(TO_COCKPIT, ship, "`YThe stars become streaks of light as you enter hyperspace.`n");
					ship->vx = ship->jx;
					ship->vy = ship->jy;
					ship->vz = ship->jz;
					ship->speed = 0;
					ship->recall = SHIP_LAND_2;
				}

				if (ship->state == SHIP_READY && ship->recall == SHIP_LAUNCH)
					ship->recall = SHIP_HYPERSPACE;

				if (ship->secondstate == MISSILE_DAMAGED && Number(0, 30) == 0) {
					ship->secondstate = MISSILE_READY;
					echo_to_ship(TO_COCKPIT, ship, "`GAutorepair: Ship's missile launcher repaired.`n");
				}
				if (ship->primstate == LASER_DAMAGED && Number(0, 30) == 0) {
					ship->primstate = LASER_READY;
					echo_to_ship(TO_COCKPIT, ship, "`GAutorepair: Ship's primary weapon repaired.`n");
				}
				if (SHP_FLAGGED(ship, SHIP_DISABLED) && Number(0, 35) == 0) {
					ship->state = SHIP_READY;
					REMOVE_BIT(SHP_FLAGS(ship), SHIP_DISABLED);
					echo_to_ship(TO_COCKPIT, ship, "`GAutorepair: Ship's drive repaired.`n");
				}

				if (ship->target) {
					float chance = 100;

					ShipData *target = ship->target;
					SET_BIT(SHP_FLAGS(ship), SHIP_AUTOTRACK);
					if (ship->shipclass != SPACE_STATION)
						ship->speed = ship->maxspeed;

					if (ship->state != SHIP_HYPERSPACE && ship->energy > 200
					&& ship->hyperspeed > 0 && ship->starsystem == target->starsystem
					&& ship->Distance(target) > 5000 && Number(1, 5) == 5) {
						ship->currjump = ship->starsystem;
						ship->hyperdistance = 1;

						echo_to_system(ship, NULL, "`Y%s disappears from your scanner.`n", ship->name);
						echo_to_ship(TO_SHIP, ship, "`YThe ship accelerates to hyperspeed.`n");

						ship->FromSystem();
						ship->state = SHIP_HYPERSPACE;
						AlterEnergy(ship, 200);
						ship->vx = target->vx + Number(-100, 100);
						ship->vy = target->vy + Number(-100, 100);
						ship->vz = target->vz + Number(-100, 100);
					}

					if (ship->state != SHIP_HYPERSPACE && ship->energy > 25
					&& !Event::FindEvent(ship->m_Events, MissileCooldownEvent::Type)
					&& ship->secondam > 0 && ship->CanSee(ship->target)
					&& ship->starsystem == target->starsystem
					&& ship->Distance(target) <= 1000) {
						if (ship->shipclass == SPACE_STATION || is_facing(ship, target)) {
							chance -= target->manuever / 5;
							chance -= target->speed / 20;
							chance += target->model * target->model * 2;
							chance -= (flabs(target->vx - ship->vx) / 100);
							chance -= (flabs(target->vy - ship->vy) / 100);
							chance -= (flabs(target->vz - ship->vz) / 100);
							chance += 30;
							chance = URANGE(10, chance, 90);

							if (chance > Number(1, 100)) {
								new_missile(ship, target, NULL);
								echo_to_ship(TO_COCKPIT, target, "`rIncoming missile from %s.`n", ship->name);
								echo_to_system(target, NULL, "`y%s fires a missile towards %s.`n", ship->name, target->name);
								ship->m_Events.push_front(new MissileCooldownEvent(model_list[ship->model].mcooldown, ship->GetID()));
							}
						}
					}
				} else {
					int targetok;
					Clan *clan = NULL;
					Clan *shipclan = NULL;

					ship->speed = 0;
					if (is_number(ship->owner))
					{
						shipclan = Clan::GetClan(atoi(ship->owner));
					}

					if (shipclan) {
						FOREACH(ShipList, ship->starsystem->ships, iter)
						{
							ShipData *target = *iter;
							targetok = 0;

							if (!ship->CanSee(target))
								continue;

//							if (autofly(target) && !str_cmp(ship->owner, target->owner))
						if (target->type == MOB_SHIP && !str_cmp(ship->owner, target->owner))
								targetok = 1;

							FOREACH(Lexi::List<RoomData *>, target->rooms, room)
							{
								FOREACH(CharacterList, (*room)->people, passenger)
								{
									if (!IS_NPC(*passenger) && GET_CLAN(*passenger)) {
										if (GET_CLAN(*passenger) == shipclan)
											targetok = 1;
										else if (targetok == 0) {
											if (Clan::GetRelation(GET_CLAN(*passenger)->GetRealNumber(), shipclan->GetRealNumber()) == RELATION_ENEMY)
												targetok = -1;
										}
									}
								}
							}

							if (targetok == 1 && target->target) {
								ship->SetTarget(target->target);
								deploy_interceptors(ship);
								echo_to_ship(TO_COCKPIT, target->target, "`rYou are being targeted by %s.`n", ship->name);
								if (autofly(target->target) && !target->target->target) {
									echo_to_ship(TO_COCKPIT, ship, "`rYou are being targetted by %s.`n", target->target->name);
									target->target->SetTarget(ship);
									deploy_interceptors(target->target);
								}
								break;
							}

							if (targetok == 0 && target->target && target->type == MOB_SHIP) {
//								if (!str_cmp(target->target->owner, strip_color(CLANNAME(shipclan))))
//									targetok = -1;
								if (is_number(target->target->owner) && atoi(target->target->owner) == shipclan->GetVirtualNumber())
									targetok = -1;
								else if ((clan = Clan::GetClan(target->clan)) && Clan::GetRelation(clan->GetRealNumber(), shipclan->GetRealNumber()) == RELATION_ENEMY)
									targetok = -1;
								else {
									FOREACH(Lexi::List<RoomData *>, target->target->rooms, room)
									{
										FOREACH(CharacterList, (*room)->people, passenger)
										{
											if (!IS_NPC(*passenger) && GET_CLAN(*passenger) == shipclan)
												targetok = -1;
										}
									}
								}
							}

							if (targetok >= 0)
								continue;

							ship->SetTarget(target);
							deploy_interceptors(ship);
							echo_to_ship(TO_COCKPIT, target, "`WYou are being scanned by %s.`n", ship->name);
							echo_to_ship(TO_COCKPIT, target, "`rYou are being targeted by %s.`n", ship->name);
							if (autofly(target) && !target->target) {
								echo_to_ship(TO_COCKPIT, ship, "`rYou are being targetted by %s.`n", target->name);
								target->SetTarget(ship);
								deploy_interceptors(target);
							}
							break;
						}
					}
				}
			} else if ((ship->shipclass == SPACE_STATION || ship->type == MOB_SHIP) && ship->home) {
				if (Number(1, 25) == 25) {
					ship->ToSystem(starsystem_from_name(ship->home));
					ship->vx = Number(-5000, 5000);
					ship->vy = Number(-5000, 5000);
					ship->vz = Number(-5000, 5000);
					ship->hx = ship->hy = ship->hz = 1;
					SET_BIT(SHP_FLAGS(ship), SHIP_AUTOPILOT);
				}
			}
		}
	}
}

#if 0
void space_control_update(void) {
	RNum clan = -1;
	ShipData *ship;
	int i = 0;
	StellarObject *planet;
	int fightershiptype, battleshiptype;
	int fleet, clan_ships, numbattleships = 0, numfighters = 0;

	FOREACH(StellarObjectList, stellar_objects, planet)
	{
		if (planet->starsystem == NULL || (clan = Clan::Real(planet->clan)) == -1)
			continue;
		ZoneData *zone = ZoneData::GetZone(planet->zone);
		if (!zone)		// heh where?
			continue;
		if ((clan_ships = Clan::Index[clan]->ships) <= 0)
			continue;

		numfighters = 0;
		numbattleships = 0;
		FOREACH(ShipList, planet->starsystem->ships, ship)
		{
			if (is_number(ship->owner) && atoi(ship->owner) == GET_CLAN_VNUM(clan) && ship->type == MOB_SHIP) {
				if (ship->size == FIGHTER_SHIP)
					numfighters++;
				else
					numbattleships++;
			}
		}

		fleet = (40 * numbattleships) + (5 * numfighters);

		if (CLAN_RACE(clan) == RACE_TERRAN) {
			battleshiptype = BATTLECRUISER;
			fightershiptype = WRAITH;
		} else if (CLAN_RACE(clan) == RACE_PROTOSS) {
			battleshiptype = CARRIER;
			fightershiptype = SCOUT;
		} else if (CLAN_RACE(clan) == RACE_ZERG) {
			battleshiptype = INFBATTLESHIP;
			fightershiptype = INFWRAITH;
		} else {
			mudlogf(BRF, LVL_STAFF, TRUE, "Clan #%d invalid race %d.", CLAN_VNUM(clan), CLAN_RACE(clan));
			continue;
		}

		if (fleet + 40 <= clan_ships)
			make_mob_ship(planet, battleshiptype);
		else if (fleet + 5 <= clan_ships)
			make_mob_ship(planet, fightershiptype);
	}
}
#endif

void echo_to_ship(Flags type, ShipData *ship, char *argument, ...) {
	va_list args;
	char send_buf[MAX_STRING_LENGTH];

	if (!argument || !*argument)
		return;

	va_start(args, argument);
	vsnprintf(send_buf, sizeof(send_buf), argument, args);
	va_end(args);

	if (IS_SET(type, TO_SHIP)) {
		FOREACH(Lexi::List<RoomData *>, ship->rooms, room)
			(*room)->Send("%s\n", send_buf);
	}

	if (IS_SET(type, TO_PILOTSEAT))
		echo_to_room(ship->pilotseat, send_buf);

	if (IS_SET(type, TO_GUNSEAT))
		echo_to_room(ship->gunseat, send_buf);

	if (IS_SET(type, TO_COCKPIT))
	{
		echo_to_room(ship->pilotseat, send_buf);
		if (ship->pilotseat != ship->gunseat)
			echo_to_room(ship->gunseat, send_buf);
		if (ship->pilotseat != ship->viewscreen && ship->viewscreen != ship->gunseat)
			echo_to_room(ship->viewscreen, send_buf);
		FOREACH(Lexi::List<Turret *>, ship->turrets, turret)
			if ((*turret)->room)
				echo_to_room((*turret)->room, send_buf);
	}

	if (IS_SET(type, TO_HANGERS)) {		// sends to 'other' ships in hangers
		FOREACH(Lexi::List<Hanger *>, ship->hangers, hanger) {
			if ((*hanger)->room) {
				FOREACH(ShipList, (*hanger)->room->ships, ship)
					echo_to_ship(TO_SHIP, *ship, "%s", send_buf);
			}
		}
	}
}

void echo_to_starsystem(StarSystem *system, char *argument, ...)
{
	va_list args;

	if (!system || !argument || !*argument)
		return;

	BUFFER(send_buf, MAX_STRING_LENGTH);
	va_start(args, argument);
	vsnprintf(send_buf, MAX_STRING_LENGTH, argument, args);
	va_end(args);

	FOREACH(ShipList, system->ships, target)
		echo_to_ship(TO_COCKPIT, *target, send_buf);

	return;
}

void echo_to_system(ShipData *ship, ShipData *ignore, char *argument, ...) {
	va_list args;

	if (!ship->starsystem || !argument || !*argument)
		return;

	BUFFER(send_buf, MAX_STRING_LENGTH);
	
	va_start(args, argument);
	vsnprintf(send_buf, MAX_STRING_LENGTH, argument, args);
	va_end(args);

	FOREACH(ShipList, ship->starsystem->ships, target)
	{
		if (*target != ship && *target != ignore && (*target)->CanSee(ship))
			echo_to_ship(TO_COCKPIT, *target, send_buf);
	}
}


void echo_to_room(RoomData *room, char *argument)
{
	if (!room)
		return;

	FOREACH(CharacterList, room->people, to)
		(*to)->Send("%s\n", argument);
}


int ships_owned(CharData *ch, bool clanbuy) {
	int num = 0;
	Clan* clan = GET_CLAN(ch);

	FOREACH(ShipList, ship_list, ship)
		if ((*ship)->type != MOB_SHIP && ((clanbuy && clan && is_number((*ship)->owner) && atoi((*ship)->owner) == clan->GetVirtualNumber()) || !str_cmp(ch->GetName(), (*ship)->owner)))
			num++;

	return num;
}

int ship_proto_value(int type) {
	int price = 0;

	price += (model_list[type].maxspeed * 10);
	price += (model_list[type].hull * 5);
	price += (model_list[type].energy / 2);
	price += (model_list[type].shield * 2);

	return price;
}

bool ship_was_in_range(ShipData *ship, ShipData *target) {
	if (target && ship && target != ship) {
		int sensor = 100 * (ship->sensor + 10) * (target->shipclass + 3);
		if (flabs(target->ox - ship->vx) < sensor
		&& flabs(target->oy - ship->vy) < sensor
		&& flabs(target->oz - ship->vz) < sensor)
			return true;
	}
	return false;
}

bool has_comlink(CharData *ch) {
#if 0
	ObjData *obj;

//  for (int i = 0; i < NUM_WEARS; i++)
//		if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_COMLINK)
//				return TRUE;

	FOREACH(ObjList, ch->cyberware, obj)
		if (GET_OBJ_VAL(obj, 2) == CYBER_COMLINK)
			return TRUE;

	return FALSE;
#endif
	return TRUE;
}

bool is_facing(ShipData *ship, ShipData *target) {
	float dx, dy, dz, hx, hy, hz, cosofa;

	hx = ship->hx;
	hy = ship->hy;
	hz = ship->hz;

	dx = target->vx - ship->vx;
	dy = target->vy - ship->vy;
	dz = target->vz - ship->vz;

	cosofa = (hx*dx + hy*dy + hz*dz) /
		(sqrt(hx*hx + hy*hy + hz*hz) + sqrt(dx*dx + dy*dy + dz*dz));

	if (cosofa > 0.75)
		return TRUE;

	return FALSE;
}

bool autofly(ShipData *ship) {
	if (!ship)
		return FALSE;

	if (ship->type == MOB_SHIP)
		return TRUE;

	return SHP_FLAGGED(ship, SHIP_AUTOPILOT);
}

bool check_owner(CharData *ch, ShipData *ship) {
	int i = 0;
	Clan * clan = GET_CLAN(ch);

	if (!str_cmp(ch->GetName(), ship->owner))
		return TRUE;
	if (!str_cmp(ch->GetName(), ship->pilot))
		return TRUE;
	if (!str_cmp(ch->GetName(), ship->copilot))
		return TRUE;
	if (clan && is_number(ship->owner) && atoi(ship->owner) == clan->GetVirtualNumber())
		return TRUE;

	return FALSE;
}

bool check_pilot(CharData *ch, ShipData *ship) {
	int i = 0;
	Clan * clan = GET_CLAN(ch);

	if (ship->type == MOB_SHIP)
		return FALSE;

	if (NO_STAFF_HASSLE(ch))
		return TRUE;

	if (!str_cmp("Public", ship->owner))
		return TRUE;
	if (!str_cmp(ch->GetName(), ship->owner))
		return TRUE;
	if (!str_cmp(ch->GetName(), ship->pilot))
		return TRUE;
	if (!str_cmp(ch->GetName(), ship->copilot))
		return TRUE;
//	if (clan != -1 && GET_CLAN_LEVEL(ch) && is_number(ship->owner) && atoi(ship->owner) == Clan::Index[clan]->GetVirtualNumber())
//		return TRUE;
	return FALSE;
}

void ShipData::FromRoom(void)
{
	if (!m_InRoom)	return;
	
	m_InRoom->ships.remove(this);
	m_InRoom = NULL;
}

bool ShipData::ToRoom(RoomData * room) {
	if (room == NULL) {
		log("SYSERR: Illegal value(s) passed to ship_to_room. (Room #%p, obj %s)", room, this->name);
		return FALSE;
	} else if (!IsPurged()) {
		room->ships.push_front(this);
		m_InRoom = room;
		return TRUE;
	}
	return FALSE;
}

bool ShipData::ToRoom(UInt32 vnum) {
	RoomData * shipto;

	if ((shipto = World::GetRoomByVNum(vnum)) == NULL)
		return FALSE;

	shipto->ships.push_front(this);
	m_InRoom = shipto;
	return TRUE;
}

StarSystem *starsystem_from_name(char *name)
{
	FOREACH(Lexi::List<StarSystem *>, starsystem_list, starsystem)
		if (!str_cmp(name, (*starsystem)->name))
			return *starsystem;

	FOREACH(Lexi::List<StarSystem *>, starsystem_list, starsystem)
		if (!str_prefix(name, (*starsystem)->name))
			return *starsystem;

	return NULL;
}

ShipData *ship_from_room(RoomData * room) {
	if (room == NULL)
		return NULL;

	if (room->ship)
		return room->ship;

	FOREACH(ShipList, ship_list, ship) {
		FOREACH(Lexi::List<RoomData *>, (*ship)->rooms, sr)
			if (room == *sr)
				return *ship;
	}

	return NULL;
}

ShipData *ship_from_turret(RoomData * room)
{
	if (room == NULL || room->ship == NULL)
		return NULL;

	if (room == room->ship->gunseat)
		return room->ship;

	FOREACH(Lexi::List<Turret *>, room->ship->turrets, turret)
		if (room == (*turret)->room)
			return room->ship;

	return NULL;
}

ShipData *ship_from_engine(RoomData * room) {
	if (room == NULL || room->ship == NULL)
		return NULL;

	if (room == room->ship->engine)
		return room->ship;

	return NULL;
}

ShipData *ship_from_cockpit(RoomData *room)
{
	if (room == NULL || room->ship == NULL)
		return NULL;

	if (room == room->ship->pilotseat)
		return room->ship;
	if (room == room->ship->gunseat)
		return room->ship;
	if (room == room->ship->viewscreen)
		return room->ship;

	FOREACH(Lexi::List<Turret *>, room->ship->turrets, turret)
		if (room == (*turret)->room)
			return room->ship;

	return NULL;
}

ShipData *ship_from_pilotseat(RoomData * room) {
	if (room && room->ship && room == room->ship->pilotseat)
		return room->ship;

	return NULL;
}

StarSystem *starsystem_from_room(RoomData * room) {
	if (room == NULL)
		return NULL;

	if (room->zone->planet != NULL
	&& !IS_SET(room->flags, ROOM_SPACECRAFT))
		return room->zone->planet->starsystem;

	if (room->ship != NULL && room->ship->starsystem != NULL)
		return room->ship->starsystem;

	if (room->ship != NULL && IN_ROOM(room->ship))
		return starsystem_from_room(IN_ROOM(room->ship));

	return NULL;
}

double MissileData::Distance(ShipData *target) {
	float x = this->mx - target->vx,
		y = this->my - target->vy,
		z = this->mz - target->vz;
	return sqrt(x*x + y*y + z*z);
}

void MissileData::Move(void) {
	ShipData *ship = find_ship(this->fired_from);
	ShipData *target = find_ship(this->target);

	if (!target || this->age >= 25) {
		echo_to_starsystem(this->starsystem, "`R%s self-detonates.`n", this->name);
		FOREACH(ShipList, this->starsystem->ships, iter)
		{
			ship = *iter;
			if (this->Distance(ship) <= 200) {
				ship->Damage(NULL, MAX(1, this->damage / 8), DAM_MISSILE);
				echo_to_ship(TO_COCKPIT, ship, "`RThe ship is caught in the blast!`n");
				echo_to_ship(TO_SHIP, ship, "`RThe ship is shaken by a small explosion.`n");
			}
		}
		extract_missile(this);
		return;
	}

	this->age++;
	if (this->starsystem == target->starsystem) {
		if (this->mx < target->vx)
			this->mx += UMIN(this->speed / 5, target->vx - this->mx);
		else if (this->mx > target->vx)
			this->mx -= UMIN(this->speed / 5, this->mx - target->vx);

		if (this->my < target->vy)
			this->my += UMIN(this->speed / 5, target->vy - this->my);
		else if (this->my > target->vy)
			this->my -= UMIN(this->speed / 5, this->my - target->vy);

		if (this->mz < target->vz)
			this->mz += UMIN(this->speed / 5, target->vz - this->mz);
		else if (this->mz > target->vz)
			this->mz -= UMIN(this->speed / 5, this->mz - target->vz);

		if (this->Distance(target) < 20) {
			CharData *ch = CharData::Find(this->fired_by);
			echo_to_ship(TO_COCKPIT, target, "`RThe ship is hit by %s.`n", this->name);
			echo_to_ship(TO_SHIP, target, "`RA loud explosion shakes the ship violently!`n");
			echo_to_system(target, ship, "`yYou see a small explosion as %s is hit by %s.`n", target->name, this->name);
			if (ship)
				echo_to_ship(TO_GUNSEAT, ship, "`YYour missile hits its target dead on!`n");
			target->Damage(ch, this->damage, DAM_MISSILE);
			extract_missile(this);
		}
	}
}

void ShipData::Move(void) {
	if (!this->starsystem)
		return;

	if (this->speed > 0) {
		float nx, ny, nz;
		float change = sqrt(this->hx * this->hx + this->hy * this->hy + this->hz * this->hz);

		if (change > 0) {
			nx = this->hx / change;
			ny = this->hy / change;
			nz = this->hz / change;
			this->vx += nx * (this->speed / 5);
			this->vy += ny * (this->speed / 5);
			this->vz += nz * (this->speed / 5);
		}
	}

	if (autofly(this))		// autopilot corrects it's flight
		return;

	// Insert gravity code here!
	FOREACH(StellarObjectList, this->starsystem->stellarObjects, iter)
	{
		StellarObject *stellarObject = *iter;
		if (this->vx != stellarObject->x)
			this->vx -= URANGE(-3, stellarObject->gravity / (this->vx - stellarObject->x) / 2, 3);
		if (this->vy != stellarObject->y)
			this->vy -= URANGE(-3, stellarObject->gravity / (this->vy - stellarObject->y) / 2, 3);
		if (this->vz != stellarObject->z)
			this->vz -= URANGE(-3, stellarObject->gravity / (this->vz - stellarObject->z) / 2, 3);

		if (stellarObject->type == Star && this->Distance(stellarObject) < 10) {
			echo_to_ship(TO_COCKPIT, this, "`R`fYou fly directly into the sun.`n");
			echo_to_system(this, NULL, "`y%s flies directly into %s!`n", this->name, stellarObject->name);
			this->Destroy();
			return;
		}
		if (stellarObject->type == Wormhole && this->Distance(stellarObject) < 10) {
			if (this->speed < 0 || stellarObject->connects == NULL)
				continue;

			echo_to_ship(TO_COCKPIT, this, "`YYou fly into the center of the %s.`n", stellarObject->name);
			echo_to_system(this, NULL, "`Y%s flies into the %s.`n", this->name, stellarObject->name);
			this->FromSystem();
			this->ToSystem(stellarObject->connects);
			this->vx += Number(-100, 100) + 5;
			this->vy += Number(-100, 100) + 5; // scoot it away from hole
			this->vz += Number(-100, 100) + 5; // add five incase of zeroes
			echo_to_system(this, NULL, "`Y%s flies out of the %s.`n", this->name, stellarObject->name);
			break;
		}

		if (this->speed > 0 && stellarObject->type != Star && this->Distance(stellarObject) < 10) {
			echo_to_ship(TO_COCKPIT, this, "`YYou begin orbiting %s.`n", stellarObject->name);
			echo_to_system(this, NULL, "`y%s begins orbiting %s.`n", this->name, stellarObject->name);
			this->speed = 0;
		}
	}
}

void StellarObject::Update(void) {
// not sure what needs to be done? move it maybe?
}

void ShipData::Update(void)
{
	if (this->state == SHIP_HYPERSPACE) {
		this->hyperdistance -= this->hyperspeed * 2;

		if (this->hyperdistance > 0)
			echo_to_ship(TO_PILOTSEAT, this, "`YRemaining jump distance: `W%d`n", this->hyperdistance);
		else {
			this->ToSystem(this->currjump);
			this->currjump = NULL;

			if (this->starsystem == NULL)
				echo_to_ship(TO_COCKPIT, this, "`RShip lost in hyperspace. Make new calculations.`n");
			else {
				echo_to_ship(TO_PILOTSEAT, this, "`YHyperjump complete.`n");
				echo_to_ship(TO_SHIP, this, "`YThe ship lurches slightly as it comes out of hyperspace.`n");
				echo_to_system(this, NULL, "`Y%s enters the starsystem at %.0f %.0f %.0f`n", this->name, this->vx, this->vy, this->vz);
				this->state = SHIP_READY;

				FREE(this->home);
				this->home = str_dup(this->starsystem->name);
			}
		}
	}

	switch (this->state) {
		case SHIP_BUSY_3:
			echo_to_ship(TO_PILOTSEAT, this, "`YManuever complete.`n");
			this->state = SHIP_READY;
			break;
		case SHIP_BUSY_2:		this->state = SHIP_BUSY_3;		break;
		case SHIP_BUSY:				this->state = SHIP_BUSY_2;		break;
		case SHIP_LAND_2:		this->Land(this->dest);				break;
		case SHIP_LAND:				this->state = SHIP_LAND_2;		break;
		case SHIP_LAUNCH_2:		this->Launch();						break;
		case SHIP_LAUNCH:		this->state = SHIP_LAUNCH_2;		break;
		default:
			break;
	}

	if (this->starsystem != NULL) {
		float distance, tooClose = this->speed + 50;

		if (this->energy < 100)
			echo_to_ship(TO_COCKPIT, this, "`RWarning: Ship fuel low.`n");

		if (this->speed > 0)
			echo_to_ship(TO_PILOTSEAT, this, "*`BSpeed: `C%d  `BCoords: `C%.0f %.0f %.0f`n", this->speed, this->vx, this->vy, this->vz);

		if (this->target) {
			if (this->starsystem != this->target->starsystem || !this->CanSee(this->target) || !this->target->starsystem)
				this->SetTarget(NULL);
			else {
				echo_to_ship(TO_COCKPIT, this, "`BTarget: `C%s %s%d  `BCoords: `C%.0f %.0f %.0f (%0.2fk)  `BSpeed: `C%d  `BHull: `C%d`n",
					model_list[this->target->model].name,
					this->target->type == MOB_SHIP ? "m" : "",
					abs(atoi(this->target->filename)),
					this->target->vx, this->target->vy, this->target->vz,
					(float)(this->Distance(this->target) / 1000),
					this->target->speed, this->target->hull);
			}
		}

		FOREACH(ShipList, this->starsystem->ships, iter)
		{
			ShipData *ship = *iter;
			
			if (this == ship)
				continue;

			if ((distance = this->Distance(ship)) < (ship->speed + tooClose))
				echo_to_ship(TO_PILOTSEAT, this, "`RProximity alert: %s  %.0f %.0f %.0f (%.0f)`n", SHPS(ship, this), ship->vx, ship->vy, ship->vz, distance);
		}

		tooClose = this->speed + 100;
		FOREACH(StellarObjectList, this->starsystem->stellarObjects, iter)
		{
			StellarObject *object = *iter;
			if (object->type != Star)
				continue;
			if ((distance = this->Distance(object)) < tooClose)
				echo_to_ship(TO_PILOTSEAT, this, "`RProximity alert: %s  %.0f %.0f %.0f (%0.2fk)`n", object->name, object->x, object->y, object->z, (float)(distance / 1000));
		}

		tooClose = this->speed + 10;
		if (SHP_FLAGGED(this, SHIP_AUTOTRACK) && this->target) {
			float TargetTooClose = tooClose + this->target->speed;

			if (this->state == SHIP_READY && this->Distance(this->target) < TargetTooClose) {
				this->hx = 0 - (this->target->vx - this->vx);
				this->hy = 0 - (this->target->vy - this->vy);
				this->hz = 0 - (this->target->vz - this->vz);
				AlterEnergy(this, this->speed / 2);
				echo_to_ship(TO_PILOTSEAT, this, "`RAutotrack: Evading to avoid collision!`n");

				if (this->manuever > 100)
					this->state = SHIP_BUSY_3;
				else if (this->manuever > 50)
					this->state = SHIP_BUSY_2;
				else
					this->state = SHIP_BUSY;
			} else if (this->state == SHIP_READY && !is_facing(this, this->target)) {
				this->hx = this->target->vx - this->vx;
				this->hy = this->target->vy - this->vy;
				this->hz = this->target->vz - this->vz;
				echo_to_ship(TO_PILOTSEAT, this, "`BAutotracking target, setting new course.`n");

				if (this->manuever > 100)
					this->state = SHIP_BUSY_3;
				else if (this->manuever > 50)
					this->state = SHIP_BUSY_2;
				else
					this->state = SHIP_BUSY;
			}
		}

		if (autofly(this)) {
			if (this->state == SHIP_READY && this->recall == SHIP_LAND) {
				echo_to_ship(TO_COCKPIT, this, "`GAutopilot: Landing sequence initiated.`n");
				this->state = SHIP_LAND;
				this->recall = -1;
			}

			if (this->state == SHIP_READY && this->recall == SHIP_LAND_2)
				this->recall = SHIP_LAND;

			if (this->currjump != NULL && this->recall == SHIP_HYPERSPACE && this->state == SHIP_READY) {
				echo_to_system(this, NULL, "`Y%s disapears from your scanner.`n", this->name);
				this->FromSystem();
				this->state = SHIP_HYPERSPACE;
				echo_to_ship(TO_COCKPIT, this, "`GAutopilot: Calculating hyperspace coordinates.`n");
				echo_to_ship(TO_SHIP, this, "`YThe ship lurches slightly as it makes the jump to lightspeed.`n");
				echo_to_ship(TO_COCKPIT, this, "`YThe stars become streaks of light as you enter hyperspace.`n");
				this->vx = this->jx;
				this->vy = this->jy;
				this->vz = this->jz;
				this->speed = 0;
				this->recall = SHIP_LAND_2;
			}

			if (this->state == SHIP_READY && this->recall == SHIP_LAUNCH)
				this->recall = SHIP_HYPERSPACE;

			if (this->target) {
				SET_BIT(SHP_FLAGS(this), SHIP_AUTOTRACK);
				this->speed = this->maxspeed;
// finish here...
			}
		}
	}		// this->starsystem != NULL
	CheckShipRates(this);
}

void ShipData::Land(char *argument) {
	ShipData *target = NULL;
	RoomData * room, *dest = NULL;
	VNum vnum;
	char *arg = get_buffer(MAX_INPUT_LENGTH);
	char *arg1 = get_buffer(MAX_INPUT_LENGTH);

	skip_spaces(argument);
	strcpy(arg, argument);
	argument = one_argument(argument, arg1);
	skip_spaces(argument);

	StellarObject *planet = NULL;
	
	FOREACH(StellarObjectList, this->starsystem->stellarObjects, iter)
	{
		StellarObject *object = *iter;
		
		if (!str_cmp(arg1, object->name)
			|| !str_prefix(arg1, object->name)
			|| isname(arg1, object->name))
		{
			ZoneData *zone = ZoneData::GetZone(object->zone);
			if (!zone)
				continue;
			for (vnum = (zone->GetVirtualNumber() * 100); vnum <= zone->top; vnum++) {
				if ((room = World::GetRoomByVNum(vnum))
				&& ROOM_FLAGGED(room, ROOM_CAN_LAND)
				&& (!str_cmp(argument, room->GetName())
				|| !str_prefix(argument, room->GetName())
				|| isname(argument, room->GetName()))) {
					dest = World::GetRoomByVNum(vnum);
					break;
				}
			}
			planet = object;
			break;
		}
	}

	if (!dest)
	{
		target = get_ship_here(arg, this->starsystem);
		if (target != this && target != NULL) {
			FOREACH(Lexi::List<Hanger *>, target->hangers, hanger)
			{
				if ((*hanger)->room->ships.size() < 2)
					dest = (*hanger)->room;
			}
		} else {
			dest = World::GetRoomByVNum(1299);
			target = NULL;
			mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: land_ship: Invalid destination for %s.", this->name);
		}
	}

	release_buffer(arg);
	release_buffer(arg1);

	if ((target != NULL && !this->ToRoom(dest)) || (target == NULL && !this->ToRoom(dest))) {
		echo_to_ship(TO_PILOTSEAT, this, "`YCould not complete aproach. Landing aborted.`n");
		echo_to_ship(TO_SHIP, this, "`YThe ship pulls back up out of its landing sequence.`n");
		this->state = SHIP_READY;
		return;
	}

	this->Appear();
	this->SetTarget(NULL);
	this->following = NULL;
	echo_to_ship(TO_PILOTSEAT, this, "`YLanding sequence complete.`n");
	echo_to_ship(TO_SHIP, this, "`YYou feel a slight thud as the ship sets down on the ground.`n");
	echo_to_ship(TO_HANGERS, this, "`YThe ship jolts and bangs against the docking moorings!`n");
	if (target == NULL && planet != NULL)
		echo_to_system(this, NULL, "`Y%s enters the atmosphere of %s.`n", this->name, planet->name);
	else if (target != NULL)
		echo_to_system(this, NULL, "`Y%s enters a hanger of %s.`n", this->name, target->name);
	else
		echo_to_system(this, NULL, "`Y%s disappears from your scanners.`n", this->name);

	this->location = dest;
	this->lastdoc = this->location;

	this->state = SHIP_DOCKED;
	this->FromSystem();

	char *buf = get_buffer(MAX_STRING_LENGTH);

	switch (this->size) {
		case FIGHTER_SHIP:
			sprintf(buf, "`Y%s lands softly on the platform.`n", this->name);
			break;
		case MIDSIZE_SHIP:
		case LARGE_SHIP:
			sprintf(buf, "`Y%s lands with a slight thud.`n", this->name);
			break;
		case CAPITAL_SHIP:
		case SHIP_PLATFORM:
			sprintf(buf, "`Y%s lands heavily against the ground as it makes a clumsy landing.`n", this->name);
			break;
		default:
			sprintf(buf, "`Y%s lands on the platform.`n", this->name);
			break;
	}
	echo_to_room(IN_ROOM(this), buf);

	AlterEnergy(this, 25 + (this->size + 1 * 5));

	if (!str_cmp("Public", this->owner)) {
		this->age				= 0;
		this->hull				= this->maxhull;
		this->maxenergy				= this->maxenergy;
		this->maxshield				= this->maxshield;
		this->state				= SHIP_DOCKED;
		this->primstate 		= LASER_READY;
		this->secondstate		= MISSILE_READY;
		REMOVE_BIT(this->flags, SHIP_AUTOTRACK | SHIP_AUTOPILOT | SHIP_DISABLED);
		echo_to_ship(TO_COCKPIT, this, "`YRepairing and refueling ship...`n");
	}
	this->addsave();
	release_buffer(buf);
}

void ShipData::Launch(void) {
	ShipData *source;
	RoomData * room, *o_room;

	o_room = IN_ROOM(this);
	this->ToSystem(starsystem_from_room(IN_ROOM(this)));

	if (this->starsystem == NULL) {
		echo_to_ship(TO_PILOTSEAT, this, "`YLaunch path unavailable.. Launch aborted.`n");
		echo_to_ship(TO_SHIP, this, "`YThe ship slowly sets back back down on the landing pad.`n");
		this->state = SHIP_DOCKED;
		return;
	}

	source = ship_from_room(IN_ROOM(this));
	this->FromRoom();
	this->location = 0;
	this->state = SHIP_READY;

	this->hx = Number(-2, 2);
	this->hy = Number(-2, 2);
	this->hz = Number(-2, 2);
	
	room = this->lastdoc;
	if (room
	&& room->zone->planet
	&& room->zone->planet->starsystem
	&& room->zone->planet->starsystem == this->starsystem) {
		this->vx = room->zone->planet->x;
		this->vy = room->zone->planet->y;
		this->vz = room->zone->planet->z;
	} else if (source) {
		this->vx = source->vx + Number(-100, 100);
		this->vy = source->vy + Number(-100, 100);
		this->vz = source->vz + Number(-100, 100);
	} else

	AlterEnergy(this, 100 + 10 * this->size);
	this->vx += (this->hx * this->speed * 2);
	this->vy += (this->hy * this->speed * 2);
	this->vz += (this->hz * this->speed * 2);

	echo_to_ship(TO_PILOTSEAT, this, "`GLaunch complete.`n");
	echo_to_ship(TO_SHIP, this, "`YThe ship leaves the platform far behind as it flies into space.`n");
	echo_to_system(this, NULL, "`Y%s enters the starsystem at %.0f %.0f %.0f`n", this->name, this->vx, this->vy, this->vz);
	o_room->Send("`Y%s lifts off into space.`n\n", this->name);
}

bool ShipData::Damage(CharData *ch, int damage, int type) {
	int dam = MAX(0, damage);

/*
	if (!str_cmp("Public", this->owner)) {
		if (!SHP_FLAGGED(this, SHIP_DISABLED)) {
			SET_BIT(SHP_FLAGS(this), SHIP_DISABLED);
			echo_to_ship(TO_COCKPIT, this, "`r`fShip disabled...`n");
			echo_to_system(this, NULL, "`y%s is disabled and begins to drift.`n", this->name);
		}
		return FALSE;
	}
*/

	if (this->upgrades[UPGRADE_SHIELD] && this->shield > 0)
		dam -= this->upgrades[UPGRADE_SHIELD];

	if (dam <= 0)
		return FALSE;

	if (this->tempshield > 0) {
		if (dam > this->tempshield) {
			Event *event;
			dam -= this->tempshield;
			this->tempshield = 0;
			echo_to_ship(TO_COCKPIT, this, "`yThe defense matrix dissipates.`n");
			echo_to_system(this, NULL, "`yThe defense matrix around %s dissipates.`n", this->name);
			if ((event = Event::FindEvent(m_Events, ShieldMatrixEvent::Type))) {
				m_Events.remove(event);
				event->Cancel();
			}
		} else {
			this->tempshield -= dam;
			return FALSE;
		}
	}

	if (this->shield > 0) {
		if (dam > this->shield) {
			dam -= this->shield;
			AlterShields(this, this->shield);
		} else {
			AlterShields(this, dam);
			return FALSE;
		}
	}

	dam -= this->armor + this->upgrades[UPGRADE_ARMOR];

	if (dam > 0) {
		AlterHull(this, dam);

		if (this->hull <= 0)
			this->Destroy(ch == NULL ? NULL : ch);
		else if (this->hull <= (this->maxhull / 10))
			echo_to_ship(TO_COCKPIT, this, "`r`fWARNING! Ship hull severely damaged!`n");
		return TRUE;
	}
	return FALSE;
}

void ShipData::Destroy(CharData *ch) {
	BUFFER(buf, MAX_INPUT_LENGTH);

	if (IN_ROOM(this))
		IN_ROOM(this)->Send("`W`f%s explodes in a blinding flash of light!`n\n", this->name);

	echo_to_system(this, NULL, "`W`f%s explodes in a blinding flash of light!`n", this->name);
	echo_to_ship(TO_SHIP, this, 
		"`WThe ship is shaken by a `RFATAL`W explosion.\n"
		"A blinding flash of light burns your eyes...\n"
		"Before you can scream you are ripped apart in a massive explosion!`n");
//		"The last thing you remember is reaching for the escape pod release lever.\n"
//		"`fA blinding flash of light.`n\n"
//		"`WAnd then darkness....`n");

	if (ch != NULL)
		sprintf(buf, "`W%s destroyed by %s.`n", this->name, ch->GetName());
	else
		sprintf(buf, "`W%s is destroyed in a blinding flash of light!`n", this->name);
	announce(buf);

	if (this->mothership) {
		this->mothership->interceptors--;
		this->mothership->fighters.remove(this);
	}

	// nuke any fighters it might have
	destroy_interceptors(this, ch);

	// remove any bad flags
	REMOVE_BIT(SHP_FLAGS(this), SHIP_DISABLED);
	REMOVE_BIT(SHP_FLAGS(this), SHIP_CLOAKED | SHIP_WEBBED);
	REMOVE_BIT(SHP_FLAGS(this), SHIP_SABOTAGE | SHIP_IRRADIATED);

	FOREACH(ShipList, ship_list, iter)
	{
		ShipData *tship = *iter;
		if (tship->target == this) {
			tship->SetTarget(NULL);
			remove_interceptors(tship);
		}

		if (tship->following == this)
			tship->following = NULL;

		FOREACH(Lexi::List<Turret *>, tship->turrets, turret)
			if ((*turret)->target == this)
				(*turret)->target = NULL;
	}

	FOREACH(Lexi::List<RoomData *>, this->rooms, iter)
	{
		RoomData *room = *iter;
		
		FOREACH(ShipList, room->ships, ship)
			(*ship)->Destroy(ch == NULL ? NULL : ch);

		if (str_cmp("Public", this->owner))
		{
			room->ship = NULL;
			World::RemoveRoom(room);
		}

		FOREACH(CharacterList, room->people, iter)
		{
			CharData *rch = *iter;
			
			if (ch && ch != rch) {
				if (AFF_FLAGGED(ch, AFF_GROUP))
					group_gain(ch, rch, TYPE_MISC);
				else if (!IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_TRAITOR)
//				&& !AFF_FLAGGED(ch, AFF_INFESTED) && !IS_DUMBASS(ch)
				&& (ch->GetRelation(rch) == RELATION_ENEMY)) {
					calc_reward(ch, rch, 1, GET_LEVEL(ch));
				}
			}

			if (NO_STAFF_HASSLE(rch)) {
				rch->FromRoom();
				rch->ToRoom(rch->StartRoom());
				look_at_room(rch, 0);
			} else
				rch->die(ch);
		}

		FOREACH(ObjectList, room->contents, obj) {
			if (!str_cmp("Public", this->owner) && OBJ_FLAGGED(*obj, ITEM_NOLOSE))
				continue;

			(*obj)->Extract();
		}
	}
	if (this->starsystem)
		this->FromSystem();

	this->EventCleanup();

	if (!str_cmp("Public", this->owner)) {
		this->FromRoom();
		this->lastdoc = this->shipyard;
		this->location = this->lastdoc;
		this->ToRoom(this->shipyard);
		this->state = SHIP_DOCKED;

		return;
	}

	sprintf(buf, "%s%s", SHIP_DIR, this->filename);

	this->Extract();

	if (this->type != MOB_SHIP) {
		unlink(buf);
		write_ship_list();
	}
}

void new_missile(ShipData *ship, ShipData *target, CharData *ch) {
	char *buf, *buf2;
	MissileData *missile;
	StarSystem *starsystem;

	if (ship == NULL || target == NULL)
		return;

	if ((starsystem = ship->starsystem) == NULL)
		return;

	CREATE(missile, MissileData, 1);
	missile_list.push_front(missile);
	starsystem->missiles.push_front(missile);

	buf = get_buffer(MAX_STRING_LENGTH);
	buf2= get_buffer(MAX_STRING_LENGTH);

	sprintf(buf2, "%s", *air_weap[ship->model] ? air_weap[ship->model] : "missile");
	sprintf(buf, "%s %s", AN(buf2), buf2);

	missile->age		= 0;
	missile->speed		= 300;
	missile->target		= target->GetID();
	missile->damage		= ship->secondam + ship->upgrades[UPGRADE_AIR];
	missile->fired_from		= ship->GetID();
	missile->name		= str_dup(buf);
	missile->fired_by		= ch == NULL ? 0 : ch->GetID();
	missile->mx				= ship->vx;
	missile->my				= ship->vy;
	missile->mz				= ship->vz;
	missile->starsystem		= starsystem;
	release_buffer(buf);
	release_buffer(buf2);
}

void ShipData::ToSystem(StarSystem *starsystem) {
	if (IsPurged() || starsystem == NULL)
		return;

	starsystem->ships.push_front(this);
	this->starsystem = starsystem;
}

void ShipData::FromSystem(void)
{
	if (this->starsystem == NULL) {
		log("SYSERR: Ship (%s) not in a starsystem passed to ship_from_starsystem", this->name);
		return;
	}

	if (IsPurged())
		return;

	FOREACH(ShipList, this->starsystem->ships, iter)
	{
		ShipData *ship = *iter;
		if (ship->target == this)
			ship->SetTarget(NULL);
		if (ship->following == this)
			ship->following = NULL;
	}
	this->starsystem->ships.remove(this);
	this->starsystem = NULL;
}

ShipData *get_ship(char *name) {

	if (!*name)
		return NULL;

	FOREACH(ShipList, ship_list, iter)
	{
		ShipData *ship = *iter;
		if (!str_cmp(name, ship->name))
			return ship;
		if (!str_prefix(name, ship->name))
			return ship;
		if (isname(name, ship->name))
			return ship;
	}

	return NULL;
}

ShipData *get_ship_here(char *name, StarSystem *starsystem) {
	if (starsystem == NULL)
		return NULL;

	FOREACH(ShipList, starsystem->ships, iter)
	{
		ShipData *ship = *iter;
		if (!str_cmp(name, ship->name))
			return ship;
		if (!str_prefix(name, ship->name))
			return ship;
		if (isname(name, ship->name))
			return ship;
	}

	return NULL;
}

ShipData *ship_in_room(RoomData * room, char *name) {
	if (room == NULL || !*name)
		return NULL;

	FOREACH(ShipList, room->ships, iter)
	{
		ShipData *ship = *iter;
		
		if (!str_cmp(name, strip_color(ship->name))
		|| !str_prefix(name, strip_color(ship->name))
		|| isname(name, strip_color(ship->name)))
			if (ship->owner && *ship->owner &&
			(is_online(ship->owner) || is_online(ship->pilot) || is_online(ship->copilot)))
				return ship;
	}
	
	return NULL;
}

ShipData *ship_from_entrance(RoomData * room)
{
	if (room == NULL)
		return NULL;

	FOREACH(ShipList, ship_list, ship)
		if (room == (*ship)->entrance)
			return *ship;

	return NULL;
}

void extract_missile(MissileData *missile) {
	StarSystem *starsystem;

	if (missile == NULL)
		return;

	if ((starsystem = missile->starsystem) != NULL) {
		starsystem->missiles.remove(missile);
		missile->starsystem = NULL;
	}

	missile_list.remove(missile);

	if (missile->name)
		free(missile->name);
	delete missile;
}

void write_starsystem_list(void) {
	FILE *fp;

	if (!(fp = fopen(SPACE_LIST, "w"))) {
		mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: Couldn't open %s for writing.", SPACE_LIST);
		return;
	}

	FOREACH(Lexi::List<StarSystem *>, starsystem_list, system)
		fprintf(fp, "%s\n", (*system)->filename);
	fprintf(fp, "$\n");
	fclose(fp);
}

void SaveShips(void) {
	while (!ship_saves.empty())
	{
		ship_saves.front()->save();
		ship_saves.pop_front();
	}
}

void save_starsystem(StarSystem *starsystem) {
	FILE *fp;

	if (!starsystem) {
		mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: save_starsystem: null starsystem!");
		return;
	}

	if (!starsystem->filename || !*starsystem->filename) {
		mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: save_starsystem: %s has no filename", starsystem->name);
		return;
	}

	if ((fp = fopen(SPACE_LIST, "w")) == NULL) {
		mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: save_starsystem: Couldn't open %s for writing.", SPACE_LIST);
		return;
	}

	fprintf(fp, "#SPACE\n");
	fprintf(fp, "Name %s~\n", starsystem->name);
	fprintf(fp, "FName %s~\n", starsystem->filename);
	fprintf(fp, "End\n");
	fprintf(fp, "#END\n");

	fclose(fp);
	return;
}

#if defined(KEY)
#undef KEY
#endif

#define KEY(literal, field, value)				\
		if (!str_cmp(word, literal)) {				\
			field = value;						\
			fMatch = TRUE;						\
			break;								\
		}

#if defined(KEYS)
#undef KEYS
#endif

#define KEYS(literal, field, value)				\
		if (!str_cmp(word, literal)) {				\
			free(field);						\
			field = value;						\
			fMatch = TRUE;						\
			break;								\
		}

void fread_starsystem(StarSystem *starsystem, FILE *fp) {
	int num = 0, num2 = 0, num3 = 0;
	char *tag = get_buffer(6);
	char *line = get_buffer(MAX_INPUT_LENGTH + 1);

	while (get_line(fp, line)) {
		tag_argument(line, tag);
		num = atoi(line);

			 if (!strcmp(tag, "FNam"))		starsystem->filename = str_dup(line);
		else if (!strcmp(tag, "Name"))		starsystem->name = str_dup(line);
		else if (!strcmp(tag, "Flag"))		starsystem->flags = asciiflag_conv(line);
		else		log("SYSERR: (#%d) Unknown starsystem tag '%s'", 1, tag);
	}
	release_buffer(tag);
	release_buffer(line);
}

bool load_starsystem(char *systemfile) {
	FILE *fp;
	bool found = FALSE;
	StarSystem *starsystem;
	char *filename = get_buffer(MAX_INPUT_LENGTH);

	sprintf(filename, "%s%s", SPACE_DIR, systemfile);

	if ((fp = fopen(filename, "r")) != NULL) {
		found = TRUE;

		CREATE(starsystem, StarSystem, 1);
		starsystem_list.push_front(starsystem);
		fread_starsystem(starsystem, fp);
		fclose(fp);
	}
	release_buffer(filename);
	return found;
}

void LoadStellarObjects(void) {
	FILE *fp;
	char *filename;

	if ((fp = fopen(STELLAROBJ_LIST, "r")) == NULL) {
		perror(STELLAROBJ_LIST);
		exit(1);
	}
	filename = get_buffer(256);

	fscanf(fp, "%s\n", filename);
	while (*filename != '$') {
		if (!LoadStellarObject(filename))
			mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: Cannot load StellarObject file: %s", filename);
		fscanf(fp, "%s\n", filename);
	}
	fclose(fp);
	release_buffer(filename);
	return;
}

void fread_stellarObject(StellarObject *object, FILE *fp) {
	int num = 0, num2 = 0, num3 = 0;
	char *tag = get_buffer(6);
	char *line = get_buffer(MAX_INPUT_LENGTH + 1);

	while (get_line(fp, line)) {
		tag_argument(line, tag);
		num = atoi(line);

			 if (!strcmp(tag, "Clan"))		object->clan = num;
		else if (!strcmp(tag, "FNam"))		object->filename = str_dup(line);
		else if (!strcmp(tag, "Grav"))		object->gravity = num;
		else if (!strcmp(tag, "Habi"))		object->habitat = (Habitat)num;
		else if (!strcmp(tag, "Flag"))		object->flags = asciiflag_conv(line);
		else if (!strcmp(tag, "LXYZ")) {
			sscanf(line, "%d %d %d", &num, &num2, &num3);
			object->x = num;
			object->y = num2;
			object->z = num3;
		}
		else if (!strcmp(tag, "DXYZ")) {
			sscanf(line, "%d %d %d", &num, &num2, &num3);
			object->dx = num;
			object->dy = num2;
			object->dz = num3;
		}
		else if (!strcmp(tag, "Link"))		object->connects = starsystem_from_name(line);
		else if (!strcmp(tag, "Name"))		object->name = str_dup(line);
		else if (!strcmp(tag, "Size"))		object->size = num;
		else if (!strcmp(tag, "SSys")) {
			object->starsystem = starsystem_from_name(line);
			if (object->starsystem)
				object->starsystem->stellarObjects.push_back(object);
		}
		else if (!strcmp(tag, "Type"))		object->type = (StellarObjectType)num;
		else if (!strcmp(tag, "Zone")) {
			object->zone = num;
			ZoneData *zone = ZoneData::GetZone(object->zone);
			if (zone)
				zone->planet = object;
			else if (object->zone != -1)
				mudlogf(BRF, LVL_STAFF, TRUE, "StellarObject %s has invalid zone #%d.", *object->name ? object->name : "NULL", object->zone);
		}
		else		log("SYSERR: Unknown StellarObject tag %s", tag);
	}
	release_buffer(tag);
	release_buffer(line);
}

bool LoadStellarObject(char *file) {
	FILE *fp;
	RNum clan = -1;
	bool found = FALSE;
	StellarObject *object;
	char *filename = get_buffer(MAX_INPUT_LENGTH);

	sprintf(filename, "%s%s", STELLAROBJ_DIR, file);

	if ((fp = fopen(filename, "r")) != NULL) {
		found = TRUE;

		object = new StellarObject();		
		stellar_objects.push_back(object);
		fread_stellarObject(object, fp);
		fclose(fp);
	}
	release_buffer(filename);
	return found;
}

void load_space(void) {
	FILE *fp;
	char *filename;

	if ((fp = fopen(SPACE_LIST, "r")) == NULL) {
		perror(SPACE_LIST);
		exit(1);
	}
	filename = get_buffer(256);

	fscanf(fp, "%s\n", filename);
	while (*filename != '$') {
		if (!load_starsystem(filename))
			mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: Cannot load starsystem file: %s", filename);
		fscanf(fp, "%s\n", filename);
	}
	fclose(fp);
	release_buffer(filename);
	return;
}


void write_ship_list(void) {
	FILE *fp;

	if (!(fp = fopen(SHIP_LIST, "w"))) {
		mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: Could not open %s for writing.", SHIP_LIST);
		return;
	}

	FOREACH(ShipList, ship_list, iter)
	{
		ShipData *ship = *iter;
		if (ship->type == MOB_SHIP)
			continue;
		if (*ship->owner)
			fprintf(fp, "%s\n", ship->filename);
		else
			mudlogf(BRF, LVL_STAFF, TRUE, "Unwritten ship: %s", *ship->name ? ship->name : "NULL");
	}
	fprintf(fp, "$\n");
	fclose(fp);
}

void ShipData::addsave(void) {
	if (this->type == MOB_SHIP)
		return;
	if (!Lexi::IsInContainer(ship_saves, this))
		ship_saves.push_back(this);
}

void ShipData::remsave(void) {
	ship_saves.remove(this);
}

void ShipData::save(void) {
	FILE *fp;
	char *buf, filename[MAX_INPUT_LENGTH];

	if (!this) {
		mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: save_ship: null pointer");
		return;
	}

	if (IsPurged()) {
		mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: save_ship: trying to save purged ship (%s)", this->name);
		return;
	}

	if (this->type == MOB_SHIP)
		return;

	if (!this->filename || !*this->filename) {
		mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: save_ship: %s has no filename.", this->name);
		return;
	}

	buf = get_buffer(MAX_STRING_LENGTH);
	sprintf(filename, "%s%s", SHIP_DIR, this->filename);

	if ((fp = fopen(filename, "w")) == NULL) {
		mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: save_ship: Cannot open %s for writing.", filename);
		perror(filename);
	} else {
		fprintf(fp, "Name: %s\n", this->name);
		fprintf(fp, "FNam: %s\n", this->filename);

		if (!SHP_FLAGGED(this, SHIP_CUSTOM_ZONE)) {
			FOREACH(Lexi::List<RoomData *>, this->rooms, room)
			{
				Lexi::BufferedFile::PrepareString(buf, (*room)->GetDescription());
				fprintf(fp, "Desc: \n%s~\n", buf);
			}
		}

		fprintf(fp, "Ownr: %s\n", this->owner);
		fprintf(fp, "Pilt: %s\n", this->pilot);
		fprintf(fp, "Copl: %s\n", this->copilot);
		fprintf(fp, "Modl: %d\n", this->model);
		fprintf(fp, "Clas: %d\n", this->shipclass);
		fprintf(fp, "Size: %d\n", this->size);
		fprintf(fp, "Shpy: %d\n", this->shipyard->GetVirtualNumber());
		fprintf(fp, "PrmD: %d\n", this->primdam);
		fprintf(fp, "PrSt: %d\n", this->primstate);
		fprintf(fp, "SecD: %d\n", this->secondam);
		fprintf(fp, "Ldoc: %d\n", this->lastdoc->GetVirtualNumber());
		fprintf(fp, "Shld: %d\n", this->shield);
		fprintf(fp, "MShd: %d\n", this->maxshield);
		fprintf(fp, "Hull: %d\n", this->hull);
		fprintf(fp, "MHul: %d\n", this->maxhull);
		fprintf(fp, "MEnr: %d\n", this->maxenergy);
		fprintf(fp, "HySp: %d\n", this->hyperspeed);
		fprintf(fp, "RlSp: %d\n", this->maxspeed);
		fprintf(fp, "Type: %d\n", this->type);
		fprintf(fp, "ScSt: %d\n", this->secondstate);
		fprintf(fp, "Enrg: %d\n", this->energy);
		fprintf(fp, "Manu: %d\n", this->manuever);
		fprintf(fp, "Armr: %d\n", this->armor);

		sprintbits(this->flags, buf);
		fprintf(fp, "Flag: %s\n", buf);

		if (Clan::GetClan(this->clan))
			fprintf(fp, "Clan: %d\n", this->clan);

		if (this->interceptors)
			fprintf(fp, "Intr: %d\n", this->interceptors);

		if (SHP_FLAGGED(this, SHIP_CUSTOM_ZONE)) {
			Turret *turret;
			Hanger *hanger;

			fprintf(fp, "CEnt: %d\n", this->entrance->GetVirtualNumber());
			fprintf(fp, "CPlt: %d\n", this->pilotseat->GetVirtualNumber());
			fprintf(fp, "CGnr: %d\n", this->gunseat->GetVirtualNumber());
			fprintf(fp, "CVew: %d\n", this->viewscreen->GetVirtualNumber());
			fprintf(fp, "CEng: %d\n", this->engine->GetVirtualNumber());

			FOREACH(Lexi::List<Turret *>, this->turrets, turret) // FOREACH_FIXME
				fprintf(fp, "CTur: %d\n", (*turret)->room->GetVirtualNumber());

			FOREACH(Lexi::List<Hanger *>, this->hangers, hanger) // FOREACH_FIXME
				fprintf(fp, "CHng: %d %d\n", (*hanger)->room->GetVirtualNumber(), (*hanger)->capacity);

			fprintf(fp, "CRms: %d %d\n", this->rooms.Top()->GetVirtualNumber(), this->rooms.Bottom()->GetVirtualNumber());
		}

		fprintf(fp, "Upgr: %d %d %d %d %d\n",
			this->upgrades[0], this->upgrades[1],
			this->upgrades[2], this->upgrades[3], this->upgrades[4]);
		fprintf(fp, "Home: %s\n", this->home);
	}
	fclose(fp);
	release_buffer(buf);
	return;
}

RoomData * AssignRoom(ShipData *ship, VNum vnum) {
	RoomData * room = World::GetRoomByVNum(vnum);

	if (room)	room->ship = ship;

	SET_BIT(SHP_FLAGS(ship), SHIP_CUSTOM_ZONE);

	return room;
}

bool ShipData::load(FILE *fp) {
	int dIndex = 0;
	int num = 0, num2 = 0, num3 = 0, num4 = 0, num5 = 0;
	char *tag = get_buffer(6);
	char *line = get_buffer(MAX_INPUT_LENGTH + 1);

	while (get_line(fp, line)) {
		tag_argument(line, tag);
		num = atoi(line);

			 if (!strcmp(tag, "Armr"))		this->armor = num;
		else if (!strcmp(tag, "Clas"))		this->shipclass = num;
		else if (!strcmp(tag, "Clan"))		{
			if (Clan::GetClan(num))
				this->clan = num;
		}
		else if (!strcmp(tag, "Copl"))		this->copilot = str_dup(line);
		else if (!strcmp(tag, "Desc"))		{
			if (dIndex < MAX_SHIP_ROOMS)
				this->description[dIndex++] = fread_string(fp, line, "ship desc");
		}
		else if (!strcmp(tag, "Enrg"))		this->energy = num;
		else if (!strcmp(tag, "FNam"))		this->filename = str_dup(line);
		else if (!strcmp(tag, "HySp"))		this->hyperspeed = num;
		else if (!strcmp(tag, "Hull"))		this->hull = num;
		else if (!strcmp(tag, "Home"))		this->home = str_dup(line);
		else if (!strcmp(tag, "PrmD"))		this->primdam = num;
		else if (!strcmp(tag, "PrSt"))		this->primstate = num;
		else if (!strcmp(tag, "Ldoc"))		this->lastdoc = World::GetRoomByVNum(num);
		else if (!strcmp(tag, "Modl"))		this->model = num;
		else if (!strcmp(tag, "Manu"))		this->manuever = num;
		else if (!strcmp(tag, "SecD"))		this->secondam = num;
		else if (!strcmp(tag, "ShSt"))		;
		else if (!strcmp(tag, "MShd"))		this->maxshield = num;
		else if (!strcmp(tag, "MEnr"))		this->maxenergy = num;
		else if (!strcmp(tag, "MHul"))		this->maxhull = num;
		else if (!strcmp(tag, "ScSt"))		this->secondstate = num;
		else if (!strcmp(tag, "Name"))		this->name = str_dup(line);
		else if (!strcmp(tag, "Ownr"))		this->owner = str_dup(line);
		else if (!strcmp(tag, "Pilt"))		this->pilot = str_dup(line);
		else if (!strcmp(tag, "RlSp"))		this->maxspeed = num;
		else if (!strcmp(tag, "Size"))		this->size = num;
		else if (!strcmp(tag, "Shpy"))		this->shipyard = World::GetRoomByVNum(num);
		else if (!strcmp(tag, "Shld"))		this->shield = num;
		else if (!strcmp(tag, "Type"))		this->type = num;
		else if (!strcmp(tag, "Flag"))		this->flags = asciiflag_conv(line);
		else if (!strcmp(tag, "Intr"))		this->interceptors = num;
//		Stuff for custom ships
//		Pilot, Gunner, Viewscreen, Engine, Entrance, Low-High
		else if (!strcmp(tag, "CEnt"))		this->entrance = AssignRoom(this, num);
		else if (!strcmp(tag, "CPlt"))		this->pilotseat = AssignRoom(this, num);
		else if (!strcmp(tag, "CGnr"))		this->gunseat = AssignRoom(this, num);
		else if (!strcmp(tag, "CVew"))		this->viewscreen = AssignRoom(this, num);
		else if (!strcmp(tag, "CEng"))		this->engine = AssignRoom(this, num);
		else if (!strcmp(tag, "CHng")) {
			Hanger *hanger;
			sscanf(line, "%d %d", &num, &num2);
			CREATE(hanger, Hanger, 1);
			hanger->room = World::GetRoomByVNum(num);
			hanger->bayopen = TRUE;
			hanger->capacity = num2;
			this->hangers.push_front(hanger);
		}
		else if (!strcmp(tag, "CTur")) {
			Turret *turret;
			CREATE(turret, Turret, 1);
			turret->room = World::GetRoomByVNum(num);
			turret->target = NULL;
			turret->laserstate = 0;
			this->turrets.push_front(turret);
		}
		else if (!strcmp(tag, "CRms")) {
			sscanf(line, "%d %d", &num, &num2);
			for (int i = num; i <= num2; i++) {
				RoomData *room = World::GetRoomByVNum(i);
				if (room)
				{
					room->ship = this;
					this->rooms.Append(room);
				}
			}
		}
		else if (!strcmp(tag, "Upgr"))		{
			sscanf(line, "%d %d %d %d %d", &num, &num2, &num3, &num4, &num5);
			this->upgrades[0] = num;
			this->upgrades[1] = num2;
			this->upgrades[2] = num3;
			this->upgrades[3] = num4;
			this->upgrades[4] = num5;
		}
		else		log("SYSERR: (#%d) Unknown ship tag %s", 1, tag);
	}
	this->CreateRooms();
	release_buffer(tag);
	release_buffer(line);
	return TRUE;
}


bool load_ship_file(char *shipfile) {
	FILE *fp;
	int Id;
	ShipData *ship;
	bool found = false, ignore = false;
	char *filename = get_buffer(MAX_INPUT_LENGTH);

	sprintf(filename, "%s%s", SHIP_DIR, shipfile);

	if ((fp = fopen(filename, "r")) != NULL) {
		found = TRUE;

		ship = new ShipData();
		ship_list.Append(ship);
		ship->SetID(max_id++);
		ship->load(fp);
		fclose(fp);

		if (ship->shipclass == SPACE_STATION || ship->type == MOB_SHIP) {
			ship->speed		= 0;
			ship->currjump		= NULL;
			ship->target		= NULL;
			ship->energy		= ship->maxenergy;
			ship->hull				= ship->maxhull;
			ship->shield		= ship->maxshield;

			ship->primstate		= LASER_READY;
			ship->secondstate		= MISSILE_READY;
			ship->bayopen		= ship->type == MOB_SHIP ? FALSE : TRUE;
			ship->hatchopen		= FALSE;

			ship->ToSystem(starsystem_from_name(ship->home));
			ship->vx = Number(-50000, 50000);
			ship->vy = Number(-50000, 50000);
			ship->vz = Number(-50000, 50000);
			ship->hx = ship->hy = ship->hz = 1;
			ship->state = SHIP_READY;
			SET_BIT(SHP_FLAGS(ship), SHIP_AUTOPILOT);
		} else {
			if (!str_cmp("Public", ship->owner)) {
				// Return public ships to original place!
				ship->ToRoom(ship->shipyard);
			}
			else if (ship->lastdoc)
				ship->ToRoom(ship->lastdoc);
			else
				ship->ToRoom(ship->shipyard);
			
			ship->location = ship->lastdoc = ship->InRoom();
		}
	}

	for (RNum clan = 0; clan <= Clan::TopOfIndex; clan++) {
		if (is_number(ship->owner) && atoi(ship->owner) == Clan::Index[clan]->GetVirtualNumber()) {
			ignore = true;
			break;
		}
	}

	if (!ignore && ship->type != MOB_SHIP && str_cmp("Public", ship->owner) && (Id = get_id_by_name(ship->owner)) == NoID) {
		RoomData * room;
		ObjData *obj;

		FOREACH(RoomList, ship->rooms, room) // FOREACH_FIXME
		{
			room->ship = NULL;
			World::RemoveRoom(room);

			FOREACH(ObjList, room->contents, obj) // FOREACH_FIXME
				obj->Extract();
		}
		log("   Deleting: %s", ship->name);
		sprintf(filename, "%s%s", SHIP_DIR, ship->filename);
		ship->Extract();
		unlink(filename);
	}

	release_buffer(filename);
	return found;
}

void load_ships(void) {
	FILE *fp;
	char *filename;

	if ((fp = fopen(SHIP_LIST, "r")) == NULL) {
		perror(SHIP_LIST);
		exit(1);
	}
	filename = get_buffer(256);

	fscanf(fp, "%s\n", filename);
	while (*filename != '$') {
		if (!load_ship_file(filename))
			mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: Couldn't load ship file: %s", filename);
		fscanf(fp, "%s\n", filename);
	}
	fclose(fp);
	write_ship_list();				// after cleanup, write it out
//	space_control_update();
	release_buffer(filename);
	return;
}

void ShipData::Reset(void) {
	this->state = SHIP_READY;

	if (this->shipclass != SPACE_STATION && this->type != MOB_SHIP) {
		this->Extract();
		this->ToRoom(this->lastdoc);
		this->location = this->lastdoc;
		this->state = SHIP_DOCKED;
		FREE(this->home);
		this->home = str_dup("");
	}

	this->FromSystem();

	this->shield		= 0;
	this->speed		= 0;
	this->hull				= this->maxhull;
	this->energy		= this->maxenergy;

	this->primstate		= LASER_READY;
	this->secondstate		= MISSILE_READY;

	this->target		= NULL;
	this->currjump		= NULL;
	this->bayopen		= FALSE;
	this->hatchopen		= FALSE;
	this->addsave();
}

void bridge_doors(RoomData * from, RoomData * to, int dir) {
   if (!from->dir_option[dir])
		from->dir_option[dir] = new RoomDirection;
   from->dir_option[dir]->room = to;
   from->dir_option[dir]->flags = EX_ISDOOR;

	if (!to->dir_option[rev_dir[dir]])
		to->dir_option[rev_dir[dir]] = new RoomDirection;
	to->dir_option[rev_dir[dir]]->room = from;
	to->dir_option[rev_dir[dir]]->flags = EX_ISDOOR;
}

void bridge_rooms(RoomData * from, RoomData * to, int dir) {
	if (!from->dir_option[dir])
		from->dir_option[dir] = new RoomDirection;
	from->dir_option[dir]->room = to;

	if (!to->dir_option[rev_dir[dir]])
		to->dir_option[rev_dir[dir]] = new RoomDirection;
	to->dir_option[rev_dir[dir]]->room = from;
}

void make_room(ShipData *ship, RoomData *room, int type) {
	ObjData *obj;
	Turret *turret = NULL;
	Hanger *hanger = NULL;

	if (room == NULL) {
		mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: make_room: Room is NOWHERE");
		return;
	}

	switch (type) {
		case RTYPE_PILOT:
			room->SetName("The Pilot's Chair");
			room->SetDescription("   The console in front of you contains the spacecrafts primary flight\n"
						"controls. All of the ships propulsion and directional controls are located\n"
						"here including the hyperdrive controls.\n");
			ship->pilotseat = room;
			obj = read_object(SHP_OBJ_PILOTCHAIR, VIRTUAL);
			if (obj)	obj->ToRoom(room);
			break;

		case RTYPE_COPILOT:
			room->SetName("The Gunner's Chair");
			room->SetDescription("   This is where the action is. All of the systems main offensive and defensive\n"
						"controls are located here. There are targetting computers for the main laser\n"
						"batteries and missile launchers as well as shield controls.\n");
			ship->gunseat = room;
			obj = read_object(SHP_OBJ_GUNNERCHAIR, VIRTUAL);
			if (obj)	obj->ToRoom(room);
			break;

		case RTYPE_BRIDGE:
			room->SetName("The Bridge");
			room->SetDescription("   A large panoramic viewscreen gives you a good view of everything\n"
						"outside of the ship. Several computer terminals provide information\n"
						"and status readouts. There are also consoles dedicated to scanning\n"
						"communications and navigation.\n");
			ship->viewscreen = room;
			obj = read_object(SHP_OBJ_VIEWSCREEN, VIRTUAL);
			if (obj)	obj->ToRoom(room);
			break;

		case RTYPE_ENGINE:
			room->SetName("The Engine Room");
			room->SetDescription("   Many large mechanical parts litter this room. Throughout the room dozens\n"
						"of small glowing readouts and computer terminals monitor and provide information\n"
						"about all of the ships systems. Engineering is the heart of the ship. If\n"
						"something is damaged fixing it will almost always start here.\n");
			ship->engine = room;
			obj = read_object(SHP_OBJ_ENGINE, VIRTUAL);
			if (obj)	obj->ToRoom(room);
			break;

		case RTYPE_ENTRANCE:
			room->SetName("The Entrance Ramp");
			room->SetDescription("   Large hydraulic doors provide an entry and exit point to the ship.\n");
			ship->entrance = room;
			obj = read_object(SHP_OBJ_EXITHATCH, VIRTUAL);
			if (obj)	obj->ToRoom(room);
			break;

		case RTYPE_TURRET:
			room->SetName("Gun Turret");
			room->SetDescription("   This turbo laser turret extends from the outter hull of the ship.\n"
						"It is more powerful than the ships main laser batteries and must\n"
						"be operated manually by a trained gunner.\n");
			CREATE(turret, Turret, 1);
			turret->room = room;
			turret->target = NULL;
			turret->laserstate = 0;
			ship->turrets.push_front(turret);
			obj = read_object(SHP_OBJ_GUNTURRET, VIRTUAL);
			if (obj)	obj->ToRoom(room);
			break;

		case RTYPE_GARAGE:
			room->SetName("Vehicle Garage");
			room->SetDescription("   This garage is reserved for non-spacecraft land vehicles.\n");
			ship->garages.Append(room);
			break;

		case RTYPE_DEPLOYMENT:
			room->SetName("Deployment Chamber");
			room->SetDescription("  This is where Orbital Insertion Troops are dropped.\n");
			ship->deployment = room;
			break;

		case RTYPE_HANGER:
			room->SetName("Spacecraft Hanger");
			room->SetDescription("   This hanger is reserved for spacecraft.\n");
			CREATE(hanger, Hanger, 1);
			hanger->room = room;
			hanger->capacity = 5;
//			hanger->capacity = ship->size + 1;
			hanger->bayopen = FALSE;
			ship->hangers.push_front(hanger);
			break;

		case RTYPE_LOUNGE:
			room->SetName("Crew Lounge");
			room->SetDescription("   The crew lounge needs to be furnished still.\n");
			obj = read_object(SHP_OBJ_USENET, VIRTUAL);
			if (obj)	obj->ToRoom(room);
			break;

		case RTYPE_COCKPIT:
			ship->gunseat = room;
			ship->pilotseat = room;
			ship->viewscreen = room;
			room->SetName("The Cockpit");
			room->SetDescription("   This small cockpit houses the pilot controls as well as the ships main\n"
						"offensive and defensive systems.\n");
			break;

		case RTYPE_MEDICAL:
			room->SetName("Medical Bay");
			room->SetDescription("   This medical bay is out of order.\n");
			obj = read_object(SHP_OBJ_MEDICAL_BED, VIRTUAL);
			if (obj)	obj->ToRoom(room);
			break;

		case RTYPE_TROOPHOLD:
			room->SetName("Troop Hold");
			room->SetDescription("   This area is for the troops to gather before deployment.\n");
			break;

		default:		// ehh wtf
			break;
	}
}

void remove_interceptors(ShipData *mothership) {
	Turret *turret;
	ShipData *ship, *tship;

	FOREACH(ShipList, mothership->fighters, ship) // FOREACH_FIXME
	{
		echo_to_ship(TO_COCKPIT, ship, "`Y%s flies back to your hangers.`n", ship->name);
		echo_to_system(ship, NULL, "`Y%s flies back to %s.`n", ship->name, mothership->name);

		FOREACH(ShipList, ship_list, tship) // FOREACH_FIXME
		{
			if (tship->target == ship) {
				tship->SetTarget(NULL);
				remove_interceptors(tship);
			}
			FOREACH(TurretList, tship->turrets, turret) // FOREACH_FIXME
				if (turret->target == ship)
					turret->target = NULL;
		}

		if (ship->starsystem)
			ship->FromSystem();

		if (ship->mothership)
			ship->mothership->fighters.remove(ship);

		ship->Extract();
	}
}

void destroy_interceptors(ShipData *mothership, CharData *killer) {
	ShipData *ship;

	FOREACH(ShipList, mothership->fighters, ship) // FOREACH_FIXME
		ship->Destroy(killer);
}

void deploy_interceptors(ShipData *mothership) {
	ShipData *ship;

	if (!mothership || mothership->model != CARRIER || mothership->IsPurged())
		return;

	while (mothership->fighters.Count() < mothership->interceptors) {
		if ((ship = make_interceptor(mothership))) {
			echo_to_ship(TO_COCKPIT, mothership, "`YAn Interceptor flies out of the ship's hanger.`n");
			echo_to_system(mothership, NULL, "`YAn Interceptor flies out of a hanger on %s.`n", mothership->name);
		}
	}
}

ShipData *make_interceptor(ShipData *mothership) {
	char *buf;
	int id = 0;
	ShipData *ship;

	if (!mothership || !mothership->starsystem)
		return NULL;

	FOREACH(ShipList, ship_list, ship) // FOREACH_FIXME
		if (id > atoi(ship->filename))
			id = atoi(ship->filename);

	id--;
	buf = get_buffer(MAX_STRING_LENGTH);

	ship = new ShipData();
	ship_list.Append(ship);
	ship->SetID(max_id++);
	mothership->fighters.Append(ship);

	sprintf(buf, "%s i%d (%s's %s)", model_list[INTERCEPTOR].name, 0-id,
		mothership->owner, model_list[mothership->model].name);
	ship->name = str_dup(buf);
	sprintf(buf, "%d.shp", id);
	ship->filename = str_dup(buf);

	ship->model				= INTERCEPTOR;
	ship->clan				= mothership->clan;
	ship->type				= MOB_SHIP;
	ship->shipclass		= SPACECRAFT;

	ship->dest				= NULL;
	ship->target		= mothership->target;
	ship->currjump		= NULL;
	ship->starsystem		= NULL;
	ship->following		= mothership;
	ship->mothership		= mothership;
	ship->home				= str_dup(mothership->starsystem->name);
	ship->owner				= str_dup(mothership->name);
	ship->pilot				= str_dup(mothership->owner);
	ship->copilot		= str_dup(mothership->pilot);		// shrug

	ship->size				= model_list[INTERCEPTOR].size;
	ship->flags				= model_list[INTERCEPTOR].flags;
	ship->armor				= model_list[INTERCEPTOR].armor;
	ship->maxhull		= model_list[INTERCEPTOR].hull;
	ship->primdam		= model_list[INTERCEPTOR].primdam;
	ship->secondam		= model_list[INTERCEPTOR].secondam;
	ship->manuever		= model_list[INTERCEPTOR].manuever;
	ship->maxshield		= model_list[INTERCEPTOR].shield;
	ship->maxenergy		= model_list[INTERCEPTOR].energy;
	ship->maxspeed		= model_list[INTERCEPTOR].maxspeed;
	ship->hyperspeed		= model_list[INTERCEPTOR].hyperspeed;

	ship->SetInRoom(NULL);
	ship->lastdoc		= NULL;
	ship->location		= NULL;
	ship->state 		= SHIP_READY;
	ship->primstate		= LASER_READY;
	ship->secondstate	= MISSILE_READY;
	for (int i = 0; i < 5; ++i)
		ship->upgrades[i]	= mothership->upgrades[i];

	ship->hull				= ship->maxhull;
	ship->energy		= ship->maxenergy;
	ship->shield		= ship->maxshield;

	for (int i = 0; i < MAX_SHIP_ROOMS; i++)
		ship->description[i] = NULL;

	ship->ToSystem(mothership->starsystem);
	ship->vx = mothership->vx + Number(-100, 100);
	ship->vy = mothership->vy + Number(-100, 100);
	ship->vz = mothership->vz + Number(-100, 100);
	release_buffer(buf);
	return ship;
}

ShipData *make_mob_ship(StellarObject *planet, int type) {
	char *buf;
	int i, id = 0;
	Clan *clan;
	ShipData *ship;

	if (!planet || (clan = Clan::GetClan(planet->clan)) == NULL || !planet->starsystem)
		return NULL;

	FOREACH(ShipList, ship_list, ship) // FOREACH_FIXME
		if (id > atoi(ship->filename))
			id = atoi(ship->filename);

	id--;
	buf = get_buffer(MAX_STRING_LENGTH);

	ship = new ShipData();
	ship_list.Append(ship);
	ship->SetID(max_id++);

	sprintf(buf, "%s m%d (%s)", model_list[type].name, 0-id, strip_color(clan->GetName()));
	ship->name = str_dup(buf);
	sprintf(buf, "%d.shp", id);
	ship->filename = str_dup(buf);

	ship->model				= type;
	ship->clan				= clan->GetVirtualNumber();
	ship->type				= MOB_SHIP;
	ship->shipclass		= model_list[type].shipclass;

	ship->dest				= NULL;
	ship->target		= NULL;
	ship->currjump		= NULL;
	ship->starsystem		= NULL;
	ship->mothership		= NULL;
	ship->home				= str_dup(planet->starsystem->name);
	sprintf(buf, "%d", clan->GetVirtualNumber());
	ship->owner				= /*str_dup(strip_color(CLANNAME(clan)))*/ str_dup(buf);
	ship->pilot				= str_dup("");
	ship->copilot		= str_dup("");

	ship->size				= model_list[type].size;
	ship->flags				= model_list[type].flags;
	ship->armor				= model_list[type].armor;
	ship->maxhull		= model_list[type].hull;
	ship->primdam		= model_list[type].primdam;
	ship->secondam		= model_list[type].secondam;
	ship->manuever		= model_list[type].manuever;
	ship->maxshield		= model_list[type].shield;
	ship->maxenergy		= model_list[type].energy;
	ship->maxspeed		= model_list[type].maxspeed;
	ship->hyperspeed		= model_list[type].hyperspeed;
	ship->SetInRoom(NULL);
	ship->lastdoc		= NULL;
	ship->location 		= NULL;
	ship->state				= SHIP_READY;
	ship->primstate		= LASER_READY;
	ship->secondstate		= MISSILE_READY;

	ship->hull				= ship->maxhull;
	ship->energy		= ship->maxenergy;
	ship->shield		= ship->maxshield;

	if (ship->model == CARRIER)
		ship->interceptors = 4;

	for (i = 0; i < MAX_SHIP_ROOMS; i++)
		ship->description[i] = NULL;

	ship->ToSystem(planet->starsystem);
	ship->vx = planet->x + Number(-2000, 2000);
	ship->vy = planet->y + Number(-2000, 2000);
	ship->vz = planet->z + Number(-2000, 2000);
	echo_to_system(ship, NULL, "`Y%s enters the starsystem at %.0f %.0f %.0f`n", ship->name, ship->vx, ship->vy, ship->vz);
	release_buffer(buf);
	return ship;
}

ShipData *make_ship(int type) {
	char *buf;
	int i, id = 0;
	ShipData *ship;

	if (model_list[type].name == NULL)
		return NULL;

	FOREACH(ShipList, ship_list, ship) // FOREACH_FIXME
		if (id < atoi(ship->filename))
			id = atoi(ship->filename);

	buf = get_buffer(MAX_STRING_LENGTH);

	id++;
	ship = new ShipData();
	ship_list.Append(ship);
	ship->SetID(max_id++);

	sprintf(buf, "%s %d", model_list[type].name, id);
	ship->name = str_dup(buf);
	sprintf(buf, "%d.shp", id);
	ship->filename = str_dup(buf);

	ship->dest				= NULL;
	ship->target		= NULL;
	ship->currjump		= NULL;
	ship->starsystem		= NULL;
	ship->mothership		= NULL;
	ship->home				= str_dup("");
	ship->owner				= str_dup("Public");
	ship->pilot				= str_dup("");
	ship->copilot		= str_dup("");

	ship->clan				= -1;
	ship->type				= PLAYER_SHIP;
	ship->model				= type;
	ship->shipclass		= model_list[type].shipclass;
	ship->size				= model_list[type].size;
	ship->flags				= model_list[type].flags;
	ship->armor				= model_list[type].armor;
	ship->maxhull		= model_list[type].hull;
	ship->primdam		= model_list[type].primdam;
	ship->secondam		= model_list[type].secondam;
	ship->manuever		= model_list[type].manuever;
	ship->maxshield		= model_list[type].shield;
	ship->maxenergy		= model_list[type].energy;
	ship->maxspeed		= model_list[type].maxspeed;
	ship->hyperspeed		= model_list[type].hyperspeed;

	ship->SetInRoom(NULL);
	ship->lastdoc		= NULL;
	ship->location		= NULL;
	ship->state				= SHIP_DOCKED;
	ship->primstate		= LASER_READY;
	ship->secondstate		= MISSILE_READY;

	ship->hull				= ship->maxhull;
	ship->energy		= ship->maxenergy;
	ship->shield		= ship->maxshield;

	for (i = 0; i < MAX_SHIP_ROOMS; i++)
		ship->description[i] = NULL;

	ship->CreateRooms();
	ship->save();
	write_ship_list();
	release_buffer(buf);
	return ship;
}


RoomData * make_ship_room(ShipData *ship)
{
	RoomData *room = World::GetFreeRoom();

	room->ship				= ship;
	room->SetName(ship->name);
	room->SetDescription("  Nothing.\n");
	room->sector_type		= SECT_INSIDE;

	SET_BIT(room->flags, ROOM_INDOORS | ROOM_SPACECRAFT);
	
	ship->rooms.Append(room);
	
	return room;
}

void ShipData::CreateRooms(void) {
	RoomData * room[MAX_SHIP_ROOMS - 1];
	int roomnum, numrooms;

	numrooms = UMIN(model_list[this->model].rooms, MAX_SHIP_ROOMS - 1);

	for (roomnum = 0; roomnum < numrooms; roomnum++)
		room[roomnum] = make_ship_room(this);

	switch (this->model) {
		case SCOUT:
		case WRAITH:
		case INFWRAITH:
		case CORSAIR:
		case VALKYRIE:
		case SIEGE_TANK:
		case REAVER:
			this->pilotseat		= room[0];
			this->gunseat		= room[0];
			this->viewscreen		= room[0];
			this->entrance		= room[0];
			this->engine		= room[0];
			break;

		case QUEEN:
		case OVERLORD:
		case GUARDIAN:
			this->pilotseat= room[0];
			this->gunseat= room[0];
			this->viewscreen= room[0];
			this->entrance= room[0];
			this->engine= room[0];
			room[0]->SetDescription("	This is an organic zerg ship, ignore that it flies like a ship and has\n"
						 "mechanical gauges.  Maybe Seishin will make it feel more organic some day.\n");
			break;

		case ATV:
			make_room(this, room[0], RTYPE_COCKPIT);
			make_room(this, room[1], RTYPE_TROOPHOLD);
			make_room(this, room[2], RTYPE_ENGINE);
			make_room(this, room[3], RTYPE_ENTRANCE);
			bridge_rooms(room[1], room[0], DIR_NORTH);
			bridge_rooms(room[1], room[2], DIR_SOUTH);
			bridge_rooms(room[1], room[3], DIR_EAST);
			break;

		case SHUTTLE:
		case DROPSHIP:
			make_room(this, room[0], RTYPE_COCKPIT);
			make_room(this, room[1], RTYPE_ENTRANCE);
			make_room(this, room[2], RTYPE_DEPLOYMENT);
			make_room(this, room[3], RTYPE_GARAGE);
			make_room(this, room[4], RTYPE_ENGINE);
			bridge_rooms(room[1], room[0], DIR_NORTH);
			bridge_doors(room[1], room[2], DIR_WEST);
			bridge_rooms(room[1], room[3], DIR_EAST);
			bridge_rooms(room[1], room[4], DIR_SOUTH);
			break;

		case ARBITER:		// frigate
			make_room(this, room[0], RTYPE_PILOT);
			make_room(this, room[1], RTYPE_BRIDGE);
			make_room(this, room[2], RTYPE_COPILOT);
			make_room(this, room[3], RTYPE_TURRET);
			make_room(this, room[5], RTYPE_TURRET);
			make_room(this, room[6], RTYPE_ENTRANCE);
			make_room(this, room[8], RTYPE_MEDICAL);
			make_room(this, room[9], RTYPE_ENGINE);
			bridge_rooms(room[0], room[1], DIR_EAST);
			bridge_rooms(room[1], room[2], DIR_EAST);
			bridge_rooms(room[3], room[4], DIR_EAST);
			bridge_rooms(room[4], room[5], DIR_EAST);
			bridge_rooms(room[7], room[8], DIR_EAST);
			bridge_rooms(room[6], room[7], DIR_EAST);
			bridge_rooms(room[1], room[4], DIR_SOUTH);
			bridge_rooms(room[4], room[7], DIR_SOUTH);
			bridge_rooms(room[7], room[9], DIR_SOUTH);
			break;

		case SCIENCEVESSEL:		// corvette
			make_room(this, room[0], RTYPE_PILOT);
			make_room(this, room[1], RTYPE_BRIDGE);
			make_room(this, room[2], RTYPE_COPILOT);
			make_room(this, room[3], RTYPE_ENTRANCE);
			make_room(this, room[5], RTYPE_ENGINE);
			make_room(this, room[6], RTYPE_HANGER);
			make_room(this, room[7], RTYPE_GARAGE);
			bridge_rooms(room[0], room[1], DIR_EAST);
			bridge_rooms(room[1], room[2], DIR_EAST);
			bridge_rooms(room[1], room[3], DIR_SOUTH);
			bridge_rooms(room[3], room[4], DIR_SOUTH);
			bridge_rooms(room[4], room[5], DIR_SOUTH);
			bridge_rooms(room[4], room[6], DIR_WEST);
			bridge_rooms(room[4], room[7], DIR_EAST);
			break;

		case INFBATTLESHIP:		// destroyer
			make_room(this, room[0], RTYPE_PILOT);
			make_room(this, room[1], RTYPE_BRIDGE);
			make_room(this, room[2], RTYPE_COPILOT);
			make_room(this, room[3], RTYPE_TURRET);
			make_room(this, room[5], RTYPE_TURRET);
			make_room(this, room[6], RTYPE_HANGER);
			make_room(this, room[8], RTYPE_GARAGE);
			make_room(this, room[9], RTYPE_ENGINE);
			make_room(this, room[10], RTYPE_ENTRANCE);
			make_room(this, room[11], RTYPE_LOUNGE);
			bridge_rooms(room[0], room[1] , DIR_EAST);
			bridge_rooms(room[1], room[2] , DIR_EAST);
			bridge_rooms(room[3], room[4] , DIR_EAST);
			bridge_rooms(room[4], room[5] , DIR_EAST);
			bridge_rooms(room[7], room[8] , DIR_EAST);
			bridge_rooms(room[6], room[7] , DIR_EAST);
			bridge_rooms(room[1], room[4] , DIR_SOUTH);
			bridge_rooms(room[4], room[7] , DIR_SOUTH);
			bridge_rooms(room[7], room[9] , DIR_SOUTH);
			bridge_rooms(room[7], room[10], DIR_DOWN);
			bridge_rooms(room[7], room[11], DIR_UP);
			break;

		case BATTLECRUISER:		// flagship
			make_room(this, room[0] , RTYPE_PILOT);
			make_room(this, room[1] , RTYPE_BRIDGE);
			make_room(this, room[2] , RTYPE_COPILOT);
			make_room(this, room[3] , RTYPE_MEDICAL);
			make_room(this, room[5] , RTYPE_LOUNGE);
			make_room(this, room[7] , RTYPE_ENTRANCE);
			make_room(this, room[8] , RTYPE_HANGER);
			make_room(this, room[14], RTYPE_HANGER);
			make_room(this, room[15], RTYPE_TURRET);
			make_room(this, room[16], RTYPE_TURRET);
			make_room(this, room[17], RTYPE_TURRET);
			make_room(this, room[18], RTYPE_TURRET);
			make_room(this, room[19], RTYPE_GARAGE);
			make_room(this, room[20], RTYPE_ENGINE);
			make_room(this, room[21], RTYPE_GARAGE);
			make_room(this, room[22], RTYPE_TURRET);
			make_room(this, room[23], RTYPE_TURRET);
			bridge_rooms(room[0] , room[1] , DIR_EAST );
			bridge_rooms(room[1] , room[2] , DIR_EAST );
			bridge_rooms(room[3] , room[4] , DIR_EAST );
			bridge_rooms(room[4] , room[5] , DIR_EAST );
			bridge_rooms(room[8] , room[9] , DIR_EAST );
			bridge_rooms(room[9] , room[10], DIR_EAST );
			bridge_rooms(room[10], room[11], DIR_EAST );
			bridge_rooms(room[11], room[12], DIR_EAST );
			bridge_rooms(room[12], room[13], DIR_EAST );
			bridge_rooms(room[13], room[14], DIR_EAST );
			bridge_rooms(room[1] , room[4] , DIR_SOUTH );
			bridge_rooms(room[4] , room[6] , DIR_SOUTH );
			bridge_rooms(room[6] , room[7] , DIR_SOUTH );
			bridge_rooms(room[7] , room[11], DIR_SOUTH );
			bridge_rooms(room[11], room[20], DIR_SOUTH );
			bridge_rooms(room[15], room[9] , DIR_SOUTH );
			bridge_rooms(room[9] , room[16], DIR_SOUTH );
			bridge_rooms(room[10], room[19], DIR_SOUTH );
			bridge_rooms(room[17], room[13], DIR_SOUTH );
			bridge_rooms(room[13], room[18], DIR_SOUTH );
			bridge_rooms(room[12], room[21], DIR_SOUTH );
			break;

		case CARRIER:		// original design
			make_room(this, room[0] , RTYPE_PILOT);
			make_room(this, room[1] , RTYPE_BRIDGE);
			make_room(this, room[2] , RTYPE_COPILOT);
			make_room(this, room[5] , RTYPE_LOUNGE);
			make_room(this, room[6] , RTYPE_MEDICAL);
			make_room(this, room[9] , RTYPE_TURRET);
			make_room(this, room[10], RTYPE_TURRET);
			make_room(this, room[11], RTYPE_ENTRANCE);
			make_room(this, room[12], RTYPE_ENGINE);
			make_room(this, room[13], RTYPE_GARAGE);
			make_room(this, room[14], RTYPE_GARAGE);
			make_room(this, room[16], RTYPE_HANGER);
			make_room(this, room[17], RTYPE_HANGER);
			make_room(this, room[19], RTYPE_HANGER);
			make_room(this, room[20], RTYPE_HANGER);
			make_room(this, room[22], RTYPE_HANGER);
			make_room(this, room[23], RTYPE_HANGER);
			bridge_rooms(room[0] , room[1] , DIR_EAST);
			bridge_rooms(room[1] , room[2] , DIR_EAST);
			bridge_rooms(room[1] , room[3] , DIR_SOUTH);
			bridge_rooms(room[3] , room[4] , DIR_SOUTH);
			bridge_rooms(room[3] , room[7] , DIR_DOWN);
			bridge_rooms(room[4] , room[5] , DIR_WEST);
			bridge_rooms(room[4] , room[6] , DIR_EAST);
			bridge_rooms(room[7] , room[8] , DIR_NORTH);
			bridge_rooms(room[7] , room[11], DIR_DOWN);
			bridge_rooms(room[8] , room[9] , DIR_WEST);
			bridge_rooms(room[8] , room[10], DIR_EAST);
			bridge_rooms(room[11], room[12], DIR_SOUTH);
			bridge_rooms(room[11], room[13], DIR_WEST);
			bridge_rooms(room[11], room[14], DIR_EAST);
			bridge_rooms(room[11], room[15], DIR_NORTH);
			bridge_rooms(room[15], room[16], DIR_WEST);
			bridge_rooms(room[15], room[17], DIR_EAST);
			bridge_rooms(room[15], room[18], DIR_NORTH);
			bridge_rooms(room[18], room[19], DIR_WEST);
			bridge_rooms(room[18], room[20], DIR_EAST);
			bridge_rooms(room[18], room[21], DIR_NORTH);
			bridge_rooms(room[21], room[22], DIR_WEST);
			bridge_rooms(room[21], room[23], DIR_EAST);
			break;

		case CUSTOM_SHIP:
			break;				// do nothing

		default:
			this->pilotseat	 = room[0];
			this->gunseat	   = room[0];
			this->viewscreen	= room[0];
			this->entrance	  = room[0];
			this->engine		= room[0];
			room[0]->SetDescription(
				"Here is a brief flying lesson:\n"
				"\n"
				"Use LAUNCH to lift off.\n"
				"Use CALCULATE to select a location for your hyperjump.\n"
				"Use HYPERSPACE to execute it.\n"
				"After your jump use RADAR to survey the starsystem.\n"
				"Use COURSE to change your direction of travel.\n"
				"When you are close enough to a planet use LAND.\n"
				"\n"
				"Type HELP SHIPS for a full list of ship commands.\n");
			break;
	}
}

ACMD(do_openhatch) {
	ShipData *ship;

	skip_spaces(argument);
	if (!*argument || !str_cmp(argument, "hatch")) {
		if ((ship = ship_from_entrance(IN_ROOM(ch))) == NULL) {
			ch->Send("%s what?\n", cmd_door[subcmd]);
			return;
		} else {
			if (subcmd == SCMD_OPEN && !ship->hatchopen) {
				if (ship->shipclass == SPACE_STATION) {
					ch->Send("Try one of the docking bays!\n");
					return;
				}
				if (ship->location != ship->lastdoc
				|| ship->state != SHIP_DOCKED) {
					ch->Send("Please wait until the ship lands!\n");
					return;
				}
				ship->hatchopen = TRUE;
				ch->Send("You open the hatch.\n");
				act("$n opens the hatch.", TRUE, ch, 0, 0, TO_ROOM);
				IN_ROOM(ship)->Send("The hatch on %s opens.\n", ship->name);
				return;
			} else if (subcmd == SCMD_CLOSE && ship->hatchopen) {
				if (ship->shipclass == SPACE_STATION) {
					ch->Send("Try one of the docking bays!\n");
					return;
				}

				ship->hatchopen = FALSE;
				ch->Send("You close the hatch.\n");
				act("$n closes the hatch.", TRUE, ch, 0, 0, TO_ROOM);
				IN_ROOM(ship)->Send("The hatch on %s closes.\n", ship->name);
				return;
			} else {
				ch->Send("Its already %s.\n", cmd_door[subcmd]);
				return;
			}
		}
	}
}

ACMD(do_boardship) {
	ShipData *ship;
	RoomData * ToRoom;

	if (!*argument) {
		ch->Send("Board what?\n");
		return;
	}

	if ((ship = ship_in_room(IN_ROOM(ch), argument)) == NULL) {
		act("I see no $T here.", TRUE, ch, 0, argument, TO_CHAR);
		return;
	}

	if ((ToRoom = ship->entrance)) {
		if (!ship->hatchopen) {
			ch->Send("The hatch is closed!\n");
			return;
		}
		if (ship->state == SHIP_LAUNCH || ship->state == SHIP_LAUNCH_2) {
			ch->Send("That ship has already started launching!\n");
			return;
		}

		act("You enter $T.", TRUE, ch, 0, ship->name, TO_CHAR);
		act("$n enters $T.", TRUE, ch, 0, ship->name, TO_ROOM);
		ch->FromRoom();
		ch->ToRoom(ToRoom);
		act("$n enters the ship.", TRUE, ch, 0, 0, TO_ROOM);
		look_at_room(ch, 0);
	} else
		ch->Send("That ship has no entrance!\n");
}

ACMD(do_land) {
	RoomData * room;
	VNum vnum = -1;
	StellarObject *object;
	ShipData *ship, *target = NULL;
	bool rfound = FALSE, pfound = FALSE;
	char arg[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH];

	skip_spaces(argument);
	strcpy(arg, argument);
	argument = one_argument(argument, arg1);
	skip_spaces(argument);

	if ((ship = ship_from_pilotseat(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be in the pilots seat of a ship to do that!`n\n");
		return;
	}

	if (!check_pilot(ch, ship)) {
		ch->Send("This isn't your ship.\n");
		return;
	}

	if (ship->shipclass > SPACE_STATION) {
		ch->Send("`RThis isn't a spacecraft!`n\n");
		return;
	}

	if (autofly(ship)) {
		ch->Send("`RYou'll have to turn off the ships autopilot first.`n\n");
		return;
	}

	if (ship->shipclass == SPACE_STATION) {
		ch->Send("`RYou can't land space stations.`n\n");
		return;
	}

	if (SHP_FLAGGED(ship, SHIP_DISABLED)) {
		ch->Send("`RThe ships drive is disabled. Unable to land.`n\n");
		return;
	}

	switch (ship->state) {
		case SHIP_DOCKED:
			ch->Send("`RThe ship is already docked!`n\n");
			return;
		case SHIP_HYPERSPACE:
			ch->Send("`RYou can only do that in realspace!`n\n");
			return;
		default:
			break;
	}

	if (ship->state != SHIP_READY) {
		ch->Send("`RPlease wait until the ship has finished its current manuever.`n\n");
		return;
	}

	if (ship->starsystem == NULL) {
		ch->Send("`RThere's nowhere to land around here.`n\n");
		return;
	}

	if (!*arg) {
		ch->Send("`CLand where? Choices:`n\n");
		FOREACH(ShipList, ship->starsystem->ships, target) // FOREACH_FIXME
		{
			if (target->hangers.Count() > 0 && target != ship && ch->CanSee(target))
				ch->Send("`C%-34s`n   `C%.0f %.0f %.0f (%0.2fk)`n\n", target->name, target->vx, target->vy, target->vz, (float)(ship->Distance(target) / 1000));
		}
		FOREACH(StellarObjectList, ship->starsystem->stellarObjects, object) // FOREACH_FIXME
		{
			if (object->type == Star)
				continue;
			if (object->zone != -1)
				ch->Send("`C%-34s`n   `C%.0f %.0f %.0f (%0.2fk)`n\n", object->name, object->x, object->y, object->z, (float)(ship->Distance(object) / 1000));
		}

		ch->Send("\nYour Coordinates: %.0f %.0f %.0f`n\n", ship->vx, ship->vy, ship->vz);
		return;
	}

	FOREACH(StellarObjectList, ship->starsystem->stellarObjects, object) // FOREACH_FIXME
	{
		if (!str_cmp(arg1, object->name)
		|| !str_prefix(arg1, object->name)
		|| isname(arg1, object->name)) {
			pfound = TRUE;

			ZoneData *zone = ZoneData::GetZone(object->zone);
			if (!zone)
			{
				ch->Send("`RThat %s doesn't have any landing areas.`n\n", StellarObjectTypes[object->type]);
				return;
			}
			if (ship->Distance(object) > 200) {
				ch->Send("`RThat %s is too far away! You'll have to fly a little closer.`n\n", StellarObjectTypes[object->type]);
				return;
			}
			if (*argument)
			{
				for (vnum = (zone->GetVirtualNumber() * 100); vnum <= zone->top; ++vnum) {
					if ((room = World::GetRoomByVNum(vnum))
					&& ROOM_FLAGGED(room, ROOM_CAN_LAND)
					&& (!str_cmp(argument, room->GetName())
					|| !str_prefix(argument, room->GetName())
					|| isname(argument, room->GetName()))) {
						rfound = TRUE;
						break;
					}
				}
			}
			if (!rfound) {
				ch->Send("`CPlease type the location after the name.`n\n");
				ch->Send("Possible choices for %s:\n\n", object->name);
				for (vnum = (zone->GetVirtualNumber() * 100); vnum <= zone->top; vnum++)
					if ((room = World::GetRoomByVNum(vnum)) && ROOM_FLAGGED(room, ROOM_CAN_LAND))
						ch->Send("%s\n", room->GetName());
				return;
			}
			break;
		}
	}

	if (!pfound) {
		if ((target = get_ship_here(arg1, ship->starsystem)) != NULL) {
			bool hfound = false;
			bool bfound = false;
			Hanger *hanger;

			if (target == ship) {
				ch->Send("`RYou can't land your ship inside itself!`n\n");
				return;
			}

			if (target->shipclass != SPACE_STATION
			&& (ship->size >= target->size || ship->size >= LARGE_SHIP)) {
				ch->Send("`RThis ship is too large to land there.`n\n");
				return;
			}

			if (target->hangers.Count() <= 0) {
				ch->Send("`RThat ship has no hanger for you to land in!`n\n");
				return;
			}

			FOREACH(HangerList, target->hangers, hanger) // FOREACH_FIXME
			{
				hfound = TRUE;
				if (hanger->bayopen && hanger->CanHold(ship)) {
					bfound = TRUE;
					break;
				}
			}

			if (!hfound) {
				ch->Send("`RThat ship has no hanger.`n\n");
				return;
			}

			if (!bfound) {
				ch->Send("`RThat ship's hangers are either full or closed.`n\n");
				return;
			}

			if (ship->Distance(target) > 200) {
				ch->Send("`RThat ship is too far away! You'll have to fly a little closer.`n\n");
				return;
			}
		} else {
			ch->Send("`RI don't see that here.`n\n");
			do_land(ch, "", 0, "", 0);
			return;
		}
	}

	ch->Send("`GLanding sequence initiated.`n\n");
	act("$n begins the landing sequence.", TRUE, ch, NULL, NULL, TO_ROOM);
	echo_to_ship(TO_SHIP, ship, "`YThe ship slowly begins its landing aproach.`n");
	ship->dest = str_dup(target != NULL ? arg1 : arg);
	ship->state = SHIP_LAND;
	ship->speed = 0;
}

ACMD(do_launch) {
	ShipData *ship;
	char buf[MAX_STRING_LENGTH];

	if ((ship = ship_from_pilotseat(IN_ROOM(ch))) == NULL) {
		ch->Send("You must be in the pilot's seat of a ship to do that!\n");
		return;
	}

	if (!check_pilot(ch, ship)) {
		ch->Send("This isn't your ship.\n");
		return;
	}

	if (ship->shipclass > SPACE_STATION) {
		ch->Send("`RThis isn't a spacecraft.`n\n");
		return;
	}

	if (ship->shipclass == SPACE_STATION) {
		ch->Send("You can't do that here.\n");
		return;
	}

	if (!ch->CanUse(ship))
		return;

	if (ship->lastdoc != ship->location) {
		ch->Send("`rYou don't seem to be docked right now.`n\n");
		return;
	}

	if (ship->state != SHIP_DOCKED) {
		ch->Send("The ship is not docked right now.\n");
		return;
	}

	if (SHP_FLAGGED(ship, SHIP_DISABLED)) {
		ch->Send("The ship's drive is damaged, no power for take off.\n");
		return;
	}

#if 0
	if (!ch->SkillRoll(ship->Skill(), Perception)) {
		ch->Send("`RYou fail to work the controls properly!`n\n");
		return;
	}
#endif

	if (ship->hatchopen) {
		ship->hatchopen = FALSE;
		sprintf(buf, "`YThe hatch on %s closes.`n",ship->name);
		echo_to_room(IN_ROOM(ship), buf);
		echo_to_room(ship->entrance, "`YThe hatch slides shut.`n");
	}

	ship->hull				= ship->maxhull;
	ship->shield		= ship->maxshield;
	ship->energy		= ship->maxenergy;

	ship->state				= SHIP_LAUNCH;
	ship->primstate		= LASER_READY;
	ship->secondstate		= MISSILE_READY;
	ship->speed				= ship->maxspeed;

	ch->Send("`GLaunch sequence initiated.`n\n");
	echo_to_ship(TO_SHIP, ship, "`YThe ship hums as it lifts off the ground.`n");
	sprintf(buf, "`Y%s begins to launch.`n", ship->name);
	echo_to_room(IN_ROOM(ship), buf);
}

ACMD(do_gate) {
	ShipData *ship;
	StellarObject *object;


	if ((ship = ship_from_pilotseat(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be in the pilots seat of the ship to do that!`n\n");
		return;
	}

	if (!check_pilot(ch, ship)) {
		ch->Send("This isn't your ship.\n");
		return;
	}

	if (ship->shipclass > SPACE_STATION) {
		ch->Send("`RThis isn't a spacecraft!`n\n");
		return;
	}

	if (SHP_FLAGGED(ship, SHIP_DISABLED)) {
		ch->Send("`RThe ship's drive is damaged. Unable to gate!`n\n");
		return;
	}

	switch (ship->state) {
		case SHIP_DOCKED:
			ch->Send("`RWait until after you launch!`n\n");
			return;
		case SHIP_HYPERSPACE:
			ch->Send("`RYou can only do that in normal space.`n\n");
			return;
		default:
			break;
	}

	if (ship->starsystem == NULL) {
		ch->Send("`RYou can't do that unless the ship is flying in normal space.`n\n");
		return;
	}

	skip_spaces(argument);
	if (!*argument) {
		ch->Send("Gate to what?\n");
		return;
	}

	bool found = false;

	FOREACH(StellarObjectList, ship->starsystem->stellarObjects, object) // FOREACH_FIXME
	{
		if (object->type != Gate)
			continue;
		if (ship->Distance(object) > 200)
			continue;

		if (!str_cmp(argument, object->name) || !str_prefix(argument, object->name) || isname(argument, object->name)) {
			if (object->connects == NULL) {
				ch->Send("`RGate Malfunction: No target system.`n\n");
				return;
			}
			found = true;
			break;
		}
	}

   if (!found || !object || object->type != Gate) {
		ch->Send("No gate in range.\n");
		return;
	}

	ch->Send("`GGate sequence initiated.`n\n");
	ship->speed = ship->maxspeed;
	ship->hx = object->dx - ship->vx;
	ship->hy = object->dy - ship->vy;
	ship->hz = object->dz - ship->vz;
	echo_to_ship(TO_COCKPIT, ship, "`YYou fly into %s.`n", object->name);
	echo_to_system(ship, NULL, "`Y%s flies into %s.`n", ship->name, object->name);
	ship->FromSystem();
	ship->ToSystem(object->connects);
	ship->vx = object->dx + Number(-100, 100) + 5;
	ship->vy = object->dy + Number(-100, 100) + 5;
	ship->vz = object->dz + Number(-100, 100) + 5;
	echo_to_system(ship, NULL, "`Y%s enters the starsystem at %.0f %.0f %.0f`n", ship->name, ship->vx, ship->vy, ship->vz);
}

ACMD(do_radar) {
	MissileData *missile;
	StellarObject *object;
	ShipData *ship, *target;

	if ((ship = ship_from_cockpit(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be in the cockpit or turret of a ship to do that!`n\n");
		return;
	}

	if (ship->shipclass > SPACE_STATION) {
		ch->Send("`RThis isn't a spacecraft!`n\n");
		return;
	}

	switch (ship->state) {
		case SHIP_DOCKED:
			ch->Send("`RWait until after you launch!`n\n");
			return;
		case SHIP_HYPERSPACE:
			ch->Send("`RYou can only do that in normal space.`n\n");
			return;
		default:
			break;
	}

	if (ship->starsystem == NULL) {
		ch->Send("`RYou can't do that unless the ship is flying in normal space.`n\n");
		return;
	}

#if 0
	if (!ch->SkillRoll(ship->Skill(), Perception)) {
		ch->Send("`RYou fail to work the controls properly.`n\n");
		return;
	}
#endif

	act("$n checks the radar.", TRUE, ch, 0, 0, TO_ROOM);
	ch->Send("%s\n\n", ship->starsystem->name);

	FOREACH(StellarObjectList, ship->starsystem->stellarObjects, object) // FOREACH_FIXME
	{
		if (!ship->CanSee(object))
			continue;
		ch->Send("`%c%-34.34s   %.0f %.0f %.0f; (%0.2fk)`n\n",
		object->type == Star ? 'Y' : 'G',
		object->name, object->x, object->y, object->z, (float)(ship->Distance(object) / 1000));
	}

	ch->Send("\n");
	FOREACH(MissileList, ship->starsystem->missiles, missile) // FOREACH_FIXME
	{
		ch->Send("`R%-34.34s   %.0f %.0f %.0f; (%0.2fk)`n\n",
			missile->name, missile->mx, missile->my, missile->mz, (float)(missile->Distance(ship) / 1000));
	}

	ch->Send("\n");
	char *buf = get_buffer(MAX_STRING_LENGTH);
	FOREACH(ShipList, ship->starsystem->ships, target) // FOREACH_FIXME
	{
		if (target == ship || !ch->CanSee(target))
			continue;

		strcpy(buf, "");

		if (target->IsCloaked())
			sprintf(buf, "`n`y*`K ");

		if (Event::FindEvent(target->m_Events, DisruptionWebEvent::Type))
			strcat(buf, "(web) ");

		strcat(buf, target->name);

		ch->Send("`C%-34s`n   `C%.0f %.0f %.0f; (%0.2fk)`n\n",
			buf, target->vx, target->vy, target->vz, (float)(ship->Distance(target) / 1000));
	}
	release_buffer(buf);

	ch->Send("\n`WYour coordinates: %.0f %.0f %.0f`n\n",
		ship->vx, ship->vy, ship->vz);
}

ACMD(do_accelerate) {
	int change;
	ShipData *ship;

	if ((ship = ship_from_pilotseat(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be at the controls of a ship to do that!`n\n");
		return;
	}

	if (!check_pilot(ch, ship)) {
		ch->Send("This isn't your ship.\n");
		return;
	}

	if (ship->shipclass > SPACE_STATION) {
		ch->Send("`RThis isn't a spacecraft!`n\n");
		return;
	}

	if (autofly(ship)) {
		ch->Send("`RYou'll have to turn off the ships autopilot first.`n\n");
		return;
	}

	if (ship->shipclass == SPACE_STATION) {
		ch->Send("`RPlatforms don't fly!`n\n");
		return;
	}

	if (SHP_FLAGGED(ship, SHIP_DISABLED)) {
		ch->Send("`RThe ships drive is disabled. Unable to accelerate.`n\n");
		return;
	}

	switch (ship->state) {
		case SHIP_HYPERSPACE:
			ch->Send("`RYou can only do that in realspace!`n\n");
			return;
		case SHIP_DOCKED:
			ch->Send("`RYou can't do that until after you've launched!`n\n");
			return;
		default:
			break;
	}

#if 0
	if (!ch->SkillRoll(ship->Skill(), Perception)) {
		ch->Send("`RYou fail to work the controls properly.`n\n");
		return;
	}
#endif

	change = atoi(argument);
	if (ship->energy < flabs((change - flabs(ship->speed)) / 10)) {
		ch->Send("`RThere is not enough fuel!`n\n");
		return;
	}

	act("$n manipulates the ship's controls.", TRUE, ch, 0, 0, TO_ROOM);

	if (ship->speed < change) {
		ch->Send("`GAccelerating`n\n");
		echo_to_ship(TO_COCKPIT, ship, "`YThe ship begins to accelerate.`n");
		echo_to_ship(TO_HANGERS, ship, "The ship groans against the docking moores at a sudden increase in speed.");
		echo_to_system(ship, NULL, "`y%s begins to speed up.`n", ship->name);
	}

	if (ship->speed > change) {
		ch->Send("`GDecelerating`n\n");
		echo_to_ship(TO_COCKPIT, ship, "`YThe ship begins to slow down.`n");
		echo_to_ship(TO_HANGERS, ship, "The ship groans against the docking moores at a decrease in speed.");
		echo_to_system(ship, NULL, "`y%s begins to slow down.`n", ship->name);
	}

	AlterEnergy(ship, flabs((change - flabs(ship->speed))/10));
	ship->speed = URANGE(0 , change, ship->maxspeed);
}

ACMD(do_course) {
	ShipData *ship;
	float vx, vy, vz;
	char *arg1, *arg2;

	if ((ship = ship_from_pilotseat(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be at the healm of a ship to do that!`n\n");
		return;
	}

	if (!check_pilot(ch, ship)) {
		ch->Send("This isn't your ship.\n");
		return;
	}

	if (ship->shipclass > SPACE_STATION) {
		ch->Send("`RThis is not a spacecraft.`n\n");
		return;
	}

	if (autofly(ship)) {
		ch->Send("`RYou'll have to turn off the ships autopilot first.`n\n");
		return;
	}

	if (SHP_FLAGGED(ship, SHIP_DISABLED)) {
		ch->Send("`RThe ships drive is disabled. Unable to manuever.`n\n");
		return;
	}

	if (ship->shipclass == SPACE_STATION) {
		ch->Send("`RPlatforms cannot fly.`n\n");
		return;
	}

	switch (ship->state) {
		case SHIP_HYPERSPACE:
			ch->Send("`RYou can only do that in realspace!`n\n");
			return;
		case SHIP_DOCKED:
			ch->Send("`RYou can't do that until after you've launched!`n\n");
			return;
		default:
			break;
	}

	if (ship->state != SHIP_READY) {
		ch->Send("`RPlease wait until the ship has finished its current manuever.`n\n");
		return;
	}

	if (ship->energy < (ship->speed / 10)) {
		ch->Send("`RThere is not enough fuel!`n\n");
		return;
	}

	arg1 = get_buffer(MAX_INPUT_LENGTH);
	arg2 = get_buffer(MAX_INPUT_LENGTH);
	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	vx = atof(arg1);
	vy = atof(arg2);
	vz = atof(argument);
	release_buffer(arg1);
	release_buffer(arg2);

	if (vx == ship->vx && vy == ship->vy && vz == ship->vz) {
		ch->Send("The ship is already at %.0f %.0f %.0f!\n", vx, vy, vz);
		return;
	}

#if 0
	if (!ch->SkillRoll(ship->Skill(), Perception)) {
		ch->Send("`RYou fail to work the controls properly!`n\n");
		return;
	}
#endif

	ship->hx = vx - ship->vx;
	ship->hy = vy - ship->vy;
	ship->hz = vz - ship->vz;
	AlterEnergy(ship, ship->speed / 10);

	ch->Send("`GNew course set, aproaching %.0f %.0f %.0f.`n\n", vx, vy, vz);
	act("$n manipulates the ships controls.", TRUE, ch, 0, 0, TO_ROOM);

	if (ship->manuever > 100)
		ship->state = SHIP_BUSY_3;
	else if (ship->manuever > 50)
		ship->state = SHIP_BUSY_2;
	else
		ship->state = SHIP_BUSY;

	echo_to_ship(TO_COCKPIT, ship, "`YThe ship begins to turn.`n");
	echo_to_system(ship, NULL, "`y%s turns altering its present course.`n", ship->name);
	return;
}

ACMD(do_shipstat) {
	ShipData *ship, *target;

	if ((ship = ship_from_cockpit(IN_ROOM(ch))) == NULL
	&& (ship = ship_from_engine(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be in the cockpit, turret or engineroom of a ship to do that!`n\n");
		return;
	}

	skip_spaces(argument);

	if (!*argument)
		target = ship;
	else
		target = get_ship_here(argument, ship->starsystem);

	if (target == NULL || !ch->CanSee(target)) {
		ch->Send("`RYou don't see that here.`n\n"
		"`RTry the radar, or type status by itself for your ship's status.`n\n");
		return;
	}

#if 0
	if (!ch->SkillRoll(ship->Skill(), Perception)) {
		ch->Send("`RYou fail to work the controls properly.`n\n");
		return;
	}
#endif

	act("$n checks various gauges and displays on the control panel.", TRUE, ch, 0, 0, TO_ROOM);
	if (target != ship && target->CanSee(ship))
		echo_to_ship(TO_COCKPIT, target, "`WYou are being scanned by %s.`n", ship->name);
	ch->Send(
		"`W%s:`n\n"
		"`yCurrent Speed	  : `Y%d`n`y/`Y%d`n\n"
		"`yCurrent Heading	: `Y%.0f %.0f %.0f`n\n"
		"`yCurrent Coordinates: `Y%.0f %.0f %.0f`n\n"
		"`yCurrent Target	 : `Y%s`n\n",
		SHPS(target, ship), target->speed, target->maxspeed,
		target->hx, target->hy, target->hz,
		target->vx, target->vy, target->vz,
		target->target ? target->target->name : "None");

	int count = 0;
	Turret *turret;
	FOREACH(TurretList, target->turrets, turret) // FOREACH_FIXME
	{
		ch->Send("`yTurret #%d Target   : `Y%s`n\n", ++count,
			turret->target ? SHPS(turret->target, ship) : "None");
	}

	if (target->following)
		ch->Send("`yFollowing		  : `Y%s`n\n", SHPS(target->following, ship));

	ch->Send(
		"`yAutopilot		  : `Y%-11.11s`n  `yShip Condition	 : `Y%s`n\n"
		"`yPrimary Condition  : `Y%-11.11s`n  `ySecondary Condition: `Y%s`n\n"
		"`yPrimary State	  : `Y%-11.11s`n  `ySecondary State	: `Y%s`n\n"
		"`yHull			   : `Y%5d`n`y/`Y%-5d`n  `yShields : `Y%d`n`y/`Y%d`n\n"
		"`yEnergy (fuel)	  : `Y%5d`n`y/`Y%-5d`n  `yDamage  : `Y%d`n`y/`Y%d`n\n",
		autofly(target) ? "Engaged" : "Disengaged",
		SHP_FLAGGED(target, SHIP_DISABLED) ? "Disabled" : "Running",
		target->primstate == LASER_DAMAGED ? "Damaged" : "Good",
		target->secondstate == MISSILE_DAMAGED ? "Damaged" : "Good",
		!Event::FindEvent(target->m_Events, VehicleCooldownEvent::Type) ? "Ready" : "Not Ready",
		!Event::FindEvent(target->m_Events, MissileCooldownEvent::Type) ? "Ready" : "Not Ready",
		target->hull, target->maxhull, target->shield + target->tempshield, target->maxshield,
		target->energy, target->maxenergy, target->primdam, target->secondam);
}

ACMD(do_calculate) {
	int count = 0;
	ShipData *ship;
	StellarObject *object;
	StarSystem *starsystem;
	char *arg1, *arg2, *arg3, *arg4;

	switch (ch->m_Substate) {
		default:
			skip_spaces(argument);
			if ((ship = ship_from_pilotseat(IN_ROOM(ch))) == NULL)
				ch->Send("`RYou must be in control of a ship to do that!`n\n");
			else if (!check_pilot(ch, ship))
				ch->Send("This isn't your ship.\n");
			else if (ship->shipclass > SPACE_STATION)
				ch->Send("`RThis isn't a space craft.`n\n");
			else if (autofly(ship))
				ch->Send("`RYou'll have to turn off the ships autopilot first.`n\n");
			else if (ship->shipclass == SPACE_STATION)
				ch->Send("`RThis station is a little too big for hyperspace.`n\n");
			else if (ship->hyperspeed == 0)
				ch->Send("`RThis ship is not equipped with a hyperdrive!\n");
			else if (ship->state == SHIP_DOCKED)
				ch->Send("`RYou can't do that until after you've launched!`n\n");
			else if (ship->starsystem == NULL)
				ch->Send("`RYou can only do that in real space.`n\n");
			else if (!*argument) {
				char *buf = get_buffer(MAX_INPUT_LENGTH);
				ch->Send("`WUsage: Calculate <starsystem> <entry x> <entry y> <entry z>\n");
				ch->Send("Possible destinations:`n\n");

				FOREACH(StarSystemList, starsystem_list, starsystem) // FOREACH_FIXME
				{
					if (STR_FLAGGED(starsystem, STAR_HIDDEN))
						continue;
					count++;
					sprintf(buf, "%s%s", starsystem->name, !(count % 5) ? "" : ",");
					ch->Send("%-15.15s%s",
						buf, !(count % 5) ? "\n" : " ");
				}
				if (count % 5)
					ch->Send("\n");
				if (!count)
					ch->Send("No starsystems found.\n");
				release_buffer(buf);
			} else {
				ch->Send("You begin imputing the coordinates into the computer.\n");
				act("$n begins to imput coordinates into the computer.", FALSE, ch, 0, 0, TO_ROOM);
				ch->AddTimer(NO_STAFF_HASSLE(ch) ? 1 : 2 RL_SEC, do_calculate, 1,
						str_dup(argument));
			}
			return;

		case 1:
			ch->m_Substate = SUB_NONE;
			if (!((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer() || !*(char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer())
				return;
			break;

		case SUB_TIMER_DO_ABORT:
			ch->Send("You are interrupted and stop imputing coordinates.\n");
			act("$n is interrupted and stops imputing coordinates.", FALSE, ch, 0, 0, TO_ROOM);
			ch->m_Substate = SUB_NONE;
			return;
	}

	if ((ship = ship_from_pilotseat(IN_ROOM(ch))) == NULL)
		return;

#if 0
	if (!ch->SkillRoll(ship->Skill(), Perception)) {
		ch->Send("`RYou fail to work the controls properly!`n\n");
		return;
	}
#endif

	arg1 = get_buffer(MAX_INPUT_LENGTH);
	arg2 = get_buffer(MAX_INPUT_LENGTH);
	arg3 = get_buffer(MAX_INPUT_LENGTH);
	arg4 = get_buffer(MAX_INPUT_LENGTH);

	half_chop((char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer(), arg1, arg3);
	half_chop(arg3, arg2, arg4);
	half_chop(arg4, arg3, arg4);				// oi that looks conzzufled

	ship->currjump = starsystem_from_name(arg1);
	ship->jx = atoi(arg2);
	ship->jy = atoi(arg3);
	ship->jz = atoi(arg4);

	release_buffer(arg1);
	release_buffer(arg2);
	release_buffer(arg3);
	release_buffer(arg4);

	if (ship->currjump == NULL) {
		ch->Send("`RYou can't seem to find that starsytem on your charts.`n\n");
		return;
	} else if (!ship->currjump->CanJump(ship)) {
		ch->Send("`RWARNING..  Unable to chart safe path to starsystem.`n\n");
		ch->Send("`RWARNING..  Hyperjump NOT set.`n\n");
		ship->currjump = NULL;
		return;
	} else if (ship->hyperspeed == 1 && ship->currjump != ship->starsystem) {
		ch->Send("`RThis ship cannot travel outside of the starsystem.`n\n");
		ship->currjump = NULL;
		return;
	} else {
		starsystem = ship->currjump;

		FOREACH(StellarObjectList, starsystem->stellarObjects, object) // FOREACH_FIXME
		{
			if (ship->JumpDistance(object) < 300) {
				echo_to_ship(TO_COCKPIT, ship, "`RWARNING.. Jump coordinates too close to stellar object.`n");
				echo_to_ship(TO_COCKPIT, ship, "`RWARNING.. Hyperjump NOT set.`n");
				ship->currjump = NULL;
				return;
			}
		}

		ship->jx += Number(-250, 250);
		ship->jy += Number(-250, 250);
		ship->jz += Number(-250, 250);
	}

	if (ship->starsystem == ship->currjump) {
		if (ship->hyperspeed == 1)
			ship->hyperdistance = Number(1, 6);
		else
			ship->hyperdistance = Number(1, 450);
	} else
		ship->hyperdistance = Number(600, 1500);

	ch->Send("`GHyperspace course set. Ready for the jump to lightspeed.`n\n");
	act("$n finishes $s calculations on the ship's computer.", FALSE, ch, 0, 0, TO_ROOM);
	return;
}

ACMD(do_hyperspace) {
	ShipData *ship;

	if ((ship = ship_from_pilotseat(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be in control of a ship to do that!`n\n");
		return;
	}

	if (!check_pilot(ch, ship)) {
		ch->Send("This isn't your ship.\n");
		return;
	}

	if (ship->shipclass > SPACE_STATION) {
		ch->Send("`RThis isn't a spacecraft.`n\n");
		return;
	}

	if (autofly(ship)) {
		ch->Send("`RYou'll have to turn off the ships autopilot first.`n\n");
		return;
	}

	if (ship->shipclass == SPACE_STATION) {
		ch->Send("Space platforms can't move!\n");
		return;
	}

	if (ship->hyperspeed == 0) {
		ch->Send("`RThis ship is not equipped with a hyperdrive!`n\n");
		return;
	}

	if (SHP_FLAGGED(ship, SHIP_DISABLED) || Event::FindEvent(ship->m_Events, DisruptionWebEvent::Type)) {
		ch->Send("`RThe ships drive is disabled. Unable to manuever.`n\n");
		return;
	}

	switch (ship->state) {
		case SHIP_HYPERSPACE:
			ch->Send("`RYou are already travelling lightspeed!`n\n");
			return;
		case SHIP_DOCKED:
			ch->Send("`RYou can't do that until after you've launched!`n\n");
			return;
		default:
			break;
	}

	if (ship->state != SHIP_READY) {
		ch->Send("`RPlease wait until the ship has finished its current manuever.`n\n");
		return;
	}

	if (ship->currjump == NULL) {
		ch->Send("`RYou need to calculate your jump first!`n\n");
		return;
	}

	if (ship->energy < (200 + ship->hyperdistance)) {
		ch->Send("`RThere is not enough fuel!`n\n");
		return;
	}

	if (ship->speed <= 0) {
		ch->Send("`RYou need to speed up first.`n\n");
		return;
	}

	if (!ship->SafeDistance()) {
		ch->Send("`RYou are too close to hostiles to make the jump to lightspeed.`n\n");
		return;
	}

#if 0
	if (!ch->SkillRoll(ship->Skill(), Perception)) {
		ch->Send("`RYou fail to work the controls properly!`n\n");
		return;
	}
#endif

	ch->Send("`GYou push forward the hyperspeed lever.`n\n");
	act("$n pushes a lever forward on the control panel.", TRUE, ch, 0, 0, TO_ROOM);
	echo_to_ship(TO_SHIP, ship, "`YThe ship lurches slightly as it makes the jump to lightspeed.`n");
	echo_to_ship(TO_COCKPIT, ship, "`YThe stars become streaks of light as you enter hyperspace.`n");
	echo_to_ship(TO_HANGERS, ship, "The ship lurches at the sudden jump to hyperspace.");

	ship->FromSystem();
	ship->state = SHIP_HYPERSPACE;
	echo_to_system(ship, NULL, "`Y%s disapears from your scanner.`n", ship->name);

	if (SHP_FLAGGED(ship, SHIP_SABOTAGE)) {
		echo_to_ship(TO_COCKPIT, ship, 
		"`R`fMalfunction`n`R: Drive damaged. Energy build-up at critical!`n");
		ship->state = SHIP_DESTROY;
		return;
	}

	AlterEnergy(ship, 100 + ship->hyperdistance);

	ship->ox = ship->vx;
	ship->oy = ship->vy;
	ship->oz = ship->vz;

	ship->vx = ship->jx;
	ship->vy = ship->jy;
	ship->vz = ship->jz;
}

ACMD(do_scanplanet) {
	Clan *clan;
	bool found;
	int c = 0;
	ShipData *ship;
	StellarObject *object;

	skip_spaces(argument);

	if (!*argument) {
		ch->Send("What do you wish to scan?\n");
		return;
	}

	if ((ship = ship_from_cockpit(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be in the cockpit of a ship to do that.`n\n");
		return;
	}

	switch (ship->state) {
		case SHIP_DOCKED:
			ch->Send("`RWait until after you launch!\n");
			return;
		case SHIP_HYPERSPACE:
			ch->Send("`RYou can only do that in normal space.`n\n");
			return;
		default:
			break;
	}

	if (ship->starsystem == NULL) {
		ch->Send("`RYou can't do that unless the ship is flying in normal space.`n\n");
		return;
	}

#if 0
	if (!ch->SkillRoll(ship->Skill(), Perception)) {
		ch->Send("`RYou fail to work the controls properly.`n\n");
		return;
	}
#endif

	FOREACH(StellarObjectList, ship->starsystem->stellarObjects, object) // FOREACH_FIXME
	{
		if (!str_cmp(argument, object->name)
		|| !str_prefix(argument, object->name)
		|| isname(argument, object->name)) {
			if (ship->Distance(object) > 1000) {
				ch->Send("`RThat %s is too far away! You'll have to fly a little closer.`n\n", StellarObjectTypes[object->type]);
				return;
			}
			break;
		}
	}

	if (object == NULL) {
		ch->Send("There is no %s here.\n", argument);
		return;
	}

	ch->Send("`W%s`n\n", object->name);
	ch->Send("`WType: `G%s`n\n", StellarObjectTypes[object->type]);
	ch->Send("`WHabitat: `G%s`n\n", HabitatTypes[object->habitat]);
	ch->Send("`WGoverned by: `G%s`n\n",
		(clan = Clan::GetClan(object->clan)) ? clan->GetName() : "Nobody");
	ch->Send("`WLifeforms Detected:`n\n	");
	found = false;
	ZoneData *zone = ZoneData::GetZone(object->zone);
	if (zone)
	{
		CharData *zch;
		FOREACH(CharList, character_list, zch) // FOREACH_FIXME
		{
			if (IS_NPC(zch) || zone != IN_ROOM(zch)->zone || ch->CanSee(zch) != VISIBLE_FULL)
				continue;

			found = true, c++;
			ch->Send("%-12s%s", zch->GetName(),
			!(c % 5) ? "\n	" : "  ");
		}
	}
	if (!found)
		ch->Send("None.\n");
	else if ((c % 5))
		ch->Send("\n");

	if (object->type == Gate) {
		ch->Send("\n\nGate Information:\n");
		ch->Send("Coordinates: %.0f %.0f %.0f\n", object->x, object->y, object->z);
		ch->Send("Dest Coords: %.0f %.0f %.0f\n", object->dx, object->dy, object->dz);
		ch->Send("Connects To: %s\n", object->connects->name);
	}
}

ACMD(do_scanship) {
	char *buf;
	UInt32 c = 0;
	CharData *vict;
	RoomData *room;
	ShipData *ship, *target;
	skip_spaces(argument);

	if ((ship = ship_from_cockpit(IN_ROOM(ch))) == NULL) {
		if (!*argument) {
			ch->Send("Which ship do you wish to scan?\n");
			return;
		}
		if (!(ship = ship_in_room(IN_ROOM(ch), argument))) {
			do_scanplanet(ch, "", 0, "", 0);
			return;
		}
		target = ship;
	} else if (!*argument)
		target = ship;
	else
		target = get_ship_here(argument, ship->starsystem);

	if (target == NULL || !ch->CanSee(target)) {
		do_scanplanet(ch, argument, 0, argument, 0);
		return;
	}

#if 0
	if (!ch->SkillRoll(ship->Skill(), Perception)) {
		ch->Send("`RYou fail to work the controls properly.`n\n");
		return;
	}
#endif

	buf = get_buffer(MAX_STRING_LENGTH);
	switch (target->shipclass) {
		case SPACECRAFT:
				sprintf(buf, model_list[target->model].name);
				break;
		case SPACE_STATION:
				sprintf(buf, "Space Station");
				break;
		case AIRCRAFT:
				sprintf(buf, "Aircraft");
				break;
		case BOAT:
				sprintf(buf, "Boat");
				break;
		case SUBMARINE:
				sprintf(buf, "Submarine");
				break;
		case LAND_VEHICLE:
//				sprintf(buf, "Land Vehicle");
				sprintf(buf, model_list[target->model].name);
				break;
		default:
				sprintf(buf, "Unknown");
				break;
	}

	ch->Send("%-15.15s: %s\n", buf, target->name);
	ch->Send("Ship Owner	 : %4s\n", *target->owner ? target->owner : "<NONE>");
	ch->Send("Ship Pilot	 : %4s\n", *target->pilot ? target->pilot : "<NONE>");
	ch->Send("Ship Copilot   : %4s\n", *target->copilot ? target->copilot : "<NONE>");
	ch->Send("Max Hull	   : %4d  Armor Plate: %2d+%d\n",
		target->maxhull, target->armor, target->upgrades[UPGRADE_ARMOR]);
	ch->Send("Max Shields	: %4d  Shield Upgr: %2d+%d  Max Energy : %d\n",
		target->maxshield, 0, target->upgrades[UPGRADE_SHIELD],
		target->maxenergy);
	ch->Send("Max Speed	  : %4d  Manuever   : %4d  Hyperspeed : %d\n",
		target->maxspeed, target->manuever, target->hyperspeed);

	if (str_cmp("None", ground_weap[target->model]))
		ch->Send("%-15s: %d+%d\n", ground_weap[target->model], target->primdam,
				target->upgrades[UPGRADE_GROUND]);

	if (str_cmp("None", air_weap[target->model]))
		ch->Send("%-15s: %d+%d\n", air_weap[target->model], target->secondam,
				target->upgrades[UPGRADE_AIR]);

	if (SHP_FLAGGED(target, SHIP_CLOAKER))
		ch->Send("%-15.15s: Cloaking Device%s\n", "Special Module", target->IsCloaked() ? " (activated)" : "");

	if (SHP_FLAGGED(target, SHIP_REGEN))
		ch->Send("%-15.15s: Regenerative Hull\n", "Special Module");

	if (SHP_FLAGGED(target, SHIP_WEBBER))
		ch->Send("%-15.15s: Disruption Web Device\n", "Special Module");

	if (SHP_FLAGGED(target, SHIP_DETECTOR))
		ch->Send("%-15.15s: Advanced Sensor Array\n", "Special Module");

	if (SHP_FLAGGED(target, SHIP_IRRADIATOR))
		ch->Send("%-15.15s: Irradiation Projector\n", "Special Module");

	if (SHP_FLAGGED(target, SHIP_MASSCLOAK))
		ch->Send("%-15.15s: Cloaking Field Projector\n", "Special Module");

	if (SHP_FLAGGED(target, SHIP_EXOTIC))
		ch->Send("%-15.15s: Exotic Power Enhancements\n", "Special Module");

	ch->Send("%-15.15s:\n	", "Crew Manifest");
	FOREACH(RoomList, target->rooms, room) // FOREACH_FIXME
	{
		if (room->people.Count()) {
			FOREACH(CharList, room->people, vict) // FOREACH_FIXME
			{
				if (ch->CanSee(vict) != VISIBLE_FULL)
					continue;

				c++;
				ch->Send("%-12s%s", vict->GetName(),
				!(c % 4) ? "\n	" : "  ");
			}
		}
	}
	if ((c % 4)) ch->Send("\n");

	act("$n checks various gauges and displays on the control panel.", TRUE, ch, 0, 0, TO_ROOM);
	if (target != ship && target->CanSee(ship))
		echo_to_ship(TO_COCKPIT, target, "`WYou are being scanned by %s.`n", ship->name);
	release_buffer(buf);
}

ACMD(do_followship) {
	ShipData *ship, *target;

	skip_spaces(argument);

	if ((ship = ship_from_turret(IN_ROOM(ch))) == NULL)
		ch->Send("`RYou must be in the gunners seat or turret of a ship to do that!`n\n");
	else if (ship->state == SHIP_HYPERSPACE)
		ch->Send("`RYou can only do that in realspace!`n\n");
	else if (!ship->starsystem && ship->shipclass <= SPACE_STATION)
		ch->Send("`RYou can't do that until you've finished launching!`n\n");
	else if (autofly(ship))
		ch->Send("`RYou'll have to turn off the ships autopilot first....`n\n");
//	else if (!ch->SkillRoll(ship->Skill(), Perception))
//		ch->Send("`RYou fail to work the controls properly!`n\n");
	else if (!*argument)
		ch->Send("`RYou need to specify a target!`n\n");
	else if (!str_cmp(argument, "none")) {
		ch->Send("`GDisengaging target.`n\n");
		ship->following = NULL;
	} else if ((target = get_ship_here(argument, ship->starsystem)) == NULL)
		ch->Send("`RThat ship isn't here!`n\n");
	else if (!ch->CanSee(target))
		ch->Send("`RThat ship isn't here!`n\n");
	else {
		ship->following = target;
		ch->Send("`GNow following %s.`n\n", target->name);
		act("$n makes an adjustment to the flight computer.", FALSE, ch, 0, 0, TO_ROOM);
		echo_to_ship(TO_COCKPIT, target, "`W%s begins following you.`n", ship->name);
	}
}

ACMD(do_target) {
	ShipData *ship, *target;

	switch (ch->m_Substate) {
		default:
			if ((ship = ship_from_turret(IN_ROOM(ch))) == NULL)
				ch->Send("`RYou must be in the gunners seat or turret of a ship to do that!`n\n");
			else if (ship->state == SHIP_HYPERSPACE)
				ch->Send("`RYou can only do that in realspace!`n\n");
			else if (!ship->starsystem && ship->shipclass <= SPACE_STATION)
				ch->Send("`RYou can't do that until you've finished launching!`n\n");
			else if (autofly(ship))
				ch->Send("`RYou'll have to turn off the ships autopilot first....`n\n");
//			else if (!ch->SkillRoll(ship->Skill(), Perception))
//				ch->Send("`RYou fail to work the controls properly!`n\n");
			else {
				skip_spaces(argument);
				if (!*argument) {
					ch->Send("`RYou need to specify a target!`n\n");
					return;
				}

				if (!str_cmp(argument, "none")) {
					if (!ship->target) {
						ch->Send("`GNo target present.`n\n");
						return;
					}
					ch->Send("`GTarget removed.`n\n");
					echo_to_ship(TO_COCKPIT, ship->target, "`yYou are no longer being targeted by %s.`n", ship->name);
					ship->SetTarget(NULL);
					return;
				}

				if (ship->shipclass > SPACE_STATION)
					target = ship_in_room(IN_ROOM(ship), argument);
				else
					target = get_ship_here(argument, ship->starsystem);

				if (target == NULL) {
					ch->Send("`RThat ship isn't here!`n\n");
					return;
				}

				if (target == ship) {
					ch->Send("`RYou can't target your own ship!`n\n");
					return;
				}

				if (!ch->CanSee(target)) {		// for holylight
					ch->Send("`RThat ship isn't here!`n\n");
					return;
				}

//				if (!NO_STAFF_HASSLE(ch) && !str_cmp("Public", target->owner)) {
//					ch->Send("Intergalactic Shuttles wouldn't be happy if you started shooting down their ships.\n");
//					return;
//				}

				if (ship->shipclass > SPACE_STATION && target->shipclass <= SPACE_STATION) {
					ch->Send("`RYou can't target that ship!`n\n");
					return;
				}

				if (ship->shipclass <= SPACE_STATION && ship->Distance(target) > 5000) {
					ch->Send("`RThat ship is too far away to target.`n\n");
					return;
				}

				ch->Send("`GTracking target.`n\n");
				act("$n makes some adjustments on the targeting computer.", TRUE, ch, 0, 0, TO_ROOM);
				ch->AddTimer(NO_STAFF_HASSLE(ch) ? 1 : 2 RL_SEC, do_target, 1,
						str_dup(argument));
			}
			return;

		case 1:
			ch->m_Substate = SUB_NONE;
			if (!((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer() || !*(char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer())
				return;
			break;

		case SUB_TIMER_DO_ABORT:
			ch->Send("`RYour concentration is broken. You fail to lock onto your target.`n\n");
			ch->m_Substate = SUB_NONE;
			return;
	}

	if ((ship = ship_from_turret(IN_ROOM(ch))) == NULL)
		return;

#if 0
	if (!ch->SkillRoll(SKILL_GUNNERY, Perception)) {
		ch->Send("You fail to aquire a target lock. Try again.\n");
		return;
	}
#endif

	if (ship->shipclass > SPACE_STATION)
		target = ship_in_room(IN_ROOM(ship), (char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer());
	else
		target = get_ship_here((char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer(), ship->starsystem);

	if (target == NULL || target == ship || target->IsPurged()) {
		ch->Send("`RThe ship has left the starsytem. Targeting aborted.`n\n");
		return;
	}

	if (!ch->CanSee(target)) {
		ch->Send("`RThat ship seems to have disappeared.`n\n");
		return;
	}

	if (ship->gunseat == IN_ROOM(ch))
		ship->SetTarget(target);
	else {
		Turret *turret;

		FOREACH(TurretList, ship->turrets, turret) // FOREACH_FIXME
			if (turret->room == IN_ROOM(ch))
				break;

		if (!turret)
			return;

		turret->target = target;
	}

	ch->Send("`GTarget Locked.`n\n");
	ship->Appear();
	echo_to_ship(TO_COCKPIT, target, "`rYou are being targetted by %s.`n", ship->name);
	if (autofly(target) && !target->target) {
		echo_to_ship(TO_COCKPIT, ship, "`rYou are being targetted by %s.`n", target->name);
		target->SetTarget(ship);
	}
	return;
}

#define TOG_OFF		0
#define TOG_ON		1
#define SHP_TOG_CHK(ch,flag) ((TOGGLE_BIT(SHP_FLAGS(ch), (flag))) & (flag))

ACMD(do_gen_autoship) {
	Flags result;
	ShipData *ship;
	char *tog_messages[][2] = {
		{ "`YAutopilot off`n.\n", "`YAutopilot on.`n\n" },
		{ "`YAutotrack off`n.\n", "`YAutotrack on.`n\n" }
	};

	if ((ship = ship_from_cockpit(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be in the cockpit of a ship to do that.`n\n");
		return;
	}

	if (!check_pilot(ch, ship)) {
		ch->Send("That's not your ship!\n");
		return;
	}

#if 0
	if (!ch->SkillRoll(ship->Skill(), Perception)) {
		ch->Send("`RYou fail to work the controls properly!`n\n");
		return;
	}
#endif

	switch (subcmd) {
		case SCMD_AUTOPILOT:
			result = SHP_TOG_CHK(ship, SHIP_AUTOPILOT);
			break;
		case SCMD_AUTOTRACK:
			result = SHP_TOG_CHK(ship, SHIP_AUTOTRACK);
			break;
		default:
			log("SYSERR: Unknown subcmd %d in do_gen_autoship", subcmd);
			return;
	}

	if (result)
		send_to_char(tog_messages[subcmd][TOG_ON], ch);
	else
		send_to_char(tog_messages[subcmd][TOG_OFF], ch);
	act("$n flips a switch on the control panel.", TRUE, ch, 0, 0, TO_ROOM);
	return;
}

ACMD(do_shipcloak) {
	ShipData *ship;

	if ((ship = ship_from_cockpit(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be in the cockpit of a ship to do that!`n\n");
		return;
	}

	switch (ship->state) {
		case SHIP_DOCKED:
			ch->Send("`RWait until after you launch!`n\n");
			return;
		case SHIP_HYPERSPACE:
			ch->Send("`RYou can only do that in normal space.`n\n");
			return;
		default:
			break;
	}

	if (ship->starsystem == NULL) {
		ch->Send("`RYou can't do that unless the ship is flying in normal space.`n\n");
		return;
	}

	if (!SHP_FLAGGED(ship, SHIP_CLOAKER)) {
		ch->Send("This ship is not equiped with a cloaking device.\n");
		return;
	}

	if (ship->target) {
		ch->Send("`RNot while the ship is enganged with an enemy!\n");
		return;
	}

#if 0
	if (!ch->SkillRoll(ship->Skill(), Perception)) {
		ch->Send("`RYou fail to work the controls properly.`n\n");
		WAIT_STATE(ch, 1 RL_SEC);
		return;
	}
#endif

	if (Event::FindEvent(ship->m_Events, ShipTimerEvent::Type)) {
		ch->Send("Ship is currently busy.\n");
		return;
	}

	ch->Send("You flip a switch on the cloaking device.\n");
	act("$n flips a switch on the cloaking device.", TRUE, ch, 0, 0, TO_ROOM);

	ship->m_Events.push_front(new ShipTimerEvent(4 RL_SEC, ship->GetID(), STYPE_CLOAKING));
	return;
}

ACMD(do_baydoors) {
	ShipData *ship;

	if ((ship = ship_from_cockpit(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be in the cockpit of a ship to do that!`n\n");
		return;
	}

	if (ship->shipclass > SPACE_STATION) {
		ch->Send("`RThis isn't a spacecraft!`n\n");
		return;
	}

	if (!check_pilot(ch, ship)) {
		ch->Send("`RThis isn't your ship!`n\n");
		return;
	}

	if (!ship->hangers.Count() && !ship->garages.Count()) {
		ch->Send("`RThis ship doesn't have any hangers!`n\n");
		return;
	}

#if 0
	if (!ch->SkillRoll(ship->Skill(), Perception)) {
		ch->Send("`RYou fail to work the controls properly!`n\n");
		return;
	}
#endif

	if (Event::FindEvent(ship->m_Events, ShipTimerEvent::Type)) {
		ch->Send("Ship is currently busy.\n");
		return;
	}

	ch->Send("You flip the switch for the bay doors.\n");
	act("$n flips a switch on the control panel.", TRUE, ch, 0, 0, TO_ROOM);

	ship->m_Events.push_front(new ShipTimerEvent(4 RL_SEC, ship->GetID(), STYPE_BAYDOORS));
	return;	
}

ACMD(do_buyship) {
	int i, c;
	char *buf;
	RNum clan = -1;
	Flags race = 0;
	ShipData *ship;
	bool land_vehicles = FALSE;
	bool found = FALSE, clanbuy = FALSE;

	if (IN_ROOM(ch) == NULL)
		return;

	land_vehicles = ROOM_FLAGGED(IN_ROOM(ch), ROOM_GARAGE);
	if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_SHIPYARD | ROOM_GARAGE)) {
		ch->Send("`RYou must be in a shipyard or garage to purchase transportation.`n\n");
		return;
	}

	skip_spaces(argument);

	if (!*argument) {
		ch->Send("%-34.34s  %-5.5s\n", "Ship", "Price");
		ch->Send("%-34.34s--%-5.5s\n",
			"----------------------------------------",
			"----------------------------------------");

		for (i = 0; model_list[i].name != NULL; i++) {
			race = 0;

			for (c = 0; c < strlen(model_list[i].race); c++)
				race |= find_race_bitvector(model_list[i].race[c]);

			if (!land_vehicles && model_list[i].shipclass > SPACE_STATION)
				continue;
			if (land_vehicles && model_list[i].shipclass <= SPACE_STATION)
				continue;

			if (model_list[i].price > 0 && (NO_STAFF_HASSLE(ch) || (race & (1 << GET_RACE(ch)))))
				ch->Send("%-34.34s  %-5d\n", model_list[i].name, model_list[i].price);
		}
		return;
	}

/*	if (subcmd == SCMD_CLANSHIP)
		clanbuy = TRUE;

	if (clanbuy && (clan = Clan::Real(GET_CLAN(ch))) == -1) {
		ch->Send("You are not a member of a clan.\n");
		return;
	}

	if (clanbuy && GET_CLAN_LEVEL(ch) < ClanRank_Commander) {
		ch->Send("You do not have sufficient rank to buy a clan ship.\n");
		return;
	}
*/
	for (i = 0; model_list[i].name != NULL; i++) {
		race = 0;

		if (model_list[i].price == 0)
			continue;

		if (!land_vehicles && model_list[i].shipclass > SPACE_STATION)
			continue;
		if (land_vehicles && model_list[i].shipclass <= SPACE_STATION)
			continue;

		for (c = 0; c < strlen(model_list[i].race); c++)
			race |= find_race_bitvector(model_list[i].race[c]);

		if ((NO_STAFF_HASSLE(ch) || (race & (1 << GET_RACE(ch))))
		&& (!str_cmp(argument, model_list[i].name)
		|| (!str_prefix(argument, model_list[i].name))
		|| (isname(argument, model_list[i].name)))) {
			found = TRUE;
			break;
		}
	}

	if (!found) {
		ch->Send("No such ship is available to you.\n");
		return;
	}

#if 0
	if ((clanbuy ? CLANSAVINGS(clan) : GET_QP(ch)) < model_list[i].price) {
		ch->Send("That %sship costs %dbp.\n", clanbuy ? "clan " : "", model_list[i].price);
		return;
	}
#endif

	if (!IS_STAFF(ch) && ships_owned(ch, clanbuy) > (clanbuy ? 8 : 3)) {
		ch->Send("You own too many ships, use the ones you have.\n");
		return;
	}
	buf = get_buffer(MAX_STRING_LENGTH);

#if 0
	if (clanbuy) {
		CLANSAVINGS(clan) -= model_list[i].price;
		Clan::Save();
	} else
		GET_QP(ch) -= model_list[i].price;
#endif

	ch->Send("`GYou pay %d credits to purchase the ship.`n\n", model_list[i].price);
	act("$n walks over to a terminal and makes a credit transaction.", TRUE, ch, 0, 0, TO_ROOM);

	ship = make_ship(i);
	ship->ToRoom(IN_ROOM(ch));
	ship->lastdoc = ship->location = ship->shipyard = ch->InRoom();

	sprintf(buf, "%s%s %s %d",
		clanbuy ? strip_color(Clan::Index[clan]->GetName()) : ch->GetName(),
		clanbuy ? "" : "'s", model_list[i].name, atoi(ship->filename));

	free(ship->name);
	ship->name = str_dup(buf);
	free(ship->owner);
//	ship->owner = str_dup(clanbuy ? strip_color(CLANNAME(clan)) : ch->GetName());
	if (clanbuy)
	{
		sprintf(buf, "%d", Clan::Index[clan]->GetVirtualNumber());
		ship->owner = str_dup(buf);
	}
	else
		ship->owner = str_dup(ch->GetName());
	ship->save();
	release_buffer(buf);
	return;
}

ACMD(do_svlist) {
	int count = 0;
	ShipData *ship;

	ch->Send("Ships to be saved:\n");
	FOREACH(ShipList, ship_saves, ship) // FOREACH_FIXME
		ch->Send(" %2d) %s\n", ++count, ship->name);
	if (count == 0)
		ch->Send(" None.\n");
}

ACMD(do_stlist) {
	int count = 0;
	StellarObject *object;
	char *buf = get_buffer(MAX_STRING_LENGTH * 4);

	sprintf(buf, "%-30s %-16s %-16s %-14s\n",
		"Name", "Location", "Type", "Habitat");
	strcat(buf, "------------------------------ ---------------- ---------------- --------------\n");

	FOREACH(StellarObjectList, stellar_objects, object) // FOREACH_FIXME
	{
		sprintf(buf + strlen(buf), "`%c%-30.30s`n %-16.16s %-16.16s %-14.14s\n",
			ZoneData::GetZone(object->zone) == NULL ? 'R' : 'n',
			object->name, object->starsystem->name,
			StellarObjectTypes[object->type], HabitatTypes[object->habitat]);
//			count++);
	}
	page_string(ch->desc, buf);
	release_buffer(buf);
}

ACMD(do_sylist) {
	StarSystem *system;
	char *buf = get_buffer(MAX_STRING_LENGTH);

	FOREACH(StarSystemList, starsystem_list, system) // FOREACH_FIXME
	{
		sprintbit(STR_FLAGS(system), StarSystemFlags, buf);
		ch->Send("%-30s %s\n", system->name, buf);
	}
	release_buffer(buf);
}

ACMD(do_shiplist) {
	ShipData *ship;
	bool found = FALSE;

	if (!has_comlink(ch)) {
		ch->Send("You need a comlink to communicate with your ship and find its location.\n");
		return;
	}

	skip_spaces(argument);
	ch->Send("%-36.36s  %-42.42s\n", "Ship", "Location");
	ch->Send("%-36.36s  %-42.42s\n",
		"--------------------------------------------------------------------------------",
		"--------------------------------------------------------------------------------");

	FOREACH(ShipList, ship_list, ship) // FOREACH_FIXME
	{
		if ((IS_STAFF(ch) && !str_cmp(argument, "all"))
		|| (check_owner(ch, ship) && ship->type != MOB_SHIP)) {
			found = TRUE;
			ch->Send("%-36.36s  `%s%-34.34s`n", ship->name,
				ship->starsystem != NULL ? ship->state == SHIP_HYPERSPACE ? "`Y" : "G" : "n",
				IN_ROOM(ship) ?
					IN_ROOM(ship)->GetName() :
					ship->starsystem != NULL ? ship->starsystem->name :
					ship->state == SHIP_HYPERSPACE ? "HYPERSPACE	" : "NOWHERE");
			if (IS_STAFF(ch))
				ch->Send(" [%5d]", IN_ROOM(ship) == NULL ? -2 : IN_ROOM(ship)->GetVirtualNumber());
			ch->Send("\n");
		}
	}

	if (!found)
		ch->Send("   No ships found.\n");
}

ACMD(do_shiprecall) {
	char *buf;
	ShipData *ship;
	bool found = FALSE;
	StellarObject *object;

	skip_spaces(argument);

	if (!*argument) {
		ch->Send("Call what ship? Type 'ships' for a list of your ships.\n");
		return;
	}

#if 0
	if (GET_QP(ch) < 10) {
		ch->Send("You must have 10bp to call your ship.\n");
		return;
	}
#endif

	if (!has_comlink(ch)) {
		ch->Send("You need a comlink to communicate with your ship.\n");
		return;
	}

	if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_CAN_LAND)) {
		ch->Send("You must be at a landing pad to call your ship.\n");
		return;
	}

	FOREACH(ShipList, ship_list, ship) // FOREACH_FIXME
	{
		if (!check_pilot(ch, ship) || !str_cmp(ship->owner, "Public"))
			continue;

		if (!str_cmp(argument, ship->name)
		|| !str_prefix(argument, ship->name)
		|| isname(argument, ship->name)) {
			found = TRUE;
			break;
		}
	}

	if (!found) {
		ch->Send("No ship by that name found.\n");
		return;
	}

	if (ship->hyperspeed == 0) {
		ch->Send("`RThat ship is not equipped with a hyperdrive!\n");
		return;
	}

	if (ship->shipclass > SPACE_STATION) {
		ch->Send("`RThat isn't a spacecraft.`n\n");
		return;
	}

	if (ship->shipclass == SPACE_STATION) {
		ch->Send("You can't do that.\n");
		return;
	}

	if (ship->lastdoc != ship->location) {
		ch->Send("`rThe ship doesn't seem to be docked right now.`n\n");
		return;
	}

	if (ship->state != SHIP_DOCKED) {
		ch->Send("That ship is not docked right now.\n");
		return;
	}

	if (SHP_FLAGGED(ship, SHIP_DISABLED)) {
		ch->Send("Ship's drive is damaged, no power for launch.\n");
		return;
	}

	if ((object = IN_ROOM(ship)->zone->planet) == NULL) {
		ch->Send("Your ship is on a non-existant planet.\n");
		return;
	}

	if (object->starsystem == NULL) {
		ch->Send("Your ship is in a non-existant starsystem.\n");
		return;
	}

	if ((object = ch->InRoom()->zone->planet) == NULL) {
		ch->Send("The ship cannot locate the planet you are on.\n");
		return;
	}

	if (object->starsystem == NULL) {
		ch->Send("The ship cannot locate the starsystem you are in.\n");
		return;
	}

	if (ship->hyperspeed == 1 && object->starsystem != IN_ROOM(ship)->zone->planet->starsystem) {
		ch->Send("`RThe ship cannot travel outside of its starsystem.`n\n");
		return;
	}

	buf = get_buffer(MAX_STRING_LENGTH);

	if (ship->hatchopen) {
		ship->hatchopen = FALSE;
		sprintf(buf, "`YThe hatch on %s closes.`n", ship->name);
		echo_to_room(IN_ROOM(ship), buf);
		echo_to_room(ship->entrance, "`YThe hatch slides shut.`n");
	}

	one_argument(object->name, buf);
	sprintf(buf, "%s %s", buf, ch->InRoom()->GetName());

	ship->jx				= object->x + Number(-50, 50);
	ship->jy				= object->y + Number(-50, 50);
	ship->jz				= object->z + Number(-50, 50);
	ship->dest				= str_dup(buf);

	ship->hull				= ship->maxhull;
	ship->shield		= ship->maxshield;
	ship->energy		= ship->maxenergy;
	ship->currjump		= object->starsystem;
	ship->speed				= ship->maxspeed;

	if (IN_ROOM(ship)->zone->planet->starsystem == ship->currjump)
		ship->hyperdistance = Number(1, 450);
	else
		ship->hyperdistance = Number(600, 1000);

	ship->state		= SHIP_LAUNCH;
	ship->primstate		= LASER_READY;
	ship->secondstate		= MISSILE_READY;
	ship->recall		= SHIP_LAUNCH;
	SET_BIT(SHP_FLAGS(ship), SHIP_AUTOPILOT);

	ch->Send("`GLaunch sequence initiated.`n\n");
	echo_to_ship(TO_COCKPIT, ship, "`GLaunch sequence initiated.`n");
	echo_to_ship(TO_SHIP, ship, "`YThe ship hums as it lifts off the ground.`n");
	sprintf(buf, "`Y%s begins to launch.`n", ship->name);
	echo_to_room(IN_ROOM(ship), buf);
//	GET_QP(ch) -= 10;
	release_buffer(buf);
}

ACMD(do_shipfire) {
	float chance = 0;
	Turret *turret = NULL;
	ShipData *ship, *target = NULL;

	skip_spaces(argument);

	if ((ship = ship_from_turret(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be in the gunners chair or turret of a ship to do that!`n\n");
		return;
	}

	if (ship->state == SHIP_HYPERSPACE) {
		ch->Send("`RYou can only do that in realspace!`n\n");
		return;
	}

	if (ship->starsystem == NULL && ship->shipclass <= SPACE_STATION) {
		ch->Send("`RYou can't do that until after you've finished launching!`n\n");
		return;
	}

	if (ship->energy < 5) {
		ch->Send("`RThere is not enough energy left to fire!`n\n");
		return;
	}

	if (autofly(ship)) {
		ch->Send("`RYou'll have to turn off the ships autopilot first.`n\n");
		return;
	}

	if (Event::FindEvent(ship->m_Events, DisruptionWebEvent::Type)) {
		ch->Send("Cannot fire while trapped in a disruption web!\n");
		return;
	}

//	chance = GET_AGI(ch) * 2 + GET_SKILL(ch, SKILL_GUNNERY);
	chance = 50;
	if (ship->gunseat == IN_ROOM(ch) && !str_cmp(argument, "missile")) {
		if (model_list[ship->model].secondam <= 0 || !str_cmp("None", air_weap[ship->model])) {
			ch->Send("`RThis ship isn't equiped with missiles!`n\n");
			return;
		}

		if (ship->secondstate == MISSILE_DAMAGED) {
			ch->Send("`RThe ships missile launchers are damaged.`n\n");
			return;
		}

		if (Event::FindEvent(ship->m_Events, MissileCooldownEvent::Type)) {
			ch->Send("`RThe missiles are still reloading.`n\n");
			return;
		}

		if (ship->target == NULL) {
			ch->Send("`RYou need to choose a target first.`n\n");
			return;
		}

		target = ship->target;
		if (target->starsystem != ship->starsystem || !ch->CanSee(target)) {
			ship->SetTarget(NULL);
			ch->Send("`RYour target seems to have left.`n\n");
			return;
		}

		if (ship->shipclass <= SPACE_STATION && ship->Distance(target) > 1000) {
			ch->Send("`RThat ship is out of missile range.`n\n");
			return;
		}

		if (ship->shipclass < SPACE_STATION && !is_facing(ship, target)) {
			ch->Send("`RMissiles can only fire in a forward. You'll need to turn your ship!`n\n");
			return;
		}

		chance -= target->manuever / 5;
		chance -= target->speed / 20;
		chance += target->model * target->model * 2;
		chance -= (flabs(target->vx - ship->vx) / 100);
		chance -= (flabs(target->vy - ship->vy) / 100);
		chance -= (flabs(target->vz - ship->vz) / 100);
		chance += 30;
		chance = URANGE(20, chance, 90);

		if (chance < Number(0, 100)) {
			ch->Send("`RYou fail to lock onto your target!`n\n");
			WAIT_STATE(ch, NO_STAFF_HASSLE(ch) ? 1 : 2 RL_SEC);
			return;
		}

		new_missile(ship, target, ch);
		act("$n presses the fire button.", TRUE, ch, 0, 0, TO_ROOM);
		echo_to_ship(TO_COCKPIT, ship, "`YMissile launched.`n");
		echo_to_ship(TO_COCKPIT, target, "`rIncoming missile from %s`n", ship->name);
		echo_to_system(ship, target, "`y%s fires a missile towards %s.`n", ship->name, target->name);

		ship->m_Events.push_front(new MissileCooldownEvent(model_list[ship->model].mcooldown, ship->GetID()));

		if (autofly(target) && target->target != ship) {
			target->SetTarget(ship);
			echo_to_ship(TO_COCKPIT, ship, "`rYou are being targetted by %s.`n", target->name);
		}
		return;
	}

	if (ship->gunseat == IN_ROOM(ch)) {
		if (model_list[ship->model].primdam <= 0 || !str_cmp("None", ground_weap[ship->model])) {
			ch->Send("`RThis ship isn't equiped for that.`n\n");
			return;
		}

		if (ship->primstate == LASER_DAMAGED) {
			ch->Send("`RThe ships main weapons are damaged.`n\n");
			return;
		}

		if (Event::FindEvent(ship->m_Events, VehicleCooldownEvent::Type)) {
			ch->Send("`RThe weapons are still recharging.`n\n");
			return;
		}

		if (ship->target == NULL) {
			ch->Send("`RYou need to choose a target first.`n\n");
			return;
		}
		target = ship->target;
	} else {
		FOREACH(TurretList, ship->turrets, turret) // FOREACH_FIXME
			if (turret->room == IN_ROOM(ch))
				break;

		if (!turret) {
			ch->Send("`RThis turret is not functioning.`n\n");
			return;
		}

		if (turret->laserstate == LASER_DAMAGED) {
			ch->Send("`RThe turret is damaged.`n\n");
			return;
		}

		if (turret->laserstate >= 1) {
			ch->Send("`RThe lasers are still recharging.`n\n");
			return;
		}

		if (turret->target == NULL) {
			ch->Send("`RYou need to choose a target first.`n\n");
			return;
		}
		target = turret->target;
	}

	if (target->starsystem != ship->starsystem || !ch->CanSee(target)) {
		ship->SetTarget(NULL);
		ch->Send("`RYour target seems to have left.`n\n");
		return;
	}

	if (ship->shipclass <= SPACE_STATION && ship->Distance(target) > 500) {
		ch->Send("`RThat ship is out of range.`n\n");
		return;
	}

	if (ship->shipclass < SPACE_STATION && !is_facing(ship, target)) {
		ch->Send("`RThe main weapons can only fire forward. You'll need to turn your ship!`n\n");
		return;
	}

	if (ship->gunseat == IN_ROOM(ch)) {
		ship->primstate++;
		ship->m_Events.push_front(new VehicleCooldownEvent(model_list[ship->model].cooldown, ship->GetID()));
	} else
		turret->laserstate++;

	chance += target->model * 5;
	chance -= target->manuever / 10;
	chance -= target->speed / 20;
	chance -= (flabs(target->vx - ship->vx) / 70);
	chance -= (flabs(target->vy - ship->vy) / 70);
	chance -= (flabs(target->vz - ship->vz) / 70);
	chance = URANGE(10, chance, 90);

	act("$n presses the fire button.", TRUE, ch, 0, 0, TO_ROOM);

	int result;
	char *buf1 = get_buffer(MAX_INPUT_LENGTH);
	char *buf2 = get_buffer(MAX_INPUT_LENGTH);

	if (turret) {
		sprintf(buf1, "lasers");
		sprintf(buf2, "Laserfire");
	} else {
		sprintf(buf1, "%s", ground_weap_pers[ship->model]);
		sprintf(buf2, "%s", ground_weap_othr[ship->model]);
	}

	if (Number(0, 100) > chance) {
		echo_to_ship(TO_COCKPIT, target, "`y%s from %s fire at you but miss.`n", buf2, ship->name);
		echo_to_ship(TO_COCKPIT, ship, "`yThe ship's %s fire at %s but miss.`n", buf1, target->name);
		if (ship->shipclass > SPACE_STATION) {
			sprintf(buf1, "`y%s from %s barely misses %s.`n", buf2, ship->name, target->name);
			echo_to_room(IN_ROOM(ship), buf1);
		} else
			echo_to_system(ship, target, "`y%s from %s barely misses %s.`n", buf2, ship->name, target->name);
		release_buffer(buf1);
		release_buffer(buf2);
		return;
	}

	echo_to_ship(TO_COCKPIT, target, "`rYou are hit by %s from %s!`n", buf1, ship->name);
	echo_to_ship(TO_COCKPIT, ship, "`YYour %s hits %s!.`n", buf1, target->name);
	if (ship->shipclass > SPACE_STATION) {
		sprintf(buf1, "`y%s from %s hits %s.`n", buf2, ship->name, target->name);
		echo_to_room(IN_ROOM(ship), buf1);
	} else
		echo_to_system(ship, target, "`y%s from %s hits %s.`n", buf2, ship->name, target->name);

	release_buffer(buf1);
	release_buffer(buf2);

	if (turret)
		result = target->Damage(ch, 2 + ship->upgrades[UPGRADE_GROUND], DAM_LASER);
	else
		result = target->Damage(ch, ship->primdam + ship->upgrades[UPGRADE_GROUND], DAM_LASER);

	if (result)
		echo_to_ship(TO_SHIP, target, "`rA small explosion vibrates through the ship.`n");

	if (!target->IsPurged() && target->starsystem && autofly(target) && target->target != ship) {
		target->SetTarget(ship);
		echo_to_ship(TO_COCKPIT, ship, "`rYou are being targetted by %s.`n", target->name);
	}
}

ACMD(do_repairship) {
	ShipData *ship;

	switch (ch->m_Substate) {
		default:
			skip_spaces(argument);

/*			if (!CLS_FLAGGED(ch, CLASS_ENGINEER | CLASS_MUTATOR | CLASS_MACHINATE))
				ch->Send("You do not know how to do that.\n");
			else*/ if ((ship = ship_from_engine(IN_ROOM(ch))) == NULL)
				ch->Send("`RYou must be in the engine room of a ship to do that!`n\n");
//			else if (!str_cmp(argument, "hull") && !GET_SKILL(ch, SKILL_REPAIR))
//				ch->Send("`RYou do not know how to repair the hull!`n\n");
			else if (str_cmp(argument, "hull") && str_cmp(argument, "drive")
			&& str_cmp(argument, "energy")
			&& str_cmp(argument, "launcher") && str_cmp(argument, "primary")) {
				ch->Send("`rYou need to specify something to repair:`n\n");
				ch->Send("`rTry: hull, drive, launcher, primary`n\n");
			} else {
				ch->Send("`GYou begin your repairs.`n\n");
				act("$n begins repairing the ship.", TRUE, ch, 0, 0, TO_ROOM);
				ch->AddTimer(NO_STAFF_HASSLE(ch) ? 1 : !str_cmp(argument, "hull") ? 15 RL_SEC : 5 RL_SEC, do_repairship, 1,
						str_dup(argument));
			}
			return;
		case 1:
			ch->m_Substate = SUB_NONE;
			if (!((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer() || !*(char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer())
				return;
			break;

		case SUB_TIMER_DO_ABORT:
			ch->Send("`RYou are distracted and fail to finish your repairs.`n\n");
			ch->m_Substate = SUB_NONE;
			return;
	}

	if ((ship = ship_from_engine(IN_ROOM(ch))) == NULL)
		return;

	if (!str_cmp((char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer(), "energy")) {
		AlterEnergy(ship, -ship->maxenergy);
		ch->Send("`GEnergy regenerated.`n\n");
	}

	else if (!str_cmp((char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer(), "hull")) {
		int change = URANGE(0, /*Number(GET_SKILL(ch, SKILL_REPAIR) / 2, GET_SKILL(ch, SKILL_REPAIR))*/ 100, ship->maxhull - ship->hull);

		ship->hull += change;
		ch->Send("`GRepair complete.. Hull strength inreased by %d points.`n\n", change);
	}

	else if (!str_cmp((char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer(), "drive")) {
		if (ship->location == ship->lastdoc)
			ship->state = SHIP_DOCKED;

		ch->Send("`GShips drive repaired.`n\n");
		REMOVE_BIT(SHP_FLAGS(ship), SHIP_DISABLED);

		if (Number(1, 5) == Number(1, 10)) {
			ch->Send("`GYou find a problem in the drive and correct the error.`n\n");
			REMOVE_BIT(SHP_FLAGS(ship), SHIP_SABOTAGE);
		}
	}

	else if (!str_cmp((char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer(), "launcher")) {
		ship->secondstate = MISSILE_READY;
		ch->Send("`GMissile launcher repaired.`n\n");
	}

	else if (!str_cmp((char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer(), "primary")) {
		ship->primstate = LASER_READY;
		ch->Send("`GMain weapon repaired.`n\n");
	}
	act("$n finishes $s repairs.", TRUE, ch, 0, 0, TO_ROOM);
}

ACMD(do_shipupgrade) {
	ShipData *ship;

	if ((ship = ship_from_engine(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be in the engine room of a ship to do that!`n\n");
		return;
	}

/*	if (GET_QP(ch) < 100) {
		ch->Send("`RYou do not have enough credits to upgrade this ship.`n\n");
		return;
	}
*/
	if (!check_pilot(ch, ship)) {
		ch->Send("This is not your ship.\n");
		return;
	}

	skip_spaces(argument);
	if (str_cmp(argument, "armor") && str_cmp(argument, "shields")
	&& str_cmp(argument, "interceptors")
	&& str_cmp(argument, "primary") && str_cmp(argument, "missiles")) {
		ch->Send("`rYou need to spceify something to upgrade:`n\n");
		ch->Send("`rTry: armor, shields, primary, missiles, interceptors`n\n");
		return;
	}

	if (!str_cmp(argument, "armor")) {
		if (ship->upgrades[UPGRADE_ARMOR] >= 3) {
			ch->Send("This ship's armor cannot be upgraded any further.\n");
			return;
		}
		ship->upgrades[UPGRADE_ARMOR]++;
	}

	if (!str_cmp(argument, "shields")) {
		if (ship->maxshield <= 0) {
			ch->Send("This ship is not equipped with shields.\n");
			return;
		}

		if (ship->upgrades[UPGRADE_SHIELD] >= 3) {
			ch->Send("This ship's shields cannot be upgraded any further.\n");
			return;
		}
		ship->upgrades[UPGRADE_SHIELD]++;
	}

	if (!str_cmp(argument, "primary")) {
		if (!str_cmp("None", ground_weap[ship->model]) && ship->model != CARRIER) {
			ch->Send("This ship is not equipped with weapons.\n");
			return;
		}

		if (ship->upgrades[UPGRADE_GROUND] >= 3) {
			ch->Send("This ship's primary weapons cannot be upgraded any further.\n");
			return;
		}
		ship->upgrades[UPGRADE_GROUND]++;
	}

	if (!str_cmp(argument, "missiles")) {
		if (!str_cmp("None", air_weap[ship->model])) {
			ch->Send("This ship is not equipped with missiles.\n");
			return;
		}

		if (ship->upgrades[UPGRADE_AIR] >= 3) {
			ch->Send("This ship's missiles cannot be upgraded any further.\n");
			return;
		}
		ship->upgrades[UPGRADE_AIR]++;
	}

	if (!str_cmp(argument, "interceptors")) {
		if (ship->model != CARRIER) {
			ch->Send("This is not a Protoss Carrier!\n");
			return;
		}

		if (ship->upgrades[UPGRADE_INTERCEPTORS]) {
			ch->Send("This ship already has the interceptor upgrade.\n");
			return;
		}
		ship->upgrades[UPGRADE_INTERCEPTORS]++;
	}

//	GET_QP(ch) -= 100;
	ch->Send("`GUpgrade complete.`n\n");
	act("$n upgrades the ship's $T.", TRUE, ch, 0, argument, TO_ROOM);
	ship->addsave();
	return;
}

ACMD(do_tranship) {
	ShipData *ship;

	skip_spaces(argument);
	if ((ship = get_ship(argument)) == NULL) {
		ch->Send("No such ship.\n");
		return;
	}

	ship->FromRoom();
	ship->ToRoom(IN_ROOM(ch));

	ship->location = ship->lastdoc = ch->InRoom();
	ship->state = SHIP_DOCKED;

	if (ship->starsystem)
		ship->FromSystem();

	ship->addsave();
	ch->Send("Ship transfered.\n");
}

ACMD(do_addpilot) {
	ShipData *ship;

	if ((ship = ship_from_cockpit(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be in the cockpit of a ship to do that!`n\n");
		return;
	}

	if (ship->shipclass == SPACE_STATION) {
		ch->Send("`RYou can't do that here.`n\n");
		return;
	}

	if (str_cmp(ch->GetName(), ship->owner)) {
		ch->Send("`RThat isn't your ship!`n\n");
		return;
	}

	skip_spaces(argument);

	if (!*argument) {
		ch->Send("Add which pilot?\n");
		return;
	}

	if (str_cmp(ship->pilot, "")) {
		if (str_cmp(ship->copilot, "")) {
			ch->Send("You already have a pilot and copilot.\n");
			return;
		}

		FREE(ship->copilot);
		ship->copilot = str_dup(argument);
		ch->Send("Copilot Added.\n");
		ship->addsave();
		return;
	}

	free(ship->pilot);
	ship->pilot = str_dup(argument);
	ch->Send("Pilot Added.\n");
	ship->addsave();
	return;
}

ACMD(do_rempilot) {
	ShipData *ship;

	if ((ship = ship_from_cockpit(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be in the cockpit of a ship to do that!`n\n");
		return;
	}

	if (ship->shipclass == SPACE_STATION) {
		ch->Send("`RYou can't do that here!`n\n");
		return;
	}

	if (str_cmp(ch->GetName(), ship->owner)) {
		ch->Send("`RThat isn't your ship!`n\n");
		return;
	}

	skip_spaces(argument);

	if (!*argument) {
		ch->Send("Remove which pilot?\n");
		return;
	}

	if (!str_cmp(ship->pilot, argument)) {
		free(ship->pilot);
		ship->pilot = str_dup("");
		ch->Send("Pilot Removed.\n");
		ship->addsave();
		return;
	}

	if (!str_cmp(ship->copilot, argument)) {
		free(ship->copilot);
		ship->copilot = str_dup("");
		ch->Send("Copilot Removed.\n");
		ship->addsave();
		return;
	}

	ch->Send("That person isn't listed as a pilot of this ship.\n");
	return;
}

#define MISC		0
#define BINARY		1
#define NUMBER		2

StellarObject *get_stellarobject(char *name) {
	StellarObject *i = NULL;

	FOREACH(StellarObjectList, stellar_objects, i) // FOREACH_FIXME
	{
		if (!str_cmp(i->name, name))
			return i;
	}
	return NULL;
}

struct stedit_struct {
	char *cmd;
	char type;
} stedit_fields[] = {
	{ "name",		MISC },		// 0
	{ "\n",		MISC },
};

bool perform_stedit(CharData *ch, StellarObject *object, int mode, char *val_arg) {
	char *output;
	int value = 0;
	bool on = false, off = false;

	output = get_buffer(MAX_STRING_LENGTH);
	if (stedit_fields[mode].type == BINARY) {
		if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
			on = true;
		else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
			off = true;
		if (!(on || off)) {
			ch->Send("Value must be 'on' or 'off'.\n");
			release_buffer(output);
			return 0;
		}
		sprintf(output, "%s %s for %s.", stedit_fields[mode].cmd, ONOFF(on), object->name);
	} else if (stedit_fields[mode].type == NUMBER) {
		value = atoi(val_arg);
		sprintf(output, "%s on %s set to %d.", stedit_fields[mode].cmd, object->name, value);
	} else
		strcpy(output, "Okay.");

	switch (mode) {
		case 0:
			free(object->name);
			object->name = str_dup(val_arg);
			sprintf(output, "Name changed to '%s'.", val_arg);
			break;
		default:
			ch->Send("Can't set that!\n");
			release_buffer(output);
			return false;
	}
	ch->Send("%s\n", CAP(output));
	release_buffer(output);
	return true;
}

ACMD(do_stedit) {
	bool retval = false;
	StellarObject *object;
	int mode = -1, len = 0;
	char *buf = get_buffer(MAX_INPUT_LENGTH);
	char *name = get_buffer(MAX_INPUT_LENGTH);
	char *field = get_buffer(MAX_INPUT_LENGTH);

	half_chop(argument, name, buf);
	half_chop(buf, field, buf);

	if (!*name || !*field)
		ch->Send("Usage: stedit <object> <field> <value>\n");

	if (!(object = get_stellarobject(name))) {
		ch->Send("No such stellar object!\n");
		release_buffer(field);
		release_buffer(name);
		release_buffer(buf);
		return;
	}

	len = strlen(field);
	for (mode = 0; *(stedit_fields[mode].cmd) != '\n'; mode++)
		if (!strncmp(field, stedit_fields[mode].cmd, len))
			break;

	retval = perform_stedit(ch, object, mode, buf);
	if (retval) {
	}

	release_buffer(buf);
	release_buffer(name);
	release_buffer(field);
}

ACMD(do_setship) {
	ShipData *ship;
	char *buf = get_buffer(MAX_INPUT_LENGTH);
	char *name = get_buffer(MAX_INPUT_LENGTH);
	char *field = get_buffer(MAX_INPUT_LENGTH);

	half_chop(argument, name, buf);
	half_chop(buf, field, buf);

	if (!*name || !*field)
		ch->Send("Usage: setship <ship> <field> <value>\n");
	else {
		ship = get_ship(name);

		if (ship != NULL) {
			if (!str_cmp(field, "owner")) {
				free(ship->owner);
				ship->owner = str_dup(buf);
				ch->Send("Owner set.\n");
				ship->addsave();
			} else if (!str_cmp(field, "name")) {
				free(ship->name);
				ship->name = str_dup(buf);
				ch->Send("Name set.\n");
				ship->addsave();
			} else if (!str_cmp(field, "manuever")) {
				ship->manuever = URANGE(0, atoi(buf), 255);
				ch->Send("Manuever set.\n");
				ship->addsave();
			} else if (!str_cmp(field, "primary")) {
				ship->primdam = URANGE(0, atoi(buf), 100);
				ch->Send("Primary damage set.\n");
				ship->addsave();
			} else if (!str_cmp(field, "missile")) {
				ship->secondam = URANGE(0, atoi(buf), 100);
				ch->Send("Missile damage set.\n");
				ship->addsave();
			} else if (!str_cmp(field, "speed")) {
				ship->maxspeed = URANGE(0, atoi(buf), 255);
				ch->Send("Speed set.\n");
				ship->addsave();
			} else if (!str_cmp(field, "hyper")) {
				ship->hyperspeed = URANGE(0, atoi(buf), 255);
				ch->Send("Hyperspeed set.\n");
				ship->addsave();
			} else if (!str_cmp(field, "shields")) {
				ship->maxshield = URANGE(0, atoi(buf), 999);
				ch->Send("Shields set.\n");
				ship->addsave();
			} else if (!str_cmp(field, "hull")) {
				ship->maxhull = URANGE(0, atoi(buf), 999);
				ch->Send("Hull set.\n");
				ship->addsave();
			} else
				ch->Send("No such field, try again.\n");
		} else {
			ch->Send("There is no such ship.\n");
		}
	}
	release_buffer(buf);
	release_buffer(name);
	release_buffer(field);
}

#if 0
ACMD(do_sabotage) {
	ObjData *obj = NULL;
	ShipData *ship = NULL;

	switch (ch->m_Substate) {
		default:
			if ((ship = ship_from_engine(IN_ROOM(ch))) == NULL)
				ch->Send("You must be in the engine room of the ship to do that.\n");
			else if (!CLS_FLAGGED(ch, CLASS_ENGINEER | CLASS_MUTATOR | CLASS_MACHINATE))
				ch->Send("You do not know how to sabotage hyperdrives.\n");
			else if (ship->hyperspeed <= 1)
				ch->Send("This ship is not equiped with a hyperdrive.\n");
			else if (!NO_STAFF_HASSLE(ch) && !(obj = get_obj_in_list_type(ITEM_TOOLKIT, ch->carrying)))
				ch->Send("You need a toolkit to sabotage this hyperdrive.\n");
			else if (!NO_STAFF_HASSLE(ch) && !str_cmp("Public", ship->owner))
				ch->Send("Intergalactic Shuttles would be very angry if you messed with their ship.\n");
			else {
				ch->Send("`GYou begin to sabotage the hyperdrive.`n\n");
				act("$n begins sabotaging the ship.", TRUE, ch, 0, 0, TO_ROOM);
				ch->AddTimer(NO_STAFF_HASSLE(ch) ? 1 : 60 RL_SEC, do_sabotage, 1);
			}
			return;

		case 1:
			ch->m_Substate = SUB_NONE;
			break;

		case SUB_TIMER_DO_ABORT:
			ch->Send("You are interrupted and can't finish your sabotage.\n");
			act("$n is interrupted and can't finish $s sabotage.", FALSE, ch, 0, 0, TO_ROOM);
			ch->m_Substate = SUB_NONE;
			return;
	}

	if ((ship = ship_from_engine(IN_ROOM(ch))) == NULL)
		return;
	if (!NO_STAFF_HASSLE(ch) && !(obj = get_obj_in_list_type(ITEM_TOOLKIT, ch->carrying)))
		return;

	if (ch->SkillRoll(SKILL_ELECTRONICS, Intelligence)) {
		SET_BIT(SHP_FLAGS(ship), SHIP_SABOTAGE);
		ship->addsave();
	}
	ch->Send("You finish your work on the hyperdrive.\n");
	act("$n has finished $s work on the hyperdrive.", FALSE, ch, 0, 0, TO_ROOM);
	if (obj)
		obj->extract();
	return;
}
#endif

ACMD(do_ping) {
	ShipData *ship, *target;

	if ((ship = ship_from_pilotseat(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be in the pilot seat of a ship to do that!`n\n");
		return;
	}

	switch (ship->state) {
		case SHIP_DOCKED:
			ch->Send("`RWait until after you launch!`n\n");
			return;
		case SHIP_HYPERSPACE:
			ch->Send("`RYou can only do that in normal space.`n\n");
			return;
		default:
			break;
	}

	if (ship->starsystem == NULL) {
		ch->Send("`RYou can't do that unless the ship is flying in normal space.`n\n");
		return;
	}

	act("$n does an active sonar ping.", TRUE, ch, 0, 0, TO_ROOM);
	ship->Appear();
	echo_to_system(ship, NULL, "%s makes an active sonar ping.", ship->name);
	ch->Send("%s:\n\n", ship->starsystem->name);

	FOREACH(ShipList, ship->starsystem->ships, target) // FOREACH_FIXME
	{
		if (target == ship)
			continue;

		if (target->IsCloaked() && ship->Distance(target) <= 1500)
			ch->Send("`C%-34s`n   `C??? ??? ???; (???)`n\n", ch->CanSee(target) ? target->name : "A faint shimmer");
		else if (ship->Distance(target) <= 1000) {
			ch->Send("`C%-34s`n   `C%.0f %.0f %.0f; (%.0f)`n\n",
				target->name, target->vx, target->vy, target->vz, ship->Distance(target));
		}
	}
}

ACMD(do_interceptor) {
	CharData *i;
	ShipData *ship;

	switch (ch->m_Substate) {
		default:
			if ((ship = ship_from_turret(IN_ROOM(ch))) == NULL) {
				ch->Send("`RYou must be in the gunners seat or turret of a ship to do that!`n\n");
				return;
			}

			if (ship->model != CARRIER) {
				ch->Send("`RThis ship doesn't have any interceptors!`n\n");
				return;
			}

			if (ship->state == SHIP_HYPERSPACE) {
				ch->Send("`RYou can only do that in realspace!`n\n");
				return;
			}

			if (!ship->starsystem) {
				ch->Send("`RYou can't do that until you've finished launching!`n\n");
				return;
			}

			if (autofly(ship)) {
				ch->Send("`RYou'll have to turn off the ships autopilot first....`n\n");
				return;
			}

			if (ship->interceptors >= (ship->upgrades[UPGRADE_INTERCEPTORS] ? 8 : 4)) {
				ch->Send("`RYou have made the maximum number of interceptors!`n\n");
				return;
			}

			ch->Send("You begin making an interceptor.\n");
			ch->AddTimer(NO_STAFF_HASSLE(ch) ? 1 : 20 RL_SEC, do_interceptor, 1);
			return;

		case 1:
			ch->m_Substate = SUB_NONE;
			break;

		case SUB_TIMER_DO_ABORT:
			ch->Send("Interceptor build aborted.\n");
			ch->m_Substate = SUB_NONE;
			return;
	}

	if ((ship = ship_from_turret(IN_ROOM(ch))) == NULL)
		return;

	if (ship->interceptors < ship->upgrades[UPGRADE_INTERCEPTORS] ? 8 : 4)
		ship->interceptors++;

	ch->Send("Interceptor built.\n");
	WAIT_STATE(ch, 1 RL_SEC);

	FOREACH(CharList, ch->InRoom()->people, i) // FOREACH_FIXME
	{
		if (ch == i)		continue;
		if (i->m_pTimerCommand && i->m_pTimerCommand->GetType() == ActionTimerCommand::Type &&
							((ActionTimerCommand *)i->m_pTimerCommand)->m_pFunction == do_interceptor) {
			i->Send("Interceptor built.\n");
			i->ExtractTimer();
		}
	}
}

ACMD(do_deploy) {
	ShipData *ship;

	if ((ship = ship_from_turret(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be in the gunners seat or turret of a ship to do that!`n\n");
		return;
	}

	if (ship->model != CARRIER) {
		ch->Send("`RThis ship doesn't have any interceptors!`n\n");
		return;
	}

	if (ship->state == SHIP_HYPERSPACE) {
		ch->Send("`RYou can only do that in realspace!`n\n");
		return;
	}

	if (!ship->starsystem) {
		ch->Send("`RYou can't do that until you've finished launching!`n\n");
		return;
	}

	if (autofly(ship)) {
		ch->Send("`RYou'll have to turn off the ships autopilot first....`n\n");
		return;
	}

	if (ship->interceptors < 1) {
		ch->Send("There are no interceptors constructed to deploy.\n");
		return;
	}

	skip_spaces(argument);
	if (!str_cmp(argument, "recall")) {
		ch->Send("`YYou recall all interceptors.`n\n");
		act("$n presses a button on the weapon console.", TRUE, ch, 0, 0, TO_ROOM);
		remove_interceptors(ship);
		return;
	}

	ch->Send("`YYou deploy all interceptors!`n\n");
	act("$n presses a button on the weapon console.", TRUE, ch, 0, 0, TO_ROOM);
	deploy_interceptors(ship);
}

ACMD(do_drive) {
	char *arg;
	int dir;
	ShipData *vehicle, *vehicle_in_out;

	if (ch->sitting_on && IS_MOUNTABLE(ch->sitting_on)) {
		do_olddrive(ch, argument, cmd, command, subcmd);
		return;
	}

	if ((vehicle = ship_from_pilotseat(IN_ROOM(ch))) == NULL) {
//		ch->Send("You can't do that here.\n");
		do_olddrive(ch, argument, cmd, command, subcmd);
		return;
	}

	if (vehicle->shipclass != LAND_VEHICLE) {
		ch->Send("This vehicle isn't equiped for land-travel.\n");
		return;
	}

	if (SHP_FLAGGED(vehicle, SHIP_DISABLED)) {
		ch->Send("The engine has been disabled.\n");
		return;
	}

	if (vehicle->energy < 1) {
		ch->Send("There is not enough fuel.\n");
		return;
	}

	arg = get_buffer(MAX_INPUT_LENGTH);
	argument = one_argument(argument, arg);

	if (!*arg)
		ch->Send("Drive which direction?\n");
	else if (is_abbrev(arg, "into")) {
		RoomData * is_going_to;
		one_argument(argument, arg);

		if (!*arg)
			ch->Send("Drive into what?\n");
		else if (!(vehicle_in_out = ship_in_room(IN_ROOM(vehicle), arg)))
			ch->Send("Nothing here by that name!\n");
		else if (!vehicle_in_out->bayopen)
			ch->Send("That vehicle's baydoors are closed.\n");
		else if (vehicle_in_out->garages.Count() < 1)
			ch->Send("There are no garages in that.\n");
		else {
			RoomData * garage;

			// check for capacity
			FOREACH(RoomList, vehicle_in_out->garages, garage) // FOREACH_FIXME
				if (garage->ships.Count() < 2)
					is_going_to = garage;

			if (is_going_to == NULL)
				ch->Send("That vehicle is full.\n");
			else {
				act("You drive into $V.", TRUE, ch, 0, vehicle_in_out, TO_CHAR);
				act("$n drives into $V.", TRUE, ch, 0, vehicle_in_out, TO_ROOM);
				IN_ROOM(vehicle)->Send("%s enters %s.\n", vehicle->name, vehicle_in_out->name);

				VehicleMove(vehicle, is_going_to);
				IN_ROOM(vehicle)->Send("%s enters.\n", vehicle->name);
				look_at_room(ch, IN_ROOM(vehicle), FALSE);
			}
		}
	} else if (is_abbrev(arg, "out")) {
		if ((vehicle_in_out = ship_from_room(IN_ROOM(vehicle))) == NULL)
			ch->Send("Nowhere to drive out of.\n");
		else if (!vehicle_in_out->bayopen)
			ch->Send("Baydoors are closed, cannot drive out.\n");
		else if (vehicle_in_out->lastdoc != vehicle_in_out->location)
			ch->Send("Maybe you should wait until the ship lands.\n");
		else if (vehicle_in_out->state != SHIP_DOCKED)
			ch->Send("Please wait until the ship is properly docked.\n");
		else if (IN_ROOM(vehicle_in_out) == NULL)
			ch->Send("The ship seems to be.. stuck.. someplace..");
		else {
			IN_ROOM(vehicle)->Send("%s exits %s.\n", vehicle->name, vehicle_in_out->name);
			VehicleMove(vehicle, IN_ROOM(vehicle_in_out));

			IN_ROOM(vehicle)->Send("%s drives out of %s.\n", vehicle->name, vehicle_in_out->name);
			act("$n drives out of $V.", TRUE, ch, 0, (ObjData *)vehicle_in_out, TO_ROOM);
			act("You drive $v out of $V.", TRUE, ch, (ObjData *)vehicle, vehicle_in_out, TO_CHAR);
			look_at_room(ch, IN_ROOM(vehicle), FALSE);
		}
	} else if ((dir = search_block(arg, dirs, FALSE)) >= 0) {
		if (!ch || (dir < 0) || (dir >= NUM_OF_DIRS) || (IN_ROOM(vehicle) == NULL))
			send_to_char("Sorry, an internal error has occurred.\n", ch);
		else if (!EXITN_OPEN(IN_ROOM(vehicle), dir))
			send_to_char("Alas, you cannot go that way...\n", ch);
		else if (EXIT_STATE_FLAGGED(EXIT(vehicle, dir), EX_STATE_CLOSED)) {
			if (*EXIT(vehicle, dir)->GetKeyword())
				act("The $F seems to be closed.", FALSE, ch, 0, const_cast<char *>(EXIT(vehicle, dir)->GetKeyword()), TO_CHAR);
			else
				act("It seems to be closed.", FALSE, ch, 0, 0, TO_CHAR);
		} else if (!ROOM_FLAGGED(EXIT(vehicle, dir)->ToRoom(), ROOM_VEHICLE) || EXIT_FLAGGED(EXIT(vehicle, dir), EX_NOVEHICLES))
			send_to_char("The vehicle can't manage that terrain.\n", ch);
		else if (ROOM_SECT(EXIT(vehicle, dir)->ToRoom()) == SECT_FLYING && vehicle->shipclass > AIRCRAFT) {
			if (ROOM_SECT(IN_ROOM(vehicle)) == SECT_SPACE)
				act("$V can't enter the atmosphere.", FALSE, ch, 0, vehicle, TO_CHAR);
			else
				act("$V can't fly.", FALSE, ch, 0, vehicle, TO_CHAR);
		} else if ((ROOM_SECT(EXIT(vehicle, dir)->ToRoom()) == SECT_WATER_NOSWIM) && vehicle->shipclass != BOAT && vehicle->shipclass != SUBMARINE)
			act("$V can't go in such deep water!", FALSE, ch, 0, vehicle, TO_CHAR);
		else if ((ROOM_SECT(EXIT(vehicle, dir)->ToRoom()) == SECT_UNDERWATER) && vehicle->shipclass != SUBMARINE)
			act("$V can't go underwater!", FALSE, ch, 0, vehicle, TO_CHAR);
		else if (ROOM_SECT(EXIT(vehicle, dir)->ToRoom()) == SECT_DEEPSPACE && vehicle->shipclass != SPACECRAFT)
			act("$V can't go into space!", FALSE, ch, 0, vehicle, TO_CHAR);
		else if (ROOM_SECT(EXIT(vehicle, dir)->ToRoom()) == SECT_SPACE && vehicle->shipclass != SPACECRAFT)
			act("$V cannot handle space travel.", FALSE, ch, 0, vehicle, TO_CHAR);
/*
		else if ((ROOM_SECT(EXIT(vehicle, dir)->ToRoom) > SECT_FIELD) &&
		(ROOM_SECT(EXIT(vehicle, dir)->ToRoom) <= SECT_MOUNTAIN) &&
		!VEHICLE_FLAGGED(vehicle, VEHICLE_GROUND))
			act("The $o cannot land on that terrain.", FALSE, ch, vehicle, 0, TO_CHAR);
*/
		else {
			IN_ROOM(vehicle)->Send("%s leaves %s.\n", vehicle->name, dirs[dir]);
			VehicleMove(vehicle, IN_ROOM(vehicle)->dir_option[dir]->ToRoom());
			IN_ROOM(vehicle)->Send("%s enters from %s.\n", vehicle->name, dir_text_the[rev_dir[dir]]);
			act("$n drives $T.", TRUE, ch, 0, dirs[dir], TO_ROOM);
			act("You drive $T.", TRUE, ch, 0, dirs[dir], TO_CHAR);
			look_at_room(ch, IN_ROOM(vehicle), FALSE);
//			WAIT_STATE(ch, MAX(1, 100 - vehicle->maxspeed));
			vehicle->addsave();
		}
	} else
		ch->Send("Thats not a valid direction.\n");
	release_buffer(arg);
}

ACMD(do_remote) {
	ShipData *ship;
	char *arg1 = get_buffer(MAX_INPUT_LENGTH);
	char *arg2 = get_buffer(MAX_INPUT_LENGTH);
	char * commands[] = { "course", "launch", "land", "scan", "status", "radar", "speed", "autopilot", "baydoors", "\n" };
	skip_spaces(argument);
	argument = one_argument(argument, arg1);
	one_argument(argument, arg2);

	if (!*arg1)
		ch->Send("Remote pilot which ship?\n");
	else if (!*argument)
		ch->Send("What do you want the ship to do?\n");
	else if (FIGHTING(ch))
		ch->Send("Not while fighting!\n");
	else if (!has_comlink(ch))
		ch->Send("You need a comlink to remote-pilot your ship.\n");
	else if (!search_block(arg2, commands, true))
		ch->Send("You cannot do that through a comlink.\n");
	else {
		bool found = false;
		FOREACH(ShipList, ship_list, ship) // FOREACH_FIXME
		{
			if (!check_pilot(ch, ship) || !str_cmp(ship->owner, "Public"))
				continue;

			if (!str_cmp(arg1, ship->name) || !str_prefix(arg1, ship->name) || isname(arg1, ship->name)) {
				found = true;
				break;
			}
		}

		if (!found)
			ch->Send("No ship by that name found.\n");
		else if (SHP_FLAGGED(ship, SHIP_DISABLED))
			ch->Send("That ship's drive is damaged, no power for comm.\n");
		else {
			RoomData * oLoc = IN_ROOM(ch);

			ch->FromRoom();
			ch->ToRoom(ship->pilotseat);
			command_interpreter(ch, argument);
			ch->FromRoom();
			ch->ToRoom(oLoc);
		}
	}
	release_buffer(arg1);
	release_buffer(arg2);
}

ACMD(do_jumpvector) {
	float rand, tx, ty, tz;
	ShipData *ship, *target;

	rand = 1.0 / (float)Number(-160, 160);		// seems pointless to be float

	if ((ship = ship_from_cockpit(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be in the cockpit of a ship to do that!`n\n");
		return;
	}

	switch (ship->state) {
		case SHIP_DOCKED:
			ch->Send("`RWait until after you launch!`n\n");
			return;
		case SHIP_HYPERSPACE:
			ch->Send("`RYou can only do that in normal space.`n\n");
			return;
		default:
			break;
	}

	if (!ship->starsystem) {
		ch->Send("`RYou can't do that unless the ship is flying in normal space.`n\n");
		return;
	}

	skip_spaces(argument);
	if (!(target = get_ship(argument)) || !ch->CanSee(target)) {
		ch->Send("No such ship.\n");
		return;
	}

	if (ship == target) {
		ch->Send("You can figure out where you are going on your own.\n");
		return;
	}

	if (!ship_was_in_range(ship, target) || target->state == SHIP_DOCKED) {
		ch->Send("No log for that ship.\n");
		return;
	}

#if 0
	if (!ch->SkillRoll(SKILL_TRACKING, Intelligence)) {
		ch->Send("`RYou cant figure out the course vectors correctly.`n\n");
		return;
	}
#endif

	if (target->state == SHIP_HYPERSPACE) {
		tx = target->vx + rand;
		ty = target->vy + rand;
		tz = target->vz + rand;

		ch->Send("After some deliberation, you figure out its projected course.\n");
		echo_to_ship(TO_COCKPIT, ship, "%s Heading: %s, %.0f %.0f %.0f", target->name, target->currjump->name, tx, ty, tz);
		return;
	}
	ch->Send("`RYou cant figure out the course vectors correctly.`n\n");
}


ACMD(do_foldspace) {
// SKILL_FOLDSPACE / 25 = level
// Lvl 1: Transport Other from Same System
// Lvl 2: Transport Other from Any System
// Lvl 3: Transport Other and all Friendlies within 5au from any system
// Lvl 4: Requires 40% less power to fold
// may need to modify CanSee(ship) to check for starsystems
	int level = 0;
	LList<ShipData *>friendlies;
	ShipData *ship, *target, *fshp;

	switch (ch->m_Substate) {
		default:
			skip_spaces(argument);
			if ((ship = ship_from_pilotseat(IN_ROOM(ch))) == NULL)
				ch->Send("`RYou must be in the pilots seat of a ship to do that!`n\n");
			else if (!check_pilot(ch, ship))
				ch->Send("`RThis isn't your ship!`n\n");
			else if (ship->speed > 0)
				ch->Send("`RThe ship cannot be moving while folding space!`n\n");
			else if (ship->shipclass > SPACE_STATION)
				ch->Send("`RThis isn't a spacecraft!`n\n");
			else if (!NO_STAFF_HASSLE(ch) && ship->model != ARBITER)
				ch->Send("`RThis ship isn't an Arbiter!`n\n");
			else if (ship->state == SHIP_DOCKED)
				ch->Send("`RYou can't do that until after you've launched!`n\n");
			else if (ship->starsystem == NULL)
				ch->Send("`RYou can only do that in realspace.`n\n");
			else if (!NO_STAFF_HASSLE(ch) && ship->energy < 1000)
				ch->Send("`RThe ship needs at least 1000 units of energy!`n\n");
			else if (!*argument)
				ch->Send("What ship do you wish to summon?\n");
			else {
//				level = URANGE(1, ch->ShipSkill(ship) / 25, 4);
				level = 4;
				switch (level) {
					case 1:
						if ((target = get_ship_here(argument, ship->starsystem)) == NULL || !ch->CanSee(target)) {
							ch->Send("`RThat ship is not in this starsystem!`n\n");
							return;
						}
						break;
					case 2:
					case 3:
					case 4:
						if ((target = get_ship(argument)) == NULL
						|| !ch->CanSee(target) || !target->starsystem) {
							ch->Send("`RThere is no such ship in space!`n\n");
							return;
						}
						break;
					default:
						ch->Send("Uh.. there is a problem with you. Contact staff.\n");
						return;
				}
				if (ship == target) {
					ch->Send("Uh.. no..\n");
					return;
				}
				if (!NO_STAFF_HASSLE(ch) && ship->GetRelation(target) != RELATION_FRIEND) {
					ch->Send("You are not allied with that ship!\n");
					return;
				}
				AlterEnergy(ship, level == 4 ? 375 : 500);
				ch->Send("You begin to fold space around %s.\n", target->name);
				act("$n begins concentrating on folding space.", FALSE, ch, 0, 0, TO_ROOM);
				target->speed = 0;
				echo_to_ship(TO_COCKPIT, target, "`mA swirling vortex in space appears in front of the ship.`n");
				echo_to_system(target, NULL, "`mA swirling vortex in space appears in front of %s.`n", target->name);
				ch->AddTimer(NO_STAFF_HASSLE(ch) ? 1 : 3 RL_SEC, do_foldspace, 1,
						str_dup(argument));
			}
			return;

		case 1:
			ch->m_Substate = SUB_NONE;
			if (!((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer() || !*(char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer())
				return;
			break;

		case SUB_TIMER_DO_ABORT:
			ch->Send("You fail to complete the fold!\n");
			ch->m_Substate = SUB_NONE;
			return;
	}

	if ((ship = ship_from_pilotseat(IN_ROOM(ch))) == NULL)
		return;

//	if ((level = URANGE(1, ch->ShipSkill(ship) / 25, 4)) == 1)
//		target = get_ship_here(((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer(), ship->starsystem);
//	else
		target = get_ship((char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer());

	if (ship == target)
		return;				// whatever

	if (!target || !target->starsystem || target->IsPurged() || !ch->CanSee(target)) {
		ch->Send("`RTarget has been lost!`n\n");
		return;
	}

#if 0
	if (!ch->SkillRoll(ship->Skill(), Perception)) {
		ch->Send("`RFold failed!`n\n");
		return;
	}
#endif
	AlterMove(ch, -20);
	AlterEnergy(ship, level == 4 ? 375 : 500);

	ch->Send("`GFold level %d complete!`n\n", level);
	act("$n folds space, creating a swirling vortex of energy.", FALSE, ch, 0, 0, TO_ROOM);

	if (level >= 3) {
		FOREACH(ShipList, target->starsystem->ships, fshp) // FOREACH_FIXME
			if (target != fshp && ship != fshp && friendlies.Count() < 10
			&& target->GetRelation(fshp) == RELATION_FRIEND
			&& fshp->Distance(target) <= 100)
				friendlies.Append(fshp);
	}

	echo_to_system(ship, NULL, "`m%s creates a swirling vortex of energy!`n", ship->name);

	echo_to_ship(TO_COCKPIT, target, "`mThe ship is pulled into the vortex!`n");
	echo_to_system(target, NULL, "`m%s disappears within the vortex.`n", target->name);
	target->FromSystem();
	target->ToSystem(ship->starsystem);
	target->vx = ship->vx + Number(-100, 100);
	target->vy = ship->vy + Number(-100, 100);
	target->vz = ship->vz + Number(-100, 100);
	echo_to_system(target, NULL, "`Y%s enters the starsystem at %.0f %.0f %.0f`n", target->name, target->vx, target->vy, target->vz);

	FOREACH(ShipList, friendlies, fshp) // FOREACH_FIXME
	{
		fshp->speed = 0;
		echo_to_ship(TO_COCKPIT, fshp, "`mThe ship is pulled into the vortex!`n");
		echo_to_system(fshp, NULL, "`m%s disappears within the vortex.`n", fshp->name);
		fshp->FromSystem();
		fshp->ToSystem(ship->starsystem);
		fshp->vx = ship->vx + Number(-100, 100);
		fshp->vy = ship->vy + Number(-100, 100);
		fshp->vz = ship->vz + Number(-100, 100);
		echo_to_system(fshp, NULL, "`Y%s enters the starsystem at %.0f %.0f %.0f`n", fshp->name, fshp->vx, fshp->vy, fshp->vz);
	}
}

ACMD(do_wormhole) {
	ShipData *ship;
	StarSystem *system;
	StellarObject *object;

	switch (ch->m_Substate) {
		default:
			skip_spaces(argument);
			if ((ship = ship_from_engine(IN_ROOM(ch))) == NULL)
				ch->Send("`RYou must be in the engine room of a ship to do that!`n\n");
			else if (ship->speed > 0)
				ch->Send("`RThe ship cannot be moving while creating a wormhole!`n\n");
			else if (!check_pilot(ch, ship))
				ch->Send("`RThis isn't your ship!`n\n");
			else if (ship->shipclass > SPACE_STATION)
				ch->Send("`RThis isn't a spacecraft!`n\n");
			else if (ship->model != ARBITER)
				ch->Send("`RThis ship isn't an Arbiter!`n\n");
			else if (ship->state == SHIP_DOCKED)
				ch->Send("`RYou can't do that until after you've launched!`n\n");
			else if (ship->starsystem == NULL)
				ch->Send("`RYou can only do that in realspace.`n\n");
			else if (ship->energy < 4000)
				ch->Send("`RThe ship must have 4000 units of energy to create a wormhole!`n\n");
			else if (!*argument)
				ch->Send("Where do you wish to create the wormhole?\n");
			else if (!(system = starsystem_from_name(argument)))
				ch->Send("`RThat starsystem does not exist!`n\n");
			else if (ship->starsystem == system)
				ch->Send("`RYou cannot create a wormhole into this system!`n\n");
			else {
				FOREACH(StellarObjectList, ship->starsystem->stellarObjects, object) // FOREACH_FIXME
				{
					if (object->type == Wormhole) {
						ch->Send("You cannot create two wormholes in the same system, they would be unstable!\n");
						return;
					}
					if (ship->Distance(object) < 500) {
						ch->Send("`RThe gravity of %s prevents the creation of a wormhole.`n\n", object->name);
						return;
					}
				}
				FOREACH(StellarObjectList, system->stellarObjects, object) // FOREACH_FIXME
				{
					if (object->type == Wormhole) {
						ch->Send("You cannot create two wormholes in the same system, they would be unstable!\n");
						return;
					}
					if (ship->Distance(object) < 500) { // heh bug
						ch->Send("`RThe gravity of %s prevents the creation of a wormhole.`n\n", object->name);
						return;
					}
				}
				ch->Send("You focus your psionic energies and begin creating a wormhole.\n");
				act("$n begins focusing $s psionic energies.", FALSE, ch, 0, 0, TO_ROOM);
				AlterEnergy(ship, 100);
				ch->AddTimer(NO_STAFF_HASSLE(ch) ? 1 : 6 RL_SEC, do_wormhole, 1,
						str_dup(argument));
			}
			return;

		case 1:
			ch->m_Substate = SUB_NONE;
			if (!((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer() || !*(char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer())
				return;
			break;

		case SUB_TIMER_DO_ABORT:
			ch->Send("You fail to finish creating the wormhole!\n");
			ch->m_Substate = SUB_NONE;
			return;
	}

	if ((ship = ship_from_engine(IN_ROOM(ch))) == NULL || !ship->starsystem)
		return;

	if ((system = starsystem_from_name((char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer())) == NULL)
		return;		// wierd, starsystem up and vanished

	if (/*!ch->SkillRoll(ship->Skill(), Perception) ||*/ ship->speed > 0 || ship->energy < 4000) {
		ch->Send("Your energies become unstable and the wormhole collapses.\n");
		return;
	}
	if (!NO_STAFF_HASSLE(ch))
		AlterEnergy(ship, ship->energy);

	char *buf = get_buffer(MAX_STRING_LENGTH);

	// first create a wormhole in the ship's system
	object = new StellarObject(Wormhole);
	stellar_objects.Append(object);
	object->connects = system;
	object->starsystem = ship->starsystem;
	ship->starsystem->stellarObjects.Append(object);
	object->habitat = Uninhabitable;
	object->x = ship->vx;
	object->y = ship->vy;
	object->z = ship->vz;
	sprintf(buf, "Wormhole to %s", system->name);
	object->name = str_dup(buf);
	SET_BIT(object->flags, OBJECT_NOSAVE);
	object->events.push_front(new WormholeCollapseEvent(10 * (60 RL_SEC), object));
	ch->Send("Your energies bend space-time, forming a stable wormhole.\n");
	act("$n releases $s energies, which are magnified by the ship!", FALSE, ch, 0, 0, TO_ROOM);
	echo_to_ship(TO_SHIP, ship, "A vibration shudders through the ship as a wormhole opens in front of it.");
	echo_to_system(ship, NULL, "`YA wormhole forms at %.0f %.0f %.0f.`n", object->x, object->y, object->z);

	// now create the wormhole in the other system
	object = new StellarObject(Wormhole);
	stellar_objects.Append(object);
	object->connects = ship->starsystem;
	object->starsystem = system;
	system->stellarObjects.Append(object);
	object->habitat = Uninhabitable;
	object->x = ship->vx;
	object->y = ship->vy;
	object->z = ship->vz;
	sprintf(buf, "Wormhole to %s", ship->starsystem->name);
	object->name = str_dup(buf);
	SET_BIT(object->flags, OBJECT_NOSAVE);
	object->events.push_front(new WormholeCollapseEvent(10 * (60 RL_SEC), object));
	echo_to_starsystem(system, "`YA wormhole forms at %.0f %.0f %.0f.`n", object->x, object->y, object->z);
	release_buffer(buf);
}

ACMD(do_disruptionweb) {
	ShipTimerEvent *event;
	ShipData *ship, *target;

	switch (ch->m_Substate) {
		default:
		skip_spaces(argument);
		if ((ship = ship_from_pilotseat(IN_ROOM(ch))) == NULL)
			ch->Send("You must be in the gunner's seat to do that!\n");
//		else if (!GET_SKILL(ch, SKILL_DISRUPTIONWEB))
//			ch->Send("You do not know how to do that.\n");
		else if (!check_pilot(ch, ship))
			ch->Send("This isn't your ship!\n");
		else if (ship->shipclass > SPACE_STATION)
			ch->Send("`RThis isn't a spacecraft!`n\n");
		else if (!SHP_FLAGGED(ship, SHIP_WEBBER))
			ch->Send("This ship cannot do that!\n");
//		else if (ship->model != CORSAIR)
//			ch->Send("This isn't a Corsair!\n");
		else if (ship->energy < 1000)
			ch->Send("You need at least 1000 units of energy!\n");
		else if (!*argument)
			ch->Send("Web what ship?\n");
		else if ((target = get_ship_here(argument, ship->starsystem)) == NULL)
			ch->Send("That ship isn't here.\n");
		else if (!ch->CanSee(target))
			ch->Send("That ship isn't here.\n");
		else if (ship->Distance(target) > 1000)
			ch->Send("That ship is too far away!\n");
		else if ((event = (ShipTimerEvent *)Event::FindEvent(ship->m_Events, ShipTimerEvent::Type)) && event->GetTimerType() == STYPE_WEBBING)
			ch->Send("Disruption web still charging.\n");
		else {
			ch->Send("You charge the disruption thingy.\n");
			act("$n begins charging the disruption thingy.", FALSE, ch, 0, 0, TO_ROOM);
			ch->AddTimer(NO_STAFF_HASSLE(ch) ? 1 : 2 RL_SEC, do_disruptionweb, 1,
					str_dup(argument));
		}
		return;

		case 1:
			ch->m_Substate = SUB_NONE;
			if (!((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer() || !*(char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer())
				return;
			break;

		case SUB_TIMER_DO_ABORT:
			ch->Send("You are interrupted and the disruption thingy fails.\n");
			ch->m_Substate = SUB_NONE;
			return;
	}

	if ((ship = ship_from_pilotseat(IN_ROOM(ch))) == NULL)
		return;

	if ((target = get_ship_here((char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer(), ship->starsystem)) == NULL)
		return;

	if (Event::FindEvent(target->m_Events, DisruptionWebEvent::Type))
		return;

	AlterEnergy(ship, 1000);
	ch->Send("`GDisruption web launched.. or something.`n\n");
	echo_to_ship(TO_COCKPIT, target, "`R%s hits you with a disruption web.`n", ship->name);
	echo_to_system(target, ship, "`r%s hits %s with a disruption web.`n", ship->name, target->name);

	if (!IS_STAFF(ch))
		WAIT_STATE(ch, 4 RL_SEC);
	target->m_Events.push_front(new DisruptionWebEvent(60 RL_SEC, target->GetID()));
	ship->m_Events.push_front(new ShipTimerEvent(120 RL_SEC, ship->GetID(), STYPE_WEBBING));
}

ACMD(do_shieldmatrix) {
	ShipData *ship, *target;

	switch (ch->m_Substate) {
		default:
			skip_spaces(argument);
			if ((ship = ship_from_pilotseat(IN_ROOM(ch))) == NULL)
				ch->Send("You must be in the pilotseat to do that!\n");
			else if (!check_pilot(ch, ship))
				ch->Send("This isn't your ship!\n");
			else if (ship->shipclass > SPACE_STATION)
				ch->Send("`RThis isn't a spacecraft!`n\n");
			else if (ship->model != SCIENCEVESSEL)
				ch->Send("This isn't a Science Vessel!\n");
			else if (ship->energy < 1000)
				ch->Send("You need at least 1000 units of energy!\n");
			else if (!*argument)
				ch->Send("What ship?\n");
			else if ((target = get_ship_here(argument, ship->starsystem)) == NULL)
				ch->Send("That ship isn't here.\n");
			else {
				ch->Send("You initialize the defense matrix.\n");
				act("$n initializes the defense matrix.", FALSE, ch, 0, 0, TO_ROOM);
				ch->AddTimer(NO_STAFF_HASSLE(ch) ? 1 : 2 RL_SEC, do_shieldmatrix, 1,
						str_dup(argument));
			}
			return;

		case 1:
			ch->m_Substate = SUB_NONE;
			if (!((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer() || !*(char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer())
				return;
			break;

		case SUB_TIMER_DO_ABORT:
			ch->Send("You are interrupted, and the shield matrix destabalizes.\n");
			ch->m_Substate = SUB_NONE;
			return;
	}

	if ((ship = ship_from_pilotseat(IN_ROOM(ch))) == NULL)
		return;

	if ((target = get_ship_here((char *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer(), ship->starsystem)) == NULL)
		return;
	
	Event *event;
	if ((event = Event::FindEvent(target->m_Events, ShieldMatrixEvent::Type))) {
		target->m_Events.remove(event);
		event->Cancel();
		echo_to_ship(TO_COCKPIT, target, "%s replenishes your defense matrix.", ship->name);
	} else
		echo_to_ship(TO_COCKPIT, target, "%s creates a defense matrix around your hull.", ship->name);

	target->tempshield = 250;
	AlterEnergy(ship, 1000);
	ch->Send("`GDefense matrix generated!`n\n");
	echo_to_system(target, ship, "%s creates a defense matrix around %s.", ship->name, target->name);
//  target->tempshield = (GET_SKILL(ch, SKILL_SHIELDMATRIX) / 25) * 100;

	target->m_Events.push_front(new ShieldMatrixEvent(120 RL_SEC, target->GetID()));
}


ACMD(do_orbitalinsertion) {
	VNum vnum;
	RoomData * room;
	ShipData *ship;
	int count, which;
	StellarObject *planet;
	LList<RoomData *> roomList;

	if (!IS_STAFF(ch))
		return;

	if ((ship = ship_from_room(IN_ROOM(ch))) == NULL) {
		ch->Send("`RYou must be in a ship to do that!`n\n");
		return;
	}

	switch (ship->state) {
		case SHIP_DOCKED:
			ch->Send("`RWait until after you launch!`n\n");
			return;
		case SHIP_HYPERSPACE:
			ch->Send("`RYou can only do that in normal space.`n\n");
			return;
		default:
			break;
	}

	if (!ship->starsystem) {
		ch->Send("`RYou can't do that unless the ship is flying in normal space.`n\n");
		return;
	}

	ZoneData *zone = NULL;
	if ((planet = ship->NearestObject()) == NULL || (zone = ZoneData::GetZone(planet->zone)) == NULL || ship->Distance(planet) > 5000) {
		ch->Send("No planets detected within deployment range.\n");
		return;
	}

	for (vnum = (zone->GetVirtualNumber() * 100); vnum <= zone->top; vnum++) {
		if ((room = World::GetRoomByVNum(vnum)) && ROOM_FLAGGED(room, ROOM_SURFACE))
			roomList.Append(room);
	}
	if ((count = roomList.Count()) == 0) {
		ch->Send("No suitable landing zone confirmed.\n");
		return;
	}
	which = Number(0, count - 1);
	FOREACH(RoomList, roomList, room) // FOREACH_FIXME
		if (which-- == 0)
			break;

	act("$n steps into a deployment tube and is shot into space!", TRUE, ch, 0, 0, TO_ROOM);
	ch->Send("Your shot out of the ship and quickly land on a planet!\n");
	ch->FromRoom();
	ch->ToRoom(room);
	look_at_room(ch, 0);
	act("$n falls from the sky and lands making a giant crater!", TRUE, ch, 0, 0, TO_ROOM);

//  check for insertion armor, if none kill the bitch ch->Die();
}


unsigned int ShipTimerEvent::Run() {
    ShipData *ship = find_ship(m_Ship);

    if (!ship || ship->IsPurged())
		return 0;

    ship->m_Events.remove(this);
    switch (m_Type) {
		case STYPE_BAYDOORS: {
		    FOREACH(HangerList, ship->hangers, hanger)
				(*hanger)->bayopen = !ship->bayopen;
		    if (ship->bayopen) {
			ship->bayopen = FALSE;
			echo_to_ship(TO_COCKPIT, ship, "`YAll bay doors closing.`n");
			echo_to_system(ship, NULL, "`Y%s bay doors closing.`n", ship->name);
		    } else {
			ship->bayopen = TRUE;
			echo_to_ship(TO_COCKPIT, ship, "`YAll bay doors opening.`n");
			echo_to_system(ship, NULL, "`Y%s bay doors opening.`n", ship->name);
		    }
		    break;
		}

		case STYPE_WEBBING:
		    echo_to_ship(TO_COCKPIT, ship, "`yDisruption web recharged.");
		    break;

		case STYPE_CLOAKING:
		    if (SHP_FLAGGED(ship, SHIP_CLOAKED))
			ship->Appear();
		    else {
			if (ship->target) {
			    echo_to_ship(TO_COCKPIT, ship, "`RShip engaged with enemy, cloaking aborted.`n");
			    break;
			}
			AlterEnergy(ship, 500);
			echo_to_ship(TO_COCKPIT, ship, "`YThe ship cloaks and disappears.`n");
			echo_to_system(ship, NULL, "`K%s shimmers briefly then disappears.`n", ship->name);
			SET_BIT(SHP_FLAGS(ship), SHIP_CLOAKED);
		    }
		    break;
		default:
		    mudlogf(BRF, LVL_STAFF, TRUE, "Invalid Ship Event type #%d from %s.", m_Type, ship->name);
		    break;
    }
    return 0;
}


unsigned int VehicleCooldownEvent::Run() {
    ShipData *ship = find_ship(m_Ship);

    if (!ship || ship->IsPurged())
	return 0;

    ship->m_Events.remove(this);
    echo_to_ship(TO_GUNSEAT, ship, "`yPrimary weapon recharged.`n");
    return 0;
}

unsigned int MissileCooldownEvent::Run()
{
    ShipData *ship = find_ship(m_Ship);

    if (!ship || ship->IsPurged())
	return 0;

    ship->m_Events.remove(this);
    echo_to_ship(TO_GUNSEAT, ship, "`yMissile launcher reloaded.`n");
    return 0;
}

unsigned int DisruptionWebEvent::Run()
{
    ShipData *ship = find_ship(m_Ship);

    if (!ship || ship->IsPurged())
	return 0;
    
    ship->m_Events.remove(this);
    echo_to_ship(TO_COCKPIT, ship, "`yThe disruption web dissipates.`n");
    echo_to_system(ship, NULL, "`yThe disruption web on %s dissipates.`n", ship->name);
    return 0;
}

unsigned int ShieldMatrixEvent::Run()
{
    ShipData *ship = find_ship(m_Ship);

    if (!ship || ship->IsPurged())
	return 0;

    ship->m_Events.remove(this);
    ship->tempshield = 0;
    echo_to_ship(TO_COCKPIT, ship, "`yThe defense matrix dissipates.`n");
    echo_to_system(ship, NULL, "`yThe defense matrix around %s dissipates.`n", ship->name);
    return 0;
}

unsigned int WormholeCollapseEvent::Run()
{
    StellarObject *object = m_Object;
    
    if (!object)
	return 0;

    if (!stellar_objects.InList(object))
	return 0;

    object->events.Remove(this);
    echo_to_starsystem(object->starsystem, "`YThe %s collapses upon itself.`n", object->name);
    object->Extract();
    return 0;
}



ShipData *find_ship(IDNum n) {
    ShipData *ship = NULL;

    FOREACH(ShipList, ship_list, ship) // FOREACH_FIXME
    {
		if (n == ship->GetID())
		    break;
    }
    return ship;
}


