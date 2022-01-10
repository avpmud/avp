/* ************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <math.h>
#include <vector>

#include "types.h"
#include "structs.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "scripts.h"
#include "handler.h"
#include "find.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "event.h"
#include "affects.h"
#include "track.h"
#include "constants.h"
#include "files.h"

class CombatMessageSet
{
	class Message
	{
	public:
		Lexi::String		attacker;		//	message to attacker
		Lexi::String		victim;			//	message to victim
		Lexi::String		room;			//	message to room
	};
	
public:
	Message				death;		//	messages when death
	Message				miss;		//	messages when miss
	Message				hit;		//	messages when hit
	Message				staff;		//	messages when hit on staff
};

typedef std::vector<CombatMessageSet>		CombatMessageGroup;
typedef std::map<int, CombatMessageGroup>	CombatMessages;

CombatMessages fight_messages;


struct DamMesgType {
	const char *			ToRoom;
	const char *			ToChar;
	const char *			ToVict;
};


/* Structures */
CharacterList	combat_list;


/* External structures */


/* External procedures */
ACMD(do_flee);
ACMD(do_get);
void forget(CharData * ch, CharData * victim);
void remember(CharData * ch, CharData * victim);
int ok_damage_shopkeeper(CharData * ch, CharData * victim);
int combat_delay(CharData *ch, ObjData *weapon, bool ranged = false);
void AlterHit(CharData *ch, int amount);
ObjData *get_combat_weapon(CharData *ch, CharData *victim);

/* local functions */
void check_level(CharData *ch);
void load_messages(void);
void check_killer(CharData * ch, CharData * vict);
void make_corpse(CharData *ch, CharData *killer);
void death_cry(CharData * ch);
void group_gain(CharData * ch, CharData * victim, int attacktype);
char *replace_string(const char *str, const char *weapon_singular, const char *weapon_singular_past, const char *weapon_plural);
void perform_violence(void);
void dam_message(int dam, CharData * ch, CharData * victim, int attack_type, bool ranged);
void multi_dam_message (int damage, CharData *ch, CharData *vict, int attack_type, int times, bool ranged);
void explosion(ObjData *obj, CharData *ch, int dam, int type, int room, int shrapnel);
void acidblood(CharData *ch, int dam);
void acidspray(RoomData *room, int dam);
void announce(const char *buf);
void mob_reaction(CharData *ch, CharData *vict, int dir);
CharData * scan_target(CharData *ch, char *arg, int range, int &dir, int *distance_result, bool isGun);
CharData * scan_target(CharData *ch, CharData *victim, int range, int &dir, int *distance_result, bool isGun);
int fire_missile(CharData *ch, char *arg1, ObjData *missile, int range, int dir);
int fire_missile(CharData *ch, CharData *victim, ObjData *missile, int range, int dir);
int perform_fire_missile(CharData *ch, CharData *victim, ObjData *missile, int distance, int dir);
void spray_missile(CharData *ch, ObjData *missile, int range, int dir, int rounds);
void calc_reward(CharData *ch, CharData *victim, int people, int groupLevel);
bool kill_valid(CharData *ch, CharData *vict);
void ExtractNoKeep(ObjData *obj, ObjData *corpse, CharData *ch);
void explosion_damage(CharData *ch, CharData *vict, int dam, int type);

void attack_otrigger_for_weapon(CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, int hits, int headshots);
void attack_otrigger(CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, int hits, int headshots);
void attack_mtrigger(CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, int hits, int headshots);
void attacked_otrigger(CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, int hits, int headshots);
void attacked_mtrigger(CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, int hits, int headshots);

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
  {"hit", "hit", "hits"},		/* 0 */
  {"sting", "sting", "stings"},
  {"whip", "whip", "whips"},
  {"slash", "slash", "slashes"},
  {"bite", "bite", "bites"},
  {"bludgeon", "bludgeon", "bludgeons"},	/* 5 */
  {"crush", "crush", "crushes"},
  {"pound", "pound", "pounds"},
  {"claw", "claw", "claws"},
  {"maul", "maul", "mauls"},
  {"thrash", "thrash", "thrashes"},	/* 10 */
  {"pierce", "pierce", "pierces"},
  {"blast", "blast", "blasts"},
  {"punch", "punch", "punches"},
  {"stab", "stab", "stabs"},
  {"shoot", "shot", "shoots"},		/* 15 */
  {"burn", "burn", "burns"},
  {"zap", "zap", "zaps"},
  {"melt", "melt", "melts"}
};

//	THIS MUST MATCH ABOVE, 1:1!
char * attack_types[] =
{
  "hit",		/* 0 */
  "sting",
  "whip",
  "slash",
  "bite",
  "bludgeon",	/* 5 */
  "crush",
  "pound",
  "claw",
  "maul",
  "thrash",	/* 10 */
  "pierce",
  "blast",
  "punch",
  "stab",
  "shoot",		/* 15 */
  "burn",
  "zap",
  "melt",
  "\n"
};

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))
#define DEDUCT_AMMO(ch, eq) if (--GET_GUN_AMMO(eq) < 1)\
						send_to_char("*CLICK* Your weapon is out of ammo\n", ch);


void load_messages(void) {
	FILE *fl;
	BUFFER(chk, 128);

	if (!(fl = fopen(MESS_FILE, "r"))) {
		log("Error reading combat message file %s", MESS_FILE);
		exit(1);
	}

	fgets(chk, 128, fl);
	while (!feof(fl) && (*chk == '\n' || *chk == '*'))
		fgets(chk, 128, fl);

	int i = 0;
	while (*chk == 'M') {
		fgets(chk, 128, fl);
		int type = 0;
		sscanf(chk, " %d\n", &type);
		
		CombatMessageSet messages;

		messages.death.attacker	= fread_action(fl, i);
		messages.death.victim	= fread_action(fl, i);
		messages.death.room		= fread_action(fl, i);
		messages.miss.attacker	= fread_action(fl, i);
		messages.miss.victim	= fread_action(fl, i);
		messages.miss.room		= fread_action(fl, i);
		messages.hit.attacker	= fread_action(fl, i);
		messages.hit.victim		= fread_action(fl, i);
		messages.hit.room		= fread_action(fl, i);
		messages.staff.attacker	= fread_action(fl, i);
		messages.staff.victim	= fread_action(fl, i);
		messages.staff.room		= fread_action(fl, i);
		
		fight_messages[type].push_back(messages);
		
		fgets(chk, 128, fl);
		while (!feof(fl) && (*chk == '\n' || *chk == '*'))
			fgets(chk, 128, fl);
		
		++i;
	}
	
	fclose(fl);
}


void check_killer(CharData * ch, CharData * vict) {
//	return;
	
	if (!pk_allowed || (ch == vict) || NO_STAFF_HASSLE(ch))
		return;
	
	if (ch->GetRelation(vict) != RELATION_FRIEND)
		return;
	
	if (!IS_NPC(ch))
	{
		(new Affect("TRAITOR", 0, APPLY_NONE, AFF_TRAITOR))->Join(ch);
		ch->GetPlayer()->m_Karma -= 10;
		send_to_char("If you want to be a TRAITOR, so be it...\n", ch);
	}
}


bool kill_valid(CharData *ch, CharData *vict) {
	if (!ch || !vict)
		return true;	
	if (IS_NPC(ch) || IS_NPC(vict))
		return true;
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOPK) || ROOM_FLAGGED(IN_ROOM(vict), ROOM_NOPK))
		return false;		// DEFAULT for now, deactivate later!
	if (!PRF_FLAGGED(ch, PRF_PK) || !PRF_FLAGGED(vict, PRF_PK))
	{
/*
		int	time_passed;
		
		time_passed = (time(0) - ch->player->time.logon) + ch->player->time.played;
		if (time_passed > (60 * 60 * 2))	//	2 hours
			SET_BIT(PRF_FLAGS(ch), PRF_PK);
		time_passed = (time(0) - vict->player->time.logon) + vict->player->time.played;
		if (time_passed > (60 * 60 * 2))	//	2 hours
			SET_BIT(PRF_FLAGS(vict), PRF_PK);
		
		if (!PRF_FLAGGED(ch, PRF_PK) || !PRF_FLAGGED(vict, PRF_PK))
*/
			return false;
	}
	return true;
}


void start_combat(CharData *ch, CharData *vict, Flags flags)
{
	if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch)))
		initiate_combat(ch, vict, NULL);
	else if (vict == FIGHTING(ch))	//	Already fighting?
	{
#if ROAMING_COMBAT
		//	Attempt to go melee
		CombatEvent *event = (CombatEvent *)Event::FindEvent(ch->m_Events, CombatEvent::Type);
		if (event && event->IsRanged())
		{
			ch->stop_fighting();
			initiate_combat(ch, vict, NULL);
		}
		else
#endif
		if (!IS_SET(flags, StartCombat_NoMessages))
			send_to_char("You do the best you can!\n", ch);
	}
	else if (!IS_SET(flags, StartCombat_NoTargetChange))	//	Target switch!
	{
		bool valid = false;
		
		if (FIGHTING(vict))
		{
			if (FIGHTING(vict) == ch)	valid = true;
			else if (AFF_FLAGGED(FIGHTING(vict), AFF_GROUP) && AFF_FLAGGED(ch, AFF_GROUP))
			{
				CharData *	chmaster = ch->m_Following ? ch->m_Following : ch;
				CharData *	victmaster = FIGHTING(vict)->m_Following ? FIGHTING(vict)->m_Following : FIGHTING(vict);
				
				if (chmaster == victmaster)	valid = true;
			}
		}
		
		if (FIGHTING(ch)
#if ROAMING_COMBAT
			&& (IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
#endif
			&& AFF_FLAGGED(FIGHTING(ch), AFF_GROUPTANK))
		{
			if (!IS_SET(flags, StartCombat_NoMessages))
				act("You cannot disengage $N!", FALSE, ch, 0, FIGHTING(ch), TO_CHAR);
		}
		else if (!valid)
		{
			if (!IS_SET(flags, StartCombat_NoMessages))
				act("You cannot switch targets to $M.", FALSE, ch, 0, vict, TO_CHAR);
		}
		else if (!FIGHTING(ch))
		{
			check_killer(ch, vict);
			set_fighting(ch, vict);
		}
		else
		{
			check_killer(ch, vict);
			act("You change your target to $N.", FALSE, ch, 0, vict, TO_CHAR);
			act("$n changes $s target to you!", FALSE, ch, 0, vict, TO_VICT);
			act("$n changes $s target to $N!", FALSE, ch, 0, vict, TO_NOTVICT);
			FIGHTING(ch) = vict;
		}
	}
}



/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(CharData * ch, CharData * vict) {
	//	Friendly fire checked and stopped here
	if ((ch == vict) || PURGED(ch) /* || ch->GetRelation(vict) == RELATION_FRIEND*/)
		return;

	if (FIGHTING(ch)) {
		core_dump();
		return;
	}

	if (Lexi::IsInContainer(combat_list, ch))
		mudlogf(NRM, 103, TRUE, "SYSERR: %s already in combat list, initiating combat with %s.", ch->GetName(), vict->GetName());
	combat_list.push_front(ch);
	
	if (AFF_FLAGGED(ch, AFF_SLEEP))
		Affect::FromChar(ch, Affect::Sleep);
	

	FIGHTING(ch) = vict;
	if (ch->sitting_on)
		ch->sitting_on = NULL;

	if (GET_POS(ch) > POS_STUNNED)
#if ROAMING_COMBAT
		if (IN_ROOM(ch) == IN_ROOM(vict))
#endif
			GET_POS(ch) = POS_FIGHTING;

	ch->update_pos();

	/*  if (!pk_allowed)*/
	check_killer(ch, vict);
}



/* remove a char from the list of fighting chars */
void CharData::stop_fighting(void) {
	Event *	event;
	combat_list.remove(this);
	FIGHTING(this) = NULL;
	this->update_pos();
	
	event = Event::FindEvent(m_Events, CombatEvent::Type);
	if (event)
	{
		m_Events.remove(event);
		event->Cancel();
	}
}


/* remove a char from the list of fighting chars */
void CharData::redirect_fighting(void)
{
	CharData *oldVict = FIGHTING(this);
	
	this->stop_fighting();
	
	if (InRoom() == NULL)
		return;
	
	FOREACH(CharacterList, InRoom()->people, iter)
	{
		CharData *ch = *iter;
		
		if (ch == this || ch == oldVict)
			continue;
		
		if (FIGHTING(ch) != this)
			continue;
		
		set_fighting(this, ch);
		return;
	}
}

void ExtractNoKeep(ObjData *obj, ObjData *corpse, CharData *ch)
{
	if (OBJ_FLAGGED(obj, ITEM_DEATHPURGE))
	{
		obj->Extract();
		return;
	}
	
	if (NO_LOSE(obj, ch)) {
		if (IS_NPC(ch))		obj->Extract();
		return;
	}

	FOREACH(ObjectList, obj->contents, tmp)
		ExtractNoKeep(*tmp, corpse, ch);
	
	if (IS_NPC(ch) || obj->contents.empty())
	{
		if (obj->WornBy())			obj = unequip_char(ch, obj->WornOn());
		else if (obj->Inside())		obj->FromObject();
		else if (obj->CarriedBy())	obj->FromChar();
		obj->ToObject(corpse);
	}
}


void make_corpse(CharData * ch, CharData *killer)
{
	BUFFER(buf, SMALL_BUFSIZE);
	
	ObjData *corpse = ObjData::Create();
	
	sprintf(buf, "corpse %s", ch->GetName());
	corpse->m_Keywords = buf;
	
	sprintf(buf, "the corpse of %s", ch->GetName());
	corpse->m_Name = buf;
	
	sprintf(buf, "The corpse of %s is lying here.", ch->GetName());
	corpse->m_RoomDescription = buf;
	
	corpse->m_Type			= ITEM_CONTAINER;
	GET_OBJ_WEAR(corpse)	= 0;/*ITEM_WEAR_TAKE*/ 
	GET_OBJ_EXTRA(corpse)	= ITEM_NODONATE;
	corpse->SetValue(OBJVAL_CONTAINER_CORPSE, 1);	/* corpse identifier */
	GET_OBJ_TOTAL_WEIGHT(corpse) = GET_OBJ_WEIGHT(corpse) = ch->GetWeight(); /* + IS_CARRYING_W(ch) */
	GET_OBJ_TIMER(corpse)	= (IS_NPC(ch) ? max_npc_corpse_time : max_pc_corpse_time);
	
	corpse->GetScript()->m_Globals.Add("level", ch->GetLevel())
		.Add("race", findtype(ch->GetRace(), race_types))
		.Add("isplayercorpse", IS_NPC(ch) ? "0" : "1")
		.Add("killer", killer);
	
	/* transfer character's equipment to the corpse */
	for (int i = 0; i < NUM_WEARS; i++)
		if (GET_EQ(ch, i))
			ExtractNoKeep(GET_EQ(ch, i), corpse, ch);
	
	/* transfer character's inventory to the corpse */
/*	corpse->contains = ch->carrying;*/
	FOREACH(ObjectList, ch->carrying, o)
	{
		ExtractNoKeep(*o, corpse, ch);
	}
	
	if (!IS_NPC(ch))
	{
		SavePlayerObjectFile(ch);
	}

	corpse->ToRoom(IN_ROOM(ch));
}


void death_cry(CharData * ch) {
	if (!MOB_FLAGGED(ch, MOB_MECHANICAL))
	{
		act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);
	}
	
	RoomData *was_in = ch->InRoom();

	for (int door = 0; door < NUM_OF_DIRS; door++)
	{
		if (Exit::IsOpen(ch->InRoom(), door))
		{
			Exit::Get(ch,door)->ToRoom()->Send(
				MOB_FLAGGED(ch, MOB_MECHANICAL)
				? "Your hear an explosion come from %s.\n"
				: "Your blood freezes as you hear someone's death cry from %s.\n",
				dir_text_the[rev_dir[door]]);
		}
	}
}


void announce(const char *buf) {
	FOREACH(DescriptorList, descriptor_list, iter)
	{
		DescriptorData *pt = *iter;
		if (pt->GetState()->IsPlaying() && !CHN_FLAGGED(pt->m_Character, Channel::NoInfo))
			pt->m_Character->AddInfoBuf(pt->m_Character->SendAndCopy("`y[`bINFO`y]`n %s\n", buf).c_str());
	}
}



/*unsigned int*/
void calc_reward(CharData *ch, CharData *victim, int people, int groupLevel)
{
	/*int*/ float	value;
	
	if (IS_NPC(ch))	return;	//	Bah
	
	if (!IS_NPC(victim) && !PRF_FLAGGED(ch, PRF_PK))
		return;
	
	bool targetWasTraitor = AFF_FLAGGED(victim, AFF_TRAITOR);
	Affect::FlagsFromChar(victim, AFF_TRAITOR);
	if (ch->GetRelation(victim) == RELATION_FRIEND)
		return;
	
	if (ch->GetFaction() == victim->GetFaction()
		&& !GET_TEAM(ch) && !GET_TEAM(victim))
		return;
	
	int playerRank, victimRank;
	
	playerRank = groupLevel / 10;
	victimRank = victim->GetLevel() / 10;
		
	if (IS_NPC(victim))
	{
		value = victim->points.lifemp;
		
		//	Below:	This is the new system that will go in soon for balancing against lower level mobs
		victimRank = victim->GetLevel() / 10;
	}
	else
	{
		value = ((victim->GetLevel() + 1) / 2.0f);
		
		if (PLR_FLAGGED(victim, PLR_PKONLY) && !PLR_FLAGGED(ch, PLR_PKONLY))
			value += 10;
//		value += value /* * capture_bonus(ch->GetRace())*/;
	}
	
	if (IS_NPC(victim) && !AFF_FLAGGED(ch, AFF_TRAITOR))
	{
		int rankDiff = playerRank - victimRank;

		if (rankDiff > 1)
		{
			//	Reduce MP by half for every additional group rank above the victim
			
			ch->Send("The MP is reduced, because the victim is %d ranks below your %srank.\n",
					(playerRank - victimRank),
					people > 1 ? "group's " : "");
			
			while (rankDiff > 1 && value > 0.0f)
			{
				value /= 2.0f;
				--rankDiff;
			}
		}
		
		if (AFF_FLAGGED(ch, AFF_GROUP) && victim->GetLevel() != 100)
		{
			playerRank = ch->GetLevel() / 10;
			
			rankDiff = victimRank - playerRank;
			
			if (rankDiff > 3)
			{
				//	Reduce MP by half for every victim rank above the player past the third rank of difference
				ch->Send("The MP is reduced, because the victim is %d ranks above you.\n", rankDiff);
				
				while (rankDiff > 3 && value > 0.0f)
				{
					value /= 2.0f;
					--rankDiff;
				}
			}
		}
	}
	
	if (people > 1)
		value = (value * 2.0f) / (float)people;
	
	int nValue = (int)nearbyintf(value);
	
	if (AFF_FLAGGED(ch, AFF_TRAITOR) && !NO_STAFF_HASSLE(ch))
	{
		AFF_FLAGS(ch).clear(AFF_TRAITOR);	//	Temporarily clear flag - restored below!
		if (!IS_NPC(victim) && ch->GetRelation(victim) == RELATION_FRIEND)
		{
			nValue = ch->GetLevel() * 2;
			
			ch->Send("You `RLOSE`n %d Mission Points for your traitorous act.\n", nValue);
			ch->points.mp -= nValue;
			if (ch->points.mp < -5000)
				ch->points.mp = -5000;
		}
		else
			ch->Send("You do not earn MP for kills while you are a traitor.\n");
		AFF_FLAGS(ch).set(AFF_TRAITOR);		//	Restore temporarily cleared flag
	}
	else if (ch->GetPlayer()->m_SkillPoints < 0)
		ch->Send("`RWhile your Skill Points are below 0, you will not earn Mission Points.  You may unpractice for free until your Skill Points are at or above 0.\n");
	else
	{
		ch->Send("You receive %d Mission Points.\n", nValue);
		ch->points.mp += nValue;
		ch->points.lifemp += nValue;
	}
	SET_BIT(PLR_FLAGS(ch), PLR_CRASH);
	check_level(ch);
}


void group_gain(CharData * ch, CharData * victim, int attacktype) {
	CharData *leader;
	int followerCount = 1;
	int groupLevel;
	
	leader = (ch->m_Following) ? ch->m_Following : ch;
	
	groupLevel = leader->GetLevel();

	FOREACH(CharacterList, leader->m_Followers, iter)
	{
		CharData *follower = *iter;

		if (AFF_FLAGGED(follower, AFF_GROUP) && IsSameEffectiveZone(ch->InRoom(), follower->InRoom()))
		{
			followerCount++;
			if (follower->GetLevel() > groupLevel)
				groupLevel = follower->GetLevel();
		}
	}
	
	if ((leader == ch
			|| (IS_NPC(victim) ?
				  leader->InRoom() == ch->InRoom()
				: IsSameEffectiveZone(ch->InRoom(), leader->InRoom())))
		&& !ROOM_FLAGGED(leader->InRoom(), ROOM_PEACEFUL)
		/*&& (IN_ROOM(ch) == IN_ROOM(leader)
			|| attacktype >= TYPE_SUFFERING
			|| AFF_FLAGGED(leader, AFF_NOQUIT))*/)
	{
		calc_reward(leader, victim, followerCount, groupLevel);
	}

	FOREACH(CharacterList, leader->m_Followers, iter)
	{
		CharData *follower = *iter;

		if (AFF_FLAGGED(follower, AFF_GROUP)
				&& (follower == ch
					|| (IS_NPC(victim) ?
						  IN_ROOM(follower) == IN_ROOM(ch)
						: IsSameEffectiveZone(ch->InRoom(), follower->InRoom())))
				&& !ROOM_FLAGGED(follower->InRoom(), ROOM_PEACEFUL)
				/*&& (IN_ROOM(ch) == IN_ROOM(follower)
					|| attacktype >= TYPE_SUFFERING
					|| AFF_FLAGGED(follower, AFF_NOQUIT))*/)
		{
			calc_reward(follower, victim, followerCount, groupLevel);
		}
	}
}


//#define LEVEL_COST(level)  (level * level * level * .75)
//#define LEVEL_COST(level)  (level * level * level * .5)
//#define PK_LEVEL_COST(level)  ((level * level * 3.125) - 1200)

int level_cost(int level)
{
/*	int cost;
	
	if (level > 80)			cost = 150000 + ((level - 80) * 4000);
	else if (level > 60)	cost = 90000 + ((level - 60) * 3000);
	else if (level > 40)	cost = 50000 + ((level - 40) * 2000);
	else if (level > 20)	cost = 20000 + ((level - 20) * 1500);
	else					cost = (level) * 1000;
	
	return cost;
*/
	return level * level /* * 10*/;	
/*
	10
	40
	90
	160
	250
	360
	490
	640
	810
	1000
	
	20 = 4000
	30 = 9000
	40 = 16000
	50 = 25000
	60 = 36000
	70 = 49000
	80 = 64000
	90 = 81000
	100 = 100000
*/	
}


void check_level(CharData *ch) {
	return;	//	No automatic levelling
	
	if (IS_NPC(ch))
		return;
	
	int level = ch->GetLevel();
	int maxlevel = 100; //((((ch->GetLevel() - 1) / 20) + 1) * 20);
	
//	maxlevel = MIN(maxlevel, 100);
	
	while (level < maxlevel && (int)ch->points.lifemp >= level_cost(level) /* &&
			(int)ch->player->special.PKills >= (int)PK_LEVEL_COST(level)*/)
		level++;
	
	if (level > ch->GetLevel()) {
		ch->SetLevel(level);
	}
}


char *replace_string(const char *str, const char *weapon_singular, const char *weapon_singular_past, const char *weapon_plural)
{
	static char buf[MAX_INPUT_LENGTH];
	char *cp;

	cp = buf;

	for (; *str; str++) {
		if (*str == '#') {
			switch (*(++str)) {
				case 'W':
					for (; *weapon_plural; *(cp++) = *(weapon_plural++))
						;
					break;
				case 'w':
					for (; *weapon_singular; *(cp++) = *(weapon_singular++))
						;
					break;
				case 'p':
					for (; *weapon_singular_past; *(cp++) = *(weapon_singular_past++))
						;
					break;
				default:
					*(cp++) = '#';
					break;
			}
		} else
			*(cp++) = *str;

		*cp = 0;
	}

	return (buf);
}


static DamMesgType dam_weapon[] = {
   // use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes")
   // use #p for past tense singular
   { "$n tries to #w $N",					//	0: 0
     "You try to #w $N",
     "$n tries to #w you"},
   { "$n grazes $N with $s #p",				//	1: 1-10
     "You graze $N with your #p",
     "$n grazes you with $s #p" },
   { "$n barely #W $N",						//	2: 11-25
     "You barely #w $N",
     "$n barely #W you" },
/*   { "$n lightly #W $N",					//	2: 26-50
     "You lightly #W $N",
     "$n lightly #W you" },*/
   { "$n #W $N",							//	3: 26-50
     "You #w $N",
     "$n #W you" },
   { "$n #W $N hard",						//	4: 51-75
     "You #w $N hard",
     "$n #W you hard" },
   { "$n #W $N rather hard",				//	5: 76-125
     "You #w $N rather hard",
     "$n #W you rather hard" },
   { "$n #W $N very hard",					//	6: 126-250
     "You #w $N very hard",
     "$n #W you very hard" },
   { "$n #W $N extremely hard",				//	7: 251-500
     "You #w $N extremely hard",
     "$n #W you extremely hard" },
   { "$n massacres $N with $s #p",		//	8: 501-999	/*	Was: #wing */
     "You massacre $N with your #p",
     "$n massacres you with $s #p" },
   { "$n annihilates $N with $s #p",		//	9: > 1000	/* Was: #wing */
     "You annihilate $N with your #p",
     "$n annihilates you with $s #p" }
};


static DamMesgType dam_amount[] = {
   { "$E hardly notices.",                   // 0: 0%-1%
     "$E hardly notices.",
     "you hardly notice." },
   { "barely hurts $M.",                     // 1: 2%
     "barely hurts $M.",
     "barely hurts you." },
   { "slightly wounds $M.",                  // 2: 3%-5%
     "slightly wounds $M.",
     "slightly wounds you." },
   { "wounds $M.",                           // 3: 6%-10%
     "wounds $M.",
     "wounds you." },
   { "severely wounds $M.",                  // 4: 11%-20%
     "severely wounds $M.",
     "severely wounds you." },
   { "maims $M.",                            // 5: 21%-30%
     "maims $M.",
     "maims you." },
   { "seriously maims $M!",                  // 6: 31%-45%
     "seriously maims $M!",
     "seriously maims you!" },
   { "cripples $M!",                         // 7: 46%-65%
     "cripples $M!",
     "cripples you!" },
   { "completely cripples $M!",              // 8: 66%-90%
     "completely cripples $M!",
     "completely cripples you!" },
   { "totally ANNIHILATES $M!!",             // 9: > 90%
     "totally ANNIHILATES $M!!",
     "totally ANNIHILATES you!!" }
};


// message for doing damage with a weapon
void dam_message (int damage, CharData *ch, CharData *vict, int attack_type, bool ranged) {
	int amount_index, weapon_index, percent;

	if (attack_type < TYPE_HIT)
		attack_type = TYPE_HIT;

	attack_type -= TYPE_HIT;          // Change to base of table with text

	percent = (damage * 100 / GET_MAX_HIT (vict));

	if		(percent <= 1)		amount_index = 0;
	else if	(percent <= 2)		amount_index = 1;
	else if	(percent <= 5)		amount_index = 2;
	else if	(percent <= 10)		amount_index = 3;
	else if	(percent <= 20)		amount_index = 4;
	else if	(percent <= 30)		amount_index = 5;
	else if	(percent <= 45)		amount_index = 6;
	else if	(percent <= 65)		amount_index = 7;
	else if	(percent <= 90)		amount_index = 8;
	else						amount_index = 9;

	if		(damage <= 0)		weapon_index = 0;
	else if	(damage <= 75)		weapon_index = 1;
	else if	(damage <= 150)		weapon_index = 2;
	else if	(damage <= 225)		weapon_index = 3;
	else if	(damage <= 300)		weapon_index = 4;
	else if	(damage <= 375)		weapon_index = 5;
	else if	(damage <= 450)		weapon_index = 6;
	else if	(damage <= 525)		weapon_index = 7;
	else if	(damage <  600)		weapon_index = 8;
	else						weapon_index = 9;

	BUFFER(message, MAX_STRING_LENGTH);

	// message to damager
	const char *combatString = replace_string (dam_weapon[weapon_index].ToChar,
			attack_hit_text[attack_type].singular,
			attack_hit_text[attack_type].singular_past,
			attack_hit_text[attack_type].plural);
	
	if (weapon_index > 0)	sprintf(message, "(1 hit) %s, which %s", combatString, dam_amount[amount_index].ToChar);
	else					sprintf(message, "(1 miss) %s, but misses.", combatString);

	act (message, FALSE, ch, NULL, vict, TO_CHAR);

	if (!ranged)
	{
		// message to onlookers
		
		combatString = replace_string (dam_weapon[weapon_index].ToRoom,
			attack_hit_text[attack_type].singular,
			attack_hit_text[attack_type].singular_past,
			attack_hit_text[attack_type].plural);
	
		if (weapon_index > 0)	sprintf(message, "(1 hit) %s, which %s", combatString, dam_amount[amount_index].ToRoom);
		else					sprintf(message, "(1 miss) %s, but misses.", combatString);

		act (message, FALSE, ch, NULL, vict, TO_NOTVICT);

		// message to damagee
		combatString = replace_string (dam_weapon[weapon_index].ToVict,
			attack_hit_text[attack_type].singular,
			attack_hit_text[attack_type].singular_past,
			attack_hit_text[attack_type].plural);
	
		if (weapon_index > 0)	sprintf(message, "(1 hit) %s, which %s", combatString, dam_amount[amount_index].ToVict);
		else					sprintf(message, "(1 miss) %s, but misses.", combatString);

		act (message, FALSE, ch, NULL, vict, TO_VICT | TO_SLEEP);
	}
}


// message for doing damage with a weapon
void multi_dam_message (int damage, CharData *ch, CharData *vict, int attack_type, int times, bool ranged) {
	int amount_index, weapon_index, percent;

	if (attack_type < TYPE_HIT)
		attack_type = TYPE_HIT;
	
	attack_type -= TYPE_HIT;          // Change to base of table with text


	percent = (damage * 100 / GET_MAX_HIT (vict));

	if		(percent <= 1)		amount_index = 0;
	else if	(percent <= 2)		amount_index = 1;
	else if	(percent <= 5)		amount_index = 2;
	else if	(percent <= 10)		amount_index = 3;
	else if	(percent <= 20)		amount_index = 4;
	else if	(percent <= 30)		amount_index = 5;
	else if	(percent <= 45)		amount_index = 6;
	else if	(percent <= 60)		amount_index = 7;
	else if	(percent <= 80)		amount_index = 8;
	else						amount_index = 9;

	if		(damage <= 0)		weapon_index = 0;
	else if	(damage <= 10)		weapon_index = 1;
	else if	(damage <= 25)		weapon_index = 2;
	else if	(damage <= 50)		weapon_index = 3;
	else if	(damage <= 75)		weapon_index = 4;
	else if	(damage <= 125)		weapon_index = 5;
	else if	(damage <= 250)		weapon_index = 6;
	else if	(damage <= 500)		weapon_index = 7;
	else if	(damage < 1000)		weapon_index = 8;
	else						weapon_index = 9;

	BUFFER(message, MAX_STRING_LENGTH);

	// message to damager
	const char *combatString = replace_string (dam_weapon[weapon_index].ToChar,
			attack_hit_text[attack_type].singular,
			attack_hit_text[attack_type].singular_past,
			attack_hit_text[attack_type].plural);
	
	if (weapon_index > 0)	sprintf(message, "(%d hits) %s %s, which %s", times, combatString, times_string[times], dam_amount[amount_index].ToChar);
	else					sprintf(message, "(%d misses) %s %s, but misses.", times, combatString, times_string[times]);

	act (message, FALSE, ch, NULL, vict, TO_CHAR);

	if (!ranged)
	{
		// message to onlookers
		combatString = replace_string (dam_weapon[weapon_index].ToRoom,
			attack_hit_text[attack_type].singular,
			attack_hit_text[attack_type].singular_past,
			attack_hit_text[attack_type].plural);
	
		if (weapon_index > 0)	sprintf(message, "(%d hits) %s %s, which %s", times, combatString, times_string[times], dam_amount[amount_index].ToRoom);
		else					sprintf(message, "(%d misses) %s %s, but misses.", times, combatString, times_string[times]);

		act (message, FALSE, ch, NULL, vict, TO_NOTVICT);

		// message to damagee
		combatString = replace_string (dam_weapon[weapon_index].ToVict,
			attack_hit_text[attack_type].singular,
			attack_hit_text[attack_type].singular_past,
			attack_hit_text[attack_type].plural);
	
		if (weapon_index > 0)	sprintf(message, "(%d hits) %s %s, which %s", times, combatString, times_string[times], dam_amount[amount_index].ToVict);
		else					sprintf(message, "(%d misses) %s %s, but misses.", times, combatString, times_string[times]);

		act (message, FALSE, ch, NULL, vict, TO_VICT | TO_SLEEP);
	}
}


/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int skill_message(int dam, CharData * ch, CharData * vict, ObjData *weapon, int attacktype, bool ranged)
{
	CombatMessages::iterator iter = fight_messages.find(attacktype);
	
	if (iter == fight_messages.end())	return 0;
	
	CombatMessageGroup &group = iter->second;
	
	if (group.empty())
		return 0;
	
	int nr = Number(1, group.size()) - 1;
	CombatMessageSet &msg = group[nr];

	if (NO_STAFF_HASSLE(vict)) {
		act(msg.staff.attacker.c_str(), FALSE, ch, weapon, vict, TO_CHAR);
		if (!ranged)
		{
			act(msg.staff.victim.c_str(), FALSE, ch, weapon, vict, TO_VICT);
			act(msg.staff.room.c_str(), FALSE, ch, weapon, vict, TO_NOTVICT);
		}
	} else if (dam != 0) {
		if (GET_POS(vict) == POS_DEAD) {
			send_to_char("`y", ch);
			act(msg.death.attacker.c_str(), FALSE, ch, weapon, vict, TO_CHAR);
			send_to_char("`n", ch);

			if (!ranged)
			{
				send_to_char("`r", vict);
				act(msg.death.victim.c_str(), FALSE, ch, weapon, vict, TO_VICT | TO_SLEEP);
				send_to_char("`n", vict);

				act(msg.death.room.c_str(), FALSE, ch, weapon, vict, TO_NOTVICT);
			}
		} else {
			send_to_char("`y", ch);
			act(msg.hit.attacker.c_str(), FALSE, ch, weapon, vict, TO_CHAR);
			send_to_char("`n", ch);

			if (!ranged)
			{
				send_to_char("`r", vict);
				act(msg.hit.victim.c_str(), FALSE, ch, weapon, vict, TO_VICT | TO_SLEEP);
				send_to_char("`n", vict);

				act(msg.hit.room.c_str(), FALSE, ch, weapon, vict, TO_NOTVICT);
			}
		}
	} else if (ch != vict) {	/* Dam == 0 */
		send_to_char("`y", ch);
		act(msg.miss.attacker.c_str(), FALSE, ch, weapon, vict, TO_CHAR);
		send_to_char("`n", ch);

		if (!ranged)
		{
			send_to_char("`r", vict);
			act(msg.miss.victim.c_str(), FALSE, ch, weapon, vict, TO_VICT | TO_SLEEP);
			send_to_char("`n", vict);

			act(msg.miss.room.c_str(), FALSE, ch, weapon, vict, TO_NOTVICT);
		}
	}
	return 1;
}

/*
 *   -2 Victim not dead, don't repeat
 *	 -1	Victim died.
 *	= 0	No damage.
 *	> 0	How much damage done.
 */
int damage(CharData * ch, CharData * victim, ObjData *weapon, int dam, int attacktype, int damagetype, int times) {
	RoomData *room;
	int missile = FALSE;
	int preHitHP;
//	int count;
//	ObjData *weapon;

/*	if (times < 0 || times > 12) {
		log("SYSERR: times too %s in damage().  (times = %d)", times < 0 ? "low" : "great", times);
		return 0;
	}
*/
	//	No damage for the dying
	if (AFF_FLAGGED(victim, AFF_DYING) || AFF_FLAGGED(victim, AFF_RESPAWNING))
		return -1;
	
	if (GET_POS(victim) <= POS_DEAD) {
//		log("SYSERR: Attempt to damage a corpse '%s' in room %s by '%s'", victim->GetName(),
//				victim->InRoom()->GetVirtualID().Print().c_str(), ch ? ch->GetName() : "<nobody>");
		victim->AbortTimer();
		victim->die(NULL);
		return -1;
	}
	
	times = RANGE(times, 1, 12);
	
	//	Friendly Fire disable
//	if (ch && (ch != victim) && (ch->GetRelation(victim) == RELATION_FRIEND))
//		return 1;	//	No hurting friendlies!
	
	if (ch) {
		//	Non-PK-flagged players going at it...
		if (!kill_valid(ch, victim)) {
			if (FIGHTING(ch) == victim)
				ch->stop_fighting();
			return -2;
		}
		
		if (IN_ROOM(ch) != IN_ROOM(victim))
			missile = TRUE;
		
		/* peaceful rooms */
		if ((ch != victim) && ROOM_FLAGGED(IN_ROOM(victim), ROOM_PEACEFUL) &&
				(attacktype < TYPE_SUFFERING) && !NO_STAFF_HASSLE(ch)) {
			send_to_char("This room just has such a peaceful, easy feeling...\n", ch);
			if (FIGHTING(ch) && (FIGHTING(ch) == victim))
				ch->stop_fighting();
			return -2;
		}
		
		if (PLR_FLAGGED(ch, PLR_PKONLY) && IS_NPC(victim))
		{
			send_to_char("Your efforts are futile.  You can only harm other players!\n", ch);
			if (FIGHTING(ch) && (FIGHTING(ch) == victim))
				ch->stop_fighting();
			return -2;
		}
		
	  /* shopkeeper protection */
		if (!ok_damage_shopkeeper(ch, victim))
			return -2;
		
	/* Daniel Houghton's missile modification */
		if (victim != ch && !missile) {
			/* Start the attacker fighting the victim */
			if (GET_POS(ch) > POS_STUNNED && !FIGHTING(ch) && 
					(IS_NPC(ch) || attacktype < TYPE_SUFFERING))
				set_fighting(ch, victim);
				
			/* Start the victim fighting the attacker */
			if (/*GET_POS(victim) > POS_STUNNED &&*/ attacktype < TYPE_SUFFERING) {
				bool	startFighting = false;
				
				if (!FIGHTING(victim))
				{
					set_fighting(victim, ch);

					if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
						remember(victim, ch);
				}
				else
				{
					CombatEvent *event = (CombatEvent *)Event::FindEvent(victim->m_Events, CombatEvent::Type);
					
					if (!event)
					{
						victim->stop_fighting();
						set_fighting(victim, ch);
					}
					else if (event->IsRanged())
					{
						event->BecomeNonRanged();

						//	Switch their target... just incase, even if this is already the target
						FIGHTING(victim) = ch;
						victim->update_pos();
					}
				}
				//	else they have a target they are actively fighting in this room
			}
		}
		/* If you attack a pet, it hates your guts */
		if (victim->m_Following == ch)
			victim->stop_follower();

		/* If the attacker is invisible, he becomes visible */
		Affect::Flags MAKE_BITSET(hideFlags, AFF_HIDE);
		if (!missile)	hideFlags.set(AFF_COVER);
		if (attacktype < TYPE_SUFFERING && AFF_FLAGS(ch).testForAny(hideFlags))
			ch->Appear(hideFlags);
	
		/* peaceful rooms, with causer */
		if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_PEACEFUL) && !NO_STAFF_HASSLE(ch) && attacktype != TYPE_SCRIPT)
			return -2;
	}
	
	//	Damage is going to occur, may be 0 however.  Stop what you are doing!
	if (!victim->AbortTimer() && PURGED(victim))
		return -1;
	
	if (NO_STAFF_HASSLE(victim))
		dam = 0;
	
	if (dam < 0)
		dam = 0;

	//	PvP Damage reduction	
//	if (!IS_NPC(victim) && ch && !IS_NPC(ch))
//		dam = dam * 2 / 3;
	
	preHitHP = GET_HIT(victim);	//	acid chance
	AlterHit(victim, -dam);	
//	GET_HIT(victim) -= dam;

	if (dam > 0 && AFF_FLAGS(victim).testForAny(Affect::AFF_INVISIBLE_FLAGS) && (GET_HIT(victim) < (GET_MAX_HIT(victim) * 1 / 3)))
		victim->Appear(Affect::AFF_INVISIBLE_FLAGS);

/*	if (ch && ch != victim)
		gain_exp(ch, victim->GetLevel() * dam);*/
	
//	victim->update_pos();	//	AlterHit does it

  /*
   * skill_message sends a message from the messages file in lib/misc.
   * dam_message just sends a generic "You hit $n extremely hard.".
   * skill_message is preferable to dam_message because it is more
   * descriptive.
   * 
   * If we are _not_ attacking with a weapon (i.e. a spell), always use
   * skill_message. If we are attacking with a weapon: If this is a miss or a
   * death blow, send a skill_message if one exists; if not, default to a
   * dam_message. Otherwise, always send a dam_message.
   */
    if (!ch || (attacktype >= TYPE_SUFFERING))
	{ } 	/* Don't say a THING!  It's being handled. */
	else if (!IS_WEAPON(attacktype) && skill_message(dam, ch, victim, weapon, attacktype, false))
	{ }
	else if ((GET_POS(victim) != POS_DEAD && dam > 0) || !skill_message(dam, ch, victim, weapon, attacktype, missile))
	{
		if (times > 1)
			multi_dam_message(dam, ch, victim, attacktype, times, missile);
		else
			dam_message(dam, ch, victim, attacktype, missile);
	}
	
  /* Use send_to_char -- act() doesn't send message if you are DEAD. */
	switch (GET_POS(victim)) {
		case POS_MORTALLYW:
//			if (MOB_FLAGGED(victim, MOB_MECHANICAL))
//				act("$n is destroyed and malfunctioning.", TRUE, victim, 0, 0, TO_ROOM);
//			else
				act("$n is mortally wounded, and will die soon, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
			send_to_char("You are mortally wounded, and will die soon, if not aided.\n", victim);
			break;
		case POS_INCAP:
//			if (MOB_FLAGGED(victim, MOB_MECHANICAL))
//				act("$n is disabled and malfunctioning.", TRUE, victim, 0, 0, TO_ROOM);
//			else
				act("$n is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
			send_to_char("You are incapacitated an will slowly die, if not aided.\n", victim);
			break;
		case POS_STUNNED:
			if (MOB_FLAGGED(victim, MOB_MECHANICAL))
				act("$n is malfunctioning, but will probably regain functionality again.", TRUE, victim, 0, 0, TO_ROOM);
			else
				act("$n is stunned, but will probably regain consciousness again.", TRUE, victim, 0, 0, TO_ROOM);
			send_to_char("You're stunned, but will probably regain consciousness again.\n", victim);
			break;
		case POS_DEAD:
			if (MOB_FLAGGED(victim, MOB_MECHANICAL))
				act("$n explodes!", FALSE, victim, 0, 0, TO_ROOM);
			else
				act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
			send_to_char("You are dead!  Sorry...\n", victim);
			break;

		default:			/* >= POSITION SLEEPING */
			if (dam > (GET_MAX_HIT(victim) / 4))
				act("That really did HURT!", FALSE, victim, 0, 0, TO_CHAR);

			if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 4)) {
				send_to_char("`RYou wish that your wounds would stop BLEEDING so much!`n\n", victim);
				if (ch && MOB_FLAGGED(victim, MOB_WIMPY) &&
						!MOB_FLAGGED(victim, MOB_SENTINEL) && (ch != victim))
					do_flee(victim, "", "flee", 0);
			}
/*			if (ch && !IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&
					GET_HIT(victim) < GET_WIMP_LEV(victim)) {
				send_to_char("You wimp out, and attempt to flee!\n", victim);
				do_flee(victim, "", "flee", 0);
			}
*/			break;
	}
	
	if (((dam >= GET_MAX_HIT(victim) / 6) || (dam > preHitHP)) &&
			(MOB_FLAGGED(victim, MOB_ACIDBLOOD) || (!IS_NPC(victim) && IS_ALIEN(victim))) &&
			//IS_WEAPON(attacktype)  &&
			attacktype != TYPE_SUFFERING &&
			damagetype != DAMAGE_UNKNOWN &&
			(damagetype != DAMAGE_ENERGY || Number(0,3) == 0) &&
			(damagetype != DAMAGE_FIRE || Number(0,50) == 0))
		acidblood(victim, damagetype != DAMAGE_ENERGY ? dam : dam * 3);

//	if (ch && !IS_NPC(victim) && !(victim->desc)) {
//		do_flee(victim, "", "flee", 0);
//	}

	/* stop someone from fighting if they're stunned or worse */
//	if ((GET_POS(victim) <= POS_STUNNED) && (FIGHTING(victim) != NULL))
//		victim->stop_fighting();
	
	/* Uh oh.  Victim died. */
	if (GET_POS(victim) == POS_DEAD)
	{
		if (ch && GET_LAST_ATTACKER(ch) == GET_ID(victim))
		{
			GET_LAST_ATTACKER(ch) = 0;
		}
		
		if ((!ch || ch == victim || IS_NPC(ch)) && GET_LAST_ATTACKER(victim)
			 && AFF_FLAGGED(victim, AFF_NOQUIT))
		{
	 		CharData *lastDamager = CharData::Find(GET_LAST_ATTACKER(victim));
	 		if (lastDamager)	ch = lastDamager;
	 		
			if (lastDamager && GET_LAST_ATTACKER(lastDamager) == GET_ID(victim))
			{
				GET_LAST_ATTACKER(lastDamager) = 0;
			}
		}
		
		if (ch && ch != victim && !PURGED(ch)) {
//			int levelDiff = (ch->GetLevel() + 3) / 4;	//	Only rewraded for killing people within 1/4 of your level
			
//			if (levelDiff < 10)	levelDiff = 10;	//	With a minimum of 10 levels difference
			
//			if (IS_NPC(victim) || (victim->GetLevel() >= (ch->GetLevel() - levelDiff)))
			/*if (IS_NPC(victim) || GET_FACTION(ch) != GET_FACTION(victim))*/
			if (!IS_NPC(victim) || attacktype != TYPE_EXPLOSION)
			{
				if (AFF_FLAGGED(ch, AFF_GROUP))
					group_gain(ch, victim, attacktype);
				else // if (attacktype >= TYPE_SUFFERING || AFF_FLAGGED(ch, AFF_NOQUIT))
					// if (!IS_NPC(ch) && /*!IS_TRAITOR(ch) &&*/ (ch->GetRelation(victim) == RELATION_ENEMY))
					calc_reward(ch, victim, 1, ch->GetLevel());
			}
			if (MOB_FLAGGED(ch, MOB_MEMORY))
				forget(ch, victim);
		}
		room = victim->InRoom();
		victim->AbortTimer();
		victim->die(ch);
		return -1;
	}
	else if (ch && ch != victim && !IS_NPC(ch) && GET_HIT(victim) < (int)(GET_MAX_HIT(victim) * .4) /*&& GET_POS(victim) < POS_STUNNED*/)
	{
		GET_LAST_ATTACKER(victim) = GET_ID(ch);
	}
	
	if (ch && !PURGED(ch) && IS_NPC(victim) && !MOB_FLAGGED(victim, MOB_WIMPY) &&
			!FIGHTING(victim) &&
			attacktype > TYPE_AGGRAVATES &&
			victim->GetRelation(ch) != RELATION_FRIEND)
	{
		if (!victim->path)	//	30 rooms to limit notice range
			victim->path = Path2Char(victim, GET_ID(ch), 30, HUNT_GLOBAL);
		if (victim->path)
			remember(victim, ch);
	}
	
	return dam;
}







#if 0

#define B	ARMOR_LOC_BODY
#define H	ARMOR_LOC_HEAD
#define AL	ARMOR_LOC_ARMS
#define AR	ARMOR_LOC_ARMS
#define HL	ARMOR_LOC_HANDS
#define HR	ARMOR_LOC_HANDS
#define LL	ARMOR_LOC_LEGS
#define LR	ARMOR_LOC_LEGS
#define FL	ARMOR_LOC_FEET
#define FR	ARMOR_LOC_FEET

#define UNUSED_5	-1,-1,-1,-1,-1
#define UNUSED_10	UNUSED_5, UNUSED_5
#define UNUSED_20	UNUSED_10, UNUSED_10

	//	[loc][angle 0-9][step 0-47]
const int hitchart[NUM_ARMOR_LOCS][10][36] =
{
/* Aiming at the body */
		/*	Body	Head	Arms	Hands	Legs	Feet	-1,-1	*/
	{
/* 0 */		{ B, B, B, B, B, B, B, B, H, H, H, H, H, H, H, H, H, H, UNUSED_10, UNUSED_5, -1, -1, -1 },
/* 10 */	{ B, B, B, B, B, B, B, B, AL, AL, UNUSED_20, UNUSED_5, -1 },
/* 20 */	{ B, B, B, B, B, B, B, AL, AL, AL, AL, AL, UNUSED_20, -1, -1, -1, -1 },
/* 30 */	{ B, B, B, B, B, B, B, B, -1, -1, -1, -1, AL, AL, AL, AL, AL, AL, AL, UNUSED_10, UNUSED_5, -1, -1 },
/* 40 */	{ B, B, B, B, B, B, B, B, B, B, B, B, B, B, LL, LL, LL, LL, LL, UNUSED_10, UNUSED_5, -1, -1 },
/* 50 */	{ B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, UNUSED_10, UNUSED_5, -1, -1, -1, -1 },
/* 60 */	{ B, B, B, B, B, B, B, B, B, B, B, B, B, B, LR, LR, LR, LR, LR, UNUSED_10, UNUSED_5, -1, -1 },
/* 70 */	{ B, B, B, B, B, B, B, B, -1, -1, -1, -1, AR, AR, AR, AR, AR, AR, AR, UNUSED_10, UNUSED_5, -1, -1 },
/* 80 */	{ B, B, B, B, B, B, B, AR, AR, AR, AR, AR, UNUSED_20, -1, -1, -1, -1 },
/* 90 */	{ B, B, B, B, B, B, B, B, AR, AR, UNUSED_20, UNUSED_5, -1 }
	},
	{
/* 0 */		{ H, H, H, H, UNUSED_20, UNUSED_10, -1, -1 },
/* 10 */	{ H, H, H, H, UNUSED_20, UNUSED_10, -1, -1 },
/* 20 */	{ H, H, H, H, H, UNUSED_20, UNUSED_10, -1 },
/* 30 */	{ H, H, H, H, H, H, -1, -1, -1, -1, -1, AL, AL, AL, AL, AL, AL, UNUSED_10, UNUSED_5, -1, -1, -1, -1 },
/* 40 */	{ H, H, H, H, H, H, H, B, B, B, B, B, B, B, B, B, B, B, B, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, HL, HL, HL, HL, HL },
/* 50 */	{ H, H, H, H, H, H, H, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, UNUSED_5, -1 },
/* 60 */	{ H, H, H, H, H, H, H, B, B, B, B, B, B, B, B, B, B, B, B, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, HR, HR, HR, HR, HR },
/* 70 */	{ H, H, H, H, H, H, -1, -1, -1, -1, -1, AR, AR, AR, AR, AR, AR, UNUSED_10, UNUSED_5, -1, -1, -1, -1 },
/* 80 */	{ H, H, H, H, H, UNUSED_20, UNUSED_10, -1 },
/* 90 */	{ H, H, H, H, UNUSED_20, UNUSED_10, -1, -1 }
	}	//	Maybe we'll do more some other day
};


	//	[distance][step]
const int hitsteps[4][6] =
{
	//	0, x, 2x, 4x, 6x, 9x
	{ 0,  3,  6,  12, 18, 27 },	//	3,  6, 12, 18, 27
	{ 0,  4,  9,  18, 27, 40 },	//	4.5,9, 18, 27, 40.5 ? 
	{ 0,  6,  12, 24, 36, 54 },	//	6, 12, 24, 36, 54
	{ 0,  7,  15, 30, 45, 67 },	//	7.5,15,30, 45, 67.5
};


#undef B
#undef H
#undef AL
#undef AR
#undef HL
#undef HR
#undef LL
#undef LR
#undef FL
#undef FR


#define UNUSED_5	-1,-1,-1,-1,-1
#define UNUSED_10	UNUSED_5, UNUSED_5
#define UNUSED_20	UNUSED_10, UNUSED_10
#endif

#define DEBUGGING	0


/*
 *  =-2 Victim not dead, don't continue combat
 *	=-1	Victim died.
 *	= 0	No damage.
 *	> 0	How much damage done.
 */
int hit(CharData * ch, CharData * victim, ObjData *weapon, int type, unsigned int times, unsigned int range, Direction dir, Flags flags)
{
	int attack_type;
	int dam;
	int	hits = 0,
			misses = 0,
			result = 0,
			total_dam = 0,
			basedam = 0,
			hit_counter;
//	float	multiplier;
	bool	critical = FALSE;
	bool	shooting = FALSE;
	int		headshots = 0;
	int		skill = 0, skill2 = 0;
	int		damtype;
	bool	dodged;
	float	hitskill, dodgeskill;
	
	if (GET_POS(ch) <= POS_STUNNED)
		return -2;
	
	if (AFF_FLAGGED(victim, AFF_RESPAWNING))
		return -2;
	
	if (IN_ROOM(ch) != IN_ROOM(victim)
#if ROAMING_COMBAT
		&& range == 0
#endif
		)
	{
		if (FIGHTING(ch) == victim)
			ch->stop_fighting();
#if !ROAMING_COMBAT
		if (range == 0)
#endif
			return -2;
	}
	
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPECIAL_MISSION) != ROOM_FLAGGED(IN_ROOM(victim), ROOM_SPECIAL_MISSION))
		return -2;
	
/*	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPECIAL_MISSION))
	{
		if (weapon && !OBJ_FLAGGED(weapon, ITEM_SPECIAL_MISSION))	return 1;
	}
	else if (weapon && OBJ_FLAGGED(weapon, ITEM_SPECIAL_MISSION))	return 1;
*/
	if (weapon && OBJ_FLAGGED(weapon, ITEM_SPECIAL_MISSION) &&
			!ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPECIAL_MISSION))	return 1;
	
	
	//	GROUP
#if defined(GROUP_COMBAT)
	CharData *originalVictim = victim;
	if (!IS_SET(flags, Hit_NoTargetRedirection)
		&& /*FIGHTING(ch)*/ AFF_FLAGGED(ch, AFF_NOQUIT) && AFF_FLAGGED(victim, AFF_GROUP))
	{
		//	If the attacker is already in combat, and the victim is in a group,
		//	then the attack may be redirected!

		CharData *master = victim->master ? victim->master : victim;
		CharData *follower;
		std::vector<CharData *>	potentials;
		int						points = 0;
		
		if (IN_ROOM(master) == IN_ROOM(victim)
			&& kill_valid(ch, master)
			&& ch->CanSee(master) != VISIBLE_NONE)
		{
			potentials.push_back(master);
			points += AFF_FLAGGED(master, AFF_GROUPTANK) ? 3 : 1;
		}
		
		START_ITER(iter, follower, CharData *, master->followers)
		{
			if (IN_ROOM(follower) == IN_ROOM(victim)
				&& kill_valid(ch, follower)
				&& ch->CanSee(follower) != VISIBLE_NONE)
			{
				potentials.push_back(follower);
				points += AFF_FLAGGED(follower, AFF_GROUPTANK) ? 3 : 1;
			}
		}
		
		if (potentials.size() > 0 && points > 0)
		{
			int num = Number(1, points);
			
			for (std::vector<CharData *>::iterator iter = potentials.begin();
				iter != potentials.end(); ++iter)
			{
				CharData *potential = *iter;
				
				points -= AFF_FLAGGED(potential, AFF_GROUPTANK) ? 3 : 1;
				
				if (points <= 0)
				{
					victim = potential;
					break;
				}
			}
		}
	}
#endif
	
	
	Affect::FromChar(ch, Affect::Sneak);
	Affect::FromChar(victim, Affect::Sneak);
	
	if (type == TYPE_THROW) {
		if (!weapon)
			return -2;
		
		attack_type = TYPE_THROW;
		basedam = weapon->GetValue(OBJVAL_WEAPON_DAMAGE);
	} else {
#if 0	//	This shouldn't be necessary since we dispatch combat elsewhere now
		if (weapon == NULL)
		{
			weapon = ch->GetWeapon();
		}
#endif
		
		shooting = weapon && IS_GUN(weapon) && ((GET_GUN_AMMO_TYPE(weapon) == -1) || (GET_GUN_AMMO(weapon) > 0)) &&
				(range > 0 || !OBJ_FLAGGED(weapon, ITEM_NOMELEE));
		
		if (type != TYPE_UNDEFINED)	attack_type = type;
		else if (shooting)			attack_type = TYPE_HIT + GET_GUN_ATTACK_TYPE(weapon);
		else if (weapon)			attack_type = TYPE_HIT + weapon->GetValue(OBJVAL_WEAPON_ATTACKTYPE);
		else if (IS_NPC(ch) && (ch->mob->m_AttackType != 0))
									attack_type = TYPE_HIT + ch->mob->m_AttackType;
		else						attack_type = (IS_ALIEN(ch) ? TYPE_CLAW : TYPE_PUNCH);

		if (shooting)		basedam = GET_GUN_DAMAGE(weapon);
		else if (weapon)	basedam = weapon->GetValue(OBJVAL_WEAPON_DAMAGE);
		else if (IS_NPC(ch) && (ch->mob->m_AttackDamage > 0))
							basedam = ch->mob->m_AttackDamage;
		else				basedam = IS_ALIEN(ch) ? 4 : 2;
	}
	
	
#if USING_STR_PENALTIES
	if ((!shooting || type == TYPE_THROW) && GET_EFFECTIVE_STR(ch) > 9)
		basedam += ((GET_EFFECTIVE_STR(ch) - 9) * 3 / 4) /* / 2*/;
#endif
		
	if (weapon)
	{
		if (shooting)
		{
		 	if (GET_GUN_AMMO_TYPE(weapon) != -1)
				times = MIN(times, GET_GUN_AMMO(weapon));	//	Limit the number of attacks to this
			skill = GET_GUN_SKILL(weapon);
			skill2 = GET_GUN_SKILL2(weapon);
			damtype = GET_GUN_DAMAGE_TYPE(weapon);
		}
		else
		{
			times *= MAX(1, weapon->GetValue(OBJVAL_WEAPON_RATE));
			skill = weapon->GetValue(OBJVAL_WEAPON_SKILL);
			skill2 = weapon->GetValue(OBJVAL_WEAPON_SKILL2);
			damtype = weapon->GetValue(OBJVAL_WEAPON_DAMAGETYPE);
		}
	}
	else
	{
		skill = SKILL_UNARMED_COMBAT;
		if (IS_NPC(ch))
		{
			damtype = ch->mob->m_AttackDamageType;
			if (ch->mob->m_AttackRate > 1)	times *= ch->mob->m_AttackRate;
		}
		else
			damtype = DAMAGE_SLASH;
		
		//	Note: NPCs should use their own value
	}
	
	
	if (DEBUGGING)	ch->Send("%d times, type %s, skill %s\n", times, damage_types[damtype], skill_name(skill));
	
	//	Dodge skill
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPECIAL_MISSION))
		dodgeskill = 20;	//	55
	else
	{
		dodgeskill = victim->GetEffectiveSkill(SKILL_DODGE);
		dodged = true;
#if USING_COMBAT_MODES
		if (range == 0 && GET_POS(victim) > POS_RESTING)
		{
			switch (GET_COMBAT_MODE(victim))
			{
				case COMBAT_MODE_DEFENSIVE:
					dodgeskill += 5;
					break;
				case COMBAT_MODE_NORMAL:
					break;
				case COMBAT_MODE_OFFENSIVE:
					dodgeskill -= 5;
					break;
			}
		}
#endif
	}
	
	if (dodgeskill < 0)					dodgeskill = 0;
	
	dodgeskill = LinearSkillRoll(dodgeskill);
	
	//	Visual modifiers
	//	Not needed with invis disabled in special mission rooms	
//	if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPECIAL_MISSION))
	{
		switch (ch->CanSee(victim))
		{
			case VISIBLE_NONE:		dodgeskill += /*12*/ 8;	break;
			case VISIBLE_SHIMMER:	dodgeskill += /*7*/ 4;	break;
//			case VISIBLE_FULL:		if (AFF_FLAGGED(victim, AFF_BLIND))	dodgeskill -= 8;	break;
		}
		
		if (AFF_FLAGGED(victim, AFF_BLIND))	dodgeskill -= 6;
	}

	//	Hit skill
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPECIAL_MISSION))
		hitskill = 20;	//	55
	else
	{
		hitskill = ch->GetEffectiveSkill(skill);
		if (skill2)	hitskill = (hitskill + ch->GetEffectiveSkill(skill2)) / 2;
	}
	
	float orighitskill = hitskill;
	
#if USING_COMBAT_MODES
	if (range == 0)
	{
		switch (GET_COMBAT_MODE(victim))
		{
			case COMBAT_MODE_DEFENSIVE:
				hitskill -= 5;
				break;
			case COMBAT_MODE_NORMAL:
				break;
			case COMBAT_MODE_OFFENSIVE:
				hitskill += 5;
				break;
		}
	}
#endif
	
	//	Range Modifiers
	if (range > 0)
	{
		if (AFF_FLAGGED(ch, AFF_DIDFLEE) && !AFF_FLAGGED(victim, AFF_DIDFLEE))
			hitskill -= 5;
		
		if (type == TYPE_THROW)
			hitskill -= range * 2;
		else if (!IS_SET(flags, Hit_IgnoreRange) && range > GET_GUN_OPTIMALRANGE(weapon))
			hitskill -= (range - GET_GUN_OPTIMALRANGE(weapon)) * 2;
	}
#if SAME_ROOM_GUN_PENALTY_DISABLED
	else if ((shooting && weapon && GET_GUN_RANGE(weapon) > 0) || type == TYPE_THROW)
		hitskill -= 7;
#endif
	
	//	Target position... crouching -2, prone -5 vs shooting only
#if PRONE_VICTIMS_DODGE_BONUS_DISABLED	//	Mostly useless, complicates things
	if (range > 0)
	{
		if (GET_POS(victim) == POS_SITTING)
			hitskill -= 2;
		else if (GET_POS(victim) <= POS_RESTING)
			hitskill -= 5;
	}
#endif
	
	//	There is already a bonus for the fact that the victim is <= resting; no dodge
	if (GET_POS(victim) < POS_RESTING)
		dodgeskill = 0;
	else if (GET_POS(victim) < POS_FIGHTING)
		dodgeskill -= ((POS_FIGHTING - GET_POS(victim)) * 2);
//	if (GET_POS(ch) < POS_FIGHTING)
//		dodgeskill += 5 + ((POS_SITTING - GET_POS(victim)) * 2);
	
	for (hit_counter = 0; hit_counter < times; hit_counter++)
	{
		float hitskillroll = LinearSkillRoll(hitskill - dodgeskill);
		
		if (range == 0
			&& (GET_POS(victim) <= POS_INCAP
				|| GET_POS(victim) == POS_SLEEPING))
			hitskillroll = 1.0f;
		
		if (hitskillroll >= 15.0f)
			critical = TRUE;

		if (hitskillroll >= 0)
		{
			int armor;
			
			//	Cover Bonus would be added here	(???)
			dam = dice(basedam, DAMAGE_DICE_SIZE);
			
			//	Right about here we would check if shooting and the ammo is armor piercing
			
			if (!shooting || type == TYPE_THROW)
			{
				if (!IS_NPC(ch))
				{
					if (GET_STR(ch) > 70 && (!weapon || weapon->GetFloatValue(OBJFVAL_WEAPON_STRENGTH) > 0.0f) )
					{
						float	strBonus = (GET_STR(ch) - 70) * 0.00375f;	//	.375% = +50% at 210 strength
						
						if (weapon)
						{
							strBonus = strBonus * weapon->GetFloatValue(OBJFVAL_WEAPON_STRENGTH);
						}
						
						dam += (int)((float)dam * strBonus);
					}
				}
				else if (GET_EFFECTIVE_STR(ch) > 10)
					dam += (int)(dam * (GET_EFFECTIVE_STR(ch) - 10.0f) * .05f);
			}
			
			//	Normally we'd figure out where it hit
			int location = Number(0, 100);
			
#if 0
			if (location < 10)					location = ARMOR_LOC_ARMS;
			else if (location < 20)				location = ARMOR_LOC_LEGS;
			else if (location < 24)				location = ARMOR_LOC_FEET;
			else if (location < 28)				location = ARMOR_LOC_HANDS;
			else if (location < 30)				location = ARMOR_LOC_HEAD;
			else								location = ARMOR_LOC_BODY;
#else
			if (location < 2)					location = ARMOR_LOC_HEAD;
			else								location = ARMOR_LOC_BODY;
#endif

			if (damtype >= 0)
			{
				armor = GET_ARMOR(victim, ARMOR_LOC_NATURAL, damtype);
					
				if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPECIAL_MISSION))
				{
					if (armor < GET_ARMOR(victim, ARMOR_LOC_MISSION, damtype))
						armor = GET_ARMOR(victim, ARMOR_LOC_MISSION, damtype);
				}
				else
				{
					if (armor < GET_ARMOR(victim, location, damtype))
						armor = GET_ARMOR(victim, location, damtype);
					armor += GET_EXTRA_ARMOR(victim);
				}
			}
			
			if (armor < 0)
				armor = 0;
			dam -= (dam * armor) / 100;
			
			if (dam <= 0)
				mudlogf(BRF, LVL_STAFF, false, "<= 0 damage dealt by %s fighting %s (basedam %d, armor %d).", ch->GetName(), victim->GetName(), basedam, armor);
			
			switch (location)
			{
				case ARMOR_LOC_HEAD:	dam *= 2;	++headshots;		break;
				case ARMOR_LOC_BODY:	break;
//				case ARMOR_LOC_FEET:
//				case ARMOR_LOC_HANDS:
//				case ARMOR_LOC_LEGS:
//				case ARMOR_LOC_ARMS:	/*dam /= 2;*/					break;
			}
			
			if (dam < 0)	dam = 0;
			
			total_dam += dam;
			hits++;
			
//			hitskill += 3;	//	+3 for each successful hit
		}
		else
		{
			misses++;
		}
	}
	
	Affect::FlagsFromChar(ch, AFF_HIDE);
	
/*	if (critical) {
		ch->Send("`B*CRITICAL STRIKE*`n ");
		victim->Send("`R*CRITICAL STRIKE*`n ");
	}
*/	if (headshots && (!weapon || !OBJ_FLAGGED(weapon, ITEM_SCRIPTEDCOMBAT)))
	{
		ch->Send("`B*(%d HEADSHOT%s)*`n ", headshots, headshots > 1 ? "s" : "");
		victim->Send("`R*(%d HEADSHOT%s)*`n ", headshots, headshots > 1 ? "s" : "");
	}
	
	//	Apply bonuses
#if 0
	if (hits > 0)
	{
		if (GET_POS(victim) < POS_FIGHTING)
			total_dam = total_dam * (1 + (POS_FIGHTING - GET_POS(victim))) / 3;
		/* Position  sitting  x 1.33 */
		/* Position  resting  x 1.66 */
		/* Position  sleeping x 2.00 */
		/* Position  stunned  x 2.33 */
		/* Position  incap    x 2.66 */
		/* Position  mortally x 3.00 */
	}
#endif
	
	const char * dirname = dir_text_the[rev_dir[dir]];
//	if (AFF_FLAGGED(ch, AFF_COVER))
//		dirname = "somewhere";
	
	if (range > 0)
	{
		if (hits)
		{
			if (!AFF_FLAGGED(ch, AFF_COVER))
				act("Something flies in from $T and strikes $n.", FALSE, victim, 0, dirname, TO_ROOM);
			else
			{
				FOREACH(CharacterList, victim->InRoom()->people, iter)
					if (*iter != victim)
						act("Something flies in from $t and strikes $N.", FALSE, *iter,
							//	If person is not fighting and succeed in skill roll, they can see the direction
							(ObjData *)((!FIGHTING(victim) && SkillRoll(victim->GetEffectiveSkill(SKILL_NOTICE)) > SkillRoll(orighitskill))
								? dirname
								: "somewhere"), victim, TO_CHAR);
			}
			
			victim->Send("(%d hit%s) `RSomething flies in from %s and hits YOU!`n\n", hits, hits > 1 ? "s" : "", dirname);
		}
		else
		{
			if (!AFF_FLAGGED(ch, AFF_COVER))
				victim->InRoom()->Send("Something zooms past from %s!\n", dirname);
			else
			{
				FOREACH(CharacterList, victim->InRoom()->people, iter)
					(*iter)->Send("Something zooms past from %s!\n",
						//	If person is not fighting and succeed in skill roll, they can see the direction
						(!FIGHTING(victim) && SkillRoll(victim->GetEffectiveSkill(SKILL_NOTICE)) > SkillRoll(orighitskill))
							? dirname : "somewhere");
			}
		}
	}
			
	//	Damage mods
	//	Global 75%
	if (!IS_NPC(ch))
	{
		if (!IS_NPC(victim))	//	Player vs Player
		{
			
			total_dam = total_dam * 2 / 3;	//	66%
		
			if (range == 0)
			{
				if (weapon && OBJ_FLAGGED(weapon, ITEM_HALF_MELEE_DAMAGE))
					total_dam = total_dam / 2;	//	1-28-04 - Gabe requested reduction to 50%
			}
		}
		else					//	Player vs Mob
		{
			if (shooting && range > 0 && MOB_FLAGGED(victim, MOB_HALFSNIPE))
			{
				total_dam = total_dam / 2;	//	1-28-04 - Incursion wants half-snipe flag for mobs
			}

			//	Player vs Mob
			if (IS_ALIEN(ch))
				total_dam = total_dam * 8 / 10;	//	3-16-04 - Gerek requested 10% reduction
			
			if (victim->GetLevel() != 100)
			{
				//	Mob tiers are multiples of 10, player tiers are +1
				int levelTierDifference = ((victim->GetLevel() /*+ 9*/) / 10) - ((ch->GetLevel() + 9) / 10);
			
				levelTierDifference = MIN(levelTierDifference, 5);

				if (levelTierDifference > 0)
				{
					total_dam = total_dam - ((total_dam * (15 * levelTierDifference)) / 100);
				}
			}
		}
		
		//	Apply Level Damage
		if (weapon && OBJ_FLAGGED(weapon, ITEM_LEVELBASED))
			total_dam += (total_dam * ch->GetLevel()) / 200;
	}
	else if (!IS_NPC(victim))	//	Mob vs Player 
	{
		
		if (range == 0)
			total_dam = total_dam * 2 / 3;
	}
	else	//	Mob vs Mob 
	{
	}
	
	if (weapon && OBJ_FLAGGED(weapon, ITEM_SCRIPTEDCOMBAT))
		result = 0;
	else if (hits)
		result = damage(ch, victim, weapon, total_dam > 0 ? total_dam : 1, attack_type, damtype, hits);
	else if (misses && !IS_SET(flags, Hit_NoMissMessages))
		result = damage(ch, victim, weapon, 0, attack_type, damtype, misses);
	
	if (result >= 0)
	{
		if (GET_POS(victim) >= POS_SLEEPING && GET_POS(victim) <= POS_SITTING)
		{
			Affect::FlagsFromChar(victim, AFF_SLEEP);
			GET_POS(victim) = POS_STANDING;
			victim->update_pos();
		}
		
		attack_otrigger_for_weapon(ch, victim, weapon, hits > 0 ? total_dam : -1, damtype, hits, headshots);
		
		attack_mtrigger(ch, victim, weapon, hits > 0 ? total_dam : -1, damtype, hits, headshots);
		attacked_mtrigger(ch, victim, weapon, hits > 0 ? total_dam : -1, damtype, hits, headshots);
		
		attack_otrigger(ch, victim, weapon, hits > 0 ? total_dam : -1, damtype, hits, headshots);
		attacked_otrigger(ch, victim, weapon, hits > 0 ? total_dam : -1, damtype, hits, headshots);
		
		if (PURGED(victim))
			result = -1;
	}
	
	if ((shooting || type == TYPE_THROW) && !PURGED(ch) && !AFF_FLAGGED(ch, AFF_RESPAWNING))
	{
		Affect::Flags MAKE_BITSET(affectflags, AFF_NOSHOOT);
		/*if (shooting)*/	affectflags.set(AFF_DIDSHOOT);
		
		(new Affect("COMBAT", 0, APPLY_NONE, affectflags))->Join(ch, combat_delay(ch, weapon, true));
	}
	
	
	if (/*!IS_NPC(ch)*/ !AFF_FLAGGED(ch, AFF_RESPAWNING))
	{
		(new Affect("COMBAT", 0, APPLY_NONE, AFF_NOQUIT))->Join(ch, IS_NPC(victim) ? 10 RL_SEC : 20 RL_SEC);
	}
//	GET_ROLL_STE(ch) = 0;
	
	if (/*!IS_NPC(victim)*/ !AFF_FLAGGED(victim, AFF_RESPAWNING))
	{
		(new Affect("COMBAT", 0, APPLY_NONE, AFF_NOQUIT))->Join(victim, IS_NPC(ch) ? 10 RL_SEC : 20 RL_SEC);
	}
//	GET_ROLL_STE(victim) = 0;
	
	if (shooting && IS_GUN(weapon) && GET_GUN_AMMO_TYPE(weapon) != -1)
	{
		GET_GUN_AMMO(weapon) -= times;
		if (GET_GUN_AMMO(weapon) <= 0)
			ch->Send("*CLICK* Your weapon is out of ammo.\n");
	}
	else if (type == TYPE_THROW && weapon && (GET_OBJ_TYPE(weapon) == ITEM_THROW))
	{
		if (hits)	weapon->Extract();
		else		unequip_char(ch, weapon->WornOn())->ToRoom(IN_ROOM(victim));
	}
	
	return result;
}


/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
	FOREACH(CharacterList, combat_list, iter)
	{
		CharData *ch = *iter;
		
		if (FIGHTING(ch) == NULL || (IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))))
		{
			CombatEvent *event = (CombatEvent *)Event::FindEvent(ch->m_Events, CombatEvent::Type);
			
			if (event && !event->IsRanged())
			{
				ch->redirect_fighting();
				continue;
			}
		}
		
		if (IS_NPC(ch)) {
			if (GET_MOB_WAIT(ch) > 0) {
//				GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
				continue;
			}
//			GET_MOB_WAIT(ch) = 0;
		} else if (CHECK_WAIT(ch))
				continue;
		
	    ch->update_pos();
	    
		if ((GET_POS(ch) > POS_STUNNED) && !Event::FindEvent(ch->m_Events, CombatEvent::Type)) {
			initiate_combat(ch, FIGHTING(ch), NULL);
		}

		if (ch->GetPrototype() && ch->GetPrototype()->m_Function)
			ch->GetPrototype()->m_Function(ch, ch, 0, "");
	}
}


void mob_reaction(CharData *ch, CharData *vict, int dir) {
	if (!IS_NPC(vict) || FIGHTING(vict) || GET_POS(vict) <= POS_STUNNED)
		return;

	/* can remember so charge! */
	if (MOB_FLAGGED(vict, MOB_MEMORY))
		remember(vict, ch);

//	act("$n bellows in pain!", FALSE, vict, 0, 0, TO_ROOM); 
//	if (GET_POS(vict) == POS_STANDING) {
//			if (!perform_move(vict, rev_dir[dir], 1))
//				act("$n stumbles while trying to run!", FALSE, vict, 0, 0, TO_ROOM);
//		} else
//			GET_POS(vict) = POS_STANDING;
      
		/* can't remember so try to run away */
//	}
	if (MOB_FLAGGED(vict, MOB_WIMPY))
		do_flee(vict, "", "flee", 0);
	else if ((MOB_FLAGGED(vict, MOB_AGGRESSIVE | MOB_AGGR_ALL | MOB_MEMORY) ||
			(MOB_FLAGGED(vict, MOB_AI) && vict->mob->m_AIAggrRoom > 0))) {
		if (GET_POS(vict) != POS_STANDING)
			GET_POS(vict) = POS_STANDING;
		if (!vict->path && (!MOB_FLAGGED(vict, MOB_AI) || (Number(1, 100) <= vict->mob->m_AIAggrRoom)))
			vict->path = Path2Char(vict, GET_ID(ch), 10, HUNT_GLOBAL);
	}
	else if (Number(1, 3) == 1)
		do_flee(vict, "", "flee", 0);
}



CharData * scan_target(CharData *ch, char *arg, int range, int &dir, int *distance_result, bool isGun)
{
	CharData *victim;
	int		endDir;
	int		distance;
	
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
		return NULL;

	if (dir == -1)
	{
		dir = 0;
		endDir = NUM_OF_DIRS;
	}
	else
		endDir = dir + 1;
	
	for (; dir < endDir; ++dir)
	{
		RoomData *room = ch->InRoom();
		
		if (isGun)
		{
			if (!Exit::IsOpen(room, dir)
				|| EXIT_FLAGGED(Exit::Get(room, dir), EX_NOSHOOT)
				|| ROOM_FLAGGED(Exit::Get(room, dir)->ToRoom(), ROOM_PEACEFUL))
				continue;
			
			room = Exit::Get(room, dir)->ToRoom();
			
			distance = 1;
		}
		else
			distance = 0;

		for (; distance <= range; ++distance)
		{		
			victim = get_char_other_room_vis(ch, arg, room);
			
			if (victim && victim != ch)
			{
				if (distance_result)
					*distance_result = distance;
				return victim;
			}
			
			if (!Exit::IsOpen(room, dir)
				|| EXIT_FLAGGED(Exit::Get(room, dir), EX_NOSHOOT)
				|| ROOM_FLAGGED(Exit::Get(room, dir)->ToRoom(), ROOM_PEACEFUL))
				break;
			
			room = Exit::Get(room, dir)->ToRoom();
		}
	}
	
	return NULL;
}


CharData * scan_target(CharData *ch, CharData *victim, int range, int &dir, int *distance_result, bool isGun)
{
	int		endDir;
	int		distance;
	
	if (victim == ch)
		return NULL;

	if (ROOM_FLAGGED(ch->InRoom(), ROOM_PEACEFUL))
		return NULL;
	
	if (dir == -1)
	{
		dir = 0;
		endDir = NUM_OF_DIRS;
	}
	else
		endDir = dir + 1;
		
	for (; dir < endDir; ++dir)
	{
		RoomData *room = ch->InRoom();
		
		if (isGun)
		{
			RoomExit *exit = Exit::Get(room, dir);
			
			if (!Exit::IsOpen(exit)
				|| EXIT_FLAGGED(exit, EX_NOSHOOT)
				|| ROOM_FLAGGED(exit->ToRoom(), ROOM_PEACEFUL))
				continue;
			
			room = exit->ToRoom();
			
			distance = 1;
		}
		else
			distance = 0;
		
		for (; distance <= range; ++distance)
		{
			if (IN_ROOM(victim) == room)
			{
				if (distance_result)
					*distance_result = distance;
				return victim;
			}
			
			RoomExit *exit = Exit::Get(room, dir);
			if (!Exit::IsOpen(exit)
				|| EXIT_FLAGGED(exit, EX_NOSHOOT)
				|| ROOM_FLAGGED(exit->ToRoom(), ROOM_PEACEFUL))
				break;
			
			room = exit->ToRoom();
		}
	}
	
	return NULL;
}


int fire_missile(CharData *ch, char *arg1, ObjData *missile, int range, int dir) {
	int	distance;
	CharData *vict;
    
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && !NO_STAFF_HASSLE(ch)) {
		send_to_char("This room just has such a peaceful, easy feeling...\n", ch);
		return -2;
	}

	vict = scan_target(ch, arg1, range, dir, &distance, GET_OBJ_TYPE(missile) == ITEM_WEAPON);
	if (!vict || !ch->CanSee(vict))
		return -3;
	else if (distance == 0)
	{
		initiate_combat(ch, vict, NULL);
		return 0;
	}
	else
		return perform_fire_missile(ch, vict, missile, distance, dir);
}



int fire_missile(CharData *ch, CharData *victim, ObjData *missile, int range, int dir)
{
	int	distance;
    
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && !NO_STAFF_HASSLE(ch)) {
		send_to_char("This room just has such a peaceful, easy feeling...\n", ch);
		return -2;
	}
	
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_GRAVITY)) {
		return -3;
	}

	victim = scan_target(ch, victim, range, dir, &distance, GET_OBJ_TYPE(missile) == ITEM_WEAPON);
	if (!victim || !ch->CanSee(victim))
		return -3;
	
	if (MOB_FLAGGED(victim, MOB_NOSNIPE))
	{
		act("Shoot at $M?  You'd only piss $M off, at best.", FALSE, ch, 0, victim, TO_CHAR);
		return -2;
	}

	return perform_fire_missile(ch, victim, missile, distance, dir);
}



int perform_fire_missile(CharData *ch, CharData *victim, ObjData *missile, int distance, int dir)
{
	int attacktype;
	int result;
	
	/* Daniel Houghton's missile modification */
	if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_PEACEFUL) && !NO_STAFF_HASSLE(ch)) {
		send_to_char("Nah.  Leave them in peace.\n", ch);
		return -2;
	}
	
	if (MOB_FLAGGED(victim, MOB_NOSNIPE))
	{
		act("Shoot at $M?  You'd only piss $M off, at best.", FALSE, ch, 0, victim, TO_CHAR);
		return -2;
	}
	
	if (!kill_valid(ch, victim))
	{
		if (!PRF_FLAGGED(ch, PRF_PK)) {
			send_to_char("You must turn on PK first!  Type \"pkill\" for more information.\n", ch);
		}
		else if (!PRF_FLAGGED(victim, PRF_PK)) {
			act("You cannot kill $N because $E has not chosen to participate in PK.", FALSE, ch, 0, victim, TO_CHAR);
		}
		else
			send_to_char("You can't player-kill here.\n", ch);
		return -2;
	}
	
	//	Group tank chars intercept the shot
#if !defined(GROUP_COMBAT)
	if (AFF_FLAGGED(victim, AFF_GROUP) && !AFF_FLAGGED(victim, AFF_GROUPTANK))
	{
		CharData *leader = victim->m_Following ? victim->m_Following : victim;
		std::vector<CharData *>	potentials;
		
		if (AFF_FLAGGED(leader, AFF_GROUPTANK) && IN_ROOM(victim) == IN_ROOM(leader) &&
			!MOB_FLAGGED(leader, MOB_NOSNIPE))
			potentials.push_back(leader);
		
		FOREACH(CharacterList, leader->m_Followers, iter)
		{
			CharData *follower = *iter;

			if (AFF_FLAGGED(follower, AFF_GROUPTANK) && IN_ROOM(victim) == IN_ROOM(follower) &&
				!MOB_FLAGGED(follower, MOB_NOSNIPE))
				potentials.push_back(follower);
		}
		
		if (potentials.size())
		{
			int num = Number(0, potentials.size() - 1);
			victim = potentials[num];
		}
	}
#endif
	
	check_killer(ch, victim);
	
	int times = 1;
	
	BUFFER(buf, MAX_STRING_LENGTH);	
	switch(GET_OBJ_TYPE(missile)) {
		case ITEM_BOOMERANG:
		case ITEM_THROW:
			if (IN_ROOM(ch) == IN_ROOM(victim))
			{
				act("You throw $p at $N!", TRUE, ch, missile, victim, TO_CHAR);
				act("$n throws $p at $N!", FALSE, ch, missile, victim, TO_NOTVICT);
				act("$n throws $p at you!", FALSE, ch, missile, victim, TO_VICT);
			}
			else
			{
				sprintf(buf, "You throw $p %s at $N!", dirs[dir]);
				act(buf, TRUE, ch, missile, victim, TO_CHAR);
				sprintf(buf, "$n throws $p %s at $N!", dirs[dir]);
				act(buf, FALSE, ch, missile, victim, TO_NOTVICT);
				sprintf(buf, "$n throws $p at you from %s!", dir_text_the[rev_dir[dir]]);
				act(buf, FALSE, ch, missile, victim, TO_VICT);
			}
			attacktype = TYPE_THROW;
			break;
		case ITEM_WEAPON:
			if (IN_ROOM(ch) == IN_ROOM(victim))
			{
				act("You fire at $N!", TRUE, ch, 0, victim, TO_CHAR);
				act("$n fires at $N!", TRUE, ch, 0, victim, TO_NOTVICT);
				act("$n fires at you!", TRUE, ch, 0, victim, TO_VICT);
			}
			else
			{
				act("$n aims $t and fires at $N!", TRUE, ch, (ObjData *)dirs[dir], victim, TO_ROOM);
				act("You aim $t and fire at $N!", TRUE, ch, (ObjData *)dirs[dir], victim, TO_CHAR);
			}
			attacktype = TYPE_UNDEFINED;
			times = GET_GUN_RATE(missile);
			if (IS_GUN(missile) && GET_GUN_AMMO_TYPE(missile) != -1)
				times = MIN((int)GET_GUN_AMMO(missile), times);
			break;
		default:
			attacktype = TYPE_UNDEFINED;
			break;
	}
	
	Flags flags = 0;
	
#if ROAMING_COMBAT //&& ENABLE_ROAMING_COMBAT
	if (GET_OBJ_TYPE(missile) == ITEM_WEAPON
		&& OBJ_FLAGGED(missile, ITEM_AUTOSNIPE)
		&& IN_ROOM(ch) != IN_ROOM(victim))
	{
		if (!FIGHTING(ch))
		{
			ch->AddOrReplaceEvent(new CombatEvent(combat_delay(ch, missile), ch, missile, IN_ROOM(ch) != IN_ROOM(victim)));
			set_fighting(ch, victim);
		}
		
//		flags |= Hit_NoTargetRedirection;
		
/*		if (IN_ROOM(ch) == IN_ROOM(victim))
		{
			if (!FIGHTING(victim))
			{
				victim->AddOrReplaceEvent(new CombatEvent(combat_delay(victim, get_combat_weapon(ch, victim)),
					victim, missile, IN_ROOM(ch) != IN_ROOM(victim)));
				set_fighting(victim, ch);
			}
		}
*/
	}
#endif
	
	result = hit(ch, victim, missile, attacktype, times, distance, (Direction)dir, flags);
	
	if (result >= 0)
	{
	//	This is already done in hit()
//			if ((GET_OBJ_TYPE(missile) == ITEM_ARROW) || (GET_OBJ_TYPE(missile) == ITEM_THROW))
//				unequip_char(ch, missile->WornOn())->ToRoom(IN_ROOM(vict));
	/* either way mob remembers */
		mob_reaction(ch, victim, dir);
	}
	
	return result;
}



void spray_missile(CharData *ch, ObjData *missile, int range, int dir, int rounds) {
	int hits = 0, misses = 0;
	int distance;
	int totalVicts, whichVict;
	RoomData *room;
	std::vector<CharData *>	targets;
    
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && !NO_STAFF_HASSLE(ch)) {
		send_to_char("This room just has such a peaceful, easy feeling...\n", ch);
		return;
	}
	
	room = ch->InRoom();
	
	if (Exit::IsOpen(room, dir) && !EXIT_FLAGGED(Exit::Get(room, dir), EX_NOSHOOT))
		room = Exit::Get(room, dir)->ToRoom();
	else
		room = NULL;
	
	if (!OBJ_FLAGGED(missile, ITEM_SCRIPTEDCOMBAT))
	{
		act("$n fires $T!", TRUE, ch, 0, dirs[dir], TO_ROOM);
		act("You fire $T!", TRUE, ch, 0, dirs[dir], TO_CHAR);
	}

	targets.reserve(8);
	
	for (distance = 1; room && (distance <= range); distance++)
	{
		if (ROOM_FLAGGED(room, ROOM_PEACEFUL))
			break;
		
		FOREACH(CharacterList, room->people, iter)
		{
			CharData *vict = *iter;
			
//			if (!ch->CanSee(vict) && Number(1, 5) != 1)
//				continue;
			if (MOB_FLAGGED(vict, MOB_NOSNIPE))
				continue;
			
			if (ch->GetRelation(vict) != RELATION_ENEMY && Number(1,3) != 1)	//	Don't bother
				continue;
			
			targets.push_back(vict);
		}
		
		if (Exit::IsOpen(room, dir) && !EXIT_FLAGGED(Exit::Get(room, dir), EX_NOSHOOT))
			room = Exit::Get(room, dir)->ToRoom();
		else
			room = NULL;
	}
	
	totalVicts = targets.size();
	totalVicts = totalVicts + ((totalVicts + 3) / 4);	//	25% extra, min + 1
	if (totalVicts < 3)		totalVicts = 3;		//	Minimum 3
	
	for (; !PURGED(ch) && rounds > 0 && (GET_GUN_AMMO_TYPE(missile) == -1 || GET_GUN_AMMO(missile) > 0); --rounds)
	{
		// pick vict
		whichVict = Number(1, totalVicts) - 1;
		CharData *vict = NULL;
		
		if (whichVict < targets.size())
		{
			vict = targets[whichVict];
			
			check_killer(ch, vict);
			
			++hits;
			
			if (hit(ch, vict, missile, TYPE_UNDEFINED, 1, 1, (Direction)dir, Hit_Spraying) < 0
				|| PURGED(vict))
				targets.erase(targets.begin() + whichVict);
			else
				mob_reaction(ch, vict, dir);
		}
		else
		{
			++misses;
		}

		for(std::vector<CharData *>::iterator iter = targets.begin(); iter != targets.end(); )
		{
			if (PURGED(*iter))	iter = targets.erase(iter);
			else				++iter;
		}
	}	//	FOR search for target
	
	//	'misses' is remaining missed; 'hits' are hits, already subtracted 1
	if (GET_GUN_AMMO_TYPE(missile) != -1 && GET_GUN_AMMO(missile) > 0)
	{
		int ammoused = misses; //(misses * 4) + (hits * 3);
		ammoused = MIN((int)GET_GUN_AMMO(missile), ammoused);
		GET_GUN_AMMO(missile) -= ammoused;
		
		if (GET_GUN_AMMO(missile) <= 0)
			ch->Send("*CLICK* Your weapon is out of ammo.\n");
	}

	if (hits == 0)
	{
		Affect *aff = new Affect("COMBAT", 0, APPLY_NONE, MAKE_BITSET(Affect::Flags, AFF_NOSHOOT, AFF_DIDSHOOT));
		aff->Join(ch, combat_delay(ch, missile, true));
	}
}



void explosion_damage(CharData *ch, CharData *vict, int dam, int type)
{
#if 0
	int individual_damage, total_damage = 0;
	int location_damage[NUM_ARMOR_LOCS];
	
	individual_damage = dice(dam, DAMAGE_DICE_SIZE);
	
	for (int i = 0; i < ARMOR_LOC_NATURAL; ++i)
	{
		location_damage[i] = (individual_damage + ARMOR_LOC_NATURAL - 1) / ARMOR_LOC_NATURAL;
		location_damage[i] -= (dice(GET_ARMOR(vict, i, type) + ARMOR_LOC_NATURAL - 1) / ARMOR_LOC_NATURAL;
		
		if (location_damage[i] > 0)
			total_damage += location_damage[i];
	}
	
	damage(ch, vict, total_damage, TYPE_EXPLOSION, DAMAGE_UNKNOWN, 1);
#else
	int armor = 0;
		
	dam = dice(dam, DAMAGE_DICE_SIZE);
	
	if (IS_NPC(vict))
		armor = GET_ARMOR(vict, ARMOR_LOC_NATURAL, type);
	else if (ROOM_FLAGGED(IN_ROOM(vict), ROOM_SPECIAL_MISSION))
		armor = GET_ARMOR(vict, ARMOR_LOC_MISSION, type);
	else
	{
		for (int i = 0; i < ARMOR_LOC_NATURAL; ++i)
			armor += GET_ARMOR(vict, i, type);
		
		armor = (armor + (ARMOR_LOC_NATURAL - 1)) / ARMOR_LOC_NATURAL;	//	+armorlocnatural-1 to round up...
	}
		
//	armor = dice(armor, DAMAGE_DICE_SIZE);
//	dam = dam - armor;

	dam -= (dam * armor) / 100;
	
#endif
	if (dam <= 0)
		return;
	
	damage(ch, vict, NULL, dam, TYPE_EXPLOSION, DAMAGE_UNKNOWN, 1);
}


void explosion(ObjData *obj, CharData *ch, int dam, int type, RoomData *room, int shrapnel)
{
	if (room == NULL)
		return;
		
    if (OBJ_FLAGGED(obj, ITEM_SPECIAL_MISSION) ^ ROOM_FLAGGED(room, ROOM_SPECIAL_MISSION))
		return;
	
	FOREACH(CharacterList, room->people, iter)
	{
		CharData *tch = *iter;
		
		/* You can't damage an immortal! */
		if (NO_STAFF_HASSLE(tch) || !kill_valid(ch, tch) ||
				MOB_FLAGGED(tch, MOB_PROGMOB))
			continue;
			
		act("You are caught in the blast!", TRUE, tch, 0, 0, TO_CHAR);
		
		explosion_damage(ch, tch, dam, type);
	}
	
	for (int door = 0; door < NUM_OF_DIRS; door++) {
		RoomData *exit_room;
		if (!Exit::IsOpen(room, door) || ((exit_room = Exit::Get(room,door)->ToRoom()) == room))
			continue;
		
		exit_room->Send("You hear a loud explosion come from %s!\n", dir_text_the[rev_dir[door]]);
		
		if (dam < 5)
			continue;
		
		/* Extended explosion sound */
		if ((dam >= 15) && Exit::IsOpen(exit_room, door) && (Exit::Get(exit_room, door)->ToRoom() != room))
			Exit::Get(exit_room, door)->ToRoom()->Send("You hear a distant explosion come from %s!\n", dir_text_the[rev_dir[door]]);
		/* End Extended sound */
		
		if (!shrapnel || ROOM_FLAGGED(exit_room, ROOM_PEACEFUL) || (dam < 10))
			continue;
		
        if (OBJ_FLAGGED(obj, ITEM_SPECIAL_MISSION) ^ ROOM_FLAGGED(exit_room, ROOM_SPECIAL_MISSION))
        	continue;
	
		exit_room->Send("Shrapnel flies into the room from %s!\n", dir_text_the[rev_dir[door]]);
		
		FOREACH(CharacterList, exit_room->people, iter)
		{
			CharData *tch = *iter;
			
			/* You can't damage an immortal! */
			if (NO_STAFF_HASSLE(tch) || ((dam < 750) && Number(0,1)) || !kill_valid(ch, tch) ||
					MOB_FLAGGED(tch, MOB_PROGMOB))
				continue;
			
			act("You are hit by shrapnel!", TRUE, tch, 0, 0, TO_CHAR);
			
			explosion_damage(ch, tch, dam / 2, type);
		}
	}
}

void acidblood(CharData *ch, int dam) {
//	EVENT(acid_burn);
//	struct event_info *temp_event;
	int acid_dam /*, message_sent = 0*/;
	
	RoomData *room = ch->InRoom();
	
	if (room == NULL)
		return;
	
	act("Acid blood sprays from $n's wound!", FALSE, ch, 0, 0, TO_ROOM);
	act("Acid blood sprays from your wound!", FALSE, ch, 0, 0, TO_CHAR);
	
	FOREACH(CharacterList, room->people, iter)
	{
		CharData *tch = *iter;
		
		/* You can't damage an immortal, alien, or acidblood character, or yourself. */
		/* Plus if the damage is less than 200, there is a 2 in 3 chance it will be ignored. */
		if (NO_STAFF_HASSLE(tch) || tch == ch || IS_ALIEN(tch) ||
				MOB_FLAGGED(tch, MOB_ACIDBLOOD) || AFF_FLAGGED(tch, AFF_ACIDPROOF))
			continue;
		
/*		if ((dam < 200)) {		// if damage is less than 200
			if (Number(0,2))	// then only a 1 in 3 chance of splashing onto char
				continue;
		} else if ((dam < 300) && Number(0,1))	// otherwise, if damage is less than 300
			continue;			// then 1 in 2 chance of splashing
*/
		if (Number(0, dam) < 100)
			continue;
		
		act("You are sprayed with $n's acid blood!", FALSE, ch, 0, tch, TO_VICT);
		/* Acid Blood handler goes here. */
//		acid_dam = dice(8, dam / 16);
		//	1 Damage per 10% HP above 50%, round up
//		damageDice = ((dam - (GET_MAX_HIT(ch) / 4)) + 49) / 50;
		acid_dam = dice((dam + 99) / 100, DAMAGE_DICE_SIZE / 2);
		//if ((temp_event = get_event(tch, acid_burn, EVENT_CAUSER))) {
		//	temp_event->info = (Ptr)((int)temp_event->info + acid_dam);
		//} else {
		
/*		int armor = 0;
		if (IS_NPC(tch)) {
			armor = GET_ARMOR(tch, ARMOR_LOC_NATURAL, DAMAGE_ACID);
		} else if (ROOM_FLAGGED(IN_ROOM(tch), ARMOR_LOC_MISSION))
			armor = GET_ARMOR(tch, ARMOR_LOC_MISSION, DAMAGE_ACID);
		else {
			for (int i = 0; i < ARMOR_LOC_NATURAL; ++i)
				armor += GET_ARMOR(tch, i, DAMAGE_ACID);
			armor = (armor + (ARMOR_LOC_NATURAL - 1)) / ARMOR_LOC_NATURAL;	//	+armorlocnatural-1 to round up...
		}

//		armor = dice(armor, DAMAGE_DICE_SIZE);
//		acid_dam = acid_dam - armor;

		acid_dam -= (acid_dam * armor) / 100;
*/		
		if (acid_dam > (GET_HIT(tch)) + 99)
			acid_dam = (GET_HIT(tch) + 99);
		
		if (acid_dam > 0)
			damage(ch, tch, NULL, acid_dam, TYPE_ACIDBLOOD, DAMAGE_UNKNOWN, 1);
//		update_pos(tch);
		//	add_event(ACID_BURN_TIME, acid_burn, tch, 0, (Ptr)acid_dam);
		//}
	}
}


void acidspray(RoomData * room, int dam) {
//	EVENT(acid_burn);
//	struct event_info *temp_event;
	int acid_dam;
	
	if (room == NULL)
		return;
		
	FOREACH(CharacterList, room->people, iter)
	{
		CharData *tch = *iter;
		
		/* You can't damage an immortal, alien, or acidblood character, or yourself. */
		/* Plus if the damage is less than 200, there is a 2 in 3 chance it will be ignored. */
		if (NO_STAFF_HASSLE(tch) || IS_ALIEN(tch) || MOB_FLAGGED(tch, MOB_ACIDBLOOD) ||
				(dam < 200 && Number(0,2)))
			continue;
		
		if ((dam > 300) || Number(0,1)) {
/*			if (!message_sent) {
				act("Acid blood sprays from $n's wound!", FALSE, ch, 0, 0, TO_ROOM);
				act("Acid blood sprays from your wound!", FALSE, ch, 0, 0, TO_CHAR);
				message_sent = 1;
			}*/
			act("$n is sprayed with acid blood!", FALSE, tch, 0, 0, TO_ROOM);
			act("You are sprayed with acid blood!", FALSE, tch, 0, 0, TO_CHAR);
			/* Acid Blood handler goes here. */
			/* acid damage 8D(dam>>3) */
			acid_dam = dice(8, dam / 8);
			//if ((temp_event = get_event(tch, acid_burn, EVENT_CAUSER))) {
			//	temp_event->info = (Ptr)((int)temp_event->info + acid_dam);
			//} else {
				damage(NULL, tch, NULL, acid_dam, TYPE_ACIDBLOOD, DAMAGE_UNKNOWN, 1);
			//	add_event(ACID_BURN_TIME, acid_burn, tch, 0, (Ptr)acid_dam);
			//}
		}
	}
}

