#ifndef __MUDLISTENERCONNECTION_H__
#define __MUDLISTENERCONNECTION_H__


#include "ListenerConnection.h"


class MUDListenerConnection : private ListenerConnection
{
public:
						MUDListenerConnection(Socket *pSocket)
						:	ListenerConnection(pSocket)
						{}
	
protected:
	//	ListenerConnection Interface
	Connection *		CreateConnection(Socket *pSocket);
};


#endif // __LISTENERCONNECTION_H__
