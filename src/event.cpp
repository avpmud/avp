/*************************************************************************
*   File: event.c++                  Part of Aliens vs Predator: The MUD *
*  Usage: Primary code for events                                        *
*************************************************************************/

#include "types.h"
#include "event.h"
#include "queue.h"
#include "utils.h"


// New Event System

extern UInt32 pulse;
Queue EventQueue;


void Event::ProcessEvents(void) {
	Event *				pEvent;
	unsigned int		newTime;
	
	while (pulse >= EventQueue.QueueKey()) {
		if (!(pEvent = (Event *)EventQueue.QueueHead())) {
			log("SYSERR: Attempt to get a NULL event!");
			return;
		}
		if ((newTime = pEvent->Execute()) != 0)
			pEvent->m_pQueue = EventQueue.Enqueue(pEvent, newTime + pulse);
		else
			delete pEvent;
	}
}


Event::Event(unsigned int when) :
	m_pQueue(NULL),
	m_bRunning(false)
{
	if (when < 1)
		when = 1;
	
	m_pQueue = EventQueue.Enqueue(this, when + pulse);
}


unsigned int Event::GetTimeRemaining() const
{
	return (m_pQueue->key - pulse);
}


unsigned int Event::Execute()
{
	unsigned int result;
	
	m_bRunning = true;
	result = Run();
	m_bRunning = false;
	
	return result;
}


void Event::Cancel(void) {
	if (m_bRunning)
		return;
	
	EventQueue.Dequeue(m_pQueue);
	delete this;
}


void Event::Reschedule(unsigned int when)
{
	EventQueue.Dequeue(m_pQueue);
	m_pQueue = EventQueue.Enqueue(this, when + pulse);
}


Event *Event::FindEvent(List & list, const char *type) {
	FOREACH(List, list, iter)
	{
		Event *event = *iter;
		if (event->GetType() == type || !str_cmp(event->GetType(), type))
			return event;
	}
	return NULL;
}
