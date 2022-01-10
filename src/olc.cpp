/***************************************************************************
 *  OasisOLC - olc.c                                                       *
 *                                                                         *
 *  Copyright 1996 Harvey Gilpin.                                          *
 ***************************************************************************/

#include <typeinfo>

#include "structs.h"
#include "interpreter.h"
#include "comm.h"
#include "utils.h"
#include "scripts.h"
#include "db.h"
#include "handler.h"
#include "find.h"
#include "olc.h"
#include "clans.h"
#include "shop.h"
#include "help.h"
#include "buffer.h"
#include "socials.h"
#include "staffcmds.h"
#include "constants.h"
#include "weather.h"
#include "boards.h"
#include "lexifile.h"
#include "house.h"


//	External Functions.
extern void zedit_setup(DescriptorData *d, RoomData *room_num);
extern void zedit_save_to_disk(ZoneData *zone);
extern void zedit_new_zone(CharData *ch, char *name);
extern void zedit_rename_zone(CharData *ch, char *src, char *dest);
extern void medit_setup(DescriptorData *d, CharacterPrototype *proto);
extern void medit_save_to_disk(ZoneData *zone);
extern void oedit_setup(DescriptorData *d, ObjectPrototype *proto);
extern void oedit_save_to_disk(ZoneData *zone);
extern void sedit_setup(DescriptorData *d, ShopData *shop);
extern void sedit_save_to_disk(ZoneData *zone);
extern void scriptedit_setup(DescriptorData *d, ScriptPrototype *proto);
extern void scriptedit_save_to_disk(ZoneData *zone);
extern void cedit_setup(DescriptorData *d, Clan *clan);
extern void aedit_save_to_disk();
extern void do_stat_object(CharData * ch, ObjData * j);
extern void do_stat_character(CharData * ch, CharData * k);
extern void do_stat_room(CharData * ch, RoomData *room);
extern void hedit_save_to_disk();
extern void hedit_setup_new(DescriptorData *d, char *arg1);
extern void hedit_setup_existing(DescriptorData *d);
extern void bedit_setup_new(DescriptorData *d, const char *arg1);
extern void bedit_setup_existing(DescriptorData *d, BehaviorSet *set);


//	Internal Functions
ACMD(do_olc);
ACMD(do_olc_other);
ACMD(do_zallow);
ACMD(do_zdeny);
ACMD(do_ostat);
ACMD(do_mstat);
ACMD(do_rstat);
ACMD(do_zstat);
ACMD(do_zlist);
ACMD(do_delete);
ACMD(do_liblist);
ACMD(do_massolcsave);
ACMD(do_dig);
ACMD(do_vattached);
ACMD(do_olccopy);
ACMD(do_olcmove);
ACMD(do_tedit);



struct olc_scmd_data {
  char *text;
//  int con_type;
	Lexi::String con_type;
};

struct olc_scmd_data olc_scmd_info[] =
{ {"room"	, typeid(RoomEditOLCConnectionState).name() },
  {"object"	, typeid(ObjectEditOLCConnectionState).name() },
  {"room"	, typeid(ZoneEditOLCConnectionState).name() },
  {"mobile"	, typeid(MobEditOLCConnectionState).name() },
  {"shop"	, typeid(ShopEditOLCConnectionState).name() },
  {"action"	, typeid(ActionEditOLCConnectionState).name() },
  {"help"	, typeid(HelpEditOLCConnectionState).name() },
  {"script"	, typeid(ScriptEditOLCConnectionState).name() },
  {"clan"	, typeid(ClanEditOLCConnectionState).name() },
  {"behaviors", typeid(BehaviorEditOLCConnectionState).name() },
  {"\n"		, ""		   }
};

/*------------------------------------------------------------*/

/*
 * Exported ACMD do_olc function.
 *
 * This function is the OLC interface.  It deals with all the 
 * generic OLC stuff, then passes control to the sub-olc sections.
 */
ACMD(do_olc)
{
	VirtualID vid;
	bool save = false;
	char *type = "ERROR";

	if (IS_NPC(ch) || !ch->desc)	// No screwing arround
		return;

	if (subcmd == SCMD_OLC_SAVEINFO) {
		ch->Send("This command does nothing, now.  Everything is saved immediately.\n");
		return;
	}
	
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	BUFFER(arg3, MAX_INPUT_LENGTH);
	
	/*. Parse any arguments .*/
	three_arguments(argument, arg1, arg2, arg3);
	
	if (!*arg1) {	//	No argument given
		switch(subcmd)
		{
			case SCMD_OLC_ZEDIT:
			case SCMD_OLC_REDIT:
				vid = ch->InRoom()->GetVirtualID();
				break;
			case SCMD_OLC_OEDIT:
			case SCMD_OLC_MEDIT:
			case SCMD_OLC_SEDIT:
			case SCMD_OLC_SCRIPTEDIT:
				ch->Send("Specify a %s Virtual ID to edit.\n", olc_scmd_info[subcmd].text);
				break;
		}
		if (!vid.IsValid())
			return;
	}
	else if (!IsVirtualID(arg1))
	{
		if (!strncmp("save", arg1, 4))
		{
			if (!*arg1)
			{
				send_to_char("Save which zone?\n", ch);
				return;
			}
			else
			{
				save = 1;
				vid.zone = GetStringFastHash(arg2);
			}
		}
		else if (subcmd == SCMD_OLC_ZEDIT && STF_FLAGGED(ch, STAFF_OLCADMIN))
		{
			if (!str_cmp("new", arg1))			zedit_new_zone(ch, arg2);
			else if (!str_cmp("rename", arg1))	zedit_rename_zone(ch, arg2, arg3);
			else ch->Send("Yikes!  Stop that, someone will get hurt!\n");
			return;
		}
		else
		{
			ch->Send("Yikes!  Stop that, someone will get hurt!\n");
			return;
		}
	}

	/*. If a numeric argument was given, get it .*/
	if (!vid.IsValid())
		vid = VirtualID(arg1, ch->InRoom()->GetVirtualID().zone);

	/*. Check whatever it is isn't already being edited .*/
	if (vid.IsValid() && !save)
	{
		FOREACH(DescriptorList, descriptor_list, iter)
		{
			DescriptorData *d = *iter;
			if ( olc_scmd_info[subcmd].con_type == typeid(*d->GetState()).name() )
			{
				if (d->olc && (OLC_VID(d) == vid)) {
					ch->Send("That %s is currently being edited by %s.\n",
							olc_scmd_info[subcmd].text,
							(ch->CanSee(d->m_Character, GloballyVisible) != VISIBLE_NONE ? d->m_Character->GetName() : "someone"));
					return;
				}
			}
		}
	}
	
	/*. Find the zone .*/
	ZoneData *zone = zone_table.Find(vid.zone);
	if (!zone)
	{
		send_to_char ("Sorry, there is no zone for that Virtual ID!\n", ch); 
		return;
	}
	
	/*. Everyone but IMPLs can only edit zones they have been assigned .*/
	if (!get_zone_perms(ch, vid))
	{
		return;
	}
/*
	else if (ch->GetLevel() < LVL_SRSTAFF) {
		if (subcmd == SCMD_OLC_AEDIT)
			send_to_char("You do not have permission to edit actions.\n", ch);
		else if (subcmd == SCMD_OLC_HEDIT)
			send_to_char("You do not have permission to edit help files.\n", ch);
		else if (subcmd == SCMD_OLC_CEDIT)
			send_to_char("You do not have permission to edit clans.\n", ch);
		else
			send_to_char("You do not have permission to edit something or other...\n", ch);
		d->olc = NULL;
		return;
	}
*/

	if(save)
	{
		switch(subcmd)
		{
			case SCMD_OLC_REDIT: type = "room";	break;
			case SCMD_OLC_ZEDIT: type = "zone";	break;
			case SCMD_OLC_SEDIT: type = "shop"; break;
			case SCMD_OLC_MEDIT: type = "mobile"; break;
			case SCMD_OLC_OEDIT: type = "object"; break;
			case SCMD_OLC_SCRIPTEDIT: type = "script"; break;
		}

		if (!type)
		{
			send_to_char("Oops, I forgot what you wanted to save.\n", ch);
			return;
		}
		
		ch->Send("Saving all %ss in zone '%s'.\n", type, zone->GetTag());
		mudlogf(NRM, ch->GetPlayer()->m_StaffInvisLevel, TRUE, "OLC: %s saves %s info for zone '%s'.",
				ch->GetName(), type, zone->GetTag());

		switch (subcmd) {
			case SCMD_OLC_REDIT: REdit::save_to_disk(zone); break;
			case SCMD_OLC_ZEDIT: zedit_save_to_disk(zone); break;
			case SCMD_OLC_OEDIT: oedit_save_to_disk(zone); break;
			case SCMD_OLC_MEDIT: medit_save_to_disk(zone); break;
			case SCMD_OLC_SEDIT: sedit_save_to_disk(zone); break;
			case SCMD_OLC_SCRIPTEDIT: scriptedit_save_to_disk(zone); break;
		}
		
		return;
	}
	
	/*. Give descriptor an OLC struct .*/
	DescriptorData *d = ch->desc;
	d->olc = new olc_data;
	OLC_SOURCE_ZONE(d) = zone;

 
	OLC_VID(d) = vid;

	/*. Steal players descriptor start up subcommands .*/
	switch(subcmd) {
		case SCMD_OLC_REDIT:
			{
				RoomData *room = world.Find(vid);
				if (room)			REdit::setup_existing(d, room);
				else				REdit::setup_new(d);
			}
			d->GetState()->Push(new RoomEditOLCConnectionState);
			break;
		case SCMD_OLC_ZEDIT:
			{
				RoomData *room = world.Find(vid);
				if (room == NULL)
				{
					send_to_char("That room does not exist.\n", ch); 
					d->olc = NULL;
					return;
				}
				zedit_setup(d, room);
			}
			d->GetState()->Push(new ZoneEditOLCConnectionState);
			break;
		case SCMD_OLC_MEDIT:
			medit_setup(d, mob_index.Find(vid));
			d->GetState()->Push(new MobEditOLCConnectionState);
			break;
		case SCMD_OLC_OEDIT:
			oedit_setup(d, obj_index.Find(vid));
			d->GetState()->Push(new ObjectEditOLCConnectionState);
			break;
		case SCMD_OLC_SEDIT:
			sedit_setup(d, shop_index.Find(vid));
			d->GetState()->Push(new ShopEditOLCConnectionState);
			break;
		case SCMD_OLC_SCRIPTEDIT:
			scriptedit_setup(d, trig_index.Find(vid));
			d->GetState()->Push(new ScriptEditOLCConnectionState);
			break;
	}

	act("$n starts using OLC.", TRUE, d->m_Character, 0, 0, TO_ROOM);
//	mudlogf(NRM, d->m_Character->GetPlayer()->m_StaffInvisLevel, FALSE, "OLC: %s starts using OLC.", d->m_Character->GetName());
//	SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
}



ACMD(do_olc_other)
{
	int number = -1;
	char *type = "ERROR";

	if (IS_NPC(ch) || !ch->desc)	// No screwing arround
		return;

	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	
	/*. Parse any arguments .*/
	two_arguments(argument, arg1, arg2);
	
	if (!*arg1)	//	No argument given
	{
		switch(subcmd) {
			case SCMD_OLC_AEDIT:
				send_to_char("Specify an action to edit.\n", ch);
				break;
			case SCMD_OLC_HEDIT:
				send_to_char("Specify a help topic to edit.\n", ch);
				break;
			case SCMD_OLC_BEDIT:
				send_to_char("Specify a behavior set to edit.\n", ch);
				break;
			case SCMD_OLC_CEDIT:
				ch->Send("Specify a clan ID to edit.\n");
				return;
		}
		return;
	}
	else if (subcmd == SCMD_OLC_CEDIT)
	{
		if (!str_cmp("save", arg1))
		{
			send_to_char("Saving all clans.\n", ch);
			mudlogf(NRM, LVL_SRSTAFF, TRUE, "OLC: %s saves all clans", ch->GetName());
			Clan::Save();
			return;
		}
		else if (!isdigit(*arg1))
		{
			send_to_char ("Yikes!  Stop that, someone will get hurt!\n", ch);
			return;
		}
	}
	
	if (subcmd == SCMD_OLC_BEDIT)
		number = GetStringFastHash(arg1);
	else if (subcmd == SCMD_OLC_CEDIT)
	{
		number = atoi(arg1);
		
		if (number <= 0)
		{
			send_to_char ("Clan number must be > 0.\n", ch);
			return;
		}
	}
	else
		number = 0;

	/*. Check whatever it is isn't already being edited .*/
	FOREACH(DescriptorList, descriptor_list, iter)
	{
		DescriptorData *d = *iter;
		if ( olc_scmd_info[subcmd].con_type == typeid(*d->GetState()).name() )
		{
			if (d->olc && (OLC_NUM(d) == number))
			{
				switch (subcmd)
				{
					case SCMD_OLC_AEDIT:
						ch->Send("Actions are already being editted by %s.\n",
								(ch->CanSee(d->m_Character, GloballyVisible) != VISIBLE_NONE ? d->m_Character->GetName() : "someone"));
					case SCMD_OLC_HEDIT:
						ch->Send("Help files are currently being edited by %s\n", 
								(ch->CanSee(d->m_Character, GloballyVisible) != VISIBLE_NONE ? d->m_Character->GetName() : "someone"));
					default:
						ch->Send("That %s is currently being edited by %s.\n",
								olc_scmd_info[subcmd].text,
								(ch->CanSee(d->m_Character, GloballyVisible) != VISIBLE_NONE ? d->m_Character->GetName() : "someone"));
				}
				return;
			}
		}
	}
	
	DescriptorData *d = ch->desc;

	/*. Give descriptor an OLC struct .*/
	d->olc = new olc_data;
	
	/*. Find the zone .*/
	if (subcmd == SCMD_OLC_AEDIT)
	{
		OLC_NUM(d) = 0;
		OLC_STORAGE(d) = arg1;
		for (OLC_NUM(d) = 0; (OLC_NUM(d) < socials.size()); OLC_NUM(d)++)  {
			if (is_abbrev(OLC_STORAGE(d).c_str(), socials[OLC_NUM(d)]->command.c_str()))
				break;
		}
		if (OLC_NUM(d) >= socials.size())  {
			if (find_command(OLC_STORAGE(d).c_str()) > NOTHING)  {
				d->olc = NULL;
				send_to_char("That command already exists.\n", ch);
				return;
			}
			ch->Send("Do you wish to add the '%s' action? ", OLC_STORAGE(d).c_str());
			OLC_MODE(d) = AEDIT_CONFIRM_ADD;
		} else {
			ch->Send("Do you wish to edit the '%s' action? ", socials[OLC_NUM(d)]->command.c_str());
			OLC_MODE(d) = AEDIT_CONFIRM_EDIT;
		}
	}
	else if (subcmd == SCMD_OLC_HEDIT)
		OLC_ORIGINAL_HELP(d) = HelpManager::Instance()->Find(arg1);
	else
		OLC_NUM(d) = number;

	/*. Steal players descriptor start up subcommands .*/
	switch(subcmd)
	{
		case SCMD_OLC_AEDIT:
			d->GetState()->Push(new ActionEditOLCConnectionState);
			break;
		case SCMD_OLC_BEDIT:
			{
				BehaviorSet *set = BehaviorSet::Find(arg1);
				if (set)					bedit_setup_existing(d, set);
				else						bedit_setup_new(d, arg1);				
			}
			d->GetState()->Push(new BehaviorEditOLCConnectionState);
			break;
		case SCMD_OLC_HEDIT:
			if (OLC_ORIGINAL_HELP(d))	hedit_setup_existing(d);
			else						hedit_setup_new(d, arg1);
			d->GetState()->Push(new HelpEditOLCConnectionState);
			break;
		case SCMD_OLC_CEDIT:
			cedit_setup(d, Clan::GetClan(number));
			d->GetState()->Push(new ClanEditOLCConnectionState);
			break;
	}

	act("$n starts using OLC.", TRUE, d->m_Character, 0, 0, TO_ROOM);
//	mudlogf(NRM, d->m_Character->GetPlayer()->m_StaffInvisLevel, FALSE, "OLC: %s starts using OLC.", d->m_Character->GetName());
//	SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
}


/*------------------------------------------------------------*\
 Exported utilities 
\*------------------------------------------------------------*/


/*. This procdure frees up the strings and/or the structures
    attatched to a descriptor, sets all flags back to how they
    should be .*/

olc_data::~olc_data()
{}


void cleanup_olc(DescriptorData *d)
{
	d->GetState()->Pop();
}


ACMD(do_zallow)
{
	ZoneData *zone;
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);

	two_arguments(argument, arg1, arg2);

	if(!*arg1 || !*arg2)
	{
		send_to_char("Usage  : zallow <player> <zone>\n", ch);
		return;
	}
	
	*arg1 = toupper(*arg1);
	
	strcat(arg2, ":0");
	VirtualID	vid(arg2);
	
//	zone = zone_table.Find(arg2);
	zone = zone_table.Find(vid.zone);
	
	if (!zone)
	{
		send_to_char("That zone doesn't seem to exist.\n", ch);
		return;
	}
	else if (!STF_FLAGGED(ch, STAFF_OLCADMIN) && !zone->IsOwner(ch->GetID()))
	{
		send_to_char("You do not have permission to allow access to that zone.\n", ch);	
		return;	
	}

	IDNum staff = get_id_by_name(arg1);
	if (!staff)
	{
		send_to_char("What?  Who?  No such person.\n", ch);	
		return;	
	}

	if (zone->IsBuilder(staff, 0))
	{
		ch->Send("That person already has global access to that zone.\n");
		return;	
	}

	if (zone->IsBuilder(staff, vid.ns))
	{
		ch->Send("That person already has access to that zone%s.\n", vid.ns ? ":namespace" : "");
		return;	
	}

	zone->AddBuilder(staff, vid.ns);
	zedit_save_to_disk(zone);
	
	send_to_char("Zone file edited.\n", ch);
	mudlogf(BRF, LVL_STAFF, TRUE, "olc: %s gives %s zone '%.*s' builder access.", ch->GetName(), arg1, strlen(arg2) - 2, arg2);
}


ACMD(do_zdeny)
{
	ZoneData *zone;
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	
	half_chop(argument, arg1, arg2);

	if(!*arg1 || !*arg2)
	{
		send_to_char("Usage  : zdeny <player> <zone>\n", ch);
		return;
	}
	
	*arg1 = toupper(*arg1);
	
	strcat(arg2, ":0");
	VirtualID	vid(arg2);
	
//	zone = zone_table.Find(arg2);
	zone = zone_table.Find(vid.zone);
	
	if(!zone)
	{
		send_to_char("That zone doesn't seem to exist.\n", ch);
		return;
	}
	else if (!STF_FLAGGED(ch, STAFF_OLCADMIN) && !zone->IsOwner(ch->GetID()))
	{
		send_to_char("You do not have permission to deny access to that zone.\n", ch);	
		return;	
	}
	
	IDNum staff = get_id_by_name(arg1);
	if (!staff)
	{
		send_to_char("What?  Who?  No such person.\n", ch);	
		return;	
	}
	
	if (!zone->IsBuilder(staff, vid.ns))
	{
		send_to_char("That person doesn't have access to that zone.\n", ch);
		return;
	}

	if (vid.ns && zone->IsBuilder(staff, 0))
	{
		send_to_char("That person has global access to that zone, you cannot remove individual namespaces.\n", ch);
		return;
	}
	
	zone->RemoveBuilder(staff, vid.ns);
	zedit_save_to_disk(zone);
	
	send_to_char("Zone file edited.\n", ch);
	mudlogf(BRF, LVL_STAFF, TRUE, "olc: %s removes %s's zone '%.*s' builder access.", ch->GetName(), arg1, strlen(arg2) - 2, arg2);
}


bool get_zone_perms(CharData * ch, VirtualID vid)
{
	if(STF_FLAGGED(ch, STAFF_OLCADMIN))
		return true;
	
	ZoneData *zone = zone_table.Find(vid.zone);
	
	if (!zone)
	{
		ch->Send("Error: Unknown zone.\n");
		return false;
	}
	
	if (!zone->IsBuilder(ch, vid.ns))
	{
		if (vid.ns)	ch->Send("You don't have builder rights in zone '%s:%s'.\n", zone->GetTag(), zone->GetNamespace(vid.ns));
		else		ch->Send("You don't have builder rights in zone '%s'.\n", zone->GetTag());
		return false;
	}

	return true;
}


ACMD(do_ostat)
{
	ObjectPrototype *proto;
	
	skip_spaces(argument);
	
	if(IsVirtualID(argument))
	{
		proto = obj_index.Find(VirtualID(argument, ch->InRoom()->GetVirtualID().zone));
		
		if (!proto)		send_to_char("There is no object with that ID.\n", ch);
		else			do_stat_object(ch, proto->m_pProto);
	}
	else
		send_to_char("Usage: ostat [ID]\n", ch);
}


ACMD(do_mstat) {
	CharacterPrototype *proto;
	
	skip_spaces(argument);
	
	if(IsVirtualID(argument))
	{
		proto = mob_index.Find(VirtualID(argument, ch->InRoom()->GetVirtualID().zone));
		
		if (!proto)		send_to_char("There is no mob with that ID.\n", ch);
		else			do_stat_character(ch, proto->m_pProto);
	}
	else
		send_to_char("Usage: mstat [ID]\n", ch);
}



ACMD(do_rstat)
{
	skip_spaces(argument);

	if(!*argument)
		do_stat_room(ch, ch->InRoom());
	else if (IsVirtualID(argument))
	{
		RoomData *room = world.Find(VirtualID(argument, ch->InRoom()->GetVirtualID().zone));
		
		if (!room)	send_to_char("There is no room with that number.\n", ch);
		else		do_stat_room(ch, room);
	}
	else
		send_to_char("Usage: rstat [ID]\n", ch);
}



ACMD(do_zstat)
{
	char *rmode[] = {	"Never resets",
						"Resets only when deserted",
						"Always resets"  };

	ZoneData *z = ch->InRoom()->GetZone();

	skip_spaces(argument);
	if(*argument)
	{
		z = zone_table.Find(argument);
		
		if(!z)
		{
			ch->Send("That zone doesn't exist.\n");
			return;
		}
	}
	else if(!z)
	{
		ch->Send("The room you're in is not part of a zone.\n");
		return;
	}

	ch->Send("`mZone        : `w%s\n"
			 "`mName        : `w%s\n"
			 "`mComment     : `w%s\n"
			 "`mAge/Lifespan: `w%3d`m/`w%3d%s\n"
			 "`mReset mode  : `w%s\n"
			 "`mOwner       : `w%s\n"
			 "`mBuilders    : `w",
			z->GetTag(),
			z->GetName(),
			z->GetComment(),
			z->age, z->lifespan,
			z->GetZoneNumber() >= 0 ? Lexi::Format("         `mTop vnum    : `w%5d", z->top).c_str() : "",
			rmode[z->m_ResetMode],
			z->GetOwner() != 0 ? get_name_by_id(z->GetOwner(), "") : "");

	if(z->GetBuilders().empty())	send_to_char("NONE", ch);
	int pos = 0;
	FOREACH_CONST(ZoneData::BuilderList, z->GetBuilders(), builder)
	{
		const char *playerName = get_name_by_id(builder->id);
		if (!playerName)
			continue;
		
		ch->Send("%s%s",
			pos++ > 0 ? "\n              " : "",
			playerName);
		
		if (!builder->ns.empty())
		{
			ch->Send(" (");
			FOREACH_CONST(Lexi::List<Hash>, builder->ns, ns)
			{
				const char *nsName = z->GetNamespace(*ns);
				if (!nsName)	continue;
				ch->Send("%s%s", ns != builder->ns.begin() ? ", " : "", nsName);
			}
			ch->Send(")");
		}
	}

	send_to_char("`n\n", ch);
}


ACMD(do_zlist)
{
	BUFFER(arg, MAX_INPUT_LENGTH);
	BUFFER(zonename, MAX_INPUT_LENGTH);
	BUFFER(buf, MAX_BUFFER_LENGTH);

	if (subcmd == SCMD_ZLIST)
	{
		two_arguments(argument, arg, zonename);
		
		if(!*arg)
		{
			send_to_char("Usage: zlist mobs|objects|rooms|triggers|scripts\n", ch);
			return;
		}
	}
	else
	{
		one_argument(argument, zonename);
		
		switch (subcmd)
		{
			case SCMD_OLIST:	arg = "objects";	break;
			case SCMD_MLIST:	arg = "mobs";		break;
			case SCMD_RLIST:	arg = "rooms";		break;
			case SCMD_TLIST:	arg = "scripts";	break;
		}
	}
	
	ZoneData *zone = *zonename ? zone_table.Find(zonename) : ch->InRoom()->GetZone();
	
	if (!zone)
	{
		ch->Send(*zonename ? "That is not a valid zone.\n" : "You are not in a zone.\n");
		return;
	}
	
	VirtualID vid;
	vid.zone = zone->GetHash();
	
	if (STF_FLAGGED(ch, STAFF_MINIOLC) && !STF_FLAGGED(ch, STAFF_OLC) && !get_zone_perms(ch, vid))
		return;
		
	bool found = false;
	if(is_abbrev(arg, "mobs"))
	{
		sprintf(buf, "Mobs in zone '%s':\n\n", zone->GetTag());
		FOREACH_BOUNDED(CharacterPrototypeMap, mob_index, zone->GetHash(), i)
		{
			CharacterPrototype *proto = *i;

			sprintf_cat(buf, "%10s - %s\n", proto->GetVirtualID().Print().c_str(),
					proto->m_pProto->m_Name.empty() ? "Unnamed"
					: proto->m_pProto->GetName());
			found = true;

			if (strlen(buf) > MAX_BUFFER_LENGTH - 1024) {
				strcat(buf, "*** OVERFLOW ***\n");
				break;
			}
		}
	}
	else if(is_abbrev(arg, "objects"))
	{
		sprintf(buf, "Objects in zone '%s':\n\n", zone->GetTag());

		FOREACH_BOUNDED(ObjectPrototypeMap, obj_index, zone->GetHash(), i)
		{
			ObjectPrototype *proto = *i;

			sprintf_cat(buf, "%10s - %s\n", proto->GetVirtualID().Print().c_str(),
				proto->m_pProto->m_Name.empty() ? "Unnamed" : proto->m_pProto->GetName());
			found = true;

            if (strlen(buf) > MAX_BUFFER_LENGTH - 1024)
            {
                    strcat(buf, "*** OVERFLOW ***\n");
                    break;
            }
		}
	}
	else if(is_abbrev(arg, "rooms"))
	{
		sprintf(buf, "Rooms in zone '%s':\n\n", zone->GetTag());
		FOREACH_BOUNDED(RoomMap, world, zone->GetHash(), i)
		{
			RoomData *room = *i;
			
			sprintf_cat(buf, "%10s - %s\n", room->GetVirtualID().Print().c_str(), room->GetName());
			found = true;
				
            if (strlen(buf) > MAX_BUFFER_LENGTH - 1024) {
                strcat(buf, "*** OVERFLOW ***\n");
                break;
            }
		}
	}
	else if (is_abbrev(arg, "triggers") || is_abbrev(arg, "scripts"))
	{
		sprintf(buf, "Triggers in zone '%s':\n\n", zone->GetTag());
		FOREACH_BOUNDED(ScriptPrototypeMap, trig_index, zone->GetHash(), i)
		{
			ScriptPrototype *proto = *i;
			
			sprintf_cat(buf, "%10s - %s\n", proto->GetVirtualID().Print().c_str(), proto->m_pProto->GetName());
			found = true;
			
            if (strlen(buf) > MAX_BUFFER_LENGTH - 1024)
            {
                strcat(buf, "*** OVERFLOW ***\n");
                break;
            }
		}
	}
	else
	{
		strcpy(buf, "Invalid argument.\n"
					"Usage: zlist mobs|objects|rooms|triggers|script\n");
		found = true;
	}

	if (!found) strcat(buf, "None!\n");

	page_string(ch->desc, buf);
}


ACMD(do_delete) {
	CharData *victim = NULL;
	ObjData *object = NULL;
	RoomData *room = NULL;
	TrigData *trigger = NULL;
	ZoneData *zone;
	
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	
	if (subcmd != SCMD_DELETE && subcmd != SCMD_UNDELETE) {
		send_to_char("ERROR: Unknown subcmd to do_delete", ch);
		return;
	}
	
	two_arguments(argument, arg1, arg2);
	
	VirtualID vid(arg2);
	
	if (!*arg1 || !*arg2 || !vid.IsValid())
		ch->Send("Usage: %sdelete mob|obj|room|trig vnum\n", subcmd == SCMD_DELETE ? "" : "un");
	else if ((zone = zone_table.Find(vid.zone)) == NULL)
		send_to_char("That zone is not valid.\n", ch);
	else if (ch->GetLevel() < LVL_ADMIN && !get_zone_perms(ch, vid))
		send_to_char("You don't have permission to (un)delete that.\n", ch);
	else if (is_abbrev(arg1, "room"))
	{
		RoomData *room = world.Find(vid);
		if (room == NULL)
			send_to_char("That is not a valid room.\n",ch);
		else
		{
			room->GetFlags().set(ROOM_DELETED, subcmd == SCMD_DELETE);
			
			mudlogf(NRM, LVL_BUILDER, TRUE, "OLC: %s marks room %s as %sdeleted.",
					ch->GetName(), room->GetVirtualID().Print().c_str(), subcmd == SCMD_DELETE ? "" : "un");
			REdit::save_to_disk(zone);
		}
	}
	else if (is_abbrev(arg1, "mob"))
	{
		CharacterPrototype *proto = mob_index.Find(vid);
		
		if (!proto)
			send_to_char("That is not a valid mobile.\n",ch);
		else {
			victim = proto->m_pProto;
			if (subcmd == SCMD_DELETE)	SET_BIT(MOB_FLAGS(victim), MOB_DELETED);
			else						REMOVE_BIT(MOB_FLAGS(victim), MOB_DELETED);
			mudlogf(NRM, LVL_BUILDER, TRUE, "OLC: %s marks mob %s as %sdeleted.",
					ch->GetName(), victim->GetVirtualID().Print().c_str(), subcmd == SCMD_DELETE ? "" : "un");
			medit_save_to_disk(zone);
		}
	} else if (is_abbrev(arg1, "object")) {
		ObjectPrototype *proto = obj_index.Find(vid);
	
		if (!proto)
			send_to_char("That is not a valid object.\n",ch);
		else {
			object = proto->m_pProto;
			if (subcmd == SCMD_DELETE)	SET_BIT(GET_OBJ_EXTRA(object), ITEM_DELETED);
			else						REMOVE_BIT(GET_OBJ_EXTRA(object), ITEM_DELETED);
			mudlogf(NRM, LVL_BUILDER, TRUE, "OLC: %s marks object %s as %sdeleted.",
					ch->GetName(), object->GetVirtualID().Print().c_str(), subcmd == SCMD_DELETE ? "" : "un");
			oedit_save_to_disk(zone);
		}
	}
	else if (is_abbrev(arg1, "trigger") || is_abbrev(arg1, "script"))
	{
		ScriptPrototype *proto = trig_index.Find(vid);
		
		if (!proto)
			send_to_char("That is not a valid script.\n",ch);
		else {
			trigger = proto->m_pProto;
			if (subcmd == SCMD_DELETE)	SET_BIT(GET_TRIG_TYPE(trigger), TRIG_DELETED);
			else						REMOVE_BIT(GET_TRIG_TYPE(trigger), TRIG_DELETED);
			mudlogf(NRM, LVL_BUILDER, TRUE, "OLC: %s marks trigger %s as %sdeleted.",
					ch->GetName(), trigger->GetVirtualID().Print().c_str(), subcmd == SCMD_DELETE ? "" : "un");
			scriptedit_save_to_disk(zone);
		}
	}
	else 
		ch->Send("Usage: %sdelete mob|obj|room|trigger <vnum>\n", (subcmd == SCMD_DELETE) ? "" : "un");
}


ACMD(do_liblist)
{
	ch->Send("Returning soon.\n");
	
#if 0
	BUFFER(buf, LARGE_BUFSIZE * 4);
	BUFFER(buf2, MAX_INPUT_LENGTH);
	VirtualID	vnum;
	int first, last, found = 0;

	two_arguments(argument, buf, buf2);

	if (!*buf || !*buf2) {
		switch (subcmd) {
			case SCMD_RLIST:
				send_to_char("Usage: rlist <begining number> <ending number>\n", ch);
				break;
			case SCMD_OLIST:
				send_to_char("Usage: olist <begining number> <ending number>\n", ch);
				break;
			case SCMD_MLIST:
				send_to_char("Usage: mlist <begining number> <ending number>\n", ch);
				break;
			case SCMD_TLIST:
				send_to_char("Usage: tlist <begining number> <ending number>\n", ch);
				break;
			default:
				mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR:: invalid SCMD passed to do_liblist!  (SCMD = %d)", subcmd);
				break;
		}
		return;
	}

	first = atoi(buf);
	last = atoi(buf2);

	if ((first < 0) || (first > 32700) || (last < 0) || (last > 32700)) {
		send_to_char("Values must be between 0 and 99999.\n", ch);
		return;
	}

	if (first >= last) {
		send_to_char("Second value must be greater than first.\n", ch);
		return;
	}
	
	if((first+300) <= last){
		send_to_char("May not exceed 300 items, try using a smaller end number!\n" ,ch);
		return;
	}
		
	switch (subcmd)
	{
		case SCMD_RLIST:
			sprintf(buf, "Room List From Vnum %d to %d\n", first, last);
			for (vnum = first; vnum <= last; vnum++) {
				RoomData *room = world.Find(vnum);
				if (room) {
					sprintf_cat(buf, "%5d. [%10s] %s\n",
							++found,
							room->GetVirtualID().Print().c_str(),
							room->GetName());
				}
			}
			break;
		case SCMD_OLIST:
			sprintf(buf, "Object List From Vnum %d to %d\n", first, last);
			for (vnum = first; vnum <= last; vnum++)
			{
				ObjectPrototype *proto = obj_index.Find(vnum);
				if (proto)
				{
					sprintf_cat(buf, "%5d. [%10s] %s\n",
							++found,
							proto->GetVirtualID().Print().c_str(),
							proto->m_pProto->shortDesc.c_str());
				}
			}
			break;
		case SCMD_MLIST:
			sprintf(buf, "Mob List From Vnum %d to %d\n", first, last);
			for (vnum = first; vnum <= last; vnum++)
			{
				CharacterPrototype *proto = mob_index.Find(vnum);
				if (proto)
				{
					sprintf_cat(buf, "%5d. [%10s] %s\n",
							++found,
							proto->GetVirtualID().Print().c_str(),
							proto->m_pProto->general.shortDesc.c_str());
				}
			}
			break;
		case SCMD_TLIST:
			sprintf(buf, "Trigger List From Vnum %d to %d\n", first, last);
			for (vnum = first; vnum <= last; vnum++)
			{
				ScriptPrototype *proto = trig_index.Find(vnum);
				if (proto) {
					sprintf_cat(buf, "%5d. [%10s] %s\n",
							++found,
							proto->GetVirtualID().Print().c_str(),
							proto->m_pProto->GetName());
				}
			}
			break;
	}

	if (!found) {
		switch (subcmd) {
			case SCMD_RLIST:
				send_to_char("No rooms found within those parameters.\n", ch);
				break;
			case SCMD_OLIST:
				send_to_char("No objects found within those parameters.\n", ch);
				break;
			case SCMD_MLIST:
				send_to_char("No mobiles found within those parameters.\n", ch);
				break;
			case SCMD_TLIST:
				send_to_char("No triggers found within those parameters.\n", ch);
				break;
			default:
				mudlogf(BRF, LVL_STAFF, TRUE, "SYSERR:: invalid SCMD passed to do_liblist!  (SCMD = %d)", subcmd);
				break;
		}
	} else
		page_string(ch->desc, buf);
#endif
}


ACMD(do_massolcsave)
{
	FOREACH(ZoneMap, zone_table, z)
	{
//		REdit::save_to_disk(*z);
//		medit_save_to_disk(*z);
		oedit_save_to_disk(*z);
//		zedit_save_to_disk(*z);
//		sedit_save_to_disk(*z);
//		scriptedit_save_to_disk(*z);
	}
}


ACMD(do_dig)
{
	BUFFER(arg1, MAX_STRING_LENGTH);
	BUFFER(arg2, MAX_STRING_LENGTH);
	
	int newroom = 0, dir = 0;
	RoomData *	fromRoom = ch->InRoom();
	ZoneData *	zone;

	two_arguments(argument, arg1, arg2);
	
	VirtualID	fromVID = fromRoom->GetVirtualID();
	VirtualID	toVID(arg2, fromVID.zone);

	if(!*arg1 || !*arg2)
		send_to_char("Syntax: dig <direction> [<vnum>]\n", ch);
    else if((dir = search_block(arg1, dirs, FALSE)) < 0) 
        send_to_char("That's not a direction!\n", ch);	
	else if (*arg2 && !toVID.IsValid())
		ch->Send("Enter a valid Virtual ID please.\n");
	else if ((zone = zone_table.Find(toVID.zone)) == NULL)
		ch->Send("There is no such zone!\n");
	else
	{
		FOREACH(DescriptorList, descriptor_list, iter)
		{
			DescriptorData *d = *iter;
            if (typeid(*d->GetState()).name() == olc_scmd_info[SCMD_OLC_REDIT].con_type
            	&& d->olc && ((OLC_VID(d) == fromVID) || (OLC_VID(d) == toVID)))
            {
//				ch->Send("This room is currently being edited by %s.\n", (ch->CanSee(d->m_Character) ? d->m_Character->GetName() : "someone"));
				act("This room is currently being edited by $N.", false, ch, 0, d->m_Character, TO_CHAR);
                return;
            }
        }
		    /* no building permissions */
        if (!get_zone_perms(ch, fromVID) || !get_zone_perms(ch, toVID))
			return;

		if(fromRoom->GetExit(dir) && fromRoom->GetExit(dir)->ToRoom()) {
			send_to_char("There's already an exit in that direction!\n", ch);
			return;
		}
		
		RoomData *toRoom = world.Find(toVID);
		if (toRoom == NULL)	// room doesnt exist
		{
			toRoom = new RoomData(toVID);
			//toRoom->zone = zone;
			toRoom->SetName("A freshly dug room.");
			toRoom->SetDescription("You are in a freshly dug room.\n");
			world.insert(toRoom);
			
			ch->Send("New room created: %s\n", toRoom->GetVirtualID().Print().c_str());
		}
		else if (toRoom->GetExit(rev_dir[dir]) && toRoom->GetExit(rev_dir[dir])->ToRoom())
		{
			send_to_char("The destination room already has an exit going in the opposite direction!\n", ch);
			return;
		}

		ch->Send("New exit created %s to %s - zone '%s'.\n", dirs[dir], toRoom->GetVirtualID().Print().c_str(), zone->GetTag());
		mudlogf(CMP, LVL_BUILDER, TRUE, "OLC: %s creates exit from %s %s to %s.", ch->GetName(), fromRoom->GetVirtualID().Print().c_str(), dirs[dir], toRoom->GetVirtualID().Print().c_str());

		if (!toRoom->GetExit(rev_dir[dir]))		toRoom->SetExit(rev_dir[dir], new RoomExit);
		toRoom->GetExit(rev_dir[dir])->room = fromRoom;

		if (!fromRoom->GetExit(dir))			fromRoom->SetExit(dir, new RoomExit);
		fromRoom->GetExit(dir)->room = toRoom;
        
		REdit::save_to_disk(fromRoom->GetZone());
		if (fromRoom->GetZone() != toRoom->GetZone())
			REdit::save_to_disk(toRoom->GetZone());
	}
}


ACMD(do_vattached)
{
	const int real_max_length = MAX_STRING_LENGTH * 4;
	const int max_length = real_max_length - 1024;
	BUFFER(buf, real_max_length);
	
	bool		overflow = false;
	int			foundObjects = 0;
	int			foundMobs = 0;
	int			foundRooms = 0;
	
	std::list<VirtualID>	foundVNums;

	//	Show all things a script attaches to: search obj protos, mob protos, and zresets

	ScriptPrototype *proto = trig_index.Find(VirtualID(argument, ch->InRoom()->GetVirtualID().zone));
	if (!proto)
	{
		ch->Send("That is not a valid trigger.\n");
		return;
	}

	
	//	Objects
	FOREACH(ObjectPrototypeMap, obj_index, iter)
	{
		ObjectPrototype *obj = *iter;
		FOREACH(ScriptVector, obj->m_Triggers, trn)
		{
			if (*trn == proto->GetVirtualID())
			{
				++foundObjects;
				
				foundVNums.push_back(obj->GetVirtualID());
				
				if (!overflow)
				{
					if (strlen(buf) > max_length)
						overflow = true;
					else
					{
						if (foundObjects == 1)
						{
							sprintf_cat(buf, "The following objects have trigger %s attached:\n\n", proto->GetVirtualID().Print().c_str());
						}
						
						sprintf_cat(buf, "%3d. `y[`c%10s`y]`n %s`n\n", foundObjects, obj->GetVirtualID().Print().c_str(), obj->m_pProto->GetName());
					}
				}
				break;
			}
		}
	}
	
	FOREACH(ObjectList, object_list, iter)
	{
		ObjData *obj = *iter;
		
		if (obj->GetVirtualID().IsValid()
			&& Lexi::IsInContainer(foundVNums, obj->GetVirtualID()))
			continue;	//	We already know this was attached!
		
		FOREACH(TrigData::List, obj->GetScript()->m_Triggers, iter)
		{
			if ((*iter)->GetPrototype() != proto)
				continue;
			
			++foundObjects;
			
			if (!overflow)
			{
				if (strlen(buf) > max_length)
					overflow = true;
				else
				{
					if (foundObjects == 1)
					{
						sprintf_cat(buf, "The following objects have trigger %s attached:\n\n", proto->GetVirtualID().Print().c_str());
					}
					
					sprintf_cat(buf, "%3d. `y[`c%10s`y]`n `r(Temporary)`n %s`n\n", foundObjects, obj->GetVirtualID().Print().c_str(), obj->GetName());
				}
			}
			break;
		}
	}
	
	foundVNums.clear();
	
	//	Mobs
	FOREACH(CharacterPrototypeMap, mob_index, iter)
	{
		CharacterPrototype *mob = *iter;
		FOREACH(ScriptVector, mob->m_Triggers, trn)
		{
			if (*trn != proto->GetVirtualID())
				continue;
			
			++foundMobs;
			
			foundVNums.push_back(mob->GetVirtualID());
			
			if (!overflow)
			{
				if (strlen(buf) > max_length)
					overflow = true;
				else
				{
					if (foundMobs == 1)
					{
						if (foundObjects > 0)
							strcat(buf, "\n");
						
						sprintf_cat(buf, "The following mobs have trigger %s attached:\n\n", proto->GetVirtualID().Print().c_str());
					}
					
					sprintf_cat(buf, "%3d. `y[`c%10s`y]`n %s`n\n", foundMobs, mob->GetVirtualID().Print().c_str(), mob->m_pProto->GetName());
				}
			}
			break;
		}
	}
	
	FOREACH(CharacterList, character_list, iter)
	{
		CharData *mob = *iter;
	
		if (mob->GetVirtualID().IsValid()
			&& Lexi::IsInContainer(foundVNums, mob->GetVirtualID()))
			continue;	//	We already know this was attached!
		
		FOREACH(TrigData::List, mob->GetScript()->m_Triggers, iter)
		{
			if ((*iter)->GetPrototype() != proto)
				continue;
			
			++foundMobs;
			
			if (!overflow)
			{
				if (strlen(buf) > max_length)
					overflow = true;
				else
				{
					if (foundMobs == 1)
					{
						if (foundObjects > 0)
							strcat(buf, "\n");
						
						sprintf_cat(buf, "The following mobs have trigger %s attached:\n\n", proto->GetVirtualID().Print().c_str());
					}
					
					sprintf_cat(buf, "%3d. `y[`c%10s`y]`n `r(Temporary)`n %s`n\n", foundMobs, mob->GetVirtualID().Print().c_str(), mob->GetName());
				}
			}
			break;
		}
	}
	
	//	TODO: Dynamic rooms covered here too
	FOREACH(RoomMap, world, iter)
	{
		RoomData *		room = *iter;
		Scriptable *	script = room->GetScript();
		
		if (script == NULL)
			continue;
		
		FOREACH(TrigData::List, script->m_Triggers, iter)
		{
			if ((*iter)->GetPrototype() == proto)
			{
				++foundRooms;
				
				if (!overflow)
				{
					if (strlen(buf) > max_length)
						overflow = true;
					else
					{
						if (foundRooms == 1)
						{
							if (foundObjects > 0 || foundMobs > 0)
								strcat(buf, "\n");
							
							sprintf_cat(buf, "The following rooms have trigger %s attached:\n\n", proto->GetVirtualID().Print().c_str());
						}
						
						sprintf_cat(buf, "%3d. `y[`c%10s`y]`n %s`n\n", foundRooms, room->GetVirtualID().Print().c_str(), room->GetName());
					}
				}
				break;
			}
		}
	}
	
	if (overflow)
	{
		strcat(buf, "***OVERFLOW***\n");
	}
	
	int foundTotal = foundObjects + foundMobs + foundRooms;
	if (foundTotal == 0)
	{
		sprintf_cat(buf, "Trigger %s is not attached to anything.\n", proto->GetVirtualID().Print().c_str());
	}
	else
	{
		strcat(buf, "\n");
		sprintf_cat(buf, "Trigger %s is attached to ", proto->GetVirtualID().Print().c_str());
		if (foundObjects != 0)
		{
			sprintf_cat(buf, "%d objects%s", foundObjects,
				foundObjects == foundTotal ? "." :
					foundMobs != 0 && foundRooms != 0 ? ", " : " and ");
		}
		if (foundMobs != 0)
		{
			sprintf_cat(buf, "%d mobs%s", foundMobs,
				foundRooms == 0 ? "." : " and ");
		}
		if (foundRooms != 0)
		{
			sprintf_cat(buf, "%d rooms.", foundRooms);
		}
		
		sprintf_cat(buf, "  (%d total)\n", foundTotal);
	}
	
	page_string(ch->desc, buf);
}


ACMD(do_olccopy)
{
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	ZoneData *	zone;
	
	two_arguments(argument, arg1, arg2);
	
	VirtualID	srcVID(arg1);
	VirtualID	destVID(arg2, srcVID.zone);
	
	if (!*arg2)
	{
		ch->Send("Usage: [rcopy|ocopy|mcopy|scriptcopy] <source> <destination>\n");
	}
	else if (!srcVID.IsValid() || !destVID.IsValid())
	{
		ch->Send("Invalid %s VirtualID.\n", srcVID.IsValid() ? "destination" : "source");
	}
	else if ((zone = zone_table.Find(destVID.zone)) == NULL)
	{
		ch->Send("The destination zone does not exist.\n");
	}
	else if (!get_zone_perms(ch, destVID))
	{
	}
	else if (subcmd == SCMD_OLC_REDIT)
	{
		RoomData *room = world.Find(srcVID);
		
		if (!room)
		{
			ch->Send("Source room not found.\n");
		}
		else if (world.Find(destVID))
		{
			ch->Send("You cannot copy over an existing room.\n");
		}
		else
		{
			RoomData *newRoom = new RoomData(destVID);
			newRoom->SetName(room->GetName());
			newRoom->SetDescription(room->GetDescription());
			//newRoom->zone = zone;
			newRoom->SetSector(room->GetSector());
			newRoom->GetFlags() = room->GetFlags();
			newRoom->m_ExtraDescriptions = room->m_ExtraDescriptions;
			
			world.insert(newRoom);
			
			mudlogf(NRM, LVL_BUILDER, TRUE,  "OLC: %s copies room %s to %s", ch->GetName(), srcVID.Print().c_str(), destVID.Print().c_str());
			ch->Send("Copied room %s to %s.\n", srcVID.Print().c_str(), destVID.Print().c_str());
			
			REdit::save_to_disk(zone);
		}
	}
	else if (subcmd == SCMD_OLC_OEDIT)
	{
		ObjectPrototype *	srcProto = obj_index.Find(srcVID);
		
		if (!srcProto)
		{
			ch->Send("Source object not found.\n");
		}
		else if (obj_index.Find(destVID))
		{
			ch->Send("You cannot copy over an existing object.\n");
		}
		else
		{
			ObjectPrototype *destProto = obj_index.insert(destVID, new ObjData(*srcProto->m_pProto));
			destProto->m_pProto->SetPrototype(destProto);
			
			destProto->m_Triggers		= srcProto->m_Triggers;
			destProto->m_BehaviorSets	= srcProto->m_BehaviorSets;
			destProto->m_InitialGlobals	= srcProto->m_InitialGlobals;
			
			mudlogf(NRM, LVL_BUILDER, TRUE,  "OLC: %s copies obj %s to %s", ch->GetName(), srcVID.Print().c_str(), destVID.Print().c_str());
			ch->Send("Copied object %s to %s.\n", srcVID.Print().c_str(), destVID.Print().c_str());
			
			oedit_save_to_disk(zone);
		}
	}
	else if (subcmd == SCMD_OLC_MEDIT)
	{
		CharacterPrototype *	srcProto = mob_index.Find(srcVID);
		
		if (!srcProto)
		{
			ch->Send("Source mobile not found.\n");
		}
		else if (mob_index.Find(destVID))
		{
			ch->Send("You cannot copy over an existing mobile.\n");
		}
		else
		{
			CharacterPrototype *destProto = mob_index.insert(destVID, new CharData(*srcProto->m_pProto));
			destProto->m_pProto->SetPrototype(destProto);
			
			destProto->m_Triggers		= srcProto->m_Triggers;
			destProto->m_BehaviorSets	= srcProto->m_BehaviorSets;
			destProto->m_InitialGlobals	= srcProto->m_InitialGlobals;
			
			mudlogf(NRM, LVL_BUILDER, TRUE,  "OLC: %s copies mobile %s to %s", ch->GetName(), srcVID.Print().c_str(), destVID.Print().c_str());
			ch->Send("Copied mobile %s to %s.\n", srcVID.Print().c_str(), destVID.Print().c_str());
			
			medit_save_to_disk(zone);
		}
	}
	else if (subcmd == SCMD_OLC_SCRIPTEDIT)
	{
		ScriptPrototype *	srcProto = trig_index.Find(srcVID);
		
		if (!srcProto)
		{
			ch->Send("Source script not found.\n");
		}
		else if (trig_index.Find(destVID))
		{
			ch->Send("You cannot copy over an existing script.\n");
		}
		else
		{
			ScriptPrototype *destProto = trig_index.insert(destVID, new TrigData(*srcProto->m_pProto));
			destProto->m_pProto->SetPrototype(destProto);
			
			mudlogf(NRM, LVL_BUILDER, TRUE,  "OLC: %s copies script %s to %s", ch->GetName(), srcVID.Print().c_str(), destVID.Print().c_str());
			ch->Send("Copied script %s to %s.\n", srcVID.Print().c_str(), destVID.Print().c_str());
			
			scriptedit_save_to_disk(zone);
		}
	}
}



struct OLCDeferredSave
{
	enum Type
	{
		Mob,
		Obj,
		Room,
		Script,
		Shop,
		Zone
	};
	
	Type				m_Type;
	ZoneData *			m_Zone;
	
	OLCDeferredSave(Type t, ZoneData *z)
	:	m_Type(t)
	,	m_Zone(z)
	{}
	
	void				Execute()
	{
		const char * save_types[] = 
		{
			"mobiles", "objects", "rooms", "scripts", "shops", "resets"
		};
		
		mudlogf(NRM, 0, true, "Saving %s for zone %s.", save_types[m_Type], m_Zone->GetName());

		switch (m_Type)
		{
			case Mob:	medit_save_to_disk(m_Zone);			break;
			case Obj:	oedit_save_to_disk(m_Zone);			break;
			case Room:	REdit::save_to_disk(m_Zone);		break;
			case Script:scriptedit_save_to_disk(m_Zone);	break;
			case Shop:	sedit_save_to_disk(m_Zone);			break;
			case Zone:	zedit_save_to_disk(m_Zone);			break;
		}
	}
	
	bool operator==(const OLCDeferredSave &sv) { return m_Type == sv.m_Type && m_Zone == sv.m_Zone; }

private:
	OLCDeferredSave();
};


class OLCDeferredSaveList : public std::list<OLCDeferredSave>
{
public:
	~OLCDeferredSaveList()
	{
		Execute();
	}
	
	void				Add(const OLCDeferredSave &sv)
	{
		if (!Lexi::IsInContainer(*this, sv))
			push_back(sv);
	}
		
	void				Add(OLCDeferredSave::Type t, ZoneData *z)
	{
		Add(OLCDeferredSave(t, z));
	}
		
	void				Add(OLCDeferredSave::Type t, Hash h)
	{
		ZoneData *z = zone_table.Find(h);
		if (!z)	return;
		Add(OLCDeferredSave(t, z));
	}

	void				Execute()
	{
		Lexi::ForEach(*this, std::mem_fun_ref(&OLCDeferredSave::Execute));
		clear();
	}
};


ACMD(do_olcmove)
{
	extern void SaveGlobalTriggers();
	
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	ZoneData *	srcZone, *destZone;
	
	argument = two_arguments(argument, arg1, arg2);
	
	VirtualID	srcVID(arg1);
	VirtualID	destVID(arg2, srcVID.zone);
	
	if (!*arg2)
	{
		ch->Send("Usage: %s <source> <destination>\n", command);
	}
	else if (!srcVID.IsValid() || !destVID.IsValid())
	{
		ch->Send("Invalid %s VirtualID.\n", srcVID.IsValid() ? "destination" : "source");
	}
	else if ((srcZone = zone_table.Find(srcVID.zone)) == NULL)
	{
		ch->Send("The source zone does not exist.\n");
	}
	else if ((destZone = zone_table.Find(destVID.zone)) == NULL)
	{
		ch->Send("The destination zone does not exist.\n");
	}
	else if (!get_zone_perms(ch, srcVID) || !get_zone_perms(ch, destVID))
	{
	}
	else if (subcmd == SCMD_OLC_REDIT)
	{
		//
		//	ROOM MOVE
		//
		RoomData *room = world.Find(srcVID);
	
		if (!room)
		{
			ch->Send("Room not found.\n");
		}
//		else if (ROOM_FLAGGED(room, ROOM_HOUSE))
//		{
//			ch->Send("Moving house rooms is not supported at this time.\n");			
//		}
		//	MAKE SURE NOBODY IS EDITING!
		else if (world.Find(destVID))
		{
			ch->Send("The destination room already exists.\n");
		}
		else
		{
			//	Verify neither source nor destination rooms are being edited
			FOREACH(DescriptorList, descriptor_list, iter)
			{
				DescriptorData *d = *iter;
	            if (typeid(*d->GetState()) == typeid(RoomEditOLCConnectionState))
	            {
	            	if (d->olc && OLC_VID(d) == srcVID)
	            	{
						act("Source room is currently being edited by $N.", false, ch, 0, d->m_Character, TO_CHAR);
		                return;
	            	}
	            	if (d->olc && OLC_VID(d) == destVID)
	            	{
						act("Destination room is currently being edited by $N.", false, ch, 0, d->m_Character, TO_CHAR);
		                return;
	            	}
	            }
	        }

			//	Houses
			House *house = House::GetHouseForRoom(room);
			if (house)
			{
				remove(House::GetRoomFilename(room).c_str());
			}
			
			world.renumber(room, destVID);
			
			//	Houses
			if (house)
			{
				house->SaveContents();
				House::Save();
			}
			
			OLCDeferredSaveList	deferredSaves;
			
			deferredSaves.Add(OLCDeferredSave::Room, srcZone);
			deferredSaves.Add(OLCDeferredSave::Room, destZone);
			
			deferredSaves.Add(OLCDeferredSave::Zone, srcZone);
			deferredSaves.Add(OLCDeferredSave::Zone, destZone);
			
			//	Other room links
			FOREACH(RoomMap, world, iter)
			{
				RoomData *otherRoom = *iter;
				
//				if (Lexi::IsInContainer(roomFiles, otherRoom->zone))
//					continue;
				
				if(otherRoom != room)
				{
					for (int i = 0; i < NUM_OF_DIRS; ++i)
					{
						if (otherRoom->GetExit(i) && otherRoom->GetExit(i)->room == room)
						{
							deferredSaves.Add(OLCDeferredSave::Room, otherRoom->GetZone());
						}
					}
				}
			}
			
			//	Zone Resets
			if (/*room->GetZone()*/ srcZone != destZone)
			{
//				room->zone = destZone;
				
				ZoneData::ResetCommandListRange	range = srcZone->GetResetsForRoom(room);
				destZone->cmd.insert(destZone->cmd.end(), range.first, range.second);
				srcZone->cmd.erase(range.first, range.second);
			}
			
			//	Shops
			FOREACH(ShopMap, shop_index, iter)
			{
				if (Lexi::IsInContainer((*iter)->in_room, destVID))
				{
					Lexi::Replace((*iter)->in_room, srcVID, destVID);
					deferredSaves.Add(OLCDeferredSave::Shop, (*iter)->GetVirtualID().zone);
				}
			}
			
			ch->Send("Moved room %s to %s.\n", srcVID.Print().c_str(), destVID.Print().c_str());
			mudlogf(NRM, LVL_BUILDER, TRUE,  "OLC: %s moved room %s to %s", ch->GetName(), srcVID.Print().c_str(), destVID.Print().c_str());
			
			deferredSaves.Execute();
		}
	}
	else if (subcmd == SCMD_OLC_OEDIT)
	{
		//
		//	OBJECT MOVE
		//
		ObjectPrototype *proto = obj_index.Find(srcVID);
	
		one_argument(argument, arg1);
		bool bSafeMove = !str_cmp(arg1, "safe");

		if (!proto)
		{
			ch->Send("Object not found.\n");
		}
		else if (obj_index.Find(destVID))
		{
			ch->Send("The destination object already exists.\n");
		}
		else
		{
			//	Verify neither source nor destination rooms are being edited
			FOREACH(DescriptorList, descriptor_list, iter)
			{
				DescriptorData *d = *iter;
	            if (typeid(*d->GetState()) == typeid(ObjectEditOLCConnectionState))
	            {
	            	if (d->olc && OLC_VID(d) == srcVID)
	            	{
						act("Source object is currently being edited by $N.", false, ch, 0, d->m_Character, TO_CHAR);
		                return;
	            	}
	            	if (d->olc && OLC_VID(d) == destVID)
	            	{
						act("Destination object is currently being edited by $N.", false, ch, 0, d->m_Character, TO_CHAR);
		                return;
	            	}
	            }
	        }


			obj_index.renumber(proto, destVID);
			
			//	Was not specified as safe, so leave a safety backup copy, for saved player files...
			if (!bSafeMove)
			{
				ObjectPrototype *backupProto = obj_index.insert(srcVID, new ObjData(*proto->m_pProto));
				backupProto->m_pProto->SetPrototype(backupProto);
				backupProto->m_Triggers = proto->m_Triggers;
				backupProto->m_Function = proto->m_Function;
				backupProto->m_SharedVariables = proto->m_SharedVariables;
				backupProto->m_InitialGlobals = proto->m_InitialGlobals;
				backupProto->m_BehaviorSets = proto->m_BehaviorSets;

				backupProto->m_InitialGlobals.Add("newVirtualID", destVID.Print());
			}
			
			OLCDeferredSaveList	deferredSaves;
			
			deferredSaves.Add(OLCDeferredSave::Obj, srcZone);
			deferredSaves.Add(OLCDeferredSave::Obj, destZone);
			
			//	Mob Equipment
			FOREACH(CharacterPrototypeMap, mob_index, iter)
			{
				CharData *mob = (*iter)->m_pProto;
				
				FOREACH(MobEquipmentList, mob->mob->m_StartingEquipment, eq)
				{
					if (eq->m_Virtual == srcVID)
					{
						eq->m_Virtual = destVID;
						deferredSaves.Add(OLCDeferredSave::Mob, mob->GetVirtualID().zone);
						break;
					}
				}
			}
			
			//	Room Keys
			FOREACH(RoomMap, world, iter)
			{
				for (int i = 0; i < NUM_OF_DIRS; ++i)
				{
					if ((*iter)->GetExit(i) && (*iter)->GetExit(i)->key == srcVID)
					{
						(*iter)->GetExit(i)->key == destVID;
						deferredSaves.Add(OLCDeferredSave::Room, (*iter)->GetVirtualID().zone);
					}
				}
				
			}
			
			//	Zone Resets
			FOREACH(ZoneMap, zone_table, iter)
			{
				FOREACH(ResetCommandList, (*iter)->cmd, cmd)
				{
					if ((strchr("POGER", cmd->command) && cmd->obj == proto)
						|| (cmd->command == 'P' && cmd->container == proto))
					{
						deferredSaves.Add(OLCDeferredSave::Zone, *iter);
						break;	//	Next zone
					}
				}
			}
			
			//	Shops
			FOREACH(ShopMap, shop_index, iter)
			{
				if (Lexi::IsInContainer((*iter)->producing, proto))
				{
					deferredSaves.Add(OLCDeferredSave::Shop, (*iter)->GetVirtualID().zone);
				}
			}
			
			ch->Send("Moved object %s to %s%s.\n", srcVID.Print().c_str(), destVID.Print().c_str(),
				!bSafeMove ? " (backup copy left behind)" : "");
			mudlogf(NRM, LVL_BUILDER, TRUE,  "OLC: %s moved object %s to %s%s", ch->GetName(), srcVID.Print().c_str(), destVID.Print().c_str(),
				!bSafeMove ? " (backup copy left behind)" : "");

			deferredSaves.Execute();
			
			//	Move Boards
			if (proto->m_pProto->GetType() == ITEM_BOARD)
			{
				Board *board = Board::GetBoard(srcVID);
				board->Renumber(destVID);
			}
			
			//	Flag houses to save
			FOREACH(ObjectList, object_list, iter)
			{
				if ((*iter)->GetPrototype() == proto)
				{
					RoomData *r = (*iter)->Room();
					if (r && r->GetFlags().test(ROOM_HOUSE))
						r->GetFlags().set(ROOM_HOUSE_CRASH);
				}
			}
		}
	}
	else if (subcmd == SCMD_OLC_MEDIT)
	{
		//
		//	MOBILE MOVE
		//
		CharacterPrototype *proto = mob_index.Find(srcVID);
	
		if (!proto)
		{
			ch->Send("Mobile not found.\n");
		}
		else if (mob_index.Find(destVID))
		{
			ch->Send("The destination mobile already exists.\n");
		}
		else
		{
			//	Verify neither source nor destination rooms are being edited
			FOREACH(DescriptorList, descriptor_list, iter)
			{
				DescriptorData *d = *iter;
	            if (typeid(*d->GetState()) == typeid(MobEditOLCConnectionState))
	            {
	            	if (d->olc && OLC_VID(d) == srcVID)
	            	{
						act("Source mobile is currently being edited by $N.", false, ch, 0, d->m_Character, TO_CHAR);
		                return;
	            	}
	            	if (d->olc && OLC_VID(d) == destVID)
	            	{
						act("Destination mobile is currently being edited by $N.", false, ch, 0, d->m_Character, TO_CHAR);
		                return;
	            	}
	            }
	        }

			mob_index.renumber(proto, destVID);
			
			
			OLCDeferredSaveList	deferredSaves;
			
			deferredSaves.Add(OLCDeferredSave::Mob, srcZone);
			deferredSaves.Add(OLCDeferredSave::Mob, destZone);
			
			//	Mob Opinion
			FOREACH(CharacterPrototypeMap, mob_index, iter)
			{
				CharData *mob = (*iter)->m_pProto;
				
				if (mob->mob->m_Hates.m_VNum == srcVID)
				{
					mob->mob->m_Hates.m_VNum = destVID;
					deferredSaves.Add(OLCDeferredSave::Mob, mob->GetVirtualID().zone);
				}
				if (mob->mob->m_Fears.m_VNum == srcVID)
				{
					mob->mob->m_Fears.m_VNum = destVID;
					deferredSaves.Add(OLCDeferredSave::Mob, mob->GetVirtualID().zone);
				}
			}
			
			//	Zone Resets
			FOREACH(ZoneMap, zone_table, iter)
			{
				FOREACH(ResetCommandList, (*iter)->cmd, cmd)
				{
					if (cmd->command == 'M' && cmd->mob == proto)
					{
						deferredSaves.Add(OLCDeferredSave::Zone, *iter);
						break;	//	Next zone
					}
				}
			}
			
			//	Shops
			FOREACH(ShopMap, shop_index, iter)
			{
				if ((*iter)->keeper == proto)
					deferredSaves.Add(OLCDeferredSave::Shop, (*iter)->GetVirtualID().zone);				
			}
			
			ch->Send("Moved mob %s to %s.\n", srcVID.Print().c_str(), destVID.Print().c_str());
			mudlogf(NRM, LVL_BUILDER, TRUE,  "OLC: %s moved mob %s to %s", ch->GetName(), srcVID.Print().c_str(), destVID.Print().c_str());

			deferredSaves.Execute();
		}
	}
	else if (subcmd == SCMD_OLC_SCRIPTEDIT)
	{
		//
		//	SCRIPT MOVE
		//

		ScriptPrototype *proto = trig_index.Find(srcVID);
	
		if (!proto)
		{
			ch->Send("Script not found.\n");
		}
		else if (trig_index.Find(destVID))
		{
			ch->Send("The destination script already exists.\n");
		}
		else
		{
			//	Verify neither source nor destination rooms are being edited
			FOREACH(DescriptorList, descriptor_list, iter)
			{
				DescriptorData *d = *iter;
	            if (typeid(*d->GetState()) == typeid(ScriptEditOLCConnectionState))
	            {
	            	if (d->olc && OLC_VID(d) == srcVID)
	            	{
						act("Source script is currently being edited by $N.", false, ch, 0, d->m_Character, TO_CHAR);
		                return;
	            	}
	            	if (d->olc && OLC_VID(d) == destVID)
	            	{
						act("Destination script is currently being edited by $N.", false, ch, 0, d->m_Character, TO_CHAR);
		                return;
	            	}
	            }
	        }


			trig_index.renumber(proto, destVID);
			
			
			OLCDeferredSaveList	deferredSaves;
			
			deferredSaves.Add(OLCDeferredSave::Script, srcZone);
			deferredSaves.Add(OLCDeferredSave::Script, destZone);
			
			//	Mob Attachs
			FOREACH(CharacterPrototypeMap, mob_index, iter)
			{
				CharacterPrototype *mob = *iter;
				if (Lexi::IsInContainer(mob->m_Triggers, srcVID))
				{
					Lexi::Replace(mob->m_Triggers, srcVID, destVID);
					deferredSaves.Add(OLCDeferredSave::Mob, mob->GetVirtualID().zone);
				}
			}
			
			//	Obj Attachs
			FOREACH(ObjectPrototypeMap, obj_index, iter)
			{
				ObjectPrototype *obj = *iter;
				if (Lexi::IsInContainer(obj->m_Triggers, srcVID))
				{
					Lexi::Replace(obj->m_Triggers, srcVID, destVID);
					deferredSaves.Add(OLCDeferredSave::Obj, obj->GetVirtualID().zone);
				}
			}

			//	Zone Resets
			FOREACH(ZoneMap, zone_table, iter)
			{
				FOREACH(ResetCommandList, (*iter)->cmd, cmd)
				{
					if (cmd->command == 'T' && cmd->script == proto)
					{
						deferredSaves.Add(OLCDeferredSave::Zone, *iter);
						break;	//	Next zone
					}
				}
			}
			
			ch->Send("Moved script %s to %s.\n", srcVID.Print().c_str(), destVID.Print().c_str());
			mudlogf(NRM, LVL_BUILDER, TRUE,  "OLC: %s moved script %s to %s", ch->GetName(), srcVID.Print().c_str(), destVID.Print().c_str());
			
			deferredSaves.Execute();

			//	Save Behavior Sets
			BehaviorSet::Save();
			
			//	Save Global Scripts
			SaveGlobalTriggers();
		}
	}
}




class TextEditOLCConnectionState : public OLCConnectionState
{
public:
	TextEditOLCConnectionState(const char *filename)
	:	m_Filename(filename)
	{}
	
	virtual void		Parse(const char *arg) { Pop(); }
	
	virtual Lexi::String GetName() const { return "Text edit"; }

	virtual void		OnEditorSave()
	{
		FILE *fl = fopen(m_Filename.c_str(), "w");
		if (!fl)
		{
			mudlogf(BRF, LVL_CODER, TRUE, "SYSERR: Can't write file '%s'.", m_Filename.c_str());
		}
		else
		{
			if (!GetDesc()->m_StringEditor->empty())
			{
				BUFFER(buf1, MAX_BUFFER_LENGTH);
				Lexi::BufferedFile::PrepareString(buf1, GetDesc()->m_StringEditor->c_str(), false);
				fputs(buf1, fl);
			}
			fclose(fl);
			mudlogf(NRM, LVL_BUILDER, TRUE, "OLC: %s saves '%s'.", GetDesc()->m_Character->GetName(), m_Filename.c_str());
			GetDesc()->Write("Saved.\n");
		}
	}
	virtual void		OnEditorAbort() { GetDesc()->Write("Edit aborted.\n"); }
	virtual void		OnEditorFinished() { Pop(); }

private:
	Lexi::String		m_Filename;
};


ACMD(do_tedit)
{
	int l, i;
			
	struct editor_struct
	{
		char *cmd;
		char level;
		Lexi::String *buffer;
		int  size;
		char *filename;
	} fields[] = {
		/* edit the lvls to your own needs */
		{ "credits",	LVL_ADMIN,		&credits,	2400,	CREDITS_FILE},
		{ "news",		LVL_SRSTAFF,	&news,		8192,	NEWS_FILE},
		{ "motd",		LVL_SRSTAFF,	&motd,		2400,	MOTD_FILE},
		{ "imotd",		LVL_SRSTAFF,	&imotd,		2400,	IMOTD_FILE},
		{ "help",       LVL_ADMIN,		&help,		2400,	HELP_PAGE_FILE},
		{ "info",		LVL_ADMIN,		&info,		8192,	INFO_FILE},
		{ "background",	LVL_ADMIN,		&background,8192,	BACKGROUND_FILE},
		{ "handbook",   LVL_ADMIN,		&handbook,	8192,	HANDBOOK_FILE},
		{ "policies",	LVL_ADMIN,		&policies,	8192,	POLICIES_FILE},
		{ "wizlist",	LVL_SRSTAFF,	&wizlist,	8192,	WIZLIST_FILE},
		{ "login",		LVL_ADMIN,		&login,		8192,	LOGIN_FILE},
		{ "welcome",	LVL_ADMIN,		&welcomeMsg,8192,	WELCOME_FILE },
		{ "start",		LVL_ADMIN,		&startMsg,	8192,	START_FILE },
		{ "file",		LVL_STAFF,		NULL,		8192,	NULL},
		{ "\n",			0,				NULL,		0,		NULL }
	};

	if (ch->desc == NULL) {
		send_to_char("Get outta here you linkdead head!\n", ch);
		return;
	}

//	if (ch->GetLevel() < LVL_STAFF) {
//		send_to_char("You do not have text editor permissions.\n", ch);
//		return;
//	}
	
	BUFFER(field, MAX_INPUT_LENGTH);
	BUFFER(buf, MAX_INPUT_LENGTH);

	half_chop(argument, field, buf);

	if (!*field) {
		strcpy(buf, "Files available to be edited:\n");
		i = 1;
		for (l = 0; *fields[l].cmd != '\n'; l++) {
			if (ch->GetLevel() >= fields[l].level) {
				sprintf_cat(buf, "%-11.11s", fields[l].cmd);
				if (!(i % 7)) strcat(buf, "\n");
				i++;
			}
		}
		if (--i % 7) strcat(buf, "\n");
		if (i == 0) strcat(buf, "None.\n");
		send_to_char(buf, ch);
		return;
	}
	for (l = 0; *(fields[l].cmd) != '\n'; l++)
		if (!strncmp(field, fields[l].cmd, strlen(field)))
			break;

	if (*fields[l].cmd == '\n') {
		send_to_char("Invalid text editor option.\n", ch);
		return;
	}

	if (ch->GetLevel() < fields[l].level) {
		send_to_char("You are not godly enough for that!\n", ch);
		return;
	}

/*	switch (l) {
		case 0: ch->desc->str = &credits; break;
		case 1: ch->desc->str = &news; break;
		case 2: ch->desc->str = &motd; break;
		case 3: ch->desc->str = &imotd; break;
		case 4: ch->desc->str = &help; break;
		case 5: ch->desc->str = &info; break;
		case 6: ch->desc->str = &background; break;
		case 7: ch->desc->str = &handbook; break;
		case 8: ch->desc->str = &policies; break;
		case 9: ch->desc->str = &wizlist; break;
		case 10:	//	break;
		default:
			send_to_char("Invalid text editor option.\n", ch);
			return;
	}
*/   

//	ch->desc->str = fields[l].buffer;
	if (!fields[l].buffer)
	{
		send_to_char("Invalid text editor option.\n", ch);
		return;
	}

	/* set up editor stats */
	send_to_char("\x1B[H\x1B[J", ch);
	send_to_char("Edit file below: (/s saves /h for help)\n", ch);
	
	if (!fields[l].buffer->empty())
	{
		send_to_char(fields[l].buffer->c_str(), ch);
	}
	
	act("$n begins using a PDA.", TRUE, ch, 0, 0, TO_ROOM);

	ch->desc->StartStringEditor(*fields[l].buffer, fields[l].size);
	ch->desc->GetState()->Push(new TextEditOLCConnectionState(fields[l].filename));
}

/*
void test(Hash ZONEHASH, Hash NS)
{
	VirtualID lastVID;
	
	lastVID.zone = ZONEHASH;
	lastVID.ns = NS;
	
	
	FOREACH_BOUNDED(RoomMap, world, ZONEHASH, i) {
		VirtualID vid = (*i)->GetVirtualID();
		if (vid.ns > NS) break;
		if (vid.ns == NS) lastVID.number = vid.number + 1;
	}
}
*/
