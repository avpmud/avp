/*************************************************************************
*   File: act.informative.c++        Part of Aliens vs Predator: The MUD *
*  Usage: Player-level information commands                              *
*************************************************************************/


#include "characters.h"
#include "objects.h"
#include "zones.h"
#include "rooms.h"
#include "descriptors.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "db.h"
#include "clans.h"
#include "boards.h"
#include "help.h"
#include "extradesc.h"
#include "spells.h"
//#include "space.h"
#include "constants.h"
#include "weather.h"


/* extern variables */
extern struct TimeInfoData time_info;


/* extern functions */
struct TimeInfoData real_time_passed(time_t t2, time_t t1);
void master_parser (char *buf, CharData *ch, RoomData *room, ObjData *obj);
int wear_message_number (ObjData *obj, WearPosition where);
ACMD(do_action);

/* prototypes */
void diag_char_to_char(CharData * i, CharData * ch);
void look_at_char(CharData * i, CharData * ch);
void list_one_char(CharData * i, CharData * ch);
void list_char_to_char(CharacterList &list, CharData * ch);
//void list_ship_to_char(ShipList &list, CharData *ch);
void do_auto_exits(CharData * ch, RoomData *room, bool bFullExits = false);
void look_in_direction(CharData * ch, int dir, int maxDist);
void look_in_obj(CharData * ch, char *arg);
void look_at_target(CharData * ch, char *arg, int read);
void perform_mortal_where(CharData * ch, char *arg);
void perform_immort_where(CharData * ch, char *arg);
void sort_commands(void);
void show_obj_to_char(ObjData * object, CharData * ch, int mode);
void list_obj_to_char(ObjectList &list, CharData * ch, int mode,  int show);
void print_object_location(int num, ObjData * obj, CharData * ch, int recur);
void list_scanned_chars(CharacterList &list, ObjectList &olist, CharData * ch, int distance, int door);
void look_out(CharData * ch);
//void look_out_ship(CharData *ch, ShipData *ship);
ACMD(do_exits);
ACMD(do_look);
ACMD(do_examine);
ACMD(do_gold);
ACMD(do_score);
ACMD(do_inventory);
ACMD(do_equipment);
ACMD(do_time);
ACMD(do_weather);
ACMD(do_help);
ACMD(do_who);
ACMD(do_users);
ACMD(do_gen_ps);
ACMD(do_where);
ACMD(do_levels);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_toggle);
ACMD(do_commands);
ACMD(do_capturestatus);
ACMD(do_scan);


/*
mode 0 - room
mode 1 - inventory/equip list
mode 2 - object contents list
mode 3 - appears unused
mode 4 - appears unused
mode 5 - look at
mode 6 - extraDesc w/ weapon info
*/
void show_obj_to_char(ObjData * object, CharData * ch, int mode) {
	BUFFER(buf, MAX_STRING_LENGTH * 2);
	int found;

	*buf = '\0';
	strcpy(buf, "`g");
	if ((mode == 0) && !object->m_RoomDescription.empty()) {
		if ((ch->sitting_on == object) && (GET_POS(ch) <= POS_SITTING))
			sprintf_cat(buf, "You are %s on %s.",
					(GET_POS(ch) == POS_RESTING) ? "resting" : "sitting",
					object->GetName());
		else
			strcat(buf, object->GetRoomDescription());
	} else if (!object->m_Name.empty() && ((mode == 1) ||
			(mode == 2) || (mode == 3) || (mode == 4)))
		strcat(buf, object->GetName());
	else if (mode == 5) {
/*		if (GET_OBJ_TYPE(object) == ITEM_NOTE) {
			if (!object->m_Description.empty()) {
				strcpy(buf, "There is something written upon it:\n\n");
				strcat(buf, object->GetDescription());
				page_string(ch->desc, buf, 1);
			} else
				act("It's blank.", FALSE, ch, 0, 0, TO_CHAR);
			return;
		} else 
*/		if (GET_OBJ_TYPE(object) == ITEM_BOARD) {
			if (ch->CanUse(object) != NotRestricted)
				ch->Send("You don't have the access privileges for that board!\n");
			else
				Board::Show(object, ch);
			return;
		} else if (GET_OBJ_TYPE(object) == ITEM_DRINKCON)
			strcpy(buf, "It looks like a drink container.");
		else if (!object->m_Description.empty())
			strcpy(buf, object->GetDescription());
		else if (GET_OBJ_TYPE(object) == ITEM_NOTE)
			strcpy(buf, "It's blank.");
		else
			strcpy(buf, "You see nothing special...");
	}
	if (mode != 3) {
		found = FALSE;
		if (OBJ_FLAGGED(object, ITEM_INVISIBLE)) {
			strcat(buf, " (invisible)");
			found = TRUE;
		}
		if (OBJ_FLAGGED(object, ITEM_GLOW)) {
			strcat(buf, " (glowin)");
			found = TRUE;
		}
		if (OBJ_FLAGGED(object, ITEM_HUM)) {
			strcat(buf, " (humming)");
			found = TRUE;
		}
		if (OBJ_FLAGGED(object, ITEM_STAFFONLY)) {
			strcat(buf, " (STAFF)");
			found = TRUE;
		}
	}
	strcat(buf, "`n\n");
	
	if (mode == 5 || mode == 6)
	{
		if (GET_OBJ_TYPE(object) == ITEM_WEAPON || GET_OBJ_TYPE(object) == ITEM_THROW || GET_OBJ_TYPE(object) == ITEM_BOOMERANG)
		{
			int skill1 = 0, skill2 = 0;
			
			if (IS_GUN(object))
			{
				skill1 = GET_GUN_SKILL(object);
				skill2 = GET_GUN_SKILL2(object);
			}
			else if (GET_OBJ_TYPE(object) == ITEM_WEAPON)
			{
				skill1 = object->GetValue(OBJVAL_WEAPON_SKILL);
				skill2 = object->GetValue(OBJVAL_WEAPON_SKILL2);
			}
			else if (GET_OBJ_TYPE(object) == ITEM_THROW || GET_OBJ_TYPE(object) == ITEM_BOOMERANG)
			{
				skill1 = object->GetValue(OBJVAL_THROWABLE_SKILL);
			}
			
			sprintf_cat(buf, "`nAs a weapon, the `(%s`) uses the skill%s `y%s`n",
				object->GetName(),
				skill2 ? "s" : "",
				skill_name(skill1));
				
			if (skill2)
			{
				sprintf_cat(buf, " and `y%s`n", skill_name(skill2));
			}
			
			strcat(buf, ".\n");
			
			if (IS_GUN(object))
			{
				skill1 = object->GetValue(OBJVAL_WEAPON_SKILL);
				skill2 = object->GetValue(OBJVAL_WEAPON_SKILL2);
				
				sprintf_cat(buf, "If you run out of ammo, the `(%s`) uses the skill%s `y%s`n",
					object->GetName(),
					skill2 ? "s" : "",
					skill_name(skill1));
				
				if (skill2)
				{
					sprintf_cat(buf, " and `y%s`n", skill_name(skill2));
				}
				
				sprintf_cat(buf, " in close combat.\n");
			}
		}
		
		page_string(ch->desc, buf);
	}
	else
	{
		ch->Send("%s", buf);
	}
}


void list_obj_to_char(ObjectList &list, CharData * ch, int mode, int show) {
	bool	found = false;
	
	FOREACH(ObjectList, list, iter)
	{
		ObjData *	i = *iter;
		
		bool	bCanStack = !IS_SITTABLE(i)
//				&& (GET_OBJ_TYPE(i) != ITEM_CONTAINER)
				&& (GET_OBJ_TYPE(i) != ITEM_VEHICLE)
				&& !OBJ_FLAGGED(i, ITEM_NOSTACK)
				&& (mode == 0 || i->GetPrototype());
		
		int num = 1;
		
		if (bCanStack)
		{
			//	See if we already have already come across one of these in the list
			ObjectList::const_iterator findIter;
			for (findIter = list.begin(); findIter != iter; ++findIter)
			{
				ObjData *j = *findIter;
				
				if (j == i)	break;
				if (j->GetPrototype() == i->GetPrototype()
					&& j->m_RoomDescription == i->m_RoomDescription
					&& j->m_Name == i->m_Name)
					break;
			}
			
			//	If we have a previous one in this list other than us, then skip us
			if (findIter != iter)
				continue;

			//	Count the rest; we know this one is already counted
			findIter = iter;
			for (++findIter; findIter != list.end(); ++findIter)
			{
				ObjData *j = *findIter;
				if (j->GetPrototype() == i->GetPrototype()
					&& j->m_RoomDescription == i->m_RoomDescription
					&& j->m_Name == i->m_Name)
					++num;
			}
		}
		
		if (ch->CanSee(i))
		{
			if (bCanStack && num > 1)
				ch->Send("(%2.2i) ", num);
			if (mode != 0)
			{
				show_obj_to_char(i, ch, mode);
				found = true;
			}
			else if (IS_SITTABLE(i))
			{
				if (i->GetValue(OBJVAL_FURNITURE_CAPACITY) == 0
					|| ch->sitting_on == i
					|| get_num_chars_on_obj(i) < i->GetValue(OBJVAL_FURNITURE_CAPACITY))
				{
					show_obj_to_char(i, ch, mode);
					found = true;
				}
			}
			else if (IS_MOUNTABLE(i))
			{
				if ((ch->sitting_on == i) || !get_num_chars_on_obj(i)) {
					show_obj_to_char(i, ch, mode);
					found = true;
				}
			}
			else
			{
				show_obj_to_char(i, ch, mode);
				found = true;
			}
		}
	}
	
	if (!found && show)
		send_to_char("`g Nothing.\n", ch);
}


void diag_char_to_char(CharData * i, CharData * ch) {
	int percent;
	char *buf;

	if (GET_MAX_HIT(i) > 0)		percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
	else						percent = -1;		//	How could MAX_HIT be < 1??
	
	if (MOB_FLAGGED(i, MOB_MECHANICAL))
	{
		if (percent >= 100)			buf = "$N is in excellent condition.\n";
		else if (percent >= 90)		buf = "$N has a little bit of damage.\n";
		else if (percent >= 75)		buf = "$N has some small holes and tears.\n";
		else if (percent >= 50)		buf = "$N has quite a bit of damage.\n";
		else if (percent >= 30)		buf = "$N has some big nasty holes and tears.\n";
		else if (percent >= 15)		buf = "$N looks pretty beaten up.\n";
		else if (percent >= 0)		buf = "$N is in awful condition.\n";
		else						buf = "$N is broken and short circuiting.\n";
	}
	else
	{
		if (percent >= 100)			buf = "$N is in excellent condition.\n";
		else if (percent >= 90)		buf = "$N has a few scratches.\n";
		else if (percent >= 75)		buf = "$N has some small wounds and bruises.\n";
		else if (percent >= 50)		buf = "$N has quite a few wounds.\n";
		else if (percent >= 30)		buf = "$N has some big nasty wounds and scratches.\n";
		else if (percent >= 15)		buf = "$N looks pretty hurt.\n";
		else if (percent >= 0)		buf = "$N is in awful condition.\n";
		else						buf = "$N is bleeding awfully from big wounds.\n";
	}

	act(buf, TRUE, ch, 0, i, TO_CHAR);
}


void look_at_char(CharData * i, CharData * ch) {
	if (!ch->desc)
		return;
		
	if (!i->m_Description.empty())
		send_to_char(i->GetDescription(), ch);
	else if (IS_NPC(ch))
		act("You see nothing special about $M.", FALSE, ch, 0, i, TO_CHAR);
	else
		act("$N hasn't described $Mself yet.", FALSE, ch, 0, i, TO_CHAR);

	diag_char_to_char(i, ch);

	bool found = false;
	for (int j = 0; !found && j < NUM_WEARS; j++)
		if (GET_EQ(i, j) && ch->CanSee(GET_EQ(i, j)))
			found = true;

	if (found)
	{
		act("\n$n is using:", FALSE, i, 0, ch, TO_VICT);
		for (int j = 0; j < NUM_WEARS; ++j)
		{
			if (GET_EQ(i, j) && ch->CanSee(GET_EQ(i, j))) {
				send_to_char(where[wear_message_number(GET_EQ(i, j), (WearPosition)j)], ch);
				show_obj_to_char(GET_EQ(i, j), ch, 1);
			}
		}
	}
	if (NO_STAFF_HASSLE(ch))
	{
		found = false;
		act("\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
		FOREACH(ObjectList, i->carrying, iter)
		{
			ObjData *tmp_obj = *iter;
			if (ch->CanSee(tmp_obj) && (Number(0, 20) < ch->GetLevel())) {
				show_obj_to_char(tmp_obj, ch, 1);
				found = true;
			}
		}

		if (!found)
			send_to_char("You can't see anything.\n", ch);
	}
}


void list_one_char(CharData * i, CharData * ch) {
	BUFFER(buf, MAX_STRING_LENGTH);
	const char *positions[] = {
		" is lying here, dead.",
		" is lying here, mortally wounded.",
		" is lying here, incapacitated.",
		" is lying here, stunned.",
		" is sleeping here.",
		" is resting here.",
		" is sitting here.",
		"!FIGHTING!",
		" is standing here."
	};
	const char *sit_positions[] = {
		"!DEAD!",
		"!MORTALLY WOUNDED!",
		"!INCAPACITATED!",
		"!STUNNED!",
		"sleeping",
		"resting",
		"sitting",
		"!FIGHTING!",
		"!STANDING!"
	};
	
	char relation = relation_colors[i->GetRelation(ch)];
	
/*	strcpy(buf, '\0');*/
	*buf = '\0';
	
	if (IS_NPC(i) && !i->m_RoomDescription.empty() && GET_POS(i) == GET_DEFAULT_POS(i)) {
/*		if (AFF_FLAGGED(i, AFF_INVISIBLE_FLAGS))
			strcpy(buf, "*");
		else
			*buf = '\0';
*/		
		ch->Send("%s`%c%s`y", AFF_FLAGS(i).testForAny( Affect::AFF_INVISIBLE_FLAGS ) ? "`y*" : "",
				relation, i->GetRoomDescription());

//		if (AFF_FLAGGED(i, AFF_SANCTUARY))
//			act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
		if (AFF_FLAGGED(i, AFF_BLIND))
			act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);

		return;
	}
  
	if (IS_NPC(i)) {
		*buf = 0;
		if (GET_TEAM(i))
			sprintf(buf, "`y[`(%s`)]`n ", team_titles[GET_TEAM(i)]);
		sprintf_cat(buf, "`%c%s`y", relation, PERS(i, ch));
		CAP(buf);
	} else {
		*buf = 0;
		if (GET_TEAM(i))
			sprintf(buf, "`y[%s`y]`n ", team_titles[GET_TEAM(i)]);
		if (ch->CanSee(i) == VISIBLE_SHIMMER)	sprintf_cat(buf, "`%c%s`y", relation, PERS(i, ch));
		else
		{
			if (IS_STAFF(i))					sprintf_cat(buf, "`%c%s `y%s`y", relation, i->GetName(), i->GetTitle());
			else								sprintf_cat(buf, "`y%s`n `%c%s`y", i->GetTitle(), relation, PERS(i, ch));
		
			if (GET_CLAN(i) && Clan::GetMember(i)) {
				sprintf_cat(buf, " `n<`C%s`n>", GET_CLAN(i)->GetTag());
			}
		}
	}
  
	if (!IS_NPC(i) && !PRF_FLAGGED(i, PRF_PK))		strcat(buf, " (`cno-pk`y)");
	
	if (AFF_FLAGS(i).testForAny(Affect::AFF_INVISIBLE_FLAGS) && ch->CanSee(i) == VISIBLE_FULL)	strcat(buf, " (invisible)");
	if (AFF_FLAGGED(i, AFF_HIDE))				strcat(buf, " (hiding)");
	if (AFF_FLAGGED(i, AFF_COVER))				strcat(buf, " (cover)");
	if (!IS_NPC(i) && !i->desc)					strcat(buf, " (linkless)");
	if (PLR_FLAGGED(i, PLR_WRITING))			strcat(buf, " (writing)");
    if (!i->GetPlayer()->m_AFKMessage.empty())	strcat(buf, " `c[AFK]`y");
	if (MOB_FLAGGED(i, MOB_PROGMOB))			strcat(buf, " `r(PROG-MOB)`y");
	if (AFF_FLAGGED(i, AFF_SANCTUARY))			strcat(buf, " `W(glowing)`y");

	if (i->sitting_on) {
		if ((GET_POS(i) < POS_SLEEPING) || (GET_POS(i) > POS_SITTING) ||
				(IN_ROOM(i->sitting_on) != IN_ROOM(i))) {
			strcat(buf, positions[(int) GET_POS(i)]);
			i->sitting_on = NULL;
		} else
			sprintf_cat(buf, "`y is %s on `g%s`y.", sit_positions[(int)GET_POS(i)],
					i->sitting_on->GetName());
	}
	else if (GET_POS(i) == POS_FIGHTING)
	{
		if (FIGHTING(i)) {
			sprintf_cat(buf, " is here, `Rfighting %s`y!",
				FIGHTING(i) == ch ? "YOU`y!" : (IN_ROOM(i) == IN_ROOM(FIGHTING(i)) ?
						PERS(FIGHTING(i), ch) : "someone who has already left"));
		} else			/* NIL fighting pointer */
			strcat(buf, " is here struggling with thin air.");
	}
	else if (AFF_FLAGGED(i, AFF_TRAPPED))
		strcat(buf, " is trapped.");
	else
		strcat(buf, positions[(int) GET_POS(i)]);

	strcat(buf, "`n\n");
	send_to_char(buf, ch);

//	if (AFF_FLAGGED(i, AFF_SANCTUARY))
//		act("...$e glows with a `Wbright light`n`y!", FALSE, i, 0, ch, TO_VICT);
	if (AFF_FLAGGED(i, AFF_BLIND))
		act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);

}


/*
bool is_online(char *argument)
{
    CharData *wch;

    if (!*argument)
	return FALSE;

    if (!str_cmp(argument, "Public"))
	return TRUE;
	
	if (is_number(argument))
		return (Clan::GetClan(atoi(argument)) != NULL);

    FOREACH(DescriptorList, descriptor_list, iter)
    {
    	DescriptorData *d = *iter;
		if (STATE(d) != CON_PLAYING)	continue;
		if (!(wch = d->Original()))		continue;

		if (!str_cmp(argument, wch->GetName()))
    		return TRUE;
    }

    return FALSE;
}
*/

/*
void list_ship_to_char(ShipList &list, CharData *ch) {
	FOREACH(ShipList, list, iter)
	{
		ShipData *ship = *iter;
		if (ship->owner && *ship->owner && ch->CanSee(ship)
			&& is_online(ship->owner) || is_online(ship->pilot) || is_online(ship->copilot))
		{
			ch->Send("`C%s`n", ship->name);

			if (ship->IsCloaked())
				ch->Send(" (`wcloaked`n)");

			ch->Send("\n");
		}
	}
}
*/

void list_char_to_char(CharacterList &list, CharData * ch)
{
	FOREACH(CharacterList, list, i)
	{
		if (ch != *i && ch->CanSee(*i) != VISIBLE_NONE)
			list_one_char(*i, ch);
	}
}


void do_auto_exits(CharData * ch, RoomData *room, bool bFullExits)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	BUFFER(buf2, MAX_STRING_LENGTH);
	

	if (!room)	return;
	
	
	if (bFullExits && AFF_FLAGGED(ch, AFF_BLIND)) {
		send_to_char("You can't see a damned thing, you're blind!\n", ch);
		return;
	}
	
	
	for (int door = 0; door < NUM_OF_DIRS; door++)
	{
		RoomExit *	exit = Exit::GetIfValid(room, door);
		
		if (!exit)
			continue;
		
		bool bHiddenExit	= exit->GetStates().test(EX_STATE_HIDDEN)
							|| (exit->GetFlags().test(EX_HIDDEN) 
								&& (!exit->GetFlags().test(EX_ISDOOR) || Exit::IsDoorClosed(exit)));

		//	Hidden exit behavior: not visible if its a door and closed, or not a door
		if (bHiddenExit && !IS_STAFF(ch))
			continue;
		
		bool bNoMovement 	= !Exit::IsPassable(exit);
		bool bBreached		= exit->GetStates().test(EX_STATE_BREACHED);
		bool bClosed		= Exit::IsDoorClosed(exit);
		
		if ((EXIT_FLAGGED(exit, EX_NOHUMAN) && IS_HUMAN(ch)) ||
			(EXIT_FLAGGED(exit, EX_NOSYNTHETIC) && IS_SYNTHETIC(ch)) ||
			(EXIT_FLAGGED(exit, EX_NOPREDATOR) && IS_PREDATOR(ch)) ||
			(EXIT_FLAGGED(exit, EX_NOALIEN) && IS_ALIEN(ch)))
			bNoMovement = true;
			
		const char *color = "`c";
		if (bHiddenExit)		color = "`g";
		else if (!bClosed && bNoMovement)	color = "`b";
		else if (bBreached)		color = "`R";

		if (!bFullExits)	sprintf_cat(buf, "%s%c ", color, bClosed ? tolower(*dirs[door]) : toupper(*dirs[door]));
		else
		{
			sprintf(buf2, "%-5s - ", dirs[door]);
			
			if (IS_STAFF(ch))	sprintf_cat(buf2, "[%10s] ", exit->ToRoom()->GetVirtualID().Print().c_str());
			
			if ( !ch->LightOk(exit->ToRoom()) )
				strcat(buf2, "Too dark to tell\n");
			else
			{
				const char *state = "";
				
				if (bClosed)		state = " (Closed)";
				else if (bBreached)	state = " `(`R(Breached)`)";
				
				sprintf_cat(buf2, "`(%s`)%s\n", exit->ToRoom()->GetName(), state);
			}

			strcat(buf, CAP(buf2));
		}
	}

	if (!bFullExits)	ch->Send("`c[ Exits: `(%s`)]`n\n", *buf ? buf : "None! ");
	else
	{
		ch->Send("Obvious exits:\n%s", *buf ? buf : " None.\n");
	}
}


ACMD(do_exits)
{
	do_auto_exits(ch, ch->InRoom(), true);
}


void look_at_room(CharData * ch, RoomData * room, bool ignore_brief) {
	if (!ch->desc || !room)
		return;
	if (!ch->LightOk(room)) {
		send_to_char("It is pitch black...\n", ch);
		return;
	} else if (AFF_FLAGGED(ch, AFF_BLIND)) {
		send_to_char("You see nothing but infinite darkness...\n", ch);
		return;
	}
	
	ch->Send("`c");
	if (ROOM_FLAGGED(room, ROOM_HIVED))
	{
		ch->Send("`R[HIVED]`n`c ");
	}
	if (PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
		BUFFER(buf, MAX_STRING_LENGTH);
		BUFFER(bitbuf, MAX_STRING_LENGTH);
		strcpy(buf, room->GetName());
		if (ROOM_FLAGGED(room, ROOM_PARSE))
			master_parser(buf, ch, room, NULL);
		ch->Send("[%s] %s [ %s ] [%s]", room->GetVirtualID().Print().c_str(), buf, room->GetFlags().print().c_str(), GetEnumName(room->GetSector()));
	} else if (ROOM_FLAGGED(room, ROOM_PARSE)) {
		BUFFER(buf, MAX_STRING_LENGTH);
		strcpy(buf, room->GetName());
		master_parser(buf, ch, room, NULL);
		send_to_char(buf, ch);
	} else
		send_to_char(room->GetName(), ch);
	
	send_to_char("`n\n", ch);

	if (!PRF_FLAGGED(ch, PRF_BRIEF) || ignore_brief || ROOM_FLAGGED(room, ROOM_DEATH)) {
		if (ROOM_FLAGGED(room, ROOM_PARSE)) {
			BUFFER(buf, MAX_STRING_LENGTH);
			strcpy(buf, room->GetDescription());
			master_parser(buf, ch, room, NULL);
			ch->Send("%s\n", buf);
		} else
			send_to_char(room->GetDescription(), ch);
	}
	
	/* autoexits */
//	if (PRF_FLAGGED(ch, PRF_AUTOEXIT))
		do_auto_exits(ch, room);

	/* now list characters & objects */
	list_obj_to_char(room->contents, ch, 0, FALSE);
	list_char_to_char(room->people, ch);
//	list_ship_to_char(room->ships, ch);
	send_to_char("`n", ch);
	
}


void look_at_room(CharData * ch, bool ignore_brief) {
	if (!ch->InRoom())	{ ch->ToRoom(world[0]); }
	look_at_room(ch, ch->InRoom(), ignore_brief);
}


void list_scanned_chars(CharacterList &list, ObjectList &olist, CharData * ch, int distance, int door) {
	const char *	how_far[] = {
		"close by",
		"nearby",
		"to the",
		"far off",
		"far in the distance"
	};
	
	typedef std::list< Entity * > VisibleEntityList;
	VisibleEntityList	visibleEntities;
	
	FOREACH(CharacterList, list, iter)
	{
		if (ch->CanSee(*iter) == VISIBLE_NONE)
			continue;
		
		visibleEntities.push_back(*iter);
	}
	
	
	FOREACH(ObjectList, olist, iter)
	{
		if (!OBJ_FLAGGED(*iter, ITEM_VIEWABLE))
			continue;
		
		visibleEntities.push_back(*iter);
	}
	
	if (visibleEntities.empty())
		return;
	
	BUFFER(buf, MAX_STRING_LENGTH);
	
	strcpy(buf, "You see ");
	
	int count = 1;
	int totalCount = visibleEntities.size();
	FOREACH(VisibleEntityList, visibleEntities, iter)
	{
		sprintf_cat(buf, "$%s%s",
			(*iter)->GetIDString(),
			(*iter)->GetEntityType() == Entity::TypeCharacter ? "N" : "P");

		++count;
		if (count == totalCount)		strcat(buf, " and ");
		else if (count < totalCount)	strcat(buf, ", ");
	}
	
	sprintf_cat(buf, " %s %s.", how_far[distance], dirs[door]);
	
	act(buf, true, ch, 0, 0, TO_CHAR);
}



void look_in_direction(CharData * ch, int dir, int maxDist) {
	int		distance;
	
	ch->Send("[%s] ", dirs[dir]);
	
	RoomExit *exit = Exit::GetIfValid(ch, dir);
	if (exit)
	{
		ch->Send("%s", *exit->GetDescription() ? exit->GetDescription() : "You see nothing special.\n");
		
		if (*exit->GetKeyword())
		{
			if (Exit::IsDoorClosed(exit))
				ch->Send("The %s is closed.\n", fname(exit->GetKeyword()));
			else if (exit->GetStates().test(EX_STATE_BLOCKED))
				ch->Send("The %s is blocked.\n", fname(exit->GetKeyword()));
			else if (EXIT_FLAGGED(exit, EX_ISDOOR))
				ch->Send("The %s is open.\n", fname(exit->GetKeyword()));
		}
		
		RoomData *room = Exit::IsViewable(exit) ? exit->ToRoom() : NULL;

		for (distance = 0; room && (distance < maxDist); distance++)
		{
			list_scanned_chars(room->people, room->contents, ch, distance, dir);

			exit = Exit::GetIfValid(room, dir);
			room = Exit::IsViewable(exit) ? exit->ToRoom() : NULL;
		}
	}
	else
		ch->Send("Nothing special there...\n");
}



void look_in_obj(CharData * ch, char *arg) {
	ObjData *obj = NULL;
	int amt, bits;

	if (!*arg)
		send_to_char("Look in what?\n", ch);
	else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, NULL, &obj)))
		ch->Send("There doesn't seem to be %s %s here.\n", AN(arg), arg);
	else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
			(GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
			(GET_OBJ_TYPE(obj) != ITEM_CONTAINER)) {
		if (IS_GUN(obj) && GET_GUN_AMMO_TYPE(obj) != -1)
			ch->Send("It contains %d units of ammunition.\n", GET_GUN_AMMO(obj));
		else if (GET_OBJ_TYPE(obj) == ITEM_AMMO)
			ch->Send("It contains %d units of ammunition.\n", obj->GetValue(OBJVAL_AMMO_AMOUNT));
		else
			ch->Send("There's nothing inside that!\n");
	} else {
		if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
		{
			if (IS_SET(obj->GetValue(OBJVAL_CONTAINER_FLAGS), CONT_CLOSED))
				send_to_char("It is closed.\n", ch);
			else {
				send_to_char(fname(obj->GetAlias()), ch);
				switch (bits) {
					case FIND_OBJ_INV:		send_to_char(" (carried): \n", ch);	break;
					case FIND_OBJ_ROOM:		send_to_char(" (here): \n", ch);		break;
					case FIND_OBJ_EQUIP:	send_to_char(" (used): \n", ch);		break;
				}

				list_obj_to_char(obj->contents, ch, 2, TRUE);
			}
		} else if (obj->GetValue(OBJVAL_FOODDRINK_CONTENT) <= 0)		/* item must be a fountain or drink container */
			send_to_char("It is empty.\n", ch);
		else if (obj->GetValue(OBJVAL_FOODDRINK_FILL) <= 0
			|| obj->GetValue(OBJVAL_FOODDRINK_CONTENT) > obj->GetValue(OBJVAL_FOODDRINK_FILL))
			ch->Send("Its contents seem somewhat murky.\n"); /* BUG */
		else {
			amt = (obj->GetValue(OBJVAL_FOODDRINK_CONTENT) * 3) / obj->GetValue(OBJVAL_FOODDRINK_FILL);
			ch->Send("It's %sfull of a %s liquid.\n", fullness[amt],
				findtype(obj->GetValue(OBJVAL_FOODDRINK_TYPE), color_liquid));
		}
	}
}



/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 */
void look_at_target(CharData * ch, char *arg, int read) {
	int bits, msg = 1;
	CharData *found_char = NULL;
	ObjData *obj = NULL, *found_obj = NULL;
	const char *	desc;
	bool	found = false;

	if (!ch->desc)
		return;
	else if (!*arg)
		send_to_char("Look at what?\n", ch);
	else if (read)
	{
		if (!(obj = get_obj_in_list_type(ITEM_BOARD, ch->carrying)))
			obj = get_obj_in_list_type(ITEM_BOARD, ch->InRoom()->contents);
			
		if (obj) {
			BUFFER(number, MAX_INPUT_LENGTH);
			
			one_argument(arg, number);
			if (!*number)
				send_to_char("Read what?\n",ch);
			else if (isname(number, obj->GetAlias())) {
				if (ch->CanUse(obj) != NotRestricted)
					ch->Send("You don't have the access privileges for that board!\n");
				else
					Board::Show(obj, ch);
			} else if (!isdigit(*number) || !(msg = atoi(number)) || strchr(number,'.'))
				look_at_target(ch, arg, 0);
			else {
				if (ch->CanUse(obj) != NotRestricted)
					ch->Send("You don't have the access privileges for that board!\n");
				else
					Board::DisplayMessage(obj, ch, msg);
			}
		} else
			look_at_target(ch, arg, 0);
	} else {
		bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
				FIND_CHAR_ROOM, ch, &found_char, &found_obj);

		/* Is the target a character? */
		if (found_char) {
			look_at_char(found_char, ch);
			if (ch != found_char) {
				if (found_char->CanSee(ch) != VISIBLE_NONE)
					act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
				act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
			}
			return;
		}
		/* Does the argument match an extra desc in the room? */
		if ((desc = ch->InRoom()->m_ExtraDescriptions.Find(arg)) != NULL) {
			page_string(ch->desc, desc);
			return;
		}
		/* Does the argument match an extra desc in the char's equipment? */
		for (int j = 0; j < NUM_WEARS && !found; j++) {
			if (GET_EQ(ch, j) && ch->CanSee(GET_EQ(ch, j))) {
				if ((desc = GET_EQ(ch, j)->m_ExtraDescriptions.Find(arg)) != NULL) {
					page_string(ch->desc, desc);
					return;
//					found = true;
				}
			}
		}
		
		/* Does the argument match an extra desc in the char's inventory? */
		FOREACH(ObjectList, ch->carrying, iter)
		{
			ObjData *obj = *iter;
			
			if (found)
				break;
			if (ch->CanSee(obj) && (desc = obj->m_ExtraDescriptions.Find(arg)))
			{
//				if(GET_OBJ_TYPE(obj) == ITEM_BOARD)
//				{
//					if (ch->CanUse(obj) != NotRestricted)
//						ch->Send("You don't have the access privileges for that board!\n");
//					else
//						Board::ShowBoard(obj->GetVirtualID(), ch);
//					return;
//				}
//				else
				{
					page_string(ch->desc, desc);
					return;
				}
				found = true;
			}
		}

		/* Does the argument match an extra desc of an object in the room? */
		FOREACH(ObjectList, ch->InRoom()->contents, iter)
		{
			ObjData *obj = *iter;
			
			if (found)
				break;
			if (ch->CanSee(obj) && (desc = obj->m_ExtraDescriptions.Find(arg))) {
//				if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
//					if (ch->CanUse(obj) != NotRestricted)
//						ch->Send("You don't have the access privileges for that board!\n");
//					else
//						Board::ShowBoard(obj->GetVirtualID(), ch);
//					return;
//				}
//				else
				{
					page_string(ch->desc, desc);
					return;
				}
				found = true;
			}
		}
		
		if (bits) {			/* If an object was found back in generic_find */
			if (!found)	show_obj_to_char(found_obj, ch, 5);	/* Show no-description */
			else		show_obj_to_char(found_obj, ch, 6);	/* Find hum, glow etc */
		} else if (!found)
			send_to_char("You do not see that here.\n", ch);
	}
}


/*
void look_out_ship(CharData *ch, ShipData *ship) {
//  ch->Send("\nThrough the transparisteel windows:\n");
	ch->Send("\nThrough the viewscreen you see:\n");

	if (ship->starsystem) {
		//	if (*ship->starsystem->star1)
		//	    ch->Send("`YThe star, %s.`n\n", ship->starsystem->star1);
		//	if (*ship->starsystem->star2)
		//	    ch->Send("`YThe star, %s.`n\n", ship->starsystem->star2);
		FOREACH(StellarObjectList, ship->starsystem->stellarObjects, stellarObject)
		{
			ch->Send("`GThe %s, %s.`n\n", StellarObjectTypes[(*stellarObject)->type], (*stellarObject)->name);
		}
		FOREACH(ShipList, ship->starsystem->ships, target)
		{
			if (*target != ship && ch->CanSee(*target))
				ch->Send("`C%s`n\n", (*target)->name);
		}
		FOREACH(Lexi::List<MissileData *>, ship->starsystem->missiles, missile)
		{
		    ch->Send("`R%s`n\n", (*missile)->name);
		}
	}
	else if (ship->location == ship->lastdoc)
	{
		look_at_room(ch, ship->InRoom(), FALSE);
	}
	else
	{
		ch->Send("All you see is an infinite blackness.\n");
	}
}
*/

void look_out(CharData * ch) {
  //  ShipData *ship;
	ObjData * viewport, * vehicle;

//    if ((ship = ship_from_cockpit(ch->InRoom())) != NULL) {
//	look_out_ship(ch, ship);
//	return;
//    }

	viewport = get_obj_in_list_type(ITEM_VEHICLE_WINDOW, ch->InRoom()->contents);
	if (!viewport)
		viewport = get_obj_in_list_type(ITEM_VEHICLE_WEAPON, ch->InRoom()->contents);

	if (!viewport) {
		send_to_char("There is no window to see out of.\n", ch);
		return;
	}
	vehicle = find_vehicle_by_vid(viewport->GetVIDValue(OBJVIDVAL_VEHICLEITEM_VEHICLE));
	if (!vehicle || vehicle->InRoom() == NULL) {
		send_to_char("All you see is an infinite blackness.\n", ch);
		return;
	}
	look_at_room(ch, vehicle->InRoom(), FALSE);
	return;
}


ACMD(do_look) {
	int look_type;

	if (!ch->desc)
		return;

	if (GET_POS(ch) < POS_SLEEPING)
		send_to_char("You can't see anything but stars!\n", ch);
	else if (AFF_FLAGGED(ch, AFF_BLIND))
		send_to_char("You can't see a damned thing, you're blind!\n", ch);
	else if (!ch->LightOk()) {
		send_to_char("It is pitch black...\n", ch);
		list_char_to_char(ch->InRoom()->people, ch);	/* glowing red eyes */
	} else {
		BUFFER(arg2, MAX_INPUT_LENGTH);
		BUFFER(arg, MAX_INPUT_LENGTH);
		
		half_chop(argument, arg, arg2);

		if (subcmd == SCMD_READ) {										//	read
			if (!*arg)	send_to_char("Read what?\n", ch);
			else		look_at_target(ch, arg, 1);
		} else if (!*arg)												//	look
			look_at_room(ch, 1);
		else if (is_abbrev(arg, "inside") || is_abbrev(arg, "into"))	//	look in
			look_in_obj(ch, arg2);
		else if (is_abbrev(arg, "outside") || is_abbrev(arg, "through") || is_abbrev(arg, "thru"))	//	look out
			look_out(ch);
		else if ((look_type = search_block(arg, dirs, 0)) >= 0)		//	look <dir>
			look_in_direction(ch, look_type, 5);
		else if (is_abbrev(arg, "at"))									//	look at
			look_at_target(ch, arg2, 0);
		else
			look_at_target(ch, arg, 0);									//	look <something>
	}
}


ACMD(do_scan)
{
	if (!ch->desc)
		return;
	
	for (int i = 0; i < NUM_OF_DIRS; ++i)
		if (Exit::IsViewable(ch, i))
			look_in_direction(ch, i, 5);
}



ACMD(do_examine) {
	BUFFER(arg, MAX_STRING_LENGTH);
	int bits;
	CharData *tmp_char;
	ObjData *tmp_object;
	
	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Examine what?\n", ch);
	} else {
		look_at_target(ch, arg, 0);

		bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
					FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

		if (tmp_object) {
			if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
					(GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
					(GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) {
				send_to_char("When you look inside, you see:\n", ch);
				look_in_obj(ch, arg);
			}
		}
	}
}


ACMD(do_gold) {
	ch->Send("You currently have %d mission point%s,\n"
		 "and have earned a total of %d mission point%s.\n",
			ch->points.mp, (ch->points.mp != 1) ? "s" : "",
			ch->points.lifemp, (ch->points.lifemp != 1) ? "s" : "");
}

unsigned int gcd(unsigned int a, unsigned int b);

unsigned int gcd(unsigned int a, unsigned int b) {
	if (a == 0)	return b;
	if (b == 0)	return a;
	
	while (a != b) {
		if (b > a)	b = b - a;
		else		a = a - b;
	}
	return b;
}

ACMD(do_score) {
	struct TimeInfoData playing_time;
	
	if (IS_NPC(ch))
		return;
	
#if 0
	ch->Send(	"You are %d years old.%s\n"
				"You have %d(%d) hit, and %d(%d) movement\n" //, and %d(%d) endurance points.\n"
//				"Your armor will absorb %d damage.\n"
				"You currently have %d mission points,\n"
				"and have earned a total of %d mission points.\n",
				GET_AGE(ch), (!age(ch).month && !age(ch).day) ? "  It's your birthday today." : "",
				GET_HIT(ch), GET_MAX_HIT(ch), GET_MOVE(ch), GET_MAX_MOVE(ch), /*GET_ENDURANCE(ch), GET_MAX_ENDURANCE(ch),*/
//				GET_ARMOR(ch),
				ch->points.mp, ch->points.lifemp);
				
	
	if (!IS_NPC(ch)) {
		int tempP = ch->player->special.PKills,
			tempM = ch->player->special.MKills,
			tempT = tempP + tempM;
		ch->Send("You have killed %d beings (%d (%2d%%) players, %d (%2d%%) mobs).\n",
				tempP + tempM, tempP, tempT ? (tempP * 100)/tempT : 0,
				tempM, tempT ? (tempM * 100)/tempT : 0);
		
		tempP = ch->player->special.PDeaths;
		tempM = ch->player->special.MDeaths;
		tempT = tempP + tempM;
		ch->Send("You have been killed by %d beings (%d (%2d%%) players, %d (%2d%%) mobs).\n",
				tempP + tempM, tempP, tempT ? (tempP * 100)/tempT : 0,
				tempM, tempT ? (tempM * 100)/tempT : 0);

//		int lcd = gcd(ch->player->special.PKills, ch->player->special.PDeaths);
//		if (lcd == 0)	lcd = 1;
//		ch->Send("PK-PD Ratio: %d:%d\n", ch->player->special.PKills / lcd, ch->player->special.PDeaths / lcd);

		playing_time = real_time_passed((time(0) - ch->player->time.logon) + ch->player->time.played, 0);
		ch->Send("You have been playing for %d day%s and %d hour%s.\n"
				"This ranks you as %s %s (level %d).\n",
				playing_time.day, playing_time.day == 1 ? "" : "s",
				playing_time.hours, playing_time.hours == 1 ? "" : "s",
				ch->GetTitle(), ch->GetName(), ch->GetLevel());
	}

	switch (GET_POS(ch)) {
		case POS_DEAD:		send_to_char("You are DEAD!\n", ch);										break;
		case POS_MORTALLYW:	send_to_char("You are mortally wounded!  You should seek help!\n", ch);	break;
		case POS_INCAP:		send_to_char("You are incapacitated, slowly fading away...\n", ch);		break;
		case POS_STUNNED:	send_to_char("You are stunned!  You can't move!\n", ch);					break;
		case POS_SLEEPING:	send_to_char("You are sleeping.\n", ch);									break;
		case POS_RESTING:	send_to_char("You are resting.\n", ch);									break;
		case POS_SITTING:	send_to_char("You are sitting.\n", ch);									break;
		case POS_FIGHTING:
			if (FIGHTING(ch))	ch->Send("You are fighting %s.\n", PERS(FIGHTING(ch), ch));
			else				send_to_char("You are fighting thin air.\n", ch);
			break;
		case POS_STANDING:	send_to_char("You are standing.\n", ch);									break;
		default:	send_to_char("You are floating.\n", ch);											break;
	}
	
//	if (GET_COND(ch, DRUNK) > 10)			send_to_char("You are intoxicated.\n", ch);
//	if (GET_COND(ch, FULL) == 0)			send_to_char("You are hungry.\n", ch);
//	if (GET_COND(ch, THIRST) == 0)			send_to_char("You are thirsty.\n", ch);
	if (AFF_FLAGGED(ch, AFF_BLIND))			send_to_char("You have been blinded!\n", ch);
	if (AFF_FLAGGED(ch, AFF_INVISIBLE_FLAGS))		send_to_char("You are invisible.\n", ch);
	if (AFF_FLAGGED(ch, AFF_DETECT_INVIS))	send_to_char("You are sensitive to the presence of invisible things.\n", ch);
	if (AFF_FLAGGED(ch, AFF_SANCTUARY))		send_to_char("You are protected by Sanctuary.\n", ch);
	if (AFF_FLAGGED(ch, AFF_POISON))		send_to_char("You are poisoned!\n", ch);
	if (AFF_FLAGGED(ch, AFF_CHARM))			send_to_char("You have been charmed!\n", ch);
	if (AFF_FLAGGED(ch, AFF_INFRAVISION))	send_to_char("You have infrared vision capabilities.\n", ch);
	if (PRF_FLAGGED(ch, PRF_SUMMONABLE))	send_to_char("You are summonable by other players.\n", ch);
#else

	playing_time = real_time_passed((time(0) - ch->GetPlayer()->m_LoginTime) + ch->GetPlayer()->m_TotalTimePlayed, 0);
	
	BUFFER(buf, MAX_STRING_LENGTH);
	strcpy(buf, findtype(ch->GetRace(), race_types));
	
	ch->Send("[%s] %s %s\n", CAP(buf), ch->GetTitle(), ch->GetName());
	
	if (subcmd == SCMD_SCORE)
	{
		TimeInfoData charAge = age(ch);
		ch->Send("--- Vitals:\n"
				 "    Level:    [%3d]\n"
				 "    Sex:      [%s]\n"
				 "    HP:       [%d/%d]\n"
				 "    MV:       [%d/%d]\n"
			//	 "    EN:       [%d/%d]\n"
				 "    MP:       [%6d]      Lifetime: [%6d]\n"
			     "    Skill Pts:[%4d]        Lifetime: [%4d]\n"
			     "    Age:      [%3d years old] %s\n"
				 "    Played:   [%d:%02d:%02d]\n"
				 "    Carrying: [%2d] items weighing [%3d] bulk\n"
//				 "    Combat Mode: `y%s`n\n"
				 ,
			ch->GetLevel(),
			GetEnumName(ch->GetSex()),
			GET_HIT(ch), GET_MAX_HIT(ch),
			GET_MOVE(ch), GET_MAX_MOVE(ch),
		//	GET_ENDURANCE(ch), GET_MAX_ENDURANCE(ch),
			ch->points.mp, ch->points.lifemp,
			ch->GetPlayer()->m_SkillPoints, ch->GetPlayer()->m_LifetimeSkillPoints,
			charAge.year, (!charAge.month && !charAge.day) ? "  `gIt's your birthday today.`n" : "",
			playing_time.day, playing_time.hours, playing_time.minutes,
			IS_CARRYING_N(ch), IS_CARRYING_W(ch)/*,
			combat_modes[GET_COMBAT_MODE(ch)]*/);
	}


	ch->Send("--- Stats:\n"
			 "    Strength:     %-3d [%3d`c+%-3d`n]     Agility:      %-3d [%3d`c+%-3d`n]\n"
			 "    Health:       %-3d [%3d`c+%-3d`n]     Perception:   %-3d [%3d`c+%-3d`n]\n"
			 "    Coordination: %-3d [%3d`c+%-3d`n]     Knowledge:    %-3d [%3d`c+%-3d`n]\n",
		GET_STR(ch), GET_REAL_STR(ch), GET_STR(ch) - GET_REAL_STR(ch),	GET_AGI(ch), GET_REAL_AGI(ch), GET_AGI(ch) - GET_REAL_AGI(ch),
		GET_HEA(ch), GET_REAL_HEA(ch), GET_HEA(ch) - GET_REAL_HEA(ch),	GET_PER(ch), GET_REAL_PER(ch), GET_PER(ch) - GET_REAL_PER(ch),
		GET_COO(ch), GET_REAL_COO(ch), GET_COO(ch) - GET_REAL_COO(ch),	GET_KNO(ch), GET_REAL_KNO(ch), GET_KNO(ch) - GET_REAL_KNO(ch));

	if (subcmd == SCMD_SCORE)
	{
		int tempP = ch->GetPlayer()->m_PlayerKills,
			tempM = ch->GetPlayer()->m_MobKills,
			tempT = tempP + tempM;
		ch->Send("--- Combat Record      Players      Mobs\n"
				 "    Kills:     [%4d]   %-5d (%3d%%) %-5d (%3d%%)\n",
			tempP + tempM,
				tempP, tempT ? (tempP * 100)/tempT : 0,
				tempM, tempT ? (tempM * 100)/tempT : 0);
		
		tempP = ch->GetPlayer()->m_PlayerDeaths;
		tempM = ch->GetPlayer()->m_MobDeaths;
		tempT = tempP + tempM;
		ch->Send("    Deaths:    [%4d]   %-5d (%3d%%) %-5d (%3d%%)\n",
			tempP + tempM,
				tempP, tempT ? (tempP * 100)/tempT : 0,
				tempM, tempT ? (tempM * 100)/tempT : 0);
		
		if (AFF_FLAGS(ch).testForAny(Affect::AFF_INVISIBLE_FLAGS))		send_to_char("You are invisible.\n", ch);
	}
	else if (subcmd == SCMD_ATTRIBUTES)
	{
		ch->Send("Armor:    ");
		for (int i = 0; i < NUM_DAMAGE_TYPES; ++i)
			ch->Send("  %-3.3s", damage_type_abbrevs[i]);
		ch->Send("\n");
		
		int startingArmorLoc = ARMOR_LOC_BODY;
		int endingArmorLoc = ARMOR_LOC_HEAD;
	
		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPECIAL_MISSION))
		{
			startingArmorLoc = endingArmorLoc = ARMOR_LOC_MISSION;
		}
	
		for (int loc = startingArmorLoc; loc <= endingArmorLoc; ++loc)
		{
			ch->Send("   %-8.8s", damage_locations[loc]);
			for (int i = 0; i < NUM_DAMAGE_TYPES; ++i)
				ch->Send(" %3d%%", GET_ARMOR(ch, loc, i));
			ch->Send("\n");
		}

		if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPECIAL_MISSION))
		{
			ch->Send("   Bonus Absorb: %3d%%\n", GET_EXTRA_ARMOR(ch));
		}
	}
#endif
}


ACMD(do_inventory) {
	send_to_char("You are carrying:\n", ch);
	list_obj_to_char(ch->carrying, ch, 1, TRUE);
}


ACMD(do_equipment) {
	bool	found = false;
	bool	bShowAll = false;
	
	bShowAll = is_abbrev(argument, "all");

	send_to_char("You are using:\n", ch);
	for (int i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(ch, i))
		{
			send_to_char(where[wear_message_number(GET_EQ(ch, i), (WearPosition)i)], ch);
			if (ch->CanSee(GET_EQ(ch, i)))	show_obj_to_char(GET_EQ(ch, i), ch, 1);
			else							send_to_char("Something.\n", ch);
			found = true;
		}
		else if (bShowAll && i < WEAR_HAND_R)
		{
			ch->Send("%s\n", where[wear_message_number(GET_EQ(ch, i), (WearPosition)i)]);
			
			found = true;
		}
	}
	if (!found)	send_to_char(" Nothing.\n", ch);
}


ACMD(do_time) {
	extern void weather_and_time(int mode);
	extern UInt32 pulse;
	char *suf;
	int day, minute, second;

	if (IS_STAFF(ch) && !str_cmp(argument, "pulse"))
	{
		weather_and_time(1);
	}

	day = time_info.day + 1;	/* day in [1..30] */

	if (day == 1)					suf = "st";
	else if (day == 2)				suf = "nd";
	else if (day == 3)				suf = "rd";
	else if (day < 20)				suf = "th";
	else if ((day % 10) == 1)		suf = "st";
	else if ((day % 10) == 2)		suf = "nd";
	else if ((day % 10) == 3)		suf = "rd";
	else							suf = "th";
	
	minute = (int)((double)(pulse % PULSES_PER_MUD_HOUR) / /*12.5*/ 10.0);
	second = (int)(((pulse % PULSES_PER_MUD_HOUR) - (minute * /*12.5*/ 10.0)) * 6.0/*4.8*/);
	
	ch->Send("It is %02d:%02d:%02d (%02d:%02d %cm) on %s\n"
			 "%s %d%s, %d.\n",
			 time_info.hours, minute, second,
			 (time_info.hours <= 12) ? ((time_info.hours == 0) ? 12 : time_info.hours) :
			 (time_info.hours - 12), minute,
			 (time_info.hours < 12) ? 'a' : 'p',
			 weekdays[time_info.weekday],
			 month_name[time_info.month], day, suf, time_info.year);
}


ACMD(do_weather)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	ZoneData *		zone = ch->InRoom()->GetZone();
	WeatherSystem * weather = zone ? zone->GetWeather() : NULL;
	
	if (IS_STAFF(ch))
	{
		
		if (!str_cmp(argument, "status"))
		{
			ch->Send("--- WEATHER DEBUG\n");
			
			//	Debug info here
			if (!zone)
				ch->Send("This room is not part of a zone.\n");
			else if (!weather && !zone->GetWeatherParent())
				ch->Send("This zone does not have a weather system.\n");
			else if (zone->GetWeatherParent())
			{
				ch->Send("This zone inherits from zone %s.\n", zone->GetWeatherParent()->GetTag());	
				if (!weather)	ch->Send("The parent zone does not appear to have a weather system, or the current zone failed to link to it.\n");
			}
			
			
			if (weather)
			{
				int currentSeason = weather->m_Climate.GetCurrentSeason();
				
				ch->Send(
					"Season    : `c%d`n\n"
					"Flags     : `c%s`n\n"
					"Light     : `c%-3d`n%%      Energy    : `c%d`n\n"
					"Wind      : `c%-3d`nmph %s\n"
					"Temp      : `c%-3d`nC      Humidity  : `c%d`n\n"
					"Precip    : `c%-3d`n%%      Change    : `c%d`n\n"
					"Pressure  : `c%-4d`n      Change    : `c%d`n\n",
					currentSeason + 1,
					weather->m_Flags.print().c_str(),
					weather->m_Light, weather->m_Energy,
					weather->m_WindSpeed, findtype(weather->m_WindDirection, dirs),
					weather->m_Temperature, weather->m_Humidity,
					weather->m_PrecipitationRate, weather->m_PrecipitationChange,
					weather->m_Pressure, weather->m_PressureChange);
					
				
				ch->Send(
					"--- Climate Settings\n"
					"Climate Flags   : `c%s`n\n"
					"Climate Energy  : `c%d`n\n"
					"Seasonal Pattern: `c%s`n\n",
					weather->m_Climate.m_Flags.print().c_str(),
					weather->m_Climate.m_Energy,
					GetEnumName(weather->m_Climate.m_SeasonalPattern));
					
				int numSeasons = Weather::num_seasons[weather->m_Climate.m_SeasonalPattern];
				for (int i = 0; i < numSeasons; ++i)
				{
					Weather::Climate::Season &season = weather->m_Climate.m_Seasons[i];
				
					ch->Send(
							"  Season %d:%s\n"
							"     Wind     : `c%s`n, %s%s\n"
							"     Precip   : `c%-10.10s`n Temp     : `c%s`n\n",
							i + 1, i == currentSeason ? " `c(Current Season)`n" : "",
							GetEnumName(season.m_Wind),
							findtype(season.m_WindDirection, dirs),
							season.m_WindVaries ? " (Varies)" : "",
							GetEnumName(season.m_Precipitation),
							GetEnumName(season.m_Temperature));
				}
			}

			ch->Send("--- END WEATHER DEBUG\n");
		}
		else if (!str_cmp(argument, "reset"))
		{
			if (!weather)
			{
				ch->Send("This zone has no weather system attached to it.\n");
				return;
			}

			weather->Start();
			ch->Send("Weather reset.\n");
		}
		else if (!str_cmp(argument, "pulse"))
		{
			if (!weather)
			{
				ch->Send("This zone has no weather system attached to it.\n");
				return;
			}

			weather->Update();
		}
	}
	
	if (!OUTSIDE(ch))
	{
		ch->Send("The weather seems fine, indoors.\n");
		return;
	}
	else if (!weather)
	{		
		ch->Send("The weather seems normal.\n");
		return;
	}

	*buf = 0;
	
	if (weather->m_PrecipitationRate > 0)
	{
		if (weather->m_Temperature <= 0)	strcat(buf, "It's snowing");
		else								strcat(buf, "It's raining");
		
		if (weather->m_PrecipitationRate > 65)		strcat(buf, " extremely hard");
		else if (weather->m_PrecipitationRate > 50)	strcat(buf, " very hard");
		else if (weather->m_PrecipitationRate > 30)	strcat(buf, " hard");
		else if (weather->m_PrecipitationRate < 15)	strcat(buf, " lightly");
	}
	else
	{
		if (weather->m_Humidity == 100)		strcat(buf, "It's completely overcast");
		else if (weather->m_Humidity > 80)	strcat(buf, "It's very cloudy");
		else if (weather->m_Humidity > 55)	strcat(buf, "It's cloudy");
		else if (weather->m_Humidity > 25)	strcat(buf, "It's partly cloudy");
		else if (weather->m_Humidity > 0)	strcat(buf, "It's mostly clear");
		else 								strcat(buf, "It's perfectly clear");
	}
		
	strcat(buf, ", ");
	
	if (weather->m_Temperature > 100)		strcat(buf, "boiling");
	else if (weather->m_Temperature > 80)	strcat(buf, "blistering");
	else if (weather->m_Temperature > 50)	strcat(buf, "incredibly hot");
	else if (weather->m_Temperature > 40)	strcat(buf, "very, very hot");
	else if (weather->m_Temperature > 30)	strcat(buf, "very hot");
	else if (weather->m_Temperature > 24)	strcat(buf, "hot");
	else if (weather->m_Temperature > 18)	strcat(buf, "warm");
	else if (weather->m_Temperature > 9)	strcat(buf, "mild");
	else if (weather->m_Temperature > 3)	strcat(buf, "cool");
	else if (weather->m_Temperature > -1)	strcat(buf, "cold");
	else if (weather->m_Temperature > -10)	strcat(buf, "freezing");
	else if (weather->m_Temperature > -25)	strcat(buf, "well past freezing");
	else									strcat(buf, "numbingly frozen");
	
	strcat(buf, ", and ");
	
	if (weather->m_WindSpeed <= 0)			strcat(buf, "there is absolutely no wind");
	else if (weather->m_WindSpeed < 10)		strcat(buf, "calm");
	else if (weather->m_WindSpeed < 20)		strcat(buf, "breezy");
	else if (weather->m_WindSpeed < 35)		strcat(buf, "windy");
	else if (weather->m_WindSpeed < 50)		strcat(buf, "very windy");
	else if (weather->m_WindSpeed < 70)		strcat(buf, "very, very windy");
	else if (weather->m_WindSpeed < 100)	strcat(buf, "there is a gale blowing");
	else									strcat(buf, "the wind is unbelievable");
	
	strcat(buf, ".\n");
	
	ch->Send("%s", buf);
}


ACMD(do_help) {
	HelpManager::Entry *this_help;

	if (!ch->desc)	return;
	
	skip_spaces(argument);

	if (!*argument)
		page_string(ch->desc, help.c_str());
//	else if (help_table.empty())
//		send_to_char("No help available.\n", ch);
	else if (!(this_help = HelpManager::Instance()->Find(argument))) {
		send_to_char("There is no help on that word.\n", ch);
		log("HELP: %s tried to get help on %s", ch->GetName(), argument);
	} else if (this_help->m_MinimumLevel > ch->GetLevel())
		send_to_char("There is no help on that word.\n", ch);
	else {
		BUFFER(entry, MAX_STRING_LENGTH * 2);
		sprintf(entry, "%s\n%s", this_help->m_Keywords.c_str(), this_help->m_Entry.c_str());
		page_string(ch->desc, entry);
	}
}


/*********************************************************************
* New 'do_who' by Daniel Koepke [aka., "Argyle Macleod"] of The Keep *
******************************************************************* */

char *WHO_USAGE =
	"Usage: who [minlev[-maxlev]] [-n name] [-c races] [-t team] [-rzmsp]\n"
	"\n"
	"Races: (H)uman, (S)ynthetic, (P)redator, (A)lien\n"
	"Teams: red, blue, green, gold, purple, teal\n"
	"\n"
	" Switches: \n"
	"\n"
	"  -r = who is in the current room\n"
	"  -z = who is in the current zone\n"
	"\n"
	"  -m = only show those on mission\n"
	"  -s = only show staff\n"
	"  -p = only show players\n"
	"\n";

unsigned int	boot_high = 0;
ACMD(do_who) {
	BUFFER(buf, MAX_STRING_LENGTH);
	BUFFER(arg, MAX_STRING_LENGTH);
	BUFFER(name_search, MAX_INPUT_LENGTH);
	
//	ShipData *s1, *s2;
	CharData *		wch;
	int			low = 0,
					high = LVL_CODER,
					i;
	bool			who_room = false,
					who_zone = false,
					who_mission = false,
					//traitors = false,
					no_staff = false,
					no_players = false;
	int				teammates = 0;
	unsigned int	staff = 0,
					players = 0;
	RaceFlags		showrace;
	int				tally_races[NUM_RACES] = { 0, 0, 0, 0, 0 };
	int				tally_teams[MAX_TEAMS + 1];
	char			mode;

	int  players_clan = 0, players_clannum = 0, ccmd = 0;
	extern char *level_string[];

	for (int i = 0; i <= MAX_TEAMS; ++i)
		tally_teams[i] = 0;

	skip_spaces(argument);
	strcpy(buf, argument);
	
	while (*buf) {
		half_chop(buf, arg, buf);
		if (isdigit(*arg)) {
			sscanf(arg, "%d-%d", &low, &high);
		} else if (*arg == '-') {
			mode = *(arg + 1);	/* just in case; we destroy arg in the switch */
			switch (mode) {
				case 't':				// Only traitors
//					traitors = true;
					half_chop(buf, arg, buf);
					if (*arg)
						teammates = search_block(arg, team_names, 0);
					else
						teammates = -1;					
					break;
				case 'z':				// In Zone
					who_zone = true;
					break;
				case 'm':				// Only Missoin
					who_mission = true;
					break;
				case 'l':				// Range
					half_chop(buf, arg, buf);
					sscanf(arg, "%d-%d", &low, &high);
					break;
				case 'n':				// Name
					half_chop(buf, name_search, buf);
					break;
				case 'r':				// In room
					who_room = true;
					break;
				case 's':				// Only staff
					no_players = true;
					break;
				case 'p':				// Only Players
					no_staff = true;
					break;
				case 'c':				//	Race specific...
					half_chop(buf, arg, buf);
					for (i = 0; i < strlen(arg); i++)
						showrace.set(GetRaceFlag(arg[i]));
					break;
				default:
					send_to_char(WHO_USAGE, ch);
					return;
			}				/* end of switch */
		} else {			/* endif */
			send_to_char(WHO_USAGE, ch);
			return;
		}
	}
	
	BUFFER(staff_buf, MAX_STRING_LENGTH);
	BUFFER(player_buf, MAX_STRING_LENGTH);
	
	strcpy(staff_buf,	"Staff currently online\n"
						"----------------------\n");
	strcpy(player_buf,	"Players currently online\n"
						"------------------------\n");

    FOREACH(DescriptorList, descriptor_list, iter)
    {
    	DescriptorData *d = *iter;
    	
		if (!d->GetState()->IsPlaying())	continue;
		if (!(wch = d->GetOriginalCharacter()))			continue;
//		if (who_zone && ch->CanSee(wch) != VISIBLE_FULL)	continue;
		if (who_zone && (ch->InRoom() == wch->InRoom()
			? ch->CanSee(wch) == VISIBLE_NONE
			: ch->GetRelation(wch) != RELATION_FRIEND && wch->m_AffectFlags.testForAny(Affect::AFF_STEALTH_FLAGS)))
				continue;
		if (GET_REAL_LEVEL(ch) < wch->GetPlayer()->m_StaffInvisLevel)	continue;
		if ((wch->GetLevel() < low) || (wch->GetLevel() > high))
			continue;
		if ((no_staff && IS_STAFF(wch)) || (no_players && !IS_STAFF(wch)))
			continue;
		if (*name_search && str_cmp(wch->GetName(), name_search) && !str_str(wch->GetTitle(), name_search))
			continue;
		if (teammates == -1 && GET_TEAM(wch) != GET_TEAM(ch))
			continue;
		if (teammates > 0 && GET_TEAM(wch) != teammates)
			continue;
		if (who_mission && !CHN_FLAGGED(wch, Channel::Mission))
			continue;
		if (who_zone)
		{
//		    s1 = ship_from_room(IN_ROOM(ch));
//		    s2 = ship_from_room(IN_ROOM(wch));
//		 	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPACECRAFT) && s1 && s2 && s1 != s2)
//		 		continue;

		 	if (!IsSameEffectiveZone(ch->InRoom(), wch->InRoom()))
				continue;
		}
		if (who_room && ((IN_ROOM(wch) != IN_ROOM(ch)) || ch->CanSee(wch) == VISIBLE_NONE))
			continue;
		if (showrace.any() && !showrace.test(wch->GetRace()))
			continue;
		
		if (!IS_STAFF(wch))	tally_races[wch->GetFaction()] += 1;
		if (GET_TEAM(wch))	tally_teams[GET_TEAM(wch)] += 1;

		*buf = '\0';
		
		if (IS_STAFF(wch)) {
//			sprintf(buf, "`y %s %s %s`n", level_string[wch->GetLevel()-LVL_IMMORT], wch->GetName(), wch->GetTitle());
			if (!wch->GetPlayer()->m_Pretitle.empty()) {
				int width = 13 - cstrlen(wch->GetPlayer()->m_Pretitle.c_str());
				width = MAX(width, 0);
				sprintf(buf, "`y %*s%s%*s ", width / 2, "", wch->GetPlayer()->m_Pretitle.c_str(), (width + 1) / 2, "");
			}
			else					sprintf(buf, "`y %s ", level_string[wch->GetLevel()-LVL_IMMORT]);
			if (GET_TEAM(wch))
				sprintf_cat(buf, "`y[%s`y]`n ", team_titles[GET_TEAM(wch)]);
			sprintf_cat(buf, "%s %s`n", wch->GetName(), wch->GetTitle());
			staff++;
		} else {
//			sprintf(buf, "`n[%3d %s] `%c%s %s`n", wch->GetLevel(), RACE_ABBR(wch),
//					relation_colors[ch->GetRelation(wch)], wch->GetTitle(), wch->GetName());
			char relationColor = relation_colors[ch->GetRelation(wch)];
			sprintf(buf, "`n[%3d %s] ", wch->GetLevel(), findtype(wch->GetRace(), race_abbrevs));
			if (GET_TEAM(wch))
				sprintf_cat(buf, "`y[%s`y]`n ", team_titles[GET_TEAM(wch)]);
			sprintf_cat(buf, "`%c%s`n `%c%s`n", relationColor, wch->GetTitle(), relationColor, wch->GetName());
			players++;
		}
		
		if (GET_CLAN(wch) && Clan::GetMember(wch))
		{
			sprintf_cat(buf, " <`C%s", GET_CLAN(wch)->GetTag());
			
			if (Clan::HasPermission(wch, Clan::Permission_ShowRank))
			{
				strcat(buf, " ");
				strcat(buf, Clan::GetMembersRank(wch)->GetName());	
			}
			
			strcat(buf, "`n>");
		}

		if (!PRF_FLAGGED(wch, PRF_PK))				strcat(buf, " (`cno-pk`n)");
		
		if (wch->GetPlayer()->m_StaffInvisLevel)	sprintf_cat(buf, " (`wi%d`n)", wch->GetPlayer()->m_StaffInvisLevel);
		else if (AFF_FLAGS(wch).testForAny(Affect::AFF_INVISIBLE_FLAGS))	strcat(buf, " (`winvis`n)");
		
		if (PLR_FLAGGED(wch, PLR_MAILING))			strcat(buf, " (`bmailing`n)");
		else if (PLR_FLAGGED(wch, PLR_WRITING))		strcat(buf, " (`rwriting`n)");

		if (CHN_FLAGGED(wch, Channel::NoShout))		strcat(buf, " (`gdeaf`n)");
		if (CHN_FLAGGED(wch, Channel::NoTell))		strcat(buf, " (`gnotell`n)");
		if (CHN_FLAGGED(wch, Channel::Mission))		strcat(buf, " (`mmission`n)");
		if (AFF_FLAGGED(wch, AFF_TRAITOR))			strcat(buf, " (`RTRAITOR`n)");
		if (!wch->GetPlayer()->m_AFKMessage.empty())strcat(buf, " `c[AFK]");
		strcat(buf, "`n\n");
		
		if (IS_STAFF(wch))		strcat(staff_buf, buf);
		else					strcat(player_buf, buf);
	}
	
	*buf = '\0';
	
	if (staff) {
		strcat(buf, staff_buf);
		strcat(buf, "\n");
	}
	if (players) {
		strcat(buf, player_buf);
		strcat(buf, "\n");
	}

	if ((staff + players) == 0) {
		strcat(buf, "No staff or players match your search criteria.\n");
	}
	else
	{
		int tallyTotal = 0;
		
		for (int i = RACE_HUMAN; i <= RACE_ALIEN; ++i)
		{
			tallyTotal += tally_races[i];
			if (tally_races[i])	sprintf_cat(buf, "%s [`c%d`n]    ", faction_types[i], tally_races[i]);
			if (i == RACE_HUMAN) i = RACE_SYNTHETIC;	//	Skip it
		}
		if (tallyTotal > 0)		strcat(buf, "\n");
		
		int team_count = 0;
		for (int i = 1; i <= MAX_TEAMS; ++i)
			team_count += tally_teams[i];
		
		if (team_count)
		{
			for (int i = 1; i <= MAX_TEAMS; ++i)
				if (tally_teams[i])	sprintf_cat(buf, "%s [`c%d`n]    ", team_titles[i], tally_teams[i]);
			strcat(buf, "\n");
		}
	}
	
	if (staff)		sprintf_cat(buf, "There %s %d staff%s", (staff == 1 ? "is" : "are"), staff, players ? " and there" : ".");
	if (players)	sprintf_cat(buf, "%s %s %d player%s.", staff ? "" : "There", (players == 1 ? "is" : "are"), players, (players == 1 ? "" : "s"));
	strcat(buf, "\n");
	
	if ((staff + players) > boot_high)
		boot_high = staff+players;

	sprintf_cat(buf, "There is a boot time high of %d player%s.\n", boot_high, (boot_high == 1 ? "" : "s"));
	
	page_string(ch->desc, buf);
}


#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-o] [-p] [-d] [-c] [-m]\n"

ACMD(do_users) {
	BUFFER(buf, MAX_STRING_LENGTH);
	BUFFER(arg, MAX_INPUT_LENGTH);
	BUFFER(name_search, MAX_INPUT_LENGTH);
	BUFFER(host_search, MAX_INPUT_LENGTH);
	
	CharData *	tch;
	int		low = 0,
				high = LVL_CODER,
				num_can_see = 0;
	bool 		traitors = false,
				playing = false,
				deadweight = false,
				complete = false,
				matching = false;
	char 		mode;

	strcpy(buf, argument);
	while (*buf) {
		half_chop(buf, arg, buf);
		if (*arg == '-') {
			mode = *(arg + 1);  /* just in case; we destroy arg in the switch */
			switch (mode) {
				case 'm':	matching = true;							break;
				case 'c':	complete = true;							break;
				case 'o':	traitors = true;	playing = true;			break;
				case 'p':	playing = true;								break;
				case 'd':	deadweight = true;							break;
				case 'l':
					playing = true;
					half_chop(buf, arg, buf);
					sscanf(arg, "%d-%d", &low, &high);
					break;
				case 'n':
					playing = true;
					half_chop(buf, name_search, buf);
					break;
				case 'h':
					playing = true;
					half_chop(buf, host_search, buf);
					break;
				default:
					send_to_char(USERS_FORMAT, ch);
					return;
			}
		} else {
			send_to_char(USERS_FORMAT, ch);
			return;
		}
	}
	
	BUFFER(idletime, MAX_INPUT_LENGTH);
	
	strcpy(buf, "Num Race      Name         State          Idl Login@   Site\n");
	strcat(buf, "--- --------- ------------ -------------- --- -------- ------------------------\n");

	one_argument(argument, arg);

    FOREACH(DescriptorList, descriptor_list, iter)
    {
    	DescriptorData *d = *iter;
		tch = d->GetOriginalCharacter();
		if (d->GetState()->IsPlaying())
		{
			if (deadweight)												continue;
			if (!tch)													continue;
			if (*host_search && d->m_Host.find(host_search) == Lexi::String::npos)	continue;
			if (*name_search && str_cmp(tch->GetName(), name_search))	continue;
			if (ch->CanSee(tch, GloballyVisible) == VISIBLE_NONE)		continue;
			if ((tch->GetLevel() < low) || (tch->GetLevel() > high))		continue;
			if (traitors && !AFF_FLAGGED(tch, AFF_TRAITOR))				continue;
		} else {
			if (playing)												continue;
			if (tch && ch->CanSee(tch, GloballyVisible) == VISIBLE_NONE)	continue;
		}
		if (matching) {
			if (d->m_Host.empty())										continue;
			if (d->m_Character && d->m_Character->GetPlayer()->m_StaffInvisLevel > ch->GetLevel())	continue;

			bool bIsMatch = false;
			FOREACH(DescriptorList, descriptor_list, iter)
			{
				DescriptorData *match = *iter;
    			
				if (match->m_Host.empty() || (match == d))				continue;
				if (match->m_Character && match->m_Character->GetPlayer()->m_StaffInvisLevel > ch->GetLevel())	continue;
				if (match->m_Host != d->m_Host)							continue;
				
				bIsMatch = true;
				break;
			}
			if (!bIsMatch)												continue;
		}
		
		Lexi::String	timestr = Lexi::CreateDateString(d->m_LoginTime, "%X");
		
		Lexi::String state = (d->GetState()->IsPlaying() && d->m_OrigCharacter) ? "Switched" : d->GetState()->GetName();

		if (d->GetState()->IsPlaying() && d->m_Character && (!IS_STAFF(d->m_Character) || d->m_Character->GetPlayer()->m_IdleTime >= 3))
			sprintf(idletime, "%3d", d->m_Character->GetPlayer()->m_IdleTime);
		else
			strcpy(idletime, "");

		strcat(buf, d->GetState()->IsPlaying() ? "`n" : "`g");

		if (!tch || tch->m_Name.empty())
			sprintf_cat(buf, "%3d     -     %-12s %-14.14s %-3s %-8s ", d->m_ConnectionNumber,
					"UNDEFINED", state.c_str(), idletime, timestr.c_str());
		else
			sprintf_cat(buf, "%3d [%3d %3s] %-12s %-14.14s %-3s %-8s ", d->m_ConnectionNumber,
					tch->GetLevel(), findtype(tch->GetRace(), race_abbrevs), tch->GetName(), state.c_str(), idletime, timestr.c_str());

		sprintf_cat(buf, complete ? "[%-22s]\n" : "[%-22.22s]\n", !d->m_Host.empty() ? d->m_Host.c_str() : "Hostname unknown");

		num_can_see++;
	}

	sprintf_cat(buf, "\n`n%d visible sockets connected.\n", num_can_see);

	page_string(ch->desc, buf);
}


/* Generic page_string function for displaying text */
ACMD(do_gen_ps) {
	switch (subcmd) {
		case SCMD_CREDITS:	page_string(ch->desc, credits.c_str());		break;
		case SCMD_NEWS:		page_string(ch->desc, news.c_str());			break;
		case SCMD_INFO:		page_string(ch->desc, info.c_str());			break;
		case SCMD_WIZLIST:	/*page_string(ch->desc, wizlist, 0);*/ send_to_char(wizlist.c_str(), ch);			break;
		case SCMD_IMMLIST:	page_string(ch->desc, immlist.c_str());		break;
		case SCMD_HANDBOOK:	page_string(ch->desc, handbook.c_str());		break;
		case SCMD_POLICIES:	page_string(ch->desc, policies.c_str());		break;
		case SCMD_MOTD:		page_string(ch->desc, motd.c_str());			break;
		case SCMD_IMOTD:	page_string(ch->desc, imotd.c_str());		break;
		case SCMD_CLEAR:	send_to_char("\033[H\033[J", ch);			break;
		case SCMD_VERSION:	send_to_char(circlemud_version, ch);		break;
		case SCMD_WHOAMI:	ch->Send("%s\n", ch->GetName());			break;
	}
}


void perform_mortal_where(CharData * ch, char *arg) {
	CharData *i;
	
	if (!*arg) {
		send_to_char("Players in your Zone\n--------------------\n", ch);
	    FOREACH(DescriptorList, descriptor_list, iter)
	    {
    		DescriptorData *d = *iter;

			if (d->GetState()->IsPlaying())
			{
				i = d->GetOriginalCharacter();
				if (i && ch->CanSee(i) != VISIBLE_NONE && i->InRoom()
					&& IsSameEffectiveZone(ch->InRoom(), i->InRoom()))
				{
					ch->Send("%-20s - %s\n", i->GetName(), i->InRoom()->GetName());
				}
			}
		}
	}
	else
	{			/* print only FIRST char, not all. */
		FOREACH(CharacterList, character_list, iter)
		{
			CharData *i = *iter;
			if (i->InRoom() && IsSameEffectiveZone(ch->InRoom(), i->InRoom())
					&& ch->CanSee(i) != VISIBLE_NONE
					&& isname(arg, i->GetAlias())) {
				ch->Send("%-25s - %s\n", i->GetName(), i->InRoom()->GetName());
				
				return;
			}
		}
		send_to_char("No-one around by that name.\n", ch);
	}
}


void print_object_location(int num, ObjData * obj, CharData * ch, int recur) {
	if (num > 0)	ch->Send("O%3d. %-25s - ", num, obj->GetName());
	else			ch->Send("%33s", " - ");

	if (obj->InRoom()) {
		ch->Send("[%10s] %s\n", obj->InRoom()->GetVirtualID().Print().c_str(), obj->InRoom()->GetName());
	} else if (obj->CarriedBy()) {
		ch->Send("carried by %s\n", PERS(obj->CarriedBy(), ch));
	} else if (obj->WornBy()) {
		ch->Send("worn by %s\n", PERS(obj->WornBy(), ch));
	} else if (obj->Inside()) {
		ch->Send("inside %s%s\n", obj->Inside()->GetName(), (recur ? ", which is" : " "));
		if (recur)
			print_object_location(0, obj->Inside(), ch, recur);
	} else
		ch->Send("in an unknown location\n");
}


void perform_immort_where(CharData * ch, char *arg) {
	CharData *i;
	int num = 0, found = 0;

	if (!*arg) {
		send_to_char("Players\n-------\n", ch);
	    FOREACH(DescriptorList, descriptor_list, iter)
	    {
	    	DescriptorData *d = *iter;
			if (d->GetState()->IsPlaying())
			{
				i = d->GetOriginalCharacter();
				if (i && ch->CanSee(i) != VISIBLE_NONE && i->InRoom()) {
					if (d->m_OrigCharacter)
						ch->Send("%-25s - [%10s] %s (in %s)\n",
								i->GetName(),
								d->m_Character->InRoom()->GetVirtualID().Print().c_str(),
								d->m_Character->InRoom()->GetName(), d->m_Character->GetName());
					else
						ch->Send("%-25s - [%10s] %s\n", i->GetName(),
								i->InRoom()->GetVirtualID().Print().c_str(), i->InRoom()->GetName());
				}
			}
		}
	} else {
		FOREACH(CharacterList, character_list, iter)
		{
			CharData *i = *iter;
			if (ch->CanSee(i) != VISIBLE_NONE && i->InRoom() && isname(arg, i->GetAlias())) {
				found = 1;
				ch->Send("M%3d. %-25s - [%10s] %s\n", ++num, i->GetName(),
						i->InRoom()->GetVirtualID().Print().c_str(), i->InRoom()->GetName());
			}
		}
		num = 0;
		FOREACH(ObjectList, object_list, iter)
		{
			ObjData *k = *iter;
			
			if (ch->CanSee(k) && isname(arg, k->GetAlias()) &&
					(!k->CarriedBy() || ch->CanSee(k->CarriedBy()) != VISIBLE_NONE)) {
				found = 1;
				print_object_location(++num, k, ch, TRUE);
			}
		}
		if (!found)
		send_to_char("Couldn't find any such thing.\n", ch);
	}
}



ACMD(do_where) {
	BUFFER(arg, MAX_STRING_LENGTH);
	one_argument(argument, arg);

	if (NO_STAFF_HASSLE(ch))
		perform_immort_where(ch, arg);
	else
		perform_mortal_where(ch, arg);
}



ACMD(do_levels) {
	BUFFER(buf, MAX_STRING_LENGTH);
	
	int	i;
	int final_level = ch->GetLevel() < LVL_IMMORT ? 10 : 15;
	
	if (IS_NPC(ch) || !ch->desc) {
		send_to_char("You ain't nothin' but a hound-dog.\n", ch);
		return;
	}

	
	for (i = 1 ; i <= final_level; i++) {
		sprintf_cat(buf, "[%3d] : ", (i <= 10) ? /*((i * 10) - 9)*/ (i == 1 ? 1 : ((i - 1) * 10)) : (i + 90));
		switch (ch->GetSex())
		{
			case SEX_MALE:
			case SEX_NEUTRAL:
				strcat(buf, titles[(int) ch->GetRace()][i].title_m);
				break;
			case SEX_FEMALE:
				strcat(buf, titles[(int) ch->GetRace()][i].title_f);
				break;
			default:
				send_to_char("Oh dear.  You seem to be sexless.\n", ch);
				return;
		}
		strcat(buf, "\n");
	}
	page_string(ch->desc, buf);
}


ACMD(do_consider) {
	BUFFER(buf, MAX_STRING_LENGTH);
	CharData *victim;

	one_argument(argument, buf);

	if (!(victim = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
		send_to_char("Consider killing who?\n", ch);
	else if (victim == ch)
		send_to_char("Easy!  Very easy indeed!\n", ch);
//	else if (!IS_NPC(victim))
//		send_to_char("Would you like to borrow a cross and a shovel?\n", ch);
	else
	{
		int playerTier = (ch->GetLevel() + 9) / 10;	//	One tier less
		int victimTier;
		
		if (IS_NPC(victim))
			//	Below:	This is the new system that will go in soon for balancing against lower level mobs
			victimTier = (victim->GetLevel() / 10) + 1;	//	mob tiers are multiples of 10 starting at 0
		else
			victimTier = (victim->GetLevel() + 9) / 10;	//	player tiers are multiples of 10 starting at 1
		
		int diff = victimTier - playerTier;

		if (NO_STAFF_HASSLE(victim))	ch->Send("Why not jump in a death trap?  It's slower.\n");
		else if (IS_NPC(victim) && victim->GetLevel() == 100)
										ch->Send("You ARE mad!\n");
		else if (diff <= -4)			ch->Send("Now where did that chicken go?\n");
		else if (diff == -3)			ch->Send("You could do it with a needle!\n");
		else if (diff == -2)			ch->Send("Easy.\n");
		else if (diff == -1)			ch->Send("Fairly easy.\n");
		else if (diff == 0)				ch->Send("The perfect match!\n");
		else if (diff == 1)				ch->Send("You would need some luck!\n");
		else if (diff == 2)				ch->Send("You would need a lot of luck!\n");
		else if (diff == 3)				ch->Send("You would need a lot of luck and great equipment!\n");
		else if (diff == 4)				ch->Send("Do you feel lucky, punk?\n");
		else/*if(diff >= 5)*/			ch->Send("Are you mad!?\n");
	}
}



ACMD(do_diagnose) {
	BUFFER(buf, MAX_STRING_LENGTH);
	CharData *vict;

	one_argument(argument, buf);

	if (*buf) {
		if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
			send_to_char(NOPERSON, ch);
			return;
		} else
			diag_char_to_char(vict, ch);
	} else if (FIGHTING(ch)
#if ROAMING_COMBAT
		&& IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))
#endif
		)
		diag_char_to_char(FIGHTING(ch), ch);
	else
		send_to_char("Diagnose who?\n", ch);
}


ACMD(do_toggle) {
	if (IS_NPC(ch))
		return;

//	if (GET_WIMP_LEV(ch) == 0)	strcpy(buf, "OFF");
//	else						sprintf(buf, "%-3d", GET_WIMP_LEV(ch));

	ch->Send(
			"`y[`GGeneral config`y]`n\n"
			"     Brief Mode: %-3s    ""    Auto-follow: %-3s    ""   Compact Mode: %-3s\n"
			"          Color: %-3s    ""   Repeat Comm.: %-3s    ""     Show Exits: %-3s\n"
//			"    Compression: %-3s    ""  Auto-Compress: %-3s\n"
			"`y[`GChannels      `y]`n\n"
			"           Chat: %-3s    ""           Race: %-3s    ""           Clan: %-3s\n"
			"          Bitch: %-3s    ""          Music: %-3s    ""          Grats: %-3s\n"
			"           Info: %-3s    ""          Shout: %-3s    ""           Tell: %-3s\n"
			"        Mission: %-3s    ""         Newbie: %-3s\n"
//			"`y[`GGame Specifics`y]`n\n"
//			"     Wimp Level: %-3s\n"
			,

			ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),	ONOFF(!PRF_FLAGGED(ch, PRF_NOAUTOFOLLOW)),ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
			ONOFF(PRF_FLAGGED(ch, PRF_COLOR)),	YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),	ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
//			ONOFF(ch->desc->deflate),			YESNO(PRF_FLAGGED(ch, PRF_AUTOZLIB)),
			
			ONOFF(!CHN_FLAGGED(ch, Channel::NoChat)),ONOFF(!CHN_FLAGGED(ch, Channel::NoRace)),ONOFF(!CHN_FLAGGED(ch, Channel::NoClan)),
			ONOFF(!CHN_FLAGGED(ch, Channel::NoBitch)),ONOFF(!CHN_FLAGGED(ch, Channel::NoMusic)),ONOFF(!CHN_FLAGGED(ch, Channel::NoGratz)), 
			ONOFF(!CHN_FLAGGED(ch, Channel::NoInfo)),ONOFF(!CHN_FLAGGED(ch, Channel::NoShout)),	ONOFF(!CHN_FLAGGED(ch, Channel::NoTell)),
			YESNO(CHN_FLAGGED(ch, Channel::Mission)), ONOFF(!CHN_FLAGGED(ch, Channel::NoNewbie))
	);
}


struct sort_struct {
	int	sort_pos;
	Flags	type;
} *cmd_sort_info = NULL;

int num_of_cmds;

#define TYPE_CMD		(1 << 0)
#define TYPE_SOCIAL		(1 << 1)
#define TYPE_STAFFCMD	(1 << 2)

void sort_commands(void) {
	int a, b, tmp;

	num_of_cmds = 0;

	// first, count commands (num_of_commands is actually one greater than the
	//	number of commands; it inclues the '\n').
	while (*complete_cmd_info[num_of_cmds].command != '\n')
		num_of_cmds++;

	//	check if there was an old sort info.. then free it -- aedit -- M. Scott
	if (cmd_sort_info) free(cmd_sort_info);
	//	create data array
	CREATE(cmd_sort_info, struct sort_struct, num_of_cmds);

	//	initialize it
	for (a = 1; a < num_of_cmds; a++) {
		cmd_sort_info[a].sort_pos = a;
		if (complete_cmd_info[a].command_pointer == do_action)
			cmd_sort_info[a].type = TYPE_SOCIAL;
		if (IS_STAFFCMD(a) || (complete_cmd_info[a].minimum_level >= LVL_IMMORT))
			cmd_sort_info[a].type |= TYPE_STAFFCMD;
		if (!cmd_sort_info[a].type)
			cmd_sort_info[a].type = TYPE_CMD;
	}

	/* the infernal special case */
	cmd_sort_info[find_command("insult")].type = TYPE_SOCIAL;

	/* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
	for (a = 1; a < num_of_cmds - 1; a++) {
		for (b = a + 1; b < num_of_cmds; b++) {
			if (strcmp(complete_cmd_info[cmd_sort_info[a].sort_pos].command,
					complete_cmd_info[cmd_sort_info[b].sort_pos].command) > 0) {
				tmp = cmd_sort_info[a].sort_pos;
				cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
				cmd_sort_info[b].sort_pos = tmp;
			}
		}
	}
}


ACMD(do_commands) {
	BUFFER(buf, MAX_STRING_LENGTH);
	
	int no, i, cmd_num;
	int wizhelp = 0, socials = 0;
	CharData *vict;
	Flags	type;

	one_argument(argument, buf);

	if (*buf) {
		if (!(vict = get_char_vis(ch, buf, FIND_CHAR_WORLD)) || IS_NPC(vict)) {
			send_to_char("Who is that?\n", ch);
			return;
		}
		if (!IS_STAFF(ch) /*&& (ch->GetLevel() < vict->GetLevel())*/) {
			send_to_char("You can't see the commands of other people.\n", ch);
			return;
		}
	} else
		vict = ch;
	
	if (subcmd == SCMD_SOCIALS)			type = TYPE_SOCIAL;
	else if (subcmd == SCMD_WIZHELP)	type = TYPE_STAFFCMD;
	else								type = TYPE_CMD;

	sprintf(buf, "The following %s%s are available to %s:\n",
			(subcmd == SCMD_WIZHELP) ? "privileged " : "",
			(subcmd == SCMD_SOCIALS) ? "socials" : "commands",
			vict == ch ? "you" : vict->GetName());

	/* cmd_num starts at 1, not 0, to remove 'RESERVED' */
	for (no = 1, cmd_num = 1; cmd_num < num_of_cmds; cmd_num++) {
		i = cmd_sort_info[cmd_num].sort_pos;
		
		if (!IS_SET(cmd_sort_info[i].type, type))
			continue;
		
		if ((vict->GetLevel() < complete_cmd_info[i].minimum_level) /* && !IS_STAFF(vict) */)
			continue;
		
		if ((subcmd == SCMD_WIZHELP) && IS_STAFFCMD(i) && !STF_FLAGGED(vict, complete_cmd_info[i].staffcmd))
			continue;
		if ((subcmd == SCMD_WIZHELP) && !IS_STAFFCMD(i) && (complete_cmd_info[i].minimum_level < LVL_IMMORT))
			continue;
		
		sprintf_cat(buf, "%-11s", complete_cmd_info[i].command);
		if (!(no % 7))
			strcat(buf, "\n");
		no++;
	}
	
	strcat(buf, "\n");
	
	page_string(ch->desc, buf);
}


/*
                                     ['help score']
===================================================
Designation (Name): FearItself  Hitroll (HR):   150
      Species/Race: Hum         Damroll (DR):    50

HP: 1000        MV: 100         Strength:       200
AA: 1000        LV: 101         Intelligence:   200
PK: 596         PD: 101         Perception:     200
MK: 548         MD: 44          Agility:        200
                                Constitution:   200
Played Time: -4d 5h     

MP: 472   TNL: 9729   (Life Total: 102003307)

(+)Flags: D-H D-I Infra Sanct Sneak 
(-)Flags: 

(Position) You are standing.
---------------------------------------------------
*/
