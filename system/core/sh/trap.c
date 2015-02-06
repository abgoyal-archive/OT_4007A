
/*	$NetBSD: trap.c,v 1.31 2005/01/11 19:38:57 christos Exp $	*/


#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)trap.c	8.5 (Berkeley) 6/5/95";
#else
__RCSID("$NetBSD: trap.c,v 1.31 2005/01/11 19:38:57 christos Exp $");
#endif
#endif /* not lint */

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include "shell.h"
#include "main.h"
#include "nodes.h"	/* for other headers */
#include "eval.h"
#include "jobs.h"
#include "show.h"
#include "options.h"
#include "syntax.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"
#include "trap.h"
#include "mystring.h"
#include "var.h"


#define S_DFL 1			/* default signal handling (SIG_DFL) */
#define S_CATCH 2		/* signal is caught */
#define S_IGN 3			/* signal is ignored (SIG_IGN) */
#define S_HARD_IGN 4		/* signal is ignored permenantly */
#define S_RESET 5		/* temporary - to reset a hard ignored sig */


char *trap[NSIG+1];		/* trap handler commands */
MKINIT char sigmode[NSIG];	/* current value of signal */
char gotsig[NSIG];		/* indicates specified signal received */
int pendingsigs;		/* indicates some signal received */

static int getsigaction(int, sig_t *);


static int
signame_to_signum(const char *p)
{
	int i;

	if (is_number(p))
		return number(p);

	if (strcasecmp(p, "exit") == 0 )
		return 0;
	
	if (strncasecmp(p, "sig", 3) == 0)
		p += 3;

	for (i = 0; i < NSIG; ++i)
		if (strcasecmp (p, sys_signame[i]) == 0)
			return i;
	return -1;
}

static void
printsignals(void)
{
	int n;

	out1str("EXIT ");

	for (n = 1; n < NSIG; n++) {
		out1fmt("%s", sys_signame[n]);
		if ((n == NSIG/2) ||  n == (NSIG - 1))
			out1str("\n");
		else
			out1c(' ');
	}
}


int
trapcmd(int argc, char **argv)
{
	char *action;
	char **ap;
	int signo;

	if (argc <= 1) {
		for (signo = 0 ; signo <= NSIG ; signo++)
			if (trap[signo] != NULL) {
				out1fmt("trap -- ");
				print_quoted(trap[signo]);
				out1fmt(" %s\n",
				    (signo) ? sys_signame[signo] : "EXIT");
			}
		return 0;
	}
	ap = argv + 1;

	action = NULL;

	if (strcmp(*ap, "--") == 0)
		if (*++ap == NULL)
			return 0;

	if (signame_to_signum(*ap) == -1) {
		if ((*ap)[0] == '-') {
			if ((*ap)[1] == '\0')
				ap++;
			else if ((*ap)[1] == 'l' && (*ap)[2] == '\0') {
				printsignals();
				return 0;
			}
			else
				error("bad option %s\n", *ap);
		}
		else
			action = *ap++;
	}

	while (*ap) {
		if (is_number(*ap))
			signo = number(*ap);
		else
			signo = signame_to_signum(*ap);

		if (signo < 0 || signo > NSIG)
			error("%s: bad trap", *ap);

		INTOFF;
		if (action)
			action = savestr(action);

		if (trap[signo])
			ckfree(trap[signo]);

		trap[signo] = action;

		if (signo != 0)
			setsignal(signo, 0);
		INTON;
		ap++;
	}
	return 0;
}




void
clear_traps(int vforked)
{
	char **tp;

	for (tp = trap ; tp <= &trap[NSIG] ; tp++) {
		if (*tp && **tp) {	/* trap not NULL or SIG_IGN */
			INTOFF;
			if (!vforked) {
				ckfree(*tp);
				*tp = NULL;
			}
			if (tp != &trap[0])
				setsignal(tp - trap, vforked);
			INTON;
		}
	}
}




long
setsignal(int signo, int vforked)
{
	int action;
	sig_t sigact = SIG_DFL;
	struct sigaction act, oact;
	char *t, tsig;

	if ((t = trap[signo]) == NULL)
		action = S_DFL;
	else if (*t != '\0')
		action = S_CATCH;
	else
		action = S_IGN;
	if (rootshell && !vforked && action == S_DFL) {
		switch (signo) {
		case SIGINT:
			if (iflag || minusc || sflag == 0)
				action = S_CATCH;
			break;
		case SIGQUIT:
#ifdef DEBUG
			if (debug)
				break;
#endif
			/* FALLTHROUGH */
		case SIGTERM:
			if (iflag)
				action = S_IGN;
			break;
#if JOBS
		case SIGTSTP:
		case SIGTTOU:
			if (mflag)
				action = S_IGN;
			break;
#endif
		}
	}

	t = &sigmode[signo - 1];
	tsig = *t;
	if (tsig == 0) {
		/*
		 * current setting unknown
		 */
		if (!getsigaction(signo, &sigact)) {
			/*
			 * Pretend it worked; maybe we should give a warning
			 * here, but other shells don't. We don't alter
			 * sigmode, so that we retry every time.
			 */
			return 0;
		}
		if (sigact == SIG_IGN) {
			if (mflag && (signo == SIGTSTP ||
			     signo == SIGTTIN || signo == SIGTTOU)) {
				tsig = S_IGN;	/* don't hard ignore these */
			} else
				tsig = S_HARD_IGN;
		} else {
			tsig = S_RESET;	/* force to be set */
		}
	}
	if (tsig == S_HARD_IGN || tsig == action)
		return 0;
	switch (action) {
		case S_DFL:	sigact = SIG_DFL;	break;
		case S_CATCH:  	sigact = onsig;		break;
		case S_IGN:	sigact = SIG_IGN;	break;
	}
	if (!vforked)
		*t = action;
    act.sa_handler = sigact;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
#ifdef SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT;
#endif
    if(sigaction(signo, &act, &oact) < 0)
        return (long) SIG_ERR;
    return (long) oact.sa_handler;
}

static int
getsigaction(int signo, sig_t *sigact)
{
	struct sigaction sa;

	if (sigaction(signo, (struct sigaction *)0, &sa) == -1)
		return 0;
	*sigact = (sig_t) sa.sa_handler;
	return 1;
}


void
ignoresig(int signo, int vforked)
{
	if (sigmode[signo - 1] != S_IGN && sigmode[signo - 1] != S_HARD_IGN)
		bsd_signal(signo, SIG_IGN);
	if (!vforked)
		sigmode[signo - 1] = S_HARD_IGN;
}


#ifdef mkinit
INCLUDE <signal.h>
INCLUDE "trap.h"

SHELLPROC {
	char *sm;

	clear_traps(0);
	for (sm = sigmode ; sm < sigmode + NSIG ; sm++) {
		if (*sm == S_IGN)
			*sm = S_HARD_IGN;
	}
}
#endif




void
onsig(int signo)
{
	bsd_signal(signo, onsig);
	if (signo == SIGINT && trap[SIGINT] == NULL) {
		onint();
		return;
	}
	gotsig[signo - 1] = 1;
	pendingsigs++;
}




void
dotrap(void)
{
	int i;
	int savestatus;

	for (;;) {
		for (i = 1 ; ; i++) {
			if (gotsig[i - 1])
				break;
			if (i >= NSIG)
				goto done;
		}
		gotsig[i - 1] = 0;
		savestatus=exitstatus;
		evalstring(trap[i], 0);
		exitstatus=savestatus;
	}
done:
	pendingsigs = 0;
}





void
setinteractive(int on)
{
	static int is_interactive;

	if (on == is_interactive)
		return;
	setsignal(SIGINT, 0);
	setsignal(SIGQUIT, 0);
	setsignal(SIGTERM, 0);
	is_interactive = on;
}




void
exitshell(int status)
{
	struct jmploc loc1, loc2;
	char *p;

	TRACE(("pid %d, exitshell(%d)\n", getpid(), status));
	if (setjmp(loc1.loc)) {
		goto l1;
	}
	if (setjmp(loc2.loc)) {
		goto l2;
	}
	handler = &loc1;
	if ((p = trap[0]) != NULL && *p != '\0') {
		trap[0] = NULL;
		evalstring(p, 0);
	}
l1:   handler = &loc2;			/* probably unnecessary */
	flushall();
#if JOBS
	setjobctl(0);
#endif
l2:   _exit(status);
	/* NOTREACHED */
}
