
/*	$NetBSD: bltin.h,v 1.11 2003/08/07 09:05:40 agc Exp $	*/



#include "../shell.h"
#include "../mystring.h"
#ifdef SHELL
#include "../output.h"
#include "../error.h"
#undef stdout
#undef stderr
#undef putc
#undef putchar
#undef fileno
#define stdout out1
#define stderr out2
#define printf out1fmt
#define putc(c, file)	outc(c, file)
#define putchar(c)	out1c(c)
#define FILE struct output
#define fprintf outfmt
#define fputs outstr
#define fflush flushout
#define fileno(f) ((f)->fd)
#define INITARGS(argv)
#define	err sh_err
#define	verr sh_verr
#define	errx sh_errx
#define	verrx sh_verrx
#define	warn sh_warn
#define	vwarn sh_vwarn
#define	warnx sh_warnx
#define	vwarnx sh_vwarnx
#define exit sh_exit
#define setprogname(s)
#define getprogname() commandname
#define setlocate(l,s) 0

#define getenv(p) bltinlookup((p),0)

#else
#undef NULL
#include <stdio.h>
#undef main
#define INITARGS(argv)	if ((commandname = argv[0]) == NULL) {fputs("Argc is zero\n", stderr); exit(2);} else
#endif

pointer stalloc(int);
void error(const char *, ...);
void sh_warnx(const char *, ...);
void sh_exit(int) __attribute__((__noreturn__));

int echocmd(int, char **);


extern const char *commandname;
