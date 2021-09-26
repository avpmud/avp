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
[*] File : ManagedPtr.h                                                   [*]
[*] Usage: Memory managing pointer wrapper                                [*]
[*]        Automatically deletes the data it points to on assignment or   [*]
[*]        destruction                                                    [*]
\***************************************************************************/


#ifndef __MANAGED_PTR_H__
#define __MANAGED_PTR_H__

template <class T>
class ManagedPtr
{
public:
	typedef T value_type;
	
	explicit ManagedPtr(T *p = 0) : ptr(p) {}
	ManagedPtr(ManagedPtr &m) : ptr(m.Release()) {}
	template<class X> ManagedPtr(ManagedPtr<X> &m) : ptr(m.Release()) {}
	
	ManagedPtr<T> &operator=(T *p);
	ManagedPtr<T> &operator=(ManagedPtr &m);
	template<class X>ManagedPtr<T> &operator=(ManagedPtr<X> &m);
	
	~ManagedPtr();
	
//	T *Get() const { return ptr; }
	T *Release();
	void Reset(T *p = NULL);
	
	T &operator*() const { return *ptr; }
	T *operator->() const { return ptr; }
	operator bool() const { return ptr != NULL; }
	
	operator T*() const { return ptr; }

private:
	T *ptr;
	
	ManagedPtr(const ManagedPtr &m);
	ManagedPtr<T> &operator=(const ManagedPtr &m);
};


template<class T>
ManagedPtr<T>::~ManagedPtr()
{
	delete ptr;
}


template<class T>
inline T *ManagedPtr<T>::Release()
{
	T *tmp = ptr;
	ptr = NULL;
	return tmp;
}


template<class T>
inline void ManagedPtr<T>::Reset(T *p)
{
	if (ptr != p)
	{
		delete ptr;
		ptr = p;
	}
}


template<class T>
inline ManagedPtr<T> &ManagedPtr<T>::operator=(T *p)
{
	Reset(p);
	return *this;
}


template<class T>
inline ManagedPtr<T> &ManagedPtr<T>::operator=(ManagedPtr &m)
{
	Reset(m.Release());
	return *this;
}


template<class T>
template<class X>
inline ManagedPtr<T> &ManagedPtr<T>::operator=(ManagedPtr<X> &m)
{
	Reset(m.Release());
	return *this;
}


#endif // __MANAGED_PTR_H__
