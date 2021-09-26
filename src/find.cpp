/**************************************************************************
*  File: find.c                                                           *
*  Usage: contains functions for finding mobs and objs in lists           *
**************************************************************************/



#include "structs.h"
#include "utils.h"
#include "find.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "buffer.h"


int get_number(char *name) {
	int i;
	char *ppos;
	BUFFER(number, MAX_INPUT_LENGTH);

	if ((ppos = strchr(name, '.'))) {
		*ppos++ = '\0';
		strcpy(number, name);
		strcpy(name, ppos);

		for (i = 0; *(number + i); ++i)
		{
			if (!isdigit(*(number + i)))
				return 0;
		}
		
		i = atoi(number);
		return i;
	}
	return 1;
}


CharData *get_char(const char *name)
{
	if (IsIDString(name))
	{
		return CharData::Find(ParseIDString(name));
//		if (i && (i->GetPlayer()->m_StaffInvisLevel >= 103))
//			i = NULL;
	}
	else
	{
		FOREACH(CharacterList, character_list, ch)
		{
			if (silly_isname(name, (*ch)->GetAlias()) /*&& !(i->GetPlayer()->m_StaffInvisLevel >= 103)*/)
				return *ch;
		}
	}
	return NULL;
}


/* returns the object in the world with name name, or NULL if not found */
ObjData *get_obj(const char *name)
{
	if (IsIDString(name))
	{
		return ObjData::Find(ParseIDString(name));
	}
	else 
	{
		FOREACH(ObjectList, object_list, obj)
		{
			if (silly_isname(name, (*obj)->GetAlias()))
				return *obj;
		}
	}
	return NULL;
}


/* finds room by with name.  returns NULL if not found */
RoomData *get_room(const char *name, Hash zone) {
	if (IsIDString(name))		return RoomData::Find(ParseIDString(name));
	else						return world.Find(VirtualID(name, zone));
//	else if (IsVirtualID(name))	return world.Find(name);
//	return NULL;
}


CharData *get_player_vis(CharData *ch, const char *name, Flags where, int *number)
{
	
	if (IsIDString(name))
	{
		CharData *i = CharData::Find(ParseIDString(name));
		
		if (i
			&& !IS_NPC(i)						//	Not an NPC
			&& (where != FIND_CHAR_ROOM		//	and Not searching for same room
				|| IN_ROOM(i) == IN_ROOM(ch))	//		or they are in same room
			&& ch->CanSee(i, where != FIND_CHAR_ROOM) != VISIBLE_NONE)	//	and at least partially visible
			return i;
	}
	else
	{
		BUFFER(tmp, MAX_INPUT_LENGTH);
		strcpy(tmp, name);
		
		int num;
		if (!number)
		{
			number = &num;
			num = get_number(tmp);
		}
		
		FOREACH(CharacterList, character_list, iter)
		{
			CharData *i = *iter;
			if (IS_NPC(i))												continue;
			if (where == FIND_CHAR_ROOM && IN_ROOM(i) != IN_ROOM(ch))	continue;
			switch (ch->CanSee(i, where == FIND_CHAR_ROOM ? false : GloballyVisible))
			{
				case VISIBLE_NONE:		continue;
				case VISIBLE_SHIMMER:
					if (where == FIND_CHAR_ROOM && isname(tmp, SHIMMER_NAME))	return i;
				case VISIBLE_FULL:
					if (!isname(tmp, i->GetAlias()))		continue;
			}
			
			if (--(*number) == 0)
				return i;
		}
	}
	return NULL;
}


CharData *get_player(const char *name, int *number)
{
	
	if (IsIDString(name)) {
		CharData *i = CharData::Find(ParseIDString(name));
		if (i && !IS_NPC(i))
			return i;
	} else {
		BUFFER(tmp, MAX_INPUT_LENGTH);
		strcpy(tmp, name);
		
		int num;
		if (!number)
		{
			number = &num;
			num = get_number(tmp);
		}
		FOREACH(CharacterList, character_list, iter)
		{
			CharData *i = *iter;
			if (IS_NPC(i))									continue;
			if (!isname(tmp, i->GetAlias()))		continue;
			
			if (--(*number) == 0)
				return i;
		}
	}
	return NULL;
}


CharData *get_char_vis(CharData * ch, const char *name, Flags where, int *number) {
	if (where == FIND_CHAR_ROOM)		return get_char_room_vis(ch, name, number);
	else if (where == FIND_CHAR_WORLD)	return get_char_world_vis(ch, name, number);
	else								return NULL;
}


/* search a room for a char, and return a pointer if found..  */
CharData *get_char_room(const char *name, RoomData *room, int *number)
{
	if (IsIDString(name))
	{
		return CharData::Find(ParseIDString(name));
		//if (i && ((i->GetPlayer()->m_StaffInvisLevel >= 103) || (IN_ROOM(i) != room)))
		//	i = NULL;
	}
	else
	{
		BUFFER(tmp, MAX_INPUT_LENGTH);
		strcpy(tmp, name);
		
		int num;
		if (!number)
		{
			number = &num;
			num = get_number(tmp);
		}
		
		if (*number == 0)
			return get_player(tmp);
		else
		{
			FOREACH(CharacterList, room->people, iter)
			{
				if (/*(i->GetPlayer()->m_StaffInvisLevel < 103) &&*/ 
					isname(tmp, (*iter)->GetAlias()) && (--(*number) == 0))
					return *iter;
			}
		}
	}
	return NULL;
}


CharData *get_char_room_vis(CharData * ch, const char *name, int *number)
{
	if (IsIDString(name))
	{
		CharData *i = CharData::Find(ParseIDString(name));
		
		if (i									//	Valid
			&& (IN_ROOM(ch) == IN_ROOM(i))		//	In same room
			&& (ch->CanSee(i) != VISIBLE_NONE))	//	And not completely invisible
			return i;
	}
	else
	{
		if (!str_cmp(name, "self") || !str_cmp(name, "me"))
			return ch;

		BUFFER(tmp, MAX_INPUT_LENGTH);
		strcpy(tmp, name);
		
		int num;
		if (!number)
		{
			number = &num;
			num = get_number(tmp);
		}
		
		/* 0.<name> means PC with name */
		if (*number == 0)
		{
			return get_player_vis(ch, tmp, FIND_CHAR_ROOM);
		}
		else
		{
			FOREACH(CharacterList, ch->InRoom()->people, iter)
			{
				CharData *i = *iter;
				if (i == ch)	continue;
				
				switch (ch->CanSee(i))
				{
					case VISIBLE_NONE:		continue;
					case VISIBLE_SHIMMER:
						if (isname(tmp, SHIMMER_NAME))		break;
					case VISIBLE_FULL:
						if (!isname(tmp, i->GetAlias()))	continue;
				}
				if (--(*number) == 0)
					return i;
			}
		}
	}
	return NULL;
}


CharData *get_char_world_vis(CharData * ch, const char *name, int *number)
{
	if (IsIDString(name))
	{
		CharData *i = CharData::Find(ParseIDString(name));
		
		if (i && ch->CanSee(i) != VISIBLE_NONE)
			return i;
	} else {
		if (!str_cmp(name, "self") || !str_cmp(name, "me"))
			return ch;

		BUFFER(tmp, MAX_INPUT_LENGTH);
		strcpy(tmp, name);
		
		int num;	
		if (!number)
		{
			number = &num;
			num = get_number(tmp);
		}
		
		CharData *i = get_char_vis(ch, name, FIND_CHAR_ROOM, number);
		if (i)
			return i;
		else if (*number == 0)
			return get_player_vis(ch, tmp);
		else
		{
			FOREACH(CharacterList, character_list, iter)
			{
				i = *iter;
				if (i == ch)								continue;
				if (IN_ROOM(ch) == IN_ROOM(i))				continue;
				if (!isname(tmp, i->GetAlias()))			continue;
				if (ch->CanSee(i, GloballyVisible) == VISIBLE_NONE)	continue;
				
				if (--(*number) == 0)
					return i;
			}
		}
	}
	return NULL;
}


CharData *get_char_by_char(CharData * ch, const char *name)
{
	if (IsIDString(name))
	{
		return CharData::Find(ParseIDString(name));
	}
	else
	{
		if (!str_cmp(name, "self") || !str_cmp(name, "me"))
			return ch;

		BUFFER(tmp, MAX_INPUT_LENGTH);
		strcpy(tmp, name);
		
		int number = get_number(tmp);
		
		CharData *i = get_char_room(name, IN_ROOM(ch), &number);
		if (i)
			return i;
		else if (number == 0)
			return get_player(tmp);
		else
		{
			FOREACH(CharacterList, character_list, iter)
			{
				i = *iter;
				if (i == ch)						continue;
				if (IN_ROOM(ch) == IN_ROOM(i))		continue;
				if (!isname(tmp, i->GetAlias()))	continue;
				
				if (--number == 0)
					return i;
			}
		}
	}
	return NULL;
}



CharData *get_char_other_room_vis(CharData * ch, const char *name, RoomData *room, int *number)
{
	if (room == NULL)
		return NULL;
	
	if (IsIDString(name)) {
		CharData *i = CharData::Find(ParseIDString(name));
		
		if (i									//	Valid
			&& (room == IN_ROOM(i))				//	and in target room
			&& (ch->CanSee(i) != VISIBLE_NONE))	//	and visible
			return i;
	}
	else
	{
		if (!str_cmp(name, "self") || !str_cmp(name, "me"))
			return ch;

		BUFFER(tmp, MAX_INPUT_LENGTH);
		strcpy(tmp, name);

		int num;
		if (!number)
		{
			number = &num;
			num = get_number(tmp);
		}
		
		/* 0.<name> means PC with name */
		if (*number == 0)
		{
			return get_player_vis(ch, tmp, FIND_CHAR_ROOM);
		}
		else
		{
			FOREACH(CharacterList, room->people, iter)
			{
				CharData *i = *iter;
				if (i == ch)	continue;
				
				switch (ch->CanSee(i))
				{
					case VISIBLE_NONE:		continue;
					case VISIBLE_SHIMMER:
						if (isname(tmp, SHIMMER_NAME))		break;
					case VISIBLE_FULL:
						if (!isname(tmp, i->GetAlias()))	continue;
				}
				if (--(*number) == 0)
					return i;
			}
		}
	}
	return NULL;
}



CharData *get_char_by_obj(ObjData *obj, const char *name) {
	if (IsIDString(name)) {
		return CharData::Find(ParseIDString(name));
		//if (ch && (ch->GetPlayer()->m_StaffInvisLevel >= 103))
		//	ch = NULL;
	} else {
		if (obj->CarriedBy() && silly_isname(name, obj->CarriedBy()->GetAlias()) /*&&
				(obj->CarriedBy()->GetPlayer()->m_StaffInvisLevel < 103)*/)
			return obj->CarriedBy();
		else if (obj->WornBy() && silly_isname(name, obj->WornBy()->GetAlias()) /*&&
				(obj->WornBy()->GetPlayer()->m_StaffInvisLevel < 103)*/)
			return obj->WornBy();
		else
		{
			FOREACH(CharacterList, character_list, ch)
			{
				if (silly_isname(name, (*ch)->GetAlias()) /*&& (ch->GetPlayer()->m_StaffInvisLevel < 103)*/)
					return *ch;
			}
		}
	}
	return NULL;
}


CharData *get_char_by_room(RoomData *room, const char *name)
{	
	if (IsIDString(name))
	{
		return CharData::Find(ParseIDString(name));
		//if (ch && (ch->GetPlayer()->m_StaffInvisLevel >= 103))
		//	ch = NULL;
	}
	else
	{
		FOREACH(CharacterList, room->people, ch)
		{
			if (silly_isname(name, (*ch)->GetAlias()) /*&& (ch->GetPlayer()->m_StaffInvisLevel < 103)*/)
				return *ch;
		}
		FOREACH(CharacterList, character_list, ch)
		{
			if (silly_isname(name, (*ch)->GetAlias()) /*&& (ch->GetPlayer()->m_StaffInvisLevel < 103)*/)
				return *ch;
		}
	}
	return NULL;
}


/************************************************************
 * object searching routines
 ************************************************************/

/* search the entire world for an object, and return a pointer  */
ObjData *get_obj_vis(CharData * ch, const char *name, int *number)
{
	if (IsIDString(name))
	{
		ObjData *i = ObjData::Find(ParseIDString(name));
		if (i && ch->CanSee(i))
			return i;
	}
	else
	{
		int num;
		BUFFER(tmp, MAX_INPUT_LENGTH);
		strcpy(tmp, name);
		
		if (!number)
		{
			number = &num;
			num = get_number(tmp);
		}
		
		/* scan items carried */
		ObjData *i = get_obj_in_list_vis(ch, tmp, ch->carrying, number);
		if (i)	return i;
		
		int eq = 0;
		i = get_obj_in_equip_vis(ch, tmp, ch->equipment, &eq, number);
		if (i)	return i;
		
		i = get_obj_in_list_vis(ch, tmp, ch->InRoom()->contents, number);
		if (i)	return i;
		
		FOREACH(ObjectList, object_list, iter)
		{
			i = *iter;
			if (!ch->CanSee(i))									continue;
			if (i->CarriedBy() && ch->CanSee(i->CarriedBy()) == VISIBLE_NONE)	continue;
			if (!isname(tmp, i->GetAlias()))					continue;
			if (--(*number) == 0)
				return i;
		}
	}
	return NULL;
}



ObjData *get_obj_by_char(CharData * ch, const char *name)
{
	if (IsIDString(name))
	{
		return ObjData::Find(ParseIDString(name));
//		if (i && !ch->CanSee(i))
//			i = NULL;
	}
	else
	{
		BUFFER(tmp, MAX_INPUT_LENGTH);
		strcpy(tmp, name);
		
		int number = get_number(tmp);
		
		/* scan items carried */
		ObjData *i = get_obj_in_list(tmp, &number, ch->carrying);
		if (i)	return i;
		
		int eq = 0;
		i = get_obj_in_equip(tmp, &number, ch->equipment, &eq);
		if (i)	return i;
		
		i = get_obj_in_list(tmp, &number, ch->InRoom()->contents);
		if (i)	return i;
		
		FOREACH(ObjectList, object_list, iter)
		{
			i = *iter;
			if (isname(tmp, i->GetAlias()) && --number == 0)
				return i;
		}
	}
	return NULL;
}



ObjData *get_obj_in_equip(const char *arg, int *number, ObjData * equipment[], int *j) {
	if (IsIDString(arg))
	{
		IDNum id = ParseIDString(arg);
		for ((*j) = 0; (*j) < NUM_WEARS; (*j)++)
		{
			ObjData *obj = equipment[(*j)];
			if (obj && (id == GET_ID(obj)))
				return obj;
		}
	}
	else
	{
		BUFFER(tmp, MAX_INPUT_LENGTH);
		strcpy(tmp, arg);
		
		int num;
		if (!number)
		{
			number = &num;
			num = get_number(tmp);
		}
		
		for ((*j) = 0; (*j) < NUM_WEARS; (*j)++) {
			ObjData *obj = equipment[(*j)];
			
			if (!obj)							continue;
			if (!isname(tmp, obj->GetAlias()))	continue;
			if (--(*number) == 0)
				return obj;
		}
	}
	return NULL;
}


ObjData *get_obj_in_equip_type(int type, ObjData * equipment[])
{
	for (int i = 0; i < NUM_WEARS; ++i)
	{
		ObjData *obj = equipment[i];
		
		if (obj && obj->GetType() == type)
			return obj;
	}
	
	return NULL;
}


ObjData *get_obj_in_equip_vis(CharData * ch, const char *arg, ObjData * equipment[], int *j, int *number) {
	long id;
	int num;
	
	if (IsIDString(arg)) {
		id = ParseIDString(arg);
		for ((*j) = 0; (*j) < NUM_WEARS; (*j)++)
		{
			ObjData *obj = equipment[(*j)];
			if (obj && (id == GET_ID(obj)) && ch->CanSee(obj))
				return obj;
		}
	} else {
		BUFFER(tmp, MAX_INPUT_LENGTH);
		
		strcpy(tmp, arg);
		
		if (!number)
		{
			number = &num;
			num = get_number(tmp);
		}
		
		for ((*j) = 0; (*j) < NUM_WEARS; (*j)++) {
			ObjData *obj = equipment[(*j)];
			
			if (!obj)							continue;
			if (!ch->CanSee(obj))				continue;
			if (!isname(tmp, obj->GetAlias()))	continue;
			if (--(*number) == 0)
				return obj;
		}
	}
	return NULL;
}


ObjData *get_obj_in_list(const char *name, int *number, ObjectList &list)
{
	if (IsIDString(name))
	{
		IDNum id = ParseIDString(name);
		FOREACH(ObjectList, list, iter)
		{
			if (id == GET_ID(*iter))
				*iter;
		}
	}
	else
	{
		BUFFER(tmp, MAX_INPUT_LENGTH);
		strcpy(tmp, name);
		
		int num;
		if (!number)
		{
			number = &num;
			num = get_number(tmp);
		}
		
		FOREACH(ObjectList, list, iter)
		{
			if (isname(tmp, (*iter)->GetAlias()) && --(*number) == 0)
				return *iter;
		}
	}
	return NULL;
}


ObjData *get_obj_in_list_vis(CharData * ch, const char *name, ObjectList &list, int *number)
{
	if (IsIDString(name))
	{
		IDNum id = ParseIDString(name);
		FOREACH(ObjectList, list, i)
		{
			if ((id == GET_ID(*i)) && ch->CanSee(*i))
				return *i;
		}
	}
	else
	{
		BUFFER(tmp, MAX_INPUT_LENGTH);	
		strcpy(tmp, name);
		
		int num;
		if (!number)
		{
			number = &num;
			num = get_number(tmp);
		}
		
		FOREACH(ObjectList, list, i)
		{
			if (ch->CanSee(*i) && isname(tmp, (*i)->GetAlias()) && --(*number) == 0)
				return *i;
		}
	}
	return NULL;
}


/* Search the given list for an object type, and return a ptr to that obj */
ObjData *get_obj_in_list_type(int type, ObjectList &list)
{
	FOREACH(ObjectList, list, i)
	{
		if ((*i)->GetType() == type)
			return *i;
	}
	return NULL;
}


/* Search the given list for an object that has 'flag' set, and return a ptr to that obj */
ObjData *get_obj_in_list_flagged(int flag, ObjectList &list)
{
	FOREACH(ObjectList, list, i)
	{
		if (OBJ_FLAGGED(*i, flag))
			return *i;
	}
	return NULL;
}


ObjData *get_obj_by_obj(ObjData *obj, const char *name) {
	ObjData *i = NULL;
	int j = 0;
	RoomData *rm;
	
	if (IsIDString(name))
		return ObjData::Find(ParseIDString(name));
		
	if (!str_cmp(name, "self") || !str_cmp(name, "me"))
		return obj;

	if ((i = get_obj_in_list(name, NULL, obj->contents)))
		return i;
	
	if (obj->Inside())
	{
		if (silly_isname(name, obj->Inside()->GetAlias()))
			return obj->Inside();
	}
	else if (obj->WornBy() && (i = get_obj_in_equip(name, NULL, obj->WornBy()->equipment, &j)))
		return i;
	else if (obj->CarriedBy() && (i = get_obj_in_list(name, NULL, obj->CarriedBy()->carrying)))
		return i;
	else if ((rm = obj->Room()) && (i = get_obj_in_list(name, NULL, rm->contents)))
		return i;
		
	FOREACH(ObjectList, object_list, iter)
	{
		if (silly_isname(name, (*iter)->GetAlias()))
			return *iter;
	}
		
	return NULL;
}


ObjData *get_obj_by_room(RoomData *room, const char *name)
{
	if (IsIDString(name))
	{
		IDNum id = ParseIDString(name);
	
		FOREACH(ObjectList, room->contents, obj)
		{
			if (id == GET_ID(*obj))
				return *obj;
		}
		
		return ObjData::Find(id);
	}
	else
	{
		FOREACH(ObjectList, room->contents, obj)
		{
			if (silly_isname(name, (*obj)->GetAlias()))
				return *obj;
		}
		
		FOREACH(ObjectList, object_list, obj)
		{
			if (silly_isname(name, (*obj)->GetAlias()))
				return *obj;
		}
	}
	return NULL;
}


/************************************************************
 * search by number routines
 ************************************************************/

/* search all over the world for a char num, and return a pointer if found */
CharData *get_char_num(VirtualID nr)
{
	FOREACH(CharacterList, character_list, i)
	{
		if ((*i)->GetVirtualID() == nr)
			return *i;
	}
	
	return NULL;
}


/* search the entire world for an object number, and return a pointer  */
ObjData *get_obj_num(VirtualID nr)
{
	FOREACH(ObjectList, object_list, i)
	{
		if ((*i)->GetVirtualID() == nr)
			return *i;
	}

	return NULL;
}


/* Search a given list for an object number, and return a ptr to that obj */
ObjData *get_obj_in_list_num(VirtualID num, ObjectList &list)
{
	FOREACH(ObjectList, list, i)
	{
		if ((*i)->GetVirtualID() == num)
			return *i;
	}

	return NULL;
}



/* Generic Find, designed to find any object/character                    */
/* Calling :                                                              */
/*  *arg     is the sting containing the string to be searched for.       */
/*           This string doesn't have to be a single word, the routine    */
/*           extracts the next word itself.                               */
/*  bitv..   All those bits that you want to "search through".            */
/*           Bit found will be result of the function                     */
/*  *ch      This is the person that is trying to "find"                  */
/*  **tar_ch Will be NULL if no character was found, otherwise points     */
/* **tar_obj Will be NULL if no object was found, otherwise points        */
/*                                                                        */
/* The routine returns a pointer to the next word in *arg (just like the  */
/* one_argument routine).                                                 */

int generic_find(const char *arg, int bitvector, CharData * ch,
		     CharData ** tar_ch, ObjData ** tar_obj)
{
	BUFFER(name, MAX_INPUT_LENGTH);
	
	if (tar_ch)		*tar_ch = NULL;
	if (tar_obj)	*tar_obj = NULL;

	one_argument(arg, name);

	if (!*name)			return 0;
	int number = get_number(name);
	if (number == 0)	return 0;
	
	if (IS_SET(bitvector, FIND_CHAR_ROOM))	/* Find person in room */
	{
		if (tar_ch && (*tar_ch = get_char_vis(ch, name, FIND_CHAR_ROOM, &number)))
			return (FIND_CHAR_ROOM);
	}
	if (IS_SET(bitvector, FIND_CHAR_WORLD))
	{
		if (tar_ch && (*tar_ch = get_char_vis(ch, name, FIND_CHAR_WORLD, &number)))
			return (FIND_CHAR_WORLD);
	}
	if (IS_SET(bitvector, FIND_OBJ_EQUIP))
	{
//		for (int i = 0; i < NUM_WEARS; i++)
//		{
//			if (GET_EQ(ch, i) && isname(name, GET_EQ(ch, i)->GetAlias()) && (--number == 0))
//			{
//				if (tar_obj)	*tar_obj = GET_EQ(ch, i);
//				return (FIND_OBJ_EQUIP);
//			}
//		}
		int pos;
		if (tar_obj && (*tar_obj = get_obj_in_equip_vis(ch, name, ch->equipment, &pos, &number)))
			return (FIND_OBJ_EQUIP);
	}
	if (IS_SET(bitvector, FIND_OBJ_INV))
	{
		if (tar_obj && (*tar_obj = get_obj_in_list_vis(ch, name, ch->carrying, &number)))
			return (FIND_OBJ_INV);
	}
	if (IS_SET(bitvector, FIND_OBJ_ROOM))
	{
		if (tar_obj && (*tar_obj = get_obj_in_list_vis(ch, name, ch->InRoom()->contents, &number)))
			return (FIND_OBJ_ROOM);
	}
	if (IS_SET(bitvector, FIND_OBJ_WORLD))
	{
		if (tar_obj && (*tar_obj = get_obj_vis(ch, name, &number)))
			return (FIND_OBJ_WORLD);
	}
	return (0);
}


/* a function to scan for "all" or "all.x" */
int find_all_dots(char *arg) {
	if (!strcmp(arg, "all"))
		return FIND_ALL;
	else if (!strncmp(arg, "all.", 4)) {
		strcpy(arg, arg + 4);
		return FIND_ALLDOT;
	} else
		return FIND_INDIV;
}



int	get_num_chars_on_obj(ObjData * obj)
{
	int temp = 0;
	
	FOREACH(CharacterList, obj->InRoom()->people, ch)
	{
		if (obj != (*ch)->sitting_on)		continue;
		if (GET_POS(*ch) <= POS_SITTING)	++temp;
		else								(*ch)->sitting_on = NULL;
	}
	return temp;
}


CharData *get_char_on_obj(ObjData *obj)
{
	FOREACH(CharacterList, obj->InRoom()->people, ch)
	{
		if(obj == (*ch)->sitting_on)
			return *ch;
	}
	return NULL;
}


int CountSpawnObjectsInList(ObjectPrototype *proto, ObjectList &list)
{
	int counter = 0;
	FOREACH(ObjectList, list, obj)
	{
		if ((*obj)->GetPrototype() == proto)
			++counter;
	}
	return counter;
}


int CountSpawnMobsInList(CharacterPrototype *proto, CharacterList &list)
{
	int counter = 0;
	FOREACH(CharacterList, list, iter)
	{
		CharData *ch = *iter;
		if (IS_NPC(ch) && ch->GetPrototype() == proto)
			++counter;
	}
	return counter;
}


/* Find a vehicle by VNUM */
ObjData *find_vehicle_by_vid(VirtualID vid)
{
	FOREACH(ObjectList, object_list, obj)
	{
		if ((*obj)->GetVirtualID() == vid)
			return *obj;
	}
	return NULL;
}


const char *ExtraDesc::List::Find(const char *key)
{
	FOREACH(ExtraDesc::List, (*this), i)
	{
		if (isname(key, i->keyword.c_str()))
			return i->description.c_str();
	}
	
	return NULL;
}
