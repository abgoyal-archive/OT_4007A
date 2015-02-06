
/*	$NetBSD: error.c,v 1.31 2003/08/07 09:05:30 agc Exp $	*/


#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)error.c	8.2 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: error.c,v 1.31 2003/08/07 09:05:30 agc Exp $");
#endif
#endif /* not lint */


#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "shell.h"
#include "main.h"
#include "options.h"
#include "output.h"
#include "error.h"
#include "show.h"

#define signal bsd_signal

struct jmploc *handler;
int exception;
volatile int suppressint;
volatile int intpending;
char *commandname;


static void exverror(int, const char *, va_list)
    __attribute__((__noreturn__));


void
exraise(int e)
{
	if (handler == NULL)
		abort();
	exception = e;
	longjmp(handler->loc, 1);
}



void
onint(void)
{
	sigset_t nsigset;

	if (suppressint) {
		intpending = 1;
		return;
	}
	intpending = 0;
	sigemptyset(&nsigset);
	sigprocmask(SIG_SETMASK, &nsigset, NULL);
	if (rootshell && iflag)
		exraise(EXINT);
	else {
		signal(SIGINT, SIG_DFL);
		raise(SIGINT);
	}
	/* NOTREACHED */
}

static void
exvwarning(int sv_errno, const char *msg, va_list ap)
{
	/* Partially emulate line buffered output so that:
	 *	printf '%d\n' 1 a 2
	 * and
	 *	printf '%d %d %d\n' 1 a 2
	 * both generate sensible text when stdout and stderr are merged.
	 */
	if (output.nextc != output.buf && output.nextc[-1] == '\n')
		flushout(&output);
	if (commandname)
		outfmt(&errout, "%s: ", commandname);
	if (msg != NULL) {
		doformat(&errout, msg, ap);
		if (sv_errno >= 0)
			outfmt(&errout, ": ");
	}
	if (sv_errno >= 0)
		outfmt(&errout, "%s", strerror(sv_errno));
	out2c('\n');
	flushout(&errout);
}

static void
exverror(int cond, const char *msg, va_list ap)
{
	CLEAR_PENDING_INT;
	INTOFF;

#ifdef DEBUG
	if (msg) {
		TRACE(("exverror(%d, \"", cond));
		TRACEV((msg, ap));
		TRACE(("\") pid=%d\n", getpid()));
	} else
		TRACE(("exverror(%d, NULL) pid=%d\n", cond, getpid()));
#endif
	if (msg)
		exvwarning(-1, msg, ap);

	flushall();
	exraise(cond);
	/* NOTREACHED */
}


void
error(const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	exverror(EXERROR, msg, ap);
	/* NOTREACHED */
	va_end(ap);
}


void
exerror(int cond, const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	exverror(cond, msg, ap);
	/* NOTREACHED */
	va_end(ap);
}


void
sh_exit(int rval)
{
	exerrno = rval & 255;
	exraise(EXEXEC);
}

void
sh_err(int status, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	exvwarning(errno, fmt, ap);
	va_end(ap);
	sh_exit(status);
}

void
sh_verr(int status, const char *fmt, va_list ap)
{
	exvwarning(errno, fmt, ap);
	sh_exit(status);
}

void
sh_errx(int status, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	exvwarning(-1, fmt, ap);
	va_end(ap);
	sh_exit(status);
}

void
sh_verrx(int status, const char *fmt, va_list ap)
{
	exvwarning(-1, fmt, ap);
	sh_exit(status);
}

void
sh_warn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	exvwarning(errno, fmt, ap);
	va_end(ap);
}

void
sh_vwarn(const char *fmt, va_list ap)
{
	exvwarning(errno, fmt, ap);
}

void
sh_warnx(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	exvwarning(-1, fmt, ap);
	va_end(ap);
}

void
sh_vwarnx(const char *fmt, va_list ap)
{
	exvwarning(-1, fmt, ap);
}



struct errname {
	short errcode;		/* error number */
	short action;		/* operation which encountered the error */
	const char *msg;	/* text describing the error */
};


#define ALL (E_OPEN|E_CREAT|E_EXEC)

STATIC const struct errname errormsg[] = {
	{ EINTR,	ALL,	"interrupted" },
	{ EACCES,	ALL,	"permission denied" },
	{ EIO,		ALL,	"I/O error" },
	{ EEXIST,	ALL,	"file exists" },
	{ ENOENT,	E_OPEN,	"no such file" },
	{ ENOENT,	E_CREAT,"directory nonexistent" },
	{ ENOENT,	E_EXEC,	"not found" },
	{ ENOTDIR,	E_OPEN,	"no such file" },
	{ ENOTDIR,	E_CREAT,"directory nonexistent" },
	{ ENOTDIR,	E_EXEC,	"not found" },
	{ EISDIR,	ALL,	"is a directory" },
#ifdef EMFILE
	{ EMFILE,	ALL,	"too many open files" },
#endif
	{ ENFILE,	ALL,	"file table overflow" },
	{ ENOSPC,	ALL,	"file system full" },
#ifdef EDQUOT
	{ EDQUOT,	ALL,	"disk quota exceeded" },
#endif
#ifdef ENOSR
	{ ENOSR,	ALL,	"no streams resources" },
#endif
	{ ENXIO,	ALL,	"no such device or address" },
	{ EROFS,	ALL,	"read-only file system" },
	{ ETXTBSY,	ALL,	"text busy" },
#ifdef EAGAIN
	{ EAGAIN,	E_EXEC,	"not enough memory" },
#endif
	{ ENOMEM,	ALL,	"not enough memory" },
#ifdef ENOLINK
	{ ENOLINK,	ALL,	"remote access failed" },
#endif
#ifdef EMULTIHOP
	{ EMULTIHOP,	ALL,	"remote access failed" },
#endif
#ifdef ECOMM
	{ ECOMM,	ALL,	"remote access failed" },
#endif
#ifdef ESTALE
	{ ESTALE,	ALL,	"remote access failed" },
#endif
#ifdef ETIMEDOUT
	{ ETIMEDOUT,	ALL,	"remote access failed" },
#endif
#ifdef ELOOP
	{ ELOOP,	ALL,	"symbolic link loop" },
#endif
	{ E2BIG,	E_EXEC,	"argument list too long" },
#ifdef ELIBACC
	{ ELIBACC,	E_EXEC,	"shared library missing" },
#endif
	{ 0,		0,	NULL },
};



const char *
errmsg(int e, int action)
{
	struct errname const *ep;
	static char buf[12];

	for (ep = errormsg ; ep->errcode ; ep++) {
		if (ep->errcode == e && (ep->action & action) != 0)
			return ep->msg;
	}
	fmtstr(buf, sizeof buf, "error %d", e);
	return buf;
}
