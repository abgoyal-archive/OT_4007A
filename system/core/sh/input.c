
/*	$NetBSD: input.c,v 1.39 2003/08/07 09:05:32 agc Exp $	*/


#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)input.c	8.3 (Berkeley) 6/9/95";
#else
__RCSID("$NetBSD: input.c,v 1.39 2003/08/07 09:05:32 agc Exp $");
#endif
#endif /* not lint */

#include <stdio.h>	/* defines BUFSIZ */
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


#include "shell.h"
#include "redir.h"
#include "syntax.h"
#include "input.h"
#include "output.h"
#include "options.h"
#include "memalloc.h"
#include "error.h"
#include "alias.h"
#include "parser.h"
#include "myhistedit.h"

#ifdef WITH_LINENOISE
#include "linenoise.h"
#endif

#define EOF_NLEFT -99		/* value of parsenleft when EOF pushed back */

MKINIT
struct strpush {
	struct strpush *prev;	/* preceding string on stack */
	char *prevstring;
	int prevnleft;
	int prevlleft;
	struct alias *ap;	/* if push was associated with an alias */
};


MKINIT
struct parsefile {
	struct parsefile *prev;	/* preceding file on stack */
	int linno;		/* current line */
	int fd;			/* file descriptor (or -1 if string) */
	int nleft;		/* number of chars left in this line */
	int lleft;		/* number of chars left in this buffer */
	char *nextc;		/* next char in buffer */
	char *buf;		/* input buffer */
	struct strpush *strpush; /* for pushing strings at this level */
	struct strpush basestrpush; /* so pushing one is fast */
};


int plinno = 1;			/* input line number */
int parsenleft;			/* copy of parsefile->nleft */
MKINIT int parselleft;		/* copy of parsefile->lleft */
char *parsenextc;		/* copy of parsefile->nextc */
MKINIT struct parsefile basepf;	/* top level input file */
MKINIT char basebuf[BUFSIZ];	/* buffer for top level input file */
struct parsefile *parsefile = &basepf;	/* current input file */
int init_editline = 0;		/* editline library initialized? */
int whichprompt;		/* 1 == PS1, 2 == PS2 */

#if WITH_HISTORY
EditLine *el;			/* cookie for editline package */
#endif

STATIC void pushfile(void);
static int preadfd(void);

#ifdef mkinit
INCLUDE <stdio.h>
INCLUDE "input.h"
INCLUDE "error.h"

INIT {
	basepf.nextc = basepf.buf = basebuf;
}

RESET {
	if (exception != EXSHELLPROC)
		parselleft = parsenleft = 0;	/* clear input buffer */
	popallfiles();
}

SHELLPROC {
	popallfiles();
}
#endif



char *
pfgets(char *line, int len)
{
	char *p = line;
	int nleft = len;
	int c;

	while (--nleft > 0) {
		c = pgetc_macro();
		if (c == PEOF) {
			if (p == line)
				return NULL;
			break;
		}
		*p++ = c;
		if (c == '\n')
			break;
	}
	*p = '\0';
	return line;
}




int
pgetc(void)
{
	return pgetc_macro();
}

int in_interactive_mode() {
    return parsefile != NULL && parsefile->fd == 0;
}

static int
preadfd(void)
{
	int nr;
	char *buf =  parsefile->buf;
	parsenextc = buf;

retry:
#ifdef WITH_HISTORY
	if (parsefile->fd == 0 && el) {
		static const char *rl_cp;
		static int el_len;

		if (rl_cp == NULL)
			rl_cp = el_gets(el, &el_len);
		if (rl_cp == NULL)
			nr = 0;
		else {
			nr = el_len;
			if (nr > BUFSIZ - 8)
				nr = BUFSIZ - 8;
			memcpy(buf, rl_cp, nr);
			if (nr != el_len) {
				el_len -= nr;
				rl_cp += nr;
			} else
				rl_cp = 0;
		}

	} else
#endif
#ifdef WITH_LINENOISE
    if (parsefile->fd == 0) {
        static char *rl_start;
        static const char *rl_cp;
        static int el_len;

        if (rl_cp == NULL) {
            rl_cp = rl_start = linenoise(getprompt(""));
            if (rl_cp != NULL) {
                el_len = strlen(rl_start);
                if (el_len != 0) {
                    /* Add non-blank lines to history. */
                    linenoiseHistoryAdd(rl_start);
                }
                out2str("\n");
                /* Client expects a newline at end of input, doesn't expect null */
                rl_start[el_len++] = '\n';
            }
        }
        if (rl_cp == NULL)
            nr = 0;
        else {
            nr = el_len;
            if (nr > BUFSIZ - 8)
                nr = BUFSIZ - 8;
            memcpy(buf, rl_cp, nr);
            if (nr != el_len) {
                el_len -= nr;
                rl_cp += nr;
            } else {
                rl_cp = 0;
                if (rl_start != NULL) {
                    free(rl_start);
                    rl_start = NULL;
                }
            }
        }
    } else
#endif
		nr = read(parsefile->fd, buf, BUFSIZ - 8);


	if (nr <= 0) {
                if (nr < 0) {
                        if (errno == EINTR)
                                goto retry;
                        if (parsefile->fd == 0 && errno == EWOULDBLOCK) {
                                int flags = fcntl(0, F_GETFL, 0);
                                if (flags >= 0 && flags & O_NONBLOCK) {
                                        flags &=~ O_NONBLOCK;
                                        if (fcntl(0, F_SETFL, flags) >= 0) {
						out2str("sh: turning off NDELAY mode\n");
                                                goto retry;
                                        }
                                }
                        }
                }
                nr = -1;
	}
	return nr;
}


int
preadbuffer(void)
{
	char *p, *q;
	int more;
	int something;
	char savec;

	if (parsefile->strpush) {
		popstring();
		if (--parsenleft >= 0)
			return (*parsenextc++);
	}
	if (parsenleft == EOF_NLEFT || parsefile->buf == NULL)
		return PEOF;
	flushout(&output);
	flushout(&errout);

again:
	if (parselleft <= 0) {
		if ((parselleft = preadfd()) == -1) {
			parselleft = parsenleft = EOF_NLEFT;
			return PEOF;
		}
	}

	q = p = parsenextc;

	/* delete nul characters */
	something = 0;
	for (more = 1; more;) {
		switch (*p) {
		case '\0':
			p++;	/* Skip nul */
			goto check;

		case '\t':
		case ' ':
			break;

		case '\n':
			parsenleft = q - parsenextc;
			more = 0; /* Stop processing here */
			break;

		default:
			something = 1;
			break;
		}

		*q++ = *p++;
check:
		if (--parselleft <= 0) {
			parsenleft = q - parsenextc - 1;
			if (parsenleft < 0)
				goto again;
			*q = '\0';
			more = 0;
		}
	}

	savec = *q;
	*q = '\0';

#ifdef WITH_HISTORY
	if (parsefile->fd == 0 && hist && something) {
		HistEvent he;
		INTOFF;
		history(hist, &he, whichprompt == 1? H_ENTER : H_APPEND,
		    parsenextc);
		INTON;
	}
#endif

	if (vflag) {
		out2str(parsenextc);
		flushout(out2);
	}

	*q = savec;

	return *parsenextc++;
}


void
pungetc(void)
{
	parsenleft++;
	parsenextc--;
}

void
pushstring(char *s, int len, void *ap)
{
	struct strpush *sp;

	INTOFF;
/*dprintf("*** calling pushstring: %s, %d\n", s, len);*/
	if (parsefile->strpush) {
		sp = ckmalloc(sizeof (struct strpush));
		sp->prev = parsefile->strpush;
		parsefile->strpush = sp;
	} else
		sp = parsefile->strpush = &(parsefile->basestrpush);
	sp->prevstring = parsenextc;
	sp->prevnleft = parsenleft;
	sp->prevlleft = parselleft;
	sp->ap = (struct alias *)ap;
	if (ap)
		((struct alias *)ap)->flag |= ALIASINUSE;
	parsenextc = s;
	parsenleft = len;
	INTON;
}

void
popstring(void)
{
	struct strpush *sp = parsefile->strpush;

	INTOFF;
	parsenextc = sp->prevstring;
	parsenleft = sp->prevnleft;
	parselleft = sp->prevlleft;
/*dprintf("*** calling popstring: restoring to '%s'\n", parsenextc);*/
	if (sp->ap)
		sp->ap->flag &= ~ALIASINUSE;
	parsefile->strpush = sp->prev;
	if (sp != &(parsefile->basestrpush))
		ckfree(sp);
	INTON;
}


void
setinputfile(const char *fname, int push)
{
	int fd;
	int fd2;

	INTOFF;
	if ((fd = open(fname, O_RDONLY)) < 0)
		error("Can't open %s", fname);
	if (fd < 10) {
		fd2 = copyfd(fd, 10);
		close(fd);
		if (fd2 < 0)
			error("Out of file descriptors");
		fd = fd2;
	}
	setinputfd(fd, push);
	INTON;
}



void
setinputfd(int fd, int push)
{
	(void) fcntl(fd, F_SETFD, FD_CLOEXEC);
	if (push) {
		pushfile();
		parsefile->buf = ckmalloc(BUFSIZ);
	}
	if (parsefile->fd > 0)
		close(parsefile->fd);
	parsefile->fd = fd;
	if (parsefile->buf == NULL)
		parsefile->buf = ckmalloc(BUFSIZ);
	parselleft = parsenleft = 0;
	plinno = 1;
}



void
setinputstring(char *string, int push)
{
	INTOFF;
	if (push)
		pushfile();
	parsenextc = string;
	parselleft = parsenleft = strlen(string);
	parsefile->buf = NULL;
	plinno = 1;
	INTON;
}




STATIC void
pushfile(void)
{
	struct parsefile *pf;

	parsefile->nleft = parsenleft;
	parsefile->lleft = parselleft;
	parsefile->nextc = parsenextc;
	parsefile->linno = plinno;
	pf = (struct parsefile *)ckmalloc(sizeof (struct parsefile));
	pf->prev = parsefile;
	pf->fd = -1;
	pf->strpush = NULL;
	pf->basestrpush.prev = NULL;
	parsefile = pf;
}


void
popfile(void)
{
	struct parsefile *pf = parsefile;

	INTOFF;
	if (pf->fd >= 0)
		close(pf->fd);
	if (pf->buf)
		ckfree(pf->buf);
	while (pf->strpush)
		popstring();
	parsefile = pf->prev;
	ckfree(pf);
	parsenleft = parsefile->nleft;
	parselleft = parsefile->lleft;
	parsenextc = parsefile->nextc;
	plinno = parsefile->linno;
	INTON;
}



void
popallfiles(void)
{
	while (parsefile != &basepf)
		popfile();
}




void
closescript(int vforked)
{
	if (vforked)
		return;
	popallfiles();
	if (parsefile->fd > 0) {
		close(parsefile->fd);
		parsefile->fd = 0;
	}
}
