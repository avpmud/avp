/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-2005        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Original LexiMUD Code                                                 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : lexistring.cp                                                  [*]
[*] Usage: Lexi::String is an std::string compatible, case-insensitive,   [*]
[*]        reimplementation, optimized for speed and memory               [*]
\***************************************************************************/

#include <stdexcept>
#include <cstdarg>

#include "types.h"
#include "lexistring.h"
#include "buffer.h"

using namespace Lexi;

extern const char *str_str(const char *cs, const char *ct);
	

#if defined(TRACK_STRING_BUFFERING)
int g_numTotalStrings = 0;
int g_numBufferedStrings = 0;
#endif


#if defined(TRACK_STRING_BUFFERING)
String::String() : str(buf), buffered(true), managed(false), len(0), cap(0) /* nulls the buf too */ 
{
	++g_numBufferedStrings;
	++g_numTotalStrings;
}
#endif


String::String(const String &s) : str(buf), buffered(true), managed(false), len(0), cap(0)	/* nulls the buf too */
{
#if defined(TRACK_STRING_BUFFERING)
	++g_numBufferedStrings;
	++g_numTotalStrings;
#endif
	if (s.is_buffered())
	{
		strcpy(str, s.str);
		len = s.len;
	}
	else
	{
		assign(s);
	}
}


String::String(const String &s, size_type pos, size_type n) : str(buf), buffered(true), managed(false), len(0), cap(0) 	/* nulls the buf too */
{
#if defined(TRACK_STRING_BUFFERING)
	++g_numBufferedStrings;
	++g_numTotalStrings;
#endif
	size_type s_s = s.size();
	size_type s_c = std::min(n, s_s - pos);
	
	if (pos >= s_s)
	{
		//	Do nothing... nothing to copy
	}
	else if (s_c < buffered_cap)
	{
		strncpy(str, s.str + pos, s_c);
		str[s_c] = 0;
		len = s_c;
	}
	else
	{
		assign(s, pos, n);
	}
}


String::String(const char *s) : str(buf), buffered(true), managed(false), len(0), cap(0) /* nulls the buf too */
{
#if defined(TRACK_STRING_BUFFERING)
	++g_numBufferedStrings;
	++g_numTotalStrings;
#endif
	size_type n = strlen(s);
	
	if (n < buffered_cap)
	{
		strncpy(str, s, n);
		str[n] = 0;
		len = n;
	}
	else
	{
		assign(s, n);
	}
}


String::String(const char *s, size_type n) : str(buf), buffered(true), managed(false), len(0), cap(0) /* nulls the buf too */
{
#if defined(TRACK_STRING_BUFFERING)
	++g_numBufferedStrings;
	++g_numTotalStrings;
#endif
	if (n < buffered_cap)
	{
		strncpy(str, s, n);
		str[n] = 0;
		len = n;
	}
	else
	{
		assign(s, n);
	}
}


String::~String()
{
	if (!is_buffered() && !is_managed())
		free(str);
#if defined(TRACK_STRING_BUFFERING)
	else
		--g_numBufferedStrings;
	--g_numTotalStrings;
#endif
}


String &String::assign(const String &s)
{
	if (is_buffered() && s.is_buffered())
	{
		strcpy(buf, s.buf);
		len = s.len;
		return *this;
	}
	
	return replace(0, size(), s);
}


void String::reserve(size_t newCapacity)
{
	if (is_managed())
		return;

	bool isbuffered = buffered;
	
	pointer p = str;
	size_type curLength = len;
	size_type oldCapacity = isbuffered ? buffered_cap : cap;
	
	newCapacity += 1;	//	Account for terminator
	newCapacity = recommend(newCapacity);	//	16 byte align, please!

	if (newCapacity <= oldCapacity)
		return;	//	Shortcut; we currently don't care about making it smaller

	if (isbuffered)
	{
		//	If buffered: allocate new buffer, copy string, assign buffer over
		pointer temp = (pointer)malloc(newCapacity);
		memcpy(temp, str, oldCapacity);
		str = temp;
		
#if defined(TRACK_STRING_BUFFERING)
		--g_numBufferedStrings;
#endif
	}
	else
	{
		//	If unbuffered, just realloc it
		str = (pointer)realloc(str, newCapacity);
	}
	
	//	Now assign over new capacity (do this before the create above,
	//	because the buffered string and cap occupy the same memory
	cap = newCapacity;
	set_buffered(false);
}


void String::resize(size_type n, char c)
{
	size_type sz = size();
	
	if (n > sz)
		replace(sz, 0, n - sz, c);
	else if (n == 0)
	{
		*str = 0;
		len = 0;
	}
	else if (n != sz)
		replace(n, sz - n, size_type(0), value_type(0));
}


String & String::do_replace(size_type insertPosition, size_type replaceLength, const_pointer first, const_pointer last)
{
//	if (first == str)
//		return *this;
	
	bool isbuffered = buffered;
	
	pointer p = str;
	size_type oldLength = len;
	size_type oldCapacity = isbuffered ? buffered_cap : cap;
	
	//	If we are trying to insert past the end
	if (insertPosition > oldLength)
		throw std::out_of_range("Lexi::String: out_of_range");
	
	size_type extractLength = std::min(replaceLength, oldLength - insertPosition);
	size_type insertLength = (size_type)(last - first);
	size_type maxSize = max_size();
	
	//	If we are trying to insert more than we can
	if ((insertLength > maxSize) || ((oldLength - extractLength) > (maxSize - insertLength)))
		throw std::out_of_range("Lexi::String: length_error");
			
	size_type newLength = (oldLength - extractLength) + insertLength;
	size_type remainingLength = oldLength - (insertPosition + extractLength);
	
	if (newLength >= oldCapacity && !is_managed())
	{
		//	We will need to copy the string over.
		//	If this occurs, it is definitely greater than what will fit in
		//	the internal buffer
		
		size_type newCapacity = recommend(oldCapacity);	//	16 byte align, please!
		while (newCapacity < newLength + 1)
			newCapacity = newCapacity * 2;
		
		pointer temp = (pointer)malloc(newCapacity);
		
		//	Copy from start to insertPosition
		if (insertPosition > 0)
			memcpy(temp, p, insertPosition);
		memcpy(temp + insertPosition, first, insertLength);
		if (remainingLength > 0)
			memcpy(temp + insertPosition + insertLength, p + insertPosition + extractLength, remainingLength);
		temp[newLength] = 0;
		
		if (!isbuffered)
			free(str);
#if defined(TRACK_STRING_BUFFERING)
		else
			--g_numBufferedStrings;
#endif

		str = temp;
		len = newLength;
		cap = newCapacity;
		
		set_buffered(false);
	}
	else
	{
		//	We can fit the new string within the old string...
		//	But once we go unbuffered we stay unbuffered!
		
		String		temp;
		
		//	If there is text after the insert,
		//	and the text to insert is a substring that would overlap
		if (remainingLength > 0
			&& (p + insertPosition + insertLength) < last
			&& (last <= (p + oldLength)))
		{
			temp.assign(first, insertLength);
			first = temp.str;
			//last = first + temp.len;	//	We don't use last after this
		}
		
		if (remainingLength > 0)
			memmove(p + insertPosition + insertLength, p + insertPosition + extractLength, remainingLength);
		memmove(p + insertPosition, first, insertLength);
		p[newLength] = 0;
		
		len = newLength;
	}
	
	return *this;
/*
	int newLength = strlen(s);
	bool willBeBuffered = false;
	bool isLongerString = false;
	
	if (newLength < sizeof(buf))
		willBeBuffered = true;
	else if (newLength > len)
		isLongerString = true;
	
	if (willBeBuffered)
	{
		if (!is_buffered())
		{
			free(str);
			str = buf;
			set_buffered(true);			
		}
	}
	else if (isLongerString)
	{
		if (!is_buffered())		free(str);
		else					set_buffered(false);
		
		str = (char *)malloc(newLength + 1);
	}
	
		
	strcpy(str, s);
	len = newLength;
	
	return *this;
*/
}


String & String::replace(size_type insertPosition, size_type replaceLength, size_type insertLength, char c)
{
	bool isbuffered = buffered;
	
	pointer p = str;
	size_type oldLength = len;
	size_type oldCapacity = isbuffered ? buffered_cap : cap;
	
	//	If we are trying to insert past the end
	if (insertPosition > oldLength)
		throw std::out_of_range("Lexi::String: out_of_range");
	
	size_type extractLength = std::min(replaceLength, oldLength - insertPosition);
	size_type maxSize = max_size();
	
	//	If we are trying to insert more than we can
	if ((insertLength > maxSize) || ((oldLength - extractLength) > (maxSize - insertLength)))
		throw std::length_error("Lexi::String: length_error");
	
	size_type newLength = (oldLength - extractLength) + insertLength;
	size_type remainingLength = oldLength - (insertPosition + extractLength);
	
	if (newLength >= oldCapacity && !is_managed())
	{
		//	We will need to copy the string over.
		//	If this occurs, it is definitely greater than what will fit in
		//	the internal buffer
		
		size_type newCapacity = recommend(oldCapacity);	//	16 byte align, please!
		while (newCapacity < newLength + 1)
			newCapacity = newCapacity * 2;
		
		pointer temp = (pointer)malloc(newCapacity);
		
		//	Copy from start to insertPosition
		if (insertPosition > 0)
			memcpy(temp, p, insertPosition);
		if (insertLength > 0)
			memset(temp + insertPosition, c, insertLength);
		if (remainingLength > 0)
			memcpy(temp + insertPosition + insertLength, p + insertPosition + extractLength, remainingLength);
		temp[newLength] = 0;
		
		if (!isbuffered)
			free(str);
#if defined(TRACK_STRING_BUFFERING)
		else
			--g_numBufferedStrings;
#endif
		
		str = temp;
		len = newLength;
		cap = newCapacity;

		set_buffered(false);
	}
	else
	{
		//	We can fit the new string within the old string...
		//	But once we go unbuffered we stay unbuffered!
		
		if (remainingLength > 0)
			memmove(p + insertPosition + insertLength, p + insertPosition + extractLength, remainingLength);
		if (insertLength > 0)
			memset(p + insertPosition, c, insertLength);
		p[newLength] = 0;
		
		len = newLength;
	}
	
	return *this;
}


String::size_type String::find(const char * s, size_type pos, size_type n) const
{
	const_pointer beg = str;
	size_type sz = len;
	
	if (pos <= sz)
	{
		size_t rem		= sz - pos;
		const char *p	= beg + pos;
		
		for (; rem >= n; ++p, --rem)
		{
			if (!strncasecmp(s, p, n))
				return p - beg;
		}
	}
	
	return npos;
}


String::size_type String::find(char c, size_type pos) const
{
	size_type sz = len;
	
	c = tolower(c);
	
	if (pos < sz)
	{
		for (const char *p = str + pos, *e = str + sz; p < e; ++p)
			if (tolower(*p) == c)
				return p - str;
	}
	
	return npos;
}


String::size_type String::rfind(const char * s, size_type pos, size_type n) const
{
	const_pointer beg = str;
	size_type sz = len;
	
	if (sz >= n)
	{
		if (pos > sz - n)
			pos = sz - n;
		
		for (const char *p = beg + pos, *e = beg; p >= e; --p)
		{
			if (!strncasecmp(s, p, n))
				return p - beg;
		}
	}
	
	return npos;
}


String::size_type String::rfind(char c, size_type pos) const
{
	const_pointer beg = str;
	size_type sz = len;
	
	c = tolower(c);
	
	if (sz > 0)
	{
		if (pos > sz - 1)
			pos = sz - 1;
		
		for (const char *p = beg + pos, *e = beg; p >= e; --p)
		{
			if (tolower(*p) == c)
				return p - beg;
		}
	}
	
	return npos;
}


void String::swap(String& s)
{
	if (this != &s)
	{
		std::swap(str, s.str);

		bool oldBuffered = buffered;
		buffered = s.buffered;
		s.buffered = oldBuffered;

		bool oldLen = len;
		len = s.len;
		s.len = oldLen;

		char oldBuf[sizeof(buf)];
		memcpy(oldBuf, buf, sizeof(buf));
		memcpy(buf, s.buf, sizeof(buf));
		memcpy(s.buf, oldBuf, sizeof(buf));
	}
}


void String::format(const char *format, ...)
{
	static char buffer[65536];
	
	va_list args;
	
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	
	assign(buffer);
}



String Lexi::Format(const char *format, ...)
{
	static char buffer[65536];
	
	va_list args;
	
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	//	I don't think this is necessary with vsnprintf...	
//	buffer[sizeof(buffer) - 1] = 0;
	
	return buffer;
}
