/*************************************************************************
*   File: descriptors.h              Part of Aliens vs Predator: The MUD *
*  Usage: Header file for descriptors                                    *
*************************************************************************/


#ifndef	__DESCRIPTORS_H__
#define __DESCRIPTORS_H__

#include "types.h"
#include "virtualid.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <netinet/in.h>
#endif
#include <zlib.h>

#include "network/protocols/TelnetProtocol.h"

#include "internal.defs.h"

#include "lexilist.h"
#include "lexistring.h"
#include "bitflags.h"
#include "lexisharedstring.h"
#include "ManagedPtr.h"
#include "connectionstates.h"

#include <vector>

//	Types
//	Modes of connectedness: used by descriptor_data.state

//	Base declarations

enum PagerBits
{
	PAGER_RAW_OUTPUT,
//	PAGER_SHOW_LINENUMS,
	
	NUM_PAGER_BITS
};



#define HISTORY_SIZE		5


class txt_block
{
public:
	txt_block() : m_Aliased(false) {}
	txt_block(size_t sz) : m_Aliased(false) { m_Text.reserve(sz - 1); }
	txt_block(const char *text, bool aliased) : m_Text(text), m_Aliased(aliased) {}
	
	Lexi::String	m_Text;
	bool			m_Aliased;
	
	size_t			getUsedSpace() { return m_Text.length(); }
	size_t			getRemainingSpace() { return m_Text.capacity() - getUsedSpace(); }
};


class txt_q : public std::list<txt_block *>
{
public:
	~txt_q() { flush(); }
	
	void write(const char *text, bool aliased = false);
	bool get(char *dest, bool &aliased);
	void flush();
};



class TelnetClient : private TelnetProtocolClient
{
public:
						TelnetClient();
						
	void				StartTelnet();
	
	const char *		GetTerminal() const { return m_Terminal.c_str(); }

	void				SetEchoEnabled(bool enable = true);

/*
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
*/

protected:
	//	TelnetProtocolClient Interface
	virtual bool		OnTelnetWill(char code);
	virtual bool		OnTelnetWont(char code);
	virtual bool		OnTelnetDo(char code);
	virtual bool		OnTelnetDont(char code);
	virtual void		OnTelnetSubnegotiate(const char *data, size_t size);
	
//	void				SetTelnetOption(char code, TelnetOptionStatus status);
//	virtual void		TelnetSendData(const char *data, size_t size);
	
public:
	//	Telnet
	TelnetProtocol		m_TelnetProtocol;
	Lexi::String		m_Terminal;
//	unsigned char		m_TelnetOptions[OptionArrayBytes];
	unsigned int		m_Width, m_Height;
	bool				m_bWindowsTelnet;
	bool				m_bEchoOn;
	bool				m_bForceEcho;
	
	bool				m_bDoEOROnPrompt;
	bool				m_bMXPSupported;
};


class CompressionClient : public TelnetClient
{
public:
	void				StartCompression();
	void				StopCompression();
	
	bool				CompressionSupported() { return m_bMCCP2Supported; }
	
	bool				CompressionFailed() { return m_State == StateFailed; }
	bool				CompressionInactive() { return m_State == StateInactive; }
	bool				CompressionActive() { return m_State == StateActive; }
	bool				CompressionStopped() { return m_State == StateStopped; }
protected:
						CompressionClient();
	virtual 			~CompressionClient();
						
	ssize_t				CompressSend(const char *data, size_t size);
	
	
	//	TelnetProtocolClient Interface
	virtual bool		OnTelnetDo(char code);
	virtual bool		OnTelnetDont(char code);
	
	//	Interface to be Implemented by Child
	virtual ssize_t		CompressionSendData(const char *buf, size_t size) = 0;
	
private:
	bool				m_bMCCP2Supported;
	ManagedPtr<z_stream>	m_pStream;
	enum
	{
		StateFailed = -1,
		StateInactive,
		StateActive,
		StateStopped
	}						m_State;
	
	static char			ms_CompressionBuffer[MAX_SOCK_BUF];
};


class DescriptorData
:	public CompressionClient
{
public:
						DescriptorData(socket_t sock);
protected:
	virtual 			~DescriptorData(void);

public:						
	CharData *			GetOriginalCharacter(void)	{ return m_OrigCharacter ? m_OrigCharacter : m_Character; }
	
	void				Write(const char *txt);
	void				Write(const char *txt, size_t size);
	ConnectionState *	GetState() { return m_ConnectionState.GetState(); }
	
	static ssize_t		Send(socket_t sock, const char *data, size_t size);
	static ssize_t		Send(socket_t sock, const char *data) { return Send(sock, data, strlen(data)); }
	ssize_t				Send(const char *data, size_t size);
	ssize_t				Send(const char *data) { return Send(data, strlen(data)); }
	
	void				Close();
	
//	int					ToggleCompression();
	
	
	int					ProcessInput();
	int					ProcessOutput();

protected:
	//	CompressionClient Implementation
	ssize_t				CompressionSendData(const char *data, size_t size) { return Send(m_Socket, data, size); }
	ssize_t				TelnetSendData(const char *data, size_t size) { return Send(data, size); }

public:
	//	Connection Information
	socket_t			m_Socket;
	struct sockaddr_in	m_SocketAddress;
	Lexi::String		m_Host;
	Lexi::String		m_HostIP;
	Lexi::String		m_Username;
	Lexi::String		m_Hostname;

	unsigned int		m_ConnectionNumber;		//	unique num assigned to desc
	time_t				m_LoginTime;			//	when the person connected

	ConnectionStateMachine	m_ConnectionState;
	int					m_FailedPasswordAttempts;//	number of bad pw attemps this login
//	ConnectionState		connected;				//	mode of 'connectedness'
	int					wait;					//	wait for how many loops

public:
	//	Paged Mode
	class Pager : private Lexi::String
	{
	public:
		typedef Lexi::BitFlags<NUM_PAGER_BITS, PagerBits>	Flags;
		
		Pager(const char *str, int pageLen, Flags flags);
		
		void			Parse(DescriptorData *d, char *input);
		
		int				GetPageNumber() { return m_PageNumber; }
		unsigned int	GetNumPages() { return m_Pages.size(); }
		
	
	private:
		const char *	FindNextPage(const char *str);
		void			OutputPage(DescriptorData *d, const char *page, size_t size);

		std::vector<const char *>	m_Pages;	//	for paging through texts
		int				m_PageLength;
		int				m_PageNumber;			//	which page are we currently showing?
		Flags			m_Flags;
	};
	ManagedPtr<Pager>			m_Pager;
	
	
	void	PageString(const char *str, Pager::Flags flags = Pager::Flags());
	
	//	Online String Editor
	class StringEditor : public Lexi::String
	{
	public:
		class Callback	//	WARNING: UNIMPLEMENTED - the callbacks are never called
		{
		public:
			virtual ~Callback() {}
			virtual void OnSave() {}
			virtual void OnAbort() {}
			virtual void OnFinished() {}
		};
		
		StringEditor(Lexi::String str, size_t mx, Callback *callback) : String(str), m_Max(mx), m_Callback(callback) { reserve(mx); }
		virtual ~StringEditor() { delete m_Callback; }
		
		virtual void	save() {}
		virtual void *	getTarget() { return NULL; }
		
		size_t			max() { return m_Max; }

		void			Parse(DescriptorData *d, char *input);
	
	private:
		size_t			m_Max;		//	Max length of string being edited
		Callback *		m_Callback;
	};
	ManagedPtr<StringEditor>	m_StringEditor;

	void				StartStringEditor(size_t maxlen, StringEditor::Callback *callback = NULL);
	void				StartStringEditor(Lexi::SharedString &str, size_t maxlen, StringEditor::Callback *callback = NULL);	//	CLONE IT !!!
	void				StartStringEditor(Lexi::String &str, size_t maxlen, StringEditor::Callback *callback = NULL);
	
	enum
	{
		WantsPrompt,
		PendingPrompt,
		HasPrompt,
	}					m_PromptState;

	//	I/O
	char				m_Input[MAX_RAW_INPUT_LENGTH];	//	buffer for raw input
	size_t				m_InputSize;
	Lexi::String		m_LastInput;					//	the last input
	Lexi::String		m_History[HISTORY_SIZE];		//	History of commands, for ! mostly.
	unsigned int		m_HistoryPos;					//	Circular array position.
	txt_q				m_InputQueue;					//	Input queue
	txt_block			m_SmallOutputBuffer;	
	txt_block *			m_CurrentOutputBuffer;
	bool				m_bOutputOverflow;

	//	Character Data
	CharData *			m_Character;			//	linked to char
	CharData *			m_OrigCharacter;		//	original char if switched
	DescriptorData *	m_Snooping;				//	Who is this char snooping
	Lexi::List<DescriptorData *>	m_SnoopedBy;//	And who is snooping this char

	//	OLC
	ManagedPtr<struct olc_data>	olc;					//	Current OLC editing data

//	int					compressionMode;
//	z_stream *			deflate;
};

typedef Lexi::List<DescriptorData *>	DescriptorList;
extern DescriptorList					descriptor_list;

#endif
