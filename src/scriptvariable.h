#ifndef __SCRIPTVARIABLE_H__
#define __SCRIPTVARIABLE_H__

#include <vector>

#include "types.h"
#include "lexistring.h"

namespace Lexi 
{
	class Parser;
	class OutputParserFile;
}

//#error TODO: Make the HashName (Lexi::String w/ a Hash)

class ScriptVariable
{
public:
						ScriptVariable(const char *name, int context = 0);
						ScriptVariable(const char *name, Hash hash, int context);
						ScriptVariable(const ScriptVariable &var);
						~ScriptVariable() { m_Impl->Release(); }
		
	ScriptVariable &	operator=(const ScriptVariable &var);

	const char *		GetName() const			{ return m_Impl->m_Name.c_str(); }
	const Lexi::String &GetNameString() const	{ return m_Impl->m_Name; }
	Hash				GetNameHash() const		{ return m_Impl->m_NameHash; }
	int					GetContext() const		{ return m_Impl->m_Context; }
	const char *		GetValue() const		{ return m_Impl->m_Value.c_str(); }
	const Lexi::String &GetValueString() const	{ return m_Impl->m_Value; }
	
	void				SetValue(const char *val);
	
	bool				IsShared() const { return m_Impl->m_RefCount > 1; }
	
//	bool				operator==(const ScriptVariable &rhs) const;
//	bool				operator!=(const ScriptVariable &rhs) const { return !(*this == rhs); }
	
	class List : private std::vector<class ScriptVariable *>
	{
	public:
		List() {}
		~List() { Clear(); }
		
		List(const List &list);
		List &operator=(const List &list);
		
		void Clear();
		
		ScriptVariable *AddBlank() 
		{
			ScriptVariable *var = new ScriptVariable("");
			insert(begin(), var);
			return var;
		}
		
		List &Add(const char *name, const char *value, int context = 0);
		List &Add(const char *name, const Lexi::String &value, int context = 0) { return Add(name, value.c_str(), context); }
		List &Add(const char *name, Entity *e, int context = 0);
		List &Add(const char *name, int i, int context = 0);
		List &Add(const char *name, float f, int context = 0);
		List &Add(ScriptVariable *add);
		
		List &Add(const List &list);
		
		ScriptVariable *Find(const char *name, Hash hash, int context = 0, bool extract = false);
		ScriptVariable *Find(const ScriptVariable *var) { return Find(var->GetName(), var->GetNameHash(), var->GetContext()); }
		
		void				Sort(); // Force a sort, mainly used for editing

		void				Parse(Lexi::Parser &parser);
		void				Write(Lexi::OutputParserFile &file);
		
		//	Expose some functionality of std::vector<>
		typedef std::vector<class ScriptVariable *>	base;
		typedef base::iterator iterator;
		using base::begin;
		using base::end;
		using base::empty;
		using base::size;
		//using push_back;
		using base::erase;
		void swap(List& x) {base::swap(x);}
	};
	
	static ScriptVariable *Parse(Lexi::Parser &parser);
	void				Write(Lexi::OutputParserFile &file, const Lexi::String &label, bool bSaving);
		//	bSaving = save in file (no flags)
		
#if defined(SCRIPTVAR_TRACKING)
	static unsigned int	GetScriptVariableCount() { return Implementation::ms_Count; }
	static unsigned int	GetScriptVariableTotalRefs() { return Implementation::ms_TotalRefs; }
#endif

private:
	class Implementation
	{
	public:
		Implementation(const char *name, int context);
		Implementation(const char *name, Hash hash, int context);
		Implementation(const Implementation &);
#if defined(SCRIPTVAR_TRACKING)
		~Implementation() { --ms_Count; }
#endif

		const Lexi::String	m_Name;				//	name of variable
		Lexi::String		m_Value;			//	value of variable
		const int			m_Context;
		Hash				m_NameHash;
//		bool				m_Save;

		unsigned int		m_RefCount;
		
		void				Retain()
		{
#if defined(SCRIPTVAR_TRACKING)
			++ms_TotalRefs;
#endif
			++m_RefCount;
		}
		void				Release();
		
#if defined(SCRIPTVAR_TRACKING)
		static unsigned int	ms_Count;
		static unsigned int ms_TotalRefs;
#endif
	};
	
	Implementation *	m_Impl;
};


inline ScriptVariable::ScriptVariable(const char *name, int context)
:	m_Impl(new Implementation(name, context))
{
}


inline ScriptVariable::ScriptVariable(const char *name, Hash hash, int context)
:	m_Impl(new Implementation(name, hash, context))
{
}


inline ScriptVariable::ScriptVariable(const ScriptVariable &var)
:	m_Impl(var.m_Impl)
{
	m_Impl->Retain();
}


inline ScriptVariable &ScriptVariable::operator=(const ScriptVariable &var)
{
	m_Impl->Release();
	m_Impl = var.m_Impl;
	m_Impl->Retain();
	return *this;
}


/*
inline bool ScriptVariable::operator==(const ScriptVariable &rhs) const
{
	return m_Impl->m_NameHash == rhs.m_Impl->m_NameHash
		&& m_Impl->m_Context == rhs.m_Impl->m_Context
#if defined(HASH_SANITY_CHECKS)
		&& m_Impl->m_Name == rhs.m_Impl->m_Name
#endif
		;
}
*/

inline ScriptVariable::Implementation::Implementation(const char *name, int context)
:	m_Name(name)
,	m_Context(context)
,	m_NameHash(GetStringFastHash(m_Name.c_str()))
,	m_RefCount(1)
{
#if defined(SCRIPTVAR_TRACKING)
	++ms_Count;
	++ms_TotalRefs;
#endif
}


inline ScriptVariable::Implementation::Implementation(const char *name, Hash hash, int context)
:	m_Name(name)
,	m_Context(context)
,	m_NameHash(hash)
,	m_RefCount(1)
{
#if defined(SCRIPTVAR_TRACKING)
	++ms_Count;
	++ms_TotalRefs;
#endif
}


inline ScriptVariable::Implementation::Implementation(const Implementation &imp)
:	m_Name(imp.m_Name)
,	m_Context(imp.m_Context)
,	m_NameHash(imp.m_NameHash)
,	m_RefCount(1)
{
#if defined(SCRIPTVAR_TRACKING)
	++ms_Count;
	++ms_TotalRefs;
#endif
}


inline void ScriptVariable::Implementation::Release()
{
#if defined(SCRIPTVAR_TRACKING)
	--ms_TotalRefs;
#endif
	if (--m_RefCount == 0)	delete this;
}


#endif
