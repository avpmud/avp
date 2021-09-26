/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-2005        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Original LexiMUD Code                                                 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : entity.h                                                       [*]
[*] Usage: Meta class for the primary interactive objects                 [*]
\***************************************************************************/

#ifndef	__ENTITY_H__
#define __ENTITY_H__


#include "types.h"


class Scriptable;


class Entity {
public:
						Entity();
						Entity(const Entity &entity);
	virtual				~Entity();	//	Non-virtual until the code is underlying code is fixed up enough

	void				operator=(const Entity &entity);
	
	IDNum				GetID() const		{ return m_ID; }
	Scriptable *		GetScript() const	{ return m_pScript; }
	bool				IsPurged() const	{ return m_bPurged; }
	
	const char *		GetIDString();
	
	virtual void		Extract();
	
//protected:
	void				SetID(IDNum nID)	{ m_ID = nID; }
	void				SetPurged()			{ m_bPurged = true; }
	void				Purge();

	static void			DeletePurged();

	enum Type
	{
		TypeUndefined = -1,
		TypeCharacter,
		TypeObject,
		TypeRoom
	};
	
	virtual Type		GetEntityType() = 0;
	
private:
	IDNum				m_ID;
	Scriptable *		m_pScript;
	bool				m_bPurged;
};



/*
 * Escape character, which indicates that the name is
 * a unique id number rather than a standard name.
 * Should be nonprintable so players can't enter it.
 */
//const char UID_CHAR = 0x1b; /* '\e' */
const char UID_CHAR = 0xAE; /* (R) */


inline bool			IsIDString(const char *str) { return *str == UID_CHAR; }
const char *		MakeIDString(IDNum id);
IDNum				ParseIDString(const char *str);


class IDManager
{
public:
	virtual ~IDManager() {} 

	static IDManager *	Instance();
	
	virtual void		GetAvailableIDNum(Entity *e) = 0;
	virtual void		Register(Entity *e) = 0;
	virtual void		Unregister(Entity *e) = 0;
	
	virtual Entity *	Find(IDNum id) const = 0;
	Entity *			Find(const char *str) const { return Find(ParseIDString(str)); }
};

#endif
