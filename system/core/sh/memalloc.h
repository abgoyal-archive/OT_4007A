
/*	$NetBSD: memalloc.h,v 1.14 2003/08/07 09:05:34 agc Exp $	*/


struct stackmark {
	struct stack_block *stackp;
	char *stacknxt;
	int stacknleft;
	struct stackmark *marknext;
};


extern char *stacknxt;
extern int stacknleft;
extern int sstrnleft;
extern int herefd;

pointer ckmalloc(int);
pointer ckrealloc(pointer, int);
char *savestr(const char *);
pointer stalloc(int);
void stunalloc(pointer);
void setstackmark(struct stackmark *);
void popstackmark(struct stackmark *);
void growstackblock(void);
void grabstackblock(int);
char *growstackstr(void);
char *makestrspace(void);
void ungrabstackstr(char *, char *);



#define stackblock() stacknxt
#define stackblocksize() stacknleft
#define STARTSTACKSTR(p)	p = stackblock(), sstrnleft = stackblocksize()
#define STPUTC(c, p)	(--sstrnleft >= 0? (*p++ = (c)) : (p = growstackstr(), *p++ = (c)))
#define CHECKSTRSPACE(n, p)	{ if (sstrnleft < n) p = makestrspace(); }
#define USTPUTC(c, p)	(--sstrnleft, *p++ = (c))
#define STACKSTRNUL(p)	(sstrnleft == 0? (p = growstackstr(), *p = '\0') : (*p = '\0'))
#define STUNPUTC(p)	(++sstrnleft, --p)
#define STTOPC(p)	p[-1]
#define STADJUST(amount, p)	(p += (amount), sstrnleft -= (amount))
#define grabstackstr(p)	stalloc(stackblocksize() - sstrnleft)

#define ckfree(p)	free((pointer)(p))
