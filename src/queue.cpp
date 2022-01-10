#include "conf.h"
#include "sysdep.h"
#include "queue.h"

//	External variables
extern UInt32 pulse;

QueueElement::QueueElement(Ptr data, int key) {
	this->data = data;
	this->key = key;
	this->next = NULL;
	this->prev = NULL;
}


Queue::Queue(void) {
	memset(head, 0, sizeof(head));
	memset(tail, 0, sizeof(tail));
}


Queue::~Queue(void) {
	int i;
	QueueElement *qe, *next_qe;
	
	for (i = 0; i < NUM_EVENT_QUEUES; i++) {
		for (qe = this->head[i]; qe; qe = next_qe) {
			next_qe = qe->next;
			delete qe;
		}
	}
}


QueueElement *Queue::Enqueue(Ptr data, int key) {
	QueueElement *qe, *i;
	int	bucket;
	
	qe = new QueueElement(data, key);
	bucket = key % NUM_EVENT_QUEUES;

	assert(bucket >= 0);

	if (!this->head[bucket]) {
		this->head[bucket] = qe;
		this->tail[bucket] = qe;
	} else {
		for (i = this->tail[bucket]; i; i = i->prev) {
			if (i->key < key) {
				if (i == this->tail[bucket])
					this->tail[bucket] = qe;
				else {
					qe->next = i->next;
					i->next->prev = qe;
				}
				qe->prev = i;
				i->next = qe;
				break;
			}
		}
		if (i == NULL) {
			qe->next = this->head[bucket];
			this->head[bucket] = qe;
			qe->next->prev = qe;
		}
	}
	return qe;
}


void Queue::Dequeue(QueueElement *qe) {
	int i;
	
	if (!qe)	return;
	
	i = qe->key % NUM_EVENT_QUEUES;
	
	if (qe->prev)	qe->prev->next = qe->next;
	else			this->head[i] = qe->next;
	
	if (qe->next)	qe->next->prev = qe->prev;
	else			this->tail[i] = qe->prev;
	
	delete qe;
}


Ptr Queue::QueueHead(void) {
	Ptr data;
	int i;
	
	i = pulse % NUM_EVENT_QUEUES;
	
	if (!this->head[i])
		return NULL;
	
	data = this->head[i]->data;
	this->Dequeue(this->head[i]);
	
	return data;
}


int Queue::QueueKey(void) {
	int i;
	
	i = pulse % NUM_EVENT_QUEUES;
	
	if (this->head[i])	return this->head[i]->key;
	else				return LONG_MAX;
}


int Queue::Count(void)
{
	int count = 0;
	for (int i = 0; i < NUM_EVENT_QUEUES; ++i)
	{
		QueueElement *q = head[i];
		
		while (q)
		{
			++count;
			q = q->next;
		}
	}
	
	return count;
}
