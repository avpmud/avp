//	TODO: STELLAR CONQUEST COPYRIGHT NOTICE

#ifndef __NETWORK_CONNECTION_H__
#define __NETWORK_CONNECTION_H__


#include "Socket.h"

#include "lexilist.h"


class Connection;

class ConnectionUpdateCallback
{
protected:
	virtual void		Update(Connection *c) = 0;
	
	friend class Connection;	
};


class Connection :
	private SocketClient
{
public:
						Connection(Socket *pSocket);

	virtual void		Start();
	virtual void		Update();	//	Update Loop

	virtual int			Send(const void *data, size_t size) = 0;
	virtual int			Send(const char *txt);
	
	virtual void		Close();
	void				SetClosing() { m_bIsClosing = true; }
	bool				IsClosing() { return m_bIsClosing; }


	//	SocketClient Interface
	virtual void		OnSocketUpdate();
	
	typedef Lexi::List<Connection *> List;
	static List &	GetConnections() { return ms_ConnectionList; }
	
	const Socket *		GetSocket();
	
protected:
	virtual				~Connection();
	
private:
	Socket * const		m_pSocket;
	bool				m_bIsClosing;
	
private:
						Connection(const Connection &);
	
	static List			ms_ConnectionList;
	
	friend class ConnectionManager;
};


class ConnectionManager
{
public:
						ConnectionManager();
						~ConnectionManager();
};

extern ConnectionManager *	TheConnectionManager;

#endif	//	__CONNECTION_H__
