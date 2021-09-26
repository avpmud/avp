#include "ListenerConnection.h"
#include "MUDConnection.h"

ListenerConnection::ListenerConnection(Socket *pSocket) :
	m_pSocket(pSocket)
{
	pSocket->SetClient(this);
}

ListenerConnection::~ListenerConnection()
{
	delete m_pSocket;
}


void ListenerConnection::Close()
{
	delete this;
}


//	SocketClient Interface
SocketClient::EventResult ListenerConnection::OnSocketInput()
{
	Socket *pNewSocket = m_pSocket->Accept();
	
	if (pNewSocket)
	{
		Connection *	connection = CreateConnection(pNewSocket);
		connection->Start();
	}
	
	return SocketClient::eOK;
}
