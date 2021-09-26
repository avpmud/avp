/* src/conf.h.  Generated automatically by configure.  */
/* src/conf.h.in.  Generated automatically from configure.in by autoheader.  */

#ifdef _WIN32
#define WINVER 0x0500
#endif

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#ifndef _WIN32
#define HAVE_SYS_WAIT_H 1
#endif

/* Define to `int' if <sys/types.h> doesn't define.  */
#ifdef _WIN32
#define pid_t int
#endif

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define if we're compiling CircleMUD under any type of UNIX system */
#ifndef _WIN32
#define CIRCLE_UNIX 1
#endif

/* Define to `int' if <sys/types.h> doesn't define.  */
#ifdef _WIN32
#define ssize_t int
#endif

/* Define to `int' if <socketbits.h> doesn't define.  */
#ifdef _WIN32
#define socklen_t int
#endif

/* Define to `int' if <socket.h> doesn't define.  */
#define socket_t int

/* Define if you have the <arpa/telnet.h> header file.  */
#ifndef _WIN32
#define HAVE_ARPA_TELNET_H 1
#endif

/* Define if you have the <signal.h> header file.  */
#define HAVE_SIGNAL_H 1

/* Define if you have the <sys/time.h> header file.  */
#ifndef _WIN32
#define TIME_WITH_SYS_TIME 1
#endif

/* Define if you have the <unistd.h> header file.  */
#ifndef _MSC_VER
#define HAVE_UNISTD_H 1
#endif

#ifdef _MSC_VER
#define _HAS_ITERATOR_DEBUGGING 0
#endif
