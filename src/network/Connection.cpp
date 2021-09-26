
#include "types.h"

#include "Connection.h"

#include <stdio.h>


Connection::List Connection::ms_ConnectionList;


Connection::Connection(Socket *pSocket) :
	m_pSocket(pSocket),
	m_bIsClosing(false)
{
	pSocket->SetClient(this);
	ms_ConnectionList.push_back(this);
}


Connection::~Connection()
{
	delete m_pSocket;
}


void Connection::Start()
{
}


void Connection::Update()
{
}


int Connection::Send(const char *txt)
{
	return Send(txt, strlen(txt));
}

	
//int Connection::Send(const Lexi::String &str)
//{
//	Send(str.c_str(), str.size());
//}


void Connection::Close()
{
	ms_ConnectionList.remove(this);
	
	delete this;
}


void Connection::OnSocketUpdate()
{
	Update();
	
	if (IsClosing())
	{
		Close();
		return;
	}
}
