
#ifndef __WEAK_PTR_H__
#define __WEAK_PTR_H__

class WeakPtrBase
{
protected:
	WeakPtrBase() : m_Prev(NULL), m_Next(NULL) {}
	~WeakPtrBase() { Unlink(); }
	
	WeakPtrBase *		m_Prev;
	WeakPtrBase *		m_Next;
	
	void				Link(class WeakPointable *);
		
	void				Unlink()
	{
		if (!m_Prev || !m_Next)
			return;
		m_Prev->m_Next = m_Next;
		m_Next->m_Prev = m_Prev;
		m_Prev = m_Next = NULL;
	}
	
	friend class WeakPointable;

private:
	WeakPtrBase(const WeakPtrBase &);
	void operator=(const WeakPtrBase &);
};


template<class T>
class WeakPtr : protected WeakPtrBase
{
public:
	typedef T			value_type;
	typedef T *			pointer;
	typedef const T *	const_pointer;
	typedef T &			reference;
	typedef const T &	const_reference;
	typedef WeakPtr<T>	__self;
	
	
	WeakPtr(pointer p = NULL) : ptr(p)
	{
		if (p)	Link(p);
	}
	
	
	WeakPtr(const __self &wp) : ptr(wp.ptr)
	{
		if (ptr) Link(ptr);
	}
	
	
	__self &operator=(pointer p)
	{
		Assign(p);
		return *this;
	}


	__self &operator=(const __self &wp)
	{
		Assign(wp.ptr);
		return *this;
	}
	
	
	reference operator*() const { return *ptr; }
	pointer operator->() const { return ptr; }
	
	operator bool() const { return ptr != NULL; }
	operator pointer() const { return ptr; }

private:
	pointer				ptr;


	void Assign(pointer p)
	{
		if (ptr)	Unlink();
		ptr = p;
		if (ptr)	Link(ptr);
	}
	
	
	friend class		WeakPointable;
};


class WeakPointable
{
protected:
						WeakPointable() { m_WeakBase.m_Prev = m_WeakBase.m_Next = &m_WeakBase; }
						WeakPointable(const WeakPointable &) { m_WeakBase.m_Prev = m_WeakBase.m_Next = &m_WeakBase; }
						
	void				operator=(const WeakPointable &) { /* Nothing */}
						
	virtual				~WeakPointable();
	
	WeakPtrBase			m_WeakBase;
	
	friend class WeakPtrBase;
};


inline WeakPointable::~WeakPointable()
{
	WeakPtrBase *	ptr = m_WeakBase.m_Next;
	WeakPtrBase *	end = &m_WeakBase;
	while (ptr != end)
	{
		WeakPtrBase *	next = ptr->m_Next;
		ptr->m_Prev = ptr->m_Next = NULL;
		static_cast<WeakPtr<WeakPointable> *>(ptr)->ptr = NULL;
		ptr = next;
	}
}


inline void WeakPtrBase::Link(class WeakPointable *ptr)
{
	WeakPtrBase &head = ptr->m_WeakBase;
	m_Next = &head;
	m_Prev = head.m_Prev;
	head.m_Prev = m_Prev->m_Next = this;
}


#endif	//	__WEAK_PTR_H__
