/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 2007             [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Original LexiMUD Code                                                 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : lexistring.h                                                   [*]
[*] Usage: Lexi::String is an std::string compatible, case-insensitive,   [*]
[*]        reimplementation, optimized for speed and memory               [*]
\***************************************************************************/

#ifndef __LEXISHAREDSTRING_H__
#define __LEXISHAREDSTRING_H__


#include "lexistring.h"
#include "SharedPtr.h"


namespace Lexi
{
	class SharedString;
}


class Lexi::SharedString
{
public:
						SharedString() : m_Shared(GetBlank()) { }
	explicit			SharedString(const char *str) : m_Shared(str && *str ? new Implementation(str) : GetBlank()) { }
						SharedString(const Lexi::String &str) : m_Shared(new Implementation(str)) { }

	/*SharedString &*/ void	operator=(const char *str);
	
	bool				operator==(const SharedString &ss) const { return *m_Shared == *ss.m_Shared; }
	bool				operator==(const char *str) const { return *m_Shared == str; }
	
//						operator const char *() const { return c_str(); }
						operator const Lexi::String &() const { return *m_Shared; }
	const char *		c_str() const { return m_Shared->c_str(); }
	bool				empty() const { return m_Shared->empty(); }
	
	Lexi::String &		GetStringRef();
	
private:
	class Implementation : public Lexi::String, public Shareable
	{
	public:
							Implementation(const char *str) : Lexi::String(str) {}
							Implementation(const Lexi::String &str) : Lexi::String(str) {}

	private:
							Implementation();
							Implementation(const Implementation &);
	};
	
	SharedPtr<Implementation>	m_Shared;
	static Implementation *	GetBlank();
};

inline const char *SSData(const Lexi::SharedString &str) { return str.c_str(); }

#endif	//	__LEXISHAREDSTRING_H__
