
/*	$NetBSD: shell.h,v 1.17 2003/08/07 09:05:38 agc Exp $	*/



#include <sys/param.h>

#define JOBS 1
#ifndef BSD
#define BSD 1
#endif

#ifndef DO_SHAREDVFORK
#if __NetBSD_Version__ >= 104000000
#define DO_SHAREDVFORK
#endif
#endif

typedef void *pointer;
#ifndef NULL
#define NULL (void *)0
#endif
#define STATIC	/* empty */
#define MKINIT	/* empty */

#include <sys/cdefs.h>

extern char nullstr[1];		/* null string */


#ifdef DEBUG
#define TRACE(param)	trace param
#define TRACEV(param)	tracev param
#else
#define TRACE(param)
#define TRACEV(param)
#endif
