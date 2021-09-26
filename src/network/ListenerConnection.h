#ifndef __LISTENERCONNECTION_H__
#define __LISTENERCONNECTION_H__


#include "Socket.h"

class ListenerConnection;
class Connection;

class ListenerConnection : private SocketClient
{
public:
						ListenerConnection(Socket *pSocket);
	
	void				Close();


	//	SocketClient Interface
	virtual bool		IsSocketInputEnabled() { return true; }
	virtual SocketClient::EventResult	OnSocketInput();

protected:
	virtual Connection *CreateConnection(Socket *pSocket) = 0;
	
private:
	Socket *			m_pSocket;
	
private:
						ListenerConnection(const ListenerConnection &);
						~ListenerConnection();
};


template <class ConnectionClass>
class MyListenerConnection : private ListenerConnection
{
public:
						MyListenerConnection(Socket *pSocket)
						:	MyListenerConnection(pSocket)
						{}

protected:
	Connection *		CreateConnection(Socket *pSocket)
	{
		return new ConnectionClass(pSocket);
	}
};


#endif // __LISTENERCONNECTION_H__
