
/*	$NetBSD: trap.h,v 1.17 2003/08/07 09:05:39 agc Exp $	*/


extern int pendingsigs;

int trapcmd(int, char **);
void clear_traps(int);
long setsignal(int, int);
void ignoresig(int, int);
void onsig(int);
void dotrap(void);
void setinteractive(int);
void exitshell(int) __attribute__((__noreturn__));
