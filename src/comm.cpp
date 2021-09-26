/***************************************************************************\
[*]    __     __  ___                ___  | LexiMUD is a C++ MUD codebase [*]
[*]   / /  ___\ \/ (_) /\/\  /\ /\  /   \ |        for Sci-Fi MUDs        [*]
[*]  / /  / _ \\  /| |/    \/ / \ \/ /\ / | Copyright(C) 1997-2005        [*]
[*] / /__|  __//  \| / /\/\ \ \_/ / /_//  | All rights reserved           [*]
[*] \____/\___/_/\_\_\/    \/\___/___,'   | Chris Jacobson (FearItself)   [*]
[*]      A Creation of the AvP Team!      | fearitself@avpmud.com         [*]
[-]---------------------------------------+-------------------------------[-]
[*] Based on CircleMUD, Copyright 1993-94, and DikuMUD, Copyright 1990-91 [*]
[-]-----------------------------------------------------------------------[-]
[*] File : comm.cp                                                        [*]
[*] Usage: Communication, socket handling, main(), central game loop      [*]
\***************************************************************************/

#define __COMM_C__

#include "types.h"


#include <stdarg.h>
#include <time.h>

#ifdef _WIN32				/* Includes for Win32 */
# include <windows.h>
# include <direct.h>
# include <mmsystem.h>
#endif

#include <fcntl.h>

#ifdef CIRCLE_UNIX
# include <arpa/inet.h>
# include <sys/ioctl.h>
# include <sys/socket.h>
# include <sys/resource.h>
# include <netinet/in.h>
# include <netdb.h>
# include <sys/wait.h>
#endif


#include "structs.h"
#include "db.h"
#include "utils.h"
#include "buffer.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "find.h"
#include "house.h"
#include "ban.h"
#include "olc.h"
#include "event.h"
#include "ident.h"
//#include "space.h"
#include "constants.h"
#include "mail.h"

#include "imc/imc.h"
//#include "imc-mercbase.h"
//#include "icec-mercbase.h"

#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#else
#include "telnet.h"
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

/* externs */
extern int circle_restrict;
bool no_rent_check = false;
extern int DFLT_PORT;
extern char *DFLT_DIR;
extern int MAX_PLAYERS;

bool no_external_procs = false;

extern struct TimeInfoData time_info;		/* In db.c */


/* local globals */
extern txt_q	bufpool;		/* pool of large output buffers */
int buf_largecount = 0;		/* # of large buffers which exist */
int buf_overflows = 0;		/* # of overflows of output */
int buf_switches = 0;		/* # of switches from small to large buf */
int circle_shutdown = 0;	/* clean shutdown */
int circle_reboot = 0;		/* reboot the game after a shutdown */
int no_specials = 0;		/* Suppress ass. of special routines */
int max_players = 0;		/* max descriptors available */
int tics = 0;			/* for extern checkpointing */
extern int nameserver_is_slow;	/* see config.c */
extern int auto_save;		/* see config.c */
extern unsigned int autosave_time;	/* see config.c */
struct timeval null_time;	/* zero-valued time structure */
FILE *logfile = stderr;
extern char *LOGFILE;

bool			fCopyOver;			//	Are we booting in copyover mode?
int				mother_desc;		//	Now a global
int				port;

IdentServer	*	Ident;

/* functions in this file */
void init_game(int port);
void signal_setup(void);
void game_loop(int mother_desc);
int init_socket(int port);
int new_descriptor(int s);
int get_max_players(void);
struct timeval timediff(struct timeval a, struct timeval b);
struct timeval timeadd(struct timeval a, struct timeval b);
void nonblock(socket_t s);
int perform_alias(DescriptorData *d, char *orig);
void record_usage(void);
char *make_prompt(DescriptorData *point);
void check_idle_passwords(void);
void heartbeat(int pulse);
void copyover_recover(void);
int set_sendbuf(socket_t s);
void sub_write_to_char(CharData *ch, char *tokens[], Ptr otokens[], char type[]);
void ProcessColor(const char *src, char *dest, size_t len, bool bColor);
void proc_color(const char *inbuf, char *outbuf, int lengthRemaining, bool useColor);
void prompt_str(CharData *ch, char *buf, int length);
void	perform_act(const char *orig, CharData *ch, ObjData *obj,
		    const void *vict_obj, CharData *to, int type, char *storeBuf);


RETSIGTYPE checkpointing(int);
RETSIGTYPE unrestrict_game(int);
RETSIGTYPE hupsig(int);
RETSIGTYPE reap(int sig);

#if defined(_WIN32)
void gettimeofday(struct timeval *t, struct timezone *dummy);
#endif

/* extern fcnts */
void boot_db(void);
void boot_world(void);
void zone_update(void);
void space_update(void);
void public_update(void);
void recharge_ships(void);
void move_ships(void);
void point_update(void);		//	In limits.c
void hour_update(void);			//	In limits.c
void free_purged_lists();		//	In handler.c
void check_mobile_activity(unsigned int pulse);
void perform_violence(void);
void weather_and_time(int mode);
void act_mtrigger(CharData *ch, const char *str, CharData *actor, CharData *victim, ObjData *object, ObjData *target, const char *arg);
void act_wtrigger(RoomData *room, const char *str, CharData *actor, ObjData *object);
void script_trigger_check(void);


UInt32 pulse;


/********************************************************************
*	main game loop and related stuff								*
*********************************************************************/

/* Windows doesn't have gettimeofday, so we'll simulate it. */
#if defined(_WIN32)
void gettimeofday(struct timeval *t, struct timezone *dummy)
{
	//	Coarse simulation - doesnt really return useful time, but good for deltas
	DWORD millisec = GetTickCount();

	t->tv_sec = (int) (millisec / 1000);
	t->tv_usec = (millisec % 1000) * 1000;

	//	More accurate simulation	
//	union { 
//		LONGLONG ns100; /*time since 1 Jan 1601 in 100ns units */ 
//		FILETIME ft; 
//	} now; 


//	GetSystemTimeAsFileTime (&now.ft); 
//	t->tv_usec = (long) ((now.ns100 / 10LL) % 1000000LL); 
//	t->tv_sec = (long) ((now.ns100 - 116444736000000000LL) / 10000000LL); 
}

/* The Mac doesn't have gettimeofday either. */
#endif


void enter_player_game(CharData *ch);

/* Reload players after a copyover */
void copyover_recover(void) {
	BUFFER(name, MAX_INPUT_LENGTH);
	BUFFER(host, 1024);

	DescriptorData *d;
	FILE *fp;
	int desc, compressionMode;
	int fOld;

	log ("Copyover recovery initiated");

	if (!(fp = fopen (COPYOVER_FILE, "r"))) { /* there are some descriptors open which will hang forever then ? */
		perror ("copyover_recover:fopen");
		log ("Copyover file not found. Exitting.\n");
		exit (1);
	}

	remove (COPYOVER_FILE); /* In case something crashes - doesn't prevent reading  */

	for (;;) {
		fOld = TRUE;
		fscanf (fp, "%d %d %s %s\n", &desc, &compressionMode, name, host);
		if (desc == -1)
			break;

		/* Write something, and check if it goes error-free */
		if (DescriptorData::Send(desc, "\nRestoring from copyover...\n") < 0)
		{
			close (desc); /* nope */
			continue;
		}

		d = new DescriptorData(desc);
		
		d->m_Host = host;
		descriptor_list.push_front(d);

		/* Now, find the pfile */

		d->m_Character = new CharData();
		d->m_Character->desc = d;

		if (!d->m_Character->Load(name) || (PLR_FLAGGED(d->m_Character, PLR_DELETED) && !PLR_FLAGGED(d->m_Character, PLR_NODELETE)))
		{
			fOld = FALSE;
		}
		else
		{
			REMOVE_BIT(PLR_FLAGS(d->m_Character),PLR_WRITING | PLR_MAILING);
		}

		if (!fOld) /* Player file not found?! */
		{
			d->Send("\nSomehow, your character was lost in the copyover. Sorry.\n");
			d->GetState()->Push(new DisconnectConnectionState);
		}
		else
		{ /* ok! */
			d->Write("\nCopyover recovery complete.\n");
			enter_player_game(d->m_Character);
			d->GetState()->Push(new PlayingConnectionState);
			look_at_room(d->m_Character);
		}
	}

	fclose (fp);
}


extern void TestParser();
extern void TestNumericSort();
extern void RunStringCRC32Test();
extern void test();

extern void InitializeFloatingEffectiveTiers();


int main(int argc, char **argv) {
	int pos = 1;
	char *dir;
	
	//	Run-time safety checks!
	assert(sizeof(Lexi::String) == Lexi::String::buffered_size);
	
	Buffer::Init();
	
#if defined(_WIN32) && 0
//	TestNumericSort();
//	TestParser();

	{
		BUFFER(buf, MAX_STRING_LENGTH);
/*		strcpy(buf, "10392.0000");			fix_float(buf);		printf("Test: %s\n", buf);
		strcpy(buf, "0.0000");				fix_float(buf);		printf("Test: %s\n", buf);
		strcpy(buf, "0.000000003448");		fix_float(buf);		printf("Test: %s\n", buf);
		strcpy(buf, "0.0490000004943");		fix_float(buf);		printf("Test: %s\n", buf);
		strcpy(buf, "239823.0930930039");	fix_float(buf);		printf("Test: %s\n", buf);
*/
		extern void ScriptEnginePrintDouble(char *result, double d);
		ScriptEnginePrintDouble(buf, 10392.0000);			printf("Test: %s\n", buf);
		ScriptEnginePrintDouble(buf, 0.0000);				printf("Test: %s\n", buf);
		ScriptEnginePrintDouble(buf, 0.000000003448);		printf("Test: %s\n", buf);
		ScriptEnginePrintDouble(buf, 0.0490000004943);		printf("Test: %s\n", buf);
		ScriptEnginePrintDouble(buf, 239823.0930930039);	printf("Test: %s\n", buf);
	}
	
#endif

	port = DFLT_PORT;
	dir = DFLT_DIR;

	/* Be nice to make this a command line option but the parser uses log() */
	if (*LOGFILE != '\0' && !(logfile = fopen(LOGFILE, "w"))) {
		fprintf(stdout, "Error opening log file!\n");
		exit(1);
	}

	while ((pos < argc) && (*(argv[pos]) == '-')) {
		switch (*(argv[pos] + 1)) {
			case 'C': /* -C<socket number> - recover from copyover, this is the control socket */
				fCopyOver = TRUE;
				mother_desc = atoi(argv[pos]+2);
				no_rent_check = true;
				break;
			case 'd':
				if (*(argv[pos] + 2))		dir = argv[pos] + 2;
				else if (++pos < argc)		dir = argv[pos];
				else {
					log("SYSERR: Directory arg expected after option -d.");
					exit(1);
				}
				break;
			case 'q':
				no_rent_check = true;
				log("Quick boot mode -- rent check supressed.");
				break;
			case 'r':
				circle_restrict = 1;
				log("Restricting game -- no new players allowed.");
				break;
			case 's':
				no_specials = 1;
				log("Suppressing assignment of special routines.");
				break;
			case 'v':
				if (*(argv[pos] + 2))		Buffer::options = atoi(argv[pos] + 2);
				else if (++pos < argc)		Buffer::options = atoi(argv[pos]);
				else {
					log("SYSERR: Number expected after option -v.");
					exit(1);
				}
			case 'x':
				no_external_procs = true;
				nameserver_is_slow = true;
				break;
			default:
				log("SYSERR: Unknown option -%c in argument string.", *(argv[pos] + 1));
				break;
		}
	pos++;
	}
	
#if defined(_WIN32)
	no_external_procs = true;
#endif

	if (pos < argc) {
		if (!isdigit(*argv[pos])) {
			log("Usage: %s [-c] [-m] [-q] [-r] [-s] [-x] [-v val] [-d pathname] [port #]", argv[0]);
			exit(1);
		} else if ((port = atoi(argv[pos])) <= 1024) {
			log("SYSERR: Illegal port number.");
			exit(1);
		}
	}
	if (chdir(dir) < 0) {
		perror("SYSERR: Fatal error changing to data directory");
		exit(1);
	}
	log("Using %s as data directory.", dir);

	Ident = new IdentServer();
	
	log("Running game on port %d.", port);
	init_game(port);
	
	log("Shutting down Ident Server");
	delete Ident;
	
	Buffer::Exit();

	return 0;
}

//	Init sockets, run game, and cleanup sockets
void init_game(int port) {
	circle_srandom(time(0));

	log("Finding player limit.");
	max_players = get_max_players();

	if (!fCopyOver) {	//	If copyover mother_desc is already set up
		log("Opening mother connection.");
		mother_desc = init_socket(port);
	}

//	InitEvents();
	
	boot_db();

	log("Signal trapping.");
	signal_setup();

//	IMC::Startup(IMC_DIR);
//	ICE::Init();
	imc_startup( FALSE, -1, FALSE );
	
	if (fCopyOver) /* reload players */
		copyover_recover();

	log("Entering game loop.");

	game_loop(mother_desc);

	SavePersistentObjects();
	House::SaveAllHousesContents();
	Clan::Save();
	
//	IMC::Shutdown();
	imc_shutdown(FALSE);

	log("Closing all sockets.");
	while (descriptor_list.front())
		descriptor_list.front()->Close();

	CLOSE_SOCKET(mother_desc);
	
/*
	if (circle_reboot != 2 && olc_save_list) { // Don't save zones.
		struct olc_save_info *entry, *next_entry;
		for (entry = olc_save_list; entry; entry = next_entry) {
			next_entry = entry->next;
			if (entry->type < 0 || entry->type > 4)
				log("OLC: Illegal save type %d!", entry->type);
			else if (entry->zone < 0)
				log("OLC: Illegal save zone %d!", entry->zone);
			else {
				log("OLC: Reboot saving %s for zone %d.",
						save_info_msg[(int)entry->type], entry->zone);
				switch (entry->type) {
					case OLC_SAVE_ROOM:		REdit::save_to_disk(entry->zone); break;
					case OLC_SAVE_OBJ:		oedit_save_to_disk(entry->zone); break;
					case OLC_SAVE_MOB:		medit_save_to_disk(entry->zone); break;
					case OLC_SAVE_ZONE:		zedit_save_to_disk(entry->zone); break;
					case OLC_SAVE_SHOP:		sedit_save_to_disk(entry->zone); break;
					case OLC_SAVE_ACTION:	aedit_save_to_disk(entry->zone); break;
					case OLC_SAVE_HELP:		hedit_save_to_disk(entry->zone); break;
					default:				log("Unexpected olc_save_list->type"); break;
				}
			}
		}
	}
*/

	if (circle_reboot) {
		log("Rebooting.");
		exit(52);			/* what's so great about HHGTTG, anyhow? */
	}
	log("Normal termination of game.");
}



/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
int init_socket(int port) {
	int s;
	int opt;
	struct sockaddr_in sa;
	
  /*
   * Should the first argument to socket() be AF_INET or PF_INET?  I don't
   * know, take your pick.  PF_INET seems to be more widely adopted, and
   * Comer (_Internetworking with TCP/IP_) even makes a point to say that
   * people erroneously use AF_INET with socket() when they should be using
   * PF_INET.  However, the man pages of some systems indicate that AF_INET
   * is correct; some such as ConvexOS even say that you can use either one.
   * All implementations I've seen define AF_INET and PF_INET to be the same
   * number anyway, so ths point is (hopefully) moot.
   */

#ifdef _WIN32
	{
		WORD wVersionRequested;
		WSADATA wsaData;

		wVersionRequested = MAKEWORD(1, 1);

		if (WSAStartup(wVersionRequested, &wsaData) != 0) {
			log("WinSock not available!");
			exit(1);
		}
		if ((wsaData.iMaxSockets - 4) < max_players) {
			max_players = wsaData.iMaxSockets - 4;
		}
		log("Max players set to %d", max_players);

		if ((s = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
			log("Error opening network connection: Winsock err #%d", WSAGetLastError());
			exit(1);
		}
	}
#else
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error creating socket");
		exit(1);
	}
#endif				/* _WIN32 */


# if defined(SO_REUSEADDR)
	opt = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
		perror("SYSERR: setsockopt REUSEADDR");
		exit(1);
	}
# endif

	set_sendbuf(s);

# if defined(SO_LINGER)
	{
		struct linger ld;

		ld.l_onoff = 0;
		ld.l_linger = 0;
		if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld)) < 0) {
			perror("SYSERR: setsockopt LINGER");
			exit(1);
		}
	}
# endif

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
#if defined(_WIN32) || 1
	sa.sin_addr.s_addr = INADDR_ANY;
#else
	sa.sin_addr.s_addr = inet_addr("70.85.16.15");
#endif

	if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		perror("SYSERR: bind");
		CLOSE_SOCKET(s);
		exit(1);
	}
	nonblock(s);
	listen(s, 5);
	
	return s;
}


int get_max_players(void) {
	int max_descs = 0;
	char *method;

/*
 * First, we'll try using getrlimit/setrlimit.  This will probably work
 * on most systems.
 */
#if defined (RLIMIT_NOFILE) || defined (RLIMIT_OFILE)
#if !defined(RLIMIT_NOFILE)
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif
	{
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
			max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;
		else
#endif
			max_descs = MIN(MAX_PLAYERS + NUM_RESERVED_DESCS, limit.rlim_max);
	}

#elif defined (OPEN_MAX) || defined(FOPEN_MAX)
#if !defined(OPEN_MAX)
#define OPEN_MAX FOPEN_MAX
#endif
	method = "OPEN_MAX";
	max_descs = OPEN_MAX;		/* Uh oh.. rlimit didn't work, but we have
								 * OPEN_MAX */
#elif defined (POSIX)
	/*
	 * Okay, you don't have getrlimit() and you don't have OPEN_MAX.  Time to
	 * use the POSIX sysconf() function.  (See Stevens' _Advanced Programming
	 * in the UNIX Environment_).
	 */
	method = "POSIX sysconf";
	errno = 0;
	if ((max_descs = sysconf(_SC_OPEN_MAX)) < 0) {
		if (errno == 0)
			max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;
		else {
			perror("SYSERR: Error calling sysconf");
			exit(1);
		}
	}
#else
	/* if everything has failed, we'll just take a guess */
	max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;
#endif

	/* now calculate max _players_ based on max descs */
	max_descs = MIN(MAX_PLAYERS, max_descs - NUM_RESERVED_DESCS);

	if (max_descs <= 0) {
		log("SYSERR: Non-positive max player limit!  (Set at %d using %s).", max_descs, method);
		exit(1);
	}
	log("Setting player limit to %d using %s.", max_descs, method);
	return max_descs;
}



/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
void game_loop(int mother_desc) {
	fd_set input_set, output_set, exc_set, null_set;
	struct timeval last_time, before_sleep, opt_time, process_time, now, timeout;
	int missed_pulses, maxdesc;
  
	/* initialize various time values */
	null_time.tv_sec = 0;
	null_time.tv_usec = 0;
	opt_time.tv_usec = OPT_USEC;
	opt_time.tv_sec = 0;
	FD_ZERO(&null_set);

	gettimeofday(&last_time, (struct timezone *) 0);
	
  /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
	while (!circle_shutdown) {
/* Sleep if we don't have any connections */
		if (descriptor_list.empty())
		{
//			log("No connections.  Going to sleep.");
//			Removed to prevent log spam, because of IMC
			FD_ZERO(&input_set);
			FD_ZERO(&output_set);	//	IMC
			FD_ZERO(&exc_set);		//	IMC
			FD_SET(mother_desc, &input_set);
			
//			maxdesc = IMC::fill_fdsets(mother_desc, &input_set, &output_set, &exc_set);
			maxdesc = mother_desc;
			if (this_imcmud && this_imcmud->desc > 0)
			{
				FD_SET(this_imcmud->desc, &input_set);
				maxdesc = MAX(maxdesc, this_imcmud->desc);	
			}
			
			if (select(maxdesc + 1, &input_set, &output_set, &exc_set, NULL) < 0) {
#if defined(_WIN32)
				if (WSAGetLastError() == WSAEINTR)
#else
				if (errno == EINTR)
#endif
					log("Waking up to process signal.");
				else				perror("Select coma");
			}// else
			//	log("New connection.  Waking up.");
			gettimeofday(&last_time, (struct timezone *) 0);
		}
/* Set up the input, output, and exception sets for select(). */
		FD_ZERO(&input_set);
		FD_ZERO(&output_set);
		FD_ZERO(&exc_set);
		FD_SET(mother_desc, &input_set);

		maxdesc = mother_desc;		

#if 1	//	ASYNCH
		if (Ident->IsActive()) {
			FD_SET(Ident->descriptor, &input_set);
			maxdesc = MAX(maxdesc, Ident->descriptor);
		}
#endif
		
	    FOREACH(DescriptorList, descriptor_list, iter)
	    {
	    	DescriptorData *d = *iter;
	    	
			if (d->m_Socket > maxdesc)
				maxdesc = d->m_Socket;
			FD_SET(d->m_Socket, &input_set);
			FD_SET(d->m_Socket, &output_set);
			FD_SET(d->m_Socket, &exc_set);
		}
		
		//maxdesc = IMC::fill_fdsets(maxdesc, &input_set, &output_set, &exc_set);
		
		/*
		 * At this point, we have completed all input, output and heartbeat
		 * activity from the previous iteration, so we have to put ourselves
		 * to sleep until the next 0.1 second tick.  The first step is to
		 * calculate how long we took processing the previous iteration.
		 */

		gettimeofday(&before_sleep, (struct timezone *) 0); /* current time */
		process_time = timediff(before_sleep, last_time);

		/*
		 * If we were asleep for more than one pass, count missed pulses and sleep
		 * until we're resynchronized with the next upcoming pulse.
		 */
		if (process_time.tv_sec == 0 && process_time.tv_usec < OPT_USEC) {
			missed_pulses = 0;
		} else {
			missed_pulses = process_time.tv_sec RL_SEC;
			missed_pulses += process_time.tv_usec / OPT_USEC;
			process_time.tv_sec = 0;
			process_time.tv_usec = process_time.tv_usec % OPT_USEC;
		}

		/* Calculate the time we should wake up */
		last_time = timeadd(before_sleep, timediff(opt_time, process_time));

		/* Now keep sleeping until that time has come */
		gettimeofday(&now, (struct timezone *) 0);
		timeout = timediff(last_time, now);

		/* Go to sleep */
		do {
#ifdef _WIN32
			Sleep(timeout.tv_sec * 1000 + timeout.tv_usec / 1000);
#else
			if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout) < 0) {
				if (errno != EINTR)
				{
					perror("SYSERR: Select sleep");
					exit(1);
				}
			}
#endif /* _WIN32 */
			gettimeofday(&now, (struct timezone *) 0);
			timeout = timediff(last_time, now);
		} while (timeout.tv_usec || timeout.tv_sec);

		/* Poll (without blocking) for new input, output, and exceptions */
		if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time) < 0) {
			perror("Select poll");
			return;
		}
		
		//	Accept new connections
		if (FD_ISSET(mother_desc, &input_set))
			new_descriptor(mother_desc);
		

#if 1	//	ASYNCH
		//	Check for completed lookups
		if (Ident->IsActive() && FD_ISSET(Ident->descriptor, &input_set))
			Ident->Receive();
#endif
		
		//	Kick out the freaky folks in the exception set
	    FOREACH(DescriptorList, descriptor_list, iter)
	    {
	    	DescriptorData *d = *iter;
			if (FD_ISSET(d->m_Socket, &exc_set)) {
				FD_CLR(d->m_Socket, &input_set);
				FD_CLR(d->m_Socket, &output_set);
				d->Close();
			}
		}
		
		//	Process descriptors with input pending
	    FOREACH(DescriptorList, descriptor_list, iter)
	    {
	    	DescriptorData *d = *iter;
			if (FD_ISSET(d->m_Socket, &input_set))
				if (d->ProcessInput() < 0)
					d->Close();
		}
		
		/* Process commands we just read from process_input */
		{
			
		BUFFER(comm, MAX_STRING_LENGTH * 2);
	    FOREACH(DescriptorList, descriptor_list, iter)
	    {
	    	DescriptorData *d = *iter;
			bool	extraInput = false;
			bool aliased = false;
//moreinput:
			
			do
			{
				bool bMailCheck = false;
				
				d->wait -= (d->wait > 0);
				if (d->m_Character) d->m_Character->move_wait -= (d->m_Character->move_wait > 0);
				if (d->wait > 0)							break;//continue;
				if (!d->m_InputQueue.get(comm, aliased))	break;//continue;
				if (d->m_Character)
				{
				//	Reset the idle timer & pull char back from void if necessary
					bMailCheck = d->m_Character->GetPlayer()->m_IdleTime >= 2;
					
					d->m_Character->GetPlayer()->m_IdleTime = 0;
					if (d->GetState()->IsPlaying() && d->m_Character->WasInRoom()) {
						if (d->m_Character->InRoom())
							d->m_Character->FromRoom();
						d->m_Character->ToRoom(d->m_Character->WasInRoom());
						d->m_Character->SetWasInRoom(NULL);
						act("$n has returned.", TRUE, d->m_Character, 0, 0, TO_ROOM);
					}
				}
				d->wait = 1;
				d->m_PromptState = DescriptorData::WantsPrompt;

				if (d->m_Pager)
					d->m_Pager->Parse(d, comm);	//	Reading something w/ pager
				else if (d->m_StringEditor) 
				{
					if (extraInput)
						d->Write(make_prompt(d));
					d->m_StringEditor->Parse(d, comm);	//	Writing boards, mail, etc.
				}
				else if (!d->GetState()->IsPlaying())	d->GetState()->Parse(comm);	//	in menus, etc.
				else
				{														//	We're playing normally
					if (aliased)	d->m_PromptState = DescriptorData::HasPrompt;	//	to prevent recursive aliases
					else if (!d->m_Character->m_ScriptPrompt.empty())
						;	//	Prevent alias system from executing
					else if (perform_alias(d, comm))			//	run it through aliasing system
						d->m_InputQueue.get(comm, aliased);
					//command_interpreter(d->m_Character, comm);	//	send it to interpreter
					d->GetState()->Parse(comm);
					
					//	Check for mail
					if (bMailCheck && has_mail(d->m_Character->GetID()))
						d->Write("You have mail waiting.\n");
				}
				
//				if (d->showstr_vector.empty() && d->str)
//				{
//					extraInput = true;
//					goto moreinput;
//				}

				extraInput = (!d->m_Pager && d->m_StringEditor);
			} while (extraInput);
		}
		
		}
		
		//	send queued output out to the operating system (ultimately to user)
	    FOREACH(DescriptorList, descriptor_list, iter)
	    {
	    	DescriptorData *d = *iter;
	    	
			// Can we write?
	    	if (!FD_ISSET(d->m_Socket, &output_set))
	    		continue;
	    	
			//	Is there output?
			if ( !d->m_CurrentOutputBuffer->m_Text.empty() )
			{
				/* Output for this player is ready */
				bool bReshowPrompt = d->m_bOutputOverflow;
				if (d->ProcessOutput() >= 0)	d->m_PromptState = bReshowPrompt ? DescriptorData::WantsPrompt : DescriptorData::HasPrompt;	//	Successfully wrote
				else							d->Close();	//	Error, close
			}
			else if ( d->m_PromptState == DescriptorData::WantsPrompt )
			{
				d->Write(make_prompt(d));
				d->m_PromptState = DescriptorData::PendingPrompt;
			}
		}
		
		//	Print prompts for other descriptors who had no other output
		//	Merged these together, also.  Seems safe to also do the ELSE, because
		//	if the previous IF is called, either d would be removed from list,
		//	or d->has_prompt would be set, therefore would be skipped during
		//	prompt making
/*		iter.Reset();
		while ((d = iter.Next())) {
			if (!d->has_prompt && (STATE(d) != CON_DISCONNECT)) {
				SEND_TO_Q(make_prompt(d), d);
				d->has_prompt = 2;
				if (d->ProcessOutput() < 0)	d->Close();
			}
		}
*/

	    FOREACH(DescriptorList, descriptor_list, iter)
	    {
	    	DescriptorData *d = *iter;
			d->GetState()->Idle();
		}

		//	Now, we execute as many pulses as necessary--just one if we haven't
		//	missed any pulses, or make up for lost time if we missed a few
		//	pulses by sleeping for too long.
		missed_pulses++;

		if (missed_pulses <= 0) {
			log("SYSERR: **BAD** MISSED_PULSES IS NONPOSITIVE (%d), TIME GOING BACKWARDS!!!", missed_pulses / PASSES_PER_SEC);
			missed_pulses = 1;
		}

		/* If we missed more than 30 seconds worth of pulses, forget it */
		if (missed_pulses > (30 RL_SEC)) {
			log("SYSERR: Missed more than 30 seconds worth of pulses");
			missed_pulses = 30 RL_SEC;
		}

//		IMC::idle_select(&input_set, &output_set, &exc_set, last_time.tv_sec);
		imc_loop();
		
		/* Now execute the heartbeat functions */
		while (missed_pulses--)		heartbeat(++pulse);

#ifdef CIRCLE_UNIX
		/* Update tics for deadlock protection (UNIX only) */
		tics++;
#endif
	}
}


void heartbeat(int pulse) {
	static unsigned int mins_since_crashsave = 0;

	Event::ProcessEvents();
	
	if (!(pulse % PULSE_ZONE))					zone_update();
	if (!(pulse % (15 RL_SEC)))					check_idle_passwords();		/* 15 seconds */
	if (!(pulse % PULSE_MOBILE))				check_mobile_activity(pulse);
	if (!(pulse % PULSE_VIOLENCE))				perform_violence();
	if (!(pulse % PULSE_SCRIPT))				script_trigger_check();

//	if (!(pulse % (PULSE_ZONE / 4)))	space_update();
//	if (!(pulse % (15 * (60 RL_SEC))))	space_control_update();
//	if (!(pulse % (PULSE_ZONE / 3)))	recharge_ships();
//	if (!(pulse % (PULSE_ZONE / 6)))	move_ships();
	
	free_purged_lists();
	
	if (!(pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
		weather_and_time(1);
		hour_update();
//		reward_zone_captures();
	}
	if (!(pulse % PULSE_POINTS))				point_update();
	/* Clear out all the global buffers now in case someone forgot. */
	if (!(pulse % PULSE_BUFFER))				Buffer::ReleaseAll();
	
	if (auto_save && !(pulse % (60 RL_SEC))) {	/* 1 minute */
		SavePersistentObjects();
	//	if (++mins_since_crashsave >= autosave_time) {
	//		mins_since_crashsave = 0;
			Crash_save_all();
			House::SaveAllHousesContents();
			if (Clan::MarkedForSave())
				Clan::Save();
	//	}
	}
	
	if (!(pulse % (5 * 60 RL_SEC)))	record_usage();				/* 5 minutes */
}


/********************************************************************
*	general utility stuff (for local use)							*
*********************************************************************/

/*
 *	new code to calculate time differences, which works on systems
 *	for which tv_usec is unsigned (and thus comparisons for something
 *	being < 0 fail).  Based on code submitted by ss@sirocco.cup.hp.com.
 */

/*
 * code to return the time difference between a and b (a-b).
 * always returns a nonnegative value (floors at 0).
 */
struct timeval timediff(struct timeval a, struct timeval b) {
	struct timeval rslt;

	if (a.tv_sec < b.tv_sec)
		return null_time;
	else if (a.tv_sec == b.tv_sec) {
		if (a.tv_usec < b.tv_usec)
			return null_time;
		else {
			rslt.tv_sec = 0;
			rslt.tv_usec = a.tv_usec - b.tv_usec;
			return rslt;
		}
	} else {			/* a->tv_sec > b->tv_sec */
		rslt.tv_sec = a.tv_sec - b.tv_sec;
		if (a.tv_usec < b.tv_usec) {
			rslt.tv_usec = a.tv_usec + 1000000 - b.tv_usec;
			rslt.tv_sec--;
		} else
			rslt.tv_usec = a.tv_usec - b.tv_usec;
		return rslt;
	}
}


//	add 2 timevals
struct timeval timeadd(struct timeval a, struct timeval b) {
	struct timeval rslt;

	rslt.tv_sec = a.tv_sec + b.tv_sec;
	rslt.tv_usec = a.tv_usec + b.tv_usec;

	while (rslt.tv_usec >= 1000000) {
		rslt.tv_usec -= 1000000;
		rslt.tv_sec++;
	}

	return rslt;
}


void record_usage(void) {
	int sockets_playing = 0;

    FOREACH(DescriptorList, descriptor_list, iter)
		if ((*iter)->GetState()->IsPlaying())
			sockets_playing++;

	log("nusage: %-3d sockets connected, %-3d sockets playing",
			descriptor_list.size(), sockets_playing);

#ifdef RUSAGE
	{
		struct rusage ru;

		getrusage(0, &ru);
		log("rusage: user time: %d sec, system time: %d sec, max res size: %d",
				ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_maxrss);
	}
#endif
}


extern int level_cost(int level);
void prompt_str(CharData *ch, char *buf, int maxlength) {
	CharData *vict = FIGHTING(ch);  
	const char *str = ch->GetPlayer()->m_Prompt.c_str();
	CharData *tank;
	int perc;  
	char *cp, *tmp;
	int length;
  	
  	while (*buf)
  	{
  		++buf;
  		--maxlength;
  	}
  	
  	length = maxlength - 1;
  	
	if (!str || !*str)
		str = "`YAvP`K: `cSet your prompt (see `K'`Chelp prompt`K'`c)`W> `n";

	if (!strchr(str, '%'))
	{
		strncpy(buf, str, maxlength - 1);
		buf[maxlength - 1] = '\0';
		return;
  	}
  	
	BUFFER(i, 256);
  
	cp = buf;
  
	while (1)
	{
		if (*str == '%')
		{
			switch (*(++str))
			{
				case 'h': // current hitp
					sprintf(i, "%d", GET_HIT(ch));
					tmp = i;
					break;
				case 'H': // maximum hitp
					sprintf(i, "%d", GET_MAX_HIT(ch));
					tmp = i;
					break;
				case 'v': // current moves
					sprintf(i, "%d", GET_MOVE(ch));
					tmp = i;
					break;
				case 'V': // maximum moves
					sprintf(i, "%d", GET_MAX_MOVE(ch));
					tmp = i;
					break;
				/*
				case 'e': // current moves
					sprintf(i, "%d", GET_ENDURANCE(ch));
					tmp = i;
					break;
				case 'E': // maximum moves
					sprintf(i, "%d", GET_MAX_ENDURANCE(ch));
					tmp = i;
					break;
				*/
				case 'P':
					case 'p': // percentage of hitp/move/mana
					str++;
					switch (tolower(*str)) {
						case 'h':
							perc = (100 * GET_HIT(ch)) / GET_MAX_HIT(ch);
							break;
						case 'v':
							perc = (100 * GET_MOVE(ch)) / GET_MAX_MOVE(ch);
							break;
						//case 'e':
						//	perc = (100 * GET_ENDURANCE(ch)) / GET_MAX_ENDURANCE(ch);
						//	break;
//						case 'x':
//							perc = (100 * GET_EXP(ch)) / titles[(int)GET_CLASS(ch)][ch->GetLevel()+1].exp;
//							break;
						default :
							perc = 0;
							break;
					}
					sprintf(i, "%d%%", perc);
					tmp = i;
					break;
				case 'O':
				case 'o': // opponent
					if (vict) {
						perc = (100*GET_HIT(vict)) / GET_MAX_HIT(vict);
						sprintf(i, "%s `K(`r%s`K)`n", PERS(vict, ch),
								(perc >= 95 ?	"unscathed"	:
								perc >= 75 ?	"scratched"	:
								perc >= 50 ?	"beaten-up"	:
								perc >= 25 ?	"bloody"	:
												"near death"));
						tmp = i;
					} else {
						str++;
						continue;
					}
					break;
//				case 'x': // current exp
//					sprintf(i, "%d", GET_EXP(ch));
//					tmp = i;
//					break;
//				case 'X': // exp to level
//					sprintf(i, "%d", titles[(int)GET_CLASS(ch)][ch->GetLevel()+1].exp - GET_EXP(ch));
//					tmp = i;
//					break;
//				case 'g': // gold on hand
//					sprintf(i, "%d", GET_GOLD(ch));
//					tmp = i;
//					break;
//				case 'G': // gold in bank
//					sprintf(i, "%d", GET_BANK_GOLD(ch));
//					tmp = i;
//					break;
				case 'm':
					sprintf(i, "%d", GET_MISSION_POINTS(ch));
					tmp = i;
					break;
				case 'M':
					sprintf(i, "%d", level_cost(ch->GetLevel()) - ch->points.mp);
					tmp = i;
					break;
				case 'T':
				case 't': // tank
					if (vict && (tank = FIGHTING(vict)) && tank != ch) {
						perc = (100*GET_HIT(tank)) / GET_MAX_HIT(tank);
						sprintf(i, "%s `K(`r%s`K)`n", PERS(tank, ch),
								(perc >= 95 ?	"unscathed"	:
								perc >= 75 ?	"scratched"	:
								perc >= 50 ?	"beaten-up"	:
								perc >= 25 ?	"bloody"	:
												"near death"));
						tmp = i;
					} else {
						str++;
						continue;
					}
					break;
				case '_':
					tmp = "\n";
					break;
				case '%':
					*(cp++) = '%';
					str++;
					continue;
				default :
					str++;
					continue;
			}

			while ((*cp = *(tmp++)) && --length > 0)
				cp++;
			str++;
		}
		else if (!(*(cp++) = *(str++)) || --length <= 0)
			break;
	}
  
	*cp = '\0';
  
	if (length >= 4)
		strcat(buf, " `n");
	buf[maxlength - 1] = '\0';
}


char *make_prompt(DescriptorData *d) {
	static char prompt[2048];
	//	Note, prompt is truncated at MAX_PROMPT_LENGTH chars (structs.h )
	
	//	reversed these top 2 if checks so that page_string() would work in
	//	the editor */
	if (d->m_Pager)
	{
		sprintf(prompt, "\n`n[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (%d/%d) ]\n",
				d->m_Pager->GetPageNumber(), d->m_Pager->GetNumPages());
	}
	else if (d->m_StringEditor)
		strcpy(prompt, "] ");
	else if (d->GetState()->IsPlaying())
	{
		*prompt = '\0';

		if (d->m_Character && !d->m_Character->m_ScriptPrompt.empty())
		{
			strcat(prompt, d->m_Character->m_ScriptPrompt.c_str());
			if (*(END_OF(prompt) - 1) != ' ')
				strcat(prompt, " ");
		}
		else
		{
			if (IS_STAFF(d->m_Character))
			{
				extern std::list<IDNum> wizcallers;
				if (!wizcallers.empty())	sprintf_cat(prompt, "`r[%d pending wizcalls]`n\n", wizcallers.size());
			}
			
			if (!IS_NPC(d->m_Character) && !d->m_Character->GetPlayer()->m_AFKMessage.empty())
				strcat(prompt, "`C[AFK]`n");
			
			if (d->m_Character->GetPlayer()->m_StaffInvisLevel > 0)
				sprintf_cat(prompt, "`gi%d`n ", d->m_Character->GetPlayer()->m_StaffInvisLevel);
			
			static const Affect::Flags MAKE_BITSET(DisplayedFlags, AFF_INVISIBLE, AFF_INVISIBLE_2, AFF_HIDE, AFF_COVER, AFF_TRAITOR);
			if (AFF_FLAGS(d->m_Character).testForAny(DisplayedFlags)
				/* && !(GET_INVIS_LEV(d->m_Character)) */)
			{
				strcat(prompt, "(");
				if (AFF_FLAGGED(d->m_Character, AFF_INVISIBLE) | AFF_FLAGGED(d->m_Character, AFF_INVISIBLE_2))
					strcat(prompt, "`wi`n");
				if (AFF_FLAGGED(d->m_Character, AFF_HIDE))
					strcat(prompt, "`KH`n");
				if (AFF_FLAGGED(d->m_Character, AFF_COVER))
					strcat(prompt, "`KC`n");
				if (AFF_FLAGGED(d->m_Character, AFF_TRAITOR))
					strcat(prompt, "`RT`n");
				strcat(prompt, ") ");
			}
			
			prompt_str(d->m_Character, prompt, sizeof(prompt) / 4);
		}
		
		//proc_color(prompt, PRF_FLAGGED(d->m_Character, PRF_COLOR)); 
	} else
		*prompt = '\0';
	
	return prompt;
}


void txt_q::write(const char *text, bool aliased)
{
	push_back(new txt_block(text, aliased));
}



bool txt_q::get(char *dest, bool &aliased)
{
	if (empty())	return false;
	
	txt_block *block = front();

	strcpy(dest, block->m_Text.c_str());
	aliased = block->m_Aliased;

	delete block;
	
	pop_front();

	return true;
}


void txt_q::flush()
{
	while (!empty())
	{
		delete front();
		pop_front();
	}
}



/********************************************************************
*	socket handling													*
********************************************************************/



/* Sets the kernel's send buffer size for the descriptor */
int set_sendbuf(socket_t s) {
#if defined(SO_SNDBUF)
	int opt = MAX_SOCK_BUF;
	if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof(opt)) < 0) {
		perror("SYSERR: setsockopt SNDBUF");
		return -1;
	}
#endif
	return 0;
}


int new_descriptor(int s) {
	socket_t desc;
//	unsigned int	addr;
	socklen_t	i;

	DescriptorData *newd;
	struct sockaddr_in peer;
//	struct hostent *from;

	/* accept the new connection */
	i = sizeof(peer);
	if ((desc = accept(s, (struct sockaddr *) &peer, &i)) == INVALID_SOCKET)
	{
		perror("accept");
		return -1;
	}
	
	/* keep it from blocking */
	nonblock(desc);

	/* set the send buffer size if available on the system */
	if (set_sendbuf(desc) < 0) {
		CLOSE_SOCKET(desc);
		return 0;
	}

	/* make sure we have room for it */
	if (descriptor_list.size() >= max_players)
	{
		DescriptorData::Send(desc, "Sorry, Aliens vs Predator is full right now... please try again later!\n");
		CLOSE_SOCKET(desc);
		return 0;
	}
	
	/* create a new descriptor */
	if (DescriptorData::Send(desc, "Validating socket, looking up hostname.") < 0)
	{
		CLOSE_SOCKET(desc);
		return 0;
	}
	
	newd = new DescriptorData(desc);
	
#if 0	// ASYNCH OFF
	/* find the sitename */
	if (nameserver_is_slow || !(from = gethostbyaddr((char *) &peer.sin_addr,
			sizeof(peer.sin_addr), AF_INET))) {

		/* resolution failed */
		if (!nameserver_is_slow)
			perror("gethostbyaddr");

		/* find the numeric site address */
		addr = ntohl(peer.sin_addr.s_addr);
		sprintf(newd->host, "%03u.%03u.%03u.%03u", (int) ((addr & 0xFF000000) >> 24),
				(int) ((addr & 0x00FF0000) >> 16), (int) ((addr & 0x0000FF00) >> 8),
				(int) ((addr & 0x000000FF)));
	} else {
		strncpy(newd->host, from->h_name, HOST_LENGTH);
		*(newd->host + HOST_LENGTH) = '\0';
	}
#endif

	/* prepend to list */
	descriptor_list.push_front(newd);

	//	HACK: Make sure 'login' does not end with a newline!
	while (!login.empty() && login[login.length() - 1] == '\n')
	{
		login.erase(login.length() - 1);
	}
	if (!login.empty() && login[login.length() - 1] != ' ')
		login.append(" ");

#if 1	//	ASYNCH
	newd->m_SocketAddress = peer;
	newd->m_Host = newd->m_HostIP = inet_ntoa(newd->m_SocketAddress.sin_addr);
	
	newd->GetState()->Push(new GetNameConnectionState);
	if (newd->m_HostIP == "204.209.44.7")
	{
		newd->GetState()->Push(new OldAddressConnectionState);
	}
	newd->GetState()->Push(new IdentConnectionState);
	
	Ident->Lookup(newd);
#endif

	return 0;
}






void check_idle_passwords(void) {
    FOREACH(DescriptorList, descriptor_list, iter)
    {
    	DescriptorData *d = *iter;

		if (d->m_Character && PLR_FLAGGED(d->m_Character, PLR_NOSHOUT) && d->m_Character->GetPlayer()->m_MuteLength != 0)
		{
			if ((int)(time(0) - d->m_Character->GetPlayer()->m_MutedAt) >= d->m_Character->GetPlayer()->m_MuteLength)
			{
				REMOVE_BIT(PLR_FLAGS(d->m_Character), PLR_NOSHOUT);
				mudlogf(BRF, LVL_STAFF, TRUE, "(GC) Squelch auto-off for %s.", d->m_Character->GetName());
				d->m_Character->GetPlayer()->m_MuteLength = 0;
			}
		}
	}
}



/*
 * I tried to universally convert Circle over to POSIX compliance, but
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

void nonblock(socket_t s) {
#if defined(_WIN32)
	unsigned long flags = 1;
	if (ioctlsocket(s, FIONBIO, &flags))
#else
	int flags = fcntl(s, F_GETFL, 0);
	flags |= O_NONBLOCK;
	if (fcntl(s, F_SETFL, flags) < 0)
#endif
	{
		perror("SYSERR: Fatal error executing nonblock (comm.c)");
		exit(1);
	}
}
/********************************************************************
*	signal-handling functions (formerly signals.c)					*
********************************************************************/

RETSIGTYPE checkpointing(int unused) {
	if (!tics) {
		log("SYSERR: CHECKPOINT shutdown: tics not updated. (Infinite Loop Suspected)");
//		raise(SIGSEGV);
		abort();
	} else
		tics = 0;
}


RETSIGTYPE unrestrict_game(int unused) {
	mudlog("Received SIGUSR2 - reloading ban list (emergent)", BRF, LVL_IMMORT, TRUE);
	Ban::Reload();
	circle_restrict = 0;
}


RETSIGTYPE hupsig(int unused) {
	log("SYSERR: Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
	exit(0);		//	perhaps something more elegant should substituted
}

/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted for uniformity, because BSD systems
 * do not restart select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 *
 * SunOS Release 4.0.2 (sun386) needs this too, according to Tim Aldric.
 */

#ifndef POSIX
#define my_signal(signo, func) signal(signo, func)
#else
sigfunc *my_signal(int signo, sigfunc * func);

sigfunc *my_signal(int signo, sigfunc * func) {
	struct sigaction act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
#ifdef SA_INTERRUPT
	act.sa_flags |= SA_INTERRUPT;	/* SunOS */
#endif

	if (sigaction(signo, &act, &oact) < 0)
		return SIG_ERR;

	return oact.sa_handler;
}
#endif				/* NeXT */

#if !defined(_WIN32)
/* clean up our zombie kids to avoid defunct processes */
RETSIGTYPE reap(int sig) {
//	log("Beginning reap...");
	while (waitpid(-1, NULL, WNOHANG) > 0);
//	log("Finishing reap...");
	my_signal(SIGCHLD, reap);
//	struct rusage	ru;
//	wait3(NULL, WNOHANG, &ru);
}
#endif


void signal_setup(void) {
#if !defined(_WIN32)
	struct itimerval itime;
	struct timeval interval;

	//	Emergency unrestriction procedure
	my_signal(SIGUSR2, unrestrict_game);

	//	Deadlock Protection
	interval.tv_sec = 60;
	interval.tv_usec = 0;
	itime.it_interval = interval;
	itime.it_value = interval;
	setitimer(ITIMER_VIRTUAL, &itime, NULL);
	my_signal(SIGVTALRM, checkpointing);

	/* just to be on the safe side: */
	my_signal(SIGHUP, hupsig);
//	my_signal(SIGCHLD, reap);
#if	defined(SIGCLD)
	my_signal(SIGCLD, SIG_IGN);
#elif defined(SIGCHLD)
	my_signal(SIGCHLD, reap);
#endif

#endif
//	my_signal(SIGABRT, hupsig);
//	my_signal(SIGFPE, hupsig);
//	my_signal(SIGILL, hupsig);
//	my_signal(SIGSEGV, hupsig);
	my_signal(SIGINT, hupsig);
	my_signal(SIGTERM, hupsig);
#if !defined(_WIN32)
	my_signal(SIGPIPE, SIG_IGN);
	my_signal(SIGALRM, SIG_IGN);
#endif
}


/* ****************************************************************
*       Public routines for system-to-player-communication        *
**************************************************************** */


void send_to_char(const char *messg, CharData *ch) {
	if (!messg || !ch)
		return;

	if (ch->desc)	ch->desc->Write(messg);
	else			act(messg, FALSE, ch, 0, 0, TO_CHAR | TO_SCRIPT);
}


void send_to_all(const char *messg)
{
	if (!messg || !*messg)
		return;
	
    FOREACH(DescriptorList, descriptor_list, iter)
    {
		if ((*iter)->GetState()->IsPlaying())
			(*iter)->Write(messg);
	}
}


void send_to_zone(char *messg, ZoneData *zone, Hash ns) {
	if (!messg || !*messg)
		return;

    FOREACH(DescriptorList, descriptor_list, iter)
    {
    	DescriptorData *i = *iter;
		if (i->GetState()->IsPlaying() && i->m_Character
				&& AWAKE(i->m_Character) && !PLR_FLAGGED(i->m_Character, PLR_WRITING)
				&& i->m_Character->InRoom()
				&& IsSameEffectiveZone(i->m_Character->InRoom(), zone, ns))
			i->Write(messg);
	}
}


void send_to_zone_race(char *messg, ZoneData *zone, int race, Hash ns) {
	if (!messg || !*messg)
		return;

    FOREACH(DescriptorList, descriptor_list, iter)
    {
    	DescriptorData *i = *iter;
		if (i->GetState()->IsPlaying() && i->m_Character
				&& AWAKE(i->m_Character) && !PLR_FLAGGED(i->m_Character, PLR_WRITING)
				&& i->m_Character->GetRace() == race
				&& i->m_Character->InRoom()
				&& IsSameEffectiveZone(i->m_Character->InRoom(), zone, ns))
			i->Write(messg);
	}
}


//void send_to_room(const char *messg, int room) {
//	CharData *i;

//	if (!messg || !*messg || room == NOWHERE)
//		return;
	
//	START_ITER(iter, i, CharData *, world[room]->people) {
//		if (i->desc && (STATE(i->desc) == CON_PLAYING) && !PLR_FLAGGED(i, PLR_WRITING) && AWAKE(i))
//			SEND_TO_Q(messg, i->desc);
//	}
//}


void send_to_players(CharData *ch, const char *messg, ...) {
	va_list args;

	if (!messg || !*messg)
		return;
 
 	BUFFER(send_buf, MAX_STRING_LENGTH);
 	
	va_start(args, messg);
	vsnprintf(send_buf, MAX_STRING_LENGTH, messg, args);
	va_end(args);

    FOREACH(DescriptorList, descriptor_list, iter)
    {
    	DescriptorData *i = *iter;
		if (i->GetState()->IsPlaying() && i->m_Character && i->m_Character != ch &&
				!PLR_FLAGGED(i->m_Character, PLR_WRITING))
			i->Write(send_buf);
	}
}


char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression) \
	if ((pointer) == NULL) i = ACTNULL; else i = (expression);


/* higher-level communication: the act() function */
void perform_act(const char *original, CharData *ch, ObjData *obj, const void *vict_obj, CharData *to, int type, char *storeBuf) {
	const char *i = NULL;
	Relation relation = RELATION_NONE;
	extern char * relation_colors;
  	CharData *victim = NULL;
  	ObjData *target = NULL;
  	char *arg = NULL;
	char *buf;
	const char *orig = original;
	
	if (IS_SET(type, TO_IGNORABLE) && ch && to->IsIgnoring(GET_ID(ch)))
		return;

	if (IS_NPC(to) && !to->desc && (IS_SET(type, TO_NOTTRIG) || (to == ch) || !SCRIPT_CHECK(to, MTRIG_ACT)))
		return;
	
	BUFFER(lbuf, MAX_SCRIPT_STRING_LENGTH);
	buf = lbuf;
	
	char c = 0;
	while ((c = *orig++))
	{
		if (c != '$')
			*buf++ = c;
		else
		{
			c = *orig++;
			
			bool bModifierPosessive = false;
			
			if (c == UID_CHAR)	//	Override vict_obj
			{
				IDNum	idnum;
			  	CharData *uid_vict = NULL;
			  	ObjData *uid_obj = NULL;
				
				idnum = ParseIDString(orig-1);
				
				if (!(uid_vict = CharData::Find(idnum)))
					uid_obj = ObjData::Find(idnum);
				
				while (isdigit(*orig))	++orig;
				
				c = *orig++;
				
				if (uid_vict || uid_obj)
				{
					while (c)
					{
						if (c == '\'')	bModifierPosessive = true;
						else			break;
						
						c = *orig++;
					}
					
					switch (toupper(c))
					{
						case 'N':
							CHECK_NULL(uid_vict, PERS(uid_vict, to));
							if (uid_vict != NULL)	relation = to->GetRelation(uid_vict);
							break;
						case 'M':
							CHECK_NULL(uid_vict, HMHR(uid_vict, to));
							break;
						case 'S':
							CHECK_NULL(uid_vict, HSHR(uid_vict, to));
							break;
						case 'E':
							CHECK_NULL(uid_vict, HSSH(uid_vict, to));
							break;
						case 'O':
							CHECK_NULL(uid_obj, OBJN(uid_obj, to));
							break;
						case 'P':
							CHECK_NULL(uid_obj, OBJS(uid_obj, to));
							break;
//						case 'V':
//							CHECK_NULL(vict_obj, SHPS((ShipData *)vict_obj, to));
//							break;
						case 'A':
							CHECK_NULL(uid_obj, SANA(uid_obj));
							break;
					}
				}
			}
			else if (IS_SET(type, TO_SCRIPT))
			{
				//	scripts support newlines but nothing else if it wasn't an IDString
				switch (c)
				{
					case '_':
						i = "\n";
						break;
					default:
						--orig;	//	Backtrack to output whatever 'c' was, and output the '$' via the fallthrough
						//	fallthrough
					case '$':
						i = "$";
						break;
				}
			}
			else
			{
				while (c)
				{
					if (c == '\'')	bModifierPosessive = true;
					else			break;
					
					c = *orig++;
				}
				
				switch (c)
				{
					case 'n':
						relation = ch->GetRelation(to);
						if (type & TO_IGNOREINVIS)		i = (GET_REAL_LEVEL(to) >= ch->GetPlayer()->m_StaffInvisLevel ? (ch)->GetName() : "someone");
						else							i = PERS(ch, to);
						break;
					case 'N':
						if (type & TO_IGNOREINVIS) {
							CHECK_NULL(vict_obj, (GET_REAL_LEVEL(to) >= ((CharData *)vict_obj)->GetPlayer()->m_StaffInvisLevel ? ((CharData *)vict_obj)->GetName() : "someone"));
						} else {
							CHECK_NULL(vict_obj, PERS((CharData *) vict_obj, to));
						}
						
						if (vict_obj != NULL)
							relation = to->GetRelation((CharData *)vict_obj);
						victim = (CharData *) vict_obj;
						break;
					case 'm':
						i = HMHR(ch, to);
						break;
					case 'M':
						CHECK_NULL(vict_obj, HMHR((CharData *) vict_obj, to));
						victim = (CharData *) vict_obj;
						break;
					case 's':
						i = HSHR(ch, to);
						break;
					case 'S':
						CHECK_NULL(vict_obj, HSHR((CharData *) vict_obj, to));
						victim = (CharData *) vict_obj;
						break;
					case 'e':
						i = HSSH(ch, to);
						break;
					case 'E':
						CHECK_NULL(vict_obj, HSSH((CharData *) vict_obj, to));
						victim = (CharData *) vict_obj;
						break;
					case 'o':
						CHECK_NULL(obj, OBJN(obj, to));
						break;
					case 'O':
						CHECK_NULL(vict_obj, OBJN((ObjData *) vict_obj, to));
						target = (ObjData *) vict_obj;
						break;
					case 'p':
						CHECK_NULL(obj, OBJS(obj, to));
						break;
					case 'P':
						CHECK_NULL(vict_obj, OBJS((ObjData *) vict_obj, to));
						target = (ObjData *) vict_obj;
						break;
//					case 'v':
//						CHECK_NULL(obj, SHPS((ShipData *)obj, to));
//						break;
//					case 'V':
//						CHECK_NULL(vict_obj, SHPS((ShipData *)vict_obj, to));
//						break;
					case 'a':
						CHECK_NULL(obj, SANA(obj));
						break;
					case 'A':
						CHECK_NULL(vict_obj, SANA((ObjData *) vict_obj));
						target = (ObjData *) vict_obj;
						break;
					case 'T':
						CHECK_NULL(vict_obj, (char *) vict_obj);
						arg = (char *) vict_obj;
						break;
					case 't':
						CHECK_NULL(obj, (char *) obj);
						break;
					case 'F':
						CHECK_NULL(vict_obj, fname((char *) vict_obj));
						break;
					case '$':
						i = "$";
						break;
					case '_':
						i = "\n";
						break;
					default:
						log("SYSERR: Illegal $-code '%c' to act(): %s", c, original);
						break;
				}
			}
			
			if (i)
			{
				if (relation != RELATION_NONE)
				{
					*buf++ = '`';
					*buf++ = '(';
					*buf++ = '`';
					*buf++ = relation_colors[relation];
				}

//				while ((*buf = *(i++)))
//					buf++;

				char c = 0;
				char lastLetter = 0;

				do
				{
					lastLetter = c;
					c = *i++;
					*buf = c;
					
					if (c)	++buf;
				} while (c);
				
				if (lastLetter)	//	At least one thing added
				{
					if (bModifierPosessive)
					{
						*buf++ = '\'';
						if (lastLetter != 's')	*buf++ = 's';
					}
				}
				
				if (relation != RELATION_NONE)
				{
					*buf++ = '`';
					*buf++ = ')'; //'n';
					relation = RELATION_NONE;
				}
				
				i = NULL;
			}
		}
	}

	*buf++ = '\n';
	*buf++ = '\0';

	CAP(lbuf);
	
	if (to->desc)
		to->desc->Write(lbuf);
	if (IS_NPC(to) && !IS_SET(type, TO_NOTTRIG) && (to != ch))
		act_mtrigger(to, lbuf, ch, victim, obj, target, arg);
	if (IS_SET(type, TO_SAYBUF))
		to->AddSayBuf(lbuf);
	
	if (storeBuf)
		strcpy(storeBuf, lbuf);
}

#define SENDOK(ch) ((IS_NPC(ch) || ((ch)->desc && ch->desc->GetState()->IsPlaying())) \
		&& (AWAKE(ch) || sleep) && !PLR_FLAGGED((ch), PLR_WRITING))


void act(const char *str, int hide_invisible, CharData *ch, ObjData *obj, const void *vict_obj, int type, char *storeBuf)
{
	CharData *			to;
	RoomData *			room;
	bool 				sleep = false;
	int					minVisLevel = IS_SET(type, TO_FULLYVISIBLE_ONLY) ? VISIBLE_FULL : VISIBLE_SHIMMER;
	
	if (!str || !*str) {
		return;
	}
	
	if (IS_SET(type, TO_SLEEP))
		sleep = true;
	
	//	If this bit is set, the act triggers are not checked.
	
	if (IS_SET(type, TO_CHAR)) {
		if (ch && SENDOK(ch))
			perform_act(str, ch, obj, vict_obj, ch, type, storeBuf);
	}
	
	if (IS_SET(type, TO_VICT)) {
		if ((to = (CharData *) vict_obj) && SENDOK(to) &&
				!(hide_invisible && ch && to->CanSee(ch) < minVisLevel))
			perform_act(str, ch, obj, vict_obj, to, type, storeBuf);
	}
	
	if (IS_SET(type, TO_ZONE | TO_GAME)) {
	    FOREACH(DescriptorList, descriptor_list, iter)
	    {
	    	DescriptorData *i = *iter;
			if (i->m_Character && SENDOK(i->m_Character) && (i->m_Character != ch) &&
					!(hide_invisible && ch && i->m_Character->CanSee(ch) < minVisLevel) &&
					(IS_SET(type, TO_GAME) || IsSameEffectiveZone(ch->InRoom(), i->m_Character->InRoom())))
				perform_act(str, ch ? ch : i->m_Character, obj, vict_obj, i->m_Character, type, storeBuf);
		}
	}
	
	if (IS_SET(type, TO_TEAM)) {
	    FOREACH(DescriptorList, descriptor_list, iter)
	    {
	    	DescriptorData *i = *iter;
			if (i->m_Character && SENDOK(i->m_Character) && (i->m_Character != ch) &&
					(GET_TEAM(i->m_Character) == (int)vict_obj))
				perform_act(str, ch ? ch : i->m_Character, obj, vict_obj, i->m_Character, type, storeBuf);
		}
	}
	
	if (IS_SET(type, TO_ROOM | TO_NOTVICT)) {
		if (ch && ch->InRoom()) {
			room = ch->InRoom();
		} else if (obj && obj->InRoom()) {
			room = obj->InRoom();
		} else {
			log("SYSERR: no valid target to act()!");
			log("SYSERR: \"%s\"", str);
			return;
		}
		
		FOREACH(CharacterList, room->people, iter)
		{
			to = *iter;
			if (SENDOK(to) && !(hide_invisible && ch && to->CanSee(ch) < minVisLevel) &&
					(to != ch) && (IS_SET(type, TO_ROOM) || (to != vict_obj)))
			{
				perform_act(str, ch, obj, vict_obj, to, type, storeBuf);
			}
		}

		if (((ch && !PURGED(ch)) || (obj && !PURGED(obj))) && !IS_SET(type, TO_NOTTRIG))
		{
			act_wtrigger(room, (char *)str, ch, obj);
		}
	}
}

#define CNRM	"\x1B[0m"
#define CBLK	"\x1B[22;30m"
#define CRED	"\x1B[22;31m"
#define CGRN	"\x1B[22;32m"
#define CYEL	"\x1B[22;33m"
#define CBLU	"\x1B[22;34m"
#define CMAG	"\x1B[22;35m"
#define CCYN	"\x1B[22;36m"
#define CWHT	"\x1B[22;37m"

#define BBLK	"\x1B[1;30m"
#define BRED	"\x1B[1;31m"
#define BGRN	"\x1B[1;32m"
#define BYEL	"\x1B[1;33m"
#define BBLU	"\x1B[1;34m"
#define BMAG	"\x1B[1;35m"
#define BCYN	"\x1B[1;36m"
#define BWHT	"\x1B[1;37m"

#define BKBLK	"\x1B[40m"
#define BKRED	"\x1B[41m"
#define BKGRN	"\x1B[42m"
#define BKYEL	"\x1B[43m"
#define BKBLU	"\x1B[44m"
#define BKMAG	"\x1B[45m"
#define BKCYN	"\x1B[46m"
#define BKWHT	"\x1B[47m"

#define REVERSE	"\x1B[7m"
#define FLASH	"\x1B[5m"
#define BELL	"\007"

#define MAX_COLOR_LEN 7
/*
const char *COLORLIST[] = {CNRM, CBLK, CRED,CGRN,CYEL,CBLU,CMAG,CCYN,CWHT,
			   BBLK, BRED,BGRN,BYEL,BBLU,BMAG,BCYN,BWHT,
			   REVERSE, REVERSE, FLASH, FLASH, BELL, "`",
			   BKBLK,BKRED,BKGRN,BKYEL,BKBLU,BKMAG,BKCYN,BKWHT, "^"};
*/
const char *COLORLIST[] = {
	CNRM, CBLK, CRED, CGRN, CYEL, CBLU, CMAG, CCYN, CWHT,
		  BBLK, BRED, BGRN, BYEL, BBLU, BMAG, BCYN, BWHT,
		  REVERSE, REVERSE, FLASH, FLASH, BELL,
		  BKBLK,BKRED,BKGRN,BKYEL,BKBLU,BKMAG,BKCYN,BKWHT };


struct ColorState
{
	enum Color
	{
		None,
		Black,
		Red,
		Green,
		Yellow,
		Blue,
		Magenta,
		Cyan,
		White/*,
		BoldBlack,
		BoldRed,
		BoldGreen,
		BoldYellow,
		BoldBlue,
		BoldMagenta,
		BoldCyan,
		BoldWhite*/
	};
	
	Color	fore, back;
	bool	bold, reverse, flash;
	bool	reset;
	
	ColorState() { Reset(); reset = true; }
	void Reset()
	{
		if (/*fore != None ||*/ back != None || bold || reverse || flash)	reset = true;
		fore = None;
		back = None;
		bold = false;
		reverse = false;
		flash = false;
	}
	bool operator==(const ColorState &s) const
	{
		return (fore == s.fore) && (back == s.back) && (bold == s.bold) && (reverse == s.reverse) && (flash == s.flash);
	}
	bool operator!=(const ColorState &s) const { return !(*this == s); }
	const char *Print()
	{
		static char buf[32];
		char *p = buf;
		bool bCodesAdded = false;
		
		*p++ = '\x1B';
		*p++ = '[';
		
		if (reset)
		{
			*p++ = '0';	
			bCodesAdded = true;
			reset = false;
		}
		
		if (fore != None)
		{
			if (bold)
			{
				if (bCodesAdded)	*p++ = ';';
				*p++ = '1';
				bCodesAdded = true;
			}
			
			if (bCodesAdded)	*p++ = ';';
			*p++ = '3';
			*p++ = '0' + fore - 1;
			bCodesAdded = true;
		}
		
		if (back != None)
		{
			if (bCodesAdded)	*p++ = ';';
			*p++ = '4';
			*p++ = '0' + back - 1;
			bCodesAdded = true;
		}

		if (flash)
		{
			if (bCodesAdded)	*p++ = ';';
			*p++ = '5';
			bCodesAdded = true;
		}		
		
		if (reverse)
		{
			if (bCodesAdded)	*p++ = ';';
			*p++ = '7';
			bCodesAdded = true;
		}
		
		*p++ = 'm';
		*p++ = '\0';
		
		return bCodesAdded ? buf : "";
	}
};


void ProcessColor(const char *src, char *dest, size_t len, bool bColor)
{
	BUFFER(buf, len);
	
	const size_t MAX_STATE_STACK_SIZE = 8;
	size_t stateStackSize = 0;
	
	ColorState	state, prevState, stateStack[MAX_STATE_STACK_SIZE];
	int			nRawDepth = 0;
	
	enum 
	{
		COLOR_CODE_NONE,
		COLOR_CODE_PRINTCODE,
		COLOR_CODE_NORMAL,
		COLOR_CODE_BACKGROUND
	} colorCodeState = COLOR_CODE_NONE;
	
	char *	p = buf;
	
	while (len > 1)
	{
		char c = *src++;
		
		if (!c)	break;
	
		if (colorCodeState < COLOR_CODE_NORMAL)
		{
			if (c == '`' && colorCodeState != COLOR_CODE_PRINTCODE)
				colorCodeState = COLOR_CODE_NORMAL;
			else
			{
//				if (c == UID_CHAR && *src != '[')
//					c = '@';
				
//				if (c == '\r' && bColor)
//				{
					//	Reset the color at a newline
//					state.Reset();
//				}
				
				if (bColor && (state.reset || isgraph(c) || ISNEWL(c)) && state != prevState)
				{
					if (state.fore == ColorState::None && prevState.fore != ColorState::None)
						state.reset = true;
					
					//	print it out
					const char *code = state.Print();
					
					if (strlen(code) >= len)	//	Too long!  Abort now
						break;
					
					if (*code)
					{
						char n;
						while ((n = *code++))
						{
							*p++ = n;
							--len;	
						}
						
						prevState = state;
					}
					
				}
				
				if (c == '\n')
				{
					if (len < 3)
						break;
					*p++ = '\r';
					--len;
				}
				
				*p++ = c;
				--len;
				
				colorCodeState = COLOR_CODE_NONE;
			}
		}
		else if (colorCodeState == COLOR_CODE_NORMAL && c == '^' && !nRawDepth)
			colorCodeState = COLOR_CODE_BACKGROUND;
		else if (colorCodeState == COLOR_CODE_NORMAL && c == '`' && !nRawDepth)
		{
			colorCodeState = COLOR_CODE_PRINTCODE;
			src -= 1;
		}
		else	//	Color code completed
		{
			static const char *COLOR_CHARS = "nkrgybmcwif*()[]";
			enum
			{
				MAX_COLOR_CHAR = ColorState::White,
				REVERSE_CHAR,
				FLASH_CHAR,
				BEEP_CHAR,
				PUSHSTATE_CHAR,
				POPSTATE_CHAR,
				BEGINRAW_CHAR,
				ENDRAW_CHAR
			};
			
			bool	bBold = isupper(c);
			int		color = search_chars(tolower(c), COLOR_CHARS);
			
			if (color == -1)
			{
				//	Force it to display the bad code
				src -= (colorCodeState == COLOR_CODE_NORMAL) ? 2 : 3;	//	Step back
				colorCodeState = COLOR_CODE_PRINTCODE;				//	Go back through color printer
//				c = *src++;
				continue;
			}
			else if (nRawDepth)
			{
				if (color == BEGINRAW_CHAR)
					++nRawDepth;
				else if (color == ENDRAW_CHAR)
					--nRawDepth;
				
				if (nRawDepth)
				{
					//	Force it to display the color code
					src -= 2;	//	Step back
					colorCodeState = COLOR_CODE_PRINTCODE;				//	Go back through color printer
//					c = *src++;
					continue;
				}
			}
			else if (color == BEGINRAW_CHAR)
				++nRawDepth;
			else if (!bColor)
			{
				//	Do nothing
			}
			else if (color == ColorState::None)
			{
				if (colorCodeState == COLOR_CODE_BACKGROUND)
				{
					state.back = ColorState::None;
					state.reset = true;
				}
				else
				{
					state.Reset();
				}
			}
			else if (color <= MAX_COLOR_CHAR)
			{
				if (colorCodeState == COLOR_CODE_BACKGROUND)
				{
					state.back = (ColorState::Color)color;
					state.reset = true;
				}
				else
				{
					state.fore = (ColorState::Color)color;
					if (!bBold && state.bold)	state.reset = true;
					state.bold = bBold;
				}
			}
			else if (color == REVERSE_CHAR)
			{
				state.reverse = true;
				state.reset = true;
			}
			else if (color == FLASH_CHAR)
				state.flash = true;
			else if (color == BEEP_CHAR)
			{
				*p++ = '\007';
				--len;
			}
			else if (color == PUSHSTATE_CHAR)
			{
				if (stateStackSize < MAX_STATE_STACK_SIZE)
					stateStack[stateStackSize++] = state;
			}
			else if (color == POPSTATE_CHAR)
			{
				if (stateStackSize > 0)
				{
					ColorState oldState = state;
					state = stateStack[--stateStackSize];
					state.reset = true;
				}
			}
			
			colorCodeState = COLOR_CODE_NONE;
		}
		
//		c = *src++;
	}
	
	if (bColor)
	{
		state.Reset();
		
		if (state != prevState)
		{
			//	Normalize the final output if necessary
			static const int CNRM_LENGTH = strlen(CNRM);
			while (len < (CNRM_LENGTH + 1))
			{
				--p;
				++len;	
			}
			
			const char *code = CNRM;
			char c;
			while ((c = *code++))	*p++ = c;
		}
	}
	
	*p = 0;
	strcpy(dest, buf);	
}


void proc_color(const char *inbuf, char *outbuf, int lengthRemaining, bool useColor)
{
	if(!*inbuf)
		return;
	
	enum 
	{
		COLOR_NONE,
		COLOR_NORMAL,
		COLOR_INVERSE
	} colorState = COLOR_NONE;
	
	BUFFER(tempbuf, lengthRemaining);
	
	const char *inp = inbuf;
	char *outp = tempbuf;
	int color = 0;
	
	char c;
	while ((c = *inp++) && lengthRemaining > 1)
	{
		if (colorState == COLOR_NONE)
		{
			if (c == '`')
				colorState = COLOR_NORMAL;
			else
			{
//				if (c == UID_CHAR && *inp != '[')
//					c = '@';
				*outp++ = c;
				--lengthRemaining;
			}
		}
		else
		{
			if (colorState == COLOR_NORMAL && c == '^')
				colorState = COLOR_INVERSE;
			else if (colorState == COLOR_NORMAL && c == '`')
			{
				colorState = COLOR_NONE;
				*outp++ = c;
				--lengthRemaining;
			}
			else
			{
				//	'c' is our color code... so heres what we do:
				//	if it's inverse, find the inverse code
				//	Color state = normal
				color = search_chars(c,
					colorState == COLOR_NORMAL ?
						"nkrgybmcwKRGYBMCWiIfF*" :
						"krgybmcw");
				
				if (color == -1)
				{
					//	Echo the bad code out!
					
					if (lengthRemaining > 3)
					{
						*outp++ = '`';	//	Er yeah... trailing "`" and "`^" should be converted...
						if (colorState == COLOR_INVERSE)
							*outp++ = '^';
						*outp++ = c;
						
						lengthRemaining -= 3;
					}
				}
				else
				{
					if (colorState == COLOR_INVERSE)
						color += 22;
					
					if (useColor /* || c == '*'*/)	//	Uncomment this part to allow beeps to get through
					{
						const char *p = COLORLIST[color];
						
						if (lengthRemaining > MAX_COLOR_LEN)
						{
							while ((c = *p++))
							{
								*outp++ = c;
								--lengthRemaining;
							}
						}
					}
				}
				
				colorState = COLOR_NONE;
			}
		}
	}
	
	if (colorState != COLOR_NONE && lengthRemaining > 1)
	{
		*outp++ = '`';	//	Er yeah... trailing "`" and "`^" should be converted...
		--lengthRemaining;
		if (colorState == COLOR_INVERSE && lengthRemaining > 1)
		{
			*outp++ = '^';
			--lengthRemaining;
		}
	}
	
	if (color && useColor)
	{
		//	Normalize the final output if necessary
		while (lengthRemaining < (MAX_COLOR_LEN + 1))
		{
			--outp;
			++lengthRemaining;
		}
		
		const char *p = COLORLIST[0];
		while ((c = *p++))	*outp++ = c;
	}
	
	*outp = '\0';
	
	strcpy(outbuf, tempbuf);
}



Lexi::String strip_color (const char *txt)
{
	if (!txt)	return "(NULL)";
	
	BUFFER(buf, MAX_STRING_LENGTH);
    proc_color(txt, buf, MAX_STRING_LENGTH, false);	//	This strips color

	return buf;
}


void sub_write_to_char(CharData *ch, char *tokens[], Ptr otokens[], char type[]) {
	BUFFER(string, MAX_STRING_LENGTH);
	int i;

	for (i = 0; tokens[i + 1]; i++) {
		strcat(string, tokens[i]);

		switch (type[i]) {
			case '~':
				if (!otokens[i])			strcat(string, "someone");
				else if (otokens[i] == ch)	strcat(string, "you");
				else						strcat(string, PERS((CharData *) otokens[i], ch));
				break;

			case '@':
				if (!otokens[i])			strcat(string, "someone's");
				else if (otokens[i] == ch)	strcat(string, "your");
				else {
					strcat(string, PERS((CharData *) otokens[i], ch));
					strcat(string, "'s");
				}
				break;

			case '#':
				if (!otokens[i] || ch->CanSee((CharData *) otokens[i]) == VISIBLE_NONE)
											strcat(string, "its");
				else if (otokens[i] == ch)	strcat(string, "your");
				else						strcat(string, HSHR((CharData *) otokens[i]));
				break;

			case '&':
				if (!otokens[i] || ch->CanSee((CharData *) otokens[i]) == VISIBLE_NONE)
											strcat(string, "it");
				else if (otokens[i] == ch)	strcat(string, "you");
				else						strcat(string, HSSH((CharData *) otokens[i]));
				break;

			case '*':
				if (!otokens[i] || ch->CanSee((CharData *) otokens[i]) == VISIBLE_NONE)
											strcat(string, "it");
				else if (otokens[i] == ch)	strcat(string, "you");
				else						strcat(string, HMHR((CharData *) otokens[i]));
				break;

			case ':':
				if (!otokens[i])			strcat(string, "something");
				else						strcat(string, OBJS(((ObjData *) otokens[i]), ch));
				break;
		}
	}

	strcat(string, tokens[i]);
	strcat(string, "\n");
	CAP(string);
	send_to_char(string, ch);
}


void sub_write(char *arg, CharData *ch, bool find_invis, int targets) {
	BUFFER(str, MAX_INPUT_LENGTH * 2);
	BUFFER(type, MAX_INPUT_LENGTH);
	BUFFER(name, MAX_INPUT_LENGTH);
	char *tokens[MAX_INPUT_LENGTH], *s, *p;
	Ptr otokens[MAX_INPUT_LENGTH];
	ObjData *obj;
	CharData *to;
	int i, tmp;
	bool sleep = false;

	if (arg) {
		tokens[0] = str;

		for (i = 0, p = arg, s = str; *p;) {
			switch (*p) {
				case '~':	//	get char_data, move to next token
				case '@':
				case '#':
				case '&':
				case '*':
					type[i] = *p;
					*s = '\0';
					p = any_one_name(++p, name);
					otokens[i] = find_invis ? get_char(name) : get_char_vis(ch, name, FIND_CHAR_ROOM);
					tokens[++i] = ++s;
					break;

				case ':':	//	get obj_data, move to next token
					type[i] = *p;
					*s = '\0';
					p = any_one_name(++p, name);
					otokens[i] =
					find_invis ? (obj = get_obj(name)) :
					((obj = get_obj_in_list_vis(ch, name, ch->InRoom()->contents)) ?
					obj : (obj = get_obj_in_equip_vis(ch, name, ch->equipment, &tmp)) ?
					obj : (obj = get_obj_in_list_vis(ch, name, ch->carrying)));
					otokens[i] = obj;
					tokens[++i] = ++s;
					break;

				case '\\':
					p++;
					*s++ = *p++;
					break;

				default:
					*s++ = *p++;
			}
		}

		*s = '\0';
		tokens[++i] = NULL;

		if (IS_SET(targets, TO_CHAR) && SENDOK(ch))
			sub_write_to_char(ch, tokens, otokens, type);

		if (IS_SET(targets, TO_ROOM)) {
			FOREACH(CharacterList, ch->InRoom()->people, iter)
			{
				to = *iter;
				if (to != ch && SENDOK(to))
					sub_write_to_char(to, tokens, otokens, type);
			}
		}
	}
}



/*
 * Ported to SMAUG by Garil of DOTDII Mud
 * aka Jesse DeFer <dotd@dotd.com>  http://www.dotd.com
 *
 * revision 1: MCCP v1 support
 * revision 2: MCCP v2 support
 * revision 3: Correct MMCP v2 support
 * revision 4: clean up of write_to_descriptor() suggested by Noplex@CB
 *
 * See the web site below for more info.
 */

/*
 * mccp.c - support functions for mccp (the Mud Client Compression Protocol)
 *
 * see http://homepages.ihug.co.nz/~icecube/compress/ and README.Rom24-mccp
 *
 * Copyright (c) 1999, Oliver Jowett <icecube@ihug.co.nz>.
 *
 * This code may be freely distributed and used if this copyright notice is
 * retained intact.
 */

static void *zlib_alloc(void *opaque, unsigned int items, unsigned int size)
{
    return calloc(items, size);
}

static void zlib_free(void *opaque, void *address)
{
    free(address);
}




#if 0
ACMD(do_performancetest)
{
	extern void ScriptEngineRunScriptProcessSetPerformSet(Scriptable *sc, TrigThread *thread, const char *name, const char *value, unsigned scope, bool bIsUnset);
	extern void ScriptEngineRunScriptProcessSetPerformSet2(Scriptable *sc, TrigThread *thread, const char *name, const char *value, unsigned scope, bool bIsUnset);
	extern ScriptVariable *ScriptEngineFindVariable(Scriptable *sc, TrigThread *thread, const char *name, Hash hash, int context, unsigned scope);

	const char *varname = "testVar";
	const char *input = "testVar.abilities.2.list.3.name";
	const char *value = "ARM/LEG SCOPING";
	const char *sourceVal = "[#name:Sniper##abilities:"
		"[#name:Marksman##list:[#name:Sniper Core#] [#name:Fitness#]#]"
		"[#name:Sharpshooter##list:[#name:Sentry Guns#] [#name:Prone#] [#name:Arm/Leg Scoping#]#]"
		"[#name:Expert##list:[#name:Head/Body Scoping#] [#name:Accuracy#]#]#]";
	
	ch->GetScript()->RemoveTrigger("1");
	TrigData *trig = read_trigger(1);
	ch->GetScript()->AddTrigger(trig);
	TrigThread *thread = new TrigThread(trig, ch);
	
	thread->GetCurrentVariables().Add(varname, sourceVal);
	ScriptVariable *vd = thread->GetCurrentVariables().Find(varname, GetStringFastHash(varname));
	
	struct timeval inTime, outTime;

	gettimeofday(&inTime, (struct timezone *) 0);
	long uSec = inTime.tv_usec;
	while (inTime.tv_usec == uSec)
		gettimeofday(&inTime, (struct timezone *) 0);
	
	for (int i = 0; i < 1000000; ++i)
	{
		ScriptEngineRunScriptProcessSetPerformSet(ch->GetScript(), thread, input, value, Scope::Local, false);
	}
	gettimeofday(&outTime, (struct timezone *) 0);
	
	long msPassed = (outTime.tv_sec * 1000 + outTime.tv_usec / 1000) -
					(inTime.tv_sec * 1000 + inTime.tv_usec / 1000);
	ch->Send("Result1:\n"
			"Variable: %s\n"
			"Time: %ld\n", vd->GetValue(), msPassed);
	
	uSec = inTime.tv_usec;
	while (inTime.tv_usec == uSec)
		gettimeofday(&inTime, (struct timezone *) 0);
	
	for (int i = 0; i < 1000000; ++i)
	{
		ScriptEngineRunScriptProcessSetPerformSet2(ch->GetScript(), thread, input, value, Scope::Local, false);
	}
	gettimeofday(&outTime, (struct timezone *) 0);
	
	msPassed = (outTime.tv_sec * 1000 + outTime.tv_usec / 1000) -
					(inTime.tv_sec * 1000 + inTime.tv_usec / 1000);
	ch->Send("Result2:\n"
			"Variable: %s\n"
			"Time: %ld\n", vd->GetValue(), msPassed);

}
#endif
