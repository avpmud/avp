/*************************************************************************
*   File: array.h                    Part of Aliens vs Predator: The MUD *
*  Usage: Header and inline code for arrays                              *
*************************************************************************/

#ifndef __ARRAY_H__
#define __ARRAY_H__

#include "types.h"
#include "utils.h"

class	Array;
class	ArrayIterator;

typedef SInt32 (*ArrayFindFunc)(SInt32 index, Ptr element, Ptr extra);

class Array {
public:
	inline			Array(Array *array, SInt32 size, SInt32 chunk);
	inline			~Array(void);
	
	inline void		Insert(Ptr element);
	inline void		Delete(Ptr element);
	inline SInt32		Find(ArrayFindFunc func, Ptr extra);
	inline void		Collapse(void);
	inline void		MakeRoom(SInt32 size);
	inline void		FixEnds(void);
	
	SInt32			size, chunk;
	SInt32			first, last;
	SInt32			iters;
	bool			dirty;
	
	Ptr *			elements;
};


class ArrayIterator {
public:
	inline			ArrayIterator(Array *array);
	inline			~ArrayIterator(void);
	
	inline Ptr		First(void);
	inline Ptr		Last(void);
	inline Ptr		Next(void);
	inline Ptr		Prev(void);
	
	Array *			array;
	SInt32			current;
};


//	INLINE FUNCTIONS
Array::Array(Array *array, SInt32 size, SInt32 chunk) {
	this->size = size;
	this->chunk = chunk;
	this->elements = new Ptr[size];
	this->first = this->last = NULL;
	this->iters = 0;
	this->dirty = 0;
	
	if (array) delete array;
}


Array::~Array(void) {
	delete this->elements;
}


void Array::Insert(Ptr element) {
	this->MakeRoom(this->last);
	this->elements[this->last++] = element;
}


void Array::Delete(Ptr element) {
	SInt32	i;
	Ptr *	ptr;
	
	for (i = this->first, ptr = this->elements; i < this->last; ++i, ++ptr) {
		if (*ptr == element) {
			*ptr = NULL;
			this->dirty = true;
		}
	}
}


SInt32 Array::Find(ArrayFindFunc func, Ptr extra) {
	SInt32	i;
	Ptr *	elements = this->elements;
	
	for (i = this->first; i < this->last; ++i) {
		if (elements[i] && !func(i, elements[i], extra))
			return i;
	}
	
	return -1;
}


void Array::Collapse(void) {
	SInt32	src, dst;
	Ptr *	elements = this->elements;
	
	if (!this->dirty)
		return;
	
	for (dst = 0, src = this->first; src < this->last; src++)
		if (elements[src])
			elements[dst++] = elements[src];
	
	this->first = 0;
	this->last = 0;
	this->dirty = false;
}


ArrayIterator::ArrayIterator(Array *array) {
	array->FixEnds();
	
	this->array = array;
	this->current = array->first;
	
	array->iters++;
}


ArrayIterator::~ArrayIterator(void) {
	if (--this->array->iters <= 0) {
		this->array->iters = 0;
		this->array->Collapse();
	}
}


Ptr ArrayIterator::First(void) {
	this->array->FixEnds();
	this->current = this->array->first - 1;
	return this->Next();
}


Ptr ArrayIterator::Last(void) {
	this->array->FixEnds();
	this->current = this->array->last;
	return this->Prev();
}


Ptr ArrayIterator::Next(void) {
	Array *		array = this->array;
	Ptr *		elements = array->elements;
	
	while (++this->current < array->last) {
		if (elements[this->current])
			return elements[this->current];
	}
	return NULL;
}


Ptr ArrayIterator::Prev(void) {
	Array *		array = this->array;
	Ptr *		elements = array->elements;
	
	while (--this->current >= array->first) {
		if (elements[this->current])
			return elements[this->current];
	}
	return NULL;
}


void Array::MakeRoom(SInt32 size) {
	SInt32		newSize;
	SInt32		i;
	Ptr *		oldElements = this->elements;
	
	for (newSize = this->size; newSize <= size; newSize += this->chunk);
	
	if (newSize != this->size) {
		this->elements = new Ptr[newSize];
		for (i = 0; i < MIN(size, newSize); i++)
			this->elements[i] = oldElements[i];
		delete oldElements;
	}
}


void Array::FixEnds(void) {
	Ptr *		elements = this->elements;
	
	while ((this->first < this->last) && !elements[this->first])
		this->first++;
	
	while ((this->last > this->first) && !elements[this->last - 1])
		this->last--;
}


#define AITER_F(iter, var, type, arr) {								\
	ArrayIterator *iter = new ArrayIterator(arr);					\
	for (var = (type)iter->First(); var; var = (type)iter->Next())

#define	AITER_B(iter, var, type, arr) {								\
	ArrayIterator *iter = new ArrayIterator(arr);					\
	for (var = (type)iter->Last(); var; var = (type)iter->Prev())
	
#define END_AITER(iter)	\
	delete iter;		\
	}

#endif

