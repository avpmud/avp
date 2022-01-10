#include "types.h"
#include "track.h"
#include "structs.h"
#include "interpreter.h"
#include "handler.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "find.h"

ACMD(do_path);
int find_first_step(RoomData *room, char *target);
int find_first_step(RoomData *room, RoomData *target);

extern char *dirs[];


class PathfinderStrategy
{
public:
	virtual			~PathfinderStrategy() {}
					
	virtual bool	IsDestination(RoomData *room) = 0;
	virtual bool	IsDestinationRecheck(RoomData *room) = 0;
};


class CharacterByNamePathfinderStrategy : public PathfinderStrategy
{
public:
					CharacterByNamePathfinderStrategy(const char *name);
	virtual			~CharacterByNamePathfinderStrategy();
					
	virtual bool	IsDestination(RoomData *room);
	virtual bool	IsDestinationRecheck(RoomData *room);

//	virtual bool	IsPathStillValid(RoomData *room);
	
private:
	Lexi::String	m_Name;
	IDNum			m_Victim;
};


CharacterByNamePathfinderStrategy::CharacterByNamePathfinderStrategy(const char *name) :
	m_Name(name),
	m_Victim(0)
{
}


CharacterByNamePathfinderStrategy::~CharacterByNamePathfinderStrategy()
{
}


bool CharacterByNamePathfinderStrategy::IsDestination(RoomData *room)
{
	FOREACH(CharacterList, room->people, iter)
	{
		CharData *ch = *iter;
		const char *victimName = ch->GetAlias();
		
		if (!AFF_FLAGGED(ch, AFF_NOTRACK)
			&& ((m_Name[0] == '.') ? str_cmp(m_Name.c_str() + 1, victimName) : isname(m_Name.c_str(), victimName)))
		{
			m_Victim = ch->GetID();
			return true;
		}
	}
	return false;
}


bool CharacterByNamePathfinderStrategy::IsDestinationRecheck(RoomData *room)
{
	FOREACH(CharacterList, room->people, iter)
	{
		CharData *ch = *iter;
		if (m_Victim == ch->GetID() && !AFF_FLAGGED(ch, AFF_NOTRACK))
		{
			return true;
		}
	}
	return false;
}



class CharacterByIDPathfinderStrategy : public PathfinderStrategy
{
public:
					CharacterByIDPathfinderStrategy(IDNum victim) : m_Victim(victim) {}
					
	virtual bool	IsDestination(RoomData *room);
	virtual bool	IsDestinationRecheck(RoomData *room) { return IsDestination(room); }

private:
	IDNum			m_Victim;
};


bool CharacterByIDPathfinderStrategy::IsDestination(RoomData *room)
{
	FOREACH(CharacterList, room->people, iter)
	{
		CharData *ch = *iter;
		if (m_Victim == ch->GetID() && !AFF_FLAGGED(ch, AFF_NOTRACK))
		{
			return true;
		}
	}
	return false;
}


class SpecificRoomPathfinderStrategy : public PathfinderStrategy
{
public:
					SpecificRoomPathfinderStrategy(RoomData *room) : m_Room(room) {}
					
	virtual bool	IsDestination(RoomData *room) { return room == m_Room; }
	virtual bool	IsDestinationRecheck(RoomData *room) { return IsDestination(room); }

private:
	RoomData *		m_Room;
};







bool NamedCharInRoom(RoomData * room, Ptr data);
bool FullNameInRoom(RoomData * room, Ptr data);
bool CharInRoom(RoomData * room, Ptr data);
bool SameRoom(RoomData * room, Ptr data);

struct NCIRData {
	char *		name;
	IDNum		vict;
};


bool NamedCharInRoom(RoomData * room, Ptr data) {
	NCIRData *	ncir = (NCIRData *)data;
	
	if (room == NULL)
		return false;
	
	FOREACH(CharacterList, room->people, iter)
	{
		CharData *ch = *iter;
		
		if (isname(ncir->name, ch->GetAlias()) && !AFF_FLAGGED(ch, AFF_NOTRACK)) {
			ncir->vict = GET_ID(ch);
			return true;
		}
	}
	return false;
}


Path *	Path2Name(RoomData * start, char *name, int depth, Flags flags) {
	NCIRData	ncir = { name, 0 };
	Path *		path;
		
	if ((path = PathBuild(start, NamedCharInRoom, &ncir, depth, flags)))
		path->m_idVictim = ncir.vict;
	
	return path;
}


//struct FNIRData {
//	SString *	name;
//	CharData *	vict;
//};

bool FullNameInRoom(RoomData * room, Ptr data) {
	NCIRData *	fnir = (NCIRData *)data;
	
	if (room == NULL)
		return false;
	
	FOREACH(CharacterList, room->people, iter)
	{
		CharData *ch = *iter;
		
		if (!str_cmp(ch->GetName(), fnir->name)) {
			fnir->vict = GET_ID(ch);
			
			return true;
		}
	}
	return false;
}


Path * Path2FullName(RoomData * start, char *name, int depth, Flags flags) {
	NCIRData	fnir = { name, 0 };
	Path *		path;
	
	if ((path = PathBuild(start, FullNameInRoom, &fnir, depth, flags)))
		path->m_idVictim = fnir.vict;
	
	return path;
}


bool CharInRoom(RoomData * room, Ptr data) {
	IDNum		id = *(IDNum *)data;
	
	if (room == NULL)
		return false;
	
	FOREACH(CharacterList, room->people, iter)
	{
		CharData *ch = *iter;
		if (GET_ID(ch) == id)
			return true;
	}
	
	return false;
}


Path * Path2Char(RoomData * start, IDNum vict, int depth, Flags flags) {
	Path *		path;
	
	if ((path = PathBuild(start, CharInRoom, &vict, depth, flags)))
		path->m_idVictim = vict;
	
	return path;
}


bool SameRoom(RoomData * room, Ptr data) {
	return (room == data);
}


Path * Path2Room(RoomData * start, RoomData *dest, int depth, Flags flags) {
	return PathBuild(start, SameRoom, dest, depth, flags);
}



Path *	Path2Name(CharData *ch, char *name, int depth, Flags flags)
{
	if (AFF_FLAGGED(ch, AFF_FLYING) || NO_STAFF_HASSLE(ch))
		SET_BIT(flags, HUNT_THRU_GRAVITY);
	
	return Path2Name(IN_ROOM(ch), name, depth, flags);
}

Path *	Path2FullName(CharData *ch, char *name, int depth, Flags flags)
{
	if (AFF_FLAGGED(ch, AFF_FLYING) || NO_STAFF_HASSLE(ch))
		SET_BIT(flags, HUNT_THRU_GRAVITY);
	
	return Path2FullName(IN_ROOM(ch), name, depth, flags);
}

Path *	Path2Char(CharData *ch, IDNum vict, int depth, Flags flags)
{
	if (AFF_FLAGGED(ch, AFF_FLYING) || NO_STAFF_HASSLE(ch))
		SET_BIT(flags, HUNT_THRU_GRAVITY);
	
	return Path2Char(IN_ROOM(ch), vict, depth, flags);
}

Path *	Path2Room(CharData *ch, RoomData * dest, int depth, Flags flags)
{
	if (AFF_FLAGGED(ch, AFF_FLYING) || NO_STAFF_HASSLE(ch))
		SET_BIT(flags, HUNT_THRU_GRAVITY);
	
	return Path2Room(IN_ROOM(ch), dest, depth, flags);
}


Path::Path(void) :
	m_Flags(0),
	m_Depth(0),
//	m_pVictim(NULL),
	m_idVictim(0),
	m_Destination(NULL)
{
}


Path::~Path(void) {
}


int Path::GetNextDir(RoomData * room)
{
	bool		bNeedsRebuild = false;
	
	std::vector<TrackStep>::iterator	curStep, i;
	
	//	Are we along the path?  If not, we need to rebuild!
	for (curStep = m_Moves.begin(); curStep != m_Moves.end(); ++curStep)
		if (curStep->room == room)
			break;
	
	if (curStep == m_Moves.end())
		bNeedsRebuild = true;
	
	if (m_idVictim)
	{
		CharData *	pVictim = CharData::Find(m_idVictim);
		
		if (!pVictim || IN_ROOM(pVictim) == room)
			return -1;
		
		//	Has target moved?
		if (!bNeedsRebuild && pVictim->InRoom() != m_Destination)
		{
			//	If so, are they somewhere further along the path than we are?  If not, rebuild!
			for (i = curStep; i != m_Moves.end(); ++i)
				if (i->room == pVictim->InRoom())
					break;
			
			if (i == m_Moves.end())
				bNeedsRebuild = true;
		}
	}
	
	if (bNeedsRebuild)
	{
		if (!PathRebuild(room, this))
			return -1;
		
		for (curStep = m_Moves.begin(); curStep != m_Moves.end(); ++curStep)
			if (curStep->room == room)
				return curStep->dir;

		return -1;
	}
	else
		return curStep->dir;

	//	In theory, we shouldn't need to loop twice.  We know we're on the path,
	//	and we know our target is somewhere further on the path as well.
#if 0
	for (retry = 2; retry > 0; --retry) {
		for (i = 0; i < m_NumMoves; ++i)
			if (m_pMoves[i]->room == room)
				return move->dir;
		
		if (/*m_idVictim == 0 ||*/ retry == 1 || !PathRebuild(room, this))
			break;
	}

	return -1;
#endif
}


#define GO_OK(exit)			(!(exit)->GetStates().test(EX_STATE_CLOSED) && (exit)->ToRoom())
#define GO_OK_SMARTER(exit)	(!(exit)->GetStates().test(EX_STATE_LOCKED) && (exit)->ToRoom())



struct VisitedRoom
{
	VisitedRoom(RoomData *r = NULL, int d = -1) : prevRoom(r), direction(d) { }
	RoomData *			prevRoom;
	int					direction;
};


Path *PathBuild(RoomData *startRoom, BuildPathFunc predicate, Ptr data, int depth, Flags flags)
{
	int			count = 0;
						
	std::list<RoomData *>roomQueue;
	typedef std::map<RoomData *, VisitedRoom>	VisitedRoomMap;
	VisitedRoomMap		visitedRooms;
	
	if ((predicate)(startRoom, data))
		return NULL;
	
	roomQueue.push_back(startRoom);
	visitedRooms[startRoom] = VisitedRoom();
	
	while (!roomQueue.empty())
	{
		RoomData *here = roomQueue.front();
		roomQueue.pop_front();

		for (int dir = 0; dir < NUM_OF_DIRS; ++dir) {
			RoomExit *	exit = here->GetExit(dir);
			RoomData *	there = exit ? exit->ToRoom() : NULL;
			
			if (there
				&& !ROOM_FLAGGED(there, ROOM_DEATH)
				&& (IS_SET(flags, HUNT_THRU_GRAVITY) || !ROOM_FLAGGED(there, ROOM_GRAVITY))
				&& (IS_SET(flags, HUNT_GLOBAL) || IsSameEffectiveZone(startRoom, there))
				&& (IS_SET(flags, HUNT_THRU_DOORS) ? GO_OK_SMARTER(exit) : GO_OK(exit))
				&& visitedRooms.find(there) == visitedRooms.end())
			{
				if ((predicate)(there, data))
				{
					Path *	path;
					there = here;
					
					path = new Path();
					
					path->m_Destination = there;
					path->m_Flags = flags;
					path->m_Depth = depth;
					path->m_Moves.push_back(TrackStep(there, (Direction)dir));
					
					while (1)
					{
						VisitedRoomMap::iterator iter = visitedRooms.find(there);
						
						if (iter == visitedRooms.end() || iter->second.prevRoom == NULL)
							break;
						
						path->m_Moves.push_back(TrackStep(iter->second.prevRoom, (Direction)iter->second.direction));
						
						there = iter->second.prevRoom;
					}
					
					return path;
				}
				else if (++count < depth)
				{
					//	Put room on queue
					roomQueue.push_back(there);
					visitedRooms[there] = VisitedRoom(here, dir);
				}
			}
		}
	}
					
	return NULL;
}


Path *PathRebuild(RoomData *start, Path *path) {
	Path *		new_path;
	
	if (!path)
		return NULL;
	
	if (path->m_idVictim)
		new_path = Path2Char(start, path->m_idVictim, path->m_Depth, path->m_Flags);
	else if (path->m_Destination)
		new_path = Path2Room(start, path->m_Destination, path->m_Depth, path->m_Flags);
	else
		return NULL;
	
	if (!new_path)
		return NULL;
	
	path->m_Moves = new_path->m_Moves;
	
	delete new_path;
	
	return path;
}


ACMD(do_path) {
	BUFFER(name, MAX_INPUT_LENGTH);
	BUFFER(directs, MAX_STRING_LENGTH);
	Path *		path = NULL;
	CharData *	victim = NULL;
	char *		dir;
	
	one_argument(argument, name);
	
	if (!*name)
		send_to_char("Find path to whom?\n", ch);
	else if (*name == '.')
		path = Path2FullName(ch, name + 1, 200, HUNT_GLOBAL | HUNT_THRU_DOORS);
	else
		path = Path2Name(ch, name, 200, HUNT_THRU_DOORS);

	if (path)
		victim = CharData::Find(path->m_idVictim);
		
	if (path && victim) {
		dir = directs;
		
		for (std::vector<TrackStep>::reverse_iterator iter = path->m_Moves.rbegin(); iter != path->m_Moves.rend(); ++iter)
			*dir++ = dirs[iter->dir][0];
		*dir++ = '\n';
		*dir = '\0';
		
		
		ch->Send("Shortest route to %s:\n%s", victim->GetName(), directs);
	} else
		send_to_char("Can't find target.\n", ch);
}


int Track(CharData *ch) {
	CharData *	vict = NULL;
	int		code;

	if(!ch)			return -1;
	if(!ch->path)	return -1;

	if (ch->path->m_idVictim)
		vict = CharData::Find(ch->path->m_idVictim);
	
	if (vict ? (IN_ROOM(ch) == IN_ROOM(vict)) : IN_ROOM(ch) == ch->path->m_Destination) {
//		send_to_char("##You have found your target!\n", ch);
		delete ch->path;
		ch->path = NULL;
		return -1;
	}

	if((code = ch->path->GetNextDir(IN_ROOM(ch))) == -1) {
//		send_to_char("##You have lost the trail.\n",ch);
		delete ch->path;
		ch->path = NULL;
		return -1;
	}

//	ch->Send("##You see a faint trail to the %s\n", dirs[code]);

	return code;
}


/* 
 * find_first_step: given a source room and a target room, find the first
 * step on the shortest path from the source to the target.
 *
 * Intended usage: in mobile_activity, give a mob a dir to go if they're
 * tracking another mob or a PC.  Or, a 'track' skill for PCs.
 */
int find_first_step(RoomData *src, char *target) {
	int	dir = -1;
	Path *	path;
	
	if (src == NULL || !target) {
		log("Illegal value %p or %p passed to find_first_step(RoomData *, char *). (" __FILE__ ")", src, target);
		return dir;
	}

	path = Path2Name(src, target, 200, HUNT_GLOBAL | HUNT_THRU_DOORS);

	if (path) {
		dir = path->m_Moves.back().dir;
		delete path;
	}
	return dir;
}

int find_first_step(RoomData * src, RoomData * target) {
	Path *		path;
	int			dir = -1;
	
	if (src == NULL || target == NULL)
	{
		log("Illegal value %p or %p passed to find_first_step(RoomData *, RoomData *). (" __FILE__ ")", src, target);
		return dir;
	}

	path = Path2Room(src, target, 200, HUNT_GLOBAL | HUNT_THRU_DOORS);

	if (path)
	{
		dir = path->m_Moves.back().dir;
		delete path;
	}
	return dir;
}
