#ifndef __IDMAP_H__
#define __IDMAP_H__

#include <utility>
#include <vector>

#include "types.h"
#include "entity.h"

class IDMap
{
public:
						IDMap();
						~IDMap();
						
	void				Add(Entity *e);
	void				Remove(Entity *e);
	Entity *			Find(IDNum id) const;

private:
	struct MapEntry
	{
		MapEntry() {}
		MapEntry(Entity *e) : id(e->GetID()), entity(e) {}
		
		IDNum			id;
		Entity *		entity;
	};

	unsigned int		m_Capacity;
	unsigned int		m_Size;
	unsigned int		m_Unused;
	MapEntry *			m_IDs;
	
	static const int mGrowSize = 500;
	static const int mUnusedThreshold = 500;
	
	int					FindMapIndex(IDNum id) const;
	void				Compact();
	
	unsigned int		size() const { return m_Size; }
	MapEntry &			back() const { return m_IDs[m_Size - 1]; }
};


#endif	//	__IDMAP_H__
