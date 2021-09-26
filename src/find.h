/**************************************************************************
*  File: find.h                                                           *
*  Usage: contains prototypes of functions for finding mobs and objs      *
*         in lists                                                        *
*                                                                         *
**************************************************************************/

#include "types.h"

#include "virtualid.h"
#include "lexilist.h"

#include "index.h"

#include "objects.h"
#include "characters.h"

ObjData *	get_obj(const char *name);
ObjData *	get_obj_num(VirtualID nr);
ObjData *	get_obj_vis(CharData *ch, const char *name, int *number = NULL);
ObjData *	get_obj_by_char(CharData * ch, const char *name);
ObjData *	get_obj_in_list(const char *name, int *number, Lexi::List<ObjData *> &list);
ObjData *	get_obj_in_list_vis(CharData *ch, const char *name, Lexi::List<ObjData *> &list, int *number = NULL);
ObjData *	get_obj_in_list_num(VirtualID num, Lexi::List<ObjData *> &list);
ObjData *	get_obj_in_list_type(int type, Lexi::List<ObjData *> &list);
ObjData *	get_obj_in_list_flagged(int flag, Lexi::List<ObjData *> &list);
ObjData *	get_obj_in_equip(const char *arg, int *number, ObjData * equipment[], int *j);
ObjData *	get_obj_in_equip_vis(CharData * ch, const char *arg, ObjData * equipment[], int *j, int *number = NULL);
ObjData *	get_obj_in_equip_type(int type, ObjData * equipment[]);
ObjData *	get_obj_by_obj(ObjData *obj, const char *name);
ObjData *	get_obj_by_room(RoomData *room, const char *name);
ObjData *	find_vehicle_by_vid(VirtualID vnum);

int	get_num_chars_on_obj(ObjData * obj);
CharData *	get_char_on_obj(ObjData *obj);

CharData *	get_char(const char *name);
CharData *	get_char_room(const char *name, RoomData *room, int *number = NULL);
CharData *	get_char_num(VirtualID nr);
CharData *	get_char_by_obj(ObjData *obj, const char *name);

/* find if character can see */
CharData *	get_char_room_vis(CharData *ch, const char *name, int *number = NULL);
CharData *	get_char_world_vis(CharData *ch, const char *name, int *number = NULL);
CharData *	get_char_other_room_vis(CharData * ch, const char *name, RoomData *room, int *number = NULL);
CharData *	get_player_vis(CharData *ch, const char *name, Flags where = 0, int *number = NULL);
CharData *	get_player(const char *name, int *number = NULL);
CharData *	get_char_vis(CharData *ch, const char *name, Flags where, int *number = NULL);
CharData *	get_char_by_char(CharData *ch, const char *name);

CharData *get_char_by_room(RoomData *room, const char *name);

RoomData *get_room(const char *name, Hash zone = 0);

//ShipData *find_ship(IDNum n);

int CountSpawnObjectsInList(ObjectPrototype *proto, ObjectList &room);
int CountSpawnMobsInList(CharacterPrototype *proto, CharacterList &list);

/* find all dots */

int	find_all_dots(char *arg);

#define FIND_INDIV	0
#define FIND_ALL	1
#define FIND_ALLDOT	2


/* Generic Find */

int	generic_find(const char *arg, int bitvector, CharData *ch, CharData **tar_ch, ObjData **tar_obj);

#define FIND_CHAR_ROOM     1
#define FIND_CHAR_WORLD    2
#define FIND_OBJ_INV       4
#define FIND_OBJ_ROOM      8
#define FIND_OBJ_WORLD    16
#define FIND_OBJ_EQUIP    32
