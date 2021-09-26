
#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "types.h"

#define NUM_EVENT_QUEUES	10

class QueueElement {
public:
	QueueElement(Ptr data, int key);
	
	Ptr		data;
	int	key;
	QueueElement *prev, *next;
};

class Queue {
public:
	Queue(void);
	~Queue(void);
	QueueElement *	Enqueue(Ptr data, int key);
	void			Dequeue(QueueElement *qe);
	Ptr				QueueHead(void);
	int				QueueKey(void);
	int				Count();
	
	QueueElement *head[NUM_EVENT_QUEUES], *tail[NUM_EVENT_QUEUES];
};

#endif

