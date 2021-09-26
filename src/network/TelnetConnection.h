//	TODO: STELLAR CONQUEST COPYRIGHT NOTICE

#ifndef __NETWORK_TELNETCONNECTION_H__
#define __NETWORK_TELNETCONNECTION_H__


#include "Socket.h"

#include "Connection.h"
#include "protocols/TelnetProtocol.h"

#include "lexistring.h"



class TelnetConnection :
	public Connection, private TelnetProtocolClient
{
public:
						TelnetConnection(Socket *pSocket);
						
	virtual void		Start();
	virtual void		Update();	//	Update Loop
	
	const char *		GetTerminal() const { return m_Terminal.c_str(); }

	enum TelnetOptionStatus
	{
		OptionInvalid = 0x0,
		OptionOffered = 0x1,
		OptionRefused = 0x2,
		OptionAccepted = 0x3,
	};
	
	enum
	{
		OptionBitsRequired = 2,
		OptionBitsMask = (1 << OptionBitsRequired) - 1,
		OptionCount = 128,
		OptionArrayBits = OptionCount * OptionBitsRequired,
		OptionArrayBytes = OptionArrayBits / 8
	};
	
	inline TelnetOptionStatus	GetTelnetOption(char code) 
	{
		return (TelnetOptionStatus)((m_TelnetOptions[code >> 2] >> ((code & 0x3) << 1)) & OptionBitsMask);
	}
	
protected:
	virtual				~TelnetConnection();

private:
	//	TelnetProtocolClient Interface
	virtual bool		OnTelnetWill(char code);
	virtual bool		OnTelnetWont(char code);
	virtual bool		OnTelnetDo(char code);
	virtual bool		OnTelnetDont(char code);
	virtual void		OnTelnetSubnegotiate(const char *data, size_t size);
	
	virtual ssize_t		TelnetSendData(const char *data, size_t size);
	
	void				SetTelnetOption(char code, TelnetOptionStatus status);

private:
	//	Telnet
	Lexi::String		m_Terminal;
	unsigned char		m_TelnetOptions[OptionArrayBytes];
		
private:
						TelnetConnection(const Connection &);
};


#endif
