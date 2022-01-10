/*************************************************************************
*   File: affects.c++                Part of Aliens vs Predator: The MUD *
*  Usage: Primary code for affections                                    *
*************************************************************************/

#ifndef __BAN_H__
#define __BAN_H__

#include "types.h"
#include "lexilist.h"
#include "lexistring.h"

#define BAN_NOT 	0
#define BAN_NEW 	1
#define BAN_SELECT	2
#define BAN_ALL		3

#define BANNED_SITE_LENGTH    40

class Ban
{
public:
	Lexi::String	site;
	Lexi::String	name;
	int				type;
	time_t			date;
	
	static void	Load();
	static void Save();
	static int	IsBanned(const Lexi::String &host);
	static bool	IsValidName(const Lexi::String &name);

	typedef Lexi::List<Ban> List;
	static List		ms_Bans;
	static Lexi::StringList	ms_BadNames;
	
	static void	Reload()	//	Emergency function
	{
		ms_Bans.clear();
		ms_BadNames.clear();
		Load();
	}
};


#endif

