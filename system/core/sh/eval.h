
/*	$NetBSD: eval.h,v 1.14 2003/08/07 09:05:31 agc Exp $	*/


extern char *commandname;	/* currently executing command */
extern int exitstatus;		/* exit status of last command */
extern int back_exitstatus;	/* exit status of backquoted command */
extern struct strlist *cmdenviron;  /* environment for builtin command */


struct backcmd {		/* result of evalbackcmd */
	int fd;			/* file descriptor to read from */
	char *buf;		/* buffer */
	int nleft;		/* number of chars in buffer */
	struct job *jp;		/* job structure for command */
};

void evalstring(char *, int);
union node;	/* BLETCH for ansi C */
void evaltree(union node *, int);
void evalbackcmd(union node *, struct backcmd *);

/* in_function returns nonzero if we are currently evaluating a function */
#define in_function()	funcnest
extern int funcnest;
extern int evalskip;

/* reasons for skipping commands (see comment on breakcmd routine) */
#define SKIPBREAK	1
#define SKIPCONT	2
#define SKIPFUNC	3
#define SKIPFILE	4
