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
[*] File : entity.cp                                                      [*]
[*] Usage: Meta class for the primary interactive objects                 [*]
\***************************************************************************/

#include "entity.h"
#include "scripts.h"
#include "lexilist.h"
#include "utils.h"

Entity::Entity()
:	m_ID(0)
,	m_pScript(NULL)
,	m_bPurged(false)
{
	m_pScript = new Scriptable;
}


Entity::Entity(const Entity &entity)
:	m_ID(0)
,	m_pScript(NULL)
,	m_bPurged(false)
{
	m_pScript = new Scriptable;
}


void Entity::operator=(const Entity &entity)
{
//	m_ID = 0;
//	m_bPurged = false;
}


Entity::~Entity()
{
	delete m_pScript;
}			//	Nothing to destruct; necessary for Virtual


void Entity::Extract()
{
	m_pScript->Extract();
}


Lexi::List<Entity *>	sPurgedEntities;

void Entity::Purge()
{
	if (IsPurged())
		return;
	
	m_bPurged = true;
	sPurgedEntities.push_back(this);
}


void Entity::DeletePurged()
{
	FOREACH(Lexi::List<Entity *>, sPurgedEntities, entity)
	{
		delete *entity;
	}
	sPurgedEntities.clear();
}


const char * Entity::GetIDString()
{
	return MakeIDString(GetID());
}


const char * MakeIDString(IDNum id)
{
	const int NUM_BUFFERS = 8;
	static char sBuffers[NUM_BUFFERS][32];
	static int	sCurBuffer = 0;
	
	char *buf = sBuffers[sCurBuffer];
	sCurBuffer = (sCurBuffer + 1) % NUM_BUFFERS;
	
	sprintf(buf, "%c%u", UID_CHAR, id);
	
	return buf;
}


IDNum ParseIDString(const char *str)
{
	if (!IsIDString(str))	return 0;
	return strtoul(str + 1, NULL, 0);
}



#include "idmap.h"
//#include "rooms.h"


class IDManagerImplementation : public IDManager
{
public:
	IDManagerImplementation()
	:	m_NextAvailableID(DYNAMIC_ID_BASE)
	{
	}
	
	virtual ~IDManagerImplementation() {}
	
	virtual void		GetAvailableIDNum(Entity *e);
	virtual void		Register(Entity *e);
	virtual void		Unregister(Entity *e);
	
	virtual Entity *	Find(IDNum id) const;

private:
	//	For optimization purposes, some ID types are separated from the rest
//	IDMap				m_PlayerIDMap;
	IDMap				m_EntityIDMap;
	
//	static const IDNum	ROOM_ID_BASE		= 50000;
//	static const IDNum	DYNAMICROOM_ID_BASE	= ROOM_ID_BASE + 50000;
//	static const IDNum	ID_BASE				= DYNAMICROOM_ID_BASE + 100000;

	static const IDNum	PERSISTENT_ID_BASE	= 80000;	//	10k persistent objects
	static const IDNum	DYNAMIC_ID_BASE		= 100000;

	IDNum				m_NextAvailableID;//= DYNAMIC_ID_BASE;
//	IDNum				nextAvailableDynamicRoomID	= DYNAMICROOM_ID_BASE;
};


IDManager *IDManager::Instance()
{
	static IDManagerImplementation sManager;
	return &sManager;
}


void IDManagerImplementation::GetAvailableIDNum(Entity *e)
{
	switch (e->GetEntityType())
	{
		case Entity::TypeCharacter:
		case Entity::TypeObject:
		case Entity::TypeRoom:
			e->SetID(m_NextAvailableID++);
			break;
#if 0
		case Entity::TypeRoom:
			{
				RoomData *r = dynamic_cast<RoomData *>(e);
				if (!r || !r->GetVirtualID().IsValid())
				{
					e->SetID(nextAvailableDynamicRoomID++);
					break;
				}
				
				e->SetID(ROOM_ID_BASE + r->GetVirtualNumber());	
			}
			break;
#endif
	}
}


void IDManagerImplementation::Register(Entity *e)
{
	if (e->GetID() == 0)		GetAvailableIDNum(e);
/*	if (e->GetID() < ID_BASE)	playerIDMap.Add(e);
	else*/						m_EntityIDMap.Add(e);
}


void IDManagerImplementation::Unregister(Entity *e)
{
/*	if (e->GetID() < ID_BASE)	playerIDMap.Remove(e);
	else*/						m_EntityIDMap.Remove(e);
}


Entity *IDManagerImplementation::Find(IDNum id) const
{
	if (id == 0)		return NULL;
/*	if (id < ID_BASE)	return playerIDMap.Find(id);
	else*/				return m_EntityIDMap.Find(id);
}
