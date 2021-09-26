
#include "types.h"

#include "TelnetConnection.h"
#include "protocols/Telnet.h"

#include "buffer.h"
#include "lexifile.h"

#include <stdio.h>


//	New Telnet Codes
#define TELOPT_MCCP2		86
#define TELOPT_MSP			90
#define TELOPT_MXP			91
#define TELOPT_ZMP			93


TelnetConnection::TelnetConnection(Socket *pSocket) :
	Connection(pSocket)
{
}


TelnetConnection::~TelnetConnection()
{
}


void TelnetConnection::Start()
{
	Connection::Start();

	//	A connection has been established
	TelnetProtocol *	pTelnet = GetTelnetProtocol();
	
	pTelnet->SendRequest(WONT, TELOPT_ECHO);		//	State that we won't echo
	pTelnet->SendRequest(WILL, TELOPT_EOR);		//	State that we will EOR
	pTelnet->SendRequest(DO, TELOPT_NEW_ENVIRON);//	Request Enviornment variables
	pTelnet->SendRequest(DO, TELOPT_TTYPE);		//	Request Terminal Type
	pTelnet->SendRequest(DO, TELOPT_NAWS);		//	Request to Negotiate About Window Size
	pTelnet->SendRequest(DO, TELOPT_LFLOW);		//	Request client do Remote Flow Control
	pTelnet->SendRequest(WILL, TELOPT_MCCP2);	//	State that we will support MCCP2
	pTelnet->SendRequest(WILL, TELOPT_MXP);		//	State that we will support MXP
}

/*
void TelnetConnection::Send(const void *data, size_t length)
{
	//	TODO: MCCP: Compress the outgoing data
	//	Then pass THAT to Connection::Send() instead
	Connection::Send(data, length);
}
*/

bool TelnetConnection::OnTelnetWill(char code)
{
#if defined(TELNET_DEBUGGING)
	if (TELOPT_OK(code))
		Log("NET: DBG: Received WILL %s", TELOPT(code));
	else
		Log("NET: DBG: Received WILL %d", code);
#endif
	
	switch (code)
	{
		case TELOPT_NAWS:
			//	Handled
			break;
		
		case TELOPT_TTYPE:
			{
				const char ttype[] = { TELOPT_TTYPE, TELQUAL_SEND };
				GetTelnetProtocol()->SendSubnegotiation(ttype, sizeof(ttype));
			}
			break;
		
		case TELOPT_NEW_ENVIRON:
			{
				const char newenviron[] =
				{
					TELOPT_NEW_ENVIRON, TELQUAL_SEND, 0,
					'S', 'Y', 'S', 'T', 'E', 'M', 'T', 'Y', 'P', 'E'
				};
				GetTelnetProtocol()->SendSubnegotiation(newenviron, sizeof(newenviron));
			}
			break;
		
		default:
			return false;
	}
	
	return true;
}


bool TelnetConnection::OnTelnetWont(char code)
{
#if defined(TELNET_DEBUGGING)
	if (TELOPT_OK(code))
		Log("NET: DBG: Received WONT %s", TELOPT(code));
	else
		Log("NET: DBG: Received WONT %d", code);
#endif
	
	switch (code)
	{
		case TELOPT_NAWS:
			//	WIDTH not supported
			Send("NAWS width not supported\r\n");
			break;
		
		default:
			return false;
	}
	
	return true;
}


bool TelnetConnection::OnTelnetDo(char code)
{
#if defined(TELNET_DEBUGGING)
	if (TELOPT_OK(code))
		Log("NET: DBG: Received DO %s", TELOPT(code));
	else
		Log("NET: DBG: Received DO %d", code);
#endif

	switch (code)
	{
		case TELOPT_ECHO:
			Send("Echoing On\r\n");
			SetTelnetOption(TELOPT_ECHO, OptionAccepted);
			break;

		case TELOPT_EOR:
			Send("EOR On\r\n");
			//	Only if we were!	GetTelnetProtocol()->SendRequest(WILL, TELOPT_EOR);
			SetTelnetOption(TELOPT_EOR, OptionAccepted);
			break;
		
		case TELOPT_MCCP2:
			Send("MCCP2 On\r\n");
			SetTelnetOption(TELOPT_MCCP2, OptionAccepted);
			break;
		
		case TELOPT_MXP:
			Send("MXP On\r\n");
			
			{
				char startMXP[] = { TELOPT_MXP };
				GetTelnetProtocol()->SendSubnegotiation( startMXP, sizeof(startMXP) );
				
				Send("\x1B[6z");	//	Force into secure mode
				Lexi::BufferedFile	file("data/MXP Definitions.txt");
				
				while (file.good())
				{
					Send(file.getline());
				}
			}

			SetTelnetOption(TELOPT_MXP, OptionAccepted);
			
			break;

		default:
			return false;
	}

	return true;
}


bool TelnetConnection::OnTelnetDont(char code)
{
#if defined(TELNET_DEBUGGING)
	if (TELOPT_OK(code))
		Log("NET: DBG: Received DONT %s", TELOPT(code));
	else
		Log("NET: DBG: Received DONT %d", code);
#endif

	switch (code)
	{
		case TELOPT_ECHO:
			Send("Echoing off\r\n");
//			if (GetTelnetOption(TELOPT_ECHO) == OptionOffered)
				GetTelnetProtocol()->SendRequest(WONT, TELOPT_ECHO);
			SetTelnetOption(TELOPT_ECHO, OptionRefused);
			break;

		case TELOPT_EOR:
			Send("EOR off\r\n");
			//	Only if we were!
//			if (GetTelnetOption(TELOPT_EOR) == OptionOffered)
				GetTelnetProtocol()->SendRequest(WONT, TELOPT_EOR);
			SetTelnetOption(TELOPT_EOR, OptionRefused);
			break;
		
		case TELOPT_MCCP2:
			Send("MCCP2 Off\r\n");
			SetTelnetOption(TELOPT_MCCP2, OptionRefused);
			break;
		
		case TELOPT_MXP:
			Send("MXP Off\r\n");
			SetTelnetOption(TELOPT_MXP, OptionRefused);
			break;

		default:
			return false;
	}

	return true;
}


void TelnetConnection::OnTelnetSubnegotiate(const char *data, size_t size)
{
#if defined(TELNET_DEBUGGING)
	if (TELOPT_OK(data[0]))
		Log("NET: DBG: Received SUB %s", TELOPT(data[0]));
	else
		Log("NET: DBG: Received SUB %d", data[0]);
#endif

	if (size == 0)	//	Error condition
		return;
	
	switch (data[0])
	{
		case TELOPT_NAWS:
			{
/*				BUFFER(buf, MAX_STRING_LENGTH);
				sprintf(buf, "Width: %d\r\nHeight: %d\r\n",
					ntohs(*(unsigned short *)(data + 1)),
					ntohs(*(unsigned short *)(data + 3)));
				Send(buf);
*/			}
			break;
		
		case TELOPT_TTYPE:
			if (size > 2 && data[1] == TELQUAL_IS)
			{
				bool	xterm = false, ansi = false;
				
				m_Terminal.assign(data + 2, size - 2);
				
				BUFFER(buf, MAX_STRING_LENGTH);
				sprintf(buf, "Terminal is: %s\r\n", m_Terminal.c_str());
				Send(buf);

				if (m_Terminal == "XTERM")		Send("XTerm detected\r\n");
				else if (m_Terminal == "ANSI")	Send("ANSI detected\r\n");
				else if (m_Terminal == "zmud")	Send("ZMud detected\r\n");
			}
			break;
		
		case TELOPT_NEW_ENVIRON:
			if (size > 3 && data[1] == TELQUAL_IS && data[2] == 0)
			{
				if (size >= 13 && !memcmp(data + 3, "SYSTEMTYPE", 10))
				{
					//	Is it Windows?
					if (size >= 19 && data[13] == 1 && !memcmp(data + 14, "WIN32", 5))
					{
						Send("Windows Telnet detected\r\n");
					}
				}
			}
			break;
	}
}


void TelnetConnection::TelnetSendData(const char *data, size_t size)
{
	Send(data, size);
}


void TelnetConnection::SetTelnetOption(char code, TelnetOptionStatus status)
{
	int byteNumber = code >> 2;
	int optionShift = (code & 0x3) << 1;
	
	unsigned char optionByte = m_TelnetOptions[byteNumber];
	
	//	Mask out old options
	optionByte &= ~(OptionBitsMask << optionShift);
	optionByte |= (code << optionShift);
	
	m_TelnetOptions[byteNumber] = optionByte;
}
