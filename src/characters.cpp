/*************************************************************************
*   File: characters.c++             Part of Aliens vs Predator: The MUD *
*  Usage: Primary code for characters                                    *
*************************************************************************/

#include <math.h>
#include <stdarg.h>
#include <map>
#include <vector>

#include "structs.h"
#include "utils.h"
#include "scripts.h"
#include "comm.h"
#include "buffer.h"
#include "files.h"
#include "interpreter.h"
#include "affects.h"
#include "db.h"
#include "clans.h"
#include "opinion.h"
#include "alias.h"
#include "event.h"
#include "handler.h"
#include "spells.h"
#include "track.h"
#include "find.h"
#include "constants.h"
#include "lexifile.h"
#include "lexiparser.h"

#include "imc/imc.h"

// External functions
int death_mtrigger(CharData *ch, CharData *actor);
void death_otrigger(CharData *actor, CharData *killer);
void kill_otrigger(CharData *actor, CharData *victim);
void kill_mtrigger(CharData *actor, CharData *victim);
void death_cry(CharData *ch);
void announce(const char *string);
void make_corpse(CharData *ch, CharData *killer);
bool find_name(const char *name);
void tag_argument(char *argument, char *tag);
void CheckRegenRates(CharData *ch);
void load_mtrigger(CharData *ch);
void load_otrigger(ObjData *ch);
int wear_otrigger(ObjData *obj, CharData *actor, int where);
void quit_otrigger(ObjData *obj, CharData *actor);

CharacterList				character_list;		//	global linked list of chars
CharacterPrototypeMap		mob_index;	/* index table for mobile file	 */

PlayerTable	player_table;
PlayerData dummy_player;	/* dummy spec area for mobs	 */
MobData dummy_mob;	/* dummy spec area for players	 */



PlayerIMCData::PlayerIMCData() :
	m_Deaf(0),
	m_Allow(0),
	m_Deny(0)
{
}


PlayerData::PlayerData(void) :
	m_Version(0),
	m_BadPasswordAttempts(0),
	
	m_Creation(0),
	m_LoginTime(0),
	m_LastLogin(0),
	m_TotalTimePlayed(0),
	m_IdleTime(0),
//	m_PlayerID(0),
	m_Channels(0),
	m_LastTellReceived(0),
	m_LastTellSent(0),
	m_StaffPrivileges(0),
	m_StaffInvisLevel(0),
	m_PageLength(DEFAULT_PAGE_LENGTH),
	m_Preferences(0),
	m_Karma(0),
	m_FreezeLevel(0),
	m_MutedAt(0),
	m_MuteLength(0),
	m_SkillPoints(0),
	m_LifetimeSkillPoints(0),
	m_pClanMembership(NULL),
	m_PlayerKills(0),
	m_MobKills(0),
	m_PlayerDeaths(0),
	m_MobDeaths(0)
{
	memset(m_Password, 0, sizeof(m_Password));
	memset(m_RealSkills, 0, sizeof(m_RealSkills));
	memset(m_Skills, 0, sizeof(m_Skills));
}

PlayerData::~PlayerData(void)
{
}



MobData::MobData(void) :
	m_BaseHP(0),
	m_VaryHP(0),
//	m_StartingEquipment,
	m_DefaultPosition(POS_STANDING),
	m_AttackSpeed(0.0f),
	m_AttackRate(0),
	m_AttackDamage(0),
	m_AttackDamageType(0),
	m_AttackType(0),
	m_WaitState(0),
	m_UpdateTick(0),
	m_AIAggrRoom(0),
	m_AIAggrRanged(0),
	m_AIAggrLoyalty(0),
	m_AIAwareRoom(0),
	m_AIAwareRange(0),
	m_AIMoveRate(0)
{
}


CharData::CharData(void)
:	m_Prototype(NULL)
,	m_InRoom(NULL)
,	m_WasInRoom(NULL)
,	m_PreviouslyInRoom(NULL)
,	m_Sex(SEX_NEUTRAL)
,	m_Race(RACE_UNDEFINED)
,	m_Level(0)
,	m_Height(72)	//	Good default
,	m_Weight(200)	//	Good default
,	m_pPlayer(NULL)
,	mob(NULL)
,	desc(NULL)
,	m_Clan(NULL)
,	m_Team(0)
,	m_NumCarriedItems(0)
,	m_CarriedWeight(0)
,	m_Position(POS_STANDING)
,	m_ActFlags(0)
,	m_LastNoticeRollTime(0)
,	m_WatchingDirection(INVALID_DIRECTION)
,	fighting(NULL)
,	m_LastAttacker(0)
,	m_CombatMode(COMBAT_MODE_DEFENSIVE)
,	m_Following(NULL)
,	m_pTimerCommand(NULL)
,	m_Substate(0)
,	m_SubstateNewTime(0)
,	move_wait(0)
,	path(NULL)
,	sitting_on(NULL)
{
	memset(equipment, 0, sizeof(equipment));
	memset(points_event, 0, sizeof(points_event));
}


//	Copy constructor, yay!
CharData::CharData(const CharData &ch)
:	Entity(ch)
,	m_Prototype(ch.m_Prototype)
,	m_InRoom(NULL)
,	m_WasInRoom(NULL)
,	m_PreviouslyInRoom(NULL)
,	m_Name(ch.m_Name)
,	m_Keywords(ch.m_Keywords)
,	m_RoomDescription(ch.m_RoomDescription)
,	m_Description(ch.m_Description)
//,	m_Title(ch.m_Title)
,	m_Sex(ch.m_Sex)
,	m_Race(ch.m_Race)
,	m_Level(ch.m_Level)
,	m_Height(ch.m_Height)
,	m_Weight(ch.m_Weight)
,	real_abils(ch.real_abils)
,	aff_abils(ch.aff_abils)
,	m_pPlayer(&dummy_player)
,	mob(NULL)
,	desc(NULL)
,	m_Clan(ch.m_Clan)
,	m_Team(ch.m_Team)
,	m_NumCarriedItems(0)
,	m_CarriedWeight(0)
,	m_AffectFlags(ch.m_AffectFlags)
,	points(ch.points)
,	m_Position(ch.m_Position)
,	m_ActFlags(ch.m_ActFlags)	//	Warning: Double usage here, MOB_FLAGS/PLR_FLAGS
,	m_LastNoticeRollTime(0)
,	m_WatchingDirection(INVALID_DIRECTION)
,	fighting(NULL)
,	m_LastAttacker(0)
,	m_CombatMode(ch.m_CombatMode)
,	m_Following(NULL)
,	m_pTimerCommand(NULL)
,	m_Substate(0)
,	m_SubstateNewTime(0)
,	move_wait(0)
,	path(NULL)
,	sitting_on(NULL)
{
	int	eq;
	
	memset(equipment, 0, sizeof(equipment));
	memset(points_event, 0, sizeof(points_event));
	
	this->mob = (!ch.mob || (ch.mob == &dummy_mob)) ? new MobData() : new MobData(*ch.mob);
	
	if (!IS_NPC(&ch))	//	Making a mob from a player
	{
		BUFFER(buf, MAX_STRING_LENGTH);
		
		if (IS_STAFF(&ch))		sprintf(buf, "%s\n", ch.GetName());
		else					sprintf(buf, "`(%s`) %s\n", ch.GetTitle(), ch.GetName());
		
		m_Keywords = strip_color(buf);
		m_Name = ch.GetName();
		m_RoomDescription = buf;
		
		MOB_FLAGS(this) = MOB_ISNPC;	//	Gaurantee - no duping players, lest to make a mob
		
		GET_DEFAULT_POS(this) = POS_DEAD;
	}
	
	FOREACH_CONST(Lexi::List<Affect *>, ch.m_Affects, aff)
	{
		m_Affects.push_back(new Affect(**aff, this));
	}
	
	for (eq = 0; eq < NUM_WEARS; eq++)
	{
		if (ch.equipment[eq])
		{
			ObjData *item = ObjData::Clone(ch.equipment[eq]);
			//	Shortcut past equip_char but we have to set the right data!
			this->equipment[eq] = item;
			item->m_WornBy = this;
			item->m_WornOn = eq;
		}
	}
	
	FOREACH_CONST(ObjectList, ch.carrying, iter)
	{
		ObjData *item = ObjData::Clone(*iter);
		//	Shortcut past ToChar but we have to set the right data!
		this->carrying.push_back(item);	//	Safer, and we can add to end of list
		item->m_CarriedBy = this;
	}
	
	if (m_Name.empty())				m_Name = "undefined";
	if (m_Keywords.empty())			m_Keywords = m_Name;
	if (m_RoomDescription.empty())	m_RoomDescription = "undefined\n";
}


//	Copy constructor, yay!
#if 0
CharData &CharData::operator=(const CharData &ch)
{
	if (this == &ch)
		return *this;
	
	Entity::operator=(ch);

	m_RealNumber = ch.m_RealNumber;
	general = ch.general;
	real_abils = ch.real_abils;
	aff_abils = ch.aff_abils;
	points = ch.points;
	m_pPlayer = &dummy_player;	//	Copy character = mob for sure
	mob = NULL;
	combat_mode = ch.combat_mode;
	
	int	eq;
	
	SetID(max_id++);
	
/*	if (!this->mob)
	{
		if (ch.mob && ch.mob != &dummy_mob)
			this->mob = new MobData(*ch.mob);
		else
			this->mob = new MobData();
	}
	else if (ch.mob && ch.mob != &dummy_mob)
		*this->mob = *ch.mob;
	else
	{
		delete this->mob;
		this->mob = new MobData();
	}
*/

	this->mob = (!ch.mob || (ch.mob == &dummy_mob)) ? new MobData() : new MobData(*ch.mob);
		
	if (!IS_NPC(&ch))
	{
		BUFFER(buf, MAX_STRING_LENGTH);
		
		if (IS_STAFF(&ch))	sprintf(buf, "%s %s", ch.GetName(), ch.GetTitle());
		else				sprintf(buf, "%s %s", ch.GetTitle(), ch.GetName());
		this->general.longDesc = buf;
		this->general.shortDesc = this->general.longDesc;

		MOB_FLAGS(this) = MOB_ISNPC;	//	Gaurantee - no duping players, lest to make a mob
	}
	
	FOREACH_CONST(Lexi::List<Affect *>, ch.m_Affects, aff)
	{
		m_Affects.push_back(new Affect(**aff, this));
	}
	
	for (eq = 0; eq < NUM_WEARS; eq++) {
		if (ch.equipment[eq]) {
			ObjData *item = create_obj(ch.equipment[eq]);
			this->equipment[eq] = item;
			item->m_WornBy = this;
			item->m_WornOn = eq;
		}
	}
	
	FOREACH_CONST(ObjectList, ch.carrying, iter)
	{
		ObjData *item = create_obj(*iter);
		this->carrying.push_back(item);
		item->m_CarriedBy = this;
	}
	
	if (!this->general.name)		this->general.name = "undefined";
	if (!this->general.shortDesc)	this->general.shortDesc = "undefined";
	if (!this->general.longDesc)	this->general.longDesc = "undefined\n";
	
	return *this;
}
#endif

CharData::~CharData(void)
{	
	//	Just in case it didn't come through the 'Extract' pineline
	SetPurged();
	
	if (m_pPlayer && (m_pPlayer != &dummy_player)) {
		delete m_pPlayer;
		if (IS_NPC(this))
			log("SYSERR: Mob %s [%s] had PlayerData allocated!", GetName(), GetVirtualID().Print().c_str());
	}
	if (this->mob && (this->mob != &dummy_mob))	{
		delete this->mob;
		if (!IS_NPC(this))
			log("SYSERR: Player %s had MobData allocated!", GetName());
	}
	
	delete this->path;
		
	FOREACH(Lexi::List<Affect *>, m_Affects, affect)
	{
		(*affect)->Remove(this, Affect::DontTotal);
	}
	
	//	Must occur last, removing affects can add events... woops!
	this->EventCleanup();
}


const char *CharData::GetAlias(void) const
{
	return (IS_NPC(this) ? m_Keywords.c_str() : m_Name.c_str());
}


Race CharData::GetFaction() const
{
	return factions[GetRace()];
}


/* Extract a ch completely from the world, and leave his stuff behind */
void CharData::Extract(void) {
	ACMD(do_return);

	if (IsPurged())
		return;
	
/*	if (m_pTimerCommand && !m_pTimerCommand->IsRunning()) {
		m_bSubstate = SUB_TIMER_DO_ABORT;
		(*m_pTimerCommand->m_pFunction)(this, "", 0, "", 0);
		ExtractTimer();
		
		if (IsPurged())
			return;
	}
*/
	
	GET_TEAM(this) = 0;
	
	if (!IS_NPC(this) && !this->desc) {
	    FOREACH(DescriptorList, descriptor_list, iter)
	    {
	    	DescriptorData *d = *iter;
			if (d->m_OrigCharacter == this)
				do_return(d->m_Character, "", "return", 0);
		}
	}
	if (InRoom() == NULL) {
		log("SYSERR: NOWHERE extracting char %s in extract_char.", GetName());
		core_dump();
//		exit(1);
	}
	if (!m_Followers.empty() || m_Following)
		this->die_follower();

	/* Forget snooping, if applicable */
	if (this->desc)
	{
		if (this->desc->m_Snooping)
		{
			this->desc->m_Snooping->m_SnoopedBy.remove(this->desc);
			this->desc->m_Snooping = NULL;
		}
		if (!this->desc->m_SnoopedBy.empty())
		{
			FOREACH(DescriptorList, this->desc->m_SnoopedBy, d)
			{
				(*d)->Write("Your victim is no longer among us.\n");
				mudlogf(BRF, (*d)->m_Character->GetLevel(), true, "(GC) %s stops snooping %s.", (*d)->m_Character->GetName(), GetName());
				(*d)->m_Snooping = NULL;
			}
			this->desc->m_SnoopedBy.clear();
		}
	}
	
	/* transfer objects to room, if any */
/*	START_ITER(object_iter, obj, ObjData *, this->carrying)
	{
		quit_otrigger(obj, this);
	}
	
	for (int i = 0; i < NUM_WEARS; ++i)
	{
		if (GET_EQ(this, i))
			quit_otrigger(GET_EQ(this, i), this);
	}
*/	
	FOREACH(ObjectList, this->carrying, iter)
	{
		ObjData *obj = *iter;
		
		if (!NO_LOSE(obj, this))
		{
			obj->FromChar();
			obj->ToRoom(IN_ROOM(this));
		}
		else
			obj->Extract();
	}
	
	
	/* transfer equipment to room, if any */
	for (int i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(this, i)) {
			if (!NO_LOSE(GET_EQ(this, i), this))
				unequip_char(this, i)->ToRoom(IN_ROOM(this));
			else
				GET_EQ(this, i)->Extract();
		}
	}
	
	if (FIGHTING(this))
		this->stop_fighting();

	FOREACH(CharacterList, character_list, iter)
	{
		CharData *k = *iter;
		if (FIGHTING(k) == this)
			k->redirect_fighting();
	}

	if (!IS_NPC(this))	//	Do we need to?
		Save();

	//	pull the char from the lists
	FromRoom();
	FromWorld();
	
	//	remove any pending events for this character.
	EventCleanup();

	//	We won't even run the followup at this point...
	ExtractTimer();

	if (this->desc && this->desc->m_OrigCharacter)
		do_return(this, "", "return", 0);

	if (IS_NPC(this))
	{
		Purge();
	}

	if (this->desc != NULL)
	{
		while (this->desc->GetState()->IsInGame())
		{
			this->desc->GetState()->Pop();
		}
	}
	else if (!IsPurged())
	{  /* if a player gets purged from within the game */
		
		while (!m_Affects.empty())
		{
			m_Affects.front()->Remove(this, Affect::DontTotal);
		}
		
		Purge();
	}
	
	Entity::Extract();
}


/* place a character in a room */
void CharData::ToRoom(RoomData *room) {
	if (!room)
		log("SYSERR: %s->ToRoom(room == NULL)", GetName());
	else if (InRoom())
		log("SYSERR: InRoom == %u on %s->ToRoom(room = %u)", InRoom()->GetID(), GetName(), room->GetID());
	else if (IsPurged())
		log("SYSERR: purged == true on %s->ToRoom(room = %u)", GetName(), room->GetID());
	else {
		if (Lexi::IsInContainer(room->people, this))
		{
			log("SYSERR: world{%u}->people.InList() == true, %s->InRoom() == %u on CharData::ToRoom()",
					room->GetID(), GetName(), InRoom()->GetID());
			return;
		}
		room->people.push_front(this);
		m_InRoom = room;
		if (room->GetVirtualID().IsValid())	GetPlayer()->m_LastRoom = room->GetVirtualID();
		
		if (room->GetZone())
		{
			room->GetZone()->m_Characters.push_back(this);
			
			if (!IS_NPC(this))	++room->GetZone()->m_NumPlayersPresent;
		}
		
#if 0
		if (GET_EQ(this, WEAR_HAND_R))
			if (GET_OBJ_TYPE(GET_EQ(this, WEAR_HAND_R)) == ITEM_LIGHT)
				if (GET_EQ(this, WEAR_HAND_R)->GetValue(OBJVAL_LIGHT_HOURS))	/* Light ON */
					room->m_NumLightsPresent++;
		if (GET_EQ(this, WEAR_HAND_L))
			if (GET_OBJ_TYPE(GET_EQ(this, WEAR_HAND_L)) == ITEM_LIGHT)
				if (GET_EQ(this, WEAR_HAND_L)->GetValue(OBJVAL_LIGHT_HOURS))	/* Light ON */
					room->m_NumLightsPresent++;
#else
		for (int i = 0; i < NUM_WEARS; ++i)
		{
			ObjData *obj = GET_EQ(this, i);
			if (obj && GET_OBJ_TYPE(obj) == ITEM_LIGHT && obj->GetValue(OBJVAL_LIGHT_HOURS))
				room->AddLight(obj->GetValue(OBJVAL_LIGHT_RADIUS));
		}
#endif
		if (AFF_FLAGGED(this, AFF_LIGHT))
			room->AddLight();
			
		m_PreviouslyInRoom = NULL;
		
#if !ROAMING_COMBAT
		if (FIGHTING(this) && (IN_ROOM(this) != IN_ROOM(FIGHTING(this)))) {
			FIGHTING(this)->redirect_fighting();
			this->stop_fighting();
		}
#endif
		
		//if (!AFF_FLAGGED(this, AFF_NOQUIT))
		GET_ROLL_STEALTH(this) = LinearCurve();
		time_t theTime = time(0);
		if (theTime > GET_ROLL_NOTICE_TIME(this))
		{
			GET_ROLL_NOTICE(this) = LinearCurve();
			GET_ROLL_NOTICE_TIME(this) = theTime + 10;
		}
		
		if (NO_STAFF_HASSLE(this))	// Because the rest of the function is only to kill off morts.
			return;
		
		if (ROOM_FLAGGED(room, ROOM_DEATH)) {
			GET_HIT(this)	= -100;
			this->update_pos();
			return;
		}

		if (ROOM_FLAGGED(room, ROOM_GRAVITY) && (!AFF_FLAGGED(this, AFF_FLYING) || AFF_FLAGGED(this, AFF_NOQUIT)) && !Event::FindEvent(m_Events, GravityEvent::Type)) {
			this->AddOrReplaceEvent(new GravityEvent(3 RL_SEC, this));
		}
		
		if (ROOM_FLAGGED(room, ROOM_VACUUM) && !AFF_FLAGGED(this, AFF_SPACESUIT)) {
			act("*Gasp* You can't breath!" /*"  Its a vacuum!"*/, TRUE, this, 0, 0, TO_CHAR);
			GET_HIT(this) = -100;
			this->update_pos();
			return;
		}
	}
}


/* move a player out of a room */
void CharData::FromRoom()
{
	if (IsPurged())
	{
		log("SYSERR: IsPurged() == TRUE in %s->FromRoom()", GetName());
		core_dump();
		return;
	}

	if (m_InRoom == NULL) {
		log("SYSERR: InRoom() == NOWHERE in %s->FromRoom()", GetName());
		return;
	}

	CHAR_WATCHING(this) = -1;

	FOREACH(CharacterList, InRoom()->people, iter)
	{
		CharData *ch = *iter;
		if (FIGHTING(ch) == this)
			ch->redirect_fighting();
	}
	
#if !ROAMING_COMBAT
	if (FIGHTING(this))
		this->stop_fighting();
#endif

#if 0
	if (GET_EQ(this, WEAR_HAND_R))
		if (GET_OBJ_TYPE(GET_EQ(this, WEAR_HAND_R)) == ITEM_LIGHT)
			if (GET_EQ(this, WEAR_HAND_R)->GetValue(OBJVAL_LIGHT_HOURS))	/* Light is ON */
				m_InRoom->RemoveLight();
	if (GET_EQ(this, WEAR_HAND_L))
		if (GET_OBJ_TYPE(GET_EQ(this, WEAR_HAND_L)) == ITEM_LIGHT)
			if (GET_EQ(this, WEAR_HAND_L)->GetValue(OBJVAL_LIGHT_HOURS))	/* Light is ON */
				m_InRoom->RemoveLight();
#else
	for (int i = 0; i < NUM_WEARS; ++i)
	{
		ObjData *obj = GET_EQ(this, i);
		if (obj && GET_OBJ_TYPE(obj) == ITEM_LIGHT && obj->GetValue(OBJVAL_LIGHT_HOURS))
			m_InRoom->RemoveLight(obj->GetValue(OBJVAL_LIGHT_RADIUS));
	}
#endif

	if (AFF_FLAGGED(this, AFF_LIGHT))
		InRoom()->RemoveLight();
	
	if (InRoom()->GetZone())
	{
		InRoom()->GetZone()->m_Characters.remove(this);
		
		if (!IS_NPC(this))	--InRoom()->GetZone()->m_NumPlayersPresent;
	}

	m_PreviouslyInRoom = InRoom();
	InRoom()->people.remove(this);
	m_InRoom = NULL;

	this->sitting_on = NULL;
	
	Affect::FlagsFromChar(this, MAKE_BITSET(Affect::Flags, AFF_HIDE, AFF_COVER));
}



void CharData::die(CharData * killer) {
//	extern void check_level(CharData *ch);
	if (killer)
	{
 		WAIT_STATE(killer, 1);
 	}
 	
	if (!IS_NPC(this) && !ROOM_FLAGGED(IN_ROOM(this), ROOM_BLACKOUT))
	{
		BUFFER(buf, MAX_INPUT_LENGTH);
		if (!killer || this == killer)	sprintf(buf, "%s killed at %s`n.", GetName(), this->InRoom()->GetName());
		else							sprintf(buf, "%s killed by %s`n at %s`n.", GetName(), killer->GetName(), this->InRoom()->GetName());
		announce(buf);
	}

	if (FIGHTING(this)) {
//		if (FIGHTING(FIGHTING(this)) == this)
//			FIGHTING(this)->stop_fighting();
		this->stop_fighting();
	}

#if ROAMING_COMBAT
	FOREACH(CharacterList, character_list, iter)
#else
	FOREACH(CharacterList, InRoom()->people, iter)
#endif
	{
		CharData *ch = *iter;
		if (FIGHTING(ch) == this)
		{
			ch->redirect_fighting();
		}
	}

	while (!m_Affects.empty())
	{
		m_Affects.front()->Remove(this, Affect::DontTotal);
	}
	AffectTotal();
	
	if (killer /*&& (this != killer)*/) {
		if (!IS_NPC(killer) && !AFF_FLAGGED(killer, AFF_TRAITOR) && (this != killer)) {
//			int levelDiff = (killer->GetLevel() + 3) / 4;	//	Only rewraded for killing people within 1/4 of your level
			
//			if (levelDiff < 10)	levelDiff = 10;	//	With a minimum of 10 levels difference
			
			if (IS_NPC(this))				++killer->GetPlayer()->m_MobKills;
			else
			{
//				 if (/*GetLevel() >= (killer->GetLevel() - levelDiff) &&*/
//						killer->GetRelation(this) == RELATION_ENEMY)
					++killer->GetPlayer()->m_PlayerKills;
				if (GetLevel() < (killer->GetLevel() - 10))
					killer->GetPlayer()->m_Karma -= (killer->GetLevel() - GetLevel()) / 2;
//				check_level(killer);
			}
		}
		
		if (!IS_NPC(this))
		{
			if (IS_NPC(killer))	++GetPlayer()->m_MobDeaths;
			else				++GetPlayer()->m_PlayerDeaths;
		}
	}
	
	GET_POS(this) = POS_STANDING;	//	So the triggers can be more effective
	m_AffectFlags.set(AFF_DYING);	//	Prevent recursive dying since we restore them to standing
	
	if (killer)
	{
		kill_otrigger(killer, this);
		kill_mtrigger(killer, this);
	}
	if (PURGED(this))	return;
	
	death_otrigger(this, killer);
	if (PURGED(this))	return;
	if (IS_NPC(this)) {
		if (death_mtrigger(this, killer) && !PURGED(this))	death_cry(this);
		if (PURGED(this))	return;
	} else
	{
        death_mtrigger(this, killer);
		death_cry(this);
	}
	
	GET_POS(this) = POS_DEAD;	//	So the triggers can be more effective
	
	GET_LAST_ATTACKER(this) = 0;
	
	make_corpse(this, killer);
	
	if (IS_NPC(this))
		this->Extract();
	else {
//		REMOVE_BIT(AFF_FLAGS(this), AFF_TRAITOR);
//		REMOVE_BIT(AFF_FLAGS(this), AFF_NOQUIT);
//		REMOVE_BIT(AFF_FLAGS(this), AFF_NOSHOOT | AFF_DIDSHOOT | AFF_DIDFLEE);
		m_AffectFlags.clear(AFF_DYING);
	
		this->EventCleanup();
		
		/* extract all the triggers too */
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(this)), iter)
			(*iter)->Extract();
		TRIGGERS(SCRIPT(this)).clear();

		this->FromRoom();
		this->ToRoom(this->desc ? this->StartRoom() : world[1]);
		
		GET_HIT(this) = GET_MAX_HIT(this);
		GET_MOVE(this) = GET_MAX_MOVE(this);
		//GET_ENDURANCE(this) = GET_MAX_ENDURANCE(this);

		this->update_pos();
		CheckRegenRates(this);
		
		if (this->desc)
		{
			//this->desc->m_Input[0] = '\0';
			this->desc->m_InputQueue.flush();
		}
		act("$n has returned from the dead.", TRUE, this, 0, 0, TO_ROOM);
		GET_POS(this) = POS_STANDING;
		look_at_room(this);
		
		(new Affect("RESPAWNING", 0, APPLY_NONE, AFF_RESPAWNING))->ToChar(this, 1);
	}
}


void CharData::EventCleanup(void) {
	int	i;

	while (!m_Events.empty())
	{
		Event *event = m_Events.front();
		m_Events.pop_front();
		event->Cancel();
	}

	/* cancel point updates */
	for (i = 0; i < 3; i++) {
		if (GET_POINTS_EVENT(this, i)) {
			GET_POINTS_EVENT(this, i)->Cancel();
			GET_POINTS_EVENT(this, i) = NULL;
		}
	}
	
	ExtractTimer();
}


void CharData::update_objects(void) {
	int i, pos;

	for (pos = 0; pos < NUM_WEARS; pos++)
	{
		ObjData *obj = GET_EQ(this, pos);
		if (obj) {
			if (GET_OBJ_TYPE(obj) == ITEM_LIGHT) {
				if (obj->GetValue(OBJVAL_LIGHT_HOURS) > 0) {
					i = obj->GetValue(OBJVAL_LIGHT_HOURS) - 1;
					obj->SetValue(OBJVAL_LIGHT_HOURS, i);
					if (i == 1) {
						act("Your $p begins to flicker and fade.", FALSE, this, obj, 0, TO_CHAR);
						act("$n's $p begins to flicker and fade.", FALSE, this, obj, 0, TO_ROOM);
					} else if (i == 0) {
						act("Your $p sputters out and dies.", FALSE, this, obj, 0, TO_CHAR);
						act("$n's $p sputters out and dies.", FALSE, this, obj, 0, TO_ROOM);
						InRoom()->RemoveLight(obj->GetValue(OBJVAL_LIGHT_RADIUS));
					}
				}
			}
		}
	}
//	for (i = 0; i < NUM_WEARS; i++)
//		if (GET_EQ(this, i))
//			GET_EQ(this, i)->update(2);
//
//	if (this->carrying)
//		this->carrying->update(1);
}


//	This updates a character by subtracting everything he is affected by
//	restoring original abilities, and then affecting all again
void CharData::AffectTotal(void)
{
	int i, j, loc, type;
	ObjData *obj;
	int oldHealth;
	
	if (PURGED(this))
		return;
	
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(this, i))
		{
			for (j = 0; j < MAX_OBJ_AFFECT; j++)
			{
				Affect::Modify(this, GET_EQ(this, i)->affect[j].location,
						GET_EQ(this, i)->affect[j].modifier, GET_EQ(this, i)->m_AffectFlags, FALSE);			
			}
		}
	}

	FOREACH(Lexi::List<Affect *>, m_Affects, aff)
		(*aff)->Unapply(this);
	
	this->aff_abils = this->real_abils;

	if (!IS_NPC(this)) {
		for (i = 1; i <= MAX_SKILLS; ++i)
			GET_SKILL(this, i) = GET_REAL_SKILL(this, i);
	}
	
	for (loc = 0; loc < ARMOR_LOC_NATURAL; ++loc)
		for (type = 0; type < NUM_DAMAGE_TYPES; ++type)
			GET_ARMOR(this, loc, type) = 0;
	for (type = 0; type < NUM_DAMAGE_TYPES; ++type)
		GET_ARMOR(this, ARMOR_LOC_MISSION, type) = 0;
	GET_EXTRA_ARMOR(this) = 0;
	
	if (!IS_NPC(this))
		GET_MAX_HIT(this) = max_hit_base[GetRace()] + (GET_HEA(this) * max_hit_bonus[GetRace()]);
	GET_MAX_MOVE(this) = 100;
	//	Max endurance calculation here!
	//GET_MAX_ENDURANCE(this) = (GetLevel() - 1) + 20;
	
	oldHealth = GET_HEA(this);
	
	for (i = 0; i < NUM_WEARS; i++) {
		obj = GET_EQ(this, i);
		if (obj) {
			for (j = 0; j < MAX_OBJ_AFFECT; j++)
				Affect::Modify(this, obj->affect[j].location, obj->affect[j].modifier, obj->m_AffectFlags, TRUE);
			
			if (OBJ_FLAGGED(obj, ITEM_SPECIAL_MISSION))
				loc = ARMOR_LOC_MISSION;
			else
				loc = wear_to_armor_loc[i];
			
			if (loc != -1 && GET_OBJ_TYPE(obj) == ITEM_ARMOR)
			{
				
				for (type = 0; type < NUM_DAMAGE_TYPES; ++type)
					if (obj->GetValue((ObjectTypeValue)type) > GET_ARMOR(this, loc, type))
						GET_ARMOR(this, loc, type) = obj->GetValue((ObjectTypeValue)type);
			}
		}
	}

	FOREACH(Lexi::List<Affect *>, m_Affects, aff)
		(*aff)->Apply(this);
	
	/* Make certain values are between 1..210, not < 1 and not > 210! */
	i = MAX_STAT;	//(IS_NPC(this) ? MAX_STAT : MAX_PC_STAT);

	GET_STR(this) = MAX(1, MIN(GET_STR(this), i));
	GET_HEA(this) = MAX(1, MIN(GET_HEA(this), i));
	GET_COO(this) = MAX(1, MIN(GET_COO(this), i));
	GET_AGI(this) = MAX(1, MIN(GET_AGI(this), i));
	GET_PER(this) = MAX(1, MIN(GET_PER(this), i));
	GET_KNO(this) = MAX(1, MIN(GET_KNO(this), i));

	if (!IS_NPC(this) && oldHealth != GET_HEA(this))
		GET_MAX_HIT(this) += (GET_HEA(this) - oldHealth) * max_hit_bonus[GetRace()];
	if (GET_HIT(this) > GET_MAX_HIT(this))
		GET_HIT(this) = GET_MAX_HIT(this);
	
	CheckRegenRates(this);
	
	update_pos();
}

#if 0
struct CheckBurdenData
{
	CharData *			ch;
};

EVENTFUNC(CheckBurdenEvent)
{
	CheckBurdenData *checkBurden = (CheckBurdenData *)event_obj;

//	float	ratio = (float)IS_CARRYING_W(checkBurden->ch) / GET_CAR(checkBurden->ch);
	bool	wasBurdened = Affect::AffectedBy(checkBurden->ch, Affect::Burdened);
	bool	wasOverBurdened = Affect::AffectedBy(checkBurden->ch, Affect::OverBurdened);
	Affect *aff;
	
	Affect::FromChar(checkBurden->ch, Affect::Burdened);
	Affect::FromChar(checkBurden->ch, Affect::OverBurdened);
	
	checkBurden->ch->events.Remove(event);
	
	//	Do nothing for now
	if (IS_CARRYING_W(checkBurden->ch) <= GET_CAR(checkBurden->ch))
	{
		//	Do nothing
		if (wasBurdened || wasOverBurdened)
			checkBurden->ch->Send("You are no longer burdened, and can move freely now.\n");
	}
	else if (IS_CARRYING_W(checkBurden->ch) <= (GET_CAR(checkBurden->ch) * 1.5))
	{
		(new Affect(Affect::Burdened, -5, APPLY_QUI, AFF_BURDENED))->ToChar(checkBurden->ch, 0);
		(new Affect(Affect::Burdened, -5, APPLY_EVA, AFF_BURDENED))->ToChar(checkBurden->ch, 0);		
		(new Affect(Affect::Burdened, -5, APPLY_STE, AFF_BURDENED))->ToChar(checkBurden->ch, 0);		
		(new Affect(Affect::Burdened, -5, APPLY_MEL, AFF_BURDENED))->ToChar(checkBurden->ch, 0);

		if (wasOverBurdened)
			checkBurden->ch->Send("You are no longer overburdened, but your burdened will still be a hinderence.\n");
		else if (!wasBurdened)
			checkBurden->ch->Send("You are now burdened, and your burden will be a hinderance.\n");
	}
	else /* ratio > 1.25 */
	{
		(new Affect(Affect::OverBurdened, -10, APPLY_QUI, AFF_OVERBURDENED))->ToChar(checkBurden->ch, 0);
		(new Affect(Affect::OverBurdened, -10, APPLY_EVA, AFF_OVERBURDENED))->ToChar(checkBurden->ch, 0);
		(new Affect(Affect::OverBurdened, -10, APPLY_STE, AFF_OVERBURDENED))->ToChar(checkBurden->ch, 0);
		(new Affect(Affect::OverBurdened, -10, APPLY_MEL, AFF_OVERBURDENED))->ToChar(checkBurden->ch, 0);

		if (!wasOverBurdened)
			checkBurden->ch->Send("You are now overburdened, and your burden will be a significant hinderance.\n");
	}
	
	return 0;
}
#endif

void CharData::UpdateWeight(void)
{
#if 0
	if (!FindEvent(this->events, CheckBurdenEvent))
	{
		CheckBurdenData *data;
		CREATE(data, CheckBurdenData, 1);
		data->ch = this;
		this->events.Add(new Event(CheckBurdenEvent, data, 1));
	}
#endif
}


/* enter a character in the world (place on lists) */
void CharData::ToWorld() {
	if (Lexi::IsInContainer(character_list, this))
		return;
	
	character_list.push_front(this);
	IDManager::Instance()->Register(this);

	if (IS_MOB(this))		/* if mobile */
		++GetPrototype()->m_Count;
	
	//	Why? ToRoom does it already
//	GET_ROLL_STE(this) = Bellcurve(GET_STE(this));
//	GET_ROLL_BRA(this) = Bellcurve(GET_BRA(this));
	
	if (!IS_NPC(this) && GET_CLAN(this))
	{
		GET_CLAN(this)->OnPlayerToWorld(this);
	}
	
	FOREACH(Lexi::List<Affect *>, m_Affects, affect)
	{
		(*affect)->ResumeTimer(this);
	}
	
	extern void imc_char_login( CharData * ch );
	imc_char_login(this);
}


/* remove a character from the world lists */
void CharData::FromWorld()
{
	if (!IS_NPC(this) && GET_CLAN(this))
	{
		GET_CLAN(this)->OnPlayerFromWorld(this);
	}

	character_list.remove_one(this);	//	Faster with the optimized Lexi::List::remove

	IDManager::Instance()->Unregister(this);

	if (GetPrototype())	--GetPrototype()->m_Count;
	
	FOREACH(Lexi::List<Affect *>, m_Affects, affect)
	{
		(*affect)->PauseTimer();
	}
}



/*
 * If missing vsnprintf(), remove the 'n' and remove MAX_STRING_LENGTH.
 */
int CharData::Send(const char *messg, ...)
{
	va_list args;
	int	length = 0;

	if (!messg || !*messg)
		return 0;
	
 	BUFFER(buf, MAX_STRING_LENGTH);
 	
	va_start(args, messg);
	length += vsnprintf(buf, MAX_STRING_LENGTH, messg, args);
	va_end(args);
	
	buf[MAX_STRING_LENGTH - 1] = '\0';

	if (this->desc)	this->desc->Write(buf);
	else			act(buf, FALSE, this, 0, 0, TO_CHAR);
	
	return length;
}


/*
 * If missing vsnprintf(), remove the 'n' and remove MAX_STRING_LENGTH.
 */
Lexi::String CharData::SendAndCopy(const char *messg, ...)
{
	va_list args;

	if (!messg || !*messg)
		return 0;
	
 	BUFFER(buf, MAX_STRING_LENGTH);
 	
	va_start(args, messg);
	vsnprintf(buf, MAX_STRING_LENGTH, messg, args);
	va_end(args);
	
	buf[MAX_STRING_LENGTH - 1] = '\0';

	if (this->desc)	this->desc->Write(buf);
	else			act(buf, FALSE, this, 0, 0, TO_CHAR);
	
	return buf;
}


#if 0
/* new save_char that writes an ascii pfile */
void CharData::OldSave(void)
{
	FILE *outfile;
	int i;
	ObjData *char_eq[NUM_WEARS];
	int hit, move, endurance;


	if (IS_NPC(this))
		return;

	/*
	 * save current values, because unequip_char will call
	 * affect_total(), which will reset points to lower values
	 * if player is wearing +points items.  We will restore old
	 * points level after reequiping.
	 */
	hit = GET_HIT(this);
	move = GET_MOVE(this);
	//endurance = GET_ENDURANCE(this);


	BUFFER(bits, MAX_INPUT_LENGTH);

	{
		Lexi::String filename = GetFilename();
		outfile = fopen(filename.c_str(), "w");

		if (!outfile)
		{
			perror(filename.c_str());
			return;
		}
		
		/* Unequip everything a character can be affected by */
		for (i = 0; i < NUM_WEARS; i++) {
			if (GET_EQ(this, i))	char_eq[i] = unequip_char(this, i);
			else					char_eq[i] = NULL;
		}

		fprintf(outfile, "Name: %s\n", GetName());
		fprintf(outfile, "Pass: %s\n", GetPlayer()->m_Password);
		fprintf(outfile, "Titl: %s\n", GetTitle());
		if(!GetPlayer()->m_Pretitle.empty())	fprintf(outfile, "PTtl: %s\n", GetPlayer()->m_Pretitle.c_str());
		if(!this->general.description.empty()) {
			BUFFER(buf, MAX_STRING_LENGTH);
			Lexi::BufferedFile::PrepareString(buf, this->general.description.c_str());
			fprintf(outfile, "Desc:\n%s~\n", buf);
		} 
		if (!GetPlayer()->m_Prompt.empty())	fprintf(outfile, "Prmp: %s\n", GetPlayer()->m_Prompt.c_str());
		fprintf(outfile, "Sex : %d\n", GetSex());
		fprintf(outfile, "Race: %d\n", GetRace());
		fprintf(outfile, "Levl: %d\n", GetLevel());
		fprintf(outfile, "Brth: %ld\n", GetPlayer()->m_Creation);
		fprintf(outfile, "Plyd: %ld\n", GetPlayer()->m_TotalTimePlayed + (time(0) - GetPlayer()->m_LoginTime));
		fprintf(outfile, "Last: %ld\n", (this->desc) ? time(0) : GetPlayer()->m_LastLogin);
		if (!GetPlayer()->m_MaskedHost.empty())
			fprintf(outfile, "Mask: %s\n", GetPlayer()->m_MaskedHost.c_str());
		else
			fprintf(outfile, "Host: %s\n", (this->desc) ? this->desc->m_Host.c_str() : GetPlayer()->m_Host.c_str());
		fprintf(outfile, "Hite: %d\n", GET_HEIGHT(this));
		fprintf(outfile, "Wate: %d\n", GET_WEIGHT(this));
		fprintf(outfile, "Krma: %d\n", GetPlayer()->m_Karma);
		fprintf(outfile, "Vers: %d\n", GetPlayer()->m_Version);

		fprintf(outfile, "Page: %d\n", GetPlayer()->m_PageLength);

		fprintf(outfile, "Id  : %d\n", GetID());
		if(this->general.act) {
			sprintbits(this->general.act, bits);
			fprintf(outfile, "Act : %s\n", bits);
//			fprintf(outfile, "Act : %d\n", this->general.act);
		}

		fprintf(outfile, "Skil:\n");
		for(i = 1; i < NUM_SKILLS; i++) {
			if(GET_REAL_SKILL(this, i))
				fprintf(outfile, "%d %d\n", i, GET_REAL_SKILL(this, i));
		}
		fprintf(outfile, "0 0\n");
		
//		if(this->player->special.wimp_level)	fprintf(outfile, "Wimp: %d\n", this->player->special.wimp_level);
		if(GetPlayer()->m_FreezeLevel > 0)		fprintf(outfile, "Frez: %d\n", GetPlayer()->m_FreezeLevel);
		if(GetPlayer()->m_StaffInvisLevel > 0)	fprintf(outfile, "Invs: %d\n", GetPlayer()->m_StaffInvisLevel);
		
		if (PLR_FLAGGED(this, PLR_LOADROOM) && GetPlayer()->m_LoadRoom != NOWHERE)
			fprintf(outfile, "Room: %d\n", GetPlayer()->m_LoadRoom);
		if (PLR_FLAGGED(this, PLR_NOSHOUT) && GetPlayer()->m_MuteLength)
			fprintf(outfile, "Mute: %ld\n", GetPlayer()->m_MuteLength - (time(0) - GetPlayer()->m_MutedAt));
		
		if (GetPlayer()->m_LastRoom != NOWHERE)	fprintf(outfile, "LstR: %d\n", GetPlayer()->m_LastRoom);
		
		if(GetPlayer()->m_Preferences) {
			sprintbits(GetPlayer()->m_Preferences, bits);
			fprintf(outfile, "Pref: %s\n", bits);
		}
		if(GetPlayer()->m_Channels) {
			sprintbits(GetPlayer()->m_Channels, bits);
			fprintf(outfile, "Chan: %s\n", bits);
		}
		if(GetPlayer()->m_BadPasswordAttempts > 0)	fprintf(outfile, "Badp: %d\n", GetPlayer()->m_BadPasswordAttempts);
#if 0
		if(!IS_STAFF(this)) {
			if(this->general.conditions[0])			fprintf(outfile, "Hung: %d\n", this->general.conditions[0]);
			if(this->general.conditions[1])			fprintf(outfile, "Thir: %d\n", this->general.conditions[1]);
			if(this->general.conditions[2])			fprintf(outfile, "Drnk: %d\n", this->general.conditions[2]);
		}
#endif
		fprintf(outfile, "Lern: %d %d\n", GetPlayer()->m_SkillPoints, GetPlayer()->m_LifetimeSkillPoints);
		if(GET_CLAN(this))
			fprintf(outfile, "Clan: %d\n", GET_CLAN(this)->GetID());
		fprintf(outfile,   "Kill: %d %d %d %d\n",
				GetPlayer()->m_PlayerKills, GetPlayer()->m_MobKills,
				GetPlayer()->m_PlayerDeaths, GetPlayer()->m_MobDeaths);

		fprintf(outfile, "Str : %d\n", this->real_abils.Strength);
		fprintf(outfile, "Hea : %d\n", this->real_abils.Health);
		fprintf(outfile, "Coo : %d\n", this->real_abils.Coordination);
		fprintf(outfile, "Agi : %d\n", this->real_abils.Agility);
		fprintf(outfile, "Per : %d\n", this->real_abils.Perception);
		fprintf(outfile, "Kno : %d\n", this->real_abils.Knowledge);

		fprintf(outfile, "Hit : %d/%d\n", this->points.hit, this->points.max_hit);
		fprintf(outfile, "Move: %d/%d\n", this->points.move, this->points.max_move);
		//fprintf(outfile, "End : %d/%d\n", this->points.endurance, this->points.max_endurance);
//		fprintf(outfile, "Armr: %d\n", this->points.armor);
		fprintf(outfile, "MP  : %d %d\n", this->points.mp, this->points.lifemp);
//		fprintf(outfile, "Hrol: %d\n", this->points.hitroll);
//		fprintf(outfile, "Drol: %d\n", this->points.damroll);
		
		if (!GetPlayer()->m_PoofInMessage.empty())	fprintf(outfile, "Pfin: %s\n", GetPlayer()->m_PoofInMessage.c_str());
		if (!GetPlayer()->m_PoofOutMessage.empty())	fprintf(outfile, "Pfou: %s\n", GetPlayer()->m_PoofOutMessage.c_str());

		if (!GetPlayer()->m_IMC.m_Listen.empty()) {
			fprintf(outfile, "ILsn: %s\n", GetPlayer()->m_IMC.m_Listen.c_str());
		}
		
		if (GetPlayer()->m_IMC.m_Deaf) {
			sprintbits(GetPlayer()->m_IMC.m_Deaf, bits);
			fprintf(outfile, "IDef: %s\n", bits);
		}
		
		if (GetPlayer()->m_IMC.m_Allow) {
			sprintbits(GetPlayer()->m_IMC.m_Allow, bits);
			fprintf(outfile, "IAlw: %s\n", bits);
		}
		
		if (GetPlayer()->m_IMC.m_Deny) {
			sprintbits(GetPlayer()->m_IMC.m_Deny, bits);
			fprintf(outfile, "IDny: %s\n", bits);
		}
		
		if(GetPlayer()->m_StaffPrivileges) {
			sprintbits(GetPlayer()->m_StaffPrivileges, bits);
			fprintf(outfile, "Stf : %s\n", bits);
		}

		//	save aliases
		FOREACH(Alias::List, GetPlayer()->m_Aliases, alias)
		{
			fprintf(outfile, "Alis: %s %s\n", (*alias)->GetCommand(), (*alias)->GetReplacement());
		}

		if (!GetPlayer()->m_IgnoredPlayers.empty())
		{
			fprintf(outfile, "Ignr:\n");
			
			FOREACH(std::list<IDNum>, GetPlayer()->m_IgnoredPlayers, id)
			{
				fprintf(outfile, "%d\n", *id);
			}
			fprintf(outfile, "0\n");
		}
/* affected_type */
		/*
		 * remove the affections so that the raw values are stored; otherwise the
		 * effects are doubled when the char logs back in.
		 */
//		while (ch->m_Affects)
//			affect_remove(ch, ch->m_Affects);
		
		if (!m_Affects.empty())
		{
			fprintf(outfile, "Affs:\n");
			
			FOREACH(Lexi::List<Affect *>, m_Affects, iter)
			{
				Affect *aff = *iter;
				
				fprintf(outfile, "%s %u %d %d %d %s\n",
					aff->GetType(),
					aff->GetTime(),
					aff->GetModifier(),
					aff->GetLocation(),
					aff->GetAffFlags(),
					aff->GetTerminationMessage());
			}
			fprintf(outfile, "END-AFFECTS\n");
		}
		  /* add spell and eq affections back in now */
//  for (i = 0; i < MAX_AFFECT; i++) {
  //  if (st->affected[i].type)
 //     affect_to_char(ch, &st->affected[i]);
//  }

//		Re-equip
		for (i = 0; i < NUM_WEARS; i++) {
			if (char_eq[i])
				equip_char(this, char_eq[i], i);
		}

		/* restore our points to previous level */
		GET_HIT(this) = hit;
		GET_MOVE(this) = move;
		//GET_ENDURANCE(this) = endurance;

		fclose(outfile);
	}
}
#endif

  /* Load a char, TRUE if loaded, FALSE if not */
bool CharData::OldLoad(const char *name)
{
	int num = 0, num2 = 0, num3 = 0, num4 = 0, num5 = 0;
	FILE *	fl;
	char	tag[6];
	
	if (!GetPlayer() || (GetPlayer() == &dummy_player))
		m_pPlayer = new PlayerData;
	
	if(!find_name(name))		return false;
	else
	{
		BUFFER(buf, MAX_INPUT_LENGTH);
		BUFFER(line, MAX_STRING_LENGTH);
		
		Lexi::String	filename = GetFilename(name);
		
		fl = fopen(filename.c_str(), "r");
		if(!fl)
		{
			mudlogf( NRM, LVL_STAFF, TRUE,  "SYSERR: Couldn't open player file %s", filename.c_str());
			return false;
		}
		
		while(get_line(fl, line)) {
			tag_argument(line, tag);
			num = atoi(line);
			/* A */
			if (!strcmp(tag, "Act "))	m_ActFlags = asciiflag_conv(line);
			else if (!strcmp(tag, "Agi "))	this->real_abils.Agility = num;
			else if (!strcmp(tag, "Alis"))
			{
				BUFFER(cmd, MAX_INPUT_LENGTH);
				line = any_one_arg(line, cmd);
				GetPlayer()->m_Aliases.Add(cmd, line);
			}
			/* B */
			else if (!strcmp(tag, "Badp"))	GetPlayer()->m_BadPasswordAttempts = num;
			else if (!strcmp(tag, "Brth"))	GetPlayer()->m_Creation = num;
			/* C */
			else if (!strcmp(tag, "Chan"))	GetPlayer()->m_Channels = asciiflag_conv(line);
			else if (!strcmp(tag, "Clan"))	GET_CLAN(this) = Clan::GetClan(num);
			else if (!strcmp(tag, "Coo "))	this->real_abils.Coordination = num;
			/* D */
			else if (!strcmp(tag, "Desc"))	m_Description = fread_lexistring(fl, line, filename.c_str());
			/* E */
			/* F */
			else if (!strcmp(tag, "Frez"))	GetPlayer()->m_FreezeLevel = num;
			/* H */
			else if (!strcmp(tag, "Hea "))	this->real_abils.Health = num;
			else if (!strcmp(tag, "Hit "))
			{
				sscanf(line, "%d/%d", &num, &num2);
				this->points.hit = num;
				this->points.max_hit = num2;
			} else if (!strcmp(tag, "Hite"))	m_Height = num;
			else if (!strcmp(tag, "Host"))	GetPlayer()->m_Host = line;
			/* I */
			else if (!strcmp(tag, "Id  "))	SetID(num);
			else if (!strcmp(tag, "Invs"))	GetPlayer()->m_StaffInvisLevel = num;
			else if (!strcmp(tag, "Ignr"))
			{
				do {
					get_line(fl, line);
					sscanf(line, "%d", &num);
					if (num && get_name_by_id(num))
						this->AddIgnore(num);
				} while (num != 0);
			}
			/* K */
			else if (!strcmp(tag, "Kill")) {
				sscanf(line, "%d %d %d %d", &num, &num2, &num3, &num4);
				GetPlayer()->m_PlayerKills = num;
				GetPlayer()->m_MobKills = num2;
				GetPlayer()->m_PlayerDeaths = num3;
				GetPlayer()->m_MobDeaths = num4;
			}
			else if (!strcmp(tag, "Kno "))	this->real_abils.Knowledge = num;
			else if (!strcmp(tag, "Krma"))	GetPlayer()->m_Karma = num;
			/* L */
			else if (!strcmp(tag, "Last"))	GetPlayer()->m_LastLogin = num;
			else if (!strcmp(tag, "LstR"))	GetPlayer()->m_LastRoom = num;
			else if (!strcmp(tag, "Lern")) {
				num2 = 0;
				sscanf(line, "%d %d", &num, &num2);
				GetPlayer()->m_SkillPoints = num;
				GetPlayer()->m_LifetimeSkillPoints = num2;
			}
			else if (!strcmp(tag, "Levl"))	m_Level = num;
			/* M */
			else if (!strcmp(tag, "Mask")) {
				if (*line)
				{
					if (this->desc)	this->desc->m_Host = line;
					GetPlayer()->m_Host = line;
					GetPlayer()->m_MaskedHost = line;
				}
			}
			else if (!strcmp(tag, "Move")) {
				sscanf(line, "%d/%d", &num, &num2);
				this->points.move = num;
				this->points.max_move = num2;
			} else if (!strcmp(tag, "MP  ")) {
				sscanf(line, "%d %d", &num, &num2);
				this->points.mp = num;
				this->points.lifemp = num2;
			} else if (!strcmp(tag, "Mute")) {
				GetPlayer()->m_MutedAt = time(0);
				GetPlayer()->m_MuteLength = num;
			} 
			/* N */
			else if (!strcmp(tag, "Name"))	m_Name = line;
			/* P */
			else if (!strcmp(tag, "Page"))	{
				GetPlayer()->m_PageLength = MAX(0, num);
			}
			else if (!strcmp(tag, "Pass")) {
				strncpy(GetPlayer()->m_Password, line, MAX_PWD_LENGTH);
				GetPlayer()->m_Password[MAX_PWD_LENGTH] = '\0';
			}
			else if (!strcmp(tag, "Per "))	this->real_abils.Perception = num;
			else if (!strcmp(tag, "Pfin"))	GetPlayer()->m_PoofInMessage = line;
			else if (!strcmp(tag, "Pfou"))	GetPlayer()->m_PoofOutMessage = line;
			else if (!strcmp(tag, "Plyd"))	GetPlayer()->m_TotalTimePlayed = num;
			else if (!strcmp(tag, "Pref"))	GetPlayer()->m_Preferences = asciiflag_conv(line);
			else if (!strcmp(tag, "Prmp"))	GetPlayer()->m_Prompt = line;
			else if (!strcmp(tag, "PTtl"))	GetPlayer()->m_Pretitle = line;
			/* Q */
			/* R */
			else if (!strcmp(tag, "Race"))	m_Race = (Race)num;
			else if (!strcmp(tag, "Room"))	GetPlayer()->m_LoadRoom = num;
			/* S */
			else if (!strcmp(tag, "Sex "))	m_Sex = (Sex)num;
			else if (!strcmp(tag, "Skil")) {
				do {
					get_line(fl, line);
					sscanf(line, "%d %d", &num, &num2);
					if(num > 0 && num < NUM_SKILLS)
						GET_SKILL(this, num) = GET_REAL_SKILL(this, num) = num2;
				} while (num != 0);
			}
			else if (!strcmp(tag, "Stf "))	GetPlayer()->m_StaffPrivileges = asciiflag_conv(line);
			else if (!strcmp(tag, "Str "))	this->real_abils.Strength = num;
			/* T */
			else if (!strcmp(tag, "Titl"))	m_Title = line;
			/* V */
			else if (!strcmp(tag, "Vers"))	GetPlayer()->m_Version = num;
			/* W */
			else if (!strcmp(tag, "Wate"))	m_Weight = num;
			/* Default */
			else	log("SYSERR: Unknown tag %s in pfile %s", tag, name);
		}
	}
	fclose(fl);
	
	// Final data initialization
	this->aff_abils = this->real_abils;	// Copy abil scores
	m_NumCarriedItems = 0;
	m_CarriedWeight = 0;
	
	/* initialization for imms */
#if 0
	if(IS_STAFF(this)) {
		for(i = 1; i <= MAX_SKILLS; i++)
			GET_SKILL(this, i) =  MAX_SKILL_LEVEL;
//		this->general.conditions[0] = -1;
//		this->general.conditions[1] = -1;
//		this->general.conditions[2] = -1;
	}
#endif

	/*
	 * If you're not poisioned and you've been away for more than an hour of
	 * real time, we'll set your HMV back to full
	 */
	
	GetPlayer()->m_LoginTime = time(0);
	if (!AFF_FLAGGED(this, AFF_POISON) && ((GetPlayer()->m_LoginTime - GetPlayer()->m_LastLogin) >= SECS_PER_REAL_HOUR)) {
		GET_HIT(this) = GET_MAX_HIT(this);
		GET_MOVE(this) = GET_MAX_MOVE(this);
		//GET_ENDURANCE(this) = GET_MAX_ENDURANCE(this);
	}
	
	if (m_Name.empty() || (GetLevel() < 1))
		return false;
	
	if (IS_NPC(this))
		m_ActFlags = 0;
	
	return true;
}


/* Called when a character that follows/is followed dies */
void CharData::die_follower(void)
{
	if (m_Following)
		this->stop_follower();

	FOREACH(CharacterList, m_Followers, follower)
	{
		(*follower)->stop_follower();
	}
}


/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void CharData::stop_follower(void) {
	if (!m_Following)
	{
		core_dump();
		return;
	}

	if (AFF_FLAGGED(this, AFF_CHARM)) {
		act("You realize that $N is a jerk!", FALSE, this, 0, m_Following, TO_CHAR);
		act("$n realizes that $N is a jerk!", FALSE, this, 0, m_Following, TO_NOTVICT);
		act("$n hates your guts!", FALSE, this, 0, m_Following, TO_VICT);
		if (Affect::IsAffectedBy(this, Affect::Charm))
			Affect::FromChar(this, Affect::Charm);
	} else {
		act("You stop following $N.", FALSE, this, 0, m_Following, TO_CHAR);
		act("$n stops following $N.", TRUE, this, 0, m_Following, TO_NOTVICT);
		act("$n stops following you.", TRUE, this, 0, m_Following, TO_VICT);
	}

//	if (this->master->followers->follower == this) {	/* Head of follower-list? */
//		k = this->master->followers;
//		this->master->followers = k->next;
//		FREE(k);
//	} else {			/* locate follower who is not head of list */
//		for (k = this->master->followers; k->next->follower != this; k = k->next);
//
//		j = k->next;
//		k->next = j->next;
//		FREE(j);
//	}
	m_Following->m_Followers.remove(this);
	bool hasGroup = false;
	
	FOREACH(CharacterList, m_Following->m_Followers, follower)
	{
		if (AFF_FLAGGED(*follower, AFF_GROUP))
		{
			hasGroup = true;
			break;
		}
	}
	if (!hasGroup)
		AFF_FLAGS(m_Following).clear(AFF_GROUP);
	
	m_Following = NULL;
	
	Affect::FlagsFromChar(this, AFF_CHARM);
	m_AffectFlags.clear(AFF_GROUP);
}



#define READ_TITLE(ch, lev) (ch->GetSex() != SEX_FEMALE ?   \
	titles[(int)ch->GetRace()][lev].title_m :  \
	titles[(int)ch->GetRace()][lev].title_f)


void CharData::SetTitle(const char *title)
{
	if (title == NULL)
	{
		int tempLev;
	
		if (IS_STAFF(this))				tempLev = (GetLevel() - LVL_IMMORT) + 11;
		else if (GetLevel() == 100)		tempLev = 10;
		else							tempLev = ((GetLevel() /*- 1*/) / 10) + 1;

		title = READ_TITLE(this, tempLev);
	}
	
	m_Title = title;
}


void CharData::SetLevel(int level) {
	void do_start(CharData *ch);
	int is_altered = FALSE;
	int num_levels = 0;
	int oldpracs;

	if (IS_NPC(this))
		return;
	
	if (level < 1)
		level = 1;
	if (level == GetLevel())
		return;

	if (level < GetLevel())
		do_start(this);

	oldpracs = GetPlayer()->m_SkillPoints;
	while ((GetLevel() < LVL_CODER) && (GetLevel() < level))
	{
		m_Level += 1;
		num_levels++;
		advance_level(this);
	}

	if (num_levels == 1)
		send_to_char("You rise a level!\n", this);
	else {
		this->Send("You rise %d levels!\n", num_levels);
	}
	this->Send("You have earned %d Skill Points!\n", GetPlayer()->m_SkillPoints - oldpracs);
	
	SetTitle(NULL);
	if ((GetLevel() % 10) == 0)
	{
		//	Hit a rank
		this->Send("`nYou are now `r%s`n %s.", GetTitle(), GetName());
	}
	
	//	CJ: Reward skill points
}


void CharData::update_pos(void) {

/*	if ((GET_HIT(this) > 0) && (GET_POS(this) > POS_STUNNED))	return;
	else if (GET_HIT(this) > 0) {
		if (GET_POS(this) > POS_STUNNED)			GET_POS(this) = POS_STANDING;
		else if (!AFF_FLAGGED(this, AFF_TRAPPED))	GET_POS(this) = POS_SITTING;
	}
	else if (MOB_FLAGGED(this, MOB_MECHANICAL))		GET_POS(this) = POS_DEAD;
	else if (GET_HIT(this) <= -101)			GET_POS(this) = POS_DEAD;
	else if (GET_HIT(this) <= -75)			GET_POS(this) = POS_MORTALLYW;
	else if (GET_HIT(this) <= -50)			GET_POS(this) = POS_INCAP;
	else									GET_POS(this) = POS_STUNNED;
	this->UpdateWeight();
*/

	if (GET_HIT(this) > 0)
	{
		CombatEvent *event;
		
		if (AFF_FLAGGED(this, AFF_STUNNED))		GET_POS(this) = POS_STUNNED;
		else if (AFF_FLAGGED(this, AFF_TRAPPED))GET_POS(this) = POS_SITTING;
		else if (AFF_FLAGGED(this, AFF_SLEEP))	GET_POS(this) = POS_SLEEPING;
		else if (FIGHTING(this) && IN_ROOM(this) == IN_ROOM(FIGHTING(this))
			&& (!(event = (CombatEvent *)Event::FindEvent(m_Events, CombatEvent::Type))
			|| !event->IsRanged()))	GET_POS(this) = POS_FIGHTING;
		else if (GET_POS(this) <= POS_STUNNED || (GET_POS(this) == POS_FIGHTING && !FIGHTING(this)))
			GET_POS(this) = POS_STANDING;
	}
	else if (MOB_FLAGGED(this, MOB_MECHANICAL))	GET_POS(this) = POS_DEAD;
	else if (GET_HIT(this) <= -101)			GET_POS(this) = POS_DEAD;
	else if (GET_HIT(this) <= -75)			GET_POS(this) = POS_MORTALLYW;
	else if (GET_HIT(this) <= -50)			GET_POS(this) = POS_INCAP;
	else									GET_POS(this) = POS_STUNNED;
	
//	if (GET_POS(this) >= POS_STUNNED)
//		GET_LAST_ATTACKER(this) = 0;
	if (GET_LAST_ATTACKER(this) != 0 && GET_POS(this) >= POS_STUNNED
		&& (!AFF_FLAGGED(this, AFF_NOQUIT) || GET_HIT(this) > (int)(GET_MAX_HIT(this) * .6)))
	{
		GET_LAST_ATTACKER(this) = 0;
	}
	
	this->UpdateWeight();
}


VNum start_rooms[NUM_RACES] = {
	1300,		//	Human (Marine)
	1300,		//	Synth (Marine)
	642,		//	Yautja
	25070		//	Alien
};

/* Get the start room number */
RoomData *CharData::StartRoom(void)
{
	RoomData *	load_room = NULL;
	
	if (PLR_FLAGGED(this, PLR_FROZEN))
		load_room = world.Find(frozen_start_room);

	if (load_room == NULL && GET_TEAM(this))
		load_room = team_recalls[GET_TEAM(this)];
	
	if (load_room == NULL)
		load_room = world.Find(GetPlayer()->m_LoadRoom);

	if (load_room == NULL && GET_CLAN(this) && Clan::GetMember(this))
		load_room = world.Find(GET_CLAN(this)->GetRoom());
	
	/* If char was saved with NOWHERE, or world.Find above failed... */
	if (load_room == NULL)
		load_room = world.Find(IS_STAFF(this) ? immort_start_room : start_rooms[GetRace()]);

	if (load_room == NULL)
		load_room = world.Find(mortal_start_room);
	
	if (!PLR_FLAGGED(this, PLR_LOADROOM))
		GetPlayer()->m_LoadRoom.Clear();

	return load_room;
}


RoomData *CharData::LastRoom(void) {
	RoomData *last = world.Find(GetPlayer()->m_LastRoom);
	return last ? last : StartRoom();
}


float CharData::GetEffectiveSkill(int skillnum)
{
	if (skillnum < 1 || skillnum >= NUM_SKILLS)
		return 0;
//	if (ROOM_FLAGGED(IN_ROOM(this), ROOM_SPECIAL_MISSION))
//		return 15;
	
	float result = 0;
	switch (skill_info[skillnum].stat)
	{
		case STAT_STR:	result = GET_EFFECTIVE_STR(this);	break;
		case STAT_HEA:	result = GET_EFFECTIVE_HEA(this);	break;
		case STAT_COO:	result = GET_EFFECTIVE_COO(this);	break;
		case STAT_AGI:	result = GET_EFFECTIVE_AGI(this);	break;
		case STAT_PER:	result = GET_EFFECTIVE_PER(this);	break;
		case STAT_KNO:	result = GET_EFFECTIVE_KNO(this);	break;
	}
	
	if (!IS_NPC(this))
	{
		result += GET_EFFECTIVE(GET_SKILL(this, skillnum));
	}
	
	//	Todo... randomly round in either direction rather than trunc?
/*	float rounded = trunc(result);
	float fraction = result - rounded;
	
	int integerResult = rounded;
	
	if (fraction > 0.01)
	{
		int randomResult		= Number(0, 100000);
		float randomFraction	= randomResult / 100000.0f;
		
		if (randomFraction < fraction)
		{
			integerResult += 1;
		}
	}
	
	return integerResult;
*/
	return result;
}


Relation CharData::GetRelation(CharData *target) {
	Clan *	chClan, *tarClan;

	if (this == target ||									//	Same person
		NO_STAFF_HASSLE(this) || NO_STAFF_HASSLE(target))	//	One is a Staff
		return RELATION_FRIEND;
	
	if (GET_TEAM(this) && GET_TEAM(target))
	{
		if (GET_TEAM(this) == GET_TEAM(target) &&
			GET_TEAM(this) != 1 /* SOLO */)		return RELATION_FRIEND;
		else									return RELATION_ENEMY;
	}
	
	chClan = (IS_NPC(this) || Clan::GetMember(this)) ? GET_CLAN(this) : NULL;
	tarClan = (IS_NPC(target) || Clan::GetMember(target)) ? GET_CLAN(target) : NULL;
	
	if (chClan && tarClan)
	{
		Relation relation = chClan->GetRelation(tarClan);
		
		if (relation == RELATION_ENEMY)	return RELATION_ENEMY;
		else if (relation == RELATION_FRIEND)
		{
			if (AFF_FLAGGED(this, AFF_TRAITOR) ^ AFF_FLAGGED(target, AFF_TRAITOR))
				return RELATION_ENEMY;
			return RELATION_FRIEND;
		}
	}
	
	if (MOB_FLAGGED(this, MOB_FRIENDLY) || MOB_FLAGGED(target, MOB_FRIENDLY))
		return RELATION_FRIEND;
	
	if (GetFaction() != target->GetFaction())
	{
		//	TODO: Test for Temporary Faction Friendliness
		return RELATION_ENEMY;						//	Seperate Race
	}
	
	if (AFF_FLAGGED(this, AFF_TRAITOR) ^ AFF_FLAGGED(target, AFF_TRAITOR))
		return RELATION_ENEMY;						//	One is a traitor, other is not
	
	if (chClan && tarClan)
		return RELATION_NEUTRAL;					//	Both clanned, seperate clans

	return RELATION_FRIEND;							//	Friendly
}


bool CharData::LightOk(void) {
	return LightOk(InRoom());
}


bool CharData::LightOk(RoomData *room) {
	if (AFF_FLAGGED(this, AFF_BLIND))
		return false;
	
	return room == NULL
		|| PRF_FLAGGED(this, PRF_HOLYLIGHT)
		|| AFF_FLAGGED(this, AFF_INFRAVISION)
		|| IS_ALIEN(this)				//	RACE-INFRA
		|| room->IsLight();
}


float CharData::ModifiedStealth() {
	float	curStealth = GetEffectiveSkill(SKILL_STEALTH) - GET_ROLL_STEALTH(this);
	
//	if (AFF_FLAGGED(this, AFF_NOQUIT))
//		return 0;
	
	//	Apply modifiers
	if (m_AffectFlags.testForAny(Affect::AFF_INVISIBLE_FLAGS))
	{
		if (m_AffectFlags.testForAll(Affect::AFF_INVISIBLE_FLAGS))	curStealth += 10;
		else if (m_AffectFlags.test(AFF_INVISIBLE_2))				curStealth += 8;
		else if (m_AffectFlags.test(AFF_INVISIBLE))					curStealth += 5;
	}
	
	if (AFF_FLAGGED(this, AFF_HIDE) /*&& !AFF_FLAGGED(this, AFF_NOQUIT)*/)
		curStealth += 5;
	
//	if (IS_ALIEN(this) && ROOM_FLAGGED(IN_ROOM(this), ROOM_HIVED))	curStealth += 2;
	
	return curStealth;
}


float CharData::ModifiedNotice(CharData *target) {
	float	curNotice = GetEffectiveSkill(SKILL_NOTICE) - GET_ROLL_NOTICE(this);
	
	//	Apply modifiers
	if (AFF_FLAGS(target).testForAny(Affect::AFF_INVISIBLE_FLAGS) && AFF_FLAGGED(this, AFF_DETECT_INVIS))
			curNotice += 5;
	
	if (AFF_FLAGGED(target, AFF_HIDE) && AFF_FLAGGED(this, AFF_SENSE_LIFE))
			curNotice += 5;
	
	return curNotice;
}


Visibility CharData::InvisOk(CharData *target) {
/*	if (AFF_FLAGGED(target, AFF_INVISIBLE) && !AFF_FLAGGED(this, AFF_DETECT_INVIS))
		return false;
	
	if (AFF_FLAGGED(target, AFF_HIDE) && !AFF_FLAGGED(this, AFF_SENSE_LIFE))
		return false;
*/
	
	if (GetRelation(target) == RELATION_FRIEND)			return VISIBLE_FULL;
	if (!AFF_FLAGS(target).testForAny(Affect::AFF_STEALTH_FLAGS))	return VISIBLE_FULL;
	
	if (AFF_FLAGGED(target, AFF_COVER))
	{
		if (IN_ROOM(this) != IN_ROOM(target) && !AFF_FLAGGED(target, AFF_DIDFLEE))
			return VISIBLE_NONE;
	}
	
	float	curStealth = target->ModifiedStealth();
	float	curNotice = this->ModifiedNotice(target);
	
	if (AFF_FLAGGED(target, AFF_HIDE))
	{
		if (curNotice < curStealth
			|| (IN_ROOM(this) != IN_ROOM(target) && !AFF_FLAGGED(target, AFF_NOQUIT)))
			return VISIBLE_NONE;
	}
	
	if (AFF_FLAGS(target).testForAny(Affect::AFF_INVISIBLE_FLAGS) /* &&
		!ROOM_FLAGGED(IN_ROOM(target), ROOM_SPECIAL_MISSION)*/)
	{
		if (curNotice < (curStealth - 5.0f) && !FIGHTING(target)
			&& !AFF_FLAGGED(target, AFF_NOQUIT))	return VISIBLE_NONE;
		if (curNotice < (curStealth + 5.0f))		return VISIBLE_SHIMMER;
		if (IN_ROOM(this) != IN_ROOM(target) && !AFF_FLAGGED(target, AFF_NOQUIT))	return VISIBLE_SHIMMER;
	}
	
	return VISIBLE_FULL;
}


Visibility CharData::CanSee(CharData *target, bool bGloballyVisible)
{
	if (!target)								return VISIBLE_NONE;
	if (target == this)							return VISIBLE_FULL;	//	this == target

	if (GET_REAL_LEVEL(this) < target->GetPlayer()->m_StaffInvisLevel)
		return VISIBLE_NONE;
	
	if (PRF_FLAGGED(this, PRF_HOLYLIGHT))	return VISIBLE_FULL;
	
	if (MOB_FLAGGED(target, MOB_PROGMOB))	return VISIBLE_NONE;	//	ProgMob check
	
	if (bGloballyVisible)					return VISIBLE_FULL;
	
	if (!LightOk(IN_ROOM(target)))			return VISIBLE_NONE;	//	Enough light to see
	return InvisOk(target);											//	Invis check
}


bool CharData::CanSee(ObjData *target) {
	if (!target)
		return false;
	
	if (PRF_FLAGGED(this, PRF_HOLYLIGHT))
		return true;
	
	if (!LightOk(target->InRoom()) && target->GetType() != ITEM_LIGHT && target->CarriedBy() != this)
		return false;
	
	if (OBJ_FLAGGED(target, ITEM_INVISIBLE) && !AFF_FLAGGED(this, AFF_DETECT_INVIS))
		return false;
	
	if (OBJ_FLAGGED(target, ITEM_STAFFONLY) && !IS_STAFF(this))
		return false;
	
	if (target->Inside() && !CanSee(target->Inside()))
		return false;
	
	if (target->CarriedBy() && CanSee(target->CarriedBy()) == VISIBLE_NONE)
		return false;
	
	return true;
}

#if 0
bool CharData::CanSee(ShipData *target) {
//	ShipData *ship;

	if (!this || !target)
		return false;

	if (PRF_FLAGGED(this, PRF_HOLYLIGHT))
		return true;

	if (!LightOk())
		return false;

//	if (IN_ROOM(target) != NOWHERE)
//		if (ROOM_FLAGGED(IN_ROOM(target), ROOM_IDETECT))
//			return true;

/*	if ((ship = ship_from_room(IN_ROOM(this))) != NULL) {
		if (ship->CanSee(target))	// this probably overrides the second
			return true;

		if (target->IsCloaked()) {
			return (SHP_FLAGGED(ship, SHIP_DETECTOR) && ship->Distance(target) <= 1000);
		}
	}
*/
	return true;
}
#endif



void CharData::Appear(Affect::Flags flags)
{
	Affect::Flags	oldBits = m_AffectFlags & flags;

	Affect::FlagsFromChar(this, flags);

	if (oldBits.testForAny(Affect::AFF_INVISIBLE_FLAGS))
	{
		if (IS_STAFF(this))							act("You feel a strange presence as $n appears, seemingly from nowhere.", FALSE, this, 0, 0, TO_ROOM);
		else /*if (GetRace() == RACE_PREDATOR)*/	act("A faint shimmer in the air slowly becomes more real as $n's stealth field fades.", FALSE, this, 0, 0, TO_ROOM);
		//else										act("$n slowly fades into existence.", FALSE, this, 0, 0, TO_ROOM);
		Send("You become visible.\n");
	}
	else if (oldBits.test(AFF_HIDE))
	{
		act("$n comes out of hiding.", FALSE, this, 0, 0, TO_ROOM);
		Send("You come out of hiding.\n");
	}
	else if (oldBits.test(AFF_COVER))
	{
		act("$n comes out of cover and exposes $mself.", FALSE, this, 0, 0, TO_ROOM);
		Send("You come out of cover.\n");
	}
}


RestrictionType CharData::CanUse(ObjData *obj) {
	if (NO_STAFF_HASSLE(this))	return NotRestricted;
	
	if (GetRace() != RACE_UNDEFINED && obj->GetRaceRestriction().test(GetRace()))	return RestrictedByRace;
	if (obj->GetMinimumLevelRestriction() > GetLevel())	return RestrictedByLevel;
	if (obj->GetClanRestriction()	//	If its clan restricted
			 //	and this is a player not in a clan
		&& (!GET_CLAN(this) || (!IS_NPC(this) && !Clan::GetMember(this))
			//	or they are not in this clan
			|| obj->GetClanRestriction() != GET_CLAN(this)->GetID()))
		return RestrictedByClan;
	
	return NotRestricted;
}

#if 0
bool CharData::CanUse(ShipData *ship) {
	bool found = true;

/*	switch (ship->size) {
		case FIGHTER_SHIP:
			if (!GET_SKILL(this, SKILL_STARFIGHTERS))
			found = false;
			break;
		case MIDSIZE_SHIP:
			if (!GET_SKILL(this, SKILL_TRANSPORTS))
			found = false;
			break;
		case LARGE_SHIP:
			if (!GET_SKILL(this, SKILL_FRIGATES))
			found = false;
			break;
		case CAPITAL_SHIP:
			if (!GET_SKILL(this, SKILL_CAPITALSHIPS))
			found = false;
			break;
		case SHIP_PLATFORM:
		default:
			break;
	}
*/

	if (!found)
		this->Send("You have no idea how to fly a ship this size.\n");
	return found;
}
#endif

void CharData::AddTellBuf(const char *buf) {
	if (!GetPlayer() || GetPlayer() == &dummy_player || !buf || !*buf)
		return;
	
	if (GetPlayer()->m_TellBuffer.size() >= 20)
		GetPlayer()->m_TellBuffer.pop_front();
	GetPlayer()->m_TellBuffer.push_back(buf);
}

void CharData::AddWizBuf(const char *buf) {
	if (!GetPlayer() || GetPlayer() == &dummy_player || !buf || !*buf)
		return;
	
	if (GetPlayer()->m_WizBuffer.size() >= 20)
		GetPlayer()->m_WizBuffer.pop_front();
	GetPlayer()->m_WizBuffer.push_back(buf);
}

void CharData::AddSayBuf(const char *buf) {
	if (!GetPlayer() || GetPlayer() == &dummy_player || !buf || !*buf)
		return;
	
	if (GetPlayer()->m_SayBuffer.size() >= 20)
		GetPlayer()->m_SayBuffer.pop_front();
	GetPlayer()->m_SayBuffer.push_back(buf);
}

void CharData::AddChatBuf(const char *buf) {
	if (!GetPlayer() || GetPlayer() == &dummy_player || !buf || !*buf)
		return;
	
	if (GetPlayer()->m_ChatBuffer.size() >= 20)
		GetPlayer()->m_ChatBuffer.pop_front();
	GetPlayer()->m_ChatBuffer.push_back(buf);
}

void CharData::AddInfoBuf(const char *buf) {
	if (!GetPlayer() || GetPlayer() == &dummy_player || !buf || !*buf)
		return;
	
	if (GetPlayer()->m_InfoBuffer.size() >= 20)
		GetPlayer()->m_InfoBuffer.pop_front();
	GetPlayer()->m_InfoBuffer.push_back(buf);
}




bool CharData::IsIgnoring(IDNum id)
{
	return !IS_NPC(this) && GetPlayer() &&
		Lexi::IsInContainer(GetPlayer()->m_IgnoredPlayers, id);
}

void CharData::AddIgnore(IDNum id)
{
	GetPlayer()->m_IgnoredPlayers.push_back(id);
}

void CharData::RemoveIgnore(IDNum id)
{
	GetPlayer()->m_IgnoredPlayers.remove(id);
}

	
ObjData *CharData::GetWeapon()
{
	ObjData *wielded = GET_EQ(this, WEAR_HAND_R);
	if (!wielded || (GET_OBJ_TYPE(wielded) != ITEM_WEAPON))
	{
		wielded = GET_EQ(this, WEAR_HAND_L);
		if (wielded && (GET_OBJ_TYPE(wielded) != ITEM_WEAPON))
			wielded = NULL;
	}
	
	return wielded;
}


ObjData *CharData::GetSecondaryWeapon(void)
{
	ObjData *wielded = GET_EQ(this, WEAR_HAND_R);
	if (wielded && (GET_OBJ_TYPE(wielded) == ITEM_WEAPON))
	{
		wielded = GET_EQ(this, WEAR_HAND_L);
		if (wielded && GET_OBJ_TYPE(wielded) != ITEM_WEAPON)
			wielded = NULL;
	}
	else
		wielded = NULL;
	
	return wielded;
}


ObjData *CharData::GetGun(void)
{
	ObjData *wielded = GET_EQ(this, WEAR_HAND_R);
	if (!wielded || (GET_OBJ_TYPE(wielded) != ITEM_WEAPON) || !IS_GUN(wielded))
	{
		wielded = GET_EQ(this, WEAR_HAND_L);
		if (!wielded || (GET_OBJ_TYPE(wielded) != ITEM_WEAPON) || !IS_GUN(wielded))
		{
			wielded = NULL;
			
			for (int i = 0; i < NUM_WEARS; ++i)
			{
				ObjData *o = GET_EQ(this, i);
				
				if (o && GET_OBJ_TYPE(o) == ITEM_WEAPON && IS_GUN(o))
				{
					wielded = o;
					break;
				}
			}
		}
	}
	
	return wielded;
}


void CharData::AddOrReplaceEvent(Event *newEvent)
{
	Event *	event = Event::FindEvent(m_Events, newEvent->GetType());
	
	if (event)
	{
		m_Events.remove(event);
		event->Cancel();
	}

	m_Events.push_front(newEvent);
}


CharData *CharData::Create(CharacterPrototype *proto)
{
	return proto ? Create(proto->m_pProto) : NULL;
}


CharData *CharData::Create(CharData *ch)
{
	static unsigned int mob_tick_count = 0;
	
	CharData *mob = new CharData(*ch);
	
	mob->points.max_hit = MAX((unsigned)1, GET_MOB_BASE_HP(mob)) + Number(0, GET_MOB_VARY_HP(mob));
	mob->points.hit = mob->points.max_hit;
	mob->points.move = mob->points.max_move;
	//mob->points.endurance = mob->points.max_endurance;

	mob->mob->m_UpdateTick = mob_tick_count++;
	mob_tick_count = mob_tick_count & TICK_WRAP_COUNT_MASK;
	

	CharacterPrototype *prototype = ch->GetPrototype();
	if (prototype)
	{
		/* add triggers */
		BehaviorSet::ApplySets(prototype->m_BehaviorSets, mob->GetScript());
		
		FOREACH(ScriptVector, prototype->m_Triggers, trigVid)
		{
			ScriptPrototype *proto = trig_index.Find(*trigVid);

			if (!proto)
				mudlogf(NRM, LVL_STAFF, TRUE, "SYSERR: Invalid trigger %s assigned to mob %s", trigVid->Print().c_str(), ch->GetVirtualID().Print().c_str());
			else
			{
				mob->GetScript()->AddTrigger(TrigData::Create(proto));
			}
		}

		SCRIPT(mob)->m_Globals.Add(prototype->m_InitialGlobals);
	}
	
//	GET_ID(mob) = max_id++;
	mob->ToWorld();
	
	mob->AffectTotal();
	
	return mob;
}


void finish_load_mob(CharData *mob)
{
	FOREACH(MobEquipmentList, mob->mob->m_StartingEquipment, mobeq)
	{
		ObjData *obj = (mobeq->m_Virtual.IsValid()) ? ObjData::Create(mobeq->m_Virtual) : NULL;
		
		if (!obj)
			mudlogf(NRM, LVL_STAFF, TRUE, "SYSERR: Invalid object %s assigned to mob %s", mobeq->m_Virtual.Print().c_str(), mob->GetVirtualID().Print().c_str());
		else
		{
			int		pos = mobeq->m_Position;
		
			if (pos == -1 || GET_EQ(mob, pos))		obj->ToChar(mob, true);
			else									equip_char(mob, obj, pos);
			
			load_otrigger(obj);
			if (pos != -1 && GET_EQ(mob, pos) == obj && !PURGED(obj) && !PURGED(mob))
				wear_otrigger(obj, mob, pos);
		}
	}
	mob->mob->m_StartingEquipment = MobEquipmentList();	//	Forces a deallocation, .clear() may not?
	
	load_mtrigger(mob);
}


void CharData::Parse(Lexi::Parser &parser, VirtualID vid)
{
	CharData *mob = new CharData;
	mob->mob = new MobData;
	
	CharacterPrototype *proto = mob_index.insert(vid, mob);
	mob->SetPrototype(proto);
	
	mob->m_Keywords				= parser.GetString("Keywords", "mob undefined");
	mob->m_Name					= parser.GetString("Name", "an undefined mob");
	mob->m_RoomDescription		= parser.GetString("RoomDescription", "An undefined mob.\n");
	mob->m_Description			= parser.GetString("Description");
	
	//	Known work-around to modify a shared string directly
	const char *tmpptr = mob->m_Name.c_str();
	if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") || !str_cmp(fname(tmpptr), "the"))
		mob->m_Name.GetStringRef()[0] = tolower(*tmpptr);
	
	mob->m_Sex					= parser.GetEnum("Gender", SEX_NEUTRAL);
	mob->m_Race					= parser.GetEnum("Race", race_types, RACE_UNDEFINED);
	mob->m_Level				= parser.GetInteger("Level");
	
	GET_MOB_BASE_HP(mob)		= parser.GetInteger("BaseHP");
	GET_MOB_VARY_HP(mob)		= parser.GetInteger("VaryHP");
	
	mob->points.lifemp			= parser.GetInteger("MPValue");
	
	GET_POS(mob)				= parser.GetEnum("Position", POS_STANDING);
	GET_DEFAULT_POS(mob)		= parser.GetEnum("DefaultPosition", POS_STANDING);

	MOB_FLAGS(mob)				= parser.GetFlags("MobFlags", action_bits);
	AFF_FLAGS(mob)				= parser.GetBitFlags<AffectFlag>("AffectFlags");

	SET_BIT(MOB_FLAGS(mob), MOB_ISNPC);	//	Done here, because it's a mob!
	
	mob->real_abils.Strength	= MIN(parser.GetInteger("Attributes/Strength", 120), MAX_STAT);
	mob->real_abils.Health		= MIN(parser.GetInteger("Attributes/Health", 120), MAX_STAT);
	mob->real_abils.Coordination= MIN(parser.GetInteger("Attributes/Coordination", 120), MAX_STAT);
	mob->real_abils.Agility		= MIN(parser.GetInteger("Attributes/Agility", 120), MAX_STAT);
	mob->real_abils.Perception	= MIN(parser.GetInteger("Attributes/Perception", 120), MAX_STAT);
	mob->real_abils.Knowledge	= MIN(parser.GetInteger("Attributes/Knowledge", 120), MAX_STAT);
	
	mob->mob->m_AttackType		= parser.GetEnum("Attack/Type", attack_types);
	mob->mob->m_AttackDamage	= parser.GetInteger("Attack/Damage");
	mob->mob->m_AttackDamageType= parser.GetEnum("Attack/DamageType", damage_types, DAMAGE_BLUNT);
	mob->mob->m_AttackRate		= parser.GetInteger("Attack/Rate");
	mob->mob->m_AttackSpeed		= RANGE(parser.GetFloat("Attack/Speed", 1.0f), 0.0f, 5.0f);
	
	{
		PARSER_SET_ROOT(parser, "Armor");
		for (int i = 0; i < NUM_DAMAGE_TYPES; ++i)
		{
			GET_ARMOR(mob, ARMOR_LOC_NATURAL, i) = parser.GetInteger(damage_types[i]);
		}
	}
	
	if (MOB_FLAGGED(mob, MOB_AI))
	{
		mob->mob->m_AIAggrRoom		= parser.GetInteger("AI/AggressionRoom");
		mob->mob->m_AIAggrRanged	= parser.GetInteger("AI/AggressionRanged");
		mob->mob->m_AIAggrLoyalty	= parser.GetInteger("AI/AggressionLoyal");
		mob->mob->m_AIAwareRange	= parser.GetInteger("AI/AwarenessRanged");
		mob->mob->m_AIMoveRate		= parser.GetInteger("AI/MovementRate");
	}
	
	const int OpinionCount = 2;
	const char *OpinionTypes[OpinionCount] = { "Hate", "Fear" };
	Opinion *Opinions[OpinionCount] = { &mob->mob->m_Hates, &mob->mob->m_Fears };
	
	for (int i = 0; i < OpinionCount; ++i)
	{
		Lexi::String	section = "Opinions/";
		section += OpinionTypes[i];
		
		if (!parser.DoesSectionExist(section))
			continue;
		
		Opinion *op = Opinions[i];
		
		PARSER_SET_ROOT(parser, section);
		op->Add(Opinion::TypeSex, parser.GetFlags<Sex>("Genders"));
		op->Add(Opinion::TypeRace, parser.GetFlags("Races", race_types));
		if (parser.DoesFieldExist("VNum"))	op->Add(Opinion::TypeVNum, parser.GetVirtualID("VNum", proto->GetVirtualID().zone));
		else								op->Add(Opinion::TypeVNum, parser.GetVirtualID("Virtual", proto->GetVirtualID().zone));
	}
	
	mob->m_Clan			= Clan::GetClan(parser.GetInteger("Affiliations/Clan", -1));
	mob->m_Team			= parser.GetEnum("Affiliations/Team", team_names);	
	
	int numSets = parser.NumFields("ScriptSets");
	for (int i = 0; i < numSets; ++i)
	{
		BehaviorSet *set = BehaviorSet::Find(parser.GetIndexedString("ScriptSets", i));
		if (set)	proto->m_BehaviorSets.push_back(set);
	}
	
	int numScripts = parser.NumFields("Scripts");
	for (int i = 0; i < numScripts; ++i)
	{
		VirtualID	script = parser.GetIndexedVirtualID("Scripts", i, proto->GetVirtualID().zone);
		if (script.IsValid())	proto->m_Triggers.push_back(script);
	}
	
	int numVariables = parser.NumSections("Variables");
	for (int i = 0; i < numVariables; ++i)
	{
		PARSER_SET_ROOT_INDEXED(parser, "Variables", i);
		
		ScriptVariable *var = ScriptVariable::Parse(parser);
		
		if (var)
			proto->m_InitialGlobals.Add(var);
	}
	
	int numEquipment = parser.NumSections("Equipment");
	for (int i = 0; i < numEquipment; ++i)
	{
		PARSER_SET_ROOT_INDEXED(parser, "Equipment", i);
		
		struct MobEquipment eq;
		if (parser.DoesFieldExist("VNum" ))		eq.m_Virtual	= parser.GetVirtualID("VNum", vid.zone);
		else									eq.m_Virtual	= parser.GetVirtualID("Virtual", vid.zone);
		eq.m_Position			= parser.GetEnum("Location", eqpos, -1);
		
		if (eq.m_Virtual.IsValid())	mob->mob->m_StartingEquipment.push_back(eq);
	}
	
	//	Initializations not done above
	//GET_MAX_ENDURANCE(mob) = 120;
	
	mob->aff_abils = mob->real_abils;
}


void CharData::Write(Lexi::OutputParserFile &file)
{
	file.WriteVirtualID("Virtual", GetVirtualID(), GetVirtualID().zone);

	file.WriteString("Keywords", this->m_Keywords);
	file.WriteString("Name", this->m_Name);
	file.WriteLongString("RoomDescription", this->m_RoomDescription);
	file.WriteLongString("Description", this->m_Description);
	file.WriteEnum("Race", GetRace(), race_types, RACE_UNDEFINED);
	file.WriteEnum("Gender", GetSex());
	file.WriteInteger("Level", GetLevel());
	file.WriteInteger("BaseHP", GET_MOB_BASE_HP(this));
	file.WriteInteger("VaryHP", GET_MOB_VARY_HP(this), 0);
	file.WriteInteger("MPValue", this->points.lifemp, 0);
	file.WriteEnum("Position", GET_POS(this), POS_STANDING);
	file.WriteEnum("DefaultPosition", GET_DEFAULT_POS(this), POS_STANDING);
	file.WriteFlags("MobFlags", MOB_FLAGS(this) & ~MOB_ISNPC, action_bits);
	file.WriteFlags("AffectFlags", m_AffectFlags);

	file.BeginSection("Attributes");
		file.WriteInteger("Strength",		GET_STR(this));
		file.WriteInteger("Health",			GET_HEA(this));
		file.WriteInteger("Coordination",	GET_COO(this));
		file.WriteInteger("Agility",		GET_AGI(this));
		file.WriteInteger("Perception",		GET_PER(this));
		file.WriteInteger("Knowledge",		GET_KNO(this));
	file.EndSection();

	file.BeginSection("Attack");
		file.WriteEnum("Type", this->mob->m_AttackType, attack_types);
		file.WriteEnum("DamageType", this->mob->m_AttackDamageType, damage_types);
		file.WriteInteger("Damage", this->mob->m_AttackDamage, 0);
		file.WriteInteger("Rate", this->mob->m_AttackRate, 0);
		//	Special Case to enforce %.2f formatting
		if (this->mob->m_AttackSpeed != 1.0f)
		{
			file.WriteString("Speed", Lexi::Format("%.2f", this->mob->m_AttackSpeed).c_str());
		}
	file.EndSection();

	file.BeginSection("Armor");
		for (int n = 0; n < NUM_DAMAGE_TYPES; ++n)
		{
			file.WriteInteger(damage_types[n], GET_ARMOR(this, ARMOR_LOC_NATURAL, n), 0);
		}
	file.EndSection();

	if (MOB_FLAGGED(this, MOB_AI))
	{
		file.BeginSection("AI");
			file.WriteInteger("AggressionRoom", this->mob->m_AIAggrRoom, 0);
			file.WriteInteger("AggressionRanged", this->mob->m_AIAggrRanged, 0);
			file.WriteInteger("AggressionLoyal", this->mob->m_AIAggrLoyalty, 0);
			file.WriteInteger("AwarenessRanged", this->mob->m_AIAwareRange, 0);
			file.WriteInteger("MovementRate", this->mob->m_AIMoveRate, 0);
		file.EndSection();
	}


	file.BeginSection("Opinions");
		const int OpinionCount = 2;
		const char *OpinionTypes[OpinionCount] = { "Hate", "Fear" };
		Opinion *Opinions[OpinionCount] = { &this->mob->m_Hates, &this->mob->m_Fears };

		for (int n = 0; n < OpinionCount; ++n)
		{
			Opinion *op = Opinions[n];
			
			if (!op->IsActive())
				continue;
			
			file.BeginSection(OpinionTypes[n]);
				file.WriteFlags("Genders", op->m_Sexes);
				file.WriteFlags("Races", op->m_Races, race_types);
				file.WriteVirtualID("Virtual", op->m_VNum, GetVirtualID().zone);
			file.EndSection();
		}
	file.EndSection();

	file.BeginSection("Affiliations");
	{
		if (GET_CLAN(this))		file.WriteInteger("Clan", GET_CLAN(this)->GetID());
		if (GET_TEAM(this))		file.WriteEnum("Team", GET_TEAM(this), team_names);
	}
	file.EndSection();
	
	file.BeginSection("ScriptSets");
	{
		BehaviorSets &	sets = GetPrototype()->m_BehaviorSets;
		int count = 0;
		FOREACH(BehaviorSets, sets, iter)
		{
			BehaviorSet *set = *iter;
			if (set)	file.WriteString(Lexi::Format("Set %d", ++count).c_str(), set->GetName());
		}
	}
	file.EndSection();
		
	file.BeginSection("Scripts");
	{
		int count = 0;
		FOREACH(ScriptVector, GetPrototype()->m_Triggers, iter)
			file.WriteVirtualID(Lexi::Format("Script %d", ++count).c_str(), *iter, GetPrototype()->GetVirtualID().zone);
	}
	file.EndSection();

	file.BeginSection("Variables");
	{
		int count = 0;
		FOREACH(ScriptVariable::List, GetPrototype()->m_InitialGlobals, var)
		{
			(*var)->Write(file, Lexi::Format("Variable %d", ++count), false);
		}
	}
	file.EndSection();

	file.BeginSection("Equipment");
	{
		int count = 0;
		FOREACH(MobEquipmentList, this->mob->m_StartingEquipment, eq)
		{
			if (!eq->m_Virtual.IsValid())	continue;
			
			file.BeginSection(Lexi::Format("Item %d", ++count));
				file.WriteVirtualID("Virtual", eq->m_Virtual, GetVirtualID().zone);
				file.WriteEnum("Location", eq->m_Position, eqpos, -1);
			file.EndSection();
		}
		
	}
	file.EndSection();
}


CharData *CharData::Find(IDNum id)
{
	Entity *e = IDManager::Instance()->Find(id);
	return dynamic_cast<CharData *>(e);
}


const char *PERS(CharData *ch, CharData *to)
{
	if (ch == to)				return "you";
	
	switch (to->CanSee(ch))
	{
		case VISIBLE_FULL:		return ch->GetName();
		case VISIBLE_SHIMMER:	return "a faint shimmer";
		case VISIBLE_NONE:
		default:				return "someone";
	}
}


const char *PERSS(CharData *ch, CharData *to)
{
	static char buf[1024];	//	I hate this...
	
	if (ch == to)				return "your";
	
	switch (to->CanSee(ch))
	{
		case VISIBLE_SHIMMER:	return "a faint shimmer's";
		case VISIBLE_NONE:		return "someone's";
	}
	
	sprintf(buf, "%s's", ch->GetName());
	
	return buf;
}


const char *HMHR(CharData *ch, CharData *to)
{
	if (ch == to)				return "you";
	
	switch (to->CanSee(ch))
	{
		case VISIBLE_SHIMMER:
		case VISIBLE_NONE:		return "it";
		default:				return HMHR(ch);
	}
}


const char *HMHR(CharData *ch)
{
	switch (ch->GetSex())
	{
		case SEX_MALE:			return "him";
		case SEX_FEMALE:		return "her";
		default:				return "it";
	}
}


const char *HSHR(CharData *ch, CharData *to)
{
	if (ch == to)				return "your";
	
	switch (to->CanSee(ch))
	{
		case VISIBLE_SHIMMER:
		case VISIBLE_NONE:		return "its";
		default:				return HSHR(ch);
	}
}


const char *HSHR(CharData *ch)
{
	switch (ch->GetSex())
	{
		case SEX_MALE:			return "his";
		case SEX_FEMALE:		return "her";
		default:				return "its";
	}
}


const char *HSSH(CharData *ch, CharData *to)
{
	if (ch == to)				return "you";
	
	switch (to->CanSee(ch))
	{
		case VISIBLE_SHIMMER:
		case VISIBLE_NONE:		return "it";
		default:				return HSSH(ch);
	}
}


const char *HSSH(CharData *ch)
{
	switch (ch->GetSex())
	{
		case SEX_MALE:			return "he";
		case SEX_FEMALE:		return "she";
		default:				return "it";
	}
}













/* new save_char that writes an ascii pfile */
void CharData::Save()
{
	BUFFER(buf, MAX_STRING_LENGTH);

	ObjData *	char_eq[NUM_WEARS];
	

	if (IS_NPC(this) || m_Name.empty())
		return;

	/*
	 * save current values, because unequip_char will call
	 * affect_total(), which will reset points to lower values
	 * if player is wearing +points items.  We will restore old
	 * points level after reequiping.
	 */
	int hit = GET_HIT(this);
	int move = GET_MOVE(this);
	//int endurance = GET_ENDURANCE(this);

	Lexi::String	filename = GetFilename();
	Lexi::String	tempFilename = filename + ".tmp";

	Lexi::OutputParserFile	file(tempFilename.c_str());
	
	if (file.fail())
	{
		mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: Cannot write player %s to disk: unable to open temporary file '%s'", GetName(), tempFilename.c_str());
		return;
	}
	
	file.BeginParser();

	//	Unequip everything a character can be affected by
	for (int i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(this, i))	char_eq[i] = unequip_char(this, i);
		else					char_eq[i] = NULL;
	}
	
	//	WARNING: This MUST be first!
	file.WriteString("Name", GetName());
	file.WriteInteger("ID", GetID());
	
	file.WriteInteger("Version", GetPlayer()->m_Version);
	
	file.WriteString("Password", GetPlayer()->m_Password);
	
	file.WriteString("Title", GetTitle());
	file.WriteString("Pretitle", GetPlayer()->m_Pretitle);
	file.WriteLongString("Description", m_Description);
	
	file.WriteEnum("Race", GetRace(), race_types);
	file.WriteEnum("Gender", GetSex());
	file.WriteInteger("Level", GetLevel());
	
	file.WriteFlags("Flags", m_ActFlags & ~(PLR_CRASH), player_bits);

	file.BeginSection("Login");
	{
		file.WriteDate("Creation", GetPlayer()->m_Creation);
		file.WriteInteger("TimePlayed", GetPlayer()->m_TotalTimePlayed + (time(0) - GetPlayer()->m_LoginTime));
		file.WriteDate("LastLogin", (this->desc) ? time(0) : GetPlayer()->m_LastLogin);
		file.WriteString("Host", (this->desc) ? this->desc->m_Host.c_str() : GetPlayer()->m_Host.c_str());
		file.WriteString("HostMask", GetPlayer()->m_MaskedHost);
		file.WriteInteger("FailedAttempts", GetPlayer()->m_BadPasswordAttempts, 0);
	}
	file.EndSection();
	
	file.BeginSection("Attributes");
	{
		file.WriteInteger("Strength", this->real_abils.Strength);
		file.WriteInteger("Health", this->real_abils.Health);
		file.WriteInteger("Coordination", this->real_abils.Coordination);
		file.WriteInteger("Agility", this->real_abils.Agility);
		file.WriteInteger("Perception", this->real_abils.Perception);
		file.WriteInteger("Knowledge", this->real_abils.Knowledge);
		
	}
	file.EndSection();
	
	file.BeginSection("Points");
	{
		file.WriteInteger("HP", this->points.hit);
		file.WriteInteger("MaxHP", this->points.max_hit);
		file.WriteInteger("MV", this->points.move);
		file.WriteInteger("MaxMV", this->points.max_move);
		//file.WriteInteger("EN", this->points.endurance);
		//file.WriteInteger("MaxEN", this->points.max_endurance);
		file.WriteInteger("MP", this->points.mp);
		file.WriteInteger("TotalMP", this->points.lifemp);
		file.WriteInteger("SP", GetPlayer()->m_SkillPoints, 0);
		file.WriteInteger("TotalSP", GetPlayer()->m_LifetimeSkillPoints);
	}
	file.EndSection();
	
	file.BeginSection("Skills");
	{
		for (int skillnum = 1; skillnum < NUM_SKILLS; ++skillnum)
		{
			const char *skillname = skill_name(skillnum);
			
			if (*skillname == '!')
				continue;
			
			file.WriteInteger(skillname, GET_REAL_SKILL(this, skillnum), 0);
		}
	}
	file.EndSection();
	
	if (GET_CLAN(this))	file.WriteInteger("Clan", GET_CLAN(this)->GetID());
	
	
	file.BeginSection("Affects");
	{
		//	Is there any actual bit that needs to be saved ???
		//	Everything should be applied by an Affect or is temporary!!!
//		Flags	affected_by = m_AffectBits & ~(AFF_GROUP | AFF_NOQUIT | AFF_NOSHOOT | AFF_DIDSHOOT | AFF_DIDFLEE);
//		file.WriteFlags("Flags", affected_by, affected_bits);
		
		int count = 0;

		FOREACH(Lexi::List<Affect *>, m_Affects, iter)
		{
			Affect *aff = *iter;
			
			file.BeginSection(Lexi::Format("Affect %d", ++count));
			{
				file.WriteString("Type", aff->GetType());
				file.WriteInteger("Time", aff->GetTime());
				if (aff->GetLocation() && aff->GetModifier())
				{
					if (aff->GetLocation() < 0)	file.WriteString("Location", skill_name(-aff->GetLocation()));
					else						file.WriteEnum("Location", (AffectLoc)aff->GetLocation());
					file.WriteInteger("Modifier", aff->GetModifier());
				}
				file.WriteFlags("Flags", aff->GetFlags());
				file.WriteString("Termination", aff->GetTerminationMessage());
			}
			file.EndSection();
		}
		
	}
	file.EndSection();
	
	file.BeginSection("Score");
	{
		
		file.WriteInteger("PlayerKills", GetPlayer()->m_PlayerKills);
		file.WriteInteger("PlayerDeaths", GetPlayer()->m_PlayerDeaths);
		file.WriteInteger("MobKills", GetPlayer()->m_MobKills);
		file.WriteInteger("MobDeaths", GetPlayer()->m_MobDeaths);
	}
	file.EndSection();
	
	file.BeginSection("Vitals");
	{
		file.WriteInteger("Height", GetHeight());
		file.WriteInteger("Weight", GetWeight());
	}
	file.EndSection();
	
	file.BeginSection("Preferences");
	{
		file.WriteString("Prompt", GetPlayer()->m_Prompt);
		file.WriteFlags("Flags", GetPlayer()->m_Preferences, preference_bits);
		file.WriteInteger("PageLength", GetPlayer()->m_PageLength, DEFAULT_PAGE_LENGTH);
		file.WriteFlags("Channels", GetPlayer()->m_Channels, channel_bits);
		if (PLR_FLAGGED(this, PLR_LOADROOM))	file.WriteVirtualID("Recall", GetPlayer()->m_LoadRoom);
		file.WriteVirtualID("LastRoom", GetPlayer()->m_LastRoom);
		
		//	Aliases
		file.BeginSection("Aliases");
		{
			int count = 0;
			FOREACH(Alias::List, GetPlayer()->m_Aliases, alias)
			{
				file.WriteString(alias->GetCommand(), alias->GetReplacement());
			}
		}
		file.EndSection();
		
		file.BeginSection("Ignores");
		{
			int count = 0;
			FOREACH(std::list<IDNum>, GetPlayer()->m_IgnoredPlayers, id)
			{
				const char *	ignoreName = get_name_by_id(*id);
				if (ignoreName)
					file.WriteString(Lexi::Format("Ignore %d", ++count).c_str(), Lexi::Format("%u %s", *id, ignoreName));
			}
		}
		file.EndSection();
	}
	file.EndSection();
	
	file.BeginSection("Security");
	{
		file.WriteInteger("Karma", GetPlayer()->m_Karma, 0);
		file.WriteInteger("Frozen", GetPlayer()->m_FreezeLevel, 0);
		if (PLR_FLAGGED(this, PLR_NOSHOUT) && GetPlayer()->m_MuteLength)
			file.WriteInteger("Mute", GetPlayer()->m_MuteLength - (time(0) - GetPlayer()->m_MutedAt));
	}
	file.EndSection();
	
	
	file.BeginSection("Staff");
	{
		file.WriteFlags("Permissions", GetPlayer()->m_StaffPrivileges, staff_bits);
		if (IS_STAFF(this))
		{
			file.WriteString("PoofIn", GetPlayer()->m_PoofInMessage);
			file.WriteString("PoofOut", GetPlayer()->m_PoofOutMessage);
			file.WriteInteger("Invisible", GetPlayer()->m_StaffInvisLevel, 0);
		}
	}
	file.EndSection();

	file.BeginSection("IMC");
	{
/*
		file.WriteString("Listen", GetPlayer()->m_IMC.m_Listen);
		sprintbits(GetPlayer()->m_IMC.m_Deaf, buf);
		file.WriteString("Deaf", buf);
		sprintbits(GetPlayer()->m_IMC.m_Allow, buf);
		file.WriteString("Allow", buf);
		sprintbits(GetPlayer()->m_IMC.m_Deny, buf);
		file.WriteString("Deny", buf);
*/
		imc_savechar(this, file);
	}
	file.EndSection();
	
	file.close();
	
	remove(filename.c_str());
	if (rename(tempFilename.c_str(), filename.c_str()))
	{
		mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR: Cannot write player %s to disk: unable to mvoe temporary file '%s' to final file '%s'", GetName(), tempFilename.c_str(), filename.c_str());
	}
	
	//	Re-equip
	for (int i = 0; i < NUM_WEARS; i++)
	{
		if (char_eq[i])
			equip_char(this, char_eq[i], i);
	}

	/* restore our points to previous level */
	GET_HIT(this) = hit;
	GET_MOVE(this) = move;
	//GET_ENDURANCE(this) = endurance;
}


  /* Load a char, TRUE if loaded, FALSE if not */
bool CharData::Load(const char *name)
{
	BUFFER(buf, MAX_INPUT_LENGTH);
	
	if (!GetPlayer() || (GetPlayer() == &dummy_player))
		m_pPlayer = new PlayerData;
	
	if(!find_name(name))		return false;
	
	Lexi::BufferedFile	file(CharData::GetFilename(name).c_str());
	
	if (file.bad())
	{
		mudlogf( NRM, LVL_STAFF, TRUE,  "SYSERR: Couldn't open player file %s", file.GetFilename());
		return false;
	}
	
	if (strn_cmp(file.peek(), "Name:\t", 6))	//	Old files use Name:<space>, new format uses Name:<tab>
	{
		file.close();
		return OldLoad(name);
	}
	
	Lexi::Parser	parser;
	parser.Parse(file);
	
	//	Basic Data
	m_Name								= parser.GetString("Name", name);
	SetID(parser.GetInteger("ID", 0));
	
	GetPlayer()->m_Version				= parser.GetInteger("Version");
	
	strncpy(GetPlayer()->m_Password, parser.GetString("Password", ""), MAX_PWD_LENGTH);
	GetPlayer()->m_Password[MAX_PWD_LENGTH] = '\0';
	
	m_Title								= parser.GetString("Title");
	GetPlayer()->m_Pretitle				= parser.GetString("Pretitle");
	m_Description						= parser.GetString("Description");
	m_Race								= parser.GetEnum("Race", race_types, RACE_HUMAN);
	m_Sex								= parser.GetEnum("Gender", SEX_NEUTRAL);
	m_Level								= parser.GetInteger("Level");

	m_ActFlags							= parser.GetFlags("Flags", player_bits);
	
	//	Login Information
	GetPlayer()->m_Creation				= parser.GetInteger("Login/Creation");
	GetPlayer()->m_TotalTimePlayed		= parser.GetInteger("Login/TimePlayed");
	GetPlayer()->m_LastLogin			= parser.GetInteger("Login/LastLogin");
	
	GetPlayer()->m_Host					= parser.GetString("Login/Host");
	GetPlayer()->m_MaskedHost			= parser.GetString("Login/HostMask");
	if (!GetPlayer()->m_MaskedHost.empty())
	{
		if (this->desc)	this->desc->m_Host = GetPlayer()->m_MaskedHost;
		GetPlayer()->m_Host = GetPlayer()->m_MaskedHost;
	}
	
	GetPlayer()->m_BadPasswordAttempts	= parser.GetInteger("Login/FailedAttempts");
	
	//	Attributes
	this->real_abils.Strength			= parser.GetInteger("Attributes/Strength");
	this->real_abils.Health				= parser.GetInteger("Attributes/Health");
	this->real_abils.Coordination		= parser.GetInteger("Attributes/Coordination");
	this->real_abils.Agility			= parser.GetInteger("Attributes/Agility");
	this->real_abils.Perception			= parser.GetInteger("Attributes/Perception");
	this->real_abils.Knowledge			= parser.GetInteger("Attributes/Knowledge");
	
	this->points.hit					= parser.GetInteger("Points/HP");
	this->points.max_hit				= parser.GetInteger("Points/MaxHP");
	this->points.move					= parser.GetInteger("Points/MV");
	this->points.max_move				= parser.GetInteger("Points/MaxMV");
	//this->points.endurance				= parser.GetInteger("Points/EN");
	//this->points.max_endurance			= parser.GetInteger("Points/MaxEN");
	this->points.mp						= parser.GetInteger("Points/MP");
	this->points.lifemp					= parser.GetInteger("Points/TotalMP");

	GetPlayer()->m_SkillPoints			= parser.GetInteger("Points/SP");
	GetPlayer()->m_LifetimeSkillPoints	= parser.GetInteger("Points/TotalSP");
	
	int numSkills = parser.NumFields("Skills");
	for (int i = 0; i < numSkills; ++i)
	{
		const char *skillname			= parser.GetIndexedField("Skills", i);
		int			skillvalue			= parser.GetIndexedInteger("Skills", i);
		int			skillnum			= find_skill(skillname);
		if (skillnum > 0)
			GET_SKILL(this, skillnum) = GET_REAL_SKILL(this, skillnum) = skillvalue;
	}
	
	Clan *clan = Clan::GetClan(parser.GetInteger("Clan", 0));
	if ( clan )
	{
		Clan::SetMember(this, clan->GetMember(GetID()));
		if (Clan::GetMember(this) || clan->IsApplicant(GetID()))
			GET_CLAN(this) = clan;
	}
	
	//	Game Data
//	m_AffectBits						= parser.GetFlags("Affects/Flags", affected_bits);
	
	int numAffects = parser.NumSections("Affects");
	for (int i = 0; i < numAffects; ++i)
	{
		PARSER_SET_ROOT_INDEXED(parser, "Affects", i);
		
		const char * type				= parser.GetString("Type");
		if (!*type)	continue;
		
		int location = APPLY_NONE;
		if (parser.DoesFieldExist("Location"))
		{
			location						= -find_skill(parser.GetString("Location"));
			if (location == 0)	location	= parser.GetEnum("Location", APPLY_NONE);
		}
		

		int modifier					= parser.GetInteger("Modifier");
		int time						= parser.GetInteger("Time");
		Affect::Flags flags				= parser.GetBitFlags<AffectFlag>("Flags");
		const char * termination		= parser.GetString("Termination");
		
		(new Affect(type, modifier, location, flags, termination))->ToChar(this, time, Affect::AttachPaused);
	}

	//	Score
	GetPlayer()->m_PlayerKills			= parser.GetInteger("Score/PlayerKills");
	GetPlayer()->m_MobKills				= parser.GetInteger("Score/MobKills");
	GetPlayer()->m_PlayerDeaths			= parser.GetInteger("Score/PlayerDeaths");
	GetPlayer()->m_MobDeaths			= parser.GetInteger("Score/MobDeaths");
	
	//	Vitals
	m_Height							= parser.GetInteger("Vitals/Height");
	m_Weight							= parser.GetInteger("Vitals/Weight");

	//	Preferences
	GetPlayer()->m_Prompt				= parser.GetString("Preferences/Prompt");
	GetPlayer()->m_Preferences			= parser.GetFlags("Preferences/Flags", preference_bits);
	GetPlayer()->m_PageLength			= parser.GetInteger("Preferences/PageLength", DEFAULT_PAGE_LENGTH);
	GetPlayer()->m_Channels				= parser.GetFlags("Preferences/Channels", channel_bits);
	GetPlayer()->m_LoadRoom				= parser.GetVirtualID("Preferences/Recall");
	GetPlayer()->m_LastRoom				= parser.GetVirtualID("Preferences/LastRoom");
	
	int numAlias = parser.NumFields("Preferences/Aliases");
	for (int i = 0; i < numAlias; ++i)
	{
		const char *alias				= parser.GetIndexedField("Preferences/Aliases", i);
		const char *replace				= parser.GetIndexedString("Preferences/Aliases", i);
		
		if (*alias && *replace)
			GetPlayer()->m_Aliases.Add(alias, replace);
	}
	
	int numIgnores = parser.NumFields("Preferences/Ignores");
	for (int i = 0; i < numIgnores; ++i)
	{
		IDNum	id = parser.GetIndexedInteger("Preferences/Ignores", i);
		
		if (get_name_by_id(id))
			AddIgnore(id);
	}
	
	//	Security
	GetPlayer()->m_Karma				= parser.GetInteger("Security/Karma");
	GetPlayer()->m_FreezeLevel			= parser.GetInteger("Security/Frozen");
	GetPlayer()->m_MuteLength			= parser.GetInteger("Security/Mute");
	if (GetPlayer()->m_MuteLength > 0)	GetPlayer()->m_MutedAt = time(0);
	
	//	Staff
	GetPlayer()->m_PoofInMessage		= parser.GetString("Staff/PoofIn");
	GetPlayer()->m_PoofOutMessage		= parser.GetString("Staff/PoofOut");
	GetPlayer()->m_StaffPrivileges		= parser.GetFlags("Staff/Permissions", staff_bits);
	GetPlayer()->m_StaffInvisLevel		= parser.GetInteger("Staff/Invisible");
	
	//	IMC
//	GetPlayer()->m_IMC.m_Listen			= parser.GetString("IMC/Listen");
//	GetPlayer()->m_IMC.m_Deaf			= asciiflag_conv(parser.GetString("IMC/Deaf"));
//	GetPlayer()->m_IMC.m_Allow			= asciiflag_conv(parser.GetString("IMC/Allow"));
//	GetPlayer()->m_IMC.m_Deny			= asciiflag_conv(parser.GetString("IMC/Deny"));		
	if (parser.DoesSectionExist("IMC"))
	{
		PARSER_SET_ROOT(parser, "IMC");
		imc_loadchar(this, parser);
	}
	
	// Final data initialization
	this->aff_abils = this->real_abils;	// Copy abil scores
	m_NumCarriedItems = 0;
	m_CarriedWeight = 0;
	
	/* initialization for imms */
#if 0
	if(IS_STAFF(this)) {
		for(i = 1; i <= MAX_SKILLS; i++)
			GET_SKILL(this, i) =  MAX_SKILL_LEVEL;
//		this->general.conditions[0] = -1;
//		this->general.conditions[1] = -1;
//		this->general.conditions[2] = -1;
	}
#endif

	/*
	 * If you're not poisioned and you've been away for more than an hour of
	 * real time, we'll set your HMV back to full
	 */
	
	GetPlayer()->m_LoginTime = time(0);
	if (!AFF_FLAGGED(this, AFF_POISON) && ((GetPlayer()->m_LoginTime - GetPlayer()->m_LastLogin) >= SECS_PER_REAL_HOUR)) {
		GET_HIT(this) = GET_MAX_HIT(this);
		GET_MOVE(this) = GET_MAX_MOVE(this);
		//GET_ENDURANCE(this) = GET_MAX_ENDURANCE(this);
	}
	
	if (m_Name.empty() || (GetLevel() < 1))
		return false;
	
	if (IS_NPC(this))
		m_ActFlags = 0;
	
	return true;
}


Lexi::String CharData::GetFilename(const char *name)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	BUFFER(filename, MAX_STRING_LENGTH);
	
	strcpy(buf, name);
	Lexi::tolower(buf);
	
	sprintf(filename, PLR_PREFIX "%c/%s", *buf, buf);
	
	return filename;
}


Lexi::String CharData::GetObjectFilename(const char *name)
{
	return GetFilename(name) + ".objs";
}
