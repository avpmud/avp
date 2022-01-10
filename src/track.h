
#ifndef __TRACK_H__
#define __TRACK_H__

#include <vector>

#include "types.h"
#include "rooms.h"

class TrackStep
{
public:
	TrackStep(RoomData *r, Direction d) : room(r), dir(d) {}
	
	RoomData *		room;
	Direction		dir;
};


class Path {
public:
	Path(void);
	~Path(void);
	int			GetNextDir(RoomData * room);
	
	Flags		m_Flags;
	int			m_Depth;
	IDNum		m_idVictim;
	RoomData *	m_Destination;
	std::vector<TrackStep>	m_Moves;
};


#define	HUNT_GLOBAL			1
#define	HUNT_THRU_DOORS		2
#define	HUNT_THRU_GRAVITY	4

//	primary entry points, these are the easiest to use the tracking package

//	build a path to either a named character, or a specific character,
//	path_to_name locates a character for whom is_name(name) returns
//	true, path_to_full_name locates a character with the exact name
//	specified

Path *	Path2Name(RoomData *start, char *name, int depth, Flags flags);
Path *	Path2FullName(RoomData *start, char *name, int depth, Flags flags);
Path *	Path2Char(RoomData *start, IDNum vict, int depth, Flags flags);
Path *	Path2Room(RoomData *start, RoomData *dest, int depth, Flags flags);

Path *	Path2Name(CharData *ch, char *name, int depth, Flags flags);
Path *	Path2FullName(CharData *ch, char *name, int depth, Flags flags);
Path *	Path2Char(CharData *ch, IDNum vict, int depth, Flags flags);
Path *	Path2Room(CharData *ch, RoomData *dest, int depth, Flags flags);


/* a lower level function to build a path, perhaps based on a more
   complicated predicate then path_to*_char */
   
typedef bool	(*BuildPathFunc)(RoomData *start, Ptr data);
typedef int		(*StepCostFunc)(RoomData *room, int dir, Ptr data);

Path *	PathBuild(RoomData *start, BuildPathFunc predicate, Ptr data, int depth, Flags flags);
Path *	PathRebuild(RoomData *start, Path *path);

/* do ongoing maintenance of an in-progress track */
int	Track(CharData* ch);

//ACMD(do_path);

#endif
