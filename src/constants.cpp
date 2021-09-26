
/* ************************************************************************
*   File: constants.c                                   Part of CircleMUD *
*  Usage: Numeric and string contants used by the MUD                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */



#include "structs.h"
#include "constants.h"
#include "utils.h"

#include "rooms.h"
#include "objects.h"
#include "affects.h"


char *circlemud_version =
"Aliens vs Predator: The MUD\n"
"Compiled on " __DATE__ " at " __TIME__ "\n";

/* strings corresponding to ordinals/bitvectors in structs.h ***********/


/* (Note: strings for class definitions in class.c instead of here) */

/* cardinal directions */
char *dirs[] = {
	"north",
	"east",
	"south",
	"west",
	"up",
	"down",
	"\n"
};


ENUM_NAME_TABLE(size_t, 1) { { 0, "" } };


ENUM_NAME_TABLE(Direction, NUM_OF_DIRS)
{
	{ NORTH,	"north"	},
	{ EAST,		"east"	},
	{ SOUTH,	"south"	},
	{ WEST,		"west"	},
	{ UP,		"up"	},
	{ DOWN,		"down"	}
};


/* EX_x */
ENUM_NAME_TABLE(ExitFlag, NUM_EXIT_FLAGS)
{
	{ EX_ISDOOR,		"DOOR",			},
	{ EX_PICKPROOF,		"PICKPROOF"		},
	{ EX_HIDDEN,		"HIDDEN"		},
	{ EX_MUTABLE,		"MUTABLE"		},
	{ EX_NOSHOOT,		"NO-SHOOT"		},
	{ EX_NOMOVE,		"NO-MOVE"		},
	{ EX_NOMOB,			"NO-MOB"		},
	{ EX_NOVEHICLES,	"NO-VEHICLE"	},
	{ EX_NOHUMAN,		"NO-HUMAN"		},
	{ EX_NOSYNTHETIC,	"NO-SYNTHETIC"	},
	{ EX_NOPREDATOR,	"NO-PREDATOR"	},
	{ EX_NOALIEN,		"NO-ALIEN"		},
};


ENUM_NAME_TABLE(ExitState, NUM_EXIT_STATES)
{
	{ EX_STATE_CLOSED,	"CLOSED"		},
	{ EX_STATE_LOCKED,	"LOCKED"		},
	{ EX_STATE_IMPASSABLE,	"IMPASSABLE"},
	{ EX_STATE_BLOCKED,	"BLOCKED"	},
	{ EX_STATE_DISABLED,"DISABLED"		},
	{ EX_STATE_BREACHED,"BREACHED"		},
	{ EX_STATE_JAMMED,	"JAMMED"		},
	{ EX_STATE_HIDDEN,	"HIDDEN"		},
};


/* SECT_ */
ENUM_NAME_TABLE(RoomSector, NUM_SECTORS)
{
	{ SECT_INSIDE,		"Inside"		},
	{ SECT_CITY,		"City"			},
	{ SECT_ROAD,		"Road"			},
	{ SECT_FIELD,		"Field"			},
	{ SECT_FOREST,		"Forest"		},
	{ SECT_BEACH,		"Beach"			},
	{ SECT_DESERT,		"Desert"		},
	{ SECT_JUNGLE,		"Jungle"		},
	{ SECT_HILLS,		"Hills"			},
	{ SECT_SNOW,		"Snow"			},
	{ SECT_MOUNTAIN,	"Mountains"		},
	{ SECT_WATER_SWIM,	"Water (Swim)"	},
	{ SECT_WATER_NOSWIM,"Water (No Swim)" },
	{ SECT_UNDERWATER,	"Underwater"	},
	{ SECT_FLYING,		"In Flight"		},
	{ SECT_SPACE,		"In Space"		},
	{ SECT_DEEPSPACE,	"Deep Space"	},
};


/* SEX_x */
ENUM_NAME_TABLE(Sex, NUM_SEXES)
{
	{ SEX_NEUTRAL,		"Neutral"		},
	{ SEX_MALE,			"Male"			},
	{ SEX_FEMALE,		"Female"		}
};


/* POS_x */
ENUM_NAME_TABLE(Position, NUM_POSITIONS)
{
	{ POS_DEAD,			"dead"		},
	{ POS_MORTALLYW,	"mortally wounded" },
	{ POS_INCAP,		"incapacitated" },
	{ POS_STUNNED,		"stunned"	},
	{ POS_SLEEPING,		"sleeping"	},
	{ POS_RESTING,		"resting"	},
	{ POS_SITTING,		"sitting"	},
	{ POS_FIGHTING,		"fighting"	},
	{ POS_STANDING,		"standing"	}
};


/* PLR_x */
char *player_bits[] = {
  "PKONLY",				/* 0 */
  "Unused1",
  "FROZEN",
  "DONTSET",
  "WRITING",
  "MAILING",			/* 5 */
  "CSH",
  "SITEOK",
  "NOSHOUT",
  "NOTITLE",
  "DELETED",			/* 10 */
  "LOADRM",
  "NO-WIZL",
  "NO-DEL",
  "INVST",
  "Unused15",				/* 15 */
  "Unused16",
  "Unused17",
  "Unused18",
  "Unused19",
  "Unused20",				/* 20 */
  "Unused21",
  "Unused22",
  "Unused23",
  "Unused24",
  "Unused25",				/* 25 */
  "Unused26",
  "Unused27",
  "Unused28",
  "Unused29",
  "SPINCREASE",				/* 30 */
  "Unused31",
  "\n"
};


/* MOB_x */
char *action_bits[] =
{
  "SPEC",			/* 0 */
  "SENTINEL",
  "SCAVENGER",
  "ISNPC",
  "AWARE",
  "AGGR",			/* 5 */
  "STAY-ZONE",
  "WIMPY",
  "AGGR-ALL",
  "MEMORY",
  "HELPER",			/* 10 */
  "NO-CHARM",
  "NO-SNIPE",
  "NO-SLEEP",
  "NO-BASH",
  "NO-BLIND",			/* 15 */
  "ACIDBLOOD",
  "PROG-MOB",
  "AI",
  "MECHANICAL",
  "NO-MELEE",				/* 20 */
  "HALF-SNIPE",
  "FRIENDLY",
  "Unused23",
  "Unused24",
  "Unused25",				/* 25 */
  "Unused26",
  "Unused27",
  "Unused28",
  "Unused29",
  "Unused30",				/* 30 */
  "DELETED",
  "\n"
};

#if 0
ENUM_NAME_TABLE(MobFlag, NUM_MOB_FLAGS)
{
	{ MOB_SPEC,			"SPEC"		},
	{ MOB_SENTINEL,		"SENTINEL"	},
	{ MOB_SCAVENGER,	"SCAVENGER"	},
	{ MOB_ISNPC,		"ISNPC"		},
	{ MOB_AWARE,		"AWARE"		},
	{ MOB_AGGRESSIVE,	"AGGR"		},
	{ MOB_STAY_ZONE,	"STAY-ZONE"	},
	{ MOB_WIMPY,		"WIMPY"		},
	{ MOB_AGGR_ALL,		"AGGR-ALL"	},
	{ MOB_MEMORY,		"MEMORY"	},
	{ MOB_HELPER,		"HELPER"	},
	{ MOB_NOCHARM,		"NO-CHARM"	},
	{ MOB_NOSNIPE,		"NO-SNIPE"	},
	{ MOB_NOSLEEP,		"NO-SLEEP"	},
	{ MOB_NOBASH,		"NO-BASH"	},
	{ MOB_NOBLIND,		"NO-BLIND"	},
	{ MOB_ACIDBLOOD,	"ACIDBLOOD"	},
	{ MOB_PROGMOB,		"PROG-MOB"	},
	{ MOB_AI,			"AI"		},
	{ MOB_MECHANICAL,	"MECHANICAL"},
	{ MOB_NOMELEE,		"NO-MELEE"	}
	{ MOB_HALFSNIPE,	"HALF-SNIPE"},
	{ MOB_FRIENDLY,		"FRIENDLY"	},
	{ MOB_DELETED,		"DELETED"	},
};
#endif


/* PRF_x */
char *preference_bits[] = {
  "BRIEF",			/* 0 */
  "COMPACT",
  "PK",
  "unused",
  "unused",
  "unused",	/* 5 */
  "NOAUTOFOLLOW",
  "AUTOEX",
  "NO-HASS",
  "unused",
  "unused",			/* 10 */
  "NO-REP",
  "LIGHT",
  "COLOR",
  "unused",
  "unused",			/* 15 */
  "LOG1",
  "LOG2",
  "unused",
  "unused",
  "unused",			/* 20 */
  "RMFLG",
  "unused",
  "unused",
  "unused",
  "AUTOSWITCH",		/* 25 */
  "unused",
  "\n"
};


char *channel_bits[] = {
	"NO-SHOUT",
	"NO-TELL",
	"NO-CHAT",
	"MISSION",
	"NO-MUSIC",
	"NO-GRATZ",
	"NO-INFO",
	"NO-WIZ",
	"NO-RACE",
	"NO-CLAN",
	"NO-BITCH",
	"NO-NEWBIE",
	"\n"
};


/* AFF_x */
ENUM_NAME_TABLE(AffectFlag, NUM_AFF_FLAGS)
{
	{ AFF_BLIND,		"BLIND"			},
	{ AFF_INVISIBLE,	"INVIS"			},
	{ AFF_INVISIBLE_2,	"INVIS-2"		},
	{ AFF_SENSE_LIFE,	"SENSE-LIFE"	},
	{ AFF_DETECT_INVIS,	"DET-INVIS"		},
	{ AFF_SANCTUARY,	"SANCT"			},
	{ AFF_GROUP,		"GROUP"			},
	{ AFF_FLYING,		"FLYING"		},
	{ AFF_INFRAVISION,	"INFRA"			},
	{ AFF_POISON,		"POISON"		},
	{ AFF_SLEEP,		"SLEEP"			},
	{ AFF_NOTRACK,		"NO-TRACK"		},
	{ AFF_SNEAK,		"SNEAK"			},
	{ AFF_HIDE,			"HIDE"			},
	{ AFF_CHARM,		"CHARM"			},
	{ AFF_MOTIONTRACKING,"TRACKING"		},
	{ AFF_SPACESUIT,	"VACUUM-SAFE"	},
	{ AFF_LIGHT,		"LIGHT"			},
	{ AFF_TRAPPED,		"TRAPPED"		},
	{ AFF_STUNNED,		"STUNNED"		},
	{ AFF_ACIDPROOF,	"ACIDPROOF"		},
	{ AFF_BLEEDING,		"BLEEDING"		},
	{ AFF_NORECALL,		"NO-RECALL"		},
	{ AFF_GROUPTANK,	"GROUP-TANK"	},
	{ AFF_COVER,		"COVER"			},
	{ AFF_TRAITOR,		"TRAITOR"		},
	{ AFF_NOAUTOFOLLOW,	"NO-AUTOFOLLOW"	},
	{ AFF_DYING,		"DYING"			},
	{ AFF_DIDFLEE,		"DID-FLEE"		},
	{ AFF_DIDSHOOT,		"DID-SHOOT-TIMER"},
	{ AFF_NOSHOOT,		"NO-SHOOT-TIMER"},
	{ AFF_NOQUIT,		"NO-QUIT-TIMER"	},
	{ AFF_RESPAWNING,	"RESPAWNING"	}
};


/* WEAR_x - for eq list */
char *where[] = {
  "<worn on finger>     ",
  "<worn on finger>     ",
  "<worn around neck>   ",
  "<worn on body>       ",
  "<worn on head>       ",
  "<worn on legs>       ",
  "<worn on feet>       ",
  "<worn on hands>      ",
  "<worn on arms>       ",
  "<worn about body>    ",
  "<worn about waist>   ",
  "<worn around wrist>  ",
  "<worn around wrist>  ",
  "<worn over eyes>     ",
  "<tail>               ",
  "ERROR - PLEASE REPORT",
  "ERROR - PLEASE REPORT",
  "<two-handed weapon>  ",
  "<held two-handed>    ",
  "<weapon>             ",
  "<off-hand weapon>    ",
  "<held as light>      ",
  "<held>               "
};


/* WEAR_x - for stat */
char *equipment_types[] = {
  "Worn on right finger",
  "Worn on left finger",
  "Worn around Neck",
  "Worn on body",
  "Worn on head",
  "Worn on legs",
  "Worn on feet",
  "Worn on hands",
  "Worn on arms",
  "Worn about body",
  "Worn around waist",
  "Worn around right wrist",
  "Worn around left wrist",
  "Worn over eyes",
  "Worn as tail",
  "Right Hand",
  "Left Hand",
  "Two-handed weapon",
  "Held two-handed",
  "Weapon",
  "Of-hand weapon",
  "Held as light",
  "Held",
  "\n"
};


char *eqpos[] = {
  "rightfinger",
  "leftfinger",
  "neck",
  "body",
  "head",
  "legs",
  "feet",
  "hands",
  "arms",
  "aboutbody",
  "waist",
  "rightwrist",
  "leftwrist",
  "eyes",
  "tail",
  "righthand",
  "lefthand",
  "weapon",
  "secondaryweapon",
  "gun",
  "\n"
};

/* ITEM_x (ordinal object types) */
ENUM_NAME_TABLE(ObjectType, NUM_ITEM_TYPES)
{
	{ ITEM_UNDEFINED,		"UNDEFINED"	},
	{ ITEM_LIGHT,			"LIGHT"		},
	{ ITEM_WEAPON,			"WEAPON"	},
	{ ITEM_AMMO,			"AMMO"		},
	{ ITEM_THROW,			"THROW"		},
	{ ITEM_GRENADE,			"GRENADE"	},
	{ ITEM_BOOMERANG,		"BOOMERANG"	},
	{ ITEM_TREASURE,		"TREASURE"	},
	{ ITEM_ARMOR,			"ARMOR"		},
	{ ITEM_OTHER,			"OTHER"		},
	{ ITEM_TRASH,			"TRASH"		},
	{ ITEM_CONTAINER,		"CONTAINER"	},
	{ ITEM_NOTE,			"NOTE"		},
	{ ITEM_KEY,				"KEY"		},
	{ ITEM_PEN,				"PEN"		},
	{ ITEM_BOAT,			"BOAT"		},
	{ ITEM_FOOD,			"FOOD"		},
	{ ITEM_DRINKCON,		"LIQ CONTAINER"	},
	{ ITEM_FOUNTAIN,		"FOUNTAIN"	},
	{ ITEM_VEHICLE,			"VEHICLE"	},
	{ ITEM_VEHICLE_HATCH,	"VEHICLE HATCH" },
	{ ITEM_VEHICLE_CONTROLS,"VEHICLE CONTROLS" },
	{ ITEM_VEHICLE_WINDOW,	"VEHICLE WINDOW" },
	{ ITEM_VEHICLE_WEAPON,	"VEHICLE WEAPON" },
	{ ITEM_BED,				"BED"		},
	{ ITEM_CHAIR,			"CHAIR"		},
	{ ITEM_BOARD,			"BOARD"		},
	{ ITEM_INSTALLABLE,		"INSTALLABLE" },
	{ ITEM_INSTALLED,		"INSTALLED"	},
	{ ITEM_ATTACHMENT,		"ATTACHMENT" },
	{ ITEM_TOKEN,			"TOKEN"		}
};


/* ITEM_WEAR_ (wear bitvector) */
char *wear_bits[] = {
  "TAKE",
  "FINGER",
  "NECK",
  "BODY",
  "HEAD",
  "LEGS",
  "FEET",
  "HANDS",
  "ARMS",
  "SHIELD",
  "ABOUT",
  "WAIST",
  "WRIST",
  "WIELD",
  "HOLD",
  "EYES",
  "TAIL",
  "\n"
};


/* ITEM_x (extra bits) */
char *extra_bits[] = {
  "GLOW",
  "HUM",
  "NO-RENT",
  "NO-DONATE",
  "LOG",
  "INVISIBLE",
  "NO-DROP",
  "AUTOSNIPE",
  "PERSISTENT",
  "NO-STACK",
  "VIEWABLE",					/* 10 */
  "NO-SELL",
  "NO-LOSE",
  "MOVEABLE",
  "MISSION",
  "TWO-HANDS",
  "SPRAY",
  "NO-SHOOT",
  "STAFF-ONLY",
  "SAVE-VARIABLES",
  "DEATH-PURGE",				/* 20 */
  "NO-MELEE",
  "NO-PUT",
  "SPECIAL-MISSION",
  "HALF-MELEE-DAMAGE",
  "LEVEL-BASED",				/* 25 */
  "SCRIPTED-COMBAT",
  "Unused27",
  "Unused28",
  "Unused29",
  "RELOADED",					/* 30 */
  "DELETED",
  "\n"
};


/* ITEM_x (extra bits) */
char *restriction_bits[] = {
  "NO-HUMAN",
  "NO-SYNTHETIC",
  "NO-PREDATOR",
  "NO-ALIEN",
  "\n"
};

char *stat_types[] =
{
	"Strength",
	"Health",
	"Coordination",
	"Agility",
	"Perception",
	"Knowledge",
	"\n"
};


/* APPLY_x */
ENUM_NAME_TABLE(AffectLoc, NUM_APPLY_FLAGS)
{
	{ APPLY_NONE,		"NONE"		},
	{ APPLY_STR,		"STRENGTH"	},
	{ APPLY_HEA,		"HEALTH"	},
	{ APPLY_COO,		"COORDINATION" },
	{ APPLY_AGI,		"AGILITY"	},
	{ APPLY_PER,		"PERCEPTION"},
	{ APPLY_KNO,		"KNOWLEDGE"	},
	{ APPLY_EXTRA_ARMOR,"EXTRA_ARMOR" },
	{ APPLY_PROXY,		"PROXY"		},
	{ APPLY_WEIGHT,		"CHAR_WEIGHT" },
	{ APPLY_HEIGHT,		"CHAR_HEIGHT" },
	{ APPLY_HIT,		"MAXHIT"	},
	{ APPLY_MOVE,		"MAXMOVE"	},
//	{ APPLY_ENDURANCE,	"MAXENDURANCE" }
};


char *ammo_types[] ={
	"M309 10mm x 24",	//	1
	"M250 10mm x 28",
	"M252 10mm x 28",
	"Reserved 10mm",
	"Generic 10mm",		//	5
	
	"M901 9mm",
	"Reserved 9mm #1",
	"Reserved 9mm #2",
	"Reserved 9mm #3",
	"Generic 9mm",		//	10
	
	"Napthal Fuel",
	"Reserved Fuel #1",
	"Reserved Fuel #2",
	
	"12 Guage",
	"20 Guage",			//	15
	"Reserved Shotgun #1",
	
	".22 Caliber",
	".30-06 Caliber",
	".357 Caliber",
	".45 Caliber",		//	20
	".50 Caliber",
	"Reserved Caliber #1",
	"Reserved Caliber #2",
	"Reserved Caliber #3",
	"Reserved Caliber #4",	//	25
	
	"Plasma Energy",
	"Heavy Plasma Energy",
	"Rapid Plasma Energy",
	"Reserved Energy",
	"Speargun Spears",	//	30
	
	"Plasma Arrow",		//	31

	"\n"
};


/* CONT_x */
char *container_bits[] = {
  "CLOSEABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "\n",
};


/* LIQ_x */
char *drinks[] =
{
  "water",
  "\n"
};


/* other constants for liquids ******************************************/


/* one-word alias for each drink */
char *drinknames[] =
{
  "water",
  "\n"
};


/* effect of drinks on hunger, thirst, and drunkenness -- see values.doc */
int drink_aff[][3] = {
  {0, 1, 10},
};


/* color of the various drinks */
char *color_liquid[] =
{
  "clear",
  "\n"
};


/* level of fullness for drink containers */
char *fullness[] =
{
  "less than half ",
  "about half ",
  "more than half ",
  ""
};


int rev_dir[] = {
	SOUTH,
	WEST,
	NORTH,
	EAST,
	DOWN,
	UP
};


int movement_loss[] =
{
  3,	//	Inside
  3,	//	City
  3,	//	Road
  4,	//	Field
  6,	//	Forest
  6,	//	Beach
  6,	//	Desert
  7,	//	Jungle
  7,	//	Hills
  8,	//	Snow
  10,	//	Mountains
  8,	/* Swimming   */
  3,	/* Unswimable */
  8,    /* Underwater */
  1,	/* Flying     */
  3,	/* Space      */
  3,    /* Deep space */
};


int movement_time[] =
{
  100,	//	Inside
  100,	//	City
  100,	//	Road
  125,	//	Field
  150,	//	Forest
  150,	//	Beach
  150,	//	Desert
  175,	//	Jungle
  175,	//	Hills
  200,	//	Snow
  250,	//	Mountains
  300,	/* Swimming   */
  200,	/* Unswimable */
  500,	/* Underwater */
  50,	/* Flying     */
  100,	/* Space      */
  100,    /* Deep space */
};


char *weekdays[] = {
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday"
};


char *month_name[] = {
	"January",		/* 0 */
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",			/* 6 */
	"August",
	"September",
	"October",
	"November",
	"December"		/* 11 */
};


char * level_string[LVL_CODER-LVL_IMMORT+1] = {
	"    Staff    ",
	"    Staff    ",
	"  Sr. Staff  ",
	"    Admin    ",
	" Coder/Admin "
};


char * dir_text_the[] = {
	"the north",
	"the east",
	"the south",
	"the west",
	"above",
	"below",
};

char * dir_text_to_the[] = {
	"to the north",
	"to the east",
	"to the south",
	"to the west",
	"above",
	"below",
};


char * dir_text_to_the_of[] = {
	"to the north of",
	"to the east of",
	"to the south of",
	"to the west of",
	"above",
	"below"
};


char *relation_colors = "nyr";


char *times_string[] = {
	"not at all",					//	0
	"once",							//	1
	"a couple times",				//	2
	"a few times",					//	3
	"several times",				//	4
	"five times",					//	5
	"six times",					//	6
	"seven times",					//	7
	"eight times",					//	8
	"nine times",					//	9
	"ten times",					//	10
	"over and over",				//	11
	"so much you lost count",		//	12
	"\n"
};


ENUM_NAME_TABLE(Relation, NUM_RELATIONS)
{
	{	RELATION_FRIEND,	"friend"	},
	{	RELATION_NEUTRAL,	"neutral"	},
	{	RELATION_ENEMY,		"enemy"		}
};


char *staff_bits[] = {
	"GENERAL",
	"ADMIN",
	"SECURITY",
	"GAME",
	"HOUSES",
	"CHARACTERS",
	"CLANS",
	"OLC",
	"OLCADMIN",
	"SOCIALS",
	"HELP",
	"SHOPS",
	"SCRIPTS",
	"IMC",
	"MINISEC",
	"MINIOLC",
	"SRSTAFF",
	"\n"
};


char *ammo_bits[] = {
	"\n"
};


ENUM_NAME_TABLE_NS(ZoneData, ResetMode, ZoneData::NUM_RESETMODES)
{
	{	ZoneData::ResetMode_Never,	"NEVER"		},
	{	ZoneData::ResetMode_Empty,	"DESERTED"	},
	{	ZoneData::ResetMode_Always,	"ALWAYS"	}
};


char *zonecmd_types[] =
{
	"Mobile",
	"Object",
	"Equip",
	"Give",
	"Put",
	"Door",
	"Remove",
	"Command",
	"Trigger",
	"\n"
};

char *hive_types[] =
{
	"IGNORED",
	"HIVED",
	"NOTHIVED",
	"\n"
};


ENUM_NAME_TABLE_NS(ResetCommand, DoorState, ResetCommand::NUM_DOOR_STATES)
{
	{	ResetCommand::DOOR_OPEN,	"OPEN"		},
	{	ResetCommand::DOOR_CLOSED,	"CLOSED"	},
	{	ResetCommand::DOOR_LOCKED,	"LOCKED"	},
	{	ResetCommand::DOOR_IMPASSABLE,"IMPASSABLE"	},
	{	ResetCommand::DOOR_BLOCKED,	"BLOCKED"	},
	{	ResetCommand::DOOR_ENABLED,	"ENABLED"	},
	{	ResetCommand::DOOR_DISABLED,"DISABLED"	},
	{	ResetCommand::DOOR_BREACHED,"BREACHED"	},
	{	ResetCommand::DOOR_JAMMED,	"JAMMED"	},
	{	ResetCommand::DOOR_REPAIRED,"REPAIRED"	},
	{	ResetCommand::DOOR_HIDDEN,	"HIDDEN"	},
};


ENUM_NAME_TABLE_NS(TrigData, ScriptFlag, TrigData::NUM_SCRIPT_FLAGS)
{
	{	TrigData::Script_Threadable,	"Threadable"	},
	{	TrigData::Script_DebugMode,		"Debug"			}
};


Race factions[NUM_RACES] =
{
	RACE_HUMAN,
	RACE_HUMAN,
	RACE_PREDATOR,
	RACE_ALIEN,
	RACE_OTHER
};


int wear_to_armor_loc[NUM_WEARS] =
{
	-1,	//	WEAR_FINGER_R
	-1,	//	WEAR_FINGER_L
	-1,	//	WEAR_NECK
	ARMOR_LOC_BODY,	//	WEAR_BODY
	ARMOR_LOC_HEAD,	//	WEAR_HEAD
	-1 /*ARMOR_LOC_LEGS*/,	//	WEAR_LEGS
	-1 /*ARMOR_LOC_FEET*/,	//	WEAR_FEET
	-1 /*ARMOR_LOC_HANDS*/,	//	WEAR_HANDS
	-1 /*ARMOR_LOC_ARMS*/,	//	WEAR_ARMS
	ARMOR_LOC_BODY,	//	WEAR_ABOUT
	-1,	//	WEAR_WAIST
	-1,	//	WEAR_WRIST_R
	-1,	//	WEAR_WRIST_L
	-1,	//	WEAR_EYES
	-1,	//	WEAR_TAIL
	-1,	//	WEAR_HAND_R
	-1,	//	WEAR_HAND_L
};


char *damage_type_abbrevs[] =
{
	"BLN",
	"SLA",
	"IMP",
	"BAL",
	"FIR",
	"ENR",
	"ACD",
	"\n"
};


char *damage_types[] =
{
	"Blunt",
	"Slash",
	"Impale",
	"Ballistic",
	"Fire",
	"Energy",
	"Acid",
	"\n"
};


char *damage_locations[] =
{
	"Body",
	"Head",
	"Natural",
	"Mission",
	"\n"
};


char *combat_modes[] =
{
	"defensive",
	"normal",
	"offensive",
	"\n"
};


char *team_titles[] =
{
	"None",
	"`WSolo`n",
	"`RRed Team`n",
	"`BBlue Team`n",
	"`GGreen Team`n",
	"`YGold Team`n",
	"`CTeal Team`n",
	"`MPurple Team`n",
	"`rT`Kea`rm R`Kamro`rd`n",
	"`YW`yeyland-`YY`yutani`n",
	"`rUS`nC`bMC `gM`ya`gr`yi`gn`ye`gs`n",
	"\n"
};


char *team_names[] =
{
	"none",
	"solo",
	"red",
	"blue",
	"green",
	"gold",
	"teal",
	"purple",
	"ramrod",
	"weyland",
	"uscmc",
	"\n"
};


RoomData * team_recalls[MAX_TEAMS + 1] =
{
	NULL,
	NULL,
	NULL,	//	Red
	NULL,	//	Blue
	NULL,	//	Green
	NULL,	//	Gold
	NULL,	//	Purple
	NULL,	//	Teal
	NULL,	//	Ramrod
	NULL,	//	Weyland
	NULL		//	USCMC
};


char *token_sets[] =
{
	"Basic Set",
	"Next Set",
	"\n"
};


int max_hit_base[NUM_RACES] =
{
	650,
	680,
	850,
	860,
	500
};

int max_hit_bonus[NUM_RACES] =
{
	5,
	6,
	5,
	8,
	4
};


ENUM_NAME_TABLE(RoomFlag, NUM_ROOM_FLAGS)
{
	{ ROOM_DARK,		"DARK"			},
	{ ROOM_DEATH,		"DEATH"			},
	{ ROOM_NOMOB,		"NO-MOB"		},
	{ ROOM_INDOORS,		"INDOORS"		},
	{ ROOM_PEACEFUL,	"PEACEFUL"		},
	{ ROOM_SOUNDPROOF,	"SOUNDPROOF"	},
	{ ROOM_BLACKOUT,	"BLACKOUT"		},
	{ ROOM_NOTRACK,		"NO-TRACK"		},
	{ ROOM_PARSE,		"PARSER"		},
	{ ROOM_TUNNEL,		"TUNNEL"		},
	{ ROOM_PRIVATE,		"PRIVATE"		},
	{ ROOM_STAFFROOM,	"STAFF-ONLY"	},
	{ ROOM_ADMINROOM,	"ADMIN-ONLY"	},
	{ ROOM_NOPK,		"NO-PK"			},
	{ ROOM_SPECIAL_MISSION,	"SPECIAL-MISSION"},
	{ ROOM_VEHICLE,		"VEHICLES"		},
	{ ROOM_NOLITTER,	"NO-LITTER"		},
	{ ROOM_GRAVITY,		"GRAVITY"		},
	{ ROOM_VACUUM,		"VACUUM"		},
	{ ROOM_NORECALL,	"NO-RECALL"		},
	{ ROOM_NOHIVE,		"NO-HIVE"		},
	{ ROOM_STARTHIVED,	"START-HIVED"	},
	//	Private Bits
	{ ROOM_HOUSE,		"HOUSE"			},
	{ ROOM_HOUSE_CRASH,	"HCRSH"			},
	{ ROOM_HIVED,		"HIVED"			},
	{ ROOM_DELETED,		"DELETED"		},
};


ENUM_NAME_TABLE(ZoneFlag, NUM_ZONE_FLAGS)
{
	{ ZONE_NS_SUBZONES,	"NS-SUBZONES"	}
};
