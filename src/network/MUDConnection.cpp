
#include "types.h"

#include "MUDConnection.h"

#include "buffer.h"
#include "utils.h"

#include <ctype.h>


#if defined(__MWERKS__)
#define isascii(c)		(((c) & ~0x7f) == 0)
#endif


MUDConnection::MUDConnection(Socket *pSocket)
:	TelnetConnection(pSocket)
{
}


void MUDConnection::Update()
{
}


void MUDConnection::Start()
{
}


int MUDConnection::Send(const void *data, size_t length)
{
	//	Color Process It?
	//	Compress it?

//	size_t newSize = m_nOutputSize + length;
//	if (newSize > m_nOutputCap)
//	{
//		log("NET: (%p)->Connection::Send(%p = %.20s, %d): Output buffer full (%d more bytes needed)", this, data, data, length, sizeof(m_outputBuffer) - newSize)	;
//	}
//	memcpy(m_outputBuffer + m_nOutputSize, data, length);
//	m_nOutputSize = newSize;
}


int MUDConnection::SendImmediate(const void *data, size_t length)
{
	return GetSocket()->Send(data, length);
}


int MUDConnection::SendImmediate(const char *str)
{
	return SendImmediate(str, strlen(str));
}


void MUDConnection::Close()
{
	log("NET: Closing connection to [%s].", GetSocket()->GetResolvedAddress());
	Connection::Close();
}


SocketClient::EventResult MUDConnection::OnSocketInput()
{
	char *	pNewline = NULL;

	do
	{
		size_t spaceLeft = sizeof(m_Input) - m_InputSize - 1;
	
		if (spaceLeft <= 0)
		{
			log("NET: MUDConnection::OnSocketInput(): socket is overflowing input, closing");
			return SocketClient::eSocketClosed;
		}
		
		int sizeRead = GetSocket()->Receive( m_Input + m_InputSize, spaceLeft );
		if (sizeRead < 0)
		{
			Close();
			return SocketClient::eSocketClosed;
		}
		
#if defined(NETWORK_DEBUGGING)
		log("NET: DBG: Received %d bytes.", sizeRead);
#endif

		//sizeRead = ProcessInputProtocols(m_inputBuffer + m_nInputSize, sizeRead);
		
		m_Input[m_InputSize + sizeRead] = '\0';	//	Null terminate
		
		for (char *p = m_Input + m_InputSize; *p; ++p)
		{
			char c = *p;
			if (c == '\r' || c == '\n')
			{
				pNewline = p;
				break;
			}
		}

		m_InputSize += sizeRead;
	}
	while (!pNewline);
	
	
	if (m_InputSize == 0 || pNewline == NULL)
		return SocketClient::eOK;
	
	//	Parse Input into Commands
	BUFFER(buf, MAX_RAW_INPUT_LENGTH + 8);
	char *		src = m_Input;
	while (pNewline)
	{
		char *		dest = buf;
		size_t		spaceLeft = MAX_RAW_INPUT_LENGTH - 1;
		
		while (spaceLeft > 0 && src < pNewline)
		{
			char c = *src++;
			
			if (c == '\b')	//	Backspace
			{
				if (dest > buf)
				{
					--dest;
					++spaceLeft;
					
					if (*dest == '$')	//	Delete duplicated '$'
					{
						--dest;
						++spaceLeft;
					}
				}
			}
			else if (isascii(c) && isprint(c))
			{
				*dest++ = c;
				--spaceLeft;
				
				if (c == '$')	//	Duplicate '$'
				{
					*dest++ = c;
					--spaceLeft;	
				}
			}
		}
		
		*dest = '\0';
		
		if ((spaceLeft <= 0) && (src < pNewline))
		{
			Send("Line too long.  Truncated to:\n");
			Send(buf);
			Send("\n");
		}
		
		//	TODO: Snooping goes here...
		
		bool	bFailedSubstitution = false;
		
		if (*buf == '!')
		{
			//	Previous or History Command
		}
		else if (*buf == '^')
		{
			//	Substitution Command
			//bFailedSubstitution = 
		}
		else
		{
			//m_LastInput = buf;
			//m_History[m_HistoryPos] = buf;
			//if (++m_HistoryPos >= HISTORY_SIZE)
			//	m_HistoryPos = 0;
		}
		
//		if (!bFailedSubstitution)
//			m_InputQueue.append(buf);

		src = pNewline;
		
		while (*src == '\r')	++src;
		if (*src == '\n')		++src;
		while (*src == '\r')	++src;
		
		pNewline = NULL;
		for (char *p = src; *p; ++p)
		{
			char c = *p;
			if (c == '\r' || c == '\n')
			{
				pNewline = p;
				break;
			}
		}
	}
	
	//	Move everything down
	char *dest = m_Input;
	while (*src)
		*dest++ = *src++;
	*dest = '\0';
	
	m_InputSize = dest - m_Input;

	return SocketClient::eOK;
}


SocketClient::EventResult MUDConnection::OnSocketOutput()
{
/*	if (m_nOutputSize == 0)	//	Nothing to output
		return SocketClient::eOK;
	
	//	TODO: MCCP: Compress the outgoing data, if we don't do it in a virtual overload of Send...
	//	specifically we want to make sure that we don't send the start-compress subnegotiation,
	//	send uncompressed text, then start compressing!
	
	int result = m_pSocket->Send( m_outputBuffer, m_nOutputSize );
	if (result <= 0)
	{
		Close();
		return SocketClient::eSocketClosed;
	}

	if (m_nOutputSize > result)
		memmove(m_outputBuffer, m_outputBuffer + result, m_nOutputSize - result);
	m_nOutputSize -= result;
*/	
	return SocketClient::eOK;
}


SocketClient::EventResult MUDConnection::OnSocketException()
{
	Close();
	return SocketClient::eSocketClosed;
}
