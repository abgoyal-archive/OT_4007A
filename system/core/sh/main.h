
/*	$NetBSD: main.h,v 1.10 2003/08/07 09:05:34 agc Exp $	*/


extern int rootpid;	/* pid of main shell */
extern int rootshell;	/* true if we aren't a child of the main shell */

void readcmdfile(char *);
void cmdloop(int);
int dotcmd(int, char **);
int exitcmd(int, char **);
