/* ************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */




//	New bootup procedure:
//		Load list of zone filenames:	world/zones
//			Requires: List of zone filenames
//				Requires: zone filename list export
//			Requires: Store zone's filename in zone data
//		Load zones						world/zon/<zonefilename>.zon
//		Load rooms						world/wld/<zonefilename>.wld
//		Renumber world
//		Check start rooms
//		Load clans (Needed for mobs)
//		Load mobs						world/mob/<zonefilename>.mob
//		Load objects					world/obj/<zonefilename>.obj
//		Load scripts					world/trg/<zonefilename>.trg
//		Load shops - TO BE REMOVED		world/shp/<zonefilename>.shp
//		Renumber zones
//		Load polls

//	Needed:

//	To be added - post-load number resolution system
//		(mob clans, zone renumbers, etc)






#include "structs.h"
#include "utils.h"
#include "buffer.h"
#include "scripts.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "find.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"
#include "files.h"
#include "clans.h"
#include "ban.h"
#include "help.h"
#include "extradesc.h"
#include "olc.h"
#include "constants.h"
#include "weather.h"

#include "lexifile.h"
#include "lexiparser.h"

#include <stdarg.h>

void init_char(CharData * ch);
void CheckRegenRates(CharData *ch);
extern void announce(const char *buf);
extern void sort_skills(void);
extern void LoadGlobalTriggers();

/**************************************************************************
*  declarations of most of the 'global' variables                         *
************************************************************************ */

IDNum top_idnum = 0;		/* highest idnum in use		 */

extern bool no_mail;		//	mail disabled?
time_t boot_time = 0;		//	time of mud boot
int circle_restrict = 0;		//	level of game restriction

Lexi::String credits;		//	game credits
Lexi::String news;			//	mud news
Lexi::String motd;			//	message of the day - mortals
Lexi::String imotd;			//	message of the day - immorts
Lexi::String help;			//	help screen
Lexi::String info;			//	info page
Lexi::String wizlist;		//	list of higher gods
Lexi::String immlist;		//	list of peon gods
Lexi::String background;	//	background story
Lexi::String handbook;		//	handbook for new immortals
Lexi::String policies;		//	policies page
Lexi::String login;
Lexi::String welcomeMsg;
Lexi::String startMsg;

struct TimeInfoData time_info;/* the infomation about the time    */


/* local functions */

int count_hash_records(FILE * fl);
ACMD(do_reboot);


enum BootMode
{
	DB_BOOT_ZON,
	DB_BOOT_ZON_RESETS,
	DB_BOOT_WLD,
	DB_BOOT_MOB,
	DB_BOOT_OBJ,
	DB_BOOT_SHP,
	DB_BOOT_TRG,
	NUM_BOOT_MODES
};


ENUM_NAME_TABLE(BootMode, NUM_BOOT_MODES)
{
	{ DB_BOOT_ZON, "Zones" },
	{ DB_BOOT_ZON_RESETS, "Zone Resets" },
	{ DB_BOOT_WLD, "Rooms" },
	{ DB_BOOT_MOB, "Mobiles" },
	{ DB_BOOT_OBJ, "Objects" },
	{ DB_BOOT_SHP, "Shops" },
	{ DB_BOOT_TRG, "Scripts" }
};


void LoadZoneFile(const char *zonename, const char *filename);
void LoadZoneResets(const char *zonename, const char *filename);
void LoadDatabaseFile(const char *filename, Hash zone, BootMode mode);
void LoadDatabase(BootMode mode, const Lexi::StringList &zonelist);
void LoadZoneList(Lexi::StringList &zonelist);
void LoadWorldDatabase(void);

void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_the_shopkeepers(void);
void build_player_index(void);
void reset_zone(ZoneData *zone);

void file_to_lexistring(char *name, Lexi::String &str);
void reset_time(void);
void tally_hive_data(void);

/* external functions */
void load_space(void);
void load_ships(void);
void load_messages(void);
void weather_and_time(int mode);
void boot_social_messages(void);
void create_command_list(void);
void sort_commands(void);
void boot_the_shops(FILE * shop_f, const char *filename);
int find_first_step(RoomData *room, RoomData *target);
struct TimeInfoData mud_time_passed(time_t t2, time_t t1);
void LoadStellarObjects(void);

void load_mtrigger(CharData *mob);
void load_otrigger(ObjData *obj);
void reset_wtrigger(RoomData *room);
int wear_otrigger(ObjData *obj, CharData *actor, int where);


/*************************************************************************
*  routines for booting the system                                       *
*********************************************************************** */


ACMD(do_reboot) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	one_argument(argument, arg);

	if (!str_cmp(arg, "all") || *arg == '*') {
		file_to_lexistring(WIZLIST_FILE, wizlist);
		file_to_lexistring(IMMLIST_FILE, immlist);
		file_to_lexistring(NEWS_FILE, news);
		file_to_lexistring(CREDITS_FILE, credits);
		file_to_lexistring(MOTD_FILE, motd);
		file_to_lexistring(IMOTD_FILE, imotd);
		file_to_lexistring(HELP_PAGE_FILE, help);
		file_to_lexistring(INFO_FILE, info);
		file_to_lexistring(POLICIES_FILE, policies);
		file_to_lexistring(HANDBOOK_FILE, handbook);
		file_to_lexistring(BACKGROUND_FILE, background);
		file_to_lexistring(LOGIN_FILE, login);
		file_to_lexistring(WELCOME_FILE, welcomeMsg);
		file_to_lexistring(START_FILE, startMsg);
	} else if (!str_cmp(arg, "wizlist"))
		file_to_lexistring(WIZLIST_FILE, wizlist);
	else if (!str_cmp(arg, "immlist"))
		file_to_lexistring(IMMLIST_FILE, immlist);
	else if (!str_cmp(arg, "news"))
		file_to_lexistring(NEWS_FILE, news);
	else if (!str_cmp(arg, "credits"))
		file_to_lexistring(CREDITS_FILE, credits);
	else if (!str_cmp(arg, "motd"))
		file_to_lexistring(MOTD_FILE, motd);
	else if (!str_cmp(arg, "imotd"))
		file_to_lexistring(IMOTD_FILE, imotd);
	else if (!str_cmp(arg, "help"))
		file_to_lexistring(HELP_PAGE_FILE, help);
	else if (!str_cmp(arg, "info"))
		file_to_lexistring(INFO_FILE, info);
	else if (!str_cmp(arg, "policy"))
		file_to_lexistring(POLICIES_FILE, policies);
	else if (!str_cmp(arg, "handbook"))
		file_to_lexistring(HANDBOOK_FILE, handbook);
	else if (!str_cmp(arg, "background"))
		file_to_lexistring(BACKGROUND_FILE, background);
	else if (!str_cmp(arg, "login"))
		file_to_lexistring(LOGIN_FILE, login);
	else if (!str_cmp(arg, "welcome"))
		file_to_lexistring(WELCOME_FILE, welcomeMsg);
	else if (!str_cmp(arg, "start"))
		file_to_lexistring(START_FILE, startMsg);
	else if (!str_cmp(arg, "plrindex"))
		build_player_index();
	else if (!str_cmp(arg, "xhelp"))
	{
		HelpManager::Instance()->Load();
	} else {
		send_to_char("Unknown reload option.\n", ch);
		return;
	}

	send_to_char(OK, ch);
}

extern void LoadPolls(void);

  

/* body of the booting system */
void boot_db(void)
{
	log("Boot db -- BEGIN.");

	log("Resetting the game time:");
	reset_time();

	log("Reading news, credits, help, bground, info & motds.");
	file_to_lexistring(NEWS_FILE, news);
	file_to_lexistring(CREDITS_FILE, credits);
	file_to_lexistring(MOTD_FILE, motd);
	file_to_lexistring(IMOTD_FILE, imotd);
	file_to_lexistring(HELP_PAGE_FILE, help);
	file_to_lexistring(INFO_FILE, info);
	file_to_lexistring(WIZLIST_FILE, wizlist);
	file_to_lexistring(IMMLIST_FILE, immlist);
	file_to_lexistring(POLICIES_FILE, policies);
	file_to_lexistring(HANDBOOK_FILE, handbook);
	file_to_lexistring(BACKGROUND_FILE, background);
	file_to_lexistring(LOGIN_FILE, login);
	file_to_lexistring(WELCOME_FILE, welcomeMsg);
	file_to_lexistring(START_FILE, startMsg);

	log("Generating player index.");
	build_player_index();

	log("Loading help entries.");
	HelpManager::Instance()->Load();

	log("Loading fight messages.");
	load_messages();

	log("Loading social messages.");
	boot_social_messages();
	create_command_list(); /* aedit patch -- M. Scott */
  
	log("Loading behavior sets.");
	BehaviorSet::Load();
	
	log("Loading clans.");
	Clan::Load();
	
	LoadWorldDatabase();
	
	log("Assigning function pointers:");

	if (!no_specials) {
		log("   Mobiles.");
		assign_mobiles();
		log("   Shopkeepers.");
		assign_the_shopkeepers();
		log("   Objects.");
		assign_objects();
		log("   Rooms.");
		assign_rooms();
	}
	
	log("Sorting command list and skills.");
	sort_commands();
	sort_skills();

	log("Booting mail system.");
	if (!scan_file()) {
		log("    Mail boot failed -- Mail system disabled");
		no_mail = true;
	}
	log("Reading banned site and invalid-name list.");
	Ban::Load();

	log("Booting houses.");
	House::Load();
	
	log("Loading persistent objects.");
	LoadPersistentObjects();

	FOREACH(ZoneMap, zone_table, z)
	{
		log("Resetting %s [%s]", (*z)->GetName(), (*z)->GetTag());
		reset_zone(*z);
	}
	
//	log("Tallying hive data.");
//	tally_hive_data();
	
	boot_time = time(0);

	log("Boot db -- DONE.");
}


/* reset the time in the game from file */
void reset_time(void)
{
	struct TimeInfoData mud_time_passed(time_t t2, time_t t1);

	time_info = mud_time_passed(time(0), /*beginning_of_time*/ 1081752604);	//	~1 am 4/12/04

	time_info.year += 2300;

	log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours, time_info.day, time_info.month, time_info.year);
}


/* new version to build the player index for the ascii pfiles */
/* generate index table for the player file */
void build_player_index(void)
{
	FILE *plr_index = fopen(PLR_INDEX_FILE, "r");
	
	if(!plr_index)
	{
		if (errno != ENOENT)
		{
			perror("SYSERR: Error opening player index file pfiles/plr_index");
			system("touch ../.killscript");
			exit(1);
		}
		else
		{
			log("No player index file. Creating a new one.");
			touch(PLR_INDEX_FILE);
			if (!(plr_index = fopen(PLR_INDEX_FILE, "r")))
			{
				perror("SYSERR: Error opening player index file pfiles/plr_index");
				system("touch ../.killscript");
				exit(1);
			}
		}
	}
	
	BUFFER(line, MAX_INPUT_LENGTH);
	BUFFER(arg1, 40);
	BUFFER(arg2, 80);

	player_table.clear();
	while(get_line(plr_index, line))
	{
		two_arguments(line, arg1, arg2);
		
		IDNum id = atoi(arg1);
		
		player_table[id] = CAP(arg2);
		
		top_idnum = MAX(top_idnum, id);
	}

	log("   %d players in database.", player_table.size());
	
	fclose(plr_index);
}


/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE * fl) {
	BUFFER(buf, 128);
	int count = 0;

	while (fgets(buf, 128, fl))
		if (*buf == '#')
			count++;

	return count;
}


void LoadZoneList(Lexi::StringList &zonelist)
{
	Lexi::BufferedFile	file(ZONE_LIST);
	
	if (file.fail())
	{
		log("SYSERR: Error opening index file '%s'", file.GetFilename());
		tar_restore_file(file.GetFilename());
	}
	
	BUFFER(zonename, MAX_STRING_LENGTH);
	while (!file.eof())
	{
		const char *line = file.getline();

		one_argument(line, zonename);
		if (*zonename && *zonename != '#')	zonelist.push_back(zonename);
	}
}


void LoadDatabase(BootMode mode, const Lexi::StringList &zonelist)
{
	const char *	prefix;
	const char *	suffix;
	
	switch (mode)
	{
		case DB_BOOT_ZON_RESETS:
		case DB_BOOT_ZON:	prefix = ZON_PREFIX;	suffix = ZON_SUFFIX;	break;
		case DB_BOOT_WLD:	prefix = WLD_PREFIX;	suffix = WLD_SUFFIX;	break;
		case DB_BOOT_MOB:	prefix = MOB_PREFIX;	suffix = MOB_SUFFIX;	break;
		case DB_BOOT_OBJ:	prefix = OBJ_PREFIX;	suffix = OBJ_SUFFIX;	break;
		case DB_BOOT_SHP:	prefix = SHP_PREFIX;	suffix = SHP_SUFFIX;	break;
		case DB_BOOT_TRG:	prefix = TRG_PREFIX;	suffix = TRG_SUFFIX;	break;
		default:
			log("SYSERR: Unknown LoadDatabase type %d!", mode);
			exit(1);
			break;
	}
	
	
	FOREACH_CONST(Lexi::StringList, zonelist, zonename)
	{
		Lexi::String	filename = Lexi::Format("%s%s%s", prefix, zonename->c_str(), suffix);
		
		if (mode == DB_BOOT_SHP)
		{
			FILE *db_file = fopen(filename.c_str(), "r");
			if (db_file)
			{
				boot_the_shops(db_file, filename.c_str());
				fclose(db_file);
			}
		}
		else if (mode == DB_BOOT_ZON)
		{
			LoadZoneFile(zonename->c_str(), filename.c_str());
		}
		else if (mode == DB_BOOT_ZON_RESETS)
		{
			LoadZoneResets(zonename->c_str(), filename.c_str());
		}
		else
		{
			ZoneData *zone = zone_table.Find(zonename->c_str());
			if (!zone)
			{
				log("Unable to find zone '%s' when loading %s.", zonename->c_str(), GetEnumName(mode));
				exit(0);
			}
			LoadDatabaseFile(filename.c_str(), zone->GetHash(), mode);
		}
	}
}


static bool ZoneListSortFunc(const Lexi::String &lhs, const Lexi::String &rhs)
{
	return atoi(lhs.c_str()) < atoi(rhs.c_str());	//	Until we switch to VID and Tag
//	return GetStringFastHash(lhs.c_str()) < GetStringFastHash(rhs.c_str());
}


void LoadWorldDatabase(void)
{
//	New bootup procedure:
//		Load list of zone filenames:	world/zones
//			Requires: List of zone filenames
//				Requires: zone filename list export
//			Requires: Store zone's filename in zone data
//		Load zones						world/zon/<zonefilename>.zon
//		Load rooms						world/wld/<zonefilename>.wld
//		Renumber world
//		Check start rooms
//		Load clans (Needed for mobs)
//		Load mobs						world/mob/<zonefilename>.mob
//		Load objects					world/obj/<zonefilename>.obj
//		Load scripts					world/trg/<zonefilename>.trg
//		Load shops - TO BE REMOVED		world/shp/<zonefilename>.shp
//		Renumber zones
//		Load polls

	Lexi::StringList	zonelist;
	
	Lexi::InputFile		file(ZONE_LIST);
	if (file.fail())
	{
		//	Old boot system!
		tar_restore_file(ZONE_LIST);
	}
	
	log("Loading zone list.");
	LoadZoneList(zonelist);
	zonelist.sort(ZoneListSortFunc);
		
	log("Loading zones.");
	LoadDatabase(DB_BOOT_ZON, zonelist);
	
	log("Loading rooms.");
	LoadDatabase(DB_BOOT_WLD, zonelist);
	
	log("Renumbering rooms.");
	renum_world();
	
	log("Checking start rooms.");
	check_start_rooms();
	
	log("Loading mobs.");
	LoadDatabase(DB_BOOT_MOB, zonelist);

	log("Loading objs.");
	LoadDatabase(DB_BOOT_OBJ, zonelist);

	log("Loading trigs.");
	LoadDatabase(DB_BOOT_TRG, zonelist);
	LoadGlobalTriggers();
	
	log("Loading shops.");
	LoadDatabase(DB_BOOT_SHP, zonelist);
	
	log("Loading zone resets.");
	LoadDatabase(DB_BOOT_ZON_RESETS, zonelist);

	log("Loading polls.");
	LoadPolls();
}


void LoadZoneFile(const char *zonename, const char *filename)
{
	Lexi::BufferedFile	file(filename);
	
	if (file.fail())
	{
		log("Zone file %s missing.", filename);
		tar_restore_file(filename);
		exit(1);
	}
	
	Lexi::Parser	parser;
	parser.Parse(file);
	
	ZoneData::Parse(parser, zonename);
}


void LoadZoneResets(const char *zonename, const char *filename)
{
	Lexi::BufferedFile	file(filename);
	
	if (file.fail())
	{
		log("Zone file %s missing.", filename);
		tar_restore_file(filename);
		exit(1);
	}
	
	Lexi::Parser	parser;
	parser.Parse(file);
	
	ZoneData::ParseResets(parser, zonename);
}


void LoadDatabaseFile(const char *filename, Hash zone, BootMode mode)
{
	Lexi::BufferedFile	file(filename);
	
	if (file.fail())
	{
		return;
	}
	
	Lexi::Parser	parser;
	parser.Parse(file);
	
	int numEntities = parser.NumSections();
	for (int i = 0; i < numEntities; ++i)
	{
		PARSER_SET_ROOT_INDEXED(parser, "", i);
		
		VirtualID	vid = parser.GetVirtualID("Virtual", zone);
		
		if (!vid.IsValid())
		{
			//	SERIOUS ERROR
			log("Serious error - invalid VID '%s'\n", parser.GetString("Virtual"));
			exit(0);
		}

		switch (mode)
		{
			case DB_BOOT_WLD:	RoomData::Parse(parser, vid);	break;
			case DB_BOOT_MOB:	CharData::Parse(parser, vid);	break;
			case DB_BOOT_OBJ:	ObjData::Parse(parser, vid);	break;
			case DB_BOOT_TRG:	TrigData::Parse(parser, vid);	break;
		}
	}
}


/*************************************************************************
*  procedures for resetting, both play-time and boot-time	 	 *
*********************************************************************** */



int vnum_mobile(char *searchname, CharData * ch, ZoneData *zone)
{
	CharacterPrototypeMap::iterator iter, end;
	
	if (zone)
	{
		iter = mob_index.lower_bound(zone->GetHash());
		end = mob_index.upper_bound(zone->GetHash());
	}
	else
	{
		iter = mob_index.begin();
		end = mob_index.end();
	}
	
	int found = 0;
	for (; iter != end; ++iter)
	{
		CharacterPrototype *mob = *iter;
	
		if (isname(searchname, mob->m_pProto->GetAlias()))
		{
			ch->Send("%3d. [%10s] %s\n", ++found, mob->GetVirtualID().Print().c_str(), mob->m_pProto->GetName());
		}
	}
	return (found);
}



int vnum_object(char *searchname, CharData * ch, ZoneData *zone) {
	ObjectPrototypeMap::iterator iter, end;
	
	if (zone)
	{
		iter = obj_index.lower_bound(zone->GetHash());
		end = obj_index.upper_bound(zone->GetHash());
	}
	else
	{
		iter = obj_index.begin();
		end = obj_index.end();
	}
	
	int found = 0;
	for (; iter != end; ++iter)
	{
		ObjectPrototype *obj = *iter;
		
		if (isname(searchname, obj->m_pProto->GetAlias()))
		{
			ch->Send("%3d. [%10s] %s\n", ++found, obj->GetVirtualID().Print().c_str(), obj->m_pProto->GetName());
		}
	}
	return (found);
}


int vnum_trigger(char *searchname, CharData * ch, ZoneData *zone)
{
	ScriptPrototypeMap::iterator iter, end;
	
	if (zone)
	{
		iter = trig_index.lower_bound(zone->GetHash());
		end = trig_index.upper_bound(zone->GetHash());
	}
	else
	{
		iter = trig_index.begin();
		end = trig_index.end();
	}
	
	int found = 0;
	for (; iter != end; ++iter)
	{
		ScriptPrototype *script = *iter;
		
		if (isname(searchname, script->m_pProto->GetName()))
		{
			ch->Send("%3d. [%10s] %s\n", ++found, script->GetVirtualID().Print().c_str(),
					script->m_pProto->GetName());
		}
	}
	return (found);
}


#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
std::list<ZoneData *>	reset_queue;	/* queue of zones to be reset	 */

void zone_update(void)
{
	static int timer = 0;

	/* jelson 10/22/92 */
	if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60)
	{
		/* one minute has passed */
		/*
		 * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
		 * factor of 60
		 */

		timer = 0;

		/* since one minute has passed, increment zone ages */
		FOREACH(ZoneMap, zone_table, z)
		{
			ZoneData *zone = *z;
			
			if (zone->age < zone->lifespan && zone->m_ResetMode != ZoneData::ResetMode_Never)
				++zone->age;

			if (zone->age >= zone->lifespan &&
					zone->age < ZO_DEAD && zone->m_ResetMode != ZoneData::ResetMode_Never)
			{
				/* enqueue zone */

				reset_queue.push_back(zone);

				zone->age = ZO_DEAD;
			}
		}
	}	/* end - one minute has passed */


	/* dequeue zones (if possible) and reset */
	/* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
	const int MAX_RESETS_PER_LOOP = 3;
	int numResets = 0;
	
	for (std::list<ZoneData *>::iterator iter = reset_queue.begin(), end = reset_queue.end();
		iter != end && numResets < MAX_RESETS_PER_LOOP; )
	{
		ZoneData *zone = *iter;
		
		if (zone->m_ResetMode == ZoneData::ResetMode_Always || !zone->ArePlayersPresent())
		{
			reset_zone(zone);
			mudlogf(CMP, LVL_STAFF, FALSE, "Auto zone reset: [%s] %s", zone->GetTag(), zone->GetName());			
			iter = reset_queue.erase(iter);
			
			++numResets;	//	use this to allow X resets every second
		}
		else
			++iter;
	}
}

static void log_zone_error(ZoneData *zone, VirtualID destination, const ResetCommand &ZCMD, const char *msg)
{
	mudlogf(NRM, LVL_STAFF, FALSE,	"ZONEERR: %s", msg);
	mudlogf(NRM, LVL_STAFF, FALSE,	"ZONEERR: ...offending cmd #%d in '%s'",
			ZCMD.line, destination.IsValid() ? destination.Print().c_str() : zone->GetTag());
}

#define ZONE_ERROR(...) { log_zone_error(zone, destination ? destination->GetVirtualID() : VirtualID(), ZCMD, Lexi::Format(__VA_ARGS__).c_str()); bLastCommandSucceeded = false; continue; }

/* execute the reset command table of a given zone */
void reset_zone(ZoneData *zone) {
	CharData *mob = NULL;
	ObjData *obj = NULL;

#if defined( NEW_ZRESET_TYPES )
	IReset::ClearVariables();
#endif

	bool bLastCommandSucceeded = false;
	
	RoomData *destination = NULL;
	
	for (ResetCommandList::iterator iter = zone->cmd.begin(), end = zone->cmd.end(); iter != end; ++iter)
	{
#if defined( NEW_ZRESET_TYPES )
		bLastCommandSucceeded = (*iter)->Execute(bLastCommandSucceeded);
#else
		ResetCommand &ZCMD = *iter;

		if ((ZCMD.if_flag || (ZCMD.command == 'C')) && !bLastCommandSucceeded)
			continue;
		
		//	0 = do always, 1 = do when hived, 2 = do when not hived
		if (ZCMD.repeat <= 0)
			ZCMD.repeat = 1;

		bLastCommandSucceeded = true;
		
		for (int repeat_no = 0; bLastCommandSucceeded && repeat_no < ZCMD.repeat; repeat_no++)
		{
			switch (ZCMD.command)
			{
				case '*':			/* ignore command */
					bLastCommandSucceeded = false;
					break;
					
				case 'M':			/* read a mobile */
					destination = ZCMD.room;
						
					if (!ZCMD.mob)		ZONE_ERROR("load mob: unknown mob")
					else if (ZCMD.room
						&& ((ZCMD.maxInGame <= 0) || (ZCMD.mob->m_Count < ZCMD.maxInGame))
						&& ((ZCMD.maxInZone <= 0) || (CountSpawnMobsInList(ZCMD.mob, ZCMD.room->GetZone()->m_Characters) < ZCMD.maxInZone))
						&& ((ZCMD.maxInRoom <= 0) || (CountSpawnMobsInList(ZCMD.mob, ZCMD.room->people) < ZCMD.maxInRoom)))
					{
						if ((ZCMD.hive & 1) && !ROOM_FLAGGED(ZCMD.room, ROOM_HIVED))
							;	//	Can't do, not hived
						else if ((ZCMD.hive & 2) && ROOM_FLAGGED(ZCMD.room, ROOM_HIVED))
							;	//	Can't do, hived
						else
						{
							mob = CharData::Create(ZCMD.mob);
							mob->ToRoom(ZCMD.room);
							finish_load_mob(mob);
						}
					}
					else
						bLastCommandSucceeded = false;
					break;

				case 'O':			/* read an object */
					destination = ZCMD.room;
						
					if (!ZCMD.obj)		ZONE_ERROR("load object: unknown object")
					else if (ZCMD.room
						&& ((ZCMD.maxInGame <= 0) || (ZCMD.obj->m_Count < ZCMD.maxInGame))
						&& ((ZCMD.maxInRoom <= 0) || (CountSpawnObjectsInList(ZCMD.obj, ZCMD.room->contents) < ZCMD.maxInRoom)))
					{
						if ((ZCMD.hive & 1) && !ROOM_FLAGGED(ZCMD.room, ROOM_HIVED))
							;	//	Can't do, not hived
						else if ((ZCMD.hive & 2) && ROOM_FLAGGED(ZCMD.room, ROOM_HIVED))
							;	//	Can't do, hived
						else
						{
							obj = ObjData::Create(ZCMD.obj);
							obj->ToRoom(ZCMD.room);
							load_otrigger(obj);
						}
					} else
						bLastCommandSucceeded = false;
					break;

				case 'P':			/* object to object */
					if (!ZCMD.obj)		ZONE_ERROR("put object: unknown object")
					else if (!ZCMD.container)	ZONE_ERROR("put object: put '%s' into unknown container", ZCMD.obj->GetVirtualID().Print().c_str())
					else if ((ZCMD.maxInGame <= 0) || (ZCMD.obj->m_Count < ZCMD.maxInGame))
					{
						ObjData *objTo = get_obj_num(ZCMD.container->GetVirtualID());
						if (!objTo)		ZONE_ERROR("put object: no container '%s' found to put '%s' into", ZCMD.container->GetVirtualID().Print().c_str(), ZCMD.obj->GetVirtualID().Print().c_str())
						obj = ObjData::Create(ZCMD.obj);
						obj->ToObject(objTo);
						load_otrigger(obj);
					} else
						bLastCommandSucceeded = false;
					break;

				case 'G':			/* obj_to_char */
					if (!ZCMD.obj)	ZONE_ERROR("give object: unknown object")
					else if (!mob)	ZONE_ERROR("give object: attempt to give '%s' to non-existant mob", ZCMD.obj->GetVirtualID().Print().c_str())
					else if ((ZCMD.maxInGame <= 0) || (ZCMD.obj->m_Count < ZCMD.maxInGame))
					{
						obj = ObjData::Create(ZCMD.obj);
						obj->ToChar(mob);
						load_otrigger(obj);
					}
					else
						bLastCommandSucceeded = false;
					break;

				case 'E':			/* object to equipment list */
					if (!ZCMD.obj)	ZONE_ERROR("equip object: unknown object")
					else if (!mob)	ZONE_ERROR("equip object: attempt to equip object '%s' to non-existant previous mob", ZCMD.obj->GetVirtualID().Print().c_str())
					else if ((ZCMD.maxInGame <= 0) || (ZCMD.obj->m_Count < ZCMD.maxInGame))
					{
						if (ZCMD.equipLocation < 0 || ZCMD.equipLocation >= NUM_WEARS)
						{
							switch (ZCMD.equipLocation)
							{
								case POS_WIELD_TWO:
								case POS_HOLD_TWO:
								case POS_WIELD:
									if (!mob->equipment[WEAR_HAND_R])
									{
										obj = ObjData::Create(ZCMD.obj);
										equip_char(mob, obj, WEAR_HAND_R);
										load_otrigger(obj);
										if (!PURGED(obj) && !PURGED(mob))	wear_otrigger(obj, mob, WEAR_HAND_R);
									}
									break;
								case POS_WIELD_OFF:
								case POS_LIGHT:
								case POS_HOLD:
									if (!mob->equipment[WEAR_HAND_L]) {
										obj = ObjData::Create(ZCMD.obj);
										equip_char(mob, obj, WEAR_HAND_L);
										load_otrigger(obj);
										if (!PURGED(obj) && !PURGED(mob))	wear_otrigger(obj, mob, WEAR_HAND_L);
									}
									break;
								default:
									ZONE_ERROR("equip object: invalid equipment position for '%s'", ZCMD.obj->GetVirtualID().Print().c_str())
							}
						}
						else if (!mob->equipment[ZCMD.equipLocation])
						{
							obj = ObjData::Create(ZCMD.obj);
							equip_char(mob, obj, ZCMD.equipLocation);
							load_otrigger(obj);
							if (!PURGED(obj) && !PURGED(mob))	wear_otrigger(obj, mob, WEAR_HAND_L);
						}
					}
					else
						bLastCommandSucceeded = false;
					break;

				case 'R': /* rem obj from room */
					destination = ZCMD.room;
						
					if (!ZCMD.obj)	ZONE_ERROR("remove object: unknown object")
					else if (!ZCMD.room)
						;	//	Remove What?
					if ((ZCMD.hive & 1) && !ROOM_FLAGGED(ZCMD.room, ROOM_HIVED))
						;	//	Can't do, not hived
					else if ((ZCMD.hive & 2) && ROOM_FLAGGED(ZCMD.room, ROOM_HIVED))
						;	//	Can't do, hived
					else
					{
						obj = get_obj_in_list_num(ZCMD.obj->GetVirtualID(), ZCMD.room->contents);
						if (obj)
						{
							obj->FromRoom();
							obj->Extract();
							obj = NULL;
						}
					}
					break;
					
				case 'C':			/* command mob to do something */
					if (!mob)								ZONE_ERROR("trying to command non-existant previous mob: %s", ZCMD.command_string.c_str())
					else if (ZCMD.command_string.empty())	ZONE_ERROR("trying to command mob with no command")
					else
						command_interpreter(mob, ZCMD.command_string.c_str());
					break;
					
				case 'D':			/* set state of door */
					destination = ZCMD.room;
						
					if (!ZCMD.room || ZCMD.direction >= NUM_OF_DIRS || !ZCMD.room->GetExit(ZCMD.direction))
						ZONE_ERROR("door: exit %s does not exist at %s", GetEnumName(ZCMD.direction), ZCMD.room ? ZCMD.room->GetVirtualID().Print().c_str() : "<NOWHERE>")
					else if ((ZCMD.hive & 1) && !ROOM_FLAGGED(ZCMD.room, ROOM_HIVED))
							;	//	Can't do, not hived
					else if ((ZCMD.hive & 2) && ROOM_FLAGGED(ZCMD.room, ROOM_HIVED))
							;	//	Can't do, hived
					else
					{
						RoomExit *exit = ZCMD.room->GetExit(ZCMD.direction);
						
						switch (ZCMD.doorState)
						{
							case ResetCommand::DOOR_OPEN:
								exit->GetStates().clear(EX_STATE_CLOSED)
												 .clear(EX_STATE_LOCKED)
												 .clear(EX_STATE_IMPASSABLE)
												 .clear(EX_STATE_BLOCKED)
												 .clear(EX_STATE_JAMMED);
								break;
							case ResetCommand::DOOR_CLOSED:
								if(!EXIT_FLAGGED(exit, EX_ISDOOR))
									break;
								exit->GetStates().set(EX_STATE_CLOSED)
												 .clear(EX_STATE_LOCKED)
												 .clear(EX_STATE_BREACHED);
								break;
							case ResetCommand::DOOR_LOCKED:
								if(!EXIT_FLAGGED(exit, EX_ISDOOR))
									break;
								exit->GetStates().set(EX_STATE_LOCKED)
												 .set(EX_STATE_CLOSED)
												 .clear(EX_STATE_BREACHED);
								break;
							case ResetCommand::DOOR_IMPASSABLE:
								//exit->GetStates().clear(EX_STATE_LOCKED);
								//exit->GetStates().clear(EX_STATE_CLOSED);
								exit->GetStates().set(EX_STATE_IMPASSABLE);
								break;
							case ResetCommand::DOOR_BLOCKED:
								//exit->GetStates().clear(EX_STATE_LOCKED);
								//exit->GetStates().clear(EX_STATE_CLOSED);
								exit->GetStates().set(EX_STATE_BLOCKED);
								break;
							case ResetCommand::DOOR_ENABLED:
								exit->GetStates().clear(EX_STATE_DISABLED);
								break;
							case ResetCommand::DOOR_DISABLED:
								exit->GetStates().set(EX_STATE_DISABLED);
								break;
							case ResetCommand::DOOR_BREACHED:
								if(!EXIT_FLAGGED(exit, EX_ISDOOR))
									break;
								exit->GetStates().clear(EX_STATE_CLOSED)
												 .set(EX_STATE_BREACHED);
								break;
							case ResetCommand::DOOR_JAMMED:
								if(!EXIT_FLAGGED(exit, EX_ISDOOR))
									break;
								//exit->GetStates().set(EX_STATE_CLOSED);
								exit->GetStates().set(EX_STATE_JAMMED);
								break;
							case ResetCommand::DOOR_REPAIRED:
								if(!EXIT_FLAGGED(exit, EX_ISDOOR))
									break;
								exit->GetStates().clear(EX_STATE_JAMMED)
												 .clear(EX_STATE_BREACHED);
								break;
							case ResetCommand::DOOR_HIDDEN:
								exit->GetStates().set(EX_STATE_HIDDEN);
								break;
						}
					}
					break;
					
				case 'T':
					if (!ZCMD.script)	ZONE_ERROR("trigger: unknown script")
					else
					{
						Scriptable *sc = NULL;
						
						if (ZCMD.maxInGame > 0 && ZCMD.script->m_Count >= ZCMD.maxInGame)
							break;
						
						switch (ZCMD.triggerType)
						{
							case MOB_TRIGGER:
								if (!mob)	ZONE_ERROR("trigger: attempt to assign '%s' to non-existant previous mob", ZCMD.script->GetVirtualID().Print().c_str())
								sc = SCRIPT(mob);
								break;
								
							case OBJ_TRIGGER:
								if (!obj)	ZONE_ERROR("trigger: attempt to assign '%s' to non-existant previous object", ZCMD.script->GetVirtualID().Print().c_str())
								sc = SCRIPT(obj);
								break;
								
							case WLD_TRIGGER:
								destination = ZCMD.room;
								
								if (ZCMD.room == NULL)
									ZONE_ERROR("trigger: attempt to assign '%s' to non-existant room", ZCMD.script->GetVirtualID().Print().c_str())
								
								if ((ZCMD.hive & 1) && !ROOM_FLAGGED(ZCMD.room, ROOM_HIVED))
									continue;	//	Can't do, not hived
								else if ((ZCMD.hive & 2) && ROOM_FLAGGED(ZCMD.room, ROOM_HIVED))
									continue;	//	Can't do, hived
									
								sc = SCRIPT(ZCMD.room);
								break;
								
							default:
								ZONE_ERROR("trigger: unknown type for '%s'", ZCMD.script->GetVirtualID().Print().c_str())
								ZCMD.command = '*';
								continue;
						}
						
/*						bool found = false;
						
						FOREACH(TrigData::List, TRIGGERS(sc), iter)
						{
							if ((*iter)->GetPrototype() == ZCMD.script) {
								found = true;
								break;
							}
						}
*/						
						if (!sc->HasTrigger(ZCMD.script->GetVirtualID()))
						{
							TrigData *trig = TrigData::Create(ZCMD.script);
							sc->AddTrigger(trig);
						}
					}
					break;
					
				default:
					ZONE_ERROR("unknown cmd in reset table; cmd disabled")
					ZCMD.command = '*';
					break;
			}
		}
#endif
	}
	
	FOREACH_BOUNDED(RoomMap, world, zone->GetHash(), room)
	{
		/*if (*room)*/	reset_wtrigger(*room);
	}
	
	zone->age = 0;
}



/*************************************************************************
*  stuff related to the save/load player system				 *
*********************************************************************** */


IDNum get_id_by_name(const char *name) {
	BUFFER(arg, MAX_INPUT_LENGTH);

	any_one_arg(name, arg);
	
	FOREACH(PlayerTable, player_table, iter)
	{
		if (iter->second == arg)
			return iter->first;
	}
	
	return 0;
}


const char *get_name_by_id(IDNum id, const char *def)
{
	PlayerTable::iterator iter = player_table.find(id);
	
	if (iter != player_table.end())
		return iter->second.c_str();
	
	return def;
}


/************************************************************************
*  funcs of a (more or less) general utility nature			*
********************************************************************** */


/* release memory allocated for a char struct */
#define READ_SIZE 1024

void file_to_lexistring(char *name, Lexi::String &str)
{
	Lexi::BufferedFile	file(name);
	str = file.getall();
}



/* initialize a new character only if class is set */
void init_char(CharData * ch) {
	IDNum newId = get_id_by_name(ch->GetName());
	
	if (newId == 0)
	{
		newId = ++top_idnum;
	
//		BUFFER(buf, MAX_STRING_LENGTH);
//		strcpy(buf, ch->GetName());
//		Lexi::tolower(buf);
//		CAP(buf);

		player_table[newId] = ch->GetName();
	}

	ch->SetID(newId);
	
	//	create a player_special structure
	if (ch->GetPlayer() == NULL)
		ch->m_pPlayer = new PlayerData;

	//	if this is our first player --- he be God
	if (newId == 1)
	{
		ch->m_Level = LVL_CODER;

		ch->points.max_hit = 5000;
		ch->points.max_move = 800;
		SET_BIT(STF_FLAGS(ch), 0xFFFFFFFF);
	}
	ch->SetTitle(NULL);

	ch->m_Keywords = "";
	ch->m_RoomDescription = "";
	ch->m_Description = "";

	ch->GetPlayer()->m_Creation = time(0);
	ch->GetPlayer()->m_TotalTimePlayed = 0;
	ch->GetPlayer()->m_LoginTime = ch->GetPlayer()->m_LastLogin = time(0);
	
	SET_BIT(CHN_FLAGS(ch), Channel::NoBitch);

	/* make favors for sex */
	if (ch->GetSex() == SEX_MALE) {
		ch->m_Height = Number(66, 78);
		ch->m_Weight = Number(180, 250);
	} else {
		ch->m_Height = Number(60, 72);
		ch->m_Weight = Number(140, 200);
	}

	ch->points.hit = GET_MAX_HIT(ch);
	ch->points.max_move = 100;
	ch->points.move = GET_MAX_MOVE(ch);
	//ch->points.max_endurance = 20;
	//ch->points.endurance = GET_MAX_ENDURANCE(ch);
//	ch->points.armor = 0 /*100*/;
	memset(ch->points.armor, 0, sizeof(ch->points.armor));

//	for (i = 1; i <= MAX_SKILLS; i++) {
//		SET_SKILL(ch, i, !IS_STAFF(ch) ? 0 : 100);
//	}

	ch->m_AffectFlags.clear();

//	ch->real_abils.intel = 100;
//	ch->real_abils.per = 100;
//	ch->real_abils.agi = 100;
//	ch->real_abils.str = 100;
//	ch->real_abils.con = 100;

//	for (i = 0; i < 3; i++)
//		GET_COND(ch, i) = (IS_STAFF(ch) ? -1 : 24);

	ch->GetPlayer()->m_LoadRoom.Clear();
	
//	ch->GetPlayer()->m_Prompt = "`R%h`rhp `G%v`gmv `B%e`ben`K>`n";
	ch->GetPlayer()->m_Prompt = "`R%h`rhp `G%v`gmv>`n";
	
	ch->Save();
				
	save_player_index();

	//	Safety catch - it is possible for a .objs file to exist for the character,
	//	and if it does, they will load their objects on entering game
	remove(ch->GetObjectFilename().c_str());
}


void vwear_object(int wearpos, CharData * ch) {
	int found = 0;
	
	FOREACH(ObjectPrototypeMap, obj_index, iter)
	{
		ObjectPrototype *	proto = *iter;
		if (CAN_WEAR(proto->m_pProto, wearpos))
			ch->Send("%3d. [%10s] %s\n", ++found, proto->GetVirtualID().Print().c_str(), proto->m_pProto->GetName());
	}
}


void save_player_index(void) {
	FILE *index_file;

	if(!(index_file = fopen(PLR_INDEX_FILE, "w"))) {
		log("SYSERR:  Could not write player index file");
		return;
	}

	FOREACH(PlayerTable, player_table, iter)
		fprintf(index_file, "%u	%s\n", iter->first, iter->second.c_str());

	fclose(index_file);
}
