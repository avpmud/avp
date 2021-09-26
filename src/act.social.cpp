/* ************************************************************************
*   File: act.social.c                                  Part of CircleMUD *
*  Usage: Functions to handle socials                                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "socials.h"
#include "characters.h"
#include "rooms.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "find.h"
#include "db.h"
#include "files.h"

/* extern variables */
extern struct command_info cmd_info[];


Social::Social() :
	act_nr(0),
	hide(false),
	min_victim_position(POS_DEAD),
	min_char_position(POS_DEAD),
	min_level_char(0)
{
}

bool Social::SortFunc(const Social *a, const Social *b)
{
	return a->sort_as < b->sort_as;
}


/* extern functions */

/* local functions */
int find_action(int cmd);
void boot_social_messages(void);
void create_command_list(void);
ACMD(do_action);
ACMD(do_insult);


SocialVector socials;

#define NUM_RESERVED_CMDS	6


int find_action(int cmd)
{
	if (socials.empty())	return -1;
	
	int bot = 0;
	int top = socials.size() - 1;
	
	do
	{
		int mid = (bot + top) / 2;

		Social *social = socials[mid];
		int social_nr = social->act_nr;
		
		if (social_nr == cmd)	return (mid);
		if (bot >= top)			break;
		if (social_nr > cmd)	top = mid - 1;
		else					bot = mid + 1;
	} while (1);
	
	return -1;
}



ACMD(do_action) {
	int act_nr;
	Social *action;
	CharData *vict;
	ObjData *targ;
	
	if ((act_nr = find_action(subcmd)) < 0) {
		send_to_char("That action is not supported.\n", ch);
		return;
	}
	action = socials[act_nr];

	BUFFER(buf, MAX_INPUT_LENGTH);
	BUFFER(buf2, MAX_INPUT_LENGTH);
	
	half_chop(argument, buf, buf2);
	
	if (*buf2 && action->char_body_found.empty()) {
		send_to_char("Sorry, this social does not support body parts.\n", ch);
	} else if (action->char_found.empty() || !*buf) {
		ch->Send("%s\n", action->char_no_arg.c_str());
		act(action->others_no_arg.c_str(), action->hide, ch, 0, 0, TO_ROOM | TO_IGNORABLE);
	} else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM))) {
		if (!action->char_obj_found.empty()
			&&	(	(targ = get_obj_in_list_vis(ch, buf, ch->carrying))
				||	(targ = get_obj_in_list_vis(ch, buf, ch->InRoom()->contents)))) {
			act(action->char_obj_found.c_str(), action->hide, ch, targ, 0, TO_CHAR | TO_IGNORABLE);
			act(action->others_obj_found.c_str(), action->hide, ch, targ, 0, TO_ROOM | TO_IGNORABLE);
		} else {
			ch->Send("%s\n", action->not_found.c_str());
		}
	} else if (vict == ch) {
		ch->Send("%s\n", action->char_auto.c_str());
		act(action->others_auto.c_str(), action->hide, ch, 0, 0, TO_ROOM);
	} else {
		if (GET_POS(vict) < action->min_victim_position)
			act("$N is not in a proper position for that.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
		else {
			if (*buf2)  {
				act(action->char_body_found.c_str(), 0, ch, (ObjData *)buf2, vict, TO_CHAR | TO_SLEEP | TO_IGNORABLE);
				act(action->others_body_found.c_str(), action->hide, ch, (ObjData *)buf2, vict, TO_NOTVICT | TO_IGNORABLE);
				act(action->vict_body_found.c_str(), action->hide, ch, (ObjData *)buf2, vict, TO_VICT | TO_IGNORABLE);
			} else  {
				act(action->char_found.c_str(), 0, ch, 0, vict, TO_CHAR | TO_SLEEP | TO_IGNORABLE);
				act(action->others_found.c_str(), action->hide, ch, 0, vict, TO_NOTVICT | TO_IGNORABLE);
				act(action->vict_found.c_str(), action->hide, ch, 0, vict, TO_VICT | TO_IGNORABLE);
			}
		}
	}
}



ACMD(do_insult) {
	CharData *victim;
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	one_argument(argument, arg);

	if (*arg) {
		if (!(victim = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
			send_to_char("Can't hear you!\n", ch);
		else {
			if (victim == ch)
				send_to_char("You feel insulted.\n", ch);
			else
			{
				act("You insult $N.\n", TRUE, ch, 0, victim, TO_CHAR);

				switch (Number(0, 2)) {
					case 0:
						if (ch->GetSex() == SEX_MALE) {
							if (victim->GetSex() == SEX_MALE)
								act("$n accuses you of fighting like a woman!", FALSE, ch, 0, victim, TO_VICT | TO_IGNORABLE);
							else
								act("$n says that women can't fight.", FALSE, ch, 0, victim, TO_VICT | TO_IGNORABLE);
						} else {		/* Ch == Woman */
							if (victim->GetSex() == SEX_MALE)
								act("$n accuses you of having the smallest... (brain?)",
										FALSE, ch, 0, victim, TO_VICT | TO_IGNORABLE);
							else
								act("$n tells you that you'd lose a beauty contest against a troll.",
										FALSE, ch, 0, victim, TO_VICT | TO_IGNORABLE);
						}
						break;
					case 1:
						act("$n calls your mother a bitch!", FALSE, ch, 0, victim, TO_VICT | TO_IGNORABLE);
						break;
					default:
						act("$n tells you to get lost!", FALSE, ch, 0, victim, TO_VICT | TO_IGNORABLE);
						break;
				}			/* end switch */

				act("$n insults $N.", TRUE, ch, 0, victim, TO_NOTVICT | TO_IGNORABLE);
			}
		}
	} else
		send_to_char("I'm sure you don't want to insult *everybody*...\n", ch);
}


void boot_social_messages(void) {
	FILE *fl;
	int nr = 0, hide, min_char_pos, min_pos, min_lvl;
	BUFFER(next_soc, MAX_STRING_LENGTH);
	BUFFER(sorted, MAX_INPUT_LENGTH);

	/* open social file */
	if (!(fl = fopen(SOCMESS_FILE, "r"))) {
		log("SYSERR: Can't open socials file '%s'", SOCMESS_FILE);
		exit(1);
	}
	/* count socials & allocate space */
//	log("Social table contains %d socials.", top_of_socialt);

	/* now read 'em */
	for (;;) {
		fscanf(fl, " %s ", next_soc);
		if (*next_soc == '$') break;
		if (fscanf(fl, " %s %d %d %d %d \n", sorted, &hide, &min_char_pos, &min_pos, &min_lvl) != 5) {
			log("SYSERR: Format error in social file near social '%s'", next_soc);
			exit(1);
		}
		/* read the stuff */
		Social *social = new Social;
		socials.push_back(social);
		
		social->command = next_soc+1;
		social->sort_as = sorted;
		social->hide = hide;
		social->min_char_position = (Position)RANGE(min_char_pos, POS_DEAD, POS_STANDING);
		social->min_victim_position = (Position)RANGE(min_pos, POS_DEAD, POS_STANDING);
		social->min_level_char = min_lvl;

		social->char_no_arg = fread_action(fl, nr);
		social->others_no_arg = fread_action(fl, nr);
		social->char_found = fread_action(fl, nr);

		social->others_found = fread_action(fl, nr);
		social->vict_found = fread_action(fl, nr);
		social->not_found = fread_action(fl, nr);
		social->char_auto = fread_action(fl, nr);
		social->others_auto = fread_action(fl, nr);
		social->char_body_found = fread_action(fl, nr);
		social->others_body_found = fread_action(fl, nr);
		social->vict_body_found = fread_action(fl, nr);
		social->char_obj_found = fread_action(fl, nr);
		social->others_obj_found = fread_action(fl, nr);
	}
	
	log("Social table contains %d socials.", socials.size());

	/* close file & set top */
	fclose(fl);
}


static bool SocialSortFunc(const Social *lhs, const Social *rhs)
{
	return lhs->sort_as < rhs->sort_as;
}

/* this function adds in the loaded socials and assigns them a command # */
void create_command_list(void)  {
	int i, j, k;

	/* free up old command list */
	if (complete_cmd_info) free(complete_cmd_info);
	complete_cmd_info = NULL;

	/* re check the sort on the socials */
/*	for (j = 0; j < socials.size() - 1; j++) {
		k = j;
		for (i = j + 1; i < socials.size(); i++)
//			if (str_cmp(soc_mess_list[i]->sort_as, soc_mess_list[k]->sort_as) < 0)
			if (socials[i]->sort_as < socials[k]->sort_as)
				k = i;
		if (j != k)
		{
			std::swap(socials[j], socials[k]);
		}
	}
*/
	Lexi::Sort(socials, SocialSortFunc);
	
	/* count the commands in the command list */
	i = 0;
	while(*cmd_info[i].command != '\n') i++;
	i++;

	CREATE(complete_cmd_info, struct command_info, socials.size() + i + 1);

	/* this loop sorts the socials and commands together into one big list */
	i = 0;
	j = 0;
	k = 0;
	while ((*cmd_info[i].command != '\n') || (j < socials.size()))  {
		if ((i < NUM_RESERVED_CMDS) || (j >= socials.size()) ||
				(str_cmp(cmd_info[i].sort_as, socials[j]->sort_as.c_str()) < 1)
			)
			complete_cmd_info[k++] = cmd_info[i++];
		else  {
			socials[j]->act_nr						= k;
			complete_cmd_info[k].command			= (char *)socials[j]->command.c_str();
			complete_cmd_info[k].sort_as			= (char *)socials[j]->sort_as.c_str();
			complete_cmd_info[k].minimum_position	= socials[j]->min_char_position;
			complete_cmd_info[k].command_pointer	= do_action;
			complete_cmd_info[k].minimum_level    	= socials[j]->min_level_char;
			complete_cmd_info[k].subcmd				= k;
			
			++j; ++k;
		}
	}
	complete_cmd_info[k].command			= str_dup("\n");
	complete_cmd_info[k].sort_as			= str_dup("zzzzzzz");
	complete_cmd_info[k].minimum_position	= 0;
	complete_cmd_info[k].command_pointer	= 0;
	complete_cmd_info[k].minimum_level		= 0;
	complete_cmd_info[k].subcmd				= 0;
	
	log("Command info rebuilt, %d total commands.", k);
}
