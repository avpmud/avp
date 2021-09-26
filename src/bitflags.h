/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 2007             [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Original LexiMUD Code                                                 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : bitflags.h                                                     [*]
[*] Usage: Lexi::BitFlags is an Enum-specific bitset implementation         [*]
\***************************************************************************/

#ifndef __BITFLAGS_H__
#define __BITFLAGS_H__

#include "lexistring.h"
#include "enum.h"
#include "SharedPtr.h"

namespace Lexi
{


class BitFlagsWrapper : public Shareable
{
public:
	virtual ~BitFlagsWrapper() {}
	virtual size_t		size() const = 0;
	virtual void		reset() {}
	virtual void		set(unsigned int i) {}
	virtual bool		test(unsigned int i) const = 0;
	virtual Lexi::String	print(char **flags) const = 0;
	virtual Lexi::String	print() const = 0;
	
	typedef SharedPtr<BitFlagsWrapper>	Ptr;
};


class BitFlagsAssignment
{
public:
	virtual ~BitFlagsAssignment() {}
	virtual void		assign(BitFlagsWrapper &bf) = 0;
};


enum BitFlagConstructorFakeParamType
{
	BitFlagConstructorFakeParam
};


template<size_t N>
class BitFlagsBase
{
protected:
	static const int NumWords = N;

	//	Basic internal functionality
	inline void SetBit(size_t bitNum)
	{
		bits[bitNum >> 5] |= ((unsigned int)1 << (bitNum & ((1 << 5) - 1)));
	}

	inline void ClearBit(size_t bitNum)
	{
		bits[bitNum >> 5] &= ~((unsigned int)1 << (bitNum & ((1 << 5) - 1)));
	}
	
	inline void FlipBit(size_t bitNum)
	{
		bits[bitNum >> 5] ^= ((unsigned int)1 << (bitNum & ((1 << 5) - 1)));
	}
	
	inline bool TestBit(size_t bitNum) const
	{
		return (bits[bitNum >> 5] & (1 << (bitNum & ((1 << 5) - 1)))) != 0;
	}
	
	inline unsigned int &GetWord(size_t wordNum) { return bits[wordNum]; }
	inline unsigned int GetWord(size_t wordNum) const { return bits[wordNum]; }
	
	static inline unsigned int CountBits(unsigned int v)
	{
		// see http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
		v -= (v >> 1) & 0x55555555; // temp  
		v = (v & 0x33333333) + ((v >> 2) & 0x33333333); // temp 
		return ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count 
	}

private:
	unsigned int	bits[NumWords];
};


template<>
class BitFlagsBase<1>
{
protected:
	static const int NumWords = 1;
	
	//	Basic internal functionality
	inline void SetBit(size_t bitNum)
	{
		bits |= (1 << bitNum);
	}

	inline void ClearBit(size_t bitNum)
	{
		bits &= ~(1 << bitNum);
	}
	
	inline void FlipBit(size_t bitNum)
	{
		bits ^= (1 << bitNum);
	}
	
	inline bool TestBit(size_t bitNum) const
	{
		return (bits & (1 << bitNum)) != 0;
	}
	
	inline unsigned int &GetWord(size_t wordNum) { return bits; }
	inline unsigned int GetWord(size_t wordNum) const { return bits; }
	
	static inline unsigned int CountBits(unsigned int v)
	{
		// see http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
		v -= (v >> 1) & 0x55555555; // temp  
		v = (v & 0x33333333) + ((v >> 2) & 0x33333333); // temp 
		return ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count 
	}

private:
	unsigned int	bits;
};


template <size_t N, typename Enum = unsigned int>
class BitFlags
{
private:
	//	Members
	template<size_t, typename> friend class BitFlags;
	static const size_t NumWords = (N == 0) ? 1 : (N + 31) / 32;
	unsigned int	bits[NumWords];

	inline size_t GetNumWords() const { return NumWords; }

	//	Basic internal functionality
	inline void SetBit(size_t bitNum)
	{
		bits[bitNum >> 5] |= ((unsigned int)1 << (bitNum & ((1 << 5) - 1)));
	}

	inline void ClearBit(size_t bitNum)
	{
		bits[bitNum >> 5] &= ~((unsigned int)1 << (bitNum & ((1 << 5) - 1)));
	}
	
	inline void FlipBit(size_t bitNum)
	{
		bits[bitNum >> 5] ^= ((unsigned int)1 << (bitNum & ((1 << 5) - 1)));
	}
	
	inline bool TestBit(size_t bitNum) const
	{
		return (bits[bitNum >> 5] & (1 << (bitNum & ((1 << 5) - 1)))) != 0;
	}
	
	static inline unsigned int CountBits(unsigned int v)
	{
		// see http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
		v -= (v >> 1) & 0x55555555; // temp  
		v = (v & 0x33333333) + ((v >> 2) & 0x33333333); // temp 
		return ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count 
	}
	
/*
	inline unsigned int FirstBit() const
	{
		for (unsigned int i = 0; i < NumWords; ++i)
		{
			unsigned int word = bits[i];
			if (!word)	continue;
			
			for (unsigned int n = 0; n < 32; ++n)
			{
				if (word & (1 << n))	return (i << 5) + n;
			}
		}
		return N;
	}
	
	inline unsigned int NextBit(unsigned int b) const
	{
		
	}
*/
public:
	typedef Enum value_type;
	typedef BitFlags<N, Enum> self;
	typedef self Mask;
	
	//	Constructors
	BitFlags()
	{ clear(); }
	
	BitFlags(BitFlagConstructorFakeParamType, value_type b)
	{ clear(); SetBit(b); }
	
	BitFlags(BitFlagConstructorFakeParamType, value_type b1, value_type b2)
	{ clear(); SetBit(b1); SetBit(b2); }
	
	BitFlags(BitFlagConstructorFakeParamType, value_type b1, value_type b2, value_type b3)
	{ clear(); SetBit(b1); SetBit(b2); SetBit(b3); }

	BitFlags(BitFlagConstructorFakeParamType, value_type b1, value_type b2, value_type b3, value_type b4)
	{ clear(); SetBit(b1); SetBit(b2); SetBit(b3); SetBit(b4); }

	BitFlags(BitFlagConstructorFakeParamType, value_type b1, value_type b2, value_type b3, value_type b4, value_type b5)
	{ clear(); SetBit(b1); SetBit(b2); SetBit(b3); SetBit(b4); SetBit(b5); }

	BitFlags(BitFlagConstructorFakeParamType, value_type b1, value_type b2, value_type b3, value_type b4, value_type b5, value_type b6)
	{ clear(); SetBit(b1); SetBit(b2); SetBit(b3); SetBit(b4); SetBit(b5); SetBit(b6); }

	BitFlags(BitFlagConstructorFakeParamType, value_type b1, value_type b2, value_type b3, value_type b4, value_type b5, value_type b6, value_type b7)
	{ clear(); SetBit(b1); SetBit(b2); SetBit(b3); SetBit(b4); SetBit(b5); SetBit(b6); SetBit(b7); }

	BitFlags(BitFlagConstructorFakeParamType, value_type b1, value_type b2, value_type b3, value_type b4, value_type b5, value_type b6, value_type b7, value_type b8)
	{ clear(); SetBit(b1); SetBit(b2); SetBit(b3); SetBit(b4); SetBit(b5); SetBit(b6); SetBit(b7); SetBit(b8); }
	
	//	Bitset-like functionality
	size_t				size() const { return N; }
	unsigned int		count() const
	{
		size_t n = 0;
		for (int i = 0; i < NumWords; ++i)
			n += CountBits(bits[i]);
		return n;
	}
	
	self &				clear() { memset(bits, 0, sizeof(bits)); return *this; }
	self &				set() { memset(bits, 0xFF, sizeof(bits)); return *this; }
	self &				reset() { memset(bits, 0, sizeof(bits)); return *this; }
	self &				flip()
	{
		for (int i = 0; i < NumWords; ++i)
			bits[i] = ~bits[i];
		return *this;
	}
	
	bool				any() const
	{
		for (int i = 0; i < NumWords; ++i)
			if (bits[i])
				return true;
		return false;
	}
	bool				none() const { return !any(); }
	bool				all() const 
	{
		for (int i = 0; i < NumWords; ++i)
			if (~bits[i])
				return false;
		return true;
	}
	
	self &				clear(value_type b) { ClearBit(b); return *this; }
	self &				set(value_type b) { SetBit(b); return *this; }
	self &				set(value_type b, bool v) { if (v) SetBit(b); else ClearBit(b); return *this; }
	self &				flip(value_type b) { FlipBit(b); return *this; }
	bool				test(value_type b) const { return TestBit(b); }
	
	self &				clear(const self &bs)
	{
		for (int i = 0; i < NumWords; ++i)
			bits[i] &= ~bs.bits[i];
		return *this;
	}
	
	self &				set(const self &bs)
	{
		for (int i = 0; i < NumWords; ++i)
			bits[i] |= bs.bits[i];
		return *this;
	}
	
	self &				flip(const self &bs)
	{
		for (int i = 0; i < NumWords; ++i)
			bits[i] ^= bs.bits[i];
		return *this;
	}

	bool				testForAny(const self &bs) const
	{
		for (int i = 0; i < NumWords; ++i)
		{
			//	If test set has bits in this word,
			//	then if we both share some bits, we match
			unsigned int testBits = bs.bits[i];
			if (testBits && (bits[i] & testBits))
				return true;
		}
		return false;
	}

	bool				testForAll(const self &bs) const
	{
		for (int i = 0; i < NumWords; ++i)
		{
			//	If test set has bits in this word,
			//	then the bits we share must exactly equal test set
			unsigned int testBits = bs.bits[i];
			if (testBits && ((bits[i] & testBits) != testBits))
				return false;
		}
		return true;
	}
	
	bool				testForNone(const self &bs) const { return !testForAny(bs); }
	
	Lexi::String		print(char **flags) const;
	Lexi::String		print() const;
	
	//	Operator overloads
						//	WARNING: This is very dangerous and results in some nasty
						//	behavior, especially if you have two similiar functions, one
						//	that expects an int and the other that expects a ConstWrapper
						//	or Wrapper!
//						operator bool() const { return any(); }
						
	bool				operator==(const self &bs)
	{
		for (int i = 0; i < NumWords; ++i)
			if (bits[i] != bs.bits[i])
				return false;
		return true;
	}
	bool				operator!=(const self &bs) { return !(*this == bs); }
	
	//	Generic Read from an untyped BitFlags
	template <size_t NumOtherBits>
	void				operator=(const BitFlags<NumOtherBits> &bs)
	{
		int totalWords = std::min(GetNumWords(), bs.GetNumWords()); //std::min(NumWords, (NumOtherBits == 0) ? 1 : ((NumOtherBits + 31) / 32));
		int i = 0;
		//      Copy bits up to the smaller of the two
		for (; i < totalWords; ++i)
			bits[i] = bs.bits[i];
		//      Clear the rest
		for (; i < NumWords; ++i)
			bits[i] = 0;
	}
	
	//	Generic Read from an untyped BitFlags
	template <size_t NumOtherBits, typename E>
	BitFlags(const BitFlags<NumOtherBits, E> &bs)
	{
		int totalWords = std::min(GetNumWords(), bs.GetNumWords()); //std::min(NumWords, (NumOtherBits == 0) ? 1 : ((NumOtherBits + 31) / 32));
		int i = 0;
		//      Copy bits up to the smaller of the two
		for (; i < totalWords; ++i)
			bits[i] = bs.bits[i];
		//      Clear the rest
		for (; i < NumWords; ++i)
			bits[i] = 0;
	}
	
	
	//	Generic Read from an untyped BitFlags
	template <size_t NumOtherBits, typename E>
	void				operator=(const BitFlags<NumOtherBits, E> &bs)
	{
		int totalWords = std::min(GetNumWords(), bs.GetNumWords()); //std::min(NumWords, (NumOtherBits == 0) ? 1 : ((NumOtherBits + 31) / 32));
		int i = 0;
		//      Copy bits up to the smaller of the two
		for (; i < totalWords; ++i)
			bits[i] = bs.bits[i];
		//      Clear the rest
		for (; i < NumWords; ++i)
			bits[i] = 0;
	}
	
//	void				operator=(const BitFlagsAssignment &asn) { asn.assign(Wrap()); }
	
	class reference
	{
	public:
		reference &			operator=(bool b) { bs.set(b); return *this; }
		reference &			operator=(const reference &rhs) { bs.set(v, rhs); return *this; }
		bool				operator~() const { return !bs.test(v); }
							operator bool() const { return bs.test(v); }
		reference &			flip() { bs.flip(v); return *this; }
	private:
		self &				bs;
		value_type			v;
		
		reference(self &inBs, value_type inVal) : bs(inBs), v(inVal) {}
		friend class BitFlags;
	};
	
	
	bool				operator[](value_type v) const { return test(v); }
	reference			operator[](value_type v) { return reference(*this, v); }
	
	self &				operator|=(const self &rhs)
	{
		for (int i = 0; i < NumWords; ++i)
			bits[i] |= rhs.bits[i];
		return *this;
	}

	self &				operator&=(const self &rhs)
	{
		for (int i = 0; i < NumWords; ++i)
			bits[i] &= rhs.bits[i];
		return *this;
	}

	self &				operator^=(const self &rhs)
	{
		for (int i = 0; i < NumWords; ++i)
			bits[i] ^= rhs.bits[i];
		return *this;
	}
	
	self				operator|(const self &rhs) const { return self(*this) |= rhs; }
	self				operator&(const self &rhs) const { return self(*this) &= rhs; }
	self				operator^(const self &rhs) const { return self(*this) ^= rhs; }
	self				operator~() const { return self(*this).flip(); }
	
/*
	self &				operator+(Enum e) const { return self(*this).set(e); }
	self &				operator-(Enum e) const { return self(*this).clear(e); }

	self &				operator+(const self &bs) const { return self(*this).set(bs); }
	self &				operator-(const self &bs) const { return self(*this).clear(bs); }

	self &				operator+=(Enum e) { return set(e); }
	self &				operator-=(Enum e) { return clear(e); }

	self &				operator+=(const self &bs) { return set(bs); }
	self &				operator-=(const self &bs) { return clear(bs); }
*/

	//	forward iterator
/*	class iterator
	{
	public:
		iterator &		operator++()
		{
			increment();
			return *this;
		}
		
		iterator		operator++(int)
		{
			iterator tmp = *this;
			increment();
			return tmp;
		}
		
		iterator &		operator=(const iterator &rhs)
		{
			
			return *this;
		}
		
		bool			operator==(const iterator &rhs) const 
		{
			return &bs == &rhs.bs && bit == rhs.bit;
		}
		bool			operator!=(const iterator &rhs) const { return !(*this == rhs);  }
		
		reference		operator*() const { return bit; }
	
	private:
		self &			bs;
		unsigned int	bit;
		
		iterator(self &inBs, unsigned int inBit) : bs(inBs), bit(inBit) {}
		
		iterator();
		
		void			increment()
		{
			
		}
		
		friend class BitFlags;
	};
*/

	//	I/O interfaces
	class Wrapper : public BitFlagsWrapper
	{
	public:
		Wrapper(self &inBs) : bs(inBs) {}
		
		virtual size_t		size() const { return bs.size(); }
		virtual void		reset() { return bs.reset(); }
		virtual void		set(unsigned int i) { if (i < size()) bs.set((value_type)i); }
		virtual bool		test(unsigned int i) const { return (i < size()) ? bs.test((value_type)i) : false; }
		virtual Lexi::String	print(char **flags) const { return bs.print(flags); }
		virtual Lexi::String	print() const { return bs.print(); }
		
	protected:
		self &				bs;
	};


	class ConstWrapper : public BitFlagsWrapper
	{
	public:
		ConstWrapper(const self &inBs) : bs(inBs) {}
		
		virtual size_t		size() const { return bs.size(); }
		virtual bool		test(unsigned int i) const { return (i < size()) ? bs.test((value_type)i) : false; }
		virtual Lexi::String	print(char **flags) const { return bs.print(flags); }
		virtual Lexi::String	print() const { return bs.print(); }
		
	protected:
		const self &		bs;

	private:
		ConstWrapper();
	};
	
	BitFlagsWrapper::Ptr	Wrap() { return new Wrapper(*this); }
	BitFlagsWrapper::Ptr	Wrap() const { return new ConstWrapper(*this); }	
};


extern Lexi::String PrintBitFlags(BitFlagsWrapper::Ptr bf, char **flags);
extern Lexi::String PrintBitFlags(BitFlagsWrapper::Ptr bf, const EnumNameResolverBase &nameResolver);


template<size_t N, typename Enum> 
Lexi::String BitFlags<N, Enum>::print(char **flags) const
{
	return PrintBitFlags(Wrap(), flags);
}


template<size_t N, typename Enum> 
Lexi::String BitFlags<N, Enum>::print() const
{
	return PrintBitFlags(Wrap(), GetEnumNameResolver<Enum>());
}


}	//	namespace Lexi

#define MAKE_BITSET(BitSet, ...)		BitSet(Lexi::BitFlagConstructorFakeParam, __VA_ARGS__)

#endif
