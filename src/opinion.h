/*************************************************************************
*   File: opinion.h                  Part of Aliens vs Predator: The MUD *
*  Usage: Header file for opinion system                                 *
*************************************************************************/

#ifndef __OPINION_H__
#define __OPINION_H__

#include <list>

#include "types.h"
#include "bitflags.h"
#include "virtualid.h"

class Opinion
{
public:
						Opinion(void);
						Opinion(const Opinion &op);
				
	Opinion &			operator=(const Opinion &op);
	
	enum Type
	{
		TypeCharacter,
		TypeSex,
		TypeRace,
		TypeVNum,
		NumTypes
	};

	void				ClearCharacters(void);
	void				Remove(Type type, int param = -1);
	void				Add(Type type, int param);
	void				Add(Type type, VirtualID param);
	void				Toggle(Type type, int param);
	bool				IsActive() { return m_ActiveTypes.any(); }
	bool				InList(IDNum id);
	bool				IsTrue(CharData *ch);
	CharData *			FindTarget(CharData *ch);
	
	std::list<IDNum>	m_Characters;
	Flags				m_Sexes;
	Flags				m_Races;
	VirtualID			m_VNum;
	
	Lexi::BitFlags<NumTypes, Type>	m_ActiveTypes;
};

#endif

