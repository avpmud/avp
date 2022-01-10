/* ************************************************************************
*   File: sysdep.h                                      Part of CircleMUD *
*  Usage: machine-specific defs based on values in conf.h (from configure)*
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/************************************************************************/
/*** Do not change anything below this line *****************************/
/************************************************************************/

/*
 * Set up various machine-specific things based on the values determined
 * from configure and conf.h.
 */

/* Standard C headers  *************************************************/

#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

/* POSIX compliance  *************************************************/

#include <sys/types.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

/* Now, we #define POSIX if we have a POSIX system. */

#ifdef HAVE_UNISTD_H
/* Ultrix's unistd.h always defines _POSIX_VERSION, but you only get
   POSIX.1 behavior with `cc -YPOSIX', which predefines POSIX itself!  */
#if defined (_POSIX_VERSION) && !defined (ultrix)
#define POSIX
#endif

/* Some systems define _POSIX_VERSION but are not really POSIX.1.  */
#if (defined (butterfly) || defined (__arm) || \
     (defined (__mips) && defined (_SYSTYPE_SVR3)) || \
     (defined (sequent) && defined (i386)))
#undef POSIX
#endif
#endif /* HAVE_UNISTD_H */


#ifdef HAVE_SIGNAL_H
# ifndef _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 2
# include <signal.h>
# undef _POSIX_C_SOURCE
# else
#  include <signal.h>	/* GNU libc 6 already defines _POSIX_C_SOURCE. */
# endif
#endif

/* Header files *******************************************************/


/* Header files common to all source files */

//#ifdef HAVE_SYS_ERRNO_H
//#include <sys/errno.h>
//#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
#endif
# include <time.h>


#if !defined(__GNUC__)
#define __attribute__(x)	/* nothing */
#define __PRETTY_FUNCTION__	__FILE__
#endif

/* Define the type of a socket and other miscellany */
#ifdef _WIN32
#define CLOSE_SOCKET(sock) closesocket(sock)
#else
#define CLOSE_SOCKET(sock) close(sock)
#endif



#if defined(_MSC_VER)
# define strcasecmp _strcmpi
# define strncasecmp _strnicmp
# define strtoll _strtoi64
# define strtoull _strtoui64
# define atoll _atoi64
# define llabs _abs64
# define snprintf _snprintf
# define vsnprintf _vsnprintf
#endif


#include "memory.h"
