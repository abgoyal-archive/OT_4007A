
/*	$NetBSD: parser.h,v 1.17 2004/06/26 22:09:49 dsl Exp $	*/


/* control characters in argument strings */
#define CTL_FIRST '\201'	/* first 'special' character */
#define CTLESC '\201'		/* escape next character */
#define CTLVAR '\202'		/* variable defn */
#define CTLENDVAR '\203'
#define CTLBACKQ '\204'
#define CTLQUOTE 01		/* ored with CTLBACKQ code if in quotes */
/*	CTLBACKQ | CTLQUOTE == '\205' */
#define	CTLARI	'\206'		/* arithmetic expression */
#define	CTLENDARI '\207'
#define	CTLQUOTEMARK '\210'
#define	CTLQUOTEEND '\211'	/* only inside ${...} */
#define	CTL_LAST '\211'		/* last 'special' character */

/* variable substitution byte (follows CTLVAR) */
#define VSTYPE	0x0f		/* type of variable substitution */
#define VSNUL	0x10		/* colon--treat the empty string as unset */
#define VSQUOTE 0x80		/* inside double quotes--suppress splitting */

/* values of VSTYPE field */
#define VSNORMAL	0x1		/* normal variable:  $var or ${var} */
#define VSMINUS		0x2		/* ${var-text} */
#define VSPLUS		0x3		/* ${var+text} */
#define VSQUESTION	0x4		/* ${var?message} */
#define VSASSIGN	0x5		/* ${var=text} */
#define VSTRIMLEFT	0x6		/* ${var#pattern} */
#define VSTRIMLEFTMAX	0x7		/* ${var##pattern} */
#define VSTRIMRIGHT	0x8		/* ${var%pattern} */
#define VSTRIMRIGHTMAX 	0x9		/* ${var%%pattern} */
#define VSLENGTH	0xa		/* ${#var} */


extern int tokpushback;
#define NEOF ((union node *)&tokpushback)
extern int whichprompt;		/* 1 == PS1, 2 == PS2 */


union node *parsecmd(int);
void fixredir(union node *, const char *, int);
int goodname(char *);
const char *getprompt(void *);
