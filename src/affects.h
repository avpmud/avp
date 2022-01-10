/*************************************************************************
*   File: affects.h                  Part of Aliens vs Predator: The MUD *
*  Usage: Header for affects                                             *
*************************************************************************/

#ifndef __AFFECTS_H__
#define __AFFECTS_H__

#include "types.h"
#include "event.h"
#include "bitflags.h"

enum AffectFlag
{
	AFF_BLIND,				
	AFF_INVISIBLE,
	AFF_INVISIBLE_2,
	AFF_SENSE_LIFE,
	AFF_DETECT_INVIS,
	AFF_SANCTUARY,
	AFF_GROUP,
	AFF_FLYING,
	AFF_INFRAVISION,
	AFF_POISON,
	AFF_SLEEP,
	AFF_NOTRACK,
	AFF_SNEAK,
	AFF_HIDE,
	AFF_CHARM,
	AFF_MOTIONTRACKING,
	AFF_SPACESUIT,
	AFF_LIGHT,
	AFF_TRAPPED,
	AFF_STUNNED,
	AFF_ACIDPROOF,
	AFF_BLEEDING,
	AFF_NORECALL,
	AFF_GROUPTANK,
	AFF_COVER,
	AFF_TRAITOR,
	AFF_NOAUTOFOLLOW,
	AFF_DYING,
	AFF_DIDFLEE,
	AFF_DIDSHOOT,
	AFF_NOSHOOT,
	AFF_NOQUIT,
	AFF_RESPAWNING,
	NUM_AFF_FLAGS
};


//	Modifier constants used with obj affects ('A' fields)
enum AffectLoc
{
	APPLY_NONE = 0,			//	No effect
	
	APPLY_STR,
	APPLY_HEA,
	APPLY_COO,
	APPLY_AGI,
	APPLY_PER,
	APPLY_KNO,
	
	APPLY_EXTRA_ARMOR,
	
	APPLY_PROXY,			//	To add Affects without having an actual flag
	
	//	9-10 unused

	APPLY_WEIGHT = 11,		//	Apply to weight
	APPLY_HEIGHT,			//	Apply to height
	APPLY_HIT,				//	Apply to max hit points
	APPLY_MOVE,				//	Apply to max move points
//	APPLY_ENDURANCE,		//	Apply to max endurance points

	NUM_APPLY_FLAGS				//	Apply the flags
};


class Affect {
public:

	typedef Lexi::BitFlags<NUM_AFF_FLAGS, AffectFlag> Flags;
	static const Flags AFF_INVISIBLE_FLAGS;
	static const Flags AFF_STEALTH_FLAGS;

	static const char * None;
	static const char * Blind;
	static const char * Charm;
	static const char * Sleep;
	static const char * Poison;
	static const char * Sneak;

//	enum AffectType { None, Blind, Charm, Sleep, Poison, Sneak, Burdened, OverBurdened, Script, ScriptStackable, NumberAffects };	//	0-5

	enum {
		AddDuration =		1 << 0,
		AverageDuration =	1 << 1,
		ShortestDuration =	1 << 2,
		LongestDuration =	1 << 3,
		OldDuration =		1 << 4,
		AddModifier = 		1 << 5,
		AverageModifier =	1 << 6,
		SmallestModifier =	1 << 7,
		LargestModifier =	1 << 8,
		OldModifier =		1 << 9
	};
	
					Affect(const char * type, int modifier, int location, AffectFlag flag, const char *terminationMessage = "");
					Affect(const char * type, int modifier, int location, Flags bitvector, const char *terminationMessage = "");
					Affect(const Affect &aff, CharData *ch);
					~Affect(void);

	enum
	{
		DontTotal	= 1 << 0,
		WithMessage	= 1 << 1
	};
	
	enum AttachAs
	{
		AttachRunning = false,
		AttachPaused = true
	};
	
	void			Remove(CharData *ch, ::Flags flags);
	void			ToChar(CharData * ch, unsigned int duration = 0, AttachAs bPaused = AttachRunning);
	void			Join(CharData * ch, unsigned int duration = 0, ::Flags method = 0);
	
	void			PauseTimer();
	void			ResumeTimer(CharData *ch);
	
	void			Unapply(CharData *ch) { Modify(ch, m_Location, m_Modifier, m_Flags, false); }
	void			Apply(CharData *ch) { Modify(ch, m_Location, m_Modifier, m_Flags, true); }

	const char *	GetType() { return m_Type.c_str(); }
	const char *	GetTerminationMessage() { return m_TerminationMessage.c_str(); }
	unsigned int	GetTime() { return m_pEvent ? m_pEvent->GetTimeRemaining() : m_TimeRemaining; }
	int				GetLocation() { return m_Location; }
	const char *	GetLocationName();
	int				GetModifier() { return m_Modifier; }
	const Flags &	GetFlags() { return m_Flags; }
	
	static void		FromChar(CharData * ch, const char * type);
	static void		FlagsFromChar(CharData * ch, Flags flags);
	static void		FlagsFromChar(CharData * ch, AffectFlag flag) { FlagsFromChar(ch, MAKE_BITSET(Affect::Flags, flag)); }
	static void		Modify(CharData * ch, int loc, int mod, Flags flags, bool add);
	static bool		IsAffectedBy(CharData * ch, const char * type);
	static void		SetFadeMessage(CharData * ch, const char * type, const char *msg);

private:
	class TerminateEvent : public Event
	{
	public:
							TerminateEvent(unsigned int time, Affect *affect, CharData *ch)
								: Event(time), m_pAffect(affect), m_Character(ch) { }
	
		virtual const char *GetType() const { return "Affect"; }

	private:
		virtual unsigned int	Run();

		Affect *			m_pAffect;
		CharData *			m_Character;
	};

	Lexi::String	m_Type;			//	The type of affect that caused this
	Lexi::String	m_TerminationMessage;		//	The message to display on termination
	int				m_Location;		//	Tells which ability to change(APPLY_XXX)
	int				m_Modifier;		//	This is added to apropriate ability
	Flags			m_Flags;		//	Tells which bits to set (AFF_XXX)
	
	TerminateEvent *m_pEvent;
	int				m_TimeRemaining;//	For paused affects
};

#endif

