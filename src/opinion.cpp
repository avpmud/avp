/*************************************************************************
*   File: opinion.c++                Part of Aliens vs Predator: The MUD *
*  Usage: Code file for opinion system                                   *
*************************************************************************/

#include <algorithm>

#include "opinion.h"
#include "characters.h"
#include "descriptors.h"
#include "rooms.h"
#include "index.h"
#include "utils.h"

Opinion::Opinion(void) :
	m_Sexes(0),
	m_Races(0)
{
}


Opinion::Opinion(const Opinion &op) :
	 //	m_Characters(),	We don't copy the characters
	m_Sexes(op.m_Sexes),
	m_Races(op.m_Races),
	m_VNum(op.m_VNum),
	m_ActiveTypes(op.m_ActiveTypes)
{
}


void Opinion::ClearCharacters(void)
{
	m_Characters.clear();
	m_ActiveTypes.clear(TypeCharacter);
}


void Opinion::Remove(Type type, int param)
{
	switch (type)
	{
		case TypeCharacter:
			m_Characters.remove(param);
			m_ActiveTypes.set(TypeCharacter, !m_Characters.empty());
			break;
		case TypeSex:
			REMOVE_BIT(m_Sexes, param);
			m_ActiveTypes.set(TypeSex, m_Sexes != 0);
			break;
		case TypeRace:
			REMOVE_BIT(m_Races, param);
			m_ActiveTypes.set(TypeRace, m_Races != 0);
			break;
		case TypeVNum:
			m_VNum.Clear();
			m_ActiveTypes.clear(TypeVNum);
			break;
		default:
			mudlogf(NRM, LVL_STAFF, TRUE, "Bad type (%d) passed to Opinion::Add.", type);
	}
}


void Opinion::Add(Type type, int param)
{
	switch (type)
	{
		case TypeCharacter:
			if (!InList(param))
				m_Characters.push_back(param);
			
			m_ActiveTypes.set(TypeCharacter, !m_Characters.empty());
			break;
		case TypeSex:
			SET_BIT(m_Sexes, param);
			m_ActiveTypes.set(TypeSex, m_Sexes != 0);
			break;
		case TypeRace:
			SET_BIT(m_Races, param);
			m_ActiveTypes.set(TypeRace, m_Sexes != 0);
			break;
		default:
			mudlogf(NRM, LVL_STAFF, TRUE, "Bad type (%d) passed to Opinion::Add.", type);
	}
}


void Opinion::Add(Type type, VirtualID param)
{
	switch (type)
	{
		case TypeVNum:
			m_VNum = param;
			m_ActiveTypes.set(TypeVNum, m_VNum.IsValid());
			break;
		default:
			mudlogf(NRM, LVL_STAFF, TRUE, "Bad type (%d) passed to Opinion::Add.", type);
	}
}

void Opinion::Toggle(Type type, int param)
{
	switch (type) {
		case TypeSex:
			TOGGLE_BIT(m_Sexes, param);
			m_ActiveTypes.set(TypeSex, m_Sexes != 0);
			break;
		case TypeRace:
			TOGGLE_BIT(m_Races, param);
			m_ActiveTypes.set(TypeRace, m_Sexes != 0);
			break;
		default:
			mudlogf(NRM, LVL_STAFF, TRUE, "Bad type (%d) passed to Opinion::Toggle.", type);
	}
}


bool Opinion::InList(IDNum id)
{
	return find(m_Characters.begin(), m_Characters.end(), id) != m_Characters.end();
}


bool Opinion::IsTrue(CharData *ch)
{
	if (IS_SET(m_Races, 1 << ch->GetRace()))				return true;
	if (IS_SET(m_Sexes, 1 << ch->GetSex()))					return true;
	if (m_VNum.IsValid() && m_VNum == ch->GetVirtualID())	return true;
	if (InList(ch->GetID()))								return true;
	
	return false;
}


CharData *Opinion::FindTarget(CharData *ch)
{
	if (ch->InRoom() == NULL)
		return NULL;
	
	FOREACH(CharacterList, ch->InRoom()->people, iter)
	{
		CharData *temp = *iter;
	
		if ((ch != temp) && ch->CanSee(temp) == VISIBLE_FULL && IsTrue(temp))
		{
			return temp;
		}
	}
	return NULL;
}
