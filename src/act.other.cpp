/* ************************************************************************
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
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
#include "house.h"
#include "affects.h"
#include "files.h"
#include "event.h"
#include "constants.h"
#include "clans.h"

#include <sys/stat.h>

/* extern variables */
extern int nameserver_is_slow;
extern int max_filesize;

/* extern procedures */
SPECIAL(shop_keeper);
void die(CharData * ch, CharData * killer);
ACMD(do_gen_comm);
void perform_immort_vis(CharData *ch);
void quit_otrigger(ObjData *obj, CharData *actor);
void AlterMove(CharData *ch, int amount);
void list_skills(CharData * ch);
void load_otrigger(ObjData *obj);
int install_otrigger(ObjData *obj, CharData *actor, ObjData *installed, float skillroll);
int destroyrepair_otrigger(ObjData *obj, CharData *actor, int repairing);

/* prototypes */
int perform_group(CharData *ch, CharData *vict, bool &predatorFailMessage);
void print_group(CharData *ch);
bool has_polls(int id);
ACMD(do_quit);
ACMD(do_save);
ACMD(do_not_here);
ACMD(do_sneak);
ACMD(do_practice);
ACMD(do_visible);
ACMD(do_title);
ACMD(do_group);
ACMD(do_ungroup);
ACMD(do_report);
ACMD(do_use);
ACMD(do_wimpy);
ACMD(do_display);
ACMD(do_gen_write);
ACMD(do_gen_tog);
ACMD(do_afk);
ACMD(do_watch);
ACMD(do_promote);
ACMD(do_prompt);
ACMD(do_hive);
ACMD(do_dehive);
ACMD(do_install);
ACMD(do_destroy);
ACMD(do_repair);
ACMD(do_pagelength);


ACMD(do_quit)
{
	if (IS_NPC(ch) /* || !ch->desc*/)
		return;

	if (AFF_FLAGGED(ch, AFF_NOQUIT))
		send_to_char("Hell no, you just fought recently!  Wait a few seconds yet.\n", ch);
	else if (GET_POS(ch) == POS_FIGHTING)
		send_to_char("No way!  You're fighting for your life!\n", ch);
//	else if (!NO_STAFF_HASSLE(ch) && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
//		send_to_char("You can't quit in no-man's land.  Get back to a safe place first!\n", ch);
	else {
		if (GET_POS(ch) <= POS_STUNNED) {
			ch->AbortTimer();
			send_to_char("You die before your time...\n", ch);
			ch->die(ch);
		}
		act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
		mudlogf( NRM, ch->GetPlayer()->m_StaffInvisLevel, TRUE,  "%s has quit the game.", ch->GetName());
		send_to_char("Go ahead, return to real life for now... you'll be back soon enough!\n", ch);
		/*
		 * kill off all sockets connected to the same player as the one who is
		 * trying to quit.  Helps to maintain sanity as well as prevent duping.
		 */
	     FOREACH(DescriptorList, descriptor_list, iter)
	    {
    		DescriptorData *d = *iter;

			if (d == ch->desc)
				continue;
			if (d->m_Character && (d->m_Character->GetID() == ch->GetID()))
				d->GetState()->Push(new DisconnectConnectionState);
		}
		
		FOREACH(ObjectList, ch->carrying, obj)
			quit_otrigger(*obj, ch);
		
		for (int i = 0; i < NUM_WEARS; ++i)
			if (GET_EQ(ch, i))
				quit_otrigger(GET_EQ(ch, i), ch);
		
		ch->Save();
		SavePlayerObjectFile(ch, SavePlayerLogout);
		ch->Extract();		/* Char is saved in extract char */

		/* If someone is quitting in their house, let them load back here */
	}
}



ACMD(do_save) {
	if (IS_NPC(ch) || !ch->desc)
		return;
	
	if (command)
		ch->Send("Saving %s.\n", ch->GetName());

	ch->Save();
	SavePlayerObjectFile(ch);
	House::SaveContents(ch->InRoom());
}


/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here) {
	send_to_char("Sorry, but you cannot do that here!\n", ch);
}


//	NOTE: Unused
ACMD(do_sneak) {
//	struct affected_type af;

	if (AFF_FLAGGED(ch, AFF_SNEAK))
	{
		Affect::FromChar(ch, Affect::Sneak);
		
		ch->Send("You stop trying to move silently.\n");
	}
	else
	{
		int duration = (int)((BellCurve() + ch->GetEffectiveSkill(SKILL_STEALTH)) * 3);
		if (duration > 0)
		{
			(new Affect(Affect::Sneak, 0, APPLY_NONE, AFF_SNEAK))->ToChar(ch, duration RL_SEC);
		}
		
		ch->Send("You start trying to move silently.\n");
	}
		
/*
	af.type = SKILL_SNEAK;
	af.duration = ch->GetLevel();
	af.modifier = 0;
	af.location = APPLY_NONE;
	af.bitvector = AFF_SNEAK;
	affect_to_char(ch, &af);
*/
}


ACMD(do_practice) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	one_argument(argument, arg);

	if (*arg)
		send_to_char("You can only practice skills at your recall.\n", ch);
	else
		list_skills(ch);
}


ACMD(do_visible) {
	if (IS_STAFF(ch)) {
		perform_immort_vis(ch);
		return;
	}

	if (AFF_FLAGS(ch).testForAny(Affect::AFF_INVISIBLE_FLAGS))
		ch->Appear(Affect::AFF_INVISIBLE_FLAGS);
	else
		send_to_char("You are already visible.\n", ch);
}



ACMD(do_title) {
	BUFFER(buf, MAX_INPUT_LENGTH);

	skip_spaces(argument);
	strcpy(buf, argument);
	delete_doubledollar(buf);

	if (IS_NPC(ch))
		send_to_char("Your title is fine... go away.\n", ch);
	else if (PLR_FLAGGED(ch, PLR_NOTITLE))
		send_to_char("You can't title yourself -- you shouldn't have abused it!\n", ch);
//	else if (strstr(argument, "(") || strstr(argument, ")"))
//		send_to_char("Titles can't contain the ( or ) characters.\n", ch);
	else if (strlen(argument) > MAX_TITLE_LENGTH) {
		ch->Send("Sorry, titles can't be longer than %d characters.\n", MAX_TITLE_LENGTH);
	} else {
		ch->SetTitle(buf);
		ch->Send("Okay, you're now %s %s.", ch->GetName(), ch->GetTitle());
	}
}


int perform_group(CharData *ch, CharData *vict, bool &predatorFailMessage) {
	if (AFF_FLAGGED(vict, AFF_GROUP) || ch->CanSee(vict) != VISIBLE_FULL)
		return 0;
	if (ch->GetRace() == RACE_PREDATOR && !GET_TEAM(ch))
	{
		if (GET_CLAN(ch) && GET_CLAN(vict)
			&& Clan::GetMember(ch) && Clan::GetMember(vict)
			&& GET_CLAN(ch) != GET_CLAN(vict))
		{
			if (!predatorFailMessage)
			{
				if (GET_CLAN(ch))	ch->Send("You may not join a hunt with Yautja not of your clan!\n");
				else				ch->Send("Only Yautja who are members of a pack may join a hunt!\n");
				predatorFailMessage = true;
			}
			
			return 0;
		}
	}

	AFF_FLAGS(ch).set(AFF_GROUP);
	AFF_FLAGS(vict).set(AFF_GROUP);
	if (ch != vict)
		act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
	act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
	act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
	return 1;
}


void print_group(CharData *ch) {
	CharData *k;

	if (!AFF_FLAGGED(ch, AFF_GROUP))
		send_to_char("But you are not the member of a group!\n", ch);
	else {
		BUFFER(buf, MAX_STRING_LENGTH);
		send_to_char("Your group consists of:\n", ch);

		k = (ch->m_Following ? ch->m_Following : ch);

		if (AFF_FLAGGED(k, AFF_GROUP)) {
			sprintf(buf, "     [%3dH %3dV" /*" %3dE"*/ "] [%2d %s] $N (Head of group)",
					GET_HIT(k), GET_MOVE(k), /*GET_ENDURANCE(k),*/ k->GetLevel(), findtype(k->GetRace(), race_abbrevs));
			act(buf, FALSE, ch, 0, k, TO_CHAR);
		}

		FOREACH(CharacterList, k->m_Followers, iter)
		{
			CharData *follower = *iter;

			if (!AFF_FLAGGED(follower, AFF_GROUP))
				continue;

			sprintf(buf, "     [%3dH %3dV" /*" %3dE"*/"] [%2d %s] $N%s", GET_HIT(follower),
					GET_MOVE(follower), /*GET_ENDURANCE(follower),*/ follower->GetLevel(), findtype(follower->GetRace(), race_abbrevs),
					PRF_FLAGGED(follower, PRF_NOAUTOFOLLOW) ? " (SOLO)" : "");
			act(buf, FALSE, ch, 0, follower, TO_CHAR);
		}
	}
}



ACMD(do_group) {
	CharData *vict;
	int found = 0;
	bool predatorFailMessage = false;
	BUFFER(buf, MAX_INPUT_LENGTH);

	one_argument(argument, buf);

	if (!*buf)
		print_group(ch);
	else if (ch->m_Following)
		act("You can not enroll group members without being head of a group.", FALSE, ch, 0, 0, TO_CHAR);
	else if (!str_cmp(buf, "all")) {
//		perform_group(ch, ch, predatorFailMessage);
		FOREACH(CharacterList, ch->m_Followers, follower)
		{
			found += perform_group(ch, *follower, predatorFailMessage);
		}
		if (!found)
			send_to_char("Everyone following you is already in your group.\n", ch);
	} else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
		send_to_char(NOPERSON, ch);
	else if ((vict->m_Following != ch) && (vict != ch))
		act("$N must follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
	else if (vict == ch)
		ch->Send("You no longer need to group yourself.\n");
	else if (!AFF_FLAGGED(vict, AFF_GROUP))
		perform_group(ch, vict, predatorFailMessage);
	else if (ch == vict) {
		act("You disband the group.", FALSE, ch, 0, 0, TO_CHAR);
		act("$n disbands the group.", FALSE, ch, 0, 0, TO_ROOM);
		ch->die_follower();
		AFF_FLAGS(ch).clear(AFF_GROUP);
	} else {
		act("$N is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
		act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
		act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);
		AFF_FLAGS(vict).clear(AFF_GROUP);
	}
}



ACMD(do_ungroup) {
	CharData *tch;
	BUFFER(buf, MAX_INPUT_LENGTH);

	one_argument(argument, buf);

	if (!*buf) {
		if (ch->m_Following || !(AFF_FLAGGED(ch, AFF_GROUP))) {
			send_to_char("But you lead no group!\n", ch);
		} else {
			sprintf(buf, "%s has disbanded the group.\n", ch->GetName());
			FOREACH(CharacterList, ch->m_Followers, iter)
			{
				CharData *follower = *iter;

				if (AFF_FLAGGED(follower, AFF_GROUP)) {
					AFF_FLAGS(follower).clear(AFF_GROUP);
					send_to_char(buf, follower);
					if (!AFF_FLAGGED(follower, AFF_CHARM))
						follower->stop_follower();
				}
			}

			AFF_FLAGS(ch).clear(AFF_GROUP);
			send_to_char("You disband the group.\n", ch);
		}
	} else if (!(tch = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
		send_to_char("There is no such person!\n", ch);
	else if (tch->m_Following != ch)
		send_to_char("That person is not following you!\n", ch);
	else if (!AFF_FLAGGED(tch, AFF_GROUP))
		send_to_char("That person isn't in your group.\n", ch);
	else {
		AFF_FLAGS(tch).clear(AFF_GROUP);

		act("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
		act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
		act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);

		if (!AFF_FLAGGED(tch, AFF_CHARM))
			tch->stop_follower();
	}
}



ACMD(do_report) {
	CharData *k;
	
	if (!AFF_FLAGGED(ch, AFF_GROUP)) {
		send_to_char("But you are not a member of any group!\n", ch);
		return;
	}
	
	BUFFER(buf, MAX_INPUT_LENGTH);
	
	sprintf(buf, "%s reports: %d/%dH, %d/%dV" /*" %d/%dE"*/ "\n",
			ch->GetName(), GET_HIT(ch), GET_MAX_HIT(ch),
			GET_MOVE(ch), GET_MAX_MOVE(ch)/*),
			GET_ENDURANCE(ch), GET_MAX_ENDURANCE(ch)*/);

	CAP(buf);

	k = (ch->m_Following ? ch->m_Following : ch);
	
	FOREACH(CharacterList, k->m_Followers, iter)
	{
		CharData *follower = *iter;

		if (AFF_FLAGGED(follower, AFF_GROUP) && (follower != ch))
			send_to_char(buf, follower);
	}
	if (k != ch)
		send_to_char(buf, k);
	send_to_char("You report to the group.\n", ch);
}


#if 0
ACMD(do_use) {
	ObjData *mag_item;
	BUFFER(arg, MAX_INPUT_LENGTH);
	BUFFER(buf, MAX_INPUT_LENGTH);
		
	half_chop(argument, arg, buf);
	
	if (!*arg)
	{
		ch->Send("What do you want to %s?\n", CMD_NAME);
		return;
	}
	mag_item = GET_EQ(ch, WEAR_HOLD);

	if (!mag_item || !isname(arg, mag_item->name)) {
		switch (subcmd) {
			case SCMD_USE:
				ch->Send("You don't seem to be holding %s %s.\n", AN(arg), arg);
				return;
			default:
				log("SYSERR: Unknown subcmd %d passed to do_use", subcmd);
				return;
		}
  	}
	
	switch (subcmd) {
		case SCMD_USE:
			send_to_char("You can't seem to figure out how to use it.\n", ch);
			break;
	}
}
#endif


/*
ACMD(do_wimpy) {
	int wimp_lev;
	
	if (IS_NPC(ch))
		return;
	
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	one_argument(argument, arg);

	if (!*arg) {
		if (GET_WIMP_LEV(ch))
			ch->Send("Your current wimp level is %d hit points.\n", GET_WIMP_LEV(ch));
		else
			ch->Send("At the moment, you're not a wimp.  (sure, sure...)\n");
	} else if (isdigit(*arg)) {
		if ((wimp_lev = atoi(arg))) {
			if (wimp_lev < 0)
				send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\n", ch);
			else if (wimp_lev > GET_MAX_HIT(ch))
				send_to_char("That doesn't make much sense, now does it?\n", ch);
			else if (wimp_lev > (GET_MAX_HIT(ch) / 2))
				send_to_char("You can't set your wimp level above half your hit points.\n", ch);
			else {
				ch->Send("Okay, you'll wimp out if you drop below %d hit points.\n", wimp_lev);
				GET_WIMP_LEV(ch) = wimp_lev;
			}
		} else {
			ch->Send("Okay, you'll now tough out fights to the bitter end.\n");
			GET_WIMP_LEV(ch) = 0;
		}
	} else
		ch->Send("Specify at how many hit points you want to wimp out at.  (0 to disable)\n");
}
*/


ACMD(do_display) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	int i, x;

	struct 
	{
		const char *	name;
		const char *	prompt;
	} def_prompts[] = {
//		{ "Minimal"						, "`w%hhp %vmv %een>`n"						},
//		{ "Standard Color"				, "`R%h`rhp `G%v`gmv `B%e`ben`K>`n"			},
//		{ "Percentages"					, "`R%ph`rhp `G%pv`gmv `B%pe`ben`K>`n"		},
//		{ "Full Featured"				,
//				"`cOpponent`K: `B%o `W/ `cTank`K: `B%t%_"
//				"`r%h`K(`R%H`K)`whitp `g%v`K(`G%V`K)`wmove `b%e`K(`B%E`K)`wend`n>"	},
		{ "Minimal"						, "`w%hhp %vmv>`n"						},
		{ "Standard Color"				, "`R%h`rhp `G%v`gmv`K>`n"			},
		{ "Percentages"					, "`R%ph`rhp `G%pv`gmv`K>`n"		},
		{ "Full Featured"				,
				"`cOpponent`K: `B%o `W/ `cTank`K: `B%t%_"
				"`r%h`K(`R%H`K)`whitp `g%v`K(`G%V`K)`wmove`n>"	},
		{ NULL }
	};

	one_argument(argument, arg);

	if (!arg || !*arg) {
		send_to_char("The following pre-set prompts are availible...\n", ch);
		for (i = 0; def_prompts[i].name; i++)
			ch->Send("  %d. %-25s  %s\n", i, def_prompts[i].name, def_prompts[i].prompt);
		send_to_char(	"Usage: display <number>\n"
						"To create your own prompt, use \"prompt <str>\".\n", ch);
	} else if (!isdigit(*arg))
		send_to_char(	"Usage: display <number>\n"
						"Type \"display\" without arguments for a list of preset prompts.\n", ch);
	else {

		i = atoi(arg);
	
	
		if (i < 0) {
			send_to_char("The number cannot be negative.\n", ch);
		} else {
			for (x = 0; def_prompts[x].name; x++) ;

			if (i >= x) {
				ch->Send("The range for the prompt number is 0-%d.\n", x);
			} else {
				ch->GetPlayer()->m_Prompt = def_prompts[i].prompt;
				ch->Send("Set your prompt to the %s preset prompt.\n", ch->GetPlayer()->m_Prompt.c_str());
			}
		}
	}
}


ACMD(do_prompt) {
	skip_spaces(argument);
	
	if (!*argument) {
		ch->Send("Your prompt is currently: `[%s`]\n",
			(!ch->GetPlayer()->m_Prompt.empty() ? ch->GetPlayer()->m_Prompt.c_str() : "n/a"));
	} else {
		delete_doubledollar(argument);
		ch->GetPlayer()->m_Prompt = argument;

		ch->Send("Okay, set your prompt to: %s\n", ch->GetPlayer()->m_Prompt.c_str());
	}
}
  


ACMD(do_gen_write) {
	FILE *fl;
	char *filename;
	struct stat fbuf;

	switch (subcmd) {
		case SCMD_BUG:
			filename = BUG_FILE;
			break;
		case SCMD_TYPO:
			filename = TYPO_FILE;
			break;
		case SCMD_IDEA:
			filename = IDEA_FILE;
			break;
		case SCMD_FEEDBACK:
			filename = FEEDBACK_FILE;
			break;
		default:
			return;
	}

	if (IS_NPC(ch)) {
		send_to_char("Monsters can't have ideas - Go away.\n", ch);
		return;
	}

	skip_spaces(argument);

	if (!*argument) {
		send_to_char("That must be a mistake...\n", ch);
		return;
	}
	
	BUFFER(buf, MAX_INPUT_LENGTH);
	strcpy(buf, argument);
	delete_doubledollar(buf);
	
	mudlogf(NRM, LVL_IMMORT, FALSE,  "%s %s: %s", ch->GetName(), command, buf);

	if (stat(filename, &fbuf) < 0)
		perror("Error statting file");
	else if (fbuf.st_size >= max_filesize)
		send_to_char("Sorry, the file is full right now.. try again later.\n", ch);
	else if (!(fl = fopen(filename, "a"))) {
		perror("do_gen_write");
		send_to_char("Could not open the file.  Sorry.\n", ch);
	} else {
		fprintf(fl, "%-8s (%s) [%10s] %s\n", ch->GetName(), Lexi::CreateDateString(time(0), "%b %e").c_str(),
				ch->InRoom()->GetVirtualID().Print().c_str(), buf);
		fclose(fl);
		send_to_char("Okay.  Thanks!\n", ch);
	}
}



#define TOG_OFF 0
#define TOG_ON  1

#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))
#define CHN_TOG_CHK(ch,flag) ((TOGGLE_BIT(CHN_FLAGS(ch), (flag))) & (flag))

ACMD(do_gen_tog) {
	Flags	result;

	char *tog_messages[][2] = {
		{"You will automatically follow your group leader.\n",
		"You will no longer automatically follow your group leader.\n"},
		{"Nohassle disabled.\n",
		"Nohassle enabled.\n"},
		{"Brief mode off.\n",
		"Brief mode on.\n"},
		{"Compact mode off.\n",
		"Compact mode on.\n"},
		{"You can now hear tells.\n",
		"You are now deaf to tells.\n"},
		{"You can now hear music.\n",
		"You are now deaf to music.\n"},
		{"You can now hear shouts.\n",
		"You are now deaf to shouts.\n"},
		{"You can now hear chat.\n",
		"You are now deaf to chat.\n"},
		{"You can now hear the congratulation messages.\n",
		"You are now deaf to the congratulation messages.\n"},
		{"You can now hear the Wiz-channel.\n",
		"You are now deaf to the Wiz-channel.\n"},
		{"You are no longer part of the Mission.\n",
		"Okay, you are part of the Mission!\n"},
		{"You will no longer see the room flags.\n",
		"You will now see the room flags.\n"},
		{"You will now have your communication repeated.\n",
		"You will no longer have your communication repeated.\n"},
		{"HolyLight mode off.\n",
		"HolyLight mode on.\n"},
		{"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\n",
		"Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\n"},
		{"Autoexits disabled.\n",
		"Autoexits enabled.\n"},
		{"You will now receive game broadcasts.\n",
		"You will no longer receive game broadcasts.\n"},
		{"You will no longer see color.\n",
		"You will now see color.\n"},
		{"You can now hear racial communications.\n",
		"You are now deaf to racial communications.\n"},
		{"You can now hear player's bitching.\n",
		"You are now deaf to player's bitching.\n"},
		{"You can now hear the Newbie channel.\n",
		 "You are now deaf to the Newbie channel.\n"}
	};


	if (IS_NPC(ch))
		return;

	switch (subcmd) {
		case SCMD_NOAUTOFOLLOW:	result = PRF_TOG_CHK(ch, PRF_NOAUTOFOLLOW);	break;
		case SCMD_NOHASSLE:	result = PRF_TOG_CHK(ch, PRF_NOHASSLE);		break;
		case SCMD_BRIEF:	result = PRF_TOG_CHK(ch, PRF_BRIEF);		break;
		case SCMD_COMPACT:	result = PRF_TOG_CHK(ch, PRF_COMPACT);		break;
		case SCMD_NOTELL:	result = CHN_TOG_CHK(ch, Channel::NoTell);	break;
		case SCMD_NOMUSIC:	result = CHN_TOG_CHK(ch, Channel::NoMusic);	break;
		case SCMD_DEAF:		result = CHN_TOG_CHK(ch, Channel::NoShout);	break;
		case SCMD_NOGOSSIP:	result = CHN_TOG_CHK(ch, Channel::NoChat);	break;
		case SCMD_NOGRATZ:	result = CHN_TOG_CHK(ch, Channel::NoGratz);	break;
		case SCMD_NOWIZ:	result = CHN_TOG_CHK(ch, Channel::NoWiz);	break;
		case SCMD_QUEST:	result = CHN_TOG_CHK(ch, Channel::Mission);	break;
		case SCMD_ROOMFLAGS:result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);	break;
		case SCMD_NOREPEAT:	result = PRF_TOG_CHK(ch, PRF_NOREPEAT);		break;
		case SCMD_HOLYLIGHT:result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);	break;
		case SCMD_SLOWNS:	result = (nameserver_is_slow = !nameserver_is_slow);	break;
		case SCMD_AUTOEXIT:	result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);		break;
		case SCMD_NOINFO:	result = CHN_TOG_CHK(ch, Channel::NoInfo);	break;
		case SCMD_COLOR:	result = PRF_TOG_CHK(ch, PRF_COLOR);		break;
		case SCMD_NORACE:	result = CHN_TOG_CHK(ch, Channel::NoRace);	break;
		case SCMD_NOBITCH:	result = CHN_TOG_CHK(ch, Channel::NoBitch);	break;
		case SCMD_NONEWBIE:	result = CHN_TOG_CHK(ch, Channel::NoNewbie);	break;
		default:	log("SYSERR: Unknown subcmd %d in do_gen_toggle", subcmd);	return;
	}

	if (result)		send_to_char(tog_messages[subcmd][TOG_ON], ch);
	else			send_to_char(tog_messages[subcmd][TOG_OFF], ch);

	return;
}


ACMD(do_afk) {
	if (IS_NPC(ch))
		return;

	skip_spaces(argument);

	if (ch->GetPlayer()->m_AFKMessage.empty()) {
		ch->GetPlayer()->m_AFKMessage = *argument ? argument : "I am AFK!";
		act("$n has just gone AFK: $t\n", TRUE, ch, (ObjData *)ch->GetPlayer()->m_AFKMessage.c_str(), 0, TO_ROOM);
	} else {
		ch->GetPlayer()->m_AFKMessage = "";
		act("$n is back from AFK!\n", TRUE, ch, 0, 0, TO_ROOM);
	}

	return;
}


ACMD(do_watch) {
	int look_type /*, skill_level*/;
	
//	if (!(skill_level = IS_NPC(ch) ? ch->GetLevel() * 3 : GET_SKILL(ch, SKILL_WATCH))) {
//		send_to_char("Huh?!?\n", ch);
//		return;
//	}
	
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	one_argument(argument, arg);
	
	if (!*arg) {
		CHAR_WATCHING(ch) = -1;
		ch->Send("You stop watching.\n");
	} else if ((look_type = search_block(arg, dirs, FALSE)) >= 0) {
		CHAR_WATCHING(ch) = look_type;
		ch->Send("You begin watching %s you.\n", dir_text_to_the_of[look_type]);
	} else
		send_to_char("That's not a valid direction...\n", ch);
}


#define LEVEL_COST(ch)  (ch->GetLevel() * ch->GetLevel())
//#define LEVEL_COST(ch)  (ch->GetLevel() * ch->GetLevel() * ch->GetLevel() * .75)
//#define LEVEL_COST(ch)  (ch->GetLevel() * ch->GetLevel() * ch->GetLevel() * .5)
//#define PK_LEVEL_COST(ch)  ((ch->GetLevel() * ch->GetLevel() * 3.125) - 1200)


ACMD(do_promote) {
	extern int level_cost(int level);
	
	int cost = level_cost(ch->GetLevel()), myArg;
	BUFFER(arg, MAX_INPUT_LENGTH);
	one_argument(argument, arg);

	if (IS_STAFF(ch))
		ch->Send("Very funny.\n");
	else if (ch->GetLevel() == LVL_IMMORT - 1)
		ch->Send("You are already at the maximum level possible.\n");
	else if (!*arg || ((myArg = atoi(arg)) != ch->GetLevel() + 1)) {
		ch->Send("You need %d MP to advance to level %d.\n"
				 "When you have enough mission points, type \"promote %d\".\n", cost,
				ch->GetLevel() + 1, ch->GetLevel() + 1);
	} else if (GET_MISSION_POINTS(ch) < cost)
		ch->Send("You don't have enough mission points to level.\n");
	else {
		GET_MISSION_POINTS(ch) -= cost;
		ch->SetLevel(myArg);
	}
}



class Poll {
public:
					Poll(void);
					~Poll(void);
					
	Lexi::String	question;
	time_t			started;
	bool			multichoice;
	bool			deleted;
	
	typedef std::list<IDNum>	IDNumList;
	
	class Answer {
	public:
		Lexi::String	text;
		IDNumList		who;
	};
	
	typedef std::list<Answer>	AnswerList;
	
	AnswerList		answers;
	IDNumList		who;
};



Poll::Poll(void) : started(time(0)), multichoice(false), deleted(false) { }
Poll::~Poll(void) {}


typedef std::list<Poll>	PollList;
PollList polls;


ACMD(do_vote);
ACMD(do_poll);
void LoadPolls(void);
void SavePolls(void);


ACMD(do_vote) {
	BUFFER(buf, MAX_STRING_LENGTH);
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	
	if (IS_NPC(ch))
		return;
	
	two_arguments(argument, arg1, arg2);
	
	if (!*arg1) {
		//	List polls
		int			i = 0, voted = 0;
		
		ch->Send("Polls Available: \n");
		
		FOREACH(PollList, polls, poll)
		{
			ch->Send("    %2d - %s\n"
					 "         Started on %s.%s\n", ++i, poll->question.c_str(),
					Lexi::CreateDateString(poll->started, "%A, %b %d").c_str(),
					poll->multichoice ? "   Multiple Choice." : "");
			
			*buf = '\0';
			
			if (Lexi::IsInContainer(poll->who, GET_ID(ch)))
			{
				int count = 0;
				
				++voted;

				FOREACH(Poll::AnswerList, poll->answers, answer)
				{
					if (!Lexi::IsInContainer(answer->who, GET_ID(ch)))
						continue;
					
					++count;
					
					sprintf_cat(buf,
						"%s%s", *buf ? "\n" : "             ",
						answer->text.c_str());
				}
				
				ch->Send(count > 1 ?
						"         Your response: \n             %s\n" :
						"         Your response: %s\n",
						buf);
			}
			else
				ch->Send("         You have not voted.\n");
		}
		
		if (voted == polls.size())		ch->Send("\nThere are no polls left to vote on.\n");
		else							ch->Send("\nThere are %d polls left to vote on.\n", polls.size() - voted);
		
		ch->Send("\nTo see a poll, type: vote <poll #>\n");
	}
	else if (!*arg2)
	{
		int		pollNum = atoi(arg1);
		
		if (pollNum < 1 || pollNum > polls.size())
			ch->Send("That is not a valid poll.  Type 'vote' to see all the polls.\n");
		else
		{
			PollList::iterator poll = polls.begin();
			std::advance(poll, pollNum - 1);
	
			int				count = 0;
			int				totalvotes = 0;
			
			FOREACH(Poll::AnswerList, poll->answers, answer)
			{
				totalvotes += answer->who.size();
			}

			ch->Send("%2d - %s\n", pollNum, poll->question.c_str());
			ch->Send("    (Created %s)\n", Lexi::CreateDateString(poll->started, "%A, %b %d").c_str());
			
			FOREACH(Poll::AnswerList, poll->answers, answer)
			{
				ch->Send("%-4s%2d. %-40s (%d votes - %.1f%%)\n",	
						Lexi::IsInContainer(answer->who, GET_ID(ch)) ? "->" : "",
						++count, answer->text.c_str(), answer->who.size(),
						answer->who.size() > 0 ? answer->who.size() * 100.0 / totalvotes : 0);
			}
			
			ch->Send("\n%d total voters to date.\n\n", poll->who.size());
			
			if (!Lexi::IsInContainer(poll->who, GET_ID(ch)))
				ch->Send("You have not yet voted.\n\n");

			ch->Send("To vote in a poll, type:             vote <poll #> <answer #>\n"
					 "This will mark your answer.  If the poll is multiple choice, and this will\n"
					 "toggle that answer for you.  You may change your vote any time while the\n"
					 "poll is available.\n");
		}
	}
	else
	{
		int		pollNum = atoi(arg1);
		int		answerNum = atoi(arg2);
		int		counter = 0;
		
//		if (ch->GetLevel() < 10)
//			ch->Send("Sorry, you must be level 10 or higher to vote.\n");
//		else
		if (pollNum < 1 || pollNum > polls.size())
			ch->Send("That is not a valid poll.  Type 'vote' to see all the polls.\n");	
		else
		{
			PollList::iterator poll = polls.begin();
			std::advance(poll, pollNum - 1);

			if (answerNum < 1 || answerNum > poll->answers.size())
				ch->Send("That is not a valid answer.  Type 'vote %d' to see the valid answers.\n", poll->answers.size());
			else
			{
				char *			which = "";
				bool			added = true;
				
				Poll::AnswerList::iterator answer = poll->answers.begin();
				std::advance(answer, answerNum - 1);
				
				if (Lexi::IsInContainer(poll->who, GET_ID(ch))
						&& !Lexi::IsInContainer(answer->who, GET_ID(ch))
						&& !poll->multichoice)
					ch->Send("You can only vote once in this poll.\n");
				else
				{
					if (Lexi::IsInContainer(answer->who, GET_ID(ch)))
					{
						answer->who.remove(GET_ID(ch));
						added = false;
						if (!poll->multichoice)
							poll->who.remove(GET_ID(ch));
					}
					else
						answer->who.push_back(GET_ID(ch));
					
					ch->Send("%2d - %s\n", pollNum, poll->question.c_str());
					ch->Send(added ? "You have voted for: %s\n" :
							"You have removed your vote for: %s\n", answer->text.c_str());

					if (added && !Lexi::IsInContainer(poll->who, GET_ID(ch)))
						poll->who.push_back(GET_ID(ch));
					
					SavePolls();
				}
			}
		}
	}
}


void LoadPolls(void) {
	FILE *fp = NULL;
	
	if (!(fp = fopen(ETC_PREFIX "polls", "r"))) {
		log("SYSERR: Error opening polls");
		return;
	}
	
	BUFFER(buf, MAX_STRING_LENGTH);
	
	while (get_line(fp, buf)) {
		int	d[4];
		
		if (*buf == '$')
			break;
		
		if (str_cmp(buf, "POLL")) {
			log("SYSERR: Unknown line \"%s\" in poll file.", buf);
			continue;
		}
		
		polls.push_back(Poll());
		Poll &poll = polls.back();
		
		poll.question = fread_lexistring(fp, "Reading poll question", ETC_PREFIX "polls");
		
		if (!get_line(fp, buf) || sscanf(buf, "%d %d %d", d + 0, d + 1, d + 2) != 3) {
			log("SYSERR: Format error in created/multichoice line of question %d of polls.", polls.size() + 1);
			break;
		}
		if (d[0] > 0)	poll.started = d[0];
		poll.multichoice = d[1];
		
		int answerCount = d[2];
		
		for (int x = 0; x < answerCount; x++) {
			poll.answers.push_back(Poll::Answer());
			Poll::Answer &answer = poll.answers.back();
			
			answer.text = fread_lexistring(fp, "Reading poll answer", ETC_PREFIX "polls");
			while (get_line(fp, buf)) {
				if (*buf == '~')	break;
				
				int who = atoi(buf);
				
				answer.who.push_back(who);
				if (!Lexi::IsInContainer(poll.who, who))
					poll.who.push_back(who);
			}
		}
		
		while (get_line(fp, buf))
			if (*buf == '~')
				break;
	}
}


void SavePolls(void) {
	FILE *fp = NULL;
	
	if (!(fp = fopen(ETC_PREFIX "polls.tmp", "w"))) {
		mudlogf(BRF, 0, true, "SYSERR: Error saving polls");
		return;
	}
	
	FOREACH(PollList, polls, poll)
	{
		fprintf(fp, "POLL\n");
		fprintf(fp, "%s~\n", poll->question.c_str());
		fprintf(fp, "%lu %d %d\n", poll->started, poll->multichoice, poll->answers.size());
		
		FOREACH(Poll::AnswerList, poll->answers, answer)
		{
			fprintf(fp, "%s~\n", answer->text.c_str());
			
			FOREACH(Poll::IDNumList, answer->who, who)
				fprintf(fp, "%d\n", *who);

			fprintf(fp, "~\n");
		}

//		START_ITER(iter3, who, int, poll->who) {
//			fprintf(fp, "%d\n", who);
//		}
		
		fprintf(fp, "~\n");
	}
	
	fprintf(fp, "$\n");
	
	fclose(fp);
	
	remove(ETC_PREFIX "polls");
	rename(ETC_PREFIX "polls.tmp", ETC_PREFIX "polls");
}


bool has_polls(int id) {
	FOREACH(PollList, polls, poll)
		if (!Lexi::IsInContainer(poll->who, id))
			return true;
	
	return false;
}


ACMD(do_poll) {
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);

	argument = one_argument(argument, arg1);
	
	if (!*arg1)
		ch->Send("Use: poll [create|delete|multi|reset|answer] ...\n");
	else if (is_abbrev(arg1, "create")) {
		if (!*argument)
			ch->Send("Use: poll create <question>\n");
		else {
			Poll	poll;
			
			poll.question = argument;
			polls.push_back(poll);
			
			ch->Send("New poll (%d) \"%s\" added.\n", polls.size(), poll.question.c_str());
			SavePolls();
		}
	}
	else if (is_abbrev(arg1, "answer"))
	{
		argument = two_arguments(argument, arg1, arg2);	

		if (!*arg1 || !*arg2)
		{
			ch->Send("Use: poll answer [add|delete] <poll #>...\n");
			return;
		}
		
		int pollNum = atoi(arg2);
		
		if (pollNum < 1 || pollNum > polls.size())
		{
			ch->Send("That is not a valid poll.\n");
			return;
		}

		PollList::iterator poll = polls.begin();
		std::advance(poll, pollNum - 1);

		if (is_abbrev(arg1, "add"))
		{
			if (!*argument)
				ch->Send("Use: poll answer add <poll #> <answer>\n");
			else
			{
				Poll::Answer answer;
				answer.text = argument;
				
				poll->answers.push_back(answer);
				ch->Send("Answer (%d) \"%s\" added to poll %d.\n", poll->answers.size(), answer.text.c_str(), pollNum);
				SavePolls();
			}
		}
		else if (is_abbrev(arg1, "delete"))
		{
			int answerNum = atoi(argument);
			
			if (answerNum < 1 || answerNum > poll->answers.size())
				ch->Send("That is not a valid answer.\n");
			else
			{
				Poll::AnswerList::iterator answer = poll->answers.begin();
				std::advance(answer, answerNum - 1);
				
				poll->answers.erase(answer);
				ch->Send("Answer (%d) deleted from poll %d.\n", answerNum, pollNum);
				SavePolls();
			}
		}
		else
			ch->Send("Use: poll answer [add|delete] <poll #> ...\n");
	}
	else
	{
		int		which = atoi(argument);
		
		if (which < 1 || which > polls.size())
		{
			ch->Send("That is not a valid poll.\n");
			return;
		}

		PollList::iterator poll;
		int tmp = which;
		for (poll = polls.begin(); poll != polls.end(); ++poll)
		{
			if (--tmp == 0)
				break;
		}
		
		if (poll == polls.end())
			ch->Send("Error - poll not found.\n");
		else if (is_abbrev(arg1, "delete"))
		{
			if (!poll->deleted)
			{
				ch->Send("Repeat the command to confirm.\n");
				poll->deleted = true;
			}
			else
			{
				polls.erase(poll);
				SavePolls();
				ch->Send("Poll %d deleted.\n", atoi(argument));
			}
		}
		else if (is_abbrev(arg1, "multi"))
		{
			poll->multichoice = !poll->multichoice;
			ch->Send("Poll %d is %s a multiple choice poll.\n", which, poll->multichoice ? "now" : "no longer");
			SavePolls();
		}
		else if (is_abbrev(arg1, "reset") && ch->GetLevel() == LVL_CODER)
		{
			poll->who.clear();

			FOREACH(Poll::AnswerList, poll->answers, answer)
				answer->who.clear();
			
			ch->Send("Poll reset.\n");
		}
		else
			ch->Send("Use: poll [create|delete|set|reset|answer] ...\n");
	}
}


//	Deployables values are: vnum, time
//	NOTE: Deprecated
ACMD(do_install)
{
	ObjData *obj;
	ObjectPrototype *proto;
	
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	switch (ch->m_Substate) {
		default:
			one_argument(argument, arg);
			
			obj = get_obj_in_list_vis(ch, arg, ch->carrying /*ch->InRoom()->contents*/);
			
			if (!obj)
				ch->Send("Install what?\n");
			else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
				ch->Send("You can't install stuff in peaceful rooms.\n");
			else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_GRAVITY))
				ch->Send("You can't install stuff in mid-air!\n");
			else if (ch->InRoom()->GetSector() >= SECT_WATER_SWIM)
				ch->Send("You can't install stuff here.\n");
			else if (GET_OBJ_TYPE(obj) != ITEM_INSTALLABLE)
				ch->Send("You can't install %s.\n", obj->GetName());
			else if (ch->CanUse(obj) != NotRestricted)
				ch->Send("You have no clue what to do with %s`n!\n", obj->GetName());
			else if ((proto = obj_index.Find(obj->GetVIDValue(OBJVIDVAL_INSTALLABLE_INSTALLED))) == NULL)
				ch->Send("Internal error; unable to install %s!\n", obj->GetName());
			else {
				IDNum *idnumptr;
				
				ch->Send("You begin installing %s`n.\n", obj->GetName());
				act("$n begins installing $p`n.", FALSE, ch, obj, 0, TO_ROOM);
				
				CREATE(idnumptr, IDNum, 1);
				*idnumptr = GET_ID(obj);
				ch->AddTimer(NO_STAFF_HASSLE(ch) ? 1 : (obj->GetValue(OBJVAL_INSTALLABLE_TIME) RL_SEC), do_install, 1,
						idnumptr);
			}
			return;
		
		case 1:
			ch->m_Substate = SUB_NONE;
			break;
		
		case SUB_TIMER_DO_ABORT:
			obj = ObjData::Find(*(IDNum *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer());
			
			if (obj) {
				act("You are interrupted and stop installing $p`n.", FALSE, ch, obj, 0, TO_CHAR);
				act("$n is interrupted and stops installing $p`n.", FALSE, ch, obj, 0, TO_ROOM);
			}
			else
			{
				ch->Send("You are interrupted, and notice that what you were installing has disappeared!\n");
				act("$n is interrupted from installing some non-existant thing.", FALSE, ch, obj, 0, TO_ROOM);
			}
			ch->m_Substate = SUB_NONE;
			return;
	}
	
	bool success = true;
	bool critical = false;
	float skillroll = 0;
	
	obj = ObjData::Find(*(IDNum *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer());
	if (obj)
	{
		if (GET_OBJ_TYPE(obj) == ITEM_INSTALLABLE && obj->GetValue(OBJVAL_INSTALLABLE_SKILL) != 0)
		{
			skillroll = SkillRoll(ch->GetEffectiveSkill(obj->GetValue(OBJVAL_INSTALLABLE_SKILL)));
			
			success = (skillroll >= 0);
			critical = (skillroll <= -15);
		}
		
		if (success)
		{
			act("`gYou finish installing $p`n.", FALSE, ch, obj, 0, TO_CHAR);
			act("$n has finished installing $p`n.", FALSE, ch, obj, 0, TO_ROOM);
		}
		else
		{
			act("`rYou fail to install $p`n.", FALSE, ch, obj, 0, TO_CHAR);
			act("`r$n`n`r fails to install $p`n.", FALSE, ch, obj, 0, TO_ROOM);
			if (critical)
				act("`R$p`n`R is ruined`n.", FALSE, ch, obj, 0, TO_CHAR | TO_ROOM);
		}
	}
	else
	{
		ch->Send("You are interrupted, and notice that what you were installing has disappeared!\n");
		act("$n is interrupted from installing some non-existant thing.", FALSE, ch, obj, 0, TO_ROOM);
	}
	
	FOREACH(CharacterList, ch->InRoom()->people, iter)
	{
		CharData *i = *iter;
		if (i == ch)	continue;
		if (i->m_pTimerCommand && i->m_pTimerCommand->GetType() == ActionTimerCommand::Type &&
				((ActionTimerCommand *)i->m_pTimerCommand)->m_pFunction == do_install &&
				((ActionTimerCommand *)i->m_pTimerCommand)->GetBuffer() &&
				(*(IDNum *)((ActionTimerCommand *)i->m_pTimerCommand)->GetBuffer() == *(IDNum *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer())) {
			//	See if what they are installing is same thing...
			if (obj)
			{
				act("You finish helping to install $p`n.", FALSE, i, obj, 0, TO_CHAR);
			}
			else
			{
				i->Send("You are interrupted, and notice that what you were installing has disappeared!\n");
			}
			i->ExtractTimer();
		}
	}
	
	if (obj)
	{
		if (success)
		{
			ObjData *newobj;
			
			VirtualID vid = obj->GetVIDValue(OBJVIDVAL_INSTALLABLE_INSTALLED);
			
			newobj = ObjData::Create(vid);
			if (newobj)
			{
				newobj->ToRoom(IN_ROOM(ch));
				
				load_otrigger(newobj);
			
				switch (GET_OBJ_TYPE(newobj))
				{
					case ITEM_INSTALLED:
						newobj->SetValue(OBJVAL_INSTALLED_DISABLED, 0);
						break;
					
					default:
						break;
				}
				
				if (!PURGED(newobj))
					install_otrigger(obj, ch, newobj, skillroll);
			}
			else
				ch->Send("Error - unable to create installed object (%s) missing from database.\n", vid.Print().c_str());
		}
		else
			install_otrigger(obj, ch, NULL, skillroll);
		if (success || critical && !PURGED(obj))
			obj->Extract();
	}
}


ACMD(do_destroy) {
	ObjData *obj;
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	switch (ch->m_Substate) {
		default:
			one_argument(argument, arg);
			
			obj = get_obj_in_list_vis(ch, arg, ch->InRoom()->contents);
			
			if (!obj)
				ch->Send("Destroy what?\n");
			else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
				ch->Send("But its so peaceful here...\n");
			else if (destroyrepair_otrigger(obj, ch, 0) || PURGED(obj) || PURGED(ch))
				;	//	Dont do anything
			else if (GET_OBJ_TYPE(obj) != ITEM_INSTALLED || obj->GetValue(OBJVAL_INSTALLED_TIME) == 0)
				ch->Send("You can't destroy %s.\n", obj->GetName());
			else if (obj->GetValue(OBJVAL_INSTALLED_PERMANENT) == 1 && obj->GetValue(OBJVAL_INSTALLED_DISABLED) == 1)
				act("$p is already destroyed.", FALSE, ch, obj, NULL, TO_CHAR);
			else
			{
				FOREACH(CharacterList, ch->InRoom()->people, iter)
				{
					CharData *i = *iter;
					if (i == ch)	continue;
					if (i->m_pTimerCommand && i->m_pTimerCommand->GetType() == ActionTimerCommand::Type &&
							((ActionTimerCommand *)i->m_pTimerCommand)->m_pFunction == do_destroy &&
							((ActionTimerCommand *)i->m_pTimerCommand)->GetBuffer() &&
							(*(IDNum *)((ActionTimerCommand *)i->m_pTimerCommand)->GetBuffer() == GET_ID(obj))) {
						//	See if what they are destroying is same thing...
						act("$N is already destroying $p.", FALSE, ch, obj, i, TO_CHAR);
						return;
					}
				}

				IDNum *idnumptr;
				ch->Send("You begin destroying %s`n.\n", obj->GetName());
				act("$n begins destroying $p`n.", FALSE, ch, obj, 0, TO_ROOM);
				
				CREATE(idnumptr, IDNum, 1);
				*idnumptr = GET_ID(obj);
				ch->AddTimer(NO_STAFF_HASSLE(ch) ? 1 : obj->GetValue(OBJVAL_INSTALLED_TIME) RL_SEC, do_destroy, 1, idnumptr);
			}
			return;
		
		case 1:
			ch->m_Substate = SUB_NONE;
			break;
		
		case SUB_TIMER_DO_ABORT:
			obj = ObjData::Find(*(IDNum *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer());
			
			if (obj) {
				act("You are interrupted and stop destroying $p`n.", FALSE, ch, obj, 0, TO_CHAR);
				act("$n is interrupted and stops destroying $p`n.", FALSE, ch, obj, 0, TO_ROOM);
			}
			else
			{
				ch->Send("You are interrupted, and notice that what you were destroying is already gone!\n");
				act("$n is interrupted from destroying some non-existant thing.", FALSE, ch, obj, 0, TO_ROOM);
			}
			ch->m_Substate = SUB_NONE;
			return;
	}
	
	obj = ObjData::Find(*(IDNum *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer());
	if (obj)
	{
		if (IN_ROOM(ch) != IN_ROOM(obj))	return;
		
		act("You finish destroying $p`n.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n has finished destroying $p`n.", FALSE, ch, obj, 0, TO_ROOM);
	}
	else
	{
		ch->Send("You are interrupted, and notice that what you were destroying has disappeared!\n");
		act("$n is interrupted from destroying some non-existant thing.", FALSE, ch, obj, 0, TO_ROOM);
	}

/*
	START_ITER(iter, i, CharData *, ch->InRoom()->people) {
		if (i == ch)	continue;
		if (i->timer_event && i->timer_func == do_destroy && i->dest_buf &&
				(*(IDNum *)i->dest_buf == *(IDNum *)ch->dest_buf)) {
			//	See if what they are destroying is same thing...
			if (obj)
			{
				act("You finish helping to destroy $p`n.", FALSE, i, obj, 0, TO_CHAR);
			}
			else
			{
				i->Send("You are interrupted, and notice that what you were destroying has disappeared!\n");
			}
			i->ExtractTimer();
		}
	}
*/

	if (obj)
	{
		switch (GET_OBJ_TYPE(obj))
		{
			case ITEM_INSTALLED:
				//	Values: race, permanent, disabled
				if (obj->GetValue(OBJVAL_INSTALLED_PERMANENT))
				{
					act("$p ceases to operate.", FALSE, 0, obj, 0, TO_ROOM);
					obj->SetValue(OBJVAL_INSTALLED_DISABLED, 1);
				}
				else
				{
					act("$p explodes!", FALSE, 0, obj, 0, TO_ROOM);
					obj->Extract();
				}
				break;
			default:	break;
		}
	}
}



ACMD(do_repair) {
	ObjData *obj;
	
	BUFFER(arg, MAX_INPUT_LENGTH);
	
	switch (ch->m_Substate) {
		default:
			one_argument(argument, arg);
			
			obj = get_obj_in_list_vis(ch, arg, ch->InRoom()->contents);
			
			if (!obj)
				ch->Send("Repair what?\n");
			else if (destroyrepair_otrigger(obj, ch, 1) || PURGED(obj) || PURGED(ch))
				;	//	Dont do anything
			else if (GET_OBJ_TYPE(obj) != ITEM_INSTALLED)
				ch->Send("You can't repair %s.\n", obj->GetName());
			else if (obj->GetValue(OBJVAL_INSTALLED_PERMANENT) == 1 && obj->GetValue(OBJVAL_INSTALLED_DISABLED) == 0)
				act("$p isn't damaged.", FALSE, ch, obj, 0, TO_CHAR);
			else {
				FOREACH(CharacterList, ch->InRoom()->people, iter)
				{
					CharData *i = *iter;
					if (i == ch)	continue;
					if (i->m_pTimerCommand && i->m_pTimerCommand->GetType() == ActionTimerCommand::Type &&
							((ActionTimerCommand *)i->m_pTimerCommand)->m_pFunction == do_repair &&
							((ActionTimerCommand *)i->m_pTimerCommand)->GetBuffer() &&
							(*(IDNum *)((ActionTimerCommand *)i->m_pTimerCommand)->GetBuffer() == GET_ID(obj))) {
						act("$N is already repairing $p.", FALSE, ch, obj, i, TO_CHAR);
						return;
					}
				}
				
				ch->Send("You begin repairing %s`n.\n", obj->GetName());
				act("$n begins repairing $p`n.", FALSE, ch, obj, 0, TO_ROOM);
				
				IDNum *idnumptr;
				CREATE(idnumptr, IDNum, 1);
				*idnumptr = GET_ID(obj);
				ch->AddTimer(NO_STAFF_HASSLE(ch) ? 1 : 20 RL_SEC, do_repair, 1,
						idnumptr);
			}
			return;
		
		case 1:
			ch->m_Substate = SUB_NONE;
			break;
		
		case SUB_TIMER_DO_ABORT:
			obj = ObjData::Find(*(IDNum *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer());
			
			if (obj) {
				act("You are interrupted and stop repairing $p`n.", FALSE, ch, obj, 0, TO_CHAR);
				act("$n is interrupted and stops repairing $p`n.", FALSE, ch, obj, 0, TO_ROOM);
			}
			else
			{
				ch->Send("You are interrupted, and notice that what you were repairing is already gone!\n");
				act("$n is interrupted from repairing some non-existant thing.", FALSE, ch, obj, 0, TO_ROOM);
			}
			ch->m_Substate = SUB_NONE;
			return;
	}
	
	obj = ObjData::Find(*(IDNum *)((ActionTimerCommand *)ch->m_pTimerCommand)->GetBuffer());
	if (obj)
	{
		if (IN_ROOM(ch) != IN_ROOM(obj))	return;
		
		act("You finish repairing $p`n.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n has finished repairing $p`n.", FALSE, ch, obj, 0, TO_ROOM);

		switch (GET_OBJ_TYPE(obj))
		{
			case ITEM_INSTALLED:
				//	Values: race, permanent, disabled
				if (obj->GetValue(OBJVAL_INSTALLED_PERMANENT))
				{
					act("$p resumes normal operations.", FALSE, 0, obj, 0, TO_ROOM);
					obj->SetValue(OBJVAL_INSTALLED_DISABLED, 0);
				}
				else
				{
					//	Heh, this shouldn't happen, so lets have some fun.
					act("$p explodes!", FALSE, 0, obj, 0, TO_ROOM);
					obj->Extract();
				}
				break;
			default:	break;
		}
	}
	else
	{
		ch->Send("You are interrupted, and notice that what you were repairing has disappeared!\n");
		act("$n is interrupted from repairing some non-existant thing.", FALSE, ch, obj, 0, TO_ROOM);
	}
}



ACMD(do_pagelength)
{
	skip_spaces(argument);
	
	int length = atoi(argument);
	
	if (IS_NPC(ch) || !ch->desc)
		ch->Send("Players only!\n");
	else if (!*argument)
		ch->Send("Your pagelength is %d.\nSetting your pagelength to 0 will turn off paging.\n", ch->GetPlayer()->m_PageLength);
	else if (!is_number(argument) || ((length != 0) && (length < 10)))
		ch->Send("Page length must be > 10, or 0.\n");
	else
	{
		ch->GetPlayer()->m_PageLength = length;
		ch->Send("Your pagelength is now %d.\n", ch->GetPlayer()->m_PageLength);
	}
}
