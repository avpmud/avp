/* ************************************************************************
*   File: act.movement.c                                Part of CircleMUD *
*  Usage: movement commands, door handling, & sleep/rest/etc state        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "structs.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "event.h"
#include "clans.h"
#include "constants.h"

/* external functs */
bool special(CharData *ch, const char *cmd, char *arg);
void death_cry(CharData *ch);
int find_eq_pos(CharData * ch, ObjData * obj, char *arg);
void add_follower(CharData *ch, CharData *leader);
int greet_mtrigger(CharData *actor, int dir);
int entry_mtrigger(CharData *ch, int dir);
int enter_wtrigger(RoomData *room, CharData *actor, int dir);
int greet_wtrigger(RoomData *room, CharData *actor, int dir);
int enter_otrigger(CharData *actor, RoomData *room, int dir);
int greet_otrigger(CharData *actor, int dir);
int sit_otrigger(ObjData *obj, CharData *actor);
int leave_mtrigger(CharData *actor, int dir);
int leave_otrigger(CharData *actor, int dir);
int leave_wtrigger(RoomData *room, CharData *actor, int dir);
int door_mtrigger(CharData *actor, int subcmd, int dir);
int door_otrigger(CharData *actor, int subcmd, int dir);
int door_wtrigger(CharData *actor, int subcmd, int dir);
void motion_otrigger(CharData *ch, CharData *actor, int dir, int motion, bool lateral, int distance, int type);
void motion_mtrigger(CharData *ch, CharData *actor, int dir, int motion, bool lateral, int distance, int type);
void motion_wtrigger(RoomData *ch, CharData *actor, int dir, int motion, bool lateral, int distance, int type);
void room_motion_otrigger(RoomData *room, CharData *actor, int dir, int motion, bool lateral, int distance, int type);
void check_killer(CharData * ch, CharData * vict);

ACMD(do_mount);

void AlterMove(CharData *ch, int amount);

/* local functions */
int has_boat(CharData *ch);
Direction find_door(CharData *ch, const char *type, const char *dir, const char *cmdname);
bool has_key(CharData *ch, VirtualID key);
int do_enter_vehicle(CharData * ch, char * buf);
ACMD(do_move);
ACMD(do_gen_door);
ACMD(do_enter);
ACMD(do_leave);
ACMD(do_stand);
ACMD(do_sit);
ACMD(do_rest);
ACMD(do_sleep);
ACMD(do_wake);
ACMD(do_follow);
ACMD(do_push_drag);
ACMD(do_recall);
ACMD(do_push_drag_out);		// Expands do_push_drag
ACMD(do_deploy);

void handle_motion_leaving(CharData *ch, int direction, Flags flags);
void handle_motion_entering(CharData *ch, int direction, Flags flags);

/* simple function to determine if char can walk on water */
int has_boat(CharData *ch)
{
	int i;
	
	if ((AFF_FLAGGED(ch, /*AFF_WATERWALK |*/ AFF_FLYING) && !AFF_FLAGGED(ch, AFF_NOQUIT)) || NO_STAFF_HASSLE(ch))
		return 1;

	/* non-wearable boats in inventory will do it */
	FOREACH(ObjectList, ch->carrying, obj)
	{
		if (GET_OBJ_TYPE(*obj) == ITEM_BOAT && (find_eq_pos(ch, *obj, NULL) < 0))
			return 1;
	}

	/* and any boat you're wearing will do it too */
	for (i = 0; i < NUM_WEARS; i++)
		if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_BOAT)
			return 1;

  return 0;
}

  

/* do_simple_move assumes
 *    1. That there is no master and no followers.
 *    2. That the direction exists.
 *
 *   Returns :
 *   1 : If success.
 *   0 : If fail
 */
 
extern int base_response[NUM_RACES+1];
int do_simple_move(CharData *ch, int nDir, Flags flags) {
	RoomData *	nPreviousRoom = ch->InRoom();
	int			nSlowest; //, nSlowestGroup, nFollowers;
	int			nTimeCost = 0;
	
	if (!MOB_FLAGGED(ch, MOB_PROGMOB)) {

//		if (!greet_mtrigger(ch))
//			return 0;
		if (!leave_mtrigger(ch, nDir) || PURGED(ch) || !RoomData::CanGo(ch, nDir))
			return 0;
		if (!leave_otrigger(ch, nDir) || PURGED(ch) || !RoomData::CanGo(ch, nDir))
			return 0;
		if (!leave_wtrigger(ch->InRoom(), ch, nDir) || PURGED(ch) || !RoomData::CanGo(ch, nDir))
			return 0;

		/* charmed? */
		if (AFF_FLAGGED(ch, AFF_CHARM) && ch->m_Following && IN_ROOM(ch) == IN_ROOM(ch->m_Following)) {
			send_to_char("The thought of leaving your master makes you weep.\n", ch);
			act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
			return 0;
		}

		/* if this room or the one we're going to is air, check for fly */
		if (Exit::Get(ch, nDir)->ToRoom()->GetSector() == SECT_FLYING)
		{
			if ((!AFF_FLAGGED(ch, AFF_FLYING) || AFF_FLAGGED(ch, AFF_NOQUIT)) && !NO_STAFF_HASSLE(ch)) {
				send_to_char("You would have to fly to go there.\n", ch);
				return 0;
			}
		}

		/* if this room or the one we're going to needs a boat, check for one */
		if ((IN_ROOM(ch)->GetSector() == SECT_WATER_NOSWIM) ||
				(Exit::Get(ch, nDir)->ToRoom()->GetSector() == SECT_WATER_NOSWIM)) {
			if (!has_boat(ch)) {
				send_to_char("You need a boat to go there.\n", ch);
				return 0;
			}
		}

		/* if this room or the one we're going to is air, check for fly */
#if 0
		if (EXIT(ch, nDir)->ToRoom->GetSector() == SECT_SPACE || EXIT(ch, nDir)->ToRoom->GetSector() == SECT_DEEPSPACE) {
			if (!NO_STAFF_HASSLE(ch)) {
				send_to_char("You need a vehicle to move through space.\n", ch);
				return 0;
			}
		}
#endif
		
		/* move points needed is avg. move loss for src and destination sect type */
		int nMvCost = (movement_loss[ch->InRoom()->GetSector()] + movement_loss[Exit::Get(ch, nDir)->ToRoom()->GetSector()]);
		nTimeCost = (movement_time[ch->InRoom()->GetSector()] + movement_time[Exit::Get(ch, nDir)->ToRoom()->GetSector()]);
		
		
#if 0
		if (AFF_FLAGGED(ch, AFF_FLYING) && (ch->InRoom()->GetSector() != SECT_UNDERWATER) &&
				(Exit::Get(ch, nDir)->ToRoom->GetSector() != SECT_UNDERWATER))
		{	//	Adjust for flying everywhere but underwater
			nMvCost = movement_loss[SECT_FLYING] + movement_loss[SECT_FLYING];
			nTimeCost = movement_time[SECT_FLYING] + movement_time[SECT_FLYING];
		}
		else
#endif
		if (has_boat(ch))
		{	//	Adjust for having boat in swimmable water
			if (ch->InRoom()->GetSector() == SECT_WATER_SWIM)
			{
				nMvCost -= movement_loss[SECT_WATER_SWIM] - movement_loss[SECT_WATER_NOSWIM];
				nTimeCost -= movement_time[SECT_WATER_SWIM] - movement_time[SECT_WATER_NOSWIM];
			}
			if (Exit::Get(ch, nDir)->ToRoom()->GetSector() == SECT_WATER_SWIM)
			{
				nMvCost -= movement_loss[SECT_WATER_SWIM] - movement_loss[SECT_WATER_NOSWIM];
				nTimeCost -= movement_time[SECT_WATER_SWIM] - movement_time[SECT_WATER_NOSWIM];
			}
		}
		
		nMvCost /= 3;	//	Average...
		
		//	FACTOR IN GROUP MOVEMENT COSTS
		
		//	Find slowest of group...
		nSlowest = 50;
#if 0		//	Disabled for now
		nSlowestGroup = 1000;
		
		nFollowers = 0;
		if (AFF_FLAGGED(ch, AFF_GROUP)) {
			CharData *follower;
			START_ITER(iter, follower, CharData *, ch->followers) {
				if (AFF_FLAGGED(follower, AFF_GROUP) &&
						(IN_ROOM(follower) == IN_ROOM(ch)) &&
						(GET_POS(follower) >= POS_STANDING)) {
					nSlowestGroup = MIN(nSlowestGroup, /*GET_QUI(follower)*/ 50);
					nFollowers++;
				}
			}
		}
		
//		nMvCost = nMvCost / GET_TOU(ch) * 2;
		//	Maybe this should be calculated before slowest?
		if (nFollowers > 9)			nSlowest = nSlowest / 4;		//	10+
		else if (nFollowers > 7)	nSlowest = nSlowest / 3;		//	8-9
		else if (nFollowers > 5)	nSlowest = nSlowest / 2;		//	6-7
		else if (nFollowers > 3)	nSlowest = nSlowest * 2 / 3;	//	4-5
		
		if (nFollowers > 0)	nSlowest = MIN(nSlowest, nSlowestGroup);
#endif		
		nTimeCost = nTimeCost / MAX(1, nSlowest);
		
		if ((GET_MOVE(ch) < nMvCost) && !IS_NPC(ch) && !NO_STAFF_HASSLE(ch))
		{
			if (IS_SET(flags, MOVE_FOLLOWING) && ch->m_Following)
				send_to_char("You are too exhausted to follow.\n", ch);
			else
				send_to_char("You are too exhausted.\n", ch);

			return 0;
		}
		
		if (!House::CanEnter(ch, Exit::Get(ch, nDir)->ToRoom()))
		{
			send_to_char("That's private property -- no trespassing!\n", ch);
			return 0;
		}
		
		if (ROOM_FLAGGED(Exit::Get(ch, nDir)->ToRoom(), ROOM_TUNNEL) &&
				Exit::Get(ch, nDir)->ToRoom()->people.size() > 1) {
			send_to_char("There isn't enough room there for more than two people!\n", ch);
			return 0;
		}
		
		if (!ch->AbortTimer())
			return 0;
		
		if (!NO_STAFF_HASSLE(ch) && !IS_NPC(ch))
			AlterMove(ch, -nMvCost);
		
		if (!enter_wtrigger(Exit::Get(ch, nDir)->ToRoom(), ch, nDir) || PURGED(ch) || !Exit::IsPassable(ch, nDir))
			return 0;
			
		if (!enter_otrigger(ch, Exit::Get(ch, nDir)->ToRoom(), nDir) || PURGED(ch) || !Exit::IsPassable(ch, nDir))
			return 0;
		
		if (IN_ROOM(ch) != nPreviousRoom)
			return 1;	//	Trick, to make FOLLOW work

		if (IS_SET(flags, MOVE_QUIET))
		{
			/* No message */
		}
		else if (!AFF_FLAGGED(ch, AFF_SNEAK) || IS_SET(flags, MOVE_FLEEING))
		{
			act("$n leaves $T.", TRUE, ch, 0, dirs[nDir], TO_ROOM | (IS_SET(flags, MOVE_FLEEING) ? 0 : TO_FULLYVISIBLE_ONLY));
		}
		else
		{
			//	Sneaking
			BUFFER(buf, MAX_STRING_LENGTH);
			sprintf(buf, "$n leaves %s.", dirs[nDir]);
			
			float	curStealth = ch->ModifiedStealth();
		
			FOREACH(CharacterList, ch->InRoom()->people, iter)
			{
				CharData *i = *iter;
				if (i == ch)	continue;
				
				if (ch->GetRelation(i) == RELATION_FRIEND || i->ModifiedNotice(ch) > curStealth)
					act(buf, TRUE, ch, 0, i, TO_VICT | (IS_SET(flags, MOVE_FLEEING) ? 0 : TO_FULLYVISIBLE_ONLY));
			}		
		}
	
		if (!PURGED(ch) && IN_ROOM(ch) == nPreviousRoom)
			handle_motion_leaving(ch, nDir, flags);
	}
	
	if (PURGED(ch) || !Exit::IsPassable(ch, nDir))
		return 0;
	
	if (IN_ROOM(ch) != nPreviousRoom)
		return 1;
	
	ch->FromRoom();
	ch->ToRoom(Exit::Get(nPreviousRoom, nDir)->ToRoom());

	//	(8 to 12) - (0 to 10) + 5
	//	.3 - 1.7 sec
	if (!NO_STAFF_HASSLE(ch))
	{
		WAIT_STATE(ch, nTimeCost);
//		MOVE_WAIT_STATE(ch, nTimeCost);
	}
	
	if (!MOB_FLAGGED(ch, MOB_PROGMOB))
	{
		if (IS_SET(flags, MOVE_QUIET))
		{
			//	No message
		}
		else if (!AFF_FLAGGED(ch, AFF_SNEAK) || IS_SET(flags, MOVE_FLEEING))
			act("$n has arrived from $T.", TRUE, ch, 0, dir_text_the[rev_dir[nDir]], TO_ROOM | (IS_SET(flags, MOVE_FLEEING) ? 0 : TO_FULLYVISIBLE_ONLY));
 		else
 		{
			BUFFER(buf, MAX_STRING_LENGTH);
			sprintf(buf, "$n has arrived from %s.", dir_text_the[rev_dir[nDir]]);
			
			float	curStealth = ch->ModifiedStealth();
			
			FOREACH(CharacterList, ch->InRoom()->people, iter)
			{
				CharData *i = *iter;
				if (i == ch)	continue;

				if (ch->GetRelation(i) == RELATION_FRIEND || i->ModifiedNotice(ch) > curStealth)
					act(buf, TRUE, ch, 0, i, TO_VICT | (IS_SET(flags, MOVE_FLEEING) ? 0 : TO_FULLYVISIBLE_ONLY));
			}
		}
	
		if (!PURGED(ch))
			handle_motion_entering(ch, nDir, flags);
	  
		if (!PURGED(ch) && ch->desc != NULL)
			look_at_room(ch);

		if (!PURGED(ch) && !entry_mtrigger(ch, nDir))
			return 1;	//	0 prevents group follow
		
		if (!PURGED(ch) && !greet_wtrigger(ch->InRoom(), ch, nDir))
			return 1;	//	0 prevents group follow
		if (!PURGED(ch) && !greet_mtrigger(ch, nDir))
			return 1;	//	0 prevents group follow
		if (!PURGED(ch) && !greet_otrigger(ch, nDir))
			return 1;	//	0 prevents group follow
		if (PURGED(ch))
			return 0;
	}
	return 1;
}


int perform_move(CharData *ch, int nDir, Flags flags)
{
#if ROAMING_COMBAT
	if (ch == NULL || nDir < 0 || nDir >= NUM_OF_DIRS || PURGED(ch) /*|| (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))*/)
#else
	if (ch == NULL || nDir < 0 || nDir >= NUM_OF_DIRS || PURGED(ch) || FIGHTING(ch))
#endif
		return 0;
	else if (!RoomData::CanGo(ch, nDir))
	{
		if (!Exit::GetIfValid(ch, nDir))
			send_to_char("Alas, you cannot go that way...\n", ch);
		else if (Exit::IsDoorClosed(ch, nDir))
		{
			if (*Exit::Get(ch, nDir)->GetKeyword())
				act("The $T seems to be closed.", FALSE, ch, 0, fname(Exit::Get(ch, nDir)->GetKeyword()), TO_CHAR);
			else
				send_to_char("It seems to be closed.\n", ch);
		}
		else if (Exit::Get(ch, nDir)->GetStates().test(EX_STATE_BLOCKED))
		{
			if (*Exit::Get(ch, nDir)->GetKeyword())
				act("The $T seems to be blocked.", FALSE, ch, 0, fname(Exit::Get(ch, nDir)->GetKeyword()), TO_CHAR);
			else
				send_to_char("It seems to be blocked.\n", ch);
		}
		else if (AFF_FLAGGED(ch, AFF_NOQUIT) && ROOM_FLAGGED(Exit::Get(ch, nDir)->ToRoom(), ROOM_PEACEFUL))
			send_to_char("You aren't allowed into a safe room if you been in combat recently!\n", ch);
		else if (*Exit::Get(ch, nDir)->GetKeyword())
			act("The $T won't let you through.", FALSE, ch, 0, fname(Exit::Get(ch, nDir)->GetKeyword()), TO_CHAR);
//		else if (IS_NPC(ch) && EXIT_FLAGGED(EXIT(ch, nDir), EX_NOMOB))
//			send_to_char("Alas, you cannot go that way...\n", ch);
		else
			send_to_char("It won't let you through.\n", ch);
	}
	else
	{
//		if (!ch->followers.Count())
//			return (do_simple_move(ch, nDir, bCheckSpecials, false));

		RoomData *was_in = ch->InRoom();
		if (!do_simple_move(ch, nDir, flags) || PURGED(ch))
			return 0;

		FOREACH(CharacterList, ch->m_Followers, iter)
		{
			CharData *follower = *iter;
			
			if ((follower->InRoom() == was_in)
				&& (GET_POS(follower) >= POS_STANDING)
				&& !PRF_FLAGGED(follower, PRF_NOAUTOFOLLOW))
			{
				act("You follow $N.\n", FALSE, follower, 0, ch, TO_CHAR);
				perform_move(follower, nDir, MOVE_FOLLOWING);
			}
		}
		return 1;
	}
	return 0;
}

#if 0
EVENTFUNC(move_after_delay)
{
	GenericEventData *	eventData = (GenericEventData *)event_obj;
	
	eventData->ch->events.Remove(event);
	
	perform_move(eventData->ch, eventData->num, 0);
	
	return 0;
}
#endif


ACMD(do_move) {
	/*
	* This is basically a mapping of cmd numbers to perform_move indices.
	* It cannot be done in perform_move because perform_move is called
	* by other functions which do not require the remapping.
	*/
	if (Event::FindEvent(ch->m_Events, GravityEvent::Type))
		send_to_char("You are falling, and can't grab anything!\n", ch);
/*	else if (CHECK_MOVE_WAIT(ch))
	{
		ch->AddOrReplaceEvent(new move_after_delay, ch->move_wait, ch, NULL, NULL, subcmd - 1);
		WAIT_STATE(ch, ch->move_wait);
	}
*/	else
		perform_move(ch, subcmd - 1, 0);
}


Direction find_door(CharData *ch, const char *type, const char *dir, const char *cmdname)
{
	int door;

	if (*dir)
	{		//	a direction was specified
		door = search_block(dir, dirs, FALSE);
		
		if (door == -1)	//	Partial Match
			send_to_char("That's not a direction.\n", ch);
		else if (Exit::GetIfValid(ch, door))
		{
			if (*Exit::Get(ch, door)->GetKeyword())
			{
				if (isname(type, Exit::Get(ch, door)->GetKeyword()))
					return (Direction)door;
				else
					ch->Send("I see no %s there.", type);
			} else
				return (Direction)door;
		}
		else
			send_to_char("I really don't see how you can close anything there.\n", ch);
	} else {		//	try to locate the keyword
		if (!*type)	ch->Send("What is it you want to %s?\n", cmdname);
		else {
			for (door = 0; door < NUM_OF_DIRS; door++)
			{
				if (Exit::GetIfValid(ch, door) && isname(type, Exit::Get(ch, door)->GetKeyword()))
					return (Direction)door;				
			}


//			if (((door = search_block(dir, dirs, FALSE)) != -1) && EXIT(ch, door)) {	//	Partial Match
//				return door;
//				send_to_char("There is no exit in that direction.\n", ch);
//			} else
			if (((door = search_block(type, dirs, FALSE)) != -1) && Exit::GetIfValid(ch, door))
				return (Direction)door;
			else
				ch->Send("There doesn't seem to be %s %s here.\n", AN(type), type);
		}
	}
	return INVALID_DIRECTION;
}


bool has_key(CharData *ch, VirtualID key)
{
	if (!key.IsValid())
		return false;

	FOREACH(ObjectList, ch->carrying, obj)
	{
		if ((*obj)->GetVirtualID() == key)
			return true;
	}

	if (GET_EQ(ch, WEAR_HAND_R) && (GET_EQ(ch, WEAR_HAND_R)->GetVirtualID() == key))
		return true;
	if (GET_EQ(ch, WEAR_HAND_L) && (GET_EQ(ch, WEAR_HAND_L)->GetVirtualID() == key))
		return true;

	return false;
}



enum DoorActionBits
{
	NEED_OPEN,
	NEED_CLOSED,
	NEED_UNLOCKED,
	NEED_LOCKED,
	NEED_KEY,
	NEED_NOTJAMMED,
	NEED_NOTBREACHED,
	NUM_DOOR_ACTION_FLAGS
};
typedef Lexi::BitFlags<NUM_DOOR_ACTION_FLAGS, DoorActionBits> DoorActionFlags;

char *cmd_door[] =
{
  "open",
  "close",
  "unlock",
  "lock",
  "pick"
};


DoorActionFlags door_action_flags[] =
{
	MAKE_BITSET( DoorActionFlags, NEED_CLOSED, NEED_UNLOCKED, NEED_NOTJAMMED, NEED_NOTBREACHED ),
	MAKE_BITSET( DoorActionFlags, NEED_OPEN, NEED_NOTJAMMED, NEED_NOTBREACHED ),
	MAKE_BITSET( DoorActionFlags, NEED_CLOSED, NEED_LOCKED, NEED_KEY, NEED_NOTBREACHED ),
	MAKE_BITSET( DoorActionFlags, NEED_CLOSED, NEED_UNLOCKED, NEED_KEY, NEED_NOTBREACHED ),
	MAKE_BITSET( DoorActionFlags, NEED_CLOSED, NEED_LOCKED, NEED_NOTBREACHED )
};


static void do_doorcmd(CharData *ch, ObjData *obj, Direction door, int scmd) {
	RoomData *other_room = NULL;
	RoomExit *exit = NULL, *back = NULL;

	if (!door_mtrigger(ch, scmd, door))	return;
	if (!door_otrigger(ch, scmd, door))	return;
	if (!door_wtrigger(ch, scmd, door))	return;
	
	BUFFER(buf, MAX_STRING_LENGTH);
	
	sprintf(buf, "$n %ss ", cmd_door[scmd]);
	if (!obj)
	{
		exit = Exit::Get(ch, door);
		
		other_room = exit->ToRoom();
		
		if (other_room)
		{
			back = Exit::GetIfValid(other_room, rev_dir[door]);
			if (back && back->ToRoom() != ch->InRoom())
				back = NULL;
		}
	}

	switch (scmd) {
		case SCMD_OPEN:
			if (obj)
			{
				REMOVE_BIT(obj->m_Value[OBJVAL_CONTAINER_FLAGS], CONT_CLOSED);	
			}
			else
			{
				exit->GetStates().clear(EX_STATE_CLOSED);
				if (back)	back->GetStates().clear(EX_STATE_CLOSED);
			}
//			send_to_char(OK, ch);
			break;
		case SCMD_CLOSE:
			if (obj)
			{
				SET_BIT(obj->m_Value[OBJVAL_CONTAINER_FLAGS], CONT_CLOSED);	
			}
			else
			{
				exit->GetStates().set(EX_STATE_CLOSED);
				if (back)	back->GetStates().set(EX_STATE_CLOSED);
			}
//			send_to_char(OK, ch);
			break;
		case SCMD_UNLOCK:
			if (obj)
			{
				REMOVE_BIT(obj->m_Value[OBJVAL_CONTAINER_FLAGS], CONT_LOCKED);	
			}
			else
			{
				exit->GetStates().clear(EX_STATE_LOCKED);
				if (back)	back->GetStates().clear(EX_STATE_LOCKED);
			}
//			send_to_char("*Click*\n", ch);
			break;
		case SCMD_LOCK:
			if (obj)
			{
				SET_BIT(obj->m_Value[OBJVAL_CONTAINER_FLAGS], CONT_LOCKED);	
			}
			else
			{
				exit->GetStates().set(EX_STATE_LOCKED);
				if (back)	back->GetStates().set(EX_STATE_LOCKED);
			}
//			send_to_char("*Click*\n", ch);
			break;
		case SCMD_PICK:
			if (obj)
			{
				REMOVE_BIT(obj->m_Value[OBJVAL_CONTAINER_FLAGS], CONT_LOCKED);	
			}
			else
			{
				exit->GetStates().clear(EX_STATE_LOCKED);
				if (back)	back->GetStates().clear(EX_STATE_LOCKED);
			}
			
			send_to_char("The lock quickly yields to your skills.\n", ch);
			strcpy(buf, "$n skillfully picks the lock on ");
			break;
	}
	if (scmd != SCMD_PICK)
		act(obj ? "You $t $P." : "You $t the $F.", FALSE, ch, (ObjData *)cmd_door[scmd],
				obj ? obj : (void *)exit->GetKeyword(), TO_CHAR);

	/* Notify the room */
	sprintf_cat(buf, "%s%s.", ((obj) ? "" : "the "), (obj) ? "$p" : (*exit->GetKeyword() ? "$F" : "door"));
	if (!obj || obj->InRoom())
		act(buf, FALSE, ch, obj, obj ? 0 : const_cast<char *>(exit->GetKeyword()), TO_ROOM);

	/* Notify the other room */
	if ((scmd == SCMD_OPEN || scmd == SCMD_CLOSE) && back && !exit->ToRoom()->people.empty())
	{
		sprintf(buf, "The %s is %s%s from the other side.\n",
				fname(back->GetKeyword()), cmd_door[scmd],
				(scmd == SCMD_CLOSE) ? "d" : "ed");
		act(buf, FALSE, exit->ToRoom()->people.front(), 0, 0, TO_ROOM | TO_CHAR);
	}
}


static bool ok_pick(CharData *ch, VirtualID keynum, int pickproof) {
//	int percent = Number(1, 101);

	if (!keynum.IsValid())	ch->Send("Odd - you can't seem to find a keyhole.\n");
	else if (pickproof)		ch->Send("It resists your attempts to pick it.\n");
//	else if (percent > GET_SKILL(ch, SKILL_PICK_LOCK))
	else					ch->Send("You failed to pick the lock.\n");
//	else				return true;
	return false;
}


static VirtualID GetDoorOrObjectKey(CharData *ch, ObjData *obj, RoomExit *exit)
{
	if (obj)	return obj->GetVIDValue(OBJVIDVAL_CONTAINER_KEY);
	else		return exit->key;
}

static bool IsDoorOrObjectOpenable(CharData *ch, ObjData *obj, RoomExit *exit)
{
	if (obj)	return obj->GetType() == ITEM_CONTAINER && IS_SET(obj->GetValue(OBJVAL_CONTAINER_FLAGS), CONT_CLOSEABLE);
	else		return EXIT_FLAGGED(exit, EX_ISDOOR);
}

static bool IsDoorOrObjectClosed(CharData *ch, ObjData *obj, RoomExit *exit)
{
	if (obj)	return IS_SET(obj->GetValue(OBJVAL_CONTAINER_FLAGS), CONT_CLOSED);
	else		return exit->GetStates().test(EX_STATE_CLOSED);
}

static bool IsDoorOrObjectOpen(CharData *ch, ObjData *obj, RoomExit *exit)
{
	return !IsDoorOrObjectClosed(ch, obj, exit);
}

static bool IsDoorOrObjectLocked(CharData *ch, ObjData *obj, RoomExit *exit)
{
	if (obj)	return IS_SET(obj->GetValue(OBJVAL_CONTAINER_FLAGS), CONT_LOCKED);
	else		return exit->GetStates().test(EX_STATE_LOCKED);
}

static bool IsDoorOrObjectPickproof(CharData *ch, ObjData *obj, RoomExit *exit)
{
	if (obj)	return IS_SET(obj->GetValue(OBJVAL_CONTAINER_FLAGS), CONT_PICKPROOF);
	else		return EXIT_FLAGGED(exit, EX_PICKPROOF);
}

static bool IsDoorOrObjectUnlocked(CharData *ch, ObjData *obj, RoomExit *exit)
{
	return !IsDoorOrObjectLocked(ch, obj, exit);
}


static bool IsDoorOrObjectBreached(CharData *ch, ObjData *obj, RoomExit *exit)
{
	return !obj && exit->GetStates().test(EX_STATE_BREACHED);
}


static bool IsDoorOrObjectJammed(CharData *ch, ObjData *obj, RoomExit *exit)
{
	return !obj && exit->GetStates().test(EX_STATE_JAMMED);
}


ACMD(do_gen_door) {
//    ShipData *ship;
	Direction door = INVALID_DIRECTION;
	ObjData *obj = NULL;

	skip_spaces(argument);
	if (!*argument) {
		ch->Send("%s what?\n", cmd_door[subcmd]);
		return;
	}
	
/*	if ((ship = ship_in_room(IN_ROOM(ch), argument)) != NULL) {
		if (ship->state != SHIP_DOCKED) {
			ch->Send("That ship has already started to launch.\n");
			return;
		}
		if (!check_pilot(ch, ship)) {
			ch->Send("That's not your ship!\n");
			return;
		}
		if (subcmd == SCMD_OPEN && !ship->hatchopen) {
			ship->hatchopen = TRUE;
			act("You open the hatch on $T.", TRUE, ch, 0, ship->name, TO_CHAR);
			act("$n opens the hatch on $T.", TRUE, ch, 0, ship->name, TO_ROOM);
			echo_to_room(ship->entrance, "The hatch opens from the outside.`n");
			return;
		} else if (subcmd == SCMD_CLOSE && ship->hatchopen) {
			ship->hatchopen = FALSE;
			act("You close the hatch on $T.", TRUE, ch, 0, ship->name, TO_CHAR);
			act("$n closes the hatch on $T.", TRUE, ch, 0, ship->name, TO_ROOM);
			echo_to_room(ship->entrance, "The hatch closes from the outside.`n");
			return;
		}
		ch->Send("Its already %s.\n", cmd_door[subcmd]);
		return;
	}
*/
	BUFFER(type, MAX_INPUT_LENGTH);
	BUFFER(dir, MAX_INPUT_LENGTH);
	two_arguments(argument, type, dir);
	
	if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, NULL, &obj))
		door = find_door(ch, type, dir, cmd_door[subcmd]);
	
	if (obj || (door >= 0))
	{
		RoomExit *exit = (door >= 0) ? Exit::Get(ch, door) : NULL;
		
		VirtualID keynum = GetDoorOrObjectKey(ch, obj, exit);
		
		DoorActionFlags &actionFlags = door_action_flags[subcmd];
		
		if (!(IsDoorOrObjectOpenable(ch, obj, exit)))
			act("You can't $F that!", FALSE, ch, 0, cmd_door[subcmd], TO_CHAR);
		else if (actionFlags.test(NEED_OPEN) && !IsDoorOrObjectOpen(ch, obj, exit))
			send_to_char("But it's already closed!\n", ch);
		else if (actionFlags.test(NEED_CLOSED) && IsDoorOrObjectOpen(ch, obj, exit))
			send_to_char("But it's currently open!\n", ch);
		else if (actionFlags.test(NEED_LOCKED) && !IsDoorOrObjectLocked(ch, obj, exit))
			send_to_char("Oh.. it wasn't locked, after all..\n", ch);
		else if (actionFlags.test(NEED_UNLOCKED) && IsDoorOrObjectLocked(ch, obj, exit))
			send_to_char("It seems to be locked.\n", ch);
		else if (actionFlags.test(NEED_KEY) && !IS_STAFF(ch) && !has_key(ch, keynum))
			send_to_char("You don't seem to have the proper key.\n", ch);
		else if (actionFlags.test(NEED_NOTBREACHED) && IsDoorOrObjectBreached(ch, obj, exit))
			ch->Send("There isn't anything left to %s.\n", cmd_door[subcmd]);
		else if (actionFlags.test(NEED_NOTJAMMED) && IsDoorOrObjectJammed(ch, obj, exit))
			send_to_char("It seems to be stuck.\n", ch);
		else if (subcmd == SCMD_PICK && !ok_pick(ch, keynum, IsDoorOrObjectPickproof(ch, obj, exit)))
			{}	//	Do Nothing
		else
			do_doorcmd(ch, obj, door, subcmd);
	}
	return;
}


static void do_enter_vehicle_individual(CharData *ch, ObjData *vehicle, RoomData *vehicle_room)
{
	RoomData *oldRoom;
	
	if (!vehicle || PURGED(vehicle) || !vehicle_room)
		return;
	
	if (AFF_FLAGGED(ch, AFF_NOQUIT) && ROOM_FLAGGED(vehicle_room, ROOM_PEACEFUL))
	{
		send_to_char("You aren't allowed into a safe room if you fought recently!\n", ch);
		return;
	}
						
	if (!leave_mtrigger(ch, -1) || PURGED(ch) || PURGED(vehicle))
		return;
	if (!leave_otrigger(ch, -1) || PURGED(ch) || PURGED(vehicle))
		return;
	if (!leave_wtrigger(ch->InRoom(), ch, -1) || PURGED(ch) || PURGED(vehicle))
		return;
	if (!enter_wtrigger(vehicle_room, ch, -1) || PURGED(ch) || PURGED(vehicle))
		return;
	if (!enter_otrigger(ch, vehicle_room, -1) || PURGED(ch) || PURGED(vehicle))
		return;
	
	act("You climb into $p.", TRUE, ch, vehicle, 0, TO_CHAR);
	act("$n climbs into $p", TRUE, ch, vehicle, 0, TO_ROOM);
	oldRoom = IN_ROOM(ch);
	ch->FromRoom();
	ch->ToRoom(vehicle_room);
	act("$n climbs in.", TRUE, ch, 0, 0, TO_ROOM);
	
	if (ch->desc)
		look_at_room(ch);
	
	FOREACH(CharacterList, ch->m_Followers, iter)
	{
		CharData *follower = *iter;

		if (/*AFF_FLAGGED(follower, AFF_GROUP) && (follower != ch) &&*/
			IN_ROOM(follower) == oldRoom
			&& GET_POS(follower) >= POS_STANDING
			&& !PRF_FLAGGED(follower, PRF_NOAUTOFOLLOW))
		{
			do_enter_vehicle_individual(follower, vehicle, vehicle_room);
		}
		if (PURGED(vehicle))
			break;
	}
}


int do_enter_vehicle(CharData * ch, char *buf) {
	ObjData *	vehicle = get_obj_in_list_vis(ch, buf, ch->InRoom()->contents);
	RoomData *	vehicle_room;
	
	if (vehicle)
	{
		VirtualID	vid = vehicle->GetVIDValue(OBJVIDVAL_VEHICLE_ENTRY);
		
		if (GET_OBJ_TYPE(vehicle) != ITEM_VEHICLE || !vid.IsValid())
			act("You cannot enter $p.", TRUE, ch, vehicle, 0, TO_CHAR);
		else if (IS_MOUNTABLE(vehicle))
			act("You can ride $p, but not get into it.", TRUE, ch, vehicle, 0, TO_CHAR);
		else if ((vehicle_room = world.Find(vid)) == NULL)
			send_to_char("Serious problem: Vehicle has no insides!", ch);
		else {
			do_enter_vehicle_individual(ch, vehicle, vehicle_room);
		}
		return 1;
	}
	return 0;
}


ACMD(do_enter) {
	BUFFER(buf, MAX_INPUT_LENGTH);
	int door;
//    ShipData *ship;
	
	one_argument(argument, buf);

/*
    ship = ship_in_room(ch->InRoom(), buf);

	if (ship)
	{
		RoomData *	room= ship->entrance;

		if (room)
		{
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
			ch->ToRoom(room);
			act("$n enters the ship.", TRUE, ch, 0, 0, TO_ROOM);
			look_at_room(ch, 0);
		}
		else
			ch->Send("That ship has no entrance.\n");
		return;
	}
*/
	if (*buf) {			/* an argument was supplied, search for door keyword */
		for (door = 0; door < NUM_OF_DIRS; door++)
			if (Exit::GetIfValid(ch, door) && !str_cmp(Exit::Get(ch, door)->GetKeyword(), buf))
			{
				perform_move(ch, door, 0);
				return;
			}
		//	No door was found.  Search for a vehicle
		if (!do_enter_vehicle(ch, buf))
			ch->Send("There is no %s here.\n", buf);
	} else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS))
		send_to_char("You are already indoors.", ch);
	else {	//	try to locate an entrance
		for (door = 0; door < NUM_OF_DIRS; door++)
		{
			if (Exit::IsPassable(ch, door)
				&& ROOM_FLAGGED(Exit::Get(ch, door)->ToRoom(), ROOM_INDOORS))
			{
				perform_move(ch, door, 0);
				return;
			}
		}
		//	No door was found.  Search for a vehicle
		if (!do_enter_vehicle(ch, buf))
			send_to_char("You can't seem to find anything to enter.\n", ch);
	}
}


static void do_leave_vehicle_individual(CharData *ch, ObjData *vehicle)
{
	RoomData *oldRoom;
		
	if (!vehicle || PURGED(vehicle))
		return;
	
	if (AFF_FLAGGED(ch, AFF_NOQUIT) && ROOM_FLAGGED(IN_ROOM(vehicle), ROOM_PEACEFUL))
	{
		send_to_char("You aren't allowed into a safe room if you fought recently!\n", ch);
		return;
	}

	if (!leave_wtrigger(ch->InRoom(), ch, -1) || PURGED(ch) || PURGED(vehicle))
		return;
	if (!enter_wtrigger(vehicle->InRoom(), ch, -1) || PURGED(ch) || PURGED(vehicle))
		return;
				
	act("$n leaves $p.", TRUE, ch, vehicle, 0, TO_ROOM);
	act("You climb out of $p.", TRUE, ch, vehicle, 0, TO_CHAR);
	oldRoom = ch->InRoom();
	ch->FromRoom();
	ch->ToRoom(IN_ROOM(vehicle));
	act("$n climbs out of $p.", TRUE, ch, vehicle, 0, TO_ROOM);
	
	if (ch->desc)
		look_at_room(ch);
	
	FOREACH(CharacterList, ch->m_Followers, iter)
	{
		CharData *follower = *iter;
		
		if (/*AFF_FLAGGED(follower, AFF_GROUP) && (follower != ch) &&*/
			IN_ROOM(follower) == oldRoom
			&& GET_POS(follower) >= POS_STANDING
			&& !PRF_FLAGGED(follower, PRF_NOAUTOFOLLOW))
		{
			do_leave_vehicle_individual(follower, vehicle);
		}
		if (PURGED(vehicle))
			break;
	}
}



ACMD(do_leave) {
	int door;
	ObjData * hatch, * vehicle;

/*	ShipData *ship = ship_from_entrance(IN_ROOM(ch));
	if (ship)
	{
		if (ship->shipclass == SPACE_STATION) {
			ch->Send("You cannot do that here.\n");
			return;
		}

		if (ship->lastdoc != ship->location) {
			ch->Send("`rMaybe you should wait until the ship lands.`n\n");
			return;
		}

		if (ship->state != SHIP_DOCKED) {
			ch->Send("`rPlease wait till the ship is properly docked.`n\n");
			return;
		}

		if (!ship->hatchopen) {
			ch->Send("`RYou need to open the hatch first.`n\n");
			return;
		}

		RoomData *room = ship->InRoom();
		if (room)
		{
			ch->Send("You exit the ship.\n");
			act("$n exits the ship.", TRUE, ch, 0, 0, TO_ROOM);
			ch->FromRoom();
			ch->ToRoom(room);
			act("$n steps out of $T.", TRUE, ch, 0, ship->name, TO_ROOM);
			look_at_room(ch, 0);
		} else
			ch->Send("The exit doesn't seem to be working properly.\n");
		return;
	}
*/
	/* FIRST see if there is a hatch in the room */
	hatch = get_obj_in_list_type(ITEM_VEHICLE_HATCH, ch->InRoom()->contents);
	if (hatch)
	{
		vehicle = find_vehicle_by_vid(hatch->GetVIDValue(OBJVIDVAL_VEHICLEITEM_VEHICLE));
		if (!vehicle || !vehicle->InRoom()) {
			send_to_char("The hatch seems to lead to nowhere...\n", ch);
			return;
		}
		
		do_leave_vehicle_individual(ch, vehicle);
		
		return;
	}
	if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS))
		send_to_char("You are outside... where do you want to go?\n", ch);
	else {
		for (door = 0; door < NUM_OF_DIRS; door++)
		{
			if (Exit::IsPassable(ch, door)
				&& !ROOM_FLAGGED(Exit::Get(ch, door)->ToRoom(), ROOM_INDOORS))
			{
				perform_move(ch, door, 0);
				return;
			}
		}
		send_to_char("I see no obvious exits to the outside.\n", ch);
	}
}


ACMD(do_stand)
{
  ACMD(do_unmount);
  
  if (AFF_FLAGGED(ch, AFF_TRAPPED))
  {
    act("You are trapped, and can't stand up.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
    
  if (ch->sitting_on) {
  	if (IN_ROOM(ch->sitting_on) == IN_ROOM(ch)) {
  	  if(IS_MOUNTABLE(ch->sitting_on)) {
  	  	do_unmount(ch, "", "unmount", 0);
  	  	return;
  	  }
  	  switch (GET_POS(ch)) {
		  case POS_SITTING:
		    act("You stop sitting on $p and stand up.",
		    		FALSE, ch, ch->sitting_on, 0, TO_CHAR);
		    act("$n stops sitting on $p and clambers to $s feet.",
		    		FALSE, ch, ch->sitting_on, 0, TO_ROOM);
		    GET_POS(ch) = POS_STANDING;
			ch->sitting_on = NULL;
		    return;
		  case POS_RESTING:
		    act("You stop resting on $p and stand up.",
		    		FALSE, ch, ch->sitting_on, 0, TO_CHAR);
		    act("$n stops resting on $p and clambers to $s feet.",
		    		FALSE, ch, ch->sitting_on, 0, TO_ROOM);
		    GET_POS(ch) = POS_STANDING;
			ch->sitting_on = NULL;
		    return;
		  default:
		    break;
  	  }
	}
  }
  switch (GET_POS(ch)) {
  case POS_STANDING:
    act("You are already standing.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_SITTING:
    act("You stand up.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    break;
  case POS_RESTING:
    act("You stop resting, and stand up.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops resting, and clambers on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    break;
  case POS_SLEEPING:
    act("You have to wake up first!", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_FIGHTING:
    act("Do you not consider fighting as standing?", FALSE, ch, 0, 0, TO_CHAR);
    break;
  default:
    act("You stop floating around, and put your feet on the ground.",
	FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops floating around, and puts $s feet on the ground.",
	TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    break;
  }
}


ACMD(do_sit)
{
  BUFFER(arg, MAX_INPUT_LENGTH);
  ObjData *obj = NULL;
  
  one_argument(argument, arg);
  
  if (*arg) {
	obj = get_obj_in_list_vis(ch, arg, ch->InRoom()->contents);
	if ((obj) && (GET_POS(ch) >= POS_STANDING))
		if (IS_MOUNTABLE(obj)) {
			do_mount(ch, argument, "mount", 0);
			return;
		} else if (IS_SITTABLE(obj)) {
			if (get_num_chars_on_obj(obj) >= obj->GetValue(OBJVAL_FURNITURE_CAPACITY)) {
				act("There is no more room on $p.", FALSE, ch, obj, 0, TO_CHAR);
				return;
			} else if (!sit_otrigger(obj, ch))
				return;
		} else {
			act("You can't sit on $p.", FALSE, ch, obj, 0, TO_CHAR);
			return;
		}
	}
  
	switch (GET_POS(ch)) {
		case POS_STANDING:
			if (obj) {
				act("You sit on $p.", FALSE, ch, obj, 0, TO_CHAR);
				act("$n sits on $p.", FALSE, ch, obj, 0, TO_ROOM);
				ch->sitting_on = obj;
			} else {
				act("You sit down.", FALSE, ch, 0, 0, TO_CHAR);
				act("$n sits down.", FALSE, ch, 0, 0, TO_ROOM);
			}
			GET_POS(ch) = POS_SITTING;
			break;
		case POS_SITTING:
			send_to_char("You're sitting already.\n", ch);
			break;
		case POS_RESTING:
			act("You stop resting, and sit up.", FALSE, ch, 0, 0, TO_CHAR);
			act("$n stops resting.", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_SITTING;
			break;
		case POS_SLEEPING:
			act("You have to wake up first.", FALSE, ch, 0, 0, TO_CHAR);
			break;
		case POS_FIGHTING:
			act("Sit down while fighting? are you MAD?", FALSE, ch, 0, 0, TO_CHAR);
			break;
		default:
			if (obj) {
				act("You stop floating around, and sit on $p.", FALSE, ch, obj, 0, TO_CHAR);
				act("$n stops floating around, and sits on $p.", TRUE, ch, obj, 0, TO_ROOM);
				ch->sitting_on = obj;
			} else {
				act("You stop floating around, and sit down.", FALSE, ch, 0, 0, TO_CHAR);
				act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, TO_ROOM);
			}
			GET_POS(ch) = POS_SITTING;
			break;
	}
}


ACMD(do_rest) {
  BUFFER(arg, MAX_INPUT_LENGTH);
  ObjData *obj = NULL;
  
  one_argument(argument, arg);
  
  if ((*arg) && (GET_POS(ch) != POS_SITTING)) {
	obj = get_obj_in_list_vis(ch, arg, ch->InRoom()->contents);
	if ((obj) && (GET_POS(ch) >= POS_STANDING))
		if (IS_SITTABLE(obj)) {
			if (get_num_chars_on_obj(obj) >= obj->GetValue(OBJVAL_FURNITURE_CAPACITY)) {
					act("There is no more room on $p.", FALSE, ch, obj, 0, TO_CHAR);
					return;
			} else if (!sit_otrigger(obj, ch))
				return;
		} else {
			act("You can't rest on $p.", FALSE, ch, obj, 0, TO_CHAR);
			return;
		}
  }
    
  switch (GET_POS(ch)) {
  case POS_STANDING:
  	if (obj) {
	    act("You sit down on $p and rest your tired bones.", FALSE, ch, obj, 0, TO_CHAR);
	    act("$n sits down on $p and rests.", TRUE, ch, obj, 0, TO_ROOM);
		ch->sitting_on = obj;
  	} else {
	    act("You sit down and rest your tired bones.", FALSE, ch, 0, 0, TO_CHAR);
	    act("$n sits down and rests.", TRUE, ch, 0, 0, TO_ROOM);
    }
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_SITTING:
    if (ch->sitting_on)
    	if(IS_MOUNTABLE(ch->sitting_on)) {
    		act("You can't rest on $p.", TRUE, ch, ch->sitting_on, 0, TO_CHAR);
    		return;
    	}
    act("You rest your tired bones.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n rests.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_RESTING:
    act("You are already resting.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_SLEEPING:
    act("You have to wake up first.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_FIGHTING:
    act("Rest while fighting?  Are you MAD?", FALSE, ch, 0, 0, TO_CHAR);
    break;
  default:
  	if (obj) {
	    act("You stop floating around, and stop to rest your tired bones on $p.",
				FALSE, ch, obj, 0, TO_CHAR);
	    act("$n stops floating around, and rests on $p.", FALSE, ch, obj, 0, TO_ROOM);
		ch->sitting_on = obj;
  	} else {
	    act("You stop floating around, and stop to rest your tired bones.",
				FALSE, ch, 0, 0, TO_CHAR);
	    act("$n stops floating around, and rests.", FALSE, ch, 0, 0, TO_ROOM);
    }
    GET_POS(ch) = POS_SITTING;
    break;
  }
}


ACMD(do_sleep) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	ObjData *obj = NULL;

	one_argument(argument, arg);

	if ((ch->sitting_on) && (IN_ROOM(ch) == IN_ROOM(ch->sitting_on)) && !IS_SLEEPABLE(ch->sitting_on)) {
		act("You can't sleep on $p.", FALSE, ch, ch->sitting_on, 0, TO_CHAR);
		return;
	}
	
	if (*arg) {
		obj = get_obj_in_list_vis(ch, arg, ch->InRoom()->contents);
		if (obj && !IS_SLEEPABLE(obj)) {
			act("You won't be able to sleep on $p.", FALSE, ch, obj, 0, TO_CHAR);
			return;
		}
		if (obj && (GET_POS(ch) >= POS_STANDING)) {
			if (GET_OBJ_TYPE(obj) != ITEM_BED) {
				act("You can't sleep on $p.", FALSE, ch, obj, 0, TO_CHAR);
				return;
			} else if (get_num_chars_on_obj(obj) >= obj->GetValue(OBJVAL_FURNITURE_CAPACITY)) {
				act("There is no more room on $p.", FALSE, ch, obj, 0, TO_CHAR);
				return;
			} else if (!sit_otrigger(obj, ch) || PURGED(ch))
				return;
			else if (PURGED(obj))
				obj = NULL;		//	In case the script purged the object, but we still let the
								//	player sleep
		}
	}

	switch (GET_POS(ch)) {
		case POS_SITTING:
		case POS_RESTING:
			if (obj && ch->sitting_on && (IN_ROOM(ch->sitting_on) == IN_ROOM(ch)) && (ch->sitting_on != obj)) {
				act("You are already $T on $p.", FALSE, ch, ch->sitting_on,
						(GET_POS(ch) == POS_SITTING) ? "sitting" : "resting", TO_CHAR);
				return;
			}
		case POS_STANDING:
			if (obj) {
				act("You lie down on $p and go to sleep.",
				FALSE, ch, obj, 0, TO_CHAR);
				act("$n lies down on $p and falls asleep.", FALSE, ch, obj, 0, TO_ROOM);
				ch->sitting_on = obj;
			} else {
				send_to_char("You go to sleep.\n", ch);
				act("$n lies down and falls asleep.", TRUE, ch, 0, 0, TO_ROOM);
			}
			GET_POS(ch) = POS_SLEEPING;
			break;
		case POS_SLEEPING:
			send_to_char("You are already sound asleep.\n", ch);
			break;
		case POS_FIGHTING:
			send_to_char("Sleep while fighting?  Are you MAD?\n", ch);
			break;
		default:
			if (obj) {
				act("You stop floating around, and fall asleep on $p.",
						FALSE, ch, obj, 0, TO_CHAR);
				act("$n stops floating around, and falls asleep on $p.", FALSE, ch, obj, 0, TO_ROOM);
				ch->sitting_on = obj;
			} else {
				act("You stop floating around, and lie down to sleep.",
						FALSE, ch, 0, 0, TO_CHAR);
				act("$n stops floating around, and lie down to sleep.",
						TRUE, ch, 0, 0, TO_ROOM);
			}
			GET_POS(ch) = POS_SLEEPING;
			break;
	}
}


ACMD(do_wake) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	CharData *vict;
	int self = 0;
	
	one_argument(argument, arg);
	
	if (*arg) {
		if (GET_POS(ch) == POS_SLEEPING)
			send_to_char("Maybe you should wake yourself up first.\n", ch);
		else if ((vict = get_char_vis(ch, arg, FIND_CHAR_ROOM)) == NULL)
			send_to_char(NOPERSON, ch);
		else if (vict == ch)
			self = 1;
		else if (GET_POS(vict) > POS_SLEEPING)
			act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
		else if (AFF_FLAGGED(vict, AFF_SLEEP))
			act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
		else if (GET_POS(vict) < POS_SLEEPING)
			act("$E's in pretty bad shape!", FALSE, ch, 0, vict, TO_CHAR);
		else if (!IS_STAFF(ch))
			send_to_char("I think not.\n", ch);
		else 
		{
			act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
			act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
			act("$n wakes $N up.", FALSE, ch, 0, vict, TO_NOTVICT);
			GET_POS(vict) = POS_SITTING;
		}
		if (!self) {
			return;
		}
	}
	
	if (AFF_FLAGGED(ch, AFF_SLEEP))			send_to_char("You can't wake up!\n", ch);
	else if (GET_POS(ch) > POS_SLEEPING)	send_to_char("You are already awake...\n", ch);
	else {
		send_to_char("You awaken, and sit up.\n", ch);
		act("$n awakens.", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_SITTING;
	}
}


ACMD(do_follow) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	CharData *leader;

	one_argument(argument, arg);

	if (!*arg)
		send_to_char("Whom do you wish to follow?\n", ch);
	else if (!(leader = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
		send_to_char(NOPERSON, ch);
	else if (ch->m_Following == leader)
		act("You are already following $M.", FALSE, ch, 0, leader, TO_CHAR);
	else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->m_Following))
		act("But you only feel like following $N!", FALSE, ch, 0, ch->m_Following, TO_CHAR);
	else if (leader == ch) {			/* Not Charmed follow person */
		if (!ch->m_Following)	send_to_char("You are already following yourself.\n", ch);
		else				ch->stop_follower();
	} else if (circle_follow(ch, leader))
		send_to_char("Sorry, but following in loops is not allowed.\n", ch);
	else if (ch->GetRelation(leader) == RELATION_ENEMY)
		send_to_char("You cannot follow an enemy!\n", ch);
	else {
		if (ch->m_Following)
			ch->stop_follower();
		AFF_FLAGS(ch).clear(AFF_GROUP);
		add_follower(ch, leader);
	}
}


#define CAN_DRAG_CHAR(ch, vict)	1
//((CAN_CARRY_W(ch)) >= (GET_WEIGHT(vict) + IS_CARRYING_W(vict)))
#define CAN_PUSH_CHAR(ch, vict)	1
//((CAN_CARRY_W(ch) /** 2*/) >= (GET_WEIGHT(vict) + IS_CARRYING_W(vict)))
#define CAN_DRAG_OBJ(ch, obj)	((CAN_CARRY_W(ch) * 2) >= GET_OBJ_TOTAL_WEIGHT(obj))
#define CAN_PUSH_OBJ(ch, obj)	((CAN_CARRY_W(ch) /* * 2*/) >= GET_OBJ_TOTAL_WEIGHT(obj))

ACMD(do_push_drag) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	ObjData *	obj = NULL;
	CharData *	vict;
	int			dir;

	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && !NO_STAFF_HASSLE(ch)) {
		ch->Send("But its so peaceful here...");
		return;
	}
		
	two_arguments(argument, arg, arg2);

	if (!*arg)
		ch->Send("Move what?\n");
	else if (!*arg2)
		ch->Send("Move it in what direction?\n");
	else if (is_abbrev(arg2, "out"))
		do_push_drag_out(ch, arg, command, subcmd);
	else if ((dir = search_block(arg2, dirs, FALSE)) < 0 || dir > NUM_OF_DIRS)
		ch->Send("That's not a valid direction.\n");
	else if (!RoomData::CanGo(ch, dir))
		ch->Send("You can't go that way.\n");
	else if (dir == UP)
		ch->Send("No pushing people up, llama.\n");
	else {
		RoomData *dest = Exit::Get(ch, dir)->ToRoom();
		int mode = 1;
		
		if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
			if (!(obj = get_obj_in_list_vis(ch, arg, ch->InRoom()->contents))) {
				ch->Send("There is nothing like that here.\n");
				return;
			}
			mode = 0;
		} else if (vict == ch) {
			ch->Send("Very funny...\n");
			return;
		}
		else if (IS_NPC(vict))
		{
			ch->Send("You can't push mobs.\n");
			return;
		}
		else if (!PRF_FLAGGED(vict, PRF_PK))
		{
			ch->Send("You can't push no-pk players.\n");
			return;
		}
		else if (!PRF_FLAGGED(ch, PRF_PK))
		{
			ch->Send("You can't push players while no-pk.\n");
			return;
		}
		else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOPK))
		{
			ch->Send("This is a no-pk room.\n");
		}
	
		
		if (!mode) {
			if (!OBJ_FLAGGED(obj, ITEM_MOVEABLE) ||
					(subcmd == SCMD_PUSH ? !CAN_PUSH_OBJ(ch, obj) : !CAN_DRAG_OBJ(ch, obj))) {
				act("You try to $T $p, but it won't move.", FALSE, ch, obj,
						(subcmd == SCMD_PUSH) ? "push" : "drag", TO_CHAR);
				act("$n tries in vain to move $p.", TRUE, ch,obj, 0, TO_ROOM);
				return;
			}
		} else {
			if (GET_POS(vict) < POS_FIGHTING && subcmd == SCMD_PUSH) {
				act("Trying to push people who are on the ground won't work.", FALSE, ch, 0, 0, TO_CHAR);
				return;
			}
			if (FIGHTING(vict)) {
				act("You can't push or drag people who are fighting!", FALSE, ch, 0, 0, TO_CHAR);
				return;
			}
						
			if (!NO_STAFF_HASSLE(ch)) {
				if (IS_NPC(vict)) {
					if (MOB_FLAGGED(vict, MOB_NOBASH)) {
						act("You try to move $N, but fail.", FALSE, ch, 0, vict, TO_CHAR);
						act("$n tries to move $N, but fails.", FALSE, ch, 0, vict, TO_ROOM);
						return;
					}
					if (MOB_FLAGGED(vict, MOB_SENTINEL)) {
						act("$N won't budge.", FALSE, ch, 0, vict, TO_CHAR);
						act("$n tries to move $N, who won't budge.", FALSE, ch, 0, vict, TO_ROOM);
						return;
					}
					if (EXIT_FLAGGED(Exit::Get(ch, dir), EX_NOMOB) || ROOM_FLAGGED(Exit::Get(ch, dir)->ToRoom(), ROOM_NOMOB)) {
						act("$N can't go that way.", FALSE, ch, 0, vict, TO_CHAR);
						act("$n tries to move $N, who won't budge.", FALSE, ch, 0, vict, TO_ROOM);
						return;
					}
//					act("Due to recent abuse of the PUSH and DRAG commands, you can no longer\n"
//						"do that to MOBs.", FALSE, ch, 0, 0, TO_CHAR);
//					return;
				}
				
				if (!RoomData::CanGo(vict, dir))
				{
					act("You try to move $N, but fail.", FALSE, ch, 0, vict, TO_CHAR);
					act("$n tries to move $N, but fails.", FALSE, ch, 0, vict, TO_NOTVICT);
					act("$n tries to push you, but fails.", FALSE, ch, 0, vict, TO_VICT);
					return;
				}
				if (NO_STAFF_HASSLE(vict)) {
					act("You can't push staff around!", FALSE, ch, 0, vict, TO_CHAR);
					act("$n is trying to push you around.", FALSE, ch, 0, vict, TO_VICT);
					return;
				}
				if ((subcmd == SCMD_PUSH) ? !CAN_PUSH_CHAR(ch, vict) : !CAN_DRAG_CHAR(ch, vict)) {
					if (subcmd == SCMD_PUSH) {
						act("You try to push $N $t, but fail.", FALSE, ch, (ObjData *)dirs[dir], vict, TO_CHAR);
						act("$n tries to push $N $t, but fails.", FALSE, ch, (ObjData *)dirs[dir], vict, TO_NOTVICT);
						act("$n tried to push you $t!", FALSE, ch, (ObjData *)dirs[dir], vict, TO_VICT);
					} else {
						act("You try to push $N $t, but fail.", FALSE, ch, (ObjData *)dirs[dir], vict, TO_CHAR);
						act("$n tries to push $N $t, but fails.", FALSE, ch, (ObjData *)dirs[dir], vict, TO_NOTVICT);
						act("$n grabs on to you and tries to drag you $T, but can't seem to move you!", FALSE, ch, (ObjData *)dirs[dir], vict, TO_VICT);
					}
					return;
				}
			}
		}
	

		if (subcmd == SCMD_DRAG) {
			switch (mode) {
				case 0:
					act("You drag $p $T.", FALSE, ch, obj, dirs[dir], TO_CHAR );
					act("$n drags $p $T.", FALSE, ch, obj, dirs[dir], TO_ROOM );
					//ch->FromRoom();
					//ch->ToRoom(dest);
					if (perform_move(ch, dir, MOVE_QUIET))
					{
						obj->FromRoom();
						obj->ToRoom(dest);
						act("$n drags $p into the room from $T.", FALSE, ch, obj, dir_text_the[rev_dir[dir]], TO_ROOM );
					}
					break;
				case 1:
					act("You drag $N $t.", FALSE, ch, (ObjData *)dirs[dir], vict, TO_CHAR );
					act("$n drags $N $t.", FALSE, ch, (ObjData *)dirs[dir], vict, TO_NOTVICT);
					act("$n drags you $t!", FALSE, ch, (ObjData *)dirs[dir], vict, TO_VICT);
					if (perform_move(ch, dir, MOVE_QUIET))
					{
//						ch->FromRoom();
//						ch->ToRoom(dest);
						
						
						if (vict->AbortTimer() == false)
							break;

						if (!leave_mtrigger(vict, dir) || PURGED(vict))
							break;
						if (!leave_otrigger(vict, dir) || PURGED(vict))
							break;
						if (!leave_wtrigger(vict->InRoom(), vict, dir) || PURGED(vict))
							break;

						if (!enter_wtrigger(dest, vict, dir) || PURGED(vict))
							break;
						if (!enter_otrigger(vict, dest, dir) || PURGED(vict))
							break;
							
						vict->FromRoom();
						vict->ToRoom(dest);

						act("$n drags $N into the room from $t.", FALSE, ch, (ObjData *)dir_text_the[rev_dir[dir]], vict, TO_NOTVICT );

						look_at_room(vict);
						
						if (!entry_mtrigger(vict, dir) || PURGED(vict))
							break;
						
						if (!greet_wtrigger(vict->InRoom(), vict, dir) || PURGED(vict))
							break;
						if (!greet_mtrigger(vict, dir) || PURGED(vict))
							break;
						if (!greet_otrigger(vict, dir) || PURGED(vict))
							break;
						
					}
					break;
			}
			if (!NO_STAFF_HASSLE(ch))
				WAIT_STATE(ch, 2 RL_SEC);
		} else {
			switch (mode) {
				case 0:
					act("You push $p $T.", FALSE, ch, obj, dirs[dir], TO_CHAR );
					act("$n pushes $p $T.", FALSE, ch, obj, dirs[dir], TO_ROOM );
					obj->FromRoom();
					obj->ToRoom(dest);
					act("$p is pushed into the room from $T.", FALSE, 0, obj,
							dir_text_the[rev_dir[dir]], TO_ROOM);
					break;
				case 1:
					if (!vict->AbortTimer())
					{
						if (!PURGED(ch) && !PURGED(vict))
							act("For some reason you can't push $N.", FALSE, ch, 0, vict, TO_CHAR );
						break;
					}
					
					check_killer(ch, vict);

					act("You push $N $t.", FALSE, ch, (ObjData *)dirs[dir], vict, TO_CHAR );
					act("$n pushes $N $t.", FALSE, ch, (ObjData *)dirs[dir], vict, TO_NOTVICT );
					act("$n pushes you $t!", FALSE, ch, (ObjData *)dirs[dir], vict, TO_VICT);

					if (!leave_mtrigger(vict, dir) || PURGED(vict))
						break;
					if (!leave_otrigger(vict, dir) || PURGED(vict))
						break;
					if (!leave_wtrigger(vict->InRoom(), vict, dir) || PURGED(vict))
						break;
						
					if (!enter_wtrigger(dest, vict, dir) || PURGED(vict))
						break;
					if (!enter_otrigger(vict, dest, dir) || PURGED(vict))
						break;
						
					vict->FromRoom();
					vict->ToRoom(dest);

					if (GET_POS(vict) > POS_SITTING) GET_POS(vict) = POS_SITTING;
					
					act("$n is pushed into the room from $T.", FALSE, vict, 0, dir_text_the[rev_dir[dir]], TO_ROOM );
					look_at_room(vict);
						
					if (!entry_mtrigger(vict, dir) || PURGED(vict))
						break;
						
					if (!greet_wtrigger(vict->InRoom(), vict, dir) || PURGED(vict))
						break;
					if (!greet_mtrigger(vict, dir) || PURGED(vict))
						break;
					if (!greet_otrigger(vict, dir) || PURGED(vict))
						break;
						
					break;
			}
		}
		if (!NO_STAFF_HASSLE(ch))
			WAIT_STATE(ch, 4 RL_SEC);
		return;
	}
}


ACMD(do_push_drag_out) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	ObjData * hatch, *vehicle, *obj;
	CharData *vict = NULL;
	int mode;
	
	if (!(hatch = get_obj_in_list_type(ITEM_VEHICLE_HATCH, ch->InRoom()->contents))) {
		act("What?  There is no vehicle exit here.", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}
	
	vehicle = find_vehicle_by_vid(hatch->GetVIDValue(OBJVIDVAL_VEHICLEITEM_VEHICLE));
	if (!vehicle)
	{
		act("The hatch seems to lead to nowhere...", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}

	RoomData *dest = vehicle->InRoom();
	one_argument(argument, arg);
	
	if (!(obj = get_obj_in_list_vis(ch, arg, ch->InRoom()->contents))) {
		if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
			act("There is nothing like that here.", FALSE, ch, 0, 0, TO_CHAR);
			return;
		} else if (vict == ch) {
			act("Very funny...", FALSE, ch, 0, 0, TO_CHAR);
			return;
		} else
			mode = 1;
		if (!NO_STAFF_HASSLE(ch) && !IS_NPC(ch) && !IS_NPC(vict)) {	// Mort pushing mort out
			act("Shya, as if.  Now everyone knows you are a loser.", FALSE, ch, 0, vict, TO_CHAR);
			act("$n tried to push $N OUT...  What a loser!", FALSE, ch, 0, vict, TO_NOTVICT);
			act("$n tried to push you OUT, because $e is such a loser!", FALSE, ch, 0, vict, TO_VICT);
			return;
		}
	} else
		mode = 0;

	if (!mode) {
		if (!OBJ_FLAGGED(obj, ITEM_MOVEABLE) ||
					(subcmd == SCMD_PUSH ? !CAN_PUSH_OBJ(ch, obj) : !CAN_DRAG_OBJ(ch, obj))) {
			act("You try to $T $p, but it won't move.", FALSE, ch, obj,
					(subcmd == SCMD_PUSH) ? "push" : "drag", TO_CHAR);
			act("$n tries in vain to move $p.", TRUE, ch,obj, 0, TO_ROOM);
			return;
		}
	} else {
		if ((GET_POS(vict) < POS_FIGHTING) && (subcmd == SCMD_PUSH)) {
			act("Trying to push people who are on the ground won't work.", FALSE, ch, 0, 0, TO_CHAR);
			return;
		}
		if (!NO_STAFF_HASSLE(ch)) {
			if (IS_NPC(vict)) {
				if (MOB_FLAGGED(vict, MOB_NOBASH)) {
					act("You try to move $N, but fail.", FALSE, ch, 0, vict, TO_CHAR);
					act("$n tries to move $N, but fails.", FALSE, ch, 0, vict, TO_ROOM);
					return;
				}
				if (MOB_FLAGGED(vict, MOB_SENTINEL)) {
					act("$N won't budge.", FALSE, ch, 0, vict, TO_CHAR);
					act("$n tries to move $N, who won't budge.", FALSE, ch, 0, vict, TO_ROOM);
					return;
				}
	/*			if (IS_SET(EXIT(ch, dir)->exit_info, EX_NOMOB)) {
					act("$N can't go that way.", FALSE, ch, 0, vict, TO_CHAR);
					act("$n tries to move $N, who won't budge.", FALSE, ch, 0, vict, TO_ROOM);
					return;
				}*/
			}
			if (NO_STAFF_HASSLE(vict)) {
				act("You can't push staff around!", FALSE, ch, 0, vict, TO_CHAR);
				act("$n is trying to push you around.", FALSE, ch, 0, vict, TO_VICT);
				return;
			}
			if ((subcmd == SCMD_PUSH) ? !CAN_PUSH_CHAR(ch, vict) : !CAN_DRAG_CHAR(ch, vict)) {
				if (subcmd == SCMD_PUSH) {
					act("You try to push $N out, but fail.", FALSE, ch, 0, vict, TO_CHAR);
					act("$n tries to push $N out, but fails.", FALSE, ch, 0, vict, TO_NOTVICT);
					act("$n tried to push you out!", FALSE, ch, 0, vict, TO_VICT);
				} else {
					act("You try to push $N out, but fail.", FALSE, ch, 0, vict, TO_CHAR);
					act("$n tries to push $N out, but fails.", FALSE, ch, 0, vict, TO_NOTVICT);
					act("$n grabs on to you and tries to drag you out, but can't seem to move you!", FALSE, ch, 0, vict, TO_VICT);
				}
				return;
			}
		}
	}
	

	if (subcmd == SCMD_DRAG) {
		switch (mode) {
			case 0:
				act("You drag $p out.", FALSE, ch, obj, 0, TO_CHAR );
				act("$n drags $p out.", FALSE, ch, obj, 0, TO_ROOM );
				ch->FromRoom();
				ch->ToRoom(dest);
				obj->FromRoom();
				obj->ToRoom(dest);
				act("$n drags $p out of $P.", FALSE, ch, obj, vehicle, TO_ROOM );
				look_at_room(ch);
				break;
			
			case 1:
				act("You drag $N out.", FALSE, ch, 0, vict, TO_CHAR );
				act("$n drags $N out.", FALSE, ch, 0, vict, TO_NOTVICT);
				act("$n drags you out!", FALSE, ch, 0, vict, TO_VICT);
				ch->FromRoom();
				ch->ToRoom(dest);
				
				if (vict->AbortTimer() == false)
					break;

				if (!leave_mtrigger(vict, -1) || PURGED(vict))
					break;
				if (!leave_otrigger(vict, -1) || PURGED(vict))
					break;
				if (!leave_wtrigger(vict->InRoom(), vict, -1) || PURGED(vict))
					break;

				vict->FromRoom();
				vict->ToRoom(dest);
				act("$n drags $N out of $p.", FALSE, ch, vehicle, vict, TO_NOTVICT );
				look_at_room(vict);
				look_at_room(ch);
				break;
		}
		if (!NO_STAFF_HASSLE(ch))
			WAIT_STATE(ch, 2 RL_SEC);
	} else {
		switch (mode) {
			case 0:
				act("You push $p out.", FALSE, ch, obj, 0, TO_CHAR );
				act("$n pushes $p out.", FALSE, ch, obj, 0, TO_ROOM );
				obj->FromRoom();
				obj->ToRoom(dest);
				act("$p is pushed out of $P.", FALSE, 0, obj, vehicle, TO_ROOM);
				break;
			
			case 1:
				act("You push $N out.", FALSE, ch, 0, vict, TO_CHAR );
				act("$n pushes $N out.", FALSE, ch, 0, vict, TO_NOTVICT );
				act("$n pushes you out!", FALSE, ch, 0, vict, TO_VICT);
				
				if (vict->AbortTimer() == false)
					break;

				if (!leave_mtrigger(vict, -1) || PURGED(vict))
					break;
				if (!leave_otrigger(vict, -1) || PURGED(vict))
					break;
				if (!leave_wtrigger(vict->InRoom(), vict, -1) || PURGED(vict))
					break;
				
				vict->FromRoom();
				vict->ToRoom(dest);
				if (GET_POS(vict) > POS_SITTING) GET_POS(vict) = POS_SITTING;
				look_at_room(vict);
				act("$n is pushed out of $p.", FALSE, vict, vehicle, 0, TO_ROOM );
				break;
		}
		if (!NO_STAFF_HASSLE(ch))
			WAIT_STATE(ch, 4 RL_SEC);
	}
//	WAIT_STATE(ch, 10 RL_SEC);
	return;
}


#define MAX_DISTANCE 6

const char * distance_text[MAX_DISTANCE + 1] = {
	"SHOULD NOT SEE!  REPORT IMMEDIATELY!",
	"Immediately",
	"Nearby",
	"A little ways away",
	"A ways",
	"Off in the distance",
	"Far away"
};
const char * distance_text_2[MAX_DISTANCE + 1] = {
	"SHOULD NOT SEE!  REPORT IMMEDIATELY!",
	"immediately",
	"nearby",
	"a little ways away",
	"a ways",
	"off in the distance",
	"far away"
};

#if 0
void MotionLeaving(CharData *ch, int directionOfTravel)
{
	RoomData *	room, prevRoom;
	CharData *	vict;
	int			reverseDirection = rev_dir[directionOfTravel];
	BUFFER(buf, MAX_INPUT_LENGTH);

	for (dir = 0; dir < NUM_OF_DIRS; ++dir)
	{
		room = IN_ROOM(ch);
		
		for (distance = 1; distance < MAX_DISTANCE + 1; ++distance)
		{
			if (!EXIT_OPEN(EXITN(room, dir)))
				break;
			
			prevRoom = room;
			room = EXITN(room, dir)->ToRoom;
			
			//	One way exits don't count.
			if (!EXIT_OPEN(EXITN(room, reverseDirection)) || EXITN(room, reverseDirection)->ToRoom != prevRoom)
				break;
			
			START_ITER(iter, vict, CharData *, room->people)
			{
				if (!AWAKE(vict) || IS_NPC(vict))	//	Potentially remove this conditions
					continue;
				
				//	First off handle scripts
				motion_otrigger
				void motion_otrigger(CharData *actor, int distance, int type);
			}
		}
	}

}
#endif

void handle_motion_leaving(CharData *ch, int direction, Flags flags)
{
	int		door, rev_door, distance;
	bool	lateral;
	BUFFER(buf, MAX_INPUT_LENGTH);
	
	for (door = 0; door < NUM_OF_DIRS; door++)
	{
		RoomData *nextroom = ch->InRoom();		/* Get the next room */
		rev_door = rev_dir[door];
		lateral = (direction == door) || (direction == rev_door);

		for (distance = 1; (distance < MAX_DISTANCE + 1); distance++) {
			if (!Exit::IsOpen(nextroom, door))
				break;
			RoomData *prevroom = nextroom;
			nextroom = Exit::Get(nextroom, door)->ToRoom();
			if (!Exit::IsOpen(nextroom, rev_door) || Exit::Get(nextroom, rev_door)->ToRoom() != prevroom)
				break;	//	two-way exits only
			
			FOREACH(CharacterList, nextroom->people, iter)
			{
				CharData *vict = *iter;
				
				motion_mtrigger(vict, ch, direction, rev_door, lateral, distance, 1 /*LEAVING*/);
				if (PURGED(ch))		return;
				if (PURGED(vict))	break;
			
				motion_otrigger(vict, ch, direction, rev_door, lateral, distance, 1 /*LEAVING*/);
				if (PURGED(ch))		return;
				if (PURGED(vict))	break;
				
				if (!AWAKE(vict) || IS_NPC(vict) || PLR_FLAGGED(vict, PLR_MAILING | PLR_WRITING))
					continue;
				if (AFF_FLAGGED(vict, AFF_MOTIONTRACKING))
				{
					sprintf(buf, "You detect motion %s %s you.\n", distance_text_2[distance], dir_text_to_the_of[rev_door] );
					act(buf, TRUE, vict, 0, 0, TO_CHAR);
				}
				
				if (!IS_SET(flags, MOVE_QUIET)
					&& CHAR_WATCHING(vict) == rev_door
					&& vict->CanSee(ch) != VISIBLE_NONE)
				{
					if (((distance == MAX_DISTANCE)	&& (CHAR_WATCHING(vict) == direction))	//	Moving out
						|| !lateral)	//	Or across
					{
						sprintf(buf, "%s %s you see $N move out of view.", distance_text[distance],
								dir_text_to_the_of[CHAR_WATCHING(vict)]);
						act(buf, TRUE, vict, 0, ch, TO_CHAR);
					}
					else if ((door != direction) || (distance > 1))	//	Moving closer or further
					{
						sprintf(buf, "%s %s you see $N move %s you.", distance_text[distance],
								dirs[CHAR_WATCHING(vict)],
								(direction == ( CHAR_WATCHING(vict) )) ? "away from" : "towards");
						act(buf, TRUE, vict, 0, ch, TO_CHAR);
					}
				}	/* IF ...watching... */
			}

			room_motion_otrigger(nextroom, ch, direction, rev_door, lateral, distance, 1 /*LEAVING*/);
			motion_wtrigger(nextroom, ch, direction, rev_door, lateral, distance, 1 /*LEAVING*/);
				
			if (PURGED(ch))
				return;
		}	/* FOR distance */
	}	/* FOR door */
}


void handle_motion_entering(CharData *ch, int direction, Flags flags) {
	int		door, rev_door, distance;
	bool	lateral;
	BUFFER(buf, MAX_INPUT_LENGTH);

	for (door = 0; door < NUM_OF_DIRS; door++)
	{
		RoomData *nextroom = ch->InRoom();		/* Get the next room */
		rev_door = rev_dir[door];
		lateral = (direction == door) || (direction == rev_door);

		for (distance = 1; (distance < MAX_DISTANCE + 1); distance++) {
			if (!Exit::IsOpen(nextroom, door))		break;
			RoomData *prevroom = nextroom;
			nextroom = Exit::Get(nextroom, door)->ToRoom();
			if (!Exit::IsOpen(nextroom, rev_door) || Exit::Get(nextroom, rev_door)->ToRoom() != prevroom)
				break;	//	two-way exits only

			FOREACH(CharacterList, nextroom->people, iter)
			{
				CharData *vict = *iter;
				
				motion_mtrigger(vict, ch, direction, rev_door, lateral, distance, 0 /*LEAVING*/);
				if (PURGED(ch))		return;
				if (PURGED(vict))	break;
				
				motion_otrigger(vict, ch, direction, rev_door, lateral, distance, 0 /*ENTERING*/);
				if (PURGED(ch))		return;
				if (PURGED(vict))	break;
				
				if (!AWAKE(vict) || IS_NPC(vict) || PLR_FLAGGED(vict, PLR_MAILING | PLR_WRITING))
					continue;
				
				if (AFF_FLAGGED(vict, AFF_MOTIONTRACKING))
				{
					if (((distance == MAX_DISTANCE) && (direction == door))	//	Moving towards
						|| !lateral)	//	Or across
					{
						sprintf(buf, "You detect motion %s %s you.\n", distance_text_2[distance],
								dir_text_to_the_of[rev_door] );
						act(buf, TRUE, vict, 0, 0, TO_CHAR);
					}
				}
				
				if (!IS_SET(flags, MOVE_QUIET)
					&& CHAR_WATCHING(vict) == rev_door
					&& vict->CanSee(ch) != VISIBLE_NONE)
				{
					if (((distance == MAX_DISTANCE) && (CHAR_WATCHING(vict) == rev_dir[direction]))	//	Moving towards
						|| !lateral)	//	Or across
					{
						sprintf(buf, "%s %s you, you see $N move into view.", distance_text[distance],
								dir_text_to_the_of[CHAR_WATCHING(vict)]);
						act(buf, TRUE, vict, 0, ch, TO_CHAR);
					}
				}	/* IF ...watching... */
			}

			room_motion_otrigger(nextroom, ch, direction, rev_door, lateral, distance, 0 /*ENTERING*/);
			motion_wtrigger(nextroom, ch, direction, rev_door, lateral, distance, 0 /*LEAVING*/);

			if (PURGED(ch))
				return;
		}	/* FOR distance */
	}	/* FOR door */
}

void Crash_extract_norents_from_equipped(CharData * ch);
void Crash_extract_norents(ObjData * obj);

ACMD(do_recall) {
	//ObjData *	tmp;
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	if (IS_NPC(ch))
		return;
	
	if (!NO_STAFF_HASSLE(ch)
		&& (Event::FindEvent(ch->m_Events, GravityEvent::Type)
			|| AFF_FLAGS(ch).test(AFF_NOQUIT) || AFF_FLAGS(ch).test(AFF_NORECALL)
			|| ROOM_FLAGGED(IN_ROOM(ch), ROOM_NORECALL)))
	{
		send_to_char("For some reason you can't recall!\n", ch);
		return;
	}
	
	argument = one_argument(argument, arg);

//	Crash_extract_norents_from_equipped(ch);

//	START_ITER(iter, tmp, ObjData *, ch->carrying) {
//		Crash_extract_norents(tmp);
//	}
	
	RoomData *room = NULL;
	if (*arg && (GET_TEAM(ch) == 0 || team_recalls[GET_TEAM(ch)] == NULL))
	{
		if (is_abbrev(arg, "clan"))
		{
			if (GET_CLAN(ch) && Clan::GetMember(ch))
				room = world.Find(GET_CLAN(ch)->GetRoom());
		}
		else if (is_abbrev(arg, "house"))
		{
			argument = one_argument(argument, arg);

			House *house = NULL;
			if (*arg)
			{
				house = House::GetHouseByID(atoi(arg));
				
				if (!house)
				{
					ch->Send("No such house.\n");
					return;
				}
				
				House::SecurityLevel guestLevel = house->GetGuestSecurity(ch->GetID()),
									 houseLevel = house->GetSecurityLevel();
				if (guestLevel < House::AccessFriend)
				{
					ch->Send("You must be a Friend or Owner to recall to a house.\n");
					return;
				}
				if (guestLevel < house->GetSecurityLevel())
				{
					ch->Send("That house is not allowing Friends to recall to it at the moment.\n");
					return;
				}
			}
			else
			{
				house = House::GetHouseForAnyOwner(ch->GetID());
				
				if (!house)
				{
					ch->Send("You do not own a house.\n");
					return;
				}
			}


			if (!house->GetRecall())
			{
				ch->Send("That house does not have a recall set up.\n");
				return;
			}
			if (!house->CanEnter(ch, house->GetRecall()))
			{
				ch->Send("You cannot recall to that house at the moment.\n");
				if (!house->IsPaidUp())	ch->Send("The payment is overdue, and the house has been locked.\n");
				return;
			}
			
			room = house->GetRecall();
		}
//		else if (is_abbrev(arg, "home"))
//		{
//			room = ch->GetPlayer()->m_LoadRoom;
//			if (room != NOWHERE)
//				room = World::FindRoom(room);
//		}
		else if (is_abbrev(arg, "race"))
		{
			room = world.Find( IS_STAFF(ch) ? immort_start_room : start_rooms[ch->GetRace()] );
		}
	}
	if (room == NULL)
		room = ch->StartRoom();
	
	act("$n disappears in a bright flash of light.", TRUE, ch, 0, 0, TO_ROOM);
	ch->FromRoom();
	ch->ToRoom(room);
	act("$n appears in a bright flash of light.", TRUE, ch, 0, 0, TO_ROOM);
	look_at_room(ch);
}
