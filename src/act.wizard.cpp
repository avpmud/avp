/*************************************************************************
*   File: act.wizard.c++             Part of Aliens vs Predator: The MUD *
*  Usage: Staff commands                                                 *
*************************************************************************/

#if defined(_WIN32)
#include <direct.h>
#endif

#include "structs.h"
#include "opinion.h"
#include "scripts.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "affects.h"
#include "event.h"
#include "olc.h"
#include "staffcmds.h"
#include "extradesc.h"
#include "track.h"
#include "ident.h"
#include "shop.h"
#include "queue.h"
#include "constants.h"
#include "help.h"
#include "ban.h"
#include "imc/imc.h"

/* extern functions */
void do_start(CharData *ch);
void show_shops(CharData * ch, char *value);
void reset_zone(ZoneData *zone);
void roll_real_abils(CharData *ch);
void forget(CharData * ch, CharData * victim);
void remember(CharData * ch, CharData * victim);
void CheckRegenRates(CharData *ch);
char *skill_name(int skill);

/*   external vars  */
extern time_t boot_time;
extern struct attack_hit_type attack_hit_text[];
extern int circle_restrict;
extern int circle_shutdown, circle_reboot;
extern bool no_external_procs;
extern int buf_switches, buf_largecount, buf_overflows;
extern IdentServer	* Ident;
extern IDNum top_idnum;
extern Queue EventQueue;

extern void check_level(CharData *ch);

/* prototypes */
RoomData *find_target_room(CharData * ch, char *rawroomstr);
void do_stat_room(CharData * ch, RoomData *rm);
void do_stat_object(CharData * ch, ObjData * j);
void do_stat_character(CharData * ch, CharData * k);
void stop_snooping(CharData * ch);
void perform_immort_vis(CharData *ch);
void perform_immort_invis(CharData *ch, int level);
void print_zone_to_buf(char *bufptr, ZoneData *zone);
void show_deleted(CharData * ch);
void send_to_imms(char *msg);
bool perform_set(CharData *ch, CharData *vict, int mode, char *val_arg);
ACMD(do_echo);
ACMD(do_send);
ACMD(do_at);
ACMD(do_goto);
ACMD(do_trans);
ACMD(do_teleport);
ACMD(do_vnum);
ACMD(do_stat);
ACMD(do_shutdown);
ACMD(do_snoop);
ACMD(do_switch);
ACMD(do_return);
ACMD(do_load);
ACMD(do_vstat);
ACMD(do_purge);
ACMD(do_advance);
ACMD(do_restore);
ACMD(do_invis);
ACMD(do_gecho);
ACMD(do_poofset);
ACMD(do_dc);
ACMD(do_wizlock);
ACMD(do_date);
ACMD(do_last);
ACMD(do_force);
ACMD(do_as);
ACMD(do_wiznet);
ACMD(do_zreset);
ACMD(do_wizutil);
ACMD(do_show);
ACMD(do_set);
ACMD(do_syslog);
ACMD(do_depiss);
ACMD(do_repiss);
ACMD(do_hunt);
ACMD(do_vwear);
ACMD(do_copyover);
ACMD(do_tedit);
ACMD(do_reward);
ACMD(do_string);
ACMD(do_allow);
ACMD(do_stopscripts);
ACMD(do_setcapture);
ACMD(do_team);
ACMD(do_objindex);
ACMD(do_mobindex);
ACMD(do_trigindex);
ACMD(do_killall);
ACMD(do_listhelp);

void load_otrigger(ObjData *obj);

extern struct sort_struct {
	int	sort_pos;
	Flags	type;
} *cmd_sort_info;

extern int num_of_cmds;

ACMD(do_echo) {
	skip_spaces(argument);

	if (!*argument)
		send_to_char("Yes.. but what?\n", ch);
	else {
		BUFFER(buf, MAX_STRING_LENGTH);
		if (subcmd == SCMD_EMOTE)
			sprintf(buf, "$n %s", argument);
		else
			strcpy(buf, argument);
		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else {
			act(buf, FALSE, ch, 0, 0, TO_CHAR);
		}
		
		int flags = TO_ROOM | TO_IGNORABLE;
		
		if (subcmd == SCMD_EMOTE)
		{
			flags |= TO_NOTTRIG;
		}
		
		act(buf, FALSE, ch, 0, 0, flags );
	}
}


ACMD(do_send) {
	CharData *vict;
	BUFFER(buf, MAX_INPUT_LENGTH);
	BUFFER(arg, MAX_INPUT_LENGTH);
			
	half_chop(argument, arg, buf);

	if (!*arg) {
		send_to_char("Send what to who?\n", ch);
	} else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD))) {
		send_to_char(NOPERSON, ch);
	} else {
		send_to_char(buf, vict);
		send_to_char("\n", vict);
		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char("Sent.\n", ch);
		else
			act("You send '$t' to $N.\n", TRUE, ch, (ObjData *)buf, vict, TO_CHAR);
	}
}



/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
RoomData *find_target_room(CharData * ch, char *rawroomstr)
{
	RoomData * location;
	CharData *target_mob;
	ObjData *target_obj;
	BUFFER(roomstr, MAX_INPUT_LENGTH);

	one_argument(rawroomstr, roomstr);

	if (!*roomstr) {
		send_to_char("You must supply a room number or name.\n", ch);
		return NULL;
	}
	
	if (IsVirtualID(roomstr) && !strchr(roomstr, '.'))
	{
		location = world.Find(VirtualID(roomstr, ch->InRoom()->GetVirtualID().zone));
		if (!location)
		{
			send_to_char("No room exists with that number.\n", ch);
			return NULL;
		}
	}
	else if ((target_mob = get_char_vis(ch, roomstr, FIND_CHAR_WORLD)))
		location = target_mob->InRoom();
	else if ((target_obj = get_obj_vis(ch, roomstr)))
	{
		if (target_obj->InRoom())
			location = target_obj->InRoom();
		else
		{
			send_to_char("That object is not available.\n", ch);
			return NULL;
		}
	}
	else
	{
		send_to_char("No such creature or object around.\n", ch);
		return NULL;
	}

	if (ROOM_FLAGGED(location, ROOM_STAFFROOM) && !IS_STAFF(ch)) {
		send_to_char("Only staff are allowed in that room!\n", ch);
		return NULL;
	}

	/* a location has been found -- if you're < GRGOD, check restrictions. */
	if (ch->GetLevel() < LVL_ADMIN) {
		if (ROOM_FLAGGED(location, ROOM_PRIVATE) && (location->people.size() > 1)) {
			send_to_char("There's a private conversation going on in that room.\n", ch);
			return NULL;
		}
		if (!House::CanEnter(ch, location)) {
			send_to_char("That's private property -- no trespassing!\n", ch);
			return NULL;
		}
		if (ROOM_FLAGGED(location, ROOM_ADMINROOM)) {
			send_to_char("Only Admins are allowed in that room!\n", ch);
			return NULL;
		}
	}
	
/*	if (ROOM_FLAGGED(location, ROOM_ULTRAPRIVATE) && (ch->GetLevel() < LVL_CODER)) {
		send_to_char(	"That room is off limits to absolutely everyone.\n"
						"Except FearItself, of course.\n", ch);
		return NOWHERE;
	}
*/	return location;
}



ACMD(do_at) {
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	RoomData *location, *original_loc;

	half_chop(argument, arg1, arg2);
	if (!*arg1) {
		send_to_char("You must supply a room number or a name.\n", ch);
	} else if (!*command) {
		send_to_char("What do you want to do there?\n", ch);
	} else if ((location = find_target_room(ch, arg1))) {

		/* a location has been found. */
		original_loc = ch->InRoom();
		ch->FromRoom();
		ch->ToRoom(location);
		command_interpreter(ch, arg2);

		/* check if the char is still there */
		if (IN_ROOM(ch) == location) {
			ch->FromRoom();
			ch->ToRoom(original_loc);
		}
	}
}

#if 0
ACMD(do_as)
{
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(to_force, MAX_INPUT_LENGTH);
	CharData *victim;

	half_chop(argument, arg1, to_force);
	
	if (ch->desc->m_OrigCharacter)
		send_to_char("You're already switched.\n", ch);
	else if (!*arg1)
		send_to_char("Do it as who?\n", ch);
	else if (!*command)
		send_to_char("What do you want to do as them?\n", ch);
	else  if (!(victim = get_char_vis(ch, arg1, FIND_CHAR_WORLD)))
		send_to_char(NOPERSON, ch);
	else if (ch == victim)
		send_to_char("Hee hee... we are jolly funny today, eh?\n", ch);
	else if (IS_STAFF(victim) && ((STF_FLAGGED(victim, STAFF_SRSTAFF) && !STF_FLAGGED(ch, STAFF_ADMIN)) ||
			(STF_FLAGGED(victim, STAFF_ADMIN) && ch->GetLevel() < 105)))
		send_to_char("No, no, no!\n", ch);
	else
	{
		mudlogf( NRM, LVL_STAFF, TRUE,  "(GC) %s as-forced %s to %s", ch->GetName(), victim->GetName(), to_force);
		
		DescriptorData *oldVictDesc = victim->desc;	//	Store victims Desc
		DescriptorData *oldChDesc;
		victim->desc = ch->desc;				//	Switch it to the staff's desc
		
		ch->desc->m_Character = victim;			//	Set staff's desc's character to victim
		ch->desc->m_OrigCharacter = ch;			//	and set the original
		
		ch->desc = NULL;						//	Unlink staff's desc
		
		command_interpreter(victim, to_force);	//	Do the command
		
		ch->desc = victim->desc;				//	Restore staff's desc
		victim->desc = oldVictDesc;				//	Restore vict's desc
		
		ch->desc->m_Character = ch;				//	Reset staff's desc's character
		ch->desc->m_OrigCharacter = NULL;		//	and clear the original linkback
	}
}
#endif

ACMD(do_goto) {
	RoomData *	location;
	BUFFER(buf, MAX_STRING_LENGTH);

	if ((location = find_target_room(ch, argument)) == NULL)
		return;


	if (!ch->GetPlayer()->m_PoofOutMessage.empty())
		sprintf(buf, "$n %s", ch->GetPlayer()->m_PoofOutMessage.c_str());
	else
		strcpy(buf, "$n disappears in a puff of smoke.");

	act(buf, TRUE, ch, 0, 0, TO_ROOM);
	ch->FromRoom();
	ch->ToRoom(location);

	if (!ch->GetPlayer()->m_PoofInMessage.empty())
		sprintf(buf, "$n %s", ch->GetPlayer()->m_PoofInMessage.c_str());
	else
		strcpy(buf, "$n appears with an ear-splitting bang.");

	act(buf, TRUE, ch, 0, 0, TO_ROOM);
	look_at_room(ch);
}



ACMD(do_trans)
{
	CharData *victim;
	BUFFER(buf, MAX_INPUT_LENGTH);
	
	argument = one_argument(argument, buf);
	if (!*buf)
		send_to_char("Whom do you wish to transfer?\n", ch);
	else if (!str_cmp("all", buf)) {
		if (!STF_FLAGGED(ch, STAFF_GAME | STAFF_ADMIN | STAFF_SECURITY)) {
			send_to_char("I think not.\n", ch);
			return;
		}

	    FOREACH(DescriptorList, descriptor_list, iter)
	    {
	    	DescriptorData *i = *iter;

			if (i->GetState()->IsPlaying() && i->m_Character && (i->m_Character != ch)) {
				victim = i->m_Character;
				if ((IS_STAFF(victim) && !STF_FLAGGED(ch, STAFF_ADMIN)) || STF_FLAGGED(victim, STAFF_ADMIN))
					continue;
				act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
				victim->FromRoom();
				victim->ToRoom(IN_ROOM(ch));
				act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
				act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
				look_at_room(victim);
			}
		}
		send_to_char(OK, ch);
	} else if (!str_cmp("team", buf)) {
		int team;
		
		one_argument(argument, buf);
		
		team = search_block(buf, team_names, 0);
		if (team <= 0)
			send_to_char("That is not a team.\n", ch);
		else {
		    FOREACH(DescriptorList, descriptor_list, iter)
		    {
	    		DescriptorData *i = *iter;

				if (i->GetState()->IsPlaying() && i->m_Character && (i->m_Character != ch) &&
						GET_TEAM(i->m_Character) == team) {
					victim = i->m_Character;
					if ((IS_STAFF(victim) && !STF_FLAGGED(ch, STAFF_ADMIN)) || STF_FLAGGED(victim, STAFF_ADMIN))
						continue;
					act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
					victim->FromRoom();
					victim->ToRoom(IN_ROOM(ch));
					act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
					act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
					look_at_room(victim);
				}
			}
			send_to_char(OK, ch);		
		}
	} else {
		if (!(victim = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
			send_to_char(NOPERSON, ch);
		else if (victim == ch)
			send_to_char("That doesn't make much sense, does it?\n", ch);
		else if (ch->GetLevel() < victim->GetLevel() && !STF_FLAGGED(ch, STAFF_ADMIN)) //((IS_STAFF(victim) && !STF_FLAGGED(ch, STAFF_ADMIN) && victim->GetLevel() >= ch->GetLevel()) || STF_FLAGGED(victim, STAFF_ADMIN))
			send_to_char("Go transfer someone your own size.\n", ch);
		else {
			act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
			victim->FromRoom();
			victim->ToRoom(IN_ROOM(ch));
			act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
			act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
			look_at_room(victim);
		}
	}
}



ACMD(do_teleport) {
	CharData *victim;
	RoomData *target;
	BUFFER(buf, MAX_INPUT_LENGTH);
	BUFFER(buf2, MAX_INPUT_LENGTH);
		
	two_arguments(argument, buf, buf2);

	if (!*buf)
		send_to_char("Whom do you wish to teleport?\n", ch);
	else if (!(victim = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
		send_to_char(NOPERSON, ch);
	else if (victim == ch)
		send_to_char("Use 'goto' to teleport yourself.\n", ch);
//	else if (victim->GetLevel() >= ch->GetLevel())
//		send_to_char("Maybe you shouldn't do that.\n", ch);
	else if (ch->GetLevel() < victim->GetLevel() && !STF_FLAGGED(ch, STAFF_ADMIN))  // ((IS_STAFF(victim) && !STF_FLAGGED(ch, STAFF_ADMIN)) || STF_FLAGGED(victim, STAFF_ADMIN))
		send_to_char("Maybe you shouldn't do that.\n", ch);
	else if (!*buf2)
		send_to_char("Where do you wish to send this person?\n", ch);
	else if ((target = find_target_room(ch, buf2))) {
		send_to_char(OK, ch);
		act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
		victim->FromRoom();
		victim->ToRoom(target);
		act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
		act("$n has teleported you!", FALSE, ch, 0, victim, TO_VICT);
		look_at_room(victim);
	}
}



ACMD(do_vnum) {
	BUFFER(buf, MAX_INPUT_LENGTH);
	BUFFER(buf2, MAX_INPUT_LENGTH);
	ZoneData *zone = NULL;
		
	argument = two_arguments(argument, buf, buf2);
	
	if (subcmd == SCMD_ZVNUM)
	{
		zone = zone_table.Find(buf);
		one_argument(argument, buf);
		std::swap(buf, buf2);
	}

	if (!*buf || !*buf2)
	{
		if (subcmd == SCMD_ZVNUM)	ch->Send("Usage: zvnum <zone> { obj | mob | trg } <name>\n");
		else						ch->Send("Usage: vnum { obj | mob | trg } <name>\n");
	}
	else if (is_abbrev(buf, "mob"))
	{
		if (!vnum_mobile(buf2, ch, zone))
			send_to_char("No mobiles by that name.\n", ch);
	}
	else if (is_abbrev(buf, "obj"))
	{
		if (!vnum_object(buf2, ch, zone))
			send_to_char("No objects by that name.\n", ch);
	}
	else if (is_abbrev(buf, "trg") || is_abbrev(buf, "trig"))
	{
		if (!vnum_trigger(buf2, ch, zone))
			send_to_char("No triggers by that name.\n", ch);
	}
	else
		send_to_char("Usage: vnum { obj | mob | trg } <name>\n", ch);
}



void do_stat_room(CharData * ch, RoomData *rm)
{
	int i, found = 0;
	BUFFER(buf, MAX_STRING_LENGTH);

	ch->Send(	"Room name     : `c%s`n\n"
				"     Virtual  : [`g%10s`n]\n"
				"     Type     : %-10s "   "ID       : [%6d]\n",
			rm->GetName(),
			//rm->zone ? (int)rm->zone->GetZoneNumber() : -1,
			rm->GetVirtualID().Print().c_str(),
			GetEnumName(rm->GetSector()), IN_ROOM(ch)->GetID());

	ch->Send(	"     SpecProc : %-10s "   "Script   : %s\n"
				"Room Flags    : %s\n"
				"[Description]\n"
				"%s",
			(rm->func == NULL) ? "None" : "Exists",
			!SCRIPT(rm)->m_Triggers.empty() ? "Exists" : "None",
			rm->GetFlags().print().c_str(),
			rm->GetDescription());

	if (!rm->m_ExtraDescriptions.empty())
	{
		send_to_char("Extra descs   :`c", ch);
		FOREACH(ExtraDesc::List, rm->m_ExtraDescriptions, i)
			ch->Send(" %s", SSData(i->keyword));
		send_to_char("`n\n", ch);
	}

	if (!rm->people.empty()) {
		*buf = found = 0;
		send_to_char(	"[Chars present]\n     `y", ch);
		int num = 0;
		FOREACH(CharacterList, rm->people, iter)
		{
			++num;
			CharData *k = *iter;
			if (ch->CanSee(k) == VISIBLE_NONE)	continue;
			sprintf_cat(buf,	"%s %s(%s)",
					found++ ? "," : "", k->GetName(),
					(!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
			if (strlen(buf) >= 62)
			{
				ch->Send("%s%s\n", buf, num < rm->people.size() ? "," : "");
				*buf = found = 0;
			}
		}
		ch->Send("%s%s`n", buf, found ? "\n" : "");
	}
	
	if (!rm->contents.empty()) {
		*buf = found = 0;
		send_to_char("[Contents]\n     `g", ch);
		int num = 0;
		FOREACH(ObjectList, rm->contents, iter)
		{
			++num;
			ObjData *j = *iter;
			if (!ch->CanSee(j))	continue;
			sprintf_cat(buf, "%s %s", found++ ? "," : "", j->GetName());
			if (strlen(buf) >= 62) {
				ch->Send("%s%s\n",  buf, num < rm->contents.size() ? "," : "");
				*buf = found = 0;
			}
		}
		ch->Send("%s%s`n", buf, found ? "\n" : "");
	}
	
	for (i = 0; i < NUM_OF_DIRS; i++)
	{
		RoomExit *exit = rm->GetExit(i);
		
		if (!exit)
			continue;
		
		if (exit->ToRoom() == NULL)
			strcpy(buf, " `cNONE`n");
		else
			sprintf(buf, "`c%10s`n", exit->ToRoom()->GetVirtualID().Print().c_str());
		ch->Send("Exit `c%-5s`n:  To: [%s], Key: [%10s], Keywrd: %s, ",
				dirs[i], buf, exit->key.Print().c_str(), exit->GetKeyword());
		
		ch->Send("Type: %s\n", exit->GetFlags().print().c_str());
		if (exit->GetStates().any())
			ch->Send("  State: %s\n", exit->GetStates().print().c_str());
		
		ch->Send("%s", *exit->GetDescription() ? exit->GetDescription() : "  No exit description.\n");
	}
}



void do_stat_object(CharData * ch, ObjData * j) {
	BUFFER(buf, MAX_STRING_LENGTH);
	
	ch->Send(	"Name   : \"`y%s`n\"\n"
				"Aliases: %s\n",
			j->m_Name.empty() ? "<None>" : j->GetName(),
			j->GetAlias());
			
	ch->Send(	"     Virtual  : [`g%10s`n]\n"
				"     Type     : %s\n"
				"     SpecProc : %-10s\n"
				"Scripts  Local: %-10s "     "Global   : %s\n"
				"L-Des         : %s\n",
			j->GetVirtualID().Print().c_str(),
			GetEnumName(GET_OBJ_TYPE(j)),
			j->GetPrototype() && j->GetPrototype()->m_Function ? "Exists" : "None",
			!SCRIPT(j)->m_Triggers.empty() ? "Exists" : "None",
				(j->GetPrototype() && !j->GetPrototype()->m_Triggers.empty()) ? "Exists" : "None",
			j->m_RoomDescription.empty() ? "None" : j->GetRoomDescription());

	if (!j->m_ExtraDescriptions.empty())
	{
		send_to_char("Extra descs   :`c", ch);
		FOREACH(ExtraDesc::List, j->m_ExtraDescriptions, i)
			ch->Send(" %s", SSData(i->keyword));
		send_to_char("`n\n", ch);
	}
	
	sprintbit(GET_OBJ_WEAR(j), wear_bits, buf);			ch->Send(	"Can be worn on: %s\n", buf);
	ch->Send(	"Set char bits : %s\n", j->m_AffectFlags.print().c_str());
	sprintbit(GET_OBJ_EXTRA(j), extra_bits, buf);		ch->Send(	"Extra flags   : %s\n", buf);
	ch->Send(	"Unusable By   : %s\n", GetDisplayRaceRestrictionFlags(j->GetRaceRestriction()).c_str());

	ch->Send(	"     Weight   : %-3d (%-3d)  Value    : %d\n"
				"     Timer    : %-5d      Contents : %d\n"
				"     Level    : %-3d        Clan     : %d\n",
			GET_OBJ_WEIGHT(j), GET_OBJ_TOTAL_WEIGHT(j), GET_OBJ_COST(j),
			GET_OBJ_TIMER(j), j->TotalCost() - GET_OBJ_COST(j),
			j->GetMinimumLevelRestriction(), j->GetClanRestriction());
	
	if (j->buyer > 0 && j->bought != 0)
	{
		ch->Send("Buyer    : %s on %s\n", get_name_by_id(j->buyer, "<UNKNOWN>"), Lexi::CreateDateString(j->bought).c_str());
	}
	if (j->owner > 0 && j->InRoom())
	{
		ch->Send("Owner    : %s\n", get_name_by_id(j->owner, "<UNKNOWN>"));
	}

	if(j->InRoom())			ch->Send("In room       : `(%s`) [`c%10s`n]\n", j->InRoom()->GetName(), j->InRoom()->GetVirtualID().Print().c_str());
	else if(j->Inside())	ch->Send("In object     : `(%s`)\n", j->Inside()->GetName());
	else if(j->CarriedBy())	ch->Send("Carried by    : `(%s`)\n", j->CarriedBy()->GetName());
	else if(j->WornBy())	ch->Send("Worn by       : `(%s`)\n", j->WornBy()->GetName());

	send_to_char("[Values]\n", ch);

	switch (GET_OBJ_TYPE(j)) {
		case ITEM_LIGHT:
			if (j->GetValue(OBJVAL_LIGHT_HOURS) == -1)	ch->Send("     Hours left: Infinite");
			else										ch->Send("     Hours left: %d", j->GetValue(OBJVAL_LIGHT_HOURS));
			break;
		case ITEM_THROW:
		case ITEM_BOOMERANG:
			ch->Send("     Damage   : %-3d        Type     : %s\n"
					 "     Range    : %-1d          Skill    : %s\n"
					 "     Speed    : %.2f         Str Bonus: x%.2f",
					j->GetValue(OBJVAL_THROWABLE_DAMAGE),		findtype(j->GetValue(OBJVAL_THROWABLE_DAMAGETYPE), damage_types),
					j->GetValue(OBJVAL_THROWABLE_RANGE),		skill_name(j->GetValue(OBJVAL_THROWABLE_SKILL)),
					j->GetFloatValue(OBJFVAL_THROWABLE_SPEED),	j->GetFloatValue(OBJFVAL_THROWABLE_STRENGTH));
			break;
		case ITEM_GRENADE:
			ch->Send("     Damage   : %-3d        Type     : %s\n"
					"      Timer    : %d",
					j->GetValue(OBJVAL_GRENADE_DAMAGE), findtype(j->GetValue(OBJVAL_GRENADE_DAMAGETYPE), damage_types),
					j->GetValue(OBJVAL_GRENADE_TIMER));
			break;
		case ITEM_WEAPON:
			ch->Send("     Damage   : %-3d        Speed    : %.2f\n"
					 "     Type     : %-10.10s Str Bonus: x%.2f\n"
					 "     Skill    : %-10.10s Skill2   : %s\n"
					 "     Attack   : %-10.10s Rate/Fire: %d\n"
					 "Balance - DPS : %-4d       DPA      : %d",
					j->GetValue(OBJVAL_WEAPON_DAMAGE),								j->GetFloatValue(OBJFVAL_WEAPON_SPEED),
					findtype(j->GetValue(OBJVAL_WEAPON_DAMAGETYPE), damage_types),	j->GetFloatValue(OBJFVAL_WEAPON_STRENGTH), 
					skill_name(j->GetValue(OBJVAL_WEAPON_SKILL)),
						j->GetValue(OBJVAL_WEAPON_SKILL2) ? skill_name(j->GetValue(OBJVAL_WEAPON_SKILL2)) : "None Set",
					findtype(j->GetValue(OBJVAL_WEAPON_ATTACKTYPE), attack_types), j->GetValue(OBJVAL_WEAPON_RATE),
					(int)((j->GetValue(OBJVAL_WEAPON_DAMAGE) * MAX(1, j->GetValue(OBJVAL_WEAPON_RATE)) * DAMAGE_DICE_SIZE / 10) / j->GetFloatValue(OBJFVAL_WEAPON_SPEED)),
							j->GetValue(OBJVAL_WEAPON_DAMAGE) * MAX(1, j->GetValue(OBJVAL_WEAPON_RATE)) * DAMAGE_DICE_SIZE / 2);


			if (IS_GUN(j)) {
				ch->Send("\nGun settings:\n"
						 "     Damage   : %-3d        Type     : %s\n"
						 "     Attack   : %-10.10s Rate/Fire: %d\n"
						 "     Range    : %-3d        Optimal  : %d\n"
						 "     Skill    : %-10.10s Skill2    : %s\n"
						 "     Ammo Type: %s",
						GET_GUN_DAMAGE(j),									findtype(GET_GUN_DAMAGE_TYPE(j), damage_types),
						findtype(GET_GUN_ATTACK_TYPE(j), attack_types),		GET_GUN_RATE(j),
						GET_GUN_RANGE(j),									GET_GUN_OPTIMALRANGE(j),
						skill_name(GET_GUN_SKILL(j)),						GET_GUN_SKILL2(j) ? skill_name(GET_GUN_SKILL2(j)) : "None Set",
						GET_GUN_AMMO_TYPE(j) == -1 ? "<NONE>" : findtype(GET_GUN_AMMO_TYPE(j), ammo_types));
				if (GET_GUN_AMMO(j) > 0) {
					sprintbit(GET_GUN_AMMO_FLAGS(j), ammo_bits, buf);
					ch->Send("\n"
							"     Ammo     : %-3d        Virtual  : %s\n"
							"     AmmoFlags: %s",
							GET_GUN_AMMO(j), GET_GUN_AMMO_VID(j).Print().c_str(),
							buf);
				}
				ch->Send("\n"
					 	"Balance - DPS : %-4d       DPA      : %d",
						(int)((GET_GUN_DAMAGE(j) * MAX(1, GET_GUN_RATE(j)) * DAMAGE_DICE_SIZE / 10) / j->GetFloatValue(OBJFVAL_WEAPON_SPEED)),
								GET_GUN_DAMAGE(j) * MAX(1, GET_GUN_RATE(j)) * DAMAGE_DICE_SIZE / 2);
			}
			break;
		case ITEM_AMMO:
			ch->Send("     Ammo     : %-4d       Ammo Type: %s", j->GetValue(OBJVAL_AMMO_AMOUNT), findtype(j->GetValue(OBJVAL_AMMO_TYPE), ammo_types));
			break;
		case ITEM_ARMOR:
			for (int i = 0; i < NUM_DAMAGE_TYPES; ++i)
				ch->Send("Absorb %-7.7s: %d%%%s", damage_types[i], j->GetValue((ObjectTypeValue)i), i < (NUM_DAMAGE_TYPES - 1) ? "\n" : "");
			break;
		case ITEM_CONTAINER:
			sprintbit(j->GetValue(OBJVAL_CONTAINER_FLAGS), container_bits, buf);
			ch->Send("     MaxWeight: %-4d       Lock Type: %s\n"
					 "     Key      : %s\n"
					 "     Corpse   : %s",
					j->GetValue(OBJVAL_CONTAINER_CAPACITY), buf,
					j->GetVIDValue(OBJVIDVAL_CONTAINER_KEY).Print().c_str(),
					YESNO(j->GetValue(OBJVAL_CONTAINER_CORPSE)));
			break;
		case ITEM_DRINKCON:
		case ITEM_FOUNTAIN:
			ch->Send("     Capacity : %-5d      Contains : %d\n"
					 "     Poisoned : %-3s        Liquid   : %s",
					j->GetValue(OBJVAL_FOODDRINK_FILL), j->GetValue(OBJVAL_FOODDRINK_CONTENT),
					YESNO(j->GetValue(OBJVAL_FOODDRINK_TYPE)), findtype(j->GetValue(OBJVAL_FOODDRINK_POISONED), drinks));
			break;
		case ITEM_NOTE:
		case ITEM_PEN:
		case ITEM_KEY:
			send_to_char("[NONE]", ch);
			break;
		case ITEM_FOOD:
			ch->Send("     Fill     : %-5d      Poisoned : %s",
					j->GetValue(OBJVAL_FOODDRINK_FILL), YESNO(j->GetValue(OBJVAL_FOODDRINK_POISONED)));
			break;
		case ITEM_BOARD:
			ch->Send("     Levels\n"
					 "     Read     : %-5d      Write    : %-5d\n"
					 "     Remove   : %-5d",
					j->GetValue(OBJVAL_BOARD_READLEVEL), j->GetValue(OBJVAL_BOARD_WRITELEVEL),
					j->GetValue(OBJVAL_BOARD_REMOVELEVEL));
			break;
		case ITEM_VEHICLE:
			ch->Send("     EntryRoom: %s\n"//"      Rooms    : %-5d-%-5d\n"
					 "     Flags    : %-5d\n"
			         "     Size     : %-5d",
			         j->GetVIDValue(OBJVIDVAL_VEHICLE_ENTRY).Print().c_str(), //j->GetValue(OBJVAL_VEHICLE_STARTROOM), j->GetValue(OBJVAL_VEHICLE_ENDROOM),
			         j->GetValue(OBJVAL_VEHICLE_FLAGS),
			         j->GetValue(OBJVAL_VEHICLE_SIZE));
			break;
		
		case ITEM_VEHICLE_HATCH:
		case ITEM_VEHICLE_CONTROLS:
		case ITEM_VEHICLE_WINDOW:
		case ITEM_VEHICLE_WEAPON:
			ch->Send("     Vehicle  : %s",
			         j->GetVIDValue(OBJVIDVAL_VEHICLEITEM_VEHICLE).Print().c_str());
			break;
		
		case ITEM_INSTALLABLE:
			{
				VirtualID vid = j->GetVIDValue(OBJVIDVAL_INSTALLABLE_INSTALLED);
				ObjectPrototype *proto = obj_index.Find(vid);
				ch->Send("     Object   : %s [%s]\n"
						 "     Time     : %d\n"
						 "     Skill    : %s",
				        vid.Print().c_str(), proto ? proto->m_pProto->GetName() : "NONE",
						j->GetValue(OBJVAL_INSTALLABLE_TIME),
						j->GetValue(OBJVAL_INSTALLABLE_SKILL) == 0 ? "None" : skill_name(j->GetValue(OBJVAL_INSTALLABLE_SKILL)));
			}
			break;

		case ITEM_INSTALLED:
			ch->Send(
					"     Time     : %d\n"
					"     Permanent: %s",
			        j->GetValue(OBJVAL_INSTALLED_TIME),
					YESNO(j->GetValue(OBJVAL_INSTALLED_PERMANENT)));
			break;
			
		case ITEM_TOKEN:
			ch->Send(
					"     Value    : %d\n"
					"     Set      : %s\n"
					"     Signed by: %s",
					j->GetValue(OBJVAL_TOKEN_VALUE),
					j->GetValue(OBJVAL_TOKEN_SET) >= 0 && j->GetValue(OBJVAL_TOKEN_SET) < NUM_TOKEN_SETS + 1 ?
							token_sets[j->GetValue(OBJVAL_TOKEN_SET)] : "<INVALID SET>",
					get_name_by_id(j->GetValue(OBJVAL_TOKEN_SIGNER), "<NOT SIGNED>"));
			break;
			
		default:
			ch->Send("     Value 0  : %-10d Value 1  : %d\n"
					 "     Value 2  : %-10d Value 3  : %d\n"
					 "     Value 4  : %-10d Value 5  : %d\n"
					 "     Value 6  : %-10d Value 7  : %d",
					j->GetValue((ObjectTypeValue)0), j->GetValue((ObjectTypeValue)1),
					j->GetValue((ObjectTypeValue)2), j->GetValue((ObjectTypeValue)3),
					j->GetValue((ObjectTypeValue)4), j->GetValue((ObjectTypeValue)5),
					j->GetValue((ObjectTypeValue)6), j->GetValue((ObjectTypeValue)7));
			break;
	}
	send_to_char("\n", ch);

	int found = 0;
	if (!j->contents.empty())
	{
		strcpy(buf, "\nContents:`g");
		*buf = 0;
		int num = 0;
		FOREACH(ObjectList, j->contents, iter)
		{
			++num;
			ObjData *j2 = *iter;
			sprintf_cat(buf, "%s %s", found++ ? "," : "", j2->GetName());
			if (strlen(buf) >= 62)
			{
				ch->Send("%s%s\n", buf, num < j->contents.size() ? "," : "");
				*buf = 0;
				found = 0;
			}
		}
		ch->Send("%s%s`n", buf, found ? "\n" : "");
	}

/*
	if (j->contains) {
		send_to_char("\nContents:`g", ch);
		len = 10;
		for (found = 0, j2 = j->contains; j2; j2 = j2->next_content) {
			ch->Send("%s %s", found++ ? "," : "", j2->GetName());
			len += strlen(j2->GetName()) + 2;
			if (len >= 62) {
				if (j2->next_content)	send_to_char(",\n", ch);
				else					send_to_char("\n", ch);
				found = 0;
				len = 0;
			}
		}
		if (len)	send_to_char("\n", ch);
		send_to_char("`n", ch);
	}
*/	

	found = 0;
	send_to_char("Affections:", ch);
	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (!j->affect[i].modifier)
			continue;
		
		if (j->affect[i].location < 0)	sprintf(buf, "Skill: %s", skill_name(-j->affect[i].location));
		else							strcpy(buf, GetEnumName((AffectLoc)j->affect[i].location));
		ch->Send("%s %+d to %s", found++ ? "," : "", j->affect[i].modifier, buf);
	}
	ch->Send("%s\n", found ? "" : " None");
	
	if (j->GetPrototype() && j->GetPrototype()->m_pProto == j)
	{
		BUFFER(name, MAX_STRING_LENGTH);
		BUFFER(buf, MAX_STRING_LENGTH);
		
		if (!j->GetPrototype()->m_BehaviorSets.empty())
		{
			ch->Send("Behavior Sets:");
			FOREACH(BehaviorSets, j->GetPrototype()->m_BehaviorSets, bset)
			{
				BehaviorSet *behavior = *bset;
				
				ch->Send(" %s", (*bset)->GetName());
			}
		}
		
		if (!j->GetPrototype()->m_Triggers.empty())
		{
			ch->Send("Scripts\n");
			FOREACH(ScriptVector, j->GetPrototype()->m_Triggers, trigVid)
			{
				ScriptPrototype *proto = trig_index.Find(*trigVid);
				
				TrigData * trig = proto ? proto->m_pProto : NULL;
				
				ch->Send("    [`g%10s`n] `c%s`n\n", trigVid->Print().c_str(), trig ? trig->GetName() : "`y<INVALID>");
				
				if (trig)
				{
					char **trig_types;
					
					switch (GET_TRIG_DATA_TYPE(trig)) {
						case WLD_TRIGGER:	trig_types = wtrig_types;	break;
						case OBJ_TRIGGER:	trig_types = otrig_types;	break;
						case MOB_TRIGGER:	trig_types = mtrig_types;	break;
						case GLOBAL_TRIGGER:trig_types = gtrig_types;	break;
						default:	continue;
					}
					
					sprintbit(GET_TRIG_TYPE(trig), trig_types, buf);
					ch->Send("        Type: %s%s%s%s; N Arg: %d; Arg list: %s\n",
							buf,
							trig->m_Flags.any() ? " (" : "", trig->m_Flags.any() ? trig->m_Flags.print().c_str() : "", trig->m_Flags.any() ? ")" : "",
							GET_TRIG_NARG(trig), 
							trig->m_Arguments.c_str());
				}

			}
		}
		
		if (!j->GetPrototype()->m_InitialGlobals.empty())
		{
			ch->Send("Variables\n");
			FOREACH(ScriptVariable::List, j->GetPrototype()->m_InitialGlobals, var)
			{
				strcpy(name, (*var)->GetName());
				if ((*var)->GetContext())	sprintf_cat(name, "{%d}", (*var)->GetContext());
				
				const char *value = (*var)->GetValue();
				if (IsIDString(value))
				{
					extern void find_uid_name(const char *uid, char *name);
					find_uid_name(value, buf);
					value = buf;
				}
				
				ch->Send("    %-15s:  %s\n", name, value);
			}
		}
	}
}


void do_stat_character(CharData * ch, CharData * k) {
	int i, i2;
	BUFFER(buf, MAX_STRING_LENGTH);


	switch (k->GetSex())
	{
		case SEX_NEUTRAL:	strcpy(buf, "NEUTRAL-SEX");		break;
		case SEX_MALE:		strcpy(buf, "MALE");			break;
		case SEX_FEMALE:	strcpy(buf, "FEMALE");			break;
		default:			strcpy(buf, "ILLEGAL-SEX!!");	break;
	}

	ch->Send("%s %s '%s'  IDNum: [%5d], In room [%10s]\n",
			buf, (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
			k->GetName(),
			(!IS_NPC(k) ? k->GetID() : 0), k->InRoom() ? k->InRoom()->GetVirtualID().Print().c_str() : "<NOWHERE>");

	if (IS_MOB(k))
		ch->Send("Alias: %s, Virtual: [%10s]\n",
				k->GetAlias(), k->GetVirtualID().Print().c_str());

	if (IS_STAFF(k) && !k->GetPlayer()->m_Pretitle.empty())
		ch->Send("Pretitle: %s\n", k->GetPlayer()->m_Pretitle.c_str());
	ch->Send(	"Title: %s\n"
				"L-Des: %s"
				"Race: [`y%s`n], Lev: [`y%2d`n]\n",
				k->GetTitle(),
				k->m_RoomDescription.empty() ? "<None>\n" : k->m_RoomDescription.c_str(),
				findtype(k->GetRace(), race_abbrevs), k->GetLevel());

	if (!IS_NPC(k)) {
		int played_time = (time(0) - k->GetPlayer()->m_LoginTime) + k->GetPlayer()->m_TotalTimePlayed;
		
		ch->Send(	"Created: [%s], Last Logon: [%s], Played [%dh %dm], Age [%d]\n"
					"Pracs: %d (Lifetime: %d)\n"
					"Karma: %d\n",
				Lexi::CreateDateString(k->GetPlayer()->m_Creation, "%a %b %e %Y").c_str(),
				Lexi::CreateDateString(k->GetPlayer()->m_LastLogin, "%a %b %e %Y").c_str(),
				played_time / 3600, ((played_time % 3600) / 60), age(k).year,
				k->GetPlayer()->m_SkillPoints, k->GetPlayer()->m_LifetimeSkillPoints,
				k->GetPlayer()->m_Karma);
				
		if (k->desc)
		{
			ch->Send("Terminal: \"%s\"%s [`c%dx%d`n]%s\n",
				k->desc->m_Terminal.c_str(), k->desc->m_bWindowsTelnet ? " (Windows Telnet)" : "",
				k->desc->m_Width, k->desc->m_Height, k->desc->m_bForceEcho ? " +ECHO" : "");
			ch->Send("    EndOfRecord: `y%-3s`n     Compression: `y%3s`n/`y%-3s`n         MXP: `y%-3s`n/`y%-3s`n\n",
				YESNO(k->desc->m_bDoEOROnPrompt),
				YESNO(k->desc->CompressionSupported()), ONOFF(k->desc->CompressionActive()),
				YESNO(k->desc->m_bMXPSupported), ONOFF(0));
		}
	}
	ch->Send(	"Strength:     [%3d]     Agility:      [%3d]     Health:       [%3d]\n"
				"Coordination: [%3d]     Perception:   [%3d]     Knowledge:    [%3d]\n",
			GET_STR(k), GET_AGI(k), GET_HEA(k),
			GET_COO(k), GET_PER(k), GET_KNO(k));
	
	ch->Send("Armor:    ");
	for (int i = 0; i < NUM_DAMAGE_TYPES; ++i)
		ch->Send("  %-3.3s", damage_type_abbrevs[i]);
	ch->Send("\n");
	
	int startingArmorLoc = 0;
	int endingArmorLoc = -1;
	
	if (IS_NPC(k))
	{
		startingArmorLoc = ARMOR_LOC_NATURAL;
		endingArmorLoc = ARMOR_LOC_NATURAL;
	}
	else
	{
		startingArmorLoc = ARMOR_LOC_BODY;
		endingArmorLoc = ARMOR_LOC_HEAD;
		
		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPECIAL_MISSION))
		{
			endingArmorLoc = ARMOR_LOC_MISSION;
		}
	}
	
	for (int loc = startingArmorLoc; loc <= endingArmorLoc; ++loc)
	{
		ch->Send("   %-8.8s", damage_locations[loc]);
		for (int i = 0; i < NUM_DAMAGE_TYPES; ++i)
			ch->Send(" %3d%%", GET_ARMOR(k, loc, i));
		ch->Send("\n");
	}
	
	if (IS_NPC(k) && k->GetPrototype() && k->GetPrototype()->m_pProto == k)
	{
		ch->Send(	"Hit p.:[`G%d+%d`n]\n",
			GET_MOB_BASE_HP(k), GET_MOB_VARY_HP(k));
	}
	else
	{
		ch->Send(	"Hit p.:[`G%d/%d+%d`n]  Move p.:[`G%d/%d+%d`n]" /*"  Endurance p.:[`g%d/%d+%d`n]"*/ "\n",
			GET_HIT(k), GET_MAX_HIT(k), hit_gain(k),
			GET_MOVE(k), GET_MAX_MOVE(k), move_gain(k)
			/*GET_ENDURANCE(k), GET_MAX_ENDURANCE(k), endurance_gain(k)*/);
	}


	ch->Send(	"Mission Points: %d (Total: %d)\n"
				"Pos: %s, Fighting: %s",
			GET_MISSION_POINTS(k),
			k->points.lifemp,
			GetEnumName(GET_POS(k)), FIGHTING(k) ? FIGHTING(k)->GetName() : "Nobody");

	if (IS_NPC(k))
		ch->Send(", Attack type: %s", findtype(k->mob->m_AttackType, attack_types));

	if (k->desc)
		ch->Send(", Connected: %s", k->desc->GetState()->GetName().c_str());

	if (IS_NPC(k))
	{
		ch->Send("\nDefault position: %s\n", GetEnumName(k->mob->m_DefaultPosition));
		sprintbit(MOB_FLAGS(k), action_bits, buf);
		ch->Send("NPC flags: `c%s`n\n", buf);
	}
	else
	{
		ch->Send("\nIdle Timer (in tics) [%d]\n", k->GetPlayer()->m_IdleTime);
		
		int tempP = k->GetPlayer()->m_PlayerKills,
			tempM = k->GetPlayer()->m_MobKills,
			tempT = tempP + tempM;
		ch->Send("Killed %d beings (%d (%2d%%) players, %d (%2d%%) mobs).\n",
				tempP + tempM, tempP, tempT ? (tempP * 100)/tempT : 0,
				tempM, tempT ? (tempM * 100)/tempT : 0);
		
		tempP = k->GetPlayer()->m_PlayerDeaths;
		tempM = k->GetPlayer()->m_MobDeaths;
		tempT = tempP + tempM;
		ch->Send("Been killed by %d beings (%d (%2d%%) players, %d (%2d%%) mobs).\n",
				tempP + tempM, tempP, tempT ? (tempP * 100)/tempT : 0,
				tempM, tempT ? (tempM * 100)/tempT : 0);

		sprintbit(PLR_FLAGS(k), player_bits, buf);			ch->Send("PLR: `c%s`n\n", buf);
		sprintbit(PRF_FLAGS(k), preference_bits, buf);		ch->Send("PRF: `g%s`n\n", buf);
		sprintbit(STF_FLAGS(k), staff_bits, buf);			ch->Send("STAFF: `g%s`n\n", buf);
	}

	if (IS_MOB(k))	//	IS_MOB implies GetPrototype() != NULL
	{
		ch->Send(	"Mob Spec-Proc : %s\n"
					"Scripts  Local: %-10s   ""Global   : %s\n",
//					"NPC Bare Hand Dam: %dd%d\n",
				k->GetPrototype()->m_Function ? "Exists" : "None",
				!SCRIPT(k)->m_Triggers.empty() ? "Exists" : "None",
				!k->GetPrototype()->m_Triggers.empty() ? "Exists" : "None"
				/*, k->mob->damage.number, k->mob->damage.size*/);
	}
	
	i = k->carrying.size();
	ch->Send("Carried: weight: %d, items: %d; Items in: inventory: %d, ",
			IS_CARRYING_W(k), IS_CARRYING_N(k), i);

	for (i = 0, i2 = 0; i < NUM_WEARS; i++)
		if (GET_EQ(k, i))
			i2++;
	
	ch->Send(	"eq: %d\n"
//				"Hunger: %d, Thirst: %d, Drunk: %d\n"
				"Master is: %s, Followers are:",
			i2,
//			GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK),
			(k->m_Following ? k->m_Following->GetName() : "<none>"));

	if (!k->m_Followers.empty())
	{
		*buf = 0;
		int found = 0;
		
		FOREACH(CharacterList, k->m_Followers, iter)
		{
			CharData *follower = *iter;
			++found;

			sprintf_cat(buf, "%s %s", *buf ? "," : "", PERS(follower, ch));
			
			if (strlen(buf) >= 62)
			{
				ch->Send("%s%s\n", buf, found < k->m_Followers.size() ? "," : "");
				*buf = 0;
				found = 0;
			}
		}

		ch->Send("%s%s`n", buf, found ? "\n" : "");
	}
	else
		ch->Send("\n");
	
	/* Showing the bitvector */
	ch->Send("AFF: `y%s`n\n", AFF_FLAGS(k).print().c_str());

	if (IS_NPC(k) && k->mob->m_Hates.IsActive()) {
		CharData *vict;
		send_to_char("Pissed List:\n", ch);
		FOREACH(std::list<IDNum>, k->mob->m_Hates.m_Characters, id)
		{
			vict = CharData::Find(*id);
			if (vict)	ch->Send("  %s\n", vict->GetName());
		}
		send_to_char("\n", ch);
	}

	/* Routine to show what spells a char is affected by */
	if (!k->m_Affects.empty())
	{
		FOREACH(Lexi::List<Affect *>, k->m_Affects, iter)
		{
			Affect *aff = *iter;
	
			ch->Send("SKL: (%3ds) `c%-21s`n ",
					aff->GetTime() / PASSES_PER_SEC, 
					aff->GetType());
			if (aff->GetModifier())
				ch->Send("%+d to %s", aff->GetModifier(), aff->GetLocationName());
			if (aff->GetFlags().any())
			{
				ch->Send("%ssets %s", aff->GetModifier() ? ", " : "", aff->GetFlags().print().c_str());
			}
			send_to_char("\n", ch);
		}
	}
	
	if (k->GetPrototype() && k->GetPrototype()->m_pProto == k)
	{
		BUFFER(name, MAX_STRING_LENGTH);
		BUFFER(buf, MAX_STRING_LENGTH);
		
		if (!k->GetPrototype()->m_Triggers.empty())
		{
			ch->Send("Scripts\n");
			FOREACH(ScriptVector, k->GetPrototype()->m_Triggers, trigVid)
			{
				ScriptPrototype *proto = trig_index.Find(*trigVid);
				
				TrigData * trig = proto ? proto->m_pProto : NULL;
				
				ch->Send("    [`g%10s`n] `c%s`n\n", trigVid->Print().c_str(), trig ? trig->GetName() : "`y<INVALID>");
				
				if (trig)
				{
					char **trig_types;
					
					switch (GET_TRIG_DATA_TYPE(trig)) {
						case WLD_TRIGGER:	trig_types = wtrig_types;	break;
						case OBJ_TRIGGER:	trig_types = otrig_types;	break;
						case MOB_TRIGGER:	trig_types = mtrig_types;	break;
						case GLOBAL_TRIGGER:trig_types = gtrig_types;	break;
						default:	continue;
					}
					
					sprintbit(GET_TRIG_TYPE(trig), trig_types, buf);
					ch->Send("        Type: %s%s%s%s; N Arg: %d; Arg list: %s\n",
							buf,
							trig->m_Flags.any() ? " (" : "", trig->m_Flags.any() ? trig->m_Flags.print().c_str() : "", trig->m_Flags.any() ? ")" : "",
							GET_TRIG_NARG(trig), 
							trig->m_Arguments.c_str());
				}

			}
		}
		
		if (!k->GetPrototype()->m_InitialGlobals.empty())
		{
			ch->Send("Variables\n");
			FOREACH(ScriptVariable::List, k->GetPrototype()->m_InitialGlobals, var)
			{
				strcpy(name, (*var)->GetName());
				if ((*var)->GetContext())	sprintf_cat(name, "{%d}", (*var)->GetContext());
				
				const char *value = (*var)->GetValue();
				if (IsIDString(value))
				{
					extern void find_uid_name(const char *uid, char *name);
					find_uid_name(value, buf);
					value = buf;
				}
				
				ch->Send("    %-15s:  %s\n", name, value);
			}
		}
	}
}


ACMD(do_stat) {
	CharData *victim = 0;
	ObjData *object = 0;
	BUFFER(buf1, MAX_INPUT_LENGTH);
	BUFFER(buf2, MAX_INPUT_LENGTH);

	half_chop(argument, buf1, buf2);

	if (!*buf1)
		send_to_char("Stats on who or what?\n", ch);
	else if (is_abbrev(buf1, "room"))
		if (subcmd == SCMD_STAT)	do_stat_room(ch, ch->InRoom());
		else						do_sstat_room(ch, ch->InRoom());
	else if (is_abbrev(buf1, "mob")) {
		if (!*buf2)
			send_to_char("Stats on which mobile?\n", ch);
		else if (!(victim = get_char_vis(ch, buf2, FIND_CHAR_WORLD)))
			send_to_char("No such mobile around.\n", ch);
	} else if (is_abbrev(buf1, "player")) {
		if (!*buf2)
			send_to_char("Stats on which player?\n", ch);
		else if (!(victim = get_player_vis(ch, buf2)))
			send_to_char("No such player around.\n", ch);
	} else if (is_abbrev(buf1, "file")) {
		if (!*buf2)
			send_to_char("Stats on which player?\n", ch);
		else {
			victim = new CharData();
			if (!victim->Load(buf2))
				send_to_char("There is no such player.\n", ch);
			else
			{
				if (IS_STAFF(victim) && !STF_FLAGGED(ch, STAFF_SECURITY | STAFF_ADMIN))
					send_to_char("Sorry, you can't do that.\n", ch);
				else
					do_stat_character(ch, victim);
			}
			delete victim;
			victim = NULL;
		}
	} else if (is_abbrev(buf1, "object")) {
		if (!*buf2)
			send_to_char("Stats on which object?\n", ch);
		else if (!(object = get_obj_vis(ch, buf2)))
			send_to_char("No such object around.\n", ch);
	} else {
		if ((victim = get_char_vis(ch, buf1, FIND_CHAR_WORLD))) ;
		else if ((object = get_obj_vis(ch, buf1))) ;
		else send_to_char("Nothing around by that name.\n", ch);
	}
	if (object) {
		if (subcmd == SCMD_STAT)	do_stat_object(ch, object);
		else						do_sstat_object(ch, object);
	} else if (victim) {
		if (subcmd == SCMD_STAT)	do_stat_character(ch, victim);
		else						do_sstat_character(ch, victim);
	}
}


ACMD(do_shutdown) {
	BUFFER(arg, MAX_INPUT_LENGTH);

	one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char("To shutdown, please type \"shutdown yes\".\n", ch);
	}
	else if (!str_cmp(arg, "yes")) {
		log("(GC) Shutdown by %s.", ch->GetName());
		send_to_all("Shutting down.\n");
		circle_shutdown = 1;
	} else if (!str_cmp(arg, "reboot")) {
		log("(GC) Reboot by %s.", ch->GetName());
		send_to_all("Rebooting.. come back in a minute or two.\n");
		touch(FASTBOOT_FILE);
		circle_shutdown = circle_reboot = 1;
	} else if (!str_cmp(arg, "die")) {
		log("(GC) Shutdown by %s.", ch->GetName());
		send_to_all("Shutting down for maintenance.\n");
		touch(KILLSCRIPT_FILE);
		circle_shutdown = 1;
	} else if (!str_cmp(arg, "pause")) {
		log("(GC) Shutdown by %s.", ch->GetName());
		send_to_all("Shutting down for maintenance.\n");
		touch(PAUSE_FILE);
		circle_shutdown = 1;
	} else
		send_to_char("Unknown shutdown option.\n", ch);
}


void stop_snooping(CharData * ch)
{
	if (!ch->desc->m_Snooping)
		send_to_char("You aren't snooping anyone.\n", ch);
	else
	{
		mudlogf(BRF, ch->GetLevel(), true, "(GC) %s stops snooping %s.", ch->GetName(), ch->desc->m_Snooping->m_Character->GetName());
		send_to_char("You stop snooping.\n", ch);
		ch->desc->m_Snooping->m_SnoopedBy.remove(ch->desc);
		ch->desc->m_Snooping = NULL;
	}
}


static bool CheckForCircularSnoop(DescriptorData *first, DescriptorData *check)
{
	while (first)
	{
		if (first->m_Snooping == check)
			return true;
		first = first->m_Snooping;
	}
	
	return false;
}


ACMD(do_snoop) {
	CharData *victim, *tch;
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	if (!ch->desc)
		return;
		
	one_argument(argument, arg);

	if (!*arg)
		stop_snooping(ch);
	else if (!(victim = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
		send_to_char("No such person around.\n", ch);
	else if (!victim->desc)
		send_to_char("There's no link.. nothing to snoop.\n", ch);
	else if (victim == ch)
		stop_snooping(ch);
//	else if (victim->desc->snoop_by)
//		send_to_char("Busy already. \n", ch);
	else if (CheckForCircularSnoop(victim->desc, ch->desc))
		send_to_char("Don't be stupid.\n", ch);
	else {
		tch = (victim->desc->m_OrigCharacter ? victim->desc->m_OrigCharacter : victim);

		if (/*IS_STAFF(victim) && !STF_FLAGGED(ch, STAFF_ADMIN)*/
			tch->GetLevel() >= ch->GetLevel())
			send_to_char("You can't.\n", ch);
		else
		{
			send_to_char(OK, ch);
			
			if (ch->desc->m_Snooping)
				ch->desc->m_Snooping->m_SnoopedBy.remove(ch->desc);
            
			mudlogf(BRF, ch->GetLevel(), true, "(GC) %s starts snooping %s.", ch->GetName(), tch->GetName());
            
			ch->desc->m_Snooping = victim->desc;
			victim->desc->m_SnoopedBy.push_back(ch->desc);
		}
	}
}



ACMD(do_switch) {
	CharData *victim;
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	one_argument(argument, arg);

	if (ch->desc->m_OrigCharacter)
		send_to_char("You're already switched.\n", ch);
	else if (!*arg)
		send_to_char("Switch with who?\n", ch);
	else if (!(victim = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
		send_to_char("No such character.\n", ch);
	else if (ch == victim)
		send_to_char("Hee hee... we are jolly funny today, eh?\n", ch);
	else if (victim->desc)
		send_to_char("You can't do that, the body is already in use!\n", ch);
	else if (!STF_FLAGGED(ch, STAFF_ADMIN | STAFF_SECURITY | STAFF_CHAR) && !IS_NPC(victim))
		send_to_char("You don't have the clearance to use a mortal's body.\n", ch);
	else if (IS_STAFF(victim) && !STF_FLAGGED(ch, STAFF_ADMIN))
		send_to_char("You don't have the clearance to use that person's body.\n", ch);
	else
	{
		send_to_char(OK, ch);

		ch->desc->m_Character = victim;
		ch->desc->m_OrigCharacter = ch;

		victim->desc = ch->desc;
		ch->desc = NULL;
	}
}


ACMD(do_return) {
	if (ch->desc && ch->desc->m_OrigCharacter) {
		send_to_char("You return to your original body.\n", ch);

		/* JE 2/22/95 */
		/* if someone switched into your original body, disconnect them */
		if (ch->desc->m_OrigCharacter->desc)
			ch->desc->m_OrigCharacter->desc->GetState()->Push(new DisconnectConnectionState);

		ch->desc->m_Character = ch->desc->m_OrigCharacter;
		ch->desc->m_OrigCharacter = NULL;

		ch->desc->m_Character->desc = ch->desc;
		ch->desc = NULL;
	}
}



ACMD(do_load) {
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
			
	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2 )
		send_to_char("Usage: load { obj | mob } <number>\n", ch);
	else if (is_abbrev(arg1, "mob"))
	{
		CharacterPrototype *proto = mob_index.Find(VirtualID(arg2, ch->InRoom()->GetVirtualID().zone));
		
		if (!proto)
			send_to_char("There is no mobile with that number.\n", ch);
		else {
			CharData *mob = CharData::Create(proto);
			mob->ToRoom(IN_ROOM(ch));

			act("$n makes a quaint, magical gesture with one hand.", TRUE, ch, 0, 0, TO_ROOM);
			act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
			act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);
			log("(GC) %s loaded %s.", ch->GetName(), mob->GetName());
			
			finish_load_mob(mob);
		}
	}
	else if (is_abbrev(arg1, "obj"))
	{
		ObjectPrototype *proto = obj_index.Find(VirtualID(arg2, ch->InRoom()->GetVirtualID().zone));
		
		if (!proto)
			send_to_char("There is no object with that number.\n", ch);
		else
		{
			ObjData *obj = ObjData::Create(proto);
			if (obj->wear)	obj->ToChar(ch);
			else 			obj->ToRoom(IN_ROOM(ch));
			act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
			act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
			act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
			log("(GC) %s loaded %s.", ch->GetName(), obj->GetName());
			load_otrigger(obj);
		}
	}
	else if (!str_cmp(arg1, "player") && STF_FLAGGED(ch, STAFF_ADMIN))
	{
		CharData *	target;
		IDNum		id = get_id_by_name(arg2);

		if (!id)
			ch->Send("Unknown player.\n");
		else if ((target = CharData::Find(id)))
		{
			if (GET_REAL_LEVEL(ch) < target->GetPlayer()->m_StaffInvisLevel
				|| (IS_STAFF(target)
					&& !(STF_FLAGGED(target, STAFF_ADMIN) ? ch->GetLevel() == LVL_CODER : STF_FLAGGED(ch, STAFF_ADMIN))))
				ch->Send("Insufficient privileges to load '%s'.\n", arg2);
			else
				ch->Send("%s is already in the game.\n", target->GetName());
		}
		else
		{
			target = new CharData();

			//	LOAD TARGET
			if (!target->Load(arg2) || (PLR_FLAGGED(target, PLR_DELETED) && !PLR_FLAGGED(target, PLR_NODELETE)))
			{
				delete target;
				ch->Send("Unable to load '%s': deleted player.\n", arg2);
				return;
			}
			
			if (IS_STAFF(target)
					&& !(STF_FLAGGED(target, STAFF_ADMIN) ? ch->GetLevel() == LVL_CODER : STF_FLAGGED(ch, STAFF_ADMIN)))
			{
				delete target;
				ch->Send("Insufficient privileges to load '%s'.\n", arg2);
				return;
			}
			
			//	ENTER GAME
			enter_player_game(target);
			target->FromRoom();
			target->ToRoom(ch->InRoom());
			
			mudlogf(BRF, ch->GetPlayer()->m_StaffInvisLevel, true, "(GC) %s loaded player %s.", ch->GetName(), target->GetName());
			
			act("$n has been loaded into the game.", TRUE, target, 0, 0, TO_ROOM);
		}
	}
	else
		send_to_char("That'll have to be either 'obj' or 'mob'.\n", ch);
}



ACMD(do_vstat)
{
	BUFFER(buf, MAX_INPUT_LENGTH);
	BUFFER(buf2, MAX_INPUT_LENGTH);

	argument = two_arguments(argument, buf, buf2);

	bool	bLineNums = false;
	if (!str_cmp(buf2, "-n"))
	{
		bLineNums = true;
		one_argument(argument, buf2);
	}
	
	if (!*buf || !*buf2 || !IsVirtualID(buf2))
		send_to_char("Usage: vstat { obj | mob | trg } <ID>\n", ch);
	else if (is_abbrev(buf, "mob"))
	{
		CharacterPrototype *proto = mob_index.Find(VirtualID(buf2, ch->InRoom()->GetVirtualID().zone));
		
		if (!proto)		send_to_char("There is no monster with that ID.\n", ch);
		else			do_stat_character(ch, proto->m_pProto);
	}
	else if (is_abbrev(buf, "obj"))
	{
		ObjectPrototype *proto = obj_index.Find(VirtualID(buf2, ch->InRoom()->GetVirtualID().zone));
		
		if (!proto)		send_to_char("There is no object with that ID.\n", ch);
		else			do_stat_object(ch, proto->m_pProto);
	}
	else if (is_abbrev(buf, "trg") || is_abbrev(buf, "trig"))
	{
		
		ScriptPrototype *proto = trig_index.Find(VirtualID(buf2, ch->InRoom()->GetVirtualID().zone));
		
		if (!proto)		send_to_char("There is no trigger with that ID.\n", ch);
		else
		{
			proto->m_pProto->Compile();
			do_stat_trigger(ch, proto->m_pProto, bLineNums);	
		}
	}
	else
		send_to_char("That'll have to be either 'obj' or 'mob'.\n", ch);
}




/* clean a room of all mobiles and objects */
ACMD(do_purge) {
	CharData *vict;
	ObjData *obj;
	BUFFER(buf, MAX_INPUT_LENGTH);
	
	one_argument(argument, buf);

	if (*buf) {			/* argument supplied. destroy single object
						 * or char */
		if ((vict = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
			if (!IS_NPC(vict) && (!STF_FLAGGED(ch, STAFF_SECURITY | STAFF_ADMIN) ||
					(IS_STAFF(vict) && !STF_FLAGGED(ch, STAFF_ADMIN)))) {
				send_to_char("Fuuuuuuuuu!\n", ch);
				return;
			}
			act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);

			if (!IS_NPC(vict)) {
				mudlogf(BRF, LVL_STAFF, TRUE, "(GC) %s has purged %s.", ch->GetName(), vict->GetName());
				if (vict->desc) {
					vict->desc->GetState()->Push(new DisconnectConnectionState);
					vict->desc->m_Character = NULL;
					vict->desc = NULL;
				}
			}
			vict->Extract();
		} else if ((obj = get_obj_in_list_vis(ch, buf, ch->InRoom()->contents))) {
			act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
			obj->Extract();
		} else {
			send_to_char("Nothing here by that name.\n", ch);
			return;
		}

		send_to_char(OK, ch);
	} else {			/* no argument. clean out the room */
		act("$n gestures... You are surrounded by scorching flames!", FALSE, ch, 0, 0, TO_ROOM);
		ch->InRoom()->Send("The world seems a little cleaner.\n");

		FOREACH(CharacterList, ch->InRoom()->people, iter) {
			if (IS_NPC(*iter))
				(*iter)->Extract();
		}

		FOREACH(ObjectList, ch->InRoom()->contents, iter) {
			(*iter)->Extract();
		}
	}
}



ACMD(do_advance) {
	extern int level_cost(int level);
	
	CharData *victim;
	int newlevel = 0, oldlevel;
	bool	rankAdvance;
	BUFFER(name, MAX_INPUT_LENGTH);
	BUFFER(level, MAX_INPUT_LENGTH);
	
	two_arguments(argument, name, level);

	rankAdvance = !str_cmp(level, "rank");
	
	if (!*name)
		send_to_char("Advance who?\n", ch);
	else if (!(victim = get_char_vis(ch, name, FIND_CHAR_WORLD)))
		send_to_char("That player is not here.\n", ch);
	else if (IS_STAFF(victim) && !STF_FLAGGED(ch, STAFF_ADMIN))
		send_to_char("Maybe that's not such a great idea.\n", ch);
	else if (IS_NPC(victim))
		send_to_char("NO!  Not on NPC's.\n", ch);
	else if (!*level || (!rankAdvance && (newlevel = atoi(level)) <= 0))
		send_to_char("That's not a level!\n", ch);
	else if (newlevel > LVL_ADMIN)
		ch->Send("%d is the highest possible level.\n", LVL_ADMIN);
//	else if (newlevel == ch->GetLevel())
//		send_to_char("Yeah, right.\n", ch);
	else if (rankAdvance && (((victim->GetLevel() % 20) != 0) ||
			(GET_LIFEMP(victim) < level_cost(victim->GetLevel()))))
		ch->Send("%s is not ready for a rank promotion.\n", victim->GetName());
	else if (!rankAdvance && (newlevel == victim->GetLevel()))
		ch->Send("They are already at that level.\n");
	else if (rankAdvance && victim->GetLevel() == 100)
		ch->Send("%s cannot be rank-advanced past 100.", victim->GetName());
	else {
		if (rankAdvance)
		{
			newlevel = victim->GetLevel() + 1;
		}
		oldlevel = victim->GetLevel();
		if (newlevel < victim->GetLevel()) {
			do_start(victim);
			victim->m_Level = newlevel;
			send_to_char("You have been demoted!\n", victim);
		} else {
			act("$n has promoted you.\n"
				"You put the new rank emblems on.\n"
				"You feel more powerful.", FALSE, ch, 0, victim, TO_VICT);
		}

		send_to_char(OK, ch);

		mudlogf(BRF, LVL_STAFF, TRUE, "(GC) %s has advanced %s to level %d (from %d)", ch->GetName(), victim->GetName(), newlevel, oldlevel);
		victim->SetLevel(newlevel);
		victim->Save();
	}
}



ACMD(do_restore) {
	CharData *vict;
	int i;
	BUFFER(buf, MAX_INPUT_LENGTH);
	
	one_argument(argument, buf);
	
	if (!*buf)
		send_to_char("Whom do you wish to restore?\n", ch);
	else if (!str_cmp(buf, "all")) {
		mudlogf(NRM, LVL_STAFF, TRUE, "(GC) %s restores all", ch->GetName());
	    FOREACH(DescriptorList, descriptor_list, iter)
	    {
	    	DescriptorData *d = *iter;
			if ((vict = d->m_Character) && (vict != ch)) {
				GET_HIT(vict) = GET_MAX_HIT(vict);
				GET_MOVE(vict) = GET_MAX_MOVE(vict);
				//GET_ENDURANCE(vict) = GET_MAX_ENDURANCE(vict);

				if ((ch->GetLevel() >= LVL_ADMIN) && (vict->GetLevel() >= LVL_STAFF)) {
					for (i = 1; i <= MAX_SKILLS; i++)
						GET_REAL_SKILL(vict, i) = MAX_SKILL_LEVEL;

					vict->real_abils.Strength = MAX_PC_STAT;
					vict->real_abils.Health = MAX_PC_STAT;
					vict->real_abils.Coordination = MAX_PC_STAT;
					vict->real_abils.Agility = MAX_PC_STAT;
					vict->real_abils.Perception = MAX_PC_STAT;
					vict->real_abils.Knowledge = MAX_PC_STAT;
					
					vict->AffectTotal();
				}
				vict->update_pos();
				act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
			}
		}
	} else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_WORLD)))
		send_to_char(NOPERSON, ch);
	else {
		GET_HIT(vict) = GET_MAX_HIT(vict);
		GET_MOVE(vict) = GET_MAX_MOVE(vict);
		//GET_ENDURANCE(vict) = GET_MAX_ENDURANCE(vict);

		if (IS_STAFF(vict)) {
			for (i = 1; i <= MAX_SKILLS; i++)
				GET_REAL_SKILL(vict, i) = MAX_SKILL_LEVEL;
				
			vict->real_abils.Strength = MAX_PC_STAT;
			vict->real_abils.Health = MAX_PC_STAT;
			vict->real_abils.Coordination = MAX_PC_STAT;
			vict->real_abils.Agility = MAX_PC_STAT;
			vict->real_abils.Perception = MAX_PC_STAT;
			vict->real_abils.Knowledge = MAX_PC_STAT;
			
			vict->aff_abils = vict->real_abils;
			
			vict->AffectTotal();
		}
		vict->update_pos();
		send_to_char(OK, ch);
		act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
	}
}


void perform_immort_vis(CharData *ch)
{
	if (ch->GetPlayer()->m_StaffInvisLevel == 0 && AFF_FLAGS(ch).testForNone(Affect::AFF_STEALTH_FLAGS))
	{
		send_to_char("You are already fully visible.\n", ch);
		return;
	}

	ch->GetPlayer()->m_StaffInvisLevel = 0;
	ch->Appear();
	send_to_char("You are now fully visible.\n", ch);
}


void perform_immort_invis(CharData *ch, int level) {
	if (IS_NPC(ch))
		return;

	FOREACH(CharacterList, ch->InRoom()->people, iter)
	{
		CharData *tch = *iter;
		if (tch == ch)
			continue;
		if (tch->GetLevel() >= ch->GetPlayer()->m_StaffInvisLevel && tch->GetLevel() < level)
			act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0, tch, TO_VICT);
		if (tch->GetLevel() < ch->GetPlayer()->m_StaffInvisLevel && tch->GetLevel() >= level)
			act("You suddenly realize that $n is standing beside you.", FALSE, ch, 0, tch, TO_VICT);
	}

	ch->GetPlayer()->m_StaffInvisLevel = level;
	ch->Send("Your invisibility level is %d.\n", level);
}


ACMD(do_invis) {
	int level;
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	if (IS_NPC(ch)) {
		send_to_char("You can't do that!\n", ch);
		return;
	}

	one_argument(argument, arg);
	if (!*arg) {
		if (ch->GetPlayer()->m_StaffInvisLevel > 0)	perform_immort_vis(ch);
		else										perform_immort_invis(ch, 101);
	} else {
		level = atoi(arg);
		if (level > ch->GetLevel())		ch->Send("You can't go invisible above %d.\n", ch->GetLevel());
		else if (level < 1)				perform_immort_vis(ch);
		else							perform_immort_invis(ch, level);
	}
}


ACMD(do_gecho) {
//	DescriptorData *pt;
	
	skip_spaces(argument);
	delete_doubledollar(argument);

	if (!*argument)
		send_to_char("That must be a mistake...\n", ch);
	else {
		mudlogf( NRM, LVL_ADMIN, TRUE,  "(GC) %s gechoed '`n%s`g'", ch->GetName(), argument);
		send_to_players(ch, "%s\n", argument);
		if (PRF_FLAGGED(ch, PRF_NOREPEAT))	send_to_char(OK, ch);
		else								ch->Send("%s\n", argument);
	}
}


ACMD(do_poofset)
{
	skip_spaces(argument);
	
	switch (subcmd) {
		case SCMD_POOFIN:	ch->GetPlayer()->m_PoofInMessage = argument;	break;
		case SCMD_POOFOUT:	ch->GetPlayer()->m_PoofOutMessage = argument;	break;
		default:			return;
	}

	ch->Send(OK);
}


ACMD(do_dc)
{
	int num_to_dc = atoi(argument);
	
	if (num_to_dc == 0)
	{
		send_to_char("Usage: DC <user number> (type USERS for a list)\n", ch);
		return;
	}
	
	DescriptorData *d = NULL;
    FOREACH(DescriptorList, descriptor_list, iter)
    {
		if ((*iter)->m_ConnectionNumber == num_to_dc)
		{
			d = *iter;	
			break;
		}
	}

	if (!d) {
		send_to_char("No such connection.\n", ch);
		return;
	}
	if (d->m_Character && ((IS_STAFF(d->m_Character) && !STF_FLAGGED(ch, STAFF_ADMIN)) || STF_FLAGGED(d->m_Character, STAFF_ADMIN))) {
//		send_to_char("Umm.. maybe that's not such a good idea...\n", ch);
		if (ch->CanSee(d->m_Character, GloballyVisible) == VISIBLE_NONE)
			send_to_char("No such connection.\n", ch);
		else
			send_to_char("Umm.. maybe that's not such a good idea...\n", ch);
		return;
	}
	/* We used to just close the socket here using close_socket(), but
	 * various people pointed out this could cause a crash if you're
	 * closing the person below you on the descriptor list.  Just setting
	 * to CON_CLOSE leaves things in a massively inconsistent state so I
	 * had to add this new flag to the descriptor.
	 */
	d->GetState()->Push(new DisconnectConnectionState);
	ch->Send("Connection #%d closed.\n", num_to_dc);
	log("(GC) Connection closed by %s.", ch->GetName());
}



ACMD(do_wizlock) {
	int value;
	char *when;
	BUFFER(buf, MAX_INPUT_LENGTH);

	one_argument(argument, buf);
	if (*buf) {
		value = atoi(buf);
		if (value < 0 || value > 101) {
			send_to_char("Invalid wizlock value.\n", ch);
			return;
		}
		circle_restrict = value;
		when = "now";
	} else
		when = "currently";

	switch (circle_restrict) {
		case 0:
			sprintf(buf, "The game is %s completely open.\n", when);
			break;
		case 1:
			sprintf(buf, "The game is %s closed to new players.\n", when);
			break;
		default:
			sprintf(buf, "Only level %d and above may enter the game %s.\n", circle_restrict, when);
			break;
	}
	send_to_char(buf, ch);
}


ACMD(do_date)
{
	time_t mytime;
	int d, h, m;
	
	if (subcmd == SCMD_DATE)
		mytime = time(0);
	else
		mytime = boot_time;
	
	Lexi::String	timeStr = Lexi::CreateDateString(mytime);

	if (subcmd == SCMD_DATE)
		ch->Send("Current machine time: %s\n", timeStr.c_str());
	else {
		mytime = time(0) - boot_time;
		d = mytime / 86400;
		h = (mytime / 3600) % 24;
		m = (mytime / 60) % 60;

		ch->Send("Up since %s: %d day%s, %d:%02d\n", timeStr.c_str(), d, ((d == 1) ? "" : "s"), h, m);
	}
}



ACMD(do_last) {
	BUFFER(buf, MAX_INPUT_LENGTH);
	ManagedPtr<CharData>	tempchar(new CharData);

	one_argument(argument, buf);
	
	if (!*buf)
		send_to_char("For whom do you wish to search?\n", ch);
	else if (!tempchar->Load(buf))
		send_to_char("There is no such player.\n", ch);
	else if (IS_STAFF(ch))
	{
		ch->Send("[%5d] [%2d %s] %-12s : %-18s : %-20s\n",
				tempchar->GetID(), tempchar->GetLevel(),
				findtype(tempchar->GetRace(), race_abbrevs), tempchar->GetName(), tempchar->GetPlayer()->m_Host.c_str(),
				Lexi::CreateDateString(tempchar->GetPlayer()->m_LastLogin).c_str());
	}
	else
	{
		CharData *online = CharData::Find(tempchar->GetID());
		
		if (online && online->desc && (!IS_STAFF(online) || GET_REAL_LEVEL(ch) >= online->GetPlayer()->m_StaffInvisLevel))
			ch->Send("%s is currently online.\n", online->GetName());
		else if (IS_STAFF(tempchar))
			ch->Send("You are not allowed to see the last login time of staff members.\n");
		else
		{
			time_t t = time(0);
			int seconds = t - tempchar->GetPlayer()->m_LastLogin;
			int minutes = seconds / 60;
			int hours = minutes / 60;
			int days = hours / 24;
			
			if (days > 0)		sprintf(buf, "%d day%s", days, days == 1 ? "" : "s");
			else if (hours > 0)	sprintf(buf, "%d hour%s", hours, hours == 1 ? "" : "s");
			else				sprintf(buf, "%d minute%s", minutes, minutes == 1 ? "" : "s");
			
			ch->Send("%s [%s] was last seen on %-20s UTC (%s ago)\n",
					tempchar->GetName(),
					findtype(tempchar->GetRace(), race_abbrevs),
					Lexi::CreateDateString(tempchar->GetPlayer()->m_LastLogin).c_str(),
					buf);
		}
	}
}


ACMD(do_force) {
	CharData *vict;
	BUFFER(to_force, MAX_INPUT_LENGTH + 2);
	BUFFER(arg, MAX_INPUT_LENGTH);
	BUFFER(buf, MAX_STRING_LENGTH);
	BUFFER(buf1, MAX_STRING_LENGTH);
	
	half_chop(argument, arg, to_force);

	sprintf(buf1, "$n has forced you to '%s'.", to_force);

	if (!*arg || !*to_force)
		send_to_char("Whom do you wish to force do what?\n", ch);
	else if (!str_cmp("room", arg)) {
		send_to_char(OK, ch);

		sprintf(buf1, "$n has forced the room to '%s'.", to_force);
		mudlogf( NRM, LVL_STAFF, TRUE,  "(GC) %s forced room %s to %s", ch->GetName(), ch->InRoom()->GetVirtualID().Print().c_str(), to_force);

		FOREACH(CharacterList, ch->InRoom()->people, iter)
		{
			CharData *vict = *iter;
			if ((IS_STAFF(vict) && !STF_FLAGGED(ch, STAFF_ADMIN)) || STF_FLAGGED(vict, STAFF_ADMIN))
				continue;
			act(buf1, TRUE, ch, NULL, vict, TO_VICT);
			command_interpreter(vict, to_force);
		}
	} else if (!str_cmp("all", arg)) { /* force all */
		send_to_char(OK, ch);

		sprintf(buf1, "$n has forced everyone to '%s'.", to_force);
		mudlogf( NRM, LVL_STAFF, TRUE,  "(GC) %s forced all to %s", ch->GetName(), to_force);

	    FOREACH(DescriptorList, descriptor_list, iter)
	    {
	    	DescriptorData *i = *iter;
	    	
			vict = i->m_Character;
			if (!i->GetState()->IsPlaying() || !vict || (vict == ch) || STF_FLAGGED(vict, STAFF_ADMIN))
				continue;
			if (IS_STAFF(vict) && !STF_FLAGGED(ch, STAFF_ADMIN))
				continue;
			act(buf1, TRUE, ch, NULL, vict, TO_VICT);
			command_interpreter(vict, to_force);
		}
	} else {
		if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
			send_to_char(NOPERSON, ch);
		else if (IS_STAFF(vict) && ((STF_FLAGGED(vict, STAFF_SRSTAFF) && !STF_FLAGGED(ch, STAFF_ADMIN)) ||
				(STF_FLAGGED(vict, STAFF_ADMIN) && ch->GetLevel() < 105)))
			send_to_char("No, no, no!\n", ch);
		else {
			send_to_char(OK, ch);

			act(buf1, TRUE, ch, NULL, vict, TO_VICT);
			mudlogf( NRM, LVL_STAFF, TRUE,  "(GC) %s forced %s to %s", ch->GetName(), vict->GetName(), to_force);
			command_interpreter(vict, to_force);
		}
	}
}



ACMD(do_wiznet)
{
	char emote = FALSE, any = FALSE;
	int level = LVL_IMMORT;
	
	skip_spaces(argument);
	delete_doubledollar(argument);

	if (!*argument) {
//		send_to_char(	"Usage: wiznet <text> | #<level> <text> | *<emotetext> |\n "
//						"       wiznet @<level> *<emotetext> | wiz @\n", ch);
		ch->Send("\n----- Wiznet History (Max: 20) -----\n");
		
		FOREACH(Lexi::StringList, ch->GetPlayer()->m_WizBuffer, msg)
		{
			ch->Send("~ `c%s`n", msg->c_str());
		}
		
		return;
	}
	
	switch (*argument) {
		case '*':
			emote = TRUE;
		case '#':
			{
			BUFFER(buf1, MAX_INPUT_LENGTH);
			one_argument(argument + 1, buf1);
			if (is_number(buf1)) {
				half_chop(argument + 1, buf1, argument);
				level = MAX(atoi(buf1), LVL_IMMORT);
				if (level > ch->GetLevel()) {
					send_to_char("You can't wizline above your own level.\n", ch);
					return;
				}
			} else if (emote)
				argument++;
			}
			break;
		case '@':
		    FOREACH(DescriptorList, descriptor_list, iter)
		    {
		    	DescriptorData *d = *iter;

				if (d->GetState()->IsPlaying() && IS_STAFF(d->m_Character) &&
						!CHN_FLAGGED(d->m_Character, Channel::NoWiz) &&
						(ch->CanSee(d->m_Character, GloballyVisible) != VISIBLE_NONE || ch->GetLevel() == LVL_CODER)) {
					if (!any) {
						ch->Send("Gods online:\n");
						any = TRUE;
					}
					ch->Send("  %s", d->m_Character->GetName());
					if (PLR_FLAGGED(d->m_Character, PLR_MAILING))			ch->Send(" (Writing mail)");
					else if (PLR_FLAGGED(d->m_Character, PLR_WRITING))	ch->Send(" (Writing)");
					ch->Send("\n");
				}
			}
			any = FALSE;
			
		    FOREACH(DescriptorList, descriptor_list, iter)
		    {
		    	DescriptorData *d = *iter;
				if (d->GetState()->IsPlaying() && IS_STAFF(d->m_Character) &&
						CHN_FLAGGED(d->m_Character, Channel::NoWiz) &&
						ch->CanSee(d->m_Character, GloballyVisible) != VISIBLE_NONE) {
					if (!any) {
						ch->Send("Gods offline:\n");
						any = TRUE;
					}
					ch->Send("  %s\n", d->m_Character->GetName());
				}
			}
			return;
		case '\\':
			++argument;
			break;
		default:
			break;
	}
	if (CHN_FLAGGED(ch, Channel::NoWiz)) {
		send_to_char("You are offline!\n", ch);
		return;
	}
	skip_spaces(argument);

	if (!*argument) {
		send_to_char("Don't bother the gods like that!\n", ch);
		return;
	}
		
	BUFFER(buf1, MAX_STRING_LENGTH);
	BUFFER(buf2, MAX_STRING_LENGTH);
	
	if (level > LVL_IMMORT) {
		sprintf(buf1, "%s: <%d> %s%s\n", ch->GetName(), level, emote ? "<--- " : "", argument);
		sprintf(buf2, "Someone: <%d> %s%s\n", level, emote ? "<--- " : "", argument);
	} else {
		sprintf(buf1, "%s: %s%s\n", ch->GetName(), emote ? "<--- " : "", argument);
		sprintf(buf2, "Someone: %s%s\n", emote ? "<--- " : "", argument);
	}

    FOREACH(DescriptorList, descriptor_list, iter)
    {
    	DescriptorData *d = *iter;

		if (d->GetState()->IsPlaying() && (d->m_Character->GetLevel() >= level) &&
				!CHN_FLAGGED(d->m_Character, Channel::NoWiz) &&
				!PLR_FLAGGED(d->m_Character, PLR_WRITING | PLR_MAILING)
				&& ((d != ch->desc) || !PRF_FLAGGED(d->m_Character, PRF_NOREPEAT))) {
			send_to_char("`c", d->m_Character);
			if (d->m_Character->CanSee(ch, GloballyVisible) != VISIBLE_NONE)
			{
				send_to_char(buf1, d->m_Character);
				d->m_Character->AddWizBuf(buf1);
			}
			else
			{
				send_to_char(buf2, d->m_Character);
				d->m_Character->AddWizBuf(buf2);
			}
			send_to_char("`n", d->m_Character);
		}
	}

	if (PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char(OK, ch);
}



ACMD(do_zreset) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	one_argument(argument, arg);
	if (!*arg)
	{
		send_to_char("You must specify a zone.\n", ch);
		return;
	}
	if (*arg == '*')
	{
//		Lexi::ForEach(zone_table, reset_zone);
		FOREACH(ZoneMap, zone_table, z)
			reset_zone(*z);
		send_to_char("Reset world.\n", ch);
		mudlogf( NRM, LVL_IMMORT, TRUE,  "(GC) %s reset entire world.", ch->GetName());
		return;
	}
	
	ZoneData *zone = NULL;
	if (*arg == '.')	zone = ch->InRoom()->GetZone();
	else				zone = zone_table.Find(arg);
	
	if (zone)
	{
		reset_zone(zone);
		ch->Send("Reset zone '%s': %s.\n", zone->GetTag(), zone->GetName());
		mudlogf( NRM, LVL_STAFF, TRUE,  "(GC) %s reset zone '%s' (%s)", ch->GetName(), zone->GetTag(), zone->GetName());
	}
	else
		send_to_char("Invalid zone number.\n", ch);
}


/*
 *  General fn for wizcommands of the sort: cmd <player>
 */

ACMD(do_wizutil) {
	CharData *vict;
	int result;
	BUFFER(arg, MAX_INPUT_LENGTH);

	argument = one_argument(argument, arg);

	if (!*arg) {
		if (subcmd == SCMD_SQUELCH) {
			int	count = 0;
			
		    FOREACH(DescriptorList, descriptor_list, iter)
		    {
		    	DescriptorData *pt = *iter;

				if (pt->GetState()->IsPlaying() && pt->m_Character && PLR_FLAGGED(pt->m_Character, PLR_NOSHOUT)) {
					if (count == 0)
						ch->Send("The following people are muted:\n");
					if (pt->m_Character->GetPlayer()->m_MuteLength)
						ch->Send("    %d. %s (%ld minutes remaining)\n", ++count, pt->m_Character->GetName(),
							((pt->m_Character->GetPlayer()->m_MuteLength - (time(0) - pt->m_Character->GetPlayer()->m_MutedAt)) + 59) / 60);
					else
						ch->Send("    %d. %s\n", ++count, pt->m_Character->GetName());
				}
			}
			
			if (count == 0)
				ch->Send("Nobody is muted.\n");
		} else
			send_to_char("Yes, but for whom?!?\n", ch);
	} else if (!(vict = get_char_vis(ch, arg, FIND_CHAR_WORLD)))
		send_to_char("There is no such player.\n", ch);
	else if (IS_NPC(vict))
		send_to_char("You can't do that to a mob!\n", ch);
	else if (ch != vict && IS_STAFF(vict) && (!STF_FLAGGED(ch, STAFF_ADMIN) || STF_FLAGGED(vict, STAFF_ADMIN))
		&& (subcmd != SCMD_FREEZE || PLR_FLAGGED(vict, PLR_FROZEN)))
		send_to_char("Hmmm...you'd better not.\n", ch);
	else {
		switch (subcmd) {
//			case SCMD_REROLL:
//				send_to_char("Rerolled...\n", ch);
//				roll_real_abils(vict);
//				log("(GC) %s has rerolled %s.", ch->GetName(), vict->GetName());
//				ch->Send("New stats: Str %d, Int %d, Per %d, Agl %d, Con %d\n",
//						GET_STR(vict), GET_INT(vict), GET_PER(vict),
//						GET_AGI(vict), GET_CON(vict));
//				break;
			case SCMD_PARDON:
				if (!AFF_FLAGGED(vict, AFF_TRAITOR)) {
					send_to_char("Your victim is not flagged.\n", ch);
					return;
				}
				Affect::FlagsFromChar(vict, AFF_TRAITOR);
				send_to_char("Pardoned.\n", ch);
				send_to_char("You have been pardoned by the Gods!\n", vict);
				mudlogf( BRF, LVL_STAFF, TRUE,  "(GC) %s pardoned by %s", vict->GetName(), ch->GetName());
				break;
			case SCMD_NOTITLE:
				result = PLR_TOG_CHK(vict, PLR_NOTITLE);
				sprintf(arg, "(GC) Notitle %s for %s by %s.", ONOFF(result), vict->GetName(), ch->GetName());
				mudlog(arg, NRM, LVL_STAFF, TRUE);
				strcat(arg, "\n");
				send_to_char(arg, ch);
				break;
			case SCMD_SQUELCH:
				{
					int length = MIN(atoi(argument), 60*24);

					result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
					
					if (length > 0 && result)
					{
						mudlogf(BRF, LVL_STAFF, TRUE, "(GC) Squelch %s for %s by %s for %d minutes.", ONOFF(result), vict->GetName(), ch->GetName(), length);
						ch->Send("(GC) Squelch %s for %s by %s for %d minutes\n.", ONOFF(result), vict->GetName(), ch->GetName(), length);
						vict->GetPlayer()->m_MutedAt = time(0);
						vict->GetPlayer()->m_MuteLength = length * 60;
					}
					else
					{
						mudlogf(BRF, LVL_STAFF, TRUE, "(GC) Squelch %s for %s by %s.", ONOFF(result), vict->GetName(), ch->GetName());
						ch->Send("(GC) Squelch %s for %s by %s\n.", ONOFF(result), vict->GetName(), ch->GetName());
					}
				}
				break;
			case SCMD_FREEZE:
				if (ch == vict) {
					send_to_char("Oh, yeah, THAT'S real smart...\n", ch);
					return;
				}
				if (PLR_FLAGGED(vict, PLR_FROZEN)) {
					send_to_char("Your victim is already pretty cold.\n", ch);
					return;
				}
				SET_BIT(PLR_FLAGS(vict), PLR_FROZEN);
//				GET_FREEZE_LEV(vict) = ch->GetLevel();
				send_to_char("A bitter wind suddenly rises and drains every bit of heat from your body!\nYou feel frozen!\n", vict);
				send_to_char("Frozen.\n", ch);
				act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict, 0, 0, TO_ROOM);
				mudlogf(BRF, LVL_STAFF, TRUE,  "(GC) %s frozen by %s.", vict->GetName(), ch->GetName());
				break;
			case SCMD_THAW:
				if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
					send_to_char("Sorry, your victim is not morbidly encased in ice at the moment.\n", ch);
					return;
				}
//				if (GET_FREEZE_LEV(vict) > ch->GetLevel()) {
//					ch->Send("Sorry, a level %d God froze %s... you can't unfreeze %s.\n",
//							GET_FREEZE_LEV(vict), vict->GetName(), HMHR(vict));
//					release_buffer(arg);
//					return;
//				}
				mudlogf(BRF, LVL_STAFF, TRUE,  "(GC) %s un-frozen by %s.", vict->GetName(), ch->GetName());
				REMOVE_BIT(PLR_FLAGS(vict), PLR_FROZEN);
				send_to_char("Thawed.\n", ch);
				act("$N turns up the heat, thawing you out.", FALSE, vict, 0, ch, TO_CHAR);
				act("A sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0, 0, TO_ROOM);
				break;
			case SCMD_UNAFFECT:
				FOREACH(Lexi::List<Affect *>, vict->m_Affects, aff)
				{
					(*aff)->Remove(vict, Affect::DontTotal);
				}
				ch->AffectTotal();
				
				send_to_char(	"There is a brief flash of light!\n"
								"You feel slightly different.\n", vict);
				send_to_char("All affects removed.\n", ch);
				CheckRegenRates(vict);
//				} else {
//					send_to_char("Your victim does not have any affections!\n", ch);
//					release_buffer(arg);
//					return;
//				}
				break;
			default:
				log("SYSERR: Unknown subcmd %d passed to do_wizutil (" __FILE__ ")", subcmd);
				break;
		}
		vict->Save();
	}
}


/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

void print_zone_to_buf(char *bufptr, ZoneData *zone)
{
	if (!zone)
		sprintf_cat(bufptr, "<INVALID ZONE>\n");
	else if (*zone->GetComment())
	{
		BUFFER(comment, MAX_STRING_LENGTH);
	
		sprintf(comment, "(%s)", zone->GetComment());
	
		int nameLen = cstrnlen_for_sprintf(zone->GetName(), 20);
		int commentLen = cstrnlen_for_sprintf(comment, 30);
		sprintf_cat(bufptr, "%10s %-*.*s %*.*s%s\n",
				zone->GetTag(),
				nameLen, nameLen, zone->GetName(),
				commentLen, commentLen, comment,
				zone->GetZoneNumber() >= 0 ? Lexi::Format(" ; Top: %5d", zone->top).c_str() : "");
	}
	else
	{
		int nameLen = cstrnlen_for_sprintf(zone->GetName(), 51);		
		sprintf_cat(bufptr, "%10s %-*.*s%s\n",
				zone->GetTag(),
				nameLen, nameLen, zone->GetName(),
				zone->GetZoneNumber() >= 0 ? Lexi::Format(" ; Top: %5d", zone->top).c_str() : "");	
	}
}


void show_deleted(CharData * ch)
{
	unsigned int found;
	BUFFER(buf, MAX_STRING_LENGTH * 8);
	
	found = 0;
	FOREACH(CharacterPrototypeMap, mob_index, mob)
	{
		if (MOB_FLAGGED((*mob)->m_pProto, MOB_DELETED))
		{
			if (found == 0)		strcat(buf, "Deleted Mobiles:\n");
			sprintf_cat(buf, "`c%3d.  `n[`y%10s`n] %s\n", ++found, (*mob)->GetVirtualID().Print().c_str(), (*mob)->m_pProto->GetName());
		}
	}
	
	found = 0;
	FOREACH(ObjectPrototypeMap, obj_index, obj) 
	{
		if (OBJ_FLAGGED((*obj)->m_pProto, ITEM_DELETED))
		{
			if (found == 0)		strcat(buf, "\nDeleted Objects:\n");
			sprintf_cat(buf, "`c%3d.  `n[`y%10s`n] %s\n", ++found, (*obj)->GetVirtualID().Print().c_str(), (*obj)->m_pProto->GetName());
		}
	}
	
	found = 0;
	FOREACH(RoomMap, world, room)
	{
		if (ROOM_FLAGGED(*room, ROOM_DELETED))
		{
			if (found == 0)		strcat(buf, "\nDeleted Rooms:\n");
			sprintf_cat(buf, "`c%3d.  `n[`y%10s`n] %s\n", ++found, (*room)->GetVirtualID().Print().c_str(), (*room)->GetName());
		}
	}
	
	found = 0;
	FOREACH(ScriptPrototypeMap, trig_index, script)
	{
		if (IS_SET(GET_TRIG_TYPE((*script)->m_pProto), TRIG_DELETED)) {
			if (found == 0)		strcat(buf, "\nDeleted Scripts:\n");
			sprintf_cat(buf, "`c%3d.  `n[`y%10s`n] %s\n", ++found, (*script)->GetVirtualID().Print().c_str(), (*script)->m_pProto->GetName());
		}
	}
	page_string(ch->desc, buf);
}

extern std::list<ZoneData *>	reset_queue;
ACMD(do_show) {
	int j, l, con;
	unsigned int	i;
	char self = 0;
	CharData *vict;
	extern int gamebyteswritten,gamebytescompresseddiff,gamebytesread;

	struct show_struct {
		char *cmd;
		char level;
	} fields[] = {
		{ "nothing"	, 0				},	//	0
		{ "zones"	, LVL_IMMORT	},	//	1
		{ "player"	, LVL_STAFF		},
		{ "rent"	, LVL_STAFF		},
		{ "stats"	, LVL_IMMORT	},
		{ "errors"	, LVL_STAFF		},	//	5
		{ "death"	, LVL_STAFF		},
		{ "godrooms", LVL_STAFF		},
		{ "shops"	, LVL_IMMORT	},
		{ "houses"	, LVL_STAFF		},
		{ "deleted"	, LVL_STAFF		},	//	10
		{ "buffers"	, LVL_STAFF		},
		{ "behaviors", LVL_STAFF	},
		{ "\n", 0 }
	};

	skip_spaces(argument);

	if (!*argument) {
		ch->Send("Show options:\n");
		for (j = 0, i = 1; fields[i].level; i++)
			if (fields[i].level <= ch->GetLevel())
				ch->Send("%-15s%s", fields[i].cmd, (!(++j % 5) ? "\n" : ""));
		ch->Send("\n");
		return;
	}
	
	BUFFER(arg, MAX_INPUT_LENGTH);
	BUFFER(field, MAX_INPUT_LENGTH);
	BUFFER(value, MAX_INPUT_LENGTH);

	strcpy(arg, two_arguments(argument, field, value));

	for (l = 0; *(fields[l].cmd) != '\n'; l++)
		if (!strncmp(field, fields[l].cmd, strlen(field)))
			break;

	if (ch->GetLevel() < fields[l].level) {
		send_to_char("You are not godly enough for that!\n", ch);
		return;
	}
	if (!strcmp(value, "."))
		self = 1;

	switch (l) {
		case 1:			/* zone */
			{
				BUFFER(buf, MAX_BUFFER_LENGTH);
				/* tightened up by JE 4/6/93 */
				if (self)
					print_zone_to_buf(buf, ch->InRoom()->GetZone());
				else if (*value && is_number(value))
				{
					ZoneData *zone = zone_table.Find(value);

					if (zone)
						print_zone_to_buf(buf, zone);
					else
					{
						send_to_char("That is not a valid zone.\n", ch);
						break;
					}
				}
				else
				{
					std::vector<ZoneData *> zones;
					zones.reserve(zone_table.size());
					FOREACH(ZoneMap, zone_table, z)
						zones.push_back(*z);
					
					Lexi::Sort(zones, ZoneMap::SortByNameFunc);
					
					FOREACH(std::vector<ZoneData *>, zones, z)
						print_zone_to_buf(buf, *z);
				}
				if (*buf)	page_string(ch->desc, buf);
			}
			break;
		case 2:			/* player */
			vict = new CharData();
			if (!*value)
				send_to_char("A name would help.\n", ch);
			else if (!vict->Load(value))
				send_to_char("There is no such player.\n", ch);
			else {
				ch->Send(	"Player: %-12s (%s) [%2d %s]\n"
							"MP: %-8d (Total: %-8d) Skill Points: %-4d (Total %-4d)\n",
						vict->GetName(), GetEnumName(vict->GetSex()), vict->GetLevel(), findtype(vict->GetRace(), race_abbrevs),
						GET_MISSION_POINTS(vict), vict->points.lifemp,
							 vict->GetPlayer()->m_SkillPoints, vict->GetPlayer()->m_LifetimeSkillPoints);
				ch->Send("Started: %-20.16s  Last: %-20.16s  Played: %3dh %2dm\n",
						Lexi::CreateDateString(vict->GetPlayer()->m_Creation).c_str(),
						Lexi::CreateDateString(vict->GetPlayer()->m_LastLogin).c_str(),
						(int) (vict->GetPlayer()->m_TotalTimePlayed / 3600),
						(int) (vict->GetPlayer()->m_TotalTimePlayed / 60 % 60));
			}
			delete vict;
			break;
		case 3:
			ShowPlayerObjectFile(ch, value);
			break;
		case 4:
			i = 0;
			j = 0;
			con = 0;
			FOREACH(CharacterList, character_list, iter)
			{
				CharData *vict = *iter;
				if (IS_NPC(vict))
					j++;
				else if (ch->CanSee(vict, GloballyVisible) != VISIBLE_NONE) {
					i++;
					if (vict->desc)
						con++;
				}
			}
			ch->Send(	"Current stats:\n"
						"  %5d players in game  %5d connected\n"
						"  %5d registered\n"
						"  %5d mobiles          %5d prototypes\n"
						"  %5d objects          %5d prototypes\n"
						"  %5d triggers         %5d prototypes\n"
						"  %5d rooms\n"
						"  %5d zones            %5d pending resets\n"
//						"  %5d total rooms\n"
#if defined(SCRIPTVAR_TRACKING)
						"  %5d unique variables %5d total var refs\n"
#endif
						"  %5d pending events   %5d script threads\n"
//						"  %5d shared strings\n"
						"  %5d large bufs\n"
						"  %5d buf switches     %5d overflows\n"
						"  %d bytes written; %d bytes read\n"
						"  %d bytes saved by compression\n",

					i, con,
					player_table.size(),
					j, mob_index.size(),
					object_list.size(), obj_index.size(),
					trig_list.size(), trig_index.size(),
					world.size(),
					zone_table.size(), reset_queue.size(),
#if defined(SCRIPTVAR_TRACKING)
					ScriptVariable::GetScriptVariableCount(), ScriptVariable::GetScriptVariableTotalRefs(),
#endif
					EventQueue.Count(), TrigThread::ms_TotalThreads,
//					SStrings.Count(),
					buf_largecount,
					buf_switches, buf_overflows,
					gamebyteswritten, gamebytesread,
					gamebytescompresseddiff);
			break;
		case 5:
			send_to_char("Errant Rooms\n------------\n", ch);
			i = 0;
			FOREACH(RoomMap, world, room)
			{
				for (unsigned int j = 0; j < NUM_OF_DIRS; j++)
				{
					if ((*room)->GetExit(j) && (*room)->GetExit(j)->ToRoom() == NULL)
					{
						ch->Send("%2d: [%10s] %s\n", ++i, (*room)->GetVirtualID().Print().c_str(), (*room)->GetName());
						break;
					}
				}
			}
			break;
		case 6:
			send_to_char("Death Traps\n-----------\n", ch);
			i = 0;
			FOREACH(RoomMap, world, room)
			{
				if (ROOM_FLAGGED(*room, ROOM_DEATH))
					ch->Send("%2d: [%10s] %s\n", ++i, (*room)->GetVirtualID().Print().c_str(), (*room)->GetName());
			}
			break;
		case 7:
			send_to_char("Godrooms\n--------------------------\n", ch);
			i = 0;
			FOREACH(RoomMap, world, room)
			{
				if (ROOM_FLAGGED(*room, ROOM_STAFFROOM) || ROOM_FLAGGED(*room, ROOM_ADMINROOM))
					ch->Send("%2d: [%10s] %s\n", ++i, (*room)->GetVirtualID().Print().c_str(), (*room)->GetName());
			}
			break;
		case 8:
			show_shops(ch, value);
			break;
		case 9:
			House::List(ch);
			break;
		case 10:
			show_deleted(ch);
			break;
		case 11:
			show_buffers(ch, 1);
			show_buffers(ch, 0);
			break;
		case 12:
			if (!*value)
				BehaviorSet::List(ch);
			else
			{
				BehaviorSet *bset = BehaviorSet::Find(value);
				
				if (!bset)
					ch->Send("Unknown behavior set.\n");
				else
				{
					bset->Show(ch);
				}
			}
			break;
		default:
			send_to_char("Sorry, I don't understand that.\n", ch);
			break;
	}
}


#define PC   1
#define NPC  2
#define BOTH 3

#define MISC	0
#define BINARY	1
#define NUMBER	2

#define SET_OR_REMOVE(flagset, flags) { \
	if (on) SET_BIT(flagset, flags); \
	else if (off) REMOVE_BIT(flagset, flags); }


struct set_struct {
	char *cmd;
	char level;
	char pcnpc;
	char type;
} set_fields[] = {
	{ "brief",		LVL_IMMORT, 	PC, 	BINARY },	/* 0 */
	{ "invstart", 	LVL_STAFF,		PC, 	BINARY },	/* 1 */
	{ "title",		LVL_IMMORT, 	PC, 	MISC },
	{ "\1nosummon",	LVL_STAFF,		PC, 	BINARY },
	{ "maxhit",		LVL_SRSTAFF, 	BOTH, 	NUMBER },
	{ "nodelete", 	LVL_SRSTAFF, 	PC, 	BINARY },	/* 5 */
	{ "maxmove", 	LVL_SRSTAFF, 	BOTH, 	NUMBER },
	{ "hit", 		LVL_SRSTAFF, 	BOTH, 	NUMBER },
	{ "mission",	LVL_IMMORT, 	PC, 	BINARY },
	{ "move",		LVL_STAFF,		BOTH, 	NUMBER },
	{ "race",		LVL_SRSTAFF,	BOTH,	MISC },		/* 10 */
	{ "passwd",		LVL_ADMIN,		PC, 	MISC },
	{ "str",		LVL_SRSTAFF, 	BOTH, 	NUMBER },
	{ "hea", 		LVL_SRSTAFF, 	BOTH, 	NUMBER },
	{ "coo", 		LVL_SRSTAFF, 	BOTH, 	NUMBER },
	{ "agi", 		LVL_SRSTAFF, 	BOTH, 	NUMBER },	/* 15 */
	{ "per", 		LVL_SRSTAFF, 	BOTH, 	NUMBER },
	{ "kno", 		LVL_SRSTAFF, 	BOTH, 	NUMBER },
	{ "pracs", 		LVL_SRSTAFF, 	BOTH, 	NUMBER },
	{ "team", 		LVL_STAFF,	 	BOTH, 	MISC },
	{ "scmudrefugee",LVL_ADMIN,	 	PC, 	BINARY },	/* 20 */
	{ "name", 		LVL_ADMIN,	 	BOTH, 	MISC },
	{ "sex", 		LVL_SRSTAFF, 	BOTH, 	MISC },
	{ "karma",	 	LVL_SRSTAFF, 	BOTH, 	NUMBER },
	{ "mp",			LVL_SRSTAFF, 	BOTH, 	NUMBER },
	{ "loadroom", 	LVL_STAFF, 		PC, 	MISC },		/* 25 */
	{ "color",		LVL_IMMORT, 	PC, 	BINARY },
	{ "invis",		LVL_SRSTAFF, 	PC, 	NUMBER },
	{ "nohassle", 	LVL_SRSTAFF, 	PC, 	BINARY },
	{ "frozen",		LVL_SRSTAFF,	PC, 	BINARY },
	{ "\1drunk",	LVL_IMMORT, 	BOTH, 	MISC },		/* 30 */
	{ "\1hunger",	LVL_IMMORT, 	BOTH, 	MISC },
	{ "\1thirst",	LVL_IMMORT, 	BOTH, 	MISC },
	{ "traitor",	LVL_SRSTAFF, 	PC, 	BINARY },
	{ "idnum",		LVL_CODER,		PC, 	NUMBER },
	{ "level",		LVL_ADMIN,		BOTH, 	NUMBER },	/* 35 */
	{ "room",		LVL_SRSTAFF, 	BOTH, 	NUMBER },
	{ "roomflag", 	LVL_IMMORT, 	PC, 	BINARY },
	{ "siteok",		LVL_IMMORT, 	PC, 	BINARY },
	{ "deleted", 	LVL_SRSTAFF, 	PC, 	BINARY },
	{ "pretitle",	LVL_SRSTAFF,	PC,		MISC },
	{ "pkonly",		LVL_STAFF,		PC,		BINARY },
	{ "\n", 0, BOTH, MISC }
};


bool perform_set(CharData *ch, CharData *vict, int mode, char *val_arg) {
	int	i, value = 0;
	bool	on = false, off = false;
	
	if (ch->GetLevel() != LVL_ADMIN) {
		if (/*((IS_STAFF(vict) && !STF_FLAGGED(ch, STAFF_ADMIN)) || STF_FLAGGED(vict, STAFF_ADMIN))*/ (vict->GetLevel() >= ch->GetLevel()) && (vict != ch)) {
			send_to_char("Maybe that's not such a great idea...\n", ch);
			return 0;
		}
	}
	if (ch->GetLevel() < set_fields[mode].level) {
		send_to_char("You do not have authority to do that.\n", ch);
		return 0;
	}
	
	//	Make sure the PC/NPC is correct
	if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC)) {
		send_to_char("You can't do that to a mob!", ch);
		return 0;
	} else if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC)) {
		send_to_char("You can only do that to a mob!", ch);		
		return 0;
	}
	
	BUFFER(output, MAX_STRING_LENGTH);
	
	if (set_fields[mode].type == BINARY) {
		if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
			on = true;
		else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
			off = true;
		if (!(on || off)) {
			send_to_char("Value must be 'on' or 'off'.\n", ch);
			return 0;
		}
		sprintf(output, "%s %s for %s.", set_fields[mode].cmd, ONOFF(on), vict->GetName());
	} else if (set_fields[mode].type == NUMBER) {
		value = atoi(val_arg);
		sprintf(output, "%s's %s set to %d.", vict->GetName(), set_fields[mode].cmd, value);
	} else {
		strcpy(output, "Okay.");
	}
	
	switch (mode) {
		case 0:
			SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
			break;
		case 1:
			SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
			break;
		case 2:
			vict->SetTitle(!str_cmp(val_arg, "default") ? NULL : val_arg);
			sprintf(output, "%s's title is now: %s", vict->GetName(), vict->GetTitle());
			vict->Send("%s set your title to \"%s\".\n", ch->GetName(), vict->GetTitle());
			break;
		case 3:
//			SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
//			sprintf(output, "Nosummon %s for %s.\n", ONOFF(!on), vict->GetName());
			break;
		case 4:
			vict->points.max_hit = value = RANGE(value, 1, 5000);
			vict->AffectTotal();
			break;
		case 5:
			SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
			break;
		case 6:
			vict->points.max_move = value = RANGE(value, 1, 5000);
			vict->AffectTotal();
			break;
		case 7:
			vict->points.hit = value = RANGE(value, -100, vict->points.max_hit);
			vict->AffectTotal();
			vict->update_pos();
			break;
		case 8:
			SET_OR_REMOVE(CHN_FLAGS(vict), Channel::Mission);
			break;
		case 9:
			vict->points.move = value = RANGE(value, 0, vict->points.max_move);
			vict->AffectTotal();
			break;
		case 10:
			if ((i = ParseRace(val_arg)) == RACE_UNDEFINED) {
				send_to_char("That is not a race.\n", ch);
				return false;
			}
			vict->m_Race = (Race)i;
			break;
		case 11:
			if (ch->GetID() > 1) {
				send_to_char("Please don't use this command, yet.\n", ch);
				return false;
			} else if (vict->GetLevel() >= LVL_ADMIN) {
				send_to_char("You cannot change that.\n", ch);
				return false;
			} else {
				strncpy(vict->GetPlayer()->m_Password, val_arg, MAX_PWD_LENGTH);
				vict->GetPlayer()->m_Password[MAX_PWD_LENGTH] = '\0';
				sprintf(output, "Password changed to '%s'.", vict->GetPlayer()->m_Password);
			}
			break;
		case 12:
			value = RANGE(value, 1, MAX_PC_STAT);
			vict->real_abils.Strength = value;
			vict->AffectTotal();
			break;
		case 13:
			value = RANGE(value, 1, MAX_PC_STAT);
			vict->real_abils.Health = value;
			vict->AffectTotal();
			break;
		case 14:
			value = RANGE(value, 1, MAX_PC_STAT);
			vict->real_abils.Coordination = value;
			vict->AffectTotal();
			break;
		case 15:
			value = RANGE(value, 1, MAX_PC_STAT);
			vict->real_abils.Agility = value;
			vict->AffectTotal();
			break;
		case 16:
			value = RANGE(value, 1, MAX_PC_STAT);
			vict->real_abils.Perception = value;
			vict->AffectTotal();
			break;
		case 17:
			value = RANGE(value, 1, MAX_PC_STAT);
			vict->real_abils.Knowledge = value;
			vict->AffectTotal();
			break;
		case 18:
			value = RANGE(value, -10000, 10000);
			vict->GetPlayer()->m_SkillPoints = value;
			break;
		case 19:
			value = search_block(val_arg, team_names, 0);
			if (value == -1)
				sprintf(output, "Valid teams are: none red blue green gold, teal, purple");
			else
			{
				sprintf(output, "%s' team set to %s", vict->GetName(), team_titles[value]);
				GET_TEAM(vict) = value;
			}
			break;
		case 20:
			break;
		case 21:
			{
				BUFFER(newName, MAX_INPUT_LENGTH);
				extern int _parse_name(const char *arg, char *name);
				extern bool reserved_word(const char *argument);
				
				bool bSameName;
				
				if (IS_NPC(vict))
				{
					ch->Send("You cannot do that to NPCs; use the 'string' command.");
					return false;
				}
				else if (_parse_name(val_arg, newName)
					|| strlen(newName) < 2
					|| strlen(newName) > MAX_NAME_LENGTH
					|| (!(bSameName = !str_cmp(newName, vict->GetName())) && !Ban::IsValidName(newName))
					|| fill_word(newName)
					|| reserved_word(newName)
					|| (!bSameName && get_id_by_name(newName) != NoID))
				{
					ch->Send("That name is invalid or already in use.\n");
					return false;
				}
				else
				{
					sprintf(output, "%s renamed to %s.", vict->GetName(), newName);
					vict->Send("%s has changed your name to %s.\n", ch->GetName(), newName);
					
					//	Delete old pfile and prepare to move the objfile
					//	New pfile will be saved
					remove(vict->GetFilename().c_str());
					Lexi::String oldObjFilename = vict->GetObjectFilename();
					
					//	Change name
					vict->m_Name = newName;
					
					//	Update the player_table
					if (!bSameName)
					{
						//Lexi::tolower(newName);
						//CAP(newName);
						
						player_table[vict->GetID()] = newName;
						save_player_index();
					}
					
					//	Rename object file
					rename(oldObjFilename.c_str(), vict->GetObjectFilename().c_str());
				}
				
			}
			break;
		case 22:
			if (!str_cmp(val_arg, "male"))				vict->m_Sex = SEX_MALE;
			else if (!str_cmp(val_arg, "female"))		vict->m_Sex = SEX_FEMALE;
			else if (!str_cmp(val_arg, "neutral"))		vict->m_Sex = SEX_NEUTRAL;
			else
			{
				send_to_char("Must be 'male', 'female', or 'neutral'.\n", ch);
				return false;
			}
			break;
		case 23:
			value = RANGE(value, -10000, 10000);
			vict->GetPlayer()->m_Karma = value;
			break;
		case 24:
			value = RANGE(value, 0, 100000000);
			GET_MISSION_POINTS(vict) = value;
			break;
		case 25:
			if (!str_cmp(val_arg, "off"))
				REMOVE_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
			else if (IsVirtualID(val_arg)) {
				RoomData *room = world.Find(VirtualID(val_arg, ch->InRoom()->GetVirtualID().zone));
				if (world.Find(val_arg) == NULL)
				{
					send_to_char("That room does not exist!", ch);
					return false;
				}
				SET_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
				vict->GetPlayer()->m_LoadRoom = room->GetVirtualID();
				sprintf(output, "%s will enter at room %s.", vict->GetName(), vict->GetPlayer()->m_LoadRoom.Print().c_str());
			} else {
				send_to_char("Must be 'off' or a room's virtual number.\n", ch);
				return false;
			}
			break;
		case 26:
			SET_OR_REMOVE(PRF_FLAGS(vict), PRF_COLOR);
			break;
		case 27:
//			if (IS_STAFF(ch) < LVL_ADMIN && ch != vict) {
//				send_to_char("You aren't godly enough for that!\n", ch);
//				release_buffer(output);
//				return false;
//			}
			vict->GetPlayer()->m_StaffInvisLevel = value = RANGE(value, 0, 101);
			break;
		case 28:
//			if (ch->GetLevel() < LVL_ADMIN && ch != vict) {
//				send_to_char("You aren't godly enough for that!\n", ch);
//				release_buffer(output);
//				return false;
//			}
			SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
			break;
		case 29:
			if (ch == vict) {
				send_to_char("Better not -- could be a long winter!\n", ch);
				return false;
			}
			SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
			break;
#if 0
		case 30:
		case 31:
		case 32:
			if (!str_cmp(val_arg, "off")) {
				GET_COND(vict, (mode - 29)) = (char) -1;
				sprintf(output, "%s's %s now off.", vict->GetName(), set_fields[mode].cmd);
			} else if (is_number(val_arg)) {
				value = atoi(val_arg);
				value = RANGE(value, 0, 24);
				GET_COND(vict, (mode - 29)) = (char) value;
				sprintf(output, "%s's %s set to %d.", vict->GetName(), set_fields[mode].cmd, value);
			} else {
				send_to_char("Must be 'off' or a value from 0 to 24.\n", ch);
				return false;
			}
//			CheckRegenRates(vict);
			break;
#endif
		case 33:
			if (AFF_FLAGGED(vict, AFF_TRAITOR))	Affect::FlagsFromChar(vict, AFF_TRAITOR);
			else								(new Affect("TRAITOR", 0, APPLY_NONE, AFF_TRAITOR))->ToChar(vict);
			break;
		case 34:
			if (ch->GetID() != 1) {
				send_to_char("Nuh-uh.\n", ch);
				return false;
			}
			else if (IS_NPC(vict))
			{
				ch->Send("You cannot change IDs of NPCs!\n");
				return false;
			}
			else if (get_name_by_id(value))
			{
				ch->Send("ID already in use.\n");
				return false;
			}
			else if (value < 1 || value > top_idnum)
			{
				ch->Send("ID is out of valid range: 2 to %d.\n", top_idnum);
				return false;
			}
			else
			{
				player_table.erase(vict->GetID());
				vict->SetID(value);
				player_table[vict->GetID()] = vict->GetName();

				sprintf(output, "%s ID changed to %d.", vict->GetName(), vict->GetID());
				
				save_player_index();
			}
			break;
		case 35:
			if (value > ch->GetLevel() || value > LVL_ADMIN) {
				send_to_char("You can't do that.\n", ch);
				return false;
			}
			value = RANGE(value, 0, LVL_CODER);
			vict->m_Level = value;
			break;
		case 36:
		{
			RoomData *dest = world.Find(val_arg);
			if (dest == NULL) {
				send_to_char("No room exists with that number.\n", ch);
				return false;
			}
			else if (vict->InRoom() == NULL)
			{
				send_to_char("You can't do that to people who are offline.  You'll crash the fucking MUD!\n", ch);
				return false;
			}
			else
			{
				vict->FromRoom();
				vict->ToRoom(dest);
			}
		}
			break;
		case 37:
			SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ROOMFLAGS);
			break;
		case 38:
			SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
			break;
		case 39:
			SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
			break;
		case 40:
			vict->GetPlayer()->m_Pretitle = val_arg;
			break;
		case 41:
			SET_OR_REMOVE(PLR_FLAGS(vict), PLR_PKONLY);
			break;
		default:
			send_to_char("Can't set that!", ch);
			return false;
	}
	ch->Send("%s\n", CAP(output));
	return true;
}



ACMD(do_set) {
	CharData *vict = NULL, *cbuf = NULL;
	int	mode = -1, len = 0;
	bool	is_file = false, is_mob = false, is_player = false, retval;
	BUFFER(field, MAX_INPUT_LENGTH);
	BUFFER(name, MAX_INPUT_LENGTH);
	BUFFER(buf, MAX_INPUT_LENGTH);

	half_chop(argument, name, buf);
	if (!strcmp(name, "file")) {
		is_file = true;
		half_chop(buf, name, buf);
	} else if (!str_cmp(name, "player")) {
		is_player = true;
		half_chop(buf, name, buf);
	} else if (!str_cmp(name, "mob")) {
		is_mob = true;
		half_chop(buf, name, buf);
	}
	half_chop(buf, field, buf);

	if (!*name || !*field) {
		send_to_char("Usage: set <victim> <field> <value>\n", ch);
	}
	
	if (!is_file) {
		if (is_player) {
			if (!(vict = get_player_vis(ch, name))) {
				send_to_char("There is no such player.\n", ch);
				return;
			}
		} else {
			if (!(vict = get_char_vis(ch, name, FIND_CHAR_WORLD))) {
				send_to_char("There is no such creature.\n", ch);
				return;
			}
		}
	} else {
		cbuf = new CharData();
		if (!cbuf->Load(name)) {
			send_to_char("There is no such player.\n", ch);
			delete cbuf;
			return;
		}
	
		if (/*(IS_STAFF(cbuf) && !STF_FLAGGED(ch, STAFF_ADMIN)) || STF_FLAGGED(cbuf, STAFF_ADMIN)*/
			cbuf->GetLevel() >= ch->GetLevel()) {
			delete cbuf;
			send_to_char("Sorry, you can't do that.\n", ch);
			return;
		}
		vict = cbuf;
	}
	
	len = strlen(field);
	for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
		if (!strncmp(field, set_fields[mode].cmd, len))
			break;
	
	Lexi::String	victName = vict->GetName();	//	A hack all thanks to 'set name'
	retval = perform_set(ch, vict, mode, buf);
	mudlogf(NRM, LVL_STAFF, TRUE, "(GC) %s%s set %s's %s to %s", ch->GetName(), retval ? "" : " attempted to", victName.c_str(), field, buf); 
	if (retval) {
		if (!IS_NPC(vict)) {
			vict->Save();
			if (is_file)
				send_to_char("Saved in file.\n", ch);
		}
	}
	
	if (is_file)
		delete cbuf;
}


static char *logtypes[] = {"off", "brief", "normal", "complete", "\n"};

ACMD(do_syslog) {
	int tp;
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	one_argument(argument, arg);

	if (!*arg) {
		tp = ((PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0));
		ch->Send("Your syslog is currently %s.\n", logtypes[tp]);
	} else if (((tp = search_block(arg, logtypes, FALSE)) == -1))
		send_to_char("Usage: syslog { Off | Brief | Normal | Complete }\n", ch);
	else {
		REMOVE_BIT(PRF_FLAGS(ch), PRF_LOG1 | PRF_LOG2);
		SET_BIT(PRF_FLAGS(ch), (PRF_LOG1 * (tp & 1)) | (PRF_LOG2 * (tp & 2) >> 1));

		ch->Send("Your syslog is now %s.\n", logtypes[tp]);
	}
}



ACMD(do_depiss) {
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	CharData *vic = 0;
	CharData *mob  = 0;
 
	two_arguments(argument, arg1, arg2);

	if (arg2)
		send_to_char("Usage: depiss <victim> <mobile>\n", ch);
	else if (((vic = get_char_vis(ch, arg1, FIND_CHAR_WORLD))) /* && !IS_NPC(vic) */) {
		if (((mob = get_char_vis(ch, arg2, FIND_CHAR_WORLD))) && (IS_NPC(mob))) {
			if (mob->mob->m_Hates.IsActive() && mob->mob->m_Hates.InList(GET_ID(vic))) {
				ch->Send("%s no longer hates %s.\n", mob->GetName(), vic->GetName());
				forget(mob, vic);
			} else
				send_to_char("Mobile does not hate the victim!\n", ch);
		} else
			send_to_char("Sorry, Mobile Not Found!\n", ch);
	} else
		send_to_char("Sorry, Victim Not Found!\n", ch);
}


ACMD(do_repiss) {
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	CharData *vic = 0;
	CharData *mob  = 0;
 
	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2)
		send_to_char("Usage: repiss <victim> <mobile>\n", ch);
	else if (((vic = get_char_vis(ch, arg1, FIND_CHAR_WORLD))) /* && !IS_NPC(vic) */) {
		if (((mob = get_char_vis(ch, arg2, FIND_CHAR_WORLD))) && (IS_NPC(mob))) {
			ch->Send("%s now hates %s.\n", mob->GetName(), vic->GetName());
			remember(mob, vic);
		} else
			send_to_char("Sorry, Hunter Not Found!\n", ch);
	} else
		send_to_char("Sorry, Victim Not Found!\n", ch);
}


ACMD(do_hunt) {
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	CharData *vict = NULL;
	CharData *mob  = NULL;
 
	two_arguments(argument, arg1, arg2);

	if (!*arg1 || !*arg2)
		send_to_char("Usage: hunt <hunter> <victim>\n", ch);
	else if (!(mob = get_char_vis(ch, arg1, FIND_CHAR_WORLD)) || !IS_NPC(mob))
		send_to_char("Hunter not found.\n", ch);
	else if (!(vict = get_char_vis(ch, arg2, FIND_CHAR_WORLD)))
		send_to_char("Victim not found.\n", ch);
	else {
		delete mob->path;
		mob->path = Path2Char(mob, GET_ID(vict), 200, HUNT_GLOBAL | HUNT_THRU_DOORS);
		if (mob->path)	ch->Send("%s is now hunted by %s.\n", vict->GetName(), mob->GetName());
		else			ch->Send("%s can't reach %s.\n", mob->GetName(), vict->GetName());
	}
}


void send_to_imms(char *msg)
{
    FOREACH(DescriptorList, descriptor_list, iter)
    {
    	DescriptorData *d = *iter;
		if (d->GetState()->IsPlaying() && d->m_Character && IS_STAFF(d->m_Character))
			send_to_char(msg, d->m_Character);
	}
}


ACMD(do_vwear) {
	BUFFER(arg, MAX_INPUT_LENGTH);

	one_argument(argument, arg);

	if (!*arg) send_to_char(	"Usage: vwear <wear position>\n"
								"Possible positions are:\n"
								"finger    neck    body    head    legs    feet    hands\n"
								"shield    arms    about   waist   wrist   wield   hold   eyes\n"
								"tail\n", ch);
	else if (is_abbrev(arg, "finger"))	vwear_object(ITEM_WEAR_FINGER, ch);
	else if (is_abbrev(arg, "neck"))		vwear_object(ITEM_WEAR_NECK, ch);
	else if (is_abbrev(arg, "body"))		vwear_object(ITEM_WEAR_BODY, ch);
	else if (is_abbrev(arg, "head"))		vwear_object(ITEM_WEAR_HEAD, ch);
	else if (is_abbrev(arg, "legs"))		vwear_object(ITEM_WEAR_LEGS, ch);
	else if (is_abbrev(arg, "feet"))		vwear_object(ITEM_WEAR_FEET, ch);
	else if (is_abbrev(arg, "hands"))		vwear_object(ITEM_WEAR_HANDS, ch);
	else if (is_abbrev(arg, "arms"))		vwear_object(ITEM_WEAR_ARMS, ch);
	else if (is_abbrev(arg, "shield"))		vwear_object(ITEM_WEAR_SHIELD, ch);
	else if (is_abbrev(arg, "about body"))	vwear_object(ITEM_WEAR_ABOUT, ch);
	else if (is_abbrev(arg, "waist"))		vwear_object(ITEM_WEAR_WAIST, ch);
	else if (is_abbrev(arg, "wrist"))		vwear_object(ITEM_WEAR_WRIST, ch);
	else if (is_abbrev(arg, "wield"))		vwear_object(ITEM_WEAR_WIELD, ch);
	else if (is_abbrev(arg, "hold"))		vwear_object(ITEM_WEAR_HOLD, ch);
	else if (is_abbrev(arg, "eyes"))		vwear_object(ITEM_WEAR_EYES, ch);
	else if (is_abbrev(arg, "tail"))		vwear_object(ITEM_WEAR_TAIL, ch);
	else send_to_char(	"Possible positions are:\n"
						"finger    neck    body    head    legs    feet    hands\n"
						"shield    arms    about   waist   wrist   wield   hold   eyes\n"
						"tail\n", ch);
}


extern int mother_desc, port;

#define EXE_FILE "bin/circle" /* maybe use argv[0] but it's not reliable */

/* (c) 1996-97 Erwin S. Andreasen <erwin@pip.dknet.dk> */
ACMD(do_copyover) {
	FILE *fp, *fp2;
	char buf[256], buf2[64];

	fp = fopen (COPYOVER_FILE, "w");

	if (!fp) {
		send_to_char ("Copyover file not writeable, aborted.\n",ch);
		return;
	}

	fp2 = fopen (IDENTCOPYOVER_FILE, "w");

	if (!fp2) {
		send_to_char ("Copyover file not writeable, aborted.\n",ch);
		fclose(fp);
		return;
	}
	
	/* Consider changing all saved areas here, if you use OLC */
	sprintf (buf, "\n *** COPYOVER by %s - please remain seated!\n", ch->GetName());
	log("COPYOVER by %s", ch->GetName());
	
	/* For each playing descriptor, save its state */
    FOREACH(DescriptorList, descriptor_list, iter)
    {
    	DescriptorData *d = *iter;
		CharData * och = d->GetOriginalCharacter();
		
		if (!och || !d->GetState()->IsPlaying() ) { /* drop those logging on */
			d->Send( "\nSorry, we are rebooting. Come back in a few minutes.\n" );
			d->Close(); /* throw'em out */
		} else {
			fprintf (fp, "%d %d %s %s\n", d->m_Socket, /*d->CompressionActive()*/ 0, och->GetName(), d->m_Host.c_str());
			fprintf (fp2, "%d\n", d->m_Socket);

			/* save och */
			och->Save();
			SavePlayerObjectFile(och);
			d->Send(buf);
			d->StopCompression();
		}
	}

	fprintf (fp, "-1\n");
	fclose (fp);
	fprintf (fp2, "-1\n");
	fclose (fp2);

	SavePersistentObjects();
	House::SaveAllHousesContents();
	Clan::Save();
	
//	IMC::Shutdown();
	imc_shutdown(FALSE);
	
//	if (no_external_procs)
//		delete Ident;		//	We don't control Ident here, unless no external procs
	
	/* Close reserve and other always-open files and release other resources */

	/* exec - descriptors are inherited */

	sprintf (buf, "%d", port);
	sprintf (buf2, "-C%d", mother_desc);

	/* Ugh, seems it is expected we are 1 step above lib - this may be dangerous! */
	chdir ("..");

	log("Shutting down Ident Server");
	delete Ident;
	
	Buffer::Exit();
	
#if !defined(_WIN32)
	//	Terminate Deadlock Protection
	struct itimerval itime;
	struct timeval interval;
	interval.tv_sec = 0;
	interval.tv_usec = 0;
	itime.it_interval = interval;
	itime.it_value = interval;
	setitimer(ITIMER_VIRTUAL, &itime, NULL);
	
	execl (EXE_FILE, "circle", buf2, buf, NULL);
	/* Failed - sucessful exec will not return */
#endif

	perror ("do_copyover: execl");
	send_to_char ("Copyover FAILED!\n",ch);
	
	exit (1); /* too much trouble to try to recover! */
}








static void do_rewardteam(CharData *ch, char *teamname, char *argument)
{
	CharData * victim;
	int amount;
	int team;
	
	if (!*teamname || !*argument || (amount = atoi(argument)) == 0)
		send_to_char("Usage: reward team <team> <amount>\n", ch);
	else if ((team = search_block(teamname, team_names, false)) <= 0)
		send_to_char("Unknown team.", ch);
	else {
		ch->Send((amount > 0) ? "You give %d MP to team %s.\n" :
					"You take away %d MP from team %s.\n",
				abs(amount), team_names[team]);
		
		mudlogf(BRF, LVL_STAFF, TRUE, 
				(amount > 0) ? "(GC) %s gives %d MP to team %s" :
					"(GC) %s takes away %d MP from team %s",
				ch->GetName(),
				abs(amount),
				team_names[team]);
		
	    FOREACH(DescriptorList, descriptor_list, iter)
	    {
			victim = (*iter)->m_Character;
			
			if (!victim || IS_NPC(victim) || GET_TEAM(victim) != team)
				continue;
			
			if ((amount < 0) && (amount < (0-GET_MISSION_POINTS(victim)))) {
				amount = 0 - (GET_MISSION_POINTS(victim));
				ch->Send("%s only has %d MP... taking all of them away.\n", victim->GetName(), GET_MISSION_POINTS(victim));
			}
			
			if (amount != 0)
			{
				GET_MISSION_POINTS(victim) += amount;
				if (amount > 0) victim->points.lifemp += amount;
				check_level(victim);
				
				ch->Send((amount > 0) ? "You give %d MP to %s.\n" :
							"You take away %d MP from %s.\n",
						abs(amount),
						victim->GetName());
				
				victim->Send((amount > 0) ? "%s gives %d MP to you.\n" : "%s takes away %d MP from you.\n",
						PERS(ch, victim),
						abs(amount));
				
				victim->Save();
				mudlogf(BRF, LVL_STAFF, TRUE, 
						(amount > 0) ? "(GC) %s gives %d MP to %s [team %s]" :
							"(GC) %s takes away %d MP from %s [team %s]",
						ch->GetName(),
						abs(amount),
						victim->GetName(),
						team_names[GET_TEAM(victim)]);
			}
		}
	}
}


ACMD(do_reward) {
	CharData * victim;
	int amount;
	bool is_file = false;
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	
	argument = two_arguments(argument, arg1, arg2);
	
	if (!str_cmp(arg1, "team"))
		do_rewardteam(ch, arg2, argument);
	else if (!*arg1 || !*arg2 || (amount = atoi(arg2)) == 0)
		send_to_char("Usage: reward <player> <amount>\n", ch);
	else
	{
		if (!(victim = get_char_vis(ch, arg1, FIND_CHAR_WORLD)))
		{
			if (subcmd == SCMD_SALARY)
			{
				victim = new CharData();
				if (!victim->Load(arg1))
				{
					delete victim;
					victim = NULL;
				}
				else
				{
					is_file = true;
				}
			}
		}
		
		if (!victim)
			send_to_char(NOPERSON, ch);
		else if (IS_NPC(victim))
			send_to_char("Not on NPCs, you dolt!\n", ch);
		else
		{
			if ((amount < 0) && (amount < (0-GET_MISSION_POINTS(victim)))) {
				amount = 0 - (GET_MISSION_POINTS(victim));
				ch->Send("%s only has %d MP... taking all of them away.\n", victim->GetName(), GET_MISSION_POINTS(victim));
			}
			
			if (amount == 0)
				send_to_char("Zero MP, eh?", ch);
			else {			
				GET_MISSION_POINTS(victim) += amount;
				if (amount > 0) victim->points.lifemp += amount;
				check_level(victim);
				
				ch->Send((amount > 0) ? "You give %d MP to %s.\n" :
							"You take away %d MP from %s.\n",
						abs(amount),
						victim->GetName());
				
				victim->Send((amount > 0) ? "%s gives %d MP to you.\n" : "%s takes away %d MP from you.\n",
						PERS(ch, victim),
						abs(amount));
				
				victim->Save();
				if (subcmd == SCMD_SALARY)
				{
					mudlogf(BRF, LVL_ADMIN, TRUE, 
							(amount > 0) ? "(GC) SALARY - %s paid %s %d MP" :
								"(GC) SALARY - %s docked %s by %d MP",
							ch->GetName(),
							victim->GetName(),
							abs(amount));
				}
				else
				{
					mudlogf(BRF, LVL_STAFF, TRUE, 
							(amount > 0) ? "(GC) %s gives %d MP to %s" :
								"(GC) %s takes away %d MP from %s",
							ch->GetName(),
							abs(amount),
							victim->GetName());
				}
				
				if (is_file)
					delete victim;
			}
		}
	}
}



/*
Usage: string <type> <name> <field> [<string> | <keyword>]

For changing the text-strings associated with objects and characters.  The
format is:

Type is either 'obj' or 'char'.
Name                  (the call-name of an obj/char - kill giant)
Short                 (for inventory lists (obj's) and actions (char's))
Long                  (for when obj/character is seen in room)
Title                 (for players)
Description           (For look at.  For obj's, must be followed by a keyword)
Delete-description    (only for obj's. Must be followed by keyword)
*/
ACMD(do_string) {
	CharData *vict = NULL;
	ObjData *obj = NULL;
	
	BUFFER(type, MAX_INPUT_LENGTH);
	BUFFER(name, MAX_INPUT_LENGTH);
	BUFFER(field, MAX_INPUT_LENGTH);

	argument = three_arguments(argument, type, name, field);
	
	if (!*type || !*name || !*field || !*argument) {
		ch->Send("Usage: string [character|object] <name> <field> [<string> | <keyword>]\n");
	} else {
		if (is_abbrev(type, "character"))	vict = get_char_vis(ch, name, FIND_CHAR_WORLD);
		else if (is_abbrev(type, "object"))	obj = get_obj_vis(ch, name);
			
		if (!vict && !obj)
			ch->Send("Usage: string [character|object] <name> <field> [<string> | <keyword>]\n");
		else if (obj && obj->WornBy() != ch && obj->CarriedBy() != ch && obj->InRoom() != ch->InRoom())
			ch->Send("You can only restring items in your inventory, equipment, or the room.\n");
		else {
			if (is_abbrev(field, "name")) {
				if (vict) {
					if (IS_NPC(vict))
					{
						ch->Send("%s's name restrung to '%s'.\n", vict->GetName(), argument);
						vict->m_Keywords = argument;
					}
					else
						ch->Send("You can't re-string player's names.\n");
				} else if (obj) {
					ch->Send("%s's name restrung to '%s'.\n", obj->GetName(), argument);
					obj->m_Keywords = argument;
				}
			} else if (is_abbrev(field, "short")) {
				if (vict) {
					if (IS_NPC(vict))
					{
						ch->Send("%s's short desc restrung to '%s'.\n", vict->GetName(), argument);
						vict->m_Name = argument;
					}
					else
						ch->Send("Players don't have short descs.\n");
				} else if (obj) {
					ch->Send("%s's short desc restrung to '%s'.\n", obj->GetName(), argument);
					obj->m_Name = argument;
				}
			} else if (is_abbrev(field, "long")) {
				if (vict) {
					if (IS_NPC(vict))
					{
						BUFFER(buf, MAX_STRING_LENGTH);
						ch->Send("%s's long desc restrung to '%s'.\n", vict->GetName(), argument);
						sprintf(buf, "%s\n", argument);
						vict->m_RoomDescription = buf;
					}
					else
						ch->Send("Players don't have long descs.\n");
				} else if (obj) {
					ch->Send("%s's long desc restrung to '%s'.\n", obj->GetName(), argument);
					obj->m_RoomDescription = argument;
				}
			} else if (is_abbrev(field, "title") && vict) {
				if (IS_NPC(vict))
					ch->Send("Mobs don't have titles.\n");
				else if ((vict->GetLevel() >= LVL_STAFF) && (ch->GetLevel() != LVL_CODER))
					ch->Send("You can't restring the title of staff members!");
				else
				{
					ch->Send("%s's title restrung to '%s'.\n", vict->GetName(), argument);
					vict->m_Title = argument;
				}
			} else if (is_abbrev(field, "owner") && obj) {
				CharData *owner = get_player(argument);
				if (owner)
				{
					obj->owner = GET_ID(owner);
					act("$p's owner set to $N.", FALSE, ch, obj, owner, TO_CHAR);
					act("You now own $p.", FALSE, owner, obj, NULL, TO_CHAR);
					mudlogf(BRF, LVL_STAFF, TRUE, "(GC) %s set %s's owner to %s.", ch->GetName(), obj->GetName(), owner->GetName());
				}
			} else if (is_abbrev(field, "description")) {
				ch->Send("Feature not implemented yet.\n");
			} else if (is_abbrev(field, "delete-description")) {
				if (vict) ch->Send("Only for objects.\n");
				else if (obj)
				{
					bool bFound = false;
					FOREACH(ExtraDesc::List, obj->m_ExtraDescriptions, i)
					{
						if (!str_cmp(i->keyword.c_str(), argument))
						{
							ch->Send("%s's extra description '%s' deleted.\n", obj->GetName(), SSData(i->keyword));
							obj->m_ExtraDescriptions.erase(i);
							bFound = true;
							break;
						}
					}
					
					if (!bFound)	ch->Send("Extra description not found.\n");
				}
			} else
				ch->Send("No such field.\n");
		}
	}
}


struct AllowableStaffPermissions {
	char *	field;
	Flags	permission;
} StaffPermissions[] = {
	{ "characters",	STAFF_CHAR		},
	{ "clans",		STAFF_CLANS		},
	{ "general",	STAFF_GEN		},
	{ "help",		STAFF_HELP		},
	{ "houses",		STAFF_HOUSES	},
	{ "olc",		STAFF_OLC		},
	{ "script",		STAFF_SCRIPT	},
	{ "security",	STAFF_SECURITY	},
	{ "shops",		STAFF_SHOPS		},
	{ "socials",	STAFF_SOCIALS	},
	{ "minisec",	STAFF_MINISEC	},
	{ "miniolc",	STAFF_MINIOLC	},
	{ "olcadmin",	STAFF_OLCADMIN	},
	{ "game",		STAFF_GAME		},
	{ "imc",		STAFF_IMC		},
	{ "admin",		STAFF_ADMIN		},
	{ "srstaff",	STAFF_SRSTAFF	},
	{ "\n", 0 }
};

ACMD(do_allow) {
	int	i, l, the_cmd, cmd_num;
	CharData *vict;
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	
	if (IS_NPC(ch)) {
		send_to_char("Nope.\n", ch);
		return;
	}
	
	
	two_arguments(argument, arg1, arg2);
	
	if (!*arg1 || !*arg2 || ch->GetLevel() < LVL_SRSTAFF) {
		ch->Send("Usage: %s <name> <permission>\n"
				 "Permissions available:\n",
				subcmd == SCMD_ALLOW ? "allow" : "deny");
		for (l = 0; *StaffPermissions[l].field != '\n'; l++) {
			i = 1;
			ch->Send("%-9.9s: ", StaffPermissions[l].field);
			for (cmd_num = 0; cmd_num < num_of_cmds; cmd_num++) {
				the_cmd = cmd_sort_info[cmd_num].sort_pos;
				if (complete_cmd_info[the_cmd].minimum_level == STAFF_CMD
					&& IS_SET(complete_cmd_info[the_cmd].staffcmd, StaffPermissions[l].permission))
				{
					if (((i % 6) == 1) && (i != 1)) send_to_char("           ", ch);
					ch->Send("%-11.11s", complete_cmd_info[the_cmd].command);
					if (!(i % 6)) send_to_char("\n", ch);
					i++;
				}
			}
			if (--i % 6) send_to_char("\n", ch);
			if (i == 0) send_to_char("None.\n", ch);
		}
	} else if (!(vict = get_player_vis(ch, arg1))) {
		send_to_char("Player not found.\n", ch);
	} else if (ch->GetLevel() <= vict->GetLevel() && ch != vict) {
		act("You cannot modify $S permissions.", TRUE, ch, 0, vict, TO_CHAR);
	} else {
		for (i = 0; *StaffPermissions[i].field != '\n'; i++)
			if (is_abbrev(arg2, StaffPermissions[i].field))
				break;
		if (*StaffPermissions[i].field == '\n')
			send_to_char("Invalid choice.\n", ch);
		else if (!STF_FLAGGED(ch, STAFF_ADMIN) && i > 11)
			send_to_char("Invalid choice; only admins can allow ADMIN, OLCADMIN, IMC, and GAME commands.\n", ch);
		else {
			if (subcmd == SCMD_ALLOW)	SET_BIT(STF_FLAGS(vict), StaffPermissions[i].permission);
			else						REMOVE_BIT(STF_FLAGS(vict), StaffPermissions[i].permission);
			
			mudlogf(BRF, LVL_STAFF, TRUE, "(GC) %s %s %s access to %s commands.", ch->GetName(),
					subcmd == SCMD_ALLOW ? "allowed" : "denied", vict->GetName(), StaffPermissions[i].field);
			
			ch->Send("You %s %s access to %s commands.\n", subcmd == SCMD_ALLOW ? "allowed" : "denied",
					vict->GetName(), StaffPermissions[i].field);
			vict->Send("%s %s you access to %s commands.\n", ch->GetName(),
					subcmd == SCMD_ALLOW ? "allowed" : "denied", StaffPermissions[i].field);
		}
	}
}



ACMD(do_wizcall);
ACMD(do_wizassist);


std::list<IDNum> wizcallers;

ACMD(do_wizcall) {
	if (Lexi::IsInContainer(wizcallers, GET_ID(ch))) {
		wizcallers.remove(GET_ID(ch));
		send_to_char("You remove yourself from the help queue.\n", ch);
		mudlogf(BRF, LVL_IMMORT, FALSE, "ASSIST: %s no longer needs assistance.", ch->GetName());
	} else {
		wizcallers.push_back(GET_ID(ch));
		send_to_char("A staff member will be with you as soon as possible.\n", ch);
		mudlogf(BRF, LVL_IMMORT, FALSE, "ASSIST: %s calls for assistance.", ch->GetName());
	}
}


ACMD(do_wizassist)
{
	CharData *who = NULL;
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	one_argument(argument, arg);
	
	unsigned int which = atoi(arg) - 1;
	
	//	Remove missing players
	for (std::list<IDNum>::iterator i = wizcallers.begin(); i != wizcallers.end(); )
	{
		if (CharData::Find(*i))	++i;
		else					i = wizcallers.erase(i);
	}
	
	if (wizcallers.empty())
	{
		send_to_char("Nobody needs assistance right now.\n", ch);
	}
	else if (!*arg || !is_number(arg))
	{
		send_to_char("Players who need assistance\n"
					 "---------------------------\n", ch);
		
		which = 0;
		for (std::list<IDNum>::iterator i = wizcallers.begin(); i != wizcallers.end();)
		{
			who = CharData::Find(*i);
			
			if (who && who->InRoom())
			{
				ch->Send("%2d. %s\n", ++which, who->GetName());
				++i;	
			}
			else
			{
				i = wizcallers.erase(i);				
			}
		}
		
		if (wizcallers.empty())
			send_to_char("Nobody needs assistance right now.\n", ch);
	}
	else if (which >= wizcallers.size())
		send_to_char("That number doesn't exist in the help queue.\n", ch);
	else
	{
		std::list<IDNum>::iterator i = wizcallers.begin();
		std::advance(i, which);
		
		who = CharData::Find(*i);
		wizcallers.erase(i);
		
		if (ch == who)
			send_to_char("You can't help yourself.  You're helpless!\n", ch);
		else if (!who || who->InRoom() == NULL)
			send_to_char("That number doesn't exist in the help queue. [ERROR - Report]\n", ch);
		else
		{
			mudlogf(BRF, LVL_STAFF, FALSE, "ASSIST: %s goes to assist %s.", ch->GetName(), who->GetName());
			ch->FromRoom();
			ch->ToRoom(IN_ROOM(who));
			look_at_room(ch, 1);
			ch->Send("You go to assist %s.\n", who->GetName());
			act("$n appears in the room, to assist $N.", TRUE, ch, 0, who, TO_NOTVICT);
			act("$n appears in the room, to assist you.", TRUE, ch, 0, who, TO_VICT);
		} 
	}
}


extern bool runScripts;
ACMD(do_stopscripts) {
	runScripts = !runScripts;
	ch->Send("Scriptengine %s.\n", runScripts ? "started" : "stopped");
}

#if 0
ACMD(do_setcapture) {
	static char *capturetypes[] = {"disable", "normal", "enable", "\n"};
	extern int capture_system_override;
	extern bool capture_system_balanced;
	int override = capture_system_override;
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	one_argument(argument, arg);
	override = search_block(arg, capturetypes, FALSE);
	
	if (!*arg || override == -1) {
		ch->Send("Capture System: %s\n"
				"Balanced      : %s\n"
				"\n"
				"You may set the status to 'enable', 'disable', and 'normal'.\n",
						capture_system_override == -1 ? "Disabled" :
								(capture_system_override == 1 ? "Enabled" : "Normal"),
						YESNO(capture_system_balanced));
	} else {
		override -= 1;
#if 0
		if (!str_cmp(arg, "disable"))		override = -1;
		else if (!str_cmp(arg, "normal"))	override = 0;
		else if (!str_cmp(arg, "enable"))	override = 1;
#endif
		
		if (override == capture_system_override)
			ch->Send("No change made - the Capture System is already in that state.\n");
		else {
			switch (override) {
				case -1:	ch->Send("Force-disabling Capture System.\n");				break;
				case 0:		ch->Send("Restoring Capture System to normal operation.\n");	break;
				case 1:		ch->Send("Force-enabling Capture System.\n");					break;
			}
			ChangeOverride(ch, override);
		}
	}
}
#endif


ACMD(do_team)
{
	int team;
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	
	argument = two_arguments(argument, arg1, arg2);
	
	team = search_block(arg1, team_names, 0);
	
	if (team <= 0)
		send_to_char("Valid teams are: solo, red, blue, green, gold, teal, purple.\n", ch);
	else if (!str_cmp(arg2, "set"))
	{
		argument = two_arguments(argument, arg1, arg2);
		
		if (!str_cmp(arg1, "loadroom"))
		{
			RoomData *room = NULL;
			
			if (!*arg2)
				ch->Send("Usage: team <teamname> set loadroom <room number>.\n");
			else if (*arg2 && (room = world.Find(arg2)) == NULL && atoi(arg2) != -1)
				ch->Send("Room %s does not exist.\n", arg2);
			else
			{
				if (room == NULL)
					ch->Send("%s's recall room cleared.\n", team_titles[team]);
				else
					ch->Send("%s's recall room set to %s.\n", team_titles[team], room->GetVirtualID().Print().c_str());
				team_recalls[team] = room;
			}
		}
		else
			send_to_char("Usage: team <teamname> set <value> ...args...\n"
				"Valid values are: loadroom\n",
				ch);
	}
	else if (!str_cmp(arg2, "reset"))
	{
		team_recalls[team] = NULL;
		ch->Send("Team %s reset.\n", team_names[team]);
		
//		START_ITER(iter, d, DescriptorData *, descriptor_list)
//			if (d->m_Character && GET_TEAM(d->m_Character) == team)
//				GET_TEAM(d->m_Character) = 0;
		FOREACH(CharacterList, character_list, iter)
		{
			CharData *vict = *iter;
			if (!IS_NPC(vict) && GET_TEAM(vict) == team)
				GET_TEAM(vict) = 0;
		}
	}
	else if (!str_cmp(arg2, "add"))
	{
		while (*argument)
		{
			argument = one_argument(argument, arg1);
			CharData *vict = get_player(arg1);
			if (vict)
			{
				ch->Send("Setting %s's team to %s.\n", vict->GetName(), team_names[team]);
				GET_TEAM(vict) = team;
			}
			else
				ch->Send("Unknown player \"%s\".\n", arg1);
		}
	}
	else if (!str_cmp(arg2, "remove"))
	{
		while (*argument)
		{
			argument = one_argument(argument, arg1);
			CharData *vict = get_player(arg1);
			if (!vict)
				ch->Send("Unknown player \"%s\".\n", arg1);
			else if (GET_TEAM(vict) != team)
				ch->Send("%s is not on team %s.\n", vict->GetName(), team_names[team]);
			else
			{
				ch->Send("Removing %s from team %s.\n", vict->GetName(), team_names[team]);
				GET_TEAM(vict) = 0;
			}
		}
	}
	else
		send_to_char("Usage: team <teamname> <cmd> ...args...\n"
			"Valid commands are: set reset add remove\n",
			ch);
}


ACMD(do_objindex)
{
	ZoneData *zn = zone_table.Find(argument);
	
	if (!zn)
	{
		ch->Send("'%s' is not a valid zone.\n", argument);
		return;
	}
	
	BUFFER(buf, MAX_STRING_LENGTH * 4);
	BUFFER(buf2, MAX_STRING_LENGTH);
	
	bool found = false;
	
	sprintf(buf, "Objects in zone '%s':\n\n", zn->GetTag());
	
	FOREACH_BOUNDED(ObjectPrototypeMap, obj_index, zn->GetHash(), i)
	{
		ObjectPrototype *proto = *i;
		
		bool storeFound = false;
		bool roomFound = false;
		bool mobFound = false;
		
		*buf2 = 0;

		if (proto->m_Count > 0)
			sprintf_cat(buf2, "\n        Loaded: %d", proto->m_Count);

		for (int shop_nr = 0; shop_nr < shop_index.size(); ++shop_nr)
		{
			ShopData *shop = shop_index[shop_nr];
			
			for (int counter = 0; counter < shop->producing.size(); ++counter)
			{
				if (SHOP_PRODUCT(shop, counter) == proto)
				{
					if (!storeFound)
						strcat(buf2, "\n        Sold in:");
					storeFound = true;
					
					sprintf_cat(buf2, " %s", shop->GetVirtualID().Print().c_str());
					break;
				}
			}
		}

#if 0
		for (int zone_nr = 0; zone_nr <= top_of_zone_table; ++zone_nr)
		{
			for (int room_nr = zone * 100; room_nr <= zn->top; ++room_nr)
			{
				RoomData *room = world.Find(room_nr);
				ObjData *obj;
				
				if(room)
				{
/*					
					START_ITER(obj_iter, obj, ObjData *, object_list)
					{
						if (obj->GetPrototype() == proto && obj->Room() == room)
						{
							if (!roomFound)
								strcat(buf2, "\n        Found in:");
							roomFound = true;
							
							sprintf(buf + strlen(buf), " %d", room_nr);
							break;
						}
					}
					START_ITER(obj_iter, obj, ObjData *, room->contents)
					{
						if (GET_OBJ_RNUM(obj) == rnum)
						{
							if (!roomFound)
								strcat(buf2, "\n        Found in:");
							roomFound = true;
							
							sprintf(buf + strlen(buf), " %d%s", room_nr, isReset ? "[R]" : "");
							break;
						}
					}
*/
					if (obj->InRoom() == room)
					{
						if (!roomFound)
							strcat(buf2, "\n        Found in:");
						roomFound = true;
						
						sprintf_cat(buf, " %d", room_nr);
					}
				}
			}
		}
#endif

		sprintf_cat(buf,
			"%10s - %s%s\n",
			proto->GetVirtualID().Print().c_str(),
			proto->m_pProto->m_Name.empty() ? "Unnamed" : proto->m_pProto->GetName(),
			buf2);
		found = true;

        if (strlen(buf) > ((MAX_STRING_LENGTH * 4) - 1024)) {
                strcat(buf, "*** OVERFLOW ***\n");
                break;
        }
	}
	if (!found) strcat(buf, "None!\n");

	page_string(ch->desc, buf);
}


ACMD(do_mobindex)
{
	ZoneData *zn = zone_table.Find(argument);
	
	if (!zn)
	{
		ch->Send("'%s' is not a valid zone.\n", argument);
		return;
	}
	
	BUFFER(buf, MAX_STRING_LENGTH * 4);
	
	bool found = false;
	sprintf(buf, "Mobs in zone '%s':\n\n", zn->GetTag());
	
	FOREACH_BOUNDED(CharacterPrototypeMap, mob_index, zn->GetHash(), i)
	{
		CharacterPrototype *proto = *i;

		sprintf_cat(buf,
			"%10s - %s\n",
			proto->GetVirtualID().Print().c_str(),
			proto->m_pProto->m_Name.empty() ? "Unnamed"
				: proto->m_pProto->m_Name.c_str());
			
		if (proto->m_Count > 0)
			sprintf_cat(buf, "        Loaded: %d\n", proto->m_Count);
		
		found = true;

        if (strlen(buf) > ((MAX_STRING_LENGTH * 4) - 1024)) {
                strcat(buf, "*** OVERFLOW ***\n");
                break;
        }
	}
	if (!found) strcat(buf, "None!\n");

	page_string(ch->desc, buf);
}


ACMD(do_trigindex)
{
	ZoneData *zn = zone_table.Find(argument);
	
	if (!zn)
	{
		ch->Send("'%s' is not a valid zone.\n", argument);
		return;
	}
	
	BUFFER(buf, MAX_STRING_LENGTH * 4);
	
	bool found = false;
	sprintf(buf, "Scripts in zone '%s':\n\n", zn->GetTag());
	FOREACH_BOUNDED(ScriptPrototypeMap, trig_index, zn->GetHash(), i)
	{
		ScriptPrototype *proto = *i;
		
		sprintf_cat(buf, "%10s - %s\n", proto->GetVirtualID().Print().c_str(), proto->m_pProto->GetName());
			
		if (proto->m_Count > 0)
			sprintf_cat(buf, "        Loaded: %d\n", proto->m_Count);
			
		found = true;
			
        if (strlen(buf) > ((MAX_STRING_LENGTH * 4) - 1024)) {
                strcat(buf, "*** OVERFLOW ***\n");
                break;
        }
	}
	if (!found) strcat(buf, "None!\n");

	page_string(ch->desc, buf);
}



ACMD(do_killall)
{
	bool bAll = false;
	ZoneData *zone = NULL;
	BUFFER(arg, MAX_INPUT_LENGTH);

	one_argument(argument, arg);
	
//	if (str_cmp(arg, "all"))
//		ch->Send("Invalid usage.  Do not use this command without permission from FearItself.\n");
//	else
	if (!str_cmp(arg, "all") && STF_FLAGGED(ch, STAFF_ADMIN))
		bAll = true;
	
	if (!bAll && !(zone = zone_table.Find(arg)))
		ch->Send("That is not a valid zone.\n");
	else
	{
		int count = 0;
		FOREACH(CharacterList, character_list, iter)
		{
			CharData *vict = *iter;
			if (IS_NPC(vict) && (bAll || zone == vict->InRoom()->GetZone()))
			{
				vict->AbortTimer();
				vict->die(NULL);
				++count;
			}
		}
		
		ch->Send("%d mobs terminated.\n", count);
	}
}


ACMD(do_listhelp)
{
	BUFFER(buf, MAX_STRING_LENGTH * 4);

	strcpy(buf, "Help files:\n");
	int i = 1;
	FOREACH_CONST(std::vector<HelpManager::Entry *>, HelpManager::Instance()->GetTable(), help)
		sprintf_cat(buf, "%4.4d: %s\n", i, (*help)->m_Keywords.c_str());
	
	page_string(ch->desc, buf);
}




ACMD(do_crash)
{
        switch (atoi(argument))
        {
                case 1:
                {
                        int *i = 0;
                        *i = 1;
                        break;
                }
                case 2:
                {
                        void *foo = malloc(1000);
                        free(foo);
                        free(foo);
                        break;
                }
                case 3:
                {
                        abort();
                        break;
                }
        }
}


#if __profile__

#include <profiler.h>
ACMD(do_profile)
{
	static bool bProfiling = false;
	
	bProfiling = !bProfiling;

	if (bProfiling)
	{
		ProfilerInit(collectDetailed, bestTimeBase, 20000, 200);
		ch->Send("Profiling enabled.\n");
	}
	else
	{
		ProfilerDumpC("Profile Report");
		ProfilerTerm();
		ch->Send("Profiling finished.\n");
	}
}

#endif
