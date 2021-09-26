/*************************************************************************
*   File: index.c++                  Part of Aliens vs Predator: The MUD *
*  Usage: Primary code for indexes                                       *
*************************************************************************/



#include "index.h"
#include "interpreter.h"
#include "zones.h"


IndexBase::~IndexBase()
{
	
}


//	Binary search
IndexMapBase::value_type IndexMapBase::Find(VirtualID vid)
{
	if (m_Size == 0)	return NULL;
	if (!vid.IsValid())	return NULL;
	
	if (m_bDirty)		Sort();
	
//	unsigned long long ull = vid.as_ull();
	
	int bot = 0;
	int top = m_Size - 1;
	
	do
	{
		int mid = (bot + top) / 2;
		
		value_type i = m_Index[mid];
//		unsigned long long	vull = i->m_Virtual.as_ull();
		const VirtualID &ivid = i->m_Virtual;
		
//		if (vull == ull)	return i;
		if (ivid == vid)	return i;
		if (bot >= top)		break;
//		if (vull > ull)		top = mid - 1;
		if (ivid > vid)		top = mid - 1;
		else				bot = mid + 1;
	} while (1);
	
	ZoneData *z = VIDSystem::GetZoneFromAlias(vid.zone);
	if (z && z->GetHash() != vid.zone)
	{
		vid.zone = z->GetHash();
		return Find(vid);
	}
	
	return NULL;
}


//	Binary search
IndexMapBase::value_type IndexMapBase::Find(const char *arg)
{
	if (!*arg)			return NULL;
	return Find(VirtualID(arg));
}



static bool IndexMapLowerBoundFunc(const IndexBase *b, Hash zone)
{
	return (b->GetVirtualID().zone < zone);
}

static bool IndexMapUpperBoundFunc(Hash zone, const IndexBase *b)
{
	return (zone < b->GetVirtualID().zone);
}

IndexMapBase::iterator IndexMapBase::lower_bound(Hash zone)
{
//	if (m_Size == 0)	return m_Index;
	
	if (m_bDirty)		Sort();
	
	return std::lower_bound(m_Index, m_Index + m_Size, zone, IndexMapLowerBoundFunc);
}


IndexMapBase::iterator IndexMapBase::upper_bound(Hash zone)
{
//	if (m_Size == 0)	return m_Index;
	
	if (m_bDirty)		Sort();
	
	return std::upper_bound(m_Index, m_Index + m_Size, zone, IndexMapUpperBoundFunc);
}


void IndexMapBase::renumber(Hash oldHash, Hash newHash)
{
	if (m_bDirty)		Sort();
	
	for (iterator i = lower_bound(oldHash), e = upper_bound(oldHash); i != e; ++i)
		(*i)->m_Virtual.zone = newHash;
	
	m_bDirty = true;
	Sort();
}


void IndexMapBase::renumber(IndexBase *i, VirtualID newVid)
{
	if (!i)	return;
	
	i->m_Virtual = newVid;
	
	m_bDirty = true;
	Sort();
}


void IndexMapBase::insert(value_type i)
{
	if (m_Size == m_Allocated)
	{
		//	Grow: increase alloc size, alloc new, copy old to new, delete old, reassign
		m_Allocated += msGrowSize;
		value_type *newIndex = new value_type[m_Allocated];
		std::copy(m_Index, m_Index + m_Size, newIndex);
		delete [] m_Index;
		m_Index = newIndex;
	}
	
	//	Mark as dirty if the item being added would fall before the end of the entries
	if (!m_bDirty
		&& m_Size > 0
		&& i->m_Virtual < m_Index[m_Size - 1]->m_Virtual)
	{
		m_bDirty = true;	
	}

	//	Add new item to end
	m_Index[m_Size] = i;
	++m_Size;
}


bool IndexMapBase::	SortFunc(value_type lhs, value_type rhs) 
{
	return lhs->m_Virtual < rhs->m_Virtual;
}


void IndexMapBase::Sort()
{
	if (!m_bDirty)	return;
	
	std::sort(m_Index, m_Index + m_Size, SortFunc);
	
	m_bDirty = false;
}
