#include "state.h"

tHash HashStr(const char *str)
{
	tHash hash = 0xffffffff;
	
	if (!str)	return 0;
	
	while (*str)
	{
		hash = (hash << 5) + hash;
		hash = hash + toupper(*(str++));
	}
	
	return hash;
}


StateMachine::StateMachine() :
	State(NULL, "\x1bStateMachine")
{ }


void StateMachine::HandleEvent(tEvent event)
{
	State *			pCurrentState;
	bool			bResult;
	
	pCurrentState = m_pCurrentState;
	do
	{
		bResult = pCurrentState->InternalHandleEvent(event);
	} while (!bResult && (pCurrentState = pCurrentState->GetParent()));
}


void StateMachine::SetState(tState nState)
{
	State * 		pNewState;
	State *			pCurrentState;

	pCurrentState = m_pCurrentState;
	do
	{
		pNewState = m_pCurrentState->GetState(nState);
	} while (!pNewState && (pCurrentState = pCurrentState->GetParent()));
	
	if (pNewState)
	{		
		m_pCurrentState = pCurrentState->AsState();
	}
}


State *State::GetState(tState nState)
{
	tStateMap::iterator	iter = m_States.find(nState);
	
	return iter != m_States.end() ? iter->second : NULL;
}


void State::RegisterState(State *pState)
{
	m_States[pState->GetNameHash()] = pState;
}


State::State(State *pParent, const char *pName) :
	m_pParent(pParent),
	m_pName(pName),
	m_nNameHash(HashStr(pName))
{
	pParent->RegisterState(this);
}




const tEvent cEnemySpottedEvent = HashStr("Enemy Spotted Event");
const tEvent cAttackedEvent = HashStr("Attacked Event");
const tEvent cIdleEvent = HashStr("Idle Event");


//	Generic State for Moving To Point
class WalkingToDestinationState : public State
{
	REGISTER_STATE(WalkingToDestinationState)
	REGISTER_FINISH
};

	
class TestMachine : public StateMachine
{
	REGISTER_STATEMACHINE(TestMachine)
		REGISTER_SUBSTATE(m_IdleState)
		REGISTER_SUBSTATE(m_WalkingToDestinationState)
		REGISTER_SUBSTATE(m_CombatState)
	REGISTER_FINISH
	
	class IdleState : public State
	{
		REGISTER_STATE(IdleState)
			REGISTER_SUBSTATE(m_WanderingState)
		REGISTER_FINISH		
		
		STATE_BEGIN_EVENTS
			STATE_HANDLE_EVENT(cIdleEvent, HandleIdle)
		STATE_END_EVENTS
		
		class WanderingState : public State
		{
			REGISTER_STATE(WanderingState)
			REGISTER_FINISH
		};
		
		DEFINE_EVENT_HANDLER(HandleIdle);
		
		WanderingState m_WanderingState;
	};
	
	
	class CombatState : public State
	{
		REGISTER_STATE(CombatState)
			REGISTER_SUBSTATE(m_WalkingToDestinationState)
		REGISTER_FINISH
		
		STATE_BEGIN_EVENTS
			STATE_HANDLE_EVENT(cEnemySpottedEvent, HandleEnemySpotted)
			STATE_HANDLE_EVENT(cAttackedEvent, HandleAttacked)
			STATE_HANDLE_EVENT(cIdleEvent, HandleIdle)
		STATE_END_EVENTS
		
		DEFINE_EVENT_HANDLER(HandleEnemySpotted) { }
		DEFINE_EVENT_HANDLER(HandleAttacked) { }
		DEFINE_EVENT_HANDLER(HandleIdle) { }
		
		WalkingToDestinationState m_WalkingToDestinationState;
	};
	
	STATE_BEGIN_EVENTS
		STATE_HANDLE_EVENT(cEnemySpottedEvent, HandleEnemySpotted)
		STATE_HANDLE_EVENT(cAttackedEvent, HandleAttacked)
	STATE_END_EVENTS
	
	DEFINE_EVENT_HANDLER(HandleEnemySpotted) { }
	DEFINE_EVENT_HANDLER(HandleAttacked) { }
	
	IdleState m_IdleState;
	WalkingToDestinationState m_WalkingToDestinationState;
	CombatState m_CombatState;
};
