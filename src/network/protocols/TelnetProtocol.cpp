//#define TELNET_DEBUGGING

#define TELCMDS
#define TELOPTS

#include "TelnetProtocol.h"
#include "Telnet.h"
#include "../../buffer.h"
//#include "Logging.h"
#include "../../utils.h"

#include <string.h>
#include <assert.h>


TelnetProtocol::TelnetProtocol(TelnetProtocolClient *client)
:	m_pClient(client)
,	m_State(eStateData)
{
	//	Nothing more
	m_SubnegotiationBuffer.reserve(64);
}


//	We only handle basic telnet commands here
//	Backspace and line parsing is not related to Telnet Protocol
size_t TelnetProtocol::ProcessBuffer(char *in, char *out, size_t size, size_t maxsize)
{
	if (size == 0)
		return 0;	//	Nothing to process
	
	register char *src = in;
	register char *dest = out;
	unsigned char c;
	
	//	Copy src to dest while scanning for Telnet IAC codes
	if (m_State == eStateData)
	{
		while (size > 0)
		{
			c = *src++;
			--size;
			if (c == IAC)
			{
				m_State = eStateIAC;
				break;
			}
			*dest++ = c;
		}
		
		if (size == 0)
			return dest - out;
	}
	
	while (size > 0)
	{
		switch (m_State)
		{
			default:
			case eStateData:
			
				//	Copy data
				while (size > 0)
				{
					c = *src++;
					--size;
					
					if (c == IAC)
					{
						//	We've reached an IAC!
						m_State = eStateIAC;
						break;
					}
					*dest++ = c;
				}
				
				break;
			
			case eStateIAC:
				//	We have received and eaten an IAC
				{
					c = *src++;
					--size;
					
					switch (c)
					{
						case IAC:
							//	It's a double IAC
							*dest++ = (char)IAC;
							m_State = eStateData;
							break;
							
						case WILL:		m_State = eStateWill;				break;
						case WONT:		m_State = eStateWont;				break;
						case DO:		m_State = eStateDo;					break;
						case DONT:		m_State = eStateDont;				break;
						case SB:		m_State = eStateSubnegotiation;		break;
					}
				}
				break;
			
			case eStateWill:
#if defined(TELNET_DEBUGGING)
				if (TELOPT_OK(*src))	log("NET: DBG: Received Telnet WILL %s", TELOPT(*src));
				else					log("NET: DBG: Received Telnet WILL %d", *src);
#endif
				m_pClient->OnTelnetWill(*src);
				++src; --size;
				m_State = eStateData;
				break;
				
			case eStateWont:
#if defined(TELNET_DEBUGGING)
				if (TELOPT_OK(*src))	log("NET: DBG: Received Telnet WONT %s", TELOPT(*src));
				else					log("NET: DBG: Received Telnet WONT %d", *src);
#endif
				m_pClient->OnTelnetWont(*src);
				++src; --size;
				m_State = eStateData;
				break;

			case eStateDo:
#if defined(TELNET_DEBUGGING)
				if (TELOPT_OK(*src))	log("NET: DBG: Received Telnet DO %s", TELOPT(*src));
				else					log("NET: DBG: Received Telnet DO %d", *src);
#endif
				m_pClient->OnTelnetDo(*src);
				++src; --size;
				m_State = eStateData;
				break;

			case eStateDont:
#if defined(TELNET_DEBUGGING)
				if (TELOPT_OK(*src))	log("NET: DBG: Received Telnet DONT %s", TELOPT(*src));
				else					log("NET: DBG: Received Telnet DONT %d", *src);
#endif
				m_pClient->OnTelnetDont(*src);
				++src; --size;
				m_State = eStateData;
				break;
			
			case eStateSubnegotiation:
				do
				{
					c = *src++;
					--size;
					
					if (c == IAC)
					{
						m_State = eStateSubnegotiationCheckFinished;
						break;
					}
					else
					{
						m_SubnegotiationBuffer.push_back(c);
					}
				} while (size > 0);
				
				break;
			
			case eStateSubnegotiationCheckFinished:
				{
					c = *src++;
					--size;
					
					if (c == SE)
					{
						m_pClient->OnTelnetSubnegotiate(
							m_SubnegotiationBuffer.c_str(),
							m_SubnegotiationBuffer.length());
						
						m_SubnegotiationBuffer.clear();
						
						m_State = eStateData;
					}
					else	//	It must be a double IAC! Or something else whacky...
					{
						//	We didn't push the previous one, so it isn't doubled in the input
						m_SubnegotiationBuffer.push_back(c);
						m_State = eStateSubnegotiation;
					}
				}
				
				break;
		}
	}
	
	return (dest - out);	//	Size written
}


void TelnetProtocol::SendRequest(unsigned char request, char code)
{
	char sequence[] = 
	{
		(char)IAC,
		(char)request,
		code/*,
		'\0'*/
	};
	
#if defined(TELNET_DEBUGGING)
	if (TELOPT_OK(code))		log("NET: DBG: Sent %s %s", TELCMD(request), TELOPT(code));
	else						log("NET: DBG: Sent %s %d", TELCMD(request), code);
#endif

	m_pClient->TelnetSendData(sequence, sizeof(sequence));
}


void TelnetProtocol::SendSubnegotiation(const char *data, size_t size)
{
//	const char sub_begin[] = { (char)IAC, (char)SB };
//	const char sub_end[] = { (char)IAC, (char)SE };
	
	if (size == 0)
		return;
	
#if defined(TELNET_DEBUGGING)
	if (TELOPT_OK(data[0]))		log("NET: DBG: Sent SUB %s", TELOPT(data[0]));
	else						log("NET: DBG: Sent SUB %d", data[0]);
#endif

	BUFFER(buf, (size * 2) + 4);
	
	const char *src = data;
	char *dst = buf;
	size_t sz2 = size;
	
	*dst++ = (char)IAC;	++size;
	*dst++ = (char)SB;	++size;
	
	do
	{
		char c = *src++;
		*dst++ = c;
		
		if (c == (char)IAC)
		{
			*dst++ = c;
			++size;	
		}
	} while (--sz2 > 0);
	
	*dst++ = (char)IAC;	++size;
	*dst++ = (char)SE;	++size;
	
//	m_pClient->TelnetSendData(sub_begin, sizeof(sub_begin));
	m_pClient->TelnetSendData(buf, size);
//	m_pClient->TelnetSendData(sub_end, sizeof(sub_end));
}
