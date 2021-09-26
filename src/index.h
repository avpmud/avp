/*************************************************************************
*   File: index.h                    Part of Aliens vs Predator: The MUD *
*  Usage: Header file for indexes                                        *
*************************************************************************/

#ifndef	__INDEX_H__
#define __INDEX_H__

#include "types.h"

#include <vector>

#include "WeakPtr.h"

#include "internal.defs.h"
#include "scriptvariable.h"
#include "virtualid.h"

typedef std::vector< WeakPtr<class BehaviorSet> >	BehaviorSets;


class IndexBase
{
public:
						IndexBase(const VirtualID &vn) : m_Virtual(vn) {}
	const VirtualID &	GetVirtualID() const { return m_Virtual; }
	virtual				~IndexBase();

protected:
	VirtualID			m_Virtual;		//	virtual number
	friend class IndexMapBase;
};


typedef std::vector<VirtualID>	ScriptVector;		//	triggers for mob/obj


template <class T>
class Index : public IndexBase
{
public:
						Index(const VirtualID &vn, T *proto)
						:	IndexBase(vn)
						,	m_pProto(proto)
						,	m_Count(0)
						,	m_Function(NULL)
						{
						}

	T *					m_pProto;
	unsigned int		m_Count;		//	number of existing units
						SPECIAL(*m_Function);	//	specfunc
	
	ScriptVector		m_Triggers;		//	triggers for mob/obj
	ScriptVariable::List	m_SharedVariables;
	ScriptVariable::List	m_InitialGlobals;
	BehaviorSets		m_BehaviorSets;

private:
						Index();
						Index(const Index &);
	Index &				operator=(const Index &);
};


class IndexMapBase
{
public:
	typedef IndexBase	data_type;
	typedef data_type *	value_type;
	typedef value_type *pointer;
	typedef value_type &reference;
	typedef pointer		iterator;
	typedef const pointer const_iterator;
	
	IndexMapBase() : m_Allocated(0), m_Size(0), m_bDirty(false), m_Index(NULL) {}
	~IndexMapBase() { delete [] m_Index; }
	
	size_t				size() { return m_Size; }
	bool				empty() { return m_Size == 0; }
		
	void				renumber(Hash old, Hash newHash);
	void				renumber(IndexBase *i, VirtualID newVid);
	
protected:

	void				insert(value_type i);
	value_type			Find(VirtualID vid);
	value_type			Find(const char *arg);
	value_type			Get(size_t i)		{ return i < m_Size ? m_Index[i] : NULL; }
		
	iterator			begin()			{ return m_Index; }
	iterator			end()			{ return m_Index + m_Size; }
	const_iterator		begin() const	{ return m_Index; }
	const_iterator		end() const 	{ return m_Index + m_Size; }

	iterator			lower_bound(Hash zone);
	iterator			upper_bound(Hash zone);

protected:
	value_type **		GetIndex()		{ return &m_Index; }
private:
	
	unsigned int		m_Allocated;
	unsigned int		m_Size;
	bool				m_bDirty;
	value_type *		m_Index;
	
	static const int	msGrowSize = 128;
	
	void				Sort();
	static bool			SortFunc(value_type lhs, value_type rhs);
		
	IndexMapBase(const IndexMapBase &);
	IndexMapBase &operator=(const IndexMapBase &);
};



template <class T>
class IndexMap : public IndexMapBase
{
public:
	typedef Index<T>		data_type;
	typedef data_type *		value_type;
	typedef value_type *	pointer;
	typedef value_type &	reference;
	typedef pointer			iterator;
	typedef const pointer	const_iterator;
	
	IndexMap() : m_Indexed(*reinterpret_cast<value_type **>(GetIndex())) {}

	value_type			insert(const VirtualID &vid, T *p) { value_type index = new data_type(vid, p); IndexMapBase::insert(index); return index; }
	value_type			Find(VirtualID vid) { return (value_type)IndexMapBase::Find(vid); }
	value_type			Find(const char *arg) { return (value_type)IndexMapBase::Find(arg); }
	value_type			Get(size_t i)		{ return (value_type)IndexMapBase::Get(i); }
	
	value_type			operator[] (size_t i) { return (value_type)IndexMapBase::Get(i); }
	
	iterator			begin()			{ return (iterator)IndexMapBase::begin();  }
	iterator			end()			{ return (iterator)IndexMapBase::end();  }
	const_iterator		begin() const	{ return (iterator)IndexMapBase::begin();  }
	const_iterator		end() const		{ return (iterator)IndexMapBase::end();  }

	iterator			lower_bound(Hash zone)
		{ return (iterator)IndexMapBase::lower_bound(zone);  }
	iterator			upper_bound(Hash zone)
		{ return (iterator)IndexMapBase::upper_bound(zone);  }

private:
	value_type * &		m_Indexed;
};



#endif

