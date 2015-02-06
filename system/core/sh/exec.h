
/*	$NetBSD: exec.h,v 1.21 2003/08/07 09:05:31 agc Exp $	*/


/* values of cmdtype */
#define CMDUNKNOWN	-1	/* no entry in table for command */
#define CMDNORMAL	0	/* command is an executable program */
#define CMDFUNCTION	1	/* command is a shell function */
#define CMDBUILTIN	2	/* command is a shell builtin */
#define CMDSPLBLTIN	3	/* command is a special shell builtin */


struct cmdentry {
	int cmdtype;
	union param {
		int index;
		int (*bltin)(int, char**);
		union node *func;
	} u;
};


/* action to find_command() */
#define DO_ERR		0x01	/* prints errors */
#define DO_ABS		0x02	/* checks absolute paths */
#define DO_NOFUNC	0x04	/* don't return shell functions, for command */
#define DO_ALTPATH	0x08	/* using alternate path */
#define DO_ALTBLTIN	0x20	/* %builtin in alt. path */

extern const char *pathopt;	/* set by padvance */

void shellexec(char **, char **, const char *, int, int)
    __attribute__((__noreturn__));
char *padvance(const char **, const char *);
int hashcmd(int, char **);
void find_command(char *, struct cmdentry *, int, const char *);
int (*find_builtin(char *))(int, char **);
int (*find_splbltin(char *))(int, char **);
void hashcd(void);
void changepath(const char *);
void deletefuncs(void);
void getcmdentry(char *, struct cmdentry *);
void addcmdentry(char *, struct cmdentry *);
void defun(char *, union node *);
int unsetfunc(char *);
int typecmd(int, char **);
void hash_special_builtins(void);
