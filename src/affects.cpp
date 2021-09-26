/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-2005        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Based on CircleMUD, Copyright 1993-94, and DikuMUD, Copyright 1990-91 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : affects.cp                                                     [*]
[*] Usage: Primary code for Affect class                                  [*]
\***************************************************************************/


#include "characters.h"
#include "objects.h"
#include "utils.h"
#include "comm.h"
#include "affects.h"
#include "event.h"
#include "spells.h"
#include "constants.h"


const Affect::Flags MAKE_BITSET(Affect::AFF_INVISIBLE_FLAGS, AFF_INVISIBLE, AFF_INVISIBLE_2);
const Affect::Flags MAKE_BITSET(Affect::AFF_STEALTH_FLAGS, AFF_INVISIBLE, AFF_INVISIBLE_2, AFF_HIDE, AFF_COVER);
const char * Affect::None = "None";
const char * Affect::Blind = "Blind";
const char * Affect::Charm = "Charm";
const char * Affect::Sleep = "Sleep";
const char * Affect::Poison = "Poison";
const char * Affect::Sneak = "Sneak";
	


Affect::Affect(const char *type, int modifier, int location, Flags flags, const char *terminationMessage)
:	m_Type(type)
,	m_TerminationMessage(terminationMessage)
,	m_Location(location)
,	m_Modifier(modifier)
,	m_Flags(flags)
,	m_pEvent(NULL)
,	m_TimeRemaining(0)
{
}


Affect::Affect(const char *type, int modifier, int location, AffectFlag flag, const char *terminationMessage)
:	m_Type(type)
,	m_TerminationMessage(terminationMessage)
,	m_Location(location)
,	m_Modifier(modifier)
,	MAKE_BITSET(m_Flags, flag)
,	m_pEvent(NULL)
,	m_TimeRemaining(0)
{
}


//	Copy affect and attach to char - a "type" of copy-constructor
Affect::Affect(const Affect &aff, CharData *ch)
:	m_Type(aff.m_Type)
,	m_TerminationMessage(aff.m_TerminationMessage)
,	m_Location(aff.m_Location)
,	m_Modifier(aff.m_Modifier)
,	m_Flags(aff.m_Flags)
,	m_pEvent(NULL)
,	m_TimeRemaining(aff.m_TimeRemaining)
{
	if (aff.m_pEvent)
	{
		m_pEvent = new TerminateEvent(aff.m_pEvent->GetTimeRemaining(), this, ch);
	}
}


Affect::~Affect(void) {
	if (m_pEvent)		m_pEvent->Cancel();
}


const char *Affect::GetLocationName()
{
	return (m_Location >= 0) ? GetEnumName((AffectLoc)m_Location) : skill_name(-m_Location);
}

//	Insert an Affect in a CharData structure
//	Automatically sets apropriate bits and apply's
void Affect::ToChar(CharData * ch, unsigned int duration, AttachAs bPaused)
{
	if (PURGED(ch))
	{
		delete this;
		return;
	}
	
	ch->m_Affects.push_back(this);
	
	if (bPaused == AttachPaused)
	{
		m_TimeRemaining = duration;
	}
	else
	{
		m_pEvent = (duration > 0) ? new TerminateEvent(duration, this, ch) : NULL;
	}

	Apply(ch);
	ch->AffectTotal();
}


void Affect::PauseTimer()
{
	if (!m_pEvent)
		return;
		
	m_TimeRemaining = m_pEvent->GetTimeRemaining();
	m_pEvent->Cancel();
	m_pEvent = NULL;
}


void Affect::ResumeTimer(CharData *ch)
{
	if (m_pEvent || m_TimeRemaining == 0)
		return;
	
	m_pEvent = (m_TimeRemaining > 0) ? new TerminateEvent(m_TimeRemaining, this, ch) : NULL;
	m_TimeRemaining = 0;
}


//	Remove an Affect from a char (called when duration reaches zero).
//	Frees mem and calls affect_modify
void Affect::Remove(CharData *ch, ::Flags flags)
{
	Unapply(ch);
	
	ch->m_Affects.remove(this);
	if (!IS_SET(flags, DontTotal))
	{
		ch->AffectTotal();
	}

	if (IS_SET(flags, WithMessage) && !m_TerminationMessage.empty() && ch->InRoom())
		act(m_TerminationMessage.c_str(), FALSE, ch, NULL, NULL, TO_CHAR);
	
	delete this;
}


void Affect::Join(CharData * ch, unsigned int duration, ::Flags method)
{
	if (PURGED(ch))
	{
		delete this;
		return;
	}
	
	FOREACH(Lexi::List<Affect *>, ch->m_Affects, iter)
	{
		Affect *hjp = *iter;
		
		if (m_Type != hjp->m_Type)
			continue;
		if (m_Location != hjp->m_Location)
			continue;
		if (hjp->m_Location == APPLY_NONE && !hjp->m_Flags.testForAny(m_Flags))
			continue;
		
		if (IS_SET(method, AddDuration) && hjp->m_pEvent)		duration += hjp->GetTime();
		if (IS_SET(method, AverageDuration))					duration = (hjp->GetTime() + duration + 1) / 2;
		if (IS_SET(method, ShortestDuration) && hjp->m_pEvent)	duration = MIN(duration, hjp->GetTime());
		if (IS_SET(method, LongestDuration) && hjp->m_pEvent)	duration = MAX(duration, hjp->GetTime());
		if (IS_SET(method, OldDuration) && hjp->m_pEvent)		duration = hjp->GetTime();
		if (IS_SET(method, AddModifier))						m_Modifier += hjp->m_Modifier;
		if (IS_SET(method, AverageModifier))					m_Modifier = (hjp->m_Modifier + m_Modifier + 1) / 2;
		if (IS_SET(method, SmallestModifier))					m_Modifier = MIN(m_Modifier, hjp->m_Modifier);
		if (IS_SET(method, LargestModifier))					m_Modifier = MAX(m_Modifier, hjp->m_Modifier);
		if (IS_SET(method, OldModifier))						m_Modifier = hjp->m_Modifier;
		
		hjp->m_Flags.clear(m_Flags);
		
		if (hjp->m_Location != APPLY_NONE || hjp->m_Flags.none())
			hjp->Remove(ch, DontTotal);
	}
	ToChar(ch, duration);
}


unsigned int Affect::TerminateEvent::Run()
{
	if (!m_Character)
		return 0;
	
	m_pAffect->m_pEvent = NULL;
	m_pAffect->Remove(m_Character, WithMessage);
	
	return 0;
}


void Affect::Modify(CharData * ch, int loc, int mod, Flags flags, bool add) {
	if (add)
		AFF_FLAGS(ch).set(flags);
	else
	{
		AFF_FLAGS(ch).clear(flags);
		mod = -mod;
	}
	
	if (loc < 0)
	{
		loc = -loc;
		
		if (loc < NUM_SKILLS && !IS_NPC(ch))
			GET_SKILL(ch, loc) = RANGE(GET_SKILL(ch, loc) + mod, 0, /*MAX_SKILL_LEVEL*/ MAX_STAT);
	}
	else
	{
		switch (loc)
		{
			case APPLY_NONE:														break;
			
			case APPLY_STR:		GET_STR(ch) = RANGE(GET_STR(ch) + mod, 1, 1 /*IS_NPC(ch)*/ ? MAX_STAT : MAX_PC_STAT);break;
			case APPLY_HEA:		GET_HEA(ch) = RANGE(GET_HEA(ch) + mod, 1, 1 /*IS_NPC(ch)*/ ? MAX_STAT : MAX_PC_STAT);break;
			case APPLY_COO:		GET_COO(ch) = RANGE(GET_COO(ch) + mod, 1, 1 /*IS_NPC(ch)*/ ? MAX_STAT : MAX_PC_STAT);break;
			case APPLY_AGI:		GET_AGI(ch) = RANGE(GET_AGI(ch) + mod, 1, 1 /*IS_NPC(ch)*/ ? MAX_STAT : MAX_PC_STAT);break;
			case APPLY_PER:		GET_PER(ch) = RANGE(GET_PER(ch) + mod, 1, 1 /*IS_NPC(ch)*/ ? MAX_STAT : MAX_PC_STAT);break;
			case APPLY_KNO:		GET_KNO(ch) = RANGE(GET_KNO(ch) + mod, 1, 1 /*IS_NPC(ch)*/ ? MAX_STAT : MAX_PC_STAT);break;
			
			case APPLY_EXTRA_ARMOR:	GET_EXTRA_ARMOR(ch) += mod;							break;
			
			case APPLY_WEIGHT:	ch->m_Weight = MAX(1, ch->GetWeight() + mod);		break;
			case APPLY_HEIGHT:	ch->m_Height = MAX(1, ch->GetHeight() + mod);		break;
			case APPLY_HIT:		GET_MAX_HIT(ch) = MAX(1, GET_MAX_HIT(ch) + mod);	GET_HIT(ch) += mod;			break;
			case APPLY_MOVE:	GET_MAX_MOVE(ch) = MAX(1, GET_MAX_MOVE(ch) + mod); GET_MOVE(ch) += mod;		break;
			//case APPLY_ENDURANCE:GET_MAX_ENDURANCE(ch) = MAX(0, GET_MAX_ENDURANCE(ch) + mod); GET_ENDURANCE(ch) += mod;		break;
			
	//		case APPLY_ARMOR:		GET_ARMOR(ch) = RANGE(GET_ARMOR(ch) + mod, 0, 32000);		break;
			default:
	//			log("SYSERR: affect_modify: Unknown apply adjust %d attempt", loc);
				break;
		}
	}
	
//	ch->update_pos();
}





/* Call affect_remove with every spell of spelltype "skill" */
void Affect::FromChar(CharData * ch, const char * type) {
	FOREACH(Lexi::List<Affect *>, ch->m_Affects, affect)
		if ((*affect)->m_Type == type)
			(*affect)->Remove(ch, DontTotal | WithMessage);
	ch->AffectTotal();
}


/* Call affect_remove with every spell of spelltype "skill" */
void Affect::SetFadeMessage(CharData * ch, const char * type, const char *msg)
{
	Affect *bestAffect = NULL;
	
	FOREACH(Lexi::List<Affect *>, ch->m_Affects, iter)
	{
		Affect *hjp = *iter;
		
		if (hjp->m_Type != type)
			continue;
		
		if (!bestAffect)
			bestAffect = hjp;
		else if (!hjp->m_TerminationMessage.empty())
		{
			bestAffect = hjp;
			break;
		}
	}
	
	if (bestAffect)
	{
		bestAffect->m_TerminationMessage = msg;
	}
}


void Affect::FlagsFromChar(CharData * ch, Flags flags) {
	
	FOREACH(Lexi::List<Affect *>, ch->m_Affects, iter)
	{
		Affect *hjp = *iter;
	
		if (!hjp->m_Flags.testForAny(flags))
			continue;
		
		hjp->m_Flags.clear(flags);
		
		if (hjp->m_Location == APPLY_NONE && hjp->m_Flags.none())
			hjp->Remove(ch, DontTotal | WithMessage);
	}
	
	REMOVE_BIT(AFF_FLAGS(ch), flags);
	
	ch->AffectTotal();
}




/*
 * Return if a char is affected by a spell (SPELL_XXX), NULL indicates
 * not affected
 */
bool Affect::IsAffectedBy(CharData * ch, const char * type)
{
	FOREACH(Lexi::List<Affect *>, ch->m_Affects, affect)
		if ((*affect)->m_Type == type)
			return true;

	return false;
}
