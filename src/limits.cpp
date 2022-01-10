/* ************************************************************************
*   File: limits.c                                      Part of CircleMUD *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "structs.h"
#include "utils.h"
#include "buffer.h"
#include "spells.h"
#include "comm.h"
#include "scripts.h"
#include "db.h"
#include "handler.h"
#include "find.h"
#include "event.h"
#include "house.h"

void hour_update(void);
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6);

void AlterHit(CharData *ch, int amount);
void AlterMove(CharData *ch, int amount);
void AlterEndurance(CharData *ch, int amount);
void CheckRegenRates(CharData *ch);

void timer_otrigger(ObjData *obj);

/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6) {
	if (age < 15)
		return (p0);		/* < 15   */
	else if (age <= 29)
		return (int) (p1 + (((age - 15) * (p2 - p1)) / 15));	/* 15..29 */
	else if (age <= 44)
		return (int) (p2 + (((age - 30) * (p3 - p2)) / 15));	/* 30..44 */
	else if (age <= 59)
		return (int) (p3 + (((age - 45) * (p4 - p3)) / 15));	/* 45..59 */
	else if (age <= 79)
		return (int) (p4 + (((age - 60) * (p5 - p4)) / 20));	/* 60..79 */
	else
		return (p6);		/* >= 80 */
}


//	Player point types for events
enum
{
	REGEN_HIT = 0,
	REGEN_MOVE,
//	REGEN_ENDURANCE,
	NUM_REGENS
};

class RegenEvent : public Event
{
public:
						RegenEvent(unsigned int when, CharData *ch, int type) : Event(when), m_Character(ch), m_Type(type)
						{}
	
	virtual const char *GetType() const { return "Regen"; }

private:
	virtual unsigned int	Run();

	CharData *			m_Character;
	int					m_Type;
};


unsigned int RegenEvent::Run() {
	CharData *ch = m_Character;
	int	gain;
	
	if (!ch)
		return 0;
	
	if (GET_POS(ch) >= POS_STUNNED) {	//	No help for the dying
		switch (m_Type) {
			case REGEN_HIT:
				if (IS_SYNTHETIC(ch))
					break;
				
				if (AFF_FLAGGED(ch, AFF_BLEEDING))
				{
					GET_HIT(ch) = GET_HIT(ch) - ((GET_MAX_HIT(ch) / 50) * 4);	//	2% per second, once every 4 seconds
					ch->Send("`RYou are bleeding heavily!`n\n");
					
					if (GET_HIT(ch) < -99)
						GET_HIT(ch) = -99;
					
					ch->update_pos();
				}
				else //if (!AFF_FLAGGED(ch, AFF_NOQUIT))
				{
					float skill = ch->GetEffectiveSkill(SKILL_HEALING);
					
					if (GET_POS(ch) == POS_RESTING)			skill += 5;
					else if (GET_POS(ch) == POS_SLEEPING)	skill += 10;
					
					//gain = skill - Bellcurve();
					gain = (int)SkillRoll(skill);
					gain = MAX(gain, 1);
					
					if (!AFF_FLAGGED(ch, AFF_NOQUIT))
						gain = gain * 2;
					
					GET_HIT(ch) = MIN(GET_HIT(ch) + gain, GET_MAX_HIT(ch));
					
					//if (GET_POS(ch) == POS_STUNNED)
						ch->update_pos();
				}
				
				
				if (GET_HIT(ch) < GET_MAX_HIT(ch)) {
					gain = hit_gain(ch);
					return (/* PULSES_PER_MUD_HOUR / */ (gain ? gain : 1));
				}
				break;
			case REGEN_MOVE:
//				if (!FIGHTING(ch) /* && !AFF_FLAGGED(ch, AFF_INVISIBLE_FLAGS | AFF_HIDE) */)
//					gain = 1;
				
//				switch (ch->GetRace())
//				{
//					case RACE_HUMAN:	gain += GET_SKILL(ch, SKILL_HEALING) / 30;	break;
//					case RACE_SYNTHETIC:gain += GET_SKILL(ch, SKILL_HEALING) / 40;	break;
//					case RACE_PREDATOR:	gain += GET_SKILL(ch, SKILL_HEALING) / 20;	break;
//					case RACE_ALIEN:	gain += GET_SKILL(ch, SKILL_HEALING) / 24;	break;
//					default:	break; //	shut up the compiler warning
//				}
				
				GET_MOVE(ch) = MIN(GET_MOVE(ch) + 1, GET_MAX_MOVE(ch));
				
				if (GET_MOVE(ch) < GET_MAX_MOVE(ch))
				{
					gain = move_gain(ch);
					return (/* PULSES_PER_MUD_HOUR / */(gain ? gain : 1));
				}
				break;
#if 0
			case REGEN_ENDURANCE:
//				if (!FIGHTING(ch) /* && !AFF_FLAGGED(ch, AFF_INVISIBLE_FLAGS | AFF_HIDE) */)
					GET_ENDURANCE(ch) = MIN(GET_ENDURANCE(ch) + 1, GET_MAX_ENDURANCE(ch));
			
				if (GET_ENDURANCE(ch) < GET_MAX_ENDURANCE(ch)) {
					gain = endurance_gain(ch);
					return (/* PULSES_PER_MUD_HOUR / */(gain ? gain : 1));
				}
				break;
#endif
			default:
				log("SYSERR:  Unknown points event type %d", m_Type);
		}
	}
	GET_POINTS_EVENT(ch, m_Type) = NULL;
	return 0;
}


//	Subtract amount of HP from ch's current and start points event
void AlterHit(CharData *ch, int amount) {
	int				time;
	int				gain;
	
	GET_HIT(ch) = MIN(GET_HIT(ch) + amount, GET_MAX_HIT(ch));
	
	ch->update_pos();
	
	if ((GET_POS(ch) < POS_INCAP)/* || (ch->GetRace() == RACE_SYNTHETIC)*/)
		return;
	
	if ((GET_HIT(ch) < GET_MAX_HIT(ch)) && !GET_POINTS_EVENT(ch, REGEN_HIT)) {
		gain = hit_gain(ch);
		time = /* PULSES_PER_MUD_HOUR / */ (gain ? gain : 1);
		GET_POINTS_EVENT(ch, REGEN_HIT) = new RegenEvent(time, ch, REGEN_HIT);
	}
	
//	if (amount >= 0) {	//	Already done...
//		ch->update_pos();
//	}
}

/* 
//	Subtracts amount of mana from ch's current and starts points event
void AlterMana(CharData *ch, int amount) {
	struct RegenEvent *regen;
	int	time;
	int	gain;

	GET_MANA(ch) = MIN(GET_MANA(ch) - amount, GET_MAX_MANA(ch));
	
	if ((GET_MANA(ch) < GET_MAX_MANA(ch)) && !GET_POINTS_EVENT(ch, REGEN_MANA)) {
		if (GET_POS(ch) >= POS_STUNNED) {
			CREATE(regen, struct RegenEvent, 1);
			regen->ch = ch;
			regen->type = REGEN_MANA;
			gain = mana_gain(ch);
			time = PULSES_PER_MUD_HOUR / (gain ? gain : 1);
			GET_POINTS_EVENT(ch, REGEN_MANA) = new Event(regen_event, regen, time, NULL);
		}
	}
}
*/


void AlterMove(CharData *ch, int amount) {
	int	time;
	int	gain;

	GET_MOVE(ch) = MIN(GET_MOVE(ch) + amount, GET_MAX_MOVE(ch));
	
	if ((GET_MOVE(ch) < GET_MAX_MOVE(ch)) && !GET_POINTS_EVENT(ch, REGEN_MOVE)) {
		if (GET_POS(ch) >= POS_STUNNED) {
			gain = move_gain(ch);
			time = /* PULSES_PER_MUD_HOUR / */(gain ? gain : 1);
			GET_POINTS_EVENT(ch, REGEN_MOVE) = new RegenEvent(time, ch, REGEN_MOVE);
		}
	}
}


#if 0
void AlterEndurance(CharData *ch, int amount) {
	int	time;
	int	gain;

	GET_ENDURANCE(ch) = MIN(GET_ENDURANCE(ch) - amount, GET_MAX_ENDURANCE(ch));
	
	if ((GET_ENDURANCE(ch) < GET_MAX_ENDURANCE(ch)) && !GET_POINTS_EVENT(ch, REGEN_ENDURANCE)) {
		if (GET_POS(ch) >= POS_STUNNED) {
			gain = endurance_gain(ch);
			time = /* PULSES_PER_MUD_HOUR / */(gain ? gain : 1);
			GET_POINTS_EVENT(ch, REGEN_ENDURANCE) = new RegenEvent(time, ch, REGEN_ENDURANCE);
		}
	}
}
#endif


//	The higher, the faster
int hit_gain(CharData * ch) {
	//	Hitpoint gain pr. game hour
	int gain;

	//	5 - 500 pulses
	//	1-10 pulses
		
	gain = /*15*/ 12;	//	20% speedup
		
	if (AFF_FLAGGED(ch, AFF_BLEEDING))	return 40;
	
	//	Race/Level calculations

	//	Skill/Spell calculations

	//	Position calculations
/*	switch (GET_POS(ch)) {
		case POS_SLEEPING:	gain /= 3;			break;		//	Triple
		case POS_RESTING:	gain /= 2;			break;		//	Double
		case POS_SITTING:	gain /= 1.5;		break;		//	One and a half
		default:								break;
	}
*/

//	if (AFF_FLAGGED(ch, AFF_POISON))		//	Half
//		gain *= 2;

//	Disabled until food is more common.
//	if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
//		gain *= 2;							//	Half yet again

	if (AFF_FLAGS(ch).testForAny(Affect::AFF_INVISIBLE_FLAGS))	gain *= 3;	//	3x slower
		
	return gain;
}



/* move gain pr. game hour */
int move_gain(CharData * ch)
{
	float gain;

	//	10 - 1000 pulses
	gain = 10.0f; //1000 / (GET_TOU(ch) > 10 ? GET_TOU(ch) : 10); //	graf(age(ch).year, 16, 20, 24, 20, 16, 12, 10);
	
	//	Position calculations
	switch (GET_POS(ch)) {
		case POS_SLEEPING:	gain *= 0.33f;		break;		//	Triple
		case POS_RESTING:	gain *= 0.50f;		break;		//	Double
		case POS_SITTING:	gain *= 0.66f;		break;		//	One and a half
		default:								break;
	}

	if (AFF_FLAGGED(ch, AFF_BLEEDING))			gain *= 2;
	
	if (!IS_SYNTHETIC(ch))
	{
		if (AFF_FLAGGED(ch, AFF_POISON))		gain *= 2;
//		if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))	gain *= 2;
	}
	if (ch->GetRace() == RACE_ALIEN)				gain *= 0.5f;	//	Double
//	if (AFF_FLAGGED(ch, AFF_INVISIBLE_FLAGS))	gain *= 4;	//	4x slower
		
	return (int)gain;
}


/* move gain pr. game hour */
int endurance_gain(CharData * ch) {
	float gain;

	//	10 - 1000 pulses
	gain = 10.0f; //1000 / (GET_TOU(ch) > 10 ? GET_TOU(ch) : 10); //	graf(age(ch).year, 16, 20, 24, 20, 16, 12, 10);
	
	//	Position calculations
	switch (GET_POS(ch)) {
		case POS_SLEEPING:	gain *= 0.33f;		break;		//	Triple
		case POS_RESTING:	gain *= 0.50f;		break;		//	Double
		case POS_SITTING:	gain *= 0.66f;		break;		//	One and a half
		default:								break;
	}

	if (AFF_FLAGGED(ch, AFF_BLEEDING))			gain *= 2;
	
	return (int)gain;
}



void CheckRegenRates(CharData *ch) {
	int				gain, time;
	
	if (GET_POS(ch) <= POS_INCAP)
		return;
	
	for (int type = 0; type < NUM_REGENS; type++) {
		switch (type) {
			case REGEN_HIT:
				if (GET_HIT(ch) >= GET_MAX_HIT(ch) && !AFF_FLAGGED(ch, AFF_BLEEDING))		continue;
				else 									gain = hit_gain(ch);
				break;
			case REGEN_MOVE:
				if (GET_MOVE(ch) >= GET_MAX_MOVE(ch))	continue;
				else									gain = move_gain(ch);
				break;
#if 0
			case REGEN_ENDURANCE:
				if (GET_ENDURANCE(ch) >= GET_MAX_ENDURANCE(ch))	continue;
				else									gain = endurance_gain(ch);
				break;
#endif
			default:
				continue;
		}
		time = /* PULSES_PER_MUD_HOUR / */ (gain ? gain : 1);
	
		if (!GET_POINTS_EVENT(ch, type) || (time < GET_POINTS_EVENT(ch, type)->GetTimeRemaining())) {
			if (GET_POINTS_EVENT(ch, type))	GET_POINTS_EVENT(ch, type)->Cancel();
				
			GET_POINTS_EVENT(ch, type) = new RegenEvent(time, ch, type);
		}
	}
}


#define READ_TITLE(ch, lev) (ch->GetSex() == SEX_MALE ?   \
	titles[(int)ch->GetRace()][lev].title_m :  \
	titles[(int)ch->GetRace()][lev].title_f)


void gain_condition(CharData * ch, int condition, int value) {
#if 0
	int intoxicated;

	if (GET_COND(ch, condition) == -1)	/* No change */
		return;

	intoxicated = (GET_COND(ch, DRUNK) > 0);

	GET_COND(ch, condition) += value;

	/* update regen rates if we were just on empty */
	if ((condition != DRUNK) && (value > 0) && (GET_COND(ch, condition) == value))
		CheckRegenRates(ch);
		
	GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
	GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

	if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
		return;

	switch (condition) {
		case FULL:
			send_to_char("You are hungry.\n", ch);
			return;
		case THIRST:
			send_to_char("You are thirsty.\n", ch);
			return;
		case DRUNK:
			if (intoxicated)
				send_to_char("You are now sober.\n", ch);
			return;
		default:
			break;
	}
#endif
}


void check_idling(CharData * ch)
{
	if (++ch->GetPlayer()->m_IdleTime > 8 && !IS_STAFF(ch)
		&& !House::GetHouseForRoom(ch->InRoom()))
	{
		if (ch->WasInRoom() == NULL && ch->InRoom())
		{
			ch->SetWasInRoom(ch->InRoom());
			act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
			send_to_char("You have been idle, and are pulled into a void.\n", ch);
			ch->Save();
			SavePlayerObjectFile(ch);
			ch->FromRoom();
			ch->ToRoom(world[1]);
		}
		else if (ch->GetPlayer()->m_IdleTime > 48)
		{
			if (ch->InRoom())
				ch->FromRoom();
			else
				mudlogf(BRF, LVL_STAFF, TRUE, "%s in NOWHERE when idle-extracted.", ch->GetName());
			ch->ToRoom(world[3]);
			if (ch->desc)
			{
				ch->desc->GetState()->Push(new DisconnectConnectionState);
				ch->desc->m_Character = NULL;
				ch->desc = NULL;
			}
			mudlogf(CMP, LVL_STAFF, TRUE, "%s idle-extracted.", ch->GetName());
			SavePlayerObjectFile(ch, SavePlayerLogout);
			ch->Extract();
		}
	}
	else if (ch->GetPlayer()->m_IdleTime < 2)
	{
		if (ch->GetPlayer()->m_Karma < 0)		ch->GetPlayer()->m_Karma += MAX(1, -ch->GetPlayer()->m_Karma / 10);
		else if (ch->GetPlayer()->m_Karma > 0)	ch->GetPlayer()->m_Karma -= MAX(1, ch->GetPlayer()->m_Karma / 10);
	}
}


void hour_update(void)
{
	/* characters */
	FOREACH(CharacterList, character_list, iter)
	{
		CharData *i = *iter;
		
		if (0 && i->GetRace() != RACE_SYNTHETIC) {
			gain_condition(i, FULL, -1);
			gain_condition(i, DRUNK, -1);
			gain_condition(i, THIRST, -1);
		}
		if (!IS_NPC(i)) {
			check_idling(i);
			i->update_objects();
			
			if (!PRF_FLAGGED(i, PRF_PK))
			{
				int time_passed = (time(0) - i->GetPlayer()->m_LoginTime) + i->GetPlayer()->m_TotalTimePlayed;
				if (time_passed > (60 * 60 * 5))	//	5 hours
				{
					i->Send("`*`f`RYour PK-safe amnesty period has expired.  You can now be attacked by other players.`n\n");
					SET_BIT(PRF_FLAGS(i), PRF_PK);
				}
			}
		}
	}
	
	/* objects */
	FOREACH(ObjectList, object_list, iter)
	{
		ObjData *j = *iter;
		
		if ((j->InRoom() == NULL) && !j->Inside() && !j->CarriedBy() && !j->WornBy()) {
			j->Extract();
			continue;
		}
		/* If this is a temporary object */
		if (GET_OBJ_TIMER(j) > 0) {
			GET_OBJ_TIMER(j)--;

			if (!GET_OBJ_TIMER(j)) {
				timer_otrigger(j);
				if (PURGED(j))
					continue;
				
				if (j->CarriedBy())
					act("$p decays in your hands.", FALSE, j->CarriedBy(), j, 0, TO_CHAR);
				else if (j->WornBy())
					act("$p decays.", FALSE, j->WornBy(), j, 0, TO_CHAR);
				else if (j->InRoom())
					act("$p falls apart.", FALSE, 0, j, 0, TO_ROOM);
				if ((GET_OBJ_TYPE(j) == ITEM_CONTAINER) && !j->GetValue(OBJVAL_CONTAINER_CORPSE))
				{
					FOREACH(ObjectList, j->contents, iter)
					{
						ObjData *jj = *iter;
						
						jj->FromObject();

						if (j->Inside())				jj->ToObject(j->Inside());
						else if (j->CarriedBy())		jj->ToChar(j->CarriedBy());
						else if (j->InRoom())			jj->ToRoom(j->InRoom());
						else if (j->WornBy())			jj->ToChar(j->WornBy());
						else							core_dump();
					}
				}
				j->Extract();
			}
		}
	}
}


/* Update PCs, NPCs, and objects */
void point_update(void) {
	/* characters */
	FOREACH(CharacterList, character_list, iter)
	{
		CharData *i = *iter;
/*		if (GET_POS(i) == POS_STUNNED) {
			if (AFF_FLAGGED(i, AFF_POISON))
				damage(i, i, 2, SPELL_POISON);
		}
*/
//		if (!AFF_FLAGGED(i, AFF_NOQUIT))
//			GET_ROLL_STE(i) = Bellcurve(GET_STE(i));
//		GET_ROLL_BRA(i) = Bellcurve(GET_BRA(i));
		
		if (GET_POS(i) == POS_STUNNED) {
			if (GET_HIT(i) <= 0 && i->GetRace() == RACE_SYNTHETIC)
				damage(NULL, i, NULL, 30, TYPE_SUFFERING, DAMAGE_UNKNOWN, 1);
			i->update_pos();
		} else if (GET_POS(i) == POS_INCAP)
			damage(NULL, i, NULL, 30, TYPE_SUFFERING, DAMAGE_UNKNOWN, 1);
		else if (GET_POS(i) <= POS_MORTALLYW)
			damage(NULL, i, NULL, 40, TYPE_SUFFERING, DAMAGE_UNKNOWN, 1);
	}
}
