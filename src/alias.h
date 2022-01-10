

#ifndef __ALIAS_H__
#define __ALIAS_H__

#include <list>

#include "types.h"
#include "lexistring.h"

class Alias {
public:
	Alias(const char *command, const char *replacement);
	
	enum Type
	{
		SimpleType = 0,
		ComplexType = 1
	};
	
	const char *		GetCommand() const 		{ return m_Command.c_str(); }
	const char *		GetReplacement() const	{ return m_Replacement.c_str(); }
	Type				GetType() const			{ return m_Type; }

	static bool			IsSameCommand(const Alias *, const char *cmd);
	
	static const char SeparatorChar = ';';
	static const char VariableChar = '$';
	static const char GlobChar = '*';
	
	class List : private std::list<Alias>
	{
	public:
		void			Add(const char *command, const char *replacement) 
		{
			push_back(Alias(command, replacement));
		}
		const Alias *	Find(const char *command) const;
		void			Remove(const Alias *a);
		
		using std::list<Alias>::iterator;
		using std::list<Alias>::begin;
		using std::list<Alias>::end;
		using std::list<Alias>::empty;
	};
	
private:
	Lexi::String		m_Command;
	Lexi::String		m_Replacement;
	Type				m_Type;
	
	//	Prevent copy construction and direct copying; although it's
	//	safe, we don't have a need to do this.
//						Alias(const Alias &);
	Alias &				operator=(const Alias &);
};


#endif
