
/*	$NetBSD: redir.c,v 1.29 2004/07/08 03:57:33 christos Exp $	*/


#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)redir.c	8.2 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: redir.c,v 1.29 2004/07/08 03:57:33 christos Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/param.h>	/* PIPE_BUF */
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>


#include "main.h"
#include "shell.h"
#include "nodes.h"
#include "jobs.h"
#include "options.h"
#include "expand.h"
#include "redir.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"


#define EMPTY -2		/* marks an unused slot in redirtab */
#ifndef PIPE_BUF
# define PIPESIZE 4096		/* amount of buffering in a pipe */
#else
# define PIPESIZE PIPE_BUF
#endif

#define signal bsd_signal

MKINIT
struct redirtab {
	struct redirtab *next;
	short renamed[10];
};


MKINIT struct redirtab *redirlist;

int fd0_redirected = 0;

STATIC void openredirect(union node *, char[10], int);
STATIC int openhere(union node *);



void
redirect(union node *redir, int flags)
{
	union node *n;
	struct redirtab *sv = NULL;
	int i;
	int fd;
	int try;
	char memory[10];	/* file descriptors to write to memory */

	for (i = 10 ; --i >= 0 ; )
		memory[i] = 0;
	memory[1] = flags & REDIR_BACKQ;
	if (flags & REDIR_PUSH) {
		/* We don't have to worry about REDIR_VFORK here, as
		 * flags & REDIR_PUSH is never true if REDIR_VFORK is set.
		 */
		sv = ckmalloc(sizeof (struct redirtab));
		for (i = 0 ; i < 10 ; i++)
			sv->renamed[i] = EMPTY;
		sv->next = redirlist;
		redirlist = sv;
	}
	for (n = redir ; n ; n = n->nfile.next) {
		fd = n->nfile.fd;
		try = 0;
		if ((n->nfile.type == NTOFD || n->nfile.type == NFROMFD) &&
		    n->ndup.dupfd == fd)
			continue; /* redirect from/to same file descriptor */

		if ((flags & REDIR_PUSH) && sv->renamed[fd] == EMPTY) {
			INTOFF;
again:
			if ((i = fcntl(fd, F_DUPFD, 10)) == -1) {
				switch (errno) {
				case EBADF:
					if (!try) {
						openredirect(n, memory, flags);
						try++;
						goto again;
					}
					/* FALLTHROUGH*/
				default:
					INTON;
					error("%d: %s", fd, strerror(errno));
					/* NOTREACHED */
				}
			}
			if (!try) {
				sv->renamed[fd] = i;
				close(fd);
			}
			INTON;
		} else {
			close(fd);
		}
                if (fd == 0)
                        fd0_redirected++;
		if (!try)
			openredirect(n, memory, flags);
	}
	if (memory[1])
		out1 = &memout;
	if (memory[2])
		out2 = &memout;
}


STATIC void
openredirect(union node *redir, char memory[10], int flags)
{
	int fd = redir->nfile.fd;
	char *fname;
	int f;
	int oflags = O_WRONLY|O_CREAT|O_TRUNC, eflags;

	/*
	 * We suppress interrupts so that we won't leave open file
	 * descriptors around.  This may not be such a good idea because
	 * an open of a device or a fifo can block indefinitely.
	 */
	INTOFF;
	memory[fd] = 0;
	switch (redir->nfile.type) {
	case NFROM:
		fname = redir->nfile.expfname;
		if (flags & REDIR_VFORK)
			eflags = O_NONBLOCK;
		else
			eflags = 0;
		if ((f = open(fname, O_RDONLY|eflags)) < 0)
			goto eopen;
		if (eflags)
			(void)fcntl(f, F_SETFL, fcntl(f, F_GETFL, 0) & ~eflags);
		break;
	case NFROMTO:
		fname = redir->nfile.expfname;
		if ((f = open(fname, O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0)
			goto ecreate;
		break;
	case NTO:
		if (Cflag)
			oflags |= O_EXCL;
		/* FALLTHROUGH */
	case NCLOBBER:
		fname = redir->nfile.expfname;
		if ((f = open(fname, oflags, 0666)) < 0)
			goto ecreate;
		break;
	case NAPPEND:
		fname = redir->nfile.expfname;
		if ((f = open(fname, O_WRONLY|O_CREAT|O_APPEND, 0666)) < 0)
			goto ecreate;
		break;
	case NTOFD:
	case NFROMFD:
		if (redir->ndup.dupfd >= 0) {	/* if not ">&-" */
			if (memory[redir->ndup.dupfd])
				memory[fd] = 1;
			else
				copyfd(redir->ndup.dupfd, fd);
		}
		INTON;
		return;
	case NHERE:
	case NXHERE:
		f = openhere(redir);
		break;
	default:
		abort();
	}

	if (f != fd) {
		copyfd(f, fd);
		close(f);
	}
	INTON;
	return;
ecreate:
	error("cannot create %s: %s", fname, errmsg(errno, E_CREAT));
eopen:
	error("cannot open %s: %s", fname, errmsg(errno, E_OPEN));
}



STATIC int
openhere(union node *redir)
{
	int pip[2];
	int len = 0;

	if (pipe(pip) < 0)
		error("Pipe call failed");
	if (redir->type == NHERE) {
		len = strlen(redir->nhere.doc->narg.text);
		if (len <= PIPESIZE) {
			xwrite(pip[1], redir->nhere.doc->narg.text, len);
			goto out;
		}
	}
	if (forkshell((struct job *)NULL, (union node *)NULL, FORK_NOJOB) == 0) {
		close(pip[0]);
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGHUP, SIG_IGN);
#ifdef SIGTSTP
		signal(SIGTSTP, SIG_IGN);
#endif
		signal(SIGPIPE, SIG_DFL);
		if (redir->type == NHERE)
			xwrite(pip[1], redir->nhere.doc->narg.text, len);
		else
			expandhere(redir->nhere.doc, pip[1]);
		_exit(0);
	}
out:
	close(pip[1]);
	return pip[0];
}




void
popredir(void)
{
	struct redirtab *rp = redirlist;
	int i;

	for (i = 0 ; i < 10 ; i++) {
		if (rp->renamed[i] != EMPTY) {
                        if (i == 0)
                                fd0_redirected--;
			close(i);
			if (rp->renamed[i] >= 0) {
				copyfd(rp->renamed[i], i);
				close(rp->renamed[i]);
			}
		}
	}
	INTOFF;
	redirlist = rp->next;
	ckfree(rp);
	INTON;
}


#ifdef mkinit

INCLUDE "redir.h"

RESET {
	while (redirlist)
		popredir();
}

SHELLPROC {
	clearredir(0);
}

#endif

/* Return true if fd 0 has already been redirected at least once.  */
int
fd0_redirected_p () {
        return fd0_redirected != 0;
}


void
clearredir(vforked)
	int vforked;
{
	struct redirtab *rp;
	int i;

	for (rp = redirlist ; rp ; rp = rp->next) {
		for (i = 0 ; i < 10 ; i++) {
			if (rp->renamed[i] >= 0) {
				close(rp->renamed[i]);
			}
			if (!vforked)
				rp->renamed[i] = EMPTY;
		}
	}
}




int
copyfd(int from, int to)
{
	int newfd;

	newfd = fcntl(from, F_DUPFD, to);
	if (newfd < 0) {
		if (errno == EMFILE)
			return EMPTY;
		else
			error("%d: %s", from, strerror(errno));
	}
	return newfd;
}
