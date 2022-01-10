/* ************************************************************************
*   File: act.item.c                                    Part of CircleMUD *
*  Usage: object handling routines -- get/drop and container handling     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "characters.h"
#include "rooms.h"
#include "objects.h"
#include "descriptors.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "db.h"
#include "spells.h"
#include "event.h"
#include "boards.h"
#include "affects.h"
#include "clans.h"
#include "constants.h"
#include "house.h"

/* extern variables */

int drop_otrigger(ObjData *obj, CharData *actor);
int put_otrigger(ObjData *obj, ObjData *container, CharData *actor);
int retrieve_otrigger(ObjData *obj, ObjData *container, CharData *actor);
int drop_wtrigger(ObjData *obj, CharData *actor);
int get_otrigger(ObjData *obj, CharData *actor);
int give_otrigger(ObjData *obj, CharData *actor, CharData *victim);
int receive_mtrigger(CharData *ch, CharData *actor, ObjData *obj);
int wear_otrigger(ObjData *obj, CharData *actor, int where);
int remove_otrigger(ObjData *obj, CharData *actor, int where);
int consume_otrigger(ObjData *obj, CharData *actor);

int wear_message_number (ObjData *obj, WearPosition where);
void combat_reset_timer(CharData *ch);


/* functions */
int can_take_obj(CharData * ch, ObjData * obj);
void get_check_money(CharData * ch, ObjData * obj);
int perform_get_from_room(CharData * ch, ObjData * obj);
void get_from_room(CharData * ch, char *arg);
CharData *give_find_vict(CharData * ch, char *arg);
void weight_change_object(ObjData * obj, int weight);
void name_from_drinkcon(ObjData * obj);
void name_to_drinkcon(ObjData * obj, int type);
void wear_message(CharData * ch, ObjData * obj, WearPosition where);
void perform_wear(CharData * ch, ObjData * obj, int where);
int find_eq_pos(CharData * ch, ObjData * obj, char *arg);
void perform_remove(CharData * ch, int pos);
void perform_put(CharData * ch, ObjData * obj, ObjData * cont);
void perform_get_from_container(CharData * ch, ObjData * obj, ObjData * cont, int mode);
int perform_drop(CharData * ch, ObjData * obj, RoomData *room);
void perform_give(CharData * ch, CharData * vict, ObjData * obj);
void get_from_container(CharData * ch, ObjData * cont, char *arg, int mode);
bool can_hold(ObjData *obj);
ACMD(do_put);
ACMD(do_get);
ACMD(do_drop);
ACMD(do_give);
ACMD(do_drink);
ACMD(do_eat);
ACMD(do_pour);
ACMD(do_wear);
ACMD(do_wield);
ACMD(do_grab);
ACMD(do_remove);
ACMD(do_pull);
ACMD(do_prime);
ACMD(do_reload);

void load_otrigger(ObjData *obj);

void perform_put(CharData * ch, ObjData * obj, ObjData * cont) {
	if (!drop_otrigger(obj, ch))
		return;
	else if (!put_otrigger(obj, cont, ch))
		return;
	else if (OBJ_FLAGGED(cont, ITEM_MISSION) && cont->InRoom() && cont->owner != GET_ID(ch))
		act("You can't put anything in $p, because it does not belong to you.", FALSE, ch, cont, NULL, TO_CHAR);
	else if ((GET_OBJ_TOTAL_WEIGHT(cont) - GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj)) > cont->GetValue(OBJVAL_CONTAINER_CAPACITY))
		act("$p won't fit in $P.", FALSE, ch, obj, cont, TO_CHAR);
	else if (OBJ_FLAGGED(obj, ITEM_MISSION) && !OBJ_FLAGGED(cont, ITEM_MISSION))
		ch->Send("You can't put Mission Equipment into containers that are not also Mission Equipment.\n");
	else if (OBJ_FLAGGED(obj, ITEM_NOPUT))
		act("You can't put $p into a container.\n", FALSE, ch, obj, NULL, TO_CHAR);
	else {
		obj->FromChar();
		obj->ToObject(cont);
		act("You put $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
		act("$n puts $p in $P.", TRUE, ch, obj, cont, TO_ROOM);
	}
}


/* The following put modes are supported by the code below:

	1) put <object> <container>
	2) put all.<object> <container>
	3) put all <container>

	<container> must be in inventory or on ground.
	all objects to be put into container must be in inventory.
*/

ACMD(do_put) {
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	ObjData *obj, *cont;
	int obj_dotmode, cont_dotmode, found = 0;

  two_arguments(argument, arg1, arg2);
  obj_dotmode = find_all_dots(arg1);
  cont_dotmode = find_all_dots(arg2);

  if (!*arg1)
    send_to_char("Put what in what?\n", ch);
  else if (cont_dotmode != FIND_INDIV)
    send_to_char("You can only put things into one container at a time.\n", ch);
  else if (!*arg2) {
    ch->Send("What do you want to put %s in?\n",
	    ((obj_dotmode == FIND_INDIV) ? "it" : "them"));
  } else {
    generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, NULL, &cont);
    if (!cont) {
      ch->Send("You don't see %s %s here.\n", AN(arg2), arg2);
    } else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
      act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
	else if (IS_SET(cont->GetValue(OBJVAL_CONTAINER_FLAGS), CONT_CLOSED))
      send_to_char("You'd better open it first!\n", ch);
    else {
      if (obj_dotmode == FIND_INDIV) {	/* put <obj> <container> */
	if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
	  ch->Send("You aren't carrying %s %s.\n", AN(arg1), arg1);
	} else if (obj == cont)
	  send_to_char("You attempt to fold it into itself, but fail.\n", ch);
	else
	  perform_put(ch, obj, cont);
      } else {
	FOREACH(ObjectList, ch->carrying, iter) {
		obj = *iter;
	  if ((obj != cont) && ch->CanSee(obj) &&
	      (obj_dotmode == FIND_ALL || isname(arg1, obj->GetAlias()))) {
	    found = 1;
	    perform_put(ch, obj, cont);
	  }
	}
	if (!found) {
	  if (obj_dotmode == FIND_ALL)
	    send_to_char("You don't seem to have anything to put in it.\n", ch);
	  else {
	    ch->Send("You don't seem to have any %ss.\n", arg1);
	  }
	}
      }
    }
  }
}



int can_take_obj(CharData * ch, ObjData * obj) {
	if (!IS_STAFF(ch) && IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
		act("$p: you can't carry that many items.", FALSE, ch, obj, 0, TO_CHAR);
	else if (!IS_STAFF(ch) && (IS_CARRYING_W(ch) + GET_OBJ_TOTAL_WEIGHT(obj)) > CAN_CARRY_W(ch))
		act("$p: you can't carry that much weight.", FALSE, ch, obj, 0, TO_CHAR);
	else if (!IS_STAFF(ch) && !(CAN_WEAR(obj, ITEM_WEAR_TAKE)))
		act("$p: you can't take that!", FALSE, ch, obj, 0, TO_CHAR);
	else
	{
		switch (ch->CanUse(obj))
		{
			case NotRestricted:
				return 1;
			
			case RestrictedByRace:
				act("$p: you can't figure what it does.", FALSE, ch, obj, 0, TO_CHAR);
				break;
			case RestrictedByLevel:
			{
				BUFFER(buf, MAX_STRING_LENGTH);
				sprintf(buf, "$p: you must be level %d to use it.", obj->GetMinimumLevelRestriction());
				act(buf, FALSE, ch, obj, 0, TO_CHAR);
				break;
			}
			case RestrictedByClan:
			{
				BUFFER(buf, MAX_STRING_LENGTH);
				Clan *clan = Clan::GetClan(obj->GetClanRestriction());
				sprintf(buf, "$p: you must be in %s to use it.", clan ? clan->GetName() : "<INVALID CLAN>");
				act(buf, FALSE, ch, obj, 0, TO_CHAR);
				break;
			}
			default:
				act("$p: you can't figure what it does.", FALSE, ch, obj, 0, TO_CHAR);
		}
	}
	
	return 0;
}


void perform_get_from_container(CharData * ch, ObjData * obj,
				     ObjData * cont, int mode)
{
	if (mode == FIND_OBJ_INV || can_take_obj(ch, obj)) {
		ObjData *in_obj = obj->Inside();
	
		if (cont->InRoom() && OBJ_FLAGGED(cont, ITEM_MISSION) && cont->owner != GET_ID(ch) && !NO_STAFF_HASSLE(ch))
			act("You can't take anything from $p, as $p does not belong to you.", FALSE, ch, cont, 0, TO_CHAR);
		else if (!NO_STAFF_HASSLE(ch) && IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
			act("$p: you can't hold any more items.", FALSE, ch, obj, 0, TO_CHAR);
		else if (get_otrigger(obj, ch) && retrieve_otrigger(obj, cont, ch) && obj->Inside() == cont) {
			obj->FromObject();
			obj->ToChar(ch);
			act("You get $p from $P.", FALSE, ch, obj, cont, TO_CHAR);
			act("$n gets $p from $P.", TRUE, ch, obj, cont, TO_ROOM);
/*			get_check_money(ch, obj); */
		}
	}
}


void get_from_container(CharData * ch, ObjData * cont,
			     char *arg, int mode) {
	ObjData *obj;
	int obj_dotmode, found = 0;

	obj_dotmode = find_all_dots(arg);

	if (IS_SET(cont->GetValue(OBJVAL_CONTAINER_FLAGS), CONT_CLOSED))
		act("$p is closed.", FALSE, ch, cont, 0, TO_CHAR);
	else if (obj_dotmode == FIND_INDIV) {
		if (!(obj = get_obj_in_list_vis(ch, arg, cont->contents))) {
			BUFFER(buf, MAX_STRING_LENGTH);
			sprintf(buf, "There doesn't seem to be %s %s in $p.", AN(arg), arg);
			act(buf, FALSE, ch, cont, 0, TO_CHAR);
		} else
			perform_get_from_container(ch, obj, cont, mode);
	} else {
		if (obj_dotmode == FIND_ALLDOT && !*arg) {
			send_to_char("Get all of what?\n", ch);
			return;
		}
		FOREACH(ObjectList, cont->contents, iter) {
			obj = *iter;
			if (ch->CanSee(obj) &&
					(obj_dotmode == FIND_ALL || isname(arg, obj->GetAlias()))) {
				found = 1;
				perform_get_from_container(ch, obj, cont, mode);
			}
		}
		if (!found) {
			if (obj_dotmode == FIND_ALL)
				act("$p seems to be empty.", FALSE, ch, cont, 0, TO_CHAR);
			else {
				BUFFER(buf, MAX_STRING_LENGTH);
				sprintf(buf, "You can't seem to find any %ss in $p.", arg);
				act(buf, FALSE, ch, cont, 0, TO_CHAR);
			}
		}
	}
}


int perform_get_from_room(CharData * ch, ObjData * obj)
{
	RoomData *room = obj->InRoom();
	
	if (OBJ_FLAGGED(obj, ITEM_MISSION) && obj->owner != GET_ID(ch) && !NO_STAFF_HASSLE(ch))
		act("You can't take $p because it does not belong to you.", FALSE, ch, obj, 0, TO_CHAR);
	else if (can_take_obj(ch, obj) && get_otrigger(obj, ch) && obj->InRoom() == room) {
		obj->FromRoom();
		obj->ToChar(ch);
		act("You get $p.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n gets $p.", TRUE, ch, obj, 0, TO_ROOM);
/*		get_check_money(ch, obj);*/
		return 1;
	}
	return 0;
}


void get_from_room(CharData * ch, char *arg) {
	ObjData *obj;
	int dotmode, found = 0;

	dotmode = find_all_dots(arg);

	if (dotmode == FIND_INDIV) {
		if (!(obj = get_obj_in_list_vis(ch, arg, ch->InRoom()->contents))) {
			ch->Send("You don't see %s %s here.\n", AN(arg), arg);
		} else
			perform_get_from_room(ch, obj);
	} else {
		if (dotmode == FIND_ALLDOT && !*arg) {
			send_to_char("Get all of what?\n", ch);
			return;
		}
		FOREACH(ObjectList, ch->InRoom()->contents, iter) {
			obj = *iter;
			if (ch->CanSee(obj) &&
					(dotmode == FIND_ALL || isname(arg, obj->GetAlias()))) {
				found = 1;
				perform_get_from_room(ch, obj);
			}
		}
		if (!found) {
			if (dotmode == FIND_ALL)
				send_to_char("There doesn't seem to be anything here.\n", ch);
			else {
				act("You don't see any $ts here.\n", FALSE, ch, (ObjData *)arg, 0, TO_CHAR);
			}
		}
	}
}



ACMD(do_get) {
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);

	int cont_dotmode, found = 0, mode;
	ObjData *cont;

	two_arguments(argument, arg1, arg2);

	if (!IS_STAFF(ch) && IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
		send_to_char("Your arms are already full!\n", ch);
	else if (!*arg1)
		send_to_char("Get what?\n", ch);
	else if (!*arg2)
		get_from_room(ch, arg1);
	else {
		cont_dotmode = find_all_dots(arg2);
		if (cont_dotmode == FIND_INDIV) {
			mode = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, NULL, &cont);
			if (!cont)
				ch->Send("You don't have %s %s.\n", AN(arg2), arg2);
			else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER)
				act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
			else
				get_from_container(ch, cont, arg1, mode);
		} else if (cont_dotmode == FIND_ALLDOT && !*arg2) {
				send_to_char("Get from all of what?\n", ch);
		} else {
			FOREACH(ObjectList, ch->carrying, iter) {
				cont = *iter;
				if (ch->CanSee(cont) && (cont_dotmode == FIND_ALL || isname(arg2, cont->GetAlias()))) {
					if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER) {
						found = 1;
						get_from_container(ch, cont, arg1, FIND_OBJ_INV);
					} else if (cont_dotmode == FIND_ALLDOT) {
						found = 1;
						act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
					}
				}
			}
			
			FOREACH(ObjectList, ch->InRoom()->contents, iter) {
				cont = *iter;
				if (ch->CanSee(cont) &&
						(cont_dotmode == FIND_ALL || isname(arg2, cont->GetAlias())))
					if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER) {
						get_from_container(ch, cont, arg1, FIND_OBJ_ROOM);
						found = 1;
					} else if (cont_dotmode == FIND_ALLDOT) {
						act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
						found = 1;
					}
			}
								
			if (!found) {
				if (cont_dotmode == FIND_ALL)
					send_to_char("You can't seem to find any containers.\n", ch);
				else
					ch->Send("You can't seem to find any %ss here.\n", arg2);
			}
		}
	}
}


int perform_drop(CharData * ch, ObjData * obj, RoomData *room) {
  if (OBJ_FLAGGED(obj, ITEM_NODROP))
  {
    act("You can't drop $p!", FALSE, ch, obj, 0, TO_CHAR);
    return 0;
  }
  if (OBJ_FLAGGED(obj, ITEM_MISSION)) {
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
    {
    	House *house = House::GetHouseForRoom(ch->InRoom());
    	if (!house || house->GetOwner() != ch->GetID())
    	{
	    	act("You can't drop $p here, as this is not your house.", FALSE, ch, obj, 0, TO_CHAR);
	    	return 0;
    	}
    	
    	obj->owner = ch->GetID();
    }
    else
    {
    	act("You can't drop $p, it is Mission Equipment.", FALSE, ch, obj, 0, TO_CHAR);
    	return 0;
    }
  }
  if (!drop_otrigger(obj, ch))
  {
  	return 0;
  }
  if (!drop_wtrigger(obj, ch))
  {
  	return 0;
  }
  
  act("You drop $p.", FALSE, ch, obj, 0, TO_CHAR);
  act("$n drops $p.", TRUE, ch, obj, 0, TO_ROOM);
  
  obj->FromChar();

	if (OBJ_FLAGGED(obj, ITEM_MISSION | ITEM_LOG) && IS_STAFF(ch))
		mudlogf(NRM, 103, TRUE, "(GC-DROP) %s drops %s in room %s.", ch->GetName(), obj->GetName(), ch->InRoom()->GetVirtualID().Print().c_str());  	
	obj->ToRoom(IN_ROOM(ch));
	return 0;
}



ACMD(do_drop) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	ObjData		*obj;
	RoomData *	room = NULL;
	int			dotmode, amount = 0;

	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOLITTER))
	{
	
		ch->Send("You shouldn't litter so much!\n");
		return;
	}

	argument = one_argument(argument, arg);

	if (!*arg)
		ch->Send("What do you want to drop?\n");
	else if (is_number(arg)) {
		amount = atoi(arg);
		argument = one_argument(argument, arg);
//		if (!str_cmp("coins", arg) || !str_cmp("coin", arg))
//			perform_drop_gold(ch, amount, mode, RDR);
//		else {
			//	code to drop multiple items.  anyone want to write it? -je
			send_to_char("Sorry, you can't do that to more than one item at a time.\n", ch);
//		}
	} else {
		dotmode = find_all_dots(arg);

		/* Can't junk or donate all */
		if (dotmode == FIND_ALL) {
			if (ch->carrying.empty())
				send_to_char("You don't seem to be carrying anything.\n", ch);
			else {
				FOREACH(ObjectList, ch->carrying, iter) {
					obj = *iter;
					if (ch->CanSee(obj))
						amount += perform_drop(ch, obj, room);
				}
			}
		} else if (dotmode == FIND_ALLDOT) {
			if (!*arg)
				ch->Send("What do you want to drop all of?\n");
			else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
				ch->Send("You don't seem to have any %ss.\n", arg);
			else {
				FOREACH(ObjectList, ch->carrying, iter) {
					obj = *iter;
					if (ch->CanSee(obj) && isname(arg, obj->GetAlias()))
						amount += perform_drop(ch, obj, room);
				}
			}
		} else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
			ch->Send("You don't seem to have %s %s.\n", AN(arg), arg);
		else
			amount += perform_drop(ch, obj, room);
	}
}


void perform_give(CharData * ch, CharData * vict,  ObjData * obj) {
	if (!IS_STAFF(ch) && OBJ_FLAGGED(obj, ITEM_NODROP))
		act("You can't let go of $p!!  Yeech!", FALSE, ch, obj, 0, TO_CHAR);
//	else if (OBJ_FLAGGED(obj, ITEM_MISSION) && !IS_STAFF(ch) && !IS_STAFF(vict))
//		act("You can't give $p away, it is Mission Equipment.", FALSE, ch, obj, vict, TO_CHAR);
  	else if (!IS_STAFF(ch) && !IS_NPC(vict) && vict->desc == NULL)
		act("$N is linkdead, and cannot receive anything.", FALSE, ch, 0, vict, TO_CHAR);
  	else if (!IS_STAFF(ch) && !IS_STAFF(vict) && IS_CARRYING_N(vict) >= CAN_CARRY_N(vict))
		act("$N seems to have $S hands full.", FALSE, ch, 0, vict, TO_CHAR);
	else if (!IS_STAFF(ch) && !IS_STAFF(vict) && GET_OBJ_TOTAL_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict))
		act("$E can't carry that much weight.", FALSE, ch, 0, vict, TO_CHAR);
	else
	{
		RestrictionType restriction = vict->CanUse(obj);
	
		if (restriction == RestrictedByLevel)
		{
			BUFFER(buf, MAX_STRING_LENGTH);
			sprintf(buf, "$E must be level %d to use $p, so you keep it.", obj->GetMinimumLevelRestriction());
			act(buf, FALSE, ch, obj, vict, TO_CHAR);
		}
		else if (restriction != NotRestricted)
			act("$E can't figure what $p does, so you keep it.", FALSE, ch, obj, vict, TO_CHAR);
		else if (give_otrigger(obj, ch, vict) && receive_mtrigger(vict, ch, obj)) {
			if ((OBJ_FLAGGED(obj, ITEM_MISSION | ITEM_LOG) && (!IS_STAFF(ch) || !IS_STAFF(vict)))
					|| ((IS_STAFF(ch) || IS_STAFF(vict)) && (!IS_STAFF(ch) || !IS_STAFF(vict))))
				mudlogf(NRM, 103, TRUE, "(GC-GIVE) %s gives %s to %s.", ch->GetName(), obj->GetName(), vict->GetName());  	
			obj->FromChar();
			obj->ToChar(vict);
			act("You give $p to $N.", FALSE, ch, obj, vict, TO_CHAR);
			act("$n gives you $p.", FALSE, ch, obj, vict, TO_VICT);
			act("$n gives $p to $N.", TRUE, ch, obj, vict, TO_NOTVICT);
		}
	}
}


/* utility function for give */
CharData *give_find_vict(CharData * ch, char *arg)
{
  CharData *vict;

  if (!*arg) {
    send_to_char("To who?\n", ch);
    return NULL;
  } else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_ROOM))) {
    send_to_char(NOPERSON, ch);
    return NULL;
  } else if (vict == ch) {
    send_to_char("What's the point of that?\n", ch);
    return NULL;
  } else
    return vict;
}


ACMD(do_give) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	BUFFER(buf1, MAX_INPUT_LENGTH);
	int amount, dotmode;
	CharData *vict;
	ObjData *obj;

	argument = one_argument(argument, arg);

	if (!*arg)
		send_to_char("Give what to who?\n", ch);
	else if (is_number(arg)) {
		amount = atoi(arg);
		argument = one_argument(argument, arg);
	/* code to give multiple items.  anyone want to write it? -je */
		send_to_char("You can't give more than one item at a time.\n", ch);
	} else {
		one_argument(argument, buf1);
		if ((vict = give_find_vict(ch, buf1))) {
			dotmode = find_all_dots(arg);
			if (dotmode == FIND_INDIV) {
				if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
					ch->Send("You don't seem to have %s %s.\n", AN(arg), arg);
				else
					perform_give(ch, vict, obj);
			} else if ((dotmode == FIND_ALLDOT) && !*arg)
				send_to_char("All of what?\n", ch);
			else if (ch->carrying.empty())
				send_to_char("You don't seem to be holding anything.\n", ch);
			else {
				FOREACH(ObjectList, ch->carrying, iter) {
					obj = *iter;
					if (ch->CanSee(obj) && (dotmode == FIND_ALL || isname( arg, obj->GetAlias() )) )
						perform_give(ch, vict, obj);
				}
			}
		}
	}
}



void weight_change_object(ObjData * obj, int weight)
{
  ObjData *tmp_obj;
  CharData *tmp_ch;

  if (obj->InRoom()) {
    GET_OBJ_WEIGHT(obj) += weight;
  } else if ((tmp_ch = obj->CarriedBy())) {
    obj->FromChar();
    GET_OBJ_WEIGHT(obj) += weight;
    obj->ToChar(tmp_ch);
  } else if ((tmp_obj = obj->Inside())) {
    obj->FromObject();
    GET_OBJ_WEIGHT(obj) += weight;
    obj->ToObject(tmp_obj);
  } else {
    log("SYSERR: Unknown attempt to subtract weight from an object.");
  }
}



void name_from_drinkcon(ObjData * obj) {
	const char *p = obj->GetAlias();
	while (p[0] && p[0] != ' ')
		++p;
	
	skip_spaces(p);
	
	if (*p)
	{
		BUFFER(new_name, MAX_STRING_LENGTH);
		strcpy(new_name, p + 1);
		obj->m_Keywords = new_name;
	}
}



void name_to_drinkcon(ObjData * obj, int type)
{
	BUFFER(new_name, MAX_STRING_LENGTH);
	sprintf(new_name, "%s %s", drinknames[type], obj->GetAlias());
	obj->m_Keywords = new_name;
}



ACMD(do_drink) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	ObjData *temp;
	int amount, weight;
	int on_ground = 0;

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Drink from what?\n", ch);
		return;
	}
	if (!(temp = get_obj_in_list_vis(ch, arg, ch->carrying))) {
		if (!(temp = get_obj_in_list_vis(ch, arg, ch->InRoom()->contents))) {
			act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
			return;
		} else
			on_ground = 1;
	}

	if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) &&
			(GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)) {
		send_to_char("You can't drink from that!\n", ch);
		return;
	}
	if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON)) {
		send_to_char("You have to be holding that to drink from it.\n", ch);
		return;
	}
#if 0
	if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0)) {
		/* The pig is drunk */
		send_to_char("You can't seem to get close enough to your mouth.\n", ch);
		act("$n tries to drink but misses $s mouth!", TRUE, ch, 0, 0, TO_ROOM);
		return;
	}
#endif
	if (!temp->GetValue(OBJVAL_FOODDRINK_CONTENT)) {
		send_to_char("It's empty.\n", ch);
		return;
	}
	
	if (!consume_otrigger(temp, ch))
		return;
	
#if 0
	if ((GET_COND(ch, FULL) > 20) && (GET_COND(ch, THIRST) > 0)) {
		send_to_char("Your stomach can't contain anymore!\n", ch);
		return;
	}
#endif
		
	if (subcmd == SCMD_DRINK) {
		act("$n drinks $T from $p.", TRUE, ch, temp, drinks[temp->GetValue(OBJVAL_FOODDRINK_TYPE)], TO_ROOM);

		act("You drink the $T.\n", FALSE, ch, 0, drinks[temp->GetValue(OBJVAL_FOODDRINK_TYPE)], TO_CHAR);

#if 0
		if (drink_aff[temp->GetValue(2)][DRUNK] > 0)
			amount = (25 - GET_COND(ch, THIRST)) / drink_aff[temp->GetValue(OBJVAL_FOODDRINK_TYPE)][DRUNK];
		else
#endif
			amount = Number(3, 10);

	} else {
		act("$n sips from $p.", TRUE, ch, temp, 0, TO_ROOM);
		act("It tastes like $t.\n", FALSE, ch, (ObjData *)drinks[temp->GetValue(OBJVAL_FOODDRINK_TYPE)], 0, TO_CHAR);
		amount = 1;
	}

	amount = MIN(amount, temp->GetValue(OBJVAL_FOODDRINK_CONTENT));

	/* You can't subtract more than the object weighs */
	weight = MIN(amount, GET_OBJ_WEIGHT(temp));

	weight_change_object(temp, -weight);	/* Subtract amount */

	if (0)
	{
		gain_condition(ch, DRUNK,
				(int) ((int) drink_aff[temp->GetValue(OBJVAL_FOODDRINK_TYPE)][DRUNK] * amount) / 4 );

		gain_condition(ch, FULL,
				(int) ((int) drink_aff[temp->GetValue(OBJVAL_FOODDRINK_TYPE)][FULL] * amount) / 4 );

		gain_condition(ch, THIRST,
				(int) ((int) drink_aff[temp->GetValue(OBJVAL_FOODDRINK_TYPE)][THIRST] * amount) / 4 );

#if 0
		if (GET_COND(ch, DRUNK) > 10)
			send_to_char("You feel drunk.\n", ch);

		if (GET_COND(ch, THIRST) > 20)
			send_to_char("You don't feel thirsty any more.\n", ch);

		if (GET_COND(ch, FULL) > 20)
			send_to_char("You are full.\n", ch);
#endif

		if (temp->GetValue(OBJVAL_FOODDRINK_POISONED)) {	/* The shit was poisoned ! */
			send_to_char("Oops, it tasted rather strange!\n", ch);
			act("$n chokes and utters some strange sounds.", TRUE, ch, 0, 0, TO_ROOM);

			Affect *af = new Affect(Affect::Poison, 0, APPLY_NONE, AFF_POISON, "You don't feel as sick.");
			af->Join(ch, amount * 3, Affect::AddDuration);
		}
	}
	/* empty the container, and no longer poison. */
	temp->SetValue(OBJVAL_FOODDRINK_CONTENT, temp->GetValue(OBJVAL_FOODDRINK_CONTENT) - amount);
	if (!temp->GetValue(OBJVAL_FOODDRINK_CONTENT)) {	/* The last bit */
		temp->SetValue(OBJVAL_FOODDRINK_TYPE, 0);
		temp->SetValue(OBJVAL_FOODDRINK_POISONED, 0);
		name_from_drinkcon(temp);
	}
	return;
}



ACMD(do_eat)
{
	BUFFER(arg, MAX_INPUT_LENGTH);
	ObjData *food;
	int amount;

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Eat what?\n", ch);
		return;
	}
	
	food = get_obj_in_list_vis(ch, arg, ch->carrying);
	
	if (!food) {
		ch->Send("You don't seem to have %s %s.\n", AN(arg), arg);
		return;
	}

	if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) ||
			(GET_OBJ_TYPE(food) == ITEM_FOUNTAIN))) {
		do_drink(ch, argument, "drink", SCMD_SIP);
		return;
	}
	if ((GET_OBJ_TYPE(food) != ITEM_FOOD) && !IS_STAFF(ch)) {
		send_to_char("You can't eat THAT!\n", ch);
		return;
	}
	
	if (!consume_otrigger(food, ch))
		return;
		
#if 0
	if (GET_COND(ch, FULL) > 20) {/* Stomach full */
		act("You are too full to eat more!", FALSE, ch, 0, 0, TO_CHAR);
		return;
	}
#endif
	if (subcmd == SCMD_EAT) {
		act("You eat the $o.", FALSE, ch, food, 0, TO_CHAR);
		act("$n eats $p.", TRUE, ch, food, 0, TO_ROOM);
	} else {
		act("You nibble a little bit of the $o.", FALSE, ch, food, 0, TO_CHAR);
		act("$n tastes a little bit of $p.", TRUE, ch, food, 0, TO_ROOM);
	}

	amount = (subcmd == SCMD_EAT ? food->GetValue(OBJVAL_FOODDRINK_FILL) : 1);

	if (0)
	{
		gain_condition(ch, FULL, amount);

#if 0
		if (GET_COND(ch, FULL) > 20)
			act("You are full.", FALSE, ch, 0, 0, TO_CHAR);
#endif

		if (food->GetValue(OBJVAL_FOODDRINK_POISONED) && !IS_STAFF(ch)) {
	/* The shit was poisoned ! */
			send_to_char("Oops, that tasted rather strange!\n", ch);
			act("$n coughs and utters some strange sounds.", FALSE, ch, 0, 0, TO_ROOM);
			
			Affect *af = new Affect(Affect::Poison, 0, APPLY_NONE, AFF_POISON, "You don't feel as sick.");
			af->Join(ch, amount * 2, Affect::AddDuration);
		}
	}
	if (subcmd == SCMD_EAT)
		food->Extract();
	else
	{
		food->SetValue(OBJVAL_FOODDRINK_FILL, food->GetValue(OBJVAL_FOODDRINK_FILL) - 1);
		if (food->GetValue(OBJVAL_FOODDRINK_FILL) <= 0) {
			send_to_char("There's nothing left now.\n", ch);
			food->Extract();
		}
	}
}


ACMD(do_pour) {
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	ObjData *fromObject = NULL, *toObject = NULL;
	int amount = TRUE;

	two_arguments(argument, arg1, arg2);

	if (subcmd == SCMD_POUR) {
		if (!*arg1) {		/* No arguments */
			act("From what do you want to pour?", FALSE, ch, 0, 0, TO_CHAR);
			amount = FALSE;
		} else if (!(fromObject = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
			act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
			amount = FALSE;
		} else if (GET_OBJ_TYPE(fromObject) != ITEM_DRINKCON) {
			act("You can't pour from that!", FALSE, ch, 0, 0, TO_CHAR);
			amount = FALSE;
		}
	} else if (subcmd == SCMD_FILL) {
		if (!*arg1) {		/* no arguments */
			send_to_char("What do you want to fill?  And what are you filling it from?\n", ch);
			amount = FALSE;
		} else if (!(toObject = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
			send_to_char("You can't find it!", ch);
			amount = FALSE;
		} else if (GET_OBJ_TYPE(toObject) != ITEM_DRINKCON) {
			act("You can't fill $p!", FALSE, ch, toObject, 0, TO_CHAR);
			amount = FALSE;
		} else if (!*arg2) {		/* no 2nd argument */
			act("What do you want to fill $p from?", FALSE, ch, toObject, 0, TO_CHAR);
			amount = FALSE;
		} else if (!(fromObject = get_obj_in_list_vis(ch, arg2, ch->InRoom()->contents))) {
			ch->Send("There doesn't seem to be %s %s here.\n", AN(arg2), arg2);
			amount = FALSE;
		} else if (GET_OBJ_TYPE(fromObject) != ITEM_FOUNTAIN) {
			act("You can't fill something from $p.", FALSE, ch, fromObject, 0, TO_CHAR);
			amount = FALSE;
		}
	}
	if (!amount) {
		return;
	}
	if (fromObject->GetValue(OBJVAL_FOODDRINK_CONTENT) == 0) {
		act("The $p is empty.", FALSE, ch, fromObject, 0, TO_CHAR);
		return;
	}
	
	if (subcmd == SCMD_POUR) {	/* pour */
		if (!*arg2) {
			act("Where do you want it?  Out or in what?", FALSE, ch, 0, 0, TO_CHAR);
			amount = FALSE;
		} else if (!str_cmp(arg2, "out")) {
			act("$n empties $p.", TRUE, ch, fromObject, 0, TO_ROOM);
			act("You empty $p.", FALSE, ch, fromObject, 0, TO_CHAR);

			weight_change_object(fromObject, -fromObject->GetValue(OBJVAL_FOODDRINK_CONTENT)); /* Empty */

			fromObject->SetValue(OBJVAL_FOODDRINK_CONTENT, 0);
			fromObject->SetValue(OBJVAL_FOODDRINK_TYPE, 0);
			fromObject->SetValue(OBJVAL_FOODDRINK_POISONED, 0);
			name_from_drinkcon(fromObject);

			amount = FALSE;
		} else if (!(toObject = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
			act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
			amount = FALSE;
		} else if ((GET_OBJ_TYPE(toObject) != ITEM_DRINKCON) &&
				(GET_OBJ_TYPE(toObject) != ITEM_FOUNTAIN)) {
			act("You can't pour anything into that.", FALSE, ch, 0, 0, TO_CHAR);
			amount = FALSE;
		}
	}
	if (!amount) {
		return;
	}
	
	if (toObject == fromObject) {
		act("A most unproductive effort.", FALSE, ch, 0, 0, TO_CHAR);
		amount = FALSE;
	} else if ((toObject->GetValue(OBJVAL_FOODDRINK_CONTENT) != 0) &&
			(toObject->GetValue(OBJVAL_FOODDRINK_TYPE) != fromObject->GetValue(OBJVAL_FOODDRINK_TYPE))) {
		act("There is already another liquid in it!", FALSE, ch, 0, 0, TO_CHAR);
		amount = FALSE;
	} else if (!(toObject->GetValue(OBJVAL_FOODDRINK_CONTENT) < toObject->GetValue(OBJVAL_FOODDRINK_FILL))) {
		act("There is no room for more.", FALSE, ch, 0, 0, TO_CHAR);
		amount = FALSE;
	}
	if (!amount) {
		return;
	}
	
	if (subcmd == SCMD_POUR) {
		act("You pour the $T into the $p.", FALSE, ch, toObject, drinks[fromObject->GetValue(OBJVAL_FOODDRINK_TYPE)], TO_CHAR);
	} else if (subcmd == SCMD_FILL) {
		act("You gently fill $p from $P.", FALSE, ch, toObject, fromObject, TO_CHAR);
		act("$n gently fills $p from $P.", TRUE, ch, toObject, fromObject, TO_ROOM);
	}
	/* New alias */
	if (toObject->GetValue(OBJVAL_FOODDRINK_CONTENT) == 0)
		name_to_drinkcon(toObject, fromObject->GetValue(OBJVAL_FOODDRINK_TYPE));

	/* First same type liq. */
	toObject->SetValue(OBJVAL_FOODDRINK_TYPE, fromObject->GetValue(OBJVAL_FOODDRINK_TYPE));

	/* Then how much to pour */
	amount = toObject->GetValue(OBJVAL_FOODDRINK_FILL) - toObject->GetValue(OBJVAL_FOODDRINK_CONTENT);
	fromObject->SetValue(OBJVAL_FOODDRINK_CONTENT, fromObject->GetValue(OBJVAL_FOODDRINK_CONTENT) - amount);

	toObject->SetValue(OBJVAL_FOODDRINK_CONTENT, toObject->GetValue(OBJVAL_FOODDRINK_FILL));

	if (fromObject->GetValue(OBJVAL_FOODDRINK_CONTENT) < 0) {	/* There was too little */
		toObject->SetValue(OBJVAL_FOODDRINK_CONTENT, toObject->GetValue(OBJVAL_FOODDRINK_CONTENT) + fromObject->GetValue(OBJVAL_FOODDRINK_CONTENT));
		amount += fromObject->GetValue(OBJVAL_FOODDRINK_CONTENT);
		fromObject->SetValue(OBJVAL_FOODDRINK_CONTENT, 0);
		fromObject->SetValue(OBJVAL_FOODDRINK_TYPE, 0);
		fromObject->SetValue(OBJVAL_FOODDRINK_POISONED, 0);
		name_from_drinkcon(fromObject);
	}
	/* Then the poison boogie */
	if (fromObject->GetValue(OBJVAL_FOODDRINK_POISONED))
		toObject->SetValue(OBJVAL_FOODDRINK_POISONED, 1);

	/* And the weight boogie */
	weight_change_object(fromObject, -amount);
	weight_change_object(toObject, amount);	/* Add weight */
}



void wear_message(CharData * ch, ObjData * obj, WearPosition where)
{
  char *wear_messages[][2] = {
    {"$n slides $p on to $s right ring finger.",
    "You slide $p on to your right ring finger."},

    {"$n slides $p on to $s left ring finger.",
    "You slide $p on to your left ring finger."},

    {"$n wears $p around $s neck.",
    "You wear $p around your neck."},

    {"$n wears $p on $s body.",
    "You wear $p on your body.",},

    {"$n wears $p on $s head.",
    "You wear $p on your head."},

    {"$n puts $p on $s legs.",
    "You put $p on your legs."},

    {"$n wears $p on $s feet.",
    "You wear $p on your feet."},

    {"$n puts $p on $s hands.",
    "You put $p on your hands."},

    {"$n wears $p on $s arms.",
    "You wear $p on your arms."},

    {"$n wears $p about $s body.",
    "You wear $p around your body."},

    {"$n wears $p around $s waist.",
    "You wear $p around your waist."},

    {"$n puts $p on around $s right wrist.",
    "You put $p on around your right wrist."},

    {"$n puts $p on around $s left wrist.",
    "You put $p on around your left wrist."},

    {"$n wears $p over $s eyes.",
    "You wear $p over your eyes."},

    {"$n wears $p on $s tail.",
    "You wear $p on your tail."},

	{"ERROR, PLEASE REPORT",
	"ERROR, PLEASE REPORT"},

	{"ERROR, PLEASE REPORT",
	"ERROR, PLEASE REPORT"},

    {"$n wields $p with both $s hands.",
    "You wield $p with both your hands."},

    {"$n grabs $p with both $s hands.",
    "You grab $p with both your hands."}, 
    
    {"$n wields $p.",
    "You wield $p."},

    {"$n wields $p in $s off hand.",
    "You wield $p in your off hand."},

    {"$n lights $p and holds it.",
    "You light $p and hold it."},

    {"$n grabs $p.",
    "You grab $p."}
  };

  act(wear_messages[wear_message_number(obj, where)][0], TRUE, ch, obj, 0, TO_ROOM);
  act(wear_messages[wear_message_number(obj, where)][1], FALSE, ch, obj, 0, TO_CHAR);
}



void perform_wear(CharData * ch, ObjData * obj, int where) {
	//	ITEM_WEAR_TAKE is used for objects that do not require special bits
	//	to be put into that position (e.g. you can hold any object, not just
	//	an object with a HOLD bit.)
//	ObjData *	obj2;
	
	int wear_bitvectors[] = {
		ITEM_WEAR_FINGER, ITEM_WEAR_FINGER, ITEM_WEAR_NECK,
		ITEM_WEAR_BODY, ITEM_WEAR_HEAD, ITEM_WEAR_LEGS,
		ITEM_WEAR_FEET, ITEM_WEAR_HANDS, ITEM_WEAR_ARMS,
		ITEM_WEAR_ABOUT, ITEM_WEAR_WAIST, ITEM_WEAR_WRIST, ITEM_WEAR_WRIST,
		ITEM_WEAR_EYES,
		ITEM_WEAR_TAIL,
		(ITEM_WEAR_HOLD | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD),
		(ITEM_WEAR_HOLD | ITEM_WEAR_SHIELD | ITEM_WEAR_WIELD)};

	char *already_wearing[] = {
		"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\n",
		"You're already wearing something on both of your ring fingers.\n",
		"You're already wearing something around your neck.\n",
		"You're already wearing something on your body.\n",
		"You're already wearing something on your head.\n",
		"You're already wearing something on your legs.\n",
		"You're already wearing something on your feet.\n",
		"You're already wearing something on your hands.\n",
		"You're already wearing something on your arms.\n",
		"You're already wearing something about your body.\n",
		"You already have something around your waist.\n",
		"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\n",
		"You're already wearing something around both of your wrists.\n",
		"You're already wearing something over your eyes.\n",
		"You're already wearing something on your tail.\n",
//		"You're already holding something in your right hand.\n",
		"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\n",
//		"You're already holding something in your left hand.\n",
		"Your hands are full.\n",
	};

	if (ch->CanUse(obj) != NotRestricted) {
		act("You can't figure out how to use $p.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}
  
	/* first, make sure that the wear position is valid. */
	if (!CAN_WEAR(obj, wear_bitvectors[where])) {
		act("You can't wear $p there.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}
  
 
	/* for neck, finger, and wrist, try pos 2 if pos 1 is already full */
	if ((where == WEAR_FINGER_R) || (where == WEAR_WRIST_R) || (where == WEAR_HAND_R))
		if (GET_EQ(ch, where))
			where++;
	
	if (where == WEAR_HAND_R || where == WEAR_HAND_L) {
		if (OBJ_FLAGGED(obj, ITEM_TWO_HAND) &&
			((GET_EQ(ch, WEAR_HAND_R) && OBJ_FLAGGED(GET_EQ(ch, WEAR_HAND_R), ITEM_TWO_HAND)) ||
			 (GET_EQ(ch, WEAR_HAND_L) && OBJ_FLAGGED(GET_EQ(ch, WEAR_HAND_L), ITEM_TWO_HAND)))) {
			send_to_char("You're already using a two-handed item, you can't use another\n", ch);
			return;
#if 0		//	Allow two-hand with another item now
		} else if (OBJ_FLAGGED(obj, ITEM_TWO_HAND)) {
			if (GET_EQ(ch, WEAR_HAND_R) || GET_EQ(ch, WEAR_HAND_L)) {
				send_to_char("You need both hands free to hold this.\n", ch);
				return;
#if 0		//	Weapons have a STRENGTH req now!
			} else if (GET_OBJ_TOTAL_WEIGHT(obj) > GET_EFFECTIVE_STR(ch)) {			//	1/2
				send_to_char("It's too heavy for you to use.\n", ch);
				return;
#endif
			} else
				where = WEAR_HAND_R;
#endif
		} else if (GET_EQ(ch, WEAR_HAND_R) && GET_EQ(ch, WEAR_HAND_L)) {
			send_to_char("Your hands are full!\n", ch);
			return;
#if 0	//	No weight restrictions on holding
		} else if (GET_OBJ_TOTAL_WEIGHT(obj) > GET_CAR(ch) / 2) {			//	1/4
			send_to_char("It's too heavy for you to use.\n", ch);
			return;
#endif
		} else if (where == WEAR_HAND_R && GET_EQ(ch, WEAR_HAND_R))
			where = WEAR_HAND_L;
		else if (where == WEAR_HAND_L && GET_EQ(ch, WEAR_HAND_L))
			where = WEAR_HAND_R;
	}
	
	if (GET_EQ(ch, where)) {
		send_to_char(already_wearing[where], ch);
		return;
	}
	
	if (!wear_otrigger(obj, ch, where) || obj->CarriedBy() != ch)
		return;

	wear_message(ch, obj, (WearPosition)where);
	obj->FromChar();
	equip_char(ch, obj, where);
			
	if (FIGHTING(ch)
#if ROAMING_COMBAT && 0
		&& IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))
#endif
		&& (where == WEAR_HAND_R || where == WEAR_HAND_L) &&
			GET_OBJ_TYPE(obj) == ITEM_WEAPON)
	{
		combat_reset_timer(ch);
	}
}



int find_eq_pos(CharData * ch, ObjData * obj, char *arg) {
	int where = -1;

	static char *keywords[] = {
		"finger",
		"!RESERVED!",
		"neck",
		"body",
		"head",
		"legs",
		"feet",
		"hands",
		"arms",
		"about",
		"waist",
		"wrist",
		"!RESERVED!",
		"eyes",
		"tail",
		"right",
		"left",
		"\n"
	};

	if (!arg || !*arg) {
		if (CAN_WEAR(obj, ITEM_WEAR_FINGER))	where = WEAR_FINGER_R;
		if (CAN_WEAR(obj, ITEM_WEAR_NECK))		where = WEAR_NECK;
		if (CAN_WEAR(obj, ITEM_WEAR_BODY))		where = WEAR_BODY;
		if (CAN_WEAR(obj, ITEM_WEAR_HEAD))		where = WEAR_HEAD;
		if (CAN_WEAR(obj, ITEM_WEAR_LEGS))		where = WEAR_LEGS;
		if (CAN_WEAR(obj, ITEM_WEAR_FEET))		where = WEAR_FEET;
		if (CAN_WEAR(obj, ITEM_WEAR_HANDS))		where = WEAR_HANDS;
		if (CAN_WEAR(obj, ITEM_WEAR_ARMS))		where = WEAR_ARMS;
		if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))		where = WEAR_ABOUT;
		if (CAN_WEAR(obj, ITEM_WEAR_WAIST))		where = WEAR_WAIST;
		if (CAN_WEAR(obj, ITEM_WEAR_WRIST))		where = WEAR_WRIST_R;
		if (CAN_WEAR(obj, ITEM_WEAR_EYES))		where = WEAR_EYES;
		if (CAN_WEAR(obj, ITEM_WEAR_TAIL))		where = WEAR_TAIL;
	} else if ((where = search_block(arg, keywords, FALSE)) < 0)
		act("'$T'?  What part of your body is THAT?\n", FALSE, ch, 0, arg, TO_CHAR);

	return where;
}


bool can_hold(ObjData *obj) {
	if (CAN_WEAR(obj, ITEM_WEAR_HOLD | ITEM_WEAR_SHIELD) /*|| GET_OBJ_TYPE(obj) == ITEM_LIGHT*/ )
		return true;
	return false;
}


ACMD(do_wear) {
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	ObjData *obj;
	int where = -1, dotmode, items_worn = 0;
	
	two_arguments(argument, arg1, arg2);

	if (!*arg1)
		send_to_char("Wear what?\n", ch);
	else {
		dotmode = find_all_dots(arg1);
	
		if (*arg2 && (dotmode != FIND_INDIV))
			send_to_char("You can't specify the same body location for more than one item!\n", ch);
		else if (dotmode == FIND_ALL) {
			FOREACH(ObjectList, ch->carrying, iter)
			{
				obj = *iter;
				if (!ch->CanSee(obj))
					continue;
				if ((where = find_eq_pos(ch, obj, 0)) >= 0) {
					items_worn++;
					perform_wear(ch, obj, where);
				} else if (CAN_WEAR(obj, ITEM_WEAR_WIELD)) {
					items_worn++;
//					do_wield(ch, obj->GetIDString(), 0, "wield", 0);				//	In that case: command_interpreter(ch, Lexi::String("wield %c%d", UID_CHAR, GET_ID(obj)).c_str())
					perform_wear(ch, obj, WEAR_HAND_R);
				} else if (can_hold(obj)) {
					items_worn++;
//					do_grab(ch, obj->GetIDString(), 0, "grab", 0);
					perform_wear(ch, obj, WEAR_HAND_L);
				}
			}
			if (!items_worn)
				send_to_char("You don't seem to have anything wearable.\n", ch);
		} else if (dotmode == FIND_ALLDOT) {
			if (!*arg1)
				send_to_char("Wear all of what?\n", ch);
			else if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
				act("You don't seem to have any $ts.\n", FALSE, ch, (ObjData *)arg1, 0, TO_CHAR);
			else {
				FOREACH(ObjectList, ch->carrying, iter)
				{
					obj = *iter;
					if (!ch->CanSee(obj) || !isname(arg1, obj->GetAlias()))
						continue;
					
					where = find_eq_pos(ch, obj, 0);
					if (where < 0)
					{
						if (CAN_WEAR(obj, ITEM_WEAR_WIELD))	where = WEAR_HAND_R;
						else if (can_hold(obj))				where = WEAR_HAND_L;
					}
					
					if (where >= 0)		perform_wear(ch, obj, where);
					else				act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
				}
			}
		} else if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
			ch->Send("You don't seem to have %s %s.\n", AN(arg1), arg1);
		else
		{
			where = find_eq_pos(ch, obj, arg2);
			if (where < 0)
			{
				if (CAN_WEAR(obj, ITEM_WEAR_WIELD))	where = WEAR_HAND_R;
				else if (can_hold(obj))				where = WEAR_HAND_L;
			}
			
			if (where >= 0)		perform_wear(ch, obj, where);
			else  if (!*arg2)	act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
		} 
	}
}



ACMD(do_wield) {
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	ObjData *obj;
	
	two_arguments(argument, arg1, arg2);

	if (!*arg1)
		send_to_char("Wield what?\n", ch);
	else if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
		ch->Send("You don't seem to have %s %s.\n", AN(arg1), arg1);
	else if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
		send_to_char("You can't wield that.\n", ch);
	else if (!strn_cmp(arg2, "right", strlen(arg2)))
		perform_wear(ch, obj, WEAR_HAND_R);
	else if (!strn_cmp(arg2, "left", strlen(arg2)))
		perform_wear(ch, obj, WEAR_HAND_L);
	else
		send_to_char("You only have two hands, right and left.\n", ch);
}


ACMD(do_grab) {
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	ObjData *obj;

	two_arguments(argument, arg1, arg2);

	if (!*arg1)
		send_to_char("Hold what?\n", ch);
	else if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying)))
		ch->Send("You don't seem to have %s %s.\n", AN(arg1), arg1);
	else if (!can_hold(obj))
		send_to_char("You can't hold that.\n", ch);
	else if (!strn_cmp(arg2, "left", strlen(arg2)))
		perform_wear(ch, obj, WEAR_HAND_L);
	else if (!strn_cmp(arg2, "right", strlen(arg2)))
		perform_wear(ch, obj, WEAR_HAND_R);
	else
		send_to_char("You only have two hands, right and left.\n", ch);
}



void perform_remove(CharData * ch, int pos)
{
	ObjData *obj;

	if (!(obj = GET_EQ(ch, pos)))
		log("Error in perform_remove: bad pos %d passed.", pos);
	else if (!IS_STAFF(ch) && IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
		act("$p: you can't carry that many items!", FALSE, ch, obj, 0, TO_CHAR);
//	else if (OBJ_FLAGGED(obj, ITEM_NODROP))
//		act("You can't remove $p, it must be CURSED!", FALSE, ch, obj, 0, TO_CHAR);
	else if (remove_otrigger(obj, ch, pos)) {
		unequip_char(ch, pos)->ToChar(ch);
		act("You stop using $p.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n stops using $p.", TRUE, ch, obj, 0, TO_ROOM);

		if (FIGHTING(ch)
#if ROAMING_COMBAT && 0
			&& IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))
#endif
			&& (pos == WEAR_HAND_R || pos == WEAR_HAND_L) &&
				GET_OBJ_TYPE(obj) == ITEM_WEAPON)
		{
			combat_reset_timer(ch);
		}
	}
}



ACMD(do_remove) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	ObjData *obj = NULL;
	int		i, dotmode, msg;
	bool	found = false;
	
	one_argument(argument, arg);

	if (!*arg) {
		send_to_char("Remove what?\n", ch);
		return;
	}
	
	if (!(obj = get_obj_in_list_type(ITEM_BOARD, ch->carrying)))
		obj = get_obj_in_list_type(ITEM_BOARD, ch->InRoom()->contents);
	
	if (obj && isdigit(*arg) && (msg = atoi(arg))) {
		if (ch->CanUse(obj) != NotRestricted)
			ch->Send("You don't have the access privileges for that board!\n");
		else
			Board::RemoveMessage(obj, ch, msg);
	} else {
		dotmode = find_all_dots(arg);

		if (dotmode == FIND_ALL) {
			for (i = 0; i < NUM_WEARS; i++)
				if (GET_EQ(ch, i)) {
					perform_remove(ch, i);
					found = true;
				}
			if (!found)
				send_to_char("You're not using anything.\n", ch);
		} else if (dotmode == FIND_ALLDOT) {
			if (!*arg)
				send_to_char("Remove all of what?\n", ch);
			else {
				for (i = 0; i < NUM_WEARS; i++)
					if (GET_EQ(ch, i) && ch->CanSee(GET_EQ(ch, i)) && isname(arg, GET_EQ(ch, i)->GetAlias()))
					{
						perform_remove(ch, i);
						found = true;
					}
				if (!found)
					ch->Send("You don't seem to be using any %ss.\n", arg);
			}
		} else if (!(obj = get_obj_in_equip_vis(ch, arg, ch->equipment, &i)))
			ch->Send("You don't seem to be using %s %s.\n", AN(arg), arg);
		else
			perform_remove(ch, i);
	}
}


ACMD(do_pull)
{
	BUFFER(arg, MAX_INPUT_LENGTH);
	ObjData * obj;

	one_argument(argument, arg);
	
	if (!*arg)
	{
		send_to_char("Pull what?\n", ch);
	}
	else if(!str_cmp(arg,"pin"))
	{
		obj = GET_EQ(ch, WEAR_HAND_L);
		
		if (!obj || (GET_OBJ_TYPE(obj) != ITEM_GRENADE))
		{
			obj = GET_EQ(ch, WEAR_HAND_R);
		}
		
		if (!obj)
		{
			send_to_char("You aren't holding a grenade!\n", ch);
			return;
		}
	}
	else if (!generic_find(arg, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, NULL, &obj))
	{
		send_to_char("Pull what?\n", ch);
	}

	if(obj)
	{
		if(GET_OBJ_TYPE(obj) == ITEM_GRENADE && obj->GetValue(OBJVAL_GRENADE_TIMER) > 0)
		{
		    act("You pull the pin on $p.  Its activated!", FALSE, ch, obj, 0, TO_CHAR);
		    act("$n pulls the pin on $p.", TRUE, ch, obj, 0, TO_ROOM);
			obj->AddOrReplaceEvent(new GrenadeExplosionEvent(obj->GetValue(OBJVAL_GRENADE_TIMER) RL_SEC, obj, GET_ID(ch)));
		}
		else
			send_to_char("That's NOT a grenade!\n", ch);
	}
	else
		send_to_char("You aren't holding a grenade!\n", ch);
}


ACMD(do_prime)
{
	BUFFER(arg, MAX_INPUT_LENGTH);
	ObjData * obj;

	one_argument(argument, arg);
	
	if (!*arg)
		send_to_char("Prime what?\n", ch);
	else if (!(obj = get_obj_in_equip_type(ITEM_GRENADE, ch->equipment)) && !(obj = get_obj_in_list_type(ITEM_GRENADE, ch->carrying)))
		send_to_char("Prime what?\n", ch);
	else if(GET_OBJ_TYPE(obj) != ITEM_GRENADE || obj->GetValue(OBJVAL_GRENADE_TIMER) <= 0)
		send_to_char("That's NOT a grenade!\n", ch);
	else
	{
	    act("You pull the pin on $p.  Its activated!", FALSE, ch, obj, 0, TO_CHAR);
	    act("$n pulls the pin on $p.", TRUE, ch, obj, 0, TO_ROOM);
		obj->AddOrReplaceEvent(new GrenadeExplosionEvent(obj->GetValue(OBJVAL_GRENADE_TIMER) RL_SEC, obj, GET_ID(ch)));
	}
}


ACMD(do_reload) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	int pos;
	ObjData *missile = NULL;
	ObjData *weapon;
	
	argument = one_argument(argument, arg);
	
	if (*arg)
	{
		weapon = get_obj_in_equip_vis(ch, arg, ch->equipment, &pos);
		if (!weapon)
			weapon = get_obj_in_list_vis(ch, arg, ch->carrying);
	}
	else
		weapon = ch->GetGun();
	
	//	Now check to see if its a loadable weapon
	if (!weapon)
	{
		if (*arg)
			ch->Send("You don't seem to have any %ss.\n", arg);
		else
			ch->Send("You aren't wielding a loadable weapon.\n");
	}
	else if (!IS_GUN(weapon) || GET_GUN_AMMO_TYPE(weapon) == -1)
		act("$p is not a loadable weapon.", FALSE, ch, weapon, 0, TO_CHAR);
	else if (subcmd == SCMD_LOAD)
	{
		FOREACH(ObjectList, ch->carrying, iter)
		{
			missile = *iter;
			
			if (ch->CanSee(missile) &&
					(GET_OBJ_TYPE(missile) == ITEM_AMMO) &&
					(missile->GetValue(OBJVAL_AMMO_TYPE) == GET_GUN_AMMO_TYPE(weapon)) &&
					(missile->GetValue(OBJVAL_AMMO_AMOUNT) > 0))
				break;
		
			missile = NULL;
		}
		
		if (GET_GUN_AMMO(weapon) > 0)
			send_to_char("The weapon is already loaded!", ch);
		else if (!missile)
			send_to_char("No ammo for the weapon.", ch);
		else
		{
			act("You load $P into $p.", TRUE, ch, weapon, missile, TO_CHAR);
			act("$n loads $P into $p.", TRUE, ch, weapon, missile, TO_ROOM);
			GET_GUN_AMMO(weapon) = missile->GetValue(OBJVAL_AMMO_AMOUNT);
			GET_GUN_AMMO_FLAGS(weapon) = missile->GetValue(OBJVAL_AMMO_FLAGS);
			GET_GUN_AMMO_VID(weapon) = missile->GetVirtualID();
			missile->Extract();			
		}
	}
	else if (subcmd == SCMD_UNLOAD)
	{
		if (GET_GUN_AMMO(weapon) <= 0)
			act("The $p is empty.", FALSE, ch, weapon, 0, TO_CHAR);
		else if (!(missile = ObjData::Create(GET_GUN_AMMO_VID(weapon))))
	    	act("Problem: the missile object couldn't be created.\n",
	    			TRUE, ch, 0, 0, TO_CHAR);
	    else if (GET_OBJ_TYPE(missile) != ITEM_AMMO)
	    	act("Problem: the VNUM listed by the gun was not of type ITEM_AMMO.\n",
	    			TRUE, ch, 0, 0, TO_CHAR);
	    else if	(missile->GetValue(OBJVAL_AMMO_TYPE) != GET_GUN_AMMO_TYPE(weapon))
	    	act("Problem: the ammo VNUM of gun is not same type as the gun.\n",
	    			TRUE, ch, 0, 0, TO_CHAR);
	    else {
		    missile->ToChar(ch);

			act("You unload $P from $p.", TRUE, ch, weapon, missile, TO_CHAR);
			act("$n unloads $P from $p.", TRUE, ch, weapon, missile, TO_ROOM);
			missile->SetValue(OBJVAL_AMMO_AMOUNT, GET_GUN_AMMO(weapon));
			
			load_otrigger(missile);
		}
		GET_GUN_AMMO(weapon) = 0;	//	Guarantees its unloaded in case of problem
	}
	else
		log("SYSERR: Unknown subcmd %d in do_reload.", subcmd);
}
