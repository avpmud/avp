
#include "types.h"
#include <map>

class State;
class StateMachine;

typedef unsigned int tHash;
typedef tHash tEvent;
typedef tHash tState;

typedef std::map<tState, State *> tStateMap;


extern tHash HashStr(const char *str);


#if 0

class StateMachine
{
protected:
							StateMachine();
	virtual					~StateMachine();
	
public:
	void					HandleEvent(tEvent nEvent);
	virtual void			SetState(tState nState);
	void					SetState(const char *pState);
	void					RegisterState(State *pState);
	
protected:
	//	Accessors
	inline StateMachine *	GetParent() { return m_pParent; }
	State *					GetState(tState nState);
	virtual State *			AsState() { return NULL; }
	
	virtual bool			InternalHandleEvent(tEvent nEvent) { return false; }
	
							StateMachine(StateMachine *pParent);
	
private:
	tStateMap				m_States;
	State *					m_pCurrentState;
	StateMachine *			m_pParent;
};


class State : public StateMachine
{
protected:
							State(StateMachine *pParent);
	
public:
	virtual void			SetState(tState nState) { GetParent()->SetState(nState); }
	
	virtual tState			GetNameHash() = 0;
	virtual const char *	GetName() = 0;
	
	
private:
							State();
	virtual State *			AsState() { return this; }
};
#endif




class State
{
protected:
							State(State *pParent, const char *pName);
	virtual					~State();
	
public:
	//	Pure Virtual Accessors
	tState					GetNameHash();
	const char *			GetName();

	//	Accessors
	State *					GetState(tState nState);			//	Find State
	inline State *			GetParent() { return m_pParent; }	//	Retrieve parent
	virtual State *			AsState() { return this; }			//	Are we a state for sure?
	virtual State *			GetCurrentState() { return GetParent()->GetCurrentState(); }
	
	//	Methods
	virtual void			SetState(tState nState) { GetParent()->SetState(nState); }
	inline void				SetState(const char *pState) { SetState(HashStr(pState)); }
	virtual bool			InternalHandleEvent(tEvent nEvent) { return false; }

private:	//	Internal methods
	void					RegisterState(State *pState);		//	Children register with parent
	
private:	//	Members
	tStateMap				m_States;			//	Map of States
	State *					m_pParent;			//	Parent
	const char * const		m_pName;			//	Name
	const tState			m_nNameHash;		//	Hash of Name

private:	//	Override Prevention
							State();
							State(const State &);
};



class StateMachine : public State
{
protected:
							StateMachine();
	
public:
	void					HandleEvent(tEvent nEvent);
	virtual void			SetState(tState nState);
	
	//	Accessors
	virtual State *			AsState() { return NULL; }
	virtual State *			GetCurrentState() { return m_pCurrentState; }
	
private:	//	Members
	State *					m_pCurrentState;
	
private:	//	Override Prevention
							StateMachine(State *pParent);
};



	//	Formerly inserted after the public: below
//	virtual const char * GetName() { return #StateName; } \
//	virtual tState GetNameHash() { \
//		static const tState cnNameHash = HashStr(#StateName); \
//		return cnNameHash; \
//	} \

#define REGISTER_STATE(StateName) \
	public: \
	StateName(State *pParent) : \
		State(pParent, #StateName)
#define REGISTER_SUBSTATE(SubstateName) \
		, SubstateName(this)
#define REGISTER_FINISH \
	{ }


#define REGISTER_STATEMACHINE(MachineName) \
	public: \
	MachineName() : \
		StateMachine()


#define DEFINE_EVENT_HANDLER(handler) \
	void handler(void)


#if 0
#define STATE_BEGIN_EVENTS			\
	virtual bool InternalHandleEvent(tEvent event) { \
		switch (event) {
#define STATE_HANDLE_EVENT(handledEvent, handler) \
..			case handledEvent: handler(); break;
#define STATE_HANDLE_EVENT_UNREGISTERED(handledEvent) \
			case handledEvent: break;
#define STATE_END_EVENTS				\
			default: return false;		\
		}								\
		return true;					\
	}
#else
#define STATE_BEGIN_EVENTS			\
	virtual bool InternalHandleEvent(tEvent event) {
#define STATE_HANDLE_EVENT(handledEvent, handler) \
		if (event == handledEvent) { handler(); } else
#define STATE_HANDLE_EVENT_UNREGISTERED(handledEvent) \
		if (event == handledEvent) { } else
#define STATE_END_EVENTS				\
		{ return false; }				\
		return true;					\
	}
#endif
