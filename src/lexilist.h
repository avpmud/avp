
#ifndef __LEXILIST_H__
#define __LEXILIST_H__


#include <iterator>

#include <cassert>
#include <cstddef>

namespace Lexi
{
	template<class T> class List;
}


template <class T>
class Lexi::List
{
public:
	typedef Lexi::List<T>	__self;
	
	typedef T			value_type;
	typedef T *			pointer;
	typedef T &			reference;
	typedef const T *	const_pointer;
	typedef const T &	const_reference;
	typedef std::size_t		size_type;
	typedef std::ptrdiff_t	difference_type;
	
protected:
	struct _node;
	typedef struct _node *	node_pointer;
	
	//	By using a 'base' that means the head node only has prev/next!
	struct node_base
	{
							node_base() : prev(NULL), next(NULL), refs(0), removed(false) {}
							
		node_pointer		prev;
		node_pointer		next;
							
		short				refs;
		bool				removed;
	};
	
	struct _node : public node_base
	{
							_node() {}
							_node(const_reference d) : data(d) {}
		
		value_type			data;
		
		//	GCC requires use of 'this->' or it fails to compile, complaining that next/prev are not declared in this scope.
		void				Remove()	{ this->next->prev = this->prev; this->prev->next = this->next; delete this; }
		void				Retain()	{ ++this->refs; }
		void				Release()	{ if (--this->refs <= 0 && this->removed) Remove(); }
	};


	struct generic_iterator_base
	{
	public:
		typedef std::bidirectional_iterator_tag	iterator_category;
		typedef std::size_t		size_type;
		typedef std::ptrdiff_t	difference_type;

		bool operator ==(const generic_iterator_base &i) const { return node == i.node; }
		bool operator !=(const generic_iterator_base &i) const { return node != i.node; }

		reference operator*() const	{ return node->data; }
		pointer operator->() const	{ return &node->data; }
		
		generic_iterator_base &operator=(const generic_iterator_base &x)
		{
			if (node)	node->Release();
			node = x.node;
			if (node)	node->Retain();
			
			return *this;
		}
		
	protected:
		void increment()
		{
			node_pointer tmp = node;
			node = node->next;
			tmp->Release();
			
			if (node->removed)
			{
				do
				{
					node = node->next;
				} while (node->removed);
			}
			node->Retain();
		}
		
		
		void decrement()
		{
			node_pointer tmp = node;
			node = node->prev;
			tmp->Release();
			
			while (node->removed)
				node = node->prev;
			node->Retain();
		}
	
		generic_iterator_base() : node(NULL) { }
		generic_iterator_base(const generic_iterator_base &i) : node(i.node) { node->Retain(); }
		~generic_iterator_base() { if (node) node->Release(); }
		generic_iterator_base(node_pointer n) : node(n) { node->Retain(); }
		node_pointer		node;
		
		//void clear() { if (node) { node->Release(); node = NULL;} }
	
		template <typename _val, typename _ref, typename _ptr>
		friend struct generic_iterator;
		friend class List;
	};

public:
//	struct const_iterator;

	template <typename _val, typename _ref, typename _ptr>
	struct generic_iterator : public generic_iterator_base
	{
	public:
		typedef generic_iterator<_val,_val&,_val*>					iterator;
		typedef generic_iterator<_val, const _val &, const _val *>	const_iterator;
		typedef generic_iterator<_val,_ref,_ptr>					__self;

		typedef _val	value_type;
		typedef _ptr	pointer;
		typedef _ref	reference;

		generic_iterator() {}
		generic_iterator(const iterator &i) : generic_iterator_base(i) {}
		~generic_iterator() {}
		
		generic_iterator &operator++()
		{
			generic_iterator_base::increment();
			return *this;
		}
		
		generic_iterator operator++(int)
		{
			generic_iterator tmp = *this;
			generic_iterator_base::increment();
			return tmp;
		}
		
		generic_iterator &operator--()
		{
			generic_iterator_base::decrement();
			return *this;
		}
		
		generic_iterator operator--(int)
		{
			generic_iterator tmp = *this;
			generic_iterator_base::decrement();
			return tmp;
		}
		
		generic_iterator &operator=(const iterator &x)
		{
			generic_iterator_base::operator=(x);
			return *this;
		}
		
	protected:
		explicit generic_iterator(node_pointer n) : generic_iterator_base(n) {}
		
		friend class List;
	};

	typedef generic_iterator<value_type, reference, pointer>				iterator;
	typedef generic_iterator<value_type, const_reference, const_pointer>	const_iterator;

	typedef std::reverse_iterator<iterator>			reverse_iterator;
	typedef std::reverse_iterator<const_iterator>	const_reverse_iterator;

	friend struct generic_iterator_base;
	
public:
	~List() { clear(); }
	
protected:
	node_base			node;
	size_type			count;
	
	iterator  		    advance_to(size_type p)
	{
		size_type sz = size();
		iterator i;
		if (p <= sz/2)
		{
			i = begin();
			std::advance(i, difference_type(p));
		}
		else
		{
			i = end();
			std::advance(i, difference_type(p - sz));
		}
		return i;
	}
	const_iterator advance_to(size_type p) const {return const_cast<List&>(*this).advance_to(p);}
	
	size_type &			sz() { return count; }
	const size_type &	sz() const { return count; }
	
	void Start()
	{
		node.refs = 1;	//	Prevent auto-removal
		node.prev = node.next = (node_pointer)&node;
		count = 0;
	}
	
	node_pointer FirstNode() const
	{
		node_pointer n = node.next;
		
		while (n->removed && n != &node)
			n = n->next;
		
		return n;
	}
	
public:
	iterator		begin()			{ return iterator(FirstNode());		}
	const_iterator	begin() const	{ return const_iterator(FirstNode());	}
	
	iterator		end() 			{ return iterator((node_pointer)&node);	}
	const_iterator	end() const		{ return const_iterator((node_pointer)&node);	}
	
	reverse_iterator		rbegin() 		{ return reverse_iterator(end());		}
	const_reverse_iterator	rbegin() const	{ return const_reverse_iterator(end());	}
	reverse_iterator		rend() 			{ return reverse_iterator(begin());		}
	const_reverse_iterator	rend() const	{ return const_reverse_iterator(begin());	}
	
	size_type		size() const	{ return sz();						}
	size_type		max_size() const{ return size_type(-1);				}
	bool			empty() const	{ return count == 0;				}
	
	reference		front()			{ return node.next->data;			}
	const_reference	front() const	{ return node.next->data;			}
	reference		back()			{ return node.prev->data;			}
	const_reference	back() const	{ return node.prev->data;			}
	
	void swap(__self &x) {
		std::swap(node, x.node);
		std::swap(count, x.count);
	}
	
	
	iterator			insert(iterator pos, const_reference x)
	{
		node_pointer	tmp = new _node(x);
		tmp->next = pos.node;
		tmp->prev = pos.node->prev;
		tmp->prev->next = tmp;
		pos.node->prev = tmp;
		
		++count;
		
		return iterator(tmp);
	}
	
	
	iterator			insert(iterator pos)
	{
		node_pointer	tmp = new _node;
		tmp->next = pos.node;
		tmp->prev = pos.node->prev;
		tmp->prev->next = tmp;
		pos.node->prev = tmp;
		
		++count;
		
		return iterator(tmp);
	}
	
	
	template <class Iter>
	void				insert(iterator pos, Iter first, Iter last)
	{
		for (; first != last; ++first)
			insert(pos, *first);
	}


	void				insert(iterator pos, size_type n, const_reference x) {
		for(; n >0; --n)
			insert(pos, x);
	}
	
	
	//	WARNING: Don't use begin() here, if you push_front() to a list
	//	while an iterator is on the first item that has been removed,
	//	the loop become infinite loop
	void				push_front(const_reference x)	{ insert(iterator(node.next), x);					}
	void				push_front()					{ insert(iterator(node.next) /* Don't use begin(); if on  */);						}
	void				push_back(const_reference x)	{ insert(end(), x);						}
	void				push_back()						{ insert(end());						}
	
	iterator			erase(iterator pos)
	{
		if (pos.node == &node)	return pos;
		
		node_pointer next = pos.node->next;
		pos.node->removed = true;
		--count;

		return iterator(next);
	}
	
	
//	iterator			erase(iterator first, iterator last) { while (first != last) erase(first++); return last; }
	iterator			erase(iterator first, iterator last)
	{
		if (first.node == &node)	return first;
		
		while (first != last)
		{
			first.node->removed = true;
			++first;
			--count;
		}

		return last;
	}
	
	void clear()
	{
		node_pointer	n = (node_pointer)&node;
		node_pointer	cur = node.next;
		while (cur != n)
		{
			node_pointer tmp = cur;
			cur = cur->next;
			if (tmp->refs)	tmp->removed = true;
			else			tmp->Remove();
		}
		
		count = 0;
	}
	
	
	void			resize(size_type newsize, const_reference value) {
		if (newsize > count)		insert(end(), newsize - count, value);
		else if (newsize < count)	erase(advance_to(count), end());
	}
	
	
	void			resize(size_type newsize)
	{
		resize(newsize, value_type());
	}
	
	void			pop_front()			{ erase(begin());					}
	void			pop_back()			{ iterator tmp(end()); erase(--tmp);}
	
	List(size_type sz, const_reference val)	{ Start(); insert(begin(), sz, val);	}
	explicit List(size_type sz)				{ Start(); insert(begin(), sz, T());	}
	explicit List()							{ Start();							}
	
	template <class Iter>
	List(Iter first, Iter last) {
		Start();
		insert(begin(), first, last);
	}
	
	List(const __self &x)					{ Start(); insert(begin(), x.begin(), x.end()); }
	
	
	void			assign(size_type n, const_reference val) {
		iterator i = begin();
		for (; i != end() && n > 0; ++i, --n)
			*i = val;
		if (n > 0)	insert(end(), n, val);
		else		erase(i, end());
	}
	
	template <class Iter>
	void assign(Iter first2, Iter last2) {
/*		typedef typename _Is_integer<Iter>::_Integral _Integral;
		_assign_dispatch(first, last, _Integral());
*/
		iterator first1 = begin(), last1 = end();
		for (; first1 != last1 && first2 != last2; ++first1, ++first2)
			*first1 = *first2;
		if (first2 == last2)	erase(first1, last1);
		else					insert(last1, first2, last2);
	}
	
/*	template <class _Integer>
	void _assign_dispatch(_Integer n, _Integer val, __true_type) {
		assign((size_type)n, (_T) val);
	}
	
	template <class _Iter>
	void _assign_dispatch(_Iter first2, _Iter last2, __false_type) {
		iterator first1 = begin(), last1 = end();
		for (; first1 != last1 && first2 != last2; ++first1, ++first2)
			*first1 = *first2;
		if (first2 == last2)	erase(first1, last1);
		else					insert(last1, first2, last2);
	}
*/	
	__self &			operator=(const __self &x)
	{
		if (this != &x)
		{
			assign(x.begin(), x.end());
		}
		return *this;
	}
	
protected:
	void			transfer(node_base *pos, node_base *first, node_base *last)
	{
		if (pos != last)
		{
			//	Remove [first, last] from old position
			first->prev->next = last->next;
			last->next->prev = first->prev;
			
			// Splice [first, last] into new position
			pos->prev->next = (node_pointer)first;
			first->prev = pos->prev;
			pos->prev = (node_pointer)last;
			last->next = (node_pointer)pos;
		}
	}

public:
	void			splice(iterator pos, __self &x)
	{
		if (!x.empty())
		{
			transfer(pos.node, x.node.next, x.node.prev);
			sz() += x.sz();
			x.sz() = 0;
		}
	}
	void			splice(iterator pos, __self &x, iterator i)
	{
		if (this != &x)
		{
			--x.sz();
			++sz();	
		}
		
		node_base *p = pos.node;
		node_base *s1 = i.node;
		if (s1 == p->prev || s1 == p)
			return;
		transfer(p, s1, s1);
	}

	void			splice(iterator pos, __self &x, iterator first, iterator last)
	{
		if (first == last)
			return;
		
		if (this != &x)
		{
			size_type delta = (size_type)std::distance(first, last);
			x.sz() -= delta;
			sz() += delta;
		}
		
		node_base *s1 = first.node;
		node_base *s2 = last.node->prev;
		transfer(pos.node, s1, s1);
	}


	void				remove(const_reference value)
	{
		node_pointer	cur = node.next;
		node_pointer	n = (node_pointer)&node;
		
		while (cur != n)
		{
			node_pointer next = cur->next;
			if (!cur->removed && cur->data == value)
			{
				if (cur->refs)	cur->removed = true;
				else			cur->Remove();
				--count;
			}
			cur = next;
		}
	}
	

	void				remove_one(const_reference value)
	{
		node_pointer	cur = node.next;
		node_pointer	n = (node_pointer)&node;
		
		while (cur != n)
		{
			node_pointer next = cur->next;
			if (!cur->removed && cur->data == value)
			{
				if (cur->refs)	cur->removed = true;
				else			cur->Remove();
				--count;
				return;
			}
			cur = next;
		}
	}
	
	
	template<class Predicate>
	void			remove_if(Predicate pred)
	{
		node_pointer	cur = node.next;
		node_pointer	n = (node_pointer)&node;
		
		while (cur != n)
		{
			node_pointer next = cur->next;
			if (!cur->removed && pred(cur->data))
			{
				if (cur->refs)	cur->removed = true;
				else			cur->Remove();
				--count;
			}
			cur = next;
		}
	}
	
	
/*	void			unique() { unique(std::equal_to<value_type>()); }
	
	template<class BinaryPredicate> void unique(BinaryPredicate pred)
	{
		for (iterator i = begin(), e = end(); i != e; )
		{
			iterator j = i;
			for (++j; j != e; ++j)
				if (!pred(*i, *j))
					break;
			++i;
			if (i != j)
				i = erase(i, j);
		}
	}
*/	

	void			reverse()
	{
		if (sz() < 2)
			return;
		
		node_base *e = &node;
		for (node_base *i = e->next; i != e; i = i->prev)
			std::swap(i->prev, i->next);
		std::swap(e->prev, e->next);
	}
	

	void				merge(__self &x) { merge(x, std::less<value_type>()); }

	template <class Compare>
	void			merge(__self &x, Compare comp)
	{
		if (this == &x)
			return;
		
		iterator first1 = begin(),
				 last1 = end(),
				 first2 = x.begin(),
				 last2 = x.end();
		for (; first1 != last1 && first2 != last2; ++first1)
		{
			if (comp(*first2, *first1))
			{
				iterator next = first2;
				size_t	count = 1;
				for (++next; next != last2; ++next, ++count)
					if (!comp(*next, *first1))
						break;
				
				transfer(first1.node, first2.node, next.node->prev);
				
				x.sz() -= count;
				sz() += count;
				
				first2 = next;
			}
		}
		if (first2 != last2)
			splice(first1, x);
	}

	void			sort() { sort(std::less<value_type>()); }

	template <class Compare>
	void			sort(Compare comp)
	{
		switch (sz())
		{
			case 0:
			case 1:
				break;
			case 2:
			{
				iterator i = begin();
				iterator j = i;
				++j;
				if (comp(*j, *i))
				{
					std::swap(i.node->prev, i.node->next);
					std::swap(j.node->prev, j.node->next);
					std::swap(node.prev, node.next);
				}
			}
			break;
			
			default:
			{
				iterator	i = begin();
				size_type	lower_size = sz() / 2;
				
				std::advance(i, lower_size);
				
				__self upper_half;
				
				node_base *s1 = i.node;
				node_base *s2 = node.prev;
				
				transfer(&upper_half.node, s1, s2);
				
				upper_half.sz() = sz() - lower_size;
				sz() = lower_size;
				
				sort(comp);
				upper_half.sort(comp);
				
				merge(upper_half, comp);
			}
			break;
		}
	}
};


template <class T>
inline bool operator==(const Lexi::List<T> &x, const Lexi::List<T> &y)
{
	typedef typename Lexi::List<T>::const_iterator	const_iterator;
	const_iterator	end1 = x.end(), end2 = y.end(),
					i1 = x.begin(), i2 = y.begin();
	
	while (i1 != end1 && i2 != end2 && *i1 == *i2) {
		++i1;
		++i2;
	}
	return i1 == end1 && i2 == end2;
}

/*
template <class T>
inline bool operator<(const List<T> &x, const List<T> &y) {
	return lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
}
*/

#endif
