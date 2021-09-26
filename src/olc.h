/*************************************************************************
*   File: olc.h                      Part of Aliens vs Predator: The MUD *
*  Usage: Header file for the OLC System                                 *
*************************************************************************/

#ifndef __OLC_H__
#define __OLC_H__

#include "types.h"
#include "characters.h"

#include "scripts.h"

#include "weather.h"
#include "objects.h"
#include "rooms.h"
#include "shop.h"
#include "clans.h"
#include "zones.h"
#include "socials.h"
#include "help.h"

#include "WeakPtr.h"

//	Include a short explanation of each field in your zone files,
#undef ZEDIT_HELP_IN_FILE

//	Clear the screen before certain Oasis menus
#define CLEAR_SCREEN	1


//	Macros, defines, structs and globals for the OLC suite.
#define NUM_MOB_FLAGS		23
#define NUM_AFF_FLAGS		24
#define NUM_ATTACK_TYPES	19

#define NUM_WTRIG_TYPES		13
#define NUM_OTRIG_TYPES		30
#define NUM_MTRIG_TYPES		22
#define NUM_GTRIG_TYPES		3

#define NUM_AMMO_TYPES		31

#define NUM_ITEM_FLAGS		27
#define NUM_ITEM_WEARS 		17
#define NUM_APPLIES			15
#define NUM_LIQ_TYPES 		1
#define NUM_POSITIONS		9

#define NUM_GENDERS			3
#define NUM_SHOP_FLAGS 		2
#define NUM_TRADERS 		3

#define NUM_TOKEN_SETS		1

/* aedit permissions magic number */
#define AEDIT_PERMISSION	999
#define HEDIT_PERMISSION	666

#define LVL_BUILDER		LVL_STAFF

/*. Limit info .*/
#define MAX_ROOM_NAME	75
#define MAX_MOB_NAME	50
#define MAX_OBJ_NAME	50
#define MAX_ROOM_DESC	1024
#define MAX_EXIT_DESC	256
#define MAX_EXTRA_DESC  512
#define MAX_MOB_DESC	512
#define MAX_OBJ_DESC	512
#define MAX_HELP_KEYWORDS	75
#define MAX_HELP_ENTRY		1024

/*
 * Utilities exported from olc.c.
 */
void cleanup_olc(DescriptorData *d);
bool get_zone_perms(CharData * ch, VirtualID vid);

/*
 * OLC structures.
 */

struct olc_data {
	inline olc_data() : mode(0), number(0), value(0), value2(0),
			sourceZone(NULL),
			resetCommand(-1),
			shop(NULL), action(NULL), desc(NULL),
			originalHelp(NULL),
			opinion(NULL), var(NULL),
			m_WeatherParent(NULL) { }
	~olc_data();
	
	int						mode;
	int						number;
	VirtualID				vid;
	int						value, value2;
	ZoneData *				sourceZone;
	
	Lexi::String			storage;
	ManagedPtr<CharData>	mob;
	ManagedPtr<RoomData>	room;
	ManagedPtr<ObjData>		obj;
	ManagedPtr<Clan>		clan;
	ManagedPtr<ZoneData>	zone;
	ManagedPtr<Weather::Climate>	m_Climate;
	int						resetCommand;
	ManagedPtr<ShopData>	shop;
	ManagedPtr<TrigData>	trig;
	ManagedPtr<Social>		action;
	ManagedPtr<BehaviorSet>	behavior;
	ExtraDesc *				desc;
	HelpManager::Entry *	originalHelp;
	ManagedPtr<HelpManager::Entry>	help;
	ScriptVector			triglist;
	Opinion *				opinion;
	ScriptVariable *		var;
	ScriptVariable::List	global_vars;
	BehaviorSets			behaviorsets;
	ZoneData *				m_WeatherParent;
};


/*
 * Descriptor access macros.
 */
#define OLC_MODE(d)		((d)->olc->mode)		/* Parse input mode		*/
#define OLC_NUM(d)		((d)->olc->number)		/* Room/Obj VNUM		*/
#define OLC_VID(d)		((d)->olc->vid)			/* Room/Obj VNUM		*/
#define OLC_VAL(d)		((d)->olc->value)		/* Scratch variable		*/
#define OLC_VAL2(d)		((d)->olc->value2)		/* Scratch variable		*/
#define OLC_SOURCE_ZONE(d)	((d)->olc->sourceZone)
#define OLC_STORAGE(d)	((d)->olc->storage)		/* For command storage	*/
#define OLC_ROOM(d)		((d)->olc->room)		/* Room structure		*/
#define OLC_OBJ(d)		((d)->olc->obj)			/* Object structure		*/
#define OLC_ZONE(d)		((d)->olc->zone)		/* Zone structure		*/
#define OLC_CLIMATE(d)	((d)->olc->m_Climate)	/* Climate structure	*/
#define OLC_WEATHERPARENT(d)	((d)->olc->m_WeatherParent)	/* Climate structure	*/
#define OLC_MOB(d)		((d)->olc->mob)			/* Mob structure		*/
#define OLC_SHOP(d)		((d)->olc->shop)		/* Shop structure		*/
#define OLC_ACTION(d)	((d)->olc->action)		/* Action structure		*/
#define OLC_DESC(d)		((d)->olc->desc)		/* Extra description	*/
#define OLC_ORIGINAL_HELP(d)	((d)->olc->originalHelp)		/* Help structure		*/
#define OLC_HELP(d)		((d)->olc->help)		/* Help structure		*/
#define OLC_TRIG(d)		((d)->olc->trig)		/* Trig structure		*/
#define OLC_TRIGLIST(d)	((d)->olc->triglist)		/* Intlist				*/
#define OLC_CLAN(d)		((d)->olc->clan)		/* Clan list			*/
#define OLC_BEHAVIOR(d)	((d)->olc->behavior)		/* Clan list			*/
#define OLC_TRIGVAR(d)	((d)->olc->var)
//#define OLC_SHAREDVARS(d)	((d)->olc->shared_vars)
#define OLC_BEHAVIORSETS(d)	((d)->olc->behaviorsets)
#define OLC_GLOBALVARS(d)	((d)->olc->global_vars)
#define OLC_RESETCOMMAND(d)	((d)->olc->resetCommand)

/*
 * Other macros.
 */
#define OLC_EXIT(d)		(OLC_ROOM(d)->m_Exits[OLC_VAL(d)])
#define OLC_OPINION(d)	((d)->olc->opinion)
#define GET_OLC_ZONE(c)	((c)->player_specials->saved.olc_zone)

/*
 * Add/Remove save list types.
 */
#define OLC_SAVE_ROOM			0
#define OLC_SAVE_OBJ			1
#define OLC_SAVE_ZONE			2
#define OLC_SAVE_MOB			3
#define OLC_SAVE_SHOP			4
#define OLC_SAVE_ACTION			5
#define OLC_SAVE_HELP			6
#define OLC_SAVE_TRIGGER		7
#define OLC_SAVE_CLAN			8


/*
 * Submodes of OEDIT connectedness.
 */
enum
{
	OEDIT_MAIN_MENU = 1,
	OEDIT_EDIT_NAMELIST,
	OEDIT_SHORTDESC,
	OEDIT_LONGDESC,
	OEDIT_ACTDESC,
	OEDIT_TYPE,
	OEDIT_EXTRAS,
	OEDIT_RESTRICTIONS,
	OEDIT_WEAR,
	OEDIT_WEIGHT,
	OEDIT_COST,
	OEDIT_TIMER,
	OEDIT_VALUE_0,
	OEDIT_VALUE_1,
	OEDIT_VALUE_2,
	OEDIT_VALUE_3,
	OEDIT_VALUE_4,
	OEDIT_VALUE_5,
	OEDIT_VALUE_6,
	OEDIT_VALUE_7,
	OEDIT_FVALUE_0,
	OEDIT_FVALUE_1,
	OEDIT_VIDVALUE_0,
	OEDIT_APPLY,
	OEDIT_APPLYMOD,
	OEDIT_EXTRADESC_KEY,
	OEDIT_CONFIRM_SAVEDB,
	OEDIT_CONFIRM_SAVESTRING,
	OEDIT_PROMPT_APPLY,
	OEDIT_EXTRADESC_DESCRIPTION,
	OEDIT_EXTRADESC_MENU,
	OEDIT_LEVEL,
	OEDIT_GUN_MENU,
	OEDIT_GUN_DAMAGE,
	OEDIT_GUN_DAMAGE_TYPE,
	OEDIT_GUN_RATE,
	OEDIT_GUN_RANGE,
	OEDIT_GUN_OPTIMALRANGE,
	OEDIT_GUN_SKILL,
	OEDIT_GUN_SKILL2,
	OEDIT_GUN_AMMO_TYPE,
	OEDIT_GUN_ATTACK_TYPE,
	OEDIT_AFFECTS,
	OEDIT_TRIGGERS,
	OEDIT_TRIGGER_ADD_BEHAVIORSET,
	OEDIT_TRIGGER_ADD_SCRIPT,
	OEDIT_TRIGGER_CHOOSE_VARIABLE,
	OEDIT_TRIGGER_VARIABLE,
	OEDIT_TRIGGER_VARIABLE_NAME,
	OEDIT_TRIGGER_VARIABLE_VALUE,
	OEDIT_TRIGGER_VARIABLE_CONTEXT,
	OEDIT_TRIGGER_REMOVE,
	OEDIT_VALUE_MENU,
	OEDIT_CLAN,
	OEDIT_APPLY_SKILL,
	OEDIT_EXTRADESC,
	OEDIT_EXTRADESC_DELETE
};

/* Submodes of REDIT connectedness */
#define REDIT_MAIN_MENU					1
#define REDIT_NAME						2
#define REDIT_DESC						3
#define REDIT_FLAGS						4
#define REDIT_SECTOR					5
#define REDIT_EXIT_MENU					6
#define REDIT_CONFIRM_SAVEDB			7
#define REDIT_CONFIRM_SAVESTRING		8
#define REDIT_EXIT_NUMBER				9
#define REDIT_EXIT_DESCRIPTION			10
#define REDIT_EXIT_KEYWORD				11
#define REDIT_EXIT_KEY					12
#define REDIT_EXIT_DOORFLAGS			13
#define REDIT_EXTRADESC_MENU			14
#define REDIT_EXTRADESC					15
#define REDIT_EXTRADESC_KEY				16
#define REDIT_EXTRADESC_DESCRIPTION		17
#define REDIT_EXTRADESC_DELETE	     	18


//	Submodes of ZEDIT connectedness
#define ZEDIT_MAIN_MENU					0
#define ZEDIT_DELETE_ENTRY				1
#define ZEDIT_NEW_ENTRY					2
#define ZEDIT_CHANGE_ENTRY				3
#define ZEDIT_COMMAND_TYPE				4
#define ZEDIT_IF_FLAG					5
#define ZEDIT_ARG1						6
#define ZEDIT_ARG2						7
#define ZEDIT_ARG3						8
#define ZEDIT_ARG4						9
#define ZEDIT_ARG5						10
#define ZEDIT_ZONE_NAME					11
#define ZEDIT_ZONE_COMMENT				12
#define ZEDIT_ZONE_LIFE					13
#define ZEDIT_ZONE_TOP					14
#define ZEDIT_ZONE_RESET				15
#define ZEDIT_CONFIRM_SAVESTRING		16
#define ZEDIT_REPEAT					17
#define ZEDIT_COMMAND_MENU				18
#define ZEDIT_HIVE						19
#define ZEDIT_OWNER						20
#define ZEDIT_WEATHER					22
#define ZEDIT_WEATHER_PARENT			23
#define ZEDIT_WEATHER_SEASON_PATTERN	24
#define ZEDIT_WEATHER_FLAGS				25
#define ZEDIT_WEATHER_ENERGY			26
#define ZEDIT_WEATHER_SEASON			27
#define ZEDIT_WEATHER_SEASON_MENU		28
#define ZEDIT_WEATHER_SEASON_WIND				29
#define ZEDIT_WEATHER_SEASON_WIND_DIRECTION		30
#define ZEDIT_WEATHER_SEASON_PRECIPITATION		31
#define ZEDIT_WEATHER_SEASON_TEMPERATURE		32
#define ZEDIT_FLAGS						33



//	Submodes of MEDIT connectedness
#define MEDIT_MAIN_MENU					0
#define MEDIT_ALIAS						1
#define MEDIT_S_DESC					2
#define MEDIT_L_DESC					3
#define MEDIT_D_DESC					4
#define MEDIT_NPC_FLAGS					5
#define MEDIT_AFF_FLAGS					6
#define MEDIT_CONFIRM_SAVESTRING		7
#define MEDIT_ATTRIBUTES				8
#define MEDIT_ARMOR						9
#define MEDIT_TRIGGERS					10
#define MEDIT_TRIGGER_ADD_BEHAVIORSET	11
#define MEDIT_TRIGGER_VARIABLE			12
#define MEDIT_TRIGGER_VARIABLE_NAME		13
#define MEDIT_TRIGGER_VARIABLE_VALUE	14
#define MEDIT_TRIGGER_VARIABLE_CONTEXT	15
#define MEDIT_EQUIPMENT					16
#define MEDIT_EQUIPMENT_EDIT			17
#define MEDIT_ATTACK_MENU				18
#define MEDIT_ATTACK_SPEED				19
#define MEDIT_AI_MENU					20
#define MEDIT_TRIGGER_ADD_SCRIPT		21
#define MEDIT_EQUIPMENT_EDIT_VNUM		22
#define MEDIT_OPINIONS_HATES_VNUM		23
#define MEDIT_OPINIONS_FEARS_VNUM		24
//	Numerical responses
#define MEDIT_NUMERICAL_RESPONSE		30
#define MEDIT_SEX						31
#define MEDIT_BASE_HP					32
#define MEDIT_VARY_HP					33
#define MEDIT_MP						34
#define MEDIT_EQUIPMENT_NEW				35
#define MEDIT_EQUIPMENT_EDIT_CHOOSE		36
#define MEDIT_EQUIPMENT_DELETE			37
#define MEDIT_EQUIPMENT_EDIT_POS		39
#define MEDIT_POS						40
#define MEDIT_DEFAULT_POS				41
#define MEDIT_ATTACK_TYPE				42
#define MEDIT_ATTACK_DAMAGETYPE			43
#define MEDIT_ATTACK_DAMAGE				44
#define MEDIT_ATTACK_RATE				45
#define MEDIT_LEVEL						46
#define MEDIT_RACE						47
#define MEDIT_CLAN						48
#define MEDIT_TRIGGER_REMOVE			55
#define MEDIT_TRIGGER_CHOOSE_VARIABLE	56
#define MEDIT_ATTR_STR					60
#define MEDIT_ATTR_HEA					61
#define MEDIT_ATTR_COO					62
#define MEDIT_ATTR_AGI					63
#define MEDIT_ATTR_PER					64
#define MEDIT_ATTR_KNO					65
#define MEDIT_OPINIONS					70
#define MEDIT_OPINIONS_HATES_SEX		71
#define MEDIT_OPINIONS_HATES_RACE		72
#define MEDIT_OPINIONS_FEARS_SEX		74
#define MEDIT_OPINIONS_FEARS_RACE		75
#define MEDIT_ARMOR_1					77
#define MEDIT_ARMOR_2					78
#define MEDIT_ARMOR_3					79
#define MEDIT_ARMOR_4					80
#define MEDIT_ARMOR_5					81
#define MEDIT_ARMOR_6					82
#define MEDIT_ARMOR_7					83
#define MEDIT_AI_AGGRROOM				90
#define MEDIT_AI_AGGRRANGED				91
#define MEDIT_AI_AGGRLOYALTY			92
#define MEDIT_AI_AWARERANGE				93
#define MEDIT_AI_MOVERATE				94
#define MEDIT_TEAM						95


//	Submodes of SEDIT connectedness
#define SEDIT_MAIN_MENU					0
#define SEDIT_CONFIRM_SAVESTRING		1
#define SEDIT_NOITEM1					2
#define SEDIT_NOITEM2					3
#define SEDIT_NOCASH1					4
#define SEDIT_NOCASH2					5
#define SEDIT_NOBUY						6
#define SEDIT_BUY						7
#define SEDIT_SELL						8
#define SEDIT_PRODUCTS_MENU				11
#define SEDIT_ROOMS_MENU				12
#define SEDIT_NAMELIST_MENU				13
#define SEDIT_NAMELIST					14
//	Numerical responses
#define SEDIT_NUMERICAL_RESPONSE		20
#define SEDIT_OPEN1						21
#define SEDIT_OPEN2						22
#define SEDIT_CLOSE1					23
#define SEDIT_CLOSE2					24
#define SEDIT_KEEPER					25
#define SEDIT_BUY_PROFIT				26
#define SEDIT_SELL_PROFIT				27
#define SEDIT_TYPE_MENU					29
#define SEDIT_DELETE_TYPE				30
#define SEDIT_DELETE_PRODUCT			31
#define SEDIT_NEW_PRODUCT				32
#define SEDIT_DELETE_ROOM				33
#define SEDIT_NEW_ROOM					34
#define SEDIT_SHOP_FLAGS				35
#define SEDIT_NOTRADE					36


//	Submodes of AEDIT connectedness
#define AEDIT_CONFIRM_SAVESTRING		0
#define AEDIT_CONFIRM_EDIT				1
#define AEDIT_CONFIRM_ADD				2
#define AEDIT_MAIN_MENU					3
#define AEDIT_ACTION_NAME				4
#define AEDIT_SORT_AS					5
#define AEDIT_MIN_CHAR_POS				6
#define AEDIT_MIN_VICT_POS				7
#define AEDIT_HIDDEN_FLAG				8
#define AEDIT_MIN_CHAR_LEVEL			9
#define AEDIT_NOVICT_CHAR				10
#define AEDIT_NOVICT_OTHERS				11
#define AEDIT_VICT_CHAR_FOUND			12
#define AEDIT_VICT_OTHERS_FOUND			13
#define AEDIT_VICT_VICT_FOUND			14
#define AEDIT_VICT_NOT_FOUND			15
#define AEDIT_SELF_CHAR					16
#define AEDIT_SELF_OTHERS				17
#define AEDIT_VICT_CHAR_BODY_FOUND     	18
#define AEDIT_VICT_OTHERS_BODY_FOUND   	19
#define AEDIT_VICT_VICT_BODY_FOUND     	20
#define AEDIT_OBJ_CHAR_FOUND			21
#define AEDIT_OBJ_OTHERS_FOUND			22


//	Submodes of HEDIT connectedness
#define HEDIT_CONFIRM_SAVESTRING		0
#define HEDIT_MAIN_MENU					1
#define HEDIT_ENTRY						2
#define HEDIT_MIN_LEVEL					3
#define HEDIT_KEYWORDS					4


//	Submodes of SCRIPTEDIT connectedness
#define SCRIPTEDIT_CONFIRM_SAVESTRING	0
#define SCRIPTEDIT_MAIN_MENU			1
#define SCRIPTEDIT_NAME					2
#define SCRIPTEDIT_CLASS				3
#define SCRIPTEDIT_TYPE					4
#define SCRIPTEDIT_ARG					5
#define SCRIPTEDIT_NARG					6
#define SCRIPTEDIT_MULTITHREAD			7
#define SCRIPTEDIT_SCRIPT				8


//	Submodes of CEDIT connectedness
#define CEDIT_CONFIRM_SAVESTRING		0
#define CEDIT_MAIN_MENU					1
#define CEDIT_NAME						2
#define CEDIT_TAG						3
#define CEDIT_OWNER						4
#define CEDIT_ROOM						5
#define CEDIT_SAVINGS					6
#define CEDIT_RACES						7
#define CEDIT_MEMBERLIMIT				8

#define BEDIT_CONFIRM_SAVESTRING		0
#define BEDIT_MAIN_MENU					1
#define BEDIT_NAME						2
#define BEDIT_TRIGGER_ADD_SCRIPT		3
#define BEDIT_TRIGGER_CHOOSE_VARIABLE	4
#define BEDIT_TRIGGER_VARIABLE			5
#define BEDIT_TRIGGER_VARIABLE_NAME		6
#define BEDIT_TRIGGER_VARIABLE_VALUE	7
#define BEDIT_TRIGGER_VARIABLE_CONTEXT	8
#define BEDIT_TRIGGER_REMOVE			9

#endif
