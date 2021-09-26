/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 2006-2007        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Original LexiMUD Code                                                 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : SharedPtr.h                                                    [*]
[*] Usage: Reference-counted objects and pointer wrappers                 [*]
[*]        Objects that automatically self-delete when their ref-count    [*]
[*]        drops to 0.                                                    [*]
\***************************************************************************/

#ifndef __SHARED_PTR_H__
#define __SHARED_PTR_H__


namespace Lexi 
{
	class Shareable;
	class SharedPtrBase;
	
	template <class T> class SharedPtr;
}


class Lexi::SharedPtrBase
{
protected:
	void Retain(Shareable *sh);
	void Release(Shareable *sh);
};

template<class T>
class Lexi::SharedPtr : private Lexi::SharedPtrBase
{
public:
	typedef T			value_type;
	typedef T *			pointer;
	typedef const T *	const_pointer;
	typedef T &			reference;
	typedef const T &	const_reference;
	typedef SharedPtr<T>	__self;
	
	SharedPtr() : ptr(NULL)
	{
	}
	
	SharedPtr(pointer p) : ptr(p)
	{
		if (ptr)	Retain(ptr);
	}
	
	SharedPtr(const __self &sp) : ptr(sp.ptr)
	{
		if (ptr)	Retain(ptr);
	}
	
	~SharedPtr()
	{
		if (ptr)	Release(ptr);	
	}
	
	__self &operator=(pointer p)
	{
		Assign(p);
		return *this;
	}
	
	__self &operator=(const __self &sp)
	{
		Assign(sp.ptr);
		return *this;
	}
	
	reference operator*() const { return *ptr; }
	pointer operator->() const { return ptr; }
	
	operator bool() const { return ptr != NULL; }
	operator pointer() const { return ptr; }
	
	pointer get() const { return ptr; }
	
	bool operator==(const pointer p) const
	{
		return ptr == p;
	}

protected:
	void Assign(pointer p)
	{
		if (ptr == p)
			return;
		
		if (ptr)	Release(ptr);
		ptr = p;
		if (ptr)	Retain(ptr);
	}
	
private:
	pointer				ptr;
	
	friend class Shareable;
};


class Lexi::Shareable
{
protected:
					Shareable() : m_Refs(0) {}
					Shareable(const Shareable &) : m_Refs(0) {}
	virtual 		~Shareable() {}
	
	void			operator=(const Shareable &) { /* Nothing */ }
	
	void			Retain() { ++m_Refs; }
	void			Release() { if (--m_Refs == 0) delete this; }

private:
	int				m_Refs;
	
	friend class SharedPtrBase;
};


inline void Lexi::SharedPtrBase::Retain(Shareable *sh) { sh->Retain(); }
inline void Lexi::SharedPtrBase::Release(Shareable *sh) { sh->Release(); }

#endif
