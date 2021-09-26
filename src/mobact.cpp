#include "characters.h"
#include "descriptors.h"
#include "event.h"
#include "index.h"
#include "objects.h"
#include "opinion.h"
#include "rooms.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "utils.h"
#include "track.h"
#include "buffer.h"
#include "find.h"
#include "spells.h"
#include "constants.h"
#include "zones.h"


ACMD(do_flee);
ACMD(do_hit);

/* local functions */
void mobile_activity(CharData *ch);
void check_mobile_activity(unsigned int pulse);
bool clearpath(CharData *ch, RoomData *room, int dir);
bool clearpathshoot(CharData *ch, RoomData *room, int dir);
int fire_missile(CharData *ch, char *arg1, ObjData *missile, int range, int dir);
void MobHunt(CharData *ch);
void FoundHated(CharData *ch, CharData *vict);
CharData *AggroScan(CharData *mob, int range);
bool ShootScan(CharData *mob, int range);
int randomize_dirs(int rand_dir[NUM_OF_DIRS]);
int perform_fire_missile(CharData *ch, CharData *victim, ObjData *missile, int distance, int dir);


bool clearpath(CharData *ch, RoomData *room, int dir)
{
	if (room == NULL)	return false;
	
	RoomExit *exit = Exit::GetIfValid(room, dir);

	if (!exit || !Exit::IsPassable(exit))						return false;
	if (IS_NPC(ch) && EXIT_FLAGGED(exit, EX_NOMOB))				return false;
	if (IS_NPC(ch) && ROOM_FLAGGED(exit->ToRoom(), ROOM_NOMOB))	return false;
	if (MOB_FLAGGED(ch, MOB_STAY_ZONE) && !IsSameEffectiveZone(room, exit->ToRoom()))	return false;
	
	return true;
}


bool clearpathshoot(CharData *ch, RoomData * room, int dir) {
	if (room == NULL)	return false;
	
	const RoomExit *exit = room->GetExit(dir);

	if (!exit || !Exit::IsViewable(exit))					return false;
	if (EXIT_FLAGGED(exit, EX_NOSHOOT))						return false;
	if (ROOM_FLAGGED(exit->ToRoom(), ROOM_PEACEFUL))		return false;
	
	return true;
}




int randomize_dirs(int rand_dir[NUM_OF_DIRS]) {
//	const int dirtrack[] = { NORTH, EAST, SOUTH, WEST, NORTH, EAST, SOUTH };
//	const int dirtrack[] = { 0, 1, 2, 3, 0, 1, 2 };

	int cardinal;
	int numcardinaldirs = 4;//	Number(1, 4);
	int numverticaldirs = 2;//	Number(0, 2);
	int current_dir = 0;
	
	int i;
	
	cardinal = Number(NORTH, WEST);
	for (i = 0; i < numcardinaldirs; ++i)
		rand_dir[current_dir++] = cardinal++ & 0x3;	// % 4
	
	cardinal = Number(UP - UP, DOWN - UP);
	for (i = 0; i < numverticaldirs; ++i)
		rand_dir[current_dir++] = UP + (cardinal++ & 1);	// % 2
	
	return current_dir;
}


CharData *AggroScan(CharData *mob, int range) {
	CharData *best = NULL;
	int relation;
	int		rand_dir[NUM_OF_DIRS], num_of_dirs;
	
	num_of_dirs = randomize_dirs(rand_dir);
	for (int realdir = 0; realdir < num_of_dirs; ++realdir) {
		int dir = rand_dir[realdir];
		
		RoomData *room = mob->InRoom();
		for (;range > 0; --range) {
			if (!clearpath(mob, room, dir))
				break;
			
			room = room->GetExit(dir)->ToRoom();
			FOREACH(CharacterList, room->people, iter) {
				CharData *ch = *iter;
				if (mob->CanSee(ch) != VISIBLE_FULL || NO_STAFF_HASSLE(ch))
					continue;
				
				//	Closest first hate
				if (mob->mob->m_Hates.IsActive() && mob->mob->m_Hates.IsTrue(ch))
					return ch;
				
				if (best)
					continue;
				
				relation = mob->GetRelation(ch);
				
				if ((MOB_FLAGGED(mob, MOB_AGGR_ALL) && (relation != RELATION_FRIEND)) ||
					(MOB_FLAGGED(mob, MOB_AGGRESSIVE) && relation == RELATION_ENEMY) ||
					(MOB_FLAGGED(mob, MOB_AI) && relation == RELATION_ENEMY))
					best = ch;
			}
		}
	}
	return best;
}


extern void check_killer(CharData * ch, CharData * vict);
extern void mob_reaction(CharData *ch, CharData *vict, int dir);
extern char *dirs[];


bool ShootScan(CharData *mob, int range) {
	ObjData *gun = mob->GetGun();
	RoomData *room;
	int		relation;
	int		friends, enemies;
	int		rand_dir[NUM_OF_DIRS], num_of_dirs;

	if (!gun)										return false;	//	No weapon
	if (GET_POS(mob) <= POS_FIGHTING)				return false;
	if (mob->InRoom() == NULL)						return false;
	if (ROOM_FLAGGED(mob->InRoom(), ROOM_PEACEFUL))	return false;
	if (AFF_FLAGGED(mob, AFF_NOSHOOT))				return false;
	if (!MOB_FLAGGED(mob, MOB_AGGRESSIVE | MOB_AGGR_ALL | MOB_AI) && !mob->mob->m_Hates.IsActive())	return false;

	if (!OBJ_FLAGGED(gun, ITEM_SPRAY) && OBJ_FLAGGED(gun, ITEM_NOSHOOT))
		return false;
	if ((GET_GUN_AMMO_TYPE(gun) != -1 && GET_GUN_AMMO(gun) <= 0) || GET_GUN_RANGE(gun) <= 0)
		return false;		//	No ammo or no range or not shootable
						
	range = MIN(range, GET_GUN_RANGE(gun));
	
	num_of_dirs = randomize_dirs(rand_dir);
	for (int realdir = 0; realdir < num_of_dirs; ++realdir) {
		int dir = rand_dir[realdir];

		room = mob->InRoom();
		
		//	This is here to make watchers only shoot one direction
		if (CHAR_WATCHING(mob) != -1) {
			dir = CHAR_WATCHING(mob);
		}

		enemies = 0;
		friends = 0;
		
		for (; range > 0; --range) {
			if (!clearpathshoot(mob, room, dir))
				break;
			
			room = room->GetExit(dir)->ToRoom();
			FOREACH(CharacterList, room->people, iter)
			{
				CharData *ch = *iter;
				if (mob->CanSee(ch) != VISIBLE_FULL || NO_STAFF_HASSLE(ch) || MOB_FLAGGED(ch, MOB_NOSNIPE))
					continue;
				
				relation = mob->GetRelation(ch);
				
				if (relation == RELATION_FRIEND) {
					friends++;
				} else if ((mob->mob->m_Hates.IsActive() && mob->mob->m_Hates.IsTrue(ch)) ||
							(MOB_FLAGGED(mob, MOB_AGGR_ALL) && relation != RELATION_FRIEND) ||
							(MOB_FLAGGED(mob, MOB_AGGRESSIVE) && relation == RELATION_ENEMY) ||
							(MOB_FLAGGED(mob, MOB_AI)  && relation == RELATION_ENEMY)) {
/*					int result;
					
					result = fire_missile(mob, ch->GetIDString(), gun, range, dir);
					if (result)
						return result;
*/
					
//					if (OBJ_FLAGGED(gun, ITEM_SPRAY) && OBJ_FLAGGED(gun, ITEM_NOSHOOT)) {
//						enemies++;
//					}
//					else
					{
						
						check_killer(mob, ch);
						
						perform_fire_missile(mob, ch, gun, range, dir);

						/* either way mob remembers */
						mob_reaction(mob, ch, dir);
					}
					
					return true;
				}
			}
		}
		
		//	This is here to make watchers only shoot one direction
		if (CHAR_WATCHING(mob) != -1) {
			break;
		}

	}
	return false;
}


static bool PrefersToShoot(CharData *mob) {
	ObjData *gun = mob->GetGun();
	return gun &&
		(/*OBJ_FLAGGED(gun, ITEM_SPRAY) ||*/ !OBJ_FLAGGED(gun, ITEM_NOSHOOT)) &&
		(GET_GUN_AMMO_TYPE(gun) == -1 || GET_GUN_AMMO(gun) > 0) &&
		(GET_GUN_RANGE(gun) > 0);
}


void FoundHated(CharData *ch, CharData *vict) {
	if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && !NO_STAFF_HASSLE(vict))
		initiate_combat(ch, vict, NULL);
}


void MobHunt(CharData *ch) {
	CharData *	vict;
	RoomData *	dest;
	int			dir;
	int			relation;
	
	if (!ch->path)	return;
	
//	if (--ch->persist <= 0) {
//		delete ch->path;
//		ch->path = NULL;
//		return;
//	}
	
	if ((dir = Track(ch)) != -1) {
		vict = ch->path->m_idVictim ? CharData::Find(ch->path->m_idVictim) : NULL;
		dest = ch->path->m_Destination;
			
		perform_move(ch, dir, 0);
		if (vict) {
			
			if (IN_ROOM(ch) == IN_ROOM(vict)) {
				//	Determined mobs: they will wait till you leave the room, then
				//	give chase!!!
				
				relation = ch->GetRelation(vict);
				
				if (((MOB_FLAGGED(ch, MOB_AGGR_ALL) && (relation != RELATION_FRIEND)) ||
							(MOB_FLAGGED(ch, MOB_AGGRESSIVE) && relation == RELATION_ENEMY) ||
							(ch->mob->m_Hates.IsActive() && ch->mob->m_Hates.IsTrue(ch)))
					&& !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
					FoundHated(ch, vict);
					delete ch->path;
					ch->path = NULL;
				}
			}
		} else if (ch->InRoom() == dest) {
			delete ch->path;
			ch->path = NULL;
		}
	}
}


ACMD(do_reload);

void check_mobile_activity(unsigned int pulse) {
	unsigned int	tick = (pulse / PULSE_MOBILE) & TICK_WRAP_COUNT_MASK;
	
	FOREACH(CharacterList, character_list, iter)
	{
		CharData *ch = *iter;

		if (IS_NPC(ch))
		{
			if (GET_MOB_WAIT(ch) > 0)
			{
				GET_MOB_WAIT(ch) -= PULSE_MOBILE;
			}
			
			if (ch->mob->m_UpdateTick == tick)
			{
				if (GET_MOB_WAIT(ch) > 0)
				{
					ch->mob->m_UpdateTick += (GET_MOB_WAIT(ch) + (PULSE_MOBILE - 1)) / PULSE_MOBILE;
					ch->mob->m_UpdateTick &= TICK_WRAP_COUNT_MASK;
				}
				else
				{
					GET_MOB_WAIT(ch) = 0;
					mobile_activity(ch);
				}
			}
		}
	}
}


void mobile_activity(CharData *ch) {
	CharData	*temp;
	int		max, door;
	Relation	relation;
	
	if (PURGED(ch))
	{
		mudlogf(BRF, 102, true, "SYSERR: Mob %s is purged yet receiving mobile_activity", ch->GetName());
		return;
	}
	
	if (ch->InRoom() == NULL) {
		log("Char %s in NOWHERE", ch->GetName());
		ch->ToRoom(world[0]);
	}
	
	//	SPECPROC
	if (!no_specials && ch->GetPrototype() && ch->GetPrototype()->m_Function)
	{
		if (ch->GetPrototype()->m_Function(ch, ch, 0, "") || PURGED(ch))
			return;		/* go to next char */
	}
	
	if (MOB_FLAGGED(ch, MOB_PROGMOB))
		return;

	if (!AWAKE(ch))
		return;
	
	if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
		return;

#if 0
	//	Check if it has a weapon in inventory but not wielded
	if ((ch->GetWeapon() == NULL) && (ch->carrying.Count() > 0)) {
		//	Unarmed!  Wield something!
		if (0) {
			//	Wield it!
			ch->mob->tick = (ch->mob->tick + Number(4,9)) /*% TICK_WRAP_COUNT*/ & TICK_WRAP_COUNT_MASK;
			return;
		}
	}
#endif

	//	Check if it has a gun wielded but unloaded
	{
		ObjData *gun = ch->GetGun();
		
		//	Reload!!
		if (gun && GET_GUN_AMMO_TYPE(gun) != -1 && GET_GUN_AMMO(gun) == 0) {
			do_reload(ch, "", "", SCMD_LOAD);

			//	Re-adjust tick for mob timer - .4-1 second response time
			if (GET_GUN_AMMO(gun) > 0 || PURGED(ch)) {
//				ch->mob->tick = (ch->mob->tick + Number(2,5)) % TICK_WRAP_COUNT;
//				GET_MOB_WAIT(ch) = 1 RL_SEC;
				//	Just make'm wait till next thought
				return;
			}
		}
	}

	//	ASSIST
	//	Helper
	//	Or Random(40) <= loyalty
	if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && !FIGHTING(ch))
	{
		FOREACH(CharacterList, ch->InRoom()->people, iter)
		{
			temp = *iter;
			if ((ch == temp) || !FIGHTING(temp) || (FIGHTING(temp) == ch) ||
					(IN_ROOM(FIGHTING(temp)) != IN_ROOM(ch)) ||
					(!IS_NPC(temp) && ch->m_Following != temp && temp->m_Following != ch) ||
					ch->CanSee(temp) == VISIBLE_NONE)
				continue;
			if ((ch->GetRelation(temp) != RELATION_ENEMY) && (MOB_FLAGGED(ch, MOB_HELPER) ||
					(MOB_FLAGGED(ch, MOB_AI) && Number(1, 10) <= ch->mob->m_AIAggrLoyalty)) &&
					(ch->GetRelation(FIGHTING(temp)) != RELATION_FRIEND)) {
				act("$n jumps to the aid of $N!", FALSE, ch, 0, temp, TO_ROOM);
				initiate_combat(ch, FIGHTING(temp), NULL);
				return;
			}
		}
	}
	
	if (FIGHTING(ch))
		return;
	
	//	It's debatable why I have this check against peacefulness...
	//	The primary reason is because mobs should be smart enough to know
	//	When they are safe from other mobs
	if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
	{
		//	FEAR (normal)
		if ((GET_HIT(ch) < (GET_MAX_HIT(ch) / 2)) && ch->mob->m_Fears.IsActive() && (temp = ch->mob->m_Fears.FindTarget(ch))) {
			do_flee(ch, "", "flee", 0);
			return;
		}
		
		//	HATE
		if (ch->mob->m_Hates.IsActive() && (temp = ch->mob->m_Hates.FindTarget(ch))) {
			FoundHated(ch, temp);
			
			if (FIGHTING(ch) || PURGED(ch))
				return;
		}
		
		//	FEAR (desperate non-haters)
		if (ch->mob->m_Fears.IsActive() && (temp = ch->mob->m_Fears.FindTarget(ch))) {
			do_flee(ch, "", "flee", 0);
			return;
		}
		
		
		//	AGGR
		if (MOB_FLAGGED(ch, MOB_AGGRESSIVE | MOB_AGGR_ALL) ||
				(MOB_FLAGGED(ch, MOB_AI) && ch->mob->m_AIAggrRoom > 0)) {
			FOREACH(CharacterList, ch->InRoom()->people, iter)
			{
				temp = *iter;
				if (ch == temp)
					continue;
				if (ch->CanSee(temp) != VISIBLE_FULL || NO_STAFF_HASSLE(temp))
					continue;
				if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(temp))
					continue;
//				if (MOB_FLAGGED(ch, MOB_AI) && Number(1, 100) > ch->mob->awareRoom)
//					continue;
				if (MOB_FLAGGED(ch, MOB_AI) && Number(1, 100) > ch->mob->m_AIAggrRoom)
					continue;
				relation = ch->GetRelation(temp);
				if ((MOB_FLAGGED(ch, MOB_AGGR_ALL) && (relation != RELATION_FRIEND))
						|| (relation == RELATION_ENEMY))
				{
					initiate_combat(ch, temp, NULL);
					return;
				}
			}
		}
	}
	
	if (PURGED(ch))
		return;
	
	if (ch->path) {
		MobHunt(ch);
		return;
	}
	
	if (MOB_FLAGGED(ch, MOB_AI) ? (ch->mob->m_AIAwareRange > 0 && Number(1,100) <= ch->mob->m_AIAggrRanged) :
			(Number(0,3) == 0))
	{
		if (ShootScan(ch, MOB_FLAGGED(ch, MOB_AI) ? ch->mob->m_AIAwareRange : 5))
			return;
		
		//	If the mob has hates, and does not have a path
		//	then scan for enemies to start looking for
		if (!ch->m_Following && !AFF_FLAGGED(ch, AFF_NOSHOOT) && !MOB_FLAGGED(ch, MOB_SENTINEL) &&
				(MOB_FLAGGED(ch, MOB_AGGRESSIVE | MOB_AGGR_ALL) || ch->mob->m_Hates.IsActive() ||
				(MOB_FLAGGED(ch, MOB_AI) && ch->mob->m_AIAggrRanged > 0)) &&
				!PrefersToShoot(ch)) {
			if ((temp = AggroScan(ch, MOB_FLAGGED(ch, MOB_AI) ? ch->mob->m_AIAwareRange : 5))) {
				ch->path = Path2Char(ch, GET_ID(temp), 200, HUNT_GLOBAL | HUNT_THRU_DOORS);
				MobHunt(ch);
				return;
			}
		}
	}
	
	//	Non-combat stuff

	//	SCAVENGE
	if (MOB_FLAGGED(ch, MOB_SCAVENGER) && !ch->InRoom()->contents.empty() && !Number(0, 10)) {
		max = -1;
		ObjData *best_obj = NULL;
		FOREACH(ObjectList, ch->InRoom()->contents, iter)
		{
			ObjData *obj = *iter;
			if (CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max) {
				best_obj = obj;
				max = GET_OBJ_COST(obj);
			}
		}
		if (best_obj) {
			best_obj->FromRoom();
			best_obj->ToChar(ch);
			act("$n gets $p.", FALSE, ch, best_obj, 0, TO_ROOM);
			return;
		}
	}
	
	//	WANDER
	
	
	if (!ch->m_Following && !MOB_FLAGGED(ch, MOB_SENTINEL) && (GET_POS(ch) == POS_STANDING))
	{
		door = NUM_OF_DIRS;
		if (MOB_FLAGGED(ch, MOB_AI) && ch->mob->m_AIMoveRate > 0)
		{
			if (Number(1, 100) <= ch->mob->m_AIMoveRate)
			{
				int dirCount = 0;
				
				for (door = 0; door < NUM_OF_DIRS; ++door)
					if (RoomData::CanGo(ch, door))
						++dirCount;
				
				dirCount = Number(1, dirCount);
				
				for (door = 0; door < NUM_OF_DIRS; ++door)
					if (RoomData::CanGo(ch, door) && (--dirCount == 0))
						break;
			} 
		}
		else
			door = Number(0, 18);
		
		if (door < NUM_OF_DIRS && RoomData::CanGo(ch, door)
			&& (!ROOM_FLAGGED(Exit::Get(ch, door)->ToRoom(), ROOM_GRAVITY) || AFF_FLAGGED(ch, AFF_FLYING))
			&& !ROOM_FLAGGED(Exit::Get(ch, door)->ToRoom(), ROOM_DEATH)
			&& (!MOB_FLAGGED(ch, MOB_STAY_ZONE) || IsSameEffectiveZone(ch->InRoom(), Exit::Get(ch, door)->ToRoom()))) {
			perform_move(ch, door, 0);
			return;		// You've done enough for now...
		}
	}

	//	SOUNDS ?	
}




void remember(CharData * ch, CharData * victim) {
	if (!IS_NPC(ch) || NO_STAFF_HASSLE(victim))
		return;
	
	ch->mob->m_Hates.Add(Opinion::TypeCharacter, GET_ID(victim));
}


void forget(CharData * ch, CharData * victim) {
	if (!IS_NPC(ch) || !ch->mob->m_Hates.IsActive())
		return;
	
	ch->mob->m_Hates.Remove(Opinion::TypeCharacter, victim->GetID());
}

