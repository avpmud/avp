#ifndef __NETWORK_MUDCONNECTION_H__
#define __NETWORK_MUDCONNECTION_H__


#include "TelnetConnection.h"
#include "protocols/TelnetProtocol.h"

#include "lexistring.h"


class MUDConnection;

class MUDConnection :
	public TelnetConnection
{
public:
						MUDConnection(Socket *pSocket);
						
	//	Connection Interface
	virtual void		Start();
	virtual void		Update();	//	Update Loop

	virtual int			Send(const void *data, size_t size);
	using Connection::Send;
	virtual int			SendImmediate(const void *data, size_t size);
	virtual int			SendImmediate(const char *str);
	
	virtual void		Close();
	
	//	SocketClient Interface
	virtual bool		IsSocketInputEnabled() { return true; }
	virtual bool		IsSocketOutputEnabled() { return true; }
	virtual bool		IsSocketExceptionEnabled() { return true; }		//	If true, there is data to send
	virtual SocketClient::EventResult	OnSocketInput();
	virtual SocketClient::EventResult	OnSocketOutput();
	virtual SocketClient::EventResult	OnSocketException();

private:
	//	Input Buffer
	static const size_t	MAX_RAW_INPUT_LENGTH = 1024;
	char				m_Input[MAX_RAW_INPUT_LENGTH];	//	buffer for raw input
	size_t				m_InputSize;

	//	Output Buffer
	
//	typedef std::list<Protocol *>	ProtocolList;
//	ProtocolList		m_InputProtocols;
//	ProtocolList		m_OutputProtocols;
	
//	size_t				ProcessInputProtocols(char *buf, size_t sz);
//	size_t				ProcessOutputProtocols(char *buf, size_t sz);
};


#endif	//	__CONNECTION_H__
