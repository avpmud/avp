/************************************************************************
 *  OasisOLC - zedit.c                                            v1.5  *
 *                                                                      *
 *  Copyright 1996 Harvey Gilpin.                                       *
 ************************************************************************/

#include "characters.h"
#include "zones.h"
#include "descriptors.h"
#include "objects.h"
#include "rooms.h"
#include "comm.h"
#include "utils.h"
#include "scripts.h"
#include "db.h"
#include "olc.h"
#include "buffer.h"
#include "constants.h"
#include "weather.h"
#include "interpreter.h"
#include "staffcmds.h"

#include "lexifile.h"

/*-------------------------------------------------------------------*/
/* function protos */
void zedit_disp_menu(DescriptorData * d);
void zedit_setup(DescriptorData *d, RoomData *room_num);
void delete_command(DescriptorData *d, int pos);
bool new_command(DescriptorData *d, int pos);
bool start_change_command(DescriptorData *d, int pos);
void zedit_disp_comtype(DescriptorData *d);
void zedit_disp_arg1(DescriptorData *d);
void zedit_disp_arg2(DescriptorData *d);
void zedit_disp_arg3(DescriptorData *d);
void zedit_disp_arg4(DescriptorData *d);
void zedit_disp_arg5(DescriptorData *d);
void zedit_save_internally(DescriptorData *d);
void zedit_save_to_disk(ZoneData *zone);
void zedit_save_zonelist();
void zedit_new_zone(CharData *ch, char *name);
void zedit_rename_zone(CharData *ch, char *src, char *dest);
void zedit_command_menu(DescriptorData * d);
void zedit_disp_repeat(DescriptorData *d);
void zedit_disp_flags(DescriptorData *d);
void zedit_parse(DescriptorData * d, const char *arg);

void zedit_disp_weather_flags(DescriptorData *d);
void zedit_disp_weather_season_pattern_menu(DescriptorData *d);
void zedit_disp_weather_season_menu(DescriptorData *d);
void zedit_disp_weather_menu(DescriptorData *d);
void zedit_disp_weather_wind_menu(DescriptorData *d);
void zedit_disp_weather_wind_direction_menu(DescriptorData *d);
void zedit_disp_weather_precipitation_menu(DescriptorData *d);
void zedit_disp_weather_temperature_menu(DescriptorData *d);

/*-------------------------------------------------------------------*/
/*. Nasty internal macros to clean up the code .*/

//#define OLC_CMD(d)	(OLC_RESETCOMMAND(d))
#define OLC_CMD(d)	(&OLC_ZONE(d)->cmd[OLC_RESETCOMMAND(d)])

enum
{
	HEADER_CHANGED = 1,
	COMMANDS_CHANGED = 2
};

/*-------------------------------------------------------------------*\
  utility functions 
\*-------------------------------------------------------------------*/

void zedit_setup(DescriptorData *d, RoomData *room)
{
	ZoneData *zone = new ZoneData(*OLC_SOURCE_ZONE(d));

	/*. The remaining fields are used as a 'has been modified' flag .*/
	OLC_VAL(d) = 0;   	/*. Header info (1) or Commands (2) have changed .*/

	//	The iterators become invalidated after the erase, so we cannot reuse the pair
	//	Delete everthing outside the given range
	zone->cmd.erase(zone->cmd.begin(), zone->GetResetsForRoom(room).first);
	zone->cmd.erase(zone->GetResetsForRoom(room).second, zone->cmd.end());
	
	OLC_ZONE(d) = zone;
	
	if (zone->GetWeatherParent())
	{
		OLC_WEATHERPARENT(d) = zone->GetWeatherParent();
	}
	else if (zone->GetWeather())
	{
		OLC_CLIMATE(d) = new Weather::Climate(zone->GetWeather()->m_Climate);
	}
	
	/*. Display main menu .*/
	zedit_disp_menu(d);
}


/*-------------------------------------------------------------------*/
/*. Create a new zone .*/

void zedit_save_zonelist()
{
	Lexi::String			tempFilename = ZONE_LIST ".tmp";
	Lexi::OutputParserFile	file(tempFilename.c_str());
	
	if (file.fail())
	{
		mudlogf(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: Cannot open new zone file \"%s\"!", file.GetFilename());
		return;
	}
	
	FOREACH(ZoneMap, zone_table, zone)
	{
		file.WriteLine(Lexi::Format("%s\t\t%s", (*zone)->GetTag(), strip_color((*zone)->GetName()).c_str()).c_str());
	}
	
	file.close();
	
	remove(ZONE_LIST);
	rename(tempFilename.c_str(), ZONE_LIST);
}

void zedit_new_zone(CharData *ch, char *name)
{
	skip_spaces(name);
	Lexi::tolower(name);
	
	if (!*name)
	{
		ch->Send("Specify a new zone name.\n");
		return;
	}
	
	if (isdigit(*name))
	{
		send_to_char("You cannot create numerical zones anymore.\n", ch);
		return;
	}
	
	for (const char *p = name; *p; ++p)
	{
		if (!isalnum(*p))
		{
			send_to_char("Zone names must consist of alphanumeric characters only, and may not begin with a number.\n", ch);
			return;
		}
	}

	/*. Check zone does not exist .*/
	if (zone_table.Find(name))
	{
		send_to_char("A zone already exists with that name (or namehash).\n", ch);
		return;
	}
	else if (VIDSystem::GetZoneFromAlias(GetStringFastHash(name)))
	{
		send_to_char("A zone has an alias for that name (or namehash).\n", ch);
		return;
	}
	
	Lexi::OutputParserFile	file(Lexi::Format(ZON_PREFIX "%s" ZON_SUFFIX, name).c_str());
	
	if (file.fail())
	{
		mudlogf(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: Cannot open new zone file \"%s\"!", file.GetFilename());
		send_to_char("Could not create zone file.\n", ch);
		return;
	}
	
	file.close();

	//	Ok, insert the new zone here.
	ZoneData *zone = new ZoneData(name);
	zone->m_Name = "New Zone";
	zone->lifespan = 30;
	zone_table.insert(zone);

	zedit_save_to_disk(zone);
	zedit_save_zonelist();
	
	mudlogf(BRF, ch->GetPlayer()->m_StaffInvisLevel, TRUE,  "OLC: %s creates new zone '%s'", ch->GetName(), zone->GetTag());
	send_to_char("Zone created successfully.\n", ch);
}



void zedit_rename_zone(CharData *ch, char *src, char *dest)
{
	extern void medit_save_to_disk(ZoneData *zone);
	extern void oedit_save_to_disk(ZoneData *zone);
	extern void sedit_save_to_disk(ZoneData *zone);
	extern void scriptedit_save_to_disk(ZoneData *zone);
	
	Lexi::tolower(src);
	Lexi::tolower(dest);
	
	if (!*src || !*dest)
	{
		ch->Send("Usage: zedit rename <oldname> <newname>\n");
		return;
	}
	
	ZoneData *zone = zone_table.Find(src);
	
	if (!zone)
	{
		ch->Send("Unable to find zone '%s'\n", src);
		return;
	}
	
	if (isdigit(*dest))
	{
		send_to_char("You cannot create numerical zones anymore.\n", ch);
		return;
	}
	
	for (const char *p = dest; *p; ++p)
	{
		if (!isalnum(*p))
		{
			send_to_char("Zone names must consist of alphanumeric characters only, and may not begin with a number.\n", ch);
			return;
		}
	}

	/*. Check zone does not exist .*/
	if (zone_table.Find(dest))
	{
		send_to_char("A zone already exists with that name (or namehash).\n", ch);
		return;
	}
	
	Lexi::OutputParserFile	file(Lexi::Format(ZON_PREFIX "%s" ZON_SUFFIX, dest).c_str());
	
	if (file.fail())
	{
		mudlogf(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: Cannot open new zone file \"%s\"!", file.GetFilename());
		send_to_char("Could not create zone file.\n", ch);
		return;
	}
	
	file.close();
	
	Lexi::String	oldTag = zone->GetTag();
	Hash			oldHash = zone->GetHash();
	
	zone->AddAlias(zone->GetTag());	//	Also adds it to the table
	zone->SetTag(dest);
	zone_table.Sort();
	
	world.renumber(oldHash, zone->GetHash());
	mob_index.renumber(oldHash, zone->GetHash());
	obj_index.renumber(oldHash, zone->GetHash());
	shop_index.renumber(oldHash, zone->GetHash());
	trig_index.renumber(oldHash, zone->GetHash());
	
	REdit::save_to_disk(zone);
	medit_save_to_disk(zone);
	oedit_save_to_disk(zone);
	sedit_save_to_disk(zone);
	scriptedit_save_to_disk(zone);
	zedit_save_to_disk(zone);
	
	zedit_save_zonelist();

	mudlogf(BRF, ch->GetPlayer()->m_StaffInvisLevel, TRUE,  "OLC: %s renames zone '%s' to '%s'", ch->GetName(), oldTag.c_str(), zone->GetTag());
	send_to_char("Zone renamed successfully.\n", ch);

	BUFFER(fname, MAX_INPUT_LENGTH);
	sprintf(fname, ZON_PREFIX "%s" ZON_SUFFIX, oldTag.c_str());
	remove(fname);
	sprintf(fname, WLD_PREFIX "%s" WLD_SUFFIX, oldTag.c_str());
	remove(fname);
	sprintf(fname, MOB_PREFIX "%s" MOB_SUFFIX, oldTag.c_str());
	remove(fname);
	sprintf(fname, OBJ_PREFIX "%s" OBJ_SUFFIX, oldTag.c_str());
	remove(fname);
	sprintf(fname, SHP_PREFIX "%s" SHP_SUFFIX, oldTag.c_str());
	remove(fname);
	sprintf(fname, TRG_PREFIX "%s" TRG_SUFFIX, oldTag.c_str());
	remove(fname);
}


/*-------------------------------------------------------------------*/

/*
 * Save all the information in the player's temporary buffer back into
 * the current zone table.
 */
void zedit_save_internally(DescriptorData *d)
{
	ZoneData *zone = OLC_SOURCE_ZONE(d);

	/*
	 * Delete all entries in zone_table that relate to this room so we
	 * can add all the ones we have in their place.
	 */
	if (IS_SET(OLC_VAL(d), COMMANDS_CHANGED))
	{
		RoomData *room = world.Find(OLC_VID(d));

#if defined( NEW_ZRESET_TYPES )
		FOREACH(ResetCommandList, OLC_ZONE(d)->cmd, iter)
		{
			(*iter)->SetDestination(room);
		}
#endif

		ZoneData::ResetCommandListRange range = zone->GetResetsForRoom(room);
		ResetCommandList::iterator i = zone->cmd.erase(range.first, range.second);

		/*. Now add all the entries in the players descriptor list .*/
		//zone->cmd.splice(zone->cmd.end(), OLC_ZONE(d)->cmd);
		zone->cmd.insert(i, OLC_ZONE(d)->cmd.begin(), OLC_ZONE(d)->cmd.end());
	}

	/*. Finally, if zone headers have been changed, copy over .*/
	if (IS_SET(OLC_VAL(d), HEADER_CHANGED))
	{
		zone->SetOwner(OLC_ZONE(d)->GetOwner());
		
		zone->GetFlags()	= OLC_ZONE(d)->GetFlags();
		
		zone->m_Name		= OLC_ZONE(d)->m_Name;
		zone->m_Comment		= OLC_ZONE(d)->m_Comment;
		zone->lifespan		= OLC_ZONE(d)->lifespan;
		zone->top			= OLC_ZONE(d)->top;
		zone->m_ResetMode	= OLC_ZONE(d)->m_ResetMode;
		
		if (OLC_CLIMATE(d))				zone->SetWeather(OLC_CLIMATE(d));
		else if (OLC_WEATHERPARENT(d))	zone->SetWeather(OLC_WEATHERPARENT(d));
		else							zone->SetWeather((ZoneData *)NULL);
		
#if 0
		bool	oldZoneOwnedWeather = zone->GetWeather() && zone->IsOwnWeatherSystem();
		bool	newZoneOwnsWeather = (bool)OLC_CLIMATE(d);

		if (oldZoneOwnedWeather && newZoneOwnsWeather)
		{
			//	Copy the climate
			zone->GetWeather()->m_Climate = *OLC_CLIMATE(d);
		}
		else if (oldZoneOwnedWeather)
		{
			//	The existing zone owns it's weather system... but the new one doesnt.
			//	Current weather system disappears, linking zones must be removed

			//	Delete now, because the loop below will likely NULL it out
			delete zone->m_WeatherSystem;
			zone->m_WeatherSystem = NULL;	//	Safety catch... one line, why not!
			
			//	If it now links to a new parent, that will be handled below
		}
		else if (newZoneOwnsWeather)
		{
			//	The old zone does not own it's own weather, but the new zone does not
			
			//	If it was linked to a parent, handle that here
			if (zone->m_WeatherSystem)	zone->m_WeatherSystem->m_Zones.remove(zone);
			
			zone->m_WeatherSystem = new WeatherSystem();
			zone->m_WeatherSystem->m_Zones.push_back(zone);
			
			zone->m_WeatherSystem->m_Climate = *OLC_CLIMATE(d);
			
			/* Link zones to it! */
			FOREACH(ZoneMap, zone_table, iter)
			{
				if ((*iter)->m_ParentZoneForWeather == zone)
				{
					(*iter)->m_WeatherSystem = zone->m_WeatherSystem;
					zone->m_WeatherSystem->m_Zones.push_back(*iter);
				}
			}
			
			zone->m_WeatherSystem->Start();
		}
		else if (zone->m_ParentZoneForWeather != OLC_ZONE(d)->m_ParentZoneForWeather)
		{
			//	The zones link to different parents now
			
			//	Unlink from the old parent, if any
			if (zone->m_ParentZoneForWeather != -1 && zone->m_WeatherSystem != NULL)
			{
				zone->m_WeatherSystem->m_Zones.remove(zone);
				zone->m_WeatherSystem = NULL;
			}
			
			//	This will be force-linked down below, if the new parent zone is not -1
		}
		
		zone->m_ParentZoneForWeather = OLC_ZONE(d)->m_ParentZoneForWeather;
		
		if (zone->m_ParentZoneForWeather != -1 && zone->m_WeatherSystem == NULL)
		{
			//	The zone specifies a parent, but has no weather system... it must be linked now.  3 cases rely on it (2 above)
			
			ZoneData *parentZone = zone_table.Find(OLC_ZONE(d)->m_ParentZoneForWeather);
			if (parentZone)
			{
				zone->m_WeatherSystem = parentZone->m_WeatherSystem;	
				if (zone->m_WeatherSystem)
					zone->m_WeatherSystem->m_Zones.push_back(zone);
			}
		}
#endif
	}
	
	zedit_save_to_disk(OLC_SOURCE_ZONE(d));
}


/*-------------------------------------------------------------------*/
/*. Save all the zone_table for this zone to disk.  Yes, this could
    automatically comment what it saves out, but why bother when you
    have an OLC as cool as this ? :> .*/

void zedit_save_to_disk(ZoneData *zone)
{
	BUFFER(fname, MAX_STRING_LENGTH);

	sprintf(fname, ZON_PREFIX "%s.new", zone->GetTag());
	Lexi::OutputParserFile	file(fname);
	
	if (file.fail())
	{
		mudlogf(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: Cannot open zone file \"%s\"!", fname);
		return;
	}
	

	//	Print zone header to file
	file.BeginParser();
	
	zone->Write(file);
		
	file.EndParser();
	file.close();

	BUFFER(buf, MAX_STRING_LENGTH);
	sprintf(buf, ZON_PREFIX "%s" ZON_SUFFIX, zone->GetTag());
	remove(buf);
	rename(fname, buf);
}

/*-------------------------------------------------------------------*/
//	Error check user input and then add new (blank) command

bool new_command(DescriptorData *d, int pos)
{
	//	Error check to ensure users hasn't given too large an index
	if ((unsigned)pos > OLC_ZONE(d)->cmd.size())
		return false;

	//	Ok, let's add a new (blank) command.
	ResetCommandList::iterator iter = OLC_ZONE(d)->cmd.begin();
	std::advance(iter, pos);
	
	iter = OLC_ZONE(d)->cmd.insert(iter, ResetCommand());
	iter->command = 'N';
		
	return true;
}


/*-------------------------------------------------------------------*/
//	Error check user input and then remove command

void delete_command(DescriptorData *d, int pos)
{
	//	Error check to ensure users hasn't given too large an index
	if ((unsigned)pos >= OLC_ZONE(d)->cmd.size())
		return;

	//	Ok, let's zap it
	ResetCommandList::iterator iter = OLC_ZONE(d)->cmd.begin();
	std::advance(iter, pos);
	
	OLC_ZONE(d)->cmd.erase(iter);
}

/*-------------------------------------------------------------------*/
//	Error check user input and then setup change

bool start_change_command(DescriptorData *d, int pos)
{
	//	Error check to ensure users hasn't given too large an index
	if ((unsigned)pos >= OLC_ZONE(d)->cmd.size())
		return false;
	
	//	Ok, let's get editing
	OLC_RESETCOMMAND(d) = pos;
//	OLC_RESETCOMMAND(d) = OLC_ZONE(d)->cmd.begin();
//	std::advance(OLC_RESETCOMMAND(d), pos);
	return true;
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

void zedit_command_menu(DescriptorData * d) {
	char *cmdtype;
	
	switch(OLC_CMD(d)->command) {
		case 'M':
			cmdtype = "Load Mobile";
			break;
		case 'G':
			cmdtype = "Give Object to Mobile";
			break;
		case 'C':
			cmdtype = "Command Mobile";
			break;
		case 'O':
			cmdtype = "Load Object";
			break;
		case 'E':
			cmdtype = "Equip Mobile with Object";
			break;
		case 'P':
			cmdtype = "Put Object in Object";
			break;
		case 'R':
			cmdtype = "Remove Object";
			break;
		case 'D':
			cmdtype = "Set Door";
			break;
		case 'T':
			cmdtype = "Trigger";
			break;
		case '*':
			cmdtype = "<Comment>";
			break;
		default:
			cmdtype = "<Unknown>";
	}
	d->m_Character->Send(
				"\x1B[H\x1B[J"
				"`GC`n) Command   : %s\n"
				"`GD`n) Dependent : %s\n",
				cmdtype,
				OLC_CMD(d)->if_flag ? "YES" : "NO");
	
	if (strchr("MORDT", OLC_CMD(d)->command)) {
		const char *hiveStatus = "Ignored";
		
		if (OLC_CMD(d)->hive == 1)		hiveStatus = "Hived";
		else if (OLC_CMD(d)->hive == 2)	hiveStatus = "Not Hived";
		
		d->m_Character->Send("`GH`n) Hived     : %s\n", hiveStatus);			
	}
	
	/* arg1 */
	switch(OLC_CMD(d)->command) {
		case 'M':
			d->m_Character->Send("`G1`n) Mobile    : `y%s [`c%s`y]\n",
					OLC_CMD(d)->mob->m_pProto->GetName(),
					OLC_CMD(d)->mob->GetVirtualID().Print().c_str());
			break;
		case 'G':
		case 'O':
		case 'E':
		case 'P':
			d->m_Character->Send("`G1`n) Object    : `y%s [`c%s`y]\n",
					OLC_CMD(d)->obj->m_pProto->GetName(),
					OLC_CMD(d)->obj->GetVirtualID().Print().c_str());
			break;
		case 'C':
			d->m_Character->Send("`G1`n) Command   : `y%s\n", OLC_CMD(d)->command_string.c_str());
			break;
	}
	
	/* arg2 */
	switch(OLC_CMD(d)->command) {
		case 'M':
		case 'G':
		case 'O':
		case 'E':
		case 'P':
			d->m_Character->Send("`G2`n) Maximum   : `c%d\n",
					OLC_CMD(d)->maxInGame);
			break;
		case 'R':
			d->m_Character->Send("`G2`n) Object    : `y%s [`c%s`y]\n",
					OLC_CMD(d)->obj->m_pProto->GetName(),
					OLC_CMD(d)->obj->GetVirtualID().Print().c_str());
			break;
		case 'D':
			d->m_Character->Send("`G2`n) Door      : `c%s\n",
					GetEnumName(OLC_CMD(d)->direction));
			break;
		case 'T':
			d->m_Character->Send("`G2`n) Script    : `y%s [`c%s`y]\n",
					OLC_CMD(d)->script->m_pProto->GetName(),
					OLC_CMD(d)->script->GetVirtualID().Print().c_str());
			break;
	}
	
	/* arg3 */
	switch(OLC_CMD(d)->command) {
		case 'E':
			d->m_Character->Send("`G3`n) Location  : `y%s\n",
				equipment_types[OLC_CMD(d)->equipLocation]);
			break;
		case 'P':
			d->m_Character->Send("`G3`n) Container : `y%s [`c%s`y]\n",
				OLC_CMD(d)->container->m_pProto->GetName(),
				OLC_CMD(d)->container->GetVirtualID().Print().c_str());
			break;
		case 'D':
			d->m_Character->Send("`G3`n) Door state: `y%s\n", GetEnumName(OLC_CMD(d)->doorState));
			break;
		case 'T':
			d->m_Character->Send("`G3`n) Maximum   : `c%d\n",
					OLC_CMD(d)->maxInGame);
			break;
	}
	
 	/* arg4 */
	switch(OLC_CMD(d)->command) {
		case 'M':
			d->m_Character->Send("`G4`n) Max/Zone  : `c%d\n",
					OLC_CMD(d)->maxInZone);
			break;
	}
	
 	/* arg5 */
	switch(OLC_CMD(d)->command) {
		case 'O':
		case 'M':
			d->m_Character->Send("`G5`n) Max/Room  : `c%d\n",
					OLC_CMD(d)->maxInRoom);
			break;
	}

	if (strchr("GORP", OLC_CMD(d)->command) != NULL)
		d->m_Character->Send("`GR`n) Repeat    : `c%d\n", OLC_CMD(d)->repeat);
	send_to_char(
			"`GQ`n) Quit\n\n"
			"`CEnter your choice :`n ",
			d->m_Character);

	OLC_MODE(d) = ZEDIT_COMMAND_MENU;
}


/* the main menu */
void zedit_disp_menu(DescriptorData * d) {
	BUFFER(buf, MAX_STRING_LENGTH);
	int counter = 0;
  
	if (OLC_CLIMATE(d))
		strcpy(buf, "Exists");
	else if (OLC_WEATHERPARENT(d))
		sprintf(buf, "Inherits from zone '%s'", OLC_WEATHERPARENT(d)->GetTag());
	else
		strcpy(buf, "None");

  /*. Menu header .*/
	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"Room number: `c%s`n		Zone: `c%s`n\n"
			"`GZ`n) Zone name   : `y%s\n"
			"`GX`n) Zone comment: `y%s\n"
			"`GO`n) Zone owner  : `y%s\n"
			"`GF`n) Flags       : `y%s\n"
			"`GL`n) Lifespan    : `y%d minutes\n",
			OLC_VID(d).Print().c_str(), OLC_ZONE(d)->GetTag(),
			!OLC_ZONE(d)->m_Name.empty() ? OLC_ZONE(d)->GetName() : "<NONE!>",
			!OLC_ZONE(d)->m_Comment.empty() ? OLC_ZONE(d)->GetComment() : "<NONE!>",
			get_name_by_id(OLC_ZONE(d)->GetOwner(), ""),
			OLC_ZONE(d)->GetFlags().print().c_str(),
			OLC_ZONE(d)->lifespan);
			
	if (OLC_ZONE(d)->GetZoneNumber() >= 0)	d->m_Character->Send("`GT`n) Top of zone : `y%d\n", OLC_ZONE(d)->top);
	
	d->m_Character->Send(
			"`GR`n) Reset Mode  : `y%s\n"
			"`GW`n) Weather     : `y%s\n"
			"`n[Command list]\n",
			OLC_ZONE(d)->m_ResetMode != ZoneData::ResetMode_Never ? ((OLC_ZONE(d)->m_ResetMode == ZoneData::ResetMode_Empty) ?
				"Reset when no players are in zone." : 
				"Normal reset.") :
				"Never reset",
			buf);

	/*. Print the commands for this room into display buffer .*/
	FOREACH(ResetCommandList, OLC_ZONE(d)->cmd, iter)
	{	// Translate what the command means
#if defined( NEW_ZRESET_TYPES )
		d->m_Character->Send("`n%-2d - `y%\n", counter++, (*iter)->Print().c_str());
#else
		ResetCommand &MYCMD = *iter;
		if (strchr("GORP", MYCMD.command) != NULL && MYCMD.repeat > 1)
			sprintf(buf, "(x%2d) ", MYCMD.repeat);
		else
			strcpy(buf, "");
			
		switch(MYCMD.command) {
			case 'M':
				sprintf_cat(buf, "Load %s [`c%s`y]",
						MYCMD.mob->m_pProto->GetName(),
						MYCMD.mob->GetVirtualID().Print().c_str());
				break;
			case 'G':
				sprintf_cat(buf, "Give it %s [`c%s`y]",
						MYCMD.obj->m_pProto->GetName(),
						MYCMD.obj->GetVirtualID().Print().c_str());
				break;
			case 'C':
				sprintf_cat(buf, "Do command: `c%s`y", MYCMD.command_string.c_str());
				break;
			case 'O':
				sprintf_cat(buf, "Load %s [`c%s`y]",
						MYCMD.obj->m_pProto->GetName(),
					MYCMD.obj->GetVirtualID().Print().c_str());
				break;
			case 'E':
				sprintf_cat(buf, "Equip with %s [`c%s`y], %s",
						MYCMD.obj->m_pProto->GetName(),
						MYCMD.obj->GetVirtualID().Print().c_str(),
						equipment_types[MYCMD.equipLocation]);
				break;
			case 'P':
				sprintf_cat(buf, "Put %s [`c%s`y] in %s [`c%s`y]",
						MYCMD.obj->m_pProto->GetName(),
						MYCMD.obj->GetVirtualID().Print().c_str(),
						MYCMD.container->m_pProto->GetName(),
						MYCMD.container->GetVirtualID().Print().c_str());
				break;
			case 'R':
				sprintf_cat(buf, "Remove %s [`c%s`y] from room.",
						MYCMD.obj->m_pProto->GetName(),
						MYCMD.obj->GetVirtualID().Print().c_str());
				break;
			case 'D':
				sprintf_cat(buf, "Set door %s as %s.",
						GetEnumName(MYCMD.direction),
						GetEnumName(MYCMD.doorState));
				break;
			case 'T':
				sprintf_cat(buf, "Attach script %s [`c%s`y]",
						MYCMD.script->m_pProto->GetName(),
						MYCMD.script->GetVirtualID().Print().c_str());
				break;
			case '*':
				strcpy(buf, "<Comment>");
				break;
			default:
				strcpy(buf, "<Unknown Command>");
				break;
		}
		/* Build the display buffer for this command */
//		if (strchr("MORDT", MYCMD.command))
		const char *hiveStatus = "";
		if (MYCMD.hive == 1)		hiveStatus = "[H] ";
		else if (MYCMD.hive == 2)	hiveStatus = "[N] ";
		
		d->m_Character->Send("`n%-2d - `y%s%s%s",
				counter++,
				MYCMD.if_flag ? " then " : "",
				hiveStatus,
				buf);
		if (strchr("EGMOPT", MYCMD.command) && MYCMD.maxInGame > 0)
			d->m_Character->Send(", Max : %d", MYCMD.maxInGame);
		if (strchr("M", MYCMD.command) && MYCMD.maxInZone > 0)
			d->m_Character->Send(", Max/Zone : %d", MYCMD.maxInZone);
		if (strchr("MO", MYCMD.command) && MYCMD.maxInRoom > 0)
			d->m_Character->Send(", Max/Room : %d", MYCMD.maxInRoom);
		send_to_char("\n", d->m_Character);
#endif
	}
	/* Finish off menu */
	d->m_Character->Send(
			"`n%-2d - <END OF LIST>\n"
			"`GN`y) New command.\n"
			"`GE`y) Edit a command.\n"
			"`GD`y) Delete a command.\n"
			"`GQ`y) Quit\n"
			"`CEnter your choice:`n ",
			counter);

	OLC_MODE(d) = ZEDIT_MAIN_MENU;
}


/* object extra flags */
void zedit_disp_flags(DescriptorData * d) {
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (int counter = 0, columns = 0; counter < NUM_ZONE_FLAGS; counter++) {
		d->m_Character->Send("`g%2d`n) `c%-20.20s %s", counter + 1, GetEnumName<ZoneFlag>(counter),
				!(++columns % 2) ? "\n" : "");
	}
	d->m_Character->Send("\n`nZone flags: `c%s\n"
				"`nEnter zone flag (0 to quit): ", OLC_ZONE(d)->GetFlags().print().c_str());
	
	OLC_MODE(d) = ZEDIT_FLAGS;
}


/*-------------------------------------------------------------------*/
//	Print the command type menu and setup response catch.

void zedit_disp_comtype(DescriptorData *d) {
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	send_to_char(
			"`gM`n) Load Mobile to room             `gO`n) Load Object to room\n"
			"`gE`n) Equip mobile with object        `gG`n) Give an object to a mobile\n"
			"`gP`n) Put object in another object    `gD`n) Open/Close/Lock a Door\n"
			"`gR`n) Remove an object from the room  `gC`n) Force mobile to do command\n"
			"`gT`n) Attach Script\n"
			"What sort of command will this be? : ",
			d->m_Character);
	OLC_MODE(d) = ZEDIT_COMMAND_TYPE;
}


/*-------------------------------------------------------------------*/
//	Print the appropriate message for the command type for arg1 and set
//	up the input catch clause

void zedit_disp_arg1(DescriptorData *d) {
	switch(OLC_CMD(d)->command) {
		case 'M':
			send_to_char("Input mob : ", d->m_Character);
			OLC_MODE(d) = ZEDIT_ARG1;
			break;
		case 'O':
		case 'E':
		case 'P':
		case 'G':
			send_to_char("Input object : ", d->m_Character);
			OLC_MODE(d) = ZEDIT_ARG1;
			break;
		case 'D':
		case 'R':
			/*. Arg1 for these is the room number, skip to arg2 .*/
			OLC_CMD(d)->room = world.Find(OLC_VID(d));
			zedit_disp_arg2(d);
			break;
		case 'C':
			send_to_char("Command : ", d->m_Character);
			OLC_MODE(d) = ZEDIT_ARG1;
			break;
		default:
			//	We should never get here
			//	cleanup_olc(d, CLEANUP_ALL);
			mudlog("SYSERR: OLC: zedit_disp_arg1(): Help!", BRF, LVL_BUILDER, TRUE);
			send_to_char("Oops...\n", d->m_Character);
	}
}

    

/*-------------------------------------------------------------------*/
/*. Print the appropriate message for the command type for arg2 and set
    up the input catch clause .*/

void zedit_disp_arg2(DescriptorData *d) {
	int i = 0;

	switch(OLC_CMD(d)->command) {
		case 'M':
		case 'O':
		case 'E':
		case 'P':
		case 'G':
			send_to_char("Input the maximum number that can exist: ", d->m_Character);
			break;
		case 'D':
			for (i = 0; i < NUM_OF_DIRS; ++i)
			{
				d->m_Character->Send("%d) Exit %s.\n", i, GetEnumName<Direction>(i));
			}
			send_to_char("Enter exit number for door : ", d->m_Character);
			break;
		case 'R':
			send_to_char("Input object : ", d->m_Character);
			break;
		case 'T':
			send_to_char("Input script : ", d->m_Character);
			break;
		default:
			cleanup_olc(d);
			mudlog("SYSERR: OLC: zedit_disp_arg2(): Help!", BRF, LVL_BUILDER, TRUE);
			return;
	}
	OLC_MODE(d) = ZEDIT_ARG2;
}


/*-------------------------------------------------------------------*/
/*. Print the appropriate message for the command type for arg3 and set
    up the input catch clause .*/

void zedit_disp_arg3(DescriptorData *d) {
	int i = 0;
	
	switch(OLC_CMD(d)->command) { 
		case 'E':
			for (int counter = 0, columns = 0; *equipment_types[counter] != '\n'; ++counter) {
				d->m_Character->Send("`g%2d`n) `y%26.26s %s",
					counter + 1, equipment_types[counter], !(++columns % 2) ? "\n" : "");
			}
			send_to_char("\n`nInput location to equip : ", d->m_Character);
			break;
		case 'P':
			send_to_char("Input container : ", d->m_Character);
			break;
		case 'D':
			for (int i = 0; i < ResetCommand::NUM_DOOR_STATES; ++i)
			{
				d->m_Character->Send("%d)  %s\n", i, GetEnumName<ResetCommand::DoorState>(i));
			}
			send_to_char("Enter state of the door : ", d->m_Character);
			break;
		case 'T':
			send_to_char("Input the maximum number that can exist on the mud : ", d->m_Character);
			break;
		default:
			cleanup_olc(d);
			mudlog("SYSERR: OLC: zedit_disp_arg3(): Help!", BRF, LVL_BUILDER, TRUE);
			return;
	}
	OLC_MODE(d) = ZEDIT_ARG3;
}

    
void zedit_disp_arg4(DescriptorData *d) {
	switch(OLC_CMD(d)->command) { 
		case 'M':
			send_to_char("Input the maximum number that can exist in the zone : ", d->m_Character);
			break;
		default:
			cleanup_olc(d);
			mudlog("SYSERR: OLC: zedit_disp_arg4(): Help!", BRF, LVL_BUILDER, TRUE);
			return;
	}
	OLC_MODE(d) = ZEDIT_ARG4;
}


void zedit_disp_arg5(DescriptorData *d) {
	switch(OLC_CMD(d)->command) { 
		case 'O':
		case 'M':
			send_to_char("Input the maximum number that can exist in the room : ", d->m_Character);
			break;
		default:
			cleanup_olc(d);
			mudlog("SYSERR: OLC: zedit_disp_arg5(): Help!", BRF, LVL_BUILDER, TRUE);
			return;
	}
	OLC_MODE(d) = ZEDIT_ARG5;
}


void zedit_disp_repeat(DescriptorData *d) {
	switch(OLC_CMD(d)->command) { 
		case 'O':
		case 'G':
		case 'P':
		case 'R':
			send_to_char("Enter number of times to repeat (0 or 1 to do once) : ", d->m_Character);
			break;
		default:
			zedit_disp_menu(d);
			return;
	}
	OLC_MODE(d) = ZEDIT_REPEAT;
}


void zedit_disp_weather_menu(DescriptorData *d)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	
	d->m_Character->Send(
#if defined(CLEAR_SCREEN)
			"\x1B[H\x1B[J"
#endif
			"Climate for zone '%s':\n",
			OLC_ZONE(d)->GetTag());

	if (!OLC_CLIMATE(d))
	{
		d->m_Character->Send(
			"%s"
			"`GC`n) Create\n"
			"`GZ`n) Parent Zone : %s\n",
			!OLC_WEATHERPARENT(d) ? "   No weather system exists for this zone, and it has no parent.\n" : "",
			OLC_WEATHERPARENT(d) ? OLC_WEATHERPARENT(d)->GetTag() : "<NONE>");
	}
	else
	{
		d->m_Character->Send(
			"`GD`n) Delete\n"
			"`GZ`n) Delete and Set a Parent Zone\n"
			"`GF`n) Flags            : `c%s`n\n"
			"`GE`n) Additional Energy: `c%d`n\n"
			"`GP`n) Seasonal Pattern : `c%s`n\n"
			"`GS`n) Edit Season\n",
			OLC_CLIMATE(d)->m_Flags.print().c_str(),
			OLC_CLIMATE(d)->m_Energy,
			GetEnumName(OLC_CLIMATE(d)->m_SeasonalPattern));
			
		int numSeasons = Weather::num_seasons[OLC_CLIMATE(d)->m_SeasonalPattern];
		for (int i = 0; i < numSeasons; ++i)
		{
			Weather::Climate::Season &season = OLC_CLIMATE(d)->m_Seasons[i];
		
			d->m_Character->Send(
					"    Season %d:\n"
					"       Wind     : `c%s`n, %s%s\n"
					"       Precip   : `c%-20.20s`n Temp     : `c%s`n\n",
					i + 1,
					GetEnumName(season.m_Wind),
					findtype(season.m_WindDirection, dirs),
					season.m_WindVaries ? " (Varies)" : "",
					GetEnumName(season.m_Precipitation),
					GetEnumName(season.m_Temperature));
		}
	}
	d->m_Character->Send(
		"`GQ`y) Quit\n"
		"`CEnter your choice:`n ");
	
	OLC_MODE(d) = ZEDIT_WEATHER;
}


void zedit_disp_weather_flags(DescriptorData *d)
{
	int i, columns = 0;
  
#if defined(CLEAR_SCREEN)
	send_to_char("\x1B[H\x1B[J", d->m_Character);
#endif
	for (i = 0; i < Weather::NumClimateBits; i++) {
		d->m_Character->Send("`g%2d`n) `c%-20.20s  %s", i+1, GetEnumName<Weather::ClimateBits>(i),
				!(++columns % 2) ? "\n" : "");
	}
	
	d->m_Character->Send("\n"
				"`nCurrent flags   : `c%s\n"
				"`nEnter aff flags (0 to quit): ",
				OLC_CLIMATE(d)->m_Flags.print().c_str());
				
	OLC_MODE(d) = ZEDIT_WEATHER_FLAGS;
}


void zedit_disp_weather_season_pattern_menu(DescriptorData *d)
{
	int counter;
	
	for (counter = 0; counter < Weather::Seasons::NumPatterns; ++counter) {
		d->m_Character->Send("`g%1d`n) `c%s\n", counter, GetEnumName<Weather::Seasons::Pattern>(counter));
	}
	d->m_Character->Send("\n`nEnter new seasonal pattern: ");
	
	OLC_MODE(d) = ZEDIT_WEATHER_SEASON_PATTERN;
}


void zedit_disp_weather_season_menu(DescriptorData *d)
{
	Weather::Climate::Season &season = OLC_CLIMATE(d)->m_Seasons[OLC_VAL2(d)];
	
	d->m_Character->Send(
			"-- Season %d:\n"
			"`gW`n) Wind          : `c%s`n\n"
			"`gD`n)   Direction   : `c%s`n\n"
			"`gV`n)   Varies      : %s\n"
			"`gP`n) Precipitation : `c%s`n\n"
			"`gT`n) Temperature   : `c%s`n\n"
			"`GQ`n) Exit Menu\n"
			"Enter choice: ",
			OLC_VAL2(d) + 1,
			GetEnumName(season.m_Wind),
			findtype(season.m_WindDirection, dirs),
			YESNO(season.m_WindVaries),
			GetEnumName(season.m_Precipitation),
			GetEnumName(season.m_Temperature));
			
	OLC_MODE(d) = ZEDIT_WEATHER_SEASON_MENU;
}


void zedit_disp_weather_wind_menu(DescriptorData *d)
{
	int counter;
	
	for (counter = 0; counter < Weather::Wind::NumTypes; ++counter) {
		d->m_Character->Send("`g%1d`n) `c%s\n", counter, GetEnumName<Weather::Wind::WindType>(counter));
	}
	d->m_Character->Send("\n`nEnter new wind: ");
	
	OLC_MODE(d) = ZEDIT_WEATHER_SEASON_WIND;
}


void zedit_disp_weather_wind_direction_menu(DescriptorData *d)
{
	int counter;
	
	for (counter = 0; counter < NUM_OF_DIRS; ++counter) {
		d->m_Character->Send("`g%1d`n) `c%s\n", counter, GetEnumName<Direction>(counter));
	}
	d->m_Character->Send("\n`nEnter new wind direction: ");
	
	OLC_MODE(d) = ZEDIT_WEATHER_SEASON_WIND_DIRECTION;
}


void zedit_disp_weather_precipitation_menu(DescriptorData *d)
{
	int counter;
	
	for (counter = 0; counter < Weather::Precipitation::NumTypes; ++counter) {
		d->m_Character->Send("`g%1d`n) `c%s\n", counter, GetEnumName<Weather::Precipitation::PrecipitationType>(counter));
	}
	d->m_Character->Send("\n`nEnter new precipitation: ");
	
	OLC_MODE(d) = ZEDIT_WEATHER_SEASON_PRECIPITATION;
}


void zedit_disp_weather_temperature_menu(DescriptorData *d)
{
	int counter;
	
	for (counter = 0; counter < Weather::Temperature::NumTypes; ++counter) {
		d->m_Character->Send("`g%1d`n) `c%s\n", counter, GetEnumName<Weather::Temperature::TemperatureType>(counter));
	}
	d->m_Character->Send("\n`nEnter new temperature: ");
	
	OLC_MODE(d) = ZEDIT_WEATHER_SEASON_TEMPERATURE;
}


/**************************************************************************
  The GARGANTAUN event handler
 **************************************************************************/

void zedit_parse(DescriptorData * d, const char *arg) {
	int pos, i = 0;
	switch (OLC_MODE(d)) {
/*-------------------------------------------------------------------*/
		case ZEDIT_CONFIRM_SAVESTRING:
			switch (*arg) {
				case 'y':
				case 'Y':
					//	Save the zone in memory
					send_to_char("Saving changes to zone info - no need to zedit save!\n", d->m_Character);
					zedit_save_internally(d);
					mudlogf(NRM, LVL_BUILDER, TRUE, "OLC: %s edits zone info for room %s",
							d->m_Character->GetName(), OLC_VID(d).Print().c_str());
					// FALL THROUGH
				case 'n':
				case 'N':
					cleanup_olc(d);
					break;
				default:
					send_to_char("Invalid choice!\n"
								 "Do you wish to save this zone info? ", d->m_Character);
					break;
			}
			break; /* end of ZEDIT_CONFIRM_SAVESTRING */

/*-------------------------------------------------------------------*/
		case ZEDIT_MAIN_MENU:
			switch (tolower(*arg))
			{
				case 'q':
					if(OLC_VAL(d)) {
						send_to_char("Do you wish to save this zone info? ", d->m_Character);
						OLC_MODE(d) = ZEDIT_CONFIRM_SAVESTRING;
					} else {
						send_to_char("No changes made.\n", d->m_Character);
						cleanup_olc(d);
					}
					break;
				case 'n':	//	New entry
					send_to_char("What number in the list should the new command be? : ", d->m_Character);
					OLC_MODE(d) = ZEDIT_NEW_ENTRY;
					break;
				case 'e':	//	Change an entry 
					send_to_char("Which command do you wish to change? : ", d->m_Character);
					OLC_MODE(d) = ZEDIT_CHANGE_ENTRY;
					break;
				case 'd':	//	Delete an entry
					send_to_char("Which command do you wish to delete? : ", d->m_Character);
					OLC_MODE(d) = ZEDIT_DELETE_ENTRY;
					break;
				case 'z':	//	Edit zone name
					send_to_char("Enter new zone name : ", d->m_Character);
					OLC_MODE(d) = ZEDIT_ZONE_NAME;
					break;
				case 'x':	//	Edit zone name
					send_to_char("Enter zone comment : ", d->m_Character);
					OLC_MODE(d) = ZEDIT_ZONE_COMMENT;
					break;
				case 't':	//	Edit zone top
					if(d->m_Character->GetLevel() < LVL_SRSTAFF
						|| OLC_ZONE(d)->GetZoneNumber() < 0)
						zedit_disp_menu(d);
					else {
						send_to_char("Enter new top of zone: ", d->m_Character);
						OLC_MODE(d) = ZEDIT_ZONE_TOP;
					}
					break;
				case 'l':	//	Edit zone lifespan
					send_to_char("Enter new zone lifespan: ", d->m_Character);
					OLC_MODE(d) = ZEDIT_ZONE_LIFE;
					break;
				case 'r':	//	Edit zone reset mode
					send_to_char(
							"\n"
							"0) Never reset\n"
							"1) Reset only when no players in zone\n"
							"2) Normal reset\n"
							"Enter new zone reset type: ", d->m_Character);
					OLC_MODE(d) = ZEDIT_ZONE_RESET;
					break;
				
				case 'o':
					if (!STF_FLAGGED(d->m_Character, STAFF_OLCADMIN))
					{
						zedit_disp_menu(d);
						break;
					}
					
					d->m_Character->Send("Choose new owner: ");
					OLC_MODE(d) = ZEDIT_OWNER;
					break;
				
				case 'w':
					zedit_disp_weather_menu(d);
					break;
					
				case 'f':
					zedit_disp_flags(d);
					break;
					
				default:
					zedit_disp_menu(d);
					break;
			}
			break;

/*-------------------------------------------------------------------*/
		case ZEDIT_NEW_ENTRY:
			/*. Get the line number and insert the new line .*/
			pos = atoi(arg);
			if (isdigit(*arg) && new_command(d, pos)) {
				if (start_change_command(d, pos)) {
					zedit_disp_comtype(d);
					SET_BIT(OLC_VAL(d), COMMANDS_CHANGED);
				}
			} else
				zedit_disp_menu(d);
			break;

/*-------------------------------------------------------------------*/
		case ZEDIT_DELETE_ENTRY:
			/*. Get the line number and delete the line .*/
			pos = atoi(arg);
			if(isdigit(*arg)) {
				delete_command(d, pos);
				SET_BIT(OLC_VAL(d), COMMANDS_CHANGED);
			}
			zedit_disp_menu(d);
			break;

/*-------------------------------------------------------------------*/
		case ZEDIT_CHANGE_ENTRY:
			//	Parse the input for which line to edit, and goto next quiz
			pos = atoi(arg);
			if(isdigit(*arg) && start_change_command(d, pos))
				zedit_command_menu(d);
			else
				zedit_disp_menu(d);
			break;

/*-------------------------------------------------------------------*/
		case ZEDIT_COMMAND_TYPE:
			//	Parse the input for which type of command this is, and goto next quiz
			{
				char cmd = toupper(*arg);
				if (!cmd || !(strchr("MOPEDGTRC", cmd)))
					send_to_char("Invalid choice, try again : ", d->m_Character);
				else
				{
					if (OLC_CMD(d)->command != cmd)
					{
						*OLC_CMD(d) = ResetCommand();
						OLC_CMD(d)->command = cmd;
						switch (OLC_CMD(d)->command)
						{
							case 'C':	OLC_CMD(d)->command_string = "say I need a command!";	break;
							case 'P':	OLC_CMD(d)->container = obj_index[0];	//	Fallthrough
							case 'O':											//	 |
							case 'R':											//	 |
							case 'G':											//	\|/
							case 'E':	OLC_CMD(d)->obj = obj_index[0];			break;
							case 'M':	OLC_CMD(d)->mob = mob_index[0];			break;
							case 'T':	OLC_CMD(d)->script = trig_index[0];		break;
						}

						switch (OLC_CMD(d)->command)
						{
							case 'O':
							case 'M':
							case 'R':
							case 'D':
							case 'T':
								OLC_CMD(d)->room = world.Find(OLC_VID(d));
								break;
						}
					}
					zedit_command_menu(d);
				}
			}
			break;

/*-------------------------------------------------------------------*/
		case ZEDIT_IF_FLAG:
			//	Parse the input for the if flag, and goto next quiz
			switch(*arg) {
				case 'y':
				case 'Y':	OLC_CMD(d)->if_flag = 1;							break;
				case 'n':
				case 'N':	OLC_CMD(d)->if_flag = 0;							break;
				default:	send_to_char("Try again : ", d->m_Character);		return;
			}
			zedit_command_menu(d);
			break;


/*-------------------------------------------------------------------*/
		case ZEDIT_ARG1:
			//	Parse the input for arg1, and goto next quiz
			if (OLC_CMD(d)->command == 'C') {
				OLC_CMD(d)->command_string = arg;
				zedit_command_menu(d);
				break;
			}
			if  (!IsVirtualID(arg))
			{
				send_to_char("Must be a Virtual ID, try again : ", d->m_Character);
				return;
			}
			switch(OLC_CMD(d)->command)
			{
				case 'M':
					OLC_CMD(d)->mob = mob_index.Find(VirtualID(arg, OLC_ZONE(d)->GetHash()));
					if (OLC_CMD(d)->mob)	zedit_command_menu(d);
					else					send_to_char("That mobile does not exist, try again : ", d->m_Character);
					break;
				case 'O':
				case 'P':
				case 'E':
				case 'G':
					OLC_CMD(d)->obj = obj_index.Find(VirtualID(arg, OLC_ZONE(d)->GetHash()));
					if (OLC_CMD(d)->obj)	zedit_command_menu(d);
					else					send_to_char("That object does not exist, try again : ", d->m_Character);
					break;
				case 'D':
				case 'R':
				case 'T':
				case 'Z':
				default:
					//	We should never get here
					//	cleanup_olc(d, CLEANUP_ALL);
					mudlog("SYSERR: OLC: zedit_parse(): case ARG1: Ack!", BRF, LVL_BUILDER, TRUE);
					send_to_char("Oops...\n", d->m_Character);
					break;
			}
			break;


/*-------------------------------------------------------------------*/
		case ZEDIT_ARG2:
			//	Parse the input for arg2, and goto next quiz
			if (OLC_CMD(d)->command == 'R'
				|| OLC_CMD(d)->command == 'T')
			{
				if  (!IsVirtualID(arg))
				{
					send_to_char("Must be a Virtual ID, try again : ", d->m_Character);
					return;
				}
			}
			else if (!isdigit(*arg))
			{
				send_to_char("Must be a numeric value, try again : ", d->m_Character);
				return;
			}
			
			switch(OLC_CMD(d)->command) {
				case 'M':
				case 'O':
				case 'G':
				case 'P':
				case 'E':
					OLC_CMD(d)->maxInGame = atoi(arg);
					zedit_command_menu(d);
					break;
				case 'D':
					pos = atoi(arg);
					if ((unsigned)pos > NUM_OF_DIRS)
						send_to_char("Try again : ", d->m_Character);
					else {
						OLC_CMD(d)->direction = (Direction)pos;
						zedit_command_menu(d);
					}
					break;
				case 'R':
					OLC_CMD(d)->obj = obj_index.Find(VirtualID(arg, OLC_ZONE(d)->GetHash()));
					if (OLC_CMD(d)->obj)	zedit_command_menu(d);
					else					send_to_char("That object does not exist, try again : ", d->m_Character);
					break;
				case 'T':
					OLC_CMD(d)->script = trig_index.Find(VirtualID(arg, OLC_ZONE(d)->GetHash()));
					if (OLC_CMD(d)->script)
					{
						OLC_CMD(d)->triggerType = GET_TRIG_DATA_TYPE(OLC_CMD(d)->script->m_pProto);
						zedit_command_menu(d);
					}
					else 
						send_to_char("That trigger does not exist, try again : ", d->m_Character);
					break;
				default:
					//	We should never get here
					//	cleanup_olc(d, CLEANUP_ALL);
					mudlog("SYSERR: OLC: zedit_parse(): case ARG2: Ack!", BRF, LVL_BUILDER, TRUE);
					send_to_char("Oops...\n", d->m_Character);
					break;
			}
			break;

/*-------------------------------------------------------------------*/
		case ZEDIT_ARG3:
			//	Parse the input for arg3, and go back to main menu.
			if (OLC_CMD(d)->command == 'P')
			{
				if  (!IsVirtualID(arg))
				{
					send_to_char("Must be a Virtual ID, try again : ", d->m_Character);
					return;
				}
			}
			else if (!isdigit(*arg))
			{
				send_to_char("Must be a numeric value, try again : ", d->m_Character);
				return;
			}
			switch(OLC_CMD(d)->command) {
				case 'E':
					pos = atoi(arg);
					//	Count number of wear positions (could use NUM_WEARS,
					//	this is more reliable)
					i = 0;
					while(*equipment_types[i] != '\n')
						++i;
					if ((unsigned)pos > i)
						send_to_char("Try again : ", d->m_Character);
					else
					{
						OLC_CMD(d)->equipLocation = pos;
						zedit_command_menu(d);
					}
					break;
				case 'P':
					OLC_CMD(d)->container = obj_index.Find(VirtualID(arg, OLC_ZONE(d)->GetHash()));
					if (OLC_CMD(d)->container)	zedit_command_menu(d);
					else						send_to_char("That object does not exist, try again : ", d->m_Character);
					break;
				case 'D':
					pos = atoi(arg);
					if ((unsigned)pos > ResetCommand::NUM_DOOR_STATES)
						send_to_char("Try again : ", d->m_Character);
					else {
						OLC_CMD(d)->doorState = (ResetCommand::DoorState)pos;
						zedit_command_menu(d);
					}
					break;
				case 'T':
					OLC_CMD(d)->maxInGame = atoi(arg);
					zedit_command_menu(d);
					break;
				default:
					//	We should never get here
					//	cleanup_olc(d, CLEANUP_ALL);
					mudlog("SYSERR: OLC: zedit_parse(): case ARG3: Ack!", BRF, LVL_BUILDER, TRUE);
					send_to_char("Oops...\n", d->m_Character);
					break;
			}
			break;

/*-------------------------------------------------------------------*/
		case ZEDIT_ARG4:
			//	Parse the input for arg4, and go back to main menu
			if  (!isdigit(*arg)) {
				send_to_char("Must be a numeric value, try again : ", d->m_Character);
				return;
			}
			switch(OLC_CMD(d)->command) { 
				case 'M':
					OLC_CMD(d)->maxInZone = atoi(arg);
					zedit_command_menu(d);
					break;
				default:
					//	We should never get here
					//	cleanup_olc(d, CLEANUP_ALL);
					mudlog("SYSERR: OLC: zedit_parse(): case ARG4: Ack!", BRF, LVL_BUILDER, TRUE);
					send_to_char("Oops...\n", d->m_Character);
					break;
			}
			break;
			
/*-------------------------------------------------------------------*/
		case ZEDIT_ARG5:
			//	Parse the input for arg5, and go back to main menu
			if  (!isdigit(*arg)) {
				send_to_char("Must be a numeric value, try again : ", d->m_Character);
				return;
			}
			switch(OLC_CMD(d)->command) { 
				case 'M':
				case 'O':
					OLC_CMD(d)->maxInRoom = atoi(arg);
					zedit_command_menu(d);
					break;
				default:
					//	We should never get here
					//	cleanup_olc(d, CLEANUP_ALL);
					mudlog("SYSERR: OLC: zedit_parse(): case ARG5: Ack!", BRF, LVL_BUILDER, TRUE);
					send_to_char("Oops...\n", d->m_Character);
					break;
			}
			break;
			
/*-------------------------------------------------------------------*/
		case ZEDIT_REPEAT:
			//	Repeat for several times
			pos = atoi(arg);
			if (pos > 0)	OLC_CMD(d)->repeat = MIN(pos, 20);
			else			OLC_CMD(d)->repeat = 1;
			zedit_command_menu(d);
			break;
  		
/*-------------------------------------------------------------------*/
  		case ZEDIT_HIVE:
  			pos = atoi(arg);
  			if ((unsigned)pos <= 2)	OLC_CMD(d)->hive = pos;
  			zedit_command_menu(d);
  			break;
  		
		case ZEDIT_OWNER:
			if (!*arg)
			{
				if (OLC_ZONE(d)->GetOwner())
				{
					OLC_ZONE(d)->SetOwner(0);
					SET_BIT(OLC_VAL(d), HEADER_CHANGED);
				}
			}
			else
			{
				IDNum player = get_id_by_name(arg);
				if (player)
				{
					OLC_ZONE(d)->SetOwner(player);
					SET_BIT(OLC_VAL(d), HEADER_CHANGED);
				}
			}
  			zedit_disp_menu(d);
  			break;
  			
/*-------------------------------------------------------------------*/
		case ZEDIT_ZONE_NAME:
			//	Add new name and return to main menu
			OLC_ZONE(d)->m_Name = arg;
			SET_BIT(OLC_VAL(d), HEADER_CHANGED);
			zedit_disp_menu(d);
			break;

/*-------------------------------------------------------------------*/
		case ZEDIT_ZONE_COMMENT:
			//	Add new name and return to main menu
			OLC_ZONE(d)->m_Comment = arg;
			SET_BIT(OLC_VAL(d), HEADER_CHANGED);
			zedit_disp_menu(d);
			break;
		
		case ZEDIT_FLAGS:
			pos = atoi(arg);
			
			if (pos == 0)
			{
				zedit_disp_menu(d);
				return;
			}
			else if ((unsigned)pos > NUM_ZONE_FLAGS)
			{
				send_to_char("That's not a valid choice!\n", d->m_Character);
			}
			else
			{
				OLC_ZONE(d)->GetFlags().flip(static_cast<ZoneFlag>(pos - 1));
				SET_BIT(OLC_VAL(d), HEADER_CHANGED);
			}
			
			zedit_disp_flags(d);
			break;
			
/*-------------------------------------------------------------------*/
		case ZEDIT_ZONE_RESET:
			//	Parse and add new reset_mode and return to main menu
			pos = atoi(arg);
			if (!isdigit(*arg) || ((unsigned)pos > ZoneData::ResetMode_Always))
				send_to_char("Try again (0-2) : ", d->m_Character);
			else {
				OLC_ZONE(d)->m_ResetMode = (ZoneData::ResetMode)pos;
				SET_BIT(OLC_VAL(d), HEADER_CHANGED);
				zedit_disp_menu(d);
			}
			break; 

/*-------------------------------------------------------------------*/
		case ZEDIT_ZONE_LIFE:
			//	Parse and add new lifespan and return to main menu
			pos = atoi(arg);
			if (!isdigit(*arg) || ((unsigned)pos > 240))
				send_to_char("Try again (0-240) : ", d->m_Character);
			else {
				OLC_ZONE(d)->lifespan = pos;
				SET_BIT(OLC_VAL(d), HEADER_CHANGED);
				zedit_disp_menu(d);
			}
			break; 

/*-------------------------------------------------------------------*/
		case ZEDIT_ZONE_TOP:
			//	Parse and add new top room in zone and return to main menu
			if (OLC_ZONE(d)->GetZoneNumber() >= 0)
			{
				OLC_ZONE(d)->top = RANGE((unsigned int)atoi(arg), (unsigned int)OLC_ZONE(d)->GetZoneNumber() * 100, VNumLegacy::MaxTopForZone(OLC_ZONE(d)->GetZoneNumber()));
			}
			SET_BIT(OLC_VAL(d), HEADER_CHANGED);
			zedit_disp_menu(d);
			break;
	
		case ZEDIT_WEATHER:
			switch (tolower(*arg))
			{
				case 'c':
					if (!OLC_CLIMATE(d))
					{
						OLC_CLIMATE(d) = new Weather::Climate;
						OLC_WEATHERPARENT(d) = NULL;
						SET_BIT(OLC_VAL(d), HEADER_CHANGED);
					}
					break;
					
				case 'd':
					if (OLC_CLIMATE(d))
					{
						OLC_CLIMATE(d) = NULL;
						OLC_WEATHERPARENT(d) = NULL;
						SET_BIT(OLC_VAL(d), HEADER_CHANGED);
						zedit_disp_menu(d);
						return;
					}
					break;
					
				case 'z':
					send_to_char("What zone's weather does this zone receive: ", d->m_Character);
					OLC_MODE(d) = ZEDIT_WEATHER_PARENT;
					return;
				
				case 'f':
					if (OLC_CLIMATE(d))
					{
						zedit_disp_weather_flags(d);
						return;
					}
					break;
				
				case 'e':
					if (OLC_CLIMATE(d))
					{
						send_to_char("How much additional energy: ", d->m_Character);
						OLC_MODE(d) = ZEDIT_WEATHER_ENERGY;
						return;
					}
				
				case 'p':
					if (OLC_CLIMATE(d))
					{
						zedit_disp_weather_season_pattern_menu(d);
						return;
					}
					break;
				
				case 's':
					if (OLC_CLIMATE(d))
					{
						send_to_char("Edit which season: ", d->m_Character);
						OLC_MODE(d) = ZEDIT_WEATHER_SEASON;
						return;
					}
					break;
				
				case 'q':
					zedit_disp_menu(d);
					return;
			}
			zedit_disp_weather_menu(d);
			break;
		
		case ZEDIT_WEATHER_SEASON_MENU:
			switch (tolower(*arg))
			{
				case 'w':
					zedit_disp_weather_wind_menu(d);
					return;

				case 'd':
					zedit_disp_weather_wind_direction_menu(d);
					return;
				
				case 'v':
					OLC_CLIMATE(d)->m_Seasons[OLC_VAL2(d)].m_WindVaries = !OLC_CLIMATE(d)->m_Seasons[OLC_VAL2(d)].m_WindVaries;
					break;
				
				case 'p':
					zedit_disp_weather_precipitation_menu(d);
					return;
					
				case 't':
					zedit_disp_weather_temperature_menu(d);
					return;
				
				case 'q':
					zedit_disp_weather_menu(d);
					return;
			}
			zedit_disp_weather_season_menu(d);
			break;
		
		case ZEDIT_WEATHER_PARENT:
			{
				ZoneData *z = zone_table.Find(arg);
				if (z && z != OLC_SOURCE_ZONE(d))
				{
					OLC_CLIMATE(d) = NULL;
					OLC_WEATHERPARENT(d) = z;
					SET_BIT(OLC_VAL(d), HEADER_CHANGED);
				}
			}
			zedit_disp_weather_menu(d);
			break;
		
		case ZEDIT_WEATHER_ENERGY:
			OLC_CLIMATE(d)->m_Energy = RANGE(atoi(arg), -10000, 50000);
			zedit_disp_weather_menu(d);
			break;
		
		case ZEDIT_WEATHER_SEASON_PATTERN:
			pos = atoi(arg);
			
			if ((unsigned)pos < Weather::Seasons::NumPatterns)
			{
				OLC_CLIMATE(d)->m_SeasonalPattern = (Weather::Seasons::Pattern)pos;
				SET_BIT(OLC_VAL(d), HEADER_CHANGED);
			}
			zedit_disp_weather_menu(d);
			break;

		case ZEDIT_WEATHER_SEASON:
			pos = atoi(arg);
			
			if (pos < 1 || pos > Weather::num_seasons[OLC_CLIMATE(d)->m_SeasonalPattern])
			{
				zedit_disp_weather_menu(d);
				return;
			}

			OLC_VAL2(d) = pos - 1;
			SET_BIT(OLC_VAL(d), HEADER_CHANGED);
			zedit_disp_weather_season_menu(d);
			break;
			
		case ZEDIT_WEATHER_FLAGS:
			i = atoi(arg) - 1;
			if (i == -1)
			{
				zedit_disp_weather_menu(d);
				break;	
			}
			else if ((unsigned)i < Weather::NumClimateBits)
			{
				OLC_CLIMATE(d)->m_Flags.flip((Weather::ClimateBits)i);
				SET_BIT(OLC_VAL(d), HEADER_CHANGED);
			}
			zedit_disp_weather_flags(d);
			break;
			
		case ZEDIT_WEATHER_SEASON_WIND:
			OLC_CLIMATE(d)->m_Seasons[OLC_VAL2(d)].m_Wind = (Weather::Wind::WindType)RANGE(atoi(arg), 0, Weather::Wind::NumTypes);
			SET_BIT(OLC_VAL(d), HEADER_CHANGED);
			zedit_disp_weather_season_menu(d);
			break;

		case ZEDIT_WEATHER_SEASON_WIND_DIRECTION:
			OLC_CLIMATE(d)->m_Seasons[OLC_VAL2(d)].m_WindDirection = RANGE(atoi(arg), 0, NUM_OF_DIRS);
			SET_BIT(OLC_VAL(d), HEADER_CHANGED);
			zedit_disp_weather_season_menu(d);
			break;

		case ZEDIT_WEATHER_SEASON_PRECIPITATION:
			OLC_CLIMATE(d)->m_Seasons[OLC_VAL2(d)].m_Precipitation = (Weather::Precipitation::PrecipitationType)RANGE(atoi(arg), 0, Weather::Precipitation::NumTypes);
			SET_BIT(OLC_VAL(d), HEADER_CHANGED);
			zedit_disp_weather_season_menu(d);
			break;

		case ZEDIT_WEATHER_SEASON_TEMPERATURE:
			OLC_CLIMATE(d)->m_Seasons[OLC_VAL2(d)].m_Temperature = (Weather::Temperature::TemperatureType)RANGE(atoi(arg), 0, Weather::Temperature::NumTypes);
			SET_BIT(OLC_VAL(d), HEADER_CHANGED);
			zedit_disp_weather_season_menu(d);
			break;

/*-------------------------------------------------------------------*/
		case ZEDIT_COMMAND_MENU:
			switch(*arg) {
				case 'C':
				case 'c':
					zedit_disp_comtype(d);
					SET_BIT(OLC_VAL(d), COMMANDS_CHANGED);
					break;
				case 'D':
				case 'd':
					if(!strchr("Z", OLC_CMD(d)->command)) { /*. If there was a previous command .*/
						send_to_char("Is this command dependent on the success of the previous one? (y/n)\n", d->m_Character);
						OLC_MODE(d) = ZEDIT_IF_FLAG;
						SET_BIT(OLC_VAL(d), COMMANDS_CHANGED);
					} else
						zedit_command_menu(d);
					break;
				case 'H':
				case 'h':
					if (strchr("MORDT", OLC_CMD(d)->command)) {
						send_to_char("\n"
									 "0) Ignored\n"
									 "1) Must be Hived\n"
									 "2) Must not be Hived\n"
									 "\n"
									 "Hived Requirements: ", d->m_Character);
						OLC_MODE(d) = ZEDIT_HIVE;
						SET_BIT(OLC_VAL(d), COMMANDS_CHANGED);
					} else
						zedit_command_menu(d);
					break;
				case '1':
					if(!strchr("RDZT", OLC_CMD(d)->command)) {
						zedit_disp_arg1(d);
						SET_BIT(OLC_VAL(d), COMMANDS_CHANGED);
					} else
						zedit_command_menu(d);
					break;
				case '2':
					if (!strchr("ZC", OLC_CMD(d)->command)) {
						zedit_disp_arg2(d);
						SET_BIT(OLC_VAL(d), COMMANDS_CHANGED);
					} else
						zedit_command_menu(d);
					break;
				case '3':
					if (strchr("EPDT", OLC_CMD(d)->command)) {
						zedit_disp_arg3(d);
						SET_BIT(OLC_VAL(d), COMMANDS_CHANGED);
					} else
						zedit_command_menu(d);
					break;
				case '4':
					if (strchr("M", OLC_CMD(d)->command)) {
						zedit_disp_arg4(d);
						SET_BIT(OLC_VAL(d), COMMANDS_CHANGED);
					} else
						zedit_command_menu(d);
					break;
				case '5':
					if (strchr("MO", OLC_CMD(d)->command)) {
						zedit_disp_arg5(d);
						SET_BIT(OLC_VAL(d), COMMANDS_CHANGED);
					} else
						zedit_command_menu(d);
					break;
				case 'R':
				case 'r':
					if (strchr("GORP", OLC_CMD(d)->command)) {
						zedit_disp_repeat(d);
						SET_BIT(OLC_VAL(d), COMMANDS_CHANGED);
					} else
						zedit_command_menu(d);
					break;
				case 'Q':
				case 'q':
					zedit_disp_menu(d);
					break;
				default:
					zedit_command_menu(d);
			}
			break;
  	
		default:
			//	We should never get here
			//	cleanup_olc(d, CLEANUP_ALL);
			mudlog("SYSERR: OLC: zedit_parse(): Reached default case!",BRF,LVL_BUILDER,TRUE);
			send_to_char("Oops...\n", d->m_Character);
			break;
	}
}
/*. End of parse_zedit() .*/
