
/*	$NetBSD: options.c,v 1.37 2004/10/30 19:29:27 christos Exp $	*/


#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)options.c	8.2 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: options.c,v 1.37 2004/10/30 19:29:27 christos Exp $");
#endif
#endif /* not lint */

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include "shell.h"
#define DEFINE_OPTIONS
#include "options.h"
#undef DEFINE_OPTIONS
#include "nodes.h"	/* for other header files */
#include "eval.h"
#include "jobs.h"
#include "input.h"
#include "output.h"
#include "trap.h"
#include "var.h"
#include "memalloc.h"
#include "error.h"
#include "mystring.h"
#ifndef SMALL
#include "myhistedit.h"
#endif
#include "show.h"

char *arg0;			/* value of $0 */
struct shparam shellparam;	/* current positional parameters */
char **argptr;			/* argument list for builtin commands */
char *optionarg;		/* set by nextopt (like getopt) */
char *optptr;			/* used by nextopt */

char *minusc;			/* argument to -c option */


STATIC void options(int);
STATIC void minus_o(char *, int);
STATIC void setoption(int, int);
STATIC int getopts(char *, char *, char **, char ***, char **);



void
procargs(int argc, char **argv)
{
	int i;

	argptr = argv;
	if (argc > 0)
		argptr++;
	for (i = 0; i < NOPTS; i++)
		optlist[i].val = 2;
	options(1);
	if (*argptr == NULL && minusc == NULL)
		sflag = 1;
	if (iflag == 2 && sflag == 1 && isatty(0) && isatty(1))
		iflag = 1;
	if (mflag == 2)
		mflag = iflag;
	for (i = 0; i < NOPTS; i++)
		if (optlist[i].val == 2)
			optlist[i].val = 0;
#if DEBUG == 2
	debug = 1;
#endif
	arg0 = argv[0];
	if (sflag == 0 && minusc == NULL) {
		commandname = argv[0];
		arg0 = *argptr++;
		setinputfile(arg0, 0);
		commandname = arg0;
	}
	/* POSIX 1003.2: first arg after -c cmd is $0, remainder $1... */
	if (minusc != NULL) {
		if (argptr == NULL || *argptr == NULL)
			error("Bad -c option");
		minusc = *argptr++;
		if (*argptr != 0)
			arg0 = *argptr++;
	}

	shellparam.p = argptr;
	shellparam.reset = 1;
	/* assert(shellparam.malloc == 0 && shellparam.nparam == 0); */
	while (*argptr) {
		shellparam.nparam++;
		argptr++;
	}
	optschanged();
}


void
optschanged(void)
{
	setinteractive(iflag);
#ifdef WITH_HISTORY
	histedit();
#endif
	setjobctl(mflag);
}


STATIC void
options(int cmdline)
{
	static char empty[] = "";
	char *p;
	int val;
	int c;

	if (cmdline)
		minusc = NULL;
	while ((p = *argptr) != NULL) {
		argptr++;
		if ((c = *p++) == '-') {
			val = 1;
                        if (p[0] == '\0' || (p[0] == '-' && p[1] == '\0')) {
                                if (!cmdline) {
                                        /* "-" means turn off -x and -v */
                                        if (p[0] == '\0')
                                                xflag = vflag = 0;
                                        /* "--" means reset params */
                                        else if (*argptr == NULL)
						setparam(argptr);
                                }
				break;	  /* "-" or  "--" terminates options */
			}
		} else if (c == '+') {
			val = 0;
		} else {
			argptr--;
			break;
		}
		while ((c = *p++) != '\0') {
			if (c == 'c' && cmdline) {
				/* command is after shell args*/
				minusc = empty;
			} else if (c == 'o') {
				minus_o(*argptr, val);
				if (*argptr)
					argptr++;
			} else {
				setoption(c, val);
			}
		}
	}
}

static void
set_opt_val(int i, int val)
{
	int j;
	int flag;

	if (val && (flag = optlist[i].opt_set)) {
		/* some options (eg vi/emacs) are mutually exclusive */
		for (j = 0; j < NOPTS; j++)
		    if (optlist[j].opt_set == flag)
			optlist[j].val = 0;
	}
	optlist[i].val = val;
#ifdef DEBUG
	if (&optlist[i].val == &debug)
		opentrace();
#endif
}

STATIC void
minus_o(char *name, int val)
{
	int i;

	if (name == NULL) {
		out1str("Current option settings\n");
		for (i = 0; i < NOPTS; i++)
			out1fmt("%-16s%s\n", optlist[i].name,
				optlist[i].val ? "on" : "off");
	} else {
		for (i = 0; i < NOPTS; i++)
			if (equal(name, optlist[i].name)) {
				set_opt_val(i, val);
				return;
			}
		error("Illegal option -o %s", name);
	}
}


STATIC void
setoption(int flag, int val)
{
	int i;

	for (i = 0; i < NOPTS; i++)
		if (optlist[i].letter == flag) {
			set_opt_val( i, val );
			return;
		}
	error("Illegal option -%c", flag);
	/* NOTREACHED */
}



#ifdef mkinit
INCLUDE "options.h"

SHELLPROC {
	int i;

	for (i = 0; optlist[i].name; i++)
		optlist[i].val = 0;
	optschanged();

}
#endif



void
setparam(char **argv)
{
	char **newparam;
	char **ap;
	int nparam;

	for (nparam = 0 ; argv[nparam] ; nparam++);
	ap = newparam = ckmalloc((nparam + 1) * sizeof *ap);
	while (*argv) {
		*ap++ = savestr(*argv++);
	}
	*ap = NULL;
	freeparam(&shellparam);
	shellparam.malloc = 1;
	shellparam.nparam = nparam;
	shellparam.p = newparam;
	shellparam.optnext = NULL;
}



void
freeparam(volatile struct shparam *param)
{
	char **ap;

	if (param->malloc) {
		for (ap = param->p ; *ap ; ap++)
			ckfree(*ap);
		ckfree(param->p);
	}
}




int
shiftcmd(int argc, char **argv)
{
	int n;
	char **ap1, **ap2;

	n = 1;
	if (argc > 1)
		n = number(argv[1]);
	if (n > shellparam.nparam)
		error("can't shift that many");
	INTOFF;
	shellparam.nparam -= n;
	for (ap1 = shellparam.p ; --n >= 0 ; ap1++) {
		if (shellparam.malloc)
			ckfree(*ap1);
	}
	ap2 = shellparam.p;
	while ((*ap2++ = *ap1++) != NULL);
	shellparam.optnext = NULL;
	INTON;
	return 0;
}




int
setcmd(int argc, char **argv)
{
	if (argc == 1)
		return showvars(0, 0, 1);
	INTOFF;
	options(0);
	optschanged();
	if (*argptr != NULL) {
		setparam(argptr);
	}
	INTON;
	return 0;
}


void
getoptsreset(value)
	const char *value;
{
	if (number(value) == 1) {
		shellparam.optnext = NULL;
		shellparam.reset = 1;
	}
}


int
getoptscmd(int argc, char **argv)
{
	char **optbase;

	if (argc < 3)
		error("usage: getopts optstring var [arg]");
	else if (argc == 3)
		optbase = shellparam.p;
	else
		optbase = &argv[3];

	if (shellparam.reset == 1) {
		shellparam.optnext = optbase;
		shellparam.optptr = NULL;
		shellparam.reset = 0;
	}

	return getopts(argv[1], argv[2], optbase, &shellparam.optnext,
		       &shellparam.optptr);
}

STATIC int
getopts(char *optstr, char *optvar, char **optfirst, char ***optnext, char **optpptr)
{
	char *p, *q;
	char c = '?';
	int done = 0;
	int ind = 0;
	int err = 0;
	char s[12];

	if ((p = *optpptr) == NULL || *p == '\0') {
		/* Current word is done, advance */
		if (*optnext == NULL)
			return 1;
		p = **optnext;
		if (p == NULL || *p != '-' || *++p == '\0') {
atend:
			ind = *optnext - optfirst + 1;
			*optnext = NULL;
			p = NULL;
			done = 1;
			goto out;
		}
		(*optnext)++;
		if (p[0] == '-' && p[1] == '\0')	/* check for "--" */
			goto atend;
	}

	c = *p++;
	for (q = optstr; *q != c; ) {
		if (*q == '\0') {
			if (optstr[0] == ':') {
				s[0] = c;
				s[1] = '\0';
				err |= setvarsafe("OPTARG", s, 0);
			} else {
				outfmt(&errout, "Illegal option -%c\n", c);
				(void) unsetvar("OPTARG", 0);
			}
			c = '?';
			goto bad;
		}
		if (*++q == ':')
			q++;
	}

	if (*++q == ':') {
		if (*p == '\0' && (p = **optnext) == NULL) {
			if (optstr[0] == ':') {
				s[0] = c;
				s[1] = '\0';
				err |= setvarsafe("OPTARG", s, 0);
				c = ':';
			} else {
				outfmt(&errout, "No arg for -%c option\n", c);
				(void) unsetvar("OPTARG", 0);
				c = '?';
			}
			goto bad;
		}

		if (p == **optnext)
			(*optnext)++;
		err |= setvarsafe("OPTARG", p, 0);
		p = NULL;
	} else
		err |= setvarsafe("OPTARG", "", 0);
	ind = *optnext - optfirst + 1;
	goto out;

bad:
	ind = 1;
	*optnext = NULL;
	p = NULL;
out:
	*optpptr = p;
	fmtstr(s, sizeof(s), "%d", ind);
	err |= setvarsafe("OPTIND", s, VNOFUNC);
	s[0] = c;
	s[1] = '\0';
	err |= setvarsafe(optvar, s, 0);
	if (err) {
		*optnext = NULL;
		*optpptr = NULL;
		flushall();
		exraise(EXERROR);
	}
	return done;
}


int
nextopt(const char *optstring)
{
	char *p;
	const char *q;
	char c;

	if ((p = optptr) == NULL || *p == '\0') {
		p = *argptr;
		if (p == NULL || *p != '-' || *++p == '\0')
			return '\0';
		argptr++;
		if (p[0] == '-' && p[1] == '\0')	/* check for "--" */
			return '\0';
	}
	c = *p++;
	for (q = optstring ; *q != c ; ) {
		if (*q == '\0')
			error("Illegal option -%c", c);
		if (*++q == ':')
			q++;
	}
	if (*++q == ':') {
		if (*p == '\0' && (p = *argptr++) == NULL)
			error("No arg for -%c option", c);
		optionarg = p;
		p = NULL;
	}
	optptr = p;
	return c;
}
