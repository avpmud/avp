//	TODO: STELLAR CONQUEST COPYRIGHT NOTICE

#ifndef __NETWORK_SOCKET_H__
#define __NETWORK_SOCKET_H__


#include "lexistring.h"
#include "lexilist.h"


#if defined(_WIN32)
# include <windows.h>
#ifndef socklen_t
# define socklen_t int
#endif
#ifndef ssize_t
# define ssize_t long
#endif
#ifndef socket_t
#define socket_t int
#endif
#else
# include <sys/socket.h>
# include <netinet/in.h>
#endif

//typedef int socket_t;



class SocketClient
{
public:
	enum EventResult
	{
		eSocketClosed = -1,
		eOK = 0,
	};

	virtual bool		IsSocketInputEnabled() { return false; }
	virtual bool		IsSocketOutputEnabled() { return false; }
	virtual bool		IsSocketExceptionEnabled() { return false; }

	virtual EventResult	OnSocketInput() { return eOK; }
	virtual EventResult	OnSocketOutput() { return eOK; }
	virtual EventResult	OnSocketException() {return eOK; }
	
	virtual void		OnSocketUpdate() {};
};



class Socket
{
public:
	//	Creation methods
	static void			InitSystem();
	
	Socket *			Accept();
	static Socket *		Listen(const char *ip, unsigned short port);
	static bool			Socketpair(Socket *&sock1, Socket *&sock2);

						~Socket();

	int					Send(const void *data, int length) const;
	int					Receive(const void *buffer, int size) const;

	void				SetClient(SocketClient *client) { m_pClient = client; }
	SocketClient *		GetClient() const { return m_pClient; }

	const char *		GetResolvedAddress() const { return m_ResolvedAddress.c_str(); }
	
	static int			GetMaximumConnectionsAllowed() { return ms_MaximumConnections; }
	static void			ProcessSockets_Poll();
	static void			ProcessSockets_Exceptions();
	static void			ProcessSockets_Input();
	static void			ProcessSockets_Update();
	static void			ProcessSockets_Output();
	static void			ProcessSockets();
	
protected:
	socket_t			GetSocket() 	{ return m_Socket; }
	
private:
						Socket();
						
	socket_t			m_Socket;
	struct sockaddr_in	m_Address;
	Lexi::String		m_ResolvedAddress;

	SocketClient *		m_pClient;
	
	bool				SetReuseaddr();
	bool				SetSndbuf();
	bool				SetLinger();
	bool				SetNonblock();
	
	//	Information Tracking
public:
	static int			ms_TotalDataWritten;
	static int			ms_TotalDataRead;
	static int			ms_TotalSocketsOpened;
	static int			ms_TotalSocketsOpen;

private:
						Socket(const Socket &);
											
	static int			ms_MaximumConnections;

	typedef Lexi::List<Socket *>	SocketList;
	static SocketList	ms_SocketList;

	static fd_set		ms_InputSet,
						ms_OutputSet,
						ms_ExceptionSet;
};


#endif // __SOCKET_H__