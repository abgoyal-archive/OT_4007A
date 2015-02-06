
/*	$NetBSD: jobs.h,v 1.19 2003/11/27 21:16:14 dsl Exp $	*/


#include "output.h"

/* Mode argument to forkshell.  Don't change FORK_FG or FORK_BG. */
#define FORK_FG 0
#define FORK_BG 1
#define FORK_NOJOB 2

/* mode flags for showjob(s) */
#define	SHOW_PGID	0x01	/* only show pgid - for jobs -p */
#define	SHOW_MULTILINE	0x02	/* one line per process */
#define	SHOW_PID	0x04	/* include process pid */
#define	SHOW_CHANGED	0x08	/* only jobs whose state has changed */
#define	SHOW_SIGNALLED	0x10	/* only if stopped/exited on signal */
#define	SHOW_ISSIG	0x20	/* job was signalled */
#define	SHOW_NO_FREE	0x40	/* do not free job */


#define	MAXCMDTEXT	200

struct procstat {
	pid_t	pid;		/* process id */
 	int	status;		/* last process status from wait() */
 	char	cmd[MAXCMDTEXT];/* text of command being run */
};

struct job {
	struct procstat ps0;	/* status of process */
	struct procstat *ps;	/* status or processes when more than one */
	int	nprocs;		/* number of processes */
	pid_t	pgrp;		/* process group of this job */
	char	state;
#define	JOBRUNNING	0	/* at least one proc running */
#define	JOBSTOPPED	1	/* all procs are stopped */
#define	JOBDONE		2	/* all procs are completed */
	char	used;		/* true if this entry is in used */
	char	changed;	/* true if status has changed */
#if JOBS
	char 	jobctl;		/* job running under job control */
	int	prev_job;	/* previous job index */
#endif
};

extern pid_t backgndpid;	/* pid of last background process */
extern int job_warning;		/* user was warned about stopped jobs */

void setjobctl(int);
int fgcmd(int, char **);
int bgcmd(int, char **);
int jobscmd(int, char **);
void showjobs(struct output *, int);
int waitcmd(int, char **);
int jobidcmd(int, char **);
struct job *makejob(union node *, int);
int forkshell(struct job *, union node *, int);
void forkchild(struct job *, union node *, int, int);
int forkparent(struct job *, union node *, int, pid_t);
int waitforjob(struct job *);
int stoppedjobs(void);
void commandtext(struct procstat *, union node *);
int getjobpgrp(const char *);

#if ! JOBS
#define setjobctl(on)	/* do nothing */
#endif
