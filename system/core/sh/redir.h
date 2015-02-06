
/*	$NetBSD: redir.h,v 1.15 2003/08/07 09:05:37 agc Exp $	*/


/* flags passed to redirect */
#define REDIR_PUSH 01		/* save previous values of file descriptors */
#define REDIR_BACKQ 02		/* save the command output in memory */
#define REDIR_VFORK 04		/* running under vfork(2), be careful */

union node;
void redirect(union node *, int);
void popredir(void);
int fd0_redirected_p(void);
void clearredir(int);
int copyfd(int, int);

