#ifndef __NETWORK_PROTOCOLS_TELNETPROTOCOL_H__
#define __NETWORK_PROTOCOLS_TELNETPROTOCOL_H__

#include "../../types.h"

#include "Protocol.h"

#include "../../lexistring.h"


class TelnetProtocol;
class TelnetProtocolClient;


class TelnetProtocol : public Protocol
{
public:
						TelnetProtocol(TelnetProtocolClient *client);
	
	//	ConnectionProtocol
	size_t				ProcessBuffer(char *in, char *out, size_t size, size_t maxsize);
	void				SendRequest(unsigned char request, char code);
	void				SendSubnegotiation(const char *data, size_t size);

private:
	enum TelnetProtocolInputState
	{
		eStateData,
		eStateIAC,
		eStateWill,
		eStateWont,
		eStateDo,
		eStateDont,
		eStateSubnegotiation,
		eStateSubnegotiationCheckFinished
	};
	
	TelnetProtocolClient *		m_pClient;
	TelnetProtocolInputState	m_State;
	Lexi::String				m_SubnegotiationBuffer;
	
	//	State Maintained Here
};


class TelnetProtocolClient : public ProtocolClient
{
private:
	virtual bool		OnTelnetWill(char code) = 0;
	virtual bool		OnTelnetWont(char code) = 0;
	virtual bool		OnTelnetDo(char code) = 0;
	virtual bool		OnTelnetDont(char code) = 0;
	virtual void		OnTelnetSubnegotiate(const char *data, size_t size) = 0;
	
	virtual ssize_t		TelnetSendData(const char *data, size_t size) = 0;
	
	friend class TelnetProtocol;
};


#endif // __NETWORK_PROTOCOL_TELNETPROTOCOL_H__
