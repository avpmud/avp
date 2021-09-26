#include "structs.h"
#include "scripts.h"
#include "scriptcompiler.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "find.h"
#include "handler.h"
#include "buffer.h"
#include "db.h"
#include "olc.h"
#include "spells.h"
#include "event.h"
#include "clans.h"
#include "track.h"
#include "affects.h"
#include "constants.h"
#include "files.h"
#include "house.h"

#define SCMD_SEND			0
#define SCMD_SEND_ECHO		1
#define SCMD_SEND_ECHOAROUND	2
#define SCMD_SEND_ECHOAROUND2	3

#define SCMD_DESTROY		0
#define SCMD_REPAIR			1
#define SCMD_DAMAGEFULL		1
#define SCMD_CLANECHO		0
#define SCMD_TEAMECHO		1
#define SCMD_RACEECHO		2
#define SCMD_MISSIONECHO	3

void AlterHit(CharData *ch, int amount);
void AlterMove(CharData *ch, int amount);
void AlterEndurance(CharData *ch, int amount);


#define GET_TARGET_CHAR(arg)	((go->GetEntityType() == Entity::TypeCharacter) ? get_char_by_char((CharData *)go, arg) : \
			((go->GetEntityType() == Entity::TypeObject) ? get_char_by_obj((ObjData *)go, arg) :					 \
			((go->GetEntityType() == Entity::TypeRoom) ? get_char_by_room((RoomData *)go, arg) : NULL)))
			
#define GET_TARGET_OBJ(arg)		((go->GetEntityType() == Entity::TypeCharacter) ? get_obj_by_char((CharData *)go, arg) : \
			((go->GetEntityType() == Entity::TypeObject) ? get_obj_by_obj((ObjData *)go, arg) :					\
			((go->GetEntityType() == Entity::TypeRoom) ? get_obj_by_room((RoomData *)go, arg) : NULL)))

#define GET_TARGET_ROOM(arg)	((go->GetEntityType() == Entity::TypeCharacter) ? find_target_room_by_char(thread, (CharData *)go, arg) : \
			((go->GetEntityType() == Entity::TypeObject) ? find_target_room_by_obj(thread, (ObjData *)go, arg) :					\
			((go->GetEntityType() == Entity::TypeRoom) ? find_target_room_by_room(thread, (RoomData *)go, arg) : NULL)))

RoomData *find_target_room_by_uid(const char *roomstr);
RoomData *find_target_room_by_room(TrigThread *thread, RoomData *room, const char *roomstr);
RoomData *find_target_room_by_obj(TrigThread *thread, ObjData *obj, const char *roomstr);
RoomData *find_target_room_by_char(TrigThread *thread, CharData * ch, const char *rawroomstr);
RoomData *this_room(Entity * go);

extern void reset_zone(ZoneData *zone);

void load_mtrigger(CharData *ch);
void load_otrigger(ObjData *obj);

int call_mtrigger(CharData *ch, Entity *caller, const char *cmd, const char *argument, char *result);
int call_otrigger(ObjData *obj, Entity *caller, const char *cmd, const char *argument, char *result);
int call_wtrigger(RoomData *room, Entity *caller, const char *cmd, const char *argument, char *result);

extern CompiledScript::Function *ScriptEngineRunScriptFindFunction(TrigThread *thread, const char *functionName, TrigData *&pNewTrigger);

/*
void SetVariable(Ptr go, TrigData * trig, char *name, char *value, int type);


void SetVariable(Ptr go, TrigData * trig, char *name, char *value, int type) {
	Scriptable *sc;
	
	switch (type) {
		case MOB_TRIGGER:
			sc = SCRIPT((CharData *) go);
			break;
		case OBJ_TRIGGER:
			sc = SCRIPT((ObjData *) go);
			break;
		case WLD_TRIGGER:
			sc = SCRIPT((RoomData *) go);
			break;
	}
	
	TrigVarData *vd = search_for_variable(sc, trig, name);
	if (vd) {
		if (vd->value)	free(vd->value);
		vd->value = str_dup(value);
	} else
		add_var(GET_TRIG_VARS(trig)->var_list, name, value, sc->context);
}
#define SETVAR(var, data)	SetVariable(GET_TRIG_VARS(trig), var, data, 0)
*/



SCMD(func_asound);
SCMD(func_echo);	//	DEPRECATED - now uses send
SCMD(func_send);
SCMD(func_zoneecho);
SCMD(func_zoneechorace);
SCMD(func_teleport);
SCMD(func_door);
SCMD(func_force);
SCMD(func_mp);
SCMD(func_stitle);
SCMD(func_kill);		//	Mob only
SCMD(func_junk);		//	Mob only
SCMD(func_load);
SCMD(func_purge);
SCMD(func_at);
SCMD(func_damage);
SCMD(func_damageen);
SCMD(func_damagemv);
SCMD(func_heal);
SCMD(func_healen);
SCMD(func_healmv);
SCMD(func_remove);
SCMD(func_do);
SCMD(func_goto);
SCMD(func_info);
SCMD(func_copy);
SCMD(func_lag);
SCMD(func_move);
SCMD(func_withdraw);
SCMD(func_deposit);
SCMD(func_call);
SCMD(func_hunt);
SCMD(func_timer);
//SCMD(func_destroy);
SCMD(func_log);
SCMD(func_restring);
SCMD(func_worldecho);
SCMD(func_affect);
SCMD(func_affectmsg);
SCMD(func_unaffect);
SCMD(func_attach);
SCMD(func_detach);
SCMD(func_startcombat);
SCMD(func_stopcombat);
SCMD(func_killthread);
SCMD(func_zreset);
SCMD(func_team);
SCMD(func_transform);
SCMD(func_oset);
SCMD(func_mset);



#define script_log(...)	ScriptEngineReportError(thread, false, __VA_ARGS__)


/*
char *ScriptEngineGetArguments(char *input, int num, ...)
{
	va_list ap;

	va_start(ap, input);
	
	while (num-- > 0)
	{
		char *arg = va_arg(ap, char *);
		input = ScriptEngineGetArgument(input, arg);
	}
	
	va_end(args);
}
*/


static const char *ScriptEngineGetArguments(const char *input, char *arg1 = NULL, char *arg2 = NULL, char *arg3 = NULL, char *arg4 = NULL)
{
	if (arg1)	input = ScriptEngineGetArgument(input, arg1);
	if (arg2)	input = ScriptEngineGetArgument(input, arg2);
	if (arg3)	input = ScriptEngineGetArgument(input, arg3);
	if (arg4)	input = ScriptEngineGetArgument(input, arg4);
	
	return input;
}


RoomData *ScriptObjectAtRoom = NULL;
RoomData *this_room(Entity * e)
{
	switch (e->GetEntityType()) {
		case Entity::TypeCharacter:	return ScriptObjectAtRoom ? ScriptObjectAtRoom : ((CharData *)e)->InRoom();
		case Entity::TypeObject:	return ScriptObjectAtRoom ? ScriptObjectAtRoom : ((ObjData *)e)->Room();
		case Entity::TypeRoom:		return ((RoomData *)e);
		default:	break;
	}
	return NULL;
}


RoomData * find_target_room_by_uid(const char *uid)
{
	Entity *e = IDManager::Instance()->Find(uid);
	if (e)
	{
		switch (e->GetEntityType())
		{
			case Entity::TypeCharacter:	return ((CharData *)e)->InRoom();
			case Entity::TypeObject:	return ((ObjData *)e)->InRoom();
			case Entity::TypeRoom:		return ((RoomData *)e);
			default:	break;
		}
	}
	
	return NULL;
}


RoomData * find_target_room_by_char(TrigThread *thread, CharData *ch, const char *roomstr) {
	RoomData *location = NULL;
	CharData *target_mob;
	ObjData *target_obj;
	
	if (IsIDString(roomstr))
		location = find_target_room_by_uid(roomstr);
	else if (IsVirtualID(roomstr) /* && !strchr(roomstr, '.') */)
		location = world.Find(VirtualID(roomstr, thread->GetCurrentVirtualID().zone));
	else if ((target_mob = get_char_by_char(ch, roomstr)))
		location = target_mob->InRoom();
	else if ((target_obj = get_obj_by_char(ch, roomstr)))
		location = target_obj->InRoom();

	return location;
}


RoomData * find_target_room_by_room(TrigThread *thread, RoomData *room, const char *roomstr) {
	RoomData *location = NULL;
	CharData *target_mob;
	ObjData *target_obj;
	
	if (IsIDString(roomstr))
		location = find_target_room_by_uid(roomstr);
	else if (IsVirtualID(roomstr) /*&& !strchr(roomstr, '.')*/)
		location = world.Find(VirtualID(roomstr, thread->GetCurrentVirtualID().zone));
	else if ((target_mob = get_char_by_room(room, roomstr)))
		location = target_mob->InRoom();
	else if ((target_obj = get_obj_by_room(room, roomstr)))
		location = target_obj->InRoom();

	return location;
}


RoomData * find_target_room_by_obj(TrigThread *thread, ObjData *obj, const char *roomstr) {
	RoomData *location = NULL;
	CharData *target_mob;
	ObjData *target_obj;
	
	if (IsIDString(roomstr))
		location = find_target_room_by_uid(roomstr);
	else if (IsVirtualID(roomstr) /*&& !strchr(roomstr, '.')*/)
		location = world.Find(VirtualID(roomstr, thread->GetCurrentVirtualID().zone));
	else if ((target_mob = get_char_by_obj(obj, roomstr)))
		location = target_mob->InRoom();
	else if ((target_obj = get_obj_by_obj(obj, roomstr)))
		location = target_obj->InRoom();

	return location;
}

SCMD(func_asound)
{
	int  	door;

	if (!*argument)
	{
		script_log("asound called with no argument");
		return;
	}

	skip_spaces(argument);
	
	RoomData *source_room = this_room(go);

	for (door = 0; door < NUM_OF_DIRS; door++)
	{
		RoomExit *exit = Exit::GetIfValid(source_room, door);

		if (exit && exit->ToRoom() != source_room)
		{
			if (!exit->ToRoom()->people.empty())
				act(argument, FALSE, exit->ToRoom()->people.front(), 0, 0, TO_ROOM | TO_CHAR | TO_SCRIPT);
		}
	}
}


SCMD(func_dsound)
{
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
	
	const char *msg = ScriptEngineGetArgument(argument, buf);
	
	RoomData *source_room = this_room(go);

	int dir = GetEnumByNameAbbrev<Direction>(buf);
	
	if (dir == -1)
		script_log("dsound called with bad direction: %s", argument);
	else if (!*msg)
		script_log("dsound called with no message: %s", argument);
	else
	{
		RoomExit *exit = Exit::GetIfValid(source_room, dir);

		if (exit && exit->ToRoom() != source_room)
		{
			if (!exit->ToRoom()->people.empty())
				act(argument, FALSE, exit->ToRoom()->people.front(), 0, 0, TO_ROOM | TO_CHAR | TO_SCRIPT);
		}
	}
}


SCMD(func_echo) {	//	DEPRECATED - now uses SEND
	skip_spaces(argument);

	if (!*argument) {
		script_log("echo called with no args");
		return;
	}
	
	RoomData *room = this_room(go);

	if (room)
	{
//		room->Send("%s\n", argument);
		if (!room->people.empty())
			act(argument, FALSE, room->people.front(), 0, 0, TO_ROOM | TO_CHAR | TO_SCRIPT);
	} else
		script_log("echo called from NOWHERE");
}


SCMD(func_send) {
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
	const char *msg;
	int team;
	
	//	flags:
	const int SEND_SLEEPING		= (1 << 0);
	
	char *send_flags[] = 
	{
		"+sleeping",
		"\n"
	};
	
	msg = ScriptEngineGetArgument(argument, buf);

	int flag;
	Flags flags = 0;
	while ((flag = search_block(buf, send_flags, true)) >= 0)
	{
		if (subcmd == SCMD_SEND_ECHO)
		{
			argument = msg;
		}
		
		flags |= (1 << flag);
		msg = ScriptEngineGetArgument(msg, buf);
	}
	
	if (!*buf)
		script_log("send called with no args");
	else {
		skip_spaces(msg);

//		if (!*msg)
//			script_log("send called without a message");
//		else
		if (subcmd == SCMD_SEND && !str_cmp(buf, "team"))
		{
			msg = ScriptEngineGetArgument(msg, buf);
			
			team = search_block(buf, team_names, true);
			
			if (team <= 0)
				script_log("invalid team: %s", buf);
			else
				act(msg, FALSE, 0, 0, (const void *)team, TO_TEAM | TO_SLEEP | TO_SCRIPT);
		}
		else if (subcmd == SCMD_SEND_ECHO)
		{
			RoomData *room = this_room(go);

			if (room)
			{
				int mode = TO_ROOM | TO_CHAR;
				
				if (IS_SET(flags, SEND_SLEEPING))
					mode |= TO_SLEEP;
				
				if (!room->people.empty())
					act(argument, FALSE, room->people.front(), 0, 0, mode | TO_SCRIPT);
			}
			else
				script_log("echo called from NOWHERE");
		}
		else
		{
			CharData *ch;
			
			Entity *e = IDManager::Instance()->Find(buf);
			
			if ( (ch = dynamic_cast<CharData *>(e)) )
			{
				CharData *vict = NULL;
				int mode = TO_CHAR;
				if (subcmd == SCMD_SEND_ECHOAROUND)
					mode = TO_ROOM;
				else if (subcmd == SCMD_SEND_ECHOAROUND2)
				{
					mode = TO_NOTVICT;
					msg = ScriptEngineGetArgument(msg, buf);
					vict = GET_TARGET_CHAR(buf);
				}
				
				if (IS_SET(flags, SEND_SLEEPING))
					mode |= TO_SLEEP;
				
				act(msg, FALSE, ch, 0, vict, mode | TO_SCRIPT);
			}
			else if (subcmd != SCMD_SEND && e)
			{
				RoomData *room = this_room(e);

				int mode = TO_ROOM | TO_CHAR;
				
				if (IS_SET(flags, SEND_SLEEPING))
					mode |= TO_SLEEP;
				
				if (room && !room->people.empty())
					act(argument, FALSE, room->people.front(), 0, 0, mode | TO_SCRIPT);
			}
			else
				script_log((subcmd == SCMD_SEND) ? "no target found for send" : "no target found for echoaround");
		}
	}
}


SCMD(func_worldecho) {
	BUFFER(targetname, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
	const char *msg;
	int	target = 0;
	
	if (subcmd != SCMD_MISSIONECHO)
		msg = ScriptEngineGetArgument(argument, targetname);
	else
		msg = argument;

	if ((subcmd != SCMD_MISSIONECHO && !*targetname) || !*msg)
		script_log("worldecho [clan|team|race|mission] called with too few args: %s", argument);
	else if (subcmd == SCMD_CLANECHO && (!is_number(targetname) || !(Clan::GetClan(target = atoi(targetname)))))
		script_log("clanecho called for nonexistant clan '%s'", targetname);
	else if (subcmd == SCMD_RACEECHO && ((target = search_block(targetname, faction_types, true)) < 0) &&
			((target = search_block(targetname, race_types, true)) < 0))
		script_log("raceecho called for invalid race '%s'", targetname);
	else if (subcmd == SCMD_TEAMECHO && ((target = search_block(targetname, team_names, true)) <= 0))
		script_log("teamecho called for invalid team '%s'", targetname);
	else {
//		sprintf(buf, "%s\n", msg);

		FOREACH(DescriptorList, descriptor_list, iter)
		{
			DescriptorData *i = *iter;
			if (i->GetState()->IsPlaying() && i->m_Character &&
					!PLR_FLAGGED(i->m_Character, PLR_WRITING) ) {

				if ((subcmd == SCMD_CLANECHO)
					&& (!GET_CLAN(i->m_Character)
						|| (!IS_NPC(i->m_Character) && !Clan::GetMember(i->m_Character))
						|| GET_CLAN(i->m_Character)->GetID() != target))
					continue;
				if ((subcmd == SCMD_TEAMECHO) && (GET_TEAM(i->m_Character) != target))
					continue;
				if ((subcmd == SCMD_RACEECHO) && (i->m_Character->GetFaction() != factions[target]))
					continue;
				if ((subcmd == SCMD_MISSIONECHO) && !CHN_FLAGGED(i->m_Character, Channel::Mission))
					continue;
				
//				SEND_TO_Q(buf, i);
				
				act(msg, FALSE, i->m_Character, 0, 0, TO_CHAR | TO_SLEEP | TO_SCRIPT);
//				i->m_Character->AddChatBuf(storeBuf);
			}
		}

	}
}


SCMD(func_zoneecho) {
	BUFFER(zone_name, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);

	const char *msg = ScriptEngineGetArgument(argument, zone_name);
	
	strcat(zone_name, ":0");
	VirtualID	vid(zone_name);
	
	//ZoneData *zone = zone_table.Find(zone_name);
	ZoneData *zone = zone_table.Find(vid.zone);

	if (!*zone_name || !*msg)
		script_log("zoneecho called with too few args");
	else if (!zone)
		script_log("zoneecho called for nonexistant zone '%s'", zone_name);
	else
	{
		sprintf(buf, "%s\n", msg);
		send_to_zone(buf, zone, vid.ns);
	}
}


SCMD(func_zoneechorace) {
	BUFFER(zone_name, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(race, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
	int racenum;

	const char *msg = ScriptEngineGetArguments(argument, zone_name, race);
	
	strcat(zone_name, ":0");
	VirtualID	vid(zone_name);
	
//	ZoneData *zone = zone_table.Find(zone_name);
	ZoneData *zone = zone_table.Find(vid.zone);

	if (!*zone_name || !*race || !*msg)
		script_log("zoneechorace called with too few args: %s", argument);
	else if (!zone)
		script_log("zoneechorace called for nonexistant zone '%s'", zone_name);
	else if ((racenum = search_block(race, race_types, 0)) < 0)
		script_log("zoneechorace: invalid race '%s'", race);
	else {
		sprintf(buf, "%s\n", msg);
		send_to_zone_race(buf, zone, racenum, vid.ns);
	}
}

static const RoomFlags MAKE_BITSET(PrivateRoomFlags, ROOM_PRIVATE, ROOM_STAFFROOM, ROOM_ADMINROOM);

SCMD(func_teleport) {
	BUFFER(arg1, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(arg2, MAX_SCRIPT_STRING_LENGTH);
	CharData *ch;
	RoomData * target = NULL;

	ScriptEngineGetArguments(argument, arg1, arg2);

	VirtualID	vid(arg2, thread->GetCurrentVirtualID().zone);
	
	if (!*arg1 || !*arg2)
		script_log("teleport called with too few args: %s", argument);
	else if (!IsIDString(arg2) && !IsVirtualID(arg2))
		script_log("teleport target is an invalid room '%s'", arg2);
	else if ((target = GET_TARGET_ROOM(arg2)) == NULL)
		script_log("teleport target is an invalid room '%s'", arg2);
	else if (!str_cmp(arg1, "all"))
	{
		RoomData *room = this_room(go);
		
		if (room == NULL)		script_log("teleport called from NOWHERE");
		if (target == room)		script_log("teleport target is itself");

		FOREACH(CharacterList, room->people, iter)
		{
			ch = *iter;
			if (House::CanEnter(ch, target) && target->GetFlags().testForNone( PrivateRoomFlags ))
			{
				ch->FromRoom();
				ch->ToRoom(target);
			}
		}
	} else if ((ch = GET_TARGET_CHAR(arg1))) {
		if (House::CanEnter(ch, target) && target->GetFlags().testForNone( PrivateRoomFlags ))
		{
			ch->FromRoom();
			ch->ToRoom(target);
		}
	} else
		script_log("teleport: can't find target '%s'", arg1);
}


SCMD(func_door) {
	BUFFER(target, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(direction, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(field, MAX_SCRIPT_STRING_LENGTH);
	const char *value;
	RoomData *rm;
	RoomExit *exit, *oppositeExit = NULL;
	int dir, fd;
	int bit;
	RoomData *targetRoom = NULL;
	
	char *door_fields[] = {
		"purge",
		"flags",
		"room",
		"setstate",
		"removestate",
		"clearstates",
		"open",
		"close",
		"lock",
		"unlock",
		"\n"
	};
	static const int PURGE_FIELD = search_block("purge", door_fields, true);
	static const int MUTABLE_FIELDS = search_block("setstate", door_fields, true);
	
	
	value = ScriptEngineGetArguments(argument, target, direction, field);
	
	bool reflect = false;
	if (!str_cmp(field, "reflect"))
	{
		reflect = true;
		value = ScriptEngineGetArguments(value, field);
	}
	
	ZoneData *	trigZone = zone_table.Find(trig->GetVirtualID().zone);
	if (!*target || !*direction || !*field)
		script_log("door called with too few args: %s", argument);
	else if ((rm = GET_TARGET_ROOM(target)) == NULL)
		script_log("door: invalid target '%s'", target);
	else if ((dir = GetEnumByNameAbbrev<Direction>(direction)) == -1)
		script_log("door: invalid direction '%s'", direction);
	else if ((fd = search_block(field, door_fields, FALSE)) == -1)
		script_log("door: invalid field '%s'", field);
	else {
		exit = rm->GetExit(dir);

		/* purge exit */
		if (fd == PURGE_FIELD)
		{
			if (trigZone != rm->GetZone())
			{
				script_log("door: invalid permissions (zone mismatch) on exit '%s-%s': %s", rm->GetVirtualID().Print().c_str(), GetEnumName<Direction>(dir), argument);
				return;
			}
				
			rm->SetExit(dir, NULL);
		}
		else
		{
			if (!exit)
			{
				if (trigZone != rm->GetZone())
				{
					script_log("door: invalid permissions (zone mismatch) on exit '%s-%s': %s", rm->GetVirtualID().Print().c_str(), GetEnumName<Direction>(dir), argument);
					return;
				}
				
				exit = new RoomExit;
				rm->SetExit(dir, exit);
			}
			
			if (trigZone != rm->GetZone() && (fd < MUTABLE_FIELDS || !EXIT_FLAGGED(exit, EX_MUTABLE)))
			{
				script_log("door: invalid permissions (zone mismatch/non mutable exit) on exit '%s-%s': %s", rm->GetVirtualID().Print().c_str(), GetEnumName<Direction>(dir), argument);
				return;
			}
			
			if (reflect && exit->room)
			{
				RoomData *	oppositeRoom = exit->room;
				
				oppositeExit = oppositeRoom->GetExit(rev_dir[dir]);
				if (oppositeExit && oppositeExit->room != rm)
					oppositeExit = NULL;
				
				if (oppositeExit && trigZone != oppositeRoom->GetZone() && (fd < MUTABLE_FIELDS || !EXIT_FLAGGED(oppositeExit, EX_MUTABLE)))
				{
					script_log("door: invalid permissions (zone mismatch/non mutable exit) on reflected exit '%s-%s': %s", oppositeRoom->GetVirtualID().Print().c_str(), GetEnumName<Direction>(rev_dir[dir]), argument);
					oppositeExit = NULL;
				}
			}

			switch (fd) {
				case 1:  /* flags       */
					script_log("door: old door system in use; please switch to setflag/removeflag/clearflags/setstate/removestate/clearstates: %s", argument);
					break;
				case 2:  /* room        */
					targetRoom = GET_TARGET_ROOM(value);
					if (!targetRoom)	script_log("door: invalid door target '%s'", value);
					else				exit->room = targetRoom;
					
					if (oppositeExit)
					{
						oppositeExit->room = rm;
					}
					break;
				case 3:	/* setstates	*/
					while (*value)
					{
						value = ScriptEngineGetArgument(value, field);
						
						bit = GetEnumByName<ExitState>(field);
						
						if (bit == -1)		script_log("door: setstate - unknown state '%s': %s", field, argument);
						else
						{
							exit->GetStates().set((ExitState)bit);
							if (oppositeExit)	oppositeExit->GetStates().set((ExitState)bit);
						}
					}
					break;
				case 4:/* removestates	*/
					while (*value)
					{
						value = ScriptEngineGetArgument(value, field);
						
						bit = GetEnumByName<ExitState>(field);
						
						if (bit == -1)		script_log("door: removestate - unknown state '%s': %s", field,  argument);
						else
						{
							exit->GetStates().clear((ExitState)bit);
							if (oppositeExit)	oppositeExit->GetStates().clear((ExitState)bit);
						}
					}
				case 5:/* clearstates	*/
					exit->GetStates().clear();
					if (oppositeExit)	oppositeExit->GetStates().clear();
					break;
				case 6:/*open*/
					exit->GetStates().clear(EX_STATE_CLOSED)
									 .clear(EX_STATE_LOCKED);
					if (oppositeExit)
					{
						oppositeExit->GetStates().clear(EX_STATE_CLOSED)
												 .clear(EX_STATE_LOCKED);
					}
					break;
				case 7:/*close*/
					exit->GetStates().set(EX_STATE_CLOSED);
					if (oppositeExit)
					{
						oppositeExit->GetStates().set(EX_STATE_CLOSED);
					}
					break;
				case 8:/*lock*/
					exit->GetStates().set(EX_STATE_CLOSED)
									 .set(EX_STATE_LOCKED);
					if (oppositeExit)
					{
						oppositeExit->GetStates().set(EX_STATE_CLOSED)
												 .set(EX_STATE_LOCKED);
					}
					break;
				case 9:/*unlock*/
					exit->GetStates().clear(EX_STATE_LOCKED);
					if (oppositeExit)
					{
						oppositeExit->GetStates().clear(EX_STATE_LOCKED);
					}
					break;
			}
			
			if (!EXIT_FLAGGED(exit, EX_ISDOOR))
			{
				exit->GetStates().clear(EX_STATE_CLOSED)
								 .clear(EX_STATE_LOCKED)
								 .clear(EX_STATE_BREACHED)
								 .clear(EX_STATE_JAMMED);
			}
			
			if (oppositeExit && !EXIT_FLAGGED(oppositeExit, EX_ISDOOR))
			{
				exit->GetStates().clear(EX_STATE_CLOSED)
								 .clear(EX_STATE_LOCKED)
								 .clear(EX_STATE_BREACHED)
								 .clear(EX_STATE_JAMMED);
			}
		}
	}
}


SCMD(func_force) {
	BUFFER(arg1, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(buf, MAX_SCRIPT_STRING_LENGTH);
	CharData *ch;
	const char *line;

	RoomData *room = this_room(go);
	
	if (room == NULL) {
		script_log("force called from NOWHERE");
		return;
	}
	
	line = ScriptEngineGetArgument(argument, arg1);
	strncpy(buf, line, MAX_INPUT_LENGTH);

	buf[MAX_INPUT_LENGTH - 1] = '\0';
	
	if (!*arg1 || !*buf)
		script_log("force called with too few args: %s", argument);
	else if (!str_cmp(arg1, "all")) {
		FOREACH(CharacterList, room->people, iter)
		{
			ch = *iter;
			if (!IS_STAFF(ch))
				command_interpreter(ch, buf);
		}
	}
	else if (!str_cmp(arg1, "team"))
	{
		line = ScriptEngineGetArgument(line, arg1);
	
		int team = search_block(arg1, team_names, 0);
		
		if (team == 0)
			script_log("force: invalid team '%s': %s", arg1, argument);
		else
		{
			FOREACH(CharacterList, character_list, iter)
			{
				ch = *iter;
				if (GET_TEAM(ch) == team && !IS_STAFF(ch))
					command_interpreter(ch, buf);
			}
		}
	}
	else if ((ch = GET_TARGET_CHAR(arg1)))
	{
		if (!IS_STAFF(ch))
			command_interpreter(ch, buf);
	}
	else
		script_log("force: unable to find target '%s'", arg1);
}


//	Reward mission points
//void check_level(CharData *ch);

SCMD(func_mp) {
	BUFFER(name, MAX_SCRIPT_STRING_LENGTH);
	CharData *ch;
	int amount;
	const char *arg;

	arg = ScriptEngineGetArgument(argument, name);

	if (!*name || !*arg)
		script_log("mp: too few arguments: %s", argument);
	else if ((ch = GET_TARGET_CHAR(name))) {
		if (!IS_NPC(ch)) {
			amount = atoi(arg); 
			GET_MISSION_POINTS(ch) += amount;
			if (amount > 0) {
				ch->points.lifemp += amount;
//   	  			check_level(ch);
			}
			SET_BIT(PLR_FLAGS(ch), PLR_CRASH);
			
			mudlogf(CMP, LVL_STAFF, TRUE,
	        			(amount > 0) ? "(GC) Script [%s] gives %d MP to %s" :
	        			"(GC) Script [%s] takes away %d MP from %s",
	        		        trig->GetVirtualID().Print().c_str(),
	        		        abs(amount),
	                		ch->GetName());
		}
	} else
		script_log("mp: unable to find target '%s'", name);
}


SCMD(func_hunt) {
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	CharData *vict = NULL;
	CharData *mob  = NULL;
 
	ScriptEngineGetArguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2)
		script_log("hunt: too few arguments");
	else if (!(mob = GET_TARGET_CHAR(arg1)) || !IS_NPC(mob))
		script_log("hunt: hunter not found.");
	else if (!(vict = GET_TARGET_CHAR(arg2)))
		script_log("hunt: victim not found.");
	else {
		delete mob->path;
		mob->path = Path2Char(mob, GET_ID(vict), 200, HUNT_GLOBAL | HUNT_THRU_DOORS);
	}
}

SCMD(func_stitle) {
	BUFFER(name, MAX_SCRIPT_STRING_LENGTH);
	CharData *ch;
	const char *arg;

	arg = ScriptEngineGetArgument(argument, name);

	if (!*name || !*arg)
		script_log("stitle: too few arguments: %s", argument);
	else if ((ch = GET_TARGET_CHAR(name)))
	{
		ch->SetTitle(arg);
		ch->Send("\nTitle changed to: %s\n", arg);
	}
	else
		script_log("stitle: unable to find target '%s'", arg);
}


SCMD(func_kill) {
	BUFFER(arg, MAX_SCRIPT_STRING_LENGTH);
	CharData *victim;

	if (go->GetEntityType() != Entity::TypeCharacter)
		return;

	ScriptEngineGetArgument(argument, arg);

	if (!*arg)
		script_log("kill called with no argument");
	else if (!(victim = get_char_vis((CharData *)go, arg, FIND_CHAR_ROOM)))
		script_log("kill: victim no in room");
	else if (victim == (CharData *)go)
		script_log("kill: victim is self");
	else if (AFF_FLAGGED((CharData *)go, AFF_CHARM) && ((CharData *)go)->m_Following == victim )
		script_log("kill: charmed mob attacking master");
	else if (FIGHTING((CharData *)go))
		script_log("kill: already fighting");
	else
		initiate_combat((CharData *)go, victim, NULL);
}


SCMD(func_startcombat)
{
	BUFFER(arg1, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(arg2, MAX_SCRIPT_STRING_LENGTH);
	CharData *victim1;
	CharData *victim2;

	ScriptEngineGetArguments(argument, arg1, arg2);

	if (!IsIDString(arg1) || !IsIDString(arg2))
		script_log("startcombat called with invalid arguments: %s", argument);
	else if (!(victim1 = CharData::Find(ParseIDString(arg1))))
		script_log("startcombat: first victim not found");
	else if (!(victim2 = CharData::Find(ParseIDString(arg2))))
		script_log("startcombat: second victim not found");
	else if (victim1 == victim2)
		script_log("startcombat: cannot engage combat with self");
//	else if ((AFF_FLAGGED(victim1, AFF_CHARM) && victim1->master == victim2 ) ||
//			 (AFF_FLAGGED(victim2, AFF_CHARM) && victim2->master == victim1 ))
//		script_log("startcombat: charmed mob attacking master");
	else
	{
		//	Set victim1 and victim2 fighting each other.
		start_combat(victim1, victim2, StartCombat_NoMessages);
//		start_combat(victim2, victim1, StartCombat_NoMessages | StartCombat_NoTargetChange);
		
		//	If victim2 is not already fighting, start fighting with victim1
//		initiate_combat(victim1, victim2, NULL);
	}
}


SCMD(func_stopcombat)
{
	BUFFER(arg1, MAX_SCRIPT_STRING_LENGTH);
	CharData *victim;

	ScriptEngineGetArguments(argument, arg1);

	if (!IsIDString(arg1))
		script_log("stopcombat called with invalid arguments: %s", argument);
	else if (!(victim = CharData::Find(ParseIDString(arg1))))
		script_log("stopcombat: victim not found");
	else if (FIGHTING(victim))
	{
		if (FIGHTING(FIGHTING(victim)) == victim)
			FIGHTING(victim)->stop_fighting();
		victim->stop_fighting();
	}
}


SCMD(func_junk) {
	BUFFER(arg, MAX_SCRIPT_STRING_LENGTH);
	int pos;
	ObjData *obj;

	if (go->GetEntityType() != Entity::TypeCharacter)
		return;

	ScriptEngineGetArgument(argument, arg);

	if (!*arg)
		script_log("junk: bad syntax");
	else if (find_all_dots(arg) != FIND_INDIV) {
		if ((obj=get_obj_in_equip_vis((CharData *)go,arg, ((CharData *)go)->equipment,&pos))!= NULL) {
			unequip_char((CharData *)go, pos);
			obj->Extract();
		} else if ((obj = get_obj_in_list_vis((CharData *)go, arg, ((CharData *)go)->carrying)) != NULL )
			obj->Extract();
	} else {
		FOREACH(ObjectList, ((CharData *)go)->carrying, iter)
		{
			obj = *iter;
			if (arg[3] == '\0' || isname(arg+4, obj->GetAlias()))
				obj->Extract();
		}
		while ((obj=get_obj_in_equip_vis((CharData *)go, arg, ((CharData *)go)->equipment, &pos))) {
			unequip_char((CharData *)go, pos);
			obj->Extract();
		}   
	}
}



SCMD(func_timer) {
	BUFFER(arg1, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(arg2, MAX_SCRIPT_STRING_LENGTH);
	int duration;
	ObjData *obj;

	ScriptEngineGetArguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2)
		script_log("timer: bad syntax");
	else if (!(obj = GET_TARGET_OBJ(arg1)))
		script_log("timer: object not found");
	else if ((duration = atoi(arg2)) < 0)
		script_log("timer: invalid duration");
	else
		GET_OBJ_TIMER(obj) = duration;
}


SCMD(func_load) {
	BUFFER(arg1, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(arg2, MAX_SCRIPT_STRING_LENGTH);
	CharData *	mob = NULL;
	ObjData *	object = NULL;

	RoomData *room = this_room(go);

	ScriptEngineGetArguments(argument, arg1, arg2);

	//	Parse it now
	VirtualID	vid(arg2, thread->GetCurrentVirtualID().zone);
		
	if ( !*arg1 || !*arg2 || !IsVirtualID(arg2) ) {
		script_log("load: bad syntax: %s", argument);
		GET_THREAD_VARS(thread).Add("loaded", "0");
	}
	else if (is_abbrev(arg1, "mob"))
	{
		CharacterPrototype *proto = mob_index.Find(vid);
		if (!proto)
			script_log("load: bad mob vnum: %s", argument);
		else
		{
			mob = CharData::Create(proto);
			mob->ToRoom(room);
			finish_load_mob(mob);
		}
		if (!PURGED(trig))
			GET_THREAD_VARS(thread).Add("loaded", mob);
	}
	else if (is_abbrev(arg1, "obj"))
	{
		ObjectPrototype *proto = obj_index.Find(vid);
		if (!proto)
			script_log("load: bad object vnum: %s", argument);
		else
		{
			object = ObjData::Create(proto);
			if (CAN_WEAR(object, ITEM_WEAR_TAKE) && (go->GetEntityType() == Entity::TypeCharacter))
				object->ToChar((CharData *)go);
			else
				object->ToRoom(room);
			load_otrigger(object);
		}
		if (!PURGED(trig))
			GET_THREAD_VARS(thread).Add("loaded", object);
	}
	else
	{
		script_log("load: bad type: %s", argument);
		GET_THREAD_VARS(thread).Add("loaded", "0");
	}
}


/* purge all objects an npcs in room, or specified object or mob */
SCMD(func_purge) {
	BUFFER(arg, MAX_SCRIPT_STRING_LENGTH);
	CharData *ch;
	ObjData *obj;

	ScriptEngineGetArgument(argument, arg);

	if (!*arg) {
		RoomData *room = this_room(go);
		
		FOREACH(CharacterList, room->people, iter)
		{
			ch = *iter;
			if (IS_NPC(ch) && (ch != go))
				ch->Extract();
		}
		FOREACH(ObjectList, room->contents, iter)
		{
			obj = *iter;
			if (obj != go)
				obj->Extract();
		}
	} else if ((ch = GET_TARGET_CHAR(arg))) {
		if (!IS_NPC(ch))
			script_log("purge: purging a PC");
		else
			ch->Extract();
	} else if ((obj = GET_TARGET_OBJ(arg)))
		obj->Extract();
	else
		script_log("purge: bad argument");
}


SCMD(func_at) {
	BUFFER(location, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(arg2, MAX_SCRIPT_STRING_LENGTH);
	RoomData *room = NULL;
	RoomData *old_room = NULL;

	half_chop(argument, location, arg2);

	if (!*location || !*arg2 || (!IsIDString(location) && !IsVirtualID(location)))
		script_log("at: bad syntax: %s", argument);
	else
	{
		if (IsIDString(location))
		{
			Entity *e = IDManager::Instance()->Find(location);
			if (e)
			{
				switch (e->GetEntityType())
				{
					case Entity::TypeCharacter:	room = static_cast<CharData *>(e)->InRoom();	break;
					case Entity::TypeObject:	room = static_cast<ObjData *>(e)->Room();		break;
					case Entity::TypeRoom:		room = static_cast<RoomData *>(e);				break;
				}
			}
		}
		else
			room = GET_TARGET_ROOM(location);
			
		if (room == NULL)
			script_log("at: location not found: %s", argument);
		else {
			switch (go->GetEntityType()) {
				case Entity::TypeCharacter:
#if 0
					old_room = ((CharData *)go)->InRoom();
					if (old_room != room)
					{
						((CharData *)go)->FromRoom();
						((CharData *)go)->ToRoom(room);
					}
					script_command_interpreter(go, trig, thread, arg2);
					if (old_room != room && ((CharData *)go)->InRoom() == room) {
						((CharData *)go)->FromRoom();
						((CharData *)go)->ToRoom(old_room);
					}
					break;
#endif
				case Entity::TypeObject:
				{
#if 0
					ObjData *obj = (ObjData *)go;
					old_room = IN_ROOM(obj);
					obj->SetInRoom(rnum);
					script_command_interpreter(obj, trig, thread, arg2);
					obj->SetInRoom(old_room);
#else
					RoomData *	oldFakeRoom = ScriptObjectAtRoom;
					ScriptObjectAtRoom = room;
					script_command_interpreter(go, trig, thread, arg2);
					ScriptObjectAtRoom = oldFakeRoom;
#endif
					break;
				}
				case Entity::TypeRoom:
					script_command_interpreter(room, trig, thread, arg2);
					break;
				default:
					break;
			}
		}
	}
}


SCMD(func_call) {
	BUFFER(call, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(func, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(result, MAX_SCRIPT_STRING_LENGTH);
	RoomData *	room = NULL;
	CharData *	ch = NULL;
	ObjData *	obj = NULL;
	const char *		arg;

	arg = ScriptEngineGetArguments(argument, call, func);

	if (!*call || !*func)
		script_log("call: bad syntax: %s", argument);
	else if (!IsVirtualID(call) && !IsIDString(call))
		script_log("call: can only call functions on references and room IDs");
	else {
		if (IsIDString(call))
		{
			Entity *e = IDManager::Instance()->Find(call);
			if (e)
			{
				switch (e->GetEntityType())
				{
					case Entity::TypeCharacter:	ch = static_cast<CharData *>(e);	break;
					case Entity::TypeObject:	obj = static_cast<ObjData *>(e);	break;
					case Entity::TypeRoom:		room = static_cast<RoomData *>(e);	break;
				}
			}
		}
		else
		{
			room = world.Find(VirtualID(call, thread->GetCurrentVirtualID().zone));
		}
		
		strcpy(result, "1");
		if (ch)			call_mtrigger(ch, go, func, arg, result);
		else if (obj)	call_otrigger(obj, go, func, arg, result);
		else if (room)	call_wtrigger(room, go, func, arg, result);
		else			script_log("call: unable to find reference");
		
		if (/*!PURGED(go) &&*/ !PURGED(trig))
		{
			GET_THREAD_VARS(thread).Add("result", result);
		}
	}
}


static int func_damage_do_damage(CharData *damager, CharData *victim, int dam, int subcmd, int damtype)
{
	if (NO_STAFF_HASSLE(victim))
	{
		send_to_char("Being the cool staff member you are, you sidestep a trap, obviously placed to kill you.\n", victim);
		return 0;
	}
	
	if (damtype != DAMAGE_UNKNOWN)
	{
		int armor = GET_ARMOR(victim, ARMOR_LOC_NATURAL, damtype);
		if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_SPECIAL_MISSION))
		{
			if (armor < GET_ARMOR(victim, ARMOR_LOC_MISSION, damtype))	
				armor = GET_ARMOR(victim, ARMOR_LOC_MISSION, damtype);
		}
		else
		{
		 	if (armor < GET_ARMOR(victim, ARMOR_LOC_BODY, damtype))
				armor = GET_ARMOR(victim, ARMOR_LOC_BODY, damtype);
		 	
			armor += GET_EXTRA_ARMOR(victim);
		}
		
		if (armor < 0)
			armor = 0;
		
		dam -= (dam * armor) / 100;
	}
	
	int result = damage(damager, victim, NULL, dam, subcmd == SCMD_DAMAGEFULL ? TYPE_SCRIPT : TYPE_MISC, damtype, 1);

	if ((result > 0 || result == -1) && damager && !PURGED(damager) && !AFF_FLAGGED(damager, AFF_RESPAWNING))
	{
		/*if (!IS_NPC(damager))*/
		{
			(new Affect("COMBAT", 0, APPLY_NONE, AFF_NOQUIT))->Join(damager, IS_NPC(victim) ? 10 RL_SEC : 20 RL_SEC);
		}
	}

	if (result > 0 /*&& !IS_NPC(victim)*/  && !PURGED(victim) && !AFF_FLAGGED(victim, AFF_RESPAWNING))
	{
		(new Affect("COMBAT", 0, APPLY_NONE, AFF_NOQUIT))->Join(victim, !damager || IS_NPC(damager) ? 10 RL_SEC : 20 RL_SEC);
	}

	return result;
}


SCMD(func_damage) {
	BUFFER(name, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(amount, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(arg, MAX_SCRIPT_STRING_LENGTH);
	
	CharData *victim, *damager = NULL;
	int damtype;
	int dam;
	
	argument = ScriptEngineGetArguments(argument, name, amount, arg);
	
	if (go->GetEntityType() == Entity::TypeCharacter && IS_NPC((CharData *)go))
			damager = (CharData *)go;		
	
	damtype = DAMAGE_UNKNOWN;
	if (*arg)
	{
		damtype = search_block(arg, damage_types, 1);
		
		if (damtype != DAMAGE_UNKNOWN)
		{
			argument = ScriptEngineGetArgument(argument, arg);
		}
		
		if (*arg)
			damager = GET_TARGET_CHAR(arg);
	}
	
	if (!*name || !*amount)
		script_log("damage: too few arguments");
	else if (!is_number(amount))
		script_log("damage: bad syntax");
	else {
		dam = atoi(amount);
		
		if (!str_cmp(name, "all")) {
			RoomData *room = this_room(go);
			if (room && !ROOM_FLAGGED(room, ROOM_PEACEFUL)) {
				FOREACH(CharacterList, room->people, iter)
				{
					victim = *iter;
					
					if (victim == (CharData *)go)
						continue;
					
					func_damage_do_damage(damager, victim, dam, subcmd, damtype);
				}
			}
		} else if ((victim = GET_TARGET_CHAR(name)))
		{
			int result = func_damage_do_damage(damager, victim, dam, subcmd, damtype);
			if (!PURGED(trig))
				GET_THREAD_VARS(thread).Add("killed", result == -1 ? "1" : "0");
		}
		else
			script_log("damage: target not found");
	}
}

#if 0
SCMD(func_damageen) {
	BUFFER(name, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(amount, MAX_SCRIPT_STRING_LENGTH);
	
	CharData *victim;
	
	ScriptEngineGetArguments(argument, name, amount);
	
	if (!*name || !*amount)
		script_log("damageen: too few arguments");
	else if (!is_number(amount))
		script_log("damageen: bad syntax");
	else {
		int dam = atoi(amount);
		
		if (!str_cmp(name, "all")) {
			RoomData * room = this_room(go);
			if (room && !ROOM_FLAGGED(room, ROOM_PEACEFUL)) {
				START_ITER(iter, victim, CharData *, room->people) {
					if (victim == (CharData *)go)
						continue;
					
					AlterEndurance(victim, -dam);
				}
			}
		}
		else if ((victim = GET_TARGET_CHAR(name)))
			AlterEndurance(victim, -dam);
		else
			script_log("damageen: target not found");
	}
}
#endif

SCMD(func_damagemv) {
	BUFFER(name, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(amount, MAX_SCRIPT_STRING_LENGTH);
	
	CharData *victim;
	
	ScriptEngineGetArguments(argument, name, amount);
	
	if (!*name || !*amount)
		script_log("damagemv: too few arguments");
	else if (!is_number(amount))
		script_log("damagemv: bad syntax");
	else {
		int dam = atoi(amount);
		
		if (!str_cmp(name, "all")) {
			RoomData * room = this_room(go);
			if (room && !ROOM_FLAGGED(room, ROOM_PEACEFUL)) {
				FOREACH(CharacterList, room->people, iter)
				{
					victim = *iter;
					
					if (victim == (CharData *)go)
						continue;
					
					AlterMove(victim, -dam);
				}
			}
		} else if ((victim = GET_TARGET_CHAR(name)))
		{
			AlterMove(victim, -dam);
		}
		else
			script_log("damagemv: target not found");
	}
}


SCMD(func_heal) {
	BUFFER(name, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(amount, MAX_SCRIPT_STRING_LENGTH);
	
	CharData *victim;
	
	ScriptEngineGetArguments(argument, name, amount);

	if (!*name || !*amount)
		script_log("heal: too few arguments");
	else if (!is_number(amount))
		script_log("heal: bad syntax");
	else if (!str_cmp(name, "all")) {
		RoomData *room = this_room(go);
		FOREACH(CharacterList, room->people, iter)
		{
			victim = *iter;
			AlterHit(victim, atoi(amount));
		}
	} else if (!(victim = GET_TARGET_CHAR(name)))
		script_log("heal: target not found");
	else
	{
		AlterHit(victim, atoi(amount));
	}
}


#if 0
SCMD(func_healen) {
	BUFFER(name, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(amount, MAX_SCRIPT_STRING_LENGTH);
	
	CharData *victim;
	
	ScriptEngineGetArguments(argument, name, amount);

	if (!*name || !*amount)
		script_log("healen: too few arguments");
	else if (!is_number(amount))
		script_log("healen: bad syntax");
	else
	{
		int heal = atoi(amount);
		
		if (!str_cmp(name, "all")) {
			RoomData *room = this_room(go);
			START_ITER(iter, victim, CharData *, room->people) {
				AlterEndurance(victim, heal);
			}
		} else if ((victim = GET_TARGET_CHAR(name)))
			AlterEndurance(victim, heal);
		else
			script_log("healmv: target not found");
	}
}
#endif


SCMD(func_healmv) {
	BUFFER(name, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(amount, MAX_SCRIPT_STRING_LENGTH);
	
	CharData *victim;
	
	ScriptEngineGetArguments(argument, name, amount);

	if (!*name || !*amount)
		script_log("healmv: too few arguments");
	else if (!is_number(amount))
		script_log("healmv: bad syntax");
	else
	{
		int heal = atoi(amount);
		
		if (!str_cmp(name, "all")) {
			RoomData *room = this_room(go);
			FOREACH(CharacterList, room->people, iter)
			{
				victim = *iter;
				AlterMove(victim, heal);
			}
		} else if ((victim = GET_TARGET_CHAR(name)))
			AlterMove(victim, heal);
		else
			script_log("healmv: target not found");
	}
}


SCMD(func_remove) {
	BUFFER(vict, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(what, MAX_SCRIPT_STRING_LENGTH);
	
	CharData *	victim;
	ObjData *	obj;
	int			x;
	bool		fAll = false;
	VirtualID	vid;
	
	ScriptEngineGetArguments(argument, vict, what);
	
	if (!*vict || !*what || (!IsVirtualID(what) && str_cmp(what, "all")))
		script_log("remove: bad syntax");
	else if ((victim = GET_TARGET_CHAR(vict)))
	{
		if (!str_cmp(what, "all"))	fAll = true;
		else 						vid = VirtualID(what, thread->GetCurrentVirtualID().zone);
		FOREACH(ObjectList, victim->carrying, iter)
		{
			obj = *iter;
			if (fAll ? !NO_LOSE(obj, victim) : (obj->GetVirtualID() == vid))
				obj->Extract();
		}
		for (x = 0; x < NUM_WEARS; x++) {
			obj = GET_EQ(victim, x);
			if (obj && (fAll ? !NO_LOSE(obj, victim) : (obj->GetVirtualID() == vid)))
				obj->Extract();
		}
	}
}


SCMD(func_do)
{
	extern void show_obj_to_char(ObjData * object, CharData * ch, int mode);
	extern void look_at_char(CharData *i, CharData *ch);

	BUFFER(what, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(action, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(arg, MAX_SCRIPT_STRING_LENGTH);
	
	const char *params = ScriptEngineGetArguments(argument, what, action);
	
	if (!IsIDString(what) || !*action)
		script_log("do: bad syntax: %s", argument);
	else
	{
		if (!str_cmp(action, "lookat"))
		{
			CharData *actor = get_char(what);
			
			RoomData *targetRoom = NULL;
			CharData *targetChar = NULL;
			ObjData *targetObj = NULL;
			
			params = ScriptEngineGetArguments(params, arg);
			
			if (!(targetRoom = get_room(arg, thread->GetCurrentVirtualID().zone)))
				if (!(targetChar = get_char(arg)))
					targetObj = get_obj(arg);
			
			if (!actor)
				script_log("do: missing actor: %s", argument);
			else if (!targetRoom && !targetChar && !targetObj)
				script_log("do: missing target: %s", argument);
			else if (targetRoom)
				look_at_room(actor, targetRoom, str_str(params, "+nobrief") != NULL);
			else if (targetChar)
				look_at_char(targetChar, actor);
			else if (targetObj)
				show_obj_to_char(targetObj, actor, 5);
		}
		else if (!str_cmp(action, "move"))
		{
			CharData *actor = get_char(what);
			
			params = ScriptEngineGetArguments(params, arg);
			
			int dir = GetEnumByNameAbbrev<Direction>(arg);
			
			if (!actor)
				script_log("do: missing actor: %s", argument);
			else if (dir == -1)
				script_log("do: invalid direction: %s", argument);
			else
			{
				Flags flags = 0;
				
				if (str_str(params, "+quiet"))	flags |= MOVE_QUIET;
				
				int result = perform_move(actor, dir, flags);
				if (!PURGED(trig))
					GET_THREAD_VARS(thread).Add("result", result);
			}
		}
		else
			script_log("do: unknown action %s", action);
	}
}


/* lets the mobile goto any location it wishes that is not private */
SCMD(func_goto) {
	BUFFER(arg, MAX_SCRIPT_STRING_LENGTH);
	RoomData * location;

	if (go->GetEntityType() != Entity::TypeCharacter)
		return;

	ScriptEngineGetArgument(argument, arg);

	if (!*arg)
		script_log("goto called with no argument");
	else if ((location = find_target_room_by_char(thread, (CharData *)go, arg)) == NULL)
		script_log("goto: invalid location");
	else
	{
#if !ROAMING_COMBAT
		if (FIGHTING((CharData *)go))
			((CharData *)go)->stop_fighting();
#endif

		if (((CharData *)go)->InRoom())
			((CharData *)go)->FromRoom();
		((CharData *)go)->ToRoom(location);
	}
}


SCMD(func_info) {
	void announce(const char *info);
		
	skip_spaces(argument);
	
	if (!*argument)
		script_log("info called with no argument");
//	else if (type == MOB_TRIGGER)	do_gen_comm((CharData *)go, argument, 0, "broadcast", SCMD_BROADCAST);
	else {
		announce(argument);
	}
}


SCMD(func_copy) {
	BUFFER(arg1, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(arg2, MAX_SCRIPT_STRING_LENGTH);
	
	int 		number = 0;
	CharData *	mob = NULL;
	ObjData *	obj = NULL;

	RoomData *room = this_room(go);


	ScriptEngineGetArguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2)
		script_log("copy: bad syntax");
	else if (is_abbrev(arg1, "mob")) {
		if (!(mob = GET_TARGET_CHAR(arg2)))		script_log("copy: mob not found");
		else if (!(mob = CharData::Create(mob)))script_log("copy: error in \"new CharData(mob)\"");
		else									mob->ToRoom(room);
		if (!PURGED(trig))
			GET_THREAD_VARS(thread).Add("copied", mob);
	} else if (is_abbrev(arg1, "obj")) {
		if (!(obj = GET_TARGET_OBJ(arg2)))		script_log("copy: mob not found");
		else if (!(obj = ObjData::Clone(obj)))	script_log("copy: error in \"ObjData::Clone(obj)\"");
		else if (CAN_WEAR(obj, ITEM_WEAR_TAKE) && (go->GetEntityType() == Entity::TypeCharacter))
			obj->ToChar((CharData *)go);
		else									obj->ToRoom(room);
		if (!PURGED(trig))
			GET_THREAD_VARS(thread).Add("copied", obj);
	} else
		script_log("copy: bad type");
}


SCMD(func_lag) {
	BUFFER(arg1, MAX_SCRIPT_STRING_LENGTH);
	CharData *ch;
	char c = 's';
	int delay = 0;
	
	const char *args = ScriptEngineGetArguments(argument, arg1);

	if (!*arg1 || !*args)
		script_log("lag called with invalid arguments");
	else if ((ch = get_char(arg1)) == NULL)
		script_log("lag: invalid target");
//	else if ((delay = atoi(arg2)) <= 0)
//		script_log("lag: invalid delay (<= 0)");
//	else if ((delay > 20))
//		script_log("lag: invalid delay (> 20)");
	else if (sscanf(args, "%d %c", &delay, &c) < 1)
		script_log("lag: invalid delay (<= 0)");
	else
	{
		if (c == 'p')		;							//	'p': pulses
		else 				delay *= PASSES_PER_SEC;	//	Default: seconds
	
		if ((delay > 20 RL_SEC))
		{
			script_log("lag: invalid delay (> 20)");
			delay = 20 RL_SEC;
		}
		
		WAIT_STATE(ch, delay);
	}
}


/*
SCMD(func_destroy) {
	BUFFER(arg, MAX_SCRIPT_STRING_LENGTH);
	
	ObjData *obj;

	ScriptEngineGetArgument(argument, arg);

	if (!*arg)
		script_log("destroy: invalid argument");
	else if ((obj = GET_TARGET_OBJ(arg)) == NULL)
		script_log("destroy: invalid object");
	else if (GET_OBJ_TYPE(obj) != ITEM_INSTALLED && GET_OBJ_TYPE(obj) != ITEM_TELEPORTER)
		script_log("destroy: invalid object");
	else {
		if (subcmd == SCMD_DESTROY)
		{
			if (obj->GetValue(1) == 0)
				obj->Extract();
			else
				obj->SetValue(2, 1);	//	Mark as destroyed
		}
		else if (subcmd == SCMD_REPAIR)
			obj->SetValue(2, 0);	//	Repair it
	}
}
*/

SCMD(func_move) {
	BUFFER(arg1, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(arg2, MAX_SCRIPT_STRING_LENGTH);
	
	RoomData *troom = NULL;
	ObjData *tobj = NULL, *sobj = NULL;
	CharData *tch = NULL, *sch = NULL;


	ScriptEngineGetArguments(argument, arg1, arg2);

	if (!*arg1)
		script_log("move called with no arguments");
	else if (!*arg2)
		script_log("move called with no destination");
	else if (!IsIDString(arg2) && !IsVirtualID(arg2))
		script_log("move target is an invalid '%s'", arg2);
	else {
		if (!(sch = get_char(arg1)))
			sobj = get_obj(arg1);
		
		if (!(troom = get_room(arg2, thread->GetCurrentVirtualID().zone)))
			if (!(tch = get_char(arg2)))
				tobj = get_obj(arg2);

		if (!troom && !tch && !tobj)
			script_log("move: no destination");
		else {
			if (sch) {
				while (tobj) {
					//	Can't do this... instead find the absolute upper room or char
					if (tobj->Inside())			tobj = tobj->Inside();
					else {
						if (tobj->CarriedBy())	tch = tobj->CarriedBy();
						else if (tobj->WornBy())tch = tobj->WornBy();
						else					troom = tobj->InRoom();
						break;
					}
				}
				
				if (tch)	troom = tch->InRoom();

				if (troom == sch->InRoom()) {
					//	Same room... but no error should occur
				} else if (!House::CanEnter(sch, troom) || troom->GetFlags().testForAny(PrivateRoomFlags))
					script_log("move: attempting to transfer a character to a house or private room.");
				else {
#if !ROAMING_COMBAT
					if (FIGHTING(sch))		sch->stop_fighting();
#endif
					sch->FromRoom();
					sch->ToRoom(troom);
				}
			} else if (sobj) {
				while (tobj && GET_OBJ_TYPE(tobj) != ITEM_CONTAINER) {
					//	Can't do this... instead move it to the super-container, character, or room
					if (tobj->Inside())			tobj = tobj->Inside();
					else {
						if (tobj->CarriedBy())	tch = tobj->CarriedBy();
						else if (tobj->WornBy())tch = tobj->WornBy();
						else					troom = tobj->InRoom();
						tobj = NULL;
					}
				}
				
				if (sobj->WornBy())			unequip_char(sobj->WornBy(), sobj->WornOn());
				else if (sobj->Inside())	sobj->FromObject();
				else if (sobj->CarriedBy())	sobj->FromChar();
				else						sobj->FromRoom();
				
				if (troom)			sobj->ToRoom(troom);
				else if (tch)		sobj->ToChar(tch);
				else if (tobj)		sobj->ToObject(tobj);
			} else
				script_log("move: no source");
		}
	}
}


SCMD(func_withdraw) {
	BUFFER(arg1, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(arg2, MAX_SCRIPT_STRING_LENGTH);
	Clan *	clan;
	int		amount;

	ScriptEngineGetArguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2)
		script_log("withdraw called with invalid arguments: %s", argument);
	else if (!is_number(arg1) || !(clan = Clan::GetClan(atoi(arg1))))
		script_log("withdraw: invalid clan '%s'", arg1);
	else if (!is_number(arg2) || (amount = atoi(arg2)) <= 0)
		script_log("withdraw: invalid amount (<= 0) '%s'", arg2);
	else if (clan->m_Savings < amount)
		GET_THREAD_VARS(thread).Add("successful", "0");
	else {
		clan->m_Savings -= amount;
		
		mudlogf(CMP, LVL_STAFF, TRUE, "CLANS: Script [%s] withdrew %d from clan %s [%d]",
				trig->GetVirtualID().Print().c_str(), amount, clan->GetName(), clan->GetID());
				
/*		DescriptorData *d;
		START_ITER(iter, d, DescriptorData *, descriptor_list) {
			if (STATE(d) == CON_PLAYING && GET_CLAN(d->m_Character) == Clan::Index[clan]->vnum)
				d->m_Character->Send("`y[`bCLAN`y]`n %d CP was automatically withdrawn from the clan savings.\n", amount);
		}
*/
		GET_THREAD_VARS(thread).Add("successful", "1");
	}
}


SCMD(func_deposit) {
	BUFFER(arg1, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(arg2,MAX_SCRIPT_STRING_LENGTH);
	Clan *	clan;
	int		amount;
	
	ScriptEngineGetArguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2)
		script_log("deposit called with invalid arguments");
	else if (!is_number(arg1) || !(clan = Clan::GetClan(atoi(arg1))))
		script_log("deposit: invalid clan '%s'", arg1);
	else if (!is_number(arg2) || (amount = atoi(arg2)) <= 0)
		script_log("deposit: invalid amount (<= 0) '%s'", arg2);
	else {
		clan->m_Savings += amount;
		Clan::Save();
		
		mudlogf(CMP, LVL_STAFF, TRUE, "CLANS: Script [%s] deposited %d into clan `(%s`) [%d]",
				trig->GetVirtualID().Print().c_str(), amount, clan->GetName(), clan->GetID());
				
/*		DescriptorData *d;
		START_ITER(iter, d, DescriptorData *, descriptor_list) {
			if (STATE(d) == CON_PLAYING && GET_CLAN(d->m_Character) == Clan::Index[clan]->vnum)
				d->m_Character->Send("`y[`bCLAN`y]`n %d CP was automatically deposited into the clan savings.\n", amount);
		}
*/
	}
}


SCMD(func_log) {
	if (!*argument)
		script_log("log: nothing to log");
	else {
		switch (go->GetEntityType()) {
			case Entity::TypeCharacter:
				mudlogf(NRM, 101, TRUE, "SCRIPTLOG: Mob [%s] \"`(%s`)\" (script %s): `(%s`)",
						((CharData *)go)->GetVirtualID().Print().c_str(),
						((CharData *)go)->GetName(),
						trig->GetVirtualID().Print().c_str(),
						argument);
				break;
			case Entity::TypeObject:
				mudlogf(NRM, 101, TRUE, "SCRIPTLOG: Obj [%s] \"`(%s`)\" (script %s): `(%s`)",
						((ObjData *)go)->GetVirtualID().Print().c_str(),
						((ObjData *)go)->GetName(),
						trig->GetVirtualID().Print().c_str(),
						argument);
				break;
			case Entity::TypeRoom:
				mudlogf(NRM, 101, TRUE, "SCRIPTLOG: Room [%s] (script %s): `(%s`)",
						((RoomData *)go)->GetVirtualID().Print().c_str(),
						trig->GetVirtualID().Print().c_str(),
						argument);
				break;
			default:
				mudlogf(NRM, 101, TRUE, "SCRIPTLOG: Unknown (Ptr %p) (script %s): `(%s`)", go, trig->GetVirtualID().Print().c_str(), argument);
				break;
		}
	}
}


SCMD(func_restring)
{
	BUFFER(entity, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(field, MAX_SCRIPT_STRING_LENGTH);
	CharData *vict;
	ObjData *obj = NULL;

	argument = ScriptEngineGetArguments(argument, entity, field);
	
	IDNum	uid = ParseIDString(entity);
	
	if (!*argument || !*entity || !*field)
		script_log("string: invalid arguments");
	else if (uid == 0)
		script_log("string: entity must be a reference");
	else if (!(vict = CharData::Find(uid)) && !(obj = ObjData::Find(uid)))
		script_log("string: cannot find target for string");
	else if (!str_cmp(field, "name"))
	{
		if (vict)
		{
			if (IS_NPC(vict))
				vict->m_Keywords = argument;
			else
				script_log("string: cannot alter PC names");
		}
		else if (obj)
			obj->m_Keywords = argument;
	}
	else if (!str_cmp(field, "short"))
	{
		if (vict)
		{
			if (IS_NPC(vict))
				vict->m_Name = argument;
			else
				script_log("string: PCs dont have shortdescs");
		}
		else if (obj)
		{
			obj->m_Name = argument;
		}
	}
	else if (!str_cmp(field, "long"))
	{
		if (vict)
		{
			sprintf(field, "%s\n", argument);
			if (IS_NPC(vict))
				vict->m_RoomDescription = field;
			else
				script_log("string: PCs dont have longdescs");
		}
		else if (obj)
		{
			obj->m_RoomDescription = argument;
		}
	}
	else if (!str_cmp(field, "description"))
	{
		if (vict)
		{
			script_log("string: characters dont have longdescs");
		}
		else if (obj)
		{
			BUFFER(newdesc, MAX_SCRIPT_STRING_LENGTH);
			strcpy(newdesc, argument);
			char *newline = newdesc;
			
			while ((newline = strstr(newline, "$_")))
				strncpy(newline, "\n", 2);
			
			obj->m_Description = newdesc;
		}
	}
	else if (!str_cmp(field, "title"))
	{
		if (vict)
		{
			if (IS_NPC(vict))
				script_log("string: cannot alter NPC titles");
			else if (vict->GetLevel() >= LVL_STAFF)
				script_log("string: cannot alter staff titles");
			else
				vict->m_Title = argument;
		}
		else if (obj)
				script_log("string: Objects dont have titles");
	}
	else
		script_log("string: invalid field");
}


SCMD(func_affect)
{
	BUFFER(target, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(argDuration, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(affecttype, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(modification, MAX_SCRIPT_STRING_LENGTH);
	
	Affect *	affect;
	CharData *	vict;
	const char *arg;
	int			amount = 0;
	int			flag;
	bool		stackable = false;
	bool		override = false;
	
	arg = ScriptEngineGetArguments(argument, target, argDuration, affecttype, modification);
	
	IDNum		uid = ParseIDString(target);
	
	if (!*target || !*argDuration || !*affecttype || !*modification)
		script_log("affect called with invalid arguments: %s", argument);
	else if (uid == 0)
		script_log("affect: target must be a reference: %s", argument);
	else if (!(vict = CharData::Find(uid)))
		script_log("affect: no target found: %s", argument);
	else
	{
		int loc = 0;
		Flags joinflags = 0;
		int duration = 0;
		Affect::Flags flags;
		
		if (!is_abbrev(argDuration, "permanent"))
		{
			char c = 's';
			if (sscanf(argDuration, "%d %c", &duration, &c) >= 1)
			{
				switch (c)
				{
					case 'h':	duration *= 60;
						//	Fall through
					case 'm':	duration *= 60;
						//	Fall through
					case '\0':
					case 's':	duration *= PASSES_PER_SEC;
						//	Fall through
					case 'p':	break;
					default:
						duration = -1;	//	Cause a failure condition
				}
			}
			
			if (duration <= 0 || duration > (24 * 60 * 60 * PASSES_PER_SEC))
			{
				script_log("affect: duration must be between 1p and 24h (or 'permanent' to last until death): %s", argument);
				return;
			}
		}
		
		loc = GetEnumByName<AffectLoc>(modification);
		if (loc < 1)
			loc = -find_skill_abbrev(modification);
		
		if (loc)
		{
			arg = ScriptEngineGetArgument(arg, modification);
			
			amount = atoi(modification);
			if (amount == 0)
				loc = 0;
			
			arg = ScriptEngineGetArgument(arg, modification);
		}
/*		else if ((flag = search_block(modification, affected_bits, true)) != -1)
				SET_BIT(flags, 1 << flag);
		else
		{
			script_log("affect: unknown modification");
		}
*/		
		while (*modification)
		{
			if (!str_cmp(modification, "stackable"))		stackable = true;
			else if (!str_cmp(modification, "override"))	override = true;
			else if (!str_cmp(modification, "addtime"))		joinflags |= Affect::AddDuration;
			else if (!str_cmp(modification, "avgtime"))		joinflags |= Affect::AverageDuration;
			else if (!str_cmp(modification, "shortest"))	joinflags |= Affect::ShortestDuration;
			else if (!str_cmp(modification, "longest"))		joinflags |= Affect::LongestDuration;
			else if (!str_cmp(modification, "oldtime"))		joinflags |= Affect::OldDuration;
			else if (!str_cmp(modification, "addmod"))		joinflags |= Affect::AddModifier;
			else if (!str_cmp(modification, "avgmod"))		joinflags |= Affect::AverageModifier;
			else if (!str_cmp(modification, "smallest"))	joinflags |= Affect::SmallestModifier;
			else if (!str_cmp(modification, "largest"))		joinflags |= Affect::LargestModifier;
			else if (!str_cmp(modification, "oldmod"))		joinflags |= Affect::OldModifier;
			else if ((flag = GetEnumByNameAbbrev<AffectFlag>(modification)) != -1)
				flags.set((AffectFlag)flag);
			else
				script_log("affect: unknown flag: %s", argument);
			
			arg = ScriptEngineGetArgument(arg, modification);
		}
		
		if (loc || flags.any())
		{
			affect = new Affect(affecttype, amount, loc, flags);
			if (stackable)
				affect->ToChar(vict, duration);
			else
				affect->Join(vict, duration, joinflags);
		}
		else
			script_log("affect: unknown modifications: %s", argument);
	}
}


SCMD(func_affectmsg)
{
	BUFFER(target, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(affecttype, MAX_SCRIPT_STRING_LENGTH);
	const char *message;
	CharData *	vict;
	
	message = ScriptEngineGetArguments(argument, target, affecttype);
	
	IDNum uid = ParseIDString(target);
	
	if (!*target || !*affecttype || !*message)
		script_log("affect called with invalid arguments: %s", argument);
	else if (uid == 0)
		script_log("affect: target must be a reference");
	else if (!(vict = CharData::Find(uid)))
		script_log("affect: no target found");
	else
		Affect::SetFadeMessage(vict, affecttype, message);
}


SCMD(func_unaffect)
{
	BUFFER(target, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(affecttype, MAX_SCRIPT_STRING_LENGTH);
	CharData *	vict;
	
	ScriptEngineGetArguments(argument, target, affecttype);
	
	IDNum uid = ParseIDString(target);
	
	if (!*target || !*affecttype)
		script_log("unaffect called with invalid arguments");
	else if (uid == 0)
		script_log("affect: target must be a reference");
	else if (!(vict = CharData::Find(uid)))
		script_log("affect: no target found");
	else
		Affect::FromChar(vict, affecttype);
}


SCMD(func_attach)
{
	BUFFER(trigger, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(target, MAX_SCRIPT_STRING_LENGTH);
	ScriptPrototype *proto;
	Entity *	e;
	
	
	ScriptEngineGetArguments(argument, trigger, target);
	
	if (!*trigger || !*target)
		script_log("attach called with invalid arguments: %s", argument);
	else if ((proto = trig_index.Find(VirtualID(trigger, thread->GetCurrentVirtualID().zone))) == NULL)
		script_log("attach: invalid vnum '%s'", trigger);
	else
	{
		//	Redo this sometime so its references and number only
		if ((e = get_room(target, thread->GetCurrentVirtualID().zone)))	{ }
		else if ((e = get_char(target)))
		{
			if (IS_STAFF((CharData *)e))
			{
				script_log("attach: cannot attach to staff; attempting to attach to '`(%s`)'", ((CharData *)e)->GetName());
				return;
			}
		}
		else if ((e = get_obj(target)))		{ }
		
		if (e)
		{
			FOREACH(TrigData::List, TRIGGERS(SCRIPT(e)), iter)
			{
				if ((*iter)->GetPrototype() == proto)
					return;
			}
			
			e->GetScript()->AddTrigger(TrigData::Create(proto));
		}
		else
			script_log("attach: unable to find target '%s'", target);
	}
}

SCMD(func_detach)
{
	BUFFER(trigger, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(target, MAX_SCRIPT_STRING_LENGTH);
	Entity *	e;
	
	ScriptEngineGetArguments(argument, trigger, target);
	
	if (!*trigger || !*target)
		script_log("detach called with invalid arguments: %s", argument);
	else
	{
		if ((e = get_char(target)))			{ }
		else if ((e = get_obj(target)))		{ }
		else if ((e = get_room(target, thread->GetCurrentVirtualID().zone)))	{ }
		
		if (e)
			e->GetScript()->RemoveTrigger(trigger);
		else
			script_log("detach: unable to find target '%s'", target);
	}
}


SCMD(func_zreset)
{
	BUFFER(zonename, MAX_INPUT_LENGTH);
	
	ScriptEngineGetArgument(argument, zonename);
	ZoneData *zone = zone_table.Find(zonename);
	
	if (!*zonename)
		script_log("zreset called with invalid arguments: %s", zonename);
	else if (!zone)
		script_log("zoneecho called for nonexistant zone '%s'", zonename);
	else
	{
		BUFFER(logMessage, MAX_SCRIPT_STRING_LENGTH);
		
		sprintf(logMessage, "Reset zone '%s' (`(%s`))", zone->GetTag(), zone->GetName());
		
		func_log(go, trig, thread, logMessage, 0);
		reset_zone(zone);
	}
}


//	If we change to doing the thread as a command; this is unlikely because the parsing is more robust
//	for internally handled commands
#if 0
SCMD(func_thread)
{
	BUFFER(entity, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(functionName, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(functionArgs, MAX_SCRIPT_STRING_LENGTH);
	
	argument = ScriptEngineGetArgument(argument, entity);
		
	Entity *e = CharData::Find(ParseIDString(entity));
	
	if (!e)
	{
		ScriptEngineReportError(NULL, true, "Script \"`(%s`)\" [%s] unable to find entity \"%s\" for threading!",
				thread->GetCurrentFunctionScript()->GetName(),
				thread->GetCurrentFunctionScript()->GetVirtualID().Print().c_str(),
				entity);
		return;
	}
	
	char *dst = functionName;
	while (*argument && !isspace(*argument) && *argument != '(')
	{
		*dst++ = *argument++;
	}
	*dst = 0;
	
	skip_spaces(argument);
	
	if (*argument == '(')
	{
		pp = ScriptParserMatchParenthesis(p);
		if (!*pp) ScriptParserParseScriptReportError("Unmatched '('");
		p = ScriptParserExtractContents(p, pp, buf);
#error FINISH THIS
	}

	
	TrigData *	functionTrig = NULL;
	CompiledScript::Function *	pFunction = ScriptEngineRunScriptFindFunction(thread, functionName, functionTrig);

	if (!pFunction)
	{
		ScriptEngineReportError(NULL, true, "Script \"%s\" [%s] unable to find function \"%s\" for threading!",
				thread->GetCurrentFunctionScript()->GetName(),
				thread->GetCurrentFunctionScript()->GetVirtualID().Print().c_str(),
				functionName);
		return;
	}
	
	TrigData * newTrig = NULL;
	START_ITER(iter, newTrig, TrigData *, e->GetScript()->m_Triggers)
	{
		if (newTrig->GetPrototype() == NULL && !str_cmp(newTrig->GetName(), "<Threaded>"))
			break;
	}
	
	if (!newTrig)
	{
//		newTrig = new TrigData(*functionTrig);
//		newTrig->m_Triggers = 0;
		newTrig = new TrigData;
		newTrig->SetName("<Threaded>");
		
		trig_list.push_front(newTrig);
		
		switch (e->GetEntityType()) {
			case Entity::TypeCharacter:	newTrig->m_Type = MOB_TRIGGER;	break;
			case Entity::TypeObject:	newTrig->m_Type = OBJ_TRIGGER;	break;
			case Entity::TypeRoom:		newTrig->m_Type = WLD_TRIGGER;	break;
			default:					newTrig->m_Type = -1;			break;
		}
		
		e->GetScript()->AddTrigger(newTrig);
	}
	
	TrigThread *newThread = new TrigThread(newTrig, e, TrigThread::DontCreateRoot);
	if (newThread->PushCallstack(functionTrig, pFunction->offset, pFunction->name.c_str()))
	{
		add_var(newThread->GetCurrentVariables(), "arg", functionArgs, 0);
		
		bytecode_t *bytecodePtr = newThread->GetCurrentBytecode()->bytecode + newThread->GetCurrentBytecodePosition();
		if (*(bytecodePtr) != SBC_START)
		{
			ScriptEngineReportError(newThread, true, "Script doesn't start with SBC_START opcode!");
		}
		else
		{
			//	Skip SBC opcode
			newThread->SetCurrentBytecodePosition(newThread->GetCurrentBytecodePosition() + 1);
		}
	}
	
	ScriptEngineRunScript(newThread);
}
#endif

SCMD(func_killthread)
{
	BUFFER(record, MAX_SCRIPT_STRING_LENGTH);
	
	ScriptEngineGetArgument(argument, record);
	
	if (!*record || !ScriptEngineRecordIsRecordStart(record))
		script_log("killthread called with invalid arguments: %s", argument);
	else
	{
		BUFFER(entity, MAX_SCRIPT_STRING_LENGTH);
		BUFFER(threadid, MAX_SCRIPT_STRING_LENGTH);
		
		ScriptEngineRecordGetField(record, "entity", entity);
		ScriptEngineRecordGetField(record, "thread", threadid);
		
		if (IsIDString(entity))
		{
			Entity *e = IDManager::Instance()->Find(entity);
			if (e)
			{
				TrigThread *pThread = e->GetScript()->GetThread(atoi(threadid));
				
				if (pThread)	pThread->Terminate();
			}
		}
	}
}


SCMD(func_transform)
{
	BUFFER(arg1, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(arg2, MAX_SCRIPT_STRING_LENGTH);
	
	ScriptEngineGetArguments(argument, arg1, arg2);
	
	if (!IsIDString(arg1) || !IsVirtualID(arg2))
	{
		script_log("copy: bad syntax");	
		return;
	}
	
	Entity *e = IDManager::Instance()->Find(arg1);
	VirtualID	vid(arg2);

	if(e->GetEntityType() == Entity::TypeObject)
	{
		ObjectPrototype *proto = obj_index.Find(vid);
		if (!proto)
		{
			script_log("transform: unsupported type of target");
			return;
		}
		
		ObjData *obj = ObjData::Create(proto);

		IDManager::Instance()->Unregister(obj);
		IDManager::Instance()->Unregister(e);

		IDNum oldID = e->GetID();
		e->SetID(obj->GetID());
		obj->SetID(oldID);

		IDManager::Instance()->Register(obj);
		IDManager::Instance()->Register(e);
		
		ObjData *old = dynamic_cast<ObjData *>(e);
		if (old->InRoom())			obj->ToRoom(old->InRoom());
		else if (old->CarriedBy())	obj->ToChar(old->CarriedBy());
		else if (old->Inside())		obj->ToObject(old->Inside());
		else if (old->WornBy())
		{
			CharData *		worn_by = old->WornBy();
			unsigned int	worn_on = old->WornOn();
			
			unequip_char(old->WornBy(), old->WornOn());
			equip_char(worn_by, obj, worn_on);
		}
		
		old->Extract();
	}
	else
	{
		script_log("transform: unsupported type of target");
		return;
	}
}


#if 0
SCMD(func_addtrigger)
{
	BUFFER(type, MAX_SCRIPT_STRING_LENGTH);
	BUFFER(numarg, MAX_SCRIPT_STRING_LENGTH);
	
	const char *arg = ScriptEngineGetArguments(argument, type);

	if (!strn_cmp(argument, "as ", 3))
	{
		script_log("addtrigger called with invalid arguments: %s", argument);
		return;
	}
	
	Flags type = 0;
	
	switch (go->GetEntityType() == Entity::TypeCharacter)
	{
		
	}
}
#endif


SCMD(func_team)
{
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	
	const char *args = ScriptEngineGetArguments(argument, arg1, arg2);
	
	int team = search_block(arg1, team_names, 0);
	
	if (team <= 0)
		script_log("team: invalid team '%s': %s", arg1, argument);
	else if (!str_cmp(arg2, "set"))
	{
		args = ScriptEngineGetArguments(args, arg1, arg2);
		
		if (!str_cmp(arg1, "loadroom"))
		{
			RoomData *room = NULL;
			
			if (!*arg2)
				script_log("team: invalid usage of set loadroom: %s", argument);
			else if ((room = world.Find(VirtualID(arg2, thread->GetCurrentVirtualID().zone))) == NULL && atoi(arg2) != -1)
				script_log("team: loadroom does not exist: %s", argument);
			else
			{
				if (room == NULL)	mudlogf(NRM, LVL_STAFF, TRUE, "(GC) Script [%s] team '%s' recall reset", trig->GetVirtualID().Print().c_str(), team_names[team]);
				else				mudlogf(NRM, LVL_STAFF, TRUE, "(GC) Script [%s] team '%s' recall set to %s", trig->GetVirtualID().Print().c_str(), team_names[team], room->GetVirtualID().Print().c_str());
				team_recalls[team] = room;
			}
		}
		else
			script_log("team: invalid usage: %s", argument);
	}
	else if (!str_cmp(arg2, "reset"))
	{
		team_recalls[team] = NULL;
		mudlogf(CMP, LVL_STAFF, TRUE, "(GC) Script [%s] team '%s' reset", trig->GetVirtualID().Print().c_str(), team_names[team]);

		FOREACH(CharacterList, character_list, iter)
		{
			CharData *vict = *iter;
			if (!IS_NPC(vict) && GET_TEAM(vict) == team)
				GET_TEAM(vict) = 0;
		}
	}
	else if (!str_cmp(arg2, "add"))
	{
		while (*args)
		{
			args = ScriptEngineGetArguments(args, arg1);
			CharData *vict = get_player(arg1);
			if (vict)
			{
				mudlogf(CMP, LVL_STAFF, TRUE, "(GC) Script [%s] added player '%s' to team '%s'", trig->GetVirtualID().Print().c_str(), vict->GetName(), team_names[team]);
				GET_TEAM(vict) = team;
			}
			else
				script_log("team: invalid player '%s': %s", arg1, argument);
		}
	}
	else if (!str_cmp(arg2, "remove"))
	{
		while (*args)
		{
			args = ScriptEngineGetArguments(args, arg1);
			CharData *vict = get_player(arg1);
			if (!vict)
				script_log("team: invalid player '%s'", arg1);
//			else if (GET_TEAM(vict) != team)
//				script_log("team: removing '%s' from team %s", vict->GetName(), team_names[team]);
			else
			{
				mudlogf(CMP, LVL_STAFF, TRUE, "(GC) Script [%s] removing player '%s' from team '%s'", trig->GetVirtualID().Print().c_str(), vict->GetName(), team_names[team]);
				GET_TEAM(vict) = 0;
			}
		}
	}
	else
		script_log("team: invalid usage: %s", argument);
}


SCMD(func_oset)
{
	BUFFER(field, MAX_SCRIPT_STRING_LENGTH);
	
	ObjData *obj = NULL;
	Entity *e = NULL;

	int context;
	const char *value = ScriptEngineParseVariableWithEntity(argument, field, context, e);
	
	obj = dynamic_cast<ObjData *>(e);
	
	if (!*field || !*value)
		script_log("oset: invalid arguments: %s", argument);
	else if (!obj)
		script_log("oset: cannot find target: %s", argument);
	else
	{
		if (!str_cmp(field, "buyer"))
		{
			IDNum	buyer;
			
			if (is_number(value))			buyer = atoi(value);
			else if (IsIDString(value))		buyer = ParseIDString(value);
			else							buyer = get_id_by_name(value);
			
			if (buyer != 0 && !get_name_by_id(buyer))	script_log("oset: invalid buyer: %s", argument);
			else
			{
				obj->buyer = buyer;
				obj->bought = time(0);
			}	
		}
		else if (!str_cmp(field, "bought"))
		{
			if (!is_number(value))	script_log("oset: invalid bought: %s", argument);
			else					obj->bought = atoi(value);
		}
		else if (!str_cmp(field, "clan"))
		{
			if (!is_number(value))	script_log("oset: invalid clan: %s", argument);
			else					obj->m_ClanRestriction = atoi(value);
		}
		else if (!str_cmp(field, "timer"))
		{
			int duration = atoi(value);
			if (!is_number(value) || duration < 0)	script_log("oset: invalid timer: %s", argument);
			else									GET_OBJ_TIMER(obj) = duration;
		}
		else if (!str_cmp(field, "gun.ammoamount"))
		{
			int ammo = atoi(value);
			if (!is_number(value) || ammo < 0)		script_log("oset: invalid gun.ammoamount: %s", argument);
			else if (!IS_GUN(obj))					script_log("oset: attempt to set gun.ammoamount on non-gun: %s", argument);
			else									GET_GUN_AMMO(obj) = ammo;
		}
//		else if (!str_cmp(field, "gun.ammovnum"))
//		{
//			ObjData *ammo = obj_index.Find( VirtualID(value, thread->GetCurrentVirtualID().zone) );
//			if (!IsVirtualID(value))				script_log("oset: invalid gun.ammovnum: %s", argument);
//			else if (!IS_GUN(obj))					script_log("oset: attempt to set gun.ammovnum on non-gun: %s", argument);
//			else if (!ammo)							script_log("oset: attempt to set gun.ammovnum to nonexistent object: %s", argument);
//			else									GET_GUN_AMMO_VNUM(obj) = ammo->GetVirtualID();
//		}
		else
			script_log("oset: invalid field: %s", argument);
	}
}


SCMD(func_mset)
{
	BUFFER(field, MAX_SCRIPT_STRING_LENGTH);
	
	CharData *mob = NULL;
	Entity *e = NULL;

	int context;
	const char *value = ScriptEngineParseVariableWithEntity(argument, field, context, e);
	
	mob = dynamic_cast<CharData *>(e);
	
	if (!*field || !*value)
		script_log("mset: invalid arguments: %s", argument);
	else if (!mob)
		script_log("mset: cannot find target: %s", argument);
	else if (!IS_NPC(mob))
	{
		if (!str_cmp(field, "loadroom"))
		{
			RoomData *	loadroom = world.Find( VirtualID(value, thread->GetCurrentVirtualID().zone) );
			
			if (!IsVirtualID(value))	script_log("mset: invalid loadroom: %s", argument);
			else if (!loadroom)			script_log("mset: nonexistent loadroom: %s", argument);
			else						mob->GetPlayer()->m_LoadRoom = loadroom->GetVirtualID();
		}
		else
			script_log("mset: invalid player mset field: %s", argument);
	}
	else
	{
		if (!str_cmp(field, "agi"))
		{
			if (!is_number(value))	script_log("mset: invalid agi: %s", argument);
			else					mob->real_abils.Agility = RANGE(atoi(value), 0, MAX_STAT);
		}
		else if (!str_cmp(field, "clan"))
		{
			if (!is_number(value))	script_log("mset: invalid clan: %s", argument);
			else					mob->m_Clan = Clan::GetClan(atoi(value));
		}
		else if (!str_cmp(field, "coo"))
		{
			if (!is_number(value))	script_log("mset: invalid coo: %s", argument);
			else					mob->real_abils.Coordination = RANGE(atoi(value), 0, MAX_STAT);
		}
		else if (!str_cmp(field, "hea"))
		{
			if (!is_number(value))	script_log("mset: invalid hea: %s", argument);
			else					mob->real_abils.Health = RANGE(atoi(value), 0, MAX_STAT);
		}
		else if (!str_cmp(field, "hp"))
		{
			if (!is_number(value))	script_log("mset: invalid hp: %s", argument);
			else					GET_HIT(mob) = RANGE(atoi(value), 0, GET_MAX_HIT(mob));
		}
		else if (!str_cmp(field, "hitpercent"))
		{
			if (!is_number(value))	script_log("mset: invalid hitpercent: %s", argument);
			else					GET_HIT(mob) = RANGE(atoi(value), 0, 100) * 100 / GET_MAX_HIT(mob);
		}
		else if (!str_cmp(field, "kno"))
		{
			if (!is_number(value))	script_log("mset: invalid kno: %s", argument);
			else					mob->real_abils.Knowledge = RANGE(atoi(value), 0, MAX_STAT);
		}
		else if (!str_cmp(field, "maxhp"))
		{
			if (!is_number(value))	script_log("mset: invalid maxhp: %s", argument);
			else					GET_MAX_HIT(mob) = MAX(0, atoi(value));
		}
		else if (!str_cmp(field, "maxmv"))
		{
			if (!is_number(value))	script_log("mset: invalid maxmv: %s", argument);
			else					GET_MAX_MOVE(mob) = MAX(0, atoi(value));
		}
		else if (!str_cmp(field, "per"))
		{
			if (!is_number(value))	script_log("mset: invalid per: %s", argument);
			else					mob->real_abils.Perception = RANGE(atoi(value), 0, MAX_STAT);
		}
		else if (!str_cmp(field, "position"))
		{
			int position = GetEnumByName<Position>(value);
			if (position == -1)		script_log("mset: invalid position: %s", argument);
			else					GET_POS(mob) = (Position)position;
		}
		else if (!str_cmp(field, "race"))
		{
			int race = search_block(value, race_types, false);
			if (race == -1)			script_log("mset: invalid race: %s", argument);
			else					mob->m_Race = (Race)race;
		}
		else if (!str_cmp(field, "sex"))
		{
			int gender = GetEnumByNameAbbrev<Sex>(value);
			if (gender == -1)		script_log("mset: invalid sex: %s", argument);
			else					mob->m_Sex = (Sex)gender;
		}
		else if (!str_cmp(field, "str"))
		{
			if (!is_number(value))	script_log("mset: invalid str: %s", argument);
			else					mob->real_abils.Strength = RANGE(atoi(value), 0, MAX_STAT);
		}
		else if (!str_cmp(field, "team"))
		{
			int team = search_block(value, team_names, false);
			if (team == -1)			script_log("mset: invalid team: %s", argument);
			else					GET_TEAM(mob) = team;
		}
		else
			script_log("mset: invalid mob mset field: %s", argument);
	}
}



const struct ScriptCommand scriptCommands[] = {
	{ "RESERVED", 0, 0 },/* this must be first -- for specprocs */

	{ "affect"		, func_affect	, ScriptCommand::All, 0 },
	{ "affectmsg"	, func_affectmsg, ScriptCommand::All, 0 },
	{ "asound"		, func_asound	, ScriptCommand::All, 0 },
	{ "at"			, func_at		, ScriptCommand::All, 0 },
	{ "attach"		, func_attach	, ScriptCommand::All, 0 },
	{ "call"		, func_call		, ScriptCommand::All, 0 },
	{ "clanecho"	, func_worldecho, ScriptCommand::All, SCMD_CLANECHO },
	{ "copy"		, func_copy		, ScriptCommand::All, 0 },
	{ "damage"		, func_damage	, ScriptCommand::All, 0 },
//	{ "damageen"	, func_damageen	, ScriptCommand::All, 0 },
	{ "damagefull"	, func_damage	, ScriptCommand::All, SCMD_DAMAGEFULL },
	{ "damagemv"	, func_damagemv	, ScriptCommand::All, 0 },
	{ "deposit"		, func_deposit	, ScriptCommand::All, 0 },
//	{ "destroy"		, func_destroy	, ScriptCommand::All, SCMD_DESTROY },
	{ "detach"		, func_detach	, ScriptCommand::All, 0 },
	{ "do"			, func_do		, ScriptCommand::All, 0 },
	{ "door"		, func_door		, ScriptCommand::All, 0 },
	{ "echo"		, func_send		, ScriptCommand::All, SCMD_SEND_ECHO },
	{ "echoaround"	, func_send		, ScriptCommand::All, SCMD_SEND_ECHOAROUND },
	{ "echoaround2"	, func_send		, ScriptCommand::All, SCMD_SEND_ECHOAROUND2 },
	{ "force"		, func_force	, ScriptCommand::All, 0 },
	{ "goto"		, func_goto		, ScriptCommand::All, 0 },
	{ "heal"		, func_heal		, ScriptCommand::All, 0 },
//	{ "healen"		, func_healen	, ScriptCommand::All, 0 },
	{ "healmv"		, func_healmv	, ScriptCommand::All, 0 },
	{ "hunt"		, func_hunt		, ScriptCommand::All, 0 },
	{ "info"		, func_info		, ScriptCommand::All, 0 },
	{ "junk"		, func_junk		, ScriptCommand::Mob, 0 },
	{ "kill"		, func_kill		, ScriptCommand::Mob, 0 },
	{ "killthread"	, func_killthread, ScriptCommand::All, 0 },
	{ "lag"			, func_lag		, ScriptCommand::All, 0 },
	{ "load"		, func_load		, ScriptCommand::All, 0 },
	{ "log"			, func_log		, ScriptCommand::All, 0 },
	{ "mecho"		, func_worldecho, ScriptCommand::All, SCMD_MISSIONECHO },
	{ "move"		, func_move		, ScriptCommand::All, 0 },
	{ "mset"		, func_mset		, ScriptCommand::All, 0 },
	{ "mp"			, func_mp		, ScriptCommand::All, 0 },
	{ "oset"		, func_oset		, ScriptCommand::All, 0 },
	{ "purge"		, func_purge	, ScriptCommand::All, 0 },
	{ "raceecho"	, func_worldecho, ScriptCommand::All, SCMD_RACEECHO },
//	{ "repair"		, func_destroy	, ScriptCommand::All, SCMD_REPAIR },
	{ "remove"		, func_remove	, ScriptCommand::All, 0 },
	{ "restring"	, func_restring	, ScriptCommand::All, 0 },
	{ "send"		, func_send		, ScriptCommand::All, SCMD_SEND },
	{ "stitle"		, func_stitle	, ScriptCommand::All, 0 },
	{ "startcombat"	, func_startcombat, ScriptCommand::All, 0 },
	{ "stopcombat"	, func_stopcombat, ScriptCommand::All, 0 },
	{ "team"		, func_team		, ScriptCommand::All, 0 },
	{ "teamecho"	, func_worldecho, ScriptCommand::All, SCMD_TEAMECHO },
	{ "timer"		, func_timer	, ScriptCommand::All, 0 },
	{ "teleport"	, func_teleport	, ScriptCommand::All, 0 },
	{ "transform"	, func_transform, ScriptCommand::All, 0 },
	{ "unaffect"	, func_unaffect	, ScriptCommand::All, 0 },
	{ "withdraw"	, func_withdraw	, ScriptCommand::All, 0 },
	{ "zoneecho"	, func_zoneecho	, ScriptCommand::All, 0 },
	{ "zoneechorace", func_zoneechorace, ScriptCommand::All, 0 },
	{ "zreset"		, func_zreset	, ScriptCommand::All, 0 },

	{ "\n", 0, ScriptCommand::All, 0 }	/* this must be last */
};


/*
 *  This is the command interpreter used by rooms, called by script_driver.
 */
void script_command_interpreter(Entity * go, TrigData * trig, TrigThread *thread, const char *argument, int cmd) {
	const char *line;

	skip_spaces(argument);

	/* just drop to next line for hitting CR */
	if (!*argument)
		return;
	
	/* find the command */
	if (cmd == -1)
	{
		BUFFER(arg, MAX_SCRIPT_STRING_LENGTH);

		line = ScriptEngineGetArgument(argument, arg);
		
		for (cmd = 0; *scriptCommands[cmd].command != '\n'; cmd++)
			if (!strcmp(scriptCommands[cmd].command, arg))
				break;

		if (*scriptCommands[cmd].command == '\n') {
			if (go->GetEntityType() == Entity::TypeCharacter && !IS_STAFF((CharData *)go)) {
				command_interpreter((CharData *)go, argument);
			} else {
				script_log("Unknown script cmd: '%s'", argument);
			}
			
			return;
		}
	}
	else
	{
		line = argument;
//		line += strlen(scriptCommands[cmd].command);
//		skip_spaces(line);
	}
	
	if (cmd != -1)
		((*scriptCommands[cmd].command_pointer) (go, trig, thread, line, scriptCommands[cmd].subcmd));
}
