/* ************************************************************************
*   File: spec_assign.c                                 Part of CircleMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "structs.h"
#include "buffer.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"

extern int dts_are_dumps;

void assign_elevators(void);

/* local functions */
void ASSIGNMOB(VirtualID mob, SPECIAL(fname));
void ASSIGNOBJ(VirtualID obj, SPECIAL(fname));
void ASSIGNROOM(VirtualID room, SPECIAL(fname));
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);


/* functions to perform assignments */

void ASSIGNMOB(VirtualID mob, SPECIAL(fname))
{
	CharacterPrototype *proto = mob_index.Find(mob);
	if (proto)	proto->m_Function = fname;
	else		log("SYSERR: Attempt to assign spec to non-existant mob '%s'", mob.Print().c_str());
}

void ASSIGNOBJ(VirtualID obj, SPECIAL(fname))
{
	ObjectPrototype *proto = obj_index.Find(obj);
	if (proto)	proto->m_Function = fname;
	else		log("SYSERR: Attempt to assign spec to non-existant obj '%s'", obj.Print().c_str());
}

void ASSIGNROOM(VirtualID vnum, SPECIAL(fname))
{
	RoomData *room = world.Find(vnum);
	if (room)	room->func = fname;
	else		log("SYSERR: Attempt to assign spec to non-existant rm. '%s'", vnum.Print().c_str());
}


/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

/* assign special procedures to mobiles */
void assign_mobiles(void)
{
  SPECIAL(puff);
  SPECIAL(postmaster);
  SPECIAL(temp_guild);

  ASSIGNMOB(1, puff);
  ASSIGNMOB(1201, postmaster);
  
  ASSIGNMOB(8020, temp_guild);		//	Marine trainer
  ASSIGNMOB(/*1500*/ 617, temp_guild);		//	Yautja Trainer
  ASSIGNMOB(25000, temp_guild);		//	Alien trainer
}



/* assign special procedures to objects */
void assign_objects(void)
{
}

/* assign special procedures to rooms */
void assign_rooms(void)
{
  SPECIAL(dump);

  assign_elevators();
  
	if (dts_are_dumps)
	{
		FOREACH(RoomMap, world, room)
		{
			if (ROOM_FLAGGED(*room, ROOM_DEATH))
				(*room)->func = dump;
	    }
	}
}


