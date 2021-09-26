#ifndef __CONNECTIONSTATES_H__
#define __CONNECTIONSTATES_H__

class ConnectionState;
class ConnectionStateMachine;

class ConnectionState : public Lexi::Shareable
{
public:
	ConnectionState();
	
	virtual void		Idle() {}					//	Idle Update
	virtual void		Enter() {}					//	Entering State
	virtual void		Exit() {}					//	Exiting State
	virtual void		Pause() {}					//	Entering State
	virtual void		Resume() {}					//	Entering State
	virtual void		Parse(const char *arg) = 0;	//	Handle user input
	virtual Lexi::String GetName() const = 0;			//	Set State Name
	
	virtual bool		IsPlaying() const { return false; }
	virtual bool		IsInGame() const { return false; }
	
	virtual void		OnEditorSave() {}
	virtual void		OnEditorAbort() {}
	virtual void		OnEditorFinished() {}
	
	typedef Lexi::SharedPtr<ConnectionState>	Ptr;
	
	void				Push(Ptr state);
	void				Pop();
	void				Swap(Ptr state);

protected:
	friend class ConnectionStateMachine;
	
	void				SetDesc(DescriptorData *d) { m_pDesc = d; }
	DescriptorData *	GetDesc() { return m_pDesc; }
	ConnectionStateMachine &GetStateMachine();
//	const ConnectionState *	GetParent() { return m_Parent; }
	
	virtual ~ConnectionState();
	
private:
	ConnectionState(const ConnectionState &);
	
	DescriptorData *	m_pDesc;
//	Ptr					m_Parent;
};


class ConnectionStateMachine : private std::list<ConnectionState *>
{
public:
	ConnectionStateMachine(DescriptorData *d)
	:	m_pDesc(d)
	{}
	
	ConnectionState::Ptr	GetState();
	
	void				Idle();
	void				Parse(const char *arg);
	
	void				Push(ConnectionState::Ptr state);	//	Push a state onto the stack
	void				Pop();								//	Pop current state
	void				Swap(ConnectionState::Ptr state);

protected:
	DescriptorData *	GetDesc() { return m_pDesc; }
	
private:
	ConnectionStateMachine(const ConnectionStateMachine &);
	
	DescriptorData *	m_pDesc;
	Lexi::List<ConnectionState::Ptr>	m_States;
};




//
//	CORE CONNECTION STATES
//


class PlayingConnectionState : public ConnectionState
{
public:
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Playing"; }
	virtual bool		IsPlaying() const { return true; }
	virtual bool		IsInGame() const { return true; }

	virtual void		OnEditorAbort();
};


class DisconnectConnectionState : public ConnectionState
{
public:
	virtual void		Idle();
	virtual void		Parse(const char *arg) { /* Nothing, we're disconnecting */ }
	
	virtual Lexi::String GetName() const { return "Disconnecting"; }
};


class IdentConnectionState : public ConnectionState
{
public:
	virtual void		Enter();
	virtual void		Exit();
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Lookup Host"; }
};


class OldAddressConnectionState : public ConnectionState
{
public:
	virtual void		Enter();
	virtual void		Exit();
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Old Address"; }
};


class IdleTimeoutConnectionState : public ConnectionState
{
public:
	IdleTimeoutConnectionState()
	:	m_StartTime(time(0))
	{}
	
	virtual void		Idle();

protected:
	void				ResetTimeout() { m_StartTime = time(0); }

private:
	static const int	ms_IdleTimeout = 2 * 60;
	time_t				m_StartTime;
};


class GetNameConnectionState : public IdleTimeoutConnectionState
{
public:
	virtual void		Enter();
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Get Name"; }
};


//
//	CHARACTER CREATION CONNECTION STATES
//	Start of creation


class ConfirmNameConnectionState : public IdleTimeoutConnectionState
{
public:
	virtual void		Enter();
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Confirm Name"; }
};


//
//	MENU CONNECTION STATES
//


class MenuConnectionState : public ConnectionState
{
public:
//	virtual void		Enter();
	virtual void		Resume();
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Main Menu"; }
};


//
//	OLC CONNECTION STATES
//


class OLCConnectionState : public ConnectionState
{
public:
	virtual void		Enter();
	virtual void		Exit();
	
	virtual bool		IsInGame() const { return true; }
};


class ObjectEditOLCConnectionState : public OLCConnectionState
{
public:
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Object edit"; }

	virtual void		OnEditorFinished();	
};


class RoomEditOLCConnectionState : public OLCConnectionState
{
public:
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Room edit"; }

	virtual void		OnEditorFinished();	
};


class ZoneEditOLCConnectionState : public OLCConnectionState
{
public:
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Zone edit"; }
};


class MobEditOLCConnectionState : public OLCConnectionState
{
public:
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Mobile edit"; }

	virtual void		OnEditorFinished();	
};


class ScriptEditOLCConnectionState : public OLCConnectionState
{
public:
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Script edit"; }

	virtual void		OnEditorFinished();	
};



class ShopEditOLCConnectionState : public OLCConnectionState
{
public:
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Shop edit"; }
};


class ActionEditOLCConnectionState : public OLCConnectionState
{
public:
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Action edit"; }
};


class HelpEditOLCConnectionState : public OLCConnectionState
{
public:
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Help edit"; }
	
	virtual void		OnEditorFinished();
};


class ClanEditOLCConnectionState : public OLCConnectionState
{
public:
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Clan edit"; }
};


class BehaviorEditOLCConnectionState : public OLCConnectionState
{
public:
	virtual void		Parse(const char *arg);
	
	virtual Lexi::String GetName() const { return "Behavior edit"; }
};


#endif
