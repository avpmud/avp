/* ************************************************************************
*   File: spec_procs.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "structs.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "db.h"
#include "spells.h"

/*   external vars  */
extern struct TimeInfoData time_info;
extern char *stat_types[];

/* extern functions */
void AlterMove(CharData *ch, int amount);
void add_follower(CharData * ch, CharData * leader);
char *fname(char *namelist);
ACMD(do_say);
ACMD(do_drop);
ACMD(do_gen_door);


struct social_type {
  char *cmd;
  int next_line;
};

/* local functions */
void sort_skills(void);
char *how_good(int percent);
void list_skills(CharData * ch);
void npc_steal(CharData * ch, CharData * victim);
SPECIAL(guild);
SPECIAL(dump);
SPECIAL(mayor);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(guild_guard);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(cityguard);
SPECIAL(pet_shops);
SPECIAL(bank);
SPECIAL(temp_guild);
SPECIAL(shiftsuit);
SPECIAL(hiver);

/* ********************************************************************
*  Special procedures for mobiles                                     *
******************************************************************** */

int skill_sort_info[NUM_SKILLS+1];

static int sort_skills_func(int a, int b)
{
	int result = 0;
	
	if (skill_info[skill_sort_info[a]].name[0] == '!')	result = 1;		//	a is greater
	if (skill_info[skill_sort_info[b]].name[0] == '!')	result = -1;	//	b is greater
	if (result == 0)	result = skill_info[skill_sort_info[a]].stat - skill_info[skill_sort_info[b]].stat;
	if (result == 0)	result = str_cmp(skill_info[skill_sort_info[a]].name, skill_info[skill_sort_info[b]].name);
	
	return result;
}


void sort_skills(void)
{
	int a, b, tmp;

	/* initialize array */
	for (a = 1; a < NUM_SKILLS; a++)
		skill_sort_info[a] = a;

	/* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
	for (a = 1; a < NUM_SKILLS - 1; ++a)
		for (b = a + 1; b < NUM_SKILLS; ++b)
		{
/*			if (skill_info[skill_sort_info[a]].name[0] != '!')
			{
				if (skill_info[skill_sort_info[a]].stat < skill_info[skill_sort_info[b]].stat)
					continue;
				if ((skill_info[skill_sort_info[a]].stat == skill_info[skill_sort_info[b]].stat) &&
					str_cmp(skill_info[skill_sort_info[a]].name, skill_info[skill_sort_info[b]].name) <= 0)
					continue;
			}
*/
			if (sort_skills_func(a, b) <= 0)
				continue;
						
			tmp = skill_sort_info[a];
			skill_sort_info[a] = skill_sort_info[b];
			skill_sort_info[b] = tmp;
		}
}


char *how_good(int percent) {
	static char *buf;

	if (percent == 0)			buf = " (not learned)";
	else if (percent <= 10)		buf = " (awful)";
	else if (percent <= 20)		buf = " (bad)";
	else if (percent <= 40)		buf = " (below average)";
	else if (percent <= 55)		buf = " (average)";
	else if (percent <= 70)		buf = " (above average)";
	else if (percent <= 80)		buf = " (good)";
	else if (percent <= 90)		buf = " (very good)";
	else if (percent <= 95)		buf = " (superb)";
	else						buf = " (expert)";

	return (buf);
}

//#define LEARNED_LEVEL	0	/* % known which is considered "learned" */
//#define MAX_PER_PRAC	1	/* max percent gain in skill per practice */
//#define MIN_PER_PRAC	2	/* min percent gain in skill per practice */

/* actual prac_params are in races.c */
//extern int prac_params[3][NUM_RACES];

//#define LEARNED(ch) (prac_params[LEARNED_LEVEL][(int)ch->GetRace()])
//#define MINGAIN(ch) (prac_params[MIN_PER_PRAC][(int)ch->GetRace()])
//#define MAXGAIN(ch) (prac_params[MAX_PER_PRAC][(int)ch->GetRace()])

void list_skills(CharData * ch) {
	int i, sortpos, prevstat;
	BUFFER(buf, MAX_STRING_LENGTH);

/*	if (!GET_PRACTICES(ch))
		strcpy(buf, "You have no Skill Points remaining.\n");
	else
		sprintf(buf, "You have %d Skill Point%s remaining.\n",
				GET_PRACTICES(ch), (GET_PRACTICES(ch) == 1 ? "" : "s"));
*/
	strcat(buf, "You know of the following skills:\n");
	
	prevstat = -1;
	for (sortpos = 1; sortpos < NUM_SKILLS; sortpos++) {
		i = skill_sort_info[sortpos];
		if (*skill_info[i].name == '!')
			continue;

		if (ch->GetLevel() >= skill_info[i].min_level[(int) ch->GetRace()]) {
			if (strlen(buf) >= MAX_STRING_LENGTH - 32) {
				strcat(buf, "**OVERFLOW**\n");
				break;
			}
		
			if (skill_info[i].stat != prevstat)
			{
				sprintf_cat(buf, "--- `c%s Skills`n\n", stat_types[skill_info[i].stat]);
				prevstat = skill_info[i].stat;
			}
			
			sprintf_cat(buf, "`y%-20s`n ", skill_info[i].name);
			if (GET_SKILL(ch, i) == GET_REAL_SKILL(ch, i))
			{
				sprintf_cat(buf, "[%3d]    ", GET_SKILL(ch, i));
			}
			else
			{
				sprintf_cat(buf, "[%3d`g%-+4d`n]",
						GET_REAL_SKILL(ch, i), GET_SKILL(ch, i) - GET_REAL_SKILL(ch, i));
			}
			sprintf_cat(buf, " | `y%s`n", skill_info[i].description);
			if (skill_info[i].cost[ch->GetRace()] > 1)
				sprintf_cat(buf, " (`cCost: %d`n)", skill_info[i].cost[ch->GetRace()]);
			strcat(buf, "\n");
		}
	}
	
	sprintf_cat(buf, "\nSkill Points remaining: %-3d   Maximum skill level: %d\n",
			ch->GetPlayer()->m_SkillPoints, GET_MAX_SKILL_ALLOWED(ch));
	
	page_string(ch->desc, buf);
}

#if 0
SPECIAL(guild)
{
  int skill_num, percent;

  if (IS_NPC(ch) || !CMD_IS("practice"))
    return 0;

  skip_spaces(argument);

  if (!*argument) {
    list_skills(ch);
    return 1;
  }
  if (GET_PRACTICES(ch) <= 0) {
    send_to_char("You do not seem to be able to practice now.\n", ch);
    return 1;
  }

  skill_num = find_skill_abbrev(argument);

  if (skill_num < 1 ||
      ch->GetLevel() < skill_info[skill_num].min_level[(int) ch->GetRace()]) {
    send_to_char("You do not know of that skill.\n", ch);
    return 1;
  }
  if (GET_SKILL(ch, skill_num) >= LEARNED(ch)) {
    send_to_char("You are already learned in that area.\n", ch);
    return 1;
  }
  send_to_char("You practice for a while...\n", ch);
  GET_PRACTICES(ch)--;

  percent = GET_SKILL(ch, skill_num) + 1;
//  percent += MIN(MAXGAIN(ch), MAX(MINGAIN(ch), GET_INT(ch) / 5));
	percent = MAX(percent, );

  SET_SKILL(ch, skill_num, MIN(LEARNED(ch), percent));

  if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
    send_to_char("You are now learned in that area.\n", ch);

  return 1;
}
#endif


extern int stat_base[NUM_RACES][7];
char *stat_names[] =
{
	"strength",
	"health",
	"coordination",
	"agility",
	"perception",
	"knowledge",
	"\n"
};


#define STAT_COST 6
SPECIAL(temp_guild)
{
  int skill_num, percent;
//  int amount, cost;
//  char *buf;
  
  if (IS_NPC(ch))
    return 0;

/*
  if (CMD_IS("buy")) {
  	skip_spaces(argument);
  	if (!*argument || (amount = atoi(argument)) <= 0) {
  		send_to_char("You must specify a number of practicess to buy at MP each.\n", ch);
  		return 1;
  	}
  	if ((cost = amount * ch->GetLevel()) > GET_MISSION_POINTS(ch)) {
  		send_to_char("You can't afford that many practices.\n", ch);
  		return 1;
  	}
  	GET_MISSION_POINTS(ch) -= cost;
  	GET_PRACTICES(ch) += amount;
  	BUFFER(buf, MAX_INPUT_LENGTH);
  	sprintf(buf, "You buy %d practices for %d Mission Points.\n", amount, cost);
  	send_to_char(buf, ch);
  	return 1;
  }
*/

	if (CMD_IS("practice"))
	{
		skip_spaces(argument);

		if (!*argument) {
			list_skills(ch);
			return 1;
		}
		else if ((skill_num = search_block(argument, stat_names, TRUE)) != -1)
		{
			if (ch->GetPlayer()->m_SkillPoints < STAT_COST)
				ch->Send("You do not have enough Skill Points to practice %s.\n", stat_names[skill_num]);
			else
			{
				unsigned short *stat = NULL;
				
				switch (skill_num)
				{
					case 0:	stat = &ch->real_abils.Strength;	break;
					case 1:	stat = &ch->real_abils.Health;		break;
					case 2:	stat = &ch->real_abils.Coordination;break;
					case 3:	stat = &ch->real_abils.Agility;		break;
					case 4:	stat = &ch->real_abils.Perception;	break;
					case 5:	stat = &ch->real_abils.Knowledge;	break;
				}
				
				if (*stat >= MAX_PC_STAT)
					ch->Send("Your %s is already at maximum.\n", stat_names[skill_num]);
				else {
					ch->GetPlayer()->m_SkillPoints -= STAT_COST;
					*stat = *stat + 1;
					ch->Send("You increase your %s by 1 (%d Skill Points).\n", stat_names[skill_num], STAT_COST);
					ch->AffectTotal();
				}
			}
			return 1;
		}

		skill_num = find_skill_abbrev(argument);

		if (skill_num < 1 || (ch->GetLevel() < skill_info[skill_num].min_level[ch->GetRace()])) {
			send_to_char("You do not know of that skill.\n", ch);
			return 1;
		}
		if (ch->GetPlayer()->m_SkillPoints < skill_info[skill_num].cost[ch->GetRace()]) {
			ch->Send("You need %d Skill Points to practice %s.\n", skill_info[skill_num].cost[ch->GetRace()], skill_info[skill_num].name);
			return 1;
		}
		if (GET_REAL_SKILL(ch, skill_num) >= MAX_SKILL_LEVEL) {
			ch->Send("You have already mastered %s.\n", skill_info[skill_num].name);
			return 1;
		}
		if (GET_REAL_SKILL(ch, skill_num) >= GET_MAX_SKILL_ALLOWED(ch)) {
			ch->Send("You cannot train any more in %s until you reach level %d.\n", skill_info[skill_num].name,
					(ch->GetLevel() < 30 ? 30 : 60));
			return 1;
		}
		
		send_to_char("You practice for a while...\n", ch);
		ch->GetPlayer()->m_SkillPoints -= skill_info[skill_num].cost[ch->GetRace()];

		percent = GET_REAL_SKILL(ch, skill_num) + 1;
		GET_REAL_SKILL(ch, skill_num) = MIN(MAX_SKILL_LEVEL, percent);

		if (GET_REAL_SKILL(ch, skill_num) >= MAX_SKILL_LEVEL)
			ch->Send("You are now an expert of %s.\n", skill_info[skill_num].name);
		else if (GET_REAL_SKILL(ch, skill_num) >= GET_MAX_SKILL_ALLOWED(ch))
			ch->Send("You have learned all you can in %s.  Continue training when you reach level %d.\n", skill_info[skill_num].name,
					(((ch->GetLevel() - 1) / 10) * 10) + 11);

		ch->AffectTotal();
		
		return 1;
	}
	else if (CMD_IS("unpractice"))
	{
		skip_spaces(argument);
		
		if ((skill_num = search_block(argument, stat_names, TRUE)) != -1)
		{
			/*if (ch->GetPlayer()->m_SkillPoints >= 0 && GET_MISSION_POINTS(ch) < 30)
				ch->Send("You need 30 MP to unpractice %s.\n", stat_names[skill_num]);
			else*/ if ((ch->real_abils.Strength + ch->real_abils.Health +
					ch->real_abils.Coordination + ch->real_abils.Agility +
					ch->real_abils.Perception + ch->real_abils.Knowledge) <=
					stat_base[ch->GetRace()][6])
				ch->Send("You cannot lower your total stats any further.  If you wish to reduce this stat, first increase another stat.\n");
			else
			{
				unsigned short *stat = NULL;
				
				switch (skill_num)
				{
					case 0:	stat = &ch->real_abils.Strength;	break;
					case 1:	stat = &ch->real_abils.Health;		break;
					case 2:	stat = &ch->real_abils.Coordination;break;
					case 3:	stat = &ch->real_abils.Agility;		break;
					case 4:	stat = &ch->real_abils.Perception;	break;
					case 5:	stat = &ch->real_abils.Knowledge;	break;
				}
				
				if (*stat <= stat_base[ch->GetRace()][skill_num])
					ch->Send("Your %s cannot go any lower.\n", stat_names[skill_num] /*, stat_base[ch->GetRace()][skill_num]*/);
				else {
					#if 0
					if (0 && ch->GetPlayer()->m_SkillPoints >= 0 && skill_num != 5 /* Knoweldge */)
					{
						send_to_char("You spend 30 MP.  ", ch);
						GET_MISSION_POINTS(ch) -= 30;
					}
					else
						send_to_char("30 MP UNPRAC COST TEMPORARILY WAIVED.\n", ch);
					#endif
					ch->GetPlayer()->m_SkillPoints += STAT_COST;
					*stat = *stat - 1;
					ch->Send("You decrease your %s by 1.\n", stat_names[skill_num] /*, *stat*/);
					ch->AffectTotal();
				}
			}
			return 1;
		}

		skill_num = find_skill_abbrev(argument);

		if (skill_num < 1 || (ch->GetLevel() < skill_info[skill_num].min_level[ch->GetRace()])) {
			send_to_char("You do not know of that skill.\n", ch);
			return 1;
		}
		if (GET_REAL_SKILL(ch, skill_num) == 0) {
			ch->Send("You haven't trained in %s.\n", skill_info[skill_num].name);
			return 1;
		}
//		if (GET_PRACTICES(ch) >= 0 && GET_MISSION_POINTS(ch) < 5) {
//			ch->Send("You need 5 Mission Points to un-practice a point of %s.\n", skill_info[skill_num].name);
//			return 1;
//		}
		//send_to_char("5 MP UNPRAC COST TEMPORARILY WAIVED.\n", ch);
		
		send_to_char("You un-practice the skill...\n", ch);
		ch->GetPlayer()->m_SkillPoints += skill_info[skill_num].cost[ch->GetRace()];
//		if (GET_PRACTICES(ch) >= 0)
//			GET_MISSION_POINTS(ch) -= 5;

		percent = GET_REAL_SKILL(ch, skill_num) - 1;
		GET_REAL_SKILL(ch, skill_num) = MAX(0, percent);

		if (GET_REAL_SKILL(ch, skill_num) == 0)
			ch->Send("You have now completely un-trained %s.\n", skill_info[skill_num].name);
		
		ch->AffectTotal();
		
		return 1;
	}
	else
		return 0;
}


SPECIAL(dump)
{
	FOREACH(ObjectList, ch->InRoom()->contents, iter)
	{
		ObjData *k = *iter;
		
		if (k->GetType() == ITEM_VEHICLE)	continue;
		
		act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
		k->Extract();
	}

	if (!CMD_IS("drop"))
		return 0;

	do_drop(ch, argument, "drop", 0);

	FOREACH(ObjectList, ch->InRoom()->contents, iter)
	{
		ObjData *k = *iter;

		if (k->GetType() == ITEM_VEHICLE)	continue;
		
		act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
		k->Extract();
	}
	return 1;
}


SPECIAL(puff)
{
  if (cmd)
    return (0);

  switch (Number(0, 60)) {
  case 0:
    do_say(ch, "My god!  It's full of stars!", "say", 0);
    return (1);
  case 1:
    do_say(ch, "How'd all those fish get up here?", "say", 0);
    return (1);
  case 2:
    do_say(ch, "I'm a very female dragon.", "say", 0);
    return (1);
  case 3:
    do_say(ch, "I've got a peaceful, easy feeling.", "say", 0);
    return (1);
  default:
    return (0);
  }
}

