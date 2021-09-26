


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "spells.h"
#include "event.h"
#include "find.h"
#include "affects.h"

int combat_event(CharData *ch, CharData *victim, ObjData *weapon, int info);
int combat_delay(CharData *ch, ObjData *weapon, bool ranged = false);
ObjData *get_combat_weapon(CharData *ch, CharData *victim);
void fight_mtrigger(CharData *ch);
void hitprcnt_mtrigger(CharData *ch);
bool kill_valid(CharData *ch, CharData *vict);
void combat_reset_timer(CharData *ch);
int fire_missile(CharData *ch, CharData *victim, ObjData *missile, int range, int dir);

// Base speeds lowered ("sped up") by Hades by -2

int base_response[NUM_RACES+1] = {
	5,	//	12 seconds for human
	4,	//	8 seconds for synthetics
	4,	//	10 seconds for predators
	3,	//	8 seconds for aliens
	5	//	10 seconds for animals
};

int combat_delay(CharData *ch, ObjData *weapon, bool ranged) {
	int	result = 5 RL_SEC;

#if 0
	int mode;
	
	mode = GET_COMBAT_MODE(ch);
	
	if (ranged)
		mode = COMBAT_MODE_NORMAL;
	
	switch (mode)
	{
		case COMBAT_MODE_DEFENSIVE:		result = (IS_ALIEN(ch) ? /*4*/ 6 : 6) RL_SEC;
		case COMBAT_MODE_NORMAL:		result = (IS_ALIEN(ch) ? /*3*/ 4 : 4.5) RL_SEC;
		case COMBAT_MODE_OFFENSIVE:		result = (IS_ALIEN(ch) ? 2 : 3) RL_SEC;
		default:						result = (IS_ALIEN(ch) ? 4 : 6) RL_SEC;
	}
#endif
	
	if (weapon)
	{
		if (weapon->GetFloatValue(OBJFVAL_WEAPON_SPEED) >= 0.1f)
			result = (int)(result * weapon->GetFloatValue(OBJFVAL_WEAPON_SPEED));
		else
			result = (int)(result * 0.1f);
	} else if (IS_NPC(ch) && ch->mob->m_AttackSpeed >= 0.1f && ch->mob->m_AttackSpeed <= 10.0f)
		result = (int)(result * ch->mob->m_AttackSpeed);
	
	
	return result;
}


ObjData *get_combat_weapon(CharData *ch, CharData *victim)
{
	if (victim && IN_ROOM(ch) != IN_ROOM(victim))
		return ch->GetGun();
	
	ObjData *weapon = ch->GetWeapon();
	if (!weapon || (/*IN_ROOM(ch) == IN_ROOM(victim) &&*/ OBJ_FLAGGED(weapon, ITEM_NOMELEE)))
	{
		weapon = ch->GetSecondaryWeapon();
		if (!weapon || (/*IN_ROOM(ch) == IN_ROOM(victim) &&*/ OBJ_FLAGGED(weapon, ITEM_NOMELEE)))
			weapon = NULL; //ch->GetWeapon();
	}
	return weapon;
}

#define GET_WAIT(ch) (IS_NPC(ch) ? GET_MOB_WAIT(ch) : ((ch)->desc ? (ch)->desc->wait : 0))


int initiate_combat(CharData *ch, CharData *victim, ObjData *weapon, bool force)
{
	int delay = 0;
	CombatEvent * eventFound;
	bool wasFlee = false;
	
	eventFound = (CombatEvent *)Event::FindEvent(ch->m_Events, CombatEvent::Type);
	
	if (eventFound && !force) {
//		mudlogf(NRM, 104, TRUE, "SYSERR: Fight error #1 (%s vs %s)\n", ch->GetName(), victim->GetName());
		return 0;
	}
	
	static Affect::Flags MAKE_BITSET(CounterattackAffects, AFF_DIDFLEE, AFF_NOSHOOT);

	//	If attacker as a Flee- or Shoot- timer, then the target can counterattack
	//	if they do NOT have a Flee- or Shoot- timer.
	bool bCounterAttack = ch->m_AffectFlags.testForAny(CounterattackAffects)
			&& victim->m_AffectFlags.testForNone(CounterattackAffects);
	
	if (bCounterAttack)
	{
		wasFlee = true;
		
//		delay = 1 RL_SEC;	//	CJ: TODO: should be remainder of 3 sec flee timer
//		Affect::FromChar(ch, "FLEE");
		
		if (!FIGHTING(victim) && GET_POS(victim) > POS_FIGHTING)
		{
			act("`rYou counter $n's attack!`n", FALSE, ch, NULL, victim, TO_VICT);
			act("`r$N counters your attack!`n", FALSE, ch, NULL, victim, TO_CHAR); 
			
			if (initiate_combat(victim, ch, NULL) == 0)
				return 0;
			
			if (PURGED(ch) || PURGED(victim) || IN_ROOM(ch) != IN_ROOM(victim))
				return 0;
		}
//		else
//		{
			//	Hit only if can't be countered
//			if (!weapon)	weapon = get_combat_weapon(ch, victim);
//			delay = combat_event(ch, victim, weapon, 0);
//		}
	}
//	else
	{
		if (!weapon)	weapon = get_combat_weapon(ch, victim);
		delay = combat_event(ch, victim, weapon, 0);
	}
	
	if (eventFound || PURGED(ch) || PURGED(victim) || delay == 0 || IN_ROOM(ch) != IN_ROOM(victim))
		return 0;
	
	if ((eventFound = (CombatEvent *)Event::FindEvent(ch->m_Events, CombatEvent::Type)))
	{
		mudlogf(NRM, 104, TRUE, "SYSERR: Fight error (%s vs %s): Initiate combat, latent combat event event in list (event=%s,flee=%s)", ch->GetName(), victim->GetName(),
				YESNO(eventFound != NULL), YESNO(wasFlee));
//		ch->events.Remove(eventFound);
//		eventFound->Cancel();
	}
	
	ch->AddOrReplaceEvent(new CombatEvent(delay, ch, weapon));
	
	return delay;
}


void combat_reset_timer(CharData *ch)
{
	CombatEvent *event;
	ObjData *weapon;
	int new_time;
	
	event = (CombatEvent *)Event::FindEvent(ch->m_Events, CombatEvent::Type);
	
	if (!event)
		return;
	
//	weapon = ch->GetWeapon();
//	if (!weapon || (/*IN_ROOM(ch) == IN_ROOM(victim) &&*/ OBJ_FLAGGED(weapon, ITEM_NOMELEE)))
//	{
//		weapon = ch->GetSecondaryWeapon();
//		if (!weapon || (/*IN_ROOM(ch) == IN_ROOM(victim) &&*/ OBJ_FLAGGED(weapon, ITEM_NOMELEE)))
//			weapon = ch->GetWeapon();
//	}
	weapon = get_combat_weapon(ch, NULL);
	
	if (weapon && weapon != event->GetWeapon())
	{
		new_time = combat_delay(ch, weapon) * 2;
		if (new_time > event->GetTimeRemaining())
			event->Reschedule(new_time);
	}
}




unsigned int CombatEvent::Run()
{
	CharData *	ch = m_Attacker;
	CharData *	victim = FIGHTING(ch);
	ObjData *	weapon;
	int	result = 0;
	
	ch->m_Events.remove(this);
	if (Event::FindEvent(ch->m_Events, CombatEvent::Type))
	{
		mudlogf(NRM, 104, TRUE, "SYSERR: Fight error (%s): Combat event dispatcher, latent combat event event in list", ch->GetName());
		return 0;
	}
	
	if (!victim)
		return 0;

	weapon = m_Weapon = get_combat_weapon(ch, victim);

#if ROAMING_COMBAT
/*	if (AFF_FLAGGED(ch, AFF_DIDSHOOT))
	{
		if (!FIGHTING(victim) && IN_ROOM(ch) == IN_ROOM(victim))
			initiate_combat(victim, ch, NULL);
	}
*/	
	if (GET_POS(ch) <= POS_RESTING)
		result = 1 RL_SEC;
	else if (AFF_FLAGGED(ch, AFF_DIDSHOOT))
		result = 1;
	else if (IN_ROOM(ch) != IN_ROOM(victim))
	{
		if (weapon && IS_GUN(weapon) && GET_GUN_RANGE(weapon))
		{
			if (!OBJ_FLAGGED(weapon, ITEM_AUTOSNIPE))
			{
				result = 0;
				m_PulsesSinceLastAttack = 0;
			}
			else if (GET_GUN_AMMO_TYPE(weapon) != -1 && GET_GUN_AMMO(weapon) == 0)
			{
				//if ((eventData->num % 5) == 0)
					ch->Send("Your weapon is out of ammo.\n");
				result = 2;
			}
			else
			{
				result = fire_missile(ch, victim, weapon, GET_GUN_RANGE(weapon), -1);
				
				if (result >= 0)
				{
					result = combat_delay(ch, weapon, true);
					m_PulsesSinceLastAttack = 0;
				}
				else if (result >= -2)
					result = 0;
				else
					result = 2;
			}
		}
		if (++m_PulsesSinceLastAttack >= 1 RL_SEC / 2)	//	
		{
			result = 0;
			ch->Send("You lost your target.\n");
		}
		
		if (result == 0)
			ch->stop_fighting();
	}
	else if (IsRanged() && FIGHTING(victim) != ch)	//	Always > 0 if it was a ranged combat event
		ch->stop_fighting();
	else
#endif
	{
		m_bIsRanged = false;
		m_PulsesSinceLastAttack = 0;
		result = combat_event(ch, victim, weapon, 1);
	}
	
	if (result != 0 && !PURGED(ch))
		ch->m_Events.push_front(this);
	
	return result;
}


int combat_event(CharData *ch, CharData *victim, ObjData *weapon, int event) {
	int result;
	int attackRate = 1;
	int notGun = TRUE;
	ACMD(do_reload);
	
	if (!ch || !victim || ch == victim)	/* bad event was posted... ignore it */
		return 0;
	
	if (event && Event::FindEvent(ch->m_Events, CombatEvent::Type)) {
		mudlogf(NRM, 104, TRUE, "SYSERR: Fight error (%s vs %s): Event-initiated round with outstanding dispatch event", ch->GetName(), victim->GetName());
		//core_dump();
		return 0;
	}
	
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && !NO_STAFF_HASSLE(ch))
	{
		ch->stop_fighting();
		return 0;
	}
	
	if (MOB_FLAGGED(victim, MOB_NOMELEE))
	{
		act("You can't possibly hurt $N.", FALSE, ch, NULL, victim, TO_CHAR);
		ch->stop_fighting();
		return 0;
	}

	
	if (!kill_valid(ch, victim)) {
		if (!PRF_FLAGGED(ch, PRF_PK)) {
			send_to_char("You must turn on PK first!  Type \"pkill\" for more information.\n", ch);
//			return;
		}
		else if (!PRF_FLAGGED(victim, PRF_PK)) {
			act("You cannot kill $N because $E has not chosen to participate in PK.", FALSE, ch, 0, victim, TO_CHAR);
//			return;
		}
		else
			send_to_char("You can't player-kill here.\n", ch);
		return 0;
	}
	
#if !ROAMING_COMBAT
	if (IN_ROOM(ch) != IN_ROOM(victim))
	{
		ch->redirect_fighting();
		victim = FIGHTING(ch);
	}
#endif
	
	if (FIGHTING(ch) != victim) {
		if (!FIGHTING(ch)) {
			set_fighting(ch, victim);
		} else if (IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
			ch->stop_fighting();
			set_fighting(ch, victim);
		} else {
//			ch->Send("ERROR: Tell an imm: fight error #3\n");
			return 0;
		}
	}

	if (!victim)
		return 0;
	
	if (IS_NPC(ch) && Number(1,3) == 1 &&	//	Change opponent...
			!AFF_FLAGGED(FIGHTING(ch), AFF_GROUPTANK))
	{
		CharData *temp;
		std::list<CharData *>	charList;
		
		FOREACH(CharacterList, ch->InRoom()->people, iter)
		{
			temp = *iter;
			if (FIGHTING(temp) == ch && temp != FIGHTING(ch))
				charList.push_back(temp);
		}
		
		charList.push_back(FIGHTING(ch));
		
		if (charList.size() > 1)
		{
			std::list<CharData *>::iterator iter = charList.begin();
			std::advance(iter, Number(0, charList.size() - 1));
			temp = *iter;
			
			if (FIGHTING(ch) != temp)
			{
				ch->stop_fighting();
				set_fighting(ch, temp);
				victim = temp;
				FIGHTING(ch) = temp;
			}
		}
	}
	
	if (GET_WAIT(ch) > 1) {
		return GET_WAIT(ch) + 1;	// Causer is already waiting.
	}
	
/*	if (AFF_FLAGGED(ch, AFF_DIDSHOOT | AFF_DIDFLEE))
	{
		if (!FIGHTING(victim))
		{
			act("You counter $n's attack!", FALSE, ch, NULL, victim, TO_VICT);
			act("$N counters your attack!", FALSE, ch, NULL, victim, TO_VICT); 
			initiate_combat(victim, ch, NULL);
		}
		return (1 RL_SEC) / 2;	//	Keep delaying 1/2 second
	}
*/
	if (AFF_FLAGGED(ch, AFF_DIDSHOOT))
	{
		if (!FIGHTING(victim))
			initiate_combat(victim, ch, NULL);
		if (AFF_FLAGGED(ch, AFF_DIDSHOOT))
			return (1 RL_SEC) / 2;	//	Keep delaying 1/2 second
	}

	if (GET_POS(ch) <= POS_SLEEPING) {
		return 1 RL_SEC;	// Causer is in no condition to do anything.
	}
	
	if (GET_POS(ch) < POS_FIGHTING) {
		if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_TRAPPED)) {
			GET_POS(ch) = POS_FIGHTING;
			if (ch->sitting_on)
				ch->sitting_on = NULL;
			act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);

			return (base_response[(int)ch->GetRace()] / 2) RL_SEC;	// Causer is jumping to feet.
		} else {
			send_to_char("You're on the ground!  Get up, fast!\n", ch);
		}
	}
	
	if (IS_NPC(ch)) {
		if (weapon && IS_GUN(weapon) && (GET_GUN_AMMO_TYPE(weapon) != -1) && (GET_GUN_AMMO(weapon) <= 0)) {
			do_reload(ch, "", "reload", SCMD_LOAD);
			if (GET_GUN_AMMO(weapon) > 0) {
				return (combat_delay(ch, weapon) / 2);	// Causer is reloading.
			}
		}
	}
	
	if (weapon && IS_GUN(weapon) && ((GET_GUN_AMMO_TYPE(weapon) == -1) || (GET_GUN_AMMO(weapon) > 0)) && !OBJ_FLAGGED(weapon, ITEM_NOMELEE))
		attackRate = GET_GUN_RATE(weapon);
		
	if (GET_POS(ch) < POS_FIGHTING)
		attackRate /= 2;

	result = hit(ch, victim, weapon, TYPE_UNDEFINED, MAX(1, attackRate));

	if (IS_NPC(ch) && !GET_MOB_WAIT(ch) && FIGHTING(ch)) {
		fight_mtrigger(ch);
		hitprcnt_mtrigger(ch);
	}
	
	return (result < 0 || PURGED(ch) || PURGED(victim)) ? 0 : combat_delay(ch, weapon);
}
