
#include "types.h"

//#include "network/Socket.h"
#include "Socket.h"

#include <stdio.h>

#include "buffer.h"
//#include "Logging.h"
#include "utils.h"


#define NUM_RESERVED_CONNECTIONS 8
#define MAX_PLAYERS 300


int	Socket::ms_TotalDataWritten = 0;
int	Socket::ms_TotalDataRead = 0;
int	Socket::ms_TotalSocketsOpened = 0;
int	Socket::ms_TotalSocketsOpen = 0;


int Socket::ms_MaximumConnections = 0;
Socket::SocketList	Socket::ms_SocketList;

fd_set	Socket::ms_InputSet,
		Socket::ms_OutputSet,
		Socket::ms_ExceptionSet;


#if defined(_WIN32)
#define MSG_DONTWAIT 0
#endif


void Socket::InitSystem()
{
#if defined(_WIN32)
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD(1, 1);

	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		log("SYSERR: NET: Winsock not available");
		exit(1);
	}
	
//	if ((wsaData.iMaxSockets - 4) < ms_MaximumConnections)
	{
		ms_MaximumConnections = wsaData.iMaxSockets - 4;
	}
#elif defined (RLIMIT_NOFILE) || defined (RLIMIT_OFILE)
#if !defined(RLIMIT_NOFILE)
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif
	struct rlimit limit;

	/* find the limit of file descs */
	method = "rlimit";
	if (getrlimit(RLIMIT_NOFILE, &limit) < 0) {
		perror("SYSERR: calling getrlimit");
		exit(1);
	}
	/* set the current to the maximum */
	limit.rlim_cur = limit.rlim_max;
	if (setrlimit(RLIMIT_NOFILE, &limit) < 0) {
		perror("SYSERR: calling setrlimit");
		exit(1);
	}
#ifdef RLIM_INFINITY
	if (limit.rlim_max == RLIM_INFINITY)
		ms_MaximumConnections = MAX_PLAYERS + NUM_RESERVED_CONNECTIONS;
	else
#endif
		ms_MaximumConnections = min(MAX_PLAYERS + NUM_RESERVED_CONNECTIONS, limit.rlim_max);

	ms_MaximumConnections = min(MAX_PLAYERS, ms_MaximumConnections - 8);

#else
#error Missing Implementation: No implemented method for determining maximum connections is supported on this machine.
#endif
	
	log("NET: Max connections: %d", ms_MaximumConnections);
}


Socket *Socket::Listen(const char *ip, unsigned short port)
{
	Socket *socket = new Socket;
	
	socket->m_Socket = ::socket(PF_INET, SOCK_STREAM, 0);
	if (socket->m_Socket == INVALID_SOCKET)
	{
		log("SYSERR: NET: Socket::Listen(%s, %hu): Error creating socket", ip, port);
		perror("NET:");
		return NULL;
	}
	
	if (!socket->SetReuseaddr())
	{
		log("SYSERR: NET: Socket::Listen(%s, %hu): setsockopt SO_REUSEADDR failed", ip, port);
		perror("NET:");
		delete socket;
		return NULL;
	}

	if (!socket->SetSndbuf())
	{
		log("SYSERR: NET: Socket::Listen(%s, %hu): setsockopt SNDBUF failed", ip, port);
		perror("NET:");
		delete socket;
		return NULL;
	}
	
	if (!socket->SetLinger())
	{
		log("SYSERR: NET: Socket::Listen(%s, %hu): setsockopt LINGER failed", ip, port);
		perror("NET:");
		delete socket;
		return NULL;
	}

	socket->m_Address.sin_family = AF_INET;
	socket->m_Address.sin_port = htons(port);
	if (!ip || !strcasecmp(ip, "ANY"))
		socket->m_Address.sin_addr.s_addr = INADDR_ANY;
	else
		socket->m_Address.sin_addr.s_addr = inet_addr(ip);
	
	if (bind(socket->m_Socket, (struct sockaddr *)&socket->m_Address, sizeof(socket->m_Address)) < 0)
	{
		log("SYSERR: NET: Socket::Listen(%s, %hu): Bind failed", ip, port);
		perror("NET:");
		delete socket;
		return NULL;
	};
	
	if (!socket->SetNonblock())
	{
		log("SYSERR: NET: Socket::Listen(%s, %hu): Fatal error setting non-blocking on socket", ip, port);
		perror("NET:");
		exit(1);
		return NULL;
	}
	
	listen(socket->m_Socket, 5);

	BUFFER(buf, MAX_STRING_LENGTH);
	sprintf(buf, "Listening: %s:%d",
		socket->m_Address.sin_addr.s_addr == INADDR_ANY ?
			"ANY" : inet_ntoa(socket->m_Address.sin_addr),
			port);
	socket->m_ResolvedAddress = buf;
	
	ms_SocketList.push_back(socket);

	return socket;
}


bool Socket::Socketpair(Socket *&sock1, Socket *&sock2)
{
#if !defined(_WIN32)
	socket_t sockets[2];

	if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sockets) < 0)
	{
		Log("NET: Socket::Socketpair(%p, %p): Unable to create sockets.", sock1, sock2);
		perror("NET:");
		return false;
	}
	
	sock1 = new Socket;
	sock2 = new Socket;
	
	sock1->m_Socket = sockets[0];
	sock2->m_Socket = sockets[1];
	
	ms_SocketList.push_back(sock1);
	ms_SocketList.push_back(sock2);

	return true;
#else
	return false;
#endif
}


Socket *Socket::Accept()
{
	Socket *socket = new Socket;
	socklen_t	i = sizeof(socket->m_Address);
	
	socket->m_Socket = accept(m_Socket, (struct sockaddr *) &socket->m_Address, &i);
	
	if (socket->m_Socket == INVALID_SOCKET)
	{
		log("SYSERR: NET: Socket::Accept(): Error accepting socket");
		perror("NET:");
		delete socket;
		return NULL;
	};
	
	++ms_TotalSocketsOpen;
	++ms_TotalSocketsOpened;
	
	//	Make it non-blocking
	if (!socket->SetNonblock())
	{
		log("SYSERR: NET: Socket::Accept(): Fatal error setting non-blocking on socket");
		perror("NET:");
		exit(1);
		return NULL;
	}

	if (!socket->SetSndbuf())
	{
		log("SYSERR: NET: Socket::Accept(): setsockopt SNDBUF failed");
		perror("NET:");
		delete socket;
		return NULL;
	}
	
	socket->m_ResolvedAddress = inet_ntoa(socket->m_Address.sin_addr);
	
	//	TODO: DO LOOKUP
	
	
	log("NET: Socket::Accept(): Opening connection from %s", socket->m_ResolvedAddress.c_str());
	
	ms_SocketList.push_back(socket);
	
	return socket;
}


Socket::Socket() :
	m_Socket(INVALID_SOCKET),
	m_pClient(NULL)
{
	memset(&m_Address, 0, sizeof(m_Address));
}


Socket::~Socket()
{
	if (m_Socket != INVALID_SOCKET)
	{
#if defined(_WIN32)
		closesocket(m_Socket);
#else
		close(m_Socket);
#endif
		ms_SocketList.remove(this);

		--ms_TotalSocketsOpen;
	}
}


int Socket::Send(const void *d, int length) const
{
	const char *	data = static_cast<const char *>(d);	//	So we can increment the pointer easily
	
	if (length == 0)
	{
		log("NET: Socket::Send(): length == 0");
		return 0;
	}
	
	ssize_t totalWritten = 0;
	while (length > 0)
	{
		ssize_t written;

		written = send(m_Socket, data, length, MSG_DONTWAIT);

		if (written < 0)	//	We had an error
		{
#if defined(_WIN32)
			if (WSAGetLastError() == WSAEWOULDBLOCK)
#else
			if (errno == EAGAIN || errno == EINTR)
#endif
				log("NET: Socket::Send(): socket write would block, closing");
			else
				perror("NET: Socket::Send(): ");
			return -1;
		}
		else if (written == 0) 
		{
			log("NET: Socket::Send(): socket write would block, closing");
			return -totalWritten;
		}

		data			+= written;
		totalWritten	+= written;
		length			-= written;
		ms_TotalDataWritten += written;
	}
	
	return totalWritten;
}


int Socket::Receive(const void *buffer, int size) const
{
	const char *	buf = static_cast<const char *>(buffer);
	int		remaining = size;
	
	ssize_t totalRead = 0;
	while (remaining > 0)	//	Don't overflow on one input!
	{
//		if (remaining <= 0)
//		{
//			log("NET: Socket::Receive(%p, %d): socket is overflowing input, closing", buffer, size);
//			return -1;
//			return totalRead;
//		}

		ssize_t bytesRead = recv(m_Socket, const_cast<char *>(buf), remaining, 0);
		
		if (bytesRead < 0)
		{
#if defined(_WIN32)
			if (WSAGetLastError() != WSAEWOULDBLOCK)
#else
			if (errno != EAGAIN && errno != EINTR)
#endif
			{
				log("NET: Socket::Receive(%p, %d): about to lose connection", buffer, size);
				perror("NET:");
				return -1;
			}
			
			return totalRead;
		}
		else if (bytesRead == 0)
		{
			log("NET: Socket::Receive(%p, %d): EOF on socket (connection broken by peer)", buffer, size);
			return -1;
		}
		
		buf				+= bytesRead;
		totalRead		+= bytesRead;
		remaining		-= bytesRead;
		ms_TotalDataRead += bytesRead;
	}
	
	return totalRead;
}


bool Socket::SetReuseaddr()
{
#if defined(SO_REUSEADDR)
	int opt = 1;
	if (setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0)
		return false;
#endif
	return true;	
}

bool Socket::SetSndbuf()
{
#if defined(SO_SNDBUF)
	int opt = (12 * 1024);
	if (setsockopt(m_Socket, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof(opt)) < 0)
		return false;
#endif
	return true;
}
	
bool Socket::SetLinger()
{
#if defined(SO_LINGER)
	struct linger ld;
	ld.l_onoff = 0;
	ld.l_linger = 0;
	if (setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld)) < 0)
		return false;
#endif
	return true;
}


bool Socket::SetNonblock()
{
#if defined(_WIN32)
	unsigned long flags = 1;
	return (ioctlsocket(m_Socket, FIONBIO, &flags) == 0);
#else
	int flags = fcntl(m_Socket, F_GETFL, 0);
	flags |= O_NONBLOCK;
	return (fcntl(m_Socket, F_SETFL, flags) >= 0);
#endif
}


void Socket::ProcessSockets_Poll()
{

	socket_t	highestSocket = INVALID_SOCKET;
	
	FD_ZERO(&ms_InputSet);
	FD_ZERO(&ms_OutputSet);
	FD_ZERO(&ms_ExceptionSet);
	
	FOREACH(SocketList, ms_SocketList, i)
	{
		Socket *		pSocket = *i;
		socket_t		socket = pSocket->GetSocket();
		SocketClient *	pClient = pSocket->GetClient();
		
		if (socket == INVALID_SOCKET || !pClient)
			continue;
		
		if (pClient->IsSocketInputEnabled())		FD_SET(socket, &ms_InputSet);
		if (pClient->IsSocketOutputEnabled())		FD_SET(socket, &ms_OutputSet);
		if (pClient->IsSocketExceptionEnabled())	FD_SET(socket, &ms_ExceptionSet);
		
		if (socket > highestSocket)
			highestSocket = socket;
	}
	
	static struct timeval nullTimeval = { 0, 0 };

	if (select(highestSocket + 1, &ms_InputSet, &ms_OutputSet, &ms_ExceptionSet, &nullTimeval) < 0)
	{
		log("SYSERR: NET: Socket::ProcessSockets(): Select failed.");
		perror("NET:");
		return;
	}
}


void Socket::ProcessSockets_Exceptions()
{
	FOREACH(SocketList, ms_SocketList, i)
	{
		Socket *	pSocket = *i;
		
		if (FD_ISSET(pSocket->GetSocket(), &ms_ExceptionSet))
			pSocket->GetClient()->OnSocketException();
	}
}


void Socket::ProcessSockets_Input()
{
	FOREACH(SocketList, ms_SocketList, i)
	{
		Socket *	pSocket = *i;
		
		if (FD_ISSET(pSocket->GetSocket(), &ms_InputSet))
			pSocket->GetClient()->OnSocketInput();
	}
}


void Socket::ProcessSockets_Update()
{
	FOREACH(SocketList, ms_SocketList, i)
	{
		Socket *	pSocket = *i;
		
		pSocket->GetClient()->OnSocketUpdate();
	}
}


void Socket::ProcessSockets_Output()
{
	FOREACH(SocketList, ms_SocketList, i)
	{
		Socket *	pSocket = *i;
		
		if (FD_ISSET(pSocket->GetSocket(), &ms_OutputSet))
			pSocket->GetClient()->OnSocketOutput();
	}
}



void Socket::ProcessSockets()
{
	Socket::ProcessSockets_Poll();
	Socket::ProcessSockets_Exceptions();
	Socket::ProcessSockets_Input();
	Socket::ProcessSockets_Update();
	Socket::ProcessSockets_Output();
}
