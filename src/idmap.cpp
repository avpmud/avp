#include "idmap.h"

IDMap::IDMap()
:	m_Capacity(0)
,	m_Size(0)
,	m_Unused(0)
,	m_IDs(NULL)
{
}


IDMap::~IDMap()
{
	delete [] m_IDs;
}


void IDMap::Add(Entity *e)
{
	if (m_Size == m_Capacity)
	{
		Compact();
		
		if (m_Size == m_Capacity)
		{
			m_Capacity += mGrowSize;
			MapEntry *	newMap = new MapEntry[m_Capacity];
			std::copy(m_IDs, m_IDs + m_Size, newMap);
			delete [] m_IDs;
			m_IDs = newMap;
		}
	}
	
	if (size() == 0 || e->GetID() > back().id)
	{
		//	Most frequent case
		m_IDs[m_Size] = MapEntry(e);
		++m_Size;
	}
	else
	{
		IDNum id = e->GetID();
		
		//	Can't use FindMapIndex because it returns -1 if not found
		//	and we want to find the nearest match!
		int		bot = 0;
		int		top = size() - 1;
		int		mid = -1;
		do
		{
			mid = (bot + top) / 2;
			
			IDNum	indexId = m_IDs[mid].id;
			
			if (indexId == id)	break;
			if (bot >= top)		break;
			if (indexId > id)	top = mid - 1;
			else				bot = mid + 1;
		} while (1);
		
		//	Persistent entry being re-added
		if (m_IDs[mid].id == id)
		{
			m_IDs[mid].entity = e;
		}
		else
		{
			if (id > m_IDs[mid].id)	++mid;
			
			//	insert it there
			//m_IDs.insert(m_IDs.begin() + mid, MapEntry(e));
			std::copy_backward(m_IDs + mid, m_IDs + m_Size, m_IDs + m_Size + 1);
			m_IDs[mid] = MapEntry(e);
			++m_Size;
		}
	}
}


void IDMap::Remove(Entity *e)
{
	int index = FindMapIndex(e->GetID());
	if (index == -1)	return;
	
	m_IDs[index].entity = NULL;
	
	++m_Unused;
}


Entity *IDMap::Find(IDNum id) const
{
	int index = FindMapIndex(id);
	return index == -1 ? NULL : m_IDs[index].entity;
}


int IDMap::FindMapIndex(IDNum id) const
{
	int bot = 0;
	int top = size() - 1;

	if (top < 0)	return (-1);
	
	do
	{
		int		mid = (bot + top) / 2;
		
		IDNum	indexId = m_IDs[mid].id;
		
		if (indexId == id)		return mid;
		if (bot >= top)			break;
		if (indexId > id)		top = mid - 1;
		else					bot = mid + 1;
	} while (1);

	return -1;
}


void IDMap::Compact()
{
	if (m_Unused < mUnusedThreshold)
		return;
	
	for (MapEntry *src = m_IDs, *end = m_IDs + m_Size, *dest = m_IDs;
		src != end; ++src)
	{
		if (src->entity)	*dest++ = *src;
		else				--m_Size;
	}
	
	m_Unused = 0;
}
