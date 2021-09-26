/*************************************************************************
*   File: extradesc.h                Part of Aliens vs Predator: The MUD *
*  Usage: Header file for Extra Descriptions                             *
*************************************************************************/

#ifndef __EXTRADESC_H__
#define __EXTRADESC_H__

#include "lexisharedstring.h"
#include "SharedPtr.h"

#include <vector>


#if 1
// Extra description: used in objects, mobiles, and rooms
class ExtraDesc// : public Lexi::Shareable
{
public:
	ExtraDesc() {}
	ExtraDesc(const char *key, const char *desc) : keyword(key), description(desc) {}
	
	Lexi::SharedString	keyword;				//	Keyword in look/examine
	Lexi::SharedString	description;			//	What to see

//	typedef Lexi::SharedPtr<ExtraDesc>	Ptr;
	
	class List : public std::vector<ExtraDesc /*Ptr*/>
	{
	public:
		const char *Find(const char *key);
	};
};

#else


//	This implementation below is still in development
//	results in an extra allocated vector for every prototype
//	but not per instance; probably uses more memory as a result (!)
//	Also needs special handling for editing

//	Current Recommendation: Do not use (yet)
//	Ideas: Use a static empty for begin()/end(), allocate on push_back, release on last erase

// Extra description: used in objects, mobiles, and rooms
class ExtraDesc
{
public:
	ExtraDesc() {}
	ExtraDesc(const char *key, const char *desc) : keyword(key), description(desc) {}
	
	Lexi::String		keyword;				//	Keyword in look/examine
	Lexi::String		description;			//	What to see

private:
	class ListImp : public Lexi::Shareable, public std::vector<ExtraDesc>
	{
	};

public:
	class List
	{
	public:
		List()
		:	m_List(new ListImp)
		{}
		
		typedef ListImp::iterator		iterator;
		typedef ListImp::const_iterator	const_iterator;
		
		bool					empty()			{ return m_List->empty(); }
		ListImp::size_type		size()			{ return m_List->size(); }
		iterator				begin() 		{ return m_List->begin(); }
		iterator				end() 			{ return m_List->end(); }
		const_iterator			begin() const	{ return m_List->begin(); }
		const_iterator			end() const 	{ return m_List->end(); }
		iterator				erase(iterator &i) { return m_List->erase(i); }
		void					push_back(const ExtraDesc &e) { return m_List->push_back(e); }

		const char *			Find(const char *key);
		
	private:
		Lexi::SharedPtr<ListImp>	m_List;
	};
};

#endif


#endif
