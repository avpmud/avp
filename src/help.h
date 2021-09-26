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
[*] File : help.c++                                                       [*]
[*] Usage: Help system code                                               [*]
\***************************************************************************/

#ifndef __HELP_H__
#define __HELP_H__

#include "types.h"

#include <vector>
#include "lexistring.h"

class HelpManager
{
public:

	virtual ~HelpManager() {}
	
	class Entry {
	public:
							Entry() : m_MinimumLevel(0) {}
							
		Lexi::String		m_Keywords;
		Lexi::String		m_Entry;
		int					m_MinimumLevel;
	};
	
	static HelpManager *Instance();
	
	virtual void		Load() = 0;
	virtual void		Save() = 0;
	
	virtual Entry *		Find(const char *keyword) = 0;
	virtual void		Add(const Entry &e) = 0;
	
	virtual const std::vector<Entry *> &GetTable() const = 0;
};

#endif

