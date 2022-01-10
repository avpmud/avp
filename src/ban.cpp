/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-2005        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Based on CircleMUD, Copyright 1993-94, and DikuMUD, Copyright 1990-91 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : ban.cp                                                         [*]
[*] Usage: Player banishment system                                       [*]
\***************************************************************************/

#include "characters.h"
#include "descriptors.h"
#include "ban.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "staffcmds.h"

Ban::List	Ban::ms_Bans;
Lexi::StringList	Ban::ms_BadNames;


ACMD(do_ban);
ACMD(do_unban);

char *ban_types[] =
{
  "no",
  "new",
  "select",
  "all",
  "\n"
};


static bool BanSorter(const Ban &lhs, const Ban &rhs)
{
	return lhs.date > rhs.date;	
}


void Ban::Load(void)
{
	FILE *fp = fopen(BAN_FILE, "r");

	if (!fp)
	{
		log("SYSERR: Unable to open banfile");
		perror(BAN_FILE);
	}
	else
	{
		BUFFER(site, MAX_STRING_LENGTH);
		BUFFER(name, MAX_STRING_LENGTH);
		BUFFER(banType, MAX_STRING_LENGTH);

		int date;
		while (fscanf(fp, " %s %s %d %s ", banType, site, &date, name) == 4)
		{
			Ban	ban;
			
			ban.site = site;
			ban.name = name;
			ban.date = date;
			ban.type = search_block(banType, ban_types, true);
			
			if (ban.type != -1)
				ms_Bans.push_back(ban);
		}
		
		ms_Bans.sort(BanSorter);
		
		fclose(fp);
	}
	
	fp = fopen(XNAME_FILE, "r");

	if (!fp)
	{
		perror("SYSERR: Unable to open invalid name file");
	}
	else
	{
		BUFFER(name, MAX_STRING_LENGTH);
		
		while(get_line(fp, name))
			ms_BadNames.push_back(name);
		
		fclose(fp);
	}
}


int Ban::IsBanned(const Lexi::String &hostname)
{
	if (hostname.empty())
		return BAN_NOT;

	int i = 0;
	FOREACH(List, ms_Bans, ban)
	{
		if (hostname.find(ban->site) != Lexi::String::npos)
			i = MAX(i, ban->type);
	}

	return i;
}


void Ban::Save(void)
{
	FILE *fl = fopen(BAN_FILE, "w");
	if (!fl)
	{
		perror("SYSERR: write_ban_list");
		return;
	}

	FOREACH(List, ms_Bans, ban)
	{
		fprintf(fl, "%s %s %ld %s\n", ban_types[ban->type], ban->site.c_str(), ban->date, ban->name.c_str());
	}
	
	fclose(fl);
}


ACMD(do_ban)
{
	const char *	format = "%-40.40s %-6.6s %-15.15s %-16.16s\n";

	if (!*argument)
	{
		if (Ban::ms_Bans.empty())
		{
			ch->Send("No sites are banned.\n");
			return;
		}
		ch->Send(format,
				"Banned Site Name",
				"Type",
				"Banned On",
				"Banned By");
		ch->Send(format,
				"----------------------------------------",
				"----------------------------------------",
				"----------------------------------------",
				"----------------------------------------");

		FOREACH(Ban::List, Ban::ms_Bans, ban)
		{
			ch->Send(format, ban->site.c_str(), ban_types[ban->type],
				ban->date ? Lexi::CreateDateString(ban->date, "%a %b %e %Y").c_str() : "Unknown",
				ban->name.c_str());
		}
		return;
	}
	
	BUFFER(site, MAX_INPUT_LENGTH);
	BUFFER(flag, MAX_INPUT_LENGTH);

	two_arguments(argument, flag, site);
	
	int bantype = search_block(flag, ban_types, true);
	
	if (!*site || !*flag)
		ch->Send("Usage: ban all|select|new <site>\n");
	else if (bantype < BAN_NEW)
		ch->Send("Flag must be ALL, SELECT, or NEW.\n");
	else if (!str_cmp(flag, ban_types[BAN_ALL]) && !STF_FLAGGED(ch, STAFF_SECURITY))
		ch->Send("You must have full security privileges to do an ALL ban.\n");
	else
	{
		FOREACH(Ban::List, Ban::ms_Bans, ban)
		{
			if (ban->site == site)
			{
				ch->Send("That site has already been banned -- unban it to change the ban type.\n");
				return;
			}
		}
		
		
		Lexi::tolower(site);
		site[BANNED_SITE_LENGTH] = '\0';
		
		Ban	ban;
		ban.site = site;
		ban.name = ch->GetName();
		ban.date = time(0);
		ban.type = bantype;

		mudlogf(NRM, LVL_STAFF, TRUE,
				"%s has banned %s for %s players.", ch->GetName(), site,
				ban_types[bantype]);
		ch->Send("Site banned.\n");
		
		Ban::ms_Bans.push_front(ban);
		Ban::Save();
	}
}


ACMD(do_unban)
{
	BUFFER(site, MAX_INPUT_LENGTH);

	one_argument(argument, site);
	
	if (!*site)
		send_to_char("A site to unban might help.\n", ch);
	else
	{
		FOREACH(Ban::List, Ban::ms_Bans, ban)
		{
			if (ban->site == site)
			{
				ch->Send("Site unbanned.\n");
				mudlogf(NRM, LVL_STAFF, TRUE,
						"%s removed the %s-player ban on %s.",
						ch->GetName(), ban_types[ban->type], ban->site.c_str());
				
				Ban::ms_Bans.erase(ban);
				Ban::Save();
				return;
			}
		}

		ch->Send("That site is not currently banned.\n");
	}
}


/**************************************************************************
 *  Code to check for invalid names (i.e., profanity, etc.)		  *
 *  Written by Sharon P. Goza						  *
 **************************************************************************/

bool Ban::IsValidName(const Lexi::String &newname)
{
	/*
	 * Make sure someone isn't trying to create this same name.  We want to
	 * do a 'str_cmp' so people can't do 'Bob' and 'BoB'.  This check is done
	 * here because this will prevent multiple creations of the same name.
	 * Note that the creating login will not have a character name yet. -gg
	 */
    FOREACH(DescriptorList, descriptor_list, iter)
    {
    	DescriptorData *d = *iter;
		if (d->m_Character && d->m_Character->GetName() && d->m_Character->GetName() == newname)
			return false;
	}
	
	/* Does the desired name contain a string in the invalid list? */
	FOREACH(Lexi::StringList, ms_BadNames, name)
	{
		if (newname.find(*name) != Lexi::String::npos)
			return false;
	}
	
	return true;
}
