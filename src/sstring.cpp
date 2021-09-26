/*************************************************************************
*   File: sstring.cp                 Part of Aliens vs Predator: The MUD *
*  Usage: Code file for shared strings                                   *
*************************************************************************/

#include "lexisharedstring.h"


/*
inline bool operator== (const char *ptr, const String & str) {
	return str.compare(ptr);
}
*/

//	Used for a super-shared-string memory architecture
//	Save (some) memory at cost of speed during SSCreate calls
//	While at same time using +8 bytes per SString

/* create a new shared string */

Lexi::SharedString::Implementation *Lexi::SharedString::GetBlank()
{
//	static Lexi::SharedString::Implementation s_Blank("");	
	
//	return &s_Blank;

	static Lexi::SharedString s_Blank(Lexi::String(""));

	return s_Blank.m_Shared;
}
//	The static above 
//static Lexi::SharedString	s_SharedStringBlankRefHolder;	//	Otherwise, it is possible to get one reference then lost it and delete this!

Lexi::String & Lexi::SharedString::GetStringRef()
{
	return *m_Shared;
}


/*SString &*/ void Lexi::SharedString::operator=(const char *str)
{
	m_Shared = str && *str ? new Implementation(str) : GetBlank();
	//return *this;
}


