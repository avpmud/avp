
#include "structs.h"
#include "utils.h"
#include "find.h"
#include "comm.h"
#include "handler.h"
#include "event.h"
#include "spells.h"
#include "constants.h"

void explosion(ObjData *obj, CharData *ch, int dam, int type, RoomData *room, int shrapnel);
int greet_mtrigger(CharData *actor, int dir);
int entry_mtrigger(CharData *ch);
int enter_wtrigger(RoomData *room, CharData *actor, int dir);
int greet_otrigger(CharData *actor, int dir);
int leave_mtrigger(CharData *actor, int dir);
int leave_otrigger(CharData *actor, int dir);
int leave_wtrigger(RoomData *room, CharData *actor, int dir);


unsigned int GrenadeExplosionEvent::Run()
{
	CharData *			ch = CharData::Find(m_Puller);
	ObjData *			obj = m_Grenade;
	
	obj->m_Events.remove(this);
	
	/*	grenades are activated by pulling the pin - ie, setting the
		one of the extra flag bits. After the pin is pulled the grenade
		starts counting down. once it reaches zero, it explodes. */

	RoomData *	room = obj->Room();
	
	/* then truly this grenade is nowhere?!? */
	if (room == NULL)
	{
		log("Serious problem, grenade truly in nowhere");
	}
	else
	{
		if (obj->InRoom())				obj->FromRoom();
		else if (obj->WornBy())			obj = unequip_char(obj->WornBy(), obj->WornOn());
		else if (obj->CarriedBy())		obj->FromChar();
		else if (obj->Inside())			obj->FromObject();
		
		obj->ToRoom(room);
		
		if (ROOM_FLAGGED(room, ROOM_PEACEFUL)) {		/* peaceful rooms */
			act("You hear $p explode harmlessly, with a loud POP!",
					FALSE, 0, obj, 0, TO_ROOM);
		} else {
			act("You hear a loud explosion as $p explodes!\n",
					FALSE, 0, obj, 0, TO_ROOM);

			explosion(obj, ch, obj->GetValue(OBJVAL_GRENADE_DAMAGE), obj->GetValue(OBJVAL_GRENADE_DAMAGETYPE), room, TRUE);
		}
	}
	obj->Extract();
	
	return 0;
}


unsigned int GravityEvent::Run()
{
	int				dam;
	CharData *		faller = m_Character;

	if (!faller)
		return 0;	// Problemo...
	
	RoomData *was_in = IN_ROOM(faller);

	faller->m_Events.remove(this);
	
	/* Character is falling... hehe */
	if (NO_STAFF_HASSLE(faller) || (faller->InRoom() == NULL) || !RoomData::CanGo(faller, DOWN, true) ||
			(AFF_FLAGGED(faller, AFF_FLYING) && !AFF_FLAGGED(faller, AFF_NOQUIT)) ||
			!ROOM_FLAGGED(IN_ROOM(faller), ROOM_GRAVITY)) {
		return 0;
	}
	
	if (	!leave_mtrigger(faller, DOWN) || IN_ROOM(faller) != was_in
		||	!leave_otrigger(faller, DOWN) || IN_ROOM(faller) != was_in
		||	!leave_wtrigger(faller->InRoom(), faller, DOWN) || IN_ROOM(faller) != was_in
		||  !Exit::IsPassable(faller, DOWN)
		||	!enter_wtrigger(Exit::Get(faller, DOWN)->ToRoom(), faller, DOWN) || IN_ROOM(faller) != was_in)
	{
		return 0;
	}

	if (m_NumRoomsFallen < 3)	act("$n falls down!\n", TRUE, faller, 0, 0, TO_ROOM);
	
	if (m_NumRoomsFallen == 0)	act("You are falling!\n", TRUE, faller, 0, 0, TO_CHAR);
	else						act("You fall!\n", TRUE, faller, 0, 0, TO_CHAR);
	
	++m_NumRoomsFallen;
	
	faller->FromRoom();
	faller->ToRoom(Exit::Get(was_in, DOWN)->ToRoom());
	
	look_at_room(faller);
	
	if (m_NumRoomsFallen < 3)	act("$n falls from above!", TRUE, faller, 0, 0, TO_ROOM);
	else						act("Something barely misses you as it comes screaming down from above!\n", TRUE, faller, 0, 0, TO_ROOM);
	
	if ((!AFF_FLAGGED(faller, AFF_FLYING) || AFF_FLAGGED(faller, AFF_NOQUIT))
		&&	  (IN_ROOM(faller) == was_in
			|| !ROOM_FLAGGED(IN_ROOM(faller), ROOM_GRAVITY)
			|| !RoomData::CanGo(faller, DOWN, true)
		 ))
	{
		const char *	hitMessage = (m_NumRoomsFallen <= 2) ? "thud" : "splat";
		
		act("You hit the ground with a $T.", TRUE, faller, 0, hitMessage, TO_CHAR);
		act("$n hits the ground with a $T.", TRUE, faller, 0, hitMessage, TO_ROOM);
		
		dam = m_NumRoomsFallen * m_NumRoomsFallen * 200;
		if (dam > (GET_HIT(faller) + 99))
			dam = GET_HIT(faller) + 99;
		
		damage(NULL, faller, NULL, dam, TYPE_GRAVITY, DAMAGE_UNKNOWN, 1);

		if (!PURGED(faller)) {
			if (GET_POS(faller) == POS_MORTALLYW)
				act("Damn... you broke your neck... you can't move... all you can do is...\nLie here until you die.",
						TRUE, faller, 0, 0, TO_CHAR);
			greet_mtrigger(faller, DOWN);
			greet_otrigger(faller, DOWN);
		}
		
		return 0;
	}
	
	if (!AFF_FLAGGED(faller, AFF_FLYING) || AFF_FLAGGED(faller, AFF_NOQUIT))
		faller->m_Events.push_back(this);
	
	greet_mtrigger(faller, DOWN);
	greet_otrigger(faller, DOWN);
	return ((AFF_FLAGGED(faller, AFF_FLYING) && !AFF_FLAGGED(faller, AFF_NOQUIT)) ? 0 : MAX(4 - m_NumRoomsFallen, 1) RL_SEC);
}


const char *	GravityEvent::Type = "Gravity";
const char *	GrenadeExplosionEvent::Type = "GrenadeExplosion";
const char *	CombatEvent::Type = "Combat";
