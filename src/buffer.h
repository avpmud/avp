/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-2005        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Advanced Buffer System by George Greer, Copyright (C) 1998-99         [*]
[-]-----------------------------------------------------------------------[-]
[*] File : buffer.h                                                       [*]
[*] Usage: Advanced Buffer System                                         [*]
\***************************************************************************/

#ifndef __BUFFER_H__
#define __BUFFER_H__


#include "types.h"
#include <stdlib.h>


//	CONFIGURABLES (aka, The place to shoot your own foot.) :)
//	---------------------------------------------------------
#define MAX_BUFFER_LENGTH	65536
#define MAX_STRING_LENGTH	8192

class Buffer {
public:
//protected:
					Buffer(size_t size);
					~Buffer(void);
	
	void			Clear(void);
	void			Remove(void);
	int				Used(void);
	int				Magnitude(void);
	
	char			magic;			//	Have we been trashed?
	size_t			req_size;		//	How much did the function request?
	const size_t	size;			//	How large is this buffer realy?
	char *			data;			//	The buffer passed back to functions
public:
	Buffer *		next;			//	Next structure
	
public:
	int			life;			//	An idle counter to free unused ones.	(B)
	int			line;			//	What source code line is using this.
	const char	*	who;			//	Name of the function using this
	
	void			Check(void);
	
public:
	static void		Init(void);
	static void		Exit(void);
	
	static void		Reboot(void);
	
	static Buffer **Head();
	
	static Buffer *	FindAvailable(size_t size);
	static Buffer *	Acquire(size_t size, const char *varname, const char *who, int line);
	void			Detach(const char *func, const int line_n);
	
	static void		ReleaseAll(void);
	
	static Buffer **buf;
	
	static Flags	options;
	static const char *const optionsDesc[];
	
private:
	static void		Decrement(void);
	static void		ReleaseOld(void);
	static void		FreeOld(void);
};


inline Buffer **Buffer::Head() {
	return buf;
}


class BufferWrapper
{
public:
	BufferWrapper(int size, const char *var, const char *func, int line) : m_Buffer(Buffer::Acquire(size, var, func, line)), m_Function(func), m_Line(line) { }
	~BufferWrapper() { if (m_Buffer) m_Buffer->Detach(m_Function, m_Line); }
	
	char *		Get() { return m_Buffer->data; }
private:
	Buffer *	m_Buffer;
	const char *m_Function;
	int			m_Line;
	
	BufferWrapper();
	BufferWrapper(const BufferWrapper &);
	BufferWrapper &operator=(const BufferWrapper &);
};

#define BUFFER(NAME, SZ) \
	BufferWrapper	Buffer_##NAME((SZ), #NAME, __PRETTY_FUNCTION__, __LINE__); \
	char *NAME = Buffer_##NAME.Get()

void show_buffers(class CharData *ch, int display_type);

#endif
