/*************************************************************************
*   File: descriptors.c++            Part of Aliens vs Predator: The MUD *
*  Usage: Primary code for descriptors                                   *
*************************************************************************/

#include "types.h"

#ifdef _WIN32
# include <winsock2.h>
#endif
#ifdef CIRCLE_UNIX
# include <sys/socket.h>
#endif

#include "network/protocols/Telnet.h"

#include "descriptors.h"
#include "characters.h"
#include "utils.h"
#include "interpreter.h"
#include "olc.h"
#include "comm.h"

#include "buffer.h"

DescriptorList descriptor_list;	//	master desc list

//	Buffer management externs
extern int buf_largecount;
extern int buf_overflows;
extern int buf_switches;
txt_q		bufpool;

int gamebyteswritten = 0,
	gamebytescompresseddiff = 0,
	gamebytesread = 0;

static int		last_desc = 1;	//	last descriptor number

#if defined(__MWERKS__)
#define isascii(c)		(((c) & ~0x7f) == 0)
#endif

//	New Telnet Codes
#define TELOPT_MCCP2		86
#define TELOPT_MSP			90
#define TELOPT_MXP			91
#define TELOPT_ZMP			93





DescriptorData::DescriptorData(socket_t sock)
:	m_Socket(sock)
,	m_ConnectionNumber(0)
,	m_LoginTime(time(0))
,	m_ConnectionState(this)
,	m_FailedPasswordAttempts(0)
,	wait(1)
,	m_PromptState(DescriptorData::WantsPrompt)
,	m_InputSize(0)
,	m_HistoryPos(0)
,	m_SmallOutputBuffer(SMALL_BUFSIZE)
,	m_CurrentOutputBuffer(&m_SmallOutputBuffer)
,	m_bOutputOverflow(false)
,	m_Character(NULL)
,	m_OrigCharacter(NULL)
,	m_Snooping(NULL)
,	olc(NULL)
//,	compressionMode(0)
//,	deflate(NULL)
{
	memset(&m_SocketAddress, 0, sizeof(m_SocketAddress));

	memset(m_Input, 0, sizeof(m_Input));
	
	do
	{
		m_ConnectionNumber = last_desc++;
		if (last_desc == 1000)
			last_desc = 1;
		
		FOREACH(DescriptorList, descriptor_list, iter)
		{
			const DescriptorData *d = *iter;
			if (d->m_ConnectionNumber == m_ConnectionNumber)
			{
				m_ConnectionNumber = 0;
				break;
			}
		}
	} while (m_ConnectionNumber == 0);
	
	//	The Menu is the basic fallback state of all connections!
	m_ConnectionState.Push(new MenuConnectionState);
	
	StartTelnet();
}


DescriptorData::~DescriptorData(void)
{
	if (m_CurrentOutputBuffer != &m_SmallOutputBuffer)
	{
		bufpool.push_front(m_CurrentOutputBuffer);
		m_CurrentOutputBuffer = &m_SmallOutputBuffer;
	}
}



void DescriptorData::Close()
{
	descriptor_list.remove(this);
	
	CLOSE_SOCKET(m_Socket);

	/* Forget snooping */
	if (m_Snooping)
		m_Snooping->m_SnoopedBy.remove(this);
	
	FOREACH(DescriptorList, m_SnoopedBy, d2)
	{
		(*d2)->Write("Your victim is no longer among us.\n");
		(*d2)->m_Snooping = NULL;

		if (m_Character)
			mudlogf(BRF, (*d2)->m_Character->GetLevel(), true, "(GC) %s stops snooping %s.", (*d2)->m_Character->GetName(), m_Character->GetName());
		else
			mudlogf(BRF, (*d2)->m_Character->GetLevel(), true, "(GC) %s stops snooping.", (*d2)->m_Character->GetName());
	}
	m_SnoopedBy.clear();

	if (m_Character)
	{
		if (PLR_FLAGGED(m_Character, PLR_MAILING) && m_StringEditor)
		{
			m_StringEditor = NULL;
		}
		if (m_Character->InRoom())
		{
			m_Character->Save();
			if (m_Character->InRoom())
				act("$n has lost $s link.", TRUE, m_Character, 0, 0, TO_ROOM);
			mudlogf( NRM, m_Character->GetPlayer()->m_StaffInvisLevel, TRUE,  "Closing link to: %s.", m_Character->m_Name.empty() ? "UNDEFINED" : m_Character->GetName());
			m_Character->desc = NULL;
			
			m_Character->GetPlayer()->m_Host = m_Host;
		}
		else
		{
			mudlogf(CMP, MAX(LVL_IMMORT, m_Character->GetLevel()), TRUE, "Losing player: %s.",
					m_Character->m_Name.empty() ? "<null>" : m_Character->GetName());

			m_Character->Purge();
		}
	}
	else
		mudlog("Losing descriptor without char.", CMP, LVL_IMMORT, TRUE);

	/* JE 2/22/95 -- part of my unending quest to make switch stable */
	if (m_OrigCharacter)
		m_OrigCharacter->desc = NULL;

	delete this;
}


/*
 * perform substitution for the '^..^' csh-esque syntax
 * orig is the orig string (i.e. the one being modified.
 * subst contains the substition string, i.e. "^telm^tell"
 */
static bool PerformSubstitution(const char *orig, char *subst)
{
	BUFFER(new_t, MAX_INPUT_LENGTH + 5);
	char *first, *second;
	const char *strpos;

	//	first is the position of the beginning of the first string (the one
	//	to be replaced
	first = subst + 1;

	//	now find the second '^'
	if (!(second = strchr(first, '^'))) {
		return false;
	}
	//	terminate "first" at the position of the '^' and make 'second' point
	//	to the beginning of the second string
	*(second++) = '\0';

	//	now, see if the contents of the first string appear in the original
	if (!(strpos = str_str(orig, first))) {
		return false;
	}
	//	now, we construct the new string for output.

	//	first, everything in the original, up to the string to be replaced
	strncpy(new_t, orig, (strpos - orig));
	new_t[(strpos - orig)] = '\0';

	//	now, the replacement string
	strncat(new_t, second, (MAX_INPUT_LENGTH - strlen(new_t) - 1));

	//	now, if there's anything left in the original after the string to replaced,
	//	copy that too.
	if (((strpos - orig) + strlen(first)) < strlen(orig))
		strncat(new_t, strpos + strlen(first), (MAX_INPUT_LENGTH - strlen(new_t) - 1));

	//	terminate the string in case of an overflow from strncat
	new_t[MAX_INPUT_LENGTH - 1] = '\0';
	strcpy(subst, new_t);
  
	return true;
}



int DescriptorData::ProcessInput()
{
	char *	pNewline = NULL;

	do
	{
		size_t		spaceLeft = sizeof(m_Input) - m_InputSize - 1;
		
		if (spaceLeft <= 0)
		{
			log("NET: DescriptorData::ProcessInput(): about to close connection: input overflow");
			return -1;
		}
		
		int bytesRead = recv(m_Socket, m_Input + m_InputSize, spaceLeft, 0);
		
		if (bytesRead < 0)
		{
#if defined(_WIN32)
			if (WSAGetLastError() != WSAEWOULDBLOCK)
#else
#if defined(EWOULDBLOCK)
			if (errno == EWOULDBLOCK)
				errno = EAGAIN;
#endif
			if (errno != EAGAIN && errno != EINTR)
#endif
			{
				perror("NET: DescriptorData::ProcessInput():  about to lose connection");
				return -1;		//	some error condition was encountered on read
			} else
				return 0;		//	the read would have blocked: no data there, but its ok
		}
		else if (bytesRead == 0)
		{
			log("NET: DescriptorData::ProcessInput(): EOF on socket read (connection broken by peer)");
			return -1;
		}
		//	at this point, we know we got some data from the read
		
		bytesRead = m_TelnetProtocol.ProcessBuffer(m_Input + m_InputSize, m_Input + m_InputSize, bytesRead, bytesRead);

		m_Input[m_InputSize + bytesRead] = '\0';	//	NULL-terminate the input

		if (/*m_bWindowsTelnet*/ m_bForceEcho && m_bEchoOn
			&& bytesRead > 0
			/* && m_bEchoEnabled */ )
		{
			Send(m_Input + m_InputSize, bytesRead);
		}
		
		//	search for a newline in the data we just read
		for (char *p = m_Input + m_InputSize ; *p && !pNewline; ++p)
			if (ISNEWL(*p))
				pNewline = p;

		m_InputSize += bytesRead;
		gamebytesread += bytesRead;
	}
	while ( !pNewline );
	
	if (m_InputSize == 0 || pNewline == NULL)
		return 0;
	
	//	Parse the Input into Commands
	BUFFER(buf, MAX_INPUT_LENGTH + 8);
	char *	src = m_Input;
	
	while (pNewline)
	{
		char *		dest = buf;
		size_t		spaceLeft = MAX_INPUT_LENGTH - 1;
		
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
			Write("Line too long.  Truncated to:\n");
			Write(buf);
			Write("\n");
		}
		
		if (!m_SnoopedBy.empty())
		{
			FOREACH(DescriptorList, m_SnoopedBy, d)
			{
				(*d)->Write("% ");
				(*d)->Write(buf);
				(*d)->Write("\n");
			}
		}
		
		bool	bFailedSubstitution = false;

		if (GetState()->IsPlaying())
		{
			if (*buf == '!')
			{
				if (!buf[1])
					strcpy(buf, m_LastInput.c_str());
				else
				{
					char *commandln = (buf + 1);
					int starting_pos = m_HistoryPos;
					int cnt = (m_HistoryPos == 0 ? HISTORY_SIZE - 1 : m_HistoryPos - 1);

					skip_spaces(commandln);
					
					for (; cnt != starting_pos; --cnt)
					{
						if (!m_History[cnt].empty() && is_abbrev(commandln, m_History[cnt].c_str())) {
							strcpy(buf, m_History[cnt].c_str());
							m_LastInput = buf;
							break;
						}
						if (cnt == 0)	/* At top, loop to bottom. */
							cnt = HISTORY_SIZE;
					}
				}
			}
			else if (*buf == '^')
			{
				bFailedSubstitution = !PerformSubstitution(m_LastInput.c_str(), buf);
				if (bFailedSubstitution)
					Write("Invalid substitution.\n");
				else
					m_LastInput = buf;
			}
			else 
			{
				m_LastInput = buf;
				m_History[m_HistoryPos] = buf;	//	Save the new
				if (++m_HistoryPos >= HISTORY_SIZE)		//	Wrap to top
					m_HistoryPos = 0;
			}
		}

		if (!bFailedSubstitution)
			m_InputQueue.write(buf);
		
		src = pNewline;
		
		/* find the end of this line */
		while (*src == '\r')	++src;
		if (ISNEWL(*src))		++src;
		while (*src == '\r')	++src;
		
		/* see if there's another newline in the input buffer */
		pNewline = NULL;
		for (char *p = src; *p && !pNewline; ++p)
			if (ISNEWL(*p))
				pNewline = p;
	}

	/* now move the rest of the buffer up to the beginning for the next pass */
	char *dest = m_Input;
	while (*src)
		*dest++ = *src++;
	*dest = '\0';
	
	m_InputSize = dest - m_Input;
	
	return 1;
}


extern char *make_prompt(DescriptorData *point);
extern void ProcessColor(const char *src, char *dest, size_t len, bool bColor);
	
int DescriptorData::ProcessOutput()
{
	static char	i[MAX_SOCK_BUF];
	static int	result;
	int			written = 0;

	//	handle snooping: prepend "% " and send to snooper
	if (m_PromptState != DescriptorData::PendingPrompt
		&& !m_SnoopedBy.empty())
	{
		FOREACH(DescriptorList, m_SnoopedBy, d)
		{
			(*d)->Write("% ");
			(*d)->Write(m_CurrentOutputBuffer->m_Text.c_str());
			(*d)->Write("%%");
		}
	}
	
	//	If we have a prompt (and not pending), start with a newline
	//	otherwise start with a clean slate	
	strcpy(i, m_PromptState == DescriptorData::HasPrompt ? "\n" : "");
	strcat(i, m_CurrentOutputBuffer->m_Text.c_str());	//	now, append the 'real' output
	
	//	add the extra CRLF if the person isn't in compact mode
	//	and does not have a pending prompt
	if (GetState()->IsPlaying()
		&& m_Character && !PRF_FLAGGED(m_Character, PRF_COMPACT)
		&& m_PromptState != DescriptorData::PendingPrompt
		&& !m_bOutputOverflow )
		strcat(i, "\n");

	//	add a prompt, if one isn't pending, and we aren't overflowing
	if (m_PromptState != DescriptorData::PendingPrompt && !m_bOutputOverflow)
	{
		strcat(i, make_prompt(this));
		if (/*m_bWindowsTelnet*/ m_bForceEcho && m_bEchoOn && m_InputSize)
			strncat(i, m_Input, sizeof(i) - 1);
	}
	
	ProcessColor(i, i, sizeof(i) - GARBAGE_SPACE, m_Character ? PRF_FLAGGED(m_Character, PRF_COLOR) : false);

	//	We add the overflow notice after the color processing
	//	Otherwise, colors can push the overflow notice off the edge
	if (m_bOutputOverflow)	//	if we're in the overflow state, notify the user
		strcat(i, "\r\n**OVERFLOW**\r\n");
	
	//	 now, send the output.  If this is an 'interruption', use the prepended
	//	CRLF, otherwise send the straight output sans CRLF.
//	if (t->has_prompt)	result = SEND_TO_SOCKET(t->descriptor, i);
//	else				result = SEND_TO_SOCKET(t->descriptor, i + 2);

	size_t length = strlen(i);
	
	if (m_bDoEOROnPrompt)
	{
		char eor_str[] = { IAC, EOR };
		memcpy(i + length, eor_str, sizeof(eor_str));
		length += sizeof(eor_str);
	}
	
	//	Anything to write?
	if (length == 0)
		return 0;
	
	result = Send(i, length);

	written = result >= 0 ? result : -result;

	//	if we were using a large buffer, put the large buffer on the buffer pool
	//	and switch back to the small one
	if (m_CurrentOutputBuffer != &m_SmallOutputBuffer)
	{
		bufpool.push_front(m_CurrentOutputBuffer);
		m_CurrentOutputBuffer = &m_SmallOutputBuffer;
	}
	/* reset total bufspace back to that of a small buffer */
	m_bOutputOverflow = false;
	m_CurrentOutputBuffer->m_Text.clear();
	
	/* Error, cut off. */
	if (result == 0)
		return -1;

	/* Normal case, wrote ok. */
	if (result > 0)
		return 1;

	/*
	 * We blocked, restore the unwritten output. Known
	 * bug in that the snooping immortal will see it twice
	 * but details...
	 */
	Write(i + written);
	return 0;
}



//	Add a new string to a player's output queue
void DescriptorData::Write(const char *txt)
{
	Write(txt, strlen(txt));
}


void DescriptorData::Write(const char *txt, size_t size)
{
	//	TODO: Send to snoopers

	//	If we're in the overflow state already, ignore this new output
	if (m_bOutputOverflow)
		return;

	//	Switch to larger buffer if we can't fit what is coming, and aren't already using one
	if (size > m_CurrentOutputBuffer->getRemainingSpace()
		&& m_CurrentOutputBuffer == &m_SmallOutputBuffer)
	{
		++buf_switches;

		/* if the pool has a buffer in it, grab it */
		if (!bufpool.empty())
		{
			m_CurrentOutputBuffer = bufpool.front();
			bufpool.pop_front();
		}
		else
		{
			//	Create a new one
			this->m_CurrentOutputBuffer = new txt_block(LARGE_BUFSIZE);
			++buf_largecount;
		}

		//	Set up the new buffer
		m_CurrentOutputBuffer->m_Text = m_SmallOutputBuffer.m_Text;
	}
	
	//	If we can fit it, do so
	if (m_CurrentOutputBuffer->getRemainingSpace() >= size)
	{
		m_CurrentOutputBuffer->m_Text.append(txt, size);
		return;
	}

	//	Otherwise, fit what we can
	if (m_CurrentOutputBuffer->getRemainingSpace() > 0)
	{
		m_CurrentOutputBuffer->m_Text.append(txt, m_CurrentOutputBuffer->getRemainingSpace());
	}
	
	m_bOutputOverflow = true;
	++buf_overflows;
}


ssize_t DescriptorData::Send(const char *data, size_t size)
{
	if (CompressionActive())
	{
		if (size == 0)
			return 0;
	
		return CompressSend(data, size);
	}

	return Send(m_Socket, data, size);
}


ssize_t DescriptorData::Send(socket_t sock, const char *data, size_t size)
{
	ssize_t totalWritten = 0;

	if (size == 0)
	{
		log("NET: DescriptorData::Send(): write nothing?!");
		return 0;
	}

	while (size > 0)
	{
		ssize_t bytesWritten = send(sock, data, size, 0);

		if (bytesWritten < 0)
		{
#if defined(_WIN32)
			if (WSAGetLastError() == WSAEWOULDBLOCK)
#else
#ifdef EWOULDBLOCK
			if (errno == EWOULDBLOCK)
				errno = EAGAIN;
#endif /* EWOULDBLOCK */
			if (errno == EAGAIN)
#endif
				log("NET: DescriptorData::Send(): socket write would block, about to close");
			else
				perror("Write to descriptor");
			return -1;
		}
		else if (bytesWritten == 0)
		{
			log("NET: DescriptorData::Send(): socket write would block, about to close");
			return -totalWritten;
		}
		
		size			-= bytesWritten;
		data			+= bytesWritten;
		totalWritten	+= bytesWritten;
		gamebyteswritten += bytesWritten;
	}

	return totalWritten;
}


void DescriptorData::StartStringEditor(size_t maxlen, StringEditor::Callback *callback)
{
	m_StringEditor = new StringEditor("", maxlen, callback);
}


class SharedStringEditor : public DescriptorData::StringEditor
{
public:
	SharedStringEditor(Lexi::SharedString &str, size_t mx, StringEditor::Callback *callback)
	:	StringEditor(str.c_str(), mx, callback)
	,	m_String(str)
	{}
	
	virtual void	save()
	{
		m_String = c_str();
	}
	virtual void *	getTarget() { return &m_String; }
	
	Lexi::SharedString &	m_String;	//	Target of save
};


void DescriptorData::StartStringEditor(Lexi::SharedString &str, size_t maxlen, StringEditor::Callback *callback)
{
	m_StringEditor = new SharedStringEditor(str, maxlen, callback);
}


class LexiStringEditor : public DescriptorData::StringEditor
{
public:
	LexiStringEditor(Lexi::String &str, size_t mx, StringEditor::Callback *callback)
	:	StringEditor(str, mx, callback)
	,	m_String(str)
	{}
	
	virtual void	save()
	{
		m_String = c_str();
	}
	virtual void *	getTarget() { return &m_String; }
	
	Lexi::String &	m_String;	//	Target of save
};


void DescriptorData::StartStringEditor(Lexi::String &str, size_t maxlen, StringEditor::Callback *callback)
{
	m_StringEditor = new LexiStringEditor(str, maxlen, callback);
}






TelnetClient::TelnetClient()
:	m_TelnetProtocol(this)
,	m_Width(0), m_Height(0)
,	m_bWindowsTelnet(false)
,	m_bEchoOn(true)
,	m_bForceEcho(false)
,	m_bDoEOROnPrompt(false)
,	m_bMXPSupported(false)
{
//	memset(m_TelnetOptions, 0, sizeof(m_TelnetOptions));
}


void TelnetClient::StartTelnet()
{
#if 1
	m_TelnetProtocol.SendRequest(WONT, TELOPT_ECHO);	//	State that we wont echo
//	m_TelnetProtocol.SendRequest(WILL, TELOPT_EOR);		//	State that we will EOR
	m_TelnetProtocol.SendRequest(DO, TELOPT_TTYPE);		//	Request Terminal Type
	m_TelnetProtocol.SendRequest(DO, TELOPT_NEW_ENVIRON);//	Request Enviornment variables - detect Windows Telnet
	m_TelnetProtocol.SendRequest(DO, TELOPT_NAWS);		//	Request to Negotiate About Window Size
//	m_TelnetProtocol.SendRequest(DO, TELOPT_LFLOW);		//	Request client do Remote Flow Control

	m_TelnetProtocol.SendRequest(WILL, TELOPT_MCCP2);	//	State that we will support MCCP2
//	m_TelnetProtocol.SendRequest(WILL, TELOPT_MXP);		//	State that we will support MXP
#endif
}


void TelnetClient::SetEchoEnabled(bool bEnabled)
{
	if (m_bEchoOn == bEnabled)
		return;
	
	m_bEchoOn = bEnabled;
	m_TelnetProtocol.SendRequest(m_bEchoOn ? WONT : WILL, TELOPT_ECHO);
}


bool TelnetClient::OnTelnetWill(char code)
{
	switch (code)
	{
		case TELOPT_NAWS:
			//	Handled
			break;
		
		case TELOPT_TTYPE:
			{
				const char ttype[] = { TELOPT_TTYPE, TELQUAL_SEND };
				m_TelnetProtocol.SendSubnegotiation(ttype, sizeof(ttype));
			}
			break;
		
		case TELOPT_NEW_ENVIRON:
			{
				const char newenviron[] =
				{
					TELOPT_NEW_ENVIRON, TELQUAL_SEND, NEW_ENV_VAR,
					'S', 'Y', 'S', 'T', 'E', 'M', 'T', 'Y', 'P', 'E'
				};
				m_TelnetProtocol.SendSubnegotiation(newenviron, sizeof(newenviron));
			}
			break;
		
		default:
			m_TelnetProtocol.SendRequest(DONT, code);
			return false;
	}
	
	return true;
}


bool TelnetClient::OnTelnetWont(char code)
{
	switch (code)
	{
		case TELOPT_NAWS:
			//	WIDTH not supported
			break;
		
		case TELOPT_NEW_ENVIRON:
			break;
		
		default:
			return false;
	}
	
	return true;
}


bool TelnetClient::OnTelnetDo(char code)
{
	switch (code)
	{
/*
		case TELOPT_MXP:
			//Write("MXP On\n");
			
			{
				//char startMXP[] = { TELOPT_MXP };
				//m_Telnet.SendSubnegotiation( startMXP, sizeof(startMXP) );
				
//				Write("\x1B[6z");	//	Force into secure mode
//				Lexi::BufferedFile	file("data/MXP Definitions.txt");
				
//				while (file.good())
//				{
//					Write(file.getline());
//				}
			}

			SetTelnetOption(TELOPT_MXP, OptionAccepted);
			
			break;
*/
		case TELOPT_ECHO:
			//m_bForceEcho = true;
			break;

		case TELOPT_EOR:
			if (!m_bDoEOROnPrompt)
			{
				m_bDoEOROnPrompt = true;
				m_TelnetProtocol.SendRequest(WILL, code);
			}
			break;
			
//		case TELOPT_MXP:
			//SetTelnetOption(code, OptionAccepted);
//			m_bMXPSupported = true;
//			break;

		default:
			m_TelnetProtocol.SendRequest(WONT, code);
			return false;
	}

	return true;
}


bool TelnetClient::OnTelnetDont(char code)
{
	switch (code)
	{
		case TELOPT_ECHO:
			if (!m_bForceEcho)
				m_bEchoOn = false;
			break;
			
		case TELOPT_EOR:
			if (m_bDoEOROnPrompt)
			{
				m_bDoEOROnPrompt = false;
				m_TelnetProtocol.SendRequest(WONT, code);	
			}
			break;
		
//		case TELOPT_MXP:
//			if (m_bMXPSupported)
//				m_TelnetProtocol.SendRequest(WONT, code);
//			m_bMXPSupported = false;
//			break;

		default:
			m_TelnetProtocol.SendRequest(WONT, code);
			return false;
	}

	return true;
}


void TelnetClient::OnTelnetSubnegotiate(const char *data, size_t size)
{
	if (size == 0)	//	Error condition
		return;
	
	switch (data[0])
	{
		case TELOPT_NAWS:
			{
				m_Width = ntohs(*(unsigned short *)(data + 1));
				m_Height = ntohs(*(unsigned short *)(data + 3));
			}
			break;
		
		case TELOPT_TTYPE:
			if (size > 2 && data[1] == TELQUAL_IS)
			{
				bool	xterm = false, ansi = false;
				
				m_Terminal.assign(data + 2, size - 2);
				
				//BUFFER(buf, MAX_STRING_LENGTH);
				//sprintf(buf, "Terminal is: %s\n", m_Terminal.c_str());
				//Write(buf);

				//if (m_Terminal == "XTERM")		Write("XTerm detected\n");
				//else if (m_Terminal == "ANSI")	Write("ANSI detected\n");
				//else if (m_Terminal == "zmud")	Write("ZMud detected\n");
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
						m_bWindowsTelnet = true;
						m_bForceEcho = m_bEchoOn = true;
						//Write("Windows Telnet detected\n");
					}
				}
			}
			break;
	}
}


/*
void TelnetClient::SetTelnetOption(char code, TelnetOptionStatus status)
{
	int byteNumber = code >> 2;
	int optionShift = (code & 0x3) << 1;
	
	unsigned char optionByte = m_TelnetOptions[byteNumber];
	
	//	Mask out old options
	optionByte &= ~(OptionBitsMask << optionShift);
	optionByte |= (status << optionShift);
	
	m_TelnetOptions[byteNumber] = optionByte;
}
*/

static void *zlib_alloc(void *opaque, unsigned int items, unsigned int size)
{
    return calloc(items, size);
}

static void zlib_free(void *opaque, void *address)
{
    free(address);
}


CompressionClient::CompressionClient()
:	m_bMCCP2Supported(false)
,	m_pStream(NULL)
,	m_State(StateInactive)
{
}


CompressionClient::~CompressionClient()
{
	if (m_pStream)
		deflateEnd(m_pStream);
}


char CompressionClient::ms_CompressionBuffer[MAX_SOCK_BUF];

//#define COMPRESSION_DEBUGGING

void CompressionClient::StartCompression()
{
	if (CompressionActive())
		return;
	
	const char enableCompressionString[] = { TELOPT_MCCP2 };
	m_TelnetProtocol.SendSubnegotiation( enableCompressionString, sizeof(enableCompressionString) );
	
	m_pStream = new z_stream;
	memset(m_pStream, 0, sizeof(z_stream));
	
	m_pStream->zalloc = zlib_alloc;
	m_pStream->zfree = zlib_free;
//	m_pStream->next_in = NULL;		//	small_outbuf?
//	m_pStream->next_out = NULL;	//	small_outbuf?
//	m_pStream->avail_out = 0;	//	SMALL_BUFSIZE?
//	m_pStream->avail_in = 0;
	
#if defined(COMPRESSION_DEBUGGING)
	log("NET: DBG: CompressionClient::StartCompression(): starting compression");
#endif
	int err = deflateInit(m_pStream, Z_DEFAULT_COMPRESSION);
	if (err)
	{
		log("NET: CompressionClient::StartCompression(): deflateInit returned %d.", err);
		m_pStream = NULL;
		m_State = StateFailed;
	}
	else
		m_State = StateActive; //StateStarting;
}


void CompressionClient::StopCompression()
{
	if (!CompressionActive())
		return;
	
	m_pStream->next_out = (unsigned char *)ms_CompressionBuffer;
	m_pStream->avail_out = sizeof(ms_CompressionBuffer);
	
	int err = deflate(m_pStream, Z_FINISH);
	
	if (err != Z_STREAM_END)
		log("NET: CompressionClient::StopCompression(): deflate returned %d upon Z_FINISH. (in: %d, out: %d)", err, m_pStream->avail_in, m_pStream->avail_out);
	
	if (m_pStream->avail_out != sizeof(ms_CompressionBuffer))
		CompressionSendData((char *)ms_CompressionBuffer, /*6*/ sizeof(ms_CompressionBuffer) - m_pStream->avail_out);	//	Why 6???	- this will be an immediate write
	
	err = deflateEnd(m_pStream);
	
	if (err != Z_OK)
		log("NET: CompressionClient::StopCompression(): deflateEnd returned %d. (in: %d, out: %d)", err, m_pStream->avail_in, m_pStream->avail_out);
	
	m_pStream = NULL;
	m_State = StateInactive;
}


ssize_t CompressionClient::CompressSend(const char *data, size_t size)
{
	size_t totalWritten = 0;
	
	m_pStream->next_in = (unsigned char *)data;
	m_pStream->avail_in = size;
	
	m_pStream->next_out = (unsigned char *)ms_CompressionBuffer;
	m_pStream->avail_out = sizeof(ms_CompressionBuffer);
	
	do
	{
		deflate( m_pStream, Z_NO_FLUSH );
		
		if ( m_pStream->avail_out == 0 )
		{
			totalWritten += CompressionSendData(ms_CompressionBuffer, sizeof(ms_CompressionBuffer));
			m_pStream->next_out = (unsigned char *)ms_CompressionBuffer;
			m_pStream->avail_out = sizeof(ms_CompressionBuffer);
		}
	} while ( m_pStream->avail_in);
	
	deflate(m_pStream, Z_SYNC_FLUSH);
	
	size_t bytesToWrite = sizeof(ms_CompressionBuffer) - m_pStream->avail_out;
	if (bytesToWrite > 0)
	{
		totalWritten += CompressionSendData(ms_CompressionBuffer, bytesToWrite);
	}
	
	gamebytescompresseddiff += size - totalWritten;
		
	return totalWritten;
}



bool CompressionClient::OnTelnetDo(char code)
{
	if (code == TELOPT_MCCP2)
	{
		m_bMCCP2Supported = true;
		StartCompression();
		return true;
	}
	
	return TelnetClient::OnTelnetDo(code);
}


bool CompressionClient::OnTelnetDont(char code)
{
	if (code == TELOPT_MCCP2)
	{
		if (m_bMCCP2Supported)
			m_TelnetProtocol.SendRequest(WONT, code);
		
		m_bMCCP2Supported = false;
		StopCompression();
		return true;
	}
	
	return TelnetClient::OnTelnetDont(code);
}
