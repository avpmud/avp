/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 2005             [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | chris@mainecoon.net           [*]
[-]---------------------------------------+-------------------------------[-]
[*] Based on CircleMUD, Copyright 1993-94, and DikuMUD, Copyright 1990-91 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : PersonalPtr.h                                                  [*]
[*] Usage: A PersonalPtr<T> owns it's own copy of T, which is copy        [*]
[*]        constructed on assignment.                                     [*]
\***************************************************************************/

#ifndef __PERSONAL_PTR_H__
#define __PERSONAL_PTR_H__

template <class T>
class PersonalPtr
{
public:
	typedef T value_type;
	
	explicit PersonalPtr(T *p = 0);
	PersonalPtr(const PersonalPtr &m);
	template<class X> PersonalPtr(const PersonalPtr<X> &m);
	
	PersonalPtr<T> &operator=(T *p);
	PersonalPtr<T> &operator=(const PersonalPtr &m);
	template<class X>PersonalPtr<T> &operator=(const PersonalPtr<X> &m);
	
	~PersonalPtr();
	
	T *Get() const { return ptr; }
	void Duplicate(T *p = NULL);
	void Create();
	
	T &operator*() const { return *ptr; }
	T *operator->() { return ptr; }
	operator bool() const { return ptr != NULL; }
	
	operator T*() const { return ptr; }
	
private:
	T *ptr;
};


	
template<class T>
PersonalPtr<T>::PersonalPtr(T *p) : ptr(NULL)
{
	if (p)
	{
		ptr = new T(*p);
	}
}

template<class T>
PersonalPtr<T>::PersonalPtr(const PersonalPtr &m) : ptr(NULL)
{
	if (m.ptr)
	{
		ptr = new T(*m.ptr);
	}
}


template<class T>
template<class X>
PersonalPtr<T>::PersonalPtr(const PersonalPtr<X> &m) : ptr(NULL)
{
	if (m.ptr)
	{
		ptr = new T(*m.ptr);
	}
}

	
template<class T>
PersonalPtr<T>::~PersonalPtr()
{
	delete ptr;
}


template<class T>
inline void PersonalPtr<T>::Duplicate(T *p)
{
	if (ptr != p)
	{
		delete ptr;
		if (p)
		{
			ptr = new T(*p);
		}
		else
		{
			ptr = NULL;
		}
	}
}


template<class T>
inline void PersonalPtr<T>::Create()
{
	delete ptr;
	ptr = new T;
}


template<class T>
inline PersonalPtr<T> &PersonalPtr<T>::operator=(T *p)
{
	Duplicate(p);
	return *this;
}


template<class T>
inline PersonalPtr<T> &PersonalPtr<T>::operator=(const PersonalPtr &m)
{
	Duplicate(m.Get());
	return *this;
}


template<class T>
template<class X>
inline PersonalPtr<T> &PersonalPtr<T>::operator=(const PersonalPtr<X> &m)
{
	Duplicate(m.Get());
	return *this;
}


#endif // __PERSONAL_PTR_H__
