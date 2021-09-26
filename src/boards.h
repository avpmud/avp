/* ************************************************************************
*   File: boards.h                                      Part of CircleMUD *
*  Usage: header file for bulletin boards                                 *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Response and threading code copyright Trevor Man 1997                  *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef __BOARDS_H__
#define __BOARDS_H__

#include "types.h"

#include "virtualid.h"
#include "characters.h"
#include "objects.h"

#include <list>
#include <map>

/* sorry everyone, but inorder to allow for board memory, I had to make
   this in the following nasty way.  Sorri. */
//struct board_memory_type {
//	int timestamp;
//	int reader;
//	struct board_memory_type *next;
//};


class Board
{
public:
	Board(VirtualID vid) : m_Virtual(vid), m_readLevel(0), m_writeLevel(0), m_removeLevel(LVL_STAFF) {}
	
	void			Save();
	
	class Message
	{
	public:
		Message(unsigned int id) : m_ID(id), m_Poster(0), m_Time(0) {}
		
		unsigned int	m_ID;
		IDNum			m_Poster;
		time_t			m_Time;
		Lexi::String	m_Subject;
		Lexi::String	m_Message;
	};
	typedef std::list<Message *>	MessageList;

	VirtualID 		GetVirtualID() const { return m_Virtual; }
	int				GetReadLevel() { return m_readLevel; }
	int				GetWriteLevel() { return m_writeLevel;}
	int				GetRemoveLevel() { return m_removeLevel; }
	
	MessageList &	GetMessages() { return m_Messages; }

	Lexi::String	GetFilename();
	Lexi::String	GetOldFilename();

	static Board *	GetBoard(VirtualID vid);
	static void		SaveBoard(VirtualID vid);
	

	static int		Show(ObjData *obj, CharData *ch);
	static int		DisplayMessage(ObjData *obj, CharData * ch, int id);
	static int		RemoveMessage(ObjData *obj, CharData * ch, int id);
	static void		WriteMessage(ObjData *obj, CharData *ch, char *arg);
	static void		RespondMessage(ObjData *obj, CharData *ch, int id);

	void			Renumber(VirtualID newVID);
	
private:
	VirtualID		m_Virtual;
	MessageList		m_Messages;

	int				m_readLevel;
	int				m_writeLevel;
	int				m_removeLevel;

	void			ParseMessage( FILE *fl, const char *filename, int id);

	typedef std::map<VirtualID, Board *>	BoardMap;
	static BoardMap	boards;
};

#endif
