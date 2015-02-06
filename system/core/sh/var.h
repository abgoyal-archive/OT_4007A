
/*	$NetBSD: var.h,v 1.23 2004/10/02 12:16:53 dsl Exp $	*/



/* flags */
#define VEXPORT		0x01	/* variable is exported */
#define VREADONLY	0x02	/* variable cannot be modified */
#define VSTRFIXED	0x04	/* variable struct is statically allocated */
#define VTEXTFIXED	0x08	/* text is statically allocated */
#define VSTACK		0x10	/* text is allocated on the stack */
#define VUNSET		0x20	/* the variable is not set */
#define VNOFUNC		0x40	/* don't call the callback function */
#define VNOSET		0x80	/* do not set variable - just readonly test */


struct var {
	struct var *next;		/* next entry in hash list */
	int flags;			/* flags are defined above */
	char *text;			/* name=value */
	int name_len;			/* length of name */
	void (*func)(const char *);
					/* function to be called when  */
					/* the variable gets set/unset */
};


struct localvar {
	struct localvar *next;		/* next local variable in list */
	struct var *vp;			/* the variable that was made local */
	int flags;			/* saved flags */
	char *text;			/* saved text */
};


struct localvar *localvars;

#if ATTY
extern struct var vatty;
#endif
extern struct var vifs;
extern struct var vmpath;
extern struct var vpath;
extern struct var vps1;
extern struct var vps2;
extern struct var vps4;
#ifdef WITH_HISTORY 
extern struct var vterm;
extern struct var vtermcap;
extern struct var vhistsize;
#endif


#define ifsval()	(vifs.text + 4)
#define ifsset()	((vifs.flags & VUNSET) == 0)
#define mpathval()	(vmpath.text + 9)
#define pathval()	(vpath.text + 5)
#define ps1val()	(vps1.text + 4)
#define ps2val()	(vps2.text + 4)
#define ps4val()	(vps4.text + 4)
#define optindval()	(voptind.text + 7)
#ifdef WITH_HISTORY
#define histsizeval()	(vhistsize.text + 9)
#define termval()	(vterm.text + 5)
#endif

#if ATTY
#define attyset()	((vatty.flags & VUNSET) == 0)
#endif
#define mpathset()	((vmpath.flags & VUNSET) == 0)

void initvar(void);
void setvar(const char *, const char *, int);
void setvareq(char *, int);
struct strlist;
void listsetvar(struct strlist *, int);
char *lookupvar(const char *);
char *bltinlookup(const char *, int);
char **environment(void);
void shprocvar(void);
int showvars(const char *, int, int);
int exportcmd(int, char **);
int localcmd(int, char **);
void mklocal(const char *, int);
void listmklocal(struct strlist *, int);
void poplocalvars(void);
int setvarcmd(int, char **);
int unsetcmd(int, char **);
int unsetvar(const char *, int);
int setvarsafe(const char *, const char *, int);
void print_quoted(const char *);
