/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 2005-2007        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Original LexiMUD Code                                                 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : clans.cp                                                       [*]
[*] Usage: Primary clan code                                              [*]
\***************************************************************************/

#include "clans.h"
#include "characters.h"
#include "interpreter.h"
#include "descriptors.h"
#include "db.h"
#include "utils.h"
#include "files.h"
#include "comm.h"
#include "find.h"
#include "rooms.h"
#include "buffer.h"
#include "constants.h"
#include "handler.h"
#include "staffcmds.h"
#include "lexifile.h"
#include "lexiparser.h"


//	Global Data
Clan **		Clan::Index = NULL;
unsigned	Clan::IndexSize = 0;
bool Clan::ms_SaveClans = false;

const Clan::RankPermissionBits	MAKE_BITSET(Clan::Permission_Bank_Mask, Permission_Bank_View, Permission_Bank_Deposit, Permission_Bank_Withdraw);

const unsigned int	MAX_CLAN_BANS = 10;

//	External Functions
int		count_hash_records(FILE * fl);
void announce(const char *buf);


//	Local Functions
ACMD(do_apply);
ACMD(do_enlist);
ACMD(do_boot);
ACMD(do_clanban);
ACMD(do_clanlist);
ACMD(do_clanstat);
ACMD(do_forceenlist);
ACMD(do_resign);
ACMD(do_clanwho);
ACMD(do_clanpromote);
ACMD(do_clandemote);
ACMD(do_clanallow);
ACMD(do_deposit);
ACMD(do_withdraw);
ACMD(do_war);
ACMD(do_ally);
ACMD(do_clanpolitics);
ACMD(do_clanrank);
ACMD(do_clanset);
ACMD(do_cmotd);


ENUM_NAME_TABLE_NS(Clan, RankPermissions, Clan::NumPermissions)
{
	{ Clan::Permission_ShowRank,		"SHOWRANK"		},
//	{ Clan::Permission_Vote,			"VOTE"			},
	
	{ Clan::Permission_EditMOTD,		"EDIT-MOTD"		},
	{ Clan::Permission_EditRanks,		"EDIT-RANKS"	},
	{ Clan::Permission_EditEnlist,		"EDIT-ENLIST"	},
	{ Clan::Permission_Enlist,			"ENLIST"		},
	{ Clan::Permission_Boot,			"BOOT"			},
	{ Clan::Permission_Ban,				"BAN"			},
	{ Clan::Permission_Promote,			"PROMOTE"		},
	{ Clan::Permission_Demote,			"DEMOTE"		},
	{ Clan::Permission_Allow,			"ALLOW"			},
	
	{ Clan::Permission_War,				"WAR"			},
	{ Clan::Permission_Ally,			"ALLY"			},
	
	{ Clan::Permission_Bank_View,		"BANK-VIEW"		},
	{ Clan::Permission_Bank_Deposit,	"BANK-DEPOSIT"	},
	{ Clan::Permission_Bank_Withdraw,	"BANK-WITHDRAW"	}
};


ENUM_NAME_TABLE_NS(Clan, ShowMOTD, Clan::NumShowMOTDOptions)
{
	{ Clan::ShowMOTD_Never,				"NEVER"			},
	{ Clan::ShowMOTD_HasChanged,		"HASCHANGED"	},
	{ Clan::ShowMOTD_WhenChanged,		"WHENCHANGED"	},
	{ Clan::ShowMOTD_Always,			"ALWAYS"		}
};


ENUM_NAME_TABLE_NS(Clan, ApplicationMode, Clan::NumApplicationModes)
{
	{ Clan::Applications_Deny,			"DENY"			},
	{ Clan::Applications_Allow,			"ALLOW"			},
	{ Clan::Applications_AutoAccept,	"AUTO"			}
};


Relation *	Clan::ms_Relations = NULL;


void Clan::RebuildRelations()
{
	unsigned int numRelationFlags = ((Clan::IndexSize * (Clan::IndexSize + 1)) / 2);
	
	delete ms_Relations;
	ms_Relations = new Relation[numRelationFlags];
	memset(ms_Relations, 0, sizeof(Relation) * numRelationFlags);
	
	for (unsigned int i = 0; i < Clan::IndexSize; ++i)
	{
		unsigned int	base = ((i * (i + 1)) / 2);
		Clan *	clan = Clan::Index[i];
		
		ms_Relations[base + i] = RELATION_FRIEND;
		
		for (unsigned int n = 0; n < i; ++n)
		{
			unsigned int index = base + n;
			Clan *otherclan = Clan::Index[n];

			Relation	relation = RELATION_NEUTRAL;
						
			//	Figure out relations here.
			if (Clan::AreAllies(clan, otherclan))			relation = RELATION_FRIEND;
			else if (Clan::AreEnemies(clan, otherclan))		relation = RELATION_ENEMY;
			
			ms_Relations[index] = relation;
		}
	}
}


Relation Clan::GetRelation(Clan *otherClan)
{
	/* ms_Relations is numFactions * (numFactions - 1) */
	
	/* 0 = 0,0
	   1 = 1,0	2 = 1,1,
	   3 = 2,0	4 = 2,1  5 = 2,2
	   6 = 3,0  7 = 3,1  8 = 3,2  9 = 3,3
	   10 =4,0 11 = 4,1 12 = 4,2 13 = 4,3 14 = 4,4
	 */
	unsigned int	low = m_RealNumber;
	unsigned int	high = otherClan->m_RealNumber;
	
	if (low > high)
		std::swap(low, high);
	
	if (high >= Clan::IndexSize)
		return RELATION_NONE;
	
	int index = ((high * (high + 1)) / 2) + low;
	
//	if (index < 0 || index > (Clan::IndexSize * Clan::TopOfIndex))
//		return 0;	//	Bad...
	
	return ms_Relations[index];
}


bool Clan::HasSharedFaction(Clan *otherClan)
{
	for (int i = 0; i < NUM_RACES; ++i)
	{
		if (!IS_SET(m_Races, 1 << i))
			continue;
		
		for (int i2 = 0; i2 < NUM_RACES; ++i2)
		{
			if (!IS_SET(otherClan->m_Races, 1 << i))
				continue;
			
			if (i == i2)						return true;
			if (factions[i] == factions[i2])	return true;
		}
	}
	
	return false;
}


Clan::Rank *Clan::GetMembersRank(CharData *ch)
{
	Member *member = GetMember(ch);
	return member ? member->m_pRank : NULL;
}


Clan::Rank *Clan::GetRank(const char *name) const
{
	for (RankList::const_iterator rank = m_Ranks.begin(); rank != m_Ranks.end(); ++rank)
		if ((*rank)->m_Name == name)
			return *rank;
	return NULL;
}


Clan::Rank *Clan::GetRank(int num) const
{
	if (num < 0 || num >= m_Ranks.size())	return NULL;
	
	Clan::RankList::const_iterator iter = m_Ranks.begin();
	std::advance(iter, num);
	
	return *iter;
}


//	Clan Data
Clan::Clan(IDNum id)
:	m_ID(id)
,	m_RealNumber(0)
,	m_Leader(0)
,	m_Races(0)
,	m_MaxMembers(0)
,	m_pDefaultRank(NULL)
,	m_ApplicationMode(Applications_Allow)
,	m_AutoEnlistMinLevel(0)
,	m_AutoEnlistMinPKs(0)
,	m_AutoEnlistMinDaysSinceCreation(0)
,	m_AutoEnlistMinHoursPlayed(0)
,	m_AutoEnlistMinPKPDRatio(0)
,	m_AutoEnlistMinPKMKRatio(0)
,	m_InactivityThreshold(0)
,	m_MOTDChangedOn(0)
,	m_ShowMOTD(ShowMOTD_Always)
,	m_Savings(0)
,	m_NumMembersOnline(0)
{
}


Clan::Member *Clan::AddMember(IDNum id)
{
	Member *member = GetMember(id);

	if (!member)
	{
		member = new Member(id);
		m_Members.push_back(member);
	}

	CharData *ch = CharData::Find(id);
	if (ch)
	{
		member->m_Name = ch->GetName();
		member->m_Character = ch;
		member->m_LastLogin = ch->GetPlayer()->m_LastLogin;
		
		Clan::SetMember(ch, member);
			
		++m_NumMembersOnline;
	}
	else
	{
		member->m_Name = get_name_by_id(id);
	}
	member->m_pRank = m_pDefaultRank;

	Clan::Save();
	
	return member;
}


void Clan::RemoveMember(Clan::Member *member)
{
	m_Members.remove(member);
	delete member;
	Clan::Save();
	
/*
	if (m_Members.size() == 0)
		return;
	
	//	Find a leader!  Make SURE we have one!
	Rank *leaderRank = NULL;
	for (RankList::iterator iter = m_Ranks.begin(), end = m_Ranks.end();
		!leaderRank && iter != end; ++iter)
	{
		if ((*iter)->m_Privileges == LEADER_PRIVILEGES)
			leaderRank = *iter;
	}
	
	if (!leaderRank)
		return;
	
	FOREACH(MemberList, m_Members, iter)
	{
		Member *member = *iter;
		if (member->m_pRank == leaderRank)
			return;	//	We got a leader, no worries!
	}
*/	
}


Clan::Member *Clan::GetMember(CharData *ch)
{
	return ch->GetPlayer()->m_pClanMembership;
}


void Clan::SetMember(CharData *ch, Clan::Member *member)
{
	ch->GetPlayer()->m_pClanMembership = member;
}


Clan::Member * Clan::GetMember(IDNum id) const
{
	FOREACH_CONST(MemberList, m_Members, iter)
	{
		if ((*iter)->m_ID == id)
		{
			return (*iter);
		}
	}
	
	return NULL;
}


Clan::Member * Clan::GetMember(const char *name) const
{
	FOREACH_CONST(MemberList, m_Members, iter)
	{
		if ((*iter)->m_Name == name)
		{
			return (*iter);
		}
	}
	
	return NULL;
}


bool Clan::IsApplicant(IDNum id) const	{ return (Lexi::IsInContainer(m_Applicants, id));	}
bool Clan::IsBanned(IDNum id) const		{ return (Lexi::IsInContainer(m_Bans, id));			}
bool Clan::IsEnemy(IDNum id) const		{ return (Lexi::IsInContainer(m_Enemies, id));		}
bool Clan::IsAlly(IDNum id) const		{ return (Lexi::IsInContainer(m_Allies, id));		}


bool	Clan::HasPermission(CharData *ch, RankPermissions permission) 
{
	if (IS_STAFF(ch) && STF_FLAGGED(ch, STAFF_CLANS))	return true;
	
	Rank *rank = GetMembersRank(ch);
	return rank && (rank->m_Order == 0
	 	|| rank->m_Privileges.test(permission)
	 	|| GetMember(ch)->m_Privileges.test(permission));
}


bool	Clan::HasAnyPermission(CharData *ch, RankPermissionBits permissions)
{
	if (IS_STAFF(ch) && STF_FLAGGED(ch, STAFF_CLANS))	return true;
	
	Rank *rank = GetMembersRank(ch);
	return rank && (rank->m_Order == 0
	 	|| rank->m_Privileges.testForAny(permissions)
	 	|| GetMember(ch)->m_Privileges.testForAny(permissions));
}


void Clan::OnPlayerToWorld(CharData *ch)
{
	Clan::Member *clanMember = GetMember(ch->GetID());
	
	Clan::SetMember(ch, clanMember);
	
	if (!clanMember)	return;

	clanMember->m_Character = ch;
	clanMember->m_Name = ch->GetName();
	++m_NumMembersOnline;
	
	clanMember->m_PreviousLogin = clanMember->m_LastLogin;	//	Cache this for the login message
	clanMember->m_LastLogin = ch->GetPlayer()->m_LoginTime;
	Clan::MarkForSave();

	FOREACH(Clan::MemberList, m_Members, iter)
	{
		Clan::Member *member = *iter;
		if (member->m_Character && member->m_Character != ch &&
			GET_REAL_LEVEL(member->m_Character) >= ch->GetPlayer()->m_StaffInvisLevel)
			member->m_Character->AddInfoBuf(
				member->m_Character->SendAndCopy("`y[`bINFO`y] %s %s has logged on.\n",
					clanMember->m_pRank->GetName(), ch->GetName()).c_str());
	}
}


void Clan::OnPlayerFromWorld(CharData *ch)
{
	Clan::Member *clanMember = Clan::GetMember(ch);
	
	if (!clanMember)	return;
	
	clanMember->m_Character = NULL;
	Clan::SetMember(ch, NULL);
	--m_NumMembersOnline;
	
	FOREACH(Clan::MemberList, m_Members, iter)
	{
		Clan::Member *member = *iter;
		if (member->m_Character
				&& member->m_Character != ch
				&& GET_REAL_LEVEL(member->m_Character) >= ch->GetPlayer()->m_StaffInvisLevel)
		{
			member->m_Character->AddInfoBuf(
				member->m_Character->SendAndCopy("`y[`bINFO`y]`n %s %s has logged off.\n",
					clanMember->m_pRank->GetName(), ch->GetName()).c_str());
		}
	}
}



void Clan::OnPlayerLoginMessages(CharData *ch)
{
	Clan::Member *clanMember = Clan::GetMember(ch);
	if (!clanMember)
	{
		if (!IsApplicant(ch->GetID()))
		{
			ch->Send("`rDuring your absence, you have been booted or rejected from %s.`n\n", GetName());
			GET_CLAN(ch) = NULL;
		}
	}
	else
	{
		if (clanMember->m_PreviousLogin == 0)
		{
			ch->Send("`rDuring your absence, you have been accepted into %s.`n\n", GetName());
		}
		
		if (!m_MOTD.empty())
		{
			bool	bMOTDChanged = clanMember->m_PreviousLogin < m_MOTDChangedOn;
			
			if (m_ShowMOTD == Clan::ShowMOTD_Always
				|| (m_ShowMOTD == Clan::ShowMOTD_WhenChanged && bMOTDChanged))
			{
				ch->Send("%s`n's Message of the Day:\n", GetName());
				page_string(ch->desc, m_MOTD.c_str());
			}
			else if (m_ShowMOTD == Clan::ShowMOTD_HasChanged && bMOTDChanged)
			{
				ch->Send("%s`n's message of the day has changed.  Type '`ccmotd`n' to read it.\n", GetName());
			}
		}
	}
}


bool Clan::CanAutoEnlist(CharData *ch)
{
	if (m_ApplicationMode != Applications_AutoAccept)
		return false;
	
	if (ch->GetLevel() < m_AutoEnlistMinLevel)
		return false;
	
	if (ch->GetPlayer())
	{
		if (ch->GetPlayer()->m_PlayerKills < m_AutoEnlistMinPKs)
			return false;
		
		int secondsSinceCreation = time(0) - ch->GetPlayer()->m_Creation;
		int daysSinceCreation = secondsSinceCreation / (60*60*24);
		if (daysSinceCreation < m_AutoEnlistMinDaysSinceCreation)
			return false;
		
		int secondsPlayed = (time(0) - ch->GetPlayer()->m_LoginTime) + ch->GetPlayer()->m_TotalTimePlayed;
		int hoursPlayed = secondsPlayed / (60 * 60);
		if (hoursPlayed < m_AutoEnlistMinHoursPlayed)
			return false;
		
		float pkpdRatio = ch->GetPlayer()->m_PlayerDeaths > 0 ? ch->GetPlayer()->m_PlayerKills / ch->GetPlayer()->m_PlayerDeaths : 0;
		if (pkpdRatio < m_AutoEnlistMinPKPDRatio)
			return false;
		
		float pkmkRatio = ch->GetPlayer()->m_MobKills > 0 ? ch->GetPlayer()->m_PlayerKills / ch->GetPlayer()->m_MobKills : 0;
		if (pkmkRatio < m_AutoEnlistMinPKMKRatio)
			return false;
	}
	
	return true;
}


void Clan::CreateDefaultRanks()
{
	Rank *	rank;
	
	rank = new Rank;
	rank->m_Name = "Leader";
	rank->m_Order = 0;
	m_Ranks.push_back(rank);
	
	rank = new Rank;
	rank->m_Name = "Member";
	rank->m_Order = 1;
	m_pDefaultRank = rank;

	m_Ranks.push_back(rank);
	
	SortRanks();
}


ACMD(do_clanlist)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	
	strcpy(buf, "   #  Clan                                   Members   Leader         Points\n"
				"----------------------------------------------------------------------------\n");
	//			 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 
	for (unsigned int i = 0; i < Clan::IndexSize; ++i)
	{
		Clan *clan = Clan::Index[i];
		
		sprintf_cat(buf, " %3d. %-*s %2d / %2d   %-12s   %6d\n",
				clan->GetID(),
				cstrnlen_for_sprintf(clan->GetName(), 38),
				clan->GetName(),
				clan->GetNumMembersOnline(),
				clan->m_Members.size(),
				get_name_by_id(clan->m_Leader, "<NONE>"),
				clan->m_Savings);
	}
	page_string(ch->desc, buf);
}     


ACMD(do_clanstat) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	Clan *	clan = NULL;
	
	one_argument(argument, arg);
	
	
	if (*argument)
	{
		clan = Clan::GetClan(atoi(argument));
		
		if (!clan)
		{
			ch->Send("Clan does not exist.\n");
			return;
		}
	}
	else if (IS_NPC(ch) || Clan::GetMember(ch))
		clan = GET_CLAN(ch);
	
	if (!clan)
		ch->Send("Stats on which clan?\n");
	else
	{
		BUFFER(buf, MAX_STRING_LENGTH);
		sprintbit(clan->m_Races, race_types, buf);
		

		RoomData *room = world.Find(clan->GetRoom());
		ch->Send("Clan       : %d\n"
				 "Name       : %s\n"
				 "Tag        : %s\n"
				 "Races      : %s\n",
				clan->GetID(),
				clan->GetName(),
				*clan->GetTag() ? clan->GetTag() : "<NONE>",
				clan->m_Races != 0 ? buf : "ANY");
	
		if (IS_STAFF(ch))
		{

			RoomData *room = world.Find(clan->GetRoom());
			ch->Send("Leader     : [%5d] %s\n"
					 "Room       : [%s] %40s\n",
					clan->m_Leader, get_name_by_id(clan->m_Leader, "<NONE>"),
					clan->GetRoom().Print().c_str(), room  ? room->GetName() : "<Not Created>");
		}
		else
		{
			ch->Send("Leader     : %s\n", get_name_by_id(clan->m_Leader, "<NONE>"));

		}
		
		int		numAllies = 0;
		ch->Send("Allies     :");
		for (int i = 0; i < Clan::IndexSize; ++i)
		{
			Clan *otherClan = Clan::Index[i];
			if (clan != otherClan && Clan::AreAllies(clan, otherClan))
			{
				++numAllies;
				ch->Send(" `(%s`)", otherClan->GetTag());
			}
		}
		if (numAllies == 0)	ch->Send("<None>");

		int		numEnemies = 0;
		ch->Send("\n"
				 "Enemies    :");
		for (int i = 0; i < Clan::IndexSize; ++i)
		{
			Clan *otherClan = Clan::Index[i];
			if (clan != otherClan && Clan::AreEnemies(clan, otherClan))
			{
				++numEnemies;
				ch->Send(" `(%s`)", otherClan->GetTag());
			}
		}
		if (numEnemies == 0)	ch->Send("<None>");

		ch->Send("\n"
				 "Savings    : %d\n"
				 "Members    : %d%s\n"
				 "Application: %s\n",
				clan->m_Savings,
				clan->m_Members.size(), clan->m_MaxMembers != 0 ? Lexi::Format(" (limit = %d)", clan->m_MaxMembers).c_str() : "",
				GetEnumName(clan->m_ApplicationMode));
		
		if (clan->m_ApplicationMode == Clan::Applications_AutoAccept)
		{
			ch->Send("     Min Level   : %-5d          Min PKs     : %d\n"
					 "     Days (Age)  : %-5d          Hours Played: %d\n"
					 "     PK:PD Ratio : %-5.2f        PK:MK Ratio : %.2f\n",
					 clan->m_AutoEnlistMinLevel, clan->m_AutoEnlistMinPKs,
					 clan->m_AutoEnlistMinDaysSinceCreation, clan->m_AutoEnlistMinHoursPlayed,
					 clan->m_AutoEnlistMinPKPDRatio, clan->m_AutoEnlistMinPKMKRatio);
		}
	}
}


#if 0
ACMD(do_members) {
	int	counter, columns = 0;
	CharData *applicant;
	DescriptorData *d;
	Clan *	clan;
	const char *name;
	bool	found = false;
	
	clan = GET_CLAN(ch);
	
	if (IS_STAFF(ch) && *argument)
	{
		clan = Clan::GetClan(atoi(argument));
		
		if (!clan)
		{
			ch->Send("Clan does not exist.\n");
			return;
		}
	}
	
	if ((clan == -1) || (!IS_NPC(ch) && !Clan::GetMember(ch))) {
		ch->Send("You aren't even in a clan!\n");
		return;
	}
	
	BUFFER(buf, MAX_STRING_LENGTH);

	sprintf(buf, "Members of %s:\n", Clan::Index[clan]->GetName());
	columns = counter = 0;
	FOREACH(Clan::MemberList, Clan::Index[clan]->m_Members, member)
	{
		name = get_name_by_id((*member)->m_ID);
		if (name)
		{
			found = true;
			sprintf_cat(buf, "  %2d] %-20.20s %s", ++counter, name,
					!(++columns % 2) ? "\n" : "");
		}
	}
	if (columns % 2)	strcat(buf, "\n");
	if (!found)			strcat(buf, "  None.\n");
	columns = 0;
	found = false;
	LListIterator<DescriptorData *>	desc_iter(descriptor_list);
	while ((d = desc_iter.Next())) {
		applicant = d->Original();
		
		if (applicant && !IS_NPC(applicant) &&
				(GET_CLAN(applicant) == clan) &&
				!Clan::GetMember(applicant))
			{
			if (!found)
			{
				strcat(buf, "Applicants Currently Online:\n");
				found = true;
			}
			sprintf_cat(buf, "  %2d] %-20.20s %s", ++counter, applicant->GetName(),
					!(++columns % 2) ? "\n" : "");
		}
	}
	if (columns % 2)	strcat(buf, "\n");
	sprintf_cat(buf, "\n%d players listed.\n", counter);
	page_string(ch->desc, buf, true);

}
#endif


static bool ClanWhoSorter(Clan::Member *a, Clan::Member *b)
{
	return a->m_pRank->m_Order < b->m_pRank->m_Order;
}

ACMD(do_clanwho)
{
	Clan *			clan = NULL;
	int				players = 0;
	bool			bShowInactive = false;
	bool			bShowLastOn = false;

	BUFFER(buf, MAX_STRING_LENGTH * 4);
	BUFFER(buf2, MAX_STRING_LENGTH);
	
	
	if (IS_NPC(ch) || Clan::GetMember(ch))
		clan = GET_CLAN(ch);

	while (*argument)
	{
		argument = one_argument(argument, buf);

		if (is_number(buf))
		{
			clan = Clan::GetClan(atoi(buf));
			
			if (!clan)
			{
				ch->Send("Clan does not exist.\n");
				return;
			}
		}
		else if (!str_cmp(buf, "-I"))	bShowInactive = true;
		else if (!str_cmp(buf, "-L"))	bShowLastOn = true;
	}
	

	if (!clan)
	{
		ch->Send("If you aren't in a clan, you must specify one!\n");
		return;
	}

	bool			bSameClan = false;
	if (IS_NPC(ch) || Clan::GetMember(ch))
		bSameClan = (clan == GET_CLAN(ch));
	
	sprintf(buf,"Members of %s\n"
				"-----------------------------\n",
				clan->GetName());

	Clan::MemberList clanMembers = clan->m_Members;
	Clan::MemberList inactiveClanMembers;
	clanMembers.sort(ClanWhoSorter);
	
	int membersOnline = 0;
	FOREACH(Clan::MemberList, clanMembers, iter)
	{
		Clan::Member *member = *iter;
		CharData *memberCh = member->m_Character;
		bool	bIsOnline = memberCh && memberCh->desc && memberCh->desc->GetState()->IsPlaying();
		
		if (!bIsOnline)
		{
			if (bShowInactive
				&& clan->m_InactivityThreshold != 0
				&& (time(0) - member->m_LastLogin) > (clan->m_InactivityThreshold * 24 * 60 * 60))
			{
				inactiveClanMembers.push_back(member);
				continue;
			}
			
			sprintf_cat(buf,  "`K[Offline] ");
		}
		else if (IS_STAFF(memberCh))
			sprintf_cat(buf,  "          ");
		else
			sprintf_cat(buf, "[%3d %s] ", memberCh->GetLevel(), findtype(memberCh->GetRace(), race_abbrevs));
		
		sprintf(buf2, "%s %s",
			member->m_pRank->GetName(),
			member->GetName());
		
		if (bIsOnline)
		{
			++membersOnline;
			
			if (PLR_FLAGGED(memberCh, PLR_MAILING))				strcat(buf2, " (`bmailing`n)");
			else if (PLR_FLAGGED(memberCh, PLR_WRITING))		strcat(buf2, " (`rwriting`n)");

			if (CHN_FLAGGED(memberCh, Channel::NoShout))		strcat(buf2, " (`gdeaf`n)");
			if (CHN_FLAGGED(memberCh, Channel::NoTell))			strcat(buf2, " (`gnotell`n)");
			if (CHN_FLAGGED(memberCh, Channel::Mission))		strcat(buf2, " (`mmission`n)");
			if (AFF_FLAGGED(memberCh, AFF_TRAITOR))				strcat(buf2, " (`RTRAITOR`n)");
			if (!memberCh->GetPlayer()->m_AFKMessage.empty())	strcat(buf2, " `c[AFK]`n");
		}
		else
		{
			int numDaysAgo = ((time(0) - member->m_LastLogin) / 3600) / 24;

			sprintf(buf2, "%-50.50s %s", strip_color(buf2).c_str(),
				bShowLastOn && member->m_LastLogin > 0 && (bSameClan || IS_STAFF(ch))
					? Lexi::Format("[%3d days]", numDaysAgo).c_str() : "");
		}
		
		sprintf_cat(buf, "%s`n\n", buf2);
	}
	
	FOREACH(Clan::MemberList, inactiveClanMembers, iter)
	{
		Clan::Member *member = *iter;
		
		int numDaysAgo = ((time(0) - member->m_LastLogin) / 3600) / 24;
		
		sprintf(buf2, "%s %s", member->m_pRank->GetName(), member->GetName());
		sprintf_cat(buf, "`K[MIA    ] %-50.50s %s`n\n",
			strip_color(buf2).c_str(),
			member->m_LastLogin > 0 && (bSameClan || IS_STAFF(ch))
				? Lexi::Format("[%3d days]", numDaysAgo).c_str() : "");
	}
	
	
	if (membersOnline == 0)	strcat(buf, "\nNo clan members are currently online.\n");
	else					sprintf_cat(buf, "\nThere %s %d clan member%s online.\n", (membersOnline == 1 ? "is" : "are"), membersOnline, (membersOnline == 1 ? "" : "s"));
	
	if (bSameClan && !clan->m_Applicants.empty())
	{
		strcat(buf, "The following players are applying to the clan:\n");
		
		int count = 0;
		FOREACH(Clan::ApplicantList, clan->m_Applicants, iter)
		{
			const char *name = get_name_by_id(*iter);
			if (!name)	continue;

			sprintf_cat(buf, " %2d. %s\n", ++count, name);
		}
	}
	
	page_string(ch->desc, buf);
}


ACMD(do_apply) {
	Clan *	clan = NULL;

	if (*argument)
	{
		clan = Clan::GetClan(atoi(argument));
		
		if (!clan)
		{
			ch->Send("That clan does not exist.\n");
			return;
		}
	}

	if (IS_NPC(ch))
		ch->Send("You can't join a clan you dolt!\n");
	else if (GET_CLAN(ch) && Clan::GetMember(ch))
		ch->Send("You are already a member of a clan!\n");
	else if (!clan)
		ch->Send("Apply to WHAT clan?\n");
	else if (!IS_STAFF(ch) && clan->m_Races != 0 && !IS_SET(clan->m_Races, 1 << ch->GetRace()))
		ch->Send("You would not be welcome among their kind.\n");
	else if (!IS_STAFF(ch) && clan->m_ApplicationMode == Clan::Applications_Deny)
		ch->Send("\"%s\" is not currently accepting new members.\n", clan->GetName());
	else
	{
		if (GET_CLAN(ch))
		{
			GET_CLAN(ch)->RemoveApplicant(ch->GetID());
			ch->Send("You remove your application to \"%s\".\n", GET_CLAN(ch)->GetName());
			
			GET_CLAN(ch) = NULL;
		}
		
		if (!IS_STAFF(ch) && clan->IsBanned(ch->GetID()))
		{
			ch->Send("You are not welcome in \"%s\".\n", clan->GetName());
			return;
		}
		
		GET_CLAN(ch) = clan;
		Clan::SetMember(ch, NULL);	//	Just to make sure
		
		if (clan->CanAutoEnlist(ch))
		{
			clan->AddMember( ch->GetID() );
			
			FOREACH(Clan::MemberList, clan->m_Members, iter)
			{
				if ((*iter)->m_Character && (*iter)->m_Character != ch)
					(*iter)->m_Character->Send("`y[`cCLAN`y]`n %s has been enlisted into %s`n.\n", ch->GetName(), clan->m_Name.c_str());
			}
			ch->Send("You have been automatically enlisted into \"%s`n\"!\n", clan->GetName());
		}
		else
		{
			clan->AddApplicant(ch->GetID());
			ch->Send("You have applied to \"%s`n\".\n", clan->GetName());
		}
		
		Clan::Save();
	}
}


ACMD(do_resign)
{
	Clan * 	clan = GET_CLAN(ch);

	if (IS_NPC(ch))
		ch->Send("Very funny.\n");
	else if (!clan)
		ch->Send("You aren't even in a clan!\n");
	else
	{
		if (Clan::GetMember(ch))
		{
			clan->RemoveMember(Clan::GetMember(ch));
			Clan::SetMember(ch, NULL);
			
			ch->Send("You resign from \"%s\".\n", clan->GetName());
			FOREACH(Clan::MemberList, clan->m_Members, iter)
			{
				if ((*iter)->m_Character && (*iter)->m_Character != ch)
					(*iter)->m_Character->Send("`y[`cCLAN`y]`n %s has resigned from %s`n.\n", ch->GetName(), clan->m_Name.c_str());
			}
			
			--clan->m_NumMembersOnline;
		}
		else
		{
			clan->RemoveApplicant(ch->GetID());
			ch->Send("You remove your application to \"%s\".\n", clan->GetName());
		}
		
		GET_CLAN(ch) = NULL;
		ch->Save();
		
		Clan::Save();
	}
}


ACMD(do_enlist)
{
	BUFFER(arg, MAX_INPUT_LENGTH);
	Clan *			clan = GET_CLAN(ch);
	if (!IS_NPC(ch) && !Clan::GetMember(ch))	clan = NULL;
	
	argument = one_argument(argument, arg);
	
	if (IS_STAFF(ch) && STF_FLAGGED(ch, STAFF_CLANS) && is_number(arg))
	{
		clan = Clan::GetClan(atoi(arg));
		if (!clan)
		{
			ch->Send("Clan not found.\n");
			return;
		}
		
		argument = one_argument(argument, arg);
	}
	
	/*if (IS_NPC(ch))
		ch->Send("Very funny.\n");
	else*/ if (clan == NULL)
		ch->Send("You aren't even in a clan!\n");
	else if (!IS_NPC(ch) && !Clan::HasPermission(ch, Clan::Permission_Enlist))
		ch->Send("You do not have the ability to do that.\n");
	else if (!*arg)
		ch->Send("Enlist who?\n");
	else
	{
		IDNum		applicantID = 0;
		CharData *	applicant = get_player(arg);
		
		if (applicant)
			applicantID = applicant->GetID();
		else
		{
			FOREACH(Clan::ApplicantList, clan->m_Applicants, iter)
			{
				IDNum id = *iter;
				const char *name = get_name_by_id(id);
				
				if (name && is_abbrev(arg, name))
				{
					applicantID = id;
					break;
				}
			}
		}
		
		if (!applicantID)
				ch->Send("Nobody by that name is applying to this clan!\n");
		else if (applicant && GET_CLAN(applicant) != clan)
			ch->Send("They are not applying for your clan!\n");
		else if (applicant && Clan::GetMember(applicant))
			ch->Send("They are already in your clan!\n");
		else if (clan->m_MaxMembers > 0 && clan->m_Members.size() >= clan->m_MaxMembers)
			ch->Send("Your clan is already at the maximum number of members.\n");
		else
		{
			clan->RemoveApplicant( applicantID );
			Clan::Member *member = clan->AddMember( applicantID );
			
			Clan::Save();
			
			ch->Send("You have enlisted %s into %s`n!\n", member->GetName(), clan->GetName());
			FOREACH(Clan::MemberList, clan->m_Members, iter)
			{
				if ((*iter)->m_Character && (*iter)->m_Character != applicant)
					(*iter)->m_Character->Send("`y[`cCLAN`y]`n %s has been enlisted into %s`n.\n", member->GetName(), clan->m_Name.c_str());
			}
			if (applicant)	applicant->Send("You have been accepted into %s`n!\n", clan->GetName());
		}
	}
}


ACMD(do_clanpromote)
{
	BUFFER(name, MAX_INPUT_LENGTH);
	BUFFER(rankName, MAX_INPUT_LENGTH);
	
	bool bStaffCommand = false;
	Clan::Member *	member;
	Clan::Rank *	rank;
	Clan *			clan = GET_CLAN(ch);
	if (!IS_NPC(ch) && !Clan::GetMember(ch))	clan = NULL;
	
	if (IS_STAFF(ch) && STF_FLAGGED(ch, STAFF_CLANS) && is_number(argument))
	{
		bStaffCommand = true;
		
		argument = one_argument(argument, name);
		
		clan = Clan::GetClan(atoi(name));
		if (!clan)
		{
			ch->Send("Clan not found.\n");
			return;
		}
	}
	
	two_arguments(argument, name, rankName);
	
	if (IS_NPC(ch))
		ch->Send("Very funny.\n");
	else if (clan == NULL)
		ch->Send("You aren't even in a clan!\n");
	else if (!*name || !*rankName)
		ch->Send("Usage: cpromote <who> <rank>\n");
	else if ((member = clan->GetMember(name)) == NULL)
		ch->Send("There is no such person in the clan!\n");
	else if (!bStaffCommand && ch->GetID() == member->m_ID)
		ch->Send("To yourself?!?  Dumbass.\n");
	else if ((rank = clan->GetRank(atoi(rankName) - 1)) == NULL)
		ch->Send("That is not a valid rank.\n");
	else if (!Clan::HasPermission(ch, Clan::Permission_Promote))	//	Can they promote
		ch->Send("You are not allowed to promote others.\n");
	else if (!bStaffCommand
			&& !Clan::IsHigherRank(Clan::GetMembersRank(ch), rank)
			&& ch->GetID() != clan->m_Leader)		//	Is the player higher than the rank?
		ch->Send("You may not promote members to that rank.");
	else if (!bStaffCommand
			&& !Clan::IsHigherRank(Clan::GetMembersRank(ch), member->m_pRank)
			&& ch->GetID() != clan->m_Leader)	//	Is the player higher than the target?
		ch->Send("You may only promote clan members below you.\n");
	else if (member->m_pRank == rank)
		ch->Send("They are aleady of that rank.");
	else if (!Clan::IsHigherRank(rank, member->m_pRank))	//	Is the rank higher than the target already?
		ch->Send("That would be a demotion.\n");	
	else
	{
		member->m_pRank = rank;
		
		if (member->m_Character)
			member->m_Character->Send("You have been promoted to %s!", rank->GetName());

		ch->Send("You have promoted %s to %s!", member->GetName(), rank->GetName());
		
		Clan::Save();
	}
}


ACMD(do_clandemote)
{
	BUFFER(name, MAX_INPUT_LENGTH);
	BUFFER(rankName, MAX_INPUT_LENGTH);
	
	bool bStaffCommand = false;
	Clan::Member *	member;
	Clan::Rank *	rank;
	Clan *			clan = GET_CLAN(ch);
	if (!IS_NPC(ch) && !Clan::GetMember(ch))	clan = NULL;
	
	if (IS_STAFF(ch) && STF_FLAGGED(ch, STAFF_CLANS) && is_number(argument))
	{
		bStaffCommand = true;
		
		argument = one_argument(argument, name);
		
		clan = Clan::GetClan(atoi(name));
		if (!clan)
		{
			ch->Send("Clan not found.\n");
			return;
		}
	}
	
	two_arguments(argument, name, rankName);
	
	if (IS_NPC(ch))
		ch->Send("Very funny.\n");
	else if (clan == NULL)
		ch->Send("You aren't even in a clan!\n");
	else if (!*name || !*rankName)
		ch->Send("Usage: cdemote <who> <rank>\n");
	else if ((member = clan->GetMember(name)) == NULL)
		ch->Send("There is no such person in the clan!\n");
	else if (!bStaffCommand && ch->GetID() == member->m_ID)
		ch->Send("To yourself?!?  Dumbass.\n");
	else if ((rank = clan->GetRank(atoi(rankName) - 1)) == NULL)
		ch->Send("That is not a valid rank.\n");
	else if (!Clan::HasPermission(ch, Clan::Permission_Demote))	//	Can they promote
		ch->Send("You are not allowed to demote others.\n");
	else if (!bStaffCommand
			&& !Clan::IsHigherRank(Clan::GetMembersRank(ch), rank)
			&& ch->GetID() != clan->m_Leader)	//	Is the player higher than the rank?
		ch->Send("You may not demote members to that rank.");
	else if (!bStaffCommand
			&& !Clan::IsHigherRank(Clan::GetMembersRank(ch), member->m_pRank)
			&& ch->GetID() != clan->m_Leader)	//	Is the player higher than the target?
		ch->Send("You may only demote clan members below you.\n");
	else if (member->m_pRank == rank)
		ch->Send("They are aleady of that rank.");
	else if (!Clan::IsHigherRank(member->m_pRank, rank))		//	Is the rank lower than the target already?
		ch->Send("That would be a promotion.\n");	
	else
	{
		member->m_pRank = rank;
		
		if (member->m_Character)
			member->m_Character->Send("You have been demoted to %s!", rank->GetName());

		ch->Send("You have demoted %s to %s!", member->GetName(), rank->GetName());
		
		Clan::Save();
	}
}


ACMD(do_forceenlist) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	CharData *	applicant;
	Clan *		clan;
	
	argument = one_argument(argument, arg);
	
	if (!*arg)
		ch->Send("Force-Enlist who?\n");
	else if (!(applicant = get_player_vis(ch, arg)))
		ch->Send(NOPERSON);
	else if ((clan = GET_CLAN(applicant)) == NULL)
		ch->Send("They aren't applying to a clan.\n");
	else if (Clan::GetMember(applicant) != NULL)
		ch->Send("They are already a member of a clan.\n");
	else if (!clan->IsApplicant(applicant->GetID()))
		ch->Send("They are not in the applicant list.  This is an error.\n");
	else
	{
		clan->RemoveApplicant( applicant->GetID() );
		Clan::Member *member = clan->AddMember( applicant->GetID() );
		
		//	To make them the leader, add 'leader' after forceenlist!
		if (!str_cmp(argument, "leader") || applicant->GetID() == clan->m_Leader)
		{
			member->m_pRank = clan->m_Ranks.front();
		}

		act("You have been accepted into $t!", TRUE, 0, (ObjData *)clan->GetName(), applicant, TO_VICT);
		act("You have force-enlisted $N into $t!", TRUE, ch, (ObjData *)clan->GetName(), applicant, TO_CHAR);
		
		if (Clan::GetMembersRank(applicant)->m_Order == 0)
			applicant->Send("You are the leader of %s.\n", clan->GetName());

		applicant->Save();
		
		Clan::Save();
	}
}


ACMD(do_boot) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	bool		bStaffCommand = false;
	CharData *	victim;
	Clan::Member *	member;
	Clan *			clan;
	
	clan = GET_CLAN(ch);
	
	if (!IS_NPC(ch) && !Clan::GetMember(ch))	clan = NULL;
	
	if (IS_STAFF(ch) && STF_FLAGGED(ch, STAFF_CLANS) && is_number(argument))
	{
		bStaffCommand = true;
		
		argument = one_argument(argument, arg);
		
		clan = Clan::GetClan(atoi(arg));
		if (!clan)
		{
			ch->Send("Clan not found.\n");
			return;
		}
	}
	
	one_argument(argument, arg);
	
	/*if (IS_NPC(ch))
		ch->Send("Very funny.\n");
	else*/ if (clan == NULL)
		ch->Send("You aren't even in a clan!\n");
	else if (!IS_NPC(ch) && !Clan::HasPermission(ch, Clan::Permission_Boot))
		ch->Send("You do not have the ability to do that.\n");
	else if (!*arg)
		ch->Send("Boot who?\n");
	else if (!(member = clan->GetMember(arg)))
	{
		//	Might be an applicant
		
		IDNum id = get_id_by_name(arg);
		
		if (id == NoID)
			ch->Send("No such player exists.\n");
		else if (!clan->IsApplicant(id))
			ch->Send("They are not applying to join your clan.\n");	
		else
		{
			victim = CharData::Find(id);
			if (victim && IS_NPC(victim))
				victim = NULL;
			
			if (victim)
			{
				victim->Send("You have been denied entry into clan %s.\n", clan->GetName());
				GET_CLAN(victim) = NULL;
				victim->Save();
			}

			ch->Send("You have denied %s into the clan.\n", get_name_by_id(id, ""));
			
			clan->RemoveApplicant(id);
			Clan::Save();
		}
	}
	else
	{
		//	A member to boot
		
		if (!bStaffCommand
				&& !IS_NPC(ch)
				&& !Clan::IsHigherRank(Clan::GetMembersRank(ch), member->m_pRank)
				&& ch->GetID() != clan->m_Leader)
			ch->Send("%s is not below you in rank.\n", member->GetName());
		else
		{
			if (member->m_Character)
			{
				member->m_Character->Send("You have been booted from %s by %s!\n", clan->GetName(), ch->GetName());
				Clan::SetMember(member->m_Character, NULL);
				GET_CLAN(member->m_Character) = NULL;
				member->m_Character->Save();
				
				--clan->m_NumMembersOnline;
			}

			ch->Send("You have booted %s from %s!\n", member->GetName(), clan->GetName());
			
			FOREACH(Clan::MemberList, clan->m_Members, iter)
			{
				if ((*iter)->m_Character && *iter != member)
					(*iter)->m_Character->Send("`y[`cCLAN`y]`n %s has been booted from %s`n.\n", member->GetName(), clan->GetName());
			}
			
			clan->RemoveMember(member);
			
			Clan::Save();
		}
	}
}


ACMD(do_clanban)
{
	BUFFER(arg, MAX_INPUT_LENGTH);
	bool			bStaffCommand = false;
	IDNum			banned;
	Clan *			clan;
	
	clan = GET_CLAN(ch);
	
	if (!IS_NPC(ch) && !Clan::GetMember(ch))	clan = NULL;
	
	if (IS_STAFF(ch) && STF_FLAGGED(ch, STAFF_CLANS) && is_number(argument))
	{
		bStaffCommand = true;
		
		argument = one_argument(argument, arg);
		
		clan = Clan::GetClan(atoi(arg));
		if (!clan)
		{
			ch->Send("Clan not found.\n");
			return;
		}
	}
	
	one_argument(argument, arg);
	
	if (IS_NPC(ch))
		ch->Send("Very funny.\n");
	else if (clan == NULL)
		ch->Send("You aren't even in a clan!\n");
	else if (!IS_NPC(ch) && !Clan::HasPermission(ch, Clan::Permission_Ban))
		ch->Send("You do not have the ability to do that.\n");
	else if (!*arg)
	{
		if (clan->m_Bans.empty())
			ch->Send("Nobody is currently banned from \"%s\".\n", clan->GetName());
		else
		{
			ch->Send("The following players are banned from applying to \"%s\":\n", clan->GetName());
			int count = 0;
			FOREACH(Clan::BanList, clan->m_Bans, ban)
			{
				const char *name = get_name_by_id(*ban);
				if (!name)	continue;
				
				ch->Send("   %-12s%s", name, (++count % 5) == 0 ? "\n" : "");
			}
		}
	}
	else if ((banned = get_id_by_name(arg)) == NoID)
		ch->Send("There is no player by that name.\n");
	else
	{
		//	Might be an applicant
		const char *victimName = get_name_by_id(banned, "");
		
		if (clan->IsBanned(banned))	
		{
			clan->RemoveBan(banned);
			
			ch->Send("You have unbanned %s from \"%s\"\n", victimName, clan->GetName());
		}
		else if (clan->m_Bans.size() >= MAX_CLAN_BANS)
			ch->Send("Your clan is limited to %d bans; unban someone first.\n", MAX_CLAN_BANS);
		else
		{
/*			if (clan->IsApplicant(banned))
			{
				victim = CharData::Find(banned);
				if (victim && IS_NPC(victim))
					victim = NULL;
				
				if (victim)
				{
					victim->Send("You have been denied entry into \"%s\", and have been banned from re-applying.\n", clan->GetName());
					GET_CLAN(victim) = NULL;
					victim->Save();
				}
				
				ch->Send("You have denied %s entry into the clan.\n", get_name_by_id(banned, ""));
				
				clan->RemoveApplicant(id);
			}
*/
			clan->AddBan(banned);
			
			ch->Send("You have banned %s from \"%s\"\n", victimName, clan->GetName());
		}
		Clan::Save();
	}
}


ACMD(do_deposit) {
//	BUFFER(arg,MAX_INPUT_LENGTH);
//	int		amount;
	
//	one_argument(argument, arg);
	
	Clan *clan = GET_CLAN(ch);
	
	if (IS_NPC(ch))
		ch->Send("Very funny.\n");
	else if (clan == NULL || Clan::GetMember(ch) == NULL)
		ch->Send("You aren't even in a clan!\n");
	else if (Clan::HasAnyPermission(ch, Clan::Permission_Bank_Mask))
		ch->Send("You do not have permission to view the bank.\n");
	else //if (!*arg || !is_number(arg))
		ch->Send("Your clan has %d `bMission Points`n in its savings.\n", clan->m_Savings);
/*	else if ((amount = atoi(arg)) <= 0)
		send_to_char("Deposit something more than nothing, please.\n", ch);
	else if (amount > GET_MISSION_POINTS(ch))
		send_to_char("You don't have that many `bMission Points`n!\n", ch);
	else {
		DescriptorData *d;
		START_ITER(iter, d, DescriptorData *, descriptor_list) {
			if (STATE(d) == CON_PLAYING && GET_CLAN(d->m_Character) == GET_CLAN(ch))
				d->m_Character->Send("`y[`bCLAN`y]`n %s has deposited %d `bMP`n into the clan savings.\n",
						ch->GetName(), amount);
		}
 
		GET_MISSION_POINTS(ch) -= amount;
		Clan::Index[clan]->m_Savings += amount;
		ch->Send("You deposit %d `bMission Points`n into clan savings.\n", amount);
		log("CLANS: %s donated %d `bMP`n to clan %s [%s]",
				ch->GetName(), amount, Clan::Index[clan]->GetName(), Clan::Index[clan]->GetVirtualID().Print().c_str());
		Clan::Save();
	}
*/
}


ACMD(do_withdraw) {
	BUFFER(arg, MAX_INPUT_LENGTH);
	unsigned int	amount;
	Clan *	clan;
	
	one_argument(argument, arg);
	
	clan = GET_CLAN(ch);
	
	if (IS_NPC(ch))
		ch->Send("Very funny.\n");
	else if (clan == NULL || Clan::GetMember(ch) == NULL)
		ch->Send("You aren't even in a clan!\n");
	else if (Clan::HasPermission(ch, Clan::Permission_Bank_Withdraw))
		ch->Send("You do not have permission to withdraw from the bank.\n");
	else if (!*arg || !is_number(arg))
		send_to_char("Withdraw how much?\n", ch);
	else if ((amount = atoi(arg)) <= 0)
		send_to_char("Withdraw something more than nothing, please.\n", ch);
	else if (amount > clan->m_Savings)
		send_to_char("You're clan doesn't have that many mission points!\n", ch);
	else {	
	    FOREACH(DescriptorList, descriptor_list, iter)
	    {
	    	DescriptorData *d = *iter;
			if (d->GetState()->IsPlaying() && GET_CLAN(d->m_Character) == GET_CLAN(ch))
				d->m_Character->Send("`y[`bCLAN`y]`n %s has withdrawn %d MP from the clan savings.\n",
						ch->GetName(), amount);
		}

		GET_MISSION_POINTS(ch) += amount;
		clan->m_Savings -= amount;
		ch->Send("You withdraw %d mission points from clan savings.\n", amount);
		mudlogf(NRM, LVL_STAFF, TRUE, "CLANS: %s withdraw %d from clan %s [%d]",
				ch->GetName(), amount, clan->GetName(), clan->GetID());

		Clan::Save();
	}
}



static void DisplayClanRelations(CharData *ch, Clan *clan)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	
	strcpy(buf, "    #  Clan                                          Status\n");
	strcat(buf, "--------------------------------------------------------------\n");
	//			 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 
	for (int i = 0; i < Clan::IndexSize; ++i)
	{
		Clan *otherClan = Clan::Index[i];
		
		if (otherClan == clan || otherClan->m_Members.empty())
			continue;
		
		bool	bWarDeclaredOn = clan->IsEnemy(otherClan);
		bool	bWarDeclaredBy = otherClan->IsEnemy(clan);
		bool	bAllianceRequestedWith = !bWarDeclaredOn && clan->IsAlly(otherClan);
		bool	bAllianceRequestedBy = !bWarDeclaredBy && otherClan->IsAlly(clan);
		
		const char *status = "`gERROR`n";
		
		if (bWarDeclaredOn)
		{
			if (bWarDeclaredBy)				status = "`rWAR`n";
			else							status = "`rWAR`n (They request Peace)";
		}
		else if (bWarDeclaredBy)			status = "`rWAR`n (You request Peace)";
		else if (bAllianceRequestedWith)
		{
			if (bAllianceRequestedBy)		status = "`gAllies`n";
			else							status = "`yNeutral`n (You request an Alliance)";
		}
		else if (bAllianceRequestedBy)		status = "`yNeutral`n (They request an Alliance)";
		else								status = "`yNeutral`n";
		
		sprintf_cat(buf, "  %3d. %-*s %s\n",
				otherClan->GetID(),
				cstrnlen_for_sprintf(otherClan->GetName(), 45),
				otherClan->GetName(),
				status);
	}
	page_string(ch->desc, buf);
}


ACMD(do_war)
{
	BUFFER(arg, MAX_INPUT_LENGTH);
	Clan *		clan;
	Clan *		enemyClan;
	
	argument = one_argument(argument, arg);
	
	clan = GET_CLAN(ch);
	if (!IS_NPC(ch) && !Clan::GetMember(ch))	clan = NULL;
	
	if (IS_STAFF(ch) && STF_FLAGGED(ch, STAFF_CLANS) && is_number(arg) && *argument)
	{
		clan = Clan::GetClan(atoi(arg));
		if (!clan)
		{
			ch->Send("Clan not found.\n");
			return;
		}
		
		argument = one_argument(argument, arg);
	}
	
	if (IS_NPC(ch))
		ch->Send("Very funny.\n");
	else if (clan == NULL)
		send_to_char("You aren't even in a clan!\n", ch);
	else if (!*arg)
		DisplayClanRelations(ch, clan);
	else if (!Clan::HasPermission(ch, Clan::Permission_War))
		send_to_char("You do not have permission to do that.\n", ch);
	else if (!is_number(arg) || !(enemyClan = Clan::GetClan(atoi(arg))))
		send_to_char("Declare war on who?\n", ch);
	else if (enemyClan == clan)
		send_to_char("Trying to start a civil war, eh?\n", ch);
	else
	{
		if (clan->IsEnemy(enemyClan))
		{
			ch->Send("You are no longer at war with %s`n.\n", enemyClan->GetName());
			
			clan->RemoveEnemy(enemyClan);

			sprintf(arg, "%s of %s has declared a cease fire on %s.`n", ch->GetName(),
					clan->GetName(), enemyClan->GetName());
		}
		else
		{
			ch->Send("You are now at war with %s`n.\n", enemyClan->GetName());

			if (clan->IsAlly(enemyClan))
				sprintf(arg, "%s of %s has broken its alliance with %s and declared war.`n", ch->GetName(),
						clan->GetName(), enemyClan->GetName());
			else
				sprintf(arg, "%s of %s has declared war on %s.`n", ch->GetName(),
						clan->GetName(), enemyClan->GetName());

			clan->RemoveAlly(enemyClan);
			enemyClan->RemoveAlly(clan);

			clan->AddEnemy(enemyClan);
		}
		
		announce(arg);

		Clan::RebuildRelations();
		Clan::Save();
	}
}


ACMD(do_ally)
{
	BUFFER(arg, MAX_INPUT_LENGTH);
	Clan *	clan;
	Clan *	otherClan;

	argument = one_argument(argument, arg);
	
	clan = GET_CLAN(ch);
	if (!IS_NPC(ch) && !Clan::GetMember(ch))	clan = NULL;
	
	if (IS_STAFF(ch) && STF_FLAGGED(ch, STAFF_CLANS) && is_number(arg) && *argument)
	{
		clan = Clan::GetClan(atoi(arg));
		if (!clan)
		{
			ch->Send("Clan not found.\n");
			return;
		}
		
		argument = one_argument(argument, arg);
	}
	
	if (IS_NPC(ch))
		ch->Send("Very funny.\n");
	else if (clan == NULL)
		ch->Send("You aren't even in a clan!\n");
	else if (!*arg)
		DisplayClanRelations(ch, clan);
	else if (!Clan::HasPermission(ch, Clan::Permission_Ally))
		ch->Send("You do not have permission to do that.\n");
	else if (!is_number(arg) || !(otherClan = Clan::GetClan(atoi(arg))))
		send_to_char("Ally with who?\n", ch);
	else if (otherClan == clan)
		send_to_char("Your clan really gets along that well, eh?\n", ch);
	else if (!clan->HasSharedFaction(otherClan))
		ch->Send("Your clans are too different from one another for that to be possible.\n");
	else
	{
		*arg = 0;
		
		if (clan->IsAlly(otherClan))
		{
			ch->Send("You break your alliance with %s`n.\n", otherClan->GetName());
			
			clan->RemoveAlly(otherClan);
			otherClan->RemoveAlly(clan);

			sprintf(arg, "%s of %s has broken its alliance with %s.`n", ch->GetName(),
					clan->GetName(), otherClan->GetName());
		}
		else if (clan->IsEnemy(otherClan))
			ch->Send("You are at war with %s; you must first declare a cease-fire.", otherClan->GetName());
		else if (otherClan->IsEnemy(clan))
			ch->Send("%s is at war with you; they must first declare a cease-fire.", otherClan->GetName());
		else
		{
			clan->AddAlly(otherClan);

			if (otherClan->IsAlly(clan))
			{
				ch->Send("You are now allied with %s`n.\n", otherClan->GetName());
				sprintf(arg, "%s of %s has allied with %s.`n", ch->GetName(),
						clan->GetName(), otherClan->GetName());
			}
			else
			{
				ch->Send("You have requested an alliance with %s`n.\n", otherClan->GetName());
				sprintf(arg, "%s of %s has requested an alliance with %s.`n", ch->GetName(),
						clan->GetName(), otherClan->GetName());
			}
		}
		
		if (*arg)
		{
			announce(arg);

			Clan::RebuildRelations();
			Clan::Save();
		}
	}
}


ACMD(do_clanpolitics)
{
	BUFFER(buf, MAX_STRING_LENGTH);
	
	const int MAX_TAG_WIDTH = 8;
	const int TAG_PADDING   = 4;
	const int COLUMN_WIDTH	= MAX_TAG_WIDTH + TAG_PADDING;
	const int NUM_COLUMNS	= (79 + TAG_PADDING) / COLUMN_WIDTH;
	const int NUM_CLANS_PER_COLUMN = NUM_COLUMNS - 1;
	
	int numPages = (Clan::IndexSize + NUM_CLANS_PER_COLUMN - 1) / NUM_CLANS_PER_COLUMN;
	
	for (int page = 0; page < numPages; ++page)
	{
		if (page > 0)	strcat(buf, "\n\n");
		
		//	Generate Header
		sprintf_cat(buf, "%*s", COLUMN_WIDTH, "");	//	Empty spot for left column
		
		for (int column = 0; column < NUM_CLANS_PER_COLUMN; ++column)
		{
			int columnClanNum = column + (page * NUM_CLANS_PER_COLUMN);
			if (columnClanNum >= Clan::IndexSize)	break;
			
			Clan *columnClan = Clan::Index[columnClanNum];
			
			int tagWidth = cstrnlen_for_sprintf(columnClan->GetTag(), MAX_TAG_WIDTH);
			sprintf_cat(buf, "%-*.*s`n%*s", tagWidth, tagWidth, columnClan->GetTag(), TAG_PADDING, "");
		}
		strcat(buf, "\n");
		
		//	Generate rows
		for (int row = 0; row < Clan::IndexSize; ++row)
		{
			Clan *rowClan = Clan::Index[row];
			
			int tagWidth = cstrnlen_for_sprintf(rowClan->GetTag(), MAX_TAG_WIDTH);
			sprintf_cat(buf, "%-*.*s`n%*s%s", tagWidth, tagWidth, rowClan->GetTag(), TAG_PADDING, "", /*row % 2 == 1 ? "`^w" :*/ "");
			
			for (int column = 0; column < NUM_CLANS_PER_COLUMN; ++column)
			{
				int columnClanNum = column + (page * NUM_CLANS_PER_COLUMN);
				if (columnClanNum >= Clan::IndexSize)	break;
				
				Clan *columnClan = Clan::Index[columnClanNum];
				
				const char *	relation = "";
				switch (rowClan->GetRelation(columnClan))
				{
					case RELATION_FRIEND:	relation = (rowClan != columnClan) ? "`gALLIED" : "";	break;
					case RELATION_ENEMY:	relation = "`rWAR";		break;
				}
				
				int relationWidth = cstrnlen_for_sprintf(relation, COLUMN_WIDTH);
				sprintf_cat(buf, "%-*s", relationWidth, relation);
			}
			
			strcat(buf, "`n\n");
		}
	}
	
	page_string(ch->desc, buf);
}


ACMD(do_clanrank)
{
	BUFFER(arg1, MAX_STRING_LENGTH);
	BUFFER(arg2, MAX_STRING_LENGTH);
	
	Clan *clan = GET_CLAN(ch);
	if (!IS_NPC(ch) && !Clan::GetMember(ch))	clan = NULL;
	
	if (IS_STAFF(ch) && STF_FLAGGED(ch, STAFF_CLANS) && is_number(argument))
	{
		argument = one_argument(argument, arg1);
		
		clan = Clan::GetClan(atoi(arg1));
		if (!clan)
		{
			ch->Send("Clan not found.\n");
			return;
		}
	}
	
	argument = one_argument(argument, arg1);
	
	if (IS_NPC(ch))
		ch->Send("Very funny.\n");
	else if (clan == NULL)
		ch->Send("You aren't even in a clan!\n");
	else if (!*arg1)
	{
		//	List of ranks
		int count = 0;
		ch->Send("%s ranks (from highest to lowest):\n", clan->GetName());
		FOREACH(Clan::RankList, clan->m_Ranks, iter)
		{
			Clan::Rank *rank = *iter;
			
			ch->Send("    %1.1d. %s%s\n", ++count, rank->GetName(),
				clan->m_pDefaultRank == rank ? "   (Default)" : "");
			if (Clan::HasPermission(ch, Clan::Permission_EditRanks))
			{
				ch->Send("        %s\n", rank->m_Privileges.print().c_str());				
			}
		}
	}
	else if (!Clan::HasPermission(ch, Clan::Permission_EditRanks))
		ch->Send("You do not have permission to edit ranks.\n");
	else
	{
		if (!str_cmp(arg1, "add"))
		{
			if (clan->m_Ranks.size() >= MAX_RANKS)
				ch->Send("Your clan already has the maximum number of ranks allowed.\n");
			else if (!*argument)
				ch->Send("Usage: clanrank add <name>\n");
			else if (cstrlen(argument) > MAX_RANK_NAME_LENGTH)
				ch->Send("Clan rank names must be %d letters or less in length, excluding color codes.\n", MAX_RANK_NAME_LENGTH);
			else if (clan->GetRank(argument))
				ch->Send("A rank with that name already exists.\n");
			else
			{
				Clan::Rank *rank = new Clan::Rank;
				
				rank->m_Name = argument;
				rank->m_Order = clan->m_Ranks.size();
				clan->m_Ranks.push_back(rank);
				
				ch->Send("Rank \"%s`n\" added.\n", rank->GetName());
				
				Clan::Save();
			}
		}
		else if (!str_cmp(arg1, "delete"))
		{
			int num = atoi(argument) - 1;
			
			//	Delete a rank
			if (clan->m_Ranks.size() <= 2)
				ch->Send("Your clan must have at least 2 ranks.\n");
			else if (!*argument || !is_number(argument))
				ch->Send("Usage: clanrank delete <number>\n");
			else if (num <= 0 || num >= clan->m_Ranks.size())
				ch->Send("That is not a valid rank number.\n");
			else
			{
				Clan::Rank *rank = clan->GetRank(num);
				clan->m_Ranks.remove(rank);
				
				FOREACH(Clan::RankList, clan->m_Ranks, rankIter)
				{
					if ((*rankIter)->m_Order > rank->m_Order)
						(*rankIter)->m_Order -= 1;
				}
				
				ch->Send("Rank \"%s`n\" deleted.\n", rank->GetName());
				
				delete rank;
				
				if (clan->m_pDefaultRank == rank)
				{
					clan->m_pDefaultRank = clan->m_Ranks.back();
					ch->Send("`rWARNING`n: The default rank has been changed to %s.\n", clan->m_pDefaultRank->GetName());
				}
				
				FOREACH(Clan::MemberList, clan->m_Members, member)
				{
					if ((*member)->m_pRank == rank)
					{
						ch->Send("`rWARNING`n: %s's rank has been changed to %s.\n", (*member)->GetName(), clan->m_pDefaultRank->GetName());
						(*member)->m_pRank = clan->m_pDefaultRank;

						if ((*member)->m_Character)
							(*member)->m_Character->Send("The clan rank you hold has been deleted, and you have been defaulted to the rank of %s.\n", clan->m_pDefaultRank->GetName());
					}
				}
				
				Clan::Save();
			}
		}
		else if (!str_cmp(arg1, "rename"))
		{
			argument = one_argument(argument, arg1);
			int num = atoi(arg1) - 1;
			
			//	Rename a rank
			if (!*arg1 || !*argument)
				ch->Send("Usage: clanrank rename <number> <name>\n");
			else if (!is_number(arg1) || num < 0 || num >= clan->m_Ranks.size())
				ch->Send("That is not a valid rank number.\n");
			else if (cstrlen(argument) > MAX_RANK_NAME_LENGTH)
				ch->Send("Clan rank names must be %d letters or less in length, excluding color codes.\n", MAX_RANK_NAME_LENGTH);
			else if (clan->GetRank(argument))
				ch->Send("A rank with that name already exists.\n");
			else
			{
				Clan::Rank *rank = clan->GetRank(num);
				
				ch->Send("You have renamed rank %s`n to %s`n.", rank->GetName(), argument);

				rank->m_Name = argument;

				FOREACH(Clan::MemberList, clan->m_Members, member)
				{
					if ((*member)->m_pRank == rank && (*member)->m_Character)
						(*member)->m_Character->Send("Your clan rank has been renamed to %s.\n", rank->GetName());
				}

				Clan::Save();
			}
		}
		else if (!str_cmp(arg1, "swap"))
		{
			two_arguments(argument, arg1, arg2);
			
			int num1 = atoi(arg1) - 1;
			int num2 = atoi(arg2) - 1;
			
			//	Delete a rank
			if (!*arg1 || !*arg2 || !is_number(arg1) || !is_number(arg2))
				ch->Send("Usage: clanrank swap <first> <second>\n");
			else if (clan->m_Ranks.size() <= 2)
				ch->Send("Your clan needs more than 2 ranks before you can re-order them.\n");
			else if (num1 <= 0 || num2 <= 0 || num1 >= clan->m_Ranks.size() || num2 >= clan->m_Ranks.size())
				ch->Send("You may only move around ranks between 2 and %d.\n", clan->m_Ranks.size());
			else
			{				
				Clan::Rank *rank1 = clan->GetRank(num1);
				Clan::Rank *rank2 = clan->GetRank(num2);
				
				if (rank1 == rank2)
					ch->Send("Same rank, not swapped.");
				else
				{
					std::swap(rank1->m_Order, rank2->m_Order);
					
					clan->SortRanks();
					Clan::Save();
				}
				
				ch->Send("You have swapped the order of ranks %s`n and %s`n.", rank1->GetName(), rank2->GetName());
			}
		}
		else if (!str_cmp(arg1, "setprivs"))
		{
			argument = one_argument(argument, arg1);
			unsigned int num = atoi(arg1) - 1;
			
			//	Delete a rank
			if (!is_number(arg1))
				ch->Send("Usage: clanrank setprivs <number> <privs>\n");
			else if (num >= clan->m_Ranks.size())
				ch->Send("That is not a valid rank number.\n");
			else if (num == 0)
				ch->Send("The leader rank cannot be altered.\n");
			else if (!*argument)
			{
				Clan::RankList::iterator iter = clan->m_Ranks.begin();
				std::advance(iter, num);
				
				Clan::Rank *rank = *iter;
				
				ch->Send("Rank \"%s\" has the following privileges:\n"
				         "Key: `gGRANTED`n\n\n", rank->GetName());

				for (int i = 0; i < Clan::NumPermissions; ++i)
				{
					const char *name = GetEnumName<Clan::RankPermissions>(i);
					if (!name)	continue;
					ch->Send("    %s%s`n\n",
						rank->m_Privileges.test((Clan::RankPermissions)i) ? "`g" : "`K",
						name);
				}
			}
			else
			{
				Clan::RankList::iterator iter = clan->m_Ranks.begin();
				std::advance(iter, num);
				
				Clan::Rank *rank = *iter;
				
				while (*argument)
				{
					argument = one_argument(argument, arg1);
					
					int index = GetEnumByNameAbbrev<Clan::RankPermissions>(arg1);
					if (index == -1)
						ch->Send("'%s' is not a valid permission.\n", arg1);
					else
					{
						rank->m_Privileges.flip((Clan::RankPermissions)index);
						ch->Send("Rank \"%s\" has had the '%s' permission turned %s.\n",
							rank->GetName(), GetEnumName<Clan::RankPermissions>(index),
							ONOFF(rank->m_Privileges.test((Clan::RankPermissions)index)));
					}
				}
			}
		}
		else
			ch->Send("Unknown command.  Commands are:\n"
					 "add, delete, rename, swap, setprivs\n");
	}
}


ACMD(do_clanallow)
{
	BUFFER(arg1, MAX_STRING_LENGTH);
	Clan::Member *	member;
	
	Clan *clan = GET_CLAN(ch);
	if (!IS_NPC(ch) && !Clan::GetMember(ch))	clan = NULL;
	
	if (IS_STAFF(ch) && STF_FLAGGED(ch, STAFF_CLANS) && is_number(argument))
	{
		argument = one_argument(argument, arg1);
		
		clan = Clan::GetClan(atoi(arg1));
		if (!clan)
		{
			ch->Send("Clan not found.\n");
			return;
		}
	}
	
	argument = one_argument(argument, arg1);
	
	if (IS_NPC(ch))
		ch->Send("Very funny.\n");
	else if (clan == NULL)
		ch->Send("You aren't even in a clan!\n");
	else if (!*arg1)
	{
		ch->Send("Usage: %s <member> [<permission>]\n"
				 "If you do not specify the permissions you will be able to see all permissions allowed to that member.\n"
				 "\n"
				 "The following permissions are available to you:\n",
				 subcmd == SCMD_ALLOW ? "callow" : "cdeny");
		
		member = Clan::GetMember(ch);
		if (clan == GET_CLAN(ch) && member)
		{
			for (int i = 0; i < Clan::NumPermissions; ++i)
			{
				bool personalPrivilege = member->m_Privileges.test((Clan::RankPermissions)i);
				bool rankPrivilege = member->m_pRank->m_Order == 0 || member->m_pRank->m_Privileges.test((Clan::RankPermissions)i);
				const char *name = GetEnumName<Clan::RankPermissions>(i);
				
				if (!name || (!personalPrivilege && !rankPrivilege))	continue;
				ch->Send("    %s%s`n\n",
					personalPrivilege ? "`r" : (rankPrivilege ? "`g" : "`K"),
					name);
			}
		}
	}
	else if (!Clan::HasPermission(ch, Clan::Permission_Allow))
		ch->Send("You do not have permission to do that.\n");
	else if (!(member = clan->GetMember(arg1)))
		ch->Send("No such member.\n");
	else if (!*argument)
	{
/*		ch->Send("%s has the following permissions:\n"
				 "     %s\n"
				 "\n"
				 "The following permissions are available:\n",
				 member->GetName(), member->m_Privileges.print().c_str());

		for (int i = 0; i < Clan::NumPermissions; ++i)
		{
			const char *name = GetEnumName<Clan::RankPermissions>(i);
			if (name)	ch->Send("    %s\n", name);
		}
*/

		ch->Send("%s has the following additional personal permissions:\n"
		         "Key: `RALLOWED`n     `GRANK`n     `KNOT AVAILABLE`n\n\n", member->GetName());

		for (int i = 0; i < Clan::NumPermissions; ++i)
		{
			bool personalPrivilege = member->m_Privileges.test((Clan::RankPermissions)i);
			bool rankPrivilege = member->m_pRank->m_Order == 0 || member->m_pRank->m_Privileges.test((Clan::RankPermissions)i);
			
			const char *name = GetEnumName<Clan::RankPermissions>(i);
			if (!name)	continue;
			ch->Send("    %s%s`n\n",
				personalPrivilege ? "`r" : rankPrivilege ? "`g" : "`K",
				name);
		}
	}
	else
	{
		while (*argument)
		{
			argument = one_argument(argument, arg1);
			
			int index = GetEnumByNameAbbrev<Clan::RankPermissions>(arg1);
			if (index == -1)
				ch->Send("'%s' is not a valid permission.\n", arg1);
			else if (subcmd == SCMD_ALLOW && member->m_Privileges.test((Clan::RankPermissions)index))
				ch->Send("%s is already allowed %s.", member->GetName(), GetEnumName<Clan::RankPermissions>(index));
			else if (subcmd == SCMD_DENY && !member->m_Privileges.test((Clan::RankPermissions)index))
				ch->Send("%s is already allowed %s.", member->GetName(), GetEnumName<Clan::RankPermissions>(index));
			else
			{
				member->m_Privileges.set((Clan::RankPermissions)index, subcmd == SCMD_ALLOW);
				ch->Send("%s has been %s %s.\n", member->GetName(), subcmd == SCMD_ALLOW ? "allowed" : "denied", GetEnumName<Clan::RankPermissions>(index));
			}
		}
	}
}


ACMD(do_clanset)
{
	BUFFER(arg1, MAX_STRING_LENGTH);
	BUFFER(arg2, MAX_STRING_LENGTH);
	bool	bStaffOverride = false;
	
	argument = one_argument(argument, arg1);
	
	Clan *clan = GET_CLAN(ch);
	if (!IS_NPC(ch) && !Clan::GetMember(ch))	clan = NULL;
	
	if (IS_STAFF(ch) && STF_FLAGGED(ch, STAFF_CLANS) && is_number(arg1))
	{
		bStaffOverride = true;
		
		clan = Clan::GetClan(atoi(arg1));
		if (!clan)
		{
			ch->Send("Clan not found.\n");
			return;
		}
		
		argument = one_argument(argument, arg1);
		
		if (!str_cmp(arg1, "delete"))
		{
			Lexi::String code = Lexi::Format("%p", clan);
			
			one_argument(argument, arg1);
			
			if (!clan->m_Members.empty())
				ch->Send("You must boot every member of a clan before it can be deleted.\n");
			else if (code != arg1)
				ch->Send("To delete the clan, type: clanset %d delete %s\n", clan->GetID(), code.c_str());
			else
			{
				for (unsigned rnum = clan->m_RealNumber + 1; rnum < Clan::IndexSize; ++rnum)
				{
					--Clan::Index[rnum]->m_RealNumber;
				}
				
				std::copy(Clan::Index + clan->m_RealNumber + 1,	Clan::Index + Clan::IndexSize,	// Copy Range
					Clan::Index + clan->m_RealNumber);
				--Clan::IndexSize;
				
				ch->Send("Clan '%s'`n (%d) deleted.\n", clan->GetName(), clan->GetID());
				mudlogf(NRM, ch->GetPlayer()->m_StaffInvisLevel, true, "OLC: %s deletes clan '%s'`g (%d)", ch->GetName(), clan->GetName(), clan->GetID());
					
				delete clan;
				
				Clan::RebuildRelations();
				Clan::Save();
			}
			
			return;
		}
	}
	
	if (IS_NPC(ch))
		ch->Send("Very funny.\n");
	else if (clan == NULL)
		ch->Send("You aren't even in a clan!\n");
//	else if ()
//		ch->Send("You do not have permission to change clan settings.\n");
	else if (is_abbrev(arg1, "inactivity") && Clan::HasPermission(ch, Clan::Permission_EditRanks) /*&& GET_CLAN(ch) == clan*/)
	{
		unsigned int numDays = atoi(argument);
		
		if (!*argument)
			ch->Send("Usage: clanset inactivity <days>\n");
		else if (numDays > 120)
			ch->Send("Inactivity threshold must be between 0 (off) and 120 days.\n");
		else
		{
			if (numDays == 0)	ch->Send("Inactivity threshold disabled (was %d).\n", clan->m_InactivityThreshold);
			else				ch->Send("Inactivity threshold set to %d days (was %d).\n", numDays, clan->m_InactivityThreshold);
			clan->m_InactivityThreshold = numDays;	
			Clan::Save();
		}
	}
	else if (is_abbrev(arg1, "showmotd") && Clan::HasPermission(ch, Clan::Permission_EditMOTD) /*&& GET_CLAN(ch) == clan*/)
	{
		unsigned int motdOption = GetEnumByName<Clan::ShowMOTD>(argument);
		
		if (!*argument || motdOption == -1)
		{
			ch->Send("Usage: clanset showmotd <when>\n"
					 "Options are: ");
			for (int i = 0; i < Clan::NumShowMOTDOptions; ++i)
				ch->Send("%s ", GetEnumName((Clan::ShowMOTD)i));
			ch->Send("\n");
		}
		else
		{
			ch->Send("ShowMOTD changed to %s (was %s).\n", GetEnumName((Clan::ShowMOTD)motdOption), GetEnumName(clan->m_ShowMOTD));
			clan->m_ShowMOTD = (Clan::ShowMOTD)motdOption;	
			Clan::Save();
		}
	}
	else if (is_abbrev(arg1, "applications") && Clan::HasPermission(ch, Clan::Permission_EditEnlist) /*&& GET_CLAN(ch) == clan*/)
	{
		argument = one_argument(argument, arg1);
		
		if (!str_cmp(arg1, "deny"))
		{
			clan->m_ApplicationMode = Clan::Applications_Deny;
			ch->Send("Applications will not be automatically denied.\n");
		}
		else if (!str_cmp(arg1, "allow"))
		{
			clan->m_ApplicationMode = Clan::Applications_Allow;
			ch->Send("Applications will now be accepted, but must be manually approved.\n");
		}
		else if (!str_cmp(arg1, "auto"))
		{
			clan->m_ApplicationMode = Clan::Applications_AutoAccept;
			ch->Send("Applications will now be accepted, and may be automatically approved.\n");
		}
		else if (!str_cmp(arg1, "setreq"))
		{
			two_arguments(argument, arg1, arg2);
			
			if (!str_cmp(arg1, "minlevel"))
			{
				int newMinLevel = RANGE(atoi(arg2), 0, 100);
				if (newMinLevel == clan->m_AutoEnlistMinLevel)
					ch->Send("Auto-enlist minimum level requirement unchanged.");
				else
				{
					if (newMinLevel == 0)	ch->Send("Auto-enlist minimum level requirement disabled (was %d).", clan->m_AutoEnlistMinLevel);
					else 					ch->Send("Auto-enlist minimum level requirement set to %d (was %d).", newMinLevel, clan->m_AutoEnlistMinLevel);
					
					clan->m_AutoEnlistMinLevel = newMinLevel;
				}
			}
			else if (!str_cmp(arg1, "minpks"))
			{
				int newMinPKs = MAX(0, atoi(arg2));
				if (newMinPKs == clan->m_AutoEnlistMinPKs)
					ch->Send("Auto-enlist minimum PK requirement unchanged.");
				else
				{
					if (newMinPKs == 0)		ch->Send("Auto-enlist minimum PK requirement disabled (was %d).", clan->m_AutoEnlistMinPKs);
					else 					ch->Send("Auto-enlist minimum PK requirement set to %d (was %d).", newMinPKs, clan->m_AutoEnlistMinPKs);
					
					clan->m_AutoEnlistMinPKs = newMinPKs;
				}
			}
			else if (!str_cmp(arg1, "mindaysage"))
			{
				int newMinDays = MAX(0, atoi(arg2));
				if (newMinDays == clan->m_AutoEnlistMinDaysSinceCreation)
					ch->Send("Auto-enlist minimum days since creation requirement unchanged.");
				else
				{
					if (newMinDays == 0)	ch->Send("Auto-enlist minimum days since creation requirement disabled (was %d).", clan->m_AutoEnlistMinDaysSinceCreation);
					else 					ch->Send("Auto-enlist minimum days since creation requirement set to %d (was %d).", newMinDays, clan->m_AutoEnlistMinDaysSinceCreation);
					
					clan->m_AutoEnlistMinDaysSinceCreation = newMinDays;
				}
			}
			else if (!str_cmp(arg1, "minhoursplayed"))
			{
				int newMinHours = MAX(0, atoi(arg2));
				if (newMinHours == clan->m_AutoEnlistMinHoursPlayed)
					ch->Send("Auto-enlist minimum hours time played requirement unchanged.");
				else
				{
					if (newMinHours == 0)	ch->Send("Auto-enlist minimum hours time played requirement disabled (was %d).", clan->m_AutoEnlistMinHoursPlayed);
					else 					ch->Send("Auto-enlist minimum hours time played requirement set to %d (was %d).", newMinHours, clan->m_AutoEnlistMinHoursPlayed);
					
					clan->m_AutoEnlistMinHoursPlayed = newMinHours;
				}
			}
			else if (!str_cmp(arg1, "minpkratio"))
			{
				float newMinPKRatio = RANGE(atof(arg2), 0.0f, 100.0f);
				if (newMinPKRatio == clan->m_AutoEnlistMinPKPDRatio)
					ch->Send("Auto-enlist minimum PK ratio requirement unchanged.");
				else
				{
					if (newMinPKRatio == 0)		ch->Send("Auto-enlist minimum PK ratio requirement disabled (was %.2f).", clan->m_AutoEnlistMinPKPDRatio);
					else 						ch->Send("Auto-enlist minimum PK ratio requirement set to %.2f (was %.2f).", newMinPKRatio, clan->m_AutoEnlistMinPKPDRatio);
					
					clan->m_AutoEnlistMinPKPDRatio = newMinPKRatio;
				}
			}
			else if (!str_cmp(arg1, "minpkmkratio"))
			{
				float newMinPKRatio = RANGE(atof(arg2), 0.0f, 100.0f);
				if (newMinPKRatio == clan->m_AutoEnlistMinPKMKRatio)
					ch->Send("Auto-enlist minimum PK:MK ratio requirement unchanged.");
				else
				{
					if (newMinPKRatio == 0)		ch->Send("Auto-enlist minimum PK:MK ratio requirement disabled (was %.2f).", clan->m_AutoEnlistMinPKMKRatio);
					else 						ch->Send("Auto-enlist minimum PK:MK ratio requirement set to %.2f (was %.2f).", newMinPKRatio, clan->m_AutoEnlistMinPKMKRatio);
					
					clan->m_AutoEnlistMinPKMKRatio = newMinPKRatio;
				}
			}
			else
			{
				ch->Send("Usage:\n"
						 "clanset applications setreq [ minlevel | minpks | mindaysage | minhoursplayed | minpkratio | minpkmkratio ] \n");
			}
		}
		else
		{
			ch->Send("Usage:\n"
					 "clanset applications [ deny | allow | auto | setreq ]\n");
		}
	}
	else
	{
		ch->Send("Current clan settings:\n"
				 "\n"
				 "Inactivity  : %d days (0 = off).\n"
				 "ShowMOTD    : %s\n"
				 "Applications: %s\n",
				 clan->m_InactivityThreshold,
				 GetEnumName(clan->m_ShowMOTD),
				 GetEnumName(clan->m_ApplicationMode));
		if (clan->m_ApplicationMode == Clan::Applications_AutoAccept)
		{
			ch->Send("     Min Level   : %-5d          Min PKs     : %d\n"
					 "     Days (Age)  : %-5d          Hours Played: %d\n"
					 "     PK:PD Ratio : %-5.2f        PK:MK Ratio : %.2f\n",
					 clan->m_AutoEnlistMinLevel, clan->m_AutoEnlistMinPKs,
					 clan->m_AutoEnlistMinDaysSinceCreation, clan->m_AutoEnlistMinHoursPlayed,
					 clan->m_AutoEnlistMinPKPDRatio, clan->m_AutoEnlistMinPKMKRatio);
		}
	}
}



class EditCMOTDConnectionState : public PlayingConnectionState
{
public:
	EditCMOTDConnectionState(Clan *clan)
	:	m_Clan(clan)
	{
	}
	
	virtual void		Enter()
	{
		CharData *ch = GetDesc()->m_Character;
		
		act("$n starts editing $s clan's MOTD.", TRUE, ch, 0, 0, TO_ROOM);
		
		ch->Send("Edit your clan's MOTD.  (/s saves /h for help)\n");
		ch->Send("%s", m_Clan->m_MOTD.c_str());
		GetDesc()->StartStringEditor(m_Clan->m_MOTD, MAX_MESSAGE_LENGTH);
		SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
	}

	virtual void		Exit()
	{
		CharData *ch = GetDesc()->m_Character;
		
		if (ch)
		{
			REMOVE_BIT(PLR_FLAGS(ch), PLR_WRITING);
			act("$n finishes editing $t clan's MOTD.", TRUE, ch, (ObjData *)m_Clan->GetName(), 0, TO_ROOM);
		}
	}

	virtual void		OnEditorSave() 
	{
		CharData *ch = GetDesc()->m_Character;
		
		GetDesc()->Write("Clan MOTD Saved.\n");
		
		if (ch && Clan::GetMember(ch))
		{
			Clan::GetMember(ch)->m_LastLogin = time(0) + 1;
		}
		
		m_Clan->m_MOTDChangedOn = time(0);
		Clan::Save();
	}
	virtual void		OnEditorAbort() { GetDesc()->Write("Edit aborted.\n"); }
	virtual void		OnEditorFinished() { Pop(); }

private:
	Clan *				m_Clan;
};


ACMD(do_cmotd)
{
	BUFFER(arg, MAX_STRING_LENGTH);
	
	argument = one_argument(argument, arg);
	
	Clan *clan = GET_CLAN(ch);
	if (!IS_NPC(ch) && !Clan::GetMember(ch))	clan = NULL;
	
	if (IS_STAFF(ch) && STF_FLAGGED(ch, STAFF_CLANS) && is_number(arg))
	{
		clan = Clan::GetClan(atoi(arg));
		
		argument = one_argument(argument, arg);
	
		if (!clan)
		{
			ch->Send("No such clan.\n");
		}
		else if (!str_cmp(arg, "edit"))
		{
			/* perform check for mesg in creation */
		    FOREACH(DescriptorList, descriptor_list, desc)
		    {
		    	DescriptorData *d = *desc;
				if (d->GetState()->IsPlaying()
					&& d->m_StringEditor
					&& (d->m_StringEditor->getTarget() == &clan->m_MOTD))
				{
					ch->Send("The MOTD is already being edited by %s.\n", d->m_Character->GetName());
					return;
				}
			}

			ch->desc->GetState()->Push(new EditCMOTDConnectionState(clan));
		}
		else if (clan->m_MOTD.empty())
		{
			ch->Send("That clan does not have a MOTD.\n");
		}
		else
		{
			page_string(ch->desc, clan->m_MOTD.c_str());
		}
		return;
	}
	
	if (IS_NPC(ch))
		ch->Send("Very funny.\n");
	else if (clan == NULL)
		ch->Send("You aren't even in a clan!\n");
	else if (!str_cmp(arg, "edit") && Clan::HasPermission(ch, Clan::Permission_EditMOTD))
	{
		/* perform check for mesg in creation */
	    FOREACH(DescriptorList, descriptor_list, desc)
	    {
	    	DescriptorData *d = *desc;
			if (d->GetState()->IsPlaying()
				&& d->m_StringEditor
				&& (d->m_StringEditor->getTarget() == &clan->m_MOTD))
			{
				ch->Send("The MOTD is already being edited by %s.\n", d->m_Character->GetName());
				return;
			}
		}

		ch->desc->GetState()->Push(new EditCMOTDConnectionState(clan));
	}
	else if (clan->m_MOTD.empty())
	{
		ch->Send("Your clan does not have a MOTD.\n");
	}
	else
	{
		page_string(ch->desc, clan->m_MOTD.c_str());
	}
}



#define USE_LEXI_FILE_EXPORTER 1

void Clan::Save(void)
{
	Lexi::OutputParserFile	file(CLAN_FILE ".new");

	if (file.fail())
	{
		mudlogf(NRM, LVL_STAFF, TRUE, "SYSERR: Cannot write clans to disk: unable to open temporary file");
		return;
	}
	
	for (unsigned int num = 0; num < Clan::IndexSize; ++num) {
		Clan *clan = Clan::Index[num];


		file.BeginParser(Lexi::Format("#%d", clan->GetID()));
		
		file.WriteString("Name", clan->GetName());
		file.WriteString("Tag", clan->GetTag());
		
		file.WriteFlags("Races", clan->m_Races, race_types);
		
		const char *	leaderName = get_name_by_id(clan->m_Leader);
		if (leaderName)
		{
			file.WriteString("Leader", Lexi::Format("%u %s", clan->m_Leader, leaderName).c_str());
		}
		
		file.WriteInteger("InactivityThreshold", clan->m_InactivityThreshold, 0);
		if (world.Find(clan->m_Room))	file.WriteVirtualID("Room", clan->m_Room);
		file.WriteInteger("Savings", clan->m_Savings, 0);
		
		file.WriteInteger("MOTDChangedOn", clan->m_MOTDChangedOn);
		file.WriteEnum("ShowMOTD", clan->m_ShowMOTD, ShowMOTD_Always);
		file.WriteLongString("MOTD", clan->m_MOTD);
		
		file.WriteEnum("Applications", clan->m_ApplicationMode, Applications_Allow);
		file.BeginSection("AutoEnlist");
		{
			file.WriteInteger("MinLevel", clan->m_AutoEnlistMinLevel, 0);
			file.WriteInteger("MinPKs", clan->m_AutoEnlistMinPKs, 0);
			file.WriteInteger("MinDays", clan->m_AutoEnlistMinDaysSinceCreation, 0);
			file.WriteInteger("MinHours", clan->m_AutoEnlistMinHoursPlayed, 0);
			file.WriteFloat("MinPKPDRatio", clan->m_AutoEnlistMinPKPDRatio, 0);
			file.WriteFloat("MinPKMKRatio", clan->m_AutoEnlistMinPKMKRatio, 0);
		}
		file.EndSection();

		file.BeginSection("Ranks");
		{
			if (clan->m_pDefaultRank)	file.WriteString("Default", clan->m_pDefaultRank->GetName());
		
			int i = 0;
			FOREACH(RankList, clan->m_Ranks, iter)
			{
				Rank *			rank = *iter;
				
				file.BeginSection(Lexi::Format("Rank %d", ++i));
				{
					file.WriteString("Name", rank->GetName());
					file.WriteInteger("Order", rank->m_Order);
					file.WriteFlags("Privileges", rank->m_Privileges);
				}
				file.EndSection();
			}
		}
		file.EndSection();
		
		file.BeginSection("Members");
		{
			file.WriteInteger("Max", clan->m_MaxMembers, 0);
			
			int i = 0;
			FOREACH(MemberList, clan->m_Members, iter)
			{
				Member *		member = *iter;
				
				file.BeginSection(Lexi::Format("Member %d", ++i));
				{
					file.WriteInteger("ID", member->m_ID);
					file.WriteString("Name", member->GetName());
					file.WriteString("Rank", member->m_pRank->GetName());
					file.WriteFlags("Privileges", member->m_Privileges);
					file.WriteDate("LastOn", member->m_LastLogin);
				}
				file.EndSection();
			}
		}
		file.EndSection();
		
		file.BeginSection("Enemies");
		{
			int i = 0;
			FOREACH(EnemyList, clan->m_Enemies, enemy)
			{
				if (Clan::GetClan(*enemy))
					file.WriteInteger(Lexi::Format("Enemy %d", ++i).c_str(), *enemy);
			}
		}
		file.EndSection();
		
		file.BeginSection("Allies");
		{
			int i = 0;
			FOREACH(AllyList, clan->m_Allies, ally)
			{
				if (Clan::GetClan(*ally))
					file.WriteInteger(Lexi::Format("Ally %d", ++i).c_str(), *ally);
			}
		}
		file.EndSection();
		
		file.BeginSection("Bans");
		{
			int i = 0;
			FOREACH(Clan::BanList, clan->m_Bans, ban)
			{
				const char *banName = get_name_by_id(*ban);
				if (banName)	file.WriteString(Lexi::Format("Ban %d", ++i).c_str(), Lexi::Format("%u %s", *ban, banName));
			}			
		}
		file.EndSection();
		
		file.EndParser();
	}
	
	file.close();

	//	We're fubar'd if we crash between the two lines below.
	remove(CLAN_FILE);
	if (rename(CLAN_FILE ".new", CLAN_FILE))
	{
		mudlogf(NRM, LVL_STAFF, TRUE, "SYSERR: Cannot write clans to disk: unable to replace file with temporary");
		return;
	}
	
	ms_SaveClans = false;
}



void Clan::Load(void) {
	const char *filename = CLAN_FILE;
	FILE *fl;
	int	clan_count, nr = -1, last = 0;
	
	
	if (!(fl = fopen(filename, "r"))) {
		log("Clan file %s not found.", filename);
		tar_restore_file(filename);
		exit(1);
	}
	
	clan_count = count_hash_records(fl) + 1;
	fclose(fl);
	
	Clan::Index = new Clan *[clan_count];
	
	Lexi::BufferedFile	file(filename);
	
	if (file.bad())
	{
		log("Clan file is corrupt.");
		tar_restore_file(filename);
	}
	
	while (!file.eof())
	{
		const char *	line = file.getline();
		
		if (*line == '#')
		{
			last = nr;
			if (sscanf(line, "#%d", &nr) != 1) {
				log("Format error after clan #%d (%s:%d)", last, file.GetFilename(), file.GetLineNumber());
				tar_restore_file(filename);
			}
			
			Clan::Parse(file, nr);
		}
		else if (!str_cmp(line, "END"))	//	TODO: REMOVE
			break;						//	TODO: REMOVE
		else
		{
			log("Format error after clan #%d (%s:%d)", nr, file.GetFilename(), file.GetLineNumber());
			log("Offending line: '%s'", line);
			tar_restore_file(filename);
		}
	}
	
	Clan::RebuildRelations();
}


void Clan::Parse(Lexi::BufferedFile & clan_f, IDNum id)
{
	Clan *clan = new Clan(id);
	Clan::Index[Clan::IndexSize] = clan;
	clan->m_RealNumber = Clan::IndexSize++;
	
	Lexi::Parser	parser;
	
	parser.Parse(clan_f);
	
	clan->m_Name				= parser.GetString("Name");
	clan->m_Tag					= parser.GetString("Tag");
	clan->m_Races				= parser.GetFlags("Races", race_types);
	clan->m_Leader				= parser.GetInteger("Leader");
	clan->m_Room				= parser.GetVirtualID("Room");
	clan->m_InactivityThreshold	= parser.GetInteger("InactivityThreshold");
	clan->m_Savings				= parser.GetInteger("Savings");
	clan->m_MOTDChangedOn		= parser.GetInteger("MOTDChangedOn");
	clan->m_ShowMOTD			= parser.GetEnum("ShowMOTD", ShowMOTD_Always);
	clan->m_MOTD				= parser.GetString("MOTD");
	clan->m_ApplicationMode		= parser.GetEnum("Applications", Applications_Allow);

	{
		PARSER_SET_ROOT(parser, "AutoEnlist");
		
		clan->m_AutoEnlistMinLevel		= parser.GetInteger("MinLevel");
		clan->m_AutoEnlistMinPKs		= parser.GetInteger("MinPKs");
		clan->m_AutoEnlistMinDaysSinceCreation	= parser.GetInteger("MinDays");
		clan->m_AutoEnlistMinHoursPlayed		= parser.GetInteger("MinHours");
		clan->m_AutoEnlistMinPKPDRatio	= parser.GetFloat("MinPKPDRatio");
		clan->m_AutoEnlistMinPKMKRatio	= parser.GetFloat("MinPKMKRatio");
	}


	int numRanks = parser.NumSections("Ranks");
	for (int i = 0; i < numRanks; ++i)
	{
//		parser.SetCurrentRootIndexed("Ranks", i);
		PARSER_SET_ROOT_INDEXED(parser, "Ranks", i);
		
		Clan::Rank *	rank = new Clan::Rank;
		
		rank->m_Name = parser.GetString("Name");
		rank->m_Order = parser.GetInteger("Order");
		rank->m_Privileges = parser.GetBitFlags<Clan::RankPermissions>("Privileges");
		
		clan->m_Ranks.push_back(rank);
		
//		parser.ResetCurrentRoot();
	}
	
	clan->SortRanks();
	
	if (clan->m_Ranks.empty())
		clan->CreateDefaultRanks();
	
	clan->m_pDefaultRank = clan->GetRank(parser.GetString("Ranks/Default", "Member"));
	
	if (!clan->m_pDefaultRank)	//	Fallback
	{
		clan->m_pDefaultRank = clan->m_Ranks.front();
		
		//	Find the rank with the lowest order...
		FOREACH(RankList, clan->m_Ranks, iter)
		{
			if ((*iter)->m_Order < clan->m_pDefaultRank->m_Order)
				clan->m_pDefaultRank = *iter;
		}
	}
	
	clan->m_MaxMembers = parser.GetInteger("Members/Max", 0);
			
	int numMembers = parser.NumSections("Members");
	for (int i = 0; i < numMembers; ++i)
	{
//		parser.SetCurrentRootIndexed("Members", i);
		PARSER_SET_ROOT_INDEXED(parser, "Members", i);
	
		IDNum id = parser.GetInteger("ID");
		
		Clan::Member *	member = new Clan::Member(id);
		
		if (!get_name_by_id(member->m_ID))	// Make sure the character still exists
		{
			delete member;
			continue;
		}

		member->m_Name = parser.GetString("Name");
		member->m_pRank = clan->GetRank(parser.GetString("Rank"));
		member->m_Privileges = parser.GetBitFlags<Clan::RankPermissions>("Privileges");
		member->m_LastLogin = parser.GetInteger("LastOn");
		
		if (!member->m_pRank)
			member->m_pRank = clan->m_pDefaultRank;
		
		clan->m_Members.push_back(member);
		
//		parser.ResetCurrentRoot();
	}
	
	int numEnemies = parser.NumFields("Enemies");
	for (int i = 0; i < numEnemies; ++i)
	{
		clan->m_Enemies.push_back(parser.GetIndexedInteger("Enemies", i));
	}
	
	int numAllies = parser.NumFields("Allies");
	for (int i = 0; i < numAllies; ++i)
	{
		clan->m_Allies.push_back(parser.GetIndexedInteger("Allies", i));
	}
	
	int numBans = parser.NumFields("Bans");
	for (int i = 0; i < numBans; ++i)
	{
		clan->m_Bans.push_back(parser.GetIndexedInteger("Bans", i));
	}
}


Clan *Clan::GetClan(IDNum id)
{
	/* First binary search. */
	if (Clan::IndexSize == 0)	return NULL;
	
	int bot = 0;
	int top = Clan::IndexSize - 1;
	
	for (;;)
	{
		int mid = (bot + top) / 2;
	
		Clan *clan = Clan::Index[mid];
		IDNum clanId = clan->GetID();
		
		if (clanId == id)	return Clan::Index[mid];
		if (bot >= top)		break;
		if (clanId > id)	top = mid - 1;
		else				bot = mid + 1;
	}

	return NULL;
}
