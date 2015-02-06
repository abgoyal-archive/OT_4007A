
/*	$NetBSD: expand.h,v 1.16 2004/07/13 15:05:59 seb Exp $	*/


struct strlist {
	struct strlist *next;
	char *text;
};


struct arglist {
	struct strlist *list;
	struct strlist **lastp;
};

#define EXP_FULL	0x1	/* perform word splitting & file globbing */
#define EXP_TILDE	0x2	/* do normal tilde expansion */
#define	EXP_VARTILDE	0x4	/* expand tildes in an assignment */
#define	EXP_REDIR	0x8	/* file glob for a redirection (1 match only) */
#define EXP_CASE	0x10	/* keeps quotes around for CASE pattern */
#define EXP_IFS_SPLIT	0x20	/* need to record arguments for ifs breakup */


union node;
void expandhere(union node *, int);
void expandarg(union node *, struct arglist *, int);
void expari(int);
int patmatch(char *, char *, int);
void rmescapes(char *);
int casematch(union node *, char *);
int wordexpcmd(int, char **);

/* From arith.y */
int arith(const char *);
int expcmd(int , char **);
void arith_lex_reset(void);
int yylex(void);
