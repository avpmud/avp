/* ************************************************************************
*   File: interpreter.c                                 Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "structs.h"
#include "alias.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "olc.h"
#include "ban.h"
#include "buffer.h"
#include "event.h"
#include "constants.h"
#include "boards.h"

#include "staffcmds.h"

//#include "imc-mercbase.h"
//#include "icec-mercbase.h"
#include "imc/imc.h"

extern int circle_restrict;

/* external functions */
void do_start(CharData *ch);
void init_char(CharData *ch);
bool special(CharData *ch, const char *cmd, char *arg);
void oedit_parse(DescriptorData *d, const char *arg);
void zedit_parse(DescriptorData *d, const char *arg);
void medit_parse(DescriptorData *d, const char *arg);
void sedit_parse(DescriptorData *d, const char *arg);
void aedit_parse(DescriptorData *d, const char *arg);
void hedit_parse(DescriptorData *d, const char *arg);
void cedit_parse(DescriptorData *d, const char *arg);
void bedit_parse(DescriptorData *d, const char *arg);
void scriptedit_parse(DescriptorData *d, const char *arg);
void roll_real_abils(CharData * ch);
int command_mtrigger(CharData *actor, const char *cmd, const char *argument);
int command_otrigger(CharData *actor, const char *cmd, const char *argument);
int command_wtrigger(CharData *actor, const char *cmd, const char *argument);
int command_gtrigger(CharData *actor, const char *cmd, const char *argument);
void start_otrigger(ObjData *obj, CharData *actor);
int wear_otrigger(ObjData *obj, CharData *actor, int where);

/* local functions */
void perform_complex_alias(txt_q &input_q, char *orig, const Alias *a);
int perform_alias(DescriptorData *d, char *orig);
bool reserved_word(const char *argument);
bool find_name(const char *name);
int _parse_name(const char *arg, char *name);
int perform_dupe_check(DescriptorData *d);
bool has_polls(int id);


/* prototypes for all do_x functions. */
ACMD(do_afk);
ACMD(do_action);
ACMD(do_advance);
ACMD(do_alias);
ACMD(do_allow);
ACMD(do_assist);
ACMD(do_at);
ACMD(do_ban);
ACMD(do_bash);
ACMD(do_circle);
ACMD(do_commands);
ACMD(do_consider);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_diagnose);
ACMD(do_dig);
ACMD(do_display);
ACMD(do_drink);
ACMD(do_drop);
ACMD(do_olddrive);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_enter);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exits);
ACMD(do_flee);
ACMD(do_retreat);
ACMD(do_follow);
ACMD(do_force);
ACMD(do_as);
ACMD(do_gecho);
ACMD(do_gen_comm);
ACMD(do_history);
ACMD(do_sayhistory);
ACMD(do_infohistory);
ACMD(do_gen_door);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_give);
ACMD(do_gold);
ACMD(do_goto);
ACMD(do_grab);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_help);
ACMD(do_hit);
ACMD(do_house);
ACMD(do_houseedit);
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_ignore);
ACMD(do_kick_bite);
ACMD(do_kill);
ACMD(do_last);
ACMD(do_leave);
ACMD(do_levels);
ACMD(do_load);
ACMD(do_look);
ACMD(do_scan);
ACMD(do_move);
ACMD(do_not_here);
ACMD(do_olc);
ACMD(do_olc_other);
ACMD(do_olccopy);
ACMD(do_olcmove);
ACMD(do_order);
ACMD(do_page);
ACMD(do_pagelength);
ACMD(do_pkill);
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_practice);
ACMD(do_purge);
ACMD(do_put);
ACMD(do_qcomm);
ACMD(do_quit);
ACMD(do_reboot);
ACMD(do_recall);
ACMD(do_reload);
ACMD(do_remove);
ACMD(do_reply);
ACMD(do_retell);
ACMD(do_report);
ACMD(do_rescue);
ACMD(do_respond);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_return);
ACMD(do_save);
ACMD(do_say);
ACMD(do_score);
ACMD(do_send);
ACMD(do_set);
ACMD(do_show);
ACMD(do_shoot);
ACMD(do_spray);
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_skillset);
ACMD(do_sleep);
ACMD(do_sneak);
ACMD(do_snoop);
ACMD(do_spec_comm);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_stun);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_throw);
ACMD(do_pull);
ACMD(do_prime);
ACMD(do_teleport);
ACMD(do_tell);
ACMD(do_time);
ACMD(do_title);
ACMD(do_toggle);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_unban);
ACMD(do_ungroup);
//ACMD(do_use);
ACMD(do_users);
ACMD(do_visible);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_wake);
ACMD(do_watch);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_wield);
ACMD(do_wimpy);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_zreset);

ACMD(do_zallow);
ACMD(do_zdeny);
ACMD(do_ostat);
ACMD(do_mstat);
ACMD(do_rstat);
ACMD(do_zstat);
ACMD(do_zlist);
ACMD(do_liblist);

ACMD(do_tedit);
  
ACMD(do_push_drag);

ACMD(do_depiss);
ACMD(do_repiss);

ACMD(do_hunt);

ACMD(do_mount);
ACMD(do_unmount);

ACMD(do_vwear);

ACMD(do_promote);

ACMD(do_reward);
ACMD(do_delete);

ACMD(do_copyover);

ACMD(do_buffer);

ACMD(do_vattached);
ACMD(do_attach);
ACMD(do_detach);

// Clan ACMDs
ACMD(do_apply);
ACMD(do_resign);
ACMD(do_enlist);
ACMD(do_boot);
ACMD(do_clanban);
ACMD(do_clanlist);
ACMD(do_clanstat);
//ACMD(do_members);
ACMD(do_forceenlist);
ACMD(do_clanwho);
ACMD(do_clanpromote);
ACMD(do_clandemote);
ACMD(do_clanallow);
ACMD(do_clanset);
ACMD(do_cmotd);
ACMD(do_deposit);
ACMD(do_withdraw);
ACMD(do_war);
ACMD(do_ally);
ACMD(do_clanpolitics);
ACMD(do_clanrank);

ACMD(do_prompt);

ACMD(do_move_element);

ACMD(do_string);

ACMD(do_massolcsave);

ACMD(do_path);
ACMD(do_vote);
ACMD(do_poll);

ACMD(do_wizcall);
ACMD(do_wizassist);
ACMD(do_stopscripts);
ACMD(do_setcapture);

ACMD(do_objindex);
ACMD(do_mobindex);
ACMD(do_trigindex);

ACMD(do_gscript);

ACMD(do_hive);
ACMD(do_dehive);
ACMD(do_capturestatus);

ACMD(do_destroy);
ACMD(do_repair);
ACMD(do_install);
ACMD(do_deploy);

ACMD(do_mode);
ACMD(do_team);

ACMD(do_compile);
ACMD(do_killall);
ACMD(do_listhelp);


ACMD(do_openhatch);
ACMD(do_boardship);
ACMD(do_land);
ACMD(do_launch);
ACMD(do_gate);
ACMD(do_radar);
ACMD(do_accelerate);
ACMD(do_course);
ACMD(do_shipstat);
ACMD(do_calculate);
ACMD(do_hyperspace);
ACMD(do_scanplanet);
ACMD(do_scanship);
ACMD(do_followship);
ACMD(do_target);
ACMD(do_gen_autoship);
ACMD(do_shipcloak);
ACMD(do_baydoors);
ACMD(do_buyship);
ACMD(do_svlist);
ACMD(do_stlist);
ACMD(do_sylist);
ACMD(do_shiplist);
ACMD(do_shiprecall);
ACMD(do_shipfire);
ACMD(do_repairship);
ACMD(do_shipupgrade);
ACMD(do_tranship);
ACMD(do_addpilot);
ACMD(do_rempilot);
ACMD(do_stedit);
ACMD(do_setship);
ACMD(do_sabotage);
ACMD(do_ping);
ACMD(do_interceptor);
ACMD(do_deploy);
ACMD(do_drive);
ACMD(do_remote);
ACMD(do_jumpvector);
ACMD(do_forge);
ACMD(do_foldspace);
ACMD(do_wormhole);
ACMD(do_disruptionweb);
ACMD(do_shieldmatrix);
ACMD(do_orbitalinsertion);



ACMD(do_performancetest);
ACMD(do_profile);
ACMD(do_crash);


struct command_info *complete_cmd_info;


/*
[Commands/North]
	Command:		n.orth
	Function:		do_move
	Parameter:		1
	Position:		STANDING
	Level:			0
	Flags:			LOSE-HIDE

[Commands/Echo]
	Command:		echo
	Function:		do_echo
	Position:		Sleeping
	Permissions:	GENERAL
*/


/* This is the Master Command List(tm).

 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */

struct command_info cmd_info[] = {
	{ "RESERVED"	, "RESERVED"	, 0, 0, 0, 0 },	/* this must be first -- for specprocs */

	/* directions must come before other commands but after RESERVED */
	{ "north"		, "n"			, POS_STANDING	, do_move		, 0, LOSE_HIDE, SCMD_NORTH },
	{ "east"		, "e"			, POS_STANDING	, do_move		, 0, LOSE_HIDE, SCMD_EAST },
	{ "south"		, "s"			, POS_STANDING	, do_move		, 0, LOSE_HIDE, SCMD_SOUTH },
	{ "west"		, "w"			, POS_STANDING	, do_move		, 0, LOSE_HIDE, SCMD_WEST },
	{ "up"			, "u"			, POS_STANDING	, do_move		, 0, LOSE_HIDE, SCMD_UP },
	{ "down"		, "d"			, POS_STANDING	, do_move		, 0, LOSE_HIDE, SCMD_DOWN },

	/* Communications channels */
	{ "broadcast"	, "broa"		, POS_STUNNED	, do_gen_comm	, STAFF_CMD	, STAFF_SECURITY | STAFF_SRSTAFF | STAFF_ADMIN | STAFF_GAME, SCMD_BROADCAST },
	{ "chat"		, "chat"		, POS_STUNNED	, do_gen_comm	, 0, 0, SCMD_GOSSIP },
	{ "clan"		, "clan"		, POS_STUNNED	, do_gen_comm	, 0, 0, SCMD_CLAN },
//	{ "gossip"		, "go"			, POS_STUNNED	, do_gen_comm	, 0, 0, SCMD_GOSSIP },
	{ "grats"		, "grats"		, POS_STUNNED	, do_gen_comm	, 0, 0, SCMD_GRATZ },
	{ "msay"		, "msay"		, POS_STUNNED 	, do_gen_comm	, 0, 0, SCMD_MISSION },
	{ "music"		, "mus"			, POS_STUNNED	, do_gen_comm	, 0, 0, SCMD_MUSIC },
	{ "race"		, "rac"			, POS_STUNNED	, do_gen_comm	, 0, 0, SCMD_RACE	},
	{ "shout"		, "shou"		, POS_STUNNED 	, do_gen_comm	, 0, 0, SCMD_SHOUT },
	{ "bitch"		, "bitc"		, POS_STUNNED 	, do_gen_comm	, 0, 0, SCMD_BITCH },
	{ "newbie"		, "newb"		, POS_STUNNED	, do_gen_comm	, 0, 0, SCMD_NEWBIE },

	/* now, the main list */
	{ "assist"		, "as"			, POS_FIGHTING	, do_assist		, 1, 0, 0 },
	{ "afk"			, "afk"			, POS_DEAD		, do_afk		, 0, 0, 0 },
	{ "alias"		, "ali"			, POS_DEAD		, do_alias		, 0, 0, 0 },
	{ "ask"			, "ask"			, POS_RESTING	, do_spec_comm	, 0, 0, SCMD_ASK },
	{ "autoexit"	, "autoex"		, POS_DEAD		, do_gen_tog	, 0, 0, SCMD_AUTOEXIT },
	{ "at"			, "a"			, POS_DEAD		, do_at			, STAFF_CMD	, STAFF_GEN, 0 },
	{ "attributes"	, "attr"		, POS_DEAD		, do_score		, 0, 0, SCMD_ATTRIBUTES },

	{ "brief"		, "brief"		, POS_DEAD		, do_gen_tog	, 0, 0, SCMD_BRIEF },
	{ "bug"			, "bug"			, POS_DEAD		, do_gen_write	, 0, 0, SCMD_BUG },
	
	{ "clear"		, "clear"		, POS_DEAD		, do_gen_ps		, 0, 0, SCMD_CLEAR },
	{ "close"		, "c"			, POS_SITTING	, do_gen_door	, 0, LOSE_HIDE, SCMD_CLOSE },
	{ "cls"			, "cls"			, POS_DEAD		, do_gen_ps		, 0, 0, SCMD_CLEAR },
	{ "consider"	, "co"			, POS_RESTING	, do_consider	, 0, 0, 0},
	{ "color"		, "color"		, POS_DEAD		, do_gen_tog	, 0, 0, SCMD_COLOR },
	{ "commands"	, "comm"		, POS_DEAD		, do_commands	, 0, 0, SCMD_COMMANDS },
	{ "compact"		, "comp"		, POS_DEAD		, do_gen_tog	, 0, 0, SCMD_COMPACT },
	{ "compile"		, "compile"		, POS_DEAD		, do_compile	, LVL_STAFF, 0, 0 },
	{ "credits"		, "cred"		, POS_DEAD		, do_gen_ps		, 0, 0, SCMD_CREDITS },

	{ "destroy"		, "des"			, POS_STANDING	, do_destroy	, 0, LOSE_HIDE, 0 },
	{ "diagnose"	, "diag"		, POS_RESTING	, do_diagnose	, 0, 0, 0 },
	{ "display"		, "disp"		, POS_DEAD		, do_display	, 0, 0, 0 },
	{ "drop"		, "drop"		, POS_RESTING	, do_drop		, 0, 0, 0 },
//	{ "drag"		, "drag"		, POS_STANDING	, do_push_drag	, 0, LOSE_HIDE, SCMD_DRAG },
	{ "drink"		, "drink"		, POS_RESTING	, do_drink		, 0, 0, SCMD_DRINK },
	{ "drive"		, "dri"			, POS_RESTING	, do_olddrive	, 0, 0, 0 },

	{ "eat"			, "eat"			, POS_RESTING	, do_eat		, 0, 0, SCMD_EAT },
	{ "emote"		, "em"			, POS_RESTING	, do_echo		, 0, 0, SCMD_EMOTE },
	{ ":"			, ":"			, POS_RESTING	, do_echo		, 0, 0, SCMD_EMOTE },
	{ "enter"		, "enter"		, POS_STANDING	, do_enter		, 0, LOSE_HIDE, 0 },
	{ "equipment"	, "eq"			, POS_SLEEPING	, do_equipment	, 0, 0, 0 },
	{ "exits"		, "ex"			, POS_RESTING	, do_exits		, 0, 0, 0 },
	{ "examine"		, "exa"			, POS_SITTING	, do_examine	, 0, 0, 0 },

	{ "feedback"	, "feedback"	, POS_DEAD		, do_gen_write	, 0, 0, SCMD_FEEDBACK },
	{ "fill"		, "fill"		, POS_STANDING	, do_pour		, 0, 0, SCMD_FILL },
	{ "flee"		, "fl"			, POS_FIGHTING	, do_flee		, 0, LOSE_HIDE, 0 },
	{ "retreat"		, "retr"		, POS_FIGHTING	, do_retreat	, 0, LOSE_HIDE, 0 },
	{ "follow"		, "fol"			, POS_RESTING	, do_follow		, 0, 0, 0 },

	{ "get"			, "g"			, POS_RESTING	, do_get		, 0, 0, 0 },
	{ "give"		, "gi"			, POS_RESTING	, do_give		, 0, LOSE_HIDE, 0 },
	{ "gold"		, "gol"			, POS_RESTING	, do_gold		, 0, 0, 0 },
	{ "group"		, "gr"			, POS_RESTING	, do_group		, 0, 0, 0 },
	{ "grab"		, "grab"		, POS_RESTING	, do_grab		, 0, 0, 0 },
	{ "gsay"		, "gs"			, POS_SLEEPING	, do_gsay		, 0, 0, 0 },
	{ "gtell"		, "gt"			, POS_SLEEPING	, do_gsay		, 0, 0, 0 },

	{ "help"		, "h"			, POS_DEAD		, do_help		, 0, 0, 0 },
	{ "history"		, "his"			, POS_DEAD		, do_history	, 0, 0, 0 },
	{ "hit"			, "hit"			, POS_FIGHTING	, do_hit		, 0, 0, SCMD_HIT },
	{ "hold"		, "hold"		, POS_RESTING	, do_grab		, 0, 0, 0 },
	{ "house"		, "house"		, POS_RESTING	, do_house		, 0, 0, 0 },

	{ "idea"		, "idea"		, POS_DEAD		, do_gen_write	, 0, 0, SCMD_IDEA },
	{ "ignore"		, "ignore"		, POS_DEAD		, do_ignore		, 0, 0, SCMD_IGNORE },
	{ "immlist"		, "imm"			, POS_DEAD		, do_gen_ps		, 0, 0, SCMD_IMMLIST },
	{ "info"		, "inf"			, POS_DEAD		, do_gen_ps		, 0, 0, SCMD_INFO },
	{ "infohistory"	, "infoh"		, POS_DEAD		, do_infohistory, 0, 0, 0 },
	{ "install"		, "inst"		, POS_STANDING	, do_install	, 0, 0, 0 },
	{ "insult"		, "insult"		, POS_RESTING	, do_insult		, 0, LOSE_HIDE, 0 },
	{ "inventory"	, "i"			, POS_DEAD		, do_inventory	, 0, 0, 0 },

	{ "kill"		, "k"			, POS_FIGHTING	, do_kill		, 0, 0, 0 },
//	{ "kick"		, "kick"		, POS_FIGHTING	, do_kick_bite	, 0, 0, SCMD_KICK },

	{ "look"		, "l"			, POS_RESTING	, do_look		, 0, 0, SCMD_LOOK },
	{ "leave"		, "le"			, POS_STANDING	, do_leave		, 0, LOSE_HIDE, 0 },
	{ "levels"		, "lev"			, POS_DEAD		, do_levels		, 0, 0, 0 },
	{ "lock"		, "loc"			, POS_SITTING	, do_gen_door	, 0, LOSE_HIDE, SCMD_LOCK },

	{ "mission"		, "mission"		, POS_DEAD		, do_gen_tog	, 0, 0, SCMD_QUEST },
	{ "motd"		, "mot"			, POS_DEAD		, do_gen_ps		, 0, 0, SCMD_MOTD },
	{ "mount"		, "mou"			, POS_STANDING	, do_mount		, 0, LOSE_HIDE, 0 },
//	{ "mode"		, "mod"			, POS_STANDING	, do_mode		, 0, 0, 0 },
	{ "mp"			, "mp"			, POS_RESTING	, do_gold		, 0, 0, 0 },

	{ "news"		, "ne"			, POS_SLEEPING	, do_gen_ps		, 0, 0, SCMD_NEWS },
	{ "norace"		, "nora"		, POS_DEAD		, do_gen_tog	, 1, 0, SCMD_NORACE },
	{ "nomusic"		, "nomu"		, POS_DEAD		, do_gen_tog	, 1, 0, SCMD_NOMUSIC },
	{ "nochat"		, "noch"		, POS_DEAD		, do_gen_tog	, 1, 0, SCMD_NOGOSSIP },
	{ "nograts"		, "nogr"		, POS_DEAD		, do_gen_tog	, 1, 0, SCMD_NOGRATZ },
	{ "norepeat"	, "nore"		, POS_DEAD		, do_gen_tog	, 1, 0, SCMD_NOREPEAT },
	{ "noshout"		, "nosh"		, POS_DEAD		, do_gen_tog	, 1, 0, SCMD_DEAF },
	{ "nofollow"	, "nofo"		, POS_DEAD		, do_gen_tog	, 1, 0, SCMD_NOAUTOFOLLOW },
	{ "notell"		, "note"		, POS_DEAD		, do_gen_tog	, 1, 0, SCMD_NOTELL },
	{ "noinfo"		, "noin"		, POS_DEAD		, do_gen_tog	, 1, 0, SCMD_NOINFO },
	{ "nobitch"		, "nobi"		, POS_DEAD		, do_gen_tog	, 1, 0, SCMD_NOBITCH },
	{ "nonewbie"	, "nonew"		, POS_DEAD		, do_gen_tog	, 1, 0, SCMD_NONEWBIE },

	{ "order"		, "ord"			, POS_RESTING	, do_order		, 1, 0, 0 },
	{ "open"		, "o"			, POS_SITTING	, do_gen_door	, 0, LOSE_HIDE, SCMD_OPEN },

	{ "pagelength"	, "pagelen"		, POS_DEAD		, do_pagelength	, 1, 0, 0 },
	{ "pick"		, "pick"		, POS_STANDING	, do_gen_door	, 1, LOSE_HIDE, SCMD_PICK },
	{ "pkill"		, "pk"			, POS_DEAD		, do_pkill		, 1, 0, 0 },
	{ "policy"		, "pol"			, POS_DEAD		, do_gen_ps		, 0, 0, SCMD_POLICIES },
	{ "policies"	, "polici"		, POS_DEAD		, do_gen_ps		, 0, 0, SCMD_POLICIES },
	{ "pour"		, "pour"		, POS_STANDING	, do_pour		, 0, 0, SCMD_POUR },
	{ "prompt"		, "prompt"		, POS_DEAD		, do_prompt		, 0, 0, 0 },
	{ "practice"	, "pr"			, POS_RESTING	, do_practice	, 1, 0, 0 },
	{ "promote"		, "promote"		, POS_RESTING	, do_promote	, 1, 0, 0 },
	{ "pull"		, "pull"		, POS_RESTING	, do_pull		, 0, 0, 0 },
	{ "prime"		, "pri"			, POS_RESTING	, do_prime		, 0, 0, 0 },
//	{ "push"		, "push"		, POS_STANDING	, do_push_drag	, 0, LOSE_HIDE, SCMD_PUSH },
	{ "put"			, "p"			, POS_RESTING	, do_put		, 0, 0, 0 },

	{ "quit"		, "quit"		, POS_SLEEPING	, do_quit		, 0, 0, 0 },

	{ "recall"		, "rec"			, POS_SLEEPING	, do_recall		, 0, LOSE_HIDE, 0 },
	{ "repair"		, "repa"		, POS_STANDING	, do_repair		, 0, LOSE_HIDE, 0 },
	{ "reply"		, "r"			, POS_DEAD	, do_reply		, 0, 0, 0 },
	{ "rest"		, "re"			, POS_RESTING	, do_rest		, 0, LOSE_HIDE, 0 },
	{ "respond"		, "resp"	    , POS_RESTING	, do_respond	, 0, 0, 0 },
	{ "read"		, "read"		, POS_RESTING	, do_look		, 0, 0, SCMD_READ },
	{ "reload"		, "rel"			, POS_RESTING	, do_reload		, 0, 0, SCMD_LOAD },
	{ "remove"		, "rem"			, POS_RESTING	, do_remove		, 0, 0, 0 },
	{ "report"		, "rep"			, POS_RESTING	, do_report		, 0, 0, 0 },
	{ "rescue"		, "resc"		, POS_FIGHTING	, do_rescue		, 0, 0, 0 },
	{ "retell"		, "rete"		, POS_SLEEPING	, do_retell		, 0, 0, 0 },
	{ "retreat"		, "retr"		, POS_FIGHTING	, do_retreat	, 0, LOSE_HIDE, 0 },
	{ "return"		, "return"		, POS_DEAD		, do_return		, 0, 0, 0 },

	{ "say"			, "sa"			, POS_STUNNED	, do_say		, 0, LOSE_HIDE, 0 },
	{ "'"			, "'"			, POS_STUNNED	, do_say		, 0, LOSE_HIDE, 0 },
	{ "sayhistory"	, "sayh"		, POS_DEAD		, do_sayhistory	, 0, 0, 0 },
	{ "save"		, "sav"			, POS_STUNNED	, do_save		, 0, 0, 0 },
	{ "scan"		, "scan"		, POS_RESTING	, do_scan		, 0, 0, 0 },
	{ "score"		, "sc"			, POS_DEAD		, do_score		, 0, 0, SCMD_SCORE },
	{ "shoot"		, "sh"			, POS_SITTING	, do_shoot		, 0, 0, 0},
	{ "sip"			, "sip"			, POS_RESTING	, do_drink		, 0, 0, SCMD_SIP },
	{ "sit"			, "si"			, POS_RESTING	, do_sit		, 0, 0, 0 },
	{ "sleep"		, "sl"			, POS_SLEEPING	, do_sleep		, 0, LOSE_HIDE, 0 },
//	{ "sneak"		, "sn"			, POS_STANDING	, do_sneak		, 1, 0, 0 },
	{ "socials"		, "soc"			, POS_DEAD		, do_commands	, 0, 0, SCMD_SOCIALS },
	{ "spray"		, "sp"			, POS_SITTING	, do_spray		, 0, 0, 0},
	{ "stand"		, "st"			, POS_RESTING	, do_stand		, 0, 0, 0 },

	{ "tell"		, "t"			, POS_DEAD		, do_tell		, 0, 0, 0 },
	{ "take"		, "ta"			, POS_RESTING	, do_get		, 0, 0, 0 },
	{ "taste"		, "tas"			, POS_RESTING	, do_eat		, 0, 0, SCMD_TASTE },
	{ "throw"		, "th"			, POS_RESTING	, do_throw		, 0, 0, 0 },
	{ "time"		, "ti"			, POS_DEAD		, do_time		, 0, 0, 0 },
	{ "toggle"		, "to"			, POS_DEAD		, do_toggle		, 0, 0, 0 },
	{ "track"		, "tr"			, POS_STANDING	, do_track		, 0, 0, 0 },
	{ "typo"		, "typo"		, POS_DEAD		, do_gen_write	, 0, 0, SCMD_TYPO },

	{ "tsay"		, "ts"			, POS_DEAD		, do_gen_comm	, 0, 0, SCMD_TEAM },

	{ "unload"		, "unloa"		, POS_RESTING	, do_reload		, 0, 0, SCMD_UNLOAD },
	{ "unlock"		, "unl"			, POS_SITTING	, do_gen_door	, 0, LOSE_HIDE, SCMD_UNLOCK },
	{ "unmount"		, "unm"			, POS_SITTING	, do_unmount	, 0, LOSE_HIDE, 0 },
	{ "ungroup"		, "ung"			, POS_DEAD		, do_ungroup	, 0, 0, 0 },
	{ "unignore"	, "unignore"	, POS_DEAD		, do_ignore		, 0, 0, SCMD_UNIGNORE },

	{ "version"		, "ver"			, POS_DEAD		, do_gen_ps		, 0, 0, SCMD_VERSION },
	{ "visible"		, "vi"			, POS_RESTING	, do_visible	, 1, 0, 0 },
	{ "vote"		, "vo"			, POS_DEAD		, do_vote		, 0, 0, 0 },
	
	{ "wake"		, "wa"			, POS_SLEEPING	, do_wake		, 0, 0, 0 },
	{ "watch"		, "wat"			, POS_RESTING	, do_watch		, 0, 0, 0 },
	{ "wear"		, "wea"			, POS_RESTING	, do_wear		, 0, 0, 0 },
	{ "weather"		, "weath"		, POS_RESTING	, do_weather	, 0, 0, 0 },
	{ "who"			, "wh"			, POS_DEAD		, do_who		, 0, 0, 0 },
	{ "whoami"		, "whoa"		, POS_DEAD		, do_gen_ps		, 0, 0, SCMD_WHOAMI },
	{ "whisper"		, "whi"			, POS_STUNNED	, do_spec_comm	, 0, 0, SCMD_WHISPER },
	{ "wield"		, "wi"			, POS_RESTING	, do_wield		, 0, 0, 0 },
//	{ "wimpy"		, "wim"			, POS_DEAD		, do_wimpy		, 0, 0, 0 },
	{ "wizlist"		, "wizl"		, POS_DEAD		, do_gen_ps		, 0, 0, SCMD_WIZLIST },
	{ "write"		, "wr"			, POS_RESTING	, do_write		, 0, 0, 0 },

	{ "?"			, "?"			, POS_DEAD		, do_help		, 0, 0, 0 },

	//	Clan commands
	{ "apply"		, "app"			, POS_DEAD		, do_apply		, 0, 0, 0 },
	{ "resign"		, "resign"		, POS_DEAD		, do_resign		, 0, 0, 0 },
	{ "enlist"		, "enl"			, POS_DEAD		, do_enlist		, 0, 0, 0 },
	{ "boot"		, "boot"		, POS_DEAD		, do_boot		, 0, 0, 0 },
	{ "cban"		, "cban"		, POS_DEAD		, do_clanban	, 0, 0, 0 },
	{ "clans"		, "clans"		, POS_DEAD		, do_clanlist	, 0, 0, 0 },
//	{ "members"		, "mem"			, POS_DEAD		, do_members	, 0, 0, 0 },
	{ "cwho"		, "cwho"		, POS_DEAD		, do_clanwho	, 0, 0, 0 },
	{ "cpromote"	, "cpro"		, POS_DEAD		, do_clanpromote, 0, 0, 0 },
	{ "cdemote"		, "cdem"		, POS_DEAD		, do_clandemote	, 0, 0, 0 },
	{ "callow"		, "callow"		, POS_DEAD		, do_clanallow	, 0, 0, SCMD_ALLOW },
	{ "cdeny"		, "cdeny"		, POS_DEAD		, do_clanallow	, 0, 0, SCMD_DENY },
	{ "deposit"		, "depo"		, POS_DEAD		, do_deposit	, 0, 0, 0 },
//	{ "withdraw"	, "withd"		, POS_DEAD		, do_withdraw	, 0, 0, 0 },
	{ "war"			, "war"			, POS_DEAD		, do_war		, 0, 0, 0 },
	{ "ally"		, "ally"		, POS_DEAD		, do_ally		, 0, 0, 0 },
	{ "politics"	, "politics"	, POS_DEAD		, do_clanpolitics, 0, 0, 0 },
	{ "clanrank"	, "clanrank"	, POS_DEAD		, do_clanrank	, 0, 0, 0 },
	{ "clanset"		, "clanset"		, POS_DEAD		, do_clanset	, 0, 0, 0 },
	{ "cmotd"		, "cmotd"		, POS_DEAD		, do_cmotd		, 0, 0, 0 },

	//	IMC commands
/*
	{ "rtell"		, "rt"			, POS_SLEEPING	, do_rtell		, 6, 0, 0 },
	{ "rreply"		, "rr"			, POS_SLEEPING	, do_rreply		, 6, 0, 0 },
	{ "rwho"		, "rw"			, POS_SLEEPING	, do_rwho		, 6, 0, 0 },
	{ "rwhois"		, "rwhoi"		, POS_SLEEPING	, do_rwhois		, 6, 0, 0 },
	{ "rquery"		, "rq"			, POS_SLEEPING	, do_rquery		, 6, 0, 0 },
	{ "rfinger"		, "rf"			, POS_SLEEPING	, do_rfinger	, 6, 0, 0 },
	{ "imc"			, "im"			, POS_DEAD		, do_imc		, STAFF_CMD, STAFF_IMC, 0 },
	{ "imclist"		, "imcl"		, POS_DEAD		, do_imclist	, 6, 0, 0 },
	{ "rbeep"		, "rb"			, POS_SLEEPING	, do_rbeep		, 6, 0, 0 },
	{ "istats"		, "ist"			, POS_DEAD		, do_istats		, STAFF_CMD, STAFF_IMC, 0 },
	{ "rchannels"	, "rch"			, POS_DEAD		, do_rchannels	, 6, 0, 0 },

	{ "rinfo"		, "rin"			, POS_DEAD		, do_rinfo		, 6, 0, 0 },
	{ "rsockets"	, "rso"			, POS_DEAD		, do_rsockets	, STAFF_CMD, STAFF_IMC, 0 },
	{ "rconnect"	, "rc"			, POS_DEAD		, do_rconnect	, STAFF_CMD, STAFF_IMC, 0 },
	{ "rdisconnect"	, "rd"			, POS_DEAD		, do_rdisconnect, STAFF_CMD, STAFF_IMC, 0 },
	{ "rignore"		, "rig"			, POS_DEAD		, do_rignore	, STAFF_CMD, STAFF_IMC, 0 },
	{ "rchanset"	, "rchans"		, POS_DEAD		, do_rchanset	, STAFF_CMD, STAFF_IMC, 0 },
//	{ "mailqueue"	, "mailq"		, POS_DEAD		, do_mailqueue	, STAFF_CMD, STAFF_IMC, 0 },

	{ "icommand"	, "ico"			, POS_DEAD		, do_icommand	, STAFF_CMD, STAFF_IMC, 0 },
	{ "isetup"		, "ise"			, POS_DEAD		, do_isetup		, STAFF_CMD, STAFF_IMC, 0 },
	{ "ilist"		, "il"			, POS_DEAD		, do_ilist		, 6, 0, 0 },
	{ "ichannels"	, "ich"			, POS_DEAD		, do_ichannels	, 6, 0, 0 },
	{ "idebug"		, "idebug"		, POS_DEAD		, do_idebug		, STAFF_CMD, STAFF_IMC, 0 },

	{ "rping"		, "rp"			, POS_DEAD		, do_rping		, STAFF_CMD, STAFF_IMC, 0 },
*/
	
	//	Assistance commands
	{ "wizcall"		, "wizc"		, POS_DEAD		, do_wizcall	, 0, 0, 0 },
	{ "wizassist"	, "wiza"		, POS_DEAD		, do_wizassist	, STAFF_CMD, STAFF_GEN, 0 },
	
//	{ "openhatch"	, "openh"		, POS_RESTING	, do_openhatch	, 1, 0, SCMD_OPEN },
//	{ "closehatch"	, "closeh"		, POS_RESTING	, do_openhatch	, 1, 0, SCMD_CLOSE },
//	{ "boardship"	, "boa"			, POS_RESTING	, do_boardship	, 1, 0, 0 },
//	{ "land"		, "lan"			, POS_RESTING	, do_land		, 1, 0, 0 },
//	{ "launch"		, "lau"			, POS_RESTING	, do_launch		, 1, 0, 0 },
//	{ "gate"		, "gate"		, POS_RESTING	, do_gate		, 1, 0, 0 },
//	{ "radar"		, "rad"			, POS_RESTING	, do_radar		, 1, 0, 0 },
//	{ "accelerate"	, "accel"		, POS_RESTING	, do_accelerate	, 1, 0, 0 },
//	{ "course"		, "course"		, POS_RESTING	, do_course		, 1, 0, 0 },
//	{ "status"		, "status"		, POS_RESTING	, do_shipstat	, 1, 0, 0 },
//	{ "calculate"	, "calc"		, POS_RESTING	, do_calculate	, 1, 0, 0 },
//	{ "hyperspace"	, "hyper"		, POS_RESTING	, do_hyperspace	, 1, 0, 0 },
//	{ "scanplanet"	, "scanp"		, POS_RESTING	, do_scanplanet	, 1, 0, 0 },
//	{ "scanship"	, "scans"		, POS_RESTING	, do_scanship	, 1, 0, 0 },
//	{ "followship"	, "follows"		, POS_RESTING	, do_followship	, 1, 0, 0 },
//	{ "target"		, "target"		, POS_RESTING	, do_target		, 1, 0, 0 },
//	{ "autopilot"	, "autop"		, POS_RESTING	, do_gen_autoship, 1, 0, SCMD_AUTOPILOT },
//	{ "autotrack"	, "autot"		, POS_RESTING	, do_gen_autoship, 1, 0, SCMD_AUTOTRACK },
//	{ "shipcloak"	, "shipcl"		, POS_RESTING	, do_shipcloak	, 1, 0, 0 },
//	{ "baydoors"	, "bay"			, POS_RESTING	, do_baydoors	, 1, 0, 0 },
//	{ "buyship"		, "buys"		, POS_RESTING	, do_buyship	, LVL_STAFF, 0, SCMD_BUYSHIP },
//	{ "clanbuyship"	, "clanbuy"		, POS_RESTING	, do_buyship	, LVL_STAFF, 0, SCMD_CLANSHIP },
//	{ "svlist"		, "svlist"		, POS_RESTING	, do_svlist		, LVL_STAFF, 0, 0 },
//	{ "stlist"		, "stlist"		, POS_RESTING	, do_stlist		, LVL_STAFF, 0, 0 },
//	{ "sylist"		, "sylist"		, POS_RESTING	, do_sylist		, LVL_STAFF, 0, 0 },
//	{ "shiplist"	, "shiplist"	, POS_RESTING	, do_shiplist	, 1, 0, 0 },
//	{ "shiprecall"	, "shiprecall"	, POS_RESTING	, do_shiprecall	, 1, 0, 0 },
//	{ "fire"		, "fire"		, POS_RESTING	, do_shipfire	, 1, 0, 0 },
//	{ "repairship"	, "repairship"	, POS_RESTING	, do_repairship	, 1, 0, 0 },
//	{ "shipupgrade"	, "shipupgrade"	, POS_RESTING	, do_shipupgrade, 1, 0, 0 },
//	{ "tranship"	, "transh"		, POS_RESTING	, do_tranship	, LVL_STAFF, 0, 0 },
//	{ "addpilot"	, "addpilot"	, POS_RESTING	, do_addpilot	, 1, 0, 0 },
//	{ "rempilot"	, "rempilot"	, POS_RESTING	, do_rempilot	, 1, 0, 0 },
//	{ "stedit"		, "stedit"		, POS_RESTING	, do_stedit		, LVL_STAFF, 0, 0 },
//	{ "setship"		, "setship"		, POS_RESTING	, do_setship	, LVL_STAFF, 0, 0 },
//	{ "sabotage"	, "sabotage"	, POS_RESTING	, do_sabotage	, 1, 0, 0 },
//	{ "ping"		, "ping"		, POS_RESTING	, do_ping		, 1, 0, 0 },
//	{ "interceptor"	, "interceptor"	, POS_RESTING	, do_interceptor, 1, 0, 0 },
//	{ "deploy"		, "deploy"		, POS_RESTING	, do_deploy		, 1, 0, 0 },
//	{ "remote"		, "remote"		, POS_RESTING	, do_remote		, 1, 0, 0 },
//	{ "jumpvector"	, "jumpvector"	, POS_RESTING	, do_jumpvector	, 1, 0, 0 },
//	{ "forge"		, "forge"		, POS_RESTING	, do_forge		, 1, 0, 0 },
//	{ "foldspace"	, "foldspace"	, POS_RESTING	, do_foldspace	, 1, 0, 0 },
//	{ "wormhole"	, "wormhole"	, POS_RESTING	, do_wormhole	, 1, 0, 0 },
//	{ "disruptionweb", "disruptionweb", POS_RESTING	, do_disruptionweb, 1, 0, 0 },
//	{ "shieldmatrix", "shieldmatrix", POS_RESTING	, do_shieldmatrix, 1, 0, 0 },
//	{ "orbitalinsertion", "orbitalinsertion", POS_RESTING, do_orbitalinsertion, 1, 0, 0 },

  /* Staff Commands */
	{ "allow"		, "allow"		, POS_DEAD		, do_allow		, LVL_IMMORT, 0, SCMD_ALLOW },
	{ "deny"		, "deny"		, POS_DEAD		, do_allow		, LVL_IMMORT, 0, SCMD_DENY },

	{ "stopscripts"	, "stopscripts"	, POS_DEAD		, do_stopscripts, LVL_CODER	, STAFF_ADMIN, 0 },
//	{ "setcapture"	, "setcapture"	, POS_DEAD		, do_setcapture	, LVL_IMMORT, STAFF_GAME, 0 },
	
//	{ "doas"		, "doas"		, POS_DEAD		, do_as			, STAFF_CMD	, STAFF_SECURITY, 0 },
	{ "date"		, "date"		, POS_DEAD		, do_date		, 1, 0, SCMD_DATE },
	{ "echo"		, "echo"		, POS_DEAD		, do_echo		, STAFF_CMD	, STAFF_GEN, SCMD_ECHO },
	{ "force"		, "f"			, POS_DEAD		, do_force		, STAFF_CMD	, STAFF_SECURITY | STAFF_GAME | STAFF_MINISEC, 0 },
	{ "handbook"	, "hand"		, POS_DEAD		, do_gen_ps		, LVL_IMMORT, 0, SCMD_HANDBOOK },
	{ "imotd"		, "im"			, POS_DEAD		, do_gen_ps		, LVL_IMMORT, 0, SCMD_IMOTD },
	{ "goto"		, "got"			, POS_DEAD		, do_goto		, STAFF_CMD	, STAFF_GEN, 0 },
	{ "holylight"	, "holy"		, POS_DEAD		, do_gen_tog	, STAFF_CMD	, STAFF_GEN, SCMD_HOLYLIGHT },
	{ "last"		, "last"		, POS_DEAD		, do_last		, 1			, 0, 0 },
	{ "mecho"		, "mecho"		, POS_DEAD		, do_qcomm		, STAFF_CMD	, STAFF_CHAR, SCMD_QECHO },
	{ "nohassle"	, "nohass"		, POS_DEAD		, do_gen_tog	, STAFF_CMD	, STAFF_GEN, SCMD_NOHASSLE },
	{ "nowiz"		, "nowiz"		, POS_DEAD		, do_gen_tog	, STAFF_CMD	, STAFF_GEN, SCMD_NOWIZ },
	{ "poofin"		, "poofi"		, POS_DEAD		, do_poofset	, STAFF_CMD	, STAFF_GEN, SCMD_POOFIN },
	{ "poofout"		, "poofo"		, POS_DEAD		, do_poofset	, STAFF_CMD	, STAFF_GEN, SCMD_POOFOUT },
	{ "poll"		, "poll"		, POS_DEAD		, do_poll		, STAFF_CMD	, STAFF_ADMIN, 0 },
	{ "send"		, "send"		, POS_DEAD		, do_send		, STAFF_CMD	, STAFF_GEN, 0 },
	{ "show"		, "show"		, POS_DEAD		, do_show		, STAFF_CMD	, STAFF_GEN, 0 },
	{ "title"		, "title"		, POS_DEAD		, do_title		, LVL_IMMORT, 0, 0 },
	{ "team"		, "team"		, POS_DEAD		, do_team		, STAFF_CMD, STAFF_GEN, SCMD_TYPO },
	{ "teleport"	, "tele"		, POS_DEAD		, do_teleport	, STAFF_CMD	, STAFF_GEN, 0 },
	{ "transfer"	, "tran"		, POS_DEAD		, do_trans		, STAFF_CMD	, STAFF_GEN, 0 },
	{ "uptime"		, "upt"			, POS_DEAD		, do_date		, 1, 0, SCMD_UPTIME },
	{ "where"		, "where"		, POS_DEAD		, do_where		, LVL_IMMORT, 0, 0 },
	{ "wizhelp"		, "wizhelp"		, POS_DEAD		, do_commands	, LVL_IMMORT, 0, SCMD_WIZHELP },
	{ "wiznet"		, "wiz"			, POS_DEAD		, do_wiznet		, STAFF_CMD	, STAFF_GEN, 0 },
	{ ";"			, ";"			, POS_DEAD		, do_wiznet		, STAFF_CMD	, STAFF_GEN, 0 },

	{ "listhelp"	, "listhelp"	, POS_DEAD		, do_listhelp	, STAFF_CMD	, STAFF_HELP, 0 },

	{ "load"		, "load"		, POS_DEAD		, do_load		, STAFF_CMD	, STAFF_GEN, 0 },
	{ "invis"		, "invis"		, POS_DEAD		, do_invis		, STAFF_CMD	, STAFF_GEN, 0 },
	{ "mute"		, "mu"			, POS_DEAD		, do_wizutil	, STAFF_CMD	, STAFF_SECURITY | STAFF_MINISEC, SCMD_SQUELCH },
	{ "page"		, "pa"			, POS_DEAD		, do_page		, STAFF_CMD	, STAFF_GEN, 0 },
	{ "purge"		, "purge"		, POS_DEAD		, do_purge		, STAFF_CMD	, STAFF_SECURITY | STAFF_OLC, 0 },
	{ "restore"		, "restore"		, POS_DEAD		, do_restore	, STAFF_CMD	, STAFF_CHAR, 0 },
	{ "reward"		, "reward"		, POS_DEAD		, do_reward		, STAFF_CMD	, STAFF_CHAR, SCMD_REWARD },
	{ "roomflags"	, "roomfl"		, POS_DEAD		, do_gen_tog	, STAFF_CMD	, STAFF_GEN, SCMD_ROOMFLAGS },
	{ "salary"		, "salary"		, POS_DEAD		, do_reward		, STAFF_CMD	, STAFF_ADMIN, SCMD_SALARY },
	{ "set"			, "set"			, POS_DEAD		, do_set		, STAFF_CMD	, STAFF_CHAR, 0 },
	{ "snoop"		, "snoop"		, POS_DEAD		, do_snoop		, STAFF_CMD	, STAFF_SECURITY | STAFF_SRSTAFF | STAFF_ADMIN | STAFF_MINISEC, 0 },
	{ "stat"		, "stat"		, POS_DEAD		, do_stat		, STAFF_CMD	, STAFF_GEN, SCMD_STAT },
	{ "sstat"		, "sstat"		, POS_DEAD		, do_stat		, STAFF_CMD	, STAFF_GEN, SCMD_SSTAT },
	{ "string"		, "string"		, POS_DEAD		, do_string		, STAFF_CMD	, STAFF_CHAR, 0 },
	{ "syslog"		, "sys"			, POS_DEAD		, do_syslog		, STAFF_CMD	, STAFF_GEN, 0 },
	{ "unaffect"	, "unaff"		, POS_DEAD		, do_wizutil	, STAFF_CMD	, STAFF_CHAR, SCMD_UNAFFECT },
	{ "users"		, "us"			, POS_DEAD		, do_users		, STAFF_CMD	, STAFF_GEN, 0 },
	{ "vnum"		, "vn"			, POS_DEAD		, do_vnum		, STAFF_CMD	, STAFF_GEN, 0 },
	{ "vstat"		, "vst"			, POS_DEAD		, do_vstat		, STAFF_CMD	, STAFF_GEN, 0 },
	{ "vwear"		, "vwear"		, POS_DEAD		, do_vwear		, STAFF_CMD	, STAFF_GEN, 0 },
	{ "zreset"		, "zreset"		, POS_DEAD		, do_zreset		, STAFF_CMD	, STAFF_GEN, 0 },
	{ "zvnum"		, "zvn"			, POS_DEAD		, do_vnum		, STAFF_CMD	, STAFF_GEN, SCMD_ZVNUM },

	{ "attach"		, "attach"		, POS_DEAD		, do_attach		, STAFF_CMD	, STAFF_SCRIPT, 0 },
	{ "detach"		, "detach"		, POS_DEAD		, do_detach		, STAFF_CMD	, STAFF_SCRIPT, 0 },
	{ "gscript"		, "gscript"		, POS_DEAD		, do_gscript	, STAFF_CMD	, STAFF_SRSTAFF | STAFF_ADMIN, 0 },
	
	{ "ban"			, "ban"			, POS_DEAD		, do_ban		, STAFF_CMD	, STAFF_SECURITY | STAFF_MINISEC, 0 },
	{ "dc"			, "dc"			, POS_DEAD		, do_dc			, STAFF_CMD	, STAFF_SECURITY, 0 },
	{ "depiss"		, "depiss"		, POS_DEAD		, do_depiss		, STAFF_CMD	, STAFF_GAME, 0 },
	{ "gecho"		, "gecho"		, POS_DEAD		, do_gecho		, STAFF_CMD	, STAFF_GAME, 0 },
	{ "hunt"		, "hunt"		, POS_DEAD		, do_hunt		, STAFF_CMD	, STAFF_GAME, 0 },
	{ "notitle"		, "notitle"		, POS_DEAD		, do_wizutil	, STAFF_CMD	, STAFF_SECURITY, SCMD_NOTITLE },
	{ "pardon"		, "pard"		, POS_DEAD		, do_wizutil	, STAFF_CMD	, STAFF_CHAR, SCMD_PARDON },
	{ "repiss"		, "repiss"		, POS_DEAD		, do_repiss		, STAFF_CMD	, STAFF_GAME, 0 },
//	{ "reroll"		, "reroll"		, POS_DEAD		, do_wizutil	, STAFF_CMD	, STAFF_CHAR, SCMD_REROLL },
	{ "skillset"	, "skillset"	, POS_DEAD		, do_skillset	, STAFF_CMD	, STAFF_CHAR, 0 },
	{ "switch"		, "swit"		, POS_DEAD		, do_switch		, STAFF_CMD	, STAFF_GAME, 0 },
	{ "unban"		, "unban"		, POS_DEAD		, do_unban		, STAFF_CMD	, STAFF_SECURITY | STAFF_MINISEC, 0 },

	{ "advance"		, "ad"			, POS_DEAD		, do_advance	, STAFF_CMD	, STAFF_SRSTAFF | STAFF_ADMIN, 0 },
	{ "copyover"	, "copyover"	, POS_DEAD		, do_copyover	, STAFF_CMD	, STAFF_ADMIN, 0 },
	{ "houseedit"	, "houseedit"	, POS_DEAD		, do_houseedit	, STAFF_CMD	, STAFF_GEN, 0 },
	{ "shutdown"	, "shutdown"	, POS_DEAD		, do_shutdown	, STAFF_CMD	, STAFF_ADMIN, 0 },
	{ "slowns"		, "slowns"		, POS_DEAD		, do_gen_tog	, STAFF_CMD	, STAFF_ADMIN, SCMD_SLOWNS },
	{ "reboot"		, "reboot"		, POS_DEAD		, do_reboot		, STAFF_CMD	, STAFF_ADMIN, 0 },
	{ "wizlock"		, "wizlock"		, POS_DEAD		, do_wizlock	, STAFF_CMD	, STAFF_ADMIN, 0 },
	{ "zallow"		, "zallow"		, POS_DEAD		, do_zallow		, STAFF_CMD	, STAFF_OLC, 0 },
	{ "zdeny"		, "zdeny"		, POS_DEAD		, do_zdeny		, STAFF_CMD	, STAFF_OLC, 0 },

	{ "buffer"		, "buffer"		, POS_DEAD		, do_buffer		, LVL_CODER	, 0, 0 },
	
	{ "path"		, "path"		, POS_DEAD		, do_path		, LVL_IMMORT, 0, 0 },

	{ "freeze"		, "fr"			, POS_DEAD		, do_wizutil	, STAFF_CMD	, STAFF_SECURITY | STAFF_MINISEC, SCMD_FREEZE },
	{ "thaw"		, "thaw"		, POS_DEAD		, do_wizutil	, STAFF_CMD	, STAFF_SECURITY | STAFF_MINISEC, SCMD_THAW },

	//	OLC Commands
//	{ "olc"			, "olc"			, POS_DEAD		, do_olc		, STAFF_CMD	, STAFF_OLC | STAFF_MINIOLC, SCMD_OLC_SAVEINFO },
	{ "dig"			, "dig"			, POS_DEAD		, do_dig		, STAFF_CMD	, STAFF_OLC, 0 },

	{ "aedit"		, "aed"			, POS_DEAD		, do_olc_other	, STAFF_CMD	, STAFF_SOCIALS, SCMD_OLC_AEDIT},
	{ "behavioredit", "behavioredit", POS_DEAD		, do_olc_other	, STAFF_CMD	, STAFF_OLCADMIN, SCMD_OLC_BEDIT},
	{ "cedit"		, "ced"			, POS_DEAD		, do_olc_other	, STAFF_CMD	, STAFF_CLANS, SCMD_OLC_CEDIT},
	{ "hedit"		, "hed"			, POS_DEAD		, do_olc_other	, STAFF_CMD	, STAFF_HELP, SCMD_OLC_HEDIT},
	{ "medit"		, "med"			, POS_DEAD		, do_olc		, STAFF_CMD	, STAFF_OLC, SCMD_OLC_MEDIT},
	{ "oedit"		, "oed"			, POS_DEAD		, do_olc		, STAFF_CMD	, STAFF_OLC, SCMD_OLC_OEDIT},
	{ "redit"		, "red"			, POS_DEAD		, do_olc		, STAFF_CMD	, STAFF_OLC | STAFF_MINIOLC, SCMD_OLC_REDIT},
	{ "sedit"		, "sed"			, POS_DEAD		, do_olc		, STAFF_CMD	, STAFF_SHOPS, SCMD_OLC_SEDIT},
	{ "tedit"		, "ted"			, POS_DEAD		, do_tedit		, STAFF_CMD	, STAFF_SRSTAFF | STAFF_ADMIN, 0 },
	{ "zedit"		, "zed"			, POS_DEAD		, do_olc		, STAFF_CMD	, STAFF_OLC, SCMD_OLC_ZEDIT},
	{ "scriptedit"	, "scripted"	, POS_DEAD		, do_olc		, STAFF_CMD	, STAFF_SCRIPT, SCMD_OLC_SCRIPTEDIT},

	{ "mcopy"		, "mcopy"		, POS_DEAD		, do_olccopy	, STAFF_CMD	, STAFF_OLC, SCMD_OLC_MEDIT},
	{ "ocopy"		, "ocopy"		, POS_DEAD		, do_olccopy	, STAFF_CMD	, STAFF_OLC, SCMD_OLC_OEDIT},
	{ "rcopy"		, "rcopy"		, POS_DEAD		, do_olccopy	, STAFF_CMD	, STAFF_OLC, SCMD_OLC_REDIT},
	{ "scriptcopy"	, "scriptcopy"	, POS_DEAD		, do_olccopy	, STAFF_CMD	, STAFF_SCRIPT, SCMD_OLC_SCRIPTEDIT},

	{ "mmove"		, "mmove"		, POS_DEAD		, do_olcmove	, STAFF_CMD	, STAFF_OLC, SCMD_OLC_MEDIT},
	{ "omove"		, "omove"		, POS_DEAD		, do_olcmove	, STAFF_CMD	, STAFF_OLC, SCMD_OLC_OEDIT},
	{ "rmove"		, "rmove"		, POS_DEAD		, do_olcmove	, STAFF_CMD	, STAFF_OLC, SCMD_OLC_REDIT},
	{ "scriptmove"	, "scriptmove"	, POS_DEAD		, do_olcmove	, STAFF_CMD	, STAFF_OLC, SCMD_OLC_SCRIPTEDIT},
	
	{ "mstat"		, "mstat"		, POS_DEAD		, do_mstat		, STAFF_CMD	, STAFF_GEN, 0 },
	{ "ostat"		, "ostat"		, POS_DEAD		, do_ostat		, STAFF_CMD	, STAFF_GEN, 0 },
	{ "rstat"		, "rstat"		, POS_DEAD		, do_rstat		, STAFF_CMD	, STAFF_GEN, 0 },
	{ "zstat"		, "zstat"		, POS_DEAD		, do_zstat		, STAFF_CMD	, STAFF_GEN, 0 },
	{ "zlist"		, "zlist"		, POS_DEAD		, do_zlist		, STAFF_CMD	, STAFF_GEN | STAFF_MINIOLC, SCMD_ZLIST },
	{ "mlist"		, "mlist"		, POS_DEAD		, do_zlist		, STAFF_CMD	, STAFF_GEN, SCMD_MLIST },
	{ "olist"		, "olist"		, POS_DEAD		, do_zlist		, STAFF_CMD	, STAFF_GEN, SCMD_OLIST },
	{ "rlist"		, "rlist"		, POS_DEAD		, do_zlist		, STAFF_CMD	, STAFF_GEN, SCMD_RLIST },
	{ "tlist"		, "tlist"		, POS_DEAD		, do_zlist		, STAFF_CMD	, STAFF_GEN, SCMD_TLIST },
//	{ "mlist"		, "mlist"		, POS_DEAD		, do_liblist	, STAFF_CMD	, STAFF_GEN, SCMD_MLIST },
//	{ "olist"		, "olist"		, POS_DEAD		, do_liblist	, STAFF_CMD	, STAFF_GEN, SCMD_OLIST },
//	{ "rlist"		, "rlist"		, POS_DEAD		, do_liblist	, STAFF_CMD	, STAFF_GEN, SCMD_RLIST },
//	{ "tlist"		, "tlist"		, POS_DEAD		, do_liblist	, STAFF_CMD	, STAFF_GEN, SCMD_TLIST },
//	{ "mmove"		, "mmove"		, POS_DEAD		, do_move_element, STAFF_CMD, STAFF_OLC, SCMD_MMOVE },
//	{ "omove"		, "omove"		, POS_DEAD		, do_move_element, STAFF_CMD, STAFF_OLC, SCMD_OMOVE },
//	{ "rmove"		, "rmove"		, POS_DEAD		, do_move_element, STAFF_CMD, STAFF_OLC, SCMD_RMOVE },
//	{ "tmove"		, "tmove"		, POS_DEAD		, do_move_element, STAFF_CMD, STAFF_OLC, SCMD_TMOVE },
	
	{ "mindex"		, "mindex"		, POS_DEAD		, do_mobindex	, STAFF_CMD	, STAFF_OLC, 0 },
	{ "oindex"		, "oindex"		, POS_DEAD		, do_objindex	, STAFF_CMD	, STAFF_OLC, 0 },
	{ "tindex"		, "tindex"		, POS_DEAD		, do_trigindex	, STAFF_CMD	, STAFF_OLC, 0 },
	
	{ "vattached"	, "vattach"		, POS_DEAD		, do_vattached	, STAFF_CMD , STAFF_GEN, 0 },
	
	{ "delete"		, "delete"		, POS_DEAD		, do_delete		, STAFF_CMD	, STAFF_OLC, SCMD_DELETE },
	{ "undelete"	, "undelete"	, POS_DEAD		, do_delete		, STAFF_CMD	, STAFF_OLC, SCMD_UNDELETE },

	{ "clanstat"	, "clanstat"	, POS_DEAD		, do_clanstat	, 0			, 0, 0 },
	{ "forceenlist"	, "forceen"		, POS_DEAD		, do_forceenlist, STAFF_CMD	, STAFF_CLANS, 0 },

	{ "massolcsave"	, "massolcsave"	, POS_DEAD		, do_massolcsave, LVL_CODER, 0, 0 },
	{ "killall"		, "killall"		, POS_DEAD		, do_killall	, STAFF_CMD	, STAFF_SRSTAFF | STAFF_ADMIN, 0 },
//	{ "performancetest", "performancetest", POS_DEAD, do_performancetest, LVL_CODER, 0, 0 },

//	{ "crash", "crash", POS_DEAD, do_crash, LVL_CODER, 0, 0 },
#if __profile__
	{ "profile"		, "profile"		, POS_DEAD		, do_profile	, LVL_CODER, 0, 0 },
#endif
	{ "\n"			, "zzzzzzz"	,0, 0, 0, 0 }	//	This must be last
};


char *fill[] =
{
  "in"	,
  "from",
  "with",
  "the"	,
  "on"	,
  "at"	,
  "to"	,
  "\n"
};

char *reserved[] =
{
  "a",
  "an",
  "self",
  "me",
  "all",
  "room",
  "someone",
  "something",
  "\n"
};

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(CharData *ch, const char *argument) {
	BUFFER(line, MAX_STRING_LENGTH);
	int cmd, length;

//	REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);

	skip_spaces(argument);
	
	if (!*argument || PURGED(ch))
		return;
	
	/*
	 * special case to handle one-character, non-alphanumeric commands;
	 * requested by many people so "'hi" or ";godnet test" is possible.
	 * Patch sent by Eric Green and Stefan Wasilewski.
	 */
	BUFFER(arg, MAX_INPUT_LENGTH);
	if (!isalpha(*argument)) {
		arg[0] = argument[0];
		arg[1] = '\0';
		strcpy(line, argument + 1);
	} else
		strcpy(line, any_one_arg(argument, arg));

	/* otherwise, find the command */
	for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
		if (!strncmp(complete_cmd_info[cmd].command, arg, length) && (length >= strlen(complete_cmd_info[cmd].sort_as)))
			if ((ch->GetLevel() >= complete_cmd_info[cmd].minimum_level) && 
					(!IS_STAFFCMD(cmd) || (complete_cmd_info[cmd].staffcmd & STF_FLAGS(ch))))
				break;

	if (PLR_FLAGGED(ch, PLR_FROZEN) && (ch->GetLevel() < LVL_CODER) && complete_cmd_info[cmd].command_pointer != do_quit)
		send_to_char("You try, but the mind-numbing cold prevents you...\n", ch);
	else
	{
		int scriptresult;
		
		if (!ch->m_ScriptPrompt.empty())
			ch->AbortTimer(argument);
		else if (/*!complete_cmd_info[cmd].dontabort) && */ !ch->AbortTimer(argument) || !ch->InRoom() /*Safety check for a delay/force quit*/)
		{ }
		else if ((scriptresult = command_wtrigger(ch, arg, line))
			|| (scriptresult = command_mtrigger(ch, arg, line))
			|| (scriptresult = command_otrigger(ch, arg, line))
			|| (scriptresult = command_gtrigger(ch, arg, line)))
		{
			if (scriptresult > 0)
				ch->Appear( MAKE_BITSET(Affect::Flags, AFF_HIDE) );
		}
		else if (!no_specials && special(ch, arg, line))
		{ }
		else if (*complete_cmd_info[cmd].command == '\n')
		{
			if (!imc_command_hook(ch, arg, line))
				send_to_char("Huh?!?\n", ch);
		}
		else if (complete_cmd_info[cmd].command_pointer == NULL)
			send_to_char("Sorry, that command hasn't been implemented yet.\n", ch);
		else if ((!ch->desc || ch->desc->m_OrigCharacter) && IS_STAFFCMD(cmd))
			send_to_char("You can't use staff commands while switched.\n", ch);
	//	else if (IS_NPC(ch) && complete_cmd_info[cmd].minimum_level >= LVL_IMMORT)
	//		send_to_char("You can't use immortal commands while switched.\n", ch);
		else if (GET_POS(ch) < complete_cmd_info[cmd].minimum_position) {
			switch (GET_POS(ch)) {
				case POS_DEAD:
					send_to_char("Lie still; you are DEAD!!!\n", ch);
					break;
				case POS_INCAP:
				case POS_MORTALLYW:
					send_to_char("You are in a pretty bad shape, unable to do anything!\n", ch);
					break;
				case POS_STUNNED:
					send_to_char("All you can do right now is think about the stars!\n", ch);
					break;
				case POS_SLEEPING:
					send_to_char("In your dreams, or what?\n", ch);
					break;
				case POS_RESTING:
					send_to_char("Nah... You feel too relaxed to do that..\n", ch);
					break;
				case POS_SITTING:
					send_to_char("Maybe you should get on your feet first?\n", ch);
					break;
				case POS_FIGHTING:
					send_to_char("No way!  You're fighting for your life!\n", ch);
					break;
				default:
					break;	//	POS_STANDING... shouldn't reach here.
			}
		}
		else
		{
			if (complete_cmd_info[cmd].minimum_level != STAFF_CMD &&
				IS_SET(complete_cmd_info[cmd].staffcmd, LOSE_HIDE))
				ch->Appear(MAKE_BITSET(Affect::Flags, AFF_HIDE, AFF_COVER));
			
			(*complete_cmd_info[cmd].command_pointer) (ch, line, arg, complete_cmd_info[cmd].subcmd);
		}
	}
}



class TimerCommandEvent : public Event
{
public:
						TimerCommandEvent(unsigned int when, TimerCommand *timer) : Event(when), m_pTimer(timer)
						{}
	
	virtual const char *GetType() const { return "TimerCommand"; }

private:
	virtual unsigned int	Run();

	TimerCommand *		m_pTimer;
};



unsigned int TimerCommandEvent::Run()
{
	if (!m_pTimer)
		return 0;
	
	return m_pTimer->Execute();
}


TimerCommand::TimerCommand(int time, const char * type) :
	m_pRefCount(0),
	m_pEvent(NULL),
	m_bRunning(false),
	m_TimerType(type)
{
	m_pEvent = new TimerCommandEvent(time > 0 ? time : 1, this);
}


TimerCommand::~TimerCommand()
{
	if (m_pEvent)
		m_pEvent->Cancel();
}


int TimerCommand::Execute()
{
	int result;
	
	++m_pRefCount;
	
	result = Run();

	//	This code shouldn't be necessary, since it's being run from an event.
	//	The Cancel() will return immediately because the event is running...	
//	if (m_pRefCount == 1)
//		result = 0;
	
//	if (result == 0)
//		m_pEvent = NULL;	//	We don't want to cancel it... that would be bad.  It's an executing event!
	
	Release(NULL);
	
	return result;
}


bool TimerCommand::ExecuteAbort(CharData *ch, const char *str)
{
	bool result;
	
	++m_pRefCount;
	
	result = Abort(ch, str);

	//	This code shouldn't be necessary, since it's being run from an event.
	//	The Cancel() will return immediately because the event is running...	
//	if (m_pRefCount == 1)
//		result = 0;
	
//	if (result == 0)
//		m_pEvent = NULL;	//	We don't want to cancel it... that would be bad.  It's an executing event!
	
	Release(NULL);
	
	return result;
}




void TimerCommand::Release(CharData *ch)
{
	if (--m_pRefCount == 0)
		delete this;
}



const char * ActionTimerCommand::Type = "ActionTimerCommand";


ActionTimerCommand::ActionTimerCommand(int time, CharData *ch, ActionCommandFunction func, int newState, void *buf1, void * /*buf2*/) :
	TimerCommand(time, ActionTimerCommand::Type),
	m_pCharacter(ch),
	m_pFunction(func),
	m_NewState(newState),
	m_pBuffer(buf1)
{
	m_pRefCount = 1;
}



ActionTimerCommand::~ActionTimerCommand()
{
	if (m_pBuffer)
		free(m_pBuffer);
}


int ActionTimerCommand::Run()
{
	m_bRunning = true;
	
	if (m_pCharacter->m_pTimerCommand != this)
		return 0;
	
	m_pCharacter->m_Substate = m_NewState;
	m_pCharacter->m_SubstateNewTime = 0;
	(*m_pFunction)(m_pCharacter, "", "", 0);
	m_bRunning = false;
	if (PURGED(m_pCharacter) || m_pCharacter->m_Substate == SUB_NONE || m_pCharacter->m_SubstateNewTime == 0)
	{
		m_pCharacter->ExtractTimer();
		return 0;
	}
	else
		return m_pCharacter->m_SubstateNewTime;
}


bool ActionTimerCommand::Abort(CharData *ch, const char *str)
{
	if (!m_bRunning && m_pCharacter->m_pTimerCommand == this)
	{
		int tempsub = m_pCharacter->m_Substate;
		m_pCharacter->m_Substate = SUB_TIMER_DO_ABORT;
		m_pCharacter->m_SubstateNewTime = 0;
		m_bRunning = true;
		(*m_pFunction)(m_pCharacter, "", "", 0);
		m_bRunning = false;
		
		if (PURGED(m_pCharacter))
			return false;	//	Failed to abort
		else if (m_pCharacter->m_Substate == SUB_TIMER_CANT_ABORT) {
			m_pCharacter->m_Substate = tempsub;
			return false;	//	Failed to abort
		} else {
			m_pCharacter->ExtractTimer();
		}
	}
	
	return true;		//	Successfully aborted (already running == success)
}




void CharData::AddTimer(int time, ActionCommandFunction func, int newstate, void *buf1, void *buf2) {
	if (m_pTimerCommand) {
		mudlogf(NRM, 103, TRUE, "%s has a timer attached at CharData::AddTimer", this->GetName());
		
		if (m_pTimerCommand->IsRunning())
		{
			//	We can't really do anything here... this shouldn't happen.
			mudlogf(NRM, 103, TRUE, "Timer was running");
			return;	//	What else can we do, safely?
		}
		
		//	Cancel it
		if (!AbortTimer())
			return;
	}
	
	m_pTimerCommand = new ActionTimerCommand(time, this, func, newstate, buf1, buf2);
}


void CharData::ExtractTimer(void) {
	if (m_pTimerCommand && !m_pTimerCommand->IsRunning())
	{
		m_pTimerCommand->Release(this);
		m_pTimerCommand = NULL;
		m_Substate = SUB_NONE;
	}
}


/**************************************************************************
 * Routines to handle aliasing                                             *
  **************************************************************************/


const Alias * Alias::List::Find(const char *command) const
{
	FOREACH_CONST(Alias::List, *this, alias)
		if (alias->m_Command == command)
			return &(*alias);
	
	return NULL;
}


void Alias::List::Remove(const Alias *alias)
{
	FOREACH(Alias::List, *this, iter)
	{
		if (&*iter == alias)
		{
			erase(iter);
			return;
		}
	}
}


Alias::Alias(const char *arg, const char *repl) :
	m_Command(arg),
	m_Replacement(repl),
	m_Type(SimpleType)
{
	if ((m_Replacement.find(SeparatorChar) != Lexi::String::npos) ||
		(m_Replacement.find(VariableChar) != Lexi::String::npos))
		m_Type = ComplexType;
//	(strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR)) ?
//		ALIAS_COMPLEX : ALIAS_SIMPLE;
}


/* The interface to the outside world: do_alias */
ACMD(do_alias) {
	char *repl;

	if (IS_NPC(ch))
		return;

	BUFFER(arg, MAX_INPUT_LENGTH);
	repl = any_one_arg(argument, arg);

	if (!*arg) {			/* no argument specified -- list currently defined aliases */
		send_to_char("Currently defined aliases:\n", ch);
		if (ch->GetPlayer()->m_Aliases.empty())
			send_to_char(" None.\n", ch);
		else
		{
			FOREACH(Alias::List, ch->GetPlayer()->m_Aliases, alias)
			{
				ch->Send("%-15s %s\n", alias->GetCommand(), alias->GetReplacement());
			}
		}
	} else {			/* otherwise, add or remove aliases */
		/* is this an alias we've already defined? */
		
		const Alias *a = ch->GetPlayer()->m_Aliases.Find(arg);
		if (a != NULL)
		{
			ch->GetPlayer()->m_Aliases.Remove(a);
		}
		
		/* if no replacement string is specified, assume we want to delete */
		if (!*repl) {
			if (!a)	send_to_char("No such alias.\n", ch);
			else	send_to_char("Alias deleted.\n", ch);
		} else  if (!str_cmp(arg, "alias"))	/* otherwise, either add or redefine an alias */
			send_to_char("You can't alias 'alias'.\n", ch);
		else {
			delete_doubledollar(repl);
			ch->GetPlayer()->m_Aliases.Add(arg, repl);
			send_to_char("Alias added.\n", ch);
		}
	}
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define MAX_TOKENS       9

void perform_complex_alias(txt_q &inputQueue, char *orig, const Alias *a)
{
	BUFFER(buf2, MAX_STRING_LENGTH);
	BUFFER(buf, MAX_STRING_LENGTH);
	const char *tokens[MAX_TOKENS], *temp;
	int num_of_tokens = 0, num;

	/* First, parse the original string */
	strcpy(buf2, orig);
	
	while ((temp = strsep(&buf2, " ")) && (num_of_tokens < MAX_TOKENS))
	{
		tokens[num_of_tokens++] = temp;
	}

	txt_q tempQueue;
	
	/* now parse the alias */
	int write_length = 0;
	for (temp = a->GetReplacement(); *temp; temp++) {
		char t = *temp;
			
		int length_left = MAX_INPUT_LENGTH - write_length;
		
		if (length_left <= 0)
		{
			while (t && t != Alias::SeparatorChar)
			{
				t = *++temp;
			}
		}

		if (t == Alias::SeparatorChar)
		{
			buf[MIN(write_length, MAX_INPUT_LENGTH - 1)] = '\0';
			tempQueue.write(buf, true);
			write_length = 0;
		}
		else if (t == Alias::VariableChar)
		{
			t = *++temp;
			
			num = t - '1';
			if (num >= 0 && num < num_of_tokens)
			{
				strncpy(buf + write_length, tokens[num], length_left);
				write_length += MIN((int)strlen(tokens[num]), length_left);
			}
			else if (t == Alias::GlobChar)
			{
				strncpy(buf + write_length, orig, length_left);
				write_length += MIN((int)strlen(orig), length_left);
			}
			else if (length_left > 2)
			{
				buf[write_length++] = t;
				if (t == '$')	/* redouble $ for act safety */
					buf[write_length++] = '$';
			}
		}
		else
			buf[write_length++] = *temp;
	}

	buf[MIN(write_length, MAX_INPUT_LENGTH - 1)] = '\0';
	tempQueue.write(buf, true);

	/* push our temp_queue on to the _front_ of the input queue */
	inputQueue.insert(inputQueue.begin(), tempQueue.begin(), tempQueue.end());
	tempQueue.clear();
}


/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(DescriptorData *d, char *orig) {
	/* bail out immediately if the guy doesn't have any aliases */
	if (IS_NPC(d->m_Character) || d->m_Character->GetPlayer()->m_Aliases.empty())
		return 0;
    
	BUFFER(first_arg, MAX_INPUT_LENGTH);

	/* find the alias we're supposed to match */
	char *ptr = any_one_arg(orig, first_arg);

	/* bail out if it's null */
	if (!*first_arg) {
		return 0;
	}

	/* if the first arg is not an alias, return without doing anything */
	const Alias *a = d->m_Character->GetPlayer()->m_Aliases.Find(first_arg);
	if (a == NULL) {
		return 0;
	}

	if (a->GetType() == Alias::SimpleType)
	{
		strcpy(orig, a->GetReplacement());
		return 0;
	}
	else
	{
		perform_complex_alias(d->m_InputQueue, ptr, a);
		return 1;
	}
}



/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(const char *arg, const char **list, bool exact) {
	int		i;
	
	if (exact) {
		for (i = 0; **(list + i) != '\n'; i++)
			if (!str_cmp(arg, *(list + i)))
				return (i);
	} else {;
		size_t l = strlen(arg);
		if (!l)
			l = 1;	//	Avoid "" to match the first available string
		for (i = 0; **(list + i) != '\n'; i++)
			if (!strn_cmp(arg, *(list + i), l))
				return (i);
	}
	
	return -1;
}


int search_chars(char arg, const char *list) {
	int i;

	for (i = 0; list[i]; ++i)
		if (arg == list[i])
			return (i);
	return -1;
}


/*
bool is_number(char *str) {
	if (!str || !*str)		return false;
	if (*str == '-')		str++;		
	while (*str)
		if (!isdigit(*(str++)))
			return false;
	return true;
}
*/
bool is_number(const char *str)
{
	char c = *str;
	if (!c)	return false;
	if (c == '-' || c == '+')		c = *(++str);
	if (!c)	return false;
/*	if (c == '0')
	{
		c = *(++str);
		if (c == 'x' || c == 'X')
		{
			c = *(++str);
			while (isxdigit(c))			c = *(++str);
			return (!c || isspace(c));
		}
	}
*/	while (isdigit(c))				c = *(++str);
	return (!c || isspace(c));
}


bool is_float(const char *str)
{
	char c = *str;
	if (!c)	return false;
	if (c == '-' || c == '+')		c = *(++str);
	if (!c)	return false;
	while (isdigit(c))				c = *(++str);
	if (c == '.')
	{
		c = *(++str);
		while (isdigit(c))			c = *(++str);
	}
	return (!c || isspace(c));
}


STR_TYPE str_type(const char *str)
{
	bool	bIsFloat = false;
	bool	bIsNumber = false;
	bool	bIsHexadecimal = false;
	
	if (!str)						return STR_TYPE_STRING;
	char c = *str;
	if (!*str)						return STR_TYPE_INTEGER;	//	because empty string == 0 is false without this
	if (c == '-' || c == '+')		c = *(++str);
	if (c == '0')	//	Supports octal and hex
	{
		bIsNumber = true;
		c = *(++str);
		if (tolower(c) == 'x')	//	hex
		{
			c = *(++str);
			bIsHexadecimal = true;
		}
	}
	
	if (bIsHexadecimal)
	{
		while (isxdigit(c))
		{
			bIsNumber = true;
			c = *(++str);
		}
	}
	else
	{
		while (isdigit(c))
		{
			bIsNumber = true;
			c = *(++str);
		}
		if (c == '.')
		{
			c = *(++str);
			if (isdigit(c))
			{
				bIsFloat = bIsNumber = true;
				do { c = *(++str); } while (isdigit(c));
			}
		}
		if (bIsNumber && (c == 'e' || c == 'E'))
		{
			c = *(++str);
			if (c == '-' || c == '+')		c = *(++str);
			if (isdigit(c))
			{
				bIsFloat = true;	
				do { c = *(++str); } while (isdigit(c));
			}
		}
	}
	
	if (bIsNumber
		&& (!c || isspace(c)))		return bIsFloat ? STR_TYPE_FLOAT : STR_TYPE_INTEGER;
	else							return STR_TYPE_STRING;
}


void	fix_float(char *str)
{
#if 0		//	Strlen work backwards removing every 0 then re-add the last 0 if at .0
	int n = strlen(str) - 1;
	
	for (; n > 1 && str[n] == '0'; --n)
		str[n] = '\0';
	
	if (str[n] == '.')
	{
		str[n+1] = '0';
		str[n+2] = '\0';
	}
#elif 1		//	Find the first zero in the last string of zeroes
	//	Find the decimal
	bool	bFoundDigit = false;
	
	char c = *str;
	while (c && c != '.')
	{
		if (c != '0')	bFoundDigit = true;
		c = *++str;	
	}
	
	if (!c)	return;
	
	//	str is at the period so skip it
	c = *++str;
	
	//	We might have a non-zero digit...
	if (!bFoundDigit)	bFoundDigit = c != '0';
	
	c = *++str;	//	We want at least one digit after the period!
	
	char *	firstZero = NULL;
	while (c && !isspace(c))
	{
		if (c == '0')
		{
			if (!firstZero)	firstZero = str;
		}
		else
		{
			firstZero = NULL;
		}
		
		c = *++str;
	}
	
	if (firstZero)	*firstZero = '\0';
#else		//	Find the first zero in a series of zeroes with max of 5 in a row terminate there

	//	Find the decimal
	bool	bFoundDigit = false;
	
	char c = *str;
	while (c && c != '.')
	{
		if (c != '0')	bFoundDigit = true;
		c = *++str;	
	}
	
	if (!c)	return;
	
	//	str is at the period so skip it
	c = *++str;
	
	//	We might have a non-zero digit...
	if (!bFoundDigit)	bFoundDigit = c != '0';
	
	c = *++str;	//	We want at least one digit after the period!
	
	char *	firstZero = NULL;
	int		numZeroes = 0;
	while (c && !isspace(c))
	{
		if (c == '0')
		{
			if (!firstZero)
			{
				firstZero = str;
				numZeroes = 0;	
			}
			else if (bFoundDigit && ++numZeroes == 4)
				break;
		}
		else
		{
			firstZero = NULL;
			bFoundDigit = true;
		}
		
		c = *++str;
	}
	
	if (firstZero)	*firstZero = '\0';
#endif
}


void skip_spaces(const char * &string) {
	while (isspace(*string))	++string;
}

void skip_spaces(char * &string) {
	while (isspace(*string))	++string;
}


char *delete_doubledollar(char *string) {
	char *read, *write;

	if ((write = strchr(string, '$')) == NULL)
		return string;

	read = write;

	while (*read)
		if ((*(write++) = *(read++)) == '$')
			if (*read == '$')
				read++;

	*write = '\0';

	return string;
}


bool fill_word(const char *argument) {
	return (search_block(argument, fill, TRUE) >= 0);
}


bool reserved_word(const char *argument) {
	return (search_block(argument, reserved, TRUE) >= 0);
}


/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(const char *argument, char *first_arg) {
	char *begin = first_arg;
	
	if (!argument) {
		log("SYSERR: one_argument received a NULL pointer!");
		*first_arg = '\0';
		return NULL;
	}
	
	do {
		skip_spaces(argument);

		first_arg = begin;
		while (*argument && !isspace(*argument)) {
			*(first_arg++) = tolower(*argument);
			argument++;
		}

		*first_arg = '\0';
	} while (fill_word(begin));

	skip_spaces(argument);
	
	return const_cast<char *>(argument);
}


/*
 * one_word is like one_argument, except that words in quotes ("") are
 * considered one word.
 */
char *one_word(const char *argument, char *first_arg)
{
  char *begin = first_arg;

	if (!argument) {
		log("SYSERR: one_argument received a NULL pointer.");
		*first_arg = '\0';
		return NULL;
	}

  do {
    skip_spaces(argument);

    first_arg = begin;

    if (*argument == '\"') {
      argument++;
      while (*argument && *argument != '\"') {
        *(first_arg++) = tolower(*argument);
        argument++;
      }
      argument++;
    } else {
      while (*argument && !isspace(*argument)) {
        *(first_arg++) = tolower(*argument);
        argument++;
      }
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return const_cast<char *>(argument);
}


/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(const char *argument, char *first_arg)
{
  skip_spaces(argument);

  while (*argument && !isspace(*argument)) {
    *(first_arg++) = tolower(*argument);
    ++argument;
  }

  *first_arg = '\0';

  skip_spaces(argument);

  return const_cast<char *>(argument);
}



/* same as any_one_arg except that it stops at punctuation */
char *any_one_name(const char *argument, char *first_arg) {
    char* arg;
  
    /* Find first non blank */
//    while(isspace(*argument))
//		argument++;
	skip_spaces(argument);	
	  
    /* Find length of first word */
    for(arg = first_arg ;
			*argument && !isspace(*argument) &&
	  		(!ispunct(*argument) || *argument == '#' || *argument == '-');
			arg++, argument++)
		*arg = tolower(*argument);
    *arg = '\0';

    return const_cast<char *>(argument);
}


/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(const char *argument, char *first_arg, char *second_arg) {
	return one_argument(one_argument(argument, first_arg), second_arg);
}


char *three_arguments(const char *argument, char *first_arg, char *second_arg, char *third_arg) {
	return one_argument(two_arguments(argument, first_arg, second_arg), third_arg);  
}


/*
 * determine if a given string is an abbreviation of another
 * returns 1 if arg1 is an abbreviation of arg2
 */
bool is_abbrev(const char *arg1, const char *arg2) {
	if (!*arg1 || !*arg2)
		return false;

	for (; *arg1 && *arg2; arg1++, arg2++)
		if (tolower(*arg1) != tolower(*arg2))
			return false;

	if (!*arg1)	return true;
	else		return false;
}

/* return first space-delimited token in arg1; remainder of string in arg2 */
void half_chop(const char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(temp);
  strcpy(arg2, temp);
}



/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command) {
	int cmd;

	for (cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
		if (!strcmp(complete_cmd_info[cmd].command, command))
			return cmd;

	return -1;
}


bool special(CharData *ch, const char *cmd, char *arg) {
	int j;

	//	Room
	if (GET_ROOM_SPEC(ch->InRoom()) != NULL)
		if (GET_ROOM_SPEC(ch->InRoom()) (ch, ch->InRoom(), cmd, arg))
			return true;

	//	In EQ List
	for (j = 0; j < NUM_WEARS; j++)
		if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
			if (GET_OBJ_SPEC(GET_EQ(ch, j)) (ch, GET_EQ(ch, j), cmd, arg))
				return true;

	//	In Inventory
	FOREACH(ObjectList, ch->carrying, iter)
	{
		ObjData *i = *iter;
		if (GET_OBJ_SPEC(i) != NULL)
			if (GET_OBJ_SPEC(i) (ch, i, cmd, arg)) {
				return true;
			}
	}

	//	In Mobile
	FOREACH(CharacterList, ch->InRoom()->people, iter)
	{
		CharData *k = *iter;
		
		if (k->GetPrototype() && k->GetPrototype()->m_Function
			&& k->GetPrototype()->m_Function(ch, k, cmd, arg))
		{
			return true;
		}
	}

	//	In Object present
	FOREACH(ObjectList, ch->InRoom()->contents, iter)
	{
		ObjData *i = *iter;
		
		if (GET_OBJ_SPEC(i) != NULL)
			if (GET_OBJ_SPEC(i) (ch, i, cmd, arg)) {
				return true;
			}
	}

	return false;
}



/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */


/* locate entry in p_table with entry->name == name. -1 mrks failed search */
bool find_name(const char *name)
{
	FOREACH(PlayerTable, player_table, iter)
	{
		if (iter->second == name)
			return true;
	}
	
	return false;
}


int _parse_name(const char *arg, char *name)
{
  int i;

  /* skip whitespaces */
  for (; isspace(*arg); arg++) ;

  for (i = 0; (*name = *arg); arg++, i++, name++)
    if (!isalpha(*arg))
      return 1;

  if (!i)
    return 1;

  return 0;
}


#define RECON		1
#define USURP		2
#define UNSWITCH	3

int perform_dupe_check(DescriptorData *d)
{
	CharData *target = NULL;
	int mode = 0;

	int id = d->m_Character->GetID();

	/*
	 * Now that this descriptor has successfully logged in, disconnect all
	 * other descriptors controlling a character with the same ID number.
	 */

	FOREACH(DescriptorList, descriptor_list, iter)
	{
		DescriptorData *k = *iter;
		if (k == d)
			continue;

		if (k->m_OrigCharacter && (k->m_OrigCharacter->GetID() == id)) {    /* switched char */
			k->Write("\nMultiple login detected -- disconnecting.\n");
			k->GetState()->Push(new DisconnectConnectionState);
			if (!target) {
				target = k->m_OrigCharacter;
				mode = UNSWITCH;
			}
			if (k->m_Character)
				k->m_Character->desc = NULL;
			k->m_Character = NULL;
			k->m_OrigCharacter = NULL;
		} else if (k->m_Character && (k->m_Character->GetID() == id)) {
			if (!target && k->GetState()->IsPlaying())
			{
				k->Write("\nThis body has been usurped!\n");
				target = k->m_Character;
				mode = USURP;
			}
			k->m_Character->desc = NULL;
			k->m_Character = NULL;
			k->m_OrigCharacter = NULL;
			k->Write("\nMultiple login detected -- disconnecting.\n");
			k->GetState()->Push(new DisconnectConnectionState);
		}
	}

	/*
	 * now, go through the character list, deleting all characters that
	 * are not already marked for deletion from the above step (i.e., in the
	 * CON_HANGUP state), and have not already been selected as a target for
	 * switching into.  In addition, if we haven't already found a target,
	 * choose one if one is available (while still deleting the other
	 * duplicates, though theoretically none should be able to exist).
	 */
	FOREACH(CharacterList, character_list, iter)
	{
		CharData *ch = *iter;
		
		if (IS_NPC(ch))				continue;
		if (ch->GetID() != id)	continue;

		//	ignore chars with descriptors (already handled by above step)
		if (ch->desc)				continue;

		//	don't extract the target char we've found one already
		if (ch == target)			continue;

		//	we don't already have a target and found a candidate for switching
		if (!target) {
			target = ch;
			mode = RECON;
			continue;
		}

		//	we've found a duplicate - blow him away, dumping his eq in limbo.
		if (ch->InRoom())
			ch->FromRoom();
		ch->ToRoom(world[1]);
		ch->Extract();
	}

	//	no target for swicthing into was found - allow login to continue
	if (!target)
		return 0;

	//	Okay, we've found a target.  Connect d to target.
	delete d->m_Character; /* get rid of the old char */
	d->m_Character = target;
	d->m_Character->desc = d;
	d->m_OrigCharacter = NULL;
	d->m_Character->GetPlayer()->m_IdleTime = 0;
	REMOVE_BIT(PLR_FLAGS(d->m_Character), PLR_MAILING | PLR_WRITING);
	d->m_ConnectionState.Swap(new PlayingConnectionState);

	switch (mode) {
		case RECON:
			d->Write("Reconnecting.\n");
			act("$n has reconnected.", TRUE, d->m_Character, 0, 0, TO_ROOM);
			mudlogf( NRM, d->m_Character->GetPlayer()->m_StaffInvisLevel, TRUE,  "%s [%s] has reconnected.", d->m_Character->GetName(), d->m_Host.c_str());
			break;
		case USURP:
			d->Write("You take over your own body, already in use!\n");
			act("$n suddenly keels over in pain, surrounded by a white aura...\n"
				"$n's body has been taken over by a new spirit!",
					TRUE, d->m_Character, 0, 0, TO_ROOM);
			mudlogf(NRM, d->m_Character->GetPlayer()->m_StaffInvisLevel, TRUE,
					"%s has re-logged in ... disconnecting old socket.",
					d->m_Character->GetName());
			break;
	case UNSWITCH:
			d->Write("Reconnecting to unswitched char.");
			mudlogf( NRM, d->m_Character->GetPlayer()->m_StaffInvisLevel, TRUE,  "%s [%s] has reconnected.", d->m_Character->GetName(), d->m_Host.c_str());
			break;
	}

	return 1;
}


/* load the player, put them in the right room - used by copyover_recover too */
void enter_player_game (CharData *ch)
{
	GET_MAX_HIT(ch) = max_hit_base[ch->GetRace()] + (ch->real_abils.Health * max_hit_bonus[ch->GetRace()]);
	if (GET_MAX_HIT(ch) <= 0)
		GET_MAX_HIT(ch) = 1;

	if (PLR_FLAGGED(ch, PLR_INVSTART))
		ch->GetPlayer()->m_StaffInvisLevel = ch->GetLevel();
	LoadPlayerObjectFile(ch);
	ch->ToRoom(ch->LastRoom());

	ch->ToWorld();
	ch->Save();
	
	FOREACH(ObjectList, ch->carrying, obj)
		start_otrigger(*obj, ch);

	for (int i = 0; i < NUM_WEARS; ++i)
		if (GET_EQ(ch, i)) {
			start_otrigger(GET_EQ(ch, i), ch);
			if (GET_EQ(ch, i))
				wear_otrigger(GET_EQ(ch, i), ch, i);
		}
	
/*	if (GET_REAL_SKILL(ch, SKILL_HIDING_UNUSED) > 0)
	{
		ch->Send("`rSYSTEM CHANGE: The HIDING skill was merged into stealth.  %d Skill Points have been restored to you.`n\n",
				GET_REAL_SKILL(ch, SKILL_HIDING_UNUSED));
		
		GET_PRACTICES(ch) += GET_REAL_SKILL(ch, SKILL_HIDING_UNUSED);
		GET_REAL_SKILL(ch, SKILL_HIDING_UNUSED) = 0;
	}
*/	
#if 0
	bool foundSkills = false;
	int reimbursement = 0;
	int maximum = GET_MAX_SKILL_ALLOWED(ch);
	for (int i = 0; i < NUM_SKILLS; ++i)
	{
		if (GET_REAL_SKILL(ch, i) > maximum)
		{
			if (!foundSkills)
				ch->Send("`rSkills have recently been level-capped in training.`n\n"
								   "`rYou cannot exceed %d points until you reach level %d.`n\n"
								   "`rThe following skills were reduced to the maximum:`n\n",
						maximum, (((ch->GetLevel() - 1) / 10) * 10) + 11);
			foundSkills = true;
			
			ch->Send("`y%s`n\n", skill_info[i].name);
			
			reimbursement += (GET_REAL_SKILL(ch, i) - maximum) *
					skill_info[i].cost[ch->GetRace()];
			GET_REAL_SKILL(ch, i) = maximum;
		}
	}
	if (reimbursement > 0)
	{
		ch->Send("`rYou have been reimbursed for `n`g%d`n`r Skill Points.\n", reimbursement);
		GET_PRACTICES(ch) += reimbursement;
		
		ch->AffectTotal();
	}
#endif
	

	//	Versions:
	//	0 - no version (pre 12/29/03)
	//	1 - 12/29/03 - first version, eliminates flags
	//	2 - 3/23/04 - second version, skill cost reduction for Preds and Marines
	//	3 - 12/10/04 - third version, eliminate SP bonus
	//	4 - 3/31/05 - fourth version, reduce skill point costs
	//	5 - 8/28/05 - fifth version, increase skill point costs
	if (ch->GetPlayer()->m_Version < CURRENT_VERSION)
	{
		REMOVE_BIT(PLR_FLAGS(ch), PLR_SKILLPOINT_INCREASE);	//	remove 12-20-03
		REMOVE_BIT(PLR_FLAGS(ch), PLR_SKILLCOST_ADJUST);		//	remove 12-29-03
		
		if (ch->GetPlayer()->m_Version < VERSION_LAST_SKILLPOINT_CHANGE && !IS_STAFF(ch))
		{
			extern int stat_base[NUM_RACES][7];

			int totalPointsSpent = 0;
			int oldPoints = ch->GetPlayer()->m_SkillPoints;

			int totalStatsBought =
					ch->real_abils.Strength +
					ch->real_abils.Health +
					ch->real_abils.Coordination +
					ch->real_abils.Agility +
					ch->real_abils.Perception +
					ch->real_abils.Knowledge - stat_base[ch->GetRace()][6];
			totalPointsSpent = totalStatsBought * 6;

			for (int i = 1; i < NUM_SKILLS; ++i)
			{
				if (skill_info[i].min_level[ch->GetRace()] > ch->GetLevel())
					GET_REAL_SKILL(ch, i) = 0;

				if (GET_REAL_SKILL(ch, i) == 0)
					continue;
				
				totalPointsSpent += GET_REAL_SKILL(ch, i) * skill_info[i].cost[ch->GetRace()];
			}
			
			ch->GetPlayer()->m_LifetimeSkillPoints = ch->GetLevel() * 12;
			ch->GetPlayer()->m_SkillPoints = ch->GetPlayer()->m_LifetimeSkillPoints - totalPointsSpent;
			
			int gain = ch->GetPlayer()->m_SkillPoints - oldPoints;

			if (gain != 0)
			{
				ch->Send("`RSYSTEM CHANGE:`y Some skills have had their costs altered.`n\n");
				
				if (gain > 0)
					ch->Send("`gYou gain %d Skill Points as a result.`n\n", gain);		
				else if (gain < 0)
					ch->Send("`gYou have lost %d Skill Points as a result.`n\n", -gain);
				else
					ch->Send("`gYour Skill Points were unaffected by this change.`n\n");
			}
		}
		
		ch->GetPlayer()->m_Version = CURRENT_VERSION;
	}
	
	if (ch->GetPlayer()->m_SkillPoints < 0)
	{
		ch->Send("`RYour Skill Points are currently below 0.  You must unpractice skills or stats`n\n"
						   "`Rto bring your points back to 0 or higher.  Until you do, you will not earn MP`n\n"
						   "`Rfor kills.  You may unpractice for free while below 0.`n\n");
	}
}



#include "connectionstates.h"
#include "lexifile.h"




//
//	LOGIN CONNECTION STATES
//

class PasswordConnectionState : public IdleTimeoutConnectionState
{
public:
	virtual void		Enter();
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Get Password"; }
};


class MOTDConnectionState : public ConnectionState
{
public:
	virtual void		Enter();
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Reading MOTD"; }
};


//
//	CHARACTER CREATION CONNECTION STATES
//

class NewPasswordConnectionState : public IdleTimeoutConnectionState
{
public:
	NewPasswordConnectionState()
	:	m_bVerifying(false)
	{}
	
	virtual void		Enter();
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Get New PW"; }

private:
	bool				m_bVerifying;
};


class ChooseRaceConnectionState : public ConnectionState
{
public:
	virtual void		Enter();
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Select race"; }
};


class ChooseSexConnectionState : public ConnectionState
{
public:
	virtual void		Enter();
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Select sex"; }
};


class AboutStatsConnectionState : public ConnectionState
{
public:
	virtual void		Enter();
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "About Stats"; }
};


class ChooseStatsConnectionState : public ConnectionState
{
public:
	virtual void		Enter();
	virtual void		Resume();
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Choose Stats"; }

private:
	int					GetPointsRemaining();
	void				DisplayStats();
};


//
//	MENU CONNECTION STATES
//


class MenuBackgroundConnectionState : public ConnectionState
{
public:
	virtual void		Enter();
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Main Menu"; }
};


class MenuWriteDescriptionConnectionState : public ConnectionState
{
public:
	virtual void		Enter();
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Get descript."; }

	virtual void		OnEditorAbort();
	virtual void		OnEditorFinished();
};


class MenuChangePasswordConnectionState : public ConnectionState
{
private:
	enum State
	{
		StateOldPassword,
		StateNewPassword,
		StateConfirmPassword
	};
	
public:	
	MenuChangePasswordConnectionState()
	:	m_State(StateOldPassword)
	{}
	
	virtual void		Enter();
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Change PW"; }

private:
	State				m_State;
};


class MenuSelfDeleteConnectionState : public ConnectionState
{
private:
	enum State
	{
		StatePassword,
		StateConfirm
	};
	
public:	
	MenuSelfDeleteConnectionState()
	:	m_State(StatePassword)
	{}
	
	virtual void		Enter();
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Self-Delete"; }

private:
	State				m_State;
};



//
//	ConnectionState Implementation
//


ConnectionState::ConnectionState()
:	m_pDesc(NULL)
{
}


ConnectionState::~ConnectionState()
{
}


ConnectionStateMachine &ConnectionState::GetStateMachine() 
{
	return GetDesc()->m_ConnectionState;
}


void ConnectionState::Push(ConnectionState::Ptr state)
{
	GetStateMachine().Push(state);
}


void ConnectionState::Pop()
{
	GetStateMachine().Pop();
}


void ConnectionState::Swap(ConnectionState::Ptr state)
{
	GetStateMachine().Swap(state);
}


void ConnectionStateMachine::Idle()
{
	if (!m_States.empty())
	{
		ConnectionState::Ptr state = m_States.front();
		state->Idle();
	}
}


ConnectionState::Ptr ConnectionStateMachine::GetState()
{
	//	Compiler doesn't like reducing this to a trinary
	if (!m_States.empty())
		return m_States.front();
	
	return NULL;
}

void ConnectionStateMachine::Parse(const char *arg)
{
	if (!m_States.empty())
	{
		ConnectionState::Ptr state = m_States.front();
		state->Parse(arg);
	}
}


void ConnectionStateMachine::Push(ConnectionState::Ptr state)
{
	if (!m_States.empty())
	{
		m_States.front()->Pause();	
//		state->m_Parent = m_States.front();
	}
	
	m_States.push_front(state);
	state->SetDesc( GetDesc() );
	state->Enter();
}


void ConnectionStateMachine::Pop()
{
	if (!m_States.empty())	m_States.front()->Exit();
	m_States.pop_front();
	if (!m_States.empty())	m_States.front()->Resume();
}


void ConnectionStateMachine::Swap(ConnectionState::Ptr state)
{
	if (!m_States.empty())	m_States.front()->Exit();
	m_States.pop_front();
	m_States.push_front(state);
	state->SetDesc( GetDesc() );
	state->Enter();
}



//
//	PlayingConnectionState
//

void PlayingConnectionState::Parse(const char *arg)
{
	//	Send to the command interpreter
	command_interpreter(GetDesc()->m_Character, arg);
}


void PlayingConnectionState::OnEditorAbort()
{
	GetDesc()->Write("Message aborted.\n");
}


//
//	DisconnectConnectionState
//

void DisconnectConnectionState::Idle()
{
	GetDesc()->Close();
}



//
//	IdentConnectionState
//

void IdentConnectionState::Enter()
{
	GetDesc()->Send("  This may take a moment.\r\n");
//	GetDesc()->Write("  This may take a moment.\n");
}


void IdentConnectionState::Parse(const char *arg)
{
	GetDesc()->Write("Your domain is being looked up, please be patient.\n");
}


void IdentConnectionState::Exit()
{
	if (GetDesc()->m_HostIP != "204.209.44.7")
		GetDesc()->Write(login.c_str());
	
	//GetDesc()->Write("\xFF\xF9");
	mudlogf(CMP, LVL_STAFF, TRUE, "New connection from [%s]", GetDesc()->m_Host.c_str());
}


//	IdentConnectionState
//

void OldAddressConnectionState::Enter()
{
	extern int port;
	BUFFER(buf, MAX_STRING_LENGTH);
	
	sprintf(buf,
"\n                                   \x1B[1;31mNOTICE\x1B[0m\n"
"\n"
"                                  We've Moved!\n"
"\n"
"You are connecting to an old address that will cease to work soon.\n"
"Please update your MUD client to connect to \"\x1B[1;31mavpmud.com %d\x1B[0m\" and restart\n"
"your MUD client.  Failure to do so WILL result in your inability to play AvP.\n"
"\n"
"If you have any problems or questions, please talk to a staff member.\n"
"\n"
"Press enter to continue and play AvP.  You may experience additional lag.", port);

	GetDesc()->Write(buf);
}


void OldAddressConnectionState::Parse(const char *arg)
{
	Pop();
}


void OldAddressConnectionState::Exit()
{
	GetDesc()->Write(login.c_str());
}


//
//	IdleTimeoutConnectionState
//

void IdleTimeoutConnectionState::Idle()
{
	if (time(0) > ( m_StartTime + ms_IdleTimeout ))
	{
		GetDesc()->Write("\nTimed out... goodbye.\n");
		GetDesc()->SetEchoEnabled();
		Push(new DisconnectConnectionState);
	}
}



//
//	GetNameConnectionState
//

void GetNameConnectionState::Enter()
{
	if (GetDesc()->m_Character)
	{
		GetDesc()->m_Character->m_Name = "";
	}
}


void GetNameConnectionState::Parse(const char *arg)
{
	IdleTimeoutConnectionState::ResetTimeout();

	if (!*arg)
	{
		Push(new DisconnectConnectionState);
		return;
	}
	
	BUFFER(buf, 128);
	BUFFER(tmp_name, MAX_INPUT_LENGTH);
	bool	bValidName = false;

	if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
			strlen(tmp_name) > MAX_NAME_LENGTH || // !Ban::IsValidName(tmp_name) ||
			fill_word(strcpy(buf, tmp_name)) || reserved_word(buf))
		;	//	Bad name breaker
	else
	{
		if (!GetDesc()->m_Character)
		{
			GetDesc()->m_Character = new CharData;
			GetDesc()->m_Character->desc = GetDesc();
		}
		
		bool bCharacterLoaded = GetDesc()->m_Character->Load(tmp_name);
		
		if (!bCharacterLoaded ||
			(PLR_FLAGGED(GetDesc()->m_Character, PLR_DELETED) && !PLR_FLAGGED(GetDesc()->m_Character, PLR_NODELETE)))
		{
			/* We get false positive from the original deleted name. */
			if (bCharacterLoaded)
			{
				remove(CharData::GetObjectFilename(tmp_name).c_str());
			}
			
			delete GetDesc()->m_Character;
			GetDesc()->m_Character = NULL;
			
			/* Check for multiple creations... */

			bValidName = Ban::IsValidName(tmp_name);
			
			if (bValidName)
			{
				GetDesc()->m_Character = new CharData;
				GetDesc()->m_Character->m_pPlayer = new PlayerData;
				GetDesc()->m_Character->desc = GetDesc();
				GetDesc()->m_Character->m_Name = CAP(tmp_name);
				GetDesc()->m_Character->GetPlayer()->m_Version = CURRENT_VERSION;

				Swap(new ConfirmNameConnectionState);
				return;
			}
		}
		else
		{
			// Undo it just in case they are set
			REMOVE_BIT(PLR_FLAGS(GetDesc()->m_Character), PLR_WRITING | PLR_MAILING);

			Swap(new PasswordConnectionState);
			return;
		}
	}

	GetDesc()->Write("We won't accept that name, private!\n"
			 "Give me a better one: ");
}



//
//	ConfirmNameConnectionState
//

void ConfirmNameConnectionState::Enter()
{
	BUFFER(buf, MAX_STRING_LENGTH);
	sprintf(buf, "Did I hear you correctly, '%s' (Y/N)? ", GetDesc()->m_Character->GetName());
	GetDesc()->Write(buf);
}


void ConfirmNameConnectionState::Parse(const char *arg)
{
	IdleTimeoutConnectionState::ResetTimeout();
	
	if (toupper(*arg) == 'Y')
	{
		if (Ban::IsBanned(GetDesc()->m_Host) >= BAN_NEW) {
			GetDesc()->Write("We aren't accepting greenies from your neck of the woods!\n"
							 "(No new characters allowed from your site.  If you think this is in error, or\n"
							 "would like it to be temporarily lifted for you, you can visit our forums at\n"
							 "http://www.avpmud.com/forums and ask for it to be temporarily lifted.)");
			mudlogf(NRM, LVL_STAFF, TRUE, "Request for new char %s denied from [%s] (siteban)",
					GetDesc()->m_Character->GetName(), GetDesc()->m_Host.c_str());
			Push(new DisconnectConnectionState);
			return;
		}
		if (circle_restrict) {
//			SEND_TO_Q("Sorry, new players can't be created at the moment.\n", d);
			GetDesc()->Write("We aren't accepting greenies at the moment!  Try again later.\n"
							 "(No new players can be created at the moment)");
			mudlogf(NRM, LVL_STAFF, TRUE, "Request for new char %s denied from [%s] (wizlock)",
					GetDesc()->m_Character->GetName(), GetDesc()->m_Host.c_str());
			Push(new DisconnectConnectionState);
			return;
		}
		GetDesc()->Write("Welcome to the Corps, private!\n");

		Swap(new NewPasswordConnectionState);
	}
	else if (toupper(*arg) == 'N')
	{
		GetDesc()->Write("Okay, what IS it, then? ");
		Swap(new GetNameConnectionState);
	}
	else
	{
		GetDesc()->Write("I won't take that for an answer!\n"
				  "Give me a 'Yes' or a 'No': ");
	}
}



//
//	NewPasswordConnectionState
//


void NewPasswordConnectionState::Enter()
{
	//	We ask here so that NewPasswordConnectionState
	//	can be reused if they don't enter a matching password
	BUFFER(buf, MAX_INPUT_LENGTH);
	sprintf(buf, "Now give me a password, '%s': ", GetDesc()->m_Character->GetName());
	GetDesc()->Write(buf);
	GetDesc()->SetEchoEnabled(false);
}


void NewPasswordConnectionState::Parse(const char *arg)
{
	IdleTimeoutConnectionState::ResetTimeout();

	if (m_bVerifying)
	{
		if (strncmp(arg, GetDesc()->m_Character->GetPlayer()->m_Password, MAX_PWD_LENGTH))
		{
			GetDesc()->Write("\nPasswords don't match... start over.\n"
							 "Password: ");
			m_bVerifying = false;
			return;
		}
		
		GetDesc()->SetEchoEnabled();
		
		Swap(new ChooseRaceConnectionState);
	}
	else
	{
		if (!*arg || strlen(arg) > MAX_PWD_LENGTH
			|| strlen(arg) < 3
			|| !str_cmp(arg, GetDesc()->m_Character->GetName()))
		{
			GetDesc()->Write("\nIllegal password.\n"
							 "Password: ");
			return;
		}
		strncpy(GetDesc()->m_Character->GetPlayer()->m_Password, arg, MAX_PWD_LENGTH);
		GetDesc()->m_Character->GetPlayer()->m_Password[MAX_PWD_LENGTH] = '\0';
		
		GetDesc()->Write("\nPlease retype password: ");
		m_bVerifying = true;
	}
}


//
//	ChooseRaceConnectionState
//

void ChooseRaceConnectionState::Enter()
{
	GetDesc()->Write(race_menu);
	GetDesc()->Write("Race: ");
}


void ChooseRaceConnectionState::Parse(const char *arg)
{
	int racenum = ParseRace(arg);
	
	if (racenum == RACE_UNDEFINED)
	{
		GetDesc()->Write("\nThat's not a race.\n"
						 "Race: ");
		return;
	}
	
	GetDesc()->m_Character->m_Race = (Race)racenum;
	
	Swap(new ChooseSexConnectionState);
}


//
//	ChooseRaceConnectionState
//

void ChooseSexConnectionState::Enter()
{
	if (GetDesc()->m_Character->m_Race == RACE_ALIEN)
	{
		GetDesc()->m_Character->m_Sex = SEX_NEUTRAL;
		Swap(new ChooseStatsConnectionState);
		return;
	}
	
	GetDesc()->Write("What is your sex (M/F)? ");
}


void ChooseSexConnectionState::Parse(const char *arg)
{
	switch (toupper(*arg))
	{
		case 'M':	GetDesc()->m_Character->m_Sex = SEX_MALE;		break;
		case 'F':	GetDesc()->m_Character->m_Sex = SEX_FEMALE;	break;
		default:
			GetDesc()->Write("That is NOT a sex..\n"
							 "Try again: ");
			return;
	}

	Swap(new ChooseStatsConnectionState);
}



//
//	AboutStatsConnectionState
//

void AboutStatsConnectionState::Enter()
{
	GetDesc()->Write(
			"\n"
			"The Character Stats are an important part of gameplay.  They are factored\n"
			"into the game in many places, most importantly combat.\n"
			"\n"
			"The stats are as follows:\n"
			"     (S)trength     - Physical strength\n"
			"     (H)ealth       - Health and vitality\n"
			"     (C)oordination - Hand-eye coordination\n"
			"     (A)gility      - Ability to move quickly\n"
			"     (P)erception   - Awareness of your surroundings\n"
			"     (K)nowledge    - General intelligence and education\n"
			"\n"
			"In the following section you will be provided with a base set of stats, and\n"
			"points to improve any stats you wish.\n"
			"\n"
			"To spend points, enter the letter of the stat, followed by +# or -# to\n"
			"increase that stat.  You may enter multiple stats, seperated by spaces.\n"
			"You cannot decrease any stat below the default of 70.\n"
			"For example: 'S+5' would improve your Strength by 5 points.\n"
			"You can see this information again by entering '?'\n"
			"\n"
			"*** FIRE WHEN READY\n");
}


void AboutStatsConnectionState::Parse(const char *arg)
{
	Pop();
}



//
//	ChooseStatsConnectionState
//

extern int stat_base[NUM_RACES][7];


void ChooseStatsConnectionState::Enter()
{
	roll_real_abils(GetDesc()->m_Character);
	Push(new AboutStatsConnectionState);
}


void ChooseStatsConnectionState::Resume()
{
	DisplayStats();
}


void ChooseStatsConnectionState::Parse(const char *arg)
{
	CharData *ch = GetDesc()->m_Character;
	int points = GetPointsRemaining();
	
	if (points == 0 && toupper(*arg) == 'Y')
	{
		ch->real_abils = ch->aff_abils;
		init_char(GetDesc()->m_Character);

		mudlogf( NRM, LVL_IMMORT, TRUE,  "%s [%s] new player.", ch->GetName(), GetDesc()->m_Host.c_str());

		Swap(new MOTDConnectionState);
		return;
	}
	
	if (*arg == '?')
	{
		Push(new AboutStatsConnectionState);
		return;
	}
	
	
	const char *cmd = arg;
	while (*cmd)
	{
		int points = GetPointsRemaining();
		
		int			amount = atoi(cmd + 1);
		
		if (amount > points)
			amount = points;
		
		/*	Parse ARG */
		switch (toupper(*cmd))
		{
			case 'S':
				GET_STR(ch) =
					RANGE(GET_STR(ch) + amount,
						  stat_base[ch->GetRace()][0],
						  MAX_PC_STAT);
				break;
			case 'H':
				GET_HEA(ch) =
					RANGE(GET_HEA(ch) + amount,
						  stat_base[ch->GetRace()][1],
						  MAX_PC_STAT);
				break;
			case 'C':
				GET_COO(ch) =
					RANGE(GET_COO(ch) + amount,
						  stat_base[ch->GetRace()][2],
						  MAX_PC_STAT);
				break;
			case 'A':
				GET_AGI(ch) =
					RANGE(GET_AGI(ch) + amount,
						  stat_base[ch->GetRace()][3],
						  MAX_PC_STAT);
				break;
			case 'P':
				GET_PER(ch) =
					RANGE(GET_PER(ch) + amount,
						  stat_base[ch->GetRace()][4],
						  MAX_PC_STAT);
				break;
			case 'K':
				GET_KNO(ch) =
					RANGE(GET_KNO(ch) + amount,
						  stat_base[ch->GetRace()][5],
						  MAX_PC_STAT);
				break;
			default:
				break;	//	do nothing					
		}
		while (*cmd && !isspace(*cmd))	++cmd;
		while (isspace(*cmd))			++cmd;
	}

	DisplayStats();
}


int ChooseStatsConnectionState::GetPointsRemaining()
{
	CharData *ch = GetDesc()->m_Character;
	int points = (stat_base[ch->GetRace()][6]) -
				   (GET_STR(ch) + GET_AGI(ch) +
					GET_HEA(ch) + GET_PER(ch) +
					GET_COO(ch) + GET_KNO(ch));

	return points;
}

void ChooseStatsConnectionState::DisplayStats()
{
	BUFFER(buf, MAX_STRING_LENGTH);
	CharData *ch = GetDesc()->m_Character;

	int points = GetPointsRemaining();
	
	sprintf(buf,
			"Your stats are:\n"
			"\n"
			"(S)trength:     [%3d]     (A)gility:      [%3d]\n"
			"(H)ealth:       [%3d]     (P)erception:   [%3d]\n"
			"(C)oordination: [%3d]     (K)nowledge:    [%3d]\n"
			"\n"
			"Points remaining: %d\n"
			"\n"
			"Improve your stats%s:",
			GET_STR(ch), GET_AGI(ch),
			GET_HEA(ch), GET_PER(ch),
			GET_COO(ch), GET_KNO(ch),
			points,
			points == 0 ? " (or 'Y' to accept)" : "");
	GetDesc()->Write(buf);
}



//
//	PasswordConnectionState
//

void PasswordConnectionState::Enter()
{
	GetDesc()->Write("Password: ");
	GetDesc()->SetEchoEnabled(false);
}


void PasswordConnectionState::Parse(const char *arg)
{
	GetDesc()->SetEchoEnabled();    /* turn echo back on */
//	GetDesc()->Write("\x1B[3;20r\x1B[H");	//	REMOVE: vt102 experiment

	if (!*arg)
	{
		Push(new DisconnectConnectionState);
		return;
	}
	
	if (strncmp(arg, GetDesc()->m_Character->GetPlayer()->m_Password, MAX_PWD_LENGTH))
	{
		mudlogf( BRF, LVL_STAFF, TRUE,  "Bad PW: %s [%s]", GetDesc()->m_Character->GetName(), GetDesc()->m_Host.c_str());
	
		++GetDesc()->m_Character->GetPlayer()->m_BadPasswordAttempts;
		GetDesc()->m_Character->desc = NULL;	//	DONT update "last login" or "last host" on a failure!
		GetDesc()->m_Character->Save();
		GetDesc()->m_Character->Purge();
		GetDesc()->m_Character = NULL;
		
		if (++(GetDesc()->m_FailedPasswordAttempts) >= max_bad_pws)	//	3 strikes and you're out
		{
			GetDesc()->Write("Wrong password... disconnecting after 3 invalid passwords.\nName? ");
			Push(new DisconnectConnectionState);
		}
		else
		{
			GetDesc()->Write("Wrong password.\nIf you are a new player, this name is already in use.\nName? ");
			Swap(new GetNameConnectionState);
		}
		return;
	}
	
	if (Ban::IsBanned(GetDesc()->m_Host) == BAN_SELECT && !PLR_FLAGGED(GetDesc()->m_Character, PLR_SITEOK)) {
		mudlogf(NRM, LVL_STAFF, TRUE, "Connection attempt for %s denied from %s",
				GetDesc()->m_Character->GetName(), GetDesc()->m_Host.c_str());

		GetDesc()->Write("Sorry, this char has not been cleared for login from your site!\n");
		Push(new DisconnectConnectionState);
		return;
	}
	
	if (GetDesc()->m_Character->GetLevel() < circle_restrict)
	{
		mudlogf(NRM, LVL_STAFF, TRUE, "Request for login denied for %s [%s] (wizlock)",
				GetDesc()->m_Character->GetName(), GetDesc()->m_Host.c_str());

		GetDesc()->Write("The game is temporarily restricted.. try again later.\n");
		Push(new DisconnectConnectionState);
		return;
	}
	
	//	Check and make sure no other copies of this player are logged in
	if (perform_dupe_check(GetDesc()))
		return;

//	if (PRF_FLAGGED(d->m_Character, PRF_AUTOZLIB) && !d->compressionMode)
//	{
//		SEND_TO_SOCKET(eor_on_str, d->m_Socket);
//		SEND_TO_SOCKET(compress2_on_str, d->m_Socket);
//		SEND_TO_SOCKET(compress_on_str, d->m_Socket);
//	}

	GetDesc()->m_Character->GetPlayer()->m_LoginTime = GetDesc()->m_Character->GetPlayer()->m_LastLogin = time(0);	// Set the last logon time to now.

	mudlogf(BRF, GetDesc()->m_Character->GetPlayer()->m_StaffInvisLevel, TRUE, 
			"%s [%s] has connected.", GetDesc()->m_Character->GetName(), GetDesc()->m_Host.c_str());
	
	Swap(new MOTDConnectionState);
}



//
//	MOTDConnectionState
//

void MOTDConnectionState::Enter()
{
	GetDesc()->Write(IS_STAFF(GetDesc()->m_Character) ? imotd.c_str() : motd.c_str());

	if (GetDesc()->m_Character->GetPlayer()->m_BadPasswordAttempts > 0)
	{
		BUFFER(buf, 128);
		sprintf_cat(buf, "\n\n\007\007\007"
				"`R%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.`n\n",
				GetDesc()->m_Character->GetPlayer()->m_BadPasswordAttempts,
				(GetDesc()->m_Character->GetPlayer()->m_BadPasswordAttempts > 1) ? "S" : "");
		GetDesc()->Write(buf);

		GetDesc()->m_Character->GetPlayer()->m_BadPasswordAttempts = 0;
	}

	GetDesc()->Write("\n\n*** FIRE WHEN READY: ");
}


void MOTDConnectionState::Parse(const char *)
{
	Pop();
}



//
//	MenuConnectionState
//

//void MenuConnectionState::Enter()
//{
//	GetDesc()->Write(MENU);
//}


void MenuConnectionState::Resume()
{
	GetDesc()->Write(MENU);
}


void MenuConnectionState::Parse(const char *arg)
{
	switch (*arg)
	{
		case '0':
			GetDesc()->Write("Goodbye.\n");
			Push(new DisconnectConnectionState);
			return;

		case '1':
			enter_player_game(GetDesc()->m_Character);
			send_to_char(welcomeMsg.c_str(), GetDesc()->m_Character);

			if (!GetDesc()->m_Character->GetLevel())
			{
				do_start(GetDesc()->m_Character);
				send_to_char(startMsg.c_str(), GetDesc()->m_Character);
				act("$n has entered the game for the first time.", TRUE, GetDesc()->m_Character, 0, 0, TO_ROOM);
			}
			else
				act("$n has entered the game.", TRUE, GetDesc()->m_Character, 0, 0, TO_ROOM);
			
			//	Show current room to character
			look_at_room(GetDesc()->m_Character);
			
			//	Clan message
			if (GET_CLAN(GetDesc()->m_Character))	GET_CLAN(GetDesc()->m_Character)->OnPlayerLoginMessages(GetDesc()->m_Character);

			//	Check for mail
			if (has_mail(GetDesc()->m_Character->GetID()))
				send_to_char("You have mail waiting.\n", GetDesc()->m_Character);
			
			//	Check for polls
			if (GetDesc()->m_Character->GetLevel() >= 21 &&  has_polls(GET_ID(GetDesc()->m_Character)))
				send_to_char("You have polls to vote on (type 'vote').\n", GetDesc()->m_Character);
			
			GetDesc()->m_PromptState = DescriptorData::WantsPrompt;
			
			Push(new PlayingConnectionState);
			return;

		case '2':
			Push(new MenuWriteDescriptionConnectionState);
			break;

		case '3':
			Push(new MenuBackgroundConnectionState);
			break;

		case '4':
			Push(new MenuChangePasswordConnectionState);
			break;

		case '5':
			Push(new MenuSelfDeleteConnectionState);
			break;

		default:
			GetDesc()->Write("\nThat's not a menu choice!\n");
			GetDesc()->Write(MENU);
			break;
	}
}



//
//	MenuBackgroundConnectionState
//

void MenuBackgroundConnectionState::Enter()
{
	page_string(GetDesc(), background.c_str());
}


void MenuBackgroundConnectionState::Parse(const char *arg)
{
	Pop();
}



//
//	MenuWriteDescriptionConnectionState
//

void MenuWriteDescriptionConnectionState::Enter()
{
	GetDesc()->Write("Enter the text you'd like others to see when they look at you.\n"
					 "(/s saves /h for help)\n");
			
	if (!GetDesc()->m_Character->m_Description.empty())
	{
		GetDesc()->Write("Current description:\n");
		GetDesc()->Write(GetDesc()->m_Character->m_Description.c_str());
	}
			
	GetDesc()->StartStringEditor(GetDesc()->m_Character->m_Description, EXDSCR_LENGTH);
}


void MenuWriteDescriptionConnectionState::Parse(const char *arg)
{
	Pop();
}


void MenuWriteDescriptionConnectionState::OnEditorAbort()
{
	GetDesc()->Write("Description aborted.\n");
}


void MenuWriteDescriptionConnectionState::OnEditorFinished()
{
	Pop();
}



//
//	MenuChangePasswordConnectionState
//

void MenuChangePasswordConnectionState::Enter()
{
	GetDesc()->SetEchoEnabled(false);
	GetDesc()->Write("\nEnter your old password: ");
}


void MenuChangePasswordConnectionState::Parse(const char *arg)
{
	switch (m_State)
	{
		case StateOldPassword:
			if (strncmp(arg, GetDesc()->m_Character->GetPlayer()->m_Password, MAX_PWD_LENGTH))
			{
				GetDesc()->Write("\nIncorrect password.\n");
				GetDesc()->SetEchoEnabled();
				Pop();
			} else {
				GetDesc()->Write("\nEnter a new password: ");
				m_State = StateNewPassword;
			}
			break;
		case StateNewPassword:
			if (!*arg
				|| strlen(arg) > MAX_PWD_LENGTH
				|| strlen(arg) < 3
				|| !str_cmp(arg, GetDesc()->m_Character->GetName()))
			{
				GetDesc()->Write("\nIllegal password.\n");
				GetDesc()->Write("Password: ");
				break;
			}
			strncpy(GetDesc()->m_Character->GetPlayer()->m_Password, arg, MAX_PWD_LENGTH);
			GetDesc()->m_Character->GetPlayer()->m_Password[MAX_PWD_LENGTH] = '\0';

			GetDesc()->Write("\nPlease retype password: ");
			
			m_State = StateConfirmPassword;
			break;
			
		case StateConfirmPassword:
			if (strncmp(arg, GetDesc()->m_Character->GetPlayer()->m_Password, MAX_PWD_LENGTH))
			{
				GetDesc()->Write("\nPasswords don't match... start over.\n");
				GetDesc()->Write("Password: ");
				m_State = StateNewPassword;
				break;
			}
			
			GetDesc()->m_Character->Save();
			GetDesc()->Write("\nDone.\n");
			GetDesc()->SetEchoEnabled();
			
			Pop();
			break;
	}
}



//
//	MenuSelfDeleteConnectionState
//

void MenuSelfDeleteConnectionState::Enter()
{
	GetDesc()->Write("\nEnter your password for verification: ");
	GetDesc()->SetEchoEnabled(false);
}


void MenuSelfDeleteConnectionState::Parse(const char *arg)
{
	switch (m_State)
	{
		case StatePassword:
			if (strncmp(arg, GetDesc()->m_Character->GetPlayer()->m_Password, MAX_PWD_LENGTH))
			{
				GetDesc()->Write("\nIncorrect password.\n");
				GetDesc()->SetEchoEnabled();
				Pop();
			}
			else
			{
				GetDesc()->Write(	"\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\n"
									"ARE YOU ABSOLUTELY SURE?\n\n"
									"Please type \"yes\" to confirm: ");
				m_State = StateConfirm;
			}
			break;
			
		case StateConfirm:
			if (str_cmp(arg, "yes"))
			{
				GetDesc()->Write("\nCharacter not deleted.\n");
				Pop();
				break;
			}
			

			if (PLR_FLAGGED(GetDesc()->m_Character, PLR_FROZEN))
			{
				GetDesc()->Write("You try to kill yourself, but the ice stops you.\n");
				GetDesc()->Write("Character not deleted.\n\n");
				
				Push(new DisconnectConnectionState);
			}
			else
			{
				if (GetDesc()->m_Character->GetLevel() < LVL_ADMIN)
					SET_BIT(PLR_FLAGS(GetDesc()->m_Character), PLR_DELETED);
				GetDesc()->m_Character->Save();
				
				BUFFER(buf, 128);
				sprintf(buf, "Character '%s' deleted!\n"
							 "Goodbye.\n", GetDesc()->m_Character->GetName());
				GetDesc()->Write(buf);
				
				remove(GetDesc()->m_Character->GetObjectFilename().c_str());
				
				mudlogf(NRM, LVL_STAFF, TRUE, "%s (lev %d) has self-deleted.",
						GetDesc()->m_Character->GetName(), GetDesc()->m_Character->GetLevel());
				
				Push(new DisconnectConnectionState);
			}
			break;
	}
}



//
//	MenuSelfDeleteConnectionState
//

void OLCConnectionState::Enter()
{
	CharData *ch = GetDesc()->m_Character;
	
//	act("$n starts using OLC.", TRUE, ch, 0, 0, TO_ROOM);
	mudlogf(NRM, ch->GetPlayer()->m_StaffInvisLevel, FALSE, "OLC: %s starts using OLC.", ch->GetName());
	SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
}


void OLCConnectionState::Exit()
{
	CharData *ch = GetDesc()->m_Character;
	
	if (ch)
	{
		REMOVE_BIT(PLR_FLAGS(ch), PLR_WRITING);
		act("$n stops using OLC.", TRUE, ch, 0, 0, TO_ROOM);
	}
	
	GetDesc()->olc = NULL;
}



//
//	ObjectEditOLCConnectionState
//

void ObjectEditOLCConnectionState::Parse(const char *arg)
{
	oedit_parse(GetDesc(), arg);
}


void ObjectEditOLCConnectionState::OnEditorFinished()
{
	extern void oedit_disp_menu(DescriptorData *d);
	extern void oedit_disp_extradesc_menu(DescriptorData *d);
	
	switch (OLC_MODE(GetDesc()))
	{
		case OEDIT_ACTDESC:					oedit_disp_menu(GetDesc());				break;
		case OEDIT_EXTRADESC_DESCRIPTION:	oedit_disp_extradesc_menu(GetDesc());	break;
	}
}



//
//	RoomEditOLCConnectionState
//

void RoomEditOLCConnectionState::Parse(const char *arg)
{
	REdit::parse(GetDesc(), arg);
}


void RoomEditOLCConnectionState::OnEditorFinished()
{
	switch (OLC_MODE(GetDesc())) {
		case REDIT_DESC:
			if (!*OLC_ROOM(GetDesc())->GetDescription())
				OLC_ROOM(GetDesc())->SetDescription("   Nothing.\n");
			REdit::disp_menu(GetDesc());
			break;
		case REDIT_EXIT_DESCRIPTION:		REdit::disp_exit_menu(GetDesc());		break;
		case REDIT_EXTRADESC_DESCRIPTION:	REdit::disp_extradesc_menu(GetDesc());	break;
	}
}



//
//	ZoneEditOLCConnectionState
//

void ZoneEditOLCConnectionState::Parse(const char *arg)
{
	zedit_parse(GetDesc(), arg);
}



//
//	MobEditOLCConnectionState
//

void MobEditOLCConnectionState::Parse(const char *arg)
{
	medit_parse(GetDesc(), arg);
}


void MobEditOLCConnectionState::OnEditorFinished()
{
	extern void medit_disp_menu(DescriptorData *d);
	switch (OLC_MODE(GetDesc())) {
		case MEDIT_D_DESC:				medit_disp_menu(GetDesc());					break;
	}
}



//
//	ScriptEditOLCConnectionState
//

void ScriptEditOLCConnectionState::Parse(const char *arg)
{
	scriptedit_parse(GetDesc(), arg);
}


void ScriptEditOLCConnectionState::OnEditorFinished()
{
	extern void scriptedit_disp_menu(DescriptorData *d);
	switch (OLC_MODE(GetDesc())) {
		case SCRIPTEDIT_SCRIPT:				scriptedit_disp_menu(GetDesc());		break;
	}
}



//
//	ShopEditOLCConnectionState
//

void ShopEditOLCConnectionState::Parse(const char *arg)
{
	sedit_parse(GetDesc(), arg);
}



//
//	ActionEditOLCConnectionState
//

void ActionEditOLCConnectionState::Parse(const char *arg)
{
	aedit_parse(GetDesc(), arg);
}



//
//	HelpEditOLCConnectionState
//

void HelpEditOLCConnectionState::Parse(const char *arg)
{
	hedit_parse(GetDesc(), arg);
}


void HelpEditOLCConnectionState::OnEditorFinished()
{
	extern void hedit_disp_menu(DescriptorData *d);
	switch (OLC_MODE(GetDesc())) {
		case HEDIT_ENTRY:					hedit_disp_menu(GetDesc());				break;
	}
}



//
//	ClanEditOLCConnectionState
//

void ClanEditOLCConnectionState::Parse(const char *arg)
{
	cedit_parse(GetDesc(), arg);
}



//
//	BehaviorEditOLCConnectionState
//


void BehaviorEditOLCConnectionState::Parse(const char *arg)
{
	bedit_parse(GetDesc(), arg);
}



