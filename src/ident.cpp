
#include "types.h"

#if !defined(_WIN32)
# include <sys/socket.h>
# include <netdb.h>
# include <netinet/in.h>
# include <arpa/inet.h>
#else
# include <winsock2.h>
# define ECONNREFUSED WSAECONNREFUSED
#endif

#include "ident.h"
#include "ban.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "characters.h"
#include "constants.h"

extern bool			fCopyOver;
extern int		mother_desc[2];
extern int		port;
extern bool			no_external_procs;

static void CopyoverRecover(void);


IdentServer::IdentServer(void) : descriptor(-1), child(-1), pid(-1), port(113) {
#if defined(CIRCLE_UNIX)
	int	fds[2];
#endif
	
	this->lookup.host = true;
	this->lookup.user = false;
	
	if (no_external_procs)
		return;		//	No external processes.
	
#if defined(CIRCLE_UNIX)
	if (socketpair(AF_UNIX, SOCK_DGRAM, 0, fds) < 0)
	{
		perror("IdentServer::IdentServer: socketpair");
		return;
	}
	
	this->descriptor = fds[0];
	this->child = fds[1];
	this->parent = getpid();
	
	if ((this->pid = fork()) < 0) {
		perror("Error creating Ident process");
	} else if (this->pid > 0) {
		log("Starting Ident Server.");
		close(this->child);
	} else {
		close(this->descriptor);
		if (fCopyOver)
		{
			CopyoverRecover();
		}
		this->Loop();
		exit(0);
	}
#endif
}


IdentServer::~IdentServer(void) {
#if defined(CIRCLE_UNIX)
	if (this->pid > 0) {
		log("Terminating Ident Server.");
		kill(this->pid, SIGKILL);
		close(this->descriptor);
	}
#endif
}


void IdentServer::Receive(void) {
#if defined(CIRCLE_UNIX)
	Message				msg;
	
	read(this->descriptor, (char *)&msg, sizeof(Message));
	
	switch (msg.type) {
		case Message::Nop:
		case Message::Error:
			break;
		case Message::IdRep:
			FOREACH(DescriptorList, descriptor_list, iter)
			{
				DescriptorData *d = *iter;
				if (d->m_Socket == msg.fd)
				{
					d->m_Host.clear();
					if (*msg.user)
					{
						d->m_Username = msg.user;
						d->m_Host = d->m_Username + "@";
					}
					if (*msg.host)
					{
						d->m_Hostname = msg.host;
						d->m_Host += d->m_Hostname;
					}
					else
						d->m_Host += d->m_HostIP;

					//	determine if the site is banned
					if (Ban::IsBanned(d->m_Host) == BAN_ALL)
					{
						d->Write("Sorry, this site is banned.\n");
						mudlogf(CMP , LVL_STAFF, TRUE,  "Connection attempt denied from [%s]", d->m_Host.c_str());
						d->GetState()->Push(new DisconnectConnectionState);
					}
					else if (typeid(*d->GetState()) == typeid(IdentConnectionState))
					{
						//	Log new connections
						//mudlogf(CMP, LVL_STAFF, TRUE, "New connection from [%s]", d->m_Host.c_str());
						//d->GetState()->Swap(new GetNameConnectionState);
						d->GetState()->Pop();
					}
					return;
				}
			}
			//	No more logging here - it means they dropped connection before
			//	domain name resolution.
//			log("SYSERR: IdentServer lookup for descriptor not connected: %d", msg.fd);
			break;
		default:
			log("SYSERR: IdentServer::Receive: msg.type = %d", msg.type);
	}
#endif
}


void IdentServer::Lookup(DescriptorData *d)
{
	if (no_external_procs)
	{
		char *hostname = LookupHost(d->m_SocketAddress);
		if (hostname)
		{
			d->m_Host = d->m_Hostname = hostname;
		}
		if (Ban::IsBanned(d->m_Host) == BAN_ALL) {
			d->Write("Sorry, this site is banned.\n");
			mudlogf(CMP , LVL_STAFF, TRUE,  "Connection attempt denied from [%s]", d->m_Host.c_str());
			d->GetState()->Push(new DisconnectConnectionState);
		} else {	//	Log new connections
			//mudlogf(CMP, LVL_STAFF, TRUE, "New connection from [%s]", d->m_Host.c_str());
			//d->GetState()->Swap(new GetNameConnectionState);
			d->GetState()->Pop();
		}
	}
#if defined(CIRCLE_UNIX)
	else if (this->pid > 0) {
		Message		msg;
		msg.type	= Message::Ident;
		msg.address	= d->m_SocketAddress;
		msg.fd		= d->m_Socket;
		write(this->descriptor, (const char *)&msg, sizeof(Message));
	} else {
		char *		str;
		if (this->lookup.host && (str = this->LookupHost(d->m_SocketAddress)))	d->m_Hostname = str;
		if (this->lookup.user && (str = this->LookupUser(d->m_SocketAddress)))	d->m_Username = str;

		d->m_Host.clear();
		if (!d->m_Username.empty())	d->m_Host = d->m_Username + "@";
		d->m_Host += !d->m_Hostname.empty() ? d->m_Hostname : d->m_HostIP;
		d->Write(login.c_str());
	}
#endif
}


void IdentServer::Loop(void) {
#if defined(CIRCLE_UNIX)
	int			n;
	fd_set			inp_set;
	Message			msg;
	char *			str;
	
	for (;;) {
		struct timeval tv;
		
		FD_ZERO(&inp_set);
		FD_SET(this->child, &inp_set);
		
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		
		if (select(this->child + 1, &inp_set, (fd_set *) NULL, (fd_set *) NULL, &tv) < 0) {
			perror("IdentServer::Loop(): select");
			continue;
		}
		
		if (FD_ISSET(this->child, &inp_set)) {
			n = read(this->child, (char *)&msg, sizeof(Message));
			//if (n != sizeof(Message))
			//	continue;
			memset(msg.host, 0, 256);
			memset(msg.user, 0, 256);
			
			switch (msg.type) {
				case Message::Nop:
					break;
				case Message::Quit:
					write(this->child, (char *)&msg, sizeof(Message));
					close(this->child);
					exit(0);
				case Message::Ident:
					if (this->lookup.host && (str = this->LookupHost(msg.address)))		strcpy(msg.host, str);
					if (this->lookup.user && (str = this->LookupUser(msg.address)))		strcpy(msg.host, str);
					msg.type = Message::IdRep;
					break;
				default:
					log("SYSERR: IdentServer::Loop(): msg.type = %d", msg.type);
					msg.type = Message::Error;
			}
			write(this->child, (char *)&msg, sizeof(Message));
		}
		
		//	Look at parent
		if (getppid() != this->parent)
			break;
	}
#endif
}


char *IdentServer::LookupHost(struct sockaddr_in sa) {
	static char			hostname[256];
	struct hostent *	hent = NULL;
	
	if (!(hent = gethostbyaddr((char *)&sa.sin_addr, sizeof(sa.sin_addr), AF_INET))) {
		perror("IdentServer::LookupHost(): gethostbyaddr");
		return NULL;
	}
	strncpy(hostname, hent->h_name, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	return hostname;
}


char *IdentServer::LookupUser(struct sockaddr_in sa) {
	static char			username[256];
	struct hostent *	hent;
	struct sockaddr_in	saddr;
	int				sock;
	
	BUFFER(string, MAX_INPUT_LENGTH);

	sprintf(string, "%d, %d\n", ntohs(sa.sin_port), port);
	
	if (!(hent = gethostbyaddr((char *) &sa.sin_addr, sizeof(sa.sin_addr), AF_INET)))
		perror("IdentServer::LookupUser(): gethostbyaddr");
	else {
		char *	result;
		saddr.sin_family	= hent->h_addrtype;
		saddr.sin_port		= htons(this->port);
		memcpy(&saddr.sin_addr, hent->h_addr, hent->h_length);
		
		if ((sock = socket(hent->h_addrtype, SOCK_STREAM, 0)) < 0) {
			perror("IdentServer::LookupUser(): socket");
			return NULL;
		}
		
		if (connect(sock, (struct sockaddr *) &saddr, sizeof(saddr)) < 0) {
			if (errno != ECONNREFUSED)
				perror("IdentServer::LookupUser(): connect");
			close(sock);
			return NULL;
		}
		
		write(sock, string, strlen(string));
		
		read(sock, string, 256);
		
		close(sock);
		
		int	sport, cport;
		BUFFER(mtype, MAX_INPUT_LENGTH);
		BUFFER(otype, MAX_INPUT_LENGTH);
		
		sscanf(string, " %d , %d : %s : %s : %s ", &sport, &cport, mtype, otype, username);
		
		result = (!strcmp(mtype, "USERID") ? username : NULL);
		
		return result;
	}
	return NULL;
}


/* Reload players after a copyover */
void CopyoverRecover(void)
{
	FILE *fp;
	int desc;

	close(mother_desc[0]);
	close(mother_desc[1]);
	
	if (!(fp = fopen (IDENTCOPYOVER_FILE, "r"))) { /* there are some descriptors open which will hang forever then ? */
		return;
	}

	remove (IDENTCOPYOVER_FILE); /* In case something crashes - doesn't prevent reading  */

	for (;;) {
		fscanf (fp, "%d\n", &desc);
		if (desc == -1)
			break;

		close (desc);
	}

	fclose (fp);
}
