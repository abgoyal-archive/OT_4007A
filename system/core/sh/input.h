
/*	$NetBSD: input.h,v 1.15 2003/08/07 09:05:33 agc Exp $	*/


/* PEOF (the end of file marker) is defined in syntax.h */

extern int plinno;
extern int parsenleft;		/* number of characters left in input buffer */
extern char *parsenextc;	/* next character in input buffer */
extern int init_editline;	/* 0 == not setup, 1 == OK, -1 == failed */

int in_interactive_mode();
char *pfgets(char *, int);
int pgetc(void);
int preadbuffer(void);
void pungetc(void);
void pushstring(char *, int, void *);
void popstring(void);
void setinputfile(const char *, int);
void setinputfd(int, int);
void setinputstring(char *, int);
void popfile(void);
void popallfiles(void);
void closescript(int);

#define pgetc_macro()	(--parsenleft >= 0? *parsenextc++ : preadbuffer())
