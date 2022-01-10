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
[*] File : lexistring.h                                                   [*]
[*] Usage: Lexi::String is an std::string compatible, case-insensitive,   [*]
[*]        reimplementation, optimized for speed and memory               [*]
\***************************************************************************/

#ifndef __LEXISTRING_H__
#define __LEXISTRING_H__

#if !defined(__GNUC__)
#define __attribute__(x)	/* nothing */
#endif

#include <climits>
#include <string.h>
#include <list>
#include <algorithm>	//	Too much depends on this from <string>

namespace Lexi
{


#include <cstddef>


//#define TRACK_STRING_BUFFERING


class String
{
public:
	// types:
//	typedef	char_traits		traits_type;
//	typedef typename char_traits::char_type	value_type;
	typedef char			value_type;
//	typedef	Allocator		allocator_type;
	typedef size_t			size_type;
	typedef std::ptrdiff_t	difference_type;
	typedef char &			reference;
	typedef const char &	const_reference;
	typedef char *			pointer;
	typedef const char *	const_pointer;
						
	typedef pointer			iterator;
	typedef const_pointer	const_iterator;
	typedef std::reverse_iterator<iterator>			reverse_iterator;
	typedef std::reverse_iterator<const_iterator>	const_reverse_iterator;
	
	static const size_type npos = size_type(-1);
	
#if defined(TRACK_STRING_BUFFERING)
	explicit String();// : str(buf), buffered(true), len(0), cap(0) {} /* nulls the buf too */ 
#else
	explicit String() : str(buf), buffered(true), managed(false), len(0), cap(0) {} /* nulls the buf too */ 
#endif
	String(const String &s);
	String(const String &s, size_type pos, size_type n = npos);
	String(const char * s);
	String(const char * s, size_type n);
	String(size_type n, char c) : str(buf), buffered(true), len(0), cap(0) 
		{ assign(n, c); }
	
	~String();
	
	String &			operator=(const String &s) { return assign(s); }
	String &			operator=(const char *s) { return assign(s); }
	String &			operator=(char c) { str[0] = c; str[1] = 0; len = 1; return *this; }
	
	iterator			begin() { return str; }
	const_iterator		begin() const { return str; }
	iterator			end() { return str + len; }
	const_iterator		end() const { return str + len; }
	
	reverse_iterator			rbegin() { return reverse_iterator(end()); }
	const_reverse_iterator		rbegin() const { return const_reverse_iterator(end()); }
	reverse_iterator			rend() { return reverse_iterator(begin()); }
	const_reverse_iterator		rend() const { return const_reverse_iterator(begin()); }
	
	size_type			size() const { return len; }
	size_type			length() const { return len; }
	size_type			max_size() const { return (LONG_MAX / 2) - 1; }
	
	void				resize(size_type n, char c);
	void				resize(size_type n) { resize(n, 0); }

	size_type			buffered_capacity() const { return buffered_cap; }
	size_type			capacity() const { return (is_buffered() ? buffered_capacity() : cap) - 1; }
	void				reserve(size_type res = 0);
	void				clear() { resize(0); }
	bool				empty() const { return size() == 0; }
	
	reference			operator[](size_type pos) { return str[pos]; }
	const_reference		operator[](size_type pos) const { return str[pos]; }
	reference			at(size_type pos) { return str[pos]; }
	const_reference		at(size_type pos) const { return str[pos]; }
	
	String &			operator+=(const String &s) { return append(s); }
	String &			operator+=(const char * s) { return append(s); }
	String &			operator+=(char c) { return append(1, c); }

	String &			append(const String &s) { return replace(size(), 0, s); }
	String &			append(const String &s, size_type pos, size_type n) { return replace(size(), 0, s, pos, n); }
	String &			append(const char * s) { return replace(size(), 0, s); }
	String &			append(const char * s, size_type n) { return replace(size(), 0, s, n); }
	String &			append(size_type n, char c) { return replace(size(), 0, n, c); }

	void				push_back(char c) { append(1, c); }
	void				pop_back();
	
	String &			assign(const String & s);
	String &			assign(const String & s, size_type pos, size_type n) { return replace(0, size(), s, pos, n); }
	String &			assign(const char* s, size_type n) { return replace(0, size(), s, n); }
	String &			assign(const char* s) { return replace(0, size(), s); }
	String &			assign(size_type n, char c) { return replace(0, size(), n, c); }
	String &			insert(size_type pos, const String & s) { return replace(pos, 0, s); }
	String &			insert(size_type pos, const String & s, size_type pos2, size_type n) { return replace(pos, 0, s, pos2, n); }
	String &			insert(size_type pos, const char * s, size_type n) { return replace(pos, 0, s, n); }
	String &			insert(size_type pos, const char * s) { return replace(pos, 0, s); }
	String &			insert(size_type pos, size_type n, char c) { return replace(pos, 0, n, c); }
	iterator			insert(iterator p, char c);
	void				insert(iterator p, size_type n, char c) { replace(size_type(p - begin()), 0, n, c); }
	String &			erase(size_type pos = 0, size_type n = npos) { return replace(pos, n, 0, (char)0); }
	iterator			erase(iterator position) { replace(size_type(position - begin()), 1, 0, (char)0); return position; }
	iterator			erase(iterator first, iterator last) { replace(size_type(first - begin()), size_type(last - first), 0, (char)0); return first; }
//	String &			replace(size_type pos1, size_type n1, const std::string& s);
//	String &			replace(size_type pos1, size_type n1, const std::string& s, size_type pos2, size_type n2);
	String &			replace(size_type pos1, size_type n1, const String& s);
	String &			replace(size_type pos1, size_type n1, const String& s, size_type pos2, size_type n2);
	String &			replace(size_type pos, size_type n1, const char* s)
		{ return do_replace(pos, n1, s, s + strlen(s) ); }
	String &			replace(size_type pos, size_type n1, const char* s, size_type n2)
		{ return do_replace(pos, n1, s, s + n2 ); }
	String &			replace(size_type pos, size_type n1, size_type n2, char c);
//	String &			replace(iterator i1, iterator i2, const std::string& s);
	String &			replace(iterator i1, iterator i2, const String& s);
	String &			replace(iterator i1, iterator i2, const char* s, size_type n);
	String &			replace(iterator i1, iterator i2, const char* s);
	String &			replace(iterator i1, iterator i2, size_type n, char c) { return replace(size_type(i1 - begin()), size_type(i2 - i1), n, c); }

	size_type			copy(char *s, size_type n, size_type pos = 0) const;
	void				swap(String& s);
	// _lib.string.ops_ string operations:
	const char *		c_str() const { return str; }         // explicit
	const char *		data() const { return str; }

//	size_type			find(const std::string& s, size_type pos = 0) const { return find(s.c_str(), pos, s.size()); }
	size_type			find(const String& s, size_type pos = 0) const { return find(s.str, pos, s.len); }
	size_type			find(const char * s, size_type pos, size_type n) const;
	size_type			find(const char * s, size_type pos = 0) const { return find(s, pos, strlen(s)); }
	size_type			find(char c, size_type pos = 0) const;
//	size_type			rfind(const std::string & s, size_type pos = npos) const { return rfind(s.c_str(), pos, s.size()); }
	size_type			rfind(const String & s, size_type pos = npos) const { return rfind(s.str, pos, s.len); }
	size_type			rfind(const char * s, size_type pos, size_type n) const;
	size_type			rfind(const char * s, size_type pos = npos) const { return rfind(s, pos, strlen(s)); }
	size_type			rfind(char c, size_type pos = npos) const;

	size_type			find_first_of(const String & s, size_type pos = 0) const;
	size_type			find_first_of(const char * s, size_type pos, size_type n) const;
	size_type			find_first_of(const char * s, size_type pos = 0) const { return find_first_of(s, pos, strlen(s)); }
	size_type			find_first_of(char c, size_type pos = 0) const { return find(c, pos); }
	size_type			find_last_of(const String & s, size_type pos = npos) const;
	size_type			find_last_of(const char * s, size_type pos, size_type n) const;
	size_type			find_last_of(const char * s, size_type pos = npos) const;
	size_type			find_last_of(char c, size_type pos = npos) const;
	size_type			find_first_not_of(const String & s, size_type pos = 0) const;
	size_type			find_first_not_of(const char * s, size_type pos, size_type n) const;
	size_type			find_first_not_of(const char * s, size_type pos = 0) const;
	size_type			find_first_not_of(char c, size_type pos = 0) const;
	size_type			find_last_not_of (const String & s, size_type pos = npos) const;
	size_type			find_last_not_of (const char * s, size_type pos, size_type n) const;
	size_type			find_last_not_of (const char * s, size_type pos = npos) const;
	size_type			find_last_not_of (char c, size_type pos = npos) const;
	String				substr(size_type pos = 0, size_type n = npos) const { return String(*this, pos, n); }
//	int					compare(const std::string& s) const { return str_cmp(str, s.c_str()); }
	int					compare(const String& s) const { return strcasecmp(str, s.str); }
	int					compare(size_type pos1, size_type n1, const String & s) const;
	int					compare(size_type pos1, size_type n1, const String & s, size_type pos2, size_type n2) const;
	int					compare(const char * s) const { return strcasecmp(str, s); }
	int					compare(size_type pos1, size_type n1, const char * s) const;  // hh 990126
	int					compare(size_type pos1, size_type n1, const char * s, size_type n2) const;

	//	Extended Lexi functionality
	void				format(const char *format, ...) __attribute__ ((format (printf, 2, 3)));

	static const size_t buffered_size = 16;	//	16 is the ideal size it seems; only 4 bytes waste at worst

protected:
	enum IsManaged { Managed };
	explicit String(IsManaged m, char *managedBuf, size_type managedCap) : str(managedBuf), buffered(false), managed(true), len(0), cap(managedCap) {} /* nulls the buf too */ 

private:
	
	bool				is_managed() const { return managed; }
	
	bool				is_buffered() const { return buffered; }
	void				set_buffered(bool isbuffered) { buffered = isbuffered; }
	
	String &			do_replace(size_type pos, size_type n, const_pointer j1, const_pointer j2);
	String &			do_replace(size_type pos, size_type n, pointer j1, pointer j2)
		{ return do_replace(pos, n, (const_pointer)j1, (const_pointer)j2); }

	static size_type	align(size_type size, size_type alignment) { return (size + (alignment - 1)) & ~(alignment - 1); }
	static size_type	recommend(size_type capacity) { return align(capacity, 16); }

	static const size_type buffered_cap = buffered_size - (sizeof(char *) + sizeof(size_type));

	char *				str;
	size_type			buffered : 1;
	size_type			managed : 1;
	size_type			len : 30;
	union
	{
		size_type			cap;
		char				buf[buffered_cap];
	};
};


inline String &String::replace(size_type pos1, size_type n1, const String& s)
{
	const_pointer p = s.str;
	size_type sz = s.len;
	return do_replace(pos1, n1, p, p + sz);

}


inline String &String::replace(size_type pos1, size_type n1, const String& s, size_type pos2, size_type n2)
{
	const_pointer p = s.str;
	size_type sz = s.len;

	if (pos2 > sz)
		return *this;
	p += pos2;
	n2 = std::min(n2, sz - pos2);
	return do_replace(pos1, n1, p, p + n2);	
}


inline String operator+(const String& lhs, const String& rhs)
{
	return String(lhs).append(rhs);
}

inline String operator+(const char* lhs, const String& rhs)
{
	return String(lhs).append(rhs);
}

inline String operator+(char lhs, const String& rhs)
{
	return String(1, lhs).append(rhs);
}

inline String operator+(const String& lhs, const char* rhs)
{
	return String(lhs).append(rhs);
}

inline String operator+(const String& lhs, char rhs)
{
	return String(lhs).append(1, rhs);
}

inline bool operator==(const String& lhs, const String& rhs)
{
	return lhs.size() == rhs.size() && lhs.compare(rhs) == 0;
}

inline bool operator==(const char* lhs, const String& rhs)
{
	return rhs.compare(lhs) == 0;
}

inline bool operator==(const String& lhs, const char* rhs)
{
	return lhs.compare(rhs) == 0;
}

inline bool operator!=(const String& lhs, const String& rhs)
{
	return lhs.size() != rhs.size() || lhs.compare(rhs) != 0;
}

inline bool operator!=(const char* lhs, const String& rhs)
{
	return rhs.compare(lhs) != 0;
}

inline bool operator!=(const String& lhs, const char* rhs)
{
	return lhs.compare(rhs) != 0;
}

inline bool operator< (const String& lhs, const String& rhs)
{
	return lhs.compare(rhs) < 0;
}

inline bool operator< (const String& lhs, const char* rhs)
{
	return lhs.compare(rhs) < 0;
}

inline bool operator< (const char* lhs, const String& rhs)
{
	return rhs.compare(lhs) > 0;
}

inline bool operator> (const String& lhs, const String& rhs)
{
	return lhs.compare(rhs) > 0;
}

inline bool operator> (const String& lhs, const char* rhs)
{
	return lhs.compare(rhs) > 0;
}

inline bool operator> (const char* lhs, const String& rhs)
{
	return rhs.compare(lhs) < 0;
}

inline bool operator<=(const String& lhs, const String& rhs)
{
	return lhs.compare(rhs) <= 0;
}

inline bool operator<=(const String& lhs, const char* rhs)
{
	return lhs.compare(rhs) <= 0;
}

inline bool operator<=(const char* lhs, const String& rhs)
{
	return rhs.compare(lhs) >= 0;
}

inline bool operator>=(const String& lhs,
           const String& rhs)
{
	return lhs.compare(rhs) >= 0;
}

inline bool operator>=(const String& lhs, const char* rhs)
{
	return lhs.compare(rhs) >= 0;
}

inline bool operator>=(const char* lhs, const String& rhs)
{
	return rhs.compare(lhs) <= 0;
}

// _lib.string.special_:
inline void swap(String& lhs, String& rhs)
{
	lhs.swap(rhs);
}


//std::basic_istream<char,String::traits_type>& operator >> (std::basic_istream<char,String::traits_type>& is, String& str);
//std::basic_ostream<char,String::traits_type>& operator << (std::basic_ostream<char,String::traits_type>& os, const String& str);
//std::basic_istream<char,String::traits_type>& getline(std::basic_istream<char,String::traits_type>& is, String& str, char delim);
//std::basic_istream<char,String::traits_type>& getline(std::basic_istream<char,String::traits_type>& is, String& str);


typedef std::list<String>			StringList;
//typedef std::list<std::string>		StdStringList;

extern String Format(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

}	//	namespace Lexi


#endif
