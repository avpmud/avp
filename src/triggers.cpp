/**************************************************************************
*  File: triggers.c                                                       *
*                                                                         *
*  Usage: contains all the trigger functions for scripts.                 *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: avp $
*  $Date: 2008-09-27 04:56:48 $
*  $Revision: 3.105 $
**************************************************************************/

#include "structs.h"
#include "scripts.h"
#include "scriptcompiler.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "db.h"
#include "buffer.h"
#include "constants.h"

/* obj command trigger types */
static const Flags OCMD_EQUIP = (1 << 0);		/* obj must be in char's equip */
static const Flags OCMD_INVEN = (1 << 1);		/* obj must be in char's inven */
static const Flags OCMD_ROOM  = (1 << 2);		/* obj must be in char's room  */
static const Flags OCMD_FLAGS = (OCMD_EQUIP | OCMD_INVEN | OCMD_FLAGS);

/* external functions from scripts.c */
const char *ScriptParserMatchQuote(const char *p);

const char *one_phrase(const char *arg, char *first_arg);
int is_substring(const char *sub, const char *string);
int word_check(const char *str, const char *wordlist);
char *cmd_check(const char *str, const char *wordlist);
int greet_mtrigger(CharData *actor, int dir);
int enter_otrigger(CharData *actor, RoomData * room, int dir);
int entry_mtrigger(CharData *ch, int dir);
int command_mtrigger(CharData *actor, const char *cmd, const char *argument);
int call_mtrigger(CharData *ch, Entity *caller, const char *cmd, const char *argument, char *result);
void speech_mtrigger(CharData *actor, const char *str);
void act_mtrigger(CharData *ch, const char *str, CharData *actor, 
		CharData *victim, ObjData *object, ObjData *target, const char *arg);
void fight_mtrigger(CharData *ch);
void hitprcnt_mtrigger(CharData *ch);
int receive_mtrigger(CharData *ch, CharData *actor, ObjData *obj);
int death_mtrigger(CharData *ch, CharData *actor);
void death_otrig(ObjData *obj, CharData *actor, CharData *killer, Flags type);
void death_otrigger(CharData *actor, CharData *killer);
int get_otrigger(ObjData *obj, CharData *actor);
int greet_otrigger(CharData *actor, int dir);
int cmd_otrig(ObjData *obj, CharData *actor, const char *cmd, const char *argument, Flags type);
int command_otrigger(CharData *actor, const char *cmd, const char *argument);
int call_otrigger(ObjData *obj, Entity *caller, const char *cmd, const char *argument, char *result);
int wear_otrig(ObjData *obj, ObjData *what, CharData *actor, int where, Flags type);
int wear_otrigger(ObjData *what, CharData *actor, int where);
int remove_otrig(ObjData *obj, ObjData *what, CharData *actor, int where, Flags type);
int remove_otrigger(ObjData *what, CharData *actor, int where);
int drop_otrigger(ObjData *obj, CharData *actor);
int give_otrigger(ObjData *obj, CharData *actor, CharData *victim);
int put_otrigger(ObjData *obj, ObjData *container, CharData *actor);
int retrieve_otrigger(ObjData *obj, ObjData *container, CharData *actor);
int sit_otrigger(ObjData *obj, CharData *actor);
int enter_wtrigger(RoomData *room, CharData *actor, int dir);
int greet_wtrigger(RoomData *room, CharData *actor, int dir);
int command_wtrigger(CharData *actor, const char *cmd, const char *argument);
int call_wtrigger(RoomData *room, Entity *caller, const char *cmd, const char *argument, char *result);
void speech_wtrigger(CharData *actor, const char *str);
int drop_wtrigger(ObjData *obj, CharData *actor);
int consume_otrigger(ObjData *obj, CharData *actor);
void act_wtrigger(RoomData *room, const char *str, CharData *actor, ObjData *object);
void store_otrigger(ObjData *obj);

int leave_mtrigger(CharData *actor, int dir);
int leave_otrigger(CharData *actor, int dir);
int leave_wtrigger(RoomData *room, CharData *actor, int dir);

int door_mtrigger(CharData *actor, int subcmd, int dir);
int door_otrigger(CharData *actor, int subcmd, int dir);
int door_wtrigger(CharData *actor, int subcmd, int dir);

int speech_otrig(ObjData *obj, CharData *actor, const char *str, int type);
void speech_otrigger(CharData *actor, const char *str);

void load_mtrigger(CharData *ch);
void load_otrigger(ObjData *obj);
void timer_otrigger(ObjData *obj);
void start_otrigger(ObjData *obj, CharData *actor);
void quit_otrigger(ObjData *obj, CharData *actor);
void reset_wtrigger(RoomData *room);
void motion_otrig(ObjData *obj, CharData *actor, int dir, int motion, bool lateral, int distance, int motionType, Flags type);
void motion_otrigger(CharData *ch, CharData *actor, int dir, int motion, bool lateral, int distance, int motionType);
void motion_mtrigger(CharData *ch, CharData *actor, int dir, int motion, bool lateral, int distance, int motionType);
void motion_wtrigger(RoomData *room, CharData *actor, int dir, int motion, bool lateral, int distance, int motionType);
void room_motion_otrigger(RoomData * room, CharData *actor, int dir, int motion, bool lateral, int distance, int motionType);
void motion_mtrigger(CharData *ch, CharData *actor, int dir, int motion, bool lateral, int distance, int motionType);
int install_otrigger(ObjData *obj, CharData *actor, ObjData *installed, float skillroll);
int destroyrepair_otrigger(ObjData *obj, CharData *actor, int repairing);
void attack_mtrigger(CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, int hits, int headshots);
void attack_otrig(ObjData *obj, CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, Flags type, int hits, int headshots);
void attack_otrigger_for_weapon(CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, int hits, int headshots);
void attack_otrigger(CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, int hits, int headshots);
void attacked_mtrigger(CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, int hits, int headshots);
void attacked_otrigger(CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, int hits, int headshots);
int attacked_otrig(ObjData *obj, CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, Flags type, int hits, int headshots);
void kill_otrig(ObjData *obj, CharData *actor, CharData *victim, int type);
void kill_otrigger(CharData *actor, CharData *victim);
void kill_mtrigger(CharData *actor, CharData *victim);

int command_gtrigger(CharData *actor, const char *cmd, const char *argument);


#define ADD_VAR(name, value)	TrigData::ms_InitialVariables.Add(name, value)


#define EFFECTIVELY_DEAD(ch)	(PURGED(ch) || AFF_FLAGGED(ch, AFF_RESPAWNING))


/* mob trigger types */
char *mtrig_types[] = {
	"Global",
	"Random",
	"Command",
	"Speech",
	"Act",
	"Death",
	"Greet",
	"Greet-All",
	"Entry",
	"Receive",
	"Fight",
	"HitPrcnt",
	"Bribe",
	"Leave",
	"Leave-All",
	"Door",
	"Load",
	"Call",
	"Kill",
	"Motion",
	"Attack",
	"Attacked",
	"\n"
};


/* obj trigger types */
char *otrig_types[] = {
	"Global",
	"Random",
	"Command",
	"Sit",
	"Leave",
	"Greet",
	"Get",
	"Drop",
	"Give",
	"Wear",
	"Door",
	"Speech",
	"Consume",
	"Remove",
	"Load",
	"Timer",
	"Start",
	"Quit",
	"Attack",
	"Install",
	"Call",
	"Enter",
	"DestroyRepair",
	"Death",
	"Attacked",
	"Kill",
	"Put",
	"Motion",
	"Retrieve",
	"Store",
	"\n"
};


/* wld trigger types */
char *wtrig_types[] = {
	"Global",
	"Random",
	"Command",
	"Speech",
	"Door",
	"Leave",
	"Enter",
	"Drop",
	"Reset",
	"Call",
	"Greet",
	"Act",
	"Motion",
	"\n"
};


char *gtrig_types[] = {
	"Global",
	"Command",
	"Speech",
	"\n"
};



char *trig_types[] = {
	"Global",
	"Random",
	"Command",
	"Speech",
	"Greet",
	"Leave",
	"Door",
	"Drop",
	"Get",
	"Act",
	"Death",
	"Fight",
	"HitPrcnt",
	"Sit",
	"Give",
	"Wear",
	"\n"
};


char *trig_data_types[] = {
	"Mob",
	"Object",
	"Room",
	"Global",
	"\n"
};

char *door_act[] = {
	"open",
	"close",
	"unlock",
	"lock",
	"pick",
	"\n"
};



/*
 *  General functions used by several triggers
 */


/*
 * Copy first phrase into first_arg, returns rest of string
 */
const char *one_phrase(const char *arg, char *first_arg) {
	skip_spaces(arg);

	if (!*arg)	*first_arg = '\0';
	else if (*arg == '"')
	{
		const char *p = arg + 1;
		char *s = first_arg;

		while (*p && *p != '"')
			*s++ = *p++;

		*s = '\0';
		return *p ? p + 1 : p;
	}
	else
	{
		const char *p = arg;
		char *		s = first_arg;
		
		while (*p && !isspace(*p) && *p != '"')
			*s++ = *p++;
			
		*s = '\0';
		return p;
	}
	return arg;
}


int is_substring(const char *sub, const char *string) {
	const char *s;

	if ((s = str_str(string, sub))) {
		int len = strlen(string);
		int sublen = strlen(sub);

		if ((s == string || isspace(*(s - 1)) || ispunct(*(s - 1))) &&	/* check front */
				((s + sublen == string + len) || isspace(s[sublen]) ||	/* check end */
				ispunct(s[sublen])))
			return 1;
	}
	return 0;
}


/*
 * return 1 if str contains a word or phrase from wordlist.
 * phrases are in double quotes (").
 * if wrdlist is NULL, then return 1, if str is NULL, return 0.
 */
int word_check(const char *str, const char *wordlist) {
	if (!wordlist || !str)
		return 0;
	
	BUFFER(words, MAX_INPUT_LENGTH);
	BUFFER(phrase, MAX_INPUT_LENGTH);
	
	skip_spaces(wordlist);
	
	const char *s;

	strcpy(words, wordlist);
    
	for (s = one_phrase(words, phrase); *phrase; s = one_phrase(s, phrase)) {
		if (is_substring(phrase, str))
//		if (is_abbrev(phrase, str))
		{
			return 1;
		}
	}
	
	return 0;
}



/*
 * return 1 if str contains a word or phrase from wordlist.
 * phrases are in double quotes (").
 * if wrdlist is NULL, then return 1, if str is NULL, return 0.
 */
char *cmd_check(const char *str, const char *wordlist) {
	static char buf[MAX_INPUT_LENGTH];
	BUFFER(words, MAX_INPUT_LENGTH);
	BUFFER(phrase, MAX_INPUT_LENGTH);
	BUFFER(word1, MAX_INPUT_LENGTH);
	BUFFER(word2, MAX_INPUT_LENGTH);
	
	skip_spaces(wordlist);

	const char *s;

	strcpy(words, wordlist);
    
    buf[0] = '\0';
	for (s = one_phrase(words, phrase); *phrase; s = one_phrase(s, phrase)) {
		phrase = const_cast<char *>(ScriptEngineGetArgument(phrase, word1));
		phrase = const_cast<char *>(ScriptEngineGetArgument(phrase, word2));
		
		if (*word2)
		{
			if (is_abbrev(str, word2) && strlen(str) >= strlen(word1))
			{
				strcpy(buf, word2);
				break;
			}
		}
		else
		{
			if (!str_cmp(str, word1))
			{
				strcpy(buf, word1);
				break;
			}
		}
	}
	
	return *buf ? buf : NULL;
}
	

/*
 *  mob triggers
 */

void random_mtrigger(CharData *ch) {
	if (!ch->m_pTimerCommand && !SCRIPT_CHECK(ch, MTRIG_RANDOM) || AFF_FLAGGED(ch, AFF_CHARM))
		return;

	FOREACH(TrigData::List, TRIGGERS(SCRIPT(ch)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, MTRIG_RANDOM) && (Number(1, 100) <= GET_TRIG_NARG(t)))
		{
			ScriptEngineRunScript(ch, t);
			if (PURGED(ch))
				break;
		}
	}
}

/*
void bribe_mtrigger(CharData *ch, CharData *actor, int amount) {
	TrigData *t;
	char *buf;
  
	if (!SCRIPT_CHECK(ch, MTRIG_BRIBE) || AFF_FLAGGED(ch, AFF_CHARM))
		return;

	for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next) {
		if (TRIGGER_CHECK(t, MTRIG_BRIBE) && (amount >= GET_TRIG_NARG(t))) {

			ADD_VAR("amount", amount);
			ADD_VAR("actor", actor);
			ScriptEngineRunScript(ch, t);
			break;
		}
	}
}
*/

int greet_mtrigger(CharData *actor, int dir)
{
	int		result = 1;
 	
 	if (EFFECTIVELY_DEAD(actor))
 		return 1;
 	
	FOREACH(CharacterList, actor->InRoom()->people, iter)
	{
		CharData *ch = *iter;
		if (!SCRIPT_CHECK(ch, MTRIG_GREET | MTRIG_GREET_ALL) || 
				!AWAKE(ch) || FIGHTING(ch) || (ch == actor) || AFF_FLAGGED(ch, AFF_CHARM))
			continue;
    
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(ch)), iter)
		{
			TrigData *t = *iter;
			
			if (((TRIGGER_CHECK(t, MTRIG_GREET) && ch->CanSee(actor) != VISIBLE_NONE) ||
					TRIGGER_CHECK(t, MTRIG_GREET_ALL)) && 
					(Number(1, 100) <= GET_TRIG_NARG(t))) {
				ADD_VAR("direction", dirs[rev_dir[dir]]);
				ADD_VAR("actor", actor);


				if (!ScriptEngineRunScript(ch, t))
					result = 0;
			}
			if (EFFECTIVELY_DEAD(ch) || EFFECTIVELY_DEAD(actor) || (IN_ROOM(ch) != IN_ROOM(actor)))
				break;
		}

		if (EFFECTIVELY_DEAD(actor) || (IN_ROOM(ch) != IN_ROOM(actor)))
			break;
	}

	return result;
}


int entry_mtrigger(CharData *ch, int dir)
{
	if (!SCRIPT_CHECK(ch, MTRIG_ENTRY) || AFF_FLAGGED(ch, AFF_CHARM))
		return 1;
 	
 	if (EFFECTIVELY_DEAD(ch))
 		return 1;

	FOREACH(TrigData::List, TRIGGERS(SCRIPT(ch)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, MTRIG_ENTRY) && (Number(1, 100) <= GET_TRIG_NARG(t)))
		{
			ADD_VAR("direction", dir != -1 ? dirs[rev_dir[dir]] : "enter");
			
			if (!ScriptEngineRunScript(ch, t))
				return 0;
			
			if (EFFECTIVELY_DEAD(ch))
				break;
		}
	}
	return 1;
}


int command_mtrigger(CharData *actor, const char *cmd, const char *argument) {
	FOREACH(CharacterList, actor->InRoom()->people, iter)
	{
		CharData *ch = *iter;
		
		if (SCRIPT_CHECK(ch, MTRIG_COMMAND) && !AFF_FLAGGED(ch, AFF_CHARM))
		{
			FOREACH(TrigData::List, TRIGGERS(SCRIPT(ch)), iter)
			{
				TrigData *t = *iter;
				char *newcmd;
				
				if (TRIGGER_CHECK(t, MTRIG_COMMAND) &&
						(GET_TRIG_NARG(t) == 0 || actor == ch) &&
						(/*word_check(cmd, SSData(GET_TRIG_ARG(t)))*/
							(newcmd = cmd_check(cmd, t->m_Arguments.c_str())) ||
						(*t->m_Arguments.c_str() == '*' && !IS_STAFF(actor)))) {
					ADD_VAR("actor", actor);
					skip_spaces(argument);
					ADD_VAR("cmd", newcmd ? newcmd : cmd);
					ADD_VAR("arg", argument);
					
					int result = ScriptEngineRunScript(ch, t);
					if (EFFECTIVELY_DEAD(ch))
						result = 1;
					if (result)
						return result;
				}
			
				if (EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(ch))
					break;
			}
		}
		
		if (EFFECTIVELY_DEAD(actor))
			break;
	}
	return 0;
}


int call_mtrigger(CharData *ch, Entity *caller, const char *cmd, const char *argument, char *result) {
	if (EFFECTIVELY_DEAD(ch))
		return 1;
	
	if (SCRIPT_CHECK(ch, MTRIG_CALL)) {
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(ch)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, MTRIG_CALL) &&
					!str_cmp(cmd, t->m_Arguments.c_str())) {
				ADD_VAR("caller", caller);
				skip_spaces(argument);
				ADD_VAR("arg", argument);
				return ScriptEngineRunScript(ch, t, result); // || EFFECTIVELY_DEAD(ch))
//					return 1;
			
			}

			if (EFFECTIVELY_DEAD(ch))
				break;
		}
	}
	return 0;
}



void speech_mtrigger(CharData *actor, const char *str) {
	FOREACH(CharacterList, actor->InRoom()->people, iter)
	{
		CharData *ch = *iter;
		
		if (SCRIPT_CHECK(ch, MTRIG_SPEECH) && AWAKE(ch) && !AFF_FLAGGED(ch, AFF_CHARM) && (actor != ch)) {
			FOREACH(TrigData::List, TRIGGERS(SCRIPT(ch)), iter)
			{
				TrigData *t = *iter;
				if (TRIGGER_CHECK(t, MTRIG_SPEECH) && (t->m_Arguments.empty() ||
						(GET_TRIG_NARG(t) ?
						(word_check(str, t->m_Arguments.c_str()) || (*t->m_Arguments.c_str() == '*')) :
						is_substring(t->m_Arguments.c_str(), str))))
				{
					ADD_VAR("actor", actor);
					ADD_VAR("speech", str);
					ScriptEngineRunScript(ch, t);
					break;
				}
			}
		}
			
		if (EFFECTIVELY_DEAD(actor))
			break;
	}
}


void act_mtrigger(CharData *ch, const char *str, CharData *actor, 
		CharData *victim, ObjData *object, ObjData *target, const char *arg) {

	if (SCRIPT_CHECK(ch, MTRIG_ACT) && !AFF_FLAGGED(ch, AFF_CHARM))
	{
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(ch)), iter)
		{
			TrigData *t = *iter;
			
			if (TRIGGER_CHECK(t, MTRIG_ACT) &&
					(GET_TRIG_NARG(t) ?
					(word_check(str, t->m_Arguments.c_str()) || (*t->m_Arguments.c_str() == '*')) :
					is_substring(t->m_Arguments.c_str(), str)))
			{
				ADD_VAR("actor", actor);
				ADD_VAR("victim", victim);
				ADD_VAR("object", object);
				ADD_VAR("target", target);
				if (arg) {
					skip_spaces(arg);
					ADD_VAR("arg", arg);
				}
				
				RoomData *room = actor ? IN_ROOM(actor) : NULL;
				ScriptEngineRunScript(ch, t);
				
				if (EFFECTIVELY_DEAD(ch) || (actor && (EFFECTIVELY_DEAD(actor) || IN_ROOM(actor) != room)))
					break;
			}
		}
	}
}


void act_wtrigger(RoomData *room, const char *str, CharData *actor, ObjData *object) {
	if (SCRIPT_CHECK(room, WTRIG_ACT)) {
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(room)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, WTRIG_ACT) &&
					(GET_TRIG_NARG(t) ?
					(word_check(str, t->m_Arguments.c_str()) || (*t->m_Arguments.c_str() == '*')) :
					is_substring(t->m_Arguments.c_str(), str)))
			{
				ADD_VAR("actor", actor);
				ADD_VAR("object", object);

				ScriptEngineRunScript(room, t);
				
				if (EFFECTIVELY_DEAD(actor) || IN_ROOM(actor) != room)
					break;
			}
		}
	}
}


void fight_mtrigger(CharData *ch) {
	if (!SCRIPT_CHECK(ch, MTRIG_FIGHT) || !FIGHTING(ch) || AFF_FLAGGED(ch, AFF_CHARM))
		return;

	FOREACH(TrigData::List, TRIGGERS(SCRIPT(ch)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, MTRIG_FIGHT) && (Number(1, 100) <= GET_TRIG_NARG(t))){
			ADD_VAR("actor", FIGHTING(ch));
			ScriptEngineRunScript(ch, t);
			
			if (EFFECTIVELY_DEAD(ch) || !FIGHTING(ch))
				break;
		}
	}
}


void hitprcnt_mtrigger(CharData *ch) {
	if (!SCRIPT_CHECK(ch, MTRIG_HITPRCNT) || !FIGHTING(ch) || AFF_FLAGGED(ch, AFF_CHARM))
		return;

	FOREACH(TrigData::List, TRIGGERS(SCRIPT(ch)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, MTRIG_HITPRCNT) && GET_MAX_HIT(ch) &&
				(((GET_HIT(ch) * 100) / GET_MAX_HIT(ch)) <= GET_TRIG_NARG(t))) {
			ADD_VAR("actor", FIGHTING(ch));
			ScriptEngineRunScript(ch, t);
			break;
		}
	}
}


int receive_mtrigger(CharData *ch, CharData *actor, ObjData *obj) {
	if (!SCRIPT_CHECK(ch, MTRIG_RECEIVE) || AFF_FLAGGED(ch, AFF_CHARM))
		return 1;

	FOREACH(TrigData::List, TRIGGERS(SCRIPT(ch)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, MTRIG_RECEIVE) && (Number(1, 100) <= GET_TRIG_NARG(t))){
			ADD_VAR("actor", actor);
			ADD_VAR("object", obj);
			
			if (!ScriptEngineRunScript(ch, t) || obj->CarriedBy() != actor)
				return 0;
		}
		
		if (EFFECTIVELY_DEAD(ch) || EFFECTIVELY_DEAD(actor) || PURGED(obj))
			break;
	}

	return 1;
}


int death_mtrigger(CharData *ch, CharData *actor) {
	if (!SCRIPT_CHECK(ch, MTRIG_DEATH))
		return 1;
  
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(ch)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, MTRIG_DEATH)) {
			ADD_VAR("actor", actor);
			
			if (!ScriptEngineRunScript(ch, t) || EFFECTIVELY_DEAD(ch))
				return 0;
		}
	}
	return 1;
}


/* checks for command trigger on specific object */
void kill_mtrigger(CharData *ch, CharData *victim)
{
	if (!SCRIPT_CHECK(ch, MTRIG_KILL))
		return;
	
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(ch)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, MTRIG_KILL))
		{
			ADD_VAR("victim", victim);
			
			ScriptEngineRunScript(ch, t);
			if (EFFECTIVELY_DEAD(ch) || EFFECTIVELY_DEAD(victim))
				return;
		}
	}
}

int leave_mtrigger(CharData *actor, int dir) {
	FOREACH(CharacterList, actor->InRoom()->people, iter)
	{
		CharData *ch = *iter;
	
		if (!SCRIPT_CHECK(ch, MTRIG_LEAVE | MTRIG_LEAVE_ALL) || !AWAKE(ch) ||
				FIGHTING(ch) || (ch == actor) || AFF_FLAGGED(ch, AFF_CHARM))
			continue;

		FOREACH(TrigData::List, TRIGGERS(SCRIPT(ch)), iter)
		{
			TrigData *t = *iter;
			
			if (((IS_SET(GET_TRIG_TYPE(t), MTRIG_LEAVE) && ch->CanSee(actor) != VISIBLE_NONE) ||
					IS_SET(GET_TRIG_TYPE(t), MTRIG_LEAVE_ALL)) &&
					(t->m_Flags.test(TrigData::Script_Threadable) || t->m_Threads.empty()) &&
					(Number(1, 100) <= GET_TRIG_NARG(t)))
			{
				ADD_VAR("direction", dir != -1 ? dirs[dir] : "leave");
				ADD_VAR("actor", actor);


				if (!ScriptEngineRunScript(ch, t))
					return 0;
			}
			
			if (EFFECTIVELY_DEAD(ch) || EFFECTIVELY_DEAD(actor))
				break;
		}

		if (EFFECTIVELY_DEAD(actor))
			break;
	}
	return 1;
}


int door_mtrigger(CharData *actor, int subcmd, int dir)
{
	FOREACH(CharacterList, actor->InRoom()->people, iter)
	{
		CharData *ch = *iter;
	
		if (!SCRIPT_CHECK(ch, MTRIG_DOOR) || !AWAKE(ch) || FIGHTING(ch) ||
				(ch == actor) || AFF_FLAGGED(ch, AFF_CHARM))
			continue;

		FOREACH(TrigData::List, TRIGGERS(SCRIPT(ch)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, MTRIG_DOOR) && ch->CanSee(actor) != VISIBLE_NONE &&
					(Number(1, 100) <= GET_TRIG_NARG(t))) {
				ADD_VAR("cmd", door_act[subcmd]);
				ADD_VAR("direction", dirs[dir]);
				ADD_VAR("actor", actor);


				if (!ScriptEngineRunScript(ch, t))
					return 0;
			}
			
			if (EFFECTIVELY_DEAD(ch) || EFFECTIVELY_DEAD(actor))
				break;
		}
		
		if (EFFECTIVELY_DEAD(actor))
			break;
	}
	return 1;
}


void load_mtrigger(CharData *ch)
{
	if (!SCRIPT_CHECK(ch, MTRIG_LOAD) || EFFECTIVELY_DEAD(ch))
		return;
	
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(ch)), iter)
	{
		TrigData *t = *iter;
		
		if (TRIGGER_CHECK(t, MTRIG_LOAD) && (Number(1, 100) <= GET_TRIG_NARG(t)))
			ScriptEngineRunScript(ch, t);

		if (EFFECTIVELY_DEAD(ch))
			break;
	}
}


/*
 *  object triggers
 */

void random_otrigger(ObjData *obj) {
	if (!SCRIPT_CHECK(obj, OTRIG_RANDOM))
		return;

	FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, OTRIG_RANDOM) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			ScriptEngineRunScript(obj, t);
			if (PURGED(obj))
				break;
		}
	}
	
}


int get_otrigger(ObjData *obj, CharData *actor) {
	if (!SCRIPT_CHECK(obj, OTRIG_GET))
		return 1;

	FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, OTRIG_GET) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_VAR("actor", actor);
			
			if (!ScriptEngineRunScript(obj, t))
				return 0;
		}

		if (PURGED(obj) || EFFECTIVELY_DEAD(actor))
			break;
	}
	return 1;
}


/* checks for command trigger on specific object */
int cmd_otrig(ObjData *obj, CharData *actor, const char *cmd, const char *argument, Flags type) {
	char *newcmd;
  
	if (obj && SCRIPT_CHECK(obj, OTRIG_COMMAND) && !PURGED(obj)) {
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, OTRIG_COMMAND) && IS_SET(GET_TRIG_NARG(t), type) &&
					(/*word_check(cmd, SSData(GET_TRIG_ARG(t)))*/
					(newcmd = cmd_check(cmd, t->m_Arguments.c_str())) ||
					(*t->m_Arguments.c_str() == '*' && !IS_STAFF(actor)))) {
				ADD_VAR("actor", actor);
				skip_spaces(argument);
				
				ADD_VAR("cmd", newcmd ? newcmd : cmd);
				ADD_VAR("arg", argument);
	
				int result = ScriptEngineRunScript(obj, t);

				if (EFFECTIVELY_DEAD(actor))
					return 1;
				
				if (PURGED(obj) || result)
					return result;
			}
		}
	}

	return 0;
}


int command_otrigger(CharData *actor, const char *cmd, const char *argument) {
	for (int i = 0; i < NUM_WEARS; i++)
	{
		int result = cmd_otrig(GET_EQ(actor, i), actor, cmd, argument, OCMD_EQUIP);
		if (result)
			return result;
	}
  
	FOREACH(ObjectList, actor->carrying, obj)
  	{
		int result = cmd_otrig(*obj, actor, cmd, argument, OCMD_INVEN);
		if (result)
			return result;
	}
		
	FOREACH(ObjectList, actor->InRoom()->contents, obj)
	{
		int result = cmd_otrig(*obj, actor, cmd, argument, OCMD_ROOM);
		if (result)
			return result;
	}

	return 0;
}



/* checks for command trigger on specific object */
void death_otrig(ObjData *obj, CharData *actor, CharData *killer, Flags type) {
	if (!SCRIPT_CHECK(obj, OTRIG_DEATH) || EFFECTIVELY_DEAD(actor))
		return;
	
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
	{
		TrigData *t = *iter;
		
		if (TRIGGER_CHECK(t, OTRIG_DEATH) && IS_SET(GET_TRIG_NARG(t), type))
		{
			ADD_VAR("actor", actor);
			ADD_VAR("killer", killer);
			
			if (ScriptEngineRunScript(obj, t) || EFFECTIVELY_DEAD(actor) || PURGED(obj))
				return;
		}
		
		if (PURGED(obj) || EFFECTIVELY_DEAD(actor))
			return;
	}
}


void death_otrigger(CharData *actor, CharData *killer) {
	for (int i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(actor, i))
		{
			death_otrig(GET_EQ(actor, i), actor, killer, OCMD_EQUIP);
		
			if (EFFECTIVELY_DEAD(actor))
				return;
		}
  	}
  	
	FOREACH(ObjectList, actor->carrying, obj)
  	{
		death_otrig(*obj, actor, killer, OCMD_INVEN);
		
		if (EFFECTIVELY_DEAD(actor))
			return;
  	}
		
	FOREACH(ObjectList, actor->InRoom()->contents, obj)
	{
		death_otrig(*obj, actor, killer, OCMD_ROOM);
		
		if (EFFECTIVELY_DEAD(actor))
			return;
  	}
}


int call_otrigger(ObjData *obj, Entity *caller, const char *cmd, const char *argument, char *result) {
	if (PURGED(obj))
		return 1;
	
	if (SCRIPT_CHECK(obj, OTRIG_CALL)) {
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
		{
			TrigData *t = *iter;
		
			if (TRIGGER_CHECK(t, OTRIG_CALL) && !str_cmp(cmd, t->m_Arguments.c_str()))
			{
				ADD_VAR("caller", caller);
				skip_spaces(argument);
				ADD_VAR("arg", argument);
				return ScriptEngineRunScript(obj, t, result); // || PURGED(obj))
				//	return 1;
			}
		}
	}
	return 0;
}


/* checks for speech trigger on specific object  */
int speech_otrig(ObjData *obj, CharData *actor, const char *str, int type) {
	if (obj && SCRIPT_CHECK(obj, OTRIG_SPEECH)) {
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
		{
			TrigData *t = *iter;
			
			if (TRIGGER_CHECK(t, OTRIG_SPEECH) && IS_SET(GET_TRIG_NARG(t), type) &&
					(t->m_Arguments.empty() ||
					(IS_SET(GET_TRIG_NARG(t), 8) ? ((*t->m_Arguments.c_str() == '*') || word_check(str, t->m_Arguments.c_str())) :
					is_substring(t->m_Arguments.c_str(), str)))) {
				ADD_VAR("actor", actor);
				ADD_VAR("speech", str);
	
				if (ScriptEngineRunScript(obj, t))
					return 1;
			}
			
			if (PURGED(obj) || EFFECTIVELY_DEAD(actor))
				break;
		}
	}
	return 0;
}


void speech_otrigger(CharData *actor, const char *str)
{
	for (int i = 0; i < NUM_WEARS; i++)
		if (speech_otrig(GET_EQ(actor, i), actor, str, OCMD_EQUIP) || EFFECTIVELY_DEAD(actor))
			return;
  
	FOREACH(ObjectList, actor->carrying, obj)
	{
		if (speech_otrig(*obj, actor, str, OCMD_INVEN) || EFFECTIVELY_DEAD(actor))
			return;
	}

	FOREACH(ObjectList, actor->InRoom()->contents, obj)
	{
		if (speech_otrig(*obj, actor, str, OCMD_ROOM) || EFFECTIVELY_DEAD(actor))
			return;
	}
}


int greet_otrigger(CharData *actor, int dir)
{
	int	result = 1;
	
 	if (EFFECTIVELY_DEAD(actor))
 		return 1;
 
	FOREACH(ObjectList, actor->InRoom()->contents, iter)
 	{
 		ObjData *obj = *iter;
 		
		if (!SCRIPT_CHECK(obj, OTRIG_GREET))
			continue;

		FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, OTRIG_GREET) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
				ADD_VAR("direction", dirs[rev_dir[dir]]);
				ADD_VAR("actor", actor);


				if (!ScriptEngineRunScript(obj, t) || EFFECTIVELY_DEAD(actor))
					result = 0;
			}
			if (PURGED(obj) || EFFECTIVELY_DEAD(actor))
				break;
		}

		if (EFFECTIVELY_DEAD(actor))
			break;
	}
	
	return result;
}


int enter_otrigger(CharData *actor, RoomData *room, int dir)
{
	if (room == NULL)
		return 1;
	
	FOREACH(ObjectList, room->contents, iter)
	{
		ObjData *obj = *iter;
		
		if (!SCRIPT_CHECK(obj, OTRIG_ENTER))
			continue;

		FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, OTRIG_ENTER) && (Number(1, 100) <= GET_TRIG_NARG(t)))
			{
				ADD_VAR("direction", dir != -1 ? dirs[rev_dir[dir]] : "enter");
				ADD_VAR("actor", actor);
				if (!ScriptEngineRunScript(obj, t))
					return 0;
			}

			if (PURGED(obj) || EFFECTIVELY_DEAD(actor))
				break;
		}
		
		if (EFFECTIVELY_DEAD(actor))
			break;
	}
	return 1;
}


int wear_otrig(ObjData *obj, ObjData *what, CharData *actor, int where, Flags type)
{
	if (!SCRIPT_CHECK(obj, OTRIG_WEAR) || PURGED(what) || EFFECTIVELY_DEAD(actor))
		return 1;

	FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
	{
		TrigData *t = *iter;
		
		if (TRIGGER_CHECK(t, OTRIG_WEAR)
			&& (IS_SET(GET_TRIG_NARG(t), type) || (type == GET_TRIG_NARG(t))))
		{
			ADD_VAR("actor", actor);
			ADD_VAR("object", what);
			if (!ScriptEngineRunScript(obj, t) || EFFECTIVELY_DEAD(actor) || PURGED(what) || what->WornBy())
				return 0;
		
			if (PURGED(obj))
				break;
		}
	}
	
	return 1;
}


int wear_otrigger(ObjData *what, CharData *actor, int where)
{
	if (!wear_otrig(what, what, actor, where, 0) || EFFECTIVELY_DEAD(actor) || PURGED(what))
		return 0;
	
	for (int i = 0; i < NUM_WEARS; i++)
	{
		if (!GET_EQ(actor, i))
			continue;
	
		if (!wear_otrig(GET_EQ(actor, i), what, actor, where, OCMD_EQUIP) || EFFECTIVELY_DEAD(actor) || PURGED(what))
			return 0;
	}
  	
	FOREACH(ObjectList, actor->carrying, obj)
  	{
		if (!wear_otrig(*obj, what, actor, where, OCMD_INVEN) || EFFECTIVELY_DEAD(actor) || PURGED(what))
			return 0;
  	}
		
	FOREACH(ObjectList, actor->InRoom()->contents, obj)
	{
		if (!wear_otrig(*obj, what, actor, where, OCMD_ROOM) || EFFECTIVELY_DEAD(actor) || PURGED(what))
			return 0;
  	}
  	
  	return 1;
}


int remove_otrig(ObjData *obj, ObjData *what, CharData *actor, int where, Flags type)
{
	if (!SCRIPT_CHECK(obj, OTRIG_REMOVE) || PURGED(what) || EFFECTIVELY_DEAD(actor))
		return 1;

	FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
	{
		TrigData *t = *iter;
		
		if (TRIGGER_CHECK(t, OTRIG_REMOVE)
			&& (IS_SET(GET_TRIG_NARG(t), type) || (type == GET_TRIG_NARG(t))))
		{
			ADD_VAR("actor", actor);
			ADD_VAR("object", what);
			if (!ScriptEngineRunScript(obj, t) || EFFECTIVELY_DEAD(actor) || PURGED(what) || what->WornBy() != actor)
				return 0;
		
			if (PURGED(obj))
				break;
		}
	}
	
	return 1;
}


int remove_otrigger(ObjData *what, CharData *actor, int where)
{
	if (!remove_otrig(what, what, actor, where, 0) || EFFECTIVELY_DEAD(actor) || PURGED(what))
		return 0;
	
	for (int i = 0; i < NUM_WEARS; i++)
	{
		if (!GET_EQ(actor, i))
			continue;
	
		if (!remove_otrig(GET_EQ(actor, i), what, actor, where, OCMD_EQUIP) || EFFECTIVELY_DEAD(actor) || PURGED(what))
			return 0;
	}
  	
	FOREACH(ObjectList, actor->carrying, obj)
  	{
		if (!remove_otrig(*obj, what, actor, where, OCMD_INVEN) || EFFECTIVELY_DEAD(actor) || PURGED(what))
			return 0;
  	}
		
	FOREACH(ObjectList, actor->InRoom()->contents, obj)
	{
		if (!remove_otrig(*obj, what, actor, where, OCMD_ROOM) || EFFECTIVELY_DEAD(actor) || PURGED(what))
			return 0;
  	}
  	
  	return 1;
}


int consume_otrigger(ObjData *obj, CharData *actor) {
	if (!SCRIPT_CHECK(obj, OTRIG_CONSUME))
		return 1;

	FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, OTRIG_CONSUME) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_VAR("actor", actor);
			if (!ScriptEngineRunScript(obj, t) || EFFECTIVELY_DEAD(actor) || PURGED(obj))
				return 0;
		}
	}
	return 1;
}


int drop_otrigger(ObjData *obj, CharData *actor) {
	if (!SCRIPT_CHECK(obj, OTRIG_DROP))
		return 1;

	FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, OTRIG_DROP) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_VAR("actor", actor);
			if (!ScriptEngineRunScript(obj, t) || EFFECTIVELY_DEAD(actor) || PURGED(obj) || obj->CarriedBy() != actor)
				return 0;
		}
	}

	return 1;
}


int give_otrigger(ObjData *obj, CharData *actor, CharData *victim)
{
	if (!SCRIPT_CHECK(obj, OTRIG_GIVE))
		return 1;

	FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, OTRIG_GIVE) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_VAR("actor", actor);
			ADD_VAR("victim", victim);
			if (!ScriptEngineRunScript(obj, t) || EFFECTIVELY_DEAD(actor) || PURGED(obj) || EFFECTIVELY_DEAD(victim) || obj->CarriedBy() != actor)
				return 0;
		}
	}
	
	return 1;
}


int put_otrigger(ObjData *obj, ObjData *container, CharData *actor) {
	if (SCRIPT_CHECK(obj, OTRIG_PUT))
	{
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, OTRIG_PUT) && GET_TRIG_NARG(t) == 0) {
				ADD_VAR("object", obj);
				ADD_VAR("container", container);
				ADD_VAR("actor", actor);
				if (!ScriptEngineRunScript(obj, t) || EFFECTIVELY_DEAD(actor) || PURGED(obj) || PURGED(container) || obj->CarriedBy() != actor)
					return 0;
			}
		}
	}

	if (SCRIPT_CHECK(container, OTRIG_PUT))
	{
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(container)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, OTRIG_PUT) && GET_TRIG_NARG(t) == 1) {
				ADD_VAR("object", obj);
				ADD_VAR("container", container);
				ADD_VAR("actor", actor);
				if (!ScriptEngineRunScript(container, t) || EFFECTIVELY_DEAD(actor) || PURGED(obj) || PURGED(container) || obj->CarriedBy() != actor)
					return 0;
			}
		}
	}
	
	return 1;
}


int retrieve_otrigger(ObjData *obj, ObjData *container, CharData *actor)
{
	if (SCRIPT_CHECK(container, OTRIG_RETRIEVE))
	{
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(container)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, OTRIG_RETRIEVE))
			{
				ADD_VAR("object", obj);
				ADD_VAR("actor", actor);
				if (!ScriptEngineRunScript(container, t) || EFFECTIVELY_DEAD(actor) || PURGED(obj) || PURGED(container) || obj->Inside() != container)
					return 0;
			}
		}
	}
	
	return 1;
}


void store_otrigger(ObjData *obj)
{
	if (SCRIPT_CHECK(obj, OTRIG_STORE))
	{
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, OTRIG_STORE))
			{
				ScriptEngineRunScript(obj, t);
				
				if (PURGED(obj))
					break;
			}
		}
	}
}


int sit_otrigger(ObjData *obj, CharData *actor) {
	if (!SCRIPT_CHECK(obj, OTRIG_SIT))
		return 1;

	FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, OTRIG_SIT) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_VAR("actor", actor);
			if (!ScriptEngineRunScript(obj, t) || EFFECTIVELY_DEAD(actor) || PURGED(obj))
				return 0;
		}
	}
	
	return 1;
}


int leave_otrigger(CharData *actor, int dir) {
	FOREACH(ObjectList, actor->InRoom()->contents, iter)
	{
		ObjData *obj = *iter;
		
		if (!SCRIPT_CHECK(obj, OTRIG_LEAVE) || PURGED(obj))
			continue;

		FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, OTRIG_LEAVE) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
				ADD_VAR("direction", dir != -1 ? dirs[dir] : "leave");
				ADD_VAR("actor", actor);
				if (!ScriptEngineRunScript(obj, t) || EFFECTIVELY_DEAD(actor))
					return 0;
				
				if (PURGED(obj))
					break;
			}
		}
	}
	
	return 1;
}


int door_otrigger(CharData *actor, int subcmd, int dir) {
	FOREACH(ObjectList, actor->InRoom()->contents, iter)
	{
		ObjData *obj = *iter;
		
		if (!SCRIPT_CHECK(obj, MTRIG_DOOR))
			continue;

		FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, OTRIG_DOOR) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
				ADD_VAR("cmd", door_act[subcmd]);
				ADD_VAR("direction", dirs[dir]);
				ADD_VAR("actor", actor);


				if (!ScriptEngineRunScript(obj, t) || EFFECTIVELY_DEAD(actor))
					return 0;
			}
			
			if (PURGED(obj))
				break;
		}
	}
	return 1;
}



void load_otrigger(ObjData *obj) {
	if (!SCRIPT_CHECK(obj, OTRIG_LOAD) || PURGED(obj))
		return;
	
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, OTRIG_LOAD) && (Number(1, 100) <= GET_TRIG_NARG(t)))
			ScriptEngineRunScript(obj, t);
		if (PURGED(obj))
			break;
	}
}


void timer_otrigger(ObjData *obj) {
	if (!SCRIPT_CHECK(obj, OTRIG_TIMER) || PURGED(obj))
		return;
	
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, OTRIG_TIMER))
			ScriptEngineRunScript(obj, t);
		if (PURGED(obj))
			break;
	}
}


void start_otrigger(ObjData *obj, CharData *actor)
{
	FOREACH(ObjectList, obj->contents, iter)
	{
		start_otrigger(*iter, actor);
		
		if (PURGED(obj) || EFFECTIVELY_DEAD(actor))
			return;
	}
	
	load_otrigger(obj);
	
	if (PURGED(obj) || EFFECTIVELY_DEAD(actor) || !SCRIPT_CHECK(obj, OTRIG_START))
		return;
	
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, OTRIG_START) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_VAR("actor", actor);
			ScriptEngineRunScript(obj, t);
			if (PURGED(obj) || EFFECTIVELY_DEAD(actor))
				break;
		}
	}
}


void quit_otrigger(ObjData *obj, CharData *actor)
{
	FOREACH(ObjectList, obj->contents, iter)
	{
		quit_otrigger(*iter, actor);
	
		if (PURGED(obj) || EFFECTIVELY_DEAD(actor))
			return;
	}
	
	if (PURGED(obj) || EFFECTIVELY_DEAD(actor) || !SCRIPT_CHECK(obj, OTRIG_QUIT))
		return;
	
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
	{
		TrigData *t = *iter;
		if (!TRIGGER_CHECK(t, OTRIG_QUIT) || (Number(1, 100) > GET_TRIG_NARG(t)))
			continue;

		ADD_VAR("actor", actor);
		ScriptEngineRunScript(obj, t);
		
		if (PURGED(obj) || EFFECTIVELY_DEAD(actor))
			break;
	}
}


#define OCMD_ATTACK_ALWAYS		(1 << 3)		/* obj must be in char's room  */


/* checks for attack trigger on specific object  */
void attack_mtrigger(CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, int hits, int headshots)
{
	if (EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim) || !SCRIPT_CHECK(actor, MTRIG_ATTACK))
		return;
	
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(actor)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, MTRIG_ATTACK)
			&& (hits > 0 || IS_SET(GET_TRIG_NARG(t), OCMD_ATTACK_ALWAYS)))
		{
			ADD_VAR("actor", actor);	//	self
			ADD_VAR("victim", victim);
			ADD_VAR("damage", damage);
			ADD_VAR("damagetype", findtype(damagetype, damage_types));
			ADD_VAR("weapon", weapon);
			ADD_VAR("headshot", headshots);
			ADD_VAR("hits", hits);
			
			if (!ScriptEngineRunScript(actor, t) || EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim))
				return;
		}
	}
}



/* checks for attack trigger on specific object  */
void attack_otrig(ObjData *obj, CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, Flags type, int hits, int headshots)
{
	if (!obj || !SCRIPT_CHECK(obj, OTRIG_ATTACK))
		return;
	
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, OTRIG_ATTACK)
			&& (IS_SET(GET_TRIG_NARG(t) & OCMD_FLAGS, type) || type == (GET_TRIG_NARG(t) & OCMD_FLAGS))
			&& (hits > 0 || IS_SET(GET_TRIG_NARG(t), OCMD_ATTACK_ALWAYS)))
		{
			ADD_VAR("actor", actor);
			ADD_VAR("victim", victim);
			ADD_VAR("damage", damage);
			ADD_VAR("damagetype", findtype(damagetype, damage_types));
			ADD_VAR("weapon", weapon);
			ADD_VAR("headshot", headshots);
			ADD_VAR("hits", hits);
			
			if (!ScriptEngineRunScript(obj, t) || PURGED(obj) || EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim))
				return;
		}
	}
}


void attack_otrigger_for_weapon(CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, int hits, int headshots)
{
	if (EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim))
		return;
	
	if (weapon)	attack_otrig(weapon, actor, victim, weapon, damage, damagetype, 0, hits, headshots);
}


void attack_otrigger(CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, int hits, int headshots)
{
	if (EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim))
		return;
	
	for (int i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(actor, i))
		{
			attack_otrig(GET_EQ(actor, i), actor, victim, weapon, damage, damagetype, OCMD_EQUIP, hits, headshots);
		
			if (EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim))
				return;
		}
  	}
  	
	
	FOREACH(ObjectList, actor->carrying, obj)
	{
		attack_otrig(*obj, actor, victim, weapon, damage, damagetype, OCMD_INVEN, hits, headshots);
		
		if (EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim))
			return;
	}

	FOREACH(ObjectList, actor->InRoom()->contents, obj)
	{
		attack_otrig(*obj, actor, victim, weapon, damage, damagetype, OCMD_ROOM, hits, headshots);

		if (EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim))
			return;
	}
}


/* checks for attack trigger on specific object  */
void attacked_mtrigger(CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, int hits, int headshots)
{
	if (EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim) || !SCRIPT_CHECK(victim, MTRIG_ATTACKED))
		return;
	
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(victim)), iter)
	{
		TrigData *t = *iter;

		if (TRIGGER_CHECK(t, MTRIG_ATTACKED)
			&& (hits > 0 || IS_SET(GET_TRIG_NARG(t), OCMD_ATTACK_ALWAYS)))
		{
			ADD_VAR("attacker", actor);
			ADD_VAR("actor", victim);	//	self
			ADD_VAR("damage", damage);
			ADD_VAR("damagetype", findtype(damagetype, damage_types));
			ADD_VAR("weapon", weapon);
			ADD_VAR("headshot", headshots);
			ADD_VAR("hits", hits);

			if (!ScriptEngineRunScript(victim, t) || EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim))
				return;
		}
	}
}


/* checks for attack trigger on specific object  */
int attacked_otrig(ObjData *obj, CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, Flags type, int hits, int headshots)
{
	if (obj && SCRIPT_CHECK(obj, OTRIG_ATTACKED))
	{
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
		{
			TrigData *t = *iter;

			if (TRIGGER_CHECK(t, OTRIG_ATTACKED)
				&& IS_SET(GET_TRIG_NARG(t) & OCMD_FLAGS, type)
				&& (hits || IS_SET(GET_TRIG_NARG(t), OCMD_ATTACK_ALWAYS)))
			{
				ADD_VAR("attacker", actor);
				ADD_VAR("actor", victim);
				ADD_VAR("damage", damage);
				ADD_VAR("damagetype", findtype(damagetype, damage_types));
				ADD_VAR("weapon", weapon);
				ADD_VAR("headshot", headshots);
				ADD_VAR("hits", hits);
	
				if (!ScriptEngineRunScript(obj, t) || EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim)) {
					return 0;
				}
			}
		}
	}
	return 1;
}


void attacked_otrigger(CharData *actor, CharData *victim, ObjData *weapon, int damage, int damagetype, int hits, int headshots)
{
	Flags hitFlags = hits ? 0 : OCMD_ATTACK_ALWAYS;
	
	if (EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim))
		return;
	
	for (int i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(victim, i))
			attacked_otrig(GET_EQ(victim, i), actor, victim, weapon, damage, damagetype, OCMD_EQUIP, hits, headshots);
		
		if (EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim))
			return;
  	}
  	
	
	FOREACH(ObjectList, victim->carrying, obj)
	{
		attacked_otrig(*obj, actor, victim, weapon, damage, damagetype, OCMD_INVEN, hits, headshots);
		
		if (EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim))
			return;
	}

	FOREACH(ObjectList, victim->InRoom()->contents, obj)
	{
		attacked_otrig(*obj, actor, victim, weapon, damage, damagetype, OCMD_ROOM, hits, headshots);
		
		if (EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim))
			return;
	}
	
	return;
}



/* checks for attack trigger on specific object  */
void motion_otrig(ObjData *obj, CharData *actor, int dir, int motion, bool lateral, int distance, int motionType, Flags type)
{
	const char *motionTypes[] =
	{
		"entering",
		"leaving"
	};
	
	if (obj && SCRIPT_CHECK(obj, OTRIG_MOTION))
	{
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, OTRIG_MOTION) && IS_SET(GET_TRIG_NARG(t), type))
			{
				ADD_VAR("actor", actor);
				ADD_VAR("direction", dirs[dir]);
				ADD_VAR("motion", dirs[motion]);
				ADD_VAR("lateral", lateral ? "1" : "0");
				ADD_VAR("distance", distance);
				ADD_VAR("type", motionTypes[motionType]);
				
				ScriptEngineRunScript(obj, t);

				if (EFFECTIVELY_DEAD(actor) || PURGED(obj))
					break;
			}
		}
	}
}


void motion_otrigger(CharData *ch, CharData *actor, int dir, int motion, bool lateral, int distance, int motionType)
{
	if (EFFECTIVELY_DEAD(ch) || EFFECTIVELY_DEAD(actor))
		return;
		
	for (int i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i))
			motion_otrig(GET_EQ(ch, i), actor, dir, motion, lateral, distance, motionType, OCMD_EQUIP);
		
		if (EFFECTIVELY_DEAD(ch) || EFFECTIVELY_DEAD(actor))
			return;
  	}
  	
	
	FOREACH(ObjectList, ch->carrying, obj)
	{
		motion_otrig(*obj, actor, dir, motion, lateral, distance, motionType, OCMD_INVEN);
		
		if (EFFECTIVELY_DEAD(ch) || EFFECTIVELY_DEAD(actor))
			return;
	}
	
	return;
}


void room_motion_otrigger(RoomData * room, CharData *actor, int dir, int motion, bool lateral, int distance, int motionType)
{	
	if (EFFECTIVELY_DEAD(actor))
		return;
			
	FOREACH(ObjectList, room->contents, obj)
	{
		motion_otrig(*obj, actor, dir, motion, lateral, distance, motionType, OCMD_ROOM);
		
		if (EFFECTIVELY_DEAD(actor))
			return;
	}
	
}


void motion_mtrigger(CharData *ch, CharData *actor, int dir, int motion, bool lateral, int distance, int motionType)
{
	const char *motionTypes[] =
	{
		"entering",
		"leaving"
	};
	
	if (EFFECTIVELY_DEAD(ch) || EFFECTIVELY_DEAD(actor) || !SCRIPT_CHECK(ch, MTRIG_MOTION))
		return;
	
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(ch)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, MTRIG_MOTION))
		{
			ADD_VAR("actor", actor);
			ADD_VAR("direction", dirs[dir]);
			ADD_VAR("motion", dirs[motion]);
			ADD_VAR("lateral", lateral ? "1" : "0");
			ADD_VAR("distance", distance);
			ADD_VAR("type", motionTypes[motionType]);
			
			ScriptEngineRunScript(ch, t);
			
			if (EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(ch))
				break;
		}
	}
}


void motion_wtrigger(RoomData * room, CharData *actor, int dir, int motion, bool lateral, int distance, int motionType)
{
	const char *motionTypes[] =
	{
		"entering",
		"leaving"
	};
	
	if (EFFECTIVELY_DEAD(actor) || !SCRIPT_CHECK(room, WTRIG_MOTION))
		return;
		
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(room)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, WTRIG_MOTION))
		{
			ADD_VAR("actor", actor);
			ADD_VAR("direction", dirs[dir]);
			ADD_VAR("motion", dirs[motion]);
			ADD_VAR("lateral", lateral ? "1" : "0");
			ADD_VAR("distance", distance);
			ADD_VAR("type", motionTypes[motionType]);

			ScriptEngineRunScript(room, t);
			
			if (EFFECTIVELY_DEAD(actor))
				break;
		}
	}
}


int install_otrigger(ObjData *obj, CharData *actor, ObjData *installed, float skillroll) {
	BUFFER(buf, MAX_INPUT_LENGTH);
	
	if (SCRIPT_CHECK(obj, OTRIG_INSTALL))
	{
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, OTRIG_INSTALL) && !GET_TRIG_NARG(t)) {
				ADD_VAR("actor", actor);
				ADD_VAR("installed", installed);
				ADD_VAR("skillroll", skillroll);
				
				if (ScriptEngineRunScript(obj, t) || PURGED(obj))
					break;
			}
		}
	}

	if (installed && !PURGED(installed) && SCRIPT_CHECK(installed, OTRIG_INSTALL))
	{
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(installed)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, OTRIG_INSTALL) && GET_TRIG_NARG(t)) {
				ADD_VAR("actor", actor);
				ADD_VAR("installable", obj);
				ADD_VAR("skillroll", skillroll);
				
				if (ScriptEngineRunScript(installed, t) || PURGED(installed))
					break;
			}
		}
	}
	return 1;
}


int destroyrepair_otrigger(ObjData *obj, CharData *actor, int repairing)
{
	if (SCRIPT_CHECK(obj, OTRIG_DESTROYREPAIR))
	{
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, OTRIG_DESTROYREPAIR) && (GET_TRIG_NARG(t) == repairing)) {
				ADD_VAR("actor", actor);
				
				if (ScriptEngineRunScript(obj, t) || PURGED(obj) || EFFECTIVELY_DEAD(actor))
					return 1;
			}
		}
	}
	
	return 0;
}



/* checks for command trigger on specific object */
void kill_otrig(ObjData *obj, CharData *actor, CharData *victim, int type)
{
	if (obj && SCRIPT_CHECK(obj, OTRIG_KILL))
	{
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(obj)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, OTRIG_KILL) && (IS_SET(GET_TRIG_NARG(t), type) /*||
				((GET_TRIG_NARG(t) == 0) && (obj == weapon))*/ ))
			{
				ADD_VAR("actor", actor);
				ADD_VAR("victim", victim);
				
				if (!ScriptEngineRunScript(obj, t) || EFFECTIVELY_DEAD(actor) || PURGED(obj) || EFFECTIVELY_DEAD(victim))
					return;
			}
		}
	}
}


void kill_otrigger(CharData *actor, CharData *victim) {
	for (int i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(actor, i))
			kill_otrig(GET_EQ(actor, i), actor, victim, OCMD_EQUIP);
		
		if (EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim))
			return;
  	}
  	
  	FOREACH(ObjectList, actor->carrying, obj)
  	{
		kill_otrig(*obj, actor, victim, OCMD_INVEN);
		
		if (EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim))
			return;
	}
		
  	FOREACH(ObjectList, actor->InRoom()->contents, obj)
	{
		kill_otrig(*obj, actor, victim, OCMD_ROOM);
				
		if (EFFECTIVELY_DEAD(actor) || EFFECTIVELY_DEAD(victim))
			return;
	}
}




/*
 *  world triggers
 */

void random_wtrigger(RoomData *room) {
	if (!SCRIPT_CHECK(room, WTRIG_RANDOM))
		return;

	FOREACH(TrigData::List, TRIGGERS(SCRIPT(room)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, WTRIG_RANDOM) && (Number(1, 100) <= GET_TRIG_NARG(t)))
		{
			ScriptEngineRunScript(room, t);
		}
	}
}


int enter_wtrigger(RoomData *room, CharData *actor, int dir) {
	if (!SCRIPT_CHECK(room, WTRIG_ENTER))
		return 1;
	
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(room)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, WTRIG_ENTER) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_VAR("direction", dir != -1 ? dirs[rev_dir[dir]] : "enter");
			ADD_VAR("actor", actor);
			if (!ScriptEngineRunScript(room, t) || EFFECTIVELY_DEAD(actor))
				return 0;
		}
	}
	return 1;
}


int greet_wtrigger(RoomData *room, CharData *actor, int dir) {
	if (EFFECTIVELY_DEAD(actor) || !SCRIPT_CHECK(room, WTRIG_GREET))
		return 1;
	
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(room)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, WTRIG_GREET) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_VAR("direction", dir != -1 ? dirs[rev_dir[dir]] : "enter");
			ADD_VAR("actor", actor);
			if (!ScriptEngineRunScript(room, t) || EFFECTIVELY_DEAD(actor))
				return 0;
		}
	}
	return 1;
}


int command_wtrigger(CharData *actor, const char *cmd, const char *argument) {
	char *newcmd;
	RoomData *room;

	if (!actor || !SCRIPT_CHECK(actor->InRoom(), WTRIG_COMMAND))
		return 0;

	room = actor->InRoom();
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(room)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, WTRIG_COMMAND) &&
				(/*word_check(cmd, SSData(GET_TRIG_ARG(t)))*/
				(newcmd = cmd_check(cmd, t->m_Arguments.c_str())) ||
				(*t->m_Arguments.c_str() == '*' && !IS_STAFF(actor)))) {
			ADD_VAR("actor", actor);
			skip_spaces(argument);
			ADD_VAR("cmd", newcmd ? newcmd : cmd);
			ADD_VAR("arg", argument);
			
			int result = ScriptEngineRunScript(room, t);
			
			if (EFFECTIVELY_DEAD(actor))
				result = 1;
			if (result)
				return result;
		}
	}

	return 0;
}


int call_wtrigger(RoomData *room, Entity *caller, const char *cmd, const char *argument, char *result) {
	if (SCRIPT_CHECK(room, WTRIG_CALL)) {
		FOREACH(TrigData::List, TRIGGERS(SCRIPT(room)), iter)
		{
			TrigData *t = *iter;
			if (TRIGGER_CHECK(t, WTRIG_CALL) && !str_cmp(cmd, t->m_Arguments.c_str())) {
				ADD_VAR("caller", caller);
				skip_spaces(argument);
				ADD_VAR("arg", argument);
				
				return ScriptEngineRunScript(room, t, result);
				//	return 1;
			}
		}
	}
	return 0;
}


void speech_wtrigger(CharData *actor, const char *str) {
	RoomData *room;

	if (!actor || (actor->InRoom() == NULL) || !SCRIPT_CHECK(actor->InRoom(), WTRIG_SPEECH))
		return;

	room = actor->InRoom();
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(room)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, WTRIG_SPEECH) && (t->m_Arguments.empty() ||
				(GET_TRIG_NARG(t) ?
				(word_check(str, t->m_Arguments.c_str()) || (*t->m_Arguments.c_str() == '*')) :
				is_substring(t->m_Arguments.c_str(), str)))) {
			ADD_VAR("actor", actor);
			ScriptEngineRunScript(room, t);
			break;
		}
	}
}


int drop_wtrigger(ObjData *obj, CharData *actor) {
	RoomData *room;

	if (!actor || !SCRIPT_CHECK(actor->InRoom(), WTRIG_DROP))
		return 1;

	room = actor->InRoom();
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(room)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, WTRIG_DROP) && (Number(1, 100) <= GET_TRIG_NARG(t))) {	
			ADD_VAR("actor", actor);
			ADD_VAR("object", obj);
			if (!ScriptEngineRunScript(room, t) || EFFECTIVELY_DEAD(actor) || PURGED(obj) || obj->CarriedBy() != actor)
				return 0;
		}	
	}
	
	return 1;
}


int leave_wtrigger(RoomData *room, CharData *actor, int dir) {
	if (!SCRIPT_CHECK(room, WTRIG_LEAVE))
		return 1;

	FOREACH(TrigData::List, TRIGGERS(SCRIPT(room)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, WTRIG_LEAVE) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_VAR("direction", dir != -1 ? dirs[dir] : "leave");
			ADD_VAR("actor", actor);
			if (!ScriptEngineRunScript(room, t) || EFFECTIVELY_DEAD(actor))
				return 0;
		}
	}

	return 1;
}


int door_wtrigger(CharData *actor, int subcmd, int dir) {
	RoomData *room;

	if (!actor || !SCRIPT_CHECK(actor->InRoom(), WTRIG_DOOR))
		return 1;

	room = actor->InRoom();
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(room)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, WTRIG_DOOR) && (Number(1, 100) <= GET_TRIG_NARG(t))) {
			ADD_VAR("cmd", door_act[subcmd]);
			ADD_VAR("direction", dirs[dir]);
			ADD_VAR("actor", actor);
			if (!ScriptEngineRunScript(room, t) || EFFECTIVELY_DEAD(actor))
				return 0;
		}
	}

	return 1;
}


void reset_wtrigger(RoomData *room) {
	if (!SCRIPT_CHECK(room, WTRIG_RESET))
		return;
	
	FOREACH(TrigData::List, TRIGGERS(SCRIPT(room)), iter)
	{
		TrigData *t = *iter;
		if (TRIGGER_CHECK(t, WTRIG_RESET) && (Number(1, 100) <= GET_TRIG_NARG(t)))
			ScriptEngineRunScript(room, t);
	}
}


extern TrigData::List	globalTriggers;

int command_gtrigger(CharData *actor, const char *cmd, const char *argument)
{
	char *newcmd;

	skip_spaces(argument);
	
	FOREACH(TrigData::List, globalTriggers, iter)
	{
		TrigData *t = *iter;
		
		if (t->m_Type == GLOBAL_TRIGGER
			&& TRIGGER_CHECK(t, GTRIG_COMMAND)
			&& ((newcmd = cmd_check(cmd, t->m_Arguments.c_str()))
				/*|| (*SSData(GET_TRIG_ARG(t)) == '*' && !IS_STAFF(actor))*/))
		{
			TrigData * newTrig = new TrigData;
			newTrig->SetName("<Global>");
			newTrig->m_Type = MOB_TRIGGER;
			trig_list.push_front(newTrig);
			actor->GetScript()->AddTrigger(newTrig);
			
			TrigThread *newThread = new TrigThread(newTrig, actor, TrigThread::DontCreateRoot);
			newThread->PushCallstack(t->m_pCompiledScript, t->m_pCompiledScript->m_Bytecode + 1, /*"<root>"*/ "");
			
			newThread->GetCurrentVariables().Add("actor", actor);
			newThread->GetCurrentVariables().Add("cmd", newcmd ? newcmd : cmd);
			newThread->GetCurrentVariables().Add("arg", argument);

			if (*(newThread->GetCurrentCompiledScript()->m_Bytecode) != SBC_START)
			{
				ScriptEngineReportError(NULL, true, "Script \"%s\" [%s] doesn't start with SBC_START opcode!",
						t->GetName(), t->GetVirtualID().Print().c_str());
				continue;
			}
		
			int result = ScriptEngineRunScript(newThread);
			
			if (result)
				return result;
		}
	
		if (EFFECTIVELY_DEAD(actor))
			break;
	}
			
	return 0;
}
