
/*	$NetBSD: myhistedit.h,v 1.10 2003/08/07 09:05:35 agc Exp $	*/


#ifdef WITH_HISTORY
#include <histedit.h>

extern History *hist;
extern EditLine *el;
extern int displayhist;

void histedit(void);
void sethistsize(const char *);
void setterm(const char *);
int histcmd(int, char **);
int inputrc(int, char **);
int not_fcnumber(char *);
int str_to_event(const char *, int);
#endif

