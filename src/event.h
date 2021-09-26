

#ifndef __EVENT_H__
#define __EVENT_H__

#include "lexilist.h"


//	The NEW event system

class Event
{
public:
						Event(unsigned int when);
	virtual				~Event() {}
	
	virtual const char *GetType() const = 0;
	unsigned int		GetTimeRemaining() const;
	
	void				Reschedule(unsigned int when);
	void				Cancel();

private:
	unsigned int		Execute();
	virtual unsigned int	Run() = 0;

	class QueueElement *m_pQueue;
	bool				m_bRunning;

						Event(const Event &);
	void				operator=(const Event &);

public:
	typedef Lexi::List<Event *>	List;
	
	static void			ProcessEvents();
	static Event *		FindEvent(List & list, const char * type);
};


class GravityEvent : public Event
{
public:
						GravityEvent(unsigned int when, CharData *faller) : Event(when), m_Character(faller), m_NumRoomsFallen(0)
						{}
	
	static const char *	Type;
	virtual const char *GetType() const { return Type; }

private:
	virtual unsigned int	Run();

	CharData *			m_Character;
	int					m_NumRoomsFallen;
};


class GrenadeExplosionEvent : public Event
{
public:
						GrenadeExplosionEvent(unsigned int when, ObjData *grenade, IDNum puller) :
							Event(when), m_Grenade(grenade), m_Puller(puller)
						{}
	
	static const char *	Type;
	virtual const char *GetType() const { return Type; }

private:
	virtual unsigned int	Run();

	ObjData *			m_Grenade;
	IDNum				m_Puller;
};



class CombatEvent : public Event
{
public:
						CombatEvent(unsigned int when, CharData *attacker, bool ranged = false) :
							Event(when), m_Attacker(attacker), m_Weapon(NULL), m_bIsRanged(ranged), m_PulsesSinceLastAttack(0)
						{}
	
						CombatEvent(unsigned int when, CharData *attacker, ObjData *weapon, bool ranged = false) :
							Event(when), m_Attacker(attacker), m_Weapon(weapon), m_bIsRanged(ranged), m_PulsesSinceLastAttack(0)
						{}
						
	static const char *	Type;
	virtual const char *GetType() const { return Type; }
	
	bool				IsRanged() const { return m_bIsRanged; }
	void				BecomeNonRanged() { m_bIsRanged = false; }
	
	ObjData *			GetWeapon() const { return m_Weapon; }

private:
	virtual unsigned int	Run();

	CharData *			m_Attacker;
	ObjData *			m_Weapon;
	bool				m_bIsRanged;
	int					m_PulsesSinceLastAttack;
};


#endif
