
/*	$NetBSD: show.h,v 1.7 2003/08/07 09:05:38 agc Exp $	*/


#include <stdarg.h>

union node;
void showtree(union node *);
void trace(const char *, ...);
void tracev(const char *, va_list);
void trargs(char **);
#ifdef DEBUG
void trputc(int);
void trputs(const char *);
void opentrace(void);
#endif
