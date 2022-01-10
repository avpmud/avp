/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "characters.h"
#include "rooms.h"
#include "objects.h"
#include "descriptors.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "find.h"
#include "db.h"
#include "boards.h"
#include "clans.h"
#include "staffcmds.h"
#include "zones.h"

void speech_mtrigger(CharData *actor, const char *str);
void speech_otrigger(CharData *actor, const char *str);
void speech_wtrigger(CharData *actor, const char *str);


/* extern variables */
extern int level_can_shout;

/* local functions */
void perform_tell(CharData *ch, CharData *vict, char *arg);
bool is_tell_ok(CharData *ch, CharData *vict);

ACMD(do_say);
ACMD(do_gsay);
ACMD(do_tell);
ACMD(do_reply);
ACMD(do_retell);
ACMD(do_spec_comm);
ACMD(do_write);
ACMD(do_page);
ACMD(do_gen_comm);
ACMD(do_qcomm);
ACMD(do_history);
ACMD(do_sayhistory);
ACMD(do_ignore);
ACMD(do_infohistory);



ACMD(do_say) {
//	char *buf;
	char *verb, *s;

	skip_spaces(argument);

	if (!*argument)
		ch->Send("Yes, but WHAT do you want to say?\n");
	else if (GET_POS(ch) == POS_SLEEPING)
		ch->Send("In your dreams, or what?\n");
	else
	{
		BUFFER(arg, MAX_STRING_LENGTH);
		
		strcpy(arg, argument);
		
		s = arg + strlen(arg);
		while (isspace(*(--s))) ;
		*(s + 1) = '\0';
		
		switch (*s) {
			case '?':	verb = "ask";		break;
			case '!':	verb = "exclaim";	break;
			default:	verb = "say";		break;
		}
		delete_doubledollar(arg);

		if (PRF_FLAGGED(ch, PRF_NOREPEAT))	ch->Send(OK);
		else
		{
			act("You $t, '$T'", FALSE, ch, (ObjData *)verb, arg, TO_CHAR | TO_NOTTRIG | TO_SAYBUF);
		}
		act("$n $ts, '$T'", FALSE, ch, (ObjData *)verb, arg, TO_ROOM | TO_NOTTRIG | TO_SAYBUF | TO_IGNORABLE);

		Lexi::String argStripped = strip_color(arg);
		const char *argConst = argStripped.c_str();

		speech_mtrigger(ch, argConst);
		speech_otrigger(ch, argConst);
		speech_wtrigger(ch, argConst);
	}
}


ACMD(do_gsay) {
	CharData *k;

	delete_doubledollar(argument);
	skip_spaces(argument);

	if (!AFF_FLAGGED(ch, AFF_GROUP))
		ch->Send("But you are not the member of a group!\n");
	else if (!*argument)
		ch->Send("Yes, but WHAT do you want to group-say?\n");
	else {
		k = ch->m_Following ? ch->m_Following : ch;

		if (AFF_FLAGGED(k, AFF_GROUP) && (k != ch))
			act("$n tells the group, '$t'", FALSE, ch, (ObjData *)argument, k, TO_VICT | TO_SLEEP | TO_SAYBUF);
		
		FOREACH(CharacterList, k->m_Followers, iter)
		{
			CharData *follower = *iter;
			if (AFF_FLAGGED(follower, AFF_GROUP) && (follower != ch))
				act("$n tells the group, '$t'", FALSE, ch, (ObjData *)argument, follower, TO_VICT | TO_SLEEP | TO_SAYBUF);
		}
		
		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			ch->Send(OK);
		else
			act("You tell the group, '$t'", FALSE, ch, (ObjData *)argument, 0, TO_CHAR | TO_SLEEP | TO_SAYBUF);
	}
}


void perform_tell(CharData *ch, CharData *vict, char *arg) {
	BUFFER(buf, MAX_STRING_LENGTH);
	
	delete_doubledollar(arg);
	act("$n `rtells you, '$t`r'`n", FALSE, ch, (ObjData *)arg, vict, TO_VICT | TO_SLEEP | TO_IGNORABLE | TO_IGNOREINVIS, buf);
	vict->AddTellBuf(buf);

	if (PRF_FLAGGED(ch, PRF_NOREPEAT))
		ch->Send(OK);
	else {
		act("`rYou tell $N`r, '$t`r'`n", FALSE, ch, (ObjData *)arg, vict, TO_CHAR | TO_SLEEP | TO_IGNOREINVIS, buf);
		ch->AddTellBuf(buf);
	}

	if (!IS_NPC(vict))
		vict->GetPlayer()->m_LastTellReceived = ch->GetID();
}


bool is_tell_ok(CharData *ch, CharData *vict) {
	if (ch == vict)
		ch->Send("You try to tell yourself something.\n");
	else if (CHN_FLAGGED(ch, Channel::NoTell))
		ch->Send("You can't tell other people while you have notell on.\n");
	else if (ROOM_FLAGGED(ch->InRoom(), ROOM_SOUNDPROOF) && !NO_STAFF_HASSLE(ch))
		ch->Send("The walls seem to absorb your words.\n");
	else if (!IS_NPC(vict) && !vict->desc)        /* linkless */
		act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if (PLR_FLAGGED(vict, PLR_WRITING))
		act("$E's writing a message right now; try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if ((CHN_FLAGGED(vict, Channel::NoTell)
			|| (ROOM_FLAGGED(IN_ROOM(vict), ROOM_SOUNDPROOF) && !NO_STAFF_HASSLE(vict)))
		&& !NO_STAFF_HASSLE(ch))
		act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if (vict->IsIgnoring(ch))
		act("$E is ignoring you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if (ch->IsIgnoring(vict))
		act("You are ignoring $M.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else
		return true;
	return false;
}



/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell) {
	BUFFER(buf, MAX_STRING_LENGTH * 4);
	CharData *vict;

	argument = one_argument(argument, buf);

	if (!*buf || !*argument) {
		ch->Send("\n----- Tell History (Max: 20) -----\n");

		FOREACH(Lexi::StringList, ch->GetPlayer()->m_TellBuffer, tell)
		{
			ch->Send("~ %s", tell->c_str());
		}
		
		return;
	} 
	
	vict = get_player_vis(ch, buf);
	if (!vict)
		vict = get_char_vis(ch, buf, FIND_CHAR_WORLD);
	
	if (!vict)
		ch->Send(NOPERSON);
	else if (is_tell_ok(ch, vict)) {
		if (!IS_NPC(vict) && !vict->GetPlayer()->m_AFKMessage.empty()) {
			act("$N is AFK and may not hear your tell.\n"
				"MESSAGE: $t\n", TRUE, ch, (ObjData *)vict->GetPlayer()->m_AFKMessage.c_str(), vict, TO_CHAR);
		}
		perform_tell(ch, vict, argument);
		if (!IS_NPC(vict))	vict->GetPlayer()->m_LastTellReceived = ch->GetID();
		if (!IS_NPC(ch))	ch->GetPlayer()->m_LastTellSent = vict->GetID();
	}
}


ACMD(do_reply) {
	CharData *tch;
	
	if (IS_NPC(ch))
		return;
	
	skip_spaces(argument);

	if (ch->GetPlayer()->m_LastTellReceived == 0)	ch->Send("You have no-one to reply to!\n");
	else if (!*argument)					ch->Send("What is your reply?\n");
	else if (!(tch = CharData::Find(ch->GetPlayer()->m_LastTellReceived)))
	{
		ch->Send("They are no longer playing.\n");
		ch->GetPlayer()->m_LastTellReceived = 0;
	} else if (is_tell_ok(ch, tch))			perform_tell(ch, tch, argument);
}


ACMD(do_retell) {
	CharData *tch;
	
	if (IS_NPC(ch))
		return;
	
	skip_spaces(argument);

	if (ch->GetPlayer()->m_LastTellSent == 0)	ch->Send("You have no-one to re-tell to!\n");
	else if (!*argument)					ch->Send("What is your message?\n");
	else if (!(tch = CharData::Find(ch->GetPlayer()->m_LastTellSent)))
	{
		ch->Send("They are no longer playing.\n");
		ch->GetPlayer()->m_LastTellSent = 0;
	} else if (is_tell_ok(ch, tch))			perform_tell(ch, tch, argument);
}


ACMD(do_spec_comm) {
	BUFFER(buf, MAX_STRING_LENGTH);
	CharData *vict;
	const char *action_sing, *action_plur, *action_others;

	if (subcmd == SCMD_WHISPER) {
		action_sing = "whisper to";
		action_plur = "whispers to";
		action_others = "$n whispers something to $N.";
	} else {
		action_sing = "ask";
		action_plur = "asks";
		action_others = "$n asks $N a question.";
	}

	argument = one_argument(argument, buf);

	if (!*buf || !*argument) {
		ch->Send("Whom do you want to %s.. and what??\n", action_sing);
	} else if (!(vict = get_char_vis(ch, buf, FIND_CHAR_ROOM)))
		send_to_char(NOPERSON, ch);
	else if (vict == ch)
		send_to_char("You can't get your mouth close enough to your ear...\n", ch);
	else if (vict->IsIgnoring(ch))
		act("$E is ignoring you.", FALSE, ch, NULL, vict, TO_CHAR);
	else {
		sprintf(buf, "$n %s you, '%s'", action_plur, argument);
		act(buf, FALSE, ch, 0, vict, TO_VICT);
		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else {
			sprintf(buf, "You %s $N, '%s'", action_sing, argument);
			act(buf, FALSE, ch, 0, vict, TO_CHAR);
		}
		act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
	}
}


#define MAX_NOTE_LENGTH 1000	/* arbitrary */

ACMD(do_write)
{
	ObjData *paper = NULL, *pen = NULL, *obj, *held;
	char *papername, *penname;
	int temp = 0;

	if (!ch->desc)
		return;

	/* before we do anything, lets see if there's a board involved. */
	if(!(obj = get_obj_in_list_type(ITEM_BOARD, ch->carrying)))
		obj = get_obj_in_list_type(ITEM_BOARD, ch->InRoom()->contents);
	
	if(obj) {                /* then there IS a board! */
		if (ch->CanUse(obj) != NotRestricted)
			ch->Send("You don't have the access privileges for that board!\n");
		else
			Board::WriteMessage(obj, ch, argument);
	} else {
		BUFFER(buf1, MAX_INPUT_LENGTH);
		BUFFER(buf2, MAX_INPUT_LENGTH);
		
		papername = buf1;
		penname = buf2;

		two_arguments(argument, papername, penname);

		if (!*papername)		/* nothing was delivered */
			send_to_char("Write?  With what?  ON what?  What are you trying to do?!?\n", ch);
		else if (*penname) {		/* there were two arguments */
			if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying)))
				ch->Send("You have no %s.\n", papername);
			else if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying)))
				ch->Send("You have no %s.\n", penname);
		} else {		/* there was one arg.. let's see what we can find */
			if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying)))
				ch->Send("There is no %s in your inventory.\n", papername);
//			else if (GET_OBJ_TYPE(paper) == ITEM_PEN) {	/* oops, a pen.. */
//				pen = paper;
//				paper = NULL;
//			}
			else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
				send_to_char("That thing has nothing to do with writing.\n", ch);
				return;
			}
			/* One object was found.. now for the other one. */
/*			held = GET_EQ(ch, WEAR_HAND_R);
			if (!held || (GET_OBJ_TYPE(held) != (pen ? ITEM_NOTE : ITEM_PEN))) {
				held = GET_EQ(ch, WEAR_HAND_L);
				if (!held || (GET_OBJ_TYPE(held) != (pen ? ITEM_NOTE : ITEM_PEN))) {
					ch->Send("You can't write with %s %s alone.\n", AN(papername), papername);
					pen = NULL;
				}
			}
			if (held && !ch->CanSee(held))
				send_to_char("The stuff in your hand is invisible!  Yeech!!\n", ch);
			else if (pen)	paper = held;
			else			pen = held;
*/		}
		
		if (paper /* && pen*/) {
			/* ok.. now let's see what kind of stuff we've found */
//			if (GET_OBJ_TYPE(pen) != ITEM_PEN)
//				act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
//			else
				if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
				act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
			else if (!paper->m_Description.empty())
				send_to_char("There's something written on it already.\n", ch);
			else {
				act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);

				send_to_char("Write your note.  (/s saves /h for help)\n", ch);

				send_to_char(paper->GetDescription(), ch);
				ch->desc->StartStringEditor(paper->m_Description, MAX_NOTE_LENGTH);				
			}
		}
	}
}


ACMD(do_page) {
	BUFFER(arg1, MAX_INPUT_LENGTH);

	CharData *vict;

	argument = one_argument(argument, arg1);

	if (IS_NPC(ch))
		send_to_char("Monsters can't page.. go away.\n", ch);
	else if (!*arg1)
		send_to_char("Whom do you wish to page?\n", ch);
	else {
		if (!str_cmp(arg1, "all")) {
			if (STF_FLAGGED(ch, STAFF_ADMIN | STAFF_GAME | STAFF_SECURITY)) {
				FOREACH(DescriptorList, descriptor_list, d) {
					if ((*d)->GetState()->IsPlaying() && (*d)->m_Character)
						act("`*`**$n* $t\n", FALSE, ch, (ObjData *)argument, (*d)->m_Character, TO_VICT);
				}
			} else
				send_to_char("You will never be privileged enough to do that!\n", ch);
		} else if ((vict = get_char_vis(ch, arg1, FIND_CHAR_WORLD)) != NULL) {
			act("`*`**$n* $t\n", FALSE, ch, (ObjData *)argument, vict, TO_VICT);
			if (PRF_FLAGGED(ch, PRF_NOREPEAT))
				send_to_char(OK, ch);
			else
				act("`*`*-$N-> $t\n", FALSE, ch, (ObjData *)argument, vict, TO_CHAR);
		} else
			send_to_char("There is no such player!\n", ch);
	}
}


/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

ACMD(do_gen_comm)
{
	/* Array of flags which must _not_ be set in order for comm to be heard */
	Flags channels[] = {
		Channel::NoShout,
		Channel::NoChat,
		Channel::NoMusic,
		Channel::NoGratz,
		0,
		Channel::NoClan,
		Channel::NoRace,
		0,
		Channel::NoBitch,
		0,
		Channel::NoNewbie
	};

	/*
	 * com_msgs: [0] Message if you can't perform the action because of noshout
	 *           [1] name of the action
	 *           [2] message if you're not on the channel
	 *           [3] a color string.
	 */
	const char *com_msgs[][5] = {
		{	"You cannot shout!!\n",
			"shout",
			"Turn off your noshout flag first!\n",
			"$n `yshouts '`(%s`)'`n",
			"`yYou shout '`(%s`)'`n",
		},

		{	"You cannot chat!!\n",
			"chat",
			"You aren't even on the channel!\n",
			"`y[`(`bCHAT`)] $n: %s`n",
			"`y[`(`bCHAT`)] -> `y%s`n",
		},

		{	"You cannot sing!!\n",
			"sing",
			"You aren't even on the channel!\n",
			"`y[`(`mMUSIC by $n`)] `m%s`n",
			"`y[`(`mMUSIC`)] -> `w%s`n",
		},

		{	"You cannot congratulate!\n",
			"congrat",
			"You aren't even on the channel!\n",
//			"`y[`bCONGRATS`y] $n`g: %s`n",
//			"`y[`bCONGRATS`y] -> `g%s`n",
			"`y[`(`gCONGRATS`)] $n`g: %s`n",
			"`y[`(`gCONGRATS`)] -> `g%s`n",
		},
		{	"You cannot broadcast info!\n",
			"broadcast",
			"",
			"`y[`bINFO`y] $n`w: %s`n",
			"`y[`bINFO`y] -> `w%s`n",
//			"`y[`wINFO`y] $n`w: %s`n",
//			"`y[`wINFO`y] -> `w%s`n",
		},
		{	"You cannot converse with your clan!\n",
			"clanchat",
			"You aren't even on the channel!\n",
//			"`y[`bCLAN`y] $n`r: %s`n",
//			"`y[`bCLAN`y] -> `r%s`n",
			"`y[`cCLAN`y] $n`c: %s`n",
			"`y[`cCLAN`y] -> `c%s`n",
		},
		{	"You cannot converse with your race!\n",
			"racechat",
			"You aren't even on the channel!\n",
//			"`y[`bRACE`y] $n`r: %s`n",
//			"`y[`bRACE`y] -> `r%s`n",
			"`y[`rRACE`y] $n`y: %s`n",
			"`y[`rRACE`y] -> `y%s`n",
		},
		{
			"You cannot chat with others on the mission!\n",
			"mission",
			"",
			"`y[`bMISSION`y] $n`g: %s`n",
			"`y[`bMISSION`y] -> `g%s`n"
		},
		{
			"You cannot bitch!\n",
			"bitch",
			"You aren't even listening to others bitch, you expect them to put up with your bitching?",
			"`y[`MBITCH`n`y] $n`y: %s`n",
			"`y[`MBITCH`n`y] -> `y%s`n"
		},
		{
			"You cannot chat with your team!\n",
			"tsay",
			"",
			"`y[`YTEAM`n`y] $n`y: %s`n",
			"`y[`YTEAM`n`y] -> `y%s`n"
		},
		{
			"You cannot talk on the newbie channel!\n",
			"newbie",
			"You aren't even on the channel!\n",
			"`y[`(`gNEWBIE`)] $n: %s`n",
			"`y[`(`gNEWBIE`)] -> %s`n"
		}
	};

	if (PLR_FLAGGED(ch, PLR_NOSHOUT) && subcmd != SCMD_CLAN) {
		if (ch->GetPlayer()->m_MuteLength)
		{
			int minutesRemaining = ((ch->GetPlayer()->m_MuteLength - (time(0) - ch->GetPlayer()->m_MutedAt)) + 59) / 60;
			
			if (minutesRemaining == 0)		ch->Send("[<1 minutes remaining]  ");
			else							ch->Send("[%d minutes remaining]  ", minutesRemaining);
		}
		send_to_char(com_msgs[subcmd][0], ch);
		return;
	}
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) && !NO_STAFF_HASSLE(ch))
	{
		send_to_char("The walls seem to absorb your words.\n", ch);
		return;
	}
	/* level_can_shout defined in config.c */
	if (ch->GetLevel() < level_can_shout) {
		BUFFER(buf1, MAX_INPUT_LENGTH);
		ch->Send("You must be at least level %d before you can %s.\n",
				level_can_shout, com_msgs[subcmd][1]);
		return;
	}
	/* make sure the char is on the channel */
	if (CHN_FLAGGED(ch, channels[subcmd])) {
		send_to_char(com_msgs[subcmd][2], ch);
		return;
	}
	if (subcmd == SCMD_MISSION && !CHN_FLAGGED(ch, Channel::Mission) && !IS_NPC(ch)) {
		send_to_char("You aren't even part of the mission!\n", ch);
		return;
	}
	if (subcmd == SCMD_TEAM && (!GET_TEAM(ch) || GET_TEAM(ch) == 1 /* SOLO */)) {
		send_to_char("You aren't even on a team!\n", ch);
		return;
	}
	if (subcmd == SCMD_CLAN && (!GET_CLAN(ch) || (!IS_NPC(ch) && !Clan::GetMember(ch)))) {
		send_to_char("You aren't even a clan member!\n", ch);
		return;
	}

	/* skip leading spaces */
	skip_spaces(argument);

	/* make sure that there is something there to say! */
	if (!*argument) {
		act("Yes, $t, fine, $t we must, but WHAT???\n", FALSE, ch, (ObjData *)com_msgs[subcmd][1], 0, TO_CHAR);
		return;
	}
	
	BUFFER(buf, MAX_STRING_LENGTH);
	BUFFER(storeBuf, MAX_STRING_LENGTH);
	
	/* first, set up strings to be given to the communicator */
	if (PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char(OK, ch);
	else {
		sprintf(buf, com_msgs[subcmd][4], argument);
		act(buf, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP, storeBuf);
		ch->AddChatBuf(storeBuf);
	}

	sprintf(buf, com_msgs[subcmd][3], argument);

	/* now send all the strings out */
	FOREACH(DescriptorList, descriptor_list, iter)
	{
		DescriptorData *i = *iter;
		
		if (i->GetState()->IsPlaying()
			&& (i != ch->desc) && i->m_Character
			&& !CHN_FLAGGED(i->m_Character, channels[subcmd])
			&& !PLR_FLAGGED(i->m_Character, PLR_WRITING)
			&& (!ROOM_FLAGGED(IN_ROOM(i->m_Character), ROOM_SOUNDPROOF) || NO_STAFF_HASSLE(i->m_Character)))
		{

			if (subcmd == SCMD_SHOUT)
			{
				if (GET_POS(i->m_Character) < POS_RESTING || !IsSameEffectiveZone(ch->InRoom(), i->m_Character->InRoom()))
					continue;
			}
			if ((subcmd == SCMD_CLAN) && 
					((GET_CLAN(ch) != GET_CLAN(i->m_Character)) || 
					!Clan::GetMember(i->m_Character)))
				continue;
			if ((subcmd == SCMD_TEAM) &&
					(GET_TEAM(ch) != GET_TEAM(i->m_Character)))
				continue;
			if ((subcmd == SCMD_RACE) && (ch->GetFaction() != i->m_Character->GetFaction()))
				continue;
			if ((subcmd == SCMD_MISSION) && !CHN_FLAGGED(i->m_Character, Channel::Mission))
				continue;
			
			*storeBuf = '\0';
			act(buf, FALSE, ch, 0, i->m_Character, TO_VICT | TO_SLEEP | TO_IGNOREINVIS | TO_IGNORABLE, storeBuf);
			i->m_Character->AddChatBuf(storeBuf);
			if (subcmd == SCMD_BROADCAST)	i->m_Character->AddInfoBuf(storeBuf);
		}
	}
}


ACMD(do_qcomm)
{
	skip_spaces(argument);
	
	if (!CHN_FLAGGED(ch, Channel::Mission)) {
		send_to_char("You aren't even on the mission channel!\n", ch);
	} else if (!*argument) {
		send_to_char("MEcho?  Yes, fine, %s we must, but WHAT??\n", ch);
	} else {
		mudlogf( NRM, LVL_ADMIN, TRUE,  "(GC) %s mechoed '`n%s`g'", ch->GetName(), argument);

		if (PRF_FLAGGED(ch, PRF_NOREPEAT))	send_to_char(OK, ch);
		else								act(argument, FALSE, ch, 0, 0, TO_CHAR);

		FOREACH(DescriptorList, descriptor_list, iter)
		{
			DescriptorData *i = *iter;
			if (i->GetState()->IsPlaying() && i != ch->desc && CHN_FLAGGED(i->m_Character, Channel::Mission))
				act(argument, 0, ch, 0, i->m_Character, TO_VICT | TO_SLEEP);
		}
	}
}


ACMD(do_history) {
	if (!ch->GetPlayer())
		return;
	
	ch->Send("\n----- Channel History (Max: 20) -----\n");
	
	FOREACH(Lexi::StringList, ch->GetPlayer()->m_ChatBuffer, msg)
	{
		ch->Send("~ %s", msg->c_str());
	}
}


ACMD(do_sayhistory) {
	if (!ch->GetPlayer())
		return;
	
	ch->Send("\n----- Say/Gsay History (Max: 20) -----\n");

	FOREACH(Lexi::StringList, ch->GetPlayer()->m_SayBuffer, msg)
	{
		ch->Send("~ %s", msg->c_str());
	}
}


ACMD(do_infohistory) {
	if (!ch->GetPlayer())
		return;
	
	ch->Send("\n----- Info History (Max: 20) -----\n");

	FOREACH(Lexi::StringList, ch->GetPlayer()->m_InfoBuffer, msg)
	{
		ch->Send("~ %s", msg->c_str());
	}
}



ACMD(do_ignore)
{
	BUFFER(arg, MAX_INPUT_LENGTH);
	CharData *victim;
	
	one_argument(argument, arg);
	
	if (IS_NPC(ch))
		ch->Send("NPCs can't ignore anyone.\n");
	else if (!*arg)
	{
		if (ch->GetPlayer()->m_IgnoredPlayers.empty())
			ch->Send("You aren't ignoring anybody.\n");
		else
		{
			ch->Send("You are currently ignoring:\n");
			
			int count = 0;
			FOREACH(std::list<IDNum>, ch->GetPlayer()->m_IgnoredPlayers, id)
			{
				const char *name = get_name_by_id(*id);
				if (name)
				{
					ch->Send("  %3d. %s\n", ++count, name);
				}
			}
		}
	}
	else if (!(victim = get_player(arg)))
		ch->Send(NOPERSON);
	else if (ch == victim)
		ch->Send("You can't ignore yourself.\n");
	else if (subcmd == SCMD_IGNORE)
	{
	
		if (IS_STAFF(victim))
			ch->Send("You can't ignore staff.\n");
		else if (GET_CLAN(ch) && (GET_CLAN(ch) == GET_CLAN(victim)))
			ch->Send("You can't ignore clan members.\n");
		else if (ch->IsIgnoring(GET_ID(victim)))
			act("You are already ignoring $N.", FALSE, ch, NULL, victim, TO_CHAR | TO_SLEEP);
		else
		{
			act("You add $N to your ignore list.", FALSE, ch, NULL, victim, TO_CHAR | TO_SLEEP);
			ch->AddIgnore(GET_ID(victim));
		}
	}
	else if (subcmd == SCMD_UNIGNORE)
	{
		if (!ch->IsIgnoring(GET_ID(victim)))
			act("You aren't ignoring $N.", FALSE, ch, NULL, victim, TO_CHAR | TO_SLEEP);
		else
		{
			act("You remove $N to your ignore list.", FALSE, ch, NULL, victim, TO_CHAR | TO_SLEEP);
			ch->RemoveIgnore(GET_ID(victim));
		}
	}
}
