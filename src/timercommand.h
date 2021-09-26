#ifndef __TIMERCOMMAND_H__
#define __TIMERCOMMAND_H__


class TimerCommand
{
public:
						TimerCommand(int time, const char *const type);
	virtual				~TimerCommand();
	
	int					Execute();
	bool				ExecuteAbort(CharData *ch, const char *str);
	virtual int			Run() = 0;					//	Return 0 if ran, time if should run again
	virtual bool		Abort(CharData *ch, const char *str) = 0;	//	Return false if failed to abort
	virtual void		Release(CharData *ch);
	
	bool				IsRunning() const { return m_bRunning; }
	const char * 		GetType() const { return m_TimerType; }
	
	int					m_pRefCount;
	Event *				m_pEvent;
	bool				m_bRunning;
	const char *		m_TimerType;
};


#endif
