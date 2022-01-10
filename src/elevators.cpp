
#include "structs.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "event.h"
#include "constants.h"

/*   external vars  */
extern struct event_info * pending_events;

void ASSIGNROOM(VirtualID room, SPECIAL(fname));

void assign_elevators(void);
int find_elevator_level_by_room(int elevator, RoomData *room);
int find_elevator_by_level(RoomData *room);
int find_elevator_by_room(RoomData *room);
SPECIAL(call_elevator);
SPECIAL(tell_elevator);



class elevRoom {
public:
	VNum			room;
	int				level;
	int				button;
};

class elevRec {
public:
	char *		name;
	int			exit;
	elevRoom	room;
	elevRoom	levels[10];
	int			numLevels;
	struct		{
		int				stop;
		int				start;
		int				move;
	} delay;
	int			pos;
	int			dir,
				prev;
	Event *		event;
};

#define EL_UP	 1
#define EL_STOP  0
#define EL_DOWN	-1

#define ELEVATOR_NAME(elev)			elevators[elev].name
#define ELEVATOR_LEVELS(elev)		elevators[elev].numLevels
#define ELEVATOR_ROOM(elev, level)	elevators[elev].levels[level]
#define ELEVATOR(elev)				elevators[elev].room
#define ELEVATOR_EXIT(elev)			elevators[elev].exit
#define ELEVATOR_POS(elev)			elevators[elev].pos
#define ELEVATOR_DIR(elev)			elevators[elev].dir
#define ELEVATOR_PREV_DIR(elev)		elevators[elev].prev
#define ELEVATOR_CUR(elev)			elevators[elev].levels[elevators[elev].pos]
#define ELEVATOR_CUR_LEVEL(elev)	elevators[elev].levels[elevators[elev].pos].level
#define ELEVATOR_CUR_BUTTON(elev)	elevators[elev].levels[elevators[elev].pos].button
#define ELEVATOR_LEVEL(elev, lev)	elevators[elev].levels[lev].level
#define ELEVATOR_BUTTON(elev, lev)	elevators[elev].levels[lev].button
#define ELEVATOR_DELAY_MOVE(elev)	elevators[elev].delay.move
#define ELEVATOR_DELAY_START(elev)	elevators[elev].delay.start
#define ELEVATOR_DELAY_STOP(elev)	elevators[elev].delay.stop

#define NUM_ELEVATORS 3
elevRec elevators[NUM_ELEVATORS] = {
/*
	{
		"hydraulic lift",
		NORTH,
		{7297},
		{	{7295},
			{7216},
			{7296},
		},
		3,
		1,
		EL_UP,
		EL_UP,
		{ 10, 5, 5 }
	},
*/
	{	//	4235
		"elevator",
		SOUTH,
		{4235},
		{	{4225, 1},
			{4236, 2},
			{4268, 3},
			{4269, 4},
			{4270, 5},
			{4271, 6}
		},
		6,
		{ 10, 5, 5 }
	},
	{
		"elevator",
		NORTH,
		{8004},
		{	{8003, 1},
			{8008, 2},
			{8018, 3}
		},
		3,
		{ 10, 3, 2 }
	},
	{
		"elevator",
		EAST,
		{13035},
		{	{13031, 1},
			{13043, 2}
		},
		2,
		{ 10, 3, 2 }
	}
};


class ElevatorEvent : public Event
{
public:
						ElevatorEvent(IDNum elevatorNum) : Event(1), m_Elevator(elevatorNum)
						{}
	
	virtual const char *GetType() const { return "Elevator"; }

private:
	virtual unsigned int	Run();

	IDNum				m_Elevator;
};


void assign_elevators(void) {
	for(int x=0; x<NUM_ELEVATORS; x++) {
		ASSIGNROOM(ELEVATOR(x).room, tell_elevator);

		ELEVATOR_POS(x) = 1;
		ELEVATOR_DIR(x) = ELEVATOR_PREV_DIR(x) = EL_UP;

		elevators[x].event = new ElevatorEvent(x);

		for(int y = 0; y < ELEVATOR_LEVELS(x); y++)
			ASSIGNROOM(ELEVATOR_ROOM(x, y).room, call_elevator);
	}
}


int find_elevator_level_by_room(int elevator, RoomData * room)
{
	int x;
	for(x=0; x < ELEVATOR_LEVELS(elevator); ++x) {
		if (room->GetVirtualID() == ELEVATOR_ROOM(elevator, x).room)
			return x;
	}
	return -1;
}


int find_elevator_by_level(RoomData * room)
{
	int x;
	for(x=0; x < NUM_ELEVATORS; x++) {
		if(find_elevator_level_by_room(x, room) != -1)
			return x;
	}
	log("Problem... elevator doesn't exist!");
	return -1;
}


int find_elevator_by_room(RoomData *room) {
	int x;
	for(x=0; x < NUM_ELEVATORS; x++) {
		if(room->GetVirtualID() == ELEVATOR(x).room)
			return x;
	}
	log("Problem... elevator doesn't exist!");
	return -1;
}


SPECIAL(call_elevator) {
	int elevator, level;
	
	
	if (CMD_IS("press")) {
		BUFFER(arg, MAX_INPUT_LENGTH);
	
		one_argument(argument, arg);
		
		if (!is_abbrev(arg, "button"))
			return 0;

		elevator = find_elevator_by_level(ch->InRoom());
		if(elevator == -1)	return 0;
		level = find_elevator_level_by_room(elevator, ch->InRoom());
		if(level == -1)		return 0;
		ELEVATOR_BUTTON(elevator, level) = 1;
 		act("You press the button.", TRUE, ch, 0, 0, TO_CHAR);
 		act("$n presses the button.", FALSE, ch, 0, 0, TO_ROOM);
//		if (!elevator_pending(elevator))
//			add_event(ELEVATOR_DELAY_MOVE(elevator), elevator_move, 0, 0, (Ptr)elevator);
 		return 1;
	}
	return 0;
}

SPECIAL(tell_elevator) {
	BUFFER(arg1, MAX_INPUT_LENGTH);
	BUFFER(arg2, MAX_INPUT_LENGTH);
	BUFFER(buf, MAX_STRING_LENGTH);
	
	int elevator = NUM_ELEVATORS, y;
	RoomData *room = ch->InRoom();
	
	if (CMD_IS("press")) {
		elevator = find_elevator_by_room(ch->InRoom());
		if(elevator == -1)
			return 0;
		two_arguments(argument, arg1, arg2);
		if (!*arg2)
			y = atoi(arg1);
		else if (is_abbrev(arg1, "button"))
			y = atoi(arg2);
		else {
			return 0;
		}
		if ((y <= 0) || (y > ELEVATOR_LEVELS(elevator))) {
			sprintf(buf, "There is no button with the number %d.\n", y);
			send_to_char(buf, ch);
			return 1;
		}
		ELEVATOR_BUTTON(elevator, y-1) = 1;
		sprintf(buf, "You press button %d.", y);
		act(buf, TRUE, ch, 0, 0, TO_CHAR);
		sprintf(buf, "$n presses button %d.", y);
		act(buf, TRUE, ch, 0, 0, TO_ROOM);
		if (Exit::IsOpen(room, ELEVATOR_EXIT(elevator))) {
			elevators[elevator].event->Reschedule(1);
		}
//		} else if (!elevator_pending(elevator))
//			add_event(ELEVATOR_DELAY_MOVE(elevator), elevator_move, 0, 0, (Ptr)elevator);
		return 1;
	} else if (CMD_IS("close")) {
		elevator = find_elevator_by_room(ch->InRoom());
		if(elevator == -1)
			return 0;
		act("You press the 'close' button.", TRUE, ch, 0, 0, TO_CHAR);
		act("$n presses the 'close' button.", FALSE, ch, 0, 0, TO_ROOM);
		if (Exit::IsOpen(room, ELEVATOR_EXIT(elevator))) {
			elevators[elevator].event->Reschedule(1);
		} else
			act("Nothing happens.", TRUE, ch, 0, 0, TO_CHAR);
		return 1;
	} else if (CMD_IS("open")) {
		elevator = find_elevator_by_room(ch->InRoom());
		if(elevator == -1)
			return 0;
		if (!Exit::IsOpen(room, ELEVATOR_EXIT(elevator))) {
			act("Try as you might, the $T doors won't open, unless it has come to a stop at a floor.",
					TRUE, ch, 0, ELEVATOR_NAME(elevator), TO_CHAR);
			act("$n tries in vain to open the doors, until $e realizes that the $T is still moving.",
					TRUE, ch, 0, ELEVATOR_NAME(elevator), TO_ROOM);
		} else {
			act("The $T doors are already open.",
					TRUE, ch, 0, ELEVATOR_NAME(elevator), TO_CHAR);
		}
		return 1;
	}
	return 0;
}


unsigned int ElevatorEvent::Run() {
	RoomData *eRoom, *tRoom;
	int		eDoor, tDoor;
	int	newTimer;
	RoomData *	room;
	
	if (ELEVATOR_DIR(m_Elevator)) {
		room = world.Find(ELEVATOR(m_Elevator).room);
		if (!room)
		{
			log("SYSERR: bad elevator %d - No room", m_Elevator);
			elevators[m_Elevator].event = NULL;
			return 0;
		}
		
		ELEVATOR_POS(m_Elevator) += ELEVATOR_DIR(m_Elevator);
		room->Send("The %s arrives at level %d.\n", ELEVATOR_NAME(m_Elevator), ELEVATOR_CUR_LEVEL(m_Elevator));

		if (ELEVATOR_POS(m_Elevator) <= 0)								ELEVATOR_DIR(m_Elevator) = EL_UP;
		if (ELEVATOR_POS(m_Elevator) >= ELEVATOR_LEVELS(m_Elevator) - 1)	ELEVATOR_DIR(m_Elevator) = EL_DOWN;
	}
	
	eRoom = world.Find(ELEVATOR(m_Elevator).room);
	tRoom = world.Find(ELEVATOR_CUR(m_Elevator).room);
	if (eRoom == NULL || tRoom == NULL) {
		log("SYSERR: Elevator error: elevator room OR target room is NOWHERE");
		elevators[m_Elevator].event = NULL;
		return 0;
	}
	
	eDoor = ELEVATOR_EXIT(m_Elevator);
	tDoor = rev_dir[eDoor];
	
	if (!ELEVATOR_DIR(m_Elevator)) {
		eRoom->Send("The doors slide closed, and the %s starts moving.\n", ELEVATOR_NAME(m_Elevator));
		tRoom->Send("The %s doors slide closed.\n", ELEVATOR_NAME(m_Elevator));
	
		eRoom->GetExit(eDoor)->room = tRoom;
		eRoom->GetExit(eDoor)->GetStates().set(EX_STATE_CLOSED);
		tRoom->GetExit(tDoor)->GetStates().set(EX_STATE_CLOSED);
		
		ELEVATOR_DIR(m_Elevator) = ELEVATOR_PREV_DIR(m_Elevator);
		{	//	Pick new direction... erm...
//			ELEVATOR_DIR(elevator) = ELEVATOR_PREV_DIR(elevator);
		}
		
		newTimer = ELEVATOR_DELAY_START(m_Elevator);
	} else {
		if ( ELEVATOR_BUTTON(m_Elevator, ELEVATOR_POS(m_Elevator)) ) {
			ELEVATOR_BUTTON(m_Elevator, ELEVATOR_POS(m_Elevator)) = 0;
			if(!eRoom->people.empty())
				eRoom->Send("The %s stops moving, and the doors slide open.\n", ELEVATOR_NAME(m_Elevator));
			if(!tRoom->people.empty())
				tRoom->Send("The %s arrives, and the doors slide open.\n", ELEVATOR_NAME(m_Elevator));
				
			
			eRoom->GetExit(eDoor)->room = tRoom;
			eRoom->GetExit(eDoor)->GetStates().clear(EX_STATE_CLOSED);
			tRoom->GetExit(tDoor)->GetStates().clear(EX_STATE_CLOSED);
			
			ELEVATOR_PREV_DIR(m_Elevator) = ELEVATOR_DIR(m_Elevator);
			ELEVATOR_DIR(m_Elevator) = EL_STOP;
			newTimer = ELEVATOR_DELAY_STOP(m_Elevator);
		} else
			newTimer = ELEVATOR_DELAY_MOVE(m_Elevator);
	}

	return ((newTimer> 0 ? newTimer : 5) RL_SEC);
}
